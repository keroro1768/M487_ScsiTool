# Filter Driver 模式與實作

## 📋 概述

本文件詳細說明 KMDF Filter Driver 的常見設計模式和實作方式。

## 🔧 基本架構

### KMDF Filter Driver vs WDM Filter Driver

| 特性 | WDM | KMDF |
|------|-----|------|
| 程式碼量 | 多 | 少 |
| 複雜度 | 高 | 低 |
| 請求轉發 | 手動 | Framework 協助 |
| PnP/Power | 手動處理 | Framework 自動處理 |

## 📝 KMDF Filter Driver 範本

### DriverEntry

```c
#include <ntddk.h>
#include <wdf.h>

// DRIVERNAME for debug output
#define DRIVERNAME "MyFilter.sys: "

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint((DRIVERNAME "DriverEntry\n"));

    // Initialize driver configuration with device add callback
    WDF_DRIVER_CONFIG_INIT(&config, FilterEvtDeviceAdd);

    // Create WDF driver object
    status = WdfDriverCreate(DriverObject,
                            RegistryPath,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            &config,
                            WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        KdPrint((DRIVERNAME "WdfDriverCreate failed 0x%x\n", status));
    }

    return status;
}
```

### EvtDeviceAdd

```c
NTSTATUS
FilterEvtDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG ioQueueConfig;

    PAGED_CODE();

    // CRITICAL: Set device as a filter driver
    // This tells framework to inherit all flags and characteristics
    // from the lower device
    WdfFdoInitSetFilter(DeviceInit);

    // Initialize device extension context
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FILTER_EXTENSION);
    
    // Create the device
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint((DRIVERNAME "WdfDeviceCreate failed 0x%x\n", status));
        return status;
    }

    // Configure default queue
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                             WdfIoQueueDispatchParallel);

    // Register I/O callbacks
    ioQueueConfig.EvtIoRead = FilterEvtIoRead;
    ioQueueConfig.EvtIoWrite = FilterEvtIoWrite;
    ioQueueConfig.EvtIoDeviceControl = FilterEvtIoDeviceControl;

    // Framework creates non-power managed queues for filter drivers
    status = WdfIoQueueCreate(device,
                            &ioQueueConfig,
                            WDF_NO_OBJECT_ATTRIBUTES,
                            WDF_NO_HANDLE);

    return status;
}
```

### Filter 擴展結構

```c
typedef struct _FILTER_EXTENSION
{
    WDFDEVICE WdfDevice;
    
    // Add filter-specific data here
    BOOLEAN FilterEnabled;
    
    // Statistics
    ULONG ReadCount;
    ULONG WriteCount;
    ULONG IoctlCount;

} FILTER_EXTENSION, *PFILTER_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILTER_EXTENSION, FilterGetData)
```

## 🔄 請求轉發模式

### 模式 1：Fire and Forget（簡單轉發）

適用於不需要處理請求結果的情況：

```c
VOID
FilterForwardRequest(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target
    )
{
    WDF_REQUEST_SEND_OPTIONS options;
    BOOLEAN ret;
    NTSTATUS status;

    // Fire and forget - don't wait for completion
    WDF_REQUEST_SEND_OPTIONS_INIT(&options,
                                  WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    ret = WdfRequestSend(Request, Target, &options);

    if (ret == FALSE) {
        // Send failed - complete with error
        status = WdfRequestGetStatus(Request);
        WdfRequestComplete(Request, status);
    }
}

VOID
FilterEvtIoRead(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t Length
    )
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    
    // Log if needed
    PFILTER_EXTENSION ext = FilterGetData(device);
    InterlockedIncrement(&ext->ReadCount);
    
    // Forward to lower driver
    FilterForwardRequest(Request, WdfDeviceGetIoTarget(device));
}
```

### 模式 2：帶完成的轉發

適用於需要處理請求結果的情況：

```c
#define FORWARD_WITH_COMPLETION 1

VOID
FilterForwardRequestWithCompletionRoutine(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target
    )
{
    BOOLEAN ret;
    NTSTATUS status;

    // Format request for lower driver
    WdfRequestFormatRequestUsingCurrentType(Request);

    // Set completion routine
    WdfRequestSetCompletionRoutine(Request,
                                FilterRequestCompletionRoutine,
                                WDF_NO_CONTEXT);

    // Send asynchronously
    ret = WdfRequestSend(Request, Target, WDF_NO_SEND_OPTIONS);

    if (ret == FALSE) {
        status = WdfRequestGetStatus(Request);
        WdfRequestComplete(Request, status);
    }
}

VOID
FilterRequestCompletionRoutine(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT Context
    )
{
    NTSTATUS status = CompletionParams->IoStatus.Status;
    size_t info = CompletionParams->IoStatus.Information;

    // POST-PROCESSING HERE
    if (NT_SUCCESS(status)) {
        // Modify output data if needed
        PVOID buffer;
        size_t length;
        
        if (WdfRequestGetOutputBuffer(Request, sizeof(buffer), &buffer, &length) 
            == STATUS_SUCCESS) {
            // Modify buffer contents
            ModifyOutputBuffer(buffer, length);
        }
    }

    // Complete the request
    WdfRequestCompleteWithInformation(Request, status, info);
}
```

### 模式 3：同步轉發（等待結果）

適用於需要同步處理的特殊情况：

```c
NTSTATUS
FilterSendSynchronously(
    WDFDEVICE Device,
    WDFREQUEST Request
    )
{
    WDF_REQUEST_SEND_OPTIONS options;
    WDFIOTARGET target;
    NTSTATUS status;

    target = WdfDeviceGetIoTarget(Device);

    WDF_REQUEST_SEND_OPTIONS_INIT(&options, 0);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options, 
        WDF_REL_TIMEOUT_IN_SEC(5));  // 5 second timeout

    if (WdfRequestSend(Request, target, &options) == FALSE) {
        status = WdfRequestGetStatus(Request);
    } else {
        status = STATUS_SUCCESS;
    }

    return status;
}
```

## 🎯 請求攔截模式

### 讀取請求修改

```c
VOID
FilterEvtIoRead(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t Length
    )
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PFILTER_EXTENSION ext = FilterGetData(device);
    NTSTATUS status;

    // Option 1: Just forward (transparent)
    if (ext->FilterEnabled == FALSE) {
        FilterForwardRequest(Request, WdfDeviceGetIoTarget(device));
        return;
    }

    // Option 2: Forward with completion to modify data
    status = WdfRequestRetrieveOutputBuffer(Request, Length, &buffer, &actualLength);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }

    // Modify data before forwarding
    MyModifyReadData(buffer, actualLength);

    // Forward
    FilterForwardRequestWithCompletionRoutine(Request, 
        WdfDeviceGetIoTarget(device));
}
```

### IOCTL 攔截

```c
VOID
FilterEvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
    )
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    NTSTATUS status = STATUS_SUCCESS;

    switch (IoControlCode) {
    
    case MY_CUSTOM_IOCTL:
        // Handle custom IOCTL
        status = HandleCustomIoctl(Request, InputBufferLength, OutputBufferLength);
        if (NT_SUCCESS(status)) {
            WdfRequestCompleteWithInformation(Request, status, outputLen);
        }
        break;

    case IOCTL_USB_FILTER_READ:
        // Read with filtering
        status = FilteredRead(Request, OutputBufferLength);
        if (status == STATUS_PENDING) {
            return;  // Will complete asynchronously
        }
        WdfRequestComplete(Request, status);
        break;

    default:
        // Forward unknown IOCTLs
        FilterForwardRequest(Request, WdfDeviceGetIoTarget(device));
        break;
    }
}
```

## 🪝 USB/HID Filter 特殊處理

### 開啟 PDO 發送 IOCTL

```c
NTSTATUS
FilterOpenPdoAndSendIoctl(
    PDEVICE_CONTEXT DeviceContext,
    ULONG IoControlCode,
    PVOID InputBuffer,
    size_t InputLength,
    PVOID OutputBuffer,
    size_t OutputLength,
    PULONG BytesReturned
    )
{
    WDFIOTARGET hidTarget;
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    WDF_MEMORY_DESCRIPTOR inputDesc, outputDesc;
    NTSTATUS status;

    // Create IO Target
    status = WdfIoTargetCreate(
        WdfObjectContextGetObject(DeviceContext),
        WDF_NO_OBJECT_ATTRIBUTES,
        &hidTarget);

    // Open by PDO name (stored during AddDevice)
    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&openParams,
        &DeviceContext->PdoName,
        FILE_WRITE_ACCESS | FILE_READ_ACCESS);
    openParams.ShareAccess = FILE_SHARE_WRITE | FILE_SHARE_READ;

    status = WdfIoTargetOpen(hidTarget, &openParams);

    // Prepare buffers
    if (InputBuffer) {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, InputBuffer, InputLength);
    }
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDesc, OutputBuffer, OutputLength);

    // Send IOCTL synchronously
    status = WdfIoTargetSendIoctlSynchronously(
        hidTarget,
        NULL,               // Request
        IoControlCode,
        InputBuffer ? &inputDesc : NULL,
        &outputDesc,
        NULL,               // BytesReturned
        NULL);              // Timeout

    WdfObjectDelete(hidTarget);

    return status;
}
```

