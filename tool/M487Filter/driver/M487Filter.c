/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487Filter.c

Abstract:

    Driver entry point for M487 HID Upper Filter Driver.
    Based on Microsoft Firefly sample.

Environment:

    Kernel mode

--*/

#include "M487Filter.h"

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("M487Filter: DriverEntry - built on %s %s\n",
             __DATE__, __TIME__));

    WDF_DRIVER_CONFIG_INIT(
        &config,
        M487FilterEvtDeviceAdd
    );

    //
    // Create the framework WDFDRIVER object.
    // Framework will automatically cleanup on error.
    //
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: WdfDriverCreate failed 0x%x\n", status));
    }

    return status;
}
