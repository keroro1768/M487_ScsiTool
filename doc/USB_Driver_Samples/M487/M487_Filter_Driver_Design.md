# M487 USB Filter Driver 設計參考

## 📋 概述

本文件說明如何借鑒 Microsoft firefly 範例來設計 M487 USB Filter Driver。

## 🎯 M487 專案需求

### 目標
- 作為 USB 裝置的上層 Filter Driver
- 攔截和處理 USB/HID 請求
- 與使用者模式應用程式通訊
- 讀取 USB 描述符（VID/PID）
- 控制 USB 資料流向

### 類似於 Firefly 的功能
- Upper Filter 附加方式
- PDO 開啟和 IOCTL 發送
- WMI 介面（或自訂 IOCTL）
- Feature Report 操作

## 🏗️ 設計架構

### 裝置堆疊位置

```
┌─────────────────────────────────────────────────────┐
│                    User Mode                          │
│              M487App.exe                              │
└────────────────────────┬────────────────────────────┘
                         │ (ReadFile/WriteFile/IOCTL)
┌────────────────────────▼────────────────────────────┐
│                 M487Filter.sys                       │
│                (Upper Filter)                        │
│                                                     │
│  ┌───────────────────────────────────────────────┐ │
│  │  M487 設計模式:                               │ │
│  │                                               │ │
│  │  - 讀取 USB 描述符                           │ │
│  │  - 攔截 SCSI/USB 請求                        │ │
│  │  - 過濾資料                                  │ │
│  │  - WMI/IOCTL 介面                           │ │
│  └───────────────────────────────────────────────┘ │
└────────────────────────┬────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────┐
│              USB Storage Function Driver              │
│              (如 usbstor.sys)                       │
└────────────────────────┬────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────┐
│                  USB Hub Driver                       │
└────────────────────────┬────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────┐
│                   USB Host Controller                 │
└─────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────┐
│                   M487 USB Device                    │
│                  (USB Composite Device)              │
└─────────────────────────────────────────────────────┘
```

## 🔧 關鍵設計決策

### 1. Filter 附加方式

基於 Firefly 的 `WdfFdoInitSetFilter()` 模式：

```c
NTSTATUS
M487EvtDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;
    WDFDEVICE device;

    PAGED_CODE();

    // 設定為 Upper Filter Driver
    WdfFdoInitSetFilter(DeviceInit);

    // 初始化上下文
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, M487_DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = M487EvtDeviceContextCleanup;

    // 建立裝置
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 設定 I/O 佇列
    // ...

    return status;
}
```

### 2. 獲取 PDO 名稱

Firefly 的關鍵模式 - 儲存 PDO 名稱用於後續開啟：

```c
NTSTATUS
M487StorePdoName(
    WDFDEVICE Device
    )
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY memory;
    PDEVICE_CONTEXT context;
    size_t bufferLength;
    NTSTATUS status;

    context = M487GetDeviceContext(Device);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;

    status = WdfDeviceAllocAndQueryProperty(
        Device,
        DevicePropertyPhysicalDeviceObjectName,
        NonPagedPoolNx,
        &attributes,
        &memory);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    context->PdoName.Buffer = WdfMemoryGetBuffer(memory, &bufferLength);
    context->PdoName.MaximumLength = (USHORT)bufferLength;
    context->PdoName.Length = (USHORT)(bufferLength - sizeof(UNICODE_NULL));

    return STATUS_SUCCESS;
}
```

### 3. 開啟 PDO 發送 IOCTL

```c
NTSTATUS
M487OpenPdoAndSendIoctl(
    PDEVICE_CONTEXT Context,
    ULONG IoControlCode,
    PVOID InputBuffer,
    SIZE_T InputLength,
    PVOID OutputBuffer,
    SIZE_T OutputLength,
    PULONG BytesReturned
    )
{
    WDFIOTARGET target;
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    WDF_MEMORY_DESCRIPTOR inputDesc, outputDesc;
    NTSTATUS status;

    // 建立 IO Target
    status = WdfIoTargetCreate(
        WdfObjectContextGetObject(Context),
        WDF_NO_OBJECT_ATTRIBUTES,
        &target);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 以名稱開啟 PDO
    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&openParams,
        &Context->PdoName,
        FILE_WRITE_ACCESS | FILE_READ_ACCESS);
    openParams.ShareAccess = FILE_SHARE_WRITE | FILE_SHARE_READ;

    status = WdfIoTargetOpen(target, &openParams);
    if (!NT_SUCCESS(status)) {
        WdfObjectDelete(target);
        return status;
    }

    // 準備緩衝區
    if (InputBuffer && InputLength > 0) {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, 
            InputBuffer, InputLength);
    }
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDesc, 
        OutputBuffer, OutputLength);

    // 發送 IOCTL
    status = WdfIoTargetSendIoctlSynchronously(
        target,
        NULL,  // Request
        IoControlCode,
        (InputBuffer && InputLength > 0) ? &inputDesc : NULL,
        &outputDesc,
        BytesReturned,
        NULL); // Timeout

    WdfObjectDelete(target);
    return status;
}
```

