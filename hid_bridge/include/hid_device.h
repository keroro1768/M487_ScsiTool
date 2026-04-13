/**
 * @file     hid_device.h
 * @brief    M487 USB HID Device - Windows HID API Interface
 * @version  1.0.0
 * 
 * Target: M487 USB Composite Device (VID=0x04F3, PID=0x0732)
 * Interface: HID I2C Bridge (Interface 1)
 * Protocol: Interrupt Endpoint (EP3) with CMD_T packets
 */

#ifndef HID_DEVICE_H
#define HID_DEVICE_H

#include <windows.h>
#include <hidsdi.h>
#include <vector>
#include <string>
#include <cstdint>

/*---------------------------------------------------------------------------------------------------------*/
/* Device Parameters                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define M487_VID                     0x04F3
#define M487_PID                     0x0732
#define HID_REPORT_SIZE              64

/*---------------------------------------------------------------------------------------------------------*/
/* HID I2C Commands (must match firmware hid_i2c.h)                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_CMD_SIGNATURE            0x43444948  /* "HDIB" little-endian */
#define HID_CMD_I2C_WRITE            0x01
#define HID_CMD_I2C_READ             0x02
#define HID_CMD_I2C_WRITEREAD        0x03
#define HID_CMD_I2C_SCAN             0x04
#define HID_CMD_NONE                 0x00

#define I2C_MAX_WRITE_LEN            60
#define I2C_MAX_READ_LEN             62

/*---------------------------------------------------------------------------------------------------------*/
/* HID Report Packet (matches firmware CMD_T)                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#pragma pack(push, 1)
typedef struct {
    uint8_t  reportId;     // Report ID (0x01-0x04)
    uint8_t  size;          // Packet size (total bytes including header)
    uint8_t  data[252];     // Command-specific data
    uint32_t signature;     // Must be HID_CMD_SIGNATURE
    uint32_t checksum;      // Sum of all bytes in packet
} HID_ReportPacket_t;
#pragma pack(pop)

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Report Structures (from firmware)                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#pragma pack(push, 1)

// I2C Write: Host → Device
typedef struct {
    uint8_t reportId;       // 0x01 = I2C Write
    uint8_t slaveAddr;      // 7-bit I2C address (shifted left 1, R/W bit = 0)
    uint8_t length;         // Number of data bytes (max 60)
    uint8_t data[60];       // Data to write
} HID_I2C_WriteReport_t;

// I2C Read Request: Host → Device
typedef struct {
    uint8_t reportId;       // 0x02 = I2C Read
    uint8_t slaveAddr;      // 7-bit I2C address (shifted left 1, R/W bit = 1)
    uint8_t length;         // Number of bytes to read (max 62)
} HID_I2C_ReadRequest_t;

// I2C Read Response: Device → Host
typedef struct {
    uint8_t reportId;       // 0x02
    uint8_t status;        // 0x00 = OK, 0x01 = Error
    uint8_t length;        // Actual bytes read
    uint8_t data[62];      // Read data
} HID_I2C_ReadResponse_t;

// I2C Write+Read: Host → Device
typedef struct {
    uint8_t reportId;       // 0x03 = I2C WriteRead
    uint8_t slaveAddr;      // 7-bit I2C address
    uint8_t writeLen;       // Number of bytes to write (max 60)
    uint8_t readLen;        // Number of bytes to read (max 62)
    uint8_t writeData[60]; // Data to write
} HID_I2C_WriteRead_t;

// I2C Write+Read Response: Device → Host
typedef struct {
    uint8_t reportId;       // 0x03
    uint8_t status;        // 0x00 = OK, 0x01 = Error
    uint8_t length;        // Actual bytes read
    uint8_t readData[62]; // Read data
} HID_I2C_WriteReadResponse_t;

#pragma pack(pop)

/*---------------------------------------------------------------------------------------------------------*/
/* Device Info                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    std::string path;
    std::string description;
    uint16_t vid;
    uint16_t pid;
    uint16_t version;
} HidDeviceInfo;

/*---------------------------------------------------------------------------------------------------------*/
/* M487HidDevice Class                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
class M487HidDevice {
public:
    M487HidDevice();
    ~M487HidDevice();

    // --- Device Enumeration ---
    bool enumerate();
    size_t deviceCount() const { return m_devices.size(); }
    const std::vector<HidDeviceInfo>& getDevices() const { return m_devices; }

    // --- Connection ---
    bool connect(size_t index = 0);
    void disconnect();
    bool isConnected() const { return m_connected; }

    // --- Device Info ---
    bool getDeviceAttributes(HIDD_ATTRIBUTES* attr) const;
    std::string getLastError() const { return m_lastError; }

    // --- HID Operations ---
    /**
     * Send a raw HID output report (interrupt OUT)
     * @param data Buffer containing report (first byte = report ID)
     * @param len  Number of bytes to send
     * @return true if successful
     */
    bool sendOutputReport(const uint8_t* data, size_t len);

    /**
     * Receive a raw HID input report (interrupt IN)
     * @param data Buffer to receive report data
     * @param len  Maximum bytes to receive
     * @param timeoutMs Timeout in milliseconds
     * @return actual bytes received, or 0 on failure
     */
    DWORD receiveInputReport(uint8_t* data, DWORD len, DWORD timeoutMs = 1000);

    // --- I2C Operations (high-level) ---
    /**
     * I2C Write: Send data to I2C slave
     * @param addr 7-bit I2C slave address
     * @param data Data to write
     * @param len  Number of bytes (max 60)
     * @return true if successful
     */
    bool i2cWrite(uint8_t addr, const uint8_t* data, uint16_t len);

    /**
     * I2C Read: Read data from I2C slave
     * @param addr 7-bit I2C slave address
     * @param data Buffer to receive data
     * @param len  Number of bytes to read (max 62)
     * @return true if successful
     */
    bool i2cRead(uint8_t addr, uint8_t* data, uint16_t len);

    /**
     * I2C Write + Read: Combined operation (for register access)
     * @param addr 7-bit I2C slave address
     * @param wdata Data to write first
     * @param wlen  Number of write bytes (max 60)
     * @param rdata Buffer to receive read data
     * @param rlen  Number of bytes to read (max 62)
     * @return true if successful
     */
    bool i2cWriteRead(uint8_t addr, const uint8_t* wdata, uint16_t wlen, uint8_t* rdata, uint16_t rlen);

    /**
     * I2C Scan: Probe I2C bus and return found addresses
     * @param foundAddrs Buffer for found addresses (up to 8)
     * @return number of devices found
     */
    int i2cScan(uint8_t* foundAddrs);

    // --- Test ---
    bool testConnection();

private:
    bool              m_connected;
    HANDLE            m_hidHandle;
    OVERLAPPED        m_readOverlap;
    OVERLAPPED        m_writeOverlap;
    std::vector<HidDeviceInfo> m_devices;
    std::string       m_lastError;

    DWORD calcChecksum(const uint8_t* data, DWORD len);
};

#endif // HID_DEVICE_H
