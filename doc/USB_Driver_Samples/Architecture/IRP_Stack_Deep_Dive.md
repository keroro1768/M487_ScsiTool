# IRP 堆疊深入解析

## 📋 概述

本文件詳細說明 IRP（I/O Request Packet）的結構和 IRP 堆疊在 USB/HID 驅動程式中的使用方式。

## 📚 IRP 基礎

### IRP 結構

```c
typedef struct IRP {
    // 指標欄位
    PMDL                MdlAddress;          // Memory Descriptor List
    PVOID               RequestorMode;       // 請求者模式
    PETHREAD            Thread;              // 執行緒指標
    PFILE_OBJECT        FileObject;          // 檔案物件

    // I/O 狀態區塊
    IO_STATUS_BLOCK      IoStatus;           // 完成狀態
    KPROCESSOR_MODE     RequestorMode;      // 處理器模式

    // 堆疊位置
    UCHAR               MajorFunction;       // 主要功能碼
    UCHAR               MinorFunction;       // 次要功能碼
    UCHAR               Flags;              // IRP 旗標
    IO_STACK_LOCATION   *CurrentLocation;    // 目前堆疊位置
    PVOID               Tail;               // 尾部資料
} IRP, *PIRP;
```

### IO_STACK_LOCATION 結構

```c
typedef struct _IO_STACK_LOCATION {
    UCHAR                  MajorFunction;     // 主要功能碼
    UCHAR                  MinorFunction;    // 次要功能碼
    UCHAR                  Flags;            // 旗標
    UCHAR                  Control;          // 控制旗標

    // 參數聯合（根據 MajorFunction 不同）
    union {
        // IRP_MJ_CREATE
        struct {
            PFILE_OBJECT          FileObject;
            ULONG                   Options;
            USHORT POFileAttributes;
            USHORT ShareAccess;
            ULONG POEA length;
        } Create;

        // IRP_MJ_READ
        struct {
            ULONG                   Length;
            ULONG POAlignment Req;
            LARGE_INTEGER           ByteOffset;
            PVOID                   Key;
        } Read;

        // IRP_MJ_WRITE
        struct {
            ULONG                   Length;
            ULONG POAlignment Req;
            LARGE_INTEGER           ByteOffset;
            PVOID                   Key;
        } Write;

        // IRP_MJ_DEVICE_CONTROL / IRP_MJ_INTERNAL_DEVICE_CONTROL
        struct {
            ULONG                   OutputBufferLength;
            ULONG                   InputBufferLength;
            ULONG                   IoControlCode;
            PVOID                   Type3InputBuffer;
        } DeviceIoControl;

        // ... 其他
    } Parameters;

    // 裝置物件
    PDEVICE_OBJECT        DeviceObject;
    PFILE_OBJECT          FileObject;

    // 完成例程（用於堆疊傳遞）
    PIO_COMPLETION_ROUTINE  CompletionRoutine;
    PVOID                   Context;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;
```

## 🔑 主要功能碼 (MajorFunction)

| 功能碼 | 值 | 說明 |
|--------|-----|------|
| `IRP_MJ_CREATE` | 0x00 | 建立/開啟 |
| `IRP_MJ_CREATE_NAMED_PIPE` | 0x01 | 建立命名管道 |
| `IRP_MJ_CLOSE` | 0x02 | 關閉 |
| `IRP_MJ_READ` | 0x03 | 讀取 |
| `IRP_MJ_WRITE` | 0x04 | 寫入 |
| `IRP_MJ_QUERY_INFORMATION` | 0x05 | 查詢資訊 |
| `IRP_MJ_SET_INFORMATION` | 0x06 | 設定資訊 |
| `IRP_MJ_QUERY_EA` | 0x07 | 查詢擴充屬性 |
| `IRP_MJ_SET_EA` | 0x08 | 設定擴充屬性 |
| `IRP_MJ_FLUSH_BUFFERS` | 0x09 | 刷新緩衝區 |
| `IRP_MJ_QUERY_VOLUME_INFORMATION` | 0x0A | 查詢卷資訊 |
| `IRP_MJ_SET_VOLUME_INFORMATION` | 0x0B | 設定卷資訊 |
| `IRP_MJ_DIRECTORY_CONTROL` | 0x0C | 目錄控制 |
| `IRP_MJ_FILE_SYSTEM_CONTROL` | 0x0D | 檔案系統控制 |
| `IRP_MJ_DEVICE_CONTROL` | 0x0E | 裝置控制 |
| `IRP_MJ_INTERNAL_DEVICE_CONTROL` | 0x0F | 內部裝置控制（🔑 HID 使用）|
| `IRP_MJ_SCSI` | 0x10 | SCSI 命令 |
| `IRP_MJ_SHUTDOWN` | 0x11 | 關機 |
| `IRP_MJ_LOCK_CONTROL` | 0x12 | 鎖定控制 |
| `IRP_MJ_CLEANUP` | 0x12 | 清理 |
| `IRP_MJ_CREATE_MAILSLOT` | 0x13 | 建立郵筒 |
| `IRP_MJ_QUERY_SECURITY` | 0x14 | 查詢安全性 |
| `IRP_MJ_SET_SECURITY` | 0x15 | 設定安全性 |
| `IRP_MJ_POWER` | 0x16 | 電源管理 |
| `IRP_MJ_SYSTEM_CONTROL` | 0x17 | 系統控制（WMI）|
| `IRP_MJ_DEVICE_CHANGE` | 0x18 | 裝置變更 |
| `IRP_MJ_QUERY_QUOTA` | 0x19 | 查詢配額 |
| `IRP_MJ_SET_QUOTA` | 0x1A | 設定配額 |
| `IRP_MJ_PNP` | 0x1B | PnP 管理 |

