/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterIntercept.h

Abstract:

    HID IOCTL interception for M487 HID Upper Filter Driver.
    Handles IOCTL_HID_SET_FEATURE, IOCTL_HID_GET_FEATURE,
    IOCTL_HID_READ_REPORT, IOCTL_HID_WRITE_REPORT.

Environment:

    Kernel mode

--*/

#ifndef _M487_FILTER_INTERCEPT_H
#define _M487_FILTER_INTERCEPT_H

//
// Intercept configuration flags
//
typedef struct _M487_INTERCEPT_CONFIG
{
    //
    // Capture SET_FEATURE (host → device)
    //
    BOOLEAN CaptureSetFeature;

    //
    // Capture GET_FEATURE (host ← device)
    //
    BOOLEAN CaptureGetFeature;

    //
    // Capture WRITE_REPORT (host → device)
    //
    BOOLEAN CaptureWriteReport;

    //
    // Capture READ_REPORT (host ← device)
    //
    BOOLEAN CaptureReadReport;

    //
    // Capture all HID string requests
    //
    BOOLEAN CaptureStringRequests;

    //
    // Capture collection info/descriptor
    //
    BOOLEAN CaptureCollectionInfo;

    //
    // Reserved for future flags
    //
    ULONG Reserved[3];

} M487_INTERCEPT_CONFIG, *PM487_INTERCEPT_CONFIG;

//
// Per-device interception state
//
typedef struct _M487_INTERCEPT_STATE
{
    //
    // Ring buffer for captured data
    //
    M487_RING_BUFFER RingBuffer;

    //
    // Current interception configuration
    //
    M487_INTERCEPT_CONFIG Config;

    //
    // Spinlock to protect config changes
    //
    KSPIN_LOCK ConfigLock;

    //
    // Whether interception has been initialized
    //
    BOOLEAN Initialized;

    //
    // Shared section handle for user-mode ring buffer access
    //
    HANDLE RingBufferSectionHandle;

    //
    // User-mode virtual address of the mapped ring buffer
    //
    PVOID RingBufferUserVa;

} M487_INTERCEPT_STATE, *PM487_INTERCEPT_STATE;

//
// Function declarations
//
NTSTATUS
M487InterceptInitialize(
    _In_ WDFDEVICE Device,
    _In_ PDEVICE_CONTEXT DevContext
    );

VOID
M487InterceptCleanup(
    _In_ PDEVICE_CONTEXT DevContext
    );

//
// Capture functions - called by interception handlers
//
VOID
M487CaptureSetFeature(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength
    );

VOID
M487CaptureGetFeature(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength,
    _In_ BOOLEAN FromDevice
    );

VOID
M487CaptureWriteReport(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength
    );

VOID
M487CaptureReadReport(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ size_t DataLength
    );

//
// Capture an arbitrary IOCTL
//
VOID
M487CaptureIoctl(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ ULONG IoctlCode,
    _In_reads_bytes_opt_(DataLength) PVOID Data,
    _In_ size_t DataLength,
    _In_ BOOLEAN IsWrite
    );

//
// Register to intercept specific HID IOCTLs from the default queue.
// This tells KMDF to dispatch these IOCTLs to our EvtIoDeviceControl.
//
NTSTATUS
M487InterceptRegisterIoctls(
    _In_ WDFDEVICE Device
    );

//
// Forward HID IOCTL synchronously to lower device and capture result
//
NTSTATUS
M487InterceptForwardIoctl(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoctlCode,
    _In_ WDFMEMORY InputMemory,
    _In_ WDFMEMORY OutputMemory,
    _In_ BOOLEAN IsWriteOperation
    );

//
// Get/Set interception configuration
//
VOID
M487InterceptGetConfig(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PM487_INTERCEPT_CONFIG Config
    );

VOID
M487InterceptSetConfig(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_ PM487_INTERCEPT_CONFIG Config
    );

//
// Get ring buffer mapping info for user-mode access
//
NTSTATUS
M487InterceptGetRingBufferInfo(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PULONG64 RingBufferPhysAddr,
    _Out_ PULONG RingBufferSize
    );

//
// Get/clear captured entries
//
NTSTATUS
M487InterceptGetStats(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PULONG64 TotalCaptured,
    _Out_ PULONG DroppedCount,
    _Out_ PULONG CurrentUsed
    );

//
// Map ring buffer to user-mode address space and return mapping info.
// On first call, creates a section object and maps it.
// Subsequent calls return the cached mapping.
// Returns: User-mode VA, ring buffer size, slot info.
//
NTSTATUS
M487InterceptMapRingBufferToUser(
    _In_ PDEVICE_CONTEXT DevContext,
    _Out_ PULONG64 UserVa,
    _Out_ PULONG RingBufferSize,
    _Out_ PULONG SlotSize,
    _Out_ PULONG SlotCount,
    _Out_ PULONG MaxDataSize
    );

#endif // _M487_FILTER_INTERCEPT_H
