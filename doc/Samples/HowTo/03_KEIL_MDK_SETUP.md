# Keil MDK 編譯流程

## ⚠️ 重要觀念：Keil 專案路徑問題

Keil 範例的 `.uvprojx` 使用**相對路徑**引用 BSP：

```
專案位置：  M480BSP\SampleCode\StdDriver\HSUSBD_VENDOR_LBK\KEIL\
           ..\..\..\..\Library\Device\Nuvoton\M480\Source\ARM\startup_M480.s
           ↑↑↑↑      = 往上 4 層 = 到 M480BSP\
```

這代表 BSP 必須放在 `M480BSP\` 根目錄，且專案必須在 `...\SampleCode\StdDriver\XXX\KEIL\` 結構下才能正確編譯。

---

## ✅ 正確編譯方式（直接用原範例路徑）

### 步驟 1：確認路徑結構

M480BSP 必須在 `D:\AiWorkSpace\KM\M480BSP\`（專案往上一層）

### 步驟 2：開啟 Keil 專案

直接在 Keil MDK GUI 中開啟：
```
D:\AiWorkSpace\KM\M480BSP\SampleCode\StdDriver\HSUSBD_Mass_Storage_ShortPacket\KEIL\HSUSBD_Mass_Storage_ShortPacket.uvprojx
```

### 步驟 3：設定目標晶片

1. `Project` → `Options for Target` → `Device`
2. 搜尋 `M487` → 選 `M487JIDAE`

### 步驟 4：安裝 Nuvoton M4 DFP（首次）

1. `Project` → `Manage` → `Pack Installer`
2. 左邊找 `Nuvoton` → `M4`
3. 找到 `NuMicro M4_DFP` → 點 `Install`

### 步驟 5：編譯燒錄

- `F7` 編譯
- `F8` 燒錄（需 Nu-Link 連接）

---

## 🔧 命令列編譯

### 基本編譯（UV4.exe）

```powershell
& "C:\Users\rinry\AppData\Local\Keil_v5\UV4\UV4.exe" -b "D:\AiWorkSpace\KM\M480BSP\SampleCode\StdDriver\HSUSBD_Mass_Storage_ShortPacket\KEIL\HSUSBD_Mass_Storage_ShortPacket.uvprojx"
```

參數：
- `-b` = Build（只編譯不進入 Debug）
- `-j0` = 不限制並行編譯數量

### 檢查編譯錯誤

開啟 `...\KEIL\obj\<專案名>.build_log.htm` 查看錯誤。

---

## ⚠️ 編譯錯誤：core_cm4.h not found

**原因**：Keil Pack 的 `NuMicroM4_DFP` 引用了 CMSIS 的 `core_cm4.h`，但路徑未設定。

**解決**：在 Pack Installer 安裝 `ARM::CMSIS` Pack（通常已預裝）。

**手動路徑加入**（若仍失敗）：
在 `Options for Target` → `C/C++` → `Include Paths` 加入：
```
C:\Users\rinry\AppData\Local\Arm\Packs\ARM\CMSIS\6.3.0\CMSIS\Core\Include
```

---

## ⚠️ 編譯錯誤：startup_M480.s / system_M480.c not found

**原因**：`uvprojx` 中的相對路徑 `../../../../Library/...` 解析失敗。

**解決**：
1. 確認 `M480BSP\` 位於正確位置（專案往上 4 層可達到）
2. 確認 `Library\Device\Nuvoton\M480\Source\` 完整存在

---

## ⚠️ 編譯錯誤：hsusbd_core.h / usbd_core.h not found

**原因**：`HSUSBD` 和 `USBD` 程式庫目錄未在 IncludePath 中。

**解決**：
在 `uvprojx` 的 `<IncludePath>` 加入：
```
C:\Users\rinry\AppData\Local\Arm\Packs\Nuvoton\NuMicroM4_DFP\1.0.1\Device\M480\Include
```

或從 M480BSP 的 `Library/USBD/` 和 `Library/HSUSBD/` 複製到專案目錄。

---

## 🔧 CMSIS 路徑問題（進階）

M480BSP 的 `Device\Nuvoton\M480\Include\M480.h` 會 `#include "core_cm4.h"`，但 IncludePath 中需要包含 CMSIS 路徑。

**標準 CMSIS 路徑**：
```
C:\Users\rinry\AppData\Local\Arm\Packs\ARM\CMSIS\6.3.0\CMSIS\Core\Include
```

---

## 📊 編譯速度參考

| 專案 | 大小 | 編譯耗時 |
|------|------|----------|
| HSUSBD_VENDOR_LBK | 16.1 KB | ~4 秒 |
| HSUSBD_Mass_Storage_ShortPacket | 16.9 KB | ~4 秒 |

---

## 📝 批次編譯腳本

建立 `D:\AiWorkSpace\KM\M487-HowTo\build.bat`：

```bat
@echo off
set KEIL_PATH=C:\Users\rinry\AppData\Local\Keil_v5\UV4\UV4.exe
set PROJECT=%1

if "%PROJECT%"=="" (
    echo Usage: build.bat ^<project.uvprojx^>
    exit /b 1
)

echo Building: %PROJECT%
"%KEIL_PATH%" -j0 -b "%PROJECT%"
echo Done.
pause
```

用法：`build.bat "D:\AiWorkSpace\KM\M480BSP\SampleCode\StdDriver\HSUSBD_Mass_Storage_ShortPacket\KEIL\HSUSBD_Mass_Storage_ShortPacket.uvprojx"`

---

## 📁 產出檔案位置

編譯後產出在 `...\KEIL\obj\` 目錄：

| 檔案 | 內容 |
|------|------|
| `<專案>.axf` | ELF/DWARF 格式（含除錯資訊） |
| `<專案>.bin` | 純二進制燒錄檔 |
| `<專案>.hex` | Intel HEX 格式 |
| `<專案>.txt` | 燒錄用文字檔（fromelf 產出） |
| `<專案>.build_log.htm` | 編譯日誌 |