## 🎯 USB/HID 特定功能碼

### IRP_MJ_INTERNAL_DEVICE_CONTROL

這是 USB/HID 驅動程式中最常用的功能碼！

```c
// HID 類別使用此 IRP 碼
typedef struct _HID_MINIDRIVERTransport {
    PIRP Irp;
    PHID_SUBMIT_IRP_NOTIFICATION_FUNCTION HidIrpCallout;
} HID_MINIDRIVER_TRANSPORT_DATA, *PHID_MINIDRIVER_TRANSPORT_DATA;
```

**HID 使用的 MinorFunction：**

| minor | IOCTL | 用途 |
|-------|-------|------|
| `IRP_MN_HIDCLASS_ACTION` | N/A | HID 類別操作 |
| `IRP_MN_HIDCLASS_DRIVER_CREATED` | N/A | 驅動程式建立通知 |
| `IRP_MN_HIDCLASS_DEVICE_READ` | N/A | 讀取通知 |
| `IRP_MN_HIDCLASS_DEVICE_WRITE` | N/A | 寫入通知 |

### HID IOCTL 列表

| IOCTL | 用途 |
|-------|------|
| `IOCTL_HID_GET_DEVICE_DESCRIPTOR` | 獲取 HID 裝置描述符 |
| `IOCTL_HID_GET_REPORT_DESCRIPTOR` | 獲取 Report 描述符 |
| `IOCTL_HID_READ_REPORT` | 讀取 Report |
| `IOCTL_HID_WRITE_REPORT` | 寫入 Report |
| `IOCTL_HID_GET_COLLECTION_INFORMATION` | 獲取集合資訊 |
| `IOCTL_HID_GET_COLLECTION_DESCRIPTOR` | 獲取集合描述符 |
| `IOCTL_HID_SET_FEATURE` | 設定 Feature 報告 |
| `IOCTL_HID_GET_FEATURE` | 獲取 Feature 報告 |
| `IOCTL_HID_GET_INPUT_REPORT` | 獲取 Input 報告 |
| `IOCTL_HID_SET_OUTPUT_REPORT` | 設定 Output 報告 |

## 🔄 IRP 堆疊操作

### WDM 方式

```c
// 獲取目前堆疊位置
PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

// 設定下一個堆疊位置
IoSkipCurrentIrpStackLocation(Irp);
IoCallDriver(NextDevice, Irp);

// 或手動複製
IoCopyCurrentIrpStackLocationToNext(Irp);
IoSetCompletionRoutine(Irp, CompletionRoutine, Context);
IoCallDriver(NextDevice, Irp);
```

### KMDF 方式

```c
// 格式化請求（自動複製堆疊位置）
WdfRequestFormatRequestUsingCurrentType(Request);

// 設定完成例程
WdfRequestSetCompletionRoutine(Request, CompletionRoutine, Context);

// 發送請求
WdfRequestSend(Request, Target, Options);

// 或同步發送
WdfRequestSendSynchronously(Request, Target, Options, Timeout);
```

## 📝 KMDF Filter 中的請求轉發

### 簡單轉發

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

    // 檢查是否需要處理
    if (ShouldFilterIoctl(IoControlCode)) {
        // 處理 IOCTL
        HandleIoctl(Request, IoControlCode);
        return;
    }

    // 轉發至下層
    WdfRequestForwardToIoQueue(Request, WdfDeviceGetIoTarget(device));
}
```

### 帶完成的轉發

```c
#define FORWARD_WITH_COMPLETION 1

