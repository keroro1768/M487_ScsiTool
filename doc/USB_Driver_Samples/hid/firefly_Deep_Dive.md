# Firefly HID Upper Filter Driver 深入分析

## 🔥 概述

Firefly 是 Microsoft 提供的 KMDF HID 上層過濾驅動程式範例，專為 Microsoft USB Intellimouse Optical 設計。它展示如何：
1. 作為 HID 裝置的 Upper Filter 附加至裝置堆疊
2. 使用 WMI 介面與使用者模式應用程式通訊
3. 透過 IO Target 發送 HID IOCTL 控制滑鼠燈光

**這是 M487 USB Filter Driver 最關鍵的參考範例！**

## 📁 專案結構

```
hid/firefly/
├── app/                # 使用者模式應用程式 (flicker.exe)
│   └── firefly.cpp
├── driver/            # 🔑 核心驅動程式碼
│   ├── device.c       # 裝置初始化與 WMI 初始化
│   ├── device.h       # 裝置上下文定義
│   ├── driver.c       # DriverEntry 入口點
│   ├── firefly.h      # 主要標頭檔
│   ├── firefly.inx    # INF 安裝檔案範本
│   ├── firefly.mof    # WMI 類別定義
│   ├── firefly.rc     # 資源檔案
│   ├── magic.h        # HID Feature 常數定義
│   ├── vfeature.c     # 🔑 HID Feature 報告處理
│   ├── vfeature.h     # Feature 函式宣告
│   ├── wmi.c          # 🔑 WMI 處理常式
│   └── wmi.h          # WMI 函式宣告
├── lib/               # 共享函式庫 (luminous.lib)
└── sauron/           # Windows Media Player 視覺化 DLL
```

## 🔧 DriverEntry 分析

**位置：** `driver/driver.c`

```c
/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:
    driver.c

Abstract:
    This modules contains the Windows Driver Framework Driver object
    handlers for the firefly filter driver.

Environment:
    Kernel mode

--*/

#include "FireFly.h"

// Main driver entry, initialize the framework, register driver event handlers
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG params;
    NTSTATUS  status;

    KdPrint(("FireFly: DriverEntry - WDF version built on %s %s\n", 
                            __DATE__, __TIME__));

    // Initialize WDF driver configuration with device add callback
    WDF_DRIVER_CONFIG_INIT(
                        &params,
                        FireFlyEvtDeviceAdd
                        );

    // Create the framework WDFDRIVER object
    status = WdfDriverCreate(DriverObject, 
                             RegistryPath, 
                             WDF_NO_OBJECT_ATTRIBUTES, 
                             &params, 
                             WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        // Framework will automatically cleanup on error Status return
        KdPrint(("FireFly: Error Creating WDFDRIVER 0x%x\n", status));
    }

    return status;
}
```

**關鍵點：**
- 標準 KMDF DriverEntry 模式
- 只註冊 `EvtDeviceAdd` 回調，不需其他複雜設定
- Framework 自動處理 Driver unload

## 📦 裝置上下文定義

**位置：** `driver/device.h`

```c
// The device context performs the same job as a WDM device extension
typedef struct _DEVICE_CONTEXT
{
    // Our WMI data generated from firefly.mof
    FireflyDeviceInformation WmiInstance;

    // Store the PDO name for opening the device
    UNICODE_STRING PdoName;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

// Declare context type for WDFDEVICE
WDF_DECLARE_CONTEXT_TYPE(DEVICE_CONTEXT)

// Declare context accessor with custom name
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FireflyDeviceInformation, InstanceGetInfo)

// Cleanup callback declaration
EVT_WDF_DEVICE_CONTEXT_CLEANUP EvtDeviceContextCleanup;
```

## 🔌 EvtDeviceAdd 分析

**位置：** `driver/device.c`

