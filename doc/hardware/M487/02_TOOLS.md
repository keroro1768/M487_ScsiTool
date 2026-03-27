# 工具安裝與設定

## 已安裝工具

### OpenOCD-Nuvoton

| 項目 | 內容 |
|------|------|
| **位置** | `C:\Users\rinry\Tool\OpenOCD-Nuvoton\` |
| **版本** | 0.10.022.0-dev-00493 |
| **支援晶片** | Nuvoton NuMicro M4 系列 |
| **支援調試器** | Nu-Link, J-Link, CMSIS-DAP |

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

### 設定檔位置

| 檔案 | 路徑 |
|------|------|
| Nu-Link interface | `C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts\interface\nulink.cfg` |
| M4 target | `C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts\target\numicroM4.cfg` |

### 環境變數 (可選)

建議將 OpenOCD 加入 PATH:
```powershell
$env:PATH += ";C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin"
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
