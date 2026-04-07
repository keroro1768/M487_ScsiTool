# usbsamp 通用 USB 裝置驅動程式分析

## 📋 概述

usbsamp 是 Microsoft 提供的完整通用 USB 裝置驅動程式範例，專為 Intel 82930 USB 測試板設計。這個範例展示了幾乎所有 USB 傳輸類型和 KMDF USB 程式設計模式。

**原始路徑：** `usb/usbsamp/`

## 📁 專案結構

```
usbsamp/
├── README.md
├── usbsamp.sln
├── exe/                    # 使用者模式測試應用程式
│   ├── testapp.c
│   └── usbsamp.vcxproj
└── sys/                    # 核心模式驅動程式
    ├── driver/
    │   ├── usbsamp.inx     # INF 安裝檔案
    │   └── usbsamp.vcxproj
    ├── bulkrwr.c          # Bulk Read/Write
    ├── device.c            # 裝置初始化
    ├── driver.c           # DriverEntry
    ├── isorwr.c           # Isochronous Read/Write
    ├── private.h          # 內部標頭檔
    ├── public.h           # 公共標頭檔 (IOCTL 定義)
    ├── queue.c            # I/O 佇列管理
    └── stream.c           # Stream 處理
```

## 🔑 核心元件分析

### 1. 裝置上下文結構

**位置：** `sys/private.h`

```c
typedef struct _DEVICE_CONTEXT {
    // USB 描述符
    USB_DEVICE_DESCRIPTOR           UsbDeviceDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR   UsbConfigurationDescriptor;

    // USB 裝置控制代碼
    WDFUSBDEVICE                    WdfUsbTargetDevice;

    // 電源管理
    ULONG                           WaitWakeEnable;
    
    // 速度檢測
    BOOLEAN                         IsDeviceHighSpeed;
    BOOLEAN                         IsDeviceSuperSpeed;

    // 介面資訊
    WDFUSBINTERFACE                 UsbInterface;
    UCHAR                           SelectedAlternateSetting;
    UCHAR                           NumberConfiguredPipes;

    // 傳輸限制
    ULONG                           MaximumTransferSize;

    // Isochronous 佇列
    WDFQUEUE                        IsochReadQueue;
    WDFQUEUE                        IsochWriteQueue;

    // Stream 支援 (USB 3.0)
    BOOLEAN                         IsStaticStreamsSupported;
    USHORT                          NumberOfStreamsSupportedByController;

    // USBD Handle
    USBD_HANDLE                     UsbdHandle;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)
```

### 2. Pipe 上下文結構

```c
typedef struct _PIPE_CONTEXT {
    // Isochronous 傳輸用
    ULONG NextFrameNumber;
    ULONG TransferSizePerMicroframe;
    ULONG TransferSizePerFrame;
    BOOLEAN  StreamConfigured;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    USBSAMP_STREAM_INFO    StreamInfo;
#endif

} PIPE_CONTEXT, *PPIPE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PIPE_CONTEXT, GetPipeContext)
```

### 3. 請求上下文結構

```c
typedef struct _REQUEST_CONTEXT {
    WDFMEMORY         UrbMemory;
    PMDL              Mdl;
    ULONG             Length;         // 剩餘傳輸量
    ULONG             Numxfer;        // 已傳輸數量
    ULONG_PTR         VirtualAddress; // 下一段的虛擬位址
    BOOLEAN           Read;           // TRUE if Read
} REQUEST_CONTEXT, *PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, GetRequestContext)
```

## 📦 EvtDeviceAdd 分析

**位置：** `sys/device.c`