```c
NTSTATUS
FireFlyEvtDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_OBJECT_ATTRIBUTES           attributes;
    NTSTATUS                        status;
    PDEVICE_CONTEXT                 pDeviceContext;
    WDFDEVICE                       device;
    WDFMEMORY                       memory;
    size_t                          bufferLength;

    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();

    //
    // Configure the device as a filter driver
    // This is the KEY step for filter drivers!
    //
    WdfFdoInitSetFilter(DeviceInit);

    // Initialize object attributes with our context type
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

    // Create the WDFDEVICE
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("FireFly: WdfDeviceCreate, Error %x\n", status));
        return status;
    }

    // Framework always zero initializes context memory
    pDeviceContext = WdfObjectGet_DEVICE_CONTEXT(device);

    // Initialize WMI support
    status = WmiInitialize(device, pDeviceContext);
    if (!NT_SUCCESS(status)) {
        KdPrint(("FireFly: Error initializing WMI 0x%x\n", status));
        return status;
    }

    //
    // IMPORTANT: In order to send ioctls to our PDO, we have to open it
    // by name so that we have a valid filehandle (fileobject).
    // When we send ioctls using IoTarget, framework automatically 
    // sets the fileobject in the stack location.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    // By parenting it to device, we don't have to worry about
    // deleting explicitly. It will be deleted along with the device.
    attributes.ParentObject = device;

    status = WdfDeviceAllocAndQueryProperty(device,
                                    DevicePropertyPhysicalDeviceObjectName,
                                    NonPagedPoolNx,
                                    &attributes,
                                    &memory);

    if (!NT_SUCCESS(status)) {
        KdPrint(("FireFly: WdfDeviceAllocAndQueryProperty failed 0x%x\n", status));        
        return STATUS_UNSUCCESSFUL;
    }

    pDeviceContext->PdoName.Buffer = WdfMemoryGetBuffer(memory, &bufferLength);
    if (pDeviceContext->PdoName.Buffer == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    pDeviceContext->PdoName.MaximumLength = (USHORT) bufferLength;
    pDeviceContext->PdoName.Length = (USHORT) bufferLength-sizeof(UNICODE_NULL);

    return status;
}
```

**關鍵步驟：**

1. **`WdfFdoInitSetFilter(DeviceInit)`** - 將裝置設定為 Filter Driver
2. **建立 WMI 支援** - 允許使用者模式應用程式通訊
3. **儲存 PDO 名稱** - 用於後續開啟下層裝置

## 🎯 WMI 處理分析

**位置：** `driver/wmi.c`

### WMI 初始化

```c
NTSTATUS
WmiInitialize(
    WDFDEVICE       Device,
    PDEVICE_CONTEXT DeviceContext
    )
{
    WDF_WMI_PROVIDER_CONFIG providerConfig;
    WDF_WMI_INSTANCE_CONFIG instanceConfig;
    WDF_OBJECT_ATTRIBUTES woa;
    WDFWMIINSTANCE instance;
    NTSTATUS status;
    DECLARE_CONST_UNICODE_STRING(mofRsrcName, MOFRESOURCENAME);

    PAGED_CODE();

    // Assign MOF resource name for WMI
    status = WdfDeviceAssignMofResourceName(Device, &mofRsrcName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize provider config with our GUID
    WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &FireflyDeviceInformation_GUID);
    providerConfig.MinInstanceBufferSize = sizeof(FireflyDeviceInformation);

    // Initialize instance config
    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);
    instanceConfig.Register = TRUE;
    
    // Register WMI callbacks
    instanceConfig.EvtWmiInstanceQueryInstance = EvtWmiInstanceQueryInstance;
    instanceConfig.EvtWmiInstanceSetInstance = EvtWmiInstanceSetInstance;
    instanceConfig.EvtWmiInstanceSetItem = EvtWmiInstanceSetItem;

    // Set context type
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&woa, FireflyDeviceInformation);

    // Create WMI instance
    status = WdfWmiInstanceCreate(Device, &instanceConfig, &woa, &instance);

    if (NT_SUCCESS(status)) {
        FireflyDeviceInformation* info;
        info = InstanceGetInfo(instance);
        info->TailLit = TRUE;  // Default: light on
    }

    return status;
}
```

