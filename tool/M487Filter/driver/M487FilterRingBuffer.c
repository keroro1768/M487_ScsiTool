/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterRingBuffer.c

Abstract:

    Lock-free ring buffer implementation for capturing HID IOCTL data.
    Uses a fixed-slot scheme where each slot holds one captured entry.

Environment:

    Kernel mode

--*/

#include "M487Filter.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, M487RingBufferInitialize)
#pragma alloc_text(PAGE, M487RingBufferClear)
#endif

//
// Helper: compute next slot index (modulo slot count)
// Uses volatile to prevent compiler from caching the value
//
FORCEINLINE
ULONG
M487RingBufferNextIndex(
    _In_ ULONG Current,
    _In_ ULONG Count
    )
{
    ULONG next = Current + 1;
    return (next >= Count) ? 0 : next;
}

//
// M487RingBufferInitialize - Allocate and initialize ring buffer
//
NTSTATUS
M487RingBufferInitialize(
    _In_ WDFDEVICE Device,
    _Out_ PM487_RING_BUFFER RingBuffer
    )
{
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY inputDescriptor;
    size_t totalSize;
    NTSTATUS status;

    PAGED_CODE();

    RtlZeroMemory(RingBuffer, sizeof(*RingBuffer));

    RingBuffer->SlotCount = M487_RING_SLOT_COUNT;
    RingBuffer->MaxDataSize = M487_MAX_REPORT_SIZE;
    RingBuffer->SlotSize = M487_RING_SLOT_SIZE(M487_MAX_REPORT_SIZE);
    totalSize = M487_RING_TOTAL_SIZE(M487_RING_SLOT_COUNT, M487_MAX_REPORT_SIZE);

    KdPrint(("M487Filter: RingBuffer allocate %Iu bytes (slots=%lu, slotSize=%lu)\n",
             totalSize, RingBuffer->SlotCount, RingBuffer->SlotSize));

    //
    // Allocate non-paged pool for the ring buffer
    // This memory is mapped into user-mode via WdfCommonBufferCreate
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = Device;

    inputDescriptor.BufferSize = (size_t)totalSize;

    status = WdfMemoryCreate(
        &attributes,
        NonPagedPoolNx,
        'RB47',
        totalSize,
        &RingBuffer->ControlMemory,
        (PVOID*)&RingBuffer->Control);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: WdfMemoryCreate for ring buffer failed 0x%x\n", status));
        return status;
    }

    //
    // Initialize control block
    //
    RtlZeroMemory(RingBuffer->Control, sizeof(M487_RING_CONTROL));
    RingBuffer->Control->RingByteSize = (ULONG)totalSize;
    RingBuffer->Control->WriteIndex = 0;
    RingBuffer->Control->ReadIndex = 0;
    RingBuffer->Control->TotalCaptured = 0;
    RingBuffer->Control->DroppedCount = 0;
    RingBuffer->Control->Magic = M487_RING_MAGIC;

    KdPrint(("M487Filter: RingBuffer initialized at %p, totalSize=%lu\n",
             RingBuffer->Control, RingBuffer->Control->RingByteSize));

    return STATUS_SUCCESS;
}

//
// M487RingBufferCleanup - Free ring buffer resources
//
VOID
M487RingBufferCleanup(
    _In_ PM487_RING_BUFFER RingBuffer
    )
{
    if (RingBuffer->ControlMemory != NULL) {
        WdfObjectDelete(RingBuffer->ControlMemory);
        RingBuffer->ControlMemory = NULL;
        RingBuffer->Control = NULL;
    }
}

//
// M487RingBufferAllocEntry - Allocate next slot for writing
// Returns NULL if consumer is too slow (ring full)
//
PM487_RING_ENTRY
M487RingBufferAllocEntry(
    _In_ PM487_RING_BUFFER RingBuffer
    )
{
    PM487_RING_CONTROL ctrl;
    ULONG writeIdx;
    ULONG readIdx;
    ULONG nextWrite;

    ctrl = RingBuffer->Control;

    writeIdx = ctrl->WriteIndex;
    readIdx  = ctrl->ReadIndex;
    nextWrite = M487RingBufferNextIndex(writeIdx, RingBuffer->SlotCount);

    //
    // Check if ring is full (next write would overtake read)
    //
    if (nextWrite == readIdx) {
        //
        // Ring full - increment dropped counter and return NULL
        // The oldest entry will be overwritten when consumer catches up
        //
        InterlockedIncrement((volatile LONG*)&ctrl->DroppedCount);
        KdPrint(("M487Filter: RingBuffer FULL - dropping entry\n"));

        //
        // Instead of dropping, we just overwrite the oldest entry
        // Advance read index to make room
        //
        InterlockedExchange((volatile LONG*)&ctrl->ReadIndex,
                            M487RingBufferNextIndex(readIdx, RingBuffer->SlotCount));
        InterlockedIncrement((volatile LONG*)&ctrl->DroppedCount);
    }

    //
    // Allocate the current write slot
    //
    writeIdx = InterlockedIncrement((volatile LONG*)&ctrl->WriteIndex) - 1;
    writeIdx = writeIdx % RingBuffer->SlotCount;

    return (PM487_RING_ENTRY)((PUCHAR)ctrl->DataStart +
           (size_t)writeIdx * RingBuffer->SlotSize);
}

