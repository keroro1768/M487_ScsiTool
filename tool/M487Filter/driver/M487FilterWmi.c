/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterWmi.c

Abstract:

    WMI support implementation for M487 HID Upper Filter Driver.
    Provides WMI interfaces for ring buffer access and configuration.

Environment:

    Kernel mode

--*/

#include "M487Filter.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, M487FilterWmiInitialize)
#pragma alloc_text(PAGE, EvtWmiInstanceQueryInstance)
#pragma alloc_text(PAGE, EvtWmiInstanceSetInstance)
#pragma alloc_text(PAGE, EvtWmiInstanceSetItem)
#pragma alloc_text(PAGE, EvtRingBufferQueryInstance)
#pragma alloc_text(PAGE, EvtRingBufferSetInstance)
#pragma alloc_text(PAGE, EvtInterceptConfigQueryInstance)
#pragma alloc_text(PAGE, EvtInterceptConfigSetInstance)
#pragma alloc_text(PAGE, EvtInterceptConfigSetItem)
#endif

//
// Context structure for Ring Buffer WMI instance
//
typedef struct _RING_BUFFER_WMI_CONTEXT
{
    PDEVICE_CONTEXT DeviceContext;

} RING_BUFFER_WMI_CONTEXT, *PRING_BUFFER_WMI_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(RING_BUFFER_WMI_CONTEXT, RingBufferContextGet)

//
// Context structure for Intercept Config WMI instance
//
typedef struct _INTERCEPT_CONFIG_WMI_CONTEXT
{
    PDEVICE_CONTEXT DeviceContext;

} INTERCEPT_CONFIG_WMI_CONTEXT, *PINTERCEPT_CONFIG_WMI_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INTERCEPT_CONFIG_WMI_CONTEXT, InterceptConfigContextGet)

//
// M487FilterWmiInitialize - Register all WMI providers
//
NTSTATUS
M487FilterWmiInitialize(
    _In_ WDFDEVICE Device,
    _In_ PDEVICE_CONTEXT DeviceContext
    )
{
    WDF_WMI_PROVIDER_CONFIG providerConfig;
    WDF_WMI_INSTANCE_CONFIG instanceConfig;
    WDF_OBJECT_ATTRIBUTES woa;
    WDFWMIINSTANCE instance;
    NTSTATUS status;
    DECLARE_CONST_UNICODE_STRING(mofRsrcName, MFRESOURCENAME);

    PAGED_CODE();

    KdPrint(("M487Filter: M487FilterWmiInitialize\n"));

    //
    // Assign MOF resource name to the device
    //
    status = WdfDeviceAssignMofResourceName(Device, &mofRsrcName);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: WdfDeviceAssignMofResourceName failed 0x%x\n", status));
        M487FILTER_ETW_LOG_ERROR(status, 0, 'MWMI');
        return status;
    }

    //
    // ===== Register WMI provider for M487FilterDeviceInformation =====
    //
    WDF_WMI_PROVIDER_CONFIG_INIT(
        &providerConfig,
        &M487FilterDeviceInformation_GUID);

    providerConfig.MinInstanceBufferSize = sizeof(M487FilterDeviceInformation);

    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);
    instanceConfig.Register = TRUE;
    instanceConfig.EvtWmiInstanceQueryInstance = EvtWmiInstanceQueryInstance;
    instanceConfig.EvtWmiInstanceSetInstance   = EvtWmiInstanceSetInstance;
    instanceConfig.EvtWmiInstanceSetItem       = EvtWmiInstanceSetItem;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&woa, M487FilterDeviceInformation);

    status = WdfWmiInstanceCreate(Device, &instanceConfig, &woa, &instance);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: Create DeviceInfo WMI instance failed 0x%x\n", status));
        M487FILTER_ETW_LOG_ERROR(status, 0, 'WMI1');
        return status;
    }

    //
    // Initialize default state
    //
    {
        M487FilterDeviceInformation* info = InstanceGetInfo(instance);
        info->PassThroughEnabled = TRUE;
        info->Active = TRUE;
        info->CaptureEnabled = TRUE;
    }

    //
    // ===== Register WMI provider for M487FilterRingBufferInfo =====
    //
    WDF_WMI_PROVIDER_CONFIG_INIT(
        &providerConfig,
        &M487FilterRingBufferInfo_GUID);

    providerConfig.MinInstanceBufferSize = sizeof(M487FilterRingBufferInfo);

    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);
    instanceConfig.Register = TRUE;
    instanceConfig.EvtWmiInstanceQueryInstance = EvtRingBufferQueryInstance;
    instanceConfig.EvtWmiInstanceSetInstance   = EvtRingBufferSetInstance;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&woa, RING_BUFFER_WMI_CONTEXT);

    status = WdfWmiInstanceCreate(Device, &instanceConfig, &woa, &instance);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: Create RingBuffer WMI instance failed 0x%x\n", status));
        M487FILTER_ETW_LOG_ERROR(status, 0, 'WMI2');
        return status;
    }

    //
    // Set context with device context pointer
    //
    {
        PRING_BUFFER_WMI_CONTEXT ctx = RingBufferContextGet(instance);
        ctx->DeviceContext = DeviceContext;
    }

    //
    // ===== Register WMI provider for M487FilterInterceptConfigWmi =====
    //
    WDF_WMI_PROVIDER_CONFIG_INIT(
        &providerConfig,
        &M487FilterInterceptConfigWmi_GUID);

    providerConfig.MinInstanceBufferSize = sizeof(M487FilterInterceptConfigWmi);

    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);
    instanceConfig.Register = TRUE;
    instanceConfig.EvtWmiInstanceQueryInstance = EvtInterceptConfigQueryInstance;
    instanceConfig.EvtWmiInstanceSetInstance   = EvtInterceptConfigSetInstance;
    instanceConfig.EvtWmiInstanceSetItem       = EvtInterceptConfigSetItem;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&woa, INTERCEPT_CONFIG_WMI_CONTEXT);

    status = WdfWmiInstanceCreate(Device, &instanceConfig, &woa, &instance);
    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: Create InterceptConfig WMI instance failed 0x%x\n", status));
        M487FILTER_ETW_LOG_ERROR(status, 0, 'WMI3');
        return status;
    }

    {
        PINTERCEPT_CONFIG_WMI_CONTEXT ctx = InterceptConfigContextGet(instance);
        ctx->DeviceContext = DeviceContext;
    }

    KdPrint(("M487Filter: WMI initialized with 3 providers, status=0x%x\n", status));
    M487FILTER_ETW_LOG_CONFIG(TRUE, TRUE, TRUE, TRUE);

    return status;
}

