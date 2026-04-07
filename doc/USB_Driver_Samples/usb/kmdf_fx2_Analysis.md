# kmdf_fx2 KMDF USB 裝置驅動程式分析

## 📋 概述

kmdf_fx2 是專為 OSR USB-FX2 Learning Kit 設計的 KMDF USB 驅動程式範例。它比 usbsamp 簡潔，同時展示了完整的 USB 開發模式。

**原始路徑：** `usb/kmdf_fx2/`

## 📁 專案結構

```
kmdf_fx2/
├── README.md
├── kmdf_fx2.sln
├── deviceMetadata/
│   └── {GUID}.deviceMetadata
├── driver/
│   ├── driver.c        # DriverEntry
│   ├── Device.c        # 裝置初始化
│   ├── bulkrwr.c       # Bulk Read/Write
│   ├── interrupt.c     # Interrupt 傳輸
│   ├── ioctl.c         # IOCTL 處理
│   ├── osrusbfx2.h    # 主要標頭檔
│   ├── osrusbfx2.inx  # INF 安裝檔
│   ├── osrusbfx2.man  # Event manifest
│   ├── osrusbfx2.rc   # 資源檔
│   └── trace.h        # WPP 追蹤
├── exe/
│   ├── testapp.c      # 測試應用程式
│   └── test.cmd       # 測試腳本
└── inc/
    ├── prototypes.h    # 函式原型
    └── public.h       # 公共定義
```

## 🔑 裝置上下文

**位置：** `driver/osrusbfx2.h`

```c
typedef struct _DEVICE_CONTEXT {
    // USB 物件
    WDFUSBDEVICE    UsbDevice;
    WDFUSBINTERFACE UsbInterface;

    // Pipe 控制代碼
    WDFUSBPIPE      BulkReadPipe;
    WDFUSBPIPE      BulkWritePipe;
    WDFUSBPIPE      InterruptPipe;

    // 同步鎖
    WDFWAITLOCK     ResetDeviceWaitLock;

    // Interrupt 狀態
    UCHAR           CurrentSwitchState;

    // Interrupt 訊息佇列
    WDFQUEUE        InterruptMsgQueue;

    // USB 特性
    ULONG           UsbDeviceTraits;

    // 事件記錄用
    WDFMEMORY       DeviceNameMemory;
    PCWSTR          DeviceName;
    WDFMEMORY       LocationMemory;
    PCWSTR          Location;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)
```

## 📦 DriverEntry 分析

**位置：** `driver/driver.c`

```c
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG       config;
    NTSTATUS                status;
    WDF_OBJECT_ATTRIBUTES   attributes;

    // 初始化 WPP 追蹤
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    // 初始化驅動程式設定
    WDF_DRIVER_CONFIG_INIT(&config, OsrFxEvtDeviceAdd);

    // 設定驅動程式卸載回調
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = OsrFxEvtDriverContextCleanup;

    // 建立 WDFDRIVER
    status = WdfDriverCreate(DriverObject, RegistryPath, &attributes,
                              &config, WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        WPP_CLEANUP(DriverObject);
    }

    return status;
}
```

## 🔌 EvtDeviceAdd 分析

**位置：** `driver/Device.c`

