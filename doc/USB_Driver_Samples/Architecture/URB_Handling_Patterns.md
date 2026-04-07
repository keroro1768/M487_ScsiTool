# URB 處理模式

## 📋 概述

本文件說明 USB Request Block (URB) 的生命週期和 KMDF 中的處理模式。

## 📚 URB 基礎

### URB 結構

```c
// URB 標頭（所有 URB 的開頭）
typedef struct _URB {
    struct _URB_HDR {
        USHORT Length;            // URB 總長度
        USHORT Function;          // URB 功能碼
        USBD_STATUS Status;       // 狀態
        PVOID UsbdDeviceHandle;   // 裝置控制代碼
        PVOID UsbdPipeHandle;     // Pipe 控制代碼
    } Hdr;
    
    // 剩餘欄位根據 Function 不同而異
    union {
        // ... 根據 URB 類型有不同的結構
    };
} URB, *PURB;
```

### URB 功能碼

| 功能碼 | 用途 |
|--------|------|
| `URB_FUNCTION_SELECT_CONFIGURATION` | 選擇設定 |
| `URB_FUNCTION_SELECT_INTERFACE` | 選擇介面 |
| `URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE` | 獲取描述符 |
| `URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT` | 獲取端點描述符 |
| `URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE` | 獲取介面描述符 |
| `URB_FUNCTION_CONTROL_TRANSFER` | 控制傳輸 |
| `URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER` | Bulk/Interrupt 傳輸 |
| `URB_FUNCTION_ISOCH_TRANSFER` | Isochronous 傳輸 |
| `URB_FUNCTION_RESET_PIPE` | 重置 Pipe |
| `URB_FUNCTION_ABORT_PIPE` | 中止 Pipe |

## 🔄 KMDF 中的 URB 處理

### 方式 1：WDF USB 物件（推薦）

KMDF 提供高層 API，自動處理 URB：

```c
// 建立 USB 裝置控制代碼
WDF_USB_DEVICE_CREATE_CONFIG config;
WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config, USBD_CLIENT_CONTRACT_VERSION_602);
WdfUsbTargetDeviceCreateWithParameters(Device, &config, 
    WDF_NO_OBJECT_ATTRIBUTES, &pDeviceContext->UsbDevice);

// 格式化讀取請求（自動建立 URB）
WdfUsbTargetPipeFormatRequestForRead(pipe, Request, memory, NULL);

// 格式化寫入請求
WdfUsbTargetPipeFormatRequestForWrite(pipe, Request, memory, NULL);

// 發送控制傳輸
WDF_USB_CONTROL_SETUP_PACKET packet;
WdfUsbTargetDeviceSendControlTransferSynchronously(Device, NULL, 
    NULL, &packet, &memDesc, &bytesReturned);
```

### 方式 2：直接 URB（僅在必要時）

```c
// 分配 URB
PURB urb = ExAllocatePool(NonPagedPool, sizeof(URB));

// 填寫 URB
RtlZeroMemory(urb, sizeof(URB));
urb->Hdr.Length = sizeof(URB);
urb->Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
urb->Hdr.PipeHandle = PipeContext->PipeHandle;
urb->Hdr.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;

// 發送 URB
WdfUsbTargetDeviceSendUrbSynchronously(Device, NULL, NULL, urb);

// 檢查狀態
if (!USBD_SUCCESS(urb->Hdr.Status)) {
    // 處理錯誤
}

ExFreePool(urb);
```

## 📡 控制傳輸

### KMDF 控制傳輸

```c
NTSTATUS
SendControlTransfer(
    WDFUSBDEVICE UsbDevice,
    UCHAR Request,
    UCHAR RequestType,
    USHORT Value,
    USHORT Index,
    PVOID Data,
    ULONG Length,
    PULONG BytesTransferred
    )
{
    WDF_USB_CONTROL_SETUP_PACKET setupPacket;
    WDF_REQUEST_SEND_OPTIONS sendOptions;
    WDF_MEMORY_DESCRIPTOR memDesc;
    NTSTATUS status;

    // 初始化設定封包 (Host to Device, Vendor)
    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(
        &setupPacket,
        BmRequestHostToDevice,     // Direction
        BmRequestToDevice,        // Recipient
        Request,                   // Request
        Value,                     // Value
        Index                      // Index
    );

    // 設定傳送選項（可選超時）
    WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions, WDF_REQUEST_SEND_OPTION_TIMEOUT);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions, 
        5 * -1 * WDF_TIMEOUT_TO_SEC);  // 5 秒

    // 設定記憶體描述符
    if (Data && Length > 0) {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc, Data, Length);
    }

    // 同步傳送
    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
        UsbDevice,
        WDF_NO_HANDLE,           // Optional request
        &sendOptions,
        &setupPacket,
        Data ? &memDesc : NULL,
        BytesTransferred
    );

    return status;
}
```