//
// EvtWmiInstanceQueryInstance - Query M487FilterDeviceInformation
//
NTSTATUS
EvtWmiInstanceQueryInstance(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG OutBufferSize,
    _In_ PVOID OutBuffer,
    _Out_ PULONG BufferUsed
    )
{
    M487FilterDeviceInformation* pInfo;
    PDEVICE_CONTEXT devContext;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(OutBufferSize);

    pInfo = InstanceGetInfo(WmiInstance);
    devContext = WdfObjectGet_DEVICE_CONTEXT(WdfWmiInstanceGetDevice(WmiInstance));

    //
    // Update dynamic info
    //
    pInfo->Active = devContext->InterceptState.Initialized;
    pInfo->CaptureEnabled = devContext->InterceptState.Config.CaptureSetFeature ||
                            devContext->InterceptState.Config.CaptureGetFeature;

    *BufferUsed = sizeof(*pInfo);
    RtlCopyMemory(OutBuffer, pInfo, sizeof(*pInfo));

    M487FILTER_ETW_LOG_CONFIG(
        devContext->InterceptState.Config.CaptureSetFeature,
        devContext->InterceptState.Config.CaptureGetFeature,
        devContext->InterceptState.Config.CaptureWriteReport,
        devContext->InterceptState.Config.CaptureReadReport);

    return STATUS_SUCCESS;
}

//
// EvtWmiInstanceSetInstance - Set M487FilterDeviceInformation
//
NTSTATUS
EvtWmiInstanceSetInstance(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG InBufferSize,
    _In_ PVOID InBuffer
    )
{
    M487FilterDeviceInformation* pInfo;
    PDEVICE_CONTEXT devContext;
    M487_INTERCEPT_CONFIG config;
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL irql;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(InBufferSize);

    pInfo = InstanceGetInfo(WmiInstance);
    devContext = WdfObjectGet_DEVICE_CONTEXT(WdfWmiInstanceGetDevice(WmiInstance));

    //
    // Copy data from user-mode
    //
    RtlMoveMemory(pInfo, InBuffer, sizeof(*pInfo));

    //
    // Apply pass-through setting to interception config
    //
    KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
    if (pInfo->PassThroughEnabled) {
        devContext->InterceptState.Config.CaptureSetFeature = FALSE;
        devContext->InterceptState.Config.CaptureGetFeature = FALSE;
        devContext->InterceptState.Config.CaptureWriteReport = FALSE;
        devContext->InterceptState.Config.CaptureReadReport = FALSE;
    } else {
        devContext->InterceptState.Config.CaptureSetFeature = TRUE;
        devContext->InterceptState.Config.CaptureGetFeature = TRUE;
        devContext->InterceptState.Config.CaptureWriteReport = TRUE;
        devContext->InterceptState.Config.CaptureReadReport = TRUE;
    }
    KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);

    KdPrint(("M487Filter: WMI SetInstance - PassThrough=%d, Capture=%d\n",
             pInfo->PassThroughEnabled, pInfo->CaptureEnabled));

    M487FILTER_ETW_LOG_CONFIG(
        devContext->InterceptState.Config.CaptureSetFeature,
        devContext->InterceptState.Config.CaptureGetFeature,
        devContext->InterceptState.Config.CaptureWriteReport,
        devContext->InterceptState.Config.CaptureReadReport);

    return status;
}

