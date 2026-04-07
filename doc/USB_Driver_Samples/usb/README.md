# USB 範例分析總覽

## 📋 概述

本目錄包含 Microsoft Windows-driver-samples 中 USB 相關範例的分析文件。這些範例涵蓋了 USB 驅動程式開發的各個方面。

## 📁 目錄內容

| 檔案 | 描述 | 重要性 |
|------|------|--------|
| `README.md` | 本檔案 - USB 範例總覽 | - |
| `usbsamp_Analysis.md` | usbsamp 通用 USB 驅動程式分析 | ⭐⭐⭐⭐ |
| `kmdf_fx2_Analysis.md` | kmdf_fx2 KMDF USB 裝置分析 | ⭐⭐⭐⭐ |

## 📂 USB 範例清單

### 1. usbsamp（通用 USB 裝置驅動程式）

**位置：** `usb/usbsamp/`

最完整的 USB 驅動程式範例，展示所有 USB 傳輸類型。

**功能：**
- Bulk Read/Write 傳輸
- Isochronous Read/Write 傳輸
- Control Transfer
- USB 描述符處理
- 電源管理
- 支援 SuperSpeed、High Speed、Full Speed

**適用性：** 學習 USB URB 處理和描述符解析

### 2. kmdf_fx2（OSR USB-FX2 學習板驅動程式）

**位置：** `usb/kmdf_fx2/`

專為 OSR USB-FX2 開發板設計，展示 KMDF USB 程式設計模式。

**功能：**
- USB 裝置初始化
- Bulk 和 Interrupt 傳輸
- 連續讀取器（Continuous Reader）
- IOCTL 介面
- 電源管理

**適用性：** 學習 KMDF USB 開發模式和 IOCTL 設計

### 3. 其他 USB 範例

| 範例 | 路徑 | 用途 |
|------|------|------|
| ufxclientsample | `usb/ufxclientsample/` | UFX (USB Function) 用戶端範例 |
| UcmCxUcsi | `usb/UcmCxUcsi/` | USB Type-C 電源傳遞範例 |
| wdf_osrfx2_lab | `usb/wdf_osrfx2_lab/` | USB FX2 實驗室範例 |

## 🔑 核心概念

### USB 傳輸類型

| 傳輸類型 | 特性 | 適用場景 |
|----------|------|----------|
| Control | 可靠、少量資料、保證傳輸 | 裝置設定、命令 |
| Bulk | 可靠、大量資料、不保證延遲 | 檔案傳輸、印表機 |
| Interrupt | 可靠、小量資料、保證延遲 | 鍵盤、滑鼠 |
| Isochronous | 不可靠、大量資料、保證延遲 | 音訊、視訊 |

### KMDF USB 物件層次

```
WDFUSBDEVICE (UsbDevice)
    │
    ├── WDFUSBINTERFACE (UsbInterface)
    │       │
    │       └── WDFUSBPIPE (Pipe 0-N)
    │               ├── Interrupt Pipe
    │               ├── Bulk IN Pipe
    │               ├── Bulk OUT Pipe
    │               └── Isochronous Pipe
    │
    └── 描述符儲存
            ├── USB_DEVICE_DESCRIPTOR
            ├── USB_CONFIGURATION_DESCRIPTOR
            ├── USB_INTERFACE_DESCRIPTOR[]
            └── USB_ENDPOINT_DESCRIPTOR[]
```

## 📚 關鍵 API

### 裝置創建與描述符讀取

```c
// 建立 USB 裝置控制代碼
WdfUsbTargetDeviceCreateWithParameters(Device, &config, ...);

// 獲取裝置描述符
WdfUsbTargetDeviceGetDeviceDescriptor(UsbDevice, &descriptor);

// 獲取設定描述符
WdfUsbTargetDeviceRetrieveConfigDescriptor(UsbDevice, ...);

// 選擇設定
WdfUsbTargetDeviceSelectConfig(UsbDevice, ...);
```

### 介面和 Pipe 處理

```c
// 獲取介面數量
WdfUsbTargetDeviceGetNumInterfaces(UsbDevice);

// 選擇介面設定
WdfUsbInterfaceSelectSetting(Interface, ...);

// 獲取設定的 Pipe
WdfUsbInterfaceGetConfiguredPipe(Interface, Index, ...);

// 獲取 Pipe 資訊
WdfUsbTargetPipeGetInformation(Pipe, &pipeInfo);
```

### 傳輸操作

```c
// 格式化讀取請求
WdfUsbTargetPipeFormatRequestForRead(Pipe, Request, ...);

// 格式化寫入請求
WdfUsbTargetPipeFormatRequestForWrite(Pipe, Request, ...);

// 同步讀取
WdfUsbTargetPipeReadSynchronously(Pipe, ...);

// 同步寫入
WdfUsbTargetPipeWriteSynchronously(Pipe, ...);

// 中止 Pipe
WdfUsbTargetPipeAbortSynchronously(Pipe, ...);

// 重置 Pipe
WdfUsbTargetPipeResetSynchronously(Pipe, ...);

// 連續讀取器（用於 Interrupt）
WdfUsbTargetPipeConfigContinuousReader(Pipe, ...);
```

### 控制傳輸

```c
// 初始化控制設定封包
WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&packet, ...);

// 同步傳送控制傳輸
WdfUsbTargetDeviceSendControlTransferSynchronously(Device, ...);
```

## 🔧 INF 檔案模式

USB 驅動程式的 INF 檔案需要指定：

1. **Target Hardware ID**
2. **Service 註冊**
3. **KMDF 設定**

```inf
[Manufacturer]
%MfgName%=Standard,NT$ARCH$.10.0....16299

[Standard.NT$ARCH$.10.0....16299]
%DeviceDesc%=UsbDevice_Install, USB\VID_045E&PID_00B4

[UsbDevice_Install.NT]
CopyFiles=UsbDevice_CopyFiles
Include=msusb.inf
Needs=MSUSB.Device

[UsbDevice_Install.NT.Services]
AddService=MyUsbDriver, 0x00000002, MyService_Install

[MyService_Install]
ServiceType=1
StartType=3
ErrorControl=1
ServiceBinary=%12%\MyUsbDriver.sys
```

## 📊 與 M487 的關聯

### 適用於 M487 的模式

| 功能 | 參考範例 | 說明 |
|------|----------|------|
| USB 初始化 | kmdf_fx2 | `WdfUsbTargetDeviceCreateWithParameters` |
| 描述符處理 | usbsamp | 設定/介面/端點描述符解析 |
| 控制傳輸 | kmdf_fx2 | IOCTL 處理和 Vendor Command |
| Bulk 傳輸 | usbsamp/kmdf_fx2 | 資料讀寫 |
| Filter 附加 | firefly | 上層過濾器模式 |

### 關鍵差異

M487 作為 Filter Driver，與這些範例的主要差異：

1. **不需要建立 USB 控制代碼** - 由下層驅動程式處理
2. **需要開啟 PDO** - 透過 IO Target
3. **需要附加至裝置堆疊** - 使用 `WdfFdoInitSetFilter`
4. **需要處理 IRP** - 攔截和轉發請求

## 📝 使用建議

1. **新手起點** - 從 kmdf_fx2 開始，它較為簡潔
2. **深入學習** - 研究 usbsamp 了解完整 USB 處理
3. **Filter 設計** - 以 firefly 為主要參考（見 hid 目錄）

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
