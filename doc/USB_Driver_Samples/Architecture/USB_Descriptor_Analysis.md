# USB 描述符分析

## 📋 概述

本文件詳細說明 USB 描述符的結構和分析方法。

## 📚 USB 描述符類型

| 描述符類型 | 類型值 | 用途 |
|------------|--------|------|
| Device | 0x01 | 整體裝置資訊 |
| Configuration | 0x02 | 電源和介面設定 |
| String | 0x03 | 人類可讀字串 |
| Interface | 0x04 | 介面功能描述 |
| Endpoint | 0x05 | 端點特性 |
| Device Qualifier | 0x06 | 高速裝置的全速資訊 |
| Other Speed Configuration | 0x07 | 其他速度設定 |
| Interface Power | 0x08 | 電源管理 |
| HID | 0x21 | HID 描述符 |
| Report | 0x22 | HID Report 描述符 |
| Physical | 0x23 | 物理描述符 |

## 📦 描述符結構

### 1. 裝置描述符 (Device Descriptor)

```c
typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;              // 描述符長度 (固定 18)
    UCHAR bDescriptorType;      // 0x01 (USB_DEVICE_DESCRIPTOR_TYPE)
    USHORT bcdUSB;             // USB 規格版本 (0x0200 = USB 2.0)
    UCHAR bDeviceClass;        // 裝置類別 (0 表示由介面定義)
    UCHAR bDeviceSubClass;     // 裝置子類別
    UCHAR bDeviceProtocol;     // 裝置通訊協定
    UCHAR bMaxPacketSize0;     // Endpoint 0 最大封包大小 (8, 16, 32, 64)
    USHORT idVendor;           // VID (Little Endian)
    USHORT idProduct;          // PID (Little Endian)
    USHORT bcdDevice;          // 裝置版本號
    UCHAR iManufacturer;        // 製造商字串索引
    UCHAR iProduct;            // 產品字串索引
    UCHAR iSerialNumber;       // 序號字串索引
    UCHAR bNumConfigurations;  // 設定數量
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;
```

**範例：**
```
bLength:           12h (18 bytes)
bDescriptorType:    01h
bcdUSB:           0200h (USB 2.0)
bDeviceClass:      00h (Defined by interface)
bDeviceSubClass:   00h
bDeviceProtocol:   00h
bMaxPacketSize0:   40h (64 bytes)
idVendor:         045Eh (Microsoft)
idProduct:        0047h (Intellimouse)
bcdDevice:        0100h (1.00)
iManufacturer:     01h
iProduct:         02h
iSerialNumber:    03h
bNumConfigurations: 01h
```

### 2. 設定描述符 (Configuration Descriptor)

```c
typedef struct _USB_CONFIGURATION_DESCRIPTOR {
    UCHAR bLength;              // 長度 (固定 9)
    UCHAR bDescriptorType;      // 0x02
    USHORT wTotalLength;        // 總長度 (含所有介面和端點描述符)
    UCHAR bNumInterfaces;       // 介面數量
    UCHAR bConfigurationValue;  // SetConfiguration() 的值
    UCHAR iConfiguration;       // 設定字串索引
    UCHAR bmAttributes;         // 特性
    UCHAR MaxPower;             // 最大耗電量 (2mA 單位)
} USB_CONFIGURATION_DESCRIPTOR;
// bmAttributes:
//   Bit 7: 保留 (必須為 1)
//   Bit 6: 自供電
//   Bit 5: 遠端喚醒
//   Bit 4-0: 保留
```

### 3. 介面描述符 (Interface Descriptor)

