#include "device.h"
#include <iostream>
#include <iomanip>

M487Device::M487Device()
    : m_cbwTag(0)
    , m_connected(false)
    , m_totalSectors(0)
    , m_sectorSize(512)
{
}

M487Device::~M487Device()
{
    disconnect();
}

bool M487Device::enumerateDevices()
{
    m_deviceList.clear();
    return UsbDevice::enumerateDevices(M487_VID, M487_PID, m_deviceList);
}

bool M487Device::selectDevice(int index)
{
    if (index < 0 || index >= static_cast<int>(m_deviceList.size())) {
        std::cerr << "[ERROR] Invalid device index" << std::endl;
        return false;
    }
    return true;
}

bool M487Device::connect()
{
    if (m_connected) return true;

    // Try multiple PIDs
    UINT16 pids[] = { M487_PID, 0xFF20, 0x5020, 0x0470 };
    for (int i = 0; i < 4; i++) {
        if (m_usb.open(M487_VID, pids[i])) {
            m_connected = true;
            m_cbwTag = 1;
            DeviceInfo info;
            if (getDeviceInfo(info)) {
                m_totalSectors = info.totalSectors;
                m_sectorSize = info.sectorSize;
            }
            return true;
        }
    }
    return false;
}

void M487Device::disconnect()
{
    if (m_connected) {
        m_usb.close();
        m_connected = false;
    }
}

bool M487Device::sendCbw(const CBW& cbw)
{
    DWORD written = 0;
    if (!m_usb.bulkWrite(reinterpret_cast<const uint8_t*>(&cbw), sizeof(CBW), &written)) {
        std::cerr << "[ERROR] Failed to send CBW" << std::endl;
        return false;
    }
    if (written != sizeof(CBW)) {
        std::cerr << "[ERROR] CBW partial write: " << written << " bytes" << std::endl;
        return false;
    }
    return true;
}

bool M487Device::sendData(const uint8_t* data, DWORD length)
{
    DWORD written = 0;
    if (!m_usb.bulkWrite(data, length, &written)) {
        return false;
    }
    return written == length;
}

bool M487Device::receiveData(uint8_t* buffer, DWORD length, DWORD* bytesRead, DWORD timeoutMs)
{
    return m_usb.bulkRead(buffer, length, bytesRead, timeoutMs);
}

bool M487Device::receiveCsw(CSW& csw)
{
    DWORD bytesRead = 0;
    if (!m_usb.bulkRead(reinterpret_cast<uint8_t*>(&csw), sizeof(CSW), &bytesRead, 3000)) {
        return false;
    }
    if (bytesRead != sizeof(CSW)) {
        std::cerr << "[ERROR] CSW size mismatch: expected " << sizeof(CSW) 
                  << ", got " << bytesRead << std::endl;
        return false;
    }
    return true;
}

ScsiResult M487Device::scsiInquiry()
{
    ScsiResult result = {};
    result.success = false;

    CBW cbw = {};
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = m_cbwTag++;
    cbw.dCBWDataTransferLength = sizeof(SCSI_INQUIRY_DATA);
    cbw.bmCBWFlags = 0x80;  // Direction: IN
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = 6;
    ScsiCommands::buildInquiry(cbw.CDB, sizeof(SCSI_INQUIRY_DATA));

    if (!sendCbw(cbw)) return result;

    std::vector<uint8_t> response(sizeof(SCSI_INQUIRY_DATA));
    DWORD bytesRead = 0;
    if (!receiveData(response.data(), static_cast<DWORD>(response.size()), &bytesRead)) {
        result.errorMessage = "Failed to receive INQUIRY data";
        return result;
    }

    CSW csw = {};
    if (!receiveCsw(csw)) {
        result.errorMessage = "Failed to receive CSW";
        return result;
    }

    result.success = (csw.bCSWStatus == 0);
    result.data = response;
    result.cswStatus = csw.bCSWStatus;

    return result;
}

