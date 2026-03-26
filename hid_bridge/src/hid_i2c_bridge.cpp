#include "hid_i2c.h"
#include <hidsdi.h>
#include <setupapi.h>
#include <iostream>

// GUID for HID devices
static const GUID GUID_HID_DEVICE = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x00, 0x00, 0x30, 0x0C } };

HidI2CBridge::HidI2CBridge()
    : m_connected(false)
    , m_hidHandle(INVALID_HANDLE_VALUE)
{
}

HidI2CBridge::~HidI2CBridge()
{
    disconnect();
}

bool HidI2CBridge::enumerateDevices()
{
    m_devicePaths.clear();

    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
        &GUID_HID_DEVICE,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        m_lastError = "SetupDiGetClassDevs failed";
        return false;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData = {};
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &GUID_HID_DEVICE, i, &interfaceData); ++i) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, NULL, 0, &requiredSize, NULL);

        if (requiredSize == 0) continue;

        std::vector<uint8_t> buffer(requiredSize);
        SP_DEVICE_INTERFACE_DETAIL_DATA* detailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(buffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, detailData, requiredSize, NULL, NULL)) {
            continue;
        }

        // Open device to check VID/PID
        HANDLE hDevice = CreateFile(
            detailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hDevice == INVALID_HANDLE_VALUE) continue;

        // Get HID attributes
        HIDD_ATTRIBUTES attrib = {};
        attrib.Size = sizeof(HIDD_ATTRIBUTES);

        if (HidD_GetAttributes(hDevice, &attrib)) {
            if (attrib.VendorID == M487_HID_VID && attrib.ProductID == M487_HID_PID) {
                m_devicePaths.push_back(detailData->DevicePath);
            }
        }

        CloseHandle(hDevice);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return !m_devicePaths.empty();
}

bool HidI2CBridge::connect(int deviceIndex)
{
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(m_devicePaths.size())) {
        if (!enumerateDevices() || m_devicePaths.empty()) {
            m_lastError = "No HID I2C Bridge device found (VID=0x0416, PID=0x5020)";
            return false;
        }
    }

    if (deviceIndex >= static_cast<int>(m_devicePaths.size())) {
        m_lastError = "Invalid device index";
        return false;
    }

    disconnect();

    m_hidHandle = CreateFile(
        m_devicePaths[deviceIndex].c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (m_hidHandle == INVALID_HANDLE_VALUE) {
        m_lastError = "CreateFile failed: " + std::to_string(GetLastError());
        return false;
    }

    m_connected = true;
    return true;
}

void HidI2CBridge::disconnect()
{
    if (m_hidHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hidHandle);
        m_hidHandle = INVALID_HANDLE_VALUE;
    }
    m_connected = false;
}

bool HidI2CBridge::sendHidReport(const uint8_t* report, DWORD reportLen)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    // HID reports include the report ID as the first byte
    if (!HidD_SetFeature(m_hidHandle, (PVOID)report, reportLen)) {
        m_lastError = "HidD_SetFeature failed: " + std::to_string(GetLastError());
        return false;
    }

    return true;
}

bool HidI2CBridge::getHidReport(uint8_t* report, DWORD* reportLen, DWORD timeoutMs)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    // For interrupt endpoint reports, use ReadFile
    // But HIDclass doesn't expose interrupt endpoint directly in many cases
    // So we use HidD_GetFeature (for devices that support it)
    
    // Try HidD_GetFeature first
    if (HidD_GetFeature(m_hidHandle, (PVOID)report, *reportLen)) {
        return true;
    }

    // Alternative: Use ReadFile for interrupt data
    OVERLAPPED overlapped = {};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!overlapped.hEvent) {
        m_lastError = "CreateEvent failed";
        return false;
    }

    BOOL result = ReadFile(m_hidHandle, report, *reportLen, reportLen, &overlapped);

    if (!result) {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            DWORD waitResult = WaitForSingleObject(overlapped.hEvent, timeoutMs);
            if (waitResult == WAIT_TIMEOUT) {
                CancelIo(m_hidHandle);
                CloseHandle(overlapped.hEvent);
                m_lastError = "ReadFile timeout";
                return false;
            } else if (waitResult == WAIT_OBJECT_0) {
                if (!GetOverlappedResult(m_hidHandle, &overlapped, reportLen, FALSE)) {
                    CloseHandle(overlapped.hEvent);
                    m_lastError = "GetOverlappedResult failed";
                    return false;
                }
            }
        } else {
            CloseHandle(overlapped.hEvent);
            m_lastError = "ReadFile failed: " + std::to_string(error);
            return false;
        }
    }

    CloseHandle(overlapped.hEvent);
    return true;
}