//
// EvtWmiInstanceSetItem - Set individual item in M487FilterDeviceInformation
//
NTSTATUS
EvtWmiInstanceSetItem(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG DataItemId,
    _In_ ULONG InBufferSize,
    _In_ PVOID InBuffer
    )
{
    M487FilterDeviceInformation* pInfo;
    PDEVICE_CONTEXT devContext;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    KIRQL irql;

    PAGED_CODE();

    pInfo = InstanceGetInfo(WmiInstance);
    devContext = WdfObjectGet_DEVICE_CONTEXT(WdfWmiInstanceGetDevice(WmiInstance));

    switch (DataItemId) {

    case M487FilterDeviceInformation_PassThroughEnabled:
        if (InBufferSize < M487FilterDeviceInformation_PassThroughEnabled_SIZE) {
            return STATUS_BUFFER_TOO_SMALL;
        }
        pInfo->PassThroughEnabled = (*(PBOOLEAN)InBuffer) ? TRUE : FALSE;

        //
        // Apply to intercept config
        //
        KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
        if (pInfo->PassThroughEnabled) {
            devContext->InterceptState.Config.CaptureSetFeature = FALSE;
            devContext->InterceptState.Config.CaptureGetFeature = FALSE;
        }
        KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);

        KdPrint(("M487Filter: WMI SetItem - PassThroughEnabled=%d\n",
                 pInfo->PassThroughEnabled));
        M487FILTER_ETW_LOG_CONFIG(
            devContext->InterceptState.Config.CaptureSetFeature,
            devContext->InterceptState.Config.CaptureGetFeature,
            devContext->InterceptState.Config.CaptureWriteReport,
            devContext->InterceptState.Config.CaptureReadReport);
        status = STATUS_SUCCESS;
        break;

    case M487FilterDeviceInformation_CaptureEnabled:
        if (InBufferSize < M487FilterDeviceInformation_CaptureEnabled_SIZE) {
            return STATUS_BUFFER_TOO_SMALL;
        }
        pInfo->CaptureEnabled = (*(PBOOLEAN)InBuffer) ? TRUE : FALSE;

        KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
        if (pInfo->CaptureEnabled) {
            devContext->InterceptState.Config.CaptureSetFeature = TRUE;
            devContext->InterceptState.Config.CaptureGetFeature = TRUE;
            devContext->InterceptState.Config.CaptureWriteReport = TRUE;
            devContext->InterceptState.Config.CaptureReadReport = TRUE;
        }
        KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);

        KdPrint(("M487Filter: WMI SetItem - CaptureEnabled=%d\n",
                 pInfo->CaptureEnabled));
        M487FILTER_ETW_LOG_CONFIG(
            devContext->InterceptState.Config.CaptureSetFeature,
            devContext->InterceptState.Config.CaptureGetFeature,
            devContext->InterceptState.Config.CaptureWriteReport,
            devContext->InterceptState.Config.CaptureReadReport);
        status = STATUS_SUCCESS;
        break;

    default:
        KdPrint(("M487Filter: WMI SetItem - unknown DataItemId %lu\n", DataItemId));
        break;
    }

    return status;
}

