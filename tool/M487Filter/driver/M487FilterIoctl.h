/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterIoctl.h

Abstract:

    Custom IOCTL definitions and handlers for M487 HID Upper Filter Driver.

Environment:

    Kernel mode

--*/

#ifndef _M487_FILTER_IOCTL_H
#define _M487_FILTER_IOCTL_H

//
// Custom IOCTLs for M487Filter (device type = FILE_DEVICE_UNKNOWN)
// These IOCTLs allow user-mode applications to communicate with the filter.
//

#define IOCTL_M487FILTER_BASE      0x9000

#define IOCTL_M487FILTER_GET_INFO   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                             IOCTL_M487FILTER_BASE + 0, \
                                             METHOD_BUFFERED,        \
                                             FILE_ANY_ACCESS)

#define IOCTL_M487FILTER_SET_MODE  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                             IOCTL_M487FILTER_BASE + 1, \
                                             METHOD_BUFFERED,        \
                                             FILE_ANY_ACCESS)

#define IOCTL_M487FILTER_SEND_FEATURE  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  IOCTL_M487FILTER_BASE + 2, \
                                                  METHOD_BUFFERED,        \
                                                  FILE_ANY_ACCESS)

#define IOCTL_M487FILTER_GET_FEATURE   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  IOCTL_M487FILTER_BASE + 3, \
                                                  METHOD_BUFFERED,        \
                                                  FILE_ANY_ACCESS)

//
// Get ring buffer statistics
//
#define IOCTL_M487FILTER_GET_RING_STATS CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  IOCTL_M487FILTER_BASE + 4, \
                                                  METHOD_BUFFERED,        \
                                                  FILE_ANY_ACCESS)

//
// Get physical address of ring buffer for user-mode mapping
//
#define IOCTL_M487FILTER_GET_RING_ADDR  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  IOCTL_M487FILTER_BASE + 5, \
                                                  METHOD_BUFFERED,        \
                                                  FILE_ANY_ACCESS)

//
// Set interception configuration
//
#define IOCTL_M487FILTER_SET_INTERCEPT_CONFIG CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  IOCTL_M487FILTER_BASE + 6, \
                                                  METHOD_BUFFERED,        \
                                                  FILE_ANY_ACCESS)

//
// Get interception configuration
//
#define IOCTL_M487FILTER_GET_INTERCEPT_CONFIG CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  IOCTL_M487FILTER_BASE + 7, \
                                                  METHOD_BUFFERED,        \
                                                  FILE_ANY_ACCESS)

//
// Map ring buffer into user-mode address space.
// Returns virtual address and metadata for reading captured entries.
//
#define IOCTL_M487FILTER_MAP_RING_BUFFER   CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                  IOCTL_M487FILTER_BASE + 8, \
                                                  METHOD_BUFFERED,        \
                                                  FILE_ANY_ACCESS)

//
// Ring buffer mapping info returned by IOCTL_M487FILTER_MAP_RING_BUFFER
//
typedef struct _M487_FILTER_RING_MAPPING_INFO
{
    //
    // User-mode virtual address of the mapped ring buffer
    //
    ULONG64 RingBufferUserVa;

    //
    // Total byte size of the ring buffer
    //
    ULONG RingBufferSize;

    //
    // Size of each slot in bytes
    //
    ULONG SlotSize;

    //
    // Number of slots in the ring buffer
    //
    ULONG SlotCount;

    //
    // Maximum data bytes per slot
    //
    USHORT MaxDataSize;

    //
    // Reserved/padding
    //
    USHORT Reserved;

} M487_FILTER_RING_MAPPING_INFO, *PM487_FILTER_RING_MAPPING_INFO;

//
// Filter operating modes
//
typedef enum _M487_FILTER_MODE
{
    FilterMode_PassThrough = 0,    // Forward all HID reports unchanged
    FilterMode_Blocking = 1,       // Block certain HID reports
    FilterMode_Modifying = 2       // Modify HID report data

} M487_FILTER_MODE, *PM487_FILTER_MODE;

//
// Device info structure returned by IOCTL_M487FILTER_GET_INFO
//
typedef struct _M487FILTER_INFO
{
    ULONG Version;
    ULONG FilterMode;
    ULONG InputReportByteLength;
    ULONG OutputReportByteLength;
    ULONG FeatureReportByteLength;
    BOOLEAN LowerDeviceOpened;

    //
    // Interception stats
    //
    ULONG64 TotalCaptured;
    ULONG DroppedCount;
    ULONG RingBufferSlotsUsed;

} M487FILTER_INFO, *PM487FILTER_INFO;

//
// Function declarations
//
NTSTATUS
M487FilterEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    );

VOID
M487FilterEvtIoDefault(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
    );

#endif // _M487_FILTER_IOCTL_H
