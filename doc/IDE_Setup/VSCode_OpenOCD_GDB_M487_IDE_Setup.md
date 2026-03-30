# VSCode + OpenOCD + GCC + GDB — M487 ICE 開發環境完整設定

> 最後更新：**2026-03-30**（根據已驗證的 ICE 連線方案更新）
> 適用對象：完全不會 Linux/VSCode 的 Windows 工程師

---

## ⚠️ 重要更新（2026-03-30）

**燒錄 + Debug 已完全驗證成功！** 所有設定已更正。

| 之前（錯誤）| 之後（正確）|
|-------------|-------------|
| `openocd_cmsis-dap.exe` | `openocd.bat`（wrapper，含 MSYS2 DLL PATH）|
| `m487_cmsis_dap.cfg` | `nulink_m487_ice.cfg` |
| `cmsis-dap` driver | `hla` driver |
| OpenOCD-Nuvoton scripts | OpenOCD-Nuvoton scripts（路徑不變）|

---

## 📋 這份文件能讓你做什麼？

- ✅ 在 VSCode 中一鍵編譯 M487 韌體
- ✅ 在 VSCode 中燒錄 M487（OpenOCD）
- ✅ 在 VSCode 中 Debug（M487 軟體中斷點、查看暫存器、記憶體）
- ✅ 不需要離開 VSCode，全程圖形化 Debug

---

## 🗺️ 學習地圖

```
Step 1：安裝必備軟體
    ↓
Step 2：確認 Nu-Link 驅動
    ↓
Step 3：設定 VSCode 專案
    ↓
Step 4：設定編譯（Build）
    ↓
Step 5：設定燒錄（Flash）
    ↓
Step 6：設定 Debug（Attach / Launch）
    ↓
Step 7：第一次 Debug
```

---

## Step 1：安裝必備軟體

### 1.1 VSCode（如果還沒安裝）

下載：https://code.visualstudio.com/

### 1.2 VSCode Extensions（必需）

在 VSCode 中開啟 Extensions（Ctrl+Shift+X），安裝以下：

| Extension | 用途 | 安裝後名稱 |
|-----------|------|-----------|
| C/C++ | C/C++ 語法支援 | `ms-vscode.cpptools` |
| Cortex-Debug | ARM Cortex Debug 支援 | `marus25.cortex-debug` |
| ARM Assembly | 組合語言語法 Highlight | `dland90.arm-assembly` |

**安裝方式：**
1. VSCode → Extensions（Ctrl+Shift+X）
2. 搜尋名稱
3. 點 Install

### 1.3 GCC ARM Toolchain（如果還沒安裝）

已安裝位置：`C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\`

**驗證方式：**
```cmd
arm-none-eabi-gcc --version
```

如果沒有安裝，下載：https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases

選擇 `xpack-arm-none-eabi-gcc-15.2.1-1.1-win32-x64.zip`

### 1.4 OpenOCD

> ⚠️ **2026-03-30 修正：** 必須使用 `openocd-build` 而非 `OpenOCD-Nuvoton` 內的 binary！

**已安裝位置（正確的 build）：**
```
D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin\openocd.exe
```
版本：OpenOCD 0.12.0+dev (2026-03-25)

**Wrapper（推薦使用）：**
```
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat
```
Wrapper 會自動設定 MSYS2 DLL PATH，呼叫正確的 openocd.exe。

**為何不用 `OpenOCD-Nuvoton` 內的 binary？**
- `openocd_cmsis-dap.exe`：HLA NULINK 初始化失敗
- `sysprogs/openocd.exe`：沒有 NULINK layout
- 只有 `openocd-build` 的版本完整支援 NULINK

---

## Step 2：確認 Nu-Link 驅動（最常出問題的地方）

### 2.1 為什麼驅動重要？

```
Nu-Link → USB → Windows → OpenOCD
                    ↑
              這裡就是驅動
```

OpenOCD 需要透過 **WinUSB** 或 **libusb** 存取 Nu-Link。

### 2.2 檢查 Nu-Link 驅動

**方法一：PowerShell**

```powershell
Get-PnpDevice | Where-Object { $_.DeviceId -match '0416.*511C' } |
  Format-Table FriendlyName, Status, InstanceId
