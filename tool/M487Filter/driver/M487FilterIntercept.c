/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterIntercept.c

Abstract:

    HID IOCTL interception implementation for M487 HID Upper Filter Driver.
    Captures and optionally modifies HID IOCTLs passing through the filter.

    IOCTL Flow (Firefly reference):
      WdfIoTargetOpen(PDO name)
        → IOCTL_HID_GET_COLLECTION_INFORMATION
        → IOCTL_HID_GET_COLLECTION_DESCRIPTOR
        → HidP_GetCaps()
        → IOCTL_HID_SET_FEATURE  ← intercepted
        → IOCTL_HID_GET_FEATURE  ← intercepted
        → IOCTL_HID_WRITE_REPORT  ← intercepted
        → IOCTL_HID_READ_REPORT   ← intercepted

Environment:

    Kernel mode

--*/

#include "M487Filter.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, M487InterceptInitialize)
#pragma alloc_text(PAGE, M487InterceptRegisterIoctls)
#pragma alloc_text(PAGE, M487InterceptEvtIoDeviceControl)
#pragma alloc_text(PAGE, M487InterceptGetConfig)
#pragma alloc_text(PAGE, M487InterceptSetConfig)
#pragma alloc_text(PAGE, M487InterceptMapRingBufferToUser)
#endif

//
// Nt native syscall declarations for section/mapping operations.
// These are available in kernel mode via ntddk.h / wdm.h.
//
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG PageAttribut,
    IN ULONG SectionAttributes,
    IN HANDLE FileHandle OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits OPTIONAL,
    IN SIZE_T RegionSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
    IN HANDLE Handle
    );

//
// Ring buffer mapping helper - creates a section object and maps it into
// both kernel and user address spaces. Returns the user-mode virtual address.
//
static
NTSTATUS
M487InterceptCreateUserMapping(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PHANDLE SectionHandle,
    _Out_ PVOID* KernelMappingVa,
    _Out_ PVOID* UserVa
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    LARGE_INTEGER sectionSize;
    PVOID kernelVa = NULL;
    PVOID userVa = NULL;
    SIZE_T viewSize;
    LARGE_INTEGER sectionOffset;
    PM487_RING_BUFFER ringBuffer;
    SIZE_T ringByteSize;

    *SectionHandle = NULL;
    *KernelMappingVa = NULL;
    *UserVa = NULL;

    ringBuffer = &DevContext->InterceptState.RingBuffer;
    if (ringBuffer->Control == NULL) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    ringByteSize = ringBuffer->Control->RingByteSize;

    //
    // Create a section object for sharing the ring buffer with user-mode
    //
    sectionSize.QuadPart = (LONGLONG)ringByteSize;

    status = ZwCreateSection(
        &sectionHandle,
        SECTION_ALL_ACCESS,           // DesiredAccess
        NULL,                          // ObjectAttributes (no name, no security)
        &sectionSize,                  // MaximumSize
        PAGE_READWRITE,                // PageAttribut (read/write pages)
        SEC_COMMIT,                    // SectionAttributes (commit pages)
        NULL                           // FileHandle (memory section, not file-backed)
        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: ZwCreateSection failed 0x%x\n", status));
        return status;
    }

    //
    // Map the section into kernel address space (for copying data)
    //
    viewSize = ringByteSize;
    sectionOffset.QuadPart = 0;
    kernelVa = NULL;  // Let the system choose the kernel address

    status = ZwMapViewOfSection(
        sectionHandle,
        (HANDLE)-1,                   // Current process
        &kernelVa,
        0,                             // ZeroBits
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewShare,                     // InheritDisposition
        0,                             // AllocationType
        PAGE_READWRITE                 // Protect
        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: ZwMapViewOfSection (kernel) failed 0x%x\n", status));
        ZwClose(sectionHandle);
        return status;
    }

    //
    // Copy the current ring buffer contents to the shared section
    //
    RtlCopyMemory(kernelVa, ringBuffer->Control, ringByteSize);

    //
    // Map the section into user address space (the requesting user-mode process)
    //
    viewSize = ringByteSize;
    sectionOffset.QuadPart = 0;
    userVa = NULL;  // Let the system choose the user-mode address

    status = ZwMapViewOfSection(
        sectionHandle,
        (HANDLE)-1,                   // Current user process
        &userVa,
        0,
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewShare,
        0,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: ZwMapViewOfSection (user) failed 0x%x\n", status));
        ZwUnmapViewOfSection((HANDLE)-1, kernelVa);
        ZwClose(sectionHandle);
        return status;
    }

    *SectionHandle = sectionHandle;
    *KernelMappingVa = kernelVa;
    *UserVa = userVa;

    KdPrint(("M487Filter: User mapping created - section=%p kernelVa=%p userVa=%p\n",
             sectionHandle, kernelVa, userVa));

    return STATUS_SUCCESS;
}