### 4. 讀取 USB 描述符

```c
NTSTATUS
M487GetDeviceDescriptor(
    PDEVICE_CONTEXT Context,
    PUSB_DEVICE_DESCRIPTOR Descriptor
    )
{
    WDFIOTARGET target;
    USB_DEVICE_DESCRIPTOR deviceDesc;
    URB urb;
    NTSTATUS status;
    ULONG bytesReturned;

    // 建立 IO Target
    status = M487OpenPdo(Context, &target);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 構建 URB
    RtlZeroMemory(&urb, sizeof(URB));
    urb.UrbHeader.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
    urb.UrbHeader.Length = sizeof(URB);
    urb.UrbDescriptorRequest.TransferBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
    urb.UrbDescriptorRequest.TransferBuffer = &deviceDesc;
    urb.UrbDescriptorRequest.TransferBufferMDL = NULL;
    urb.UrbDescriptorRequest.DescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
    urb.UrbDescriptorRequest.Index = 0;
    urb.UrbDescriptorRequest.LanguageId = 0;

    // 發送 URB
    status = WdfIoTargetSendUrbSynchronously(target, NULL, &urb);
    if (NT_SUCCESS(status)) {
        *Descriptor = deviceDesc;
    }

    WdfObjectDelete(target);
    return status;
}
```

## 📦 裝置上下文結構

```c
typedef struct _M487_DEVICE_CONTEXT {
    // PDO 名稱（用於開啟下層裝置）
    UNICODE_STRING PdoName;

    // USB 描述符資訊
    USHORT VendorId;
    USHORT ProductId;
    UCHAR DeviceClass;
    UCHAR DeviceSubClass;

    // 過濾選項
    BOOLEAN FilterEnabled;
    BOOLEAN LogReads;
    BOOLEAN LogWrites;

    // 統計
    ULONG ReadCount;
    ULONG WriteCount;
    ULONG IoctlCount;

    // 同步鎖
    WDFWAITLOCK Lock;

    // 額外的上下文
    PVOID DeviceExtension;

} M487_DEVICE_CONTEXT, *PM487_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(M487_DEVICE_CONTEXT, M487GetDeviceContext)
```

## 🔄 I/O 請求處理

### 讀取請求

```c
VOID
M487EvtIoRead(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t Length
    )
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PM487_DEVICE_CONTEXT context = M487GetDeviceContext(device);

    if (context->LogReads) {
        KdPrint(("M487: Read request, Length=%d\n", Length));
    }

    // 轉發至下層
    WdfRequestForwardToIoQueue(Request, WdfDeviceGetIoTarget(device));
}
```

### IOCTL 處理

```c
VOID
M487EvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
    )
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PM487_DEVICE_CONTEXT context = M487GetDeviceContext(device);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    size_t bytesReturned = 0;

    switch (IoControlCode) {

    case IOCTL_M487_GET_DEVICE_INFO:
        // 返回 USB 描述符資訊
        if (OutputBufferLength >= sizeof(USB_DEVICE_DESCRIPTOR)) {
            PVOID buffer;
            status = WdfRequestRetrieveOutputBuffer(Request, 
                OutputBufferLength, &buffer, NULL);
            if (NT_SUCCESS(status)) {
                RtlCopyMemory(buffer, &context->DeviceDescriptor, 
                    sizeof(USB_DEVICE_DESCRIPTOR));
                bytesReturned = sizeof(USB_DEVICE_DESCRIPTOR);
            }
        }
        break;

    case IOCTL_M487_SET_FILTER_MODE:
        // 設定過濾模式
        if (InputBufferLength >= sizeof(BOOLEAN)) {
            PVOID buffer;
            status = WdfRequestRetrieveInputBuffer(Request, 
                InputBufferLength, &buffer, NULL);
            if (NT_SUCCESS(status)) {
                context->FilterEnabled = *(PBOOLEAN)buffer;
                status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_M487_GET_STATISTICS:
        // 返回統計資訊
        if (OutputBufferLength >= sizeof(M487_STATISTICS)) {
            PM487_STATISTICS stats;
            status = WdfRequestRetrieveOutputBuffer(Request, 
                OutputBufferLength, &stats, NULL);
            if (NT_SUCCESS(status)) {
                stats->ReadCount = context->ReadCount;
                stats->WriteCount = context->WriteCount;
                stats->IoctlCount = context->IoctlCount;
                bytesReturned = sizeof(M487_STATISTICS);
            }
        }
        break;

    default:
        // 轉發未知 IOCTL
        WdfRequestForwardToIoQueue(Request, WdfDeviceGetIoTarget(device));
        return;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}
```