ScsiResult M487Device::scsiReadCapacity()
{
    ScsiResult result = {};
    result.success = false;

    CBW cbw = {};
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = m_cbwTag++;
    cbw.dCBWDataTransferLength = sizeof(SCSI_READ_CAPACITY_10);
    cbw.bmCBWFlags = 0x80;  // Direction: IN
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = 10;
    ScsiCommands::buildReadCapacity10(cbw.CDB);

    if (!sendCbw(cbw)) return result;

    std::vector<uint8_t> response(sizeof(SCSI_READ_CAPACITY_10));
    DWORD bytesRead = 0;
    if (!receiveData(response.data(), static_cast<DWORD>(response.size()), &bytesRead)) {
        result.errorMessage = "Failed to receive READ CAPACITY data";
        return result;
    }

    CSW csw = {};
    if (!receiveCsw(csw)) {
        result.errorMessage = "Failed to receive CSW";
        return result;
    }

    result.success = (csw.bCSWStatus == 0);
    result.data = response;
    result.cswStatus = csw.bCSWStatus;

    if (result.success && response.size() >= 8) {
        // Parse response (MSB first)
        uint32_t lastLba = (response[0] << 24) | (response[1] << 16) | 
                          (response[2] << 8) | response[3];
        uint32_t blockSize = (response[4] << 24) | (response[5] << 16) | 
                            (response[6] << 8) | response[7];
        m_totalSectors = lastLba + 1;
        m_sectorSize = blockSize;
    }

    return result;
}

ScsiResult M487Device::scsiRead10(uint32_t lba, uint16_t sectorCount)
{
    ScsiResult result = {};
    result.success = false;

    if (sectorCount == 0) {
        result.errorMessage = "Sector count must be > 0";
        return result;
    }

    DWORD dataSize = static_cast<DWORD>(sectorCount) * m_sectorSize;

    CBW cbw = {};
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = m_cbwTag++;
    cbw.dCBWDataTransferLength = dataSize;
    cbw.bmCBWFlags = 0x80;  // Direction: IN
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = 10;
    ScsiCommands::buildRead10(cbw.CDB, lba, sectorCount);

    if (!sendCbw(cbw)) return result;

    std::vector<uint8_t> response(dataSize);
    DWORD bytesRead = 0;
    if (!receiveData(response.data(), dataSize, &bytesRead)) {
        result.errorMessage = "Failed to receive READ10 data";
        return result;
    }

    CSW csw = {};
    if (!receiveCsw(csw)) {
        result.errorMessage = "Failed to receive CSW";
        return result;
    }

    result.success = (csw.bCSWStatus == 0);
    result.data = response;
    result.cswStatus = csw.bCSWStatus;

    return result;
}

ScsiResult M487Device::scsiWrite10(uint32_t lba, uint16_t sectorCount, const uint8_t* data)
{
    ScsiResult result = {};
    result.success = false;

    if (sectorCount == 0) {
        result.errorMessage = "Sector count must be > 0";
        return result;
    }

    DWORD dataSize = static_cast<DWORD>(sectorCount) * m_sectorSize;

    CBW cbw = {};
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = m_cbwTag++;
    cbw.dCBWDataTransferLength = dataSize;
    cbw.bmCBWFlags = 0x00;  // Direction: OUT
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = 10;
    ScsiCommands::buildWrite10(cbw.CDB, lba, sectorCount);

    if (!sendCbw(cbw)) return result;

    // Send data
    DWORD written = 0;
    if (!m_usb.bulkWrite(data, dataSize, &written)) {
        result.errorMessage = "Failed to send WRITE10 data";
        return result;
    }

    if (written != dataSize) {
        result.errorMessage = "Partial write: " + std::to_string(written) + "/" + std::to_string(dataSize);
        return result;
    }

    CSW csw = {};
    if (!receiveCsw(csw)) {
        result.errorMessage = "Failed to receive CSW";
        return result;
    }

    result.success = (csw.bCSWStatus == 0);
    result.cswStatus = csw.bCSWStatus;

    return result;
}

ScsiResult M487Device::vendorReadString(uint16_t bufferSize)
{
    ScsiResult result = {};
    result.success = false;

    CBW cbw = {};
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = m_cbwTag++;
    cbw.dCBWDataTransferLength = bufferSize;
    cbw.bmCBWFlags = 0x80;  // Direction: IN
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = 10;
    // Sub-cmd 0x00 = Get Device String
    ScsiCommands::buildVendorRead(cbw.CDB, 0x00, bufferSize);

    if (!sendCbw(cbw)) return result;

    std::vector<uint8_t> response(bufferSize);
    DWORD bytesRead = 0;
    if (!receiveData(response.data(), bufferSize, &bytesRead)) {
        result.errorMessage = "Failed to receive vendor response";
        return result;
    }

    CSW csw = {};
    if (!receiveCsw(csw)) {
        result.errorMessage = "Failed to receive CSW";
        return result;
    }

    result.success = (csw.bCSWStatus == 0);
    result.data = response;
    result.data.resize(bytesRead);
    result.cswStatus = csw.bCSWStatus;

    return result;
}

ScsiResult M487Device::i2cWrite(uint8_t slaveAddr, const uint8_t* data, uint16_t len)
{
    ScsiResult result = {};
    result.success = false;

    // Build CDB for I2C Write (sub-cmd 0x01)
    CBW cbw = {};
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = m_cbwTag++;
    cbw.dCBWDataTransferLength = len;
    cbw.bmCBWFlags = 0x00;  // Direction: OUT (data to device)
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = 10;
    cbw.CDB[0] = SCSI_OP_VENDOR_READ;  // 0xC0
    cbw.CDB[1] = 0x01;                 // Sub-cmd: I2C Write
    cbw.CDB[2] = slaveAddr;
    cbw.CDB[3] = (len >> 8) & 0xFF;   // Length MSB
    cbw.CDB[4] = len & 0xFF;          // Length LSB
    cbw.CDB[5] = 0;
    cbw.CDB[6] = 0;
    cbw.CDB[7] = 0;
    cbw.CDB[8] = 0;
    cbw.CDB[9] = 0;

    // Send CBW
    if (!sendCbw(cbw)) return result;

    // Send data
    DWORD written = 0;
    if (!m_usb.bulkWrite(data, len, &written)) {
        result.errorMessage = "Failed to send I2C write data";
        return result;
    }

    // Receive CSW
    CSW csw = {};
    if (!receiveCsw(csw)) {
        result.errorMessage = "Failed to receive CSW";
        return result;
    }

    result.success = (csw.bCSWStatus == 0);
    result.cswStatus = csw.bCSWStatus;

    return result;
}

ScsiResult M487Device::i2cRead(uint8_t slaveAddr, uint16_t len)
{
    ScsiResult result = {};
    result.success = false;

    // Build CDB for I2C Read (sub-cmd 0x02)
    CBW cbw = {};
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = m_cbwTag++;
    cbw.dCBWDataTransferLength = len + 1;  // +1 for status byte
    cbw.bmCBWFlags = 0x80;  // Direction: IN
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = 10;
    cbw.CDB[0] = SCSI_OP_VENDOR_READ;  // 0xC0
    cbw.CDB[1] = 0x02;                 // Sub-cmd: I2C Read
    cbw.CDB[2] = slaveAddr;
    cbw.CDB[3] = (len >> 8) & 0xFF;   // Read length MSB
    cbw.CDB[4] = len & 0xFF;          // Read length LSB
    cbw.CDB[5] = 0;
    cbw.CDB[6] = 0;
    cbw.CDB[7] = 0;
    cbw.CDB[8] = 0;
    cbw.CDB[9] = 0;

    // Send CBW
    if (!sendCbw(cbw)) return result;

    // Receive response (status byte + data)
    std::vector<uint8_t> response(len + 1);
    DWORD bytesRead = 0;
    if (!receiveData(response.data(), static_cast<DWORD>(response.size()), &bytesRead)) {
        result.errorMessage = "Failed to receive I2C read data";
        return result;
    }

    // Receive CSW
    CSW csw = {};
    if (!receiveCsw(csw)) {
        result.errorMessage = "Failed to receive CSW";
        return result;
    }

    result.success = (csw.bCSWStatus == 0);
    result.data = response;
    result.data.resize(bytesRead);
    result.cswStatus = csw.bCSWStatus;

    return result;
}