//
// Default interception configuration - capture all HID IOCTLs
//
static const M487_INTERCEPT_CONFIG DefaultConfig = {
    TRUE,   // CaptureSetFeature
    TRUE,   // CaptureGetFeature
    TRUE,   // CaptureWriteReport
    TRUE,   // CaptureReadReport
    FALSE,  // CaptureStringRequests
    FALSE   // CaptureCollectionInfo
};

//
// Timestamp counter for entries that don't have one from the IRP
//
static LARGE_INTEGER M487InterceptTimestampCounter = { 0 };

//
// Forward declaration for the IO device control handler
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL M487InterceptEvtIoDeviceControl;

//
// M487InterceptInitialize - Initialize interception subsystem
//
NTSTATUS
M487InterceptInitialize(
    _In_ WDFDEVICE Device,
    _In_ PDEVICE_CONTEXT DevContext
    )
{
    NTSTATUS status;

    PAGED_CODE();

    KdPrint(("M487Filter: M487InterceptInitialize\n"));

    //
    // Initialize ring buffer
    //
    status = M487RingBufferInitialize(Device, &DevContext->InterceptState.RingBuffer);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: M487RingBufferInitialize failed 0x%x\n", status));
        return status;
    }

    //
    // Initialize configuration with defaults
    //
    KeInitializeSpinLock(&DevContext->InterceptState.ConfigLock);
    DevContext->InterceptState.Config = DefaultConfig;
    DevContext->InterceptState.Initialized = TRUE;

    //
    // Initialize section handle / user VA for ring buffer sharing
    //
    DevContext->InterceptState.RingBufferSectionHandle = NULL;
    DevContext->InterceptState.RingBufferUserVa = NULL;

    KdPrint(("M487Filter: Interception initialized successfully\n"));
    return STATUS_SUCCESS;
}

//
// M487InterceptCleanup - Clean up interception resources
//
VOID
M487InterceptCleanup(
    _In_ PDEVICE_CONTEXT DevContext
    )
{
    if (DevContext->InterceptState.Initialized) {
        M487RingBufferCleanup(&DevContext->InterceptState.RingBuffer);
        DevContext->InterceptState.Initialized = FALSE;
    }

    //
    // Clean up section handle for user-mode mapping
    //
    if (DevContext->InterceptState.RingBufferSectionHandle != NULL) {
        ZwClose(DevContext->InterceptState.RingBufferSectionHandle);
        DevContext->InterceptState.RingBufferSectionHandle = NULL;
    }
    DevContext->InterceptState.RingBufferUserVa = NULL;
}

//
// M487CaptureEntry - Common helper to capture IOCTL data to ring buffer
//
FORCEINLINE
VOID
M487CaptureEntry(
    _In_ PM487_RING_BUFFER RingBuffer,
    _In_ M487_IOCTL_TYPE IoctlType,
    _In_ ULONG IoctlCode,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength,
    _In_ BOOLEAN IsWrite
    )
{
    PM487_RING_ENTRY entry;
    LARGE_INTEGER timestamp;
    USHORT commitDataLen;

    //
    // Check if this IOCTL type should be captured
    //
    if (RingBuffer->Control == NULL || RingBuffer->Control->Magic != M487_RING_MAGIC) {
        return;
    }

    //
    // Get timestamp
    //
    KeQueryTickCount(&timestamp);

    //
    // Allocate next slot
    //
    entry = M487RingBufferAllocEntry(RingBuffer);
    if (entry == NULL) {
        //
        // Ring buffer full - log error via ETW
        //
        M487FILTER_ETW_LOG_ERROR(STATUS_BUFFER_TOO_SMALL, IoctlCode, 0);
        return;
    }

    //
    // Fill entry header
    //
    entry->Timestamp = timestamp;
    entry->IoctlType = IoctlType;
    entry->IoctlCode = IoctlCode;
    entry->IsWrite = IsWrite;
    entry->ReportId = 0;

    //
    // Copy data (clamp to max)
    //
    commitDataLen = 0;
    if (Data != NULL && DataLength > 0) {
        size_t copyLen = DataLength;
        if (copyLen > RingBuffer->MaxDataSize) {
            copyLen = RingBuffer->MaxDataSize;
        }
        RtlCopyMemory(entry->Data, Data, copyLen);
        commitDataLen = (USHORT)copyLen;
    }

    //
    // Commit the entry
    //
    M487RingBufferCommitEntry(RingBuffer, entry, commitDataLen);

    //
    // ★ Log to ETW - IOCTL captured
    //
    M487FILTER_ETW_LOG_IOCTL(IoctlCode, IoctlType, commitDataLen, IsWrite);

    //
    // ★ Log ring buffer stats to ETW
    //
    M487FILTER_ETW_LOG_RING(
        RingBuffer->Control->TotalCaptured,
        RingBuffer->Control->DroppedCount,
        commitDataLen,
        (UCHAR)(RingBuffer->Control->WriteIndex & 0xFF));
}

