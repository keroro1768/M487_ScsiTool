# T033 - GDB RSP Server 研究

**狀態:** ⏳ Research
**起始:** 2026-03-27
**優先:** P2
**負責:** 🐱 Giroro

## 目標

透過 USB MSC Debug Channel 接受 GDB 命令，實現 self-hosted debugging。

## GDB RSP 概述

GDB Remote Serial Protocol (RSP) 是一種基於 ASCII 的調試協議，常用於嵌入式系統的遠程調試。

### 通信方式

1. **GDB → Target:** 發送 ASCII 命令（如 `g` 讀取寄存器，`m` 讀取內存）
2. **Target → GDB:** 返回 ASCII 響應（如寄存器值、內存內容）
3. **傳輸層:** 可透過 UART、USB MSC Debug Channel、或自定義通道

### 常見命令

| 命令 | 說明 | 響應格式 |
|------|------|---------|
| `g` | 讀取所有寄存器 | `XXXXXXXX...` (hex) |
| `G XX...` | 寫入所有寄存器 | `OK` 或 `EXX` |
| `m ADDR,LENGTH` | 讀取內存 | `XX...` (hex) |
| `M ADDR,LENGTH:XX...` | 寫入內存 | `OK` 或 `EXX` |
| `c` | 繼續執行 | `OK` 或 signal |
| `s` | 單步執行 | `OK` 或 signal |
| `z TYPE,ADDR,LENGTH` | 清除斷點 | `OK` 或 `EXX` |
| `Z TYPE,ADDR,LENGTH` | 設置斷點 | `OK` 或 `EXX` |
| `?` | 查詢當前 signal | `SXX` |
| `p N` | 讀取第 N 個寄存器 | `XXXXXXXX` |
| `P N=XXXXXXXX` | 寫入第 N 個寄存器 | `OK` 或 `EXX` |
| `qXXX` | 查詢擴展命令 | 視命令而定 |
| `QXXX` | 設置擴展命令 | 視命令而定 |
| `D` | 斷開連接 | `OK` |

### 斷點類型 (TYPE)

| TYPE | 說明 |
|------|------|
| 0 | 軟件斷點 (0xBE) |
| 1 | 硬件斷點 |
| 2 | 寫入 watchpoint |
| 3 | 讀取 watchpoint |
| 4 | 訪問 watchpoint |

## 實現方案

### 1. 協議解析層

需要實現一個 RSP 命令解析器：
- 監聽 MSC Debug Channel 的字節流
- 解析 ASCII 命令
- 返回 ASCII 響應

### 2. 核心命令實作

**必備命令:**
- `g` / `G` - 寄存器讀寫
- `m` / `M` - 內存讀寫
- `c` / `s` - 繼續/單步
- `?` - 當前狀態
- `Z` / `z` - 設置/清除斷點

**可選命令:**
- `qSupported` - GDB 功能查詢
- `qC` - 當前線程
- `qAttached` - 進程/線程附著查詢

### 3. 斷點機制

**軟件斷點:**
- 在內存中用 `0xBE` (BKPT) 指令替換原指令
- 需要保存原指令以便恢復

**硬件斷點 (DWT):**
- 使用 ARM CoreSight DWT 單元
- M487 有 6 個硬件斷點
- 不需要修改內存

### 4. 存儲佈局

```
MSC Debug Channel Response Buffer (64 bytes):
+0x00: Command status (1 byte)
+0x01: RSP response data (63 bytes max)
```

## 限制與已知問題

### 自-debug 限制

```
問題: 無法在 MCU halt 自己之前先 halt 自己

原因: 
- MCU 正常運行時無法響應 GDB 命令
- 需要外部條件觸發 halt (如硬件斷點、異常)

適用場景:
1. 已知錯誤發生點，手動暂停後調查
2. 硬件斷點觸發後進入調試模式
3. 異常發生後保持 halt 狀態
```

### 工作流程

1. **正常模式:** MCU 運行用戶代碼
2. **觸發條件:** 用戶通過 GDB 連接並發送 `Ctrl+C` 或硬件斷點觸發
3. **Halt 模式:** MCU 進入調試狀態，響應 GDB 命令
4. **調查/修改:** GDB 讀寫寄存器/內存
5. **繼續:** GDB 發送 `c` 命令，MCU 恢復運行

## 實作規劃

### Phase 1: 基本框架
- [ ] RSP 命令解析器
- [ ] 基本命令 `g`, `p`, `m`, `M`
- [ ] MSC Debug Channel 集成

### Phase 2: 執行控制
- [ ] `c`, `s` 命令
- [ ] 軟件斷點 `Z0`, `z0`
- [ ] 硬件斷點 `Z1`, `z1` (DWT)

### Phase 3: 擴展功能
- [ ] `qSupported` 查詢
- [ ] Watchpoint 支持
- [ ] PC sampling

## 參考資源

- [GDB Remote Serial Protocol](https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html)
- [ARM CoreSight DWT](https://developer.arm.com/documentation/ddi0439/b/System-Control/Debug/DWT)

## 依賴

- T025: MSC Debug Channel ✅ (已完成)
- ITM/SWO (T024) ✅ (已完成)
- DWT 研究 (T034) - 可並行