```c
NTSTATUS
UsbSamp_EvtDeviceAdd(
    WDFDRIVER        Driver,
    PWDFDEVICE_INIT  DeviceInit
    )
{
    WDF_FILEOBJECT_CONFIG     fileConfig;
    WDF_PNPPOWER_EVENT_CALLBACKS  pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES    attributes;
    NTSTATUS                  status;
    WDFDEVICE                 device;
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
    WDF_IO_QUEUE_CONFIG       ioQueueConfig;
    PDEVICE_CONTEXT           pDevContext;
    WDFQUEUE                 queue;
    ULONG                    maximumTransferSize;

    PAGED_CODE();

    // 設定 PnP/Power 回調
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = UsbSamp_EvtDevicePrepareHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    // 設定請求屬性
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, REQUEST_CONTEXT);
    WdfDeviceInitSetRequestAttributes(DeviceInit, &attributes);

    // 設定檔案物件設定
    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
        UsbSamp_EvtDeviceFileCreate,
        WDF_NO_EVENT_CALLBACK,
        WDF_NO_EVENT_CALLBACK);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FILE_CONTEXT);
    WdfDeviceInitSetFileObjectConfig(DeviceInit, &fileConfig, &attributes);

    // 設定 I/O 類型
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

    // 建立 WDFDEVICE
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
    attributes.EvtCleanupCallback = UsbSamp_EvtDeviceContextCleanup;
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

    // 讀取登錄檔設定
    maximumTransferSize = 0;
    ReadFdoRegistryKeyValue(Driver, L"MaximumTransferSize", &maximumTransferSize);
    pDevContext->MaximumTransferSize = 
        maximumTransferSize ? maximumTransferSize : DEFAULT_REGISTRY_TRANSFER_SIZE;

    // 設定 PnP 能力
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.SurpriseRemovalOK = WdfTrue;
    WdfDeviceSetPnpCapabilities(device, &pnpCaps);

    // 建立預設佇列（平行分派）
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
    ioQueueConfig.EvtIoRead = UsbSamp_EvtIoRead;
    ioQueueConfig.EvtIoWrite = UsbSamp_EvtIoWrite;
    ioQueueConfig.EvtIoDeviceControl = UsbSamp_EvtIoDeviceControl;
    ioQueueConfig.EvtIoStop = UsbSamp_EvtIoStop;
    WdfIoQueueCreate(device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);

    // 建立 Isochronous 佇列（手動分派）
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);
    ioQueueConfig.EvtIoStop = UsbSamp_EvtIoStop;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.SynchronizationScope = WdfSynchronizationScopeQueue;
    WdfIoQueueCreate(device, &ioQueueConfig, &attributes, 
                     &pDevContext->IsochReadQueue);
    WdfIoQueueReadyNotify(pDevContext->IsochReadQueue, 
                           UsbSamp_EvtIoQueueReadyNotification, pDevContext);

    // 建立裝置介面
    status = WdfDeviceCreateDeviceInterface(device,
        (LPGUID) &GUID_CLASS_USBSAMP_USB, NULL);

    // 建立 USBD 控制代碼
    status = USBD_CreateHandle(
        WdfDeviceWdmGetDeviceObject(device),
        WdfDeviceWdmGetAttachedDevice(device),
        USBD_CLIENT_CONTRACT_VERSION_602,
        POOL_TAG,
        &pDevContext->UsbdHandle);

    return status;
}
```

## 🔌 EvtDevicePrepareHardware 分析

**位置：** `sys/device.c`

```c
NTSTATUS
UsbSamp_EvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourceList,
    _In_ WDFCMRESLIST ResourceListTranslated
    )
{
    NTSTATUS status;
    PDEVICE_CONTEXT pDeviceContext;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    pDeviceContext = GetDeviceContext(Device);

    // 讀取和選擇 USB 描述符
    status = ReadAndSelectDescriptors(Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 設定電源管理
    if (pDeviceContext->WaitWakeEnable) {
        status = SetPowerPolicy(Device);
    }

    return status;
}
```

## 📋 USB 描述符處理

### ReadAndSelectDescriptors

