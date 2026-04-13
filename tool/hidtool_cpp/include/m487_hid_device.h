#pragma once
#ifndef M487_HID_DEVICE_H
#define M487_HID_DEVICE_H

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

// ============================================================================
// Device identifiers
// ============================================================================
constexpr uint16_t M487_VID = 0x04F3;
constexpr uint16_t M487_PID = 0x0732;

// ============================================================================
// HID Report IDs (matches firmware USB HID Report Descriptor)
// ============================================================================
constexpr uint8_t REPORT_ID_OUTPUT      = 0x01;  // Host -> Device (Output Report)
constexpr uint8_t REPORT_ID_INPUT       = 0x02;  // Device -> Host (Input Report)
constexpr uint8_t REPORT_ID_FEATURE_OUT = 0x03;  // Host -> Device (Feature Report)
constexpr uint8_t REPORT_ID_FEATURE_IN  = 0x04;  // Device -> Host (Feature Report)
constexpr uint8_t REPORT_ID_CONTROL     = 0x05;  // Host -> Device (Control Command)
constexpr uint8_t REPORT_ID_STATUS      = 0x06;  // Device -> Host (Status Response)

// ============================================================================
// I2C operation codes (within Output Report payload)
// ============================================================================
constexpr uint8_t I2C_OP_WRITE     = 0x01;
constexpr uint8_t I2C_OP_READ      = 0x02;
constexpr uint8_t I2C_OP_WRITEREAD = 0x03;

// ============================================================================
// Status codes returned by device
// ============================================================================
constexpr uint8_t STATUS_OK          = 0x00;
constexpr uint8_t STATUS_NAK         = 0x01;
constexpr uint8_t STATUS_BUS_ERROR   = 0x02;
constexpr uint8_t STATUS_TIMEOUT     = 0x03;

// ============================================================================
// Report size limits
// ============================================================================
constexpr size_t HID_REPORT_SIZE     = 64;   // Max HID report payload (EP0/EP1)
constexpr size_t I2C_MAX_WRITE_DATA  = 60;   // Max I2C write payload per report
constexpr size_t I2C_MAX_READ_DATA   = 62;   // Max I2C read payload per report

// ============================================================================
// Packed report structures
// ============================================================================
#pragma pack(push, 1)

// Output Report: I2C Write command  (Host -> Device)
struct I2CWriteReport {
    uint8_t reportId;       // REPORT_ID_OUTPUT (0x01)
    uint8_t slaveAddr;      // 7-bit I2C address
    uint8_t operation;      // I2C_OP_WRITE (0x01)
    uint8_t regAddr;        // Register / offset address
    uint8_t length;         // Number of data bytes
    uint8_t data[59];       // Write payload (max 59 bytes)
};

// Output Report: I2C Read request  (Host -> Device)
struct I2CReadRequest {
    uint8_t reportId;       // REPORT_ID_OUTPUT (0x01)
    uint8_t slaveAddr;      // 7-bit I2C address
    uint8_t operation;      // I2C_OP_READ (0x02)
    uint8_t regAddr;        // Register / offset address
    uint8_t length;         // Number of bytes to read
};

// Input Report: I2C Read response  (Device -> Host)
struct I2CReadResponse {
    uint8_t reportId;       // REPORT_ID_INPUT (0x02)
    uint8_t status;         // STATUS_OK / STATUS_NAK / ...
    uint8_t length;         // Actual bytes returned
    uint8_t data[61];       // Read payload
};

// Feature Report: Generic 64-byte report (bidirectional)
struct FeatureReport {
    uint8_t reportId;       // REPORT_ID_FEATURE_OUT or REPORT_ID_FEATURE_IN
    uint8_t payload[63];    // Up to 63 bytes of payload
};

// Status Response (Device -> Host)
struct StatusReport {
    uint8_t reportId;       // REPORT_ID_STATUS (0x06)
    uint8_t statusCode;
    uint8_t reserved[62];
};

#pragma pack(pop)

// ============================================================================
// Device information
// ============================================================================
struct HidDeviceInfo {
    std::string path;
    uint16_t    vendorId;
    uint16_t    productId;
    uint16_t    versionNumber;
    std::string manufacturer;
    std::string product;
    std::string serialNumber;
};

// ============================================================================
// M487HidDevice — Main API class
// ============================================================================
class M487HidDevice {
public:
    M487HidDevice();
    ~M487HidDevice();

    // Non-copyable
    M487HidDevice(const M487HidDevice&) = delete;
    M487HidDevice& operator=(const M487HidDevice&) = delete;

    // ---- Device enumeration & connection ----

    // Enumerate all HID interfaces matching M487 VID/PID.
    // Returns number of matching interfaces found.
    int enumerate();

    // Get list of discovered devices (call enumerate() first).
    const std::vector<HidDeviceInfo>& getDevices() const { return m_devices; }

    // Open the device at the given index (from getDevices()).
    bool open(int index = 0);

    // Open the first device matching M487 VID/PID (enumerate + open).
    bool openFirst();

    // Close the device handle.
    void close();

    // Is the device handle open?
    bool isOpen() const { return m_handle != INVALID_HANDLE_VALUE; }

    // ---- Output Report (Host -> Device via Interrupt OUT / SET_REPORT) ----

    // Send a raw Output Report (64 bytes, first byte = report ID).
    bool sendOutputReport(const uint8_t* report, size_t length);

    // ---- Input Report (Device -> Host via Interrupt IN / ReadFile) ----

    // Receive a raw Input Report.  Returns bytes read, or 0 on failure.
    size_t recvInputReport(uint8_t* report, size_t maxLength, DWORD timeoutMs = 1000);

    // ---- Feature Reports ----

    // Send a Feature Report (HidD_SetFeature).
    bool sendFeatureReport(const uint8_t* report, size_t length);

    // Receive a Feature Report (HidD_GetFeature).
    // Caller must set report[0] = desired report ID before calling.
    bool recvFeatureReport(uint8_t* report, size_t length);

    // ---- High-level I2C operations ----

    // Write data to an I2C slave.
    bool i2cWrite(uint8_t slaveAddr, uint8_t regAddr,
                  const uint8_t* data, uint8_t length);

    // Read data from an I2C slave.
    bool i2cRead(uint8_t slaveAddr, uint8_t regAddr,
                 uint8_t* data, uint8_t length);

    // Combined write-then-read (restart condition).
    bool i2cWriteRead(uint8_t slaveAddr,
                      const uint8_t* wdata, uint8_t wlen,
                      uint8_t* rdata, uint8_t rlen);

    // ---- Error handling ----
    const std::string& lastError() const { return m_lastError; }

private:
    HANDLE                      m_handle;
    std::vector<HidDeviceInfo>  m_devices;
    std::string                 m_lastError;

    void setError(const std::string& msg);
    void setErrorWin32(const std::string& prefix);
};

#endif // M487_HID_DEVICE_H
