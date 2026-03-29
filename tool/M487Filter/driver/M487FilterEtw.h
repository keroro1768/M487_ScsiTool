/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterEtw.h

Abstract:

    ETW (Event Tracing for Windows) instrumentation for M487 Filter Driver.
    Provides real-time event logging for IOCTL interception and ring buffer activity.

Environment:

    Kernel mode

--*/

#ifndef _M487_FILTER_ETW_H
#define _M487_FILTER_ETW_H

#include <evntrace.h>
#include <evntcons.h>

//
// ETW Provider GUID for M487Filter
// {A1B2C3D4-E5F6-4A5B-8C7D-9E0F1A2B3C4D}
//
DEFINE_GUID(
    M487FilterEtwProviderGuid,
    0xA1B2C3D4, 0xE5F6, 0x4A5B, 0x8C, 0x7D, 0x9E, 0x0F, 0x1A, 0x2B, 0x3C, 0x4D);

//
// ETW Enable Flags
//
#define M487FILTER_ETW_FLAG_IOCTL           0x00000001  // IOCTL interception events
#define M487FILTER_ETW_FLAG_RING_BUFFER     0x00000002  // Ring buffer operations
#define M487FILTER_ETW_FLAG_INTERCEPT       0x00000004  // Intercept config changes
#define M487FILTER_ETW_FLAG_ERROR            0x00000008  // Error conditions
#define M487FILTER_ETW_FLAG_WMI             0x00000010  // WMI operations
#define M487FILTER_ETW_FLAG_ALL             0xFFFFFFFF  // All events

//
// ETW Enable Level (verbosity)
//
#define M487FILTER_ETW_LEVEL_LOGICAL        1  // Functional events
#define M487FILTER_ETW_LEVEL_OPERATIONAL    2  // Normal operations
#define M487FILTER_ETW_LEVEL_VERBOSE        3  // Detailed debug
#define M487FILTER_ETW_LEVEL_INFORMATIONAL  4  // Info level
#define M487FILTER_ETW_LEVEL_DEBUG         5  // Debug only

//
// ETW Event IDs
//
typedef enum _M487FILTER_ETW_EVENT_ID
{
    //
    // IOCTL Events (1-99)
    //
    M487FILTER_ETW_EVENT_IOCTL_INTERCEPTED = 1,
    M487FILTER_ETW_EVENT_IOCTL_SET_FEATURE = 2,
    M487FILTER_ETW_EVENT_IOCTL_GET_FEATURE = 3,
    M487FILTER_ETW_EVENT_IOCTL_WRITE_REPORT = 4,
    M487FILTER_ETW_EVENT_IOCTL_READ_REPORT = 5,

    //
    // Ring Buffer Events (100-199)
    //
    M487FILTER_ETW_EVENT_RING_ENTRY_CAPTURED = 100,
    M487FILTER_ETW_EVENT_RING_BUFFER_FULL = 101,
    M487FILTER_ETW_EVENT_RING_ENTRY_READ = 102,
    M487FILTER_ETW_EVENT_RING_STATS = 103,

    //
    // Configuration Events (200-299)
    //
    M487FILTER_ETW_EVENT_CONFIG_CHANGED = 200,
    M487FILTER_ETW_EVENT_INTERCEPT_ENABLED = 201,
    M487FILTER_ETW_EVENT_INTERCEPT_DISABLED = 202,

    //
    // WMI Events (300-399)
    //
    M487FILTER_ETW_EVENT_WMI_QUERY = 300,
    M487FILTER_ETW_EVENT_WMI_SET = 301,

    //
    // Error Events (900-999)
    //
    M487FILTER_ETW_EVENT_ERROR_IOCTL_FORWARD = 900,
    M487FILTER_ETW_EVENT_ERROR_RING_FULL = 901,
    M487FILTER_ETW_EVENT_ERROR_DEVICE_LOST = 902,
    M487FILTER_ETW_EVENT_ERROR_WMI_FAILURE = 903

} M487FILTER_ETW_EVENT_ID;

//
// ETW Event Data Structures
//

//
// Event: IOCTL_INTERCEPTED
//
typedef struct _M487FILTER_ETW_EVENT_IOCTL
{
    ULONG IoctlCode;
    M487_IOCTL_TYPE IoctlType;
    USHORT DataLength;
    BOOLEAN IsWrite;
    UCHAR ReportId;
    UCHAR Reserved[3];

} M487FILTER_ETW_EVENT_IOCTL, *PM487FILTER_ETW_EVENT_IOCTL;

//
// Event: RING_ENTRY_CAPTURED
//
typedef struct _M487FILTER_ETW_EVENT_RING
{
    ULONG64 TotalCaptured;
    ULONG DroppedCount;
    USHORT DataLength;
    UCHAR SlotIndex;
    UCHAR Reserved;

} M487FILTER_ETW_EVENT_RING, *PM487FILTER_ETW_EVENT_RING;

//
// Event: CONFIG_CHANGED
//
typedef struct _M487FILTER_ETW_EVENT_CONFIG
{
    BOOLEAN CaptureSetFeature;
    BOOLEAN CaptureGetFeature;
    BOOLEAN CaptureWriteReport;
    BOOLEAN CaptureReadReport;

} M487FILTER_ETW_EVENT_CONFIG, *PM487FILTER_ETW_EVENT_CONFIG;

//
// Event: ERROR
//
typedef struct _M487FILTER_ETW_EVENT_ERROR
{
    NTSTATUS NtStatus;
    ULONG IoctlCode;
    ULONG ExtraInfo;

} M487FILTER_ETW_EVENT_ERROR, *PM487FILTER_ETW_EVENT_ERROR;