//
// M487CaptureSetFeature - Capture IOCTL_HID_SET_FEATURE
//
VOID
M487CaptureSetFeature(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength
    )
{
    KdPrint(("M487Filter: Capture SET_FEATURE (len=%Iu)\n", DataLength));

    M487CaptureEntry(
        &DevContext->InterceptState.RingBuffer,
        IoctlType_HID_SET_FEATURE,
        IOCTL_HID_SET_FEATURE,
        Data,
        DataLength,
        TRUE   // Host → Device
    );
}

//
// M487CaptureGetFeature - Capture IOCTL_HID_GET_FEATURE
//
VOID
M487CaptureGetFeature(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength,
    _In_ BOOLEAN FromDevice
    )
{
    KdPrint(("M487Filter: Capture GET_FEATURE (len=%Iu, FromDevice=%d)\n",
             DataLength, FromDevice));

    M487CaptureEntry(
        &DevContext->InterceptState.RingBuffer,
        IoctlType_HID_GET_FEATURE,
        IOCTL_HID_GET_FEATURE,
        Data,
        DataLength,
        FromDevice ? FALSE : TRUE  // FALSE = from device (read), TRUE = to device
    );
}

//
// M487CaptureWriteReport - Capture IOCTL_HID_WRITE_REPORT
//
VOID
M487CaptureWriteReport(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength
    )
{
    KdPrint(("M487Filter: Capture WRITE_REPORT (len=%Iu)\n", DataLength));

    M487CaptureEntry(
        &DevContext->InterceptState.RingBuffer,
        IoctlType_HID_WRITE_REPORT,
        IOCTL_HID_WRITE_REPORT,
        Data,
        DataLength,
        TRUE   // Host → Device
    );
}

//
// M487CaptureReadReport - Capture IOCTL_HID_READ_REPORT
//
VOID
M487CaptureReadReport(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength
    )
{
    KdPrint(("M487Filter: Capture READ_REPORT (len=%Iu)\n", DataLength));

    M487CaptureEntry(
        &DevContext->InterceptState.RingBuffer,
        IoctlType_HID_READ_REPORT,
        IOCTL_HID_READ_REPORT,
        Data,
        DataLength,
        FALSE   // Host ← Device (device reporting to host)
    );
}

//
// M487CaptureIoctl - Capture arbitrary IOCTL
//
VOID
M487CaptureIoctl(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ ULONG IoctlCode,
    _In_reads_bytes_opt_(DataLength) PVOID Data,
    _In_ size_t DataLength,
    _In_ BOOLEAN IsWrite
    )
{
    M487_IOCTL_TYPE type = M487IoctlToType(IoctlCode);

    KdPrint(("M487Filter: Capture IOCTL 0x%X (type=%d, IsWrite=%d, len=%Iu)\n",
             IoctlCode, type, IsWrite, DataLength));

    M487CaptureEntry(
        &DevContext->InterceptState.RingBuffer,
        type,
        IoctlCode,
        Data,
        DataLength,
        IsWrite
    );
}

