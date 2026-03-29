/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterDevice.c

Abstract:

    Device object handling for M487 HID Upper Filter Driver.
    - FireFlyEvtDeviceAdd
    - PDO name acquisition
    - Lower IO target setup

Environment:

    Kernel mode

--*/

#include "M487Filter.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, M487FilterEvtDeviceAdd)
#pragma alloc_text(PAGE, M487FilterEvtDeviceContextCleanup)
#endif

NTSTATUS
M487FilterEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _In_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Called by framework in response to AddDevice from PnP manager.
    Creates and initializes device object as an upper filter in the stack.

Arguments:

    Driver - Handle to framework driver object created in DriverEntry

    DeviceInit - Pointer to framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;
    PDEVICE_CONTEXT devContext;
    WDFDEVICE device;
    WDFMEMORY memory;
    size_t bufferLength;

    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();

    KdPrint(("M487Filter: M487FilterEvtDeviceAdd called\n"));

    //
    // ★ KEY STEP: Register as an upper filter driver.
    // KMDF will automatically handle IRP pass-through for PnP/Power/IO.
    // No manual IRP dispatch table needed!
    //
    WdfFdoInitSetFilter(DeviceInit);

    //
    // Set device I/O type to Direct for IOCTL passthrough
    //
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

    //
    // Initialize device context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: WdfDeviceCreate failed 0x%x\n", status));
        return status;
    }

    //
    // Framework zero-initializes context memory
    //
    devContext = WdfObjectGet_DEVICE_CONTEXT(device);

    //
    // Initialize ETW tracing (registration only, no events until device starts)
    //
    status = M487FilterEtwInitialize();
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: M487FilterEtwInitialize failed 0x%x\n", status));
        // Non-fatal: continue without ETW
    }

    //
    // Initialize WMI support
    //
    status = M487FilterWmiInitialize(device, devContext);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: M487FilterWmiInitialize failed 0x%x\n", status));
        return status;
    }

    //
    // Acquire PDO device name for opening lower IO target.
    // This is required to send IOCTLs to the underlying HID device.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfDeviceAllocAndQueryProperty(
        device,
        DevicePropertyPhysicalDeviceObjectName,
        NonPagedPoolNx,
        &attributes,
        &memory);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: WdfDeviceAllocAndQueryProperty failed 0x%x\n", status));
        return status;
    }

    devContext->PdoName.Buffer = WdfMemoryGetBuffer(memory, &bufferLength);
    if (devContext->PdoName.Buffer == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    devContext->PdoName.MaximumLength = (USHORT)bufferLength;
    devContext->PdoName.Length = (USHORT)(bufferLength - sizeof(UNICODE_NULL));

    KdPrint(("M487Filter: PDO name obtained, length=%u\n",
             devContext->PdoName.Length));

    //
    // Open lower HID device as IO Target
    //
    status = M487FilterOpenLowerDevice(device, devContext);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: M487FilterOpenLowerDevice failed 0x%x\n", status));
        return status;
    }

    //
    // Query HID capabilities for future IOCTL operations
    //
    status = M487FilterQueryHIDCapabilities(device, devContext);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: M487FilterQueryHIDCapabilities failed 0x%x\n", status));
        // Non-fatal: we can still operate in pass-through mode
    }

    //
    // Initialize IOCTL interception subsystem (ring buffer, etc.)
    //
    status = M487InterceptInitialize(device, devContext);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: M487InterceptInitialize failed 0x%x\n", status));
        // Non-fatal: continue without interception
    }

    //
    // Register to intercept specific HID IOCTLs before they pass through.
    // By registering EvtIoDeviceControl, we can capture and optionally
    // modify HID IOCTLs before forwarding them to the lower device.
    //
    status = M487InterceptRegisterIoctls(device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: M487InterceptRegisterIoctls failed 0x%x\n", status));
        // Non-fatal: automatic pass-through will still work
    }

    KdPrint(("M487Filter: Device add complete, status=0x%x\n", status));
    return status;
}