//
// EvtRingBufferQueryInstance - Query ring buffer status via WMI
//
NTSTATUS
EvtRingBufferQueryInstance(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG OutBufferSize,
    _In_ PVOID OutBuffer,
    _Out_ PULONG BufferUsed
    )
{
    M487FilterRingBufferInfo ringInfo;
    PRING_BUFFER_WMI_CONTEXT ctx;
    PDEVICE_CONTEXT devContext;
    PM487_RING_BUFFER ringBuffer;
    PHYSICAL_ADDRESS physAddr;
    ULONG64 totalCaptured = 0;
    ULONG droppedCount = 0;
    ULONG currentUsed = 0;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(OutBufferSize);

    ctx = RingBufferContextGet(WmiInstance);
    devContext = ctx->DeviceContext;
    ringBuffer = &devContext->InterceptState.RingBuffer;

    RtlZeroMemory(&ringInfo, sizeof(ringInfo));

    if (ringBuffer->Control != NULL && ringBuffer->Control->Magic == M487_RING_MAGIC) {
        ringInfo.RingInitialized = TRUE;
        ringInfo.RingBufferSize = ringBuffer->Control->RingByteSize;
        ringInfo.TotalCaptured = ringBuffer->Control->TotalCaptured;
        ringInfo.DroppedCount = ringBuffer->Control->DroppedCount;
        ringInfo.TotalSlots = M487_RING_SLOT_COUNT;
        ringInfo.SlotSize = M487_RING_SLOT_SIZE(M487_MAX_REPORT_SIZE);

        //
        // Calculate used slots
        //
        currentUsed = ringBuffer->Control->WriteIndex - ringBuffer->Control->ReadIndex;
        ringInfo.CurrentUsedSlots = currentUsed;

        //
        // Get physical address
        //
        physAddr = MmGetPhysicalAddress(ringBuffer->Control);
        ringInfo.RingBufferPhysicalAddress = physAddr.QuadPart;

        M487FILTER_ETW_LOG_RING(
            ringInfo.TotalCaptured,
            ringInfo.DroppedCount,
            (USHORT)ringInfo.CurrentUsedSlots,
            (UCHAR)(ringBuffer->Control->WriteIndex & 0xFF));
    } else {
        ringInfo.RingInitialized = FALSE;
    }

    *BufferUsed = sizeof(ringInfo);
    RtlCopyMemory(OutBuffer, &ringInfo, sizeof(ringInfo));

    KdPrint(("M487Filter: WMI RingBufferQuery - TotalCap=%I64u, Used=%lu, Dropped=%lu\n",
             ringInfo.TotalCaptured, ringInfo.CurrentUsedSlots, ringInfo.DroppedCount));

    return STATUS_SUCCESS;
}

//
// EvtRingBufferSetInstance - Ring buffer WMI set (no-op, ring buffer is read-only)
//
NTSTATUS
EvtRingBufferSetInstance(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG InBufferSize,
    _In_ PVOID InBuffer
    )
{
    UNREFERENCED_PARAMETER(WmiInstance);
    UNREFERENCED_PARAMETER(InBufferSize);
    UNREFERENCED_PARAMETER(InBuffer);

    PAGED_CODE();

    //
    // Ring buffer is not directly settable - it's controlled via intercept config
    //
    KdPrint(("M487Filter: WMI RingBufferSetInstance called (no-op)\n"));
    return STATUS_WMI_READ_ONLY;
}

//
// EvtInterceptConfigQueryInstance - Query interception configuration
//
NTSTATUS
EvtInterceptConfigQueryInstance(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG OutBufferSize,
    _In_ PVOID OutBuffer,
    _Out_ PULONG BufferUsed
    )
{
    M487FilterInterceptConfigWmi configWmi;
    PINTERCEPT_CONFIG_WMI_CONTEXT ctx;
    PDEVICE_CONTEXT devContext;
    KIRQL irql;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(OutBufferSize);

    ctx = InterceptConfigContextGet(WmiInstance);
    devContext = ctx->DeviceContext;

    KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
    configWmi.CaptureSetFeature = devContext->InterceptState.Config.CaptureSetFeature;
    configWmi.CaptureGetFeature = devContext->InterceptState.Config.CaptureGetFeature;
    configWmi.CaptureWriteReport = devContext->InterceptState.Config.CaptureWriteReport;
    configWmi.CaptureReadReport = devContext->InterceptState.Config.CaptureReadReport;
    KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);

    *BufferUsed = sizeof(configWmi);
    RtlCopyMemory(OutBuffer, &configWmi, sizeof(configWmi));

    M487FILTER_ETW_LOG_CONFIG(
        configWmi.CaptureSetFeature,
        configWmi.CaptureGetFeature,
        configWmi.CaptureWriteReport,
        configWmi.CaptureReadReport);

    return STATUS_SUCCESS;
}

