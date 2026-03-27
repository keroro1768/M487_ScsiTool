# OpenOCD + Nu-Link 燒錄與除錯指南

> 工具版本：Nuvoton OpenOCD (2024-01)  
> 燒錄器：Nu-Link (VID=0x0416, PID=0x511c)  
> 目標晶片：M487 (NuMicro M480 series, Cortex-M4)

---

## 1. OpenOCD 安裝位置

```
C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\
├── bin\openocd.exe          # 主程式
└── share\openocd\scripts\   # 設定檔
    ├── interface\           # 燒錄器設定
    │   └── nulink.cfg       # Nu-Link 驅動
    └── target\              # 目標晶片設定
        ├── numicroM4.cfg    # M4 系列（含 M487）
        ├── numicroM4_nulink2.cfg
        └── ...
```

---

## 2. 快速開始

### 2.1 基本燒錄

```bat
"C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" ^
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" ^
  -f interface/nulink.cfg ^
  -f target/numicroM4.cfg ^
  -c "init" ^
  -c "reset halt" ^
  -c "flash write_image erase <BIN_PATH> 0" ^
  -c "reset run" ^
  -c "shutdown"
```

### 2.2 快速燒錄（flash.bat）

```bat
@echo off
"C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" ^
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" ^
  -f interface/nulink.cfg ^
  -f target/numicroM4.cfg ^
  -c "init" -c "reset halt" ^
  -c "flash write_image erase <FIRMWARE.bin> 0" ^
  -c "reset run" -c "shutdown"
```

---

## 3. 常用 OpenOCD 指令

### 3.1 燒錄相關

| 指令 | 說明 |
|------|------|
| `init` | 初始化偵錯器與目標 |
| `reset halt` | 重置並 halt CPU |
| `reset run` | 重置並執行 |
| `flash write_image erase <file> <addr>` | 燒錄 binary |
| `flash write_image erase <file> <addr> <len>` | 指定長度燒錄 |
| `flash write_image erase <file> <addr> verify` | 燒錄後驗證 |
| `flash erase_address <addr> <len>` | 抹除 flash 區塊 |
| `flash banks` | 列出所有 flash bank |
| `flash info <bank>` | 顯示 flash 資訊 |

### 3.2 讀取相關

| 指令 | 說明 |
|------|------|
| `dump_image <file> <addr> <len>` | 傾印記憶體到檔案 |
| `load_image <file> <addr>` | 載入檔案到記憶體 |
| `read_memory <addr> <width> <count>` | 讀取記憶體 |

`width`: `b`=byte, `h`=halfword(16-bit), `w`=word(32-bit)

### 3.3 執行控制

| 指令 | 說明 |
|------|------|
| `halt` | Halt CPU |
| `resume` | 繼續執行 |
| `step` | 單步執行 |
| `reset` | 重置目標 |
| `wait_halt <timeout>` | 等候 halt |

### 3.4 中斷（Breakpoint）

| 指令 | 說明 |
|------|------|
| `bp <addr> <len> <type>` | 設定 breakpoint |
| `rbp <addr>` | 移除 breakpoint |
| `bp` | 列出所有 breakpoints |

`type`: `hw`=硬體, `sw`=軟體（預設）

### 3.5 暫存器

| 指令 | 說明 |
|------|------|
| `reg` | 顯示所有暫存器 |
| `reg <name>` | 顯示特定暫存器 |
| `reg <name> <value>` | 寫入暫存器 |

---

## 4. GDB 除錯整合

### 4.1 啟動 OpenOCD + GDB Server

```bat
"C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" ^
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" ^
  -f interface/nulink.cfg ^
  -f target/numicroM4.cfg ^
  -c "init" ^
  -c "reset halt"
```

預設 GDB Server port: `3333`

### 4.2 連接 GDB