VOID
M487FilterEvtDeviceContextCleanup(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

    Cleanup callback when device is being removed.
    Free any allocated resources.

Arguments:

    Device - Framework device object

Return Value:

    None

--*/
{
    PDEVICE_CONTEXT devContext;

    PAGED_CODE();

    KdPrint(("M487Filter: EvtDeviceContextCleanup\n"));

    devContext = WdfObjectGet_DEVICE_CONTEXT(Device);

    //
    // Free preparsed data if allocated
    //
    if (devContext->PreparsedData != NULL) {
        ExFreePool(devContext->PreparsedData);
        devContext->PreparsedData = NULL;
    }

    //
    // Cleanup interception subsystem
    //
    M487InterceptCleanup(devContext);

    //
    // Uninitialize ETW tracing
    //
    M487FilterEtwUninitialize();
}

//
// M487FilterOpenLowerDevice - Open the underlying HID device as IO Target
//
_Must_inspect_result_
NTSTATUS
M487FilterOpenLowerDevice(
    _In_ WDFDEVICE Device,
    _In_ PDEVICE_CONTEXT DevContext
    )
{
    WDF_IO_TARGET_OPEN_PARAMS openParams;
    NTSTATUS status;

    PAGED_CODE();

    status = WdfIoTargetCreate(
        Device,
        WDF_NO_OBJECT_ATTRIBUTES,
        &DevContext->LowerHidTarget);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: WdfIoTargetCreate failed 0x%x\n", status));
        return status;
    }

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        &DevContext->PdoName,
        FILE_WRITE_ACCESS);  // Write access for sending IOCTLs

    openParams.ShareAccess = FILE_SHARE_WRITE | FILE_SHARE_READ;

    //
    // Framework automatically handles target open/close on PnP state changes
    //
    status = WdfIoTargetOpen(DevContext->LowerHidTarget, &openParams);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: WdfIoTargetOpen failed 0x%x\n", status));
        return status;
    }

    KdPrint(("M487Filter: Lower HID device opened successfully\n"));
    return STATUS_SUCCESS;
}

//
// M487FilterQueryHIDCapabilities - Query HID device capabilities
//
_Must_inspect_result_
NTSTATUS
M487FilterQueryHIDCapabilities(
    _In_ WDFDEVICE Device,
    _In_ PDEVICE_CONTEXT DevContext
    )
{
    WDF_MEMORY_DESCRIPTOR outputDescriptor;
    HID_COLLECTION_INFORMATION collectionInfo = {0};
    NTSTATUS status;

    PAGED_CODE();

    //
    // Step 1: Get collection information
    //
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outputDescriptor,
        &collectionInfo,
        sizeof(HID_COLLECTION_INFORMATION));

    status = WdfIoTargetSendIoctlSynchronously(
        DevContext->LowerHidTarget,
        NULL,
        IOCTL_HID_GET_COLLECTION_INFORMATION,
        NULL,
        &outputDescriptor,
        NULL,
        NULL);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: IOCTL_HID_GET_COLLECTION_INFORMATION failed 0x%x\n", status));
        return status;
    }

    KdPrint(("M487Filter: HID DescriptorSize=%lu\n", collectionInfo.DescriptorSize));

    //
    // Step 2: Allocate and get collection descriptor (Preparsed Data)
    //
    DevContext->PreparsedData = (PHIDP_PREPARSED_DATA)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        collectionInfo.DescriptorSize,
        'M487');

    if (DevContext->PreparsedData == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &outputDescriptor,
        DevContext->PreparsedData,
        collectionInfo.DescriptorSize);

    status = WdfIoTargetSendIoctlSynchronously(
        DevContext->LowerHidTarget,
        NULL,
        IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
        NULL,
        &outputDescriptor,
        NULL,
        NULL);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: IOCTL_HID_GET_COLLECTION_DESCRIPTOR failed 0x%x\n", status));
        ExFreePool(DevContext->PreparsedData);
        DevContext->PreparsedData = NULL;
        return status;
    }

    //
    // Step 3: Get HID capabilities from preparsed data
    //
    RtlZeroMemory(&DevContext->HidCaps, sizeof(HIDP_CAPS));
    status = HidP_GetCaps(DevContext->PreparsedData, &DevContext->HidCaps);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: HidP_GetCaps failed 0x%x\n", status));
        ExFreePool(DevContext->PreparsedData);
        DevContext->PreparsedData = NULL;
        return status;
    }

    KdPrint(("M487Filter: HID Capabilities:\n"));
    KdPrint(("  InputReportByteLength=%u\n", DevContext->HidCaps.InputReportByteLength));
    KdPrint(("  OutputReportByteLength=%u\n", DevContext->HidCaps.OutputReportByteLength));
    KdPrint(("  FeatureReportByteLength=%u\n", DevContext->HidCaps.FeatureReportByteLength));
    KdPrint(("  UsagePage=0x%02X\n", DevContext->HidCaps.UsagePage));
    KdPrint(("  Usage=0x%02X\n", DevContext->HidCaps.Usage));

    return STATUS_SUCCESS;
}