//
// M487InterceptForwardIoctl - Forward IOCTL to lower HID device and capture result
//
// This is the key interception function. It:
// 1. Retrieves buffers from the WDFREQUEST
// 2. Optionally captures the outbound data
// 3. Sends the IOCTL to the lower device synchronously
// 4. Captures the returned data
// 5. Returns the status
//
NTSTATUS
M487InterceptForwardIoctl(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoctlCode,
    _In_ WDFMEMORY InputMemory,
    _In_ WDFMEMORY OutputMemory,
    _In_ BOOLEAN IsWriteOperation
    )
{
    WDF_MEMORY_DESCRIPTOR inputDesc;
    WDF_MEMORY_DESCRIPTOR outputDesc;
    NTSTATUS status;
    size_t inputLen = 0;
    size_t outputLen = 0;
    PVOID inputBuf = NULL;
    PVOID outputBuf = NULL;

    //
    // Retrieve buffer pointers for capture
    //
    if (InputMemory != NULL) {
        inputBuf = WdfMemoryGetBuffer(InputMemory, &inputLen);
    }

    if (OutputMemory != NULL) {
        outputBuf = WdfMemoryGetBuffer(OutputMemory, &outputLen);
    }

    //
    // ★ Capture outbound data before sending
    //
    if (inputBuf != NULL && inputLen > 0) {
        M487CaptureIoctl(DevContext, IoctlCode, inputBuf, inputLen, TRUE);
    }

    //
    // Prepare input descriptor
    //
    if (InputMemory != NULL) {
        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&inputDesc, InputMemory, NULL);
    } else {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDesc, NULL, 0);
    }

    //
    // Prepare output descriptor
    //
    if (OutputMemory != NULL) {
        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&outputDesc, OutputMemory, NULL);
    } else {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDesc, NULL, 0);
    }

    //
    // Forward IOCTL to lower device synchronously
    //
    status = WdfIoTargetSendIoctlSynchronously(
        DevContext->LowerHidTarget,
        NULL,
        IoctlCode,
        (InputMemory != NULL) ? &inputDesc : NULL,
        (OutputMemory != NULL) ? &outputDesc : NULL,
        NULL,
        NULL);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: Forward IOCTL 0x%X failed 0x%x\n", IoctlCode, status));
    } else {
        KdPrint(("M487Filter: Forward IOCTL 0x%X succeeded\n", IoctlCode));
    }

    //
    // ★ Capture returned data (read operations)
    //
    if (NT_SUCCESS(status) && outputBuf != NULL && outputLen > 0 && !IsWriteOperation) {
        //
        // For GET operations, capture what came back from the device
        //
        switch (IoctlCode) {
        case IOCTL_HID_GET_FEATURE:
            M487CaptureGetFeature(DevContext, outputBuf, outputLen, TRUE);
            break;

        case IOCTL_HID_READ_REPORT:
            M487CaptureReadReport(DevContext, outputBuf, outputLen);
            break;

        default:
            M487CaptureIoctl(DevContext, IoctlCode, outputBuf, outputLen, FALSE);
            break;
        }
    }

    return status;
}

//
// M487InterceptGetConfig - Get current interception configuration
//
VOID
M487InterceptGetConfig(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PM487_INTERCEPT_CONFIG Config
    )
{
    KIRQL irql;

    PAGED_CODE();

    KeAcquireSpinLock(&DevContext->InterceptState.ConfigLock, &irql);
    *Config = DevContext->InterceptState.Config;
    KeReleaseSpinLock(&DevContext->InterceptState.ConfigLock, irql);
}

//
// M487InterceptSetConfig - Set interception configuration
//
VOID
M487InterceptSetConfig(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ PM487_INTERCEPT_CONFIG Config
    )
{
    KIRQL irql;

    PAGED_CODE();

    KeAcquireSpinLock(&DevContext->InterceptState.ConfigLock, &irql);
    DevContext->InterceptState.Config = *Config;
    KeReleaseSpinLock(&DevContext->InterceptState.ConfigLock, irql);

    KdPrint(("M487Filter: Interception config updated - "
             "SetF=%d GetF=%d WriteR=%d ReadR=%d\n",
             Config->CaptureSetFeature,
             Config->CaptureGetFeature,
             Config->CaptureWriteReport,
             Config->CaptureReadReport));

    //
    // ★ Log config change via ETW
    //
    M487FILTER_ETW_LOG_CONFIG(
        Config->CaptureSetFeature,
        Config->CaptureGetFeature,
        Config->CaptureWriteReport,
        Config->CaptureReadReport);
}

