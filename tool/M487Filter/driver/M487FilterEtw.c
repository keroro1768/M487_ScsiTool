/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterEtw.c

Abstract:

    ETW (Event Tracing for Windows) implementation for M487 Filter Driver.
    Provides real-time event logging capability.

Environment:

    Kernel mode

--*/

#include "M487Filter.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, M487FilterEtwInitialize)
#pragma alloc_text(PAGE, M487FilterEtwUninitialize)
#endif

//
// Global ETW registration handle
//
REGHANDLE M487FilterEtwRegistrationHandle = 0;

//
// Global ETW enable info (updated by callback)
//
M487FILTER_ETW_ENABLE_INFO M487FilterEtwEnableInfo = {
    FALSE,                     // Enabled
    M487FILTER_ETW_LEVEL_LOGICAL, // EnableLevel
    {0},                       // Reserved
    0                          // EnableFlags
};

//
// Forward declarations for paged code
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, EventWriteIOCTL)
#pragma alloc_text(PAGE, EventWriteRingBuffer)
#pragma alloc_text(PAGE, EventWriteConfig)
#pragma alloc_text(PAGE, EventWriteError)
#pragma alloc_text(PAGE, M487FilterEtwEnableCallback)
#endif

//
// M487FilterEtwInitialize - Register ETW provider
//
NTSTATUS
M487FilterEtwInitialize(
    VOID
    )
{
    NTSTATUS status;

    PAGED_CODE();

    KdPrint(("M487Filter: M487FilterEtwInitialize\n"));

    //
    // Register this driver as an ETW provider
    //
    status = EventRegister(
        &M487FilterEtwProviderGuid,
        M487FilterEtwEnableCallback,
        NULL,  // CallbackContext
        &M487FilterEtwRegistrationHandle);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: EventRegister failed 0x%x\n", status));
        M487FilterEtwRegistrationHandle = 0;
        return status;
    }

    KdPrint(("M487Filter: ETW provider registered, handle=0x%p\n",
             M487FilterEtwRegistrationHandle));

    return STATUS_SUCCESS;
}

//
// M487FilterEtwUninitialize - Unregister ETW provider
//
VOID
M487FilterEtwUninitialize(
    VOID
    )
{
    PAGED_CODE();

    KdPrint(("M487Filter: M487FilterEtwUninitialize\n"));

    if (M487FilterEtwRegistrationHandle != 0) {
        EventUnregister(M487FilterEtwRegistrationHandle);
        M487FilterEtwRegistrationHandle = 0;
        M487FilterEtwEnableInfo.Enabled = FALSE;
    }
}

//
// M487FilterEtwEnableCallback - Called when ETW session enables/disables provider
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
    )
{
    UNREFERENCED_PARAMETER(SourceId);
    UNREFERENCED_PARAMETER(FilterData);
    UNREFERENCED_PARAMETER(CallbackContext);
    UNREFERENCED_PARAMETER(MatchAllKeyword);

    PAGED_CODE();

    //
    // Update enable info based on session settings
    //
    M487FilterEtwEnableInfo.Enabled = (IsEnabled != 0);
    M487FilterEtwEnableInfo.EnableLevel = Level;
    M487FilterEtwEnableInfo.EnableFlags = (ULONG)(MatchAnyKeyword & 0xFFFFFFFF);

    KdPrint(("M487Filter: ETW EnableCallback - Enabled=%d, Level=%u, Flags=0x%08X\n",
             IsEnabled, Level, M487FilterEtwEnableInfo.EnableFlags));
}

//
// EventWriteIOCTL - Log IOCTL interception event
//
VOID
EventWriteIOCTL(
    _In_ PM487FILTER_ETW_EVENT_IOCTL EventData
    )
{
    EVENT_DATA_DESCRIPTOR eventDataDescriptor[1];
    NTSTATUS status;

    PAGED_CODE();

    if (M487FilterEtwRegistrationHandle == 0) {
        return;
    }

    //
    // Create event data descriptor
    //
    EventDataDescCreate(
        &eventDataDescriptor[0],
        EventData,
        sizeof(M487FILTER_ETW_EVENT_IOCTL));

    //
    // Write the event
    //
    status = EventWrite(
        M487FilterEtwRegistrationHandle,
        NULL,  // EventDescriptor (use manifest-defined)
        1,     // UserDataCount
        eventDataDescriptor);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: EventWriteIOCTL failed 0x%x\n", status));
    }
}

//
// EventWriteRingBuffer - Log ring buffer event
//
VOID
EventWriteRingBuffer(
    _In_ PM487FILTER_ETW_EVENT_RING EventData
    )
{
    EVENT_DATA_DESCRIPTOR eventDataDescriptor[1];
    NTSTATUS status;

    PAGED_CODE();

    if (M487FilterEtwRegistrationHandle == 0) {
        return;
    }

    EventDataDescCreate(
        &eventDataDescriptor[0],
        EventData,
        sizeof(M487FILTER_ETW_EVENT_RING));

    status = EventWrite(
        M487FilterEtwRegistrationHandle,
        NULL,
        1,
        eventDataDescriptor);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: EventWriteRingBuffer failed 0x%x\n", status));
    }
}

//
// EventWriteConfig - Log configuration change event
//
VOID
EventWriteConfig(
    _In_ PM487FILTER_ETW_EVENT_CONFIG EventData
    )
{
    EVENT_DATA_DESCRIPTOR eventDataDescriptor[1];
    NTSTATUS status;

    PAGED_CODE();

    if (M487FilterEtwRegistrationHandle == 0) {
        return;
    }

    EventDataDescCreate(
        &eventDataDescriptor[0],
        EventData,
        sizeof(M487FILTER_ETW_EVENT_CONFIG));

    status = EventWrite(
        M487FilterEtwRegistrationHandle,
        NULL,
        1,
        eventDataDescriptor);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: EventWriteConfig failed 0x%x\n", status));
    }
}

//
// EventWriteError - Log error event
//
VOID
EventWriteError(
    _In_ PM487FILTER_ETW_EVENT_ERROR EventData
    )
{
    EVENT_DATA_DESCRIPTOR eventDataDescriptor[1];
    NTSTATUS status;

    PAGED_CODE();

    if (M487FilterEtwRegistrationHandle == 0) {
        return;
    }

    EventDataDescCreate(
        &eventDataDescriptor[0],
        EventData,
        sizeof(M487FILTER_ETW_EVENT_ERROR));

    status = EventWrite(
        M487FilterEtwRegistrationHandle,
        NULL,
        1,
        eventDataDescriptor);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: EventWriteError failed 0x%x\n", status));
    }
}