bool HidI2CBridge::i2cWrite(uint8_t slaveAddr, const uint8_t* data, uint16_t len)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    if (len > 60) {
        m_lastError = "Write length exceeds 60 bytes";
        return false;
    }

    HID_I2C_WriteReport_t report = {};
    report.reportId = HID_REPORT_ID_I2C_WRITE;
    report.slaveAddr = (slaveAddr << 1) & 0xFE;  // 7-bit address, W bit = 0
    report.length = len;
    memcpy(report.data, data, len);

    // Send report
    DWORD reportLen = 3 + len;
    if (!sendHidReport((const uint8_t*)&report, reportLen)) {
        return false;
    }

    // Get response
    uint8_t response[64] = {};
    DWORD respLen = sizeof(response);
    
    // Wait a bit for device to process
    Sleep(10);
    
    if (!getHidReport(response, &respLen, 1000)) {
        // Some devices don't send response for write
        // So we consider write success if send succeeded
        return true;
    }

    // Check status
    if (response[1] != 0x00) {
        m_lastError = "I2C write failed - slave NACK or error";
        return false;
    }

    return true;
}

bool HidI2CBridge::i2cRead(uint8_t slaveAddr, uint8_t* data, uint16_t len)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    if (len > 62) {
        m_lastError = "Read length exceeds 62 bytes";
        return false;
    }

    // Send read request
    HID_I2C_ReadReport_t request = {};
    request.reportId = HID_REPORT_ID_I2C_READ;
    request.slaveAddr = (slaveAddr << 1) | 0x01;  // 7-bit address, R bit = 1
    request.length = len;

    if (!sendHidReport((const uint8_t*)&request, sizeof(request))) {
        return false;
    }

    // Get response
    HID_I2C_ReadResponse_t response = {};
    DWORD respLen = sizeof(response);
    
    // Wait for device to process and send response
    Sleep(10);
    
    if (!getHidReport((uint8_t*)&response, &respLen, 1000)) {
        m_lastError = "Failed to get I2C read response";
        return false;
    }

    if (response.status != 0x00) {
        m_lastError = "I2C read failed - slave NACK or error";
        return false;
    }

    // Copy data (skip reportId, status, length)
    uint16_t copyLen = (len < response.length) ? len : response.length;
    memcpy(data, response.data, copyLen);

    return true;
}

bool HidI2CBridge::i2cWriteRead(uint8_t slaveAddr, const uint8_t* wdata, uint16_t wlen, uint8_t* rdata, uint16_t rlen)
{
    if (!m_connected) {
        m_lastError = "Not connected";
        return false;
    }

    if (wlen > 60 || rlen > 62) {
        m_lastError = "Write or read length exceeds limit";
        return false;
    }

    // Send write+read request
    HID_I2C_WriteReadReport_t request = {};
    request.reportId = HID_REPORT_ID_I2C_WRITEREAD;
    request.slaveAddr = (slaveAddr << 1);  // Will be ORed with R/W in firmware
    request.writeLen = wlen;
    request.readLen = rlen;
    memcpy(request.writeData, wdata, wlen);

    if (!sendHidReport((const uint8_t*)&request, 4 + wlen)) {
        return false;
    }

    // Get response
    HID_I2C_WriteReadResponse_t response = {};
    DWORD respLen = sizeof(response);
    
    // Wait for device to process I2C transaction and send response
    Sleep(10);
    
    if (!getHidReport((uint8_t*)&response, &respLen, 1000)) {
        m_lastError = "Failed to get I2C write+read response";
        return false;
    }

    if (response.status != 0x00) {
        m_lastError = "I2C write+read failed - slave NACK or error";
        return false;
    }

    // Copy data
    uint16_t copyLen = (rlen < response.length) ? rlen : response.length;
    memcpy(rdata, response.readData, copyLen);

    return true;
}