```

**預期輸出：**
```
FriendlyName                            Status  InstanceId
------------                            ------  ----------
USB Composite Device                    OK      USB\VID_0416&PID_511C\...
Nuvoton Nu-Link USB                     OK      USB\VID_0416&PID_511C&MI_00\...
Nuvoton Nu-Link USB                     OK      USB\VID_0416&PID_511C&MI_01\...
```

**方法二：用 Zadig 檢查**

1. 開啟 `D:\AiWorkSpace\M487_ScsiTool\tool\external\zadig-2.9.exe`
2. Options → List All Devices
3. 找 `NuLink [0416:511C]`
4. 看 Current Driver

### 2.3 Interface 1 驅動狀態確認

```powershell
Get-PnpDeviceProperty -InstanceId 'USB\VID_0416&PID_511C&MI_01\...' -KeyName 'DEVPKEY_Device_DriverInfPath'
```

確認 driver = `oemXX.inf`（WINUSB）。

### 2.4 驗證 OpenOCD 可以連線

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -c "adapter list"
```

**成功輸出：** 看見 `hla { jtag swd }` 在清單中。

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -c "init" -c "targets" -c "shutdown"
```

**成功輸出（已驗證）：**
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

---

## Step 3：VSCode 專案設定

### 3.1 開啟 M487_ScsiTool 資料夾

```vscode
File → Open Folder → D:\AiWorkSpace\M487_ScsiTool\firmware\composite
```

### 3.2 建立 `.vscode` 資料夾

在 `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\` 建立 `.vscode` 資料夾

### 3.3 建立 `c_cpp_properties.json`（C/C++ 路徑）

```json
{
  "configurations": [
    {
      "name": "M487",
      "includePath": [
        "${workspaceFolder}/**",
        "${workspaceFolder}/../../../KM/M480BSP/Library/CMSIS/CMSIS/Include",
        "${workspaceFolder}/../../../KM/M480BSP/Library/CMSIS/Device/Nuvoton/M480/Include",
        "${workspaceFolder}/../../../KM/M480BSP/StdDriver/inc",
        "${workspaceFolder}/../../../KM/M480BSP/StdDriver/inc",
        "${workspaceFolder}/dwt",
        "${workspaceFolder}/gdb_rsp"
      ],
      "defines": [
        "__M487JIDAE__",
        "ARM_MATH_CM4",
        "USE_ARM_MATH"
      ],
      "compilerPath": "C:/Users/rinry/Tool/xpack-arm-none-eabi-gcc-15.2.1-1.1/bin/arm-none-eabi-gcc.exe",
      "cStandard": "c11",
      "cppStandard": "c++17",
      "intelliSenseMode": "gcc-arm"
    }
  ],
  "version": 4
}
```

---

## Step 4：設定編譯（Build Tasks）

### 4.1 建立 `tasks.json`

在 `.vscode` 資料夾建立 `tasks.json`：

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build M487 Firmware",
      "type": "shell",
      "command": "make",
      "args": ["-f", "Makefile"],
      "options": {
        "cwd": "${workspaceFolder}"
      },
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": ["$gcc"],
      "detail": "使用 GCC 編譯 M487 韌體"
    },
    {
      "label": "Clean Build",
      "type": "shell",
      "command": "make",
      "args": ["-f", "Makefile", "clean"],
      "options": {
        "cwd": "${workspaceFolder}"
      },
      "problemMatcher": []
    },
    {
      "label": "Flash M487",
      "type": "shell",
      "command": "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/openocd.bat",
      "args": [
        "-s", "D:/AiWorkSpace/M487_ScsiTool/tool/OpenOCD-Nuvoton/OpenOCD/scripts",
        "-f", "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg",
        "-c", "init",
        "-c", "reset halt",
        "-c", "flash write_image erase ${workspaceFolder}/build/firmware.bin 0",
        "-c", "shutdown"
      ],
      "options": {
        "cwd": "${workspaceFolder}"
      },
      "problemMatcher": [],
      "dependsOn": ["Build M487 Firmware"]
    }
  ]
}
```

### 4.2 編譯快捷鍵

- **Ctrl+Shift+B**：編譯
- **終端機 → Run Task**：選擇其他任務

---

## Step 5：設定燒錄（Flash）

### 5.1 建立燒錄 Script

> ⚠️ **2026-03-30 修正：** 更新路徑使用 `openocd.bat` + `nulink_m487_ice.cfg`

在 `D:\AiWorkSpace\M487_ScsiTool\tool\` 建立 `flash_m487.bat`：

```bat
@echo off
set OPENOCD_BAT=D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat
set OPENOCD_SCR=D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD-Nuvoton\OpenOCD\scripts
set CFG_FILE=D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg
set FIRMWARE=%1