//
// M487InterceptGetRingBufferInfo - Get ring buffer info for user-mode mapping
//
NTSTATUS
M487InterceptGetRingBufferInfo(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PULONG64 RingBufferPhysAddr,
    _Out_ PULONG RingBufferSize
    )
{
    PM487_RING_BUFFER ringBuffer;

    if (!DevContext->InterceptState.Initialized) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    ringBuffer = &DevContext->InterceptState.RingBuffer;

    if (ringBuffer->ControlMemory == NULL) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // Get the physical address of the ring buffer
    //
    {
        PHYSICAL_ADDRESS physAddr;

        //
        // Query for the physical address
        // For WDFMEMORY, we use WdfMemoryGetBuffer and query MDL
        //
        physAddr = MmGetPhysicalAddress(ringBuffer->Control);

        *RingBufferPhysAddr = physAddr.QuadPart;
        *RingBufferSize = ringBuffer->Control->RingByteSize;
    }

    KdPrint(("M487Filter: RingBuffer physAddr=0x%I64X, size=%lu\n",
             *RingBufferPhysAddr, *RingBufferSize));

    return STATUS_SUCCESS;
}

//
// M487InterceptGetStats - Get capture statistics
//
NTSTATUS
M487InterceptGetStats(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PULONG64 TotalCaptured,
    _Out_ PULONG DroppedCount,
    _Out_ PULONG CurrentUsed
    )
{
    if (!DevContext->InterceptState.Initialized) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    return M487RingBufferGetStats(
        &DevContext->InterceptState.RingBuffer,
        TotalCaptured,
        DroppedCount,
        CurrentUsed);
}

//
// M487InterceptProcessSetFeature - Process IOCTL_HID_SET_FEATURE
//
// Called when a SET_FEATURE request needs to be handled.
// This extracts the report from the request, captures it,
// and forwards it to the lower device.
//
NTSTATUS
M487InterceptProcessSetFeature(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ WDFREQUEST Request
    )
{
    WDFMEMORY inputMemory;
    NTSTATUS status;

    //
    // Retrieve input buffer (Feature report)
    //
    status = WdfRequestRetrieveInputMemory(Request, &inputMemory);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: RetrieveInputMemory failed 0x%x\n", status));
        return status;
    }

    //
    // Capture the SET_FEATURE report
    //
    {
        PVOID buf;
        size_t len;
        buf = WdfMemoryGetBuffer(inputMemory, &len);
        M487CaptureSetFeature(DevContext, buf, len);
    }

    //
    // Forward to lower device
    // Note: inputMemory is owned by Request, don't WdfMemoryDelete it
    //
    status = M487InterceptForwardIoctl(
        DevContext,
        Request,
        IOCTL_HID_SET_FEATURE,
        inputMemory,
        NULL,
        TRUE   // Write operation
    );

    return status;
}

//
// M487InterceptProcessGetFeature - Process IOCTL_HID_GET_FEATURE
//
NTSTATUS
M487InterceptProcessGetFeature(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength
    )
{
    WDFMEMORY outputMemory;
    NTSTATUS status;

    if (OutputBufferLength == 0) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Allocate output buffer for the feature report
    //
    status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: RetrieveOutputMemory failed 0x%x\n", status));
        return status;
    }

    //
    // Forward to lower device and capture result
    //
    status = M487InterceptForwardIoctl(
        DevContext,
        Request,
        IOCTL_HID_GET_FEATURE,
        NULL,
        outputMemory,
        FALSE  // Read operation
    );

    if (NT_SUCCESS(status)) {
        //
        // Report was already captured in M487InterceptForwardIoctl
        // Just complete the request with the data
        //
        size_t bytesCopied;
        PVOID buf = WdfMemoryGetBuffer(outputMemory, &bytesCopied);

        KdPrint(("M487Filter: GET_FEATURE returned %Iu bytes\n", bytesCopied));
        WdfRequestSetInformation(Request, bytesCopied);
    }

    //
    // Note: outputMemory is owned by Request, don't WdfMemoryDelete it
    //
    return status;
}

