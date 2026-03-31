# M487 ICE 快速上手指南

> 適用對象：第一次使用 M487 + Nu-Link + OpenOCD  
> 整理時間：2026-03-31（更新）  
> 前置條件： Nu-Link 已連接 USB、Windows 已安裝驅動

---

## 🎯 這份文件能讓你做什麼？

- ✅ **燒錄** M487 韌體（CLI 或 VSCode）
- ✅ **Debug** 在 VSCode 中單步執行、設斷點、查看暫存器
- ✅ **驗證** ICE 連線是否正常

---

## 0️⃣ 前置檢查

### 確認 Nu-Link 已連接

在 PowerShell 執行：
```powershell
Get-PnpDevice | Where-Object { $_.DeviceId -match '0416.*511C' }
```

預期輸出：
```
USB\VID_0416&PID_511C&MI_01\...  Status: OK  (WinUSB ✅)
```

### 確認 OpenOCD 可執行

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg -c "adapter list"
```

預期：看見 `hla { jtag swd }` 在清單中。

---

## 1️⃣ 燒錄韌體

### 方法一：VSCode（推薦）

1. 開啟 VSCode：`D:\AiWorkSpace\M487_ScsiTool\firmware\composite\`
2. 編譯：`Ctrl+Shift+B`
3. 選擇 **「OpenOCD Flash & Debug」** 設定檔
4. 按 **F5** → 自動燒錄 →停在 main

### 方法二：CLI 燒錄

```powershell
# 燒錄
cd D:\AiWorkSpace\M487_ScsiTool\firmware\composite\build_gcc
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat `
  -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg `
  -c "init" `
  -c "reset halt" `
  -c "flash write_image erase firmware.bin 0" `
  -c "shutdown"
```

**成功輸出：**
```
Info : Padding image section 1 at 0x1004e000 with 2048 bytes
Info : Write 63996 bytes to flash
Info : Write 2048 bytes to flash
Info : Verified OK
```

---

## 2️⃣ Debug（VSCode）

### 完整流程

```
1. 開啟 VSCode → firmware/composite/
2. Ctrl+Shift+B  (編譯)
3. F5  (燒錄 + Debug 或 直接 Debug，取決於設定檔)
4. 在 main() 或感興趣的地方按 F9 設斷點
5. F5 繼續執行到斷點
6. F10/F11 單步執行
7. 在 VARIABLES 視窗看變數
8. 在 REGISTERS 視窗看 CPU 暫存器
```

### Debug 按鍵

| 按鍵 | 功能 |
|------|------|
| **F5** | 繼續執行（到下一個斷點）|
| **F9** | 設定/取消斷點 |
| **F10** | 單步執行（不進函式）|
| **F11** | 單步執行（進函式）|
| **Shift+F11** | 跳出函式 |
| **F6** | 停止 Debug |

### Debug 視窗

| 視窗 | 開啟方式 | 用途 |
|------|---------|------|
| **Variables** | 預設顯示 | 看區域變數 |
| **Watch** | 右鍵新增 | 長期監看特定變數 |
| **REGISTERS** | 預設顯示 | 看 R0-R15, XPSR, MSP, PSP |
| **Call Stack** | 預設顯示 | 看函式呼叫堆疊 |
| **Memory** | Debug Console 輸入 `memory read` | 看記憶體內容 |

### 三種 Debug 模式

| 模式 | 用途 | 流程 |
|------|------|------|
| **OpenOCD Flash & Debug** | 第一次燒錄 | 燒錄 →停在 main |
| **Debug M487 (OpenOCD Attach)** | 已有韌體 | 直接 attach →停在 main |
| **Debug M487 (OpenOCD Launch)** | 不燒錄 | 直接 launch |

---

## 3️⃣ 驗證 ICE 連線

### 基本連線測試

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat `
  -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg `
  -c "init" -c "targets" -c "shutdown"
```

**預期輸出：**
```
Info : clock speed 4000 kHz
Info : Nu-Link firmware_version 7946, product_id (0x40012009)
Info : Adapter is Nu-Link
Info : IDCODE: 0x2BA01477
Info : [M487.cpu] Cortex-M4 r0p1 processor detected
Info : [M487.cpu] target has 6 breakpoints, 4 watchpoints
    TargetName         Type       Endian TapName            State
--  ------------------ ---------- ------ ------------------ ------------
 0* M487.cpu           hla_target little M487.cpu           unknown
