/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterIoctl.c

Abstract:

    Custom IOCTL handlers for M487 HID Upper Filter Driver.
    These handlers process user-mode requests that are NOT passed through
    to the lower HID device.

Environment:

    Kernel mode

--*/

#include "M487Filter.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, M487FilterEvtIoDeviceControl)
#pragma alloc_text(PAGE, M487FilterEvtIoDefault)
#endif

//
// Forward declaration for internal helper
//
_Must_inspect_result_
NTSTATUS
M487FilterSendHIDFeatureReport(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_(ReportLength) PCHAR ReportBuffer,
    _In_ size_t ReportLength
    );

//
// Ring buffer address info returned by IOCTL_M487FILTER_GET_RING_ADDR
//
typedef struct _M487FILTER_RING_ADDR
{
    ULONG64 PhysicalAddress;
    ULONG RingBufferSize;

} M487FILTER_RING_ADDR, *PM487FILTER_RING_ADDR;

NTSTATUS
M487FilterEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    Handle custom IOCTLs from user-mode applications.

Arguments:

    Queue - WDF queue handle
    Request - WDF request handle
    OutputBufferLength - Size of output buffer
    InputBufferLength - Size of input buffer
    IoControlCode - IOCTL code

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_CONTEXT devContext;
    WDFDEVICE device;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    size_t bytesReturned = 0;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    PAGED_CODE();

    device = WdfQueueGetDevice(Queue);
    devContext = WdfObjectGet_DEVICE_CONTEXT(device);

    KdPrint(("M487Filter: IOCTL 0x%X received\n", IoControlCode));

    switch (IoControlCode) {

    case IOCTL_M487FILTER_GET_INFO:
        {
            M487FILTER_INFO info;
            ULONG64 totalCaptured = 0;
            ULONG droppedCount = 0;
            ULONG currentUsed = 0;

            if (OutputBufferLength < sizeof(info)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            RtlZeroMemory(&info, sizeof(info));
            info.Version = 0x0100;  // v1.0.0
            info.FilterMode = (ULONG)devContext->WmiInstance.PassThroughEnabled;
            info.InputReportByteLength = devContext->HidCaps.InputReportByteLength;
            info.OutputReportByteLength = devContext->HidCaps.OutputReportByteLength;
            info.FeatureReportByteLength = devContext->HidCaps.FeatureReportByteLength;
            info.LowerDeviceOpened = (devContext->LowerHidTarget != NULL);

            //
            // Get interception stats
            //
            M487InterceptGetStats(devContext, &totalCaptured, &droppedCount, &currentUsed);
            info.TotalCaptured = totalCaptured;
            info.DroppedCount = droppedCount;
            info.RingBufferSlotsUsed = currentUsed;

            status = WdfRequestRetrieveOutputBuffer(
                Request, sizeof(info), &info, NULL);

            if (NT_SUCCESS(status)) {
                RtlCopyMemory(&info, WdfRequestOutputBuffer(Request),
                              sizeof(info));
                bytesReturned = sizeof(info);
                status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_M487FILTER_SET_MODE:
        {
            M487_FILTER_MODE mode;

            if (InputBufferLength < sizeof(ULONG)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = WdfRequestRetrieveInputBuffer(
                Request, sizeof(ULONG), &mode, NULL);

            if (NT_SUCCESS(status)) {
                devContext->WmiInstance.PassThroughEnabled =
                    (mode == FilterMode_PassThrough) ? TRUE : FALSE;
                KdPrint(("M487Filter: Set mode to %lu\n", mode));
                status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_M487FILTER_SEND_FEATURE:
        {
            PCHAR reportBuffer;
            size_t reportLength;

            if (InputBufferLength == 0) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = WdfRequestRetrieveInputBuffer(
                Request, InputBufferLength, &reportBuffer, &reportLength);

            if (NT_SUCCESS(status)) {
                status = M487FilterSendHIDFeatureReport(
                    devContext, reportBuffer, reportLength);
                if (NT_SUCCESS(status)) {
                    bytesReturned = reportLength;
                }
            }
        }
        break;

    case IOCTL_M487FILTER_GET_RING_STATS:
        {
            M487FILTER_RING_ADDR ringAddr;
            ULONG64 totalCaptured = 0;
            ULONG droppedCount = 0;
            ULONG currentUsed = 0;

            if (OutputBufferLength < sizeof(ringAddr)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = M487InterceptGetRingBufferInfo(
                devContext, &ringAddr.PhysicalAddress, &ringAddr.RingBufferSize);

            if (NT_SUCCESS(status)) {
                M487InterceptGetStats(
                    devContext, &totalCaptured, &droppedCount, &currentUsed);
                bytesReturned = sizeof(ringAddr);
            }
        }
        break;

    case IOCTL_M487FILTER_GET_RING_ADDR:
        {
            M487FILTER_RING_ADDR ringAddr;
            ULONG64 totalCaptured = 0;
            ULONG droppedCount = 0;
            ULONG currentUsed = 0;

            if (OutputBufferLength < sizeof(ringAddr)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            ringAddr.PhysicalAddress = 0;
            ringAddr.RingBufferSize = 0;

            status = M487InterceptGetRingBufferInfo(
                devContext, &ringAddr.PhysicalAddress, &ringAddr.RingBufferSize);

            if (NT_SUCCESS(status)) {
                M487InterceptGetStats(
                    devContext, &totalCaptured, &droppedCount, &currentUsed);

                status = WdfRequestRetrieveOutputBuffer(
                    Request, sizeof(ringAddr), &ringAddr, NULL);

                if (NT_SUCCESS(status)) {
                    bytesReturned = sizeof(ringAddr);
                }
            }
        }
        break;

    case IOCTL_M487FILTER_GET_INTERCEPT_CONFIG:
        {
            M487_INTERCEPT_CONFIG config;

            if (OutputBufferLength < sizeof(config)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            M487InterceptGetConfig(devContext, &config);

            status = WdfRequestRetrieveOutputBuffer(
                Request, sizeof(config), &config, NULL);

            if (NT_SUCCESS(status)) {
                bytesReturned = sizeof(config);
                status = STATUS_SUCCESS;
            }
        }
        break;

    case IOCTL_M487FILTER_MAP_RING_BUFFER:
        {
            M487_FILTER_RING_MAPPING_INFO mappingInfo;
            ULONG64 userVa = 0;
            ULONG ringSize = 0;
            ULONG slotSize = 0;
            ULONG slotCount = 0;
            ULONG maxDataSize = 0;

            if (OutputBufferLength < sizeof(mappingInfo)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = M487InterceptMapRingBufferToUser(
                devContext,
                &userVa,
                &ringSize,
                &slotSize,
                &slotCount,
                &maxDataSize
            );

            if (NT_SUCCESS(status)) {
                RtlZeroMemory(&mappingInfo, sizeof(mappingInfo));
                mappingInfo.RingBufferUserVa = userVa;
                mappingInfo.RingBufferSize = ringSize;
                mappingInfo.SlotSize = slotSize;
                mappingInfo.SlotCount = slotCount;
                mappingInfo.MaxDataSize = (USHORT)maxDataSize;

                status = WdfRequestRetrieveOutputBuffer(
                    Request, sizeof(mappingInfo), &mappingInfo, NULL);

                if (NT_SUCCESS(status)) {
                    RtlCopyMemory(WdfRequestOutputBuffer(Request),
                                  &mappingInfo, sizeof(mappingInfo));
                    bytesReturned = sizeof(mappingInfo);
                    status = STATUS_SUCCESS;
                }
            }
        }
        break;

    case IOCTL_M487FILTER_SET_INTERCEPT_CONFIG:
        {
            M487_INTERCEPT_CONFIG config;

            if (InputBufferLength < sizeof(config)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = WdfRequestRetrieveInputBuffer(
                Request, sizeof(config), &config, NULL);

            if (NT_SUCCESS(status)) {
                M487InterceptSetConfig(devContext, &config);
                status = STATUS_SUCCESS;
            }
        }
        break;

    default:
        KdPrint(("M487Filter: Unknown IOCTL 0x%X\n", IoControlCode));
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
    return status;
}

VOID
M487FilterEvtIoDefault(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
    )
/*++

Routine Description:

    Default I/O handler - pass all requests through to lower device.
    Since we registered with WdfFdoInitSetFilter(), this is only called
    for custom requests not handled by KMDF's automatic pass-through.

Arguments:

    Queue - WDF queue handle
    Request - WDF request handle

Return Value:

    None

--*/
{
    PAGED_CODE();

    KdPrint(("M487Filter: EvtIoDefault - forwarding request\n"));

    //
    // Forward to lower target (automatic with WdfFdoInitSetFilter)
    // This path should rarely be hit for IRPs that KMDF auto-passes through.
    //
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

NTSTATUS
M487FilterSendHIDFeatureReport(
    _In_ PDEVICE_CONTEXT DevContext,
    _In_reads_(ReportLength) PCHAR ReportBuffer,
    _In_ size_t ReportLength
    )
/*++

Routine Description:

    Send a HID Feature Report to the lower HID device.

Arguments:

    DevContext - Device context
    ReportBuffer - Feature report data
    ReportLength - Length of report data

Return Value:

    NTSTATUS

--*/
{
    WDF_MEMORY_DESCRIPTOR inputDescriptor;
    NTSTATUS status;

    PAGED_CODE();

    if (DevContext->LowerHidTarget == NULL) {
        KdPrint(("M487Filter: Lower device not opened\n"));
        return STATUS_DEVICE_NOT_READY;
    }

    if (DevContext->PreparsedData == NULL) {
        KdPrint(("M487Filter: No preparsed data\n"));
        return STATUS_DEVICE_NOT_READY;
    }

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
        &inputDescriptor,
        ReportBuffer,
        ReportLength);

    status = WdfIoTargetSendIoctlSynchronously(
        DevContext->LowerHidTarget,
        NULL,
        IOCTL_HID_SET_FEATURE,
        &inputDescriptor,
        NULL,
        NULL,
        NULL);

    if (!NT_SUCCESS(status)) {
        KdPrint(("M487Filter: IOCTL_HID_SET_FEATURE failed 0x%x\n", status));
    }

    return status;
}
