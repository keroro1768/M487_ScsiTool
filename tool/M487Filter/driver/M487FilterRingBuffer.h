/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterRingBuffer.h

Abstract:

    Lock-free ring buffer for capturing HID IOCTL data.
    Single-producer (driver) / single-consumer (user-mode app) model.

Environment:

    Kernel mode

--*/

#ifndef _M487_FILTER_RING_BUFFER_H
#define _M487_FILTER_RING_BUFFER_H

#include <ntddk.h>
#include <wdf.h>

//
// Ring buffer entry header (fixed-size, followed by variable data)
// Total slot size = sizeof(M487_RING_ENTRY) + MaxReportSize
//
#define M487_RING_SLOT_COUNT     256
#define M487_MAX_REPORT_SIZE     64   // Max HID report size in bytes

//
// IOCTL types captured by the ring buffer
//
typedef enum _M487_IOCTL_TYPE
{
    IoctlType_Unknown = 0,
    IoctlType_HID_SET_FEATURE,       // Host → Device feature report
    IoctlType_HID_GET_FEATURE,        // Host ← Device feature report
    IoctlType_HID_WRITE_REPORT,       // Host → Device output report
    IoctlType_HID_READ_REPORT,        // Host ← Device input report
    IoctlType_HID_GET_COLLECTION_DESCRIPTOR,
    IoctlType_HID_GET_COLLECTION_INFORMATION,
    IoctlType_HID_GET_HARDWARE_ID,
    IoctlType_HID_GETManufacturerString,
    IoctlType_HID_GETProductString,
    IoctlType_HID_GETSerialNumberString,
    IoctlType_HID_GETIndexedString,
    IoctlType_HID_GET_MS_GENRE_DESCRIPTOR

} M487_IOCTL_TYPE;

//
// Ring buffer entry
//
typedef struct _M487_RING_ENTRY
{
    //
    // Timestamp when entry was captured (in 100ns units since boot)
    //
    LARGE_INTEGER Timestamp;

    //
    // IOCTL type identifier
    //
    M487_IOCTL_TYPE IoctlType;

    //
    // Actual IOCTL code received (e.g. IOCTL_HID_SET_FEATURE)
    //
    ULONG IoctlCode;

    //
    // Direction: TRUE = Host→Device (WRITE), FALSE = Host←Device (READ)
    //
    BOOLEAN IsWrite;

    //
    // Length of data in Data[] field (0 if no data)
    //
    USHORT DataLength;

    //
    // Report ID if applicable (0 = no report ID)
    //
    UCHAR ReportId;

    //
    // Padding for alignment
    //
    UCHAR Reserved[2];

    //
    // Variable-length HID report data starts here.
    // Access via: RING_ENTRY_DATA(Entry)[0..DataLength-1]
    //
    UCHAR Data[1];

} M487_RING_ENTRY, *PM487_RING_ENTRY;

//
// Helper macro to get data pointer from entry
//
#define RING_ENTRY_DATA(Entry)  ((Entry)->Data)

//
// Compute total slot size for a given data length
//
#define M487_RING_SLOT_SIZE(DataSize)  \
    (sizeof(M487_RING_ENTRY) + (DataSize) - 1)

//
// Control block shared between driver and user-mode app
// Located at the beginning of the ring buffer section
//
typedef struct _M487_RING_CONTROL
{
    //
    // Total byte size of the ring buffer (power of 2)
    //
    volatile ULONG RingByteSize;

    //
    // Write slot index (producer = driver)
    //
    volatile ULONG WriteIndex;

    //
    // Read slot index (consumer = user-mode app)
    //
    volatile ULONG ReadIndex;

    //
    // Number of entries ever captured (wraps at 32-bit)
    //
    volatile ULONG64 TotalCaptured;

    //
    // Number of entries dropped (consumer couldn't keep up)
    //
    volatile ULONG DroppedCount;

    //
    // Magic signature to validate buffer is initialized
    //
    volatile ULONG Magic;

    //
    // Padding to align DataStart to 8 bytes
    //
    UCHAR Reserved[4];

    //
    // Start of ring buffer slots
    // Each slot is M487_RING_SLOT_SIZE(M487_MAX_REPORT_SIZE) bytes
    //
    UCHAR DataStart[1];

} M487_RING_CONTROL, *PM487_RING_CONTROL;

#define M487_RING_MAGIC   0x4D343837  // 'M487'

//
// Maximum size of control block (without slots)
//
#define M487_RING_CONTROL_SIZE  \
    FIELD_OFFSET(M487_RING_CONTROL, DataStart)

//
// Compute total allocation size for ring buffer
//
#define M487_RING_TOTAL_SIZE(SlotCount, DataSize)  \
    (M487_RING_CONTROL_SIZE + (SlotCount) * M487_RING_SLOT_SIZE(DataSize))

//
// Ring buffer state object
//
typedef struct _M487_RING_BUFFER
{
    //
    // WDF memory for the control block
    //
    WDFMEMORY ControlMemory;

    //
    // Pointer to the control block
    //
    PM487_RING_CONTROL Control;

    //
    // Slot size in bytes
    //
    ULONG SlotSize;

    //
    // Number of slots
    //
    ULONG SlotCount;

    //
    // Max data size per slot
    //
    USHORT MaxDataSize;

} M487_RING_BUFFER, *PM487_RING_BUFFER;

//
// Function declarations
//
NTSTATUS
M487RingBufferInitialize(
    _In_ WDFDEVICE Device,
    _Out_ PM487_RING_BUFFER RingBuffer
    );

VOID
M487RingBufferCleanup(
    _In_ PM487_RING_BUFFER RingBuffer
    );

PM487_RING_ENTRY
M487RingBufferAllocEntry(
    _In_ PM487_RING_BUFFER RingBuffer
    );

VOID
M487RingBufferCommitEntry(
    _In_ PM487_RING_BUFFER RingBuffer,
    _In_ PM487_RING_ENTRY Entry,
    _In_ USHORT DataLength
    );

VOID
M487RingBufferClear(
    _In_ PM487_RING_BUFFER RingBuffer
    );

ULONG
M487RingBufferGetStats(
    _In_ PM487_RING_BUFFER RingBuffer,
    _Out_ PULONG64 TotalCaptured,
    _Out_ PULONG DroppedCount,
    _Out_ PULONG CurrentUsed
    );

//
// Convert IOCTL code to M487_IOCTL_TYPE
//
M487_IOCTL_TYPE
M487IoctlToType(
    _In_ ULONG IoctlCode
    );

#endif // _M487_FILTER_RING_BUFFER_H
