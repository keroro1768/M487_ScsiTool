# M487 ICE 連線解決方案

> 日期：2026-03-30  
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

**使用：** `D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin\openocd.exe`  
**版本：** OpenOCD 0.12.0+dev (2026-03-25)  
**原因：** 唯一完整支援 NULINK HLA layout 的 build

### 2. MSYS2 DLL 環境

**PATH 設定：**
```
C:\msys64\mingw64\bin;D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin;%PATH%
```

**需要的 DLL（在 C:\msys64\mingw64\bin）：**
- `libusb-1.0.dll` ✅
- `hidapi.dll` ✅
- `libgcc_s_dw2-1.dll` ✅
- `libwinpthread-1.dll` ✅

### 3. 正確的命令序列（OpenOCD 0.12 語法）

```
adapter driver hla
hla layout nulink
hla vid_pid 0x0416 0x511C
transport select swd
swd newdap M487 cpu -expected-id 0x2BA01477
dap create M487.dap -chain-position M487.cpu
target create M487.cpu cortex_m -dap M487.dap
```

**注意：** OpenOCD 0.12 的 target create 使用 `-dap` 參數，舊版（numicroM4.cfg）使用 `-chain-position` 會導致 hang。

---

## 🚀 快速啟動

### 方法一：使用 Wrapper Batch（推薦）

```powershell
# 測試連線
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -c "init" -c "reset halt" -c "targets" -c "shutdown"

# 啟動 GDB Server
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat
```

### 方法二：手動設定 PATH

```powershell
$env:PATH = 'C:\msys64\mingw64\bin;D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin;' + $env:PATH
cd D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD-Nuvoton\OpenOCD\bin
.\openocd.exe -s ../scripts -f D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg
```

### 方法三：VSCode F5（最方便）

1. 在 VSCode 開啟 `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\`
2. 確保已編譯：`Ctrl+Shift+B`
3. 按 **F5** 開始 Debug

---

## 💾 燒錄韌體

```powershell
$env:PATH = 'C:\msys64\mingw64\bin;D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin;' + $env:PATH
.\openocd.exe -s ../scripts -f nulink_m487_ice.cfg -c "init" -c "reset halt" -c "flash write_image erase build/firmware.bin 0" -c "shutdown"
```

---

## 🐛 GDB 連線

```bash
arm-none-eabi-gdb build/firmware.elf
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
```

---

## 📁 設定檔

### nulink_m487_ice.cfg

**位置：** `D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg`

```tcl
# OpenOCD Config for M487 via Nu-Link (First Gen)
# Compatible with openocd-build (0.12.0+dev 2026-03-25)
#
# Requirements:
#   PATH must include: C:\msys64\mingw64\bin;D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin;
#
# Usage:
#   openocd.exe -s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD-Nuvoton\OpenOCD\scripts -f nulink_m487_ice.cfg

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

### openocd.bat（Wrapper）

**位置：** `D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat`

```bat
@echo off
set PATH=C:\msys64\mingw64\bin;%PATH%
set PATH=%PATH%;D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin
"D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin\openocd.exe" %*
```

---

## ⚠️ 重要提醒

1. **不要使用 Nuvoton 內建的 `numicroM4.cfg`** — 該檔案使用舊版 OpenOCD 語法，會導致 hang
2. **務必設定 MSYS2 PATH** — 否則 openocd.exe 會因為缺少 DLL 而無法啟動
3. **wrapper batch 會阻斷 Ctrl+C** — 如需中斷，按 Ctrl+Break 或關閉視窗

---

## 📊 技術規格

| 項目 | 值 |
|------|---|
| **晶片** | Nuvoton M487JIDAE (Cortex-M4 r0p1) |
| **IDCODE** | 0x2BA01477 |
| **Transport** | SWD |
| **Clock** | 4 MHz |
| **Breakpoints** | 6 |
| **Watchpoints** | 4 |
| **GDB Server Port** | 3333 |
| **Telnet Port** | 4444 |
| **TCL Port** | 6666 |

---

*最後更新：2026-03-30 11:30*