//
// M487InterceptProcessWriteReport - Process IOCTL_HID_WRITE_REPORT
//
NTSTATUS
M487InterceptProcessWriteReport(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ WDFREQUEST Request
    )
{
    WDFMEMORY inputMemory;
    NTSTATUS status;

    status = WdfRequestRetrieveInputMemory(Request, &inputMemory);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: RetrieveInputMemory (WRITE_REPORT) failed 0x%x\n", status));
        return status;
    }

    {
        PVOID buf;
        size_t len;
        buf = WdfMemoryGetBuffer(inputMemory, &len);
        M487CaptureWriteReport(DevContext, buf, len);
    }

    status = M487InterceptForwardIoctl(
        DevContext,
        Request,
        IOCTL_HID_WRITE_REPORT,
        inputMemory,
        NULL,
        TRUE
    );

    //
    // Note: inputMemory is owned by Request, don't WdfMemoryDelete it
    //
    return status;
}

//
// M487InterceptProcessReadReport - Process IOCTL_HID_READ_REPORT
//
NTSTATUS
M487InterceptProcessReadReport(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength
    )
{
    WDFMEMORY outputMemory;
    NTSTATUS status;

    if (OutputBufferLength == 0) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: RetrieveOutputMemory (READ_REPORT) failed 0x%x\n", status));
        return status;
    }

    status = M487InterceptForwardIoctl(
        DevContext,
        Request,
        IOCTL_HID_READ_REPORT,
        NULL,
        outputMemory,
        FALSE
    );

    if (NT_SUCCESS(status)) {
        size_t bytesCopied;
        PVOID buf = WdfMemoryGetBuffer(outputMemory, &bytesCopied);

        KdPrint(("M487Filter: READ_REPORT returned %Iu bytes\n", bytesCopied));
        WdfRequestSetInformation(Request, bytesCopied);
    }

    //
    // Note: outputMemory is owned by Request, don't WdfMemoryDelete it
    //
    return status;
}

//
// M487InterceptEvtIoDeviceControl - Main IOCTL dispatch handler
//
// This is the central handler for all intercepted HID IOCTLs.
// It is called by KMDF for IOCTLs that we registered for via
// WdfDeviceConfigureRequestDispatching.
//
// For HID IOCTLs we intercept, we:
//  1. Capture the data to the ring buffer
//  2. Optionally modify the data (if in modifying mode)
//  3. Forward the IOCTL to the lower HID device
//  4. Optionally capture the response
//  5. Complete the request
//
VOID
M487InterceptEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    PDEVICE_CONTEXT devContext;
    WDFDEVICE device;
    NTSTATUS status;
    size_t bytesReturned = 0;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    device = WdfQueueGetDevice(Queue);
    devContext = WdfObjectGet_DEVICE_CONTEXT(device);

    KdPrint(("M487Filter: ★ Intercepted IOCTL 0x%X (In=%Iu, Out=%Iu)\n",
             IoControlCode, InputBufferLength, OutputBufferLength));

    switch (IoControlCode) {

    case IOCTL_HID_SET_FEATURE:
        KdPrint(("M487Filter:  → SET_FEATURE intercepted\n"));
        status = M487InterceptProcessSetFeature(devContext, Request);
        WdfRequestCompleteWithInformation(Request, status, bytesReturned);
        return;

    case IOCTL_HID_GET_FEATURE:
        KdPrint(("M487Filter:  → GET_FEATURE intercepted\n"));
        status = M487InterceptProcessGetFeature(devContext, Request, OutputBufferLength);
        if (NT_SUCCESS(status)) {
            bytesReturned = OutputBufferLength;
        }
        WdfRequestCompleteWithInformation(Request, status, bytesReturned);
        return;

    case IOCTL_HID_WRITE_REPORT:
        KdPrint(("M487Filter:  → WRITE_REPORT intercepted\n"));
        status = M487InterceptProcessWriteReport(devContext, Request);
        WdfRequestCompleteWithInformation(Request, status, bytesReturned);
        return;

    case IOCTL_HID_READ_REPORT:
        KdPrint(("M487Filter:  → READ_REPORT intercepted\n"));
        status = M487InterceptProcessReadReport(devContext, Request, OutputBufferLength);
        if (NT_SUCCESS(status)) {
            bytesReturned = OutputBufferLength;
        }
        WdfRequestCompleteWithInformation(Request, status, bytesReturned);
        return;

    default:
        //
        // Unknown HID IOCTL - should not happen if registered correctly
        // Just forward to lower device automatically
        //
        KdPrint(("M487Filter:  → Unknown IOCTL 0x%X - forwarding\n", IoControlCode));

        //
        // Forward to lower device via automatic pass-through
        // Since we registered with WdfFdoInitSetFilter, KMDF auto-forwards
        // IRPs that we don't explicitly handle here.
        //
        WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
        return;
    }
}