### 常見控制傳輸模式

| 模式 | RequestType | 用途 |
|------|-------------|------|
| Host to Device, Standard | 0x00 | 標準請求（設定地址等）|
| Device to Host, Standard | 0x80 | 標準請求（獲取描述符等）|
| Host to Device, Class | 0x20 | 類別特定請求 |
| Device to Host, Class | 0xA0 | 類別特定請求 |
| Host to Device, Vendor | 0x40 | 廠商特定請求 |
| Device to Host, Vendor | 0xC0 | 廠商特定請求 |

### 控制傳輸結構

```
┌─────────────────────────────────────────┐
│            Setup Packet (8 bytes)        │
├─────┬─────┬──────┼───────┬─────────────┤
│ bm  │ bRequest │ wValue  │ wIndex │ wLength │
│ Request │ Type   │        │        │         │
│ (1)  │ (1)    │ (2)    │ (2)   │ (2)     │
├─────┴─────┴──────┼───────┴─────────────┤
│                   │                     │
│ 0=Host→Device    │ Data payload        │
│ 1=Device→Host    │ (wLength bytes)     │
│ D7: Direction    │                     │
│ D6-5: Type       │                     │
│   00=Standard    │                     │
│   01=Class       │                     │
│   10=Vendor      │                     │
│   11=Reserved    │                     │
│ D4-0: Recipient  │                     │
│   00000=Device   │                     │
│   00001=Interface│                     │
│   00010=Endpoint │                     │
│   00011=Other    │                     │
└─────────────────┴─────────────────────┘
```

## 🔄 Bulk/Interrupt 傳輸

### Bulk 傳輸

```c
NTSTATUS
BulkRead(
    WDFUSBPIPE Pipe,
    WDFREQUEST Request,
    PVOID Buffer,
    SIZE_T Length,
    PULONG BytesRead
    )
{
    NTSTATUS status;
    WDFMEMORY memory;

    // 從請求獲取記憶體
    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 格式化 Bulk 讀取請求
    status = WdfUsbTargetPipeFormatRequestForRead(
        Pipe,
        Request,
        memory,
        NULL  // Offset
    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 設定完成例程（非同步處理）
    WdfRequestSetCompletionRoutine(Request, 
        BulkReadComplete, NULL);

    // 發送
    if (WdfRequestSend(Request, 
        WdfUsbTargetPipeGetIoTarget(Pipe),
        WDF_NO_SEND_OPTIONS) == FALSE) {
        return WdfRequestGetStatus(Request);
    }

    return STATUS_PENDING;
}

VOID
BulkReadComplete(
    WDFREQUEST Request,
    WDFIOTARGET Target,
    PWDF_REQUEST_COMPLETION_PARAMS Params,
    WDFCONTEXT Context
    )
{
    NTSTATUS status = Params->IoStatus.Status;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbParams;
    size_t bytesRead = 0;

    usbParams = Params->Parameters.Usb.Completion;
    if (NT_SUCCESS(status)) {
        bytesRead = usbParams->Parameters.PipeRead.Length;
    }

    // 完成請求
    WdfRequestCompleteWithInformation(Request, status, bytesRead);
}
```

### Interrupt 傳輸

Interrupt 傳輸通常使用連續讀取器模式：

```c
NTSTATUS
ConfigureInterruptReader(
    WDFUSBPIPE Pipe,
    PDEVICE_CONTEXT Context
    )
{
    WDF_USB_CONTINUOUS_READER_CONFIG config;
    NTSTATUS status;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(
        &config,
        InterruptReadComplete,     // Completion routine
        Context,                   // Context
        sizeof(UCHAR)              // Transfer length
    );

    // 設定失敗回調（可選）
    config.EvtUsbTargetPipeReadersFailed = InterruptReaderFailed;

    // 設定讀取器
    status = WdfUsbTargetPipeConfigContinuousReader(Pipe, &config);

    return status;
}

VOID
InterruptReadComplete(
    WDFUSBPIPE Pipe,
    WDFMEMORY Buffer,
    size_t NumBytes,
    WDFCONTEXT Context
    )
{
    PUCHAR data;

    if (NumBytes == 0) {
        return;
    }

    data = WdfMemoryGetBuffer(Buffer, NULL);
    
    // 處理收到的資料
    ProcessInterruptData(data, NumBytes);
}
```