### WMI 資料類別（MOF 定義）

**位置：** `driver/firefly.mof`

```mof
[Dynamic, Provider("WMIProv"),
 WMI,
 Description("Firefly driver information"),
 guid("{AB27DB29-DB25-42E6-A3E7-28BD46BDB666}"),
 locale("MS\\0x409")]
class FireflyDeviceInformation
{
    [key, read]
     string InstanceName;
    [read] boolean Active;

    [WmiDataId(1),
     read,
     write,
     Description("Current state of the tail light.")]
    boolean TailLit;
};
```

### WMI Set 處理（核心功能）

```c
NTSTATUS
EvtWmiInstanceSetInstance(
    IN  WDFWMIINSTANCE WmiInstance,
    IN  ULONG InBufferSize,
    IN  PVOID InBuffer
    )
{
    FireflyDeviceInformation* pInfo;
    ULONG length;
    NTSTATUS status;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(InBufferSize);

    pInfo = InstanceGetInfo(WmiInstance);
    length = sizeof(*pInfo);

    // Copy data from user buffer
    RtlMoveMemory(pInfo, InBuffer, length);

    // Tell the HID device about the new tail light state
    status = FireflySetFeature(
        WdfObjectGet_DEVICE_CONTEXT(WdfWmiInstanceGetDevice(WmiInstance)),
        TAILLIGHT_PAGE,      // 0xFF
        TAILLIGHT_FEATURE,   // 0x02
        pInfo->TailLit       // TRUE/FALSE
        );

    return status;
}
```

## 🔥 HID Feature 報告處理（核心）

**位置：** `driver/vfeature.c`

這是整個範例最關鍵的部分！展示如何發送 HID IOCTL 來控制 HID 裝置。

