#pragma once
#ifndef HID_I2C_H
#define HID_I2C_H

#include <windows.h>
#include <vector>
#include <string>

// HID Report IDs
#define HID_REPORT_ID_I2C_WRITE     0x01
#define HID_REPORT_ID_I2C_READ     0x02
#define HID_REPORT_ID_I2C_WRITEREAD 0x03

// Device parameters
#define M487_HID_VID    0x0416
#define M487_HID_PID    0x5020

#pragma pack(push, 1)

// HID I2C Write Report
typedef struct {
    uint8_t reportId;       // 0x01
    uint8_t slaveAddr;      // 7-bit I2C address
    uint8_t length;         // Write length
    uint8_t data[60];       // Write data
} HID_I2C_WriteReport_t;

// HID I2C Read Report (Host → Device)
typedef struct {
    uint8_t reportId;       // 0x02
    uint8_t slaveAddr;      // 7-bit I2C address
    uint8_t length;         // Read length
} HID_I2C_ReadReport_t;

// HID I2C Read Response Report (Device → Host)
typedef struct {
    uint8_t reportId;       // 0x02
    uint8_t status;        // 0=OK, 1=Error
    uint8_t length;         // Actual read length
    uint8_t data[62];       // Read data
} HID_I2C_ReadResponse_t;

// HID I2C Write+Read Report (Host → Device)
typedef struct {
    uint8_t reportId;       // 0x03
    uint8_t slaveAddr;      // 7-bit I2C address
    uint8_t writeLen;       // Write length
    uint8_t readLen;        // Read length
    uint8_t writeData[60]; // Write data
} HID_I2C_WriteReadReport_t;

// HID I2C Write+Read Response (Device → Host)
typedef struct {
    uint8_t reportId;       // 0x03
    uint8_t status;         // 0=OK, 1=Error
    uint8_t length;        // Actual read length
    uint8_t readData[62];  // Read data
} HID_I2C_WriteReadResponse_t;

#pragma pack(pop)

class HidI2CBridge {
public:
    HidI2CBridge();
    ~HidI2CBridge();

    // Device enumeration and connection
    bool enumerateDevices();
    bool connect(int deviceIndex = 0);
    void disconnect();
    bool isConnected() const { return m_connected; }

    // I2C operations
    bool i2cWrite(uint8_t slaveAddr, const uint8_t* data, uint16_t len);
    bool i2cRead(uint8_t slaveAddr, uint8_t* data, uint16_t len);
    bool i2cWriteRead(uint8_t slaveAddr, const uint8_t* wdata, uint16_t wlen, uint8_t* rdata, uint16_t rlen);

    // Get device info
    const std::vector<std::string>& getDeviceList() const { return m_devicePaths; }

    // Get last error
    std::string getLastError() const { return m_lastError; }

private:
    bool m_connected;
    HANDLE m_hidHandle;
    std::vector<std::string> m_devicePaths;
    std::string m_lastError;

    bool sendHidReport(const uint8_t* report, DWORD reportLen);
    bool getHidReport(uint8_t* report, DWORD* reportLen, DWORD timeoutMs = 1000);
};

#endif // HID_I2C_H