```c
NTSTATUS
ReadAndSelectDescriptors(_In_ WDFDEVICE Device)
{
    PDEVICE_CONTEXT pDeviceContext = GetDeviceContext(Device);

    // 建立 USB 裝置控制代碼
    if (pDeviceContext->WdfUsbTargetDevice == NULL) {
        WDF_USB_DEVICE_CREATE_CONFIG config;
        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config, USBD_CLIENT_CONTRACT_VERSION_602);

        status = WdfUsbTargetDeviceCreateWithParameters(Device,
            &config, WDF_NO_OBJECT_ATTRIBUTES, &pDeviceContext->WdfUsbTargetDevice);
    }

    // 獲取裝置描述符
    WdfUsbTargetDeviceGetDeviceDescriptor(pDeviceContext->WdfUsbTargetDevice,
        &pDeviceContext->UsbDeviceDescriptor);

    // 讀取和設定設定
    status = ConfigureDevice(Device);

    return status;
}
```

### ConfigureDevice

```c
NTSTATUS
ConfigureDevice(_In_ WDFDEVICE Device)
{
    USHORT size = 0;
    NTSTATUS status;
    PDEVICE_CONTEXT pDeviceContext;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY memory;

    pDeviceContext = GetDeviceContext(Device);

    // 步驟 1：查詢描述符大小
    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(pDeviceContext->WdfUsbTargetDevice,
        NULL, &size);
    if (status != STATUS_BUFFER_TOO_SMALL || size == 0) {
        return status;
    }

    // 步驟 2：分配記憶體
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = pDeviceContext->WdfUsbTargetDevice;
    status = WdfMemoryCreate(&attributes, NonPagedPoolNx, POOL_TAG,
        size, &memory, &configurationDescriptor);

    // 步驟 3：獲取描述符
    status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
        pDeviceContext->WdfUsbTargetDevice, configurationDescriptor, &size);

    // 步驟 4：驗證描述符
    status = UsbSamp_ValidateConfigurationDescriptor(
        configurationDescriptor, size, &Offset);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pDeviceContext->UsbConfigurationDescriptor = configurationDescriptor;

    // 步驟 5：選擇介面
    status = SelectInterfaces(Device);

    return status;
}
```

### SelectInterfaces

```c
NTSTATUS
SelectInterfaces(_In_ WDFDEVICE Device)
{
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    NTSTATUS status;
    PDEVICE_CONTEXT pDeviceContext;
    WDF_OBJECT_ATTRIBUTES pipeAttributes;

    pDeviceContext = GetDeviceContext(Device);

    // 初始化單一介面設定
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);

    // 選擇設定
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&pipeAttributes, PIPE_CONTEXT);
    status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->WdfUsbTargetDevice,
        &pipeAttributes, &configParams);

    if (NT_SUCCESS(status)) {
        // 獲取設定的介面
        pDeviceContext->UsbInterface = 
            configParams.Types.SingleInterface.ConfiguredUsbInterface;

        // 遍歷所有備用設定，找到有端點的設定
        numberAlternateSettings = WdfUsbInterfaceGetNumSettings(pDeviceContext->UsbInterface);
        for (i = 0; i < numberAlternateSettings && numberConfiguredPipes == 0; i++) {
            WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&selectSettingParams, i);
            status = WdfUsbInterfaceSelectSetting(pDeviceContext->UsbInterface,
                &pipeAttributes, &selectSettingParams);
            if (NT_SUCCESS(status)) {
                numberConfiguredPipes = 
                    WdfUsbInterfaceGetNumConfiguredPipes(pDeviceContext->UsbInterface);
            }
        }

        // 初始化每個 Pipe 的上下文
        for (i = 0; i < numberConfiguredPipes; i++) {
            WDFUSBPIPE pipe = WdfUsbInterfaceGetConfiguredPipe(
                pDeviceContext->UsbInterface, i, NULL);
            // 根據速度初始化 Pipe
            if (pDeviceContext->IsDeviceSuperSpeed) {
                status = InitializePipeContextForSuperSpeedDevice(pDeviceContext, pipe);
            } else if (pDeviceContext->IsDeviceHighSpeed) {
                status = InitializePipeContextForHighSpeedDevice(pipe);
            } else {
                status = InitializePipeContextForFullSpeedDevice(pipe);
            }
        }
    }

    return status;
}
```