```c
typedef struct _USB_INTERFACE_DESCRIPTOR {
    UCHAR bLength;             // 長度 (固定 9)
    UCHAR bDescriptorType;      // 0x04
    UCHAR bInterfaceNumber;    // 介面編號 (從 0 開始)
    UCHAR bAlternateSetting;   // 備用設定值
    UCHAR bNumEndpoints;       // 端點數量 (不含 EP0)
    UCHAR bInterfaceClass;     // 介面類別
    UCHAR bInterfaceSubClass;  // 介面子類別
    UCHAR bInterfaceProtocol;  // 介面通訊協定
    UCHAR iInterface;          // 介面字串索引
} USB_INTERFACE_DESCRIPTOR;
// 常見介面類別:
//   0x03: HID (Human Interface Device)
//   0x08: Mass Storage
//   0xFF: Vendor Specific
```

### 4. 端點描述符 (Endpoint Descriptor)

```c
typedef struct _USB_ENDPOINT_DESCRIPTOR {
    UCHAR bLength;          // 長度 (固定 7)
    UCHAR bDescriptorType;   // 0x05
    UCHAR bEndpointAddress;  // 端點位址和方向
    UCHAR bmAttributes;      // 端點特性
    USHORT wMaxPacketSize; // 最大封包大小
    UCHAR bInterval;        // 輪詢間隔
} USB_ENDPOINT_DESCRIPTOR;
// bEndpointAddress:
//   Bits 0-3: 端點號碼
//   Bits 4-6: 保留
//   Bit 7: 方向 (0 = OUT, 1 = IN)
// bmAttributes:
//   Bits 0-1: 傳輸類型 (00=Control, 01=Isochronous, 10=Bulk, 11=Interrupt)
//   Bits 2-3: Isochronous 同步類型 (僅用於 Iso)
//   Bits 4-5: 用法類型 (僅用於 Iso)
```

### 5. HID 描述符 (HID Descriptor)

```c
typedef struct _USB_HID_DESCRIPTOR {
    UCHAR bLength;             // 長度
    UCHAR bDescriptorType;     // 0x21 (HID_DESCRIPTOR_TYPE)
    USHORT bcdHID;            // HID 版本 (如 0x0110)
    UCHAR bCountryCode;        // 國家碼
    UCHAR bNumDescriptors;     // 描述符數量
    UCHAR bDescriptorType2;    // 下一個描述符的類型 (0x22 = Report)
    USHORT wDescriptorLength; // 下一個描述符的長度
} USB_HID_DESCRIPTOR;
// 可有多個描述符，bDescriptorType2/bDescriptorLength 重複
```

## 🔍 描述符解析

### 解析流程

```
1. 獲取 Configuration Descriptor
   └─> 得知 wTotalLength

2. 分配 wTotalLength 大小的緩衝區

3. 再次請求 Configuration Descriptor
   └─> 獲得完整配置（含所有介面、端點）

4. 解析 Configuration Descriptor
   ├─> 解析 Configuration 本身
   ├─> 解析 Interface 0
   │   ├─> 解析 Endpoint 0
   │   ├─> 解析 Endpoint 1
   │   └─> ...
   ├─> 解析 Interface 1 (可選)
   │   └─> ...
   └─> 解析 Interface N
```

### KMDF 解析範例

```c
// usbsamp 中的解析程式碼
NTSTATUS
ConfigureDevice(_In_ WDFDEVICE Device)
{
    PDEVICE_CONTEXT pDeviceContext = GetDeviceContext(Device);
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    WDFMEMORY memory;
    USHORT size = 0;
    NTSTATUS status;

    // 步驟 1: 查詢大小
    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
        pDeviceContext->WdfUsbTargetDevice, NULL, &size);

    // 步驟 2: 分配記憶體
    status = WdfMemoryCreate(&attributes, NonPagedPoolNx, POOL_TAG,
        size, &memory, &configurationDescriptor);

    // 步驟 3: 獲取描述符
    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
        pDeviceContext->WdfUsbTargetDevice,
        configurationDescriptor, &size);

    // 步驟 4: 驗證
    status = UsbSamp_ValidateConfigurationDescriptor(
        configurationDescriptor, size, &Offset);

    // 步驟 5: 解析
    // 方式 1: USBD 函式
    PUSB_INTERFACE_DESCRIPTOR intfDesc = 
        USBD_ParseConfigurationDescriptorEx(
            configurationDescriptor,
            configurationDescriptor,  // 起始位置
            0,                      // 介面號碼
            0,                      // 備用設定
            -1, -1, -1              // 不限制類別
        );

    // 步驟 6: 解析端點
    PUSB_ENDPOINT_DESCRIPTOR epDesc = 
        USBD_ParseDescriptors(
            configurationDescriptor,
            size,
            intfDesc,
            USB_ENDPOINT_DESCRIPTOR_TYPE
        );
}
```