## 🎵 Isochronous 傳輸

Isochronous 是最複雜的傳輸類型：

```c
NTSTATUS
IsochTransfer(
    WDFUSBDEVICE UsbDevice,
    WDFUSBPIPE Pipe,
    WDFREQUEST Request,
    USHORT StartFrame,
    ULONG NumberOfPackets,
    SIZE_T PacketSize,
    PVOID Buffer
    )
{
    PURB urb;
    ULONG urbSize;
    NTSTATUS status;

    // 分配 URB
    urbSize = GET_ISO_URB_SIZE(NumberOfPackets);
    urb = ExAllocatePool(NonPagedPool, urbSize);

    RtlZeroMemory(urb, urbSize);

    // 填寫 Isochronous Transfer URB
    urb->UrbIsochronousTransfer.Hdr.Length = (USHORT)urbSize;
    urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    urb->UrbIsochronousTransfer.PipeHandle = 
        WdfUsbTargetPipeGetPipeHandle(Pipe);
    urb->UrbIsochronousTransfer.TransferFlags = 
        USBD_TRANSFER_DIRECTION_IN | 
        USBD_SHORT_TRANSFER_OK | 
        USBD_ISO_START_FRAME_INITIALIZATION;
    urb->UrbIsochronousTransfer.TransferBufferLength = 
        NumberOfPackets * PacketSize;
    urb->UrbIsochronousTransfer.TransferBuffer = Buffer;
    urb->UrbIsochronousTransfer.TransferBufferMDL = NULL;
    urb->UrbIsochronousTransfer.StartFrame = StartFrame;
    urb->UrbIsochronousTransfer.NumberOfPackets = NumberOfPackets;
    urb->UrbIsochronousTransfer.UrbLink = NULL;

    // 設定每個封包的偏移
    for (i = 0; i < NumberOfPackets; i++) {
        urb->UrbIsochronousTransfer.IsoPacket[i].Offset = 
            i * PacketSize;
        urb->UrbIsochronousTransfer.IsoPacket[i].Length = 
            PacketSize;
    }

    // 發送
    status = WdfUsbTargetDeviceSendUrbSynchronously(
        UsbDevice, NULL, Request, urb);

    // 檢查結果
    if (NT_SUCCESS(status)) {
        for (i = 0; i < NumberOfPackets; i++) {
            if (urb->UrbIsochronousTransfer.IsoPacket[i].Status != 0) {
                // 封包傳輸失敗
            }
        }
    }

    ExFreePool(urb);
    return status;
}
```

## 🔧 Pipe 操作

### Pipe 重置

```c
NTSTATUS
ResetPipe(
    WDFUSBPIPE Pipe
    )
{
    NTSTATUS status;

    // KMDF 包裝
    status = WdfUsbTargetPipeResetSynchronously(
        Pipe,
        WDF_NO_HANDLE,
        NULL
    );

    return status;
}
```

### Pipe 中止

```c
NTSTATUS
AbortPipe(
    WDFUSBPIPE Pipe
    )
{
    NTSTATUS status;

    status = WdfUsbTargetPipeAbortSynchronously(
        Pipe,
        WDF_NO_HANDLE,
        NULL
    );

    return status;
}
```

### 選擇介面

```c
NTSTATUS
SelectAlternateSetting(
    WDFUSBINTERFACE Interface,
    UCHAR AlternateSetting
    )
{
    WDF_USB_INTERFACE_SELECT_SETTING_PARAMS params;
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;

    WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&params, 
        AlternateSetting);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, PIPE_CONTEXT);

    status = WdfUsbInterfaceSelectSetting(
        Interface,
        &attributes,
        &params
    );

    return status;
}
```

## ⚠️ 常見錯誤處理

| 錯誤碼 | 原因 | 處理方式 |
|--------|------|----------|
| USBD_STATUS_TIMEOUT | 傳輸超時 | 重試或報告錯誤 |
| USBD_STATUS_CANCELED | 請求被取消 | 清理狀態 |
| USBD_STATUS_DEVICE_GONE | 裝置已移除 | 停止操作 |
| USBD_STATUS_ENDPOINT_HALTED | 端點停止 | 重置 Pipe |
| USBD_STATUS_STALL_PID | 端點停滯 | 清除 Stall |

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