## 📡 Bulk 傳輸處理

**位置：** `sys/bulkrwr.c`

### 同步讀取

```c
NTSTATUS
ReadBulkEndPoints(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
    )
{
    WDFUSBPIPE pipe;
    NTSTATUS status;
    WDFMEMORY reqMemory;
    PDEVICE_CONTEXT pDeviceContext;

    pDeviceContext = GetDeviceContext(Device);
    pipe = GetBulkReadPipe(pDeviceContext); // 根據速度選擇 Pipe

    // 獲取請求記憶體
    status = WdfRequestRetrieveOutputMemory(Request, &reqMemory);

    // 格式化讀取請求（建立 URB）
    status = WdfUsbTargetPipeFormatRequestForRead(pipe, Request, reqMemory, NULL);

    // 設定完成例程
    WdfRequestSetCompletionRoutine(Request, ReadComplete, pipe);

    // 發送請求
    if (WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), 
                       WDF_NO_SEND_OPTIONS) == FALSE) {
        status = WdfRequestGetStatus(Request);
        WdfRequestCompleteWithInformation(Request, status, 0);
    }

    return STATUS_PENDING;
}
```

### 完成例程

```c
VOID
ReadComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    _In_ WDFCONTEXT Context
    )
{
    NTSTATUS status = CompletionParams->IoStatus.Status;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbParams;
    size_t bytesRead = 0;

    usbParams = CompletionParams->Parameters.Usb.Completion;
    bytesRead = usbParams->Parameters.PipeRead.Length;

    // 完成請求
    WdfRequestCompleteWithInformation(Request, status, bytesRead);
}
```

## ⚡ Interrupt 傳輸

usbsamp 支援 Interrupt 傳輸，使用連續讀取器模式：

```c
NTSTATUS
ConfigureInterruptContinuousReader(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    WDF_USB_CONTINUOUS_READER_CONFIG contReaderConfig;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&contReaderConfig,
        EvtUsbInterruptPipeReadComplete,  // 完成回調
        DeviceContext,                     // 上下文
        sizeof(UCHAR));                    // 傳輸長度

    // 設定失敗回調
    contReaderConfig.EvtUsbTargetPipeReadersFailed = 
        EvtUsbInterruptReadersFailed;

    // 設定讀取器
    return WdfUsbTargetPipeConfigContinuousReader(
        DeviceContext->InterruptPipe,
        &contReaderConfig);
}
```

## 🎵 Isochronous 傳輸

**位置：** `sys/isorwr.c`

Isochronous 傳輸是最複雜的，需要處理框架號碼：

```c
VOID
PerformIsochTransfer(
    _In_ PDEVICE_CONTEXT DeviceContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG TotalLength
    )
{
    // 分配 URB
    PURB urb = ExAllocatePool(NonPagedPool, 
        GET_ISO_URB_SIZE(NumberOfPackets));

    // 填寫 URB
    urb->UrbIsochronousTransfer.Hdr.Function = 
        URB_FUNCTION_ISOCH_TRANSFER;
    urb->UrbIsochronousTransfer.PipeHandle = 
        PipeContext->PipeHandle;
    urb->UrbIsochronousTransfer.TransferFlags = 
        USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
    urb->UrbIsochronousTransfer.StartFrame = 
        PipeContext->NextFrameNumber;
    urb->UrbIsochronousTransfer.NumberOfPackets = NumberOfPackets;

    // 設定每個封包的偏移和長度
    for (i = 0; i < NumberOfPackets; i++) {
        urb->UrbIsochronousTransfer.IsoPacket[i].Offset = i * PacketSize;
        urb->UrbIsochronousTransfer.IsoPacket[i].Length = PacketSize;
    }

    // 發送 URB
    status = WdfUsbTargetDeviceSendUrbSynchronously(
        DeviceContext->WdfUsbTargetDevice,
        NULL, Request, NULL, urb);
}
```

## 🎮 IOCTL 處理

**位置：** `sys/queue.c`

```c
VOID
UsbSamp_EvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _