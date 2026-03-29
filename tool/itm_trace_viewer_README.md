# ITM SWO Trace Viewer

PC 端接收並顯示 M487 Cortex-M4 ITM SWO trace 資料。

## 硬體連接方式

### 方案 A：UART-to-USB Bridge（最簡單，DIY）

```
M487 PB8 (SWO) ──→ UART-to-USB (FTDI/CH340) ──→ USB ──→ PC
                    e.g. FT232R
```

**線路連接：**
| M487 | UART-to-USB |
|------|-------------|
| PB8  | RX          |
| GND  | GND         |

**注意：**
- UART TTL 等級要 3.3V（不要接 5V UART）
- UART 參數：`baud=2000000, 8N1, no flow control`
- UART-to-USB 晶片支援 2 MHz baud：FT232R, FT230X, CP2102N, CH340G

### 方案 B：Nu-Link（有 Nu-Link 就用這個）

Nuvoton Nu-Link / Nu-Link-II 除錯器支援 SWO，但驅動需要確認。
可用 OpenOCD + Nu-Link 嘗試讀取 SWO：
```bash
openocd -f interface/nulink.cfg -c "transport select swd" \
  -c "init" -c "arm semihosting enable" \
  -c "tpiu config internal /tmp/swo.log uart off 2000000"
```

### 方案 C：J-Link（最完整）

```
M487 ──→ J-Link ──→ USB ──→ PC
          SWD
          SWO
```

SEGGER J-Link 硬體支援原生 SWO 接收，搭配 J-Link Software：
```bash
JLinkSWOViewer -Device M487 -If SWD -SWOBr 2000000
```

或透過 pyocd：
```bash
pyocd pack -i M487
pyocd gdbserver --tool xds110 --swo-port 2000000
```

## 安裝需求

```bash
pip install pyserial
```

## 使用方式

### 列出可用 COM 埠
```bash
py itm_trace_viewer.py --list-ports
```

### UART 模式（最常用）
```bash
# 基本用法
py itm_trace_viewer.py --uart COM5

# 指定 baud rate（預設 2 MHz 與 firmware 設定一致）
py itm_trace_viewer.py --uart COM5 --baud 2000000

# 無顏色輸出（輸出到檔案時推薦）
py itm_trace_viewer.py --uart COM5 --no-color

# 顯示 HEX dump
py itm_trace_viewer.py --uart COM5 --hex

# 只看某個 ITM port（port 0 = 主要 log port）
py itm_trace_viewer.py --uart COM5 --port 0
```

### 離線分析（Replay from file）
```bash
# 先把 trace 存成 binary 檔案
# (可透過 UART receiver 的 tee 功能或其他方式)
py itm_trace_viewer.py --file trace.bin

# 迴圈播放（壓力測試用）
py itm_trace_viewer.py --file trace.bin --loop
```

### J-Link 模式（需要 J-Link 硬體）
```bash
py itm_trace_viewer.py --jlink --jlink-device M487 --jlink-speed 4000
```

## 輸出範例

```
════════════════════════════════════════════════════════════
  ITM SWO Trace Viewer  (M487 Cortex-M4)
════════════════════════════════════════════════════════════
  Mode  : UART COM5 @ 2000000 baud
  Filter: port=all
  Color : True
────────────────────────────────────────────────────────────
  Press Ctrl+C to exit...
════════════════════════════════════════════════════════════

[12:34:56.789]  ℹ  ========================================
[12:34:56.790]  ℹ    ITM Trace Initialized
[12:34:56.791]  ℹ    SWO freq: 2000000 Hz
[12:34:56.792]  ℹ    Core clock: 192000000 Hz
[12:34:56.793]  ℹ  ========================================
[12:34:57.100]  ℹ  [USB] Device attached
[12:34:57.105]  ◷  [I2C] DBG: I2C addr=0x50
[12:34:57.200]  ℹ  [MSC] Read sector 0
```

## ITM Protocol 原理

### M487 SWO 硬體
- **SWO Pin**: PB8（Single Wire Output）
- **Peripheral**: ARM CoreSight ITM (Instrumentation Trace Macrocell) + TPIU
- **Baud Rate**: 2 MHz（預設）
- **Encoding**: Manchester / NRZ（依 debugger 而定）

### ITM 運作原理
1. **ITM Stimulus Port**：ARM Cortex-M4 有 32 個 stimulus ports（ITM_STIM_PORT = 0 用於主要 log）
2. **TPIU Formatter**：TPIU 將 ITM 資料封裝成 frame，加上 source ID 和同步位元
3. **SWO Output**：PB8 輸出單wire 的 trace 資料

### ITM Frame Format（TPIU）
```
Byte:
  Bit 7:   Stimulus Valid (1 = this byte is a stimulus byte)
  Bits 6-3: Source ID (000 = ITM)
  Bits 2-0: Stimulus Port number (low bits)

Payload bytes follow the frame header directly.
```

### Firmware ITM 輸出（itm.c）
- `ITM_LOG()` → 格式化字串 → `_ITM_PutChar()` → ITM_STIM_PORT[0]
- 每個字元寫入 `ITM->PORT[0].u8`，TPIU 自動封裝並輸出到 SWO

## Architecture

```
┌──────────────────────────────────────────────────────┐
│                  ITM Trace Viewer                     │
├──────────────────────────────────────────────────────┤
│  Receiver Layer                                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
│  │UART      │  │File      │  │J-Link     │           │
│  │Receiver  │  │Receiver  │  │Receiver   │           │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘           │
│       │              │              │                  │
│  ┌────▼──────────────▼──────────────▼────┐           │
│  │           ITMParser                    │           │
│  │  - TPIU frame removal                 │           │
│  │  - Stimulus port demux                │           │
│  │  - Byte stream reassembly             │           │
│  └────┬────────────────────────────┬─────┘           │
│       │                              │                  │
│  ┌────▼────┐  ┌────────────┐  ┌────▼────┐           │
│  │LineBuf  │  │ HexDump    │  │Stats    │           │
│  │         │  │            │  │         │           │
│  └────┬────┘  └────────────┘  └─────────┘           │
│       │                                               │
│  ┌────▼──────────────────────────────────────┐       │
│  │       OutputFormatter                       │       │
│  │  - ANSI color (module-aware)               │       │
│  │  - Timestamp                               │       │
│  │  - Level detection                         │       │
│  └────────────────────────────────────────────┘       │
└──────────────────────────────────────────────────────┘
```

## 擴展方向

1. **GUI**：使用 PyQt5 / DearPyGui 實作圖形化 viewer
2. **TCP Server**：接收 UART-to-TCP bridge 的網路串流
3. **Protocol 分析**：解碼更多 ITM packet types（timestamp, sync, etc.）
4. **Recording**：邊錄邊放，支援 pause/resume
5. **Filters**：時間範圍過濾、正則表達式過濾
6. **Export**：CSV, JSON, HTML 格式輸出

## 已知限制

- J-Link 模式需要 J-Link 硬體和軟體，目前僅為結構預留
- TPIU formatter decoder 目前假設 `async/NRZ` 模式（多數 UART bridge 使用）
- 不支援 ITM Hardware Event packets（需要 DWT 整合）
- Windows 上 ANSI color 需要 ANSI-capable terminal（如 Windows Terminal）
