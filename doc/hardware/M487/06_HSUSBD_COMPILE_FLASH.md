# M487 HSUSBD_VENDOR_LBK 編譯與燒錄指南

## 📋 任務摘要

| 項目 | 內容 |
|------|------|
| 範例 | HSUSBD_VENDOR_LBK（USB High Speed Bulk Transfer） |
| 晶片 | M487JIDAE |
| 用途 | USB 2.0 High Speed Bulk Device 測試 |
| bin 大小 | 16.1 KB |
| Flash 燒錄速度 | ~5.9 KiB/s |

---

## 🔧 編譯流程

### 1. 準備 Junction

Keil 專案使用相對路徑 `../../../../Library/...` 引用 BSP。從 `HSUSBD_VENDOR_LBK\KEIL\` 往上四層需要有 `Library` 目錄。

```powershell
# 在 M487-Examples 根目錄建立 Junction 指向 M480BSP
New-Item -ItemType Junction -Path "D:\AiWorkSpace\KM\M487-Examples\Library" -Target "D:\AiWorkSpace\KM\M480BSP\Library"
```

> ⚠️ 注意：Windows Junction 在某些目錄下效能差，建議放在 `M487-Examples\` 根目錄而非專案資料夾內

### 2. 編譯

```powershell
& "C:\Users\rinry\AppData\Local\Keil_v5\UV4\UV4.exe" -b "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_VENDOR_LBK\HSUSBD_VENDOR_LBK\KEIL\HSUSBD_VENDOR_LBK.uvprojx"
```

### 3. 編譯產出

| 檔案 | 路徑 | 大小 |
|------|------|------|
| `.bin` | `...\KEIL\obj\HSUSBD_VENDOR_LBK.bin` | 16.1 KB |
| `.axf` | `...\KEIL\obj\HSUSBD_VENDOR_LBK.axf` | 98 KB |
| `.txt` | `...\KEIL\obj\HSUSBD_VENDOR_LBK.txt` | 燒錄用 |

---

## 🔌 燒錄流程

### 前置需求
- Nu-Link 驅動已安裝（Interface 1 為 WinUSB）
- Nuvoton OpenOCD 已就緒

### 燒錄指令

```powershell
& "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" `
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f interface/nulink.cfg `
  -f target/numicroM4.cfg `
  -c "init" `
  -c "reset halt" `
  -c "flash write_image erase D:/AiWorkSpace/KM/M487-Examples/Projects/HSUSBD_VENDOR_LBK/HSUSBD_VENDOR_LBK/KEIL/obj/HSUSBD_VENDOR_LBK.bin 0" `
  -c "shutdown"
```

### 批次檔（flash_hsusbd.bat）

位於：`D:\AiWorkSpace\KM\M487-Examples\Projects\flash_hsusbd.bat`

---

## 📂 完整目錄結構

```
D:\AiWorkSpace\KM\M487-Examples\
├── Library/                          # ← Junction → M480BSP\Library
├── Projects/
│   ├── HSUSBD_VENDOR_LBK/
│   │   ├── HSUSBD_VENDOR_LBK/       # 原始範例（含 KEIL/VSCode/GCC）
│   │   └── flash_hsusbd.bat         # 燒錄批次檔
│   └── flash_hsusbd.ps1             # PowerShell 版本
├── USB_HS_Samples/                  # 55 個 USB 範例
│   ├── HSUSBD_VENDOR_LBK/
│   ├── HSUSBD_VCOM_SerialEmulator/
│   ├── HSUSBD_Mass_Storage_*/
│   ├── ISP_HID/
│   └── Nu-Link2-Bridge_Firmware/
└── Nu-Link2-Bridge_Firmware/
```

---

## ⚙️ Junction 原理說明

Keil 專案 `.uvprojx` 中設定的相對路徑：

```
專案位置:  ...\HSUSBD_VENDOR_LBK\KEIL\HSUSBD_VENDOR_LBK.uvprojx
          ..\..\..\..\Library\Device\Nuvoton\M480\Source\ARM\
          ↑↑↑↑
          往上四層

所需路徑: ...\Library\Device\Nuvoton\M480\Source\ARM\startup_M480.s
```

從 `KEIL\` 往上四層剛好是 `M487-Examples\` 根目錄，所以在 `M487-Examples\Library` 建立 Junction，指向實際的 `M480BSP\Library`，路徑解析就能成功。

### 為什麼不直接複製？

- M480BSP 的 Library 有 600+ MB，複製耗時且佔空間
- Junction 保持單一資料來源，多個專案可共用
-缺點：Junction 在 VSCode 存取可能緩慢

---

## 🛠 疑難排解

| 問題 | 解決 |
|------|------|
| `core_cm4.h file not found` | 確認 Junction 存在且路徑正確 |
| 編譯慢（Junction） | 將 Library 改放至專案內（犧牲空間換速度） |
| `LIBUSB_ERROR_ACCESS` | 確認 Nu-Link Interface 1 為 WinUSB |
| 燒錄無反應 | 確認 M487 已連接 USB 且供電正常 |
| `couldn't open file` | 確認輸出路徑有寫入權限 |

---

## 📝 燒錄後驗證

燒錄完成後，確認燒入成功：

```powershell
# 讀回比對
& "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" `
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f interface/nulink.cfg -f target/numicroM4.cfg `
  -c "init" -c "reset halt" `
  -c "flash read_bank 0 C:/Users/rinry/hsusbd_verify.bin 0 0x1000" `
  -c "shutdown"

# 比對 MD5
(Get-FileHash "D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_VENDOR_LBK\HSUSBD_VENDOR_LBK\KEIL\obj\HSUSBD_VENDOR_LBK.bin" -Algorithm MD5).Hash
(Get-FileHash "C:\Users\rinry\hsusbd_verify.bin" -Algorithm MD5).Hash
```

---

*建立時間: 2026-03-26*
