#include "m487_hid_device.h"

#include <hidsdi.h>
#include <setupapi.h>
#include <cstring>
#include <algorithm>

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

M487HidDevice::M487HidDevice()
    : m_handle(INVALID_HANDLE_VALUE)
{
}

M487HidDevice::~M487HidDevice()
{
    close();
}

// ---------------------------------------------------------------------------
// Error helpers
// ---------------------------------------------------------------------------

void M487HidDevice::setError(const std::string& msg)
{
    m_lastError = msg;
}

void M487HidDevice::setErrorWin32(const std::string& prefix)
{
    DWORD err = GetLastError();
    m_lastError = prefix + " (Win32 error " + std::to_string(err) + ")";
}

// ---------------------------------------------------------------------------
// Enumerate HID devices matching M487 VID/PID
// ---------------------------------------------------------------------------

static std::string wcharToString(const wchar_t* wstr)
{
    if (!wstr || wstr[0] == L'\0') return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr);
    return result;
}

int M487HidDevice::enumerate()
{
    m_devices.clear();

    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO devInfoSet = SetupDiGetClassDevs(
        &hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (devInfoSet == INVALID_HANDLE_VALUE) {
        setErrorWin32("SetupDiGetClassDevs failed");
        return 0;
    }

    SP_DEVICE_INTERFACE_DATA ifData{};
    ifData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD idx = 0;
         SetupDiEnumDeviceInterfaces(devInfoSet, nullptr, &hidGuid, idx, &ifData);
         ++idx)
    {
        // Get required buffer size
        DWORD reqSize = 0;
        SetupDiGetDeviceInterfaceDetailA(devInfoSet, &ifData, nullptr, 0, &reqSize, nullptr);
        if (reqSize == 0) continue;

        std::vector<char> buf(reqSize, 0);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_A*>(buf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        if (!SetupDiGetDeviceInterfaceDetailA(devInfoSet, &ifData, detail, reqSize, nullptr, nullptr))
            continue;

        // Open to read attributes
        HANDLE hDev = CreateFileA(
            detail->DevicePath,
            0,  // No read/write — just query attributes
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, 0, nullptr);

        if (hDev == INVALID_HANDLE_VALUE) continue;

        HIDD_ATTRIBUTES attrs{};
        attrs.Size = sizeof(HIDD_ATTRIBUTES);

        if (HidD_GetAttributes(hDev, &attrs) &&
            attrs.VendorID == M487_VID &&
            attrs.ProductID == M487_PID)
        {
            HidDeviceInfo info;
            info.path          = detail->DevicePath;
            info.vendorId      = attrs.VendorID;
            info.productId     = attrs.ProductID;
            info.versionNumber = attrs.VersionNumber;

            // Read string descriptors (best-effort)
            wchar_t wbuf[256] = {};
            if (HidD_GetManufacturerString(hDev, wbuf, sizeof(wbuf)))
                info.manufacturer = wcharToString(wbuf);
            if (HidD_GetProductString(hDev, wbuf, sizeof(wbuf)))
                info.product = wcharToString(wbuf);
            if (HidD_GetSerialNumberString(hDev, wbuf, sizeof(wbuf)))
                info.serialNumber = wcharToString(wbuf);

            m_devices.push_back(std::move(info));
        }

        CloseHandle(hDev);
    }

    SetupDiDestroyDeviceInfoList(devInfoSet);
    return static_cast<int>(m_devices.size());
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

bool M487HidDevice::open(int index)
{
    if (index < 0 || index >= static_cast<int>(m_devices.size())) {
        setError("Device index out of range");
        return false;
    }

    close();

    // Open with overlapped I/O for async reads
    m_handle = CreateFileA(
        m_devices[index].path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr);

    if (m_handle == INVALID_HANDLE_VALUE) {
        setErrorWin32("CreateFile failed for " + m_devices[index].path);
        return false;
    }

    return true;
}

bool M487HidDevice::openFirst()
{
    if (enumerate() == 0) {
        setError("No M487 device found (VID=0x04F3, PID=0x0732)");
        return false;
    }
    return open(0);
}

void M487HidDevice::close()
{
    if (m_handle != INVALID_HANDLE_VALUE) {
        CancelIo(m_handle);
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

// ---------------------------------------------------------------------------
// Output Report  (Host -> Device)
// ---------------------------------------------------------------------------

bool M487HidDevice::sendOutputReport(const uint8_t* report, size_t length)
{
    if (!isOpen()) { setError("Device not open"); return false; }

    // Pad to full report size (Windows HID requires exact report size)
    uint8_t buf[HID_REPORT_SIZE] = {};
    size_t copyLen = (std::min)(length, HID_REPORT_SIZE);
    memcpy(buf, report, copyLen);

    OVERLAPPED ov{};
    ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) { setErrorWin32("CreateEvent"); return false; }

    DWORD written = 0;
    BOOL ok = WriteFile(m_handle, buf, static_cast<DWORD>(HID_REPORT_SIZE), &written, &ov);

    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            DWORD wait = WaitForSingleObject(ov.hEvent, 1000);
            if (wait == WAIT_OBJECT_0) {
                ok = GetOverlappedResult(m_handle, &ov, &written, FALSE);
            } else {
                CancelIo(m_handle);
                CloseHandle(ov.hEvent);
                setError("WriteFile timeout");
                return false;
            }
        }
    }

    CloseHandle(ov.hEvent);

    if (!ok) {
        setErrorWin32("WriteFile failed");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Input Report  (Device -> Host)
// ---------------------------------------------------------------------------

size_t M487HidDevice::recvInputReport(uint8_t* report, size_t maxLength, DWORD timeoutMs)
{
    if (!isOpen()) { setError("Device not open"); return 0; }

    uint8_t buf[HID_REPORT_SIZE] = {};

    OVERLAPPED ov{};
    ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!ov.hEvent) { setErrorWin32("CreateEvent"); return 0; }

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(m_handle, buf, static_cast<DWORD>(HID_REPORT_SIZE), &bytesRead, &ov);

    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            DWORD wait = WaitForSingleObject(ov.hEvent, timeoutMs);
            if (wait == WAIT_OBJECT_0) {
                ok = GetOverlappedResult(m_handle, &ov, &bytesRead, FALSE);
            } else {
                CancelIo(m_handle);
                CloseHandle(ov.hEvent);
                setError("ReadFile timeout");
                return 0;
            }
        }
    }

    CloseHandle(ov.hEvent);

    if (!ok) {
        setErrorWin32("ReadFile failed");
        return 0;
    }

    size_t copyLen = (std::min)(static_cast<size_t>(bytesRead), maxLength);
    memcpy(report, buf, copyLen);
    return copyLen;
}