```c
#include "firefly.h"

#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <hidpddi.h>      // HID Parser DDI
#include <hidclass.h>     // HID Class IOCTLs

NTSTATUS
FireflySetFeature(
    IN  PDEVICE_CONTEXT DeviceContext,
    IN  UCHAR           PageId,       // Usage Page (e.g., 0xFF for vendor)
    IN  USHORT          FeatureId,    // Usage ID
    IN  BOOLEAN         EnableFeature // TRUE=on, FALSE=off
    )
{
    WDF_MEMORY_DESCRIPTOR       inputDescriptor, outputDescriptor;
    NTSTATUS                    status;
    HID_COLLECTION_INFORMATION  collectionInformation = {0};
    PHIDP_PREPARSED_DATA       preparsedData;
    HIDP_CAPS                  caps;
    USAGE                      usage;
    ULONG                       usageLength;
    PCHAR                       report;
    WDFIOTARGET                hidTarget;
    WDF_IO_TARGET_OPEN_PARAMS   openParams;

    PAGED_CODE();

    // Preinit for error cleanup
    preparsedData = NULL;
    report = NULL;
    hidTarget = NULL;
    
    // Step 1: Create IO Target for the PDO
    status = WdfIoTargetCreate(WdfObjectContextGetObject(DeviceContext), 
                            WDF_NO_OBJECT_ATTRIBUTES, 
                            &hidTarget);    
    if (!NT_SUCCESS(status)) {
        KdPrint(("FireFly: WdfIoTargetCreate failed 0x%x\n", status));        
        return status;
    }

    // Step 2: Open the PDO by name (WRITE access only!)
    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
                                    &openParams,
                                    &DeviceContext->PdoName,
                                    FILE_WRITE_ACCESS);

    // Let framework respond to PnP state changes automatically
    openParams.ShareAccess = FILE_SHARE_WRITE | FILE_SHARE_READ;

    status = WdfIoTargetOpen(hidTarget, &openParams);
    if (!NT_SUCCESS(status)) {
        KdPrint(("FireFly: WdfIoTargetOpen failed 0x%x\n", status));                
        goto ExitAndFree;
    }

    // Step 3: Get HID Collection Information
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor,
                                      (PVOID) &collectionInformation,
                                      sizeof(HID_COLLECTION_INFORMATION));

    status = WdfIoTargetSendIoctlSynchronously(hidTarget,
                                  NULL,
                                  IOCTL_HID_GET_COLLECTION_INFORMATION,
                                  NULL,
                                  &outputDescriptor,
                                  NULL,
                                  NULL);

    if (!NT_SUCCESS(status)) {
        goto ExitAndFree;
    }

    // Step 4: Allocate buffer for preparsed data
    preparsedData = (PHIDP_PREPARSED_DATA) ExAllocatePool2(
        POOL_FLAG_NON_PAGED, collectionInformation.DescriptorSize, 'ffly');

    if (preparsedData == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitAndFree;
    }

    // Step 5: Get Collection Descriptor (preparsed data)
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor,
                                      (PVOID) preparsedData,
                                      collectionInformation.DescriptorSize);

    status = WdfIoTargetSendIoctlSynchronously(hidTarget,
                                  NULL,
                                  IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                  NULL,
                                  &outputDescriptor,
                                  NULL,
                                  NULL);

    if (!NT_SUCCESS(status)) {
        goto ExitAndFree;
    }

    // Step 6: Get HID Capabilities using parser
    RtlZeroMemory(&caps, sizeof(HIDP_CAPS));
    status = HidP_GetCaps(preparsedData, &caps);
    if (!NT_SUCCESS(status)) {
        goto ExitAndFree;
    }

    // Step 7: Create Feature Report buffer
    report = (PCHAR) ExAllocatePool2(
        POOL_FLAG_NON_PAGED, caps.FeatureReportByteLength, 'ffly');

    if (report == NULL) {
        goto ExitAndFree;
    }

    // Start with zeroed report
    status = STATUS_SUCCESS;

    if (EnableFeature) {
        // Step 8: Set the usage in the report
        usage = FeatureId;
        usageLength = 1;

        status = HidP_SetUsages(
            HidP_Feature,        // Report type
            PageId,              // Usage Page
            0,                   // Link Collection (unused)
            &usage,              // pointer to the usage list
            &usageLength,        // number of usages in the usage list
            preparsedData,       // preparsed data
            report,              // report buffer
            caps.FeatureReportByteLength
            );
        
        if (!NT_SUCCESS(status)) {
            goto ExitAndFree;
        }
    }

    // Step 9: Send Feature Report to device
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor,
                                      report,
                                      caps.FeatureReportByteLength);
    
    status = WdfIoTargetSendIoctlSynchronously(hidTarget,
                                  NULL,
                                  IOCTL_HID_SET_FEATURE,
                                  &inputDescriptor,
                                  NULL,
                                  NULL,
                                  NULL);

ExitAndFree:
    // Cleanup
    if (preparsedData != NULL) {
        ExFreePool(preparsedData);
    }
    if (report != NULL) {
        ExFreePool(report);
    }
    if (hidTarget != NULL) {
        WdfObjectDelete(hidTarget);
    }

    return status;
}
```

**HID Feature 設定流程總結：**

1. **建立 WDFIOTARGET** - 用於與 PDO 通訊
2. **以名稱開啟 PDO** - `WdfIoTargetOpen` with PDO name
3. **發送 `IOCTL_HID_GET_COLLECTION_INFORMATION`** - 獲取集合資訊
4. **發送 `IOCTL_HID_GET_COLLECTION_DESCRIPTOR`** - 獲取預解析資料
5. **呼叫 `HidP_GetCaps`** - 解析 HID 能力
6. **呼叫 `HidP_SetUsages`** - 在報告緩衝區中設定 Usage
7. **發送 `IOCTL_HID_SET_FEATURE`** - 將 Feature 報告傳送至裝置

## 📝 INF 安裝檔案分析

**位置：** `driver/firefly.inx`