```c
NTSTATUS
OsrFxEvtDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )
{
    // ... 初始化程式碼 ...

    // 設定 PnP/Power 回調
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = OsrFxEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceD0Entry = OsrFxEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = OsrFxEvtDeviceD0Exit;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoFlush = OsrFxEvtDeviceSelfManagedIoFlush;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    // 設定 I/O 類型
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    // 建立 WDFDEVICE
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

    // 建立預設佇列（平行分派）
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
    ioQueueConfig.EvtIoDeviceControl = OsrFxEvtIoDeviceControl;
    WdfIoQueueCreate(device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);

    // 建立讀取佇列（順序分派）
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchSequential);
    ioQueueConfig.EvtIoRead = OsrFxEvtIoRead;
    ioQueueConfig.EvtIoStop = OsrFxEvtIoStop;
    WdfDeviceConfigureRequestDispatching(device, queue, WdfRequestTypeRead);

    // 建立寫入佇列（順序分派）
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchSequential);
    ioQueueConfig.EvtIoWrite = OsrFxEvtIoWrite;
    ioQueueConfig.EvtIoStop = OsrFxEvtIoStop;
    WdfDeviceConfigureRequestDispatching(device, queue, WdfRequestTypeWrite);

    // 建立 Interrupt 訊息佇列（手動分派）
    WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);
    ioQueueConfig.PowerManaged = WdfFalse;  // 非電源管理
    WdfIoQueueCreate(device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES,
                     &pDevContext->InterruptMsgQueue);

    // 建立裝置介面
    status = WdfDeviceCreateDeviceInterface(device,
        (LPGUID) &GUID_DEVINTERFACE_OSRUSBFX2, NULL);

    // 建立重置鎖
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;
    WdfWaitLockCreate(&attributes, &pDevContext->ResetDeviceWaitLock);

    return status;
}
```

## ⚡ EvtDevicePrepareHardware 分析

```c
NTSTATUS
OsrFxEvtDevicePrepareHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourceList,
    WDFCMRESLIST ResourceListTranslated
    )
{
    PDEVICE_CONTEXT pDeviceContext;
    WDF_USB_DEVICE_INFORMATION deviceInfo;
    ULONG waitWakeEnable;

    pDeviceContext = GetDeviceContext(Device);

    // 建立 USB 裝置控制代碼
    if (pDeviceContext->UsbDevice == NULL) {
        WDF_USB_DEVICE_CREATE_CONFIG config;
        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config, USBD_CLIENT_CONTRACT_VERSION_602);

        status = WdfUsbTargetDeviceCreateWithParameters(Device, &config,
            WDF_NO_OBJECT_ATTRIBUTES, &pDeviceContext->UsbDevice);
    }

    // 獲取裝置資訊
    WDF_USB_DEVICE_INFORMATION_INIT(&deviceInfo);
    WdfUsbTargetDeviceRetrieveInformation(pDeviceContext->UsbDevice, &deviceInfo);

    pDeviceContext->UsbDeviceTraits = deviceInfo.Traits;
    waitWakeEnable = deviceInfo.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE;

    // 選擇介面
    status = SelectInterfaces(Device);

    // 設定電源管理
    if (waitWakeEnable) {
        status = OsrFxSetPowerPolicy(Device);
    }

    // 設定 Interrupt 連續讀取器
    status = OsrFxConfigContReaderForInterruptEndPoint(pDeviceContext);

    return status;
}
```

## 📋 SelectInterfaces 分析

```c
NTSTATUS
SelectInterfaces(_In_ WDFDEVICE Device)
{
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_CONTEXT pDeviceContext;
    WDFUSBPIPE pipe;
    WDF_USB_PIPE_INFORMATION pipeInfo;
    UCHAR index, numberConfiguredPipes;

    pDeviceContext = GetDeviceContext(Device);

    // 選擇單一介面設定
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);

    status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice,
        WDF_NO_OBJECT_ATTRIBUTES, &configParams);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    pDeviceContext->UsbInterface = 
        configParams.Types.SingleInterface.ConfiguredUsbInterface;
    numberConfiguredPipes = 
        configParams.Types.SingleInterface.NumberConfiguredPipes;

    // 遍歷所有 Pipe
    for (index = 0; index < numberConfiguredPipes; index++) {
        WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
        pipe = WdfUsbInterfaceGetConfiguredPipe(pDeviceContext->UsbInterface,
            index, &pipeInfo);

        // 允許讀取小於 MaximumPacketSize 的資料
        WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

        // 根據 Pipe 類型分類儲存
        if (WdfUsbPipeTypeInterrupt == pipeInfo.PipeType) {
            pDeviceContext->InterruptPipe = pipe;
        }
        if (WdfUsbPipeTypeBulk == pipeInfo.PipeType && 
            WdfUsbTargetPipeIsInEndpoint(pipe)) {
            pDeviceContext->BulkReadPipe = pipe;
        }
        if (WdfUsbPipeTypeBulk == pipeInfo.PipeType && 
            WdfUsbTargetPipeIsOutEndpoint(pipe)) {
            pDeviceContext->BulkWritePipe = pipe;
        }
    }

    // 驗證所有必要的 Pipe 都存在
    if (!(pDeviceContext->BulkWritePipe && pDeviceContext->BulkReadPipe && 
          pDeviceContext->InterruptPipe)) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    return status;
}
```