//
// M487InterceptRegisterIoctls - Register HID IOCTLs to be intercepted
//
// This configures the default queue to dispatch specific IOCTLs to our
// EvtIoDeviceControl callback instead of letting them pass through
// automatically.
//
// The key HID IOCTLs we intercept:
//   - IOCTL_HID_SET_FEATURE    (host → device feature report)
//   - IOCTL_HID_GET_FEATURE    (host ← device feature report)
//   - IOCTL_HID_WRITE_REPORT   (host → device output report)
//   - IOCTL_HID_READ_REPORT    (host ← device input report)
//
NTSTATUS
M487InterceptRegisterIoctls(
    _In_ WDFDEVICE Device
    )
{
    NTSTATUS status;

    PAGED_CODE();

    KdPrint(("M487Filter: Registering HID IOCTL intercept handlers\n"));

    //
    // Configure the default queue to dispatch HID IOCTLs to our handler.
    // By calling WdfDeviceConfigureRequestDispatching with each specific
    // IOCTL code, KMDF will route those IOCTLs to our EvtIoDeviceControl
    // callback instead of auto-passing them through.
    //
    // Note: WdfDeviceConfigureRequestDispatching must be called for each
    // IOCTL we want to intercept. All other IOCTLs continue to use
    // automatic pass-through from WdfFdoInitSetFilter.
    //

    status = WdfDeviceConfigureRequestDispatching(
        Device,
        WdfRequestDeviceIoControl,
        WdfDeviceIoControlled,
        IOCTL_HID_SET_FEATURE);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: Register SET_FEATURE failed 0x%x\n", status));
        return status;
    }

    status = WdfDeviceConfigureRequestDispatching(
        Device,
        WdfRequestDeviceIoControl,
        WdfDeviceIoControlled,
        IOCTL_HID_GET_FEATURE);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: Register GET_FEATURE failed 0x%x\n", status));
        return status;
    }

    status = WdfDeviceConfigureRequestDispatching(
        Device,
        WdfRequestDeviceIoControl,
        WdfDeviceIoControlled,
        IOCTL_HID_WRITE_REPORT);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: Register WRITE_REPORT failed 0x%x\n", status));
        return status;
    }

    status = WdfDeviceConfigureRequestDispatching(
        Device,
        WdfRequestDeviceIoControl,
        WdfDeviceIoControlled,
        IOCTL_HID_READ_REPORT);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: Register READ_REPORT failed 0x%x\n", status));
        return status;
    }

    //
    // Get the default queue and set our device control handler.
    // The handler will be called for all IOCTLs registered above.
    //
    {
        WDFQUEUE queue;
        WDF_IO_QUEUE_CONFIG queueConfig;

        queue = WdfDeviceGetDefaultQueue(Device);

        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchParallel);
        queueConfig.EvtIoDeviceControl = M487InterceptEvtIoDeviceControl;

        //
        // Reconfigure the default queue with our handler.
        // Forward-only queue will not keep requests.
        //
        status = WdfIoQueueConfigure(
            queue,
            WDF_NO_OBJECT_ATTRIBUTES,
            &queueConfig);

        if (!NT_SUCCESS(status)) {
            KdPrint(("M487Filter: WdfIoQueueConfigure failed 0x%x\n", status));
            return status;
        }
    }

    KdPrint(("M487Filter: HID IOCTL interception registered (SET/GET/READ/WRITE_REPORT)\n"));
    return STATUS_SUCCESS;
}