## 📝 INF 檔案範本

### Upper Filter INF

```inf
[Version]
Signature="$Windows NT$"
Class=USB
ClassGUID={36FC9E60-C465-11CF-8056-444553540000}
Provider=%ProviderName%
DriverVer=01/01/2020,1.0.0.1
CatalogFile=KmdlSamples.cat
PnpLockdown=1

[DestinationDirs]
DefaultDestDir = 13

[SourceDisksNames]
1 = %DiskName%,,,""

[SourceDisksFiles]
MyFilter.sys = 1

[Manufacturer]
%MfgName%=Standard,NT$ARCH$.10.0....16299

[Standard.NT$ARCH$.10.0....16299]
%MyDevice.DeviceDesc%=MyDevice_Install, USB\VID_1234&PID_5678

[MyDevice_Install.NT]
CopyFiles=MyDevice_CopyFiles

[MyDevice_CopyFiles]
MyFilter.sys

; CRITICAL: Register as Upper Filter
[MyDevice_Install.NT.HW]
AddReg = MyDevice_HWAddReg.NT

[MyDevice_HWAddReg.NT]
HKR,,"UpperFilters",0x00010000,"MyFilter"

[MyDevice_Install.NT.Services]
AddService = MyFilter, , Filter_Service_Inst

[Filter_Service_Inst]
DisplayName    = %FilterSvcDesc%
ServiceType    = 1        ; SERVICE_KERNEL_DRIVER
StartType      = 3        ; SERVICE_DEMAND_START
ErrorControl   = 1        ; SERVICE_ERROR_NORMAL
ServiceBinary  = %13%\MyFilter.sys

[MyDevice_Install.NT.Wdf]
KmdlService = MyFilter, Filter_wdfsect

[Filter_wdfsect]
KmdlLibraryVersion = $KMDFVERSION$

[Strings]
ProviderName = "My Company"
MfgName = "My Company"
DiskName = "My Filter Driver Disk"
MyDevice.DeviceDesc = "My USB Device"
FilterSvcDesc = "My USB Filter Driver"
```

## 🔄 請求處理流程圖

```
┌─────────────────────────────────────────────────────────┐
│                    Request Received                       │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│              EvtIo* Callback (Read/Write/Ioctl)        │
│                                                         │
│  1. Log/Count request                                   │
│  2. Optionally modify request data                      │
│  3. Check if we should filter                          │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
        ┌─────────────────┴─────────────────┐
        │   Filter Enabled?                │
        └─────────────────┬─────────────────┘
              │           │           │
            YES          NO           ▼
              │           │    ┌──────────────────┐
              │           │    │ Forward Request   │
              │           │    │ to lower driver  │
              │           │    │ (sync/async)     │
              │           │    └──────────────────┘
              │           │
              ▼           ▼
        ┌────────────────────┐
        │ Forward with       │
        │ completion routine  │
        │ or modify & forward│
        └─────────┬──────────┘
                  │
                  ▼
        ┌────────────────────┐
        │ Lower Driver       │
        │ Processes Request  │
        └─────────┬──────────┘
                  │
                  ▼
        ┌────────────────────┐
        │ Completion Routine │
        │ (if set)           │
        │ - Post-process     │
        │ - Modify output    │
        │ - Log results      │
        └─────────┬──────────┘
                  │
                  ▼
        ┌────────────────────┐
        │ Complete Request   │
        │ to upper caller    │
        └────────────────────┘
```

## ⚠️ 常見陷阱

1. **忘記設定 Filter** - `WdfFdoInitSetFilter()` 是必需的
2. **電源管理** - Framework 預設為 filter 建立非電源管理佇列
3. **PDO 名稱** - 儲存名稱用於後續開啟
4. **請求生命週期** - 確保請求不會懸空
5. **鎖定** - 使用 Framework 的鎖定機制而非自旋鎖

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