## 🔄 Bulk 傳輸處理

**位置：** `driver/bulkrwr.c`

### 讀取

```c
VOID
OsrFxEvtIoRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t Length
    )
{
    WDFUSBPIPE pipe;
    NTSTATUS status;
    WDFMEMORY reqMemory;
    PDEVICE_CONTEXT pDeviceContext;

    pDeviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
    pipe = pDeviceContext->BulkReadPipe;

    // 驗證傳輸大小
    if (Length > TEST_BOARD_TRANSFER_BUFFER_SIZE) {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // 獲取請求記憶體
    status = WdfRequestRetrieveOutputMemory(Request, &reqMemory);
    if (!NT_SUCCESS(status)) goto Exit;

    // 格式化讀取請求
    status = WdfUsbTargetPipeFormatRequestForRead(pipe, Request, reqMemory, NULL);
    if (!NT_SUCCESS(status)) goto Exit;

    // 設定完成例程
    WdfRequestSetCompletionRoutine(Request, EvtRequestReadCompletionRoutine, pipe);

    // 發送請求（非同步）
    if (WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), 
                       WDF_NO_SEND_OPTIONS) == FALSE) {
        status = WdfRequestGetStatus(Request);
        goto Exit;
    }

    return;

Exit:
    WdfRequestCompleteWithInformation(Request, status, 0);
}
```

### 完成例程

```c
VOID
EvtRequestReadCompletionRoutine(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    _In_ WDFCONTEXT Context
    )
{
    NTSTATUS status = CompletionParams->IoStatus.Status;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;
    size_t bytesRead;

    usbCompletionParams = CompletionParams->Parameters.Usb.Completion;
    bytesRead = usbCompletionParams->Parameters.PipeRead.Length;

    // 完成請求
    WdfRequestCompleteWithInformation(Request, status, bytesRead);
}
```

## ⚡ Interrupt 連續讀取器

**位置：** `driver/interrupt.c`

### 設定

```c
NTSTATUS
OsrFxConfigContReaderForInterruptEndPoint(
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    WDF_USB_CONTINUOUS_READER_CONFIG contReaderConfig;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&contReaderConfig,
        OsrFxEvtUsbInterruptPipeReadComplete,  // 完成回調
        DeviceContext,                          // 上下文
        sizeof(UCHAR));                         // 傳輸長度

    // 設定失敗回調
    contReaderConfig.EvtUsbTargetPipeReadersFailed = 
        OsrFxEvtUsbInterruptReadersFailed;

    // 設定連續讀取器
    return WdfUsbTargetPipeConfigContinuousReader(
        DeviceContext->InterruptPipe, &contReaderConfig);
}
```

### 完成回調

```c
VOID
OsrFxEvtUsbInterruptPipeReadComplete(
    WDFUSBPIPE Pipe,
    WDFMEMORY Buffer,
    size_t NumBytesTransferred,
    WDFCONTEXT Context
    )
{
    PUCHAR switchState;
    WDFDEVICE device;
    PDEVICE_CONTEXT pDeviceContext = Context;

    device = WdfObjectContextGetObject(pDeviceContext);

    if (NumBytesTransferred == 0) {
        return;
    }

    switchState = WdfMemoryGetBuffer(Buffer, NULL);
    pDeviceContext->CurrentSwitchState = *switchState;

    // 處理等待中的 Interrupt 訊息請求
    OsrUsbIoctlGetInterruptMessage(device, STATUS_SUCCESS);
}
```

## 🎮 IOCTL 處理

**位置：** `driver/ioctl.c`