if "%FIRMWARE%"=="" (
    set FIRMWARE=D:\AiWorkSpace\M487_ScsiTool\firmware\composite\build\firmware.bin
)

echo Flashing: %FIRMWARE%
"%OPENOCD_BAT%" -s "%OPENOCD_SCR%" -f "%CFG_FILE%" -c "init" -c "reset halt" -c "flash write_image erase %FIRMWARE% 0" -c "shutdown"
pause
```

### 5.2 燒錄驗證

1. 按 **Ctrl+Shift+P**
2. 輸入 `Tasks: Run Task`
3. 選擇 `Flash M487`

---

## Step 6：設定 Debug（重要！）

> ⚠️ **2026-03-30 修正：** 所有路徑已更新為 `openocd.bat` + `nulink_m487_ice.cfg`

### 6.1 建立 `launch.json`

在 `.vscode` 資料夾建立 `launch.json`：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug M487 (OpenOCD Attach)",
      "type": "cortex-debug",
      "request": "attach",
      "servertype": "openocd",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/build/firmware.elf",
      "serverpath": "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/openocd.bat",
      "searchDir": [
        "D:/AiWorkSpace/M487_ScsiTool/tool/OpenOCD-Nuvoton/OpenOCD/scripts"
      ],
      "configFiles": [
        "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg"
      ],
      "overrideLaunchCommands": [
        "monitor reset halt",
        "load"
      ],
      "runToEntryPoint": "main",
      "svdFile": "D:/AiWorkSpace/KM/M480BSP/Library/CMSIS/Device/Nuvoton/M480/Source/arm/M487.svd",
      "preLaunchTask": "Build M487 Firmware",
      "device": "M487JIDAE",
      "interface": "swd",
      "toolchainPrefix": "arm-none-eabi",
      "toolchainPath": "C:/Users/rinry/Tool/xpack-arm-none-eabi-gcc-15.2.1-1.1/bin"
    },
    {
      "name": "Debug M487 (OpenOCD Launch)",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "openocd",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/build/firmware.elf",
      "serverpath": "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/openocd.bat",
      "searchDir": [
        "D:/AiWorkSpace/M487_ScsiTool/tool/OpenOCD-Nuvoton/OpenOCD/scripts"
      ],
      "configFiles": [
        "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg"
      ],
      "runToEntryPoint": "main",
      "svdFile": "D:/AiWorkSpace/KM/M480BSP/Library/CMSIS/Device/Nuvoton/M480/Source/arm/M487.svd",
      "preLaunchTask": "Build M487 Firmware",
      "device": "M487JIDAE",
      "interface": "swd"
    },
    {
      "name": "OpenOCD Flash & Debug",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "openocd",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/build/firmware.elf",
      "serverpath": "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/openocd.bat",
      "searchDir": [
        "D:/AiWorkSpace/M487_ScsiTool/tool/OpenOCD-Nuvoton/OpenOCD/scripts"
      ],
      "configFiles": [
        "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg"
      ],
      "overrideLaunchCommands": [
        "monitor reset halt",
        "flash write_image erase ${workspaceFolder}/build/firmware.bin 0",
        "monitor reset halt",
        "load"
      ],
      "runToEntryPoint": "main",
      "svdFile": "D:/AiWorkSpace/KM/M480BSP/Library/CMSIS/Device/Nuvoton/M480/Source/arm/M487.svd",
      "preLaunchTask": "Build M487 Firmware"
    }
  ]
}
```

### 6.2 設定解釋

| 欄位 | 說明 |
|------|------|
| `servertype` | 使用 OpenOCD |
| `serverpath` | OpenOCD Wrapper 路徑（`openocd.bat`）|
| `configFiles` | OpenOCD 設定檔（`nulink_m487_ice.cfg`）|
| `executable` | 要 Debug 的 .elf 檔 |
| `runToEntryPoint` | 停在哪個函式（預設 main）|
| `svdFile` | 周邊暫存器視圖定義（M487.svd）|

---

## Step 7：第一次 Debug

### 7.1 確認硬體連接

```
M487 開發板
    ↓ USB
Nu-Link
    ↓ USB
電腦
```

### 7.2 Debug 流程

1. **確認 Nu-Link 已連接**
2. **確認韌體已編譯**（Ctrl+Shift+B）
3. **按 F5 或選單 → Debug → Start Debugging**
4. **第一次可能停在 main**，這是正常的

### 7.3 Debug 操作說明

