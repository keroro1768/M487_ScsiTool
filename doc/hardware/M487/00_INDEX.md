# M487JIDAE 燒錄工具與環境

## 📁 目錄結構

```
D:\AiWorkSpace\KM\M487\
├── M480BSP/                     # Nuvoton M480 BSP (含 M487 驅動與範例)
│   ├── Library/StdDriver/src/   # 標準驅動程式
│   ├── SampleCode/              # 範例程式
│   └── Document/
├── flash_backup/
│   ├── m487_flash.bin           # 完整 Flash 備份 (512KB)
│   └── m487_flash_backup.bin    # 備份副本
├── tools/
│   ├── read_m487_flash.tcl      # Flash 讀取腳本
│   └── write_m487_flash.tcl     # Flash 燒錄腳本
├── 00_INDEX.md                  # 本文件
├── 01_SPEC.md                   # M487 晶片規格
├── 02_TOOLS.md                  # 工具安裝與設定
├── 03_FLASH_READ_WRITE.md       # Flash 讀寫流程
├── 04_KEIL_SETUP.md             # Keil MDK 燒錄指南
├── 05_OPENOCD_REF.md            # OpenOCD 常用指令
├── 06_HSUSBD_COMPILE_FLASH.md   # HSUSBD_VENDOR_LBK 編譯與燒錄
└── datasheet_download.md         # Datasheet 下載說明
```

---

## 📥 M480BSP (作為 Git Submodule)

> 注意：`M480BSP/` 為獨立的 Git repo，請用以下方式加入：
> ```bash
> cd Hardware/M487
> git submodule add https://github.com/OpenNuvoton/M480BSP.git M480BSP
> ```

| 項目 | 內容 |
|------|------|
| **位置** | `D:\AiWorkSpace\KM\M487\M480BSP\` |
| **Clone URL** | https://github.com/OpenNuvoton/M480BSP.git |

---

## ⚙️ 環境狀態 (2026-03-26)

| 項目                | 狀態      | 備註                                    |
| ----------------- | ------- | ------------------------------------- |
| OpenOCD           | ✅ 已設定   | Nu-Link 可被識別                          |
| M480BSP           | ✅ 已下載   | `D:\AiWorkSpace\KM\M487\M480BSP\`     |
| Keil MDK          | ✅ 已設定   | License: Caro Lin, 有效期至 2034/9/27     |
| Nu-Link 驅動        | ✅ 已安裝   | Interface 1: WinUSB (Nuvoton)         |
| Flash 備份          | ✅ 已完成   | `m487_flash.bin` 512KB, MD5: F8618AA1 |
| HSUSBD_VENDOR_LBK | ✅ 已編譯燒錄 | 16.1 KB                               |
