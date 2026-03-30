# 工具安裝與設定

## 已安裝工具

### OpenOCD（已驗證可用）

| 項目 | 內容 |
|------|------|
| **位置** | `D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin\openocd.exe` |
| **版本** | 0.12.0+dev-02429-ge4c49d860 (2026-03-25) |
| **支援晶片** | Nuvoton NuMicro M4 系列 |
| **支援調試器** | Nu-Link（HLA driver）、J-Link、ST-Link、CMSIS-DAP |
| **Wrapper** | `tool\openocd\openocd.bat`（含 MSYS2 DLL）|

### Keil MDK

| 項目 | 內容 |
|------|------|
| **位置** | `C:\Users\rinry\AppData\Local\Keil_v5\` |
| **版本** | v5.x |
| **License** | 需申請 (Nuvoton 免費 Edition) |

### Nu-Link USB 驅動

| 介面 | 驅動類型 | INF 檔 |
|------|----------|--------|
| Interface 0 | HID (系統內建) | input.inf |
| Interface 1 | WinUSB (Nuvoton) | oem132.inf |

---

## OpenOCD 設定

> ⚠️ **2026-03-30 更新**：請使用 `openocd-build` 而非 `OpenOCD-Nuvoton` 內的 binary

### 設定檔位置

| 檔案 | 路徑 |
|------|------|
| OpenOCD Binary | `D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin\openocd.exe` |
| OpenOCD Wrapper | `D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat`（推薦）|
| OpenOCD Scripts | `D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD-Nuvoton\OpenOCD\scripts\` |
| M487 Config | `D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg`（已驗證）|

### 環境變數（使用 Wrapper 即可）

```powershell
# 不需要手動設定 PATH，使用 openocd.bat wrapper
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -c "adapter list"
```

---

## USB 驅動狀態確認

```powershell
# 檢查 Nu-Link USB 狀態
Get-PnpDevice | Where-Object { $_.DeviceID -like '*0416*511C*' } |
  Format-Table FriendlyName, Status, InstanceId
```

預期輸出:
```
HID-compliant device              OK   HID\VID_0416&PID_511C&MI_00\...
Nuvoton Nu-Link USB              OK   USB\VID_0416&PID_511C&MI_01\...
USB Composite Device             OK   USB\VID_0416&PID_511C\...
```

---

## 燒錄速度優化

### 提高 SWD 時脈

預設 1000 kHz，可提高到 4000 kHz:

```tcl
# 在 OpenOCD 指令列加入
-c "adapter speed 4000"
```

### 燒錄速度比較

| 方式 | 速度 |
|------|------|
| OpenOCD + SWD | ~7.4 KiB/s |
| Nuvoton ISP Tool (UART) | 更快 |

---

## TCL 腳本

### 讀取 Flash 腳本
- 位置: `C:\Users\rinry\Tool\read_m487_flash.tcl`
- 功能: 分 16 區塊讀取，顯示進度
- 用法: `-f read_m487_flash.tcl`

### 燒錄 Flash 腳本
- 位置: `D:\AiWorkSpace\KM\M487\scripts\write_flash.tcl`
- 功能: 燒錄並驗證

---

## 工具鏈總結

```
開發 → 編譯 → 燒錄 → 調試
  │       │       │       │
  ▼       ▼       ▼       ▼
NuEclipse  GCC    OpenOCD  OpenOCD
Keil MDK   ARMCC   Nu-Link  GDB
```
