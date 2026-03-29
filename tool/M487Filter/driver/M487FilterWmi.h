/*++

Copyright (c) 2026. All rights reserved.

Module Name:

    M487FilterWmi.h

Abstract:

    WMI declarations for M487 HID Upper Filter Driver.

Environment:

    Kernel mode

--*/

#ifndef _M487_FILTER_WMI_H
#define _M487_FILTER_WMI_H

//
// MOF resource name for WMI provider
//
#define MFRESOURCENAME L"M487FilterWMI"

//
// Initialize WMI support
//
NTSTATUS
M487FilterWmiInitialize(
    _In_ WDFDEVICE Device,
    _In_ PDEVICE_CONTEXT DeviceContext
    );

//
// WMI instance event callbacks for M487FilterDeviceInformation
//
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceQueryInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE   EvtWmiInstanceSetInstance;
EVT_WDF_WMI_INSTANCE_SET_ITEM       EvtWmiInstanceSetItem;

//
// WMI instance event callbacks for Ring Buffer Info
//
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtRingBufferQueryInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE   EvtRingBufferSetInstance;

//
// WMI instance event callbacks for Intercept Config
//
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtInterceptConfigQueryInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE   EvtInterceptConfigSetInstance;
EVT_WDF_WMI_INSTANCE_SET_ITEM        EvtInterceptConfigSetItem;

#endif // _M487_FILTER_WMI_H