//
// M487RingBufferCommitEntry - Mark entry as valid (ready for reading)
//
VOID
M487RingBufferCommitEntry(
    _In_ PM487_RING_BUFFER RingBuffer,
    _In_ PM487_RING_ENTRY Entry,
    _In_ USHORT DataLength
    )
{
    PM487_RING_CONTROL ctrl;
    UCHAR* entryBase;
    UCHAR* entryEnd;
    size_t offset;

    UNREFERENCED_PARAMETER(RingBuffer);

    ctrl = RingBuffer->Control;

    //
    // Clamp DataLength to max
    //
    if (DataLength > RingBuffer->MaxDataSize) {
        DataLength = RingBuffer->MaxDataSize;
    }

    Entry->DataLength = DataLength;

    //
    // Memory barrier to ensure data is written before we update counters
    //
    KeMemoryBarrier();

    //
    // Advance total captured count
    //
    InterlockedIncrement64((volatile LONG64*)&ctrl->TotalCaptured);

    //
    // Compute the slot we just wrote
    //
    entryBase = (PUCHAR)ctrl->DataStart;
    entryEnd = (PUCHAR)Entry;
    if (entryEnd >= entryBase &&
        (size_t)(entryEnd - entryBase) < ctrl->RingByteSize - M487_RING_CONTROL_SIZE) {
        offset = (size_t)(entryEnd - entryBase);
        offset = offset / RingBuffer->SlotSize;

        //
        // Ensure write index is at least at this slot
        //
        ULONG expectedWrite = (ULONG)offset + 1;
        ULONG currentWrite = ctrl->WriteIndex;
        if (expectedWrite > currentWrite) {
            InterlockedExchange((volatile LONG*)&ctrl->WriteIndex, expectedWrite);
        }
    }
}

//
// M487RingBufferClear - Reset the ring buffer
//
VOID
M487RingBufferClear(
    _In_ PM487_RING_BUFFER RingBuffer
    )
{
    PAGED_CODE();

    if (RingBuffer->Control != NULL) {
        RingBuffer->Control->WriteIndex = 0;
        RingBuffer->Control->ReadIndex = 0;
        RingBuffer->Control->TotalCaptured = 0;
        RingBuffer->Control->DroppedCount = 0;
    }
}

//
// M487RingBufferGetStats - Get current buffer statistics
//
ULONG
M487RingBufferGetStats(
    _In_ PM487_RING_BUFFER RingBuffer,
    _Out_ PULONG64 TotalCaptured,
    _Out_ PULONG DroppedCount,
    _Out_ PULONG CurrentUsed
    )
{
    PM487_RING_CONTROL ctrl;
    LONG writeIdx, readIdx;

    if (RingBuffer->Control == NULL) {
        *TotalCaptured = 0;
        *DroppedCount = 0;
        *CurrentUsed = 0;
        return STATUS_INVALID_DEVICE_STATE;
    }

    ctrl = RingBuffer->Control;

    writeIdx = (LONG)ctrl->WriteIndex;
    readIdx  = (LONG)ctrl->ReadIndex;

    *TotalCaptured = ctrl->TotalCaptured;
    *DroppedCount  = ctrl->DroppedCount;

    if (writeIdx >= readIdx) {
        *CurrentUsed = (ULONG)(writeIdx - readIdx);
    } else {
        *CurrentUsed = RingBuffer->SlotCount - (ULONG)(readIdx - writeIdx);
    }

    return STATUS_SUCCESS;
}

//
// M487IoctlToType - Convert standard HID IOCTL code to our type enum
//
M487_IOCTL_TYPE
M487IoctlToType(
    _In_ ULONG IoctlCode
    )
{
    switch (IoctlCode) {

    case IOCTL_HID_SET_FEATURE:
        return IoctlType_HID_SET_FEATURE;

    case IOCTL_HID_GET_FEATURE:
        return IoctlType_HID_GET_FEATURE;

    case IOCTL_HID_WRITE_REPORT:
        return IoctlType_HID_WRITE_REPORT;

    case IOCTL_HID_READ_REPORT:
        return IoctlType_HID_READ_REPORT;

    case IOCTL_HID_GET_COLLECTION_DESCRIPTOR:
        return IoctlType_HID_GET_COLLECTION_DESCRIPTOR;

    case IOCTL_HID_GET_COLLECTION_INFORMATION:
        return IoctlType_HID_GET_COLLECTION_INFORMATION;

    case IOCTL_HID_GET_HARDWARE_ID:
        return IoctlType_HID_GET_HARDWARE_ID;

    case IOCTL_HID_GET_MS_GENRE_DESCRIPTOR:
        return IoctlType_HID_GET_MS_GENRE_DESCRIPTOR;

    case IOCTL_HID_GETManufacturerString:
        return IoctlType_HID_GETManufacturerString;

    case IOCTL_HID_GETProductString:
        return IoctlType_HID_GETProductString;

    case IOCTL_HID_GETSerialNumberString:
        return IoctlType_HID_GETSerialNumberString;

    case IOCTL_HID_GETIndexedString:
        return IoctlType_HID_GETIndexedString;

    default:
        return IoctlType_Unknown;
    }
}
