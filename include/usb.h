#pragma once
#ifndef USB_H
#define USB_H

#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>

// GUID for USB device interface - declared extern, defined in usb.cpp
// {A5DCBF10-6530-11D2-901F-00C04FB951ED} = GUID_DEVINTERFACE_USB_DEVICE
extern const GUID GUID_DEVINTERFACE_USB_DEVICE;

// Define WinUSB handle types (opaque, avoid SDK header conflicts)
typedef void* WINUSB_INTERFACE_HANDLE;
typedef WINUSB_INTERFACE_HANDLE* PWINUSB_INTERFACE_HANDLE;

#include <vector>
#include <string>

// M487 USB Storage Device VID/PID
#define M487_VID         0x0416
#define M487_PID         0x501E

// Alternative PIDs for testing
#define M487_PID_ALT1    0xFF20  // Vendor Loopback
#define M487_PID_ALT2    0x5020  // Composite Device

// Endpoint addresses
#define BULK_OUT_EP      0x03
#define BULK_IN_EP       0x82

// Endpoint max packet size
#define BULK_EP_MAX_PACKET 64

struct UsbDeviceInfo {
    std::string devicePath;
    std::string description;
    UINT16 vid;
    UINT16 pid;
};

class UsbDevice {
public:
    UsbDevice();
    ~UsbDevice();

    bool open(UINT16 vid, UINT16 pid);
    void close();

    bool bulkWrite(const uint8_t* data, DWORD length, DWORD* bytesWritten);
    bool bulkRead(uint8_t* buffer, DWORD length, DWORD* bytesRead, DWORD timeoutMs = 5000);

    bool isOpen() const { return m_handle != INVALID_HANDLE_VALUE; }

    // Public for device enumeration
    static bool enumerateDevices(UINT16 vid, UINT16 pid, std::vector<UsbDeviceInfo>& devices);

private:
    HANDLE m_handle;
    WINUSB_INTERFACE_HANDLE m_winusbHandle;
    bool m_initialized;
};

#endif // USB_H
