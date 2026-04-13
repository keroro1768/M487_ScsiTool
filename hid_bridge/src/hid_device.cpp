/**
 * @file     hid_device.cpp
 * @brief    M487 USB HID Device - Windows HID API Implementation
 * @version  1.0.0
 */

#include "hid_device.h"
#include <hidsdi.h>
#include <setupapi.h>
#include <iostream>
#include <algorithm>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

// {4D1E55B2-F16F-11CF-88CB-001100000030}
static const GUID GUID_DEVINTERFACE_HID = {
    0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x00, 0x00, 0x30, 0x0C }
};

/*---------------------------------------------------------------------------------------------------------*/
/* Constructor / Destructor                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
M487HidDevice::M487HidDevice()
    : m_connected(false)
    , m_hidHandle(INVALID_HANDLE_VALUE)
{
    memset(&m_readOverlap, 0, sizeof(OVERLAPPED));
    memset(&m_writeOverlap, 0, sizeof(OVERLAPPED));
}

M487HidDevice::~M487HidDevice()
{
    disconnect();
}

/*---------------------------------------------------------------------------------------------------------*/
/* Device Enumeration                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
bool M487HidDevice::enumerate()
{
    m_devices.clear();

    // Get all HID devices
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
        &GUID_DEVINTERFACE_HID,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        m_lastError = "SetupDiGetClassDevs failed: " + std::to_string(GetLastError());
        return false;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData = {};
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD idx = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &GUID_DEVINTERFACE_HID, idx, &interfaceData); ++idx) {
        // Get required size
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, NULL, 0, &requiredSize, NULL);

        if (requiredSize == 0) continue;

        // Allocate buffer
        std::vector<uint8_t> buffer(requiredSize);
        SP_DEVICE_INTERFACE_DETAIL_DATA* detailData =
            reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(buffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, detailData, requiredSize, NULL, NULL)) {
            continue;
        }

        // Open device to read attributes
        HANDLE hDevice = CreateFile(
            detailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,  // Use overlapped for interrupt reads
            NULL);

        if (hDevice == INVALID_HANDLE_VALUE) continue;

        // Get HID attributes (VID/PID)
        HIDD_ATTRIBUTES attrib = {};
        attrib.Size = sizeof(HIDD_ATTRIBUTES);

        if (HidD_GetAttributes(hDevice, &attrib)) {
            // Filter by M487 VID/PID
            if (attrib.VendorID == M487_VID && attrib.ProductID == M487_PID) {
                HidDeviceInfo info = {};
                info.path = detailData->DevicePath;
                info.vid = attrib.VendorID;
                info.pid = attrib.ProductID;
                info.version = attrib.VersionNumber;

                // Get manufacturer/product strings
                wchar_t mfgStr[256] = {0};
                wchar_t prodStr[256] = {0};
                if (HidD_GetManufacturerString(hDevice, mfgStr, sizeof(mfgStr))) {
                    char mfg[256];
                    WideCharToMultiByte(CP_ACP, 0, mfgStr, -1, mfg, sizeof(mfg), NULL, NULL);
                    info.description = mfg;
                }
                if (HidD_GetProductString(hDevice, prodStr, sizeof(prodStr))) {
                    char prod[256];
                    WideCharToMultiByte(CP_ACP, 0, prodStr, -1, prod, sizeof(prod), NULL, NULL);
                    if (!info.description.empty()) info.description += " - ";
                    info.description += prod;
                }
                if (info.description.empty()) {
                    info.description = "(Unknown device)";
                }

                m_devices.push_back(info);
            }
        }

        CloseHandle(hDevice);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return !m_devices.empty();
}

/*---------------------------------------------------------------------------------------------------------*/
/* Connect / Disconnect                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
bool M487HidDevice::connect(size_t index)
{
    if (!m_devices.empty() && index >= m_devices.size()) {
        m_lastError = "Invalid device index";
        return false;
    }

    disconnect();

    std::string devicePath;
    if (!m_devices.empty()) {
        devicePath = m_devices[index].path;
    } else {
        // Try to enumerate first
        if (!enumerate() || m_devices.empty()) {
            m_lastError = "No M487 HID device found (VID=0x" +
                std::to_string(M487_VID) + ", PID=0x" + std::to_string(M487_PID) +
                "). Is the device connected?";
            return false;
        }
        devicePath = m_devices[0].path;
    }

    m_hidHandle = CreateFile(
        devicePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (m_hidHandle == INVALID_HANDLE_VALUE) {
        m_lastError = "CreateFile failed: " + std::to_string(GetLastError());
        return false;
    }

    // Create event for overlapped I/O
    m_readOverlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_writeOverlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!m_readOverlap.hEvent || !m_writeOverlap.hEvent) {
        m_lastError = "CreateEvent failed";
        CloseHandle(m_hidHandle);
        m_hidHandle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Set up read mode: tell driver we want interrupt-based reads
    // This is done automatically when we use ReadFile with the HID handle

    m_connected = true;
    return true;
}

void M487HidDevice::disconnect()
{
    if (m_readOverlap.hEvent) {
        CloseHandle(m_readOverlap.hEvent);
        m_readOverlap.hEvent = NULL;
    }
    if (m_writeOverlap.hEvent) {
        CloseHandle(m_writeOverlap.hEvent);
        m_writeOverlap.hEvent = NULL;
    }

    if (m_hidHandle != INVALID_HANDLE_VALUE) {
        // Cancel any pending I/O
        CancelIo(m_hidHandle);
        CloseHandle(m_hidHandle);
        m_hidHandle = INVALID_HANDLE_VALUE;
    }

    m_connected = false;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Device Attributes                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
bool M487HidDevice::getDeviceAttributes(HIDD_ATTRIBUTES* attr) const
{
    if (!m_connected || !attr) return false;
    attr->Size = sizeof(HIDD_ATTRIBUTES);
    return HidD_GetAttributes(m_hidHandle, attr) != FALSE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Raw HID Transfer                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
bool M487HidDevice::sendOutputReport(const uint8_t* data, size_t len)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    if (len == 0 || len > HID_REPORT_SIZE) {
        m_lastError = "Invalid report length";
        return false;
    }

    // For interrupt OUT, we use WriteFile
    // HID reports: first byte is the Report ID
    uint8_t buffer[HID_REPORT_SIZE] = {0};
    memcpy(buffer, data, len);

    DWORD bytesWritten = 0;
    ResetEvent(m_writeOverlap.hEvent);

    BOOL result = WriteFile(m_hidHandle, buffer, (DWORD)len, &bytesWritten, &m_writeOverlap);

    if (!result) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            DWORD wait = WaitForSingleObject(m_writeOverlap.hEvent, 1000);
            if (wait == WAIT_TIMEOUT) {
                CancelIo(m_hidHandle);
                m_lastError = "WriteFile timeout";
                return false;
            } else if (wait != WAIT_OBJECT_0) {
                m_lastError = "WriteFile wait failed: " + std::to_string(GetLastError());
                return false;
            }
            if (!GetOverlappedResult(m_hidHandle, &m_writeOverlap, &bytesWritten, FALSE)) {
                m_lastError = "GetOverlappedResult (write) failed: " + std::to_string(GetLastError());
                return false;
            }
        } else {
            m_lastError = "WriteFile failed: " + std::to_string(err);
            return false;
        }
    }

    return bytesWritten == (DWORD)len;
}

DWORD M487HidDevice::receiveInputReport(uint8_t* data, DWORD len, DWORD timeoutMs)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return 0;
    }

    uint8_t buffer[HID_REPORT_SIZE] = {0};
    DWORD bytesRead = 0;

    ResetEvent(m_readOverlap.hEvent);

    BOOL result = ReadFile(m_hidHandle, buffer, HID_REPORT_SIZE, &bytesRead, &m_readOverlap);

    if (!result) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            DWORD wait = WaitForSingleObject(m_readOverlap.hEvent, timeoutMs);
            if (wait == WAIT_TIMEOUT) {
                CancelIo(m_hidHandle);
                m_lastError = "ReadFile timeout";
                return 0;
            } else if (wait != WAIT_OBJECT_0) {
                m_lastError = "ReadFile wait failed: " + std::to_string(GetLastError());
                return 0;
            }
            if (!GetOverlappedResult(m_hidHandle, &m_readOverlap, &bytesRead, FALSE)) {
                m_lastError = "GetOverlappedResult (read) failed: " + std::to_string(GetLastError());
                return 0;
            }
        } else {
            m_lastError = "ReadFile failed: " + std::to_string(err);
            return 0;
        }
    }

    if (bytesRead > 0 && data && len > 0) {
        DWORD copyLen = (bytesRead < len) ? bytesRead : len;
        memcpy(data, buffer, copyLen);
        return copyLen;
    }

    return bytesRead;
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C High-Level Operations                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
bool M487HidDevice::i2cWrite(uint8_t addr, const uint8_t* data, uint16_t len)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }
    if (len > I2C_MAX_WRITE_LEN) {
        m_lastError = "Write length exceeds " + std::to_string(I2C_MAX_WRITE_LEN) + " bytes";
        return false;
    }

    HID_I2C_WriteReport_t report = {};
    report.reportId = HID_CMD_I2C_WRITE;
    report.slaveAddr = (addr << 1) & 0xFE;  // 7-bit addr, R/W = 0
    report.length = (uint8_t)len;
    if (data && len > 0)
        memcpy(report.data, data, len);

    DWORD reportLen = 3 + len;

    // Send write report
    if (!sendOutputReport((const uint8_t*)&report, reportLen)) {
        return false;
    }

    // Read response (interrupt IN)
    Sleep(5);  // Give device time to process

    HID_I2C_ReadResponse_t response = {};
    DWORD respLen = receiveInputReport((uint8_t*)&response, sizeof(response), 500);

    if (respLen >= 3 && response.reportId == HID_CMD_I2C_WRITE && response.status != 0x00) {
        m_lastError = "I2C write failed: slave NACK or bus error";
        return false;
    }

    return true;
}

bool M487HidDevice::i2cRead(uint8_t addr, uint8_t* data, uint16_t len)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }
    if (len > I2C_MAX_READ_LEN) {
        m_lastError = "Read length exceeds " + std::to_string(I2C_MAX_READ_LEN) + " bytes";
        return false;
    }

    // Send read request
    HID_I2C_ReadRequest_t request = {};
    request.reportId = HID_CMD_I2C_READ;
    request.slaveAddr = (addr << 1) | 0x01;  // 7-bit addr, R/W = 1
    request.length = (uint8_t)len;

    if (!sendOutputReport((const uint8_t*)&request, sizeof(request))) {
        return false;
    }

    // Read response
    HID_I2C_ReadResponse_t response = {};
    DWORD respLen = receiveInputReport((uint8_t*)&response, sizeof(response), 1000);

    if (respLen == 0) {
        m_lastError = "No response from device (timeout)";
        return false;
    }

    if (response.reportId != HID_CMD_I2C_READ) {
        m_lastError = "Unexpected report ID in response: 0x" +
            std::to_string(response.reportId);
        return false;
    }

    if (response.status != 0x00) {
        m_lastError = "I2C read failed: slave NACK or bus error (status=0x" +
            std::to_string(response.status) + ")";
        return false;
    }

    uint16_t copyLen = (len < response.length) ? len : response.length;
    if (data && copyLen > 0)
        memcpy(data, response.data, copyLen);

    return true;
}

bool M487HidDevice::i2cWriteRead(uint8_t addr, const uint8_t* wdata, uint16_t wlen, uint8_t* rdata, uint16_t rlen)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }
    if (wlen > I2C_MAX_WRITE_LEN) {
        m_lastError = "Write length exceeds " + std::to_string(I2C_MAX_WRITE_LEN) + " bytes";
        return false;
    }
    if (rlen > I2C_MAX_READ_LEN) {
        m_lastError = "Read length exceeds " + std::to_string(I2C_MAX_READ_LEN) + " bytes";
        return false;
    }

    // Build write+read request
    HID_I2C_WriteRead_t request = {};
    request.reportId = HID_CMD_I2C_WRITEREAD;
    request.slaveAddr = (addr << 1);  // R/W bit set by firmware
    request.writeLen = (uint8_t)wlen;
    request.readLen = (uint8_t)rlen;
    if (wdata && wlen > 0)
        memcpy(request.writeData, wdata, wlen);

    DWORD reportLen = 4 + wlen;

    // Send request
    if (!sendOutputReport((const uint8_t*)&request, reportLen)) {
        return false;
    }

    // Read response
    HID_I2C_WriteReadResponse_t response = {};
    DWORD respLen = receiveInputReport((uint8_t*)&response, sizeof(response), 1000);

    if (respLen == 0) {
        m_lastError = "No response from device (timeout)";
        return false;
    }

    if (response.reportId != HID_CMD_I2C_WRITEREAD) {
        m_lastError = "Unexpected report ID in response: 0x" + std::to_string(response.reportId);
        return false;
    }

    if (response.status != 0x00) {
        m_lastError = "I2C write+read failed (status=0x" + std::to_string(response.status) + ")";
        return false;
    }

    uint16_t copyLen = (rlen < response.length) ? rlen : response.length;
    if (rdata && copyLen > 0)
        memcpy(rdata, response.readData, copyLen);

    return true;
}

int M487HidDevice::i2cScan(uint8_t* foundAddrs)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return -1;
    }

    int count = 0;
    for (uint8_t addr = 1; addr < 127 && count < 8; ++addr) {
        uint8_t dummy;
        // Try to read 1 byte - if ACK, device exists
        if (i2cRead(addr, &dummy, 1)) {
            if (foundAddrs)
                foundAddrs[count] = addr;
            count++;
        }
    }
    return count;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Test Connection                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
bool M487HidDevice::testConnection()
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    HIDD_ATTRIBUTES attr = {};
    attr.Size = sizeof(HIDD_ATTRIBUTES);

    if (!HidD_GetAttributes(m_hidHandle, &attr)) {
        m_lastError = "HidD_GetAttributes failed";
        return false;
    }

    if (attr.VendorID != M487_VID || attr.ProductID != M487_PID) {
        m_lastError = "Wrong device: VID=0x" + std::to_string(attr.VendorID) +
            " PID=0x" + std::to_string(attr.ProductID);
        return false;
    }

    return true;
}
