# Windows HID 堆疊架構解析

## 📋 概述

HID（Human Input Device）是人機介面裝置的標準，涵蓋滑鼠、鍵盤、遊戲控制器、觸控板等。本文件解析 Windows HID 堆疊的完整架構。

## 🏗️ HID 堆疊層次結構

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          User Mode                                      │
│                                                                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │
│  │   應用程式   │  │   應用程式   │  │   遊戲      │  │   控制台    │   │
│  │  (遊戲手把) │  │  (鍵盤映射) │  │  (控制器)   │  │  (設定工具) │   │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘   │
│         │                │                │                │            │
│         └────────────────┼────────────────┼────────────────┘            │
│                          │                │                              │
│                          ▼                ▼                              │
│                   ┌─────────────────────────────────┐                  │
│                   │        Win32 HID API              │                  │
│                   │   HidD_* / HidP_* 函數集          │                  │
│                   └─────────────────┬───────────────┘                  │
│                                     │ HID Class Driver                  │
└─────────────────────────────────────┼───────────────────────────────────┘
                                      │ Device I/O
┌─────────────────────────────────────┼───────────────────────────────────┐
│                          Kernel Mode                                   │
│                                     │                                   │
│                           ┌─────────▼─────────┐                        │
│                           │    HIDCLASS.SYS  │                        │
│                           │  (HID Class Driver) │                      │
│                           │                   │                        │
│                           │  • 裝置列舉        │                        │
│                           │  • Report 管理    │                        │
│                           │  • Feature/Input  │                        │
│                           │    Output 處理    │                        │
│                           └─────────┬─────────┘                        │
│                                     │                                   │
│              ┌──────────────────────┼──────────────────────┐            │
│              │                      │                      │            │
│              ▼                      ▼                      ▼            │
│     ┌──────────────┐      ┌──────────────┐      ┌──────────────┐      │
│     │  mouhid.sys  │      │  kbdhid.sys  │      │  joyhid.sys  │      │
│     │  (滑鼠)       │      │  (鍵盤)       │      │  (遊戲控制器) │      │
│     │              │      │              │      │              │      │
│     │ • 滑鼠資料    │      │ • 鍵盤掃描碼 │      │ • 遊戲桿資料 │      │
│     │   解析       │      │   轉換       │      │   解析       │      │
│     └───────┬──────┘      └───────┬──────┘      └───────┬──────┘      │
│             │                      │                      │            │
│             └──────────────────────┼──────────────────────┘            │
│                                    │                                     │
│                           ┌────────▼────────┐                           │
│                           │     USBHUB     │                           │
│                           │   (USB Hub)     │                           │
│                           └────────┬────────┘                           │
│                                    │                                    │
│               ┌────────────────────┼────────────────────┐              │
│               │                    │                    │              │
│               ▼                    ▼                    ▼              │
│        ┌──────────┐        ┌──────────┐        ┌──────────┐         │
│        │ USB Mouse │        │ USB KB   │        │ USB Joy  │         │
│        │           │        │          │        │          │         │
│        └──────────┘        └──────────┘        └──────────┘         │
│               │                    │                    │            │
│               └────────────────────┼────────────────────┘            │
│                                    │                                    │
│                           ┌────────▼────────┐                          │
│                           │   USB Stack     │                          │
│                           │ (USBD + USBHUB) │                          │
│                           └────────┬────────┘                          │
│                                    │                                    │
└────────────────────────────────────┼────────────────────────────────────┘
                                     │ URB
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         USB Hardware                                     │
│                                                                         │
│    ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐        │
│    │ USB Host │───►│ USB Hub  │───►│ USB HID  │───►│  HID     │        │
│    │  Controller│   │          │   │  Device  │    │  Report  │        │
│    └──────────┘    └──────────┘    └──────────┘    └──────────┘        │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## 🔧 HID 描述符結構

### 1. 裝置描述符 (Device Descriptor)

```c
typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;           // 描述符長度 (18)
    UCHAR bDescriptorType;   // USB_DEVICE_DESCRIPTOR_TYPE (0x01)
    USHORT bcdUSB;           // USB 規格版本 (如 0x0200 for USB 2.0)
    UCHAR bDeviceClass;      // 裝置類別 (通常為 0)
    UCHAR bDeviceSubClass;   // 裝置子類別 (通常為 0)
    UCHAR bDeviceProtocol;   // 裝置通訊協定 (通常為 0)
    UCHAR bMaxPacketSize0;   // Endpoint 0 最大封包大小
    USHORT idVendor;         // VID
    USHORT idProduct;        // PID
    USHORT bcdDevice;        // 裝置版本
    UCHAR iManufacturer;     // 製造商字串索引
    UCHAR iProduct;          // 產品字串索引
    UCHAR iSerialNumber;     // 序號字串索引
    UCHAR bNumConfigurations;// 設定數量
} USB_DEVICE_DESCRIPTOR;
```

### 2. 設定描述符 (Configuration Descriptor)

