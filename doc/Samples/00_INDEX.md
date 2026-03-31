# Samples — 範例コード ＆ 參考資料總覽

> 整理時間：2026-03-30  
> 位置：`D:\AiWorkSpace\M487_ScsiTool\doc\Samples\`

---

## ⚠️ 注意：部分資料夾為本地參考（未納入 Git）

由於 Windows 路徑長度限制（260 字元），以下資料夾**不在 Git 管理範圍內**，但**仍存在於本機磁碟**：

| 資料夾 | 說明 |
|--------|------|
| `Projects/` | Nuvoton 官方範例專案（57+ 個，HSUSBD_VENDOR_LBK 等）|
| `Library/` | CMSIS / Device / StdDriver / UsbHostLib 驅動程式庫 |
| `USB_HS_Samples/` | USB HS 範例集合（57+ 個）|
| `Nu-Link2-Bridge_Firmware/` | Nu-Link2 橋接韌體 |
| `Reference/UnitTest/` | 單元測試框架（Ceedling/GoogleTest/Unity）|

> 若需取用，請直接到對應資料夾中查找。

---

## 📁 Git 管理範圍內的目錄結構

```
doc/Samples/
├── 00_INDEX.md                    ← 本文件
│
├── HowTo/                        ← 📖 操作指南文件（在 Git 中）
│   ├── 01_ENV_SETUP.md          ← 環境設定
│   ├── 02_OPENOCD_FLASH.md       ← OpenOCD 燒錄（含新版設定）
│   ├── 03_KEIL_MDK_SETUP.md      ← Keil MDK 設定
│   ├── 04_USB_MSC_IMPLEMENT.md   ← USB MSC 實作
│   ├── 05_DRIVER_ISSUES.md       ← 驅動程式問題
│   ├── 06_TROUBLESHOOTING.md     ← 疑難排解
│   ├── 07_MEMORY_MAP.md          ← 記憶體對照表
│   ├── 08_GITHUB_REPOS.md        ← GitHub Repo 整理
│   └── UM_NuMaker-PFM-M487_User_Manual_EN_Rev1.01.pdf
│
└── Reference/                    ← 🔧 參考工具與資料（在 Git 中）
    ├── flash_backup/             ← ⚠️ 本地檔案（未納入 Git）
    │   ├── m487_flash.bin
    │   └── m487_flash_backup.bin
    └── tools/
        ├── read_m487_flash.tcl  ← OpenOCD Flash 讀取腳本
        └── write_m487_flash.tcl  ← OpenOCD Flash 燒錄腳本
```

---

## 🔥 Projects — 範例專案（本機磁碟）

> ⚠️ 路徑太長未納入 Git，請直接到 `doc/Samples/Projects/` 查找

### 與本專案最相關的範例

| 範例資料夾 | 用途 | 對應本專案 |
|-----------|------|-----------|
| `Projects/HSUSBD_VENDOR_LBK/` | USB VENDOR Command (LBK) | MSC Debug Channel (T025) |
| `Projects/Projects/HSUSBD_VENDOR_LBK/` | （同上，深層路徑）| |
| `Projects/USB_HS_Samples/USBD_Mass_Storage_Flash/` | MSC + SPI Flash | USB MSC RAM Disk |
| `Projects/USB_HS_Samples/USBD_HID_Transfer/` | USB HID Transfer | HID I2C Bridge (T001) |
| `Projects/USB_HS_Samples/USBD_HID_Transfer_And_MSC/` | HID + MSC 複合 | 參考 `firmware/composite/` |
| `Projects/Library/StdDriver/src/usbd.c` | USB Device Driver 參考 | USB 初始化 |
| `Projects/Library/StdDriver/src/scu.c` | System Control | Clock/PLL 設定 |

### VENDOR_LBK 參考價值最高

`Projects/HSUSBD_VENDOR_LBK/` 是最接近本專案架構的範例：
- USB VENDOR Command 處理
- Bulk-In / Bulk-Out 傳輸
- Device Descriptor 設定

**使用方式：** 直接複製 `HSUSBD_VENDOR_LBK/` 為基底修改。

---

## 📖 HowTo — 操作指南

| 文件 | 說明 |
|------|------|
| `01_ENV_SETUP.md` | M487 開發環境設定 |
| `02_OPENOCD_FLASH.md` | OpenOCD 燒錄教學 |
| `03_KEIL_MDK_SETUP.md` | Keil MDK 安裝與授權 |
| `04_USB_MSC_IMPLEMENT.md` | USB MSC 實作要點 |
| `05_DRIVER_ISSUES.md` | 常見驅動問題與解法 |
| `06_TROUBLESHOOTING.md` | 燒錄/除錯疑難排解 |
| `07_MEMORY_MAP.md` | M487 記憶體對照表 |
| `08_GITHUB_REPOS.md` | Nuvoton GitHub Repo 整理 |
| `09_GCC_BUILD_NOTES.md` | GCC 編譯範例注意事項（⚠️ Linker Script 陷阱）|

> ⚠️ `02_OPENOCD_FLASH.md` 中的 OpenOCD 路徑/命令可能已過時，請以 `doc/ICE/QUICK_START.md` 為準。

---

## 🔧 Reference — 參考工具

| 工具 | 路徑 | 說明 |
|------|------|------|
| Flash 備份 | `Reference/flash_backup/m487_flash.bin` | 完整 512KB Flash 映像檔 |
| OpenOCD 讀取腳本 | `Reference/tools/read_m487_flash.tcl` | TCL 批次讀取 Flash |
| OpenOCD 燒錄腳本 | `Reference/tools/write_m487_flash.tcl` | TCL 批次燒錄 Flash |

> ⚠️ `flash_backup/` 未納入 Git，如需使用請從原始位置 `doc/Samples/Reference/flash_backup/` 取用。

---

## 📌 與其他 doc 資料夾的關係

| 資料夾 | 關係 |
|--------|------|
| `doc/hardware/M487/` | M487 硬體/晶片手冊（晶片規格、工具設定、Flash 讀寫）|
| `doc/ICE/` | ICE 連線專用（OpenOCD + Nu-Link 突破方案）|
| `doc/NuLink/` | Nu-Link 驅動/使用經驗 |
| `doc/HID-over-I2C/` | HID-over-I²C 協定研究 |
| `doc/IDE_Setup/` | VSCode + OpenOCD + GDB 整合 |
| **`doc/Samples/`** | **範例程式碼 + How-To + 參考工具（本文件）** |

---

## 🔗 快速連結

| 需求 | 前往 |
|------|------|
| **MSC + VENDOR_LBK 參考** | `Projects/HSUSBD_VENDOR_LBK/`（本機磁碟）|
| **HID Transfer 參考** | `Projects/USB_HS_Samples/USBD_HID_Transfer/`（本機磁碟）|
| **StdDriver USB** | `Projects/Library/StdDriver/src/usbd.c`（本機磁碟）|
| **Flash 備份** | `Reference/flash_backup/m487_flash.bin` |
| **OpenOCD TCL** | `Reference/tools/read_m487_flash.tcl` |
| **環境設定 How-To** | `HowTo/01_ENV_SETUP.md` |
| **燒錄教學** | `HowTo/02_OPENOCD_FLASH.md` |

---

*最後更新：2026-03-30 15:28*
