/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487Filter.h

Abstract:

    Main header for the M487 HID Upper Filter Driver.
    Based on Microsoft Firefly sample.

Environment:

    Kernel mode

--*/

#ifndef _M487_FILTER_H
#define _M487_FILTER_H

#include <ntddk.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <initguid.h>
#include <wdmguid.h>

//
// HID related headers
//
#include <hidpddi.h>
#include <hidclass.h>

//
// WMI support
//
#include "M487FilterMof.h"

//
// Ring buffer for IOCTL capture
//
#include "M487FilterRingBuffer.h"

//
// ETW Tracing support
//
#include "M487FilterEtw.h"

//
// Local module headers
//
#include "M487FilterDevice.h"
#include "M487FilterWmi.h"
#include "M487FilterIoctl.h"
#include "M487FilterIntercept.h"

//
// Driver global events
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD M487FilterEvtDeviceAdd;
EVT_WDF_DEVICE_CONTEXT_CLEANUP M487FilterEvtDeviceContextCleanup;

#endif // _M487_FILTER_H
