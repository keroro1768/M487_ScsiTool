# Documents — 文件總覽

> 整理時間：2026-03-30

---

## 📁 總目錄

| 資料夾 | 主題 | 文件數 |
|--------|------|-------|
| **`Samples/`** | 🔥 **範例程式碼 ＋ How-To ＋ 參考資料（整理）** | 8+ |
| `ICE/` | 🔥 **M487 ICE 連線（最新）** | 6 |
| `HID-over-I2C/` | HID-over-I²C 橋接協定 | 7 |
| `NuLink/` | Nu-Link 燒錄器使用經驗 | 1 |
| `M487/` | M487 晶片使用方法 | 1 |
| `hardware/M487/` | M487 硬體/燒錄工具/環境 | 11 |
| `hardware/ARM-Cortex-M4/` | ARM Cortex-M4 核心知識 | 2 |
| `hardware/I3C-USB-Bridge/` | I3C 橋接研究 | 11 |
| `IDE_Setup/` | VSCode + OpenOCD + GDB 整合 | 1 |
| `DWT/` | DWT Debug 研究 | 1 |
| `GDB_RSP/` | GDB RSP 研究 | 1 |
| `design/UI-UX-Design/` | UI/UX 設計文件 | 5 |
| `USB_Driver_Samples/` | USB Driver Samples 研究 | 1 |

---

## 🔥 Samples/ — 範例程式碼 ＋ 參考資料

**主題：** Nuvoton 官方範例程式碼、How-To 操作指南、參考工具

| 子資料夾 | 說明 |
|---------|------|
| `Projects/` | 🔥 Nuvoton 官方範例（57+ 專案，含 MSC/HID/VENDOR_LBK）|
| `Library/` | CMSIS / Device / StdDriver / UsbHostLib |
| `USB_HS_Samples/` | USB HS 範例集合 |
| `Nu-Link2-Bridge_Firmware/` | Nu-Link2 橋接韌體 |
| `HowTo/` | 8 篇操作指南（環境設定/OpenOCD/燒錄/驅動）|
| `Reference/` | Flash 備份、OpenOCD TCL 腳本、單元測試 |

**關鍵範例：**
- `Projects/HSUSBD_VENDOR_LBK/` — **最重要參考**（USB VENDOR Command + MSC Debug Channel）
- `Projects/USB_HS_Samples/USBD_Mass_Storage_Flash/` — MSC + SPI Flash
- `Projects/USB_HS_Samples/USBD_HID_Transfer/` — HID Transfer

**🚀 立即使用：** `Projects/HSUSBD_VENDOR_LBK/` 可直接複製修改作為本專案基底

**📖 索引文件：** [doc/Samples/00_INDEX.md](Samples/00_INDEX.md)

---

## 🔥 ICE/ — M487 ICE 連線（最新完成）

**主題：** Nu-Link + OpenOCD 連線問題的完整解決方案

| 文件 | 說明 |
|------|------|
| **[QUICK_START.md](ICE/QUICK_START.md)** | 🚀 **快速上手** — 新手必讀！燒錄 + Debug + 驗證 |
| [00_INDEX.md](ICE/00_INDEX.md) | **文檔索引**，含所有文件連結 |
| [01_PROBLEM.md](ICE/01_PROBLEM.md) | 問題分析：LIBUSB_ERROR_ACCESS、CMSIS-DAP vs NULINK Protocol |
| [02_SOLUTION.md](ICE/02_SOLUTION.md) | 解決方案：成功配置與快速啟動指南 |
| [03_VSCODE.md](ICE/03_VSCODE.md) | VSCode Debug 整合：launch.json 完整設定 |
| [04_VERIFICATION.md](ICE/04_VERIFICATION.md) | 驗證方法：連線測試與預期輸出 |

**關鍵產出：**
- `tool/openocd/nulink_m487_ice.cfg` — OpenOCD 設定檔
- `tool/openocd/openocd.bat` — DLL 環境 wrapper
- `firmware/composite/.vscode/launch.json` — VSCode Debug 設定

**🚀 立即使用：** VSCode 中 F5 即可 Debug M487，見 [快速上手指南](ICE/QUICK_START.md)

---

## HID-over-I2C/ — HID-over-I²C 橋接協定（已完成）

**主題：** M487 作為 USB ↔ I²C 橋接，支援 HID-over-I²C 協定

### 架構

