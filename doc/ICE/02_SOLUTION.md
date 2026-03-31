# M487 ICE 連線解決方案

> 日期：2026-03-31（更新）  
> 狀態：✅ 完全成功

---

## 🎉 成功輸出

```
Info : Nu-Link firmware_version 7946, product_id (0x40012009)
Info : Adapter is Nu-Link
Info : IDCODE: 0x2BA01477
Info : [M487.cpu] Cortex-M4 r0p1 processor detected
Info : [M487.cpu] target has 6 breakpoints, 4 watchpoints
[M487.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x100028f0 msp: 0x20020000
```

---

## 🔑 成功關鍵

### 1. 正確的 OpenOCD Build

**使用：** `tool\OpenOCD\bin\openocd.exe`  
**版本：** OpenOCD 0.12.0+dev (2026-03-25)  
**大小：** 19MB  
**位置：** `D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\`

**需要的 DLL（在 `C:\msys64\mingw64\bin`，已透過 openocd.bat 自動設定）：**
- `libusb-1.0.dll`
- `libiconv-2.dll`
- `libhidapi-0.dll`
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

### 2. NULINK HLA Driver（不是 CMSIS-DAP）

| 設定 | 值 |
|------|---|
| **Driver** | `hla`（HLA, High-Level Adapter）|
| **Layout** | `nulink`（Nu-Link 專有）|
| **VID/PID** | `0x0416 / 0x511C`（第一代 Nu-Link）|
| **Transport** | `swd` |

### 3. OpenOCD 設定檔（nulink_m487_ice.cfg）

**位置：** `D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg`

```tcl
adapter driver hla
hla layout nulink
hla vid_pid 0x0416 0x511C
transport select swd

# M487 SWD DP-ID = 0x2BA01477
swd newdap M487 cpu -expected-id 0x2BA01477
dap create M487.dap -chain-position M487.cpu
target create M487.cpu cortex_m -dap M487.dap

# Clock speed (kHz)
adapter speed 4000
```

> ⚠️ **陣阱：** `hla layout nulink` 這行是關鍵。若被註解掉或遺失，OpenOCD 將無法認識 Nu-Link，導致燒錄失敗而且不一定會顯示明顯錯誤訊息。

### 4. Wrapper Script（含 DLL PATH）

**位置：** `D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat`

```bat
@echo off
set PATH=%~dp0bin;C:\msys64\mingw64\bin;%PATH%
"%~dp0bin\openocd.exe" %*
```

> ⚠️ **說明：** 此 wrapper 自動將 `tool\OpenOCD\bin\` 和 `C:\msys64\mingw64\bin` 加入 PATH，解決 MSYS2 DLL 依賴問題。**必須透過此檔執行 OpenOCD，勿直接呼叫 `openocd.exe`。**

---

## 🚀 快速啟動

### 方法一：使用 Wrapper Batch（推薦）

```powershell
# 測試連線
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg -c "init" -c "targets" -c "shutdown"

# 啟動 GDB Server
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg

# Reset + Halt
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg -c "init" -c "reset halt" -c "shutdown"
```

### 方法二：VSCode F5（最方便）

1. 在 VSCode 開啟 `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\`
2. 確保已編譯：`Ctrl+Shift+B`
3. 選擇 Debug 模式（`OpenOCD Flash & Debug`）
4. 按 **F5** → 自動燒錄 → 停在 main

---

## 💾 燒錄韌體

### 方法一：OpenOCD CLI

```powershell
cd D:\AiWorkSpace\M487_ScsiTool\firmware\composite\build_gcc
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg -c "init" -c "reset halt" -c "flash write_image erase firmware.bin 0" -c "shutdown"
```

> ⚠️ **陷阱：** 務必確認 `-f` 指向 `D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg`，若路徑錯誤會導致 OpenOCD 找不到設定檔而無法啟動。

### 方法二：VSCode（推薦）

按 `Ctrl+Shift+B` 編譯後，按 `F5` 選擇 `OpenOCD Flash & Debug`。

---

## 🐛 GDB Debug

### 前置條件

OpenOCD GDB Server 必須已啟動（在同一台機器的 port 3333）。

### GDB CLI 快速指令

```powershell
$GDB = "C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\bin\arm-none-eabi-gdb.exe"
$ELF = "D:\AiWorkSpace\M487_ScsiTool\firmware\composite\build_gcc\firmware.elf"

& $GDB --batch -ex "file $ELF" -ex "target remote localhost:3333" -ex "monitor reset halt" -ex "load" -ex "info registers" -ex "detach"
```

### 完整 GDB 指令對照表

| 指令 | 功能 |
|------|------|
| `target remote localhost:3333` | 連線到 OpenOCD |
| `monitor reset halt` | 重置 + Halt MCU |
| `load` | 燒錄 ELF 到 Flash |
| `file firmware.elf` | 載入符號表 |
| `info registers` | 顯示所有暫存器 |
| `x/16x 0x20000000` | 讀 SRAM（16 words）|
| `x/16x 0x00000000` | 讀 Flash（16 words）|
| `set {int}0x20000010 = 0xDEADBEEF` | 寫記憶體 |
| `break main` | 設斷點 |
| `watch *0x20000010` | 設硬體觀看點 |
| `stepi` | 單步執行 |
| `continue` | 繼續執行 |
| `detach` | 結束 Debug |

---

## 📊 技術規格

| 項目 | 值 |
|------|---|
| **晶片** | Nuvoton M487JIDAE (Cortex-M4 r0p1) |
| **IDCODE** | 0x2BA01477 |
| **Transport** | SWD |
| **Clock** | 4 MHz |
| **Breakpoints** | 6 個（硬體）|
| **Watchpoints** | 4 個（硬體）|
| **GDB Server Port** | 3333 |
| **Telnet Port** | 4444 |
| **TCL Port** | 6666 |

---

## ⚠️ 重要提醒

1. **使用 `tool\OpenOCD\`（整合版）**，不是 `tool\openocd\`（舊版，已廢棄）
2. **務必透過 `openocd.bat` 執行**，自動設定 MSYS2 DLL PATH
3. **不要使用 Nuvoton 內建的 `numicroM4.cfg`** — 該檔案使用舊版 OpenOCD 語法，會導致 hang
4. **GDB 必須先 `file firmware.elf`** 才能正確解析符號，否則顯示 `??`
5. **`hla layout nulink` 絕對不能遺失** — 若遺失此行，燒錄會骙默失敗，且沒有明顯錯誤提示

---

## 📁 工具目錄結構

```
D:\AiWorkSpace\M487_ScsiTool\tool\
├── OpenOCD\                              ← 整合版（使用這個）
│   ├── openocd.bat                       ← DLL wrapper
│   ├── bin\
│   │   ├── openocd.exe                  ← 19MB, 0.12.0+dev
│   │   └── *.dll                         ← MSYS2 DLLs
│   ├── cfg\
│   │   └── nulink_m487_ice.cfg          ← M487 + Nu-Link 設定
│   └── scripts\                          ← OpenOCD TCL scripts
└── openocd\                              ← 舊版（已廢棄）
```

---

*最後更新：2026-03-31 10:54*