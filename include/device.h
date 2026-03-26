#pragma once
#ifndef DEVICE_H
#define DEVICE_H

#include "usb.h"
#include "scsi.h"
#include <string>
#include <vector>
#include <cstdint>

// Result structure for SCSI operations
struct ScsiResult {
    bool success;
    std::vector<uint8_t> data;
    std::string errorMessage;
    uint8_t cswStatus;
};

// Device information
struct DeviceInfo {
    std::string vendorId;
    std::string productId;
    std::string serialNumber;
    std::string firmwareVersion;
    uint32_t totalSectors;
    uint32_t sectorSize;
    uint32_t blockSize;  // Same as sectorSize, for compatibility
};

class M487Device {
public:
    M487Device();
    ~M487Device();

    // Enumerate and select device
    bool enumerateDevices();
    bool selectDevice(int index);

    // Connect/disconnect
    bool connect();
    void disconnect();

    // Device information
    bool getDeviceInfo(DeviceInfo& info);
    const std::vector<UsbDeviceInfo>& getDeviceList() const { return m_deviceList; }

    // SCSI operations
    ScsiResult scsiRead10(uint32_t lba, uint16_t sectorCount);
    ScsiResult scsiWrite10(uint32_t lba, uint16_t sectorCount, const uint8_t* data);
    ScsiResult scsiReadCapacity();
    ScsiResult scsiInquiry();
    
    // Vendor-specific command: Read device string
    ScsiResult vendorReadString(uint16_t bufferSize = 512);

    // I2C operations (via vendor command 0xC0)
    ScsiResult i2cWrite(uint8_t slaveAddr, const uint8_t* data, uint16_t len);
    ScsiResult i2cRead(uint8_t slaveAddr, uint16_t len);
    ScsiResult i2cWriteRead(uint8_t slaveAddr, const uint8_t* wdata, uint16_t wlen, uint16_t rlen);

    // Bulk storage info
    uint32_t getTotalSectors() const { return m_totalSectors; }
    uint32_t getSectorSize() const { return m_sectorSize; }
    bool isConnected() const { return m_connected; }

private:
    bool sendCbw(const CBW& cbw);
    bool sendData(const uint8_t* data, DWORD length);
    bool receiveData(uint8_t* buffer, DWORD length, DWORD* bytesRead, DWORD timeoutMs = 5000);
    bool receiveCsw(CSW& csw);

    UINT32 m_cbwTag;

    bool m_connected;
    std::vector<UsbDeviceInfo> m_deviceList;
    UsbDevice m_usb;

    uint32_t m_totalSectors;
    uint32_t m_sectorSize;
};

#endif // DEVICE_H