```inf
[Version]
Signature="$Windows NT$"
Class=Mouse
ClassGUID={4D36E96F-E325-11CE-BFC1-08002BE10318}
Provider=%Provider%
DriverVer=03/17/2001,1.0.0.1
CatalogFile=KmdlSamples.cat
PnpLockdown=1

[Manufacturer]
%ShinyThings%=ShinyThingsMfg,NT$ARCH$.10.0....16299

; Target devices - specific Microsoft mice
[ShinyThingsMfg.NT$ARCH$.10.0....16299]
%HID\Vid_045E&Pid_001E.DeviceDesc%=Firefly_Inst, HID\Vid_045E&Pid_001E
%HID\Vid_045E&Pid_0029.DeviceDesc%=Firefly_Inst, HID\Vid_045E&Pid_0029
%HID\Vid_045E&Pid_0039.DeviceDesc%=Firefly_Inst, HID\Vid_045E&Pid_0039
%HID\Vid_045E&Pid_0040.DeviceDesc%=Firefly_Inst, HID\Vid_045E&Pid_0040
%HID\Vid_045E&Pid_0047.DeviceDesc%=Firefly_Inst, HID\Vid_045E&Pid_0047

; Installation section
[Firefly_Inst.NT]
Include = MSMOUSE.INF
Needs = HID_Mouse_Inst.NT
CopyFiles = Firefly_Inst_CopyFiles.NT

; HW AddReg - THIS IS THE KEY PART FOR UPPER FILTER!
[Firefly_Inst.NT.HW]
Include = MSMOUSE.INF
Needs = HID_Mouse_Inst.NT.HW
AddReg = Firefly_Inst_HWAddReg.NT

; Register as Upper Filter
[Firefly_Inst_HWAddReg.NT]
HKR,,"UpperFilters",0x00010000,"Firefly"

; Service installation
[Firefly_Inst.NT.Services]
Include = MSMOUSE.INF
Needs = HID_Mouse_Inst.NT.Services
AddService = Firefly, , Firefly_Service_Inst

[Firefly_Service_Inst]
DisplayName    = %Firefly.SvcDesc%
ServiceType    = 1        ; SERVICE_KERNEL_DRIVER
StartType      = 3        ; SERVICE_DEMAND_START
ErrorControl   = 1        ; SERVICE_ERROR_NORMAL
ServiceBinary  = %13%\Firefly.sys

; KMDF co-installler registration
[Firefly_Inst.NT.Wdf]
KmdlService = Firefly, Firefly_wdfsect

[Firefly_wdfsect]
KmdlLibraryVersion = $KMDFVERSION$

[Strings]
ShinyThings = "Shiny Things"
Firefly.SvcDesc = "Firefly Service"
HID\VID_045E&PID_001E.DeviceDesc = "Shiny Things Firefly Mouse"
HID\VID_045E&PID_0029.DeviceDesc = "Shiny Things Firefly Mouse"
; ... (more PIDs)
```

**關鍵 INF 設定：**

1. **Target Hardware IDs** - 指定要過濾的 VID/PID
   ```
   HID\Vid_045E&Pid_001E
   ```

2. **Include/Needs** - 包含基礎滑鼠驅動程式
   ```
   Include = MSMOUSE.INF
   Needs = HID_Mouse_Inst.NT
   ```

3. **UpperFilters 註冊** - 最重要的部分！
   ```inf
   [Firefly_Inst_HWAddReg.NT]
   HKR,,"UpperFilters",0x00010000,"Firefly"
   ```

4. **KMDF 版本註冊** - 確保正確的 KMDF 版本
   ```inf
   [Firefly_Inst.NT.Wdf]
   KmdlService = Firefly, Firefly_wdfsect
   ```

## 🔑 常數定義

**位置：** `driver/magic.h`

```c
// Vendor-specific HID page for Microsoft optical mice
#define TAILLIGHT_PAGE      0xFF
#define TAILLIGHT_FEATURE   0x02
```

## 📊 架構流程圖

