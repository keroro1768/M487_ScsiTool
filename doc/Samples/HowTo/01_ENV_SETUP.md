# 環境建置

## 📦 需要安裝的軟體

### 1. Nuvoton OpenOCD（燒錄用）

| 項目 | 內容 |
|------|------|
| **下載** | https://github.com/OpenNuvoton/OpenOCD-Nuvoton/releases |
| **版本** | v1.02.029r |
| **位置** | `C:\Users\rinry\Tool\OpenOCD-Nuvoton\` |
| **用途** | 透過 Nu-Link 燒錄/讀取 Flash |

### 2. Keil MDK Nuvoton Edition（編譯用）

| 項目 | 內容 |
|------|------|
| **下載** | https://www.keil.arm.com/demo/eval/arm.htm |
| **License** | Nuvoton 免費 License（有效期至 2027/9/25） |
| **申請** | https://www.nuvoton.com/tool-and-software/ide-and-compiler/keil-mdk-nuvoton-edition/application-form/ |
| **位置** | `C:\Users\rinry\AppData\Local\Keil_v5\` |

### 3. Nuvoton Nu-Link Keil Driver

| 項目 | 內容 |
|------|------|
| **下載** | https://www.nuvoton.com/tool-and-software/ide-and-compiler/ （找 `Nu-Link_Keil_Driver_V3.22.7946r`） |
| **用途** | 讓 Keil MDK 能辨識 Nu-Link 調試器 |
| **備選** | Zadig (`C:\Users\rinry\Tool\zadig-2.9.exe`) |

### 4. Nuvoton BSP（M480 Series）

| 項目 | 內容 |
|------|------|
| **Clone** | `git clone https://github.com/OpenNuvoton/M480BSP.git D:\AiWorkSpace\KM\M480BSP` |
| **內容** | 完整驅動程式庫、範例程式、Header 檔 |
| **大小** | ~646MB |

---

## 🔌 Nu-Link USB 驅動狀態

M487 開發板的 Nu-Link 有兩個 USB 介面：

| 介面 | VID:PID | 驅動 | 用途 |
|------|---------|------|------|
| Interface 0 | 0416:511C | HID（系統內建） | 系統識別用 |
| Interface 1 | 0416:511C | WinUSB (Nuvoton) | SWD 調試/燒錄 |

檢查方式：
```powershell
Get-PnpDevice | Where-Object { $_.DeviceID -like '*0416*511C*' } | Format-Table FriendlyName, Status
```

預期輸出：
```
HID-compliant device              OK   HID\VID_0416&PID_511C&MI_00\...
Nuvoton Nu-Link USB              OK   USB\VID_0416&PID_511C&MI_01\...
```

---

## 🔧 驅動安裝（Zadig 方式）

當 Nu-Link 無法被 OpenOCD 識別時，用 Zadig 更換驅動：

1. 以**管理員身份**執行 `C:\Users\rinry\Tool\zadig-2.9.exe`
2. `Device` → `Load Preset Device` → 選 `C:\Users\rinry\Tool\NULink.preset`
3. 選 `WinUSB` → 點 `Replace Driver`

### 驅動預設檔（NULink.preset）

```ini
[Preset Device]
Name=Nuvoton Nu-Link CMSIS-DAP
VID=0x0416
PID=0x511C
MI=00
Driver=WinUSB
```

---

## ⚠️ 常見驅動問題

| 問題 | 原因 | 解決 |
|------|------|------|
| LIBUSB_ERROR_ACCESS | 驅動為 HID | 用 Zadig 換成 WinUSB |
| LIBUSB_ERROR_NOT_FOUND | Nu-Link 未連接 | 重新插拔 USB |
| 燒錄時無法找到晶片 | Interface 0/1 混淆 | 確認燒錄走 Interface 1 |

---

## 📂 工具路徑總整理

| 工具 | 路徑 |
|------|------|
| Nuvoton OpenOCD | `C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe` |
| Keil UV4 | `C:\Users\rinry\AppData\Local\Keil_v5\UV4\UV4.exe` |
| Keil PackInstaller | `C:\Users\rinry\AppData\Local\Keil_v5\UV4\PackInstaller.exe` |
| Zadig | `C:\Users\rinry\Tool\zadig-2.9.exe` |
| M480BSP | `D:\AiWorkSpace\KM\M480BSP\` |
| Flash 備份 | `D:\AiWorkSpace\KM\M487\flash_backup\m487_flash.bin` |