// ---------------------------------------------------------------------------
// Feature Reports
// ---------------------------------------------------------------------------

bool M487HidDevice::sendFeatureReport(const uint8_t* report, size_t length)
{
    if (!isOpen()) { setError("Device not open"); return false; }

    uint8_t buf[HID_REPORT_SIZE] = {};
    size_t copyLen = (std::min)(length, HID_REPORT_SIZE);
    memcpy(buf, report, copyLen);

    if (!HidD_SetFeature(m_handle, buf, static_cast<ULONG>(HID_REPORT_SIZE))) {
        setErrorWin32("HidD_SetFeature failed");
        return false;
    }
    return true;
}

bool M487HidDevice::recvFeatureReport(uint8_t* report, size_t length)
{
    if (!isOpen()) { setError("Device not open"); return false; }

    // report[0] must already contain the desired report ID
    uint8_t buf[HID_REPORT_SIZE] = {};
    buf[0] = report[0];

    if (!HidD_GetFeature(m_handle, buf, static_cast<ULONG>(HID_REPORT_SIZE))) {
        setErrorWin32("HidD_GetFeature failed");
        return false;
    }

    size_t copyLen = (std::min)(length, HID_REPORT_SIZE);
    memcpy(report, buf, copyLen);
    return true;
}

// ---------------------------------------------------------------------------
// High-level I2C: Write
// ---------------------------------------------------------------------------

