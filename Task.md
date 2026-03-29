# M487_ScsiTool 任務追蹤

## 任務狀態總覽

| Task | 標題 | 狀態 | 備註 |
|------|------|------|------|
| T001 | 專案初始化與環境建置 | ✅ 完成 | |
| T002 | Nuvoton M487 USB 開發板研究 | ✅ 完成 | |
| T003 | USB 描述符與傳輸類型研究 | ✅ 完成 | |
| T004 | WDF 框架架構分析 | ✅ 完成 | |
| T005 | USB 描述符解析（M487） | ✅ 完成 | |
| T006 | HID 協定與 Report Descriptor 研究 | ✅ 完成 | |
| T007 | SCSI/UVC 指令集研究 | 🔄 進行中 | |
| T008 | M487 USB Device 驅動程式架構設計 | 🔄 進行中 | |
| T009 | USB 描述符定義（M487） | 🔄 進行中 | |
| T010 | USB Driver Stack 研究 | ✅ 完成 | |
| ... | ... | ... | ... |

| T027b | Firefly HID Filter Driver 研究 | ✅ 完成 | 2026-03-28 |
| T027c | M487 HID Filter Driver 骨架實作 | ✅ 完成 | 2026-03-28 || T037 | 微軟 USB Driver Samples 研究 | ✅ 完成 | 2026-03-28 |

---

## T037：微軟 USB Driver Samples 研究

**狀態**：✅ 完成  
**完成日期**：2026-03-28

### 任務目標

分析 `Windows-driver-samples/usb/` 及 `hid/` 目錄下的所有範例，重點研究：
- `usbsamp/` — 基本 USB 裝置範例
- `usbview/` — USB 檢視工具
- `kmdf_fx2/` — KMDF USB FX2 開發板
- `hid/hclient/` — HID Client 範例

### 完成產出

- **研究報告**：`D:\AiWorkSpace\M487_ScsiTool\doc\USB_Driver_Samples\README.md`

### 關鍵發現

1. **usbsamp**：最完整的 USB 驅動範例，支援 Bulk/Interrupt/Isochronous 三種傳輸類型
2. **kmdf_fx2**：最乾淨的教學範例，含 Continuous Reader + Vendor Command + Interrupt Pipe
3. **hclient**：User Mode HID 應用最佳參考，含 HID Report Descriptor 解析
4. **usbview**：USB 描述符結構的完整參考，含 Audio/Video Class 描述符定義

### M487 應用建議

| 應用場景 | 優先參考 | 關鍵點 |
|---------|---------|--------|
| USB HID | hclient | HIDP_* API、Report Descriptor |
| Bulk 傳輸 | usbsamp + kmdf_fx2 | WDF USB API、FormatRequestForRead/Write |
| Interrupt 通知 | kmdf_fx2/interrupt.c | Continuous Reader 模式 |
| Vendor Command | kmdf_fx2/ioctl.c | Control Transfer |

---

## T027b：Firefly HID Filter Driver 研究

**狀態**：✅ 完成  
**完成日期**：2026-03-28

### 任務目標

研究 Microsoft Windows-driver-samples 中 hid/firefly/ 的 KMDF HID Upper Filter Driver 範例，
理解 Upper Filter 的 attach 機制、IRP pass-through、WMI 介面設計。

### 完成產出

- **研究報告**：D:\AiWorkSpace\M487_ScsiTool\T027b\Firefly_Research_Report.md

### 關鍵發現

1. **KMDF Filter 設定**：WdfFdoInitSetFilter() 自動處理 IRP pass-through
2. **INF UpperFilters**：HKR,,"UpperFilters",0x00010000,"Firefly" 註冊為 upper filter
3. **PDO 名稱取得**：DevicePropertyPhysicalDeviceObjectName 取得底層 PDO 設備名
4. **IOCTL_HID_SET/GET_FEATURE**：向 HID 設備發送 Feature Report 的標準方式
5. **WMI 設計**：MOF 定義資料區塊，KMDF 處理 WMI 實例注册

### 與 M487 的相關性

Firefly 提供了完整的 HID upper filter 實作模式，可直接套用於 M487 HID Filter Driver 開發。

---

## T027c：M487 HID Filter Driver 骨架實作

**狀態**：✅ 完成  
**完成日期**：2026-03-28

### 任務目標

基於 Firefly 研究成果，建立 KMDF HID Upper Filter Driver 骨架。

### 完成產出

- **Driver 原始碼**：D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\
- **DriverEntry**：M487Filter.c
- **EvtDriverDeviceAdd**：M487FilterDevice.c
- **WMI 支援**：M487FilterWmi.c + M487Filter.mof
- **Custom IOCTL**：M487FilterIoctl.c + M487FilterIoctl.h
- **INF 安裝檔**：M487Filter.inf
- **說明文件**：M487Filter\README.md

### 實作檔案

`
tool/M487Filter/
├── driver/
│   ├── M487Filter.h           # Main header
│   ├── M487Filter.c           # DriverEntry
│   ├── M487FilterDevice.h     # DEVICE_CONTEXT
│   ├── M487FilterDevice.c     # EvtDriverDeviceAdd
│   ├── M487FilterWmi.h        # WMI declarations
│   ├── M487FilterWmi.c       # WMI handlers
│   ├── M487FilterIoctl.h      # Custom IOCTL codes
│   ├── M487FilterIoctl.c      # IOCTL handlers
│   ├── M487Filter.mof         # WMI MOF definition
│   ├── M487FilterMof.h        # Auto-generated MOF header (stub)
│   ├── M487Filter.rc          # Version resource
│   ├── M487Filter.inf         # INF installation file
│   ├── SOURCES               # WDK build file
│   ├── makefile
│   └── CMakeLists.txt
└── app/
    └── (placeholder)
`

### 關鍵實作模式

1. **Upper Filter 設定**：WdfFdoInitSetFilter(DeviceInit) — KMDF 自動 IRP pass-through
2. **PDO 取得**：DevicePropertyPhysicalDeviceObjectName + WdfIoTargetOpen
3. **HID Capabilities**：IOCTL_HID_GET_COLLECTION_INFORMATION → IOCTL_HID_GET_COLLECTION_DESCRIPTOR → HidP_GetCaps
4. **INF UpperFilters**：HKR,,"UpperFilters",0x00010000,"M487Filter"
5. **WMI PassThroughEnabled**：切換 filter 模式（pass-through/blocking/modifying）

### 待完成項目

- [ ] 確認 M487 HID 介面 VID/PID
- [ ] 實作 Input Report 攔截
- [ ] 實作 Report Descriptor 解析
- [ ] User-mode 控制應用程式
- [ ] 數位簽章