```c
VOID
OsrFxEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT pDevContext = GetDeviceContext(device);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    size_t bytesReturned = 0;

    switch (IoControlCode) {
    case IOCTL_OSRUSBFX2_GET_CONFIG_DESCRIPTOR:
        // 獲取 USB 設定描述符
        break;

    case IOCTL_OSRUSBFX2_RESET_DEVICE:
        status = ResetDevice(device);
        break;

    case IOCTL_OSRUSBFX2_REENUMERATE_DEVICE:
        status = ReenumerateDevice(pDevContext);
        break;

    case IOCTL_OSRUSBFX2_GET_BAR_GRAPH_DISPLAY:
        // 讀取 Bar Graph 狀態
        break;

    case IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY:
        // 設定 Bar Graph 狀態
        break;

    case IOCTL_OSRUSBFX2_READ_SWITCHES:
        // 讀取開關狀態
        break;

    case IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE:
        // 等待 Interrupt 並返回訊息
        status = WdfRequestForwardToIoQueue(Request, 
            pDevContext->InterruptMsgQueue);
        if (NT_SUCCESS(status)) {
            return;  // 請求尚未完成
        }
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}
```

### 控制傳輸範例

```c
NTSTATUS
GetSwitchState(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PSWITCH_STATE SwitchState
    )
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS sendOptions;
    WDF_MEMORY_DESCRIPTOR memDesc;
    ULONG bytesTransferred;

    // 初始化傳送選項（帶超時）
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, 
        DEFAULT_CONTROL_TRANSFER_TIMEOUT);

    // 初始化 Vendor 控制封包
    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
        BmRequestDeviceToHost,    // 方向：裝置到主機
        BmRequestToDevice,        // 目標：裝置
        USBFX2LK_READ_SWITCHES,   // 請求碼
        0,                        // Value
        0);                       // Index

    // 設定記憶體描述符
    SwitchState->SwitchesAsUChar = 0;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc, SwitchState, 
        sizeof(SWITCH_STATE));

    // 同步傳送控制傳輸
    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
        DevContext->UsbDevice, NULL, &sendOptions, &controlSetupPacket,
        &memDesc, &bytesTransferred);

    return status;
}
```

## 🔌 電源管理

```c
NTSTATUS
OsrFxSetPowerPolicy(_In_ WDFDEVICE Device)
{
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;
    NTSTATUS status = STATUS_SUCCESS;

    // 設定空閒策略
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleUsbSelectiveSuspend);
    idleSettings.IdleTimeout = 10000;  // 10 秒
    status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);

    // 設定喚醒策略
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);
    status = WdfDeviceAssignSxWakeSettings(Device, &wakeSettings);

    return status;
}
```

## 📝 關鍵學習點

### 1. USB 裝置初始化流程
```c
// 1. 建立 WDFUSBDEVICE
WdfUsbTargetDeviceCreateWithParameters()

// 2. 選擇設定
WdfUsbTargetDeviceSelectConfig()

// 3. 獲取 Pipe 控制代碼
WdfUsbInterfaceGetConfiguredPipe()
```

### 2. Bulk 傳輸模式
```c
// 格式化請求
WdfUsbTargetPipeFormatRequestForRead/Write()

// 設定完成例程
WdfRequestSetCompletionRoutine()

// 發送（非同步）
WdfRequestSend()
```

### 3. Interrupt 連續讀取器
```c
WDF_USB_CONTINUOUS_READER_CONFIG_INIT()
WdfUsbTargetPipeConfigContinuousReader()
```

### 4. 控制傳輸
```c
WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR()
WdfUsbTargetDeviceSendControlTransferSynchronously()
```

## 🔗 與 M487 的關聯

kmdf_fx2 提供了完整的 KMDF USB 程式設計模式，M487 可借鑒：

1. **IOCTL 設計模式** - 定義和處理自訂 IOCTL
2. **電源管理** - Idle/Wake 設定
3. **Interrupt 處理** - 連續讀取器使用
4. **控制傳輸** - Vendor Command 傳送

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