bool M487HidDevice::i2cWrite(uint8_t slaveAddr, uint8_t regAddr,
                              const uint8_t* data, uint8_t length)
{
    if (!isOpen()) { setError("Device not open"); return false; }
    if (length > 59) { setError("Write length exceeds 59 bytes"); return false; }

    I2CWriteReport rpt{};
    rpt.reportId  = REPORT_ID_OUTPUT;
    rpt.slaveAddr = slaveAddr;
    rpt.operation = I2C_OP_WRITE;
    rpt.regAddr   = regAddr;
    rpt.length    = length;
    if (length > 0 && data)
        memcpy(rpt.data, data, length);

    if (!sendOutputReport(reinterpret_cast<const uint8_t*>(&rpt), sizeof(rpt)))
        return false;

    // Read status response
    I2CReadResponse resp{};
    size_t n = recvInputReport(reinterpret_cast<uint8_t*>(&resp), sizeof(resp), 2000);
    if (n == 0) {
        // Some firmware may not return a status for writes — treat send success as OK
        return true;
    }

    if (resp.status != STATUS_OK) {
        setError("I2C write failed: device returned status 0x" +
                 std::to_string(resp.status));
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// High-level I2C: Read
// ---------------------------------------------------------------------------

bool M487HidDevice::i2cRead(uint8_t slaveAddr, uint8_t regAddr,
                             uint8_t* data, uint8_t length)
{
    if (!isOpen()) { setError("Device not open"); return false; }
    if (length > 61) { setError("Read length exceeds 61 bytes"); return false; }

    I2CReadRequest rpt{};
    rpt.reportId  = REPORT_ID_OUTPUT;
    rpt.slaveAddr = slaveAddr;
    rpt.operation = I2C_OP_READ;
    rpt.regAddr   = regAddr;
    rpt.length    = length;

    if (!sendOutputReport(reinterpret_cast<const uint8_t*>(&rpt), sizeof(rpt)))
        return false;

    I2CReadResponse resp{};
    size_t n = recvInputReport(reinterpret_cast<uint8_t*>(&resp), sizeof(resp), 2000);
    if (n == 0) {
        setError("I2C read: no response from device");
        return false;
    }

    if (resp.status != STATUS_OK) {
        setError("I2C read failed: device returned status 0x" +
                 std::to_string(resp.status));
        return false;
    }

    uint8_t copyLen = (std::min)(length, resp.length);
    memcpy(data, resp.data, copyLen);
    return true;
}

// ---------------------------------------------------------------------------
// High-level I2C: Write-then-Read (repeated start)
// ---------------------------------------------------------------------------

bool M487HidDevice::i2cWriteRead(uint8_t slaveAddr,
                                  const uint8_t* wdata, uint8_t wlen,
                                  uint8_t* rdata, uint8_t rlen)
{
    if (!isOpen()) { setError("Device not open"); return false; }
    if (wlen > 59) { setError("Write portion exceeds 59 bytes"); return false; }
    if (rlen > 61) { setError("Read portion exceeds 61 bytes"); return false; }

    // Pack as a single output report with OP_WRITEREAD
    uint8_t buf[HID_REPORT_SIZE] = {};
    buf[0] = REPORT_ID_OUTPUT;
    buf[1] = slaveAddr;
    buf[2] = I2C_OP_WRITEREAD;
    buf[3] = wlen;
    buf[4] = rlen;
    if (wlen > 0 && wdata)
        memcpy(&buf[5], wdata, wlen);

    if (!sendOutputReport(buf, HID_REPORT_SIZE))
        return false;

    I2CReadResponse resp{};
    size_t n = recvInputReport(reinterpret_cast<uint8_t*>(&resp), sizeof(resp), 2000);
    if (n == 0) {
        setError("I2C writeread: no response from device");
        return false;
    }

    if (resp.status != STATUS_OK) {
        setError("I2C writeread failed: device returned status 0x" +
                 std::to_string(resp.status));
        return false;
    }

    uint8_t copyLen = (std::min)(rlen, resp.length);
    memcpy(rdata, resp.data, copyLen);
    return true;
}