```
PC (USB HID Host) ←USB→ M487 Bridge ←I²C→ HID-over-I²C Device
```

### 文件索引

| 文件 | 說明 |
|------|------|
| [README.md](HID-over-I2C/README.md) | **文件索引**，含實作 Roadmap |
| [01-Spec/SPEC.md](HID-over-I2C/01-Spec/SPEC.md) | HID-over-I²C Protocol Specification（from Microsoft v1.0）|
| [02-Architecture/ARCHITECTURE.md](HID-over-I2C/02-Architecture/ARCHITECTURE.md) | M487 Bridge 架構設計 |
| [03-Protocol/TRANSLATION.md](HID-over-I2C/03-Protocol/TRANSLATION.md) | USB ↔ I²C 翻譯層協定細節 |
| [04-Registers/REGISTERS.md](HID-over-I2C/04-Registers/REGISTERS.md) | 韌體暫存器定義與資料結構 |
| [05-Example/EXAMPLE.md](HID-over-I2C/05-Example/EXAMPLE.md) | 實例：MLX90614 IR Sensor 整合 |
| [06-TestPlan/TEST_PLAN.md](HID-over-I2C/06-TestPlan/TEST_PLAN.md) | 測試策略與測試案例 |
| [07-Review/REVIEW.md](HID-over-I2C/07-Review/REVIEW.md) | Review 檢查清單、報告模板與流程 |

### 實作狀態

| Phase | 內容 | 狀態 |
|-------|------|------|
| Phase 1 | I²C driver（poll mode, 基本讀寫）| ✅ 完成 |
| Phase 2 | HID Descriptor parser | ✅ 完成 |
| Phase 3 | USB HID device layer（列舉, EP0/1/2）| ✅ 完成 |
| Phase 4 | Translation layer（命令建立/回應解析）| ✅ 完成 |
| Phase 5 | 整合與測試 | ✅ 編譯成功（43.3KB），USB HID 為 Stub |

**⚠️ 注意：** `firmware/hid-over-i2c/src/usb_hid.c` 目前為 Stub，真正 USB HID 在 `firmware/composite/hid_i2c.c`

---

## NuLink/ — Nu-Link 燒錄器使用經驗

**主題：** Nu-Link 驅動、驅動程式架構、 LIBUSB_ERROR_ACCESS 解法

| 文件 | 說明 |
|------|------|
| [NuLink_Experience_Compilation.md](NuLink/NuLink_Experience_Compilation.md) | Nu-Link 使用經驗彙整：驅動架構、驅動修復、OpenOCD 設定 |

**內容涵蓋：**
- Nu-Link 型號對照（Nu-Link / Nu-Link2-Pro / Nu-Link3-Pro）
- USB Interface 架構（MI_00 HID / MI_01 WinUSB）
- Zadig 安裝 WinUSB 驅動流程
- OpenOCD + Nu-Link 驅動設定
- 常見錯誤排除

---

## M487/ — M487 晶片使用方法

**主題：** M487 晶片的正確使用方式（相對於 HID-over-I²C 實作）

| 文件 | 說明 |
|------|------|
| [M487_Correct_Usage_Guide.md](M487/M487_Correct_Usage_Guide.md) | M487 晶片正確使用方法指引 |

---

## hardware/M487/ — M487 硬體/燒錄工具/環境

**主題：** M487JIDAE 燒錄工具、環境設定、Flash 讀寫

### 文件索引

| 文件 | 說明 |
|------|------|
| [00_INDEX.md](hardware/M487/00_INDEX.md) | **目錄索引** |
| [01_SPEC.md](hardware/M487/01_SPEC.md) | M487 晶片規格 |
| [02_TOOLS.md](hardware/M487/02_TOOLS.md) | 工具安裝與設定 |
| [03_FLASH_READ_WRITE.md](hardware/M487/03_FLASH_READ_WRITE.md) | Flash 讀寫流程 |
| [04_KEIL_SETUP.md](hardware/M487/04_KEIL_SETUP.md) | Keil MDK 燒錄指南 |
| [05_OPENOCD_REF.md](hardware/M487/05_OPENOCD_REF.md) | OpenOCD 常用指令 |
| [06_HSUSBD_COMPILE_FLASH.md](hardware/M487/06_HSUSBD_COMPILE_FLASH.md) | HSUSBD_VENDOR_LBK 編譯與燒錄 |
| [datasheet_download.md](hardware/M487/datasheet_download.md) | Datasheet 下載說明 |
| [M487-resources.md](hardware/M487/M487-resources.md) | M487 資源總覽 |
| [tools/read_m487_flash.tcl](hardware/M487/tools/read_m487_flash.tcl) | OpenOCD Flash 讀取腳本 |
| [tools/write_m487_flash.tcl](hardware/M487/tools/tools/write_m487_flash.tcl) | OpenOCD Flash 燒錄腳本 |
| [flash_backup/m487_flash.bin](hardware/M487/flash_backup/m487_flash.bin) | 完整 Flash 備份（512KB）|