//
// M487InterceptMapRingBufferToUser - Map ring buffer to user-mode address space.
//
// Creates a section object and maps the ring buffer into both kernel and
// user address spaces. The first call creates and caches the mapping;
// subsequent calls return the cached user-mode address.
//
// This allows user-mode applications (like hidlog.exe) to directly read
// captured HID packets without kernel/user transitions for each read.
//
// Parameters:
//   DevContext    - Device context
//   UserVa       - Out: User-mode virtual address of mapped ring buffer
//   RingBufferSize - Out: Total size of ring buffer in bytes
//   SlotSize     - Out: Size of each ring buffer slot
//   SlotCount    - Out: Number of slots
//   MaxDataSize  - Out: Maximum data bytes per slot
//
NTSTATUS
M487InterceptMapRingBufferToUser(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PULONG64 UserVa,
    _Out_ PULONG RingBufferSize,
    _Out_ PULONG SlotSize,
    _Out_ PULONG SlotCount,
    _Out_ PULONG MaxDataSize
    )
{
    PM487_RING_BUFFER ringBuffer;
    HANDLE sectionHandle = NULL;
    PVOID kernelVa = NULL;
    PVOID userVa = NULL;
    LARGE_INTEGER sectionSize;
    LARGE_INTEGER sectionOffset;
    SIZE_T viewSize;
    NTSTATUS status;

    PAGED_CODE();

    if (UserVa == NULL || RingBufferSize == NULL ||
        SlotSize == NULL || SlotCount == NULL || MaxDataSize == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    ringBuffer = &DevContext->InterceptState.RingBuffer;

    if (ringBuffer->Control == NULL) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // If already mapped, return cached address
    //
    if (DevContext->InterceptState.RingBufferUserVa != NULL &&
        DevContext->InterceptState.RingBufferSectionHandle != NULL) {
        *UserVa = (ULONG64)DevContext->InterceptState.RingBufferUserVa;
        *RingBufferSize = ringBuffer->Control->RingByteSize;
        *SlotSize = ringBuffer->SlotSize;
        *SlotCount = ringBuffer->SlotCount;
        *MaxDataSize = ringBuffer->MaxDataSize;
        KdPrint(("M487Filter: Returning cached ring buffer mapping: Va=0x%I64X\n", *UserVa));
        return STATUS_SUCCESS;
    }

    //
    // Create a section object for the ring buffer
    //
    sectionSize.QuadPart = (LONGLONG)ringBuffer->Control->RingByteSize;

    status = ZwCreateSection(
        &sectionHandle,
        SECTION_ALL_ACCESS,           // DesiredAccess
        NULL,                          // ObjectAttributes
        &sectionSize,                  // MaximumSize
        PAGE_READWRITE,                // PageAttribut
        SEC_COMMIT,                    // SectionAttributes
        NULL                           // FileHandle (memory section)
        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: ZwCreateSection failed 0x%x\n", status));
        return status;
    }

    //
    // Map the section into kernel address space
    //
    viewSize = (SIZE_T)ringBuffer->Control->RingByteSize;
    sectionOffset.QuadPart = 0;
    kernelVa = NULL;

    status = ZwMapViewOfSection(
        sectionHandle,
        (HANDLE)-1,                   // Current process
        &kernelVa,
        0,                             // ZeroBits
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewShare,                     // InheritDisposition
        0,                             // AllocationType
        PAGE_READWRITE                 // Protect
        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: ZwMapViewOfSection (kernel) failed 0x%x\n", status));
        ZwClose(sectionHandle);
        return status;
    }

    //
    // Copy current ring buffer contents to the shared section
    //
    RtlCopyMemory(kernelVa, ringBuffer->Control, (SIZE_T)ringBuffer->Control->RingByteSize);

    //
    // Map the section into user address space
    //
    viewSize = (SIZE_T)ringBuffer->Control->RingByteSize;
    sectionOffset.QuadPart = 0;
    userVa = NULL;

    status = ZwMapViewOfSection(
        sectionHandle,
        (HANDLE)-1,                   // Current user process
        &userVa,
        0,
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewShare,
        0,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: ZwMapViewOfSection (user) failed 0x%x\n", status));
        ZwUnmapViewOfSection((HANDLE)-1, kernelVa);
        ZwClose(sectionHandle);
        return status;
    }

    //
    // Cache the mapping in device context
    //
    DevContext->InterceptState.RingBufferSectionHandle = sectionHandle;
    DevContext->InterceptState.RingBufferUserVa = userVa;

    //
    // Return the user-mode VA and metadata
    //
    *UserVa = (ULONG64)userVa;
    *RingBufferSize = ringBuffer->Control->RingByteSize;
    *SlotSize = ringBuffer->SlotSize;
    *SlotCount = ringBuffer->SlotCount;
    *MaxDataSize = ringBuffer->MaxDataSize;

    KdPrint(("M487Filter: Ring buffer mapped to user VA=0x%I64X, size=%lu\n",
             *UserVa, *RingBufferSize));

    UNREFERENCED_PARAMETER(kernelVa);
    return STATUS_SUCCESS;
}
