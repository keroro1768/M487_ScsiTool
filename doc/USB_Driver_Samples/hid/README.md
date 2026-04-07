# HID 範例分析總覽

## 📋 概述

本目錄包含 Microsoft Windows-driver-samples 中 HID 相關範例的分析文件。HID（Human Input Device）是人機介面裝置的標準，滑鼠、鍵盤、遊戲控制器等都屬於此類。

## 📁 目錄內容

| 檔案 | 描述 | 重要性 |
|------|------|--------|
| `README.md` | 本檔案 - HID 範例總覽 | - |
| `firefly_Deep_Dive.md` | Firefly HID Upper Filter Driver 深入分析 | ⭐⭐⭐⭐⭐ |
| `hid_Architecture.md` | Windows HID 堆疊架構解析 | ⭐⭐⭐⭐ |

## 🔑 Firefly 範例（HID Upper Filter Driver）

**位置：** `hid/firefly/`

Firefly 是本知識庫中最重要的範例，它是 KMDF HID 上層過濾驅動程式的完整實作。

### 主要功能
- 作為 HID 裝置的上層過濾器附加至裝置堆疊
- 使用 WMI 介面與使用者模式應用程式通訊
- 發送 HID Feature 報告控制滑鼠燈光

### 核心程式碼模組

| 檔案 | 功能 |
|------|------|
| `driver/driver.c` | DriverEntry 入口點 |
| `driver/device.c` | 裝置初始化、Filter 設定、WMI 初始化 |
| `driver/vfeature.c` | HID Feature 報告發送邏輯 |
| `driver/wmi.c` | WMI Provider 實作 |
| `driver/firefly.mof` | WMI 資料類別定義 |
| `driver/firefly.inx` | INF 安裝檔案 |

### 關鍵學習點

1. **WdfFdoInitSetFilter** - 將 WDFDEVICE 設定為 Filter Driver
2. **WdfDeviceAllocAndQueryProperty** - 獲取 PDO 名稱
3. **WdfIoTargetCreate/Open** - 開啟 PDO 以發送 IOCTL
4. **IOCTL_HID_XXX** - HID IOCTL 控制碼
5. **HidP_GetCaps/HidP_SetUsages** - HID Parser 使用
6. **WMI Provider** - 使用者/核心通訊

### 與 M487 的關聯

M487 USB Filter Driver 需要類似 Firefly 的架構：
- 作為 USB 裝置的上層 Filter
- 攔截和處理 HID/USB 請求
- 與使用者模式應用程式通訊

## 🏗️ HID 堆疊架構

### Windows HID 堆疊層次

```
┌─────────────────────────────────────────┐
│      User Mode Applications              │
│  (flicker.exe, game controllers, etc.) │
└────────────────┬────────────────────────┘
                 │ Win32 API
┌────────────────▼────────────────────────┐
│         HIDCLASS.SYS (HID Class Driver) │
│  - Enumerates HID devices              │
│  - Handles HID reports                  │
│  - Exports HID API to user mode        │
└────────────────┬────────────────────────┘
                 │ IOCTL
┌────────────────▼────────────────────────┐
│       HID Minidriver (例如 mouhid.sys)  │
│  - Vendor-specific HID minidriver       │
│  - Translates to USB/HID protocols     │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│      USB Stack (USBD + USBHUB)         │
└────────────────┬────────────────────────┘
                 │ URB
┌────────────────▼────────────────────────┐
│           USB Hardware                  │
└─────────────────────────────────────────┘
```

### Filter Driver 位置

```
                    ┌─────────────────────────┐
                    │   Upper Filter Driver   │
                    │   (如 firefly.sys)      │
                    │   - Intercepts IRPs    │
                    │   - Can modify data    │
                    └───────────┬─────────────┘
                                │
                    ┌───────────▼─────────────┐
                    │   Function Driver       │
                    │   (mouhid.sys)          │
                    │   - Main device driver │
                    └───────────┬─────────────┘
                                │
                    ┌───────────▼─────────────┐
                    │   Lower Filter Driver  │
                    │   (optional)            │
                    └───────────┬─────────────┘
                                │
                    ┌───────────▼─────────────┐
                    │   USB Hub Driver        │
                    │   (usbhub.sys)          │
                    └─────────────────────────┘
```

## 📚 相關標準和文件

### USB HID 規範
- USB HID 1.1 規格書
- HID Usage Tables 規格書

### Windows HID 程式設計
- [HID Architecture](https://docs.microsoft.com/windows-hardware/drivers/hid/)
- [HID Clients](https://docs.microsoft.com/windows-hardware/drivers/hid/hid-clients)
- [HID Transports](https://docs.microsoft.com/windows-hardware/drivers/hid/hid-transports)

### 相關 IOCTL
- `IOCTL_HID_GET_DEVICE_DESCRIPTOR`
- `IOCTL_HID_GET_REPORT_DESCRIPTOR`
- `IOCTL_HID_READ_REPORT`
- `IOCTL_HID_WRITE_REPORT`
- `IOCTL_HID_GET_COLLECTION_DESCRIPTOR`
- `IOCTL_HID_GET_COLLECTION_INFORMATION`
- `IOCTL_HID_GET_HARDWARE_DESCRIPTOR`
- `IOCTL_HID_GET_MANUFACTURER_DESCRIPTOR`
- `IOCTL_HID_GET_PRODUCT_DESCRIPTOR`
- `IOCTL_HID_GET_SERIAL_NUMBER_DESCRIPTOR`
- `IOCTL_HID_GET_INDEXED_STRING`
- `IOCTL_HID_GET_DEVICE_ATTRIBUTES`
- `IOCTL_HID_SET_FEATURE`
- `IOCTL_HID_GET_FEATURE`
- `IOCTL_HID_GET_INPUT_REPORT`
- `IOCTL_HID_SET_OUTPUT_REPORT`
- `IOCTL_HID_SET_POLL_FREQUENCY_MCU`
- `IOCTL_HID_GET_POLL_FREQUENCY_MCU`
- `IOCTL_HID_GET_STRING`

## 🔗 其他 HID 範例

除了 firefly，Windows-driver-samples 還包含其他 HID 範例：

| 範例 | 路徑 | 用途 |
|------|------|------|
| hclient | `hid/hclient/` | HID Class Driver 擴展範例 |
| hidusbfx2 | `hid/hidusbfx2/` | USB FX2 板的 HID 範例 |
| vhidmini2 | `hid/vhidmini2/` | 虛擬 HID 裝置驅動程式 |

## 📝 使用建議

1. **首先閱讀** `hid_Architecture.md` 了解 HID 堆疊
2. **深入研究** `firefly_Deep_Dive.md` 學習 Filter Driver 實作
3. **根據需要** 查閱相關 IOCTL 和 API 文件

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
