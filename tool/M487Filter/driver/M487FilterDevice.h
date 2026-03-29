/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterDevice.h

Abstract:

    Device context and declarations for M487 HID Upper Filter Driver.

Environment:

    Kernel mode

--*/

#ifndef _M487_FILTER_DEVICE_H
#define _M487_FILTER_DEVICE_H

//
// Forward declaration for intercept state
//
typedef struct _M487_INTERCEPT_STATE M487_INTERCEPT_STATE;

//
// Device context - equivalent to WDM device extension
//
typedef struct _DEVICE_CONTEXT
{
    //
    // WMI data instance (generated from MOF)
    //
    M487FilterDeviceInformation WmiInstance;

    //
    // PDO device name (obtained at attach time, used for IO Target)
    //
    UNICODE_STRING PdoName;

    //
    // Handle to the lower HID device (IO Target)
    //
    WDFIOTARGET LowerHidTarget;

    //
    // Preparsed data from HID device (for HIDP_* APIs)
    //
    PHIDP_PREPARSED_DATA PreparsedData;

    //
    // HID device capabilities
    //
    HIDP_CAPS HidCaps;

    //
    // IOCTL interception state (ring buffer, config, etc.)
    //
    M487_INTERCEPT_STATE InterceptState;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceContextGet)

//
// WMI instance info accessor (generated from MOF)
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(M487FilterDeviceInformation, InstanceGetInfo)

#endif // _M487_FILTER_DEVICE_H