```
┌─────────────────────────────────────────────────────────────────┐
│                        User Mode                                 │
│  ┌──────────────┐                                               │
│  │ flicker.exe  │                                               │
│  └──────┬───────┘                                               │
│         │ WMI COM Interface                                      │
└─────────┼─────────────────────────────────────────────────────────┘
          │
┌─────────┼─────────────────────────────────────────────────────────┐
│         ▼         Kernel Mode                                   │
│  ┌──────────────┐                                               │
│  │  firefly.sys │  (Upper Filter)                              │
│  │              │  - WMI Provider                               │
│  │  WMI Callbacks ──────► FireflySetFeature()                  │
│  │              │           │                                  │
│  │              │           ├── IOCTL_HID_GET_COLLECTION_INFO  │
│  │              │           ├── IOCTL_HID_GET_COLLECTION_DESC  │
│  │              │           ├── HidP_GetCaps()                  │
│  │              │           ├── HidP_SetUsages()                │
│  │              │           └── IOCTL_HID_SET_FEATURE          │
│  └──────┬───────┘           │                                  │
│         │ IO Target          │                                  │
└─────────┼───────────────────┼───────────────────────────────────┘
          ▼                   ▼
┌─────────────────────────────────────────────────────────────────┐
│  ┌──────────────┐     ┌──────────────┐     ┌──────────────┐     │
│  │   mouhid.sys │ ──► │  mouclass.sys │ ──► │  hidusb.sys  │     │
│  │  (HID Minidriver)│    │ (HID Class)    │     │ (HID USB)     │     │
│  └──────────────┘     └──────────────┘     └──────────────┘     │
│         │                                                       │
│         ▼                                                       │
│  ┌──────────────┐                                               │
│  │   USB Hub    │                                               │
│  └──────────────┘                                               │
└─────────────────────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────────┐
│                     USB Hardware                                 │
│              Microsoft USB Intellimouse                          │
└─────────────────────────────────────────────────────────────────┘
```

## 🎓 對 M487 的啟示

Firefly 範例為 M487 USB Filter Driver 提供了以下關鍵設計模式：

### 1. Filter Driver 附加方式
```c
// 在 EvtDeviceAdd 中設定為 Filter
WdfFdoInitSetFilter(DeviceInit);
```

### 2. PDO 名稱獲取與開啟
```c
// 獲取 PDO 名稱
WdfDeviceAllocAndQueryProperty(device,
    DevicePropertyPhysicalDeviceObjectName,
    NonPagedPoolNx, &attributes, &memory);

// 開啟 PDO
WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&openParams,
    &DeviceContext->PdoName, FILE_WRITE_ACCESS);
WdfIoTargetOpen(hidTarget, &openParams);
```

### 3. 發送 HID IOCTL
```c
WdfIoTargetSendIoctlSynchronously(hidTarget,
    NULL, IOCTL_HID_XXX, inputDesc, outputDesc, NULL, NULL);
```

### 4. Feature 報告操作
```c
// 使用 HID Parser
HidP_GetCaps(preparsedData, &caps);
HidP_SetUsages(HidP_Feature, page, 0, &usage, &usageLength, 
               preparsedData, report, reportLen);
```

### 5. INF UpperFilters 註冊
```inf
HKR,,"UpperFilters",0x00010000,"M487Filter"
```

## 📌 總結

Firefly 是學習 KMDF HID Filter Driver 的完美範例：

| 特性 | 實作方式 |
|------|----------|
| Filter 附加 | `WdfFdoInitSetFilter()` |
| PDO 通訊 | `WdfIoTargetOpen()` + `WdfIoTargetSendIoctlSynchronously()` |
| Feature 報告 | `HidP_SetUsages()` + `IOCTL_HID_SET_FEATURE` |
| 使用者通訊 | WMI Provider |
| INF 註冊 | UpperFilters registry key |

**這個範例可以直接作為 M487 USB Filter Driver 的設計藍圖！**

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**重要性：** ⭐⭐⭐⭐⭐ (最關鍵參考)