VOID
FilterForwardRequestWithCompletion(
    WDFREQUEST Request,
    WDFIOTARGET Target
    )
{
    NTSTATUS status;

    // 格式化（複製堆疊位置）
    WdfRequestFormatRequestUsingCurrentType(Request);

    // 設定完成例程
    WdfRequestSetCompletionRoutine(Request,
        FilterRequestCompletion, NULL);

    // 發送（非同步）
    if (WdfRequestSend(Request, Target, WDF_NO_SEND_OPTIONS) == FALSE) {
        status = WdfRequestGetStatus(Request);
        WdfRequestComplete(Request, status);
    }
}

NTSTATUS
FilterRequestCompletion(
    WDFREQUEST Request,
    WDFIOTARGET Target,
    PWDF_REQUEST_COMPLETION_PARAMS Params,
    WDFCONTEXT Context
    )
{
    NTSTATUS status = Params->IoStatus.Status;

    // 後處理
    if (NT_SUCCESS(status) && 
        IoControlCode == MY_CUSTOM_IOCTL) {
        // 修改輸出資料
    }

    // 繼續完成（或返回 STATUS_MORE_PROCESSING_REQUIRED）
    return STATUS_SUCCESS;
}
```

## 🔧 IOCTL 處理模式

### 解析 IOCTL

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
    switch (IoControlCode) {
    case IOCTL_HID_GET_COLLECTION_INFORMATION:
        HandleGetCollectionInfo(Request, OutputBufferLength);
        break;

    case IOCTL_HID_SET_FEATURE:
        HandleSetFeature(Request, InputBufferLength);
        break;

    case IOCTL_HID_GET_FEATURE:
        HandleGetFeature(Request, OutputBufferLength);
        break;

    case IOCTL_HID_READ_REPORT:
        HandleReadReport(Request);
        break;

    case IOCTL_HID_WRITE_REPORT:
        HandleWriteReport(Request, InputBufferLength);
        break;

    default:
        // 轉發未知 IOCTL
        FilterForwardRequest(Request, WdfDeviceGetIoTarget(device));
        break;
    }
}
```

### 處理緩衝區

```c
NTSTATUS
HandleGetFeature(
    WDFREQUEST Request,
    size_t OutputBufferLength
    )
{
    PVOID buffer;
    NTSTATUS status;

    // 獲取輸出緩衝區
    status = WdfRequestRetrieveOutputBuffer(Request, 
        OutputBufferLength, &buffer, NULL);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 填充 Feature 資料
    PFEATURE_DATA featureData = (PFEATURE_DATA)buffer;
    featureData->ReportId = 0x01;
    featureData->Value = 0x1234;

    // 完成請求
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 
        sizeof(FEATURE_DATA));

    return STATUS_SUCCESS;
}
```

## 🔗 IRP 生命週期

```
┌─────────────────────────────────────────────────────────┐
│              IRP Created by I/O Manager                  │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│          IRP_MJ_CREATE (Open device)                    │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│          IRP_MJ_DEVICE_CONTROL / INTERNAL_DEVICE_CONTROL │
│                                                         │
│   ┌─────────────────────────────────────────────────┐   │
│   │         For loop through device stack:          │   │
│   │                                                  │   │
│   │   Driver A (Upper Filter)                       │   │
│   │     - IoCallDriver to next                    │   │
│   │         ▼                                        │   │
│   │   Driver B (Function)                          │   │
│   │     - Process request                          │   │
│   │     - Call lower driver                        │   │
│   │         ▼                                        │   │
│   │   Driver C (Lower Filter)                       │   │
│   │     - IoCallDriver to next                    │   │
│   │         ▼                                        │   │
│   │   USB Stack                                     │   │
│   │     - Send URB to hardware                    │   │
│   └─────────────────────────────────────────────────┘   │
│                          │                               │
│                          ▼                               │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│          Completion Routines run (reverse order)         │
│                                                         │
│   Driver C's completion → Driver B's completion → ...   │
└─────────────────────────┬───────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│              IRP Completed to application                  │
└─────────────────────────────────────────────────────────┘
```

## ⚠️ 常見陷阱

1. **忘記呼叫下層** - 必須呼叫 `IoCallDriver` 或轉發請求
2. **忘記設定完成例程** - 完成例程必須設定才能接收完成通知
3. **緩衝區鎖定** - 使用 `MmGetSystemAddressForMdl` 鎖定使用者緩衝區
4. **IRP 懸空** - 確保請求最終被完成或轉發
5. **堆疊位置錯誤** - 在 Filter 中必須正確操作堆疊

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