//
// EvtInterceptConfigSetInstance - Set interception configuration
//
NTSTATUS
EvtInterceptConfigSetInstance(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG InBufferSize,
    _In_ PVOID InBuffer
    )
{
    M487FilterInterceptConfigWmi configWmi;
    PINTERCEPT_CONFIG_WMI_CONTEXT ctx;
    PDEVICE_CONTEXT devContext;
    KIRQL irql;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(InBufferSize);

    if (InBufferSize < sizeof(configWmi)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    ctx = InterceptConfigContextGet(WmiInstance);
    devContext = ctx->DeviceContext;

    RtlCopyMemory(&configWmi, InBuffer, sizeof(configWmi));

    KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
    devContext->InterceptState.Config.CaptureSetFeature = configWmi.CaptureSetFeature;
    devContext->InterceptState.Config.CaptureGetFeature = configWmi.CaptureGetFeature;
    devContext->InterceptState.Config.CaptureWriteReport = configWmi.CaptureWriteReport;
    devContext->InterceptState.Config.CaptureReadReport = configWmi.CaptureReadReport;
    KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);

    KdPrint(("M487Filter: WMI SetInterceptConfig - SetF=%d, GetF=%d, WriteR=%d, ReadR=%d\n",
             configWmi.CaptureSetFeature,
             configWmi.CaptureGetFeature,
             configWmi.CaptureWriteReport,
             configWmi.CaptureReadReport));

    M487FILTER_ETW_LOG_CONFIG(
        configWmi.CaptureSetFeature,
        configWmi.CaptureGetFeature,
        configWmi.CaptureWriteReport,
        configWmi.CaptureReadReport);

    return STATUS_SUCCESS;
}

//
// EvtInterceptConfigSetItem - Set individual interception config item
//
NTSTATUS
EvtInterceptConfigSetItem(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG DataItemId,
    _In_ ULONG InBufferSize,
    _In_ PVOID InBuffer
    )
{
    PINTERCEPT_CONFIG_WMI_CONTEXT ctx;
    PDEVICE_CONTEXT devContext;
    KIRQL irql;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE();

    ctx = InterceptConfigContextGet(WmiInstance);
    devContext = ctx->DeviceContext;

    switch (DataItemId) {
    case 1:  // CaptureSetFeature
        if (InBufferSize < 1) return STATUS_BUFFER_TOO_SMALL;
        KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
        devContext->InterceptState.Config.CaptureSetFeature = (*(PBOOLEAN)InBuffer) ? TRUE : FALSE;
        KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);
        KdPrint(("M487Filter: WMI SetItem - CaptureSetFeature=%d\n",
                 devContext->InterceptState.Config.CaptureSetFeature));
        status = STATUS_SUCCESS;
        break;

    case 2:  // CaptureGetFeature
        if (InBufferSize < 1) return STATUS_BUFFER_TOO_SMALL;
        KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
        devContext->InterceptState.Config.CaptureGetFeature = (*(PBOOLEAN)InBuffer) ? TRUE : FALSE;
        KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);
        KdPrint(("M487Filter: WMI SetItem - CaptureGetFeature=%d\n",
                 devContext->InterceptState.Config.CaptureGetFeature));
        status = STATUS_SUCCESS;
        break;

    case 3:  // CaptureWriteReport
        if (InBufferSize < 1) return STATUS_BUFFER_TOO_SMALL;
        KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
        devContext->InterceptState.Config.CaptureWriteReport = (*(PBOOLEAN)InBuffer) ? TRUE : FALSE;
        KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);
        KdPrint(("M487Filter: WMI SetItem - CaptureWriteReport=%d\n",
                 devContext->InterceptState.Config.CaptureWriteReport));
        status = STATUS_SUCCESS;
        break;

    case 4:  // CaptureReadReport
        if (InBufferSize < 1) return STATUS_BUFFER_TOO_SMALL;
        KeAcquireSpinLock(&devContext->InterceptState.ConfigLock, &irql);
        devContext->InterceptState.Config.CaptureReadReport = (*(PBOOLEAN)InBuffer) ? TRUE : FALSE;
        KeReleaseSpinLock(&devContext->InterceptState.ConfigLock, irql);
        KdPrint(("M487Filter: WMI SetItem - CaptureReadReport=%d\n",
                 devContext->InterceptState.Config.CaptureReadReport));
        status = STATUS_SUCCESS;
        break;

    default:
        KdPrint(("M487Filter: WMI SetItem - unknown DataItemId %lu\n", DataItemId));
        break;
    }

    if (NT_SUCCESS(status)) {
        M487FILTER_ETW_LOG_CONFIG(
            devContext->InterceptState.Config.CaptureSetFeature,
            devContext->InterceptState.Config.CaptureGetFeature,
            devContext->InterceptState.Config.CaptureWriteReport,
            devContext->InterceptState.Config.CaptureReadReport);
    }

    return status;
}