shutdown command invoked
```

### Reset Halt 測試

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat `
  -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg `
  -c "init" -c "reset halt" -c "targets" -c "shutdown"
```

**預期輸出：**
```
[M487.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x100028f0 msp: 0x20020000
```

### 寄存器讀取測試

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\openocd.bat `
  -f D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\nulink_m487_ice.cfg `
  -c "init" -c "reset halt" -c "reg pc" -c "reg xpsr" -c "shutdown"
```

---

## 4️⃣ GDB CLI Debug（指令列模式）

**適用場景：** 不使用 VSCode，直接用 GDB 指令操作  
**前置條件：** OpenOCD 已在 port 3333 執行

### 快速測試腳本

```powershell
$GDB = "C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\bin\arm-none-eabi-gdb.exe"
$ELF = "D:\AiWorkSpace\M487_ScsiTool\firmware\composite\build_gcc\firmware.elf"

& $GDB --batch -ex "file $ELF" -ex "target remote localhost:3333" -ex "monitor reset halt" -ex "load" -ex "info registers" -ex "x/8x 0x20000000" -ex "detach"
```

### 完整 GDB 指令對照表

| 指令 | 功能 | 說明 |
|------|------|------|
| `target remote localhost:3333` | 連線 | 連到 OpenOCD GDB Server |
| `monitor reset halt` | 重置 | Reset + Halt MCU |
| `load` | 燒錄 | 燒錄 ELF 到 Flash |
| `file firmware.elf` | 載入符號表 | 載入 Debug 資訊 |
| `info registers` | 顯示暫存器 | R0-R15, XPSR, MSP, PSP |
| `x/16x 0x20000000` | 讀 SRAM | 顯示 16 個 word |
| `x/16x 0x00000000` | 讀 Flash | 顯示 16 個 word |
| `set {int}0x20000010 = 0xDEADBEEF` | 寫記憶體 | 寫入指定值 |
| `break main` | 設斷點 | 在 main() 停下 |
| `watch *0x20000010` | 設觀看點 | 記憶體被改時停下 |
| `stepi` | 單步 | 執行一條指令 |
| `continue` | 繼續執行 | 執行到下一個斷點 |
| `detach` | 離開 | 結束 Debug Session |

---

## ⚠️ 常見問題

### Q: `LIBUSB_ERROR_ACCESS`

執行時缺少 MSYS2 DLL。透過 `openocd.bat` 執行即可，對內已自動設定 PATH。

### Q: 燒錄成功但查不到燒錄內容

cfg 檔遺失 `hla layout nulink`。編輯 `nulink_m487_ice.cfg`，確認內容如下：
```
adapter driver hla
hla layout nulink     ← 此行必須存在
hla vid_pid 0x0416 0x511C
```

### Q: `CMSIS-DAP command CMD_INFO failed`

使用了錯誤的 OpenOCD driver。確認使用的是 `openocd-build\bin\openocd.exe` + `hla driver`。

### Q: F5 沒反應

檢查：
- [ ] Nu-Link USB 已插上
- [ ] `build/firmware.elf` 存在（先 `Ctrl+Shift+B` 編譯）
- [ ] `launch.json` 中 `serverpath` 正確

### Q: Watch 看不到變數

確認編譯時最佳化設為 `-O0`（在 Makefile 中）。

---

## 📁 關鍵檔案位置

| 用途 | 路徑 |
|------|------|
| **OpenOCD** | `tool\OpenOCD\bin\openocd.exe` |
| **OpenOCD Config** | `tool\OpenOCD\nulink_m487_ice.cfg` |
| **Wrapper** | `tool\OpenOCD\openocd.bat` |
| **launch.json** | `firmware\composite\.vscode\launch.json` |
| **韌體 (ELF)** | `firmware\composite\build_gcc\firmware.elf` |
| **韌體 (BIN)** | `firmware\composite\build_gcc\firmware.bin` |
| **MSYS2 DLL** | `C:\msys64\mingw64\bin\` |

---

## 🔗 相關文件

| 文件 | 說明 |
|------|------|
| [00_INDEX.md](00_INDEX.md) | ICE 文檔索引 |
| [01_PROBLEM.md](01_PROBLEM.md) | 問題分析 |
| [02_SOLUTION.md](02_SOLUTION.md) | 解決方案與技術細節 |
| [03_VSCODE.md](03_VSCODE.md) | VSCode Debug 完整設定 |
| [04_VERIFICATION.md](04_VERIFICATION.md) | 驗證方法與預期輸出 |

---

*最後更新：2026-03-31 10:54*