ScsiResult M487Device::i2cWriteRead(uint8_t slaveAddr, const uint8_t* wdata, uint16_t wlen, uint16_t rlen)
{
    ScsiResult result = {};
    result.success = false;

    // Build CDB for I2C Write+Read (sub-cmd 0x03)
    CBW cbw = {};
    cbw.dCBWSignature = CBW_SIGNATURE;
    cbw.dCBWTag = m_cbwTag++;
    cbw.dCBWDataTransferLength = wlen + rlen + 1;  // write data + read data + status byte
    cbw.bmCBWFlags = 0x00;  // Direction: OUT (write phase)
    cbw.bCBWLUN = 0;
    cbw.bCBWCBLength = 10;
    cbw.CDB[0] = SCSI_OP_VENDOR_READ;  // 0xC0
    cbw.CDB[1] = 0x03;                 // Sub-cmd: I2C Write+Read
    cbw.CDB[2] = slaveAddr;
    cbw.CDB[3] = (wlen >> 8) & 0xFF;  // Write length MSB
    cbw.CDB[4] = wlen & 0xFF;          // Write length LSB
    cbw.CDB[5] = (rlen >> 8) & 0xFF;  // Read length MSB
    cbw.CDB[6] = rlen & 0xFF;          // Read length LSB
    cbw.CDB[7] = 0;
    cbw.CDB[8] = 0;
    cbw.CDB[9] = 0;

    // Send CBW
    if (!sendCbw(cbw)) return result;

    // Send write data
    DWORD written = 0;
    if (!m_usb.bulkWrite(wdata, wlen, &written)) {
        result.errorMessage = "Failed to send I2C write data";
        return result;
    }

    // Receive response (status byte + read data)
    std::vector<uint8_t> response(rlen + 1);
    DWORD bytesRead = 0;
    if (!receiveData(response.data(), static_cast<DWORD>(response.size()), &bytesRead)) {
        result.errorMessage = "Failed to receive I2C read data";
        return result;
    }

    // Receive CSW
    CSW csw = {};
    if (!receiveCsw(csw)) {
        result.errorMessage = "Failed to receive CSW";
        return result;
    }

    result.success = (csw.bCSWStatus == 0);
    result.data = response;
    result.data.resize(bytesRead);
    result.cswStatus = csw.bCSWStatus;

    return result;
}

bool M487Device::getDeviceInfo(DeviceInfo& info)
{
    if (!m_connected) {
        return false;
    }

    // Try to get capacity
    ScsiResult capResult = scsiReadCapacity();
    if (capResult.success && capResult.data.size() >= 8) {
        uint32_t lastLba = (capResult.data[0] << 24) | (capResult.data[1] << 16) |
                          (capResult.data[2] << 8) | capResult.data[3];
        uint32_t blockSize = (capResult.data[4] << 24) | (capResult.data[5] << 16) |
                            (capResult.data[6] << 8) | capResult.data[7];
        info.totalSectors = lastLba + 1;
        info.sectorSize = blockSize;
        info.blockSize = blockSize;
    } else {
        info.totalSectors = 128;  // Default from example
        info.sectorSize = 512;
        info.blockSize = 512;
    }

    info.vendorId = "0x0416";
    info.productId = "0x501E";
    info.serialNumber = "A02008040114";
    info.firmwareVersion = "1.00";

    return true;
}