### 手動解析（當 USBD 函式不可用時）

```c
PUCHAR
ParseConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    ULONG BufferLength
    )
{
    PUCHAR current = (PUCHAR)ConfigDesc;
    PUCHAR end = current + BufferLength;

    while (current < end) {
        PUSB_COMMON_DESCRIPTOR common = (PUSB_COMMON_DESCRIPTOR)current;

        // 檢查邊界
        if ((current + common->bLength) > end) {
            break;
        }

        switch (common->bDescriptorType) {
        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            // Configuration Descriptor
            {
                PUSB_CONFIGURATION_DESCRIPTOR config = 
                    (PUSB_CONFIGURATION_DESCRIPTOR)common;
                // 處理 Configuration
            }
            break;

        case USB_INTERFACE_DESCRIPTOR_TYPE:
            // Interface Descriptor
            {
                PUSB_INTERFACE_DESCRIPTOR intf = 
                    (PUSB_INTERFACE_DESCRIPTOR)common;
                // 處理 Interface
            }
            break;

        case USB_ENDPOINT_DESCRIPTOR_TYPE:
            // Endpoint Descriptor
            {
                PUSB_ENDPOINT_DESCRIPTOR ep = 
                    (PUSB_ENDPOINT_DESCRIPTOR)common;
                // 處理 Endpoint
            }
            break;

        case HID_DESCRIPTOR_TYPE:
            // HID Descriptor
            {
                PUSB_HID_DESCRIPTOR hid = (PUSB_HID_DESCRIPTOR)common;
                // 處理 HID
            }
            break;
        }

        current += common->bLength;
    }
}
```

## 📊 M487 描述符考量

### 識別 M487 裝置

```c
BOOLEAN
IsM487Device(
    PDEVICE_CONTEXT DeviceContext
    )
{
    USB_DEVICE_DESCRIPTOR deviceDesc;
    
    // 獲取裝置描述符
    WdfUsbTargetDeviceGetDeviceDescriptor(
        DeviceContext->WdfUsbTargetDevice, &deviceDesc);

    // M487 VID/PID (假設)
    if (deviceDesc.idVendor == 0x0416 &&  // Nuvoton
        deviceDesc.idProduct == 0xM487) {  // M487 PID
        return TRUE;
    }
    
    return FALSE;
}
```

### 驗證描述符有效性

```c
USBD_STATUS
ValidateUsbDescriptors(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    ULONG BufferLength,
    PUCHAR *ErrorOffset
    )
{
    USBD_STATUS status;

    // 基本驗證
    if (ConfigDesc->bLength < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
        return USBD_STATUS_BAD_DESCRIPTOR;
    }

    // USBD 函式驗證
    status = USBD_ValidateConfigurationDescriptor(
        ConfigDesc,
        BufferLength,
        3,              // ValidationLevel
        ErrorOffset,
        POOL_TAG
    );

    if (!USBD_SUCCESS(status)) {
        return status;
    }

    // 額外驗證：檢查 HID 描述符（如果需要）
    // ...

    return USBD_STATUS_SUCCESS;
}
```

## 🔗 相關資源

- [USB Descriptor](https://docs.microsoft.com/windows-hardware/drivers/usbcon/usb-descriptors)
- [Standard USB Descriptors](https://docs.microsoft.com/windows-hardware/drivers/usbcon/standard-usb-descriptors)
- [USB in a Nutshell](https://beyondlogic.org/usbnutshell/usb6.shtml)

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