| VSCode 功能 | 功能 |
|------------|------|
| **F9** | 設定/取消斷點 |
| **F10** | 單步執行（不進函式）|
| **F11** | 單步執行（進函式）|
| **Shift+F11** | 跳出函式 |
| **F5** | 繼續執行（到下一個斷點）|
| **F6** | 停止 Debug |

### 7.4 Debug 視窗介紹

| 視窗 | 用途 |
|------|------|
| **Call Stack** | 目前的函式呼叫堆疊 |
| **Variables** | 目前區域變數 |
| **Watch** | 自行加入要監看的變數 |
| **REGISTERS** | CPU 暫存器（R0-R15, XPSR...）|
| **Memory View** | 記憶體內容 |

---

## ✅ 網路佐證

這份文件的設定方式與以下**官方/權威資源**一致：

### 1. Nuvoton 官方 VSCode Extension

| Extension | VSCode Marketplace |
|-----------|------------------|
| Nuvoton.nuvoton-openocd | https://marketplace.visualstudio.com/items?itemName=Nuvoton.nuvoton-openocd |
| Nuvoton.nuvoton-numicro-cortex-m-pack | https://marketplace.visualstudio.com/items?itemName=Nuvoton.nuvoton-numicro-cortex-m-pack |

### 2. GitHub 實作範例

| Repo | 說明 |
|------|------|
| Ed-Yang/nu-cmake-example | https://github.com/Ed-Yang/nu-cmake-example |
| danchouzhou/Nuvoton-VScode-template | https://github.com/danchouzhou/Nuvoton-VScode-template |

### 3. 教學文章

| 主題 | 連結 |
|------|------|
| VSCode Cortex-Debug Launch Config | https://labs.dese.iisc.ac.in/embeddedlab/vscode-cortex-debug-launch-configurations/ |
| Debugging with VSCode | https://embedded-house.ghost.io/debugging-with-vscode/ |
| STM32 + VSCode + OpenOCD | https://community.st.com/t5/stm32-mcus/how-to-configure-stm32-vs-code-extension-to-use-openocd/ta-p/748562 |
| Cortex-Debug GitHub | https://github.com/Marus/cortex-debug |

### 4. 關鍵設定驗證

這些設定檔的欄位名稱（如 `servertype`、`serverpath`、`overrideLaunchCommands`）皆與 **Cortex-Debug 官方文件** 一致。

---

## ❓ 常見問題

### Q1：按 F5 沒反應

檢查：
- [ ] Nu-Link USB 有沒有插？
- [ ] OpenOCD 可以連線嗎？（`openocd.bat -c "adapter list"`）
- [ ] .elf 檔存在嗎？（先 `Ctrl+Shift+B` 編譯）

### Q2：LIBUSB_ERROR_ACCESS

這是驅動問題，見 Step 2 確認 WinUSB 驅動。**或使用 `openocd.bat` wrapper**（自動設定 MSYS2 DLL PATH）。

### Q3：停在 HardFault

正常，因為某些初始化在 main 之前。
按 **F5** 繼續到 main。

### Q4：Watch 視窗看不到變數

- 編譯時要加 `-g`（GCC 已有）
- 確認最佳化等級不是 `-O2` 或 `-O3`（設為 `-O0`）

### Q5：無法看記憶體

用 **Memory View**：
1. Debug 模式下
2. Debug Console 輸入：`memory read 0x20000000 0x100`

---

## 📁 完整目錄結構

```
D:\AiWorkSpace\M487_ScsiTool\
└── firmware\
    └── composite\
        ├── build\               ← 編譯產出（.elf, .bin）
        ├── .vscode\
        │   ├── c_cpp_properties.json   ← C/C++ 路徑設定
        │   ├── launch.json              ← Debug 設定
        │   └── tasks.json               ← 編譯任務
        ├── Makefile
        └── ...
└── tool\
    └── openocd\
        ├── openocd.bat              ← 🚀 OpenOCD Wrapper（含 DLL PATH）
        └── nulink_m487_ice.cfg      ← 🚀 OpenOCD 設定檔（已驗證）
```

---

## 🎉 恭喜完成！

現在你可以在 VSCode 中：
- ✅ **Ctrl+Shift+B**：編譯
- ✅ **F5**：燒錄 + Debug
- ✅ **F9**：設斷點
- ✅ **Watch 視窗**：看變數
- ✅ **REGISTERS 視窗**：看 CPU 暫存器

**快速上手見：[doc/ICE/QUICK_START.md](../ICE/QUICK_START.md)**