```c
typedef struct _USB_CONFIGURATION_DESCRIPTOR {
    UCHAR bLength;              // 長度 (9)
    UCHAR bDescriptorType;      // USB_CONFIGURATION_DESCRIPTOR_TYPE (0x02)
    USHORT wTotalLength;        // 總長度 (含所有介面和端點)
    UCHAR bNumInterfaces;       // 介面數量
    UCHAR bConfigurationValue; // 設定值 (用於 SetConfiguration)
    UCHAR iConfiguration;       // 設定字串索引
    UCHAR bmAttributes;         // 特性 (供電、遠端喚醒等)
    UCHAR MaxPower;             // 最大耗電量 (2mA 單位)
} USB_CONFIGURATION_DESCRIPTOR;
```

### 3. 介面描述符 (Interface Descriptor)

```c
typedef struct _USB_INTERFACE_DESCRIPTOR {
    UCHAR bLength;             // 長度 (9)
    UCHAR bDescriptorType;      // USB_INTERFACE_DESCRIPTOR_TYPE (0x04)
    UCHAR bInterfaceNumber;     // 介面編號
    UCHAR bAlternateSetting;    // 備用設定值
    UCHAR bNumEndpoints;        // 端點數量 (不含 EP0)
    UCHAR bInterfaceClass;      // 介面類別 (HID = 0x03)
    UCHAR bInterfaceSubClass;   // 子類別 (Boot = 0x01)
    UCHAR bInterfaceProtocol;   // 通訊協定 (鍵盤=0x01, 滑鼠=0x02)
    UCHAR iInterface;           // 介面字串索引
} USB_INTERFACE_DESCRIPTOR;
```

### 4. 端點描述符 (Endpoint Descriptor)

```c
typedef struct _USB_ENDPOINT_DESCRIPTOR {
    UCHAR bLength;          // 長度 (7)
    UCHAR bDescriptorType;   // USB_ENDPOINT_DESCRIPTOR_TYPE (0x05)
    UCHAR bEndpointAddress;  // 端點位址 (位址 + 方向)
    UCHAR bmAttributes;      // 端點特性 (Interrupt = 0x03)
    USHORT wMaxPacketSize;  // 最大封包大小
    UCHAR bInterval;        // 輪詢間隔 (ms)
} USB_ENDPOINT_DESCRIPTOR;
// bEndpointAddress: BIT7=方向(0=OUT,1=IN), BIT6-4=保留, BIT3-0=端點號碼
```

### 5. HID 描述符 (HID Descriptor)

```c
typedef struct _HID_DESCRIPTOR {
    UCHAR bLength;             // 長度
    UCHAR bDescriptorType;     // HID_DESCRIPTOR_TYPE (0x21)
    USHORT bcdHID;             // HID 版本 (如 0x0110)
    UCHAR bCountryCode;        // 國家碼
    UCHAR bNumDescriptors;      // 描述符數量 (通常為 1)
    UCHAR bDescriptorType2;    // Report 描述符類型 (0x22)
    USHORT wDescriptorLength;  // Report 描述符長度
} HID_DESCRIPTOR;
```

### 6. HID Report 描述符

HID Report 描述符是 HID 裝置最重要的描述符，定義資料格式：

```c
// 簡化範例：2 按鍵 + X/Y 座標的滑鼠
const UCHAR MouseReportDescriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    
    // 按鈕部分
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (Button 1)
    0x29, 0x03,        //     Usage Maximum (Button 3)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x03,        //     Report Count (3 buttons)
    0x75, 0x01,        //     Report Size (1 bit each)
    0x81, 0x02,        //     Input (Data, Variable, Absolute)
    
    // Padding (5 bits)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x05,        //     Report Size (5 bits)
    0x81, 0x01,        //     Input (Constant)
    
    // X/Y 座標
    0x05, 0x01,        //     Usage Page (Generic Desktop)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8 bits each)
    0x95, 0x02,        //     Report Count (2: X, Y)
    0x81, 0x06,        //     Input (Data, Variable, Relative)
    
    0xC0,              //   End Collection
    0xC0               // End Collection
};
```

## 📡 HID IOCTL 程式碼

### 主要 IOCTL 控制碼

| IOCTL | 用途 | 方向 |
|-------|------|------|
| `IOCTL_HID_GET_DEVICE_DESCRIPTOR` | 獲取 HID 裝置描述符 | 讀取 |
| `IOCTL_HID_GET_REPORT_DESCRIPTOR` | 獲取 Report 描述符 | 讀取 |
| `IOCTL_HID_READ_REPORT` | 讀取 HID 報告 | 讀取 |
| `IOCTL_HID_WRITE_REPORT` | 寫入 HID 報告 | 寫入 |
| `IOCTL_HID_GET_COLLECTION_INFORMATION` | 獲取集合資訊 | 讀取 |
| `IOCTL_HID_GET_COLLECTION_DESCRIPTOR` | 獲取集合描述符 | 讀取 |
| `IOCTL_HID_GET_HARDWARE_DESCRIPTOR` | 獲取硬體描述符 | 讀取 |
| `IOCTL_HID_SET_FEATURE` | 設定 Feature 報告 | 寫入 |
| `IOCTL_HID_GET_FEATURE` | 獲取 Feature 報告 | 讀取 |
| `IOCTL_HID_GET_INPUT_REPORT` | 獲取 Input 報告 | 讀取 |
| `IOCTL_HID_SET_OUTPUT_REPORT` | 設定 Output 報告 | 寫入 |
| `IOCTL_HID_GET_MANUFACTURER_DESCRIPTOR` | 獲取製造商 | 讀取 |
| `IOCTL_HID_GET_PRODUCT_DESCRIPTOR` | 獲取產品描述 | 讀取 |
| `IOCTL_HID_GET_STRING` | 獲取字串 | 讀取 |