### 環境狀態

| 項目 | 狀態 | 備註 |
|------|------|------|
| OpenOCD | ✅ 已設定 | Nu-Link 可被識別 |
| M480BSP | ✅ 已下載 | `D:\AiWorkSpace\KM\M487\M480BSP\` |
| Keil MDK | ✅ 已設定 | License: Caro Lin, 有效期至 2034/9/27 |
| Nu-Link 驅動 | ✅ 已安裝 | Interface 1: WinUSB (Nuvoton) |
| Flash 備份 | ✅ 已完成 | `m487_flash.bin` 512KB, MD5: F8618AA1 |
| HSUSBD_VENDOR_LBK | ✅ 已編譯燒錄 | 16.1 KB |

---

## hardware/ARM-Cortex-M4/ — ARM Cortex-M4 核心知識

**主題：** ARM Cortex-M4 核心、CMSIS-DAP 協定、Flash 程式設計安全

| 文件 | 說明 |
|------|------|
| [cmsis-dap-spec.md](hardware/ARM-Cortex-M4/cmsis-dap-spec.md) | CMSIS-DAP 協定規格 |
| [flash-programming-security.md](hardware/ARM-Cortex-M4/flash-programming-security.md) | Flash 程式設計安全機制 |

---

## hardware/I3C-USB-Bridge/ — I3C 橋接研究

**主題：** I3C 協定、MIPI I3C Basic、Microsoft HID-over-I2C 規格

### 文件索引

| 文件 | 說明 |
|------|------|
| [01-I3C-Protocol-Research.md](hardware/I3C-USB-Bridge/01-I3C-Protocol-Research.md) | I3C 協定研究 |
| [02-Task-Tracking.md](hardware/I3C-USB-Bridge/02-Task-Tracking.md) | 任務追蹤 |
| [HID-over-I3C-Speculation.md](hardware/I3C-USB-Bridge/HID-over-I3C-Speculation.md) | HID-over-I3C 推測 |
| [I3C-Host-Client-Development.md](hardware/I3C-USB-Bridge/I3C-Host-Client-Development.md) | I3C Host/Client 開發 |
| [I3C-Hot-Join-Tutorial.md](hardware/I3C-USB-Bridge/I3C-Hot-Join-Tutorial.md) | I3C Hot Join 教學 |
| [Microsoft-HID-over-I2C.md](hardware/I3C-USB-Bridge/Microsoft-HID-over-I2C.md) | Microsoft HID-over-I2C 摘要 |
| [Microsoft-HID-over-I2C-spec.md](hardware/I3C-USB-Bridge/Microsoft-HID-over-I2C-spec.md) | HID-over-I2C 規格摘要 |
| [Microsoft-HID-over-I2C-spec-full.md](hardware/I3C-USB-Bridge/Microsoft-HID-over-I2C-spec-full.md) | HID-over-I2C 完整規格 |
| [MIPI-DisCo-for-I3C.md](hardware/I3C-USB-Bridge/MIPI-DisCo-for-I3C.md) | MIPI DisCo for I3C |
| [MIPI-I3C-Basic-v1.2.md](hardware/I3C-USB-Bridge/MIPI-I3C-Basic-v1.2.md) | MIPI I3C Basic v1.2 規格 |
| [WinDbg-commands.md](hardware/I3C-USB-Bridge/WinDbg-commands.md) | WinDbg 命令 |
| [WinDbg-complete-guide.md](hardware/I3C-USB-Bridge/WinDbg-complete-guide.md) | WinDbg 完整指南 |

---

## IDE_Setup/ — VSCode + OpenOCD + GDB 整合

**主題：** 在 VSCode 中設定 M487 開發環境（編譯/燒錄/Debug）

| 文件 | 說明 |
|------|------|
| [VSCode_OpenOCD_GDB_M487_IDE_Setup.md](IDE_Setup/VSCode_OpenOCD_GDB_M487_IDE_Setup.md) | **完整 IDE 設定指南**：VSCode + OpenOCD + GCC + GDB，含 launch.json、tasks.json、c_cpp_properties.json |

**涵蓋內容：**
- 安裝必要軟體（VSCode, Cortex-Debug, GCC ARM Toolchain）
- Nu-Link 驅動確認
- OpenOCD 燒錄設定
- VSCode Debug（F5）完整設定
- 常見問題排除

---

## DWT/ — DWT Debug 研究

**主題：** ARM CoreSight DWT（Data Watchpoint and Trace）研究

| 文件 | 說明 |
|------|------|
| [T034_RESEARCH.md](DWT/T034_RESEARCH.md) | DWT Framework 研究（Task T034）|

---

## GDB_RSP/ — GDB RSP 研究

**主題：** GDB Remote Serial Protocol，M487 MSC Debug Channel 自-hosted Debug

| 文件 | 說明 |
|------|------|
| [T033_RESEARCH.md](GDB_RSP/T033_RESEARCH.md) | GDB RSP Server 研究（Task T033）|

---

## design/UI-UX-Design/ — UI/UX 設計文件

**主題：** 桌面應用程式 UI/UX 設計原則與實務

### 文件索引

| 文件 | 說明 |
|------|------|
| [README.md](design/UI-UX-Design/README.md) | UI/UX 設計文件總覽 |
| [Desktop-App-Layout/desktop-layout-principles.md](design/UI-UX-Design/Desktop-App-Layout/desktop-layout-principles.md) | 桌面應用程式版面原則 |
| [UI-UX-Principles/ui-ux-principles.md](design/UI-UX-Design/UI-UX-Principles/ui-ux-principles.md) | UI/UX 設計原則 |
| [Windows-Forms/winforms-best-practices-2024.md](design/UI-UX-Design/Windows-Forms/winforms-best-practices-2024.md) | WinForms 2024 最佳實踐 |
| [Windows-Forms/winforms-custom-controls.md](design/UI-UX-Design/Windows-Forms/winforms-custom-controls.md) | WinForms 自訂控制項 |
| [Windows-Forms/winforms-layout-complete.md](design/UI-UX-Design/Windows-Forms/winforms-layout-complete.md) | WinForms 版面完整指南 |

---

## USB_Driver_Samples/ — USB Driver Samples 研究

**主題：** USB Driver Samples（M487 USB 驅動範例）

| 文件 | 說明 |
|------|------|
| [README.md](USB_Driver_Samples/README.md) | USB Driver Samples 說明 |

---

## 📌 主題快速索引

| 需求 | 前往 |
|------|------|
| **🚀 第一次使用（新手上路）** | [ICE/QUICK_START.md](ICE/QUICK_START.md) |
| **🚀 VSCode Debug M487（F5）** | [ICE/QUICK_START.md](ICE/QUICK_START.md) |
| **🔧 ICE 連線問題解決** | [ICE/01_PROBLEM.md](ICE/01_PROBLEM.md) |
| **🔌 HID-over-I²C 協定** | [HID-over-I2C/01-Spec/SPEC.md](HID-over-I2C/01-Spec/SPEC.md) |
| **🔧 Nu-Link 驅動問題** | [NuLink/NuLink_Experience_Compilation.md](NuLink/NuLink_Experience_Compilation.md) |
| **⚙️ M487 燒錄工具設定** | [hardware/M487/02_TOOLS.md](hardware/M487/02_TOOLS.md) |
| **💻 VSCode IDE 完整設定** | [IDE_Setup/VSCode_OpenOCD_GDB_M487_IDE_Setup.md](IDE_Setup/VSCode_OpenOCD_GDB_M487_IDE_Setup.md) |
| **🔬 DWT Debug** | [DWT/T034_RESEARCH.md](DWT/T034_RESEARCH.md) |
| **🖥️ GDB RSP 自-debug** | [GDB_RSP/T033_RESEARCH.md](GDB_RSP/T033_RESEARCH.md) |
| **📐 UI/UX 設計** | [design/UI-UX-Design/README.md](design/UI-UX-Design/README.md) |

---

*最後更新：2026-03-30 11:33*