## 📝 INF 安裝檔案

```inf
[Version]
Signature="$Windows NT$"
Class=USB
ClassGUID={36FC9E60-C465-11CF-8056-444553540000}
Provider=%ProviderName%
DriverVer=01/01/2020,1.0.0.1
CatalogFile=M487Filter.cat
PnpLockdown=1

[DestinationDirs]
DefaultDestDir = 13

[SourceDisksNames]
1 = %DiskName%,,,""

[SourceDisksFiles]
M487Filter.sys = 1

[Manufacturer]
%MfgName%=Standard,NT$ARCH$.10.0....16299

[Standard.NT$ARCH$.10.0....16299]
%M487Device.DeviceDesc%=M487Filter_Install, USB\VID_0416&PID_5024

[M487Filter_Install.NT]
CopyFiles=M487Filter_CopyFiles
Include=usb.inf
Needs=USB.NT

[M487Filter_CopyFiles]
M487Filter.sys

; 關鍵：註冊為 Upper Filter
[M487Filter_Install.NT.HW]
AddReg = M487Filter_HWAddReg.NT

[M487Filter_HWAddReg.NT]
HKR,,"UpperFilters",0x00010000,"M487Filter"

[M487Filter_Install.NT.Services]
AddService = M487Filter, , M487Filter_Service_Inst

[M487Filter_Service_Inst]
DisplayName    = %M487FilterSvcDesc%
ServiceType    = 1        ; SERVICE_KERNEL_DRIVER
StartType      = 3        ; SERVICE_DEMAND_START
ErrorControl   = 1        ; SERVICE_ERROR_NORMAL
ServiceBinary  = %13%\M487Filter.sys

[M487Filter_Install.NT.Wdf]
KmdlService = M487Filter, M487Filter_wdfsect

[M487Filter_wdfsect]
KmdlLibraryVersion = $KMDFVERSION$

[Strings]
ProviderName = "Nuvoton"
MfgName = "Nuvoton"
DiskName = "M487 Filter Driver Disk"
M487Device.DeviceDesc = "Nuvoton M487 USB Device"
M487FilterSvcDesc = "Nuvoton M487 USB Filter Driver"
```

## 🔑 與 Firefly 的差異

| 功能 | Firefly | M487 Filter |
|------|---------|-------------|
| 目標硬體 | HID 滑鼠 | USB 儲存/HID |
| 主要 IOCTL | HID Feature | 自訂 IOCTL + USB |
| 通訊介面 | WMI | IOCTL + WMI |
| 報告類型 | Feature Report | SCSI/USB Transfer |
| 複雜度 | 中等 | 中等 |

## 📊 開發步驟建議

1. **基礎專案設定**
   - 建立 KMDF 專案
   - 設定為 Filter Driver

2. **EvtDeviceAdd 實作**
   - `WdfFdoInitSetFilter()`
   - 儲存 PDO 名稱
   - 設定 I/O 佇列

3. **基本轉發**
   - 實現讀取/寫入轉發
   - 實現 IOCTL 轉發

4. **USB 描述符讀取**
   - 開啟 PDO
   - 發送 URB
   - 解析描述符

5. **過濾功能**
   - 添加過濾選項
   - 實作資料修改邏輯

6. **使用者介面**
   - 實作 IOCTL 介面
   - 或添加 WMI 支援

7. **測試和驗證**
   - 連接 M487 裝置
   - 驗證過濾功能
   - 效能測試

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
