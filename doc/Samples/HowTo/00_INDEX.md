# M487 USB 開發完全手冊

> 涵蓋驅動安裝、OpenOCD 燒錄、Keil MDK 編譯、USB MSC 實作
> 建立時間：2026-03-26

---

## 📁 目錄結構

```
D:\AiWorkSpace\KM\M487-HowTo\
├── 00_INDEX.md                    # 本文件（總索引）
├── 01_ENV_SETUP.md               # 環境建置（工具鏈、驅動）
├── 02_OPENOCD_FLASH.md            # OpenOCD 燒錄流程
├── 03_KEIL_MDK_SETUP.md          # Keil MDK 安裝與編譯
├── 04_USB_MSC_IMPLEMENT.md       # USB Mass Storage 實作
├── 05_DRIVER_ISSUES.md           # 驅動程式問題與解決
├── 06_TROUBLESHOOTING.md         # 疑難排解
├── 07_MEMORY_MAP.md              # M487 記憶體對照表
└── 08_GITHUB_REPOS.md           # GitHub Repo 整理
```

---

## 🎯 完成功能

| 功能 | 狀態 | 日期 |
|------|------|------|
| OpenOCD + Nu-Link 燒錄 | ✅ | 2026-03-26 |
| M487 Flash 備份/還原 | ✅ | 2026-03-26 |
| HSUSBD_VENDOR_LBK 燒錄 | ✅ | 2026-03-26 |
| HSUSBD_Mass_Storage 燒錄 | ✅ | 2026-03-26 |
| RAM Disk（檔案讀寫）| ✅ | 2026-03-26 |
| Write Protect 功能 | ⚠️ 待啟用 | — |

---

## 📋 硬體資訊

| 項目 | 內容 |
|------|------|
| 晶片 | M487JIDAE |
| Flash | 512KB APROM |
| SRAM | 160KB |
| SWD IDCODE | 0x2BA01477 |
| USB VID:PID | 0x0416:0xFF20 (VENDOR_LBK), 0x0416:0x511C (Nu-Link) |
