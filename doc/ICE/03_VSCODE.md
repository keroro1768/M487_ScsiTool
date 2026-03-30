# VSCode Debug 整合設定

> 日期：2026-03-30  
> 狀態：✅ 已完成

---

## 📁 設定檔位置

| 檔案 | 路徑 |
|------|------|
| **launch.json** | `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\.vscode\launch.json` |
| **OpenOCD Config** | `D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg` |
| **OpenOCD Wrapper** | `D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat` |
| **tasks.json** | `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\.vscode\tasks.json` |

---

## 🎯 三種 Debug 模式

### 1. Debug M487 (OpenOCD Attach) — 常用

附加到已燒錄的晶片，適合線上除錯。

```json
{
  "name": "Debug M487 (OpenOCD Attach)",
  "type": "cortex-debug",
  "request": "attach",
  "servertype": "openocd",
  "executable": "${workspaceFolder}/build/firmware.elf",
  "serverpath": "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/openocd.bat",
  "configFiles": [
    "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg"
  ],
  "overrideLaunchCommands": [
    "monitor reset halt",
    "load"
  ],
  "runToEntryPoint": "main"
}
```

**流程：** 燒錄 → F5 → 停在 main

---

### 2. Debug M487 (OpenOCD Launch)

直接啟動 Debug，不先燒錄。

```json
{
  "name": "Debug M487 (OpenOCD Launch)",
  "type": "cortex-debug",
  "request": "launch",
  "servertype": "openocd",
  "executable": "${workspaceFolder}/build/firmware.elf",
  "serverpath": "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/openocd.bat",
  "configFiles": [
    "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg"
  ],
  "runToEntryPoint": "main"
}
```

**流程：** F5 → 連線到現有韌體 → 停在 main

---

### 3. OpenOCD Flash & Debug — 燒錄 + Debug

一次完成燒錄和除錯。

```json
{
  "name": "OpenOCD Flash & Debug",
  "type": "cortex-debug",
  "request": "launch",
  "servertype": "openocd",
  "executable": "${workspaceFolder}/build/firmware.elf",
  "serverpath": "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/openocd.bat",
  "configFiles": [
    "D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg"
  ],
  "overrideLaunchCommands": [
    "monitor reset halt",
    "flash write_image erase ${workspaceFolder}/build/firmware.bin 0",
    "monitor reset halt",
    "load"
  ],
  "runToEntryPoint": "main"
}
```

**流程：** F5 → 自動燒錄 → 停在 main

---

## 🔧 工作流程

### 第一次設定

1. 確認 Nu-Link 已連接 USB
2. 在 VSCode 開啟 `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\`
3. 編譯：`Ctrl+Shift+B`
4. 選擇 Debug 模式（見下圖）
5. 按 **F5** 開始

### Debug 操作

| 按鍵 | 功能 |
|------|------|
| **F5** | 繼續執行（到下一個斷點）|
| **F9** | 設定/取消斷點 |
| **F10** | 單步執行（不進函式）|
| **F11** | 單步執行（進函式）|
| **Shift+F11** | 跳出函式 |
| **F6** | 停止 Debug |

### Debug 視窗

| 視窗 | 用途 |
|------|------|
| **Call Stack** | 目前函式呼叫堆疊 |
| **Variables** | 區域變數 |
| **Watch** | 自訂監看變數 |
| **REGISTERS** | CPU 暫存器（R0-R15, XPSR...）|
| **Memory View** | 記憶體內容 |

---

## ⚠️ 已知問題

### Q: F5 沒反應

檢查：
- [ ] Nu-Link USB 已插上
- [ ] `openocd.bat` 路徑正確
- [ ] `nulink_m487_ice.cfg` 存在
- [ ] 韌體已編譯（`build/firmware.elf` 存在）

### Q: 停在 HardFault

正常，某些初始化在 main 之前。按 **F5** 繼續到 main。

### Q: Watch 視窗看不到變數

確認編譯有加 `-g`（GCC 預設有），且最佳化設為 `-O0`。

### Q: 無法燒錄

確認路徑中的 `firmware.bin` 存在，或在 `tasks.json` 中確認編譯產出路徑。

---

## 📝 完整 launch.json

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

---

*最後更新：2026-03-30 11:30*