```bash
# arm-none-eabi-gdb
arm-none-eabi-gdb firmware.elf

# 在 GDB 內：
(gdb) target remote localhost:3333
(gdb) load              # 燒錄程式
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

### 4.3 常用 GDB + OpenOCD 指令

```bash
(gdb) monitor halt              # Halt CPU
(gdb) monitor reset halt        # Reset 並 halt
(gdb) monitor reg               # 顯示暫存器
(gdb) monitor reg pc            # 顯示 PC
(gdb) monitor step              # 單步
(gdb) monitor cortex_m reset-system  # 系統重置
(gdb) load                      # 燒錄 elf 到 flash
(gdb) x/16x 0x20000000          # 查看記憶體 (16 個 word)
(gdb) print myVariable          # 印出變數值
(gdb) bt                        # Backtrace (呼叫堆疊)
(gdb) info threads              # 執行緒資訊
```

### 4.4 VSCode GDB 設定

```json
{
  "name": "OpenOCD Debug",
  "type": "cppdbg",
  "request": "launch",
  "executable": "${workspaceFolder}/firmware.elf",
  "MIMode": "gdb",
  "miDebuggerPath": "C:/Users/rinry/Tool/xpack-arm-none-eabi-gcc-15.2.1-1.1/bin/arm-none-eabi-gdb.exe",
  "miDebuggerServerAddress": "localhost:3333",
  "setupCommands": [
    { "text": "target remote localhost:3333" },
    { "text": "monitor reset halt" },
    { "text": "load" }
  ],
  "preLaunchTask": "OpenOCD Server"
}
```

---

## 5. Debugger 設定選項

### 5.1 設定 SWD 速度

```bat
# 預設 1 MHz（適用大部分情境）
adapter_khz 1000

# 提高速度（除錯用，燒錄也可）
adapter_khz 4000

# 降低速度（穩定性問題時）
adapter_khz 100
```

### 5.2 設定 CPU DAP-ID（一般不需要改）

```tcl
# numicroM4.cfg 內
set CPUDAPID 0x2BA01477
```

### 5.3 設定 Work-Area 大小

```tcl
# 用於 flash 燒錄時的 RAM buffer
set WORKAREASIZE 0x4000   # 16KB
```

### 5.4 Reset 設定

```tcl
# 無 SRST 信號（常用）
reset_config none

# 使用 SYSRESETREQ
reset_config sysresetreq

# 使用 VECTRESET
reset_config vectreseta
```

---

## 6. Flash 燒錄細節

### 6.1 M487 Flash 架構

| Bank | 位址 | 大小 | 用途 |
|------|------|------|------|
| APROM | 0x00000000 | 最大 | 主要程式 |
| LDROM | 0x00100000 | 4KB | bootloader |
| SPROM | 0x00200000 | 4KB | 安全性 |
| CONFIG | 0x00300000 | - | 設定 |
| Data Flash | 0x0001F000 | - | 資料 |

### 6.2 燒錄特定位置

```tcl
# 燒錄到 APROM
flash write_image erase firmware.bin 0x00000000

# 燒錄到 LDROM
flash write_image erase firmware.bin 0x00100000

# 燒錄並驗證
flash write_image erase firmware.bin 0x00000000 verify
```

### 6.3 抹除

```tcl
# 抹除整個 APROM
flash erase_address 0x00000000 0x40000

# 抹除特定範圍
flash erase_address 0x00000000 0x1000
```

---

## 7. 常見問題

### 7.1 燒錄失敗

```
Error: couldn't open ... No matching device
```
→ 確認 Nu-Link USB 已連接，VID/PID=0x0416/0x511c

### 7.2 找不到燒錄器

```
Error: no devices found on USB
```
→ 確認驅動程式已安裝（需手動安裝 USB 驅動）

### 7.3 燒錄時無回應

```
Timeout during flash operation
```
→ 降低 SWD 速度：`adapter_khz 100`

### 7.4 GDB 無法連線

```
Remote connection failed
```
→ 確認 OpenOCD 仍在執行，port 3333 未被佔用

### 7.5 無法 Halt

```
Target not halted
```
→ 嘗試 `reset halt`，或降低 `adapter_khz`

---

## 8. TCL 批次指令檔

將指令寫入 `.tcl` 檔案，用 `-c "source script.tcl"` 執行：

```tcl
# flash_and_run.tcl
init
reset halt
flash write_image erase firmware.bin 0x00000000 verify
reset run
shutdown
```

執行：
```bat
openocd.exe -f interface/nulink.cfg -f target/numicroM4.cfg -c "source flash_and_run.tcl"
```

---

## 9. 進階除錯

### 9.1 ITM (Instrumentation Trace Macrocell)

M487 支援 ARM CoreSight ITM，可用於即時追蹤輸出。

```tcl
# 啟用 ITM
cortex_m show_faults
```

### 9.2 ETB (Embedded Trace Buffer)

```tcl
# 傾印 ETB 內容
trace history
```

### 9.3 即時記憶體監看

```tcl
# 每秒更新一次記憶體顯示
poll 1000
```