//
// ETW Control GUID - for session control
// {B2C3D4E5-F6A7-4B5C-9D8E-0F1A2B3C4D5E}
//
DEFINE_GUID(
    M487FilterEtwControlGuid,
    0xB2C3D4E5, 0xF6A7, 0x4B5C, 0x9D, 0x8E, 0x0F, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E);

//
// ETW Trace Enable Callback Flags
//
typedef struct _M487FILTER_ETW_ENABLE_INFO
{
    BOOLEAN Enabled;
    UCHAR EnableLevel;
    UCHAR Reserved[2];
    ULONG EnableFlags;

} M487FILTER_ETW_ENABLE_INFO, *PM487FILTER_ETW_ENABLE_INFO;

//
// Function declarations
//
NTSTATUS
M487FilterEtwInitialize(
    VOID
    );

VOID
M487FilterEtwUninitialize(
    VOID
    );

//
// ETW Event Logging Macros
//

//
// Check if ETW is enabled at a given level and flag
//
#define M487FILTER_ETW_IS_ENABLED(Level, Flag) \
    (M487FilterEtwEnableInfo.Enabled && \
     (M487FilterEtwEnableInfo.EnableLevel >= (Level)) && \
     ((M487FilterEtwEnableInfo.EnableFlags & (Flag)) != 0))

//
// Log IOCTL interception event
//
#define M487FILTER_ETW_LOG_IOCTL(IoctlCode, Type, DataLen, IsWrite) \
    do { \
        if (M487FILTER_ETW_IS_ENABLED(M487FILTER_ETW_LEVEL_LOGICAL, M487FILTER_ETW_FLAG_IOCTL)) { \
            M487FILTER_ETW_EVENT_IOCTL eventData; \
            eventData.IoctlCode = (IoctlCode); \
            eventData.IoctlType = (Type); \
            eventData.DataLength = (DataLen); \
            eventData.IsWrite = (IsWrite); \
            eventData.ReportId = 0; \
            EventWriteIOCTL(&eventData); \
        } \
    } while (FALSE)

//
// Log ring buffer event
//
#define M487FILTER_ETW_LOG_RING(TotalCap, Dropped, DataLen, SlotIdx) \
    do { \
        if (M487FILTER_ETW_IS_ENABLED(M487FILTER_ETW_LEVEL_LOGICAL, M487FILTER_ETW_FLAG_RING_BUFFER)) { \
            M487FILTER_ETW_EVENT_RING eventData; \
            eventData.TotalCaptured = (TotalCap); \
            eventData.DroppedCount = (Dropped); \
            eventData.DataLength = (DataLen); \
            eventData.SlotIndex = (SlotIdx); \
            EventWriteRingBuffer(&eventData); \
        } \
    } while (FALSE)

//
// Log config change
//
#define M487FILTER_ETW_LOG_CONFIG(SetF, GetF, WriteR, ReadR) \
    do { \
        if (M487FILTER_ETW_IS_ENABLED(M487FILTER_ETW_LEVEL_LOGICAL, M487FILTER_ETW_FLAG_INTERCEPT)) { \
            M487FILTER_ETW_EVENT_CONFIG eventData; \
            eventData.CaptureSetFeature = (SetF); \
            eventData.CaptureGetFeature = (GetF); \
            eventData.CaptureWriteReport = (WriteR); \
            eventData.CaptureReadReport = (ReadR); \
            EventWriteConfig(&eventData); \
        } \
    } while (FALSE)

//
// Log error event
//
#define M487FILTER_ETW_LOG_ERROR(Status, Ioctl, Extra) \
    do { \
        if (M487FILTER_ETW_IS_ENABLED(M487FILTER_ETW_LEVEL_LOGICAL, M487FILTER_ETW_FLAG_ERROR)) { \
            M487FILTER_ETW_EVENT_ERROR eventData; \
            eventData.NtStatus = (Status); \
            eventData.IoctlCode = (Ioctl); \
            eventData.ExtraInfo = (Extra); \
            EventWriteError(&eventData); \
        } \
    } while (FALSE)

//
// Internal functions (called by macros)
// Must be implemented in M487FilterEtw.c
//
VOID
EventWriteIOCTL(
    _In_ PM487FILTER_ETW_EVENT_IOCTL EventData
    );

VOID
EventWriteRingBuffer(
    _In_ PM487FILTER_ETW_EVENT_RING EventData
    );

VOID
EventWriteConfig(
    _In_ PM487FILTER_ETW_EVENT_CONFIG EventData
    );

VOID
EventWriteError(
    _In_ PM487FILTER_ETW_EVENT_ERROR EventData
    );

//
// ETW enable info (updated by callback)
//
extern M487FILTER_ETW_ENABLE_INFO M487FilterEtwEnableInfo;

//
// ETW registration handle
//
extern REGHANDLE M487FilterEtwRegistrationHandle;

//
// ETW Control Callback
//
VOID
M487FilterEtwEnableCallback(
    _In_ LPCGUID SourceId,
    _In_ ULONG IsEnabled,
    _In_ UCHAR Level,
    _In_ ULONGLONG MatchAnyKeyword,
    _In_ ULONGLONG MatchAllKeyword,
    _In_ PEVENT_FILTER_DESCRIPTOR FilterData,
    _In_ PVOID CallbackContext
    );

#endif // _M487_FILTER_ETW_H