### 程式碼中使用方式

```c
// 發送 IOCTL 範例
WDF_MEMORY_DESCRIPTOR outputDescriptor;
HID_COLLECTION_INFORMATION collectionInfo;

WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor,
    &collectionInfo, sizeof(collectionInfo));

status = WdfIoTargetSendIoctlSynchronously(
    hidTarget,
    NULL, // Request (optional)
    IOCTL_HID_GET_COLLECTION_INFORMATION,
    NULL, // Input buffer (none)
    &outputDescriptor, // Output buffer
    NULL, // BytesReturned (optional)
    NULL); // Timeout (none)
```

## 🔄 HID 報告類型

| 報告類型 | 方向 | 用途 | Firefly 範例 |
|----------|------|------|-------------|
| Input Report | 裝置→主機 | 報告按鍵、軸向等狀態 | - |
| Output Report | 主機→裝置 | 控制 LED、震動等 | - |
| Feature Report | 主機↔裝置 | 設定和讀取配置 | ✅ 用於控制滑鼠燈光 |

### Feature 報告操作流程

```c
// 1. 獲取集合資訊
IOCTL_HID_GET_COLLECTION_INFORMATION
→ 返回 HID_COLLECTION_INFORMATION

// 2. 獲取預解析資料
IOCTL_HID_GET_COLLECTION_DESCRIPTOR
→ 返回 Report Descriptor

// 3. 解析能力
HidP_GetCaps(preparsedData, &caps);

// 4. 操作 Feature Report
// 設定 Usage
HidP_SetUsages(HidP_Feature, page, 0, &usage, &len, 
                preparsedData, report, reportLen);

// 5. 發送 Feature Report
IOCTL_HID_SET_FEATURE
```

## 🪝 HID Filter Driver 位置

```
┌─────────────────────────────────────────┐
│      Upper Filter Driver                │
│      (firefly.sys / M487.sys)          │
│                                         │
│  - 可以攔截所有 HID IRP                 │
│  - 可以修改 Input/Output/Feature 報告    │
│  - 可以新增 WMI 介面                    │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│      HID Minidriver                     │
│      (mouhid.sys / kbdhid.sys)         │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│      HIDCLASS.SYS                       │
└───────────────┬─────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────┐
│      USB Stack                          │
└─────────────────────────────────────────┘
```

## 📝 Filter Driver 設計考量

### 1. 請求攔截點

| IRP Major Function | 說明 | 常見用途 |
|-------------------|------|----------|
| `IRP_MJ_CREATE` | 建立檔案連線 | 記錄、驗證 |
| `IRP_MJ_CLOSE` | 關閉檔案連線 | 清理資源 |
| `IRP_MJ_READ` | 讀取資料 | 修改 Input Report |
| `IRP_MJ_WRITE` | 寫入資料 | 修改 Output Report |
| `IRP_MJ_DEVICE_CONTROL` | IOCTL 控制 | 處理 HID IOCTL |
| `IRP_MJ_INTERNAL_DEVICE_CONTROL` | 內部 IOCTL | HID Feature/Report |

### 2. WMI 整合

HID Filter 通常透過 WMI 與使用者模式通訊：

```mof
class FireflyDeviceInformation {
    [key, read] string InstanceName;
    [read] boolean Active;
    [WmiDataId(1), read, write] boolean TailLit;
};
```

### 3. IO Target 使用

Filter Driver 透過 IO Target 與下層通訊：

```c
// 開啟 PDO
WdfIoTargetOpen(hidTarget, &openParams);

// 發送 IOCTL
WdfIoTargetSendIoctlSynchronously(hidTarget, ...);

// 發送讀取/寫入
WdfIoTargetSendReadSynchronously(hidTarget, ...);
```

## 🔗 相關資源

- [HID Architecture (MSDN)](https://docs.microsoft.com/windows-hardware/drivers/hid/)
- [HID Transports](https://docs.microsoft.com/windows-hardware/drivers/hid/hid-transports)
- [HID Client Drivers](https://docs.microsoft.com/windows-hardware/drivers/hid/hid-clients)
- [HID Parser](https://docs.microsoft.com/windows-hardware/drivers/hid/hid-parser)
- [HID Over USB](https://docs.microsoft.com/windows-hardware/drivers/hid/hid-over-usb)

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
