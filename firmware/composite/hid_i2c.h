/**
 * @file     hid_i2c.h
 * @brief    HID I2C Bridge - USB HID interface for I2C control
 * @version  1.0.0
 * 
 * HID Report Descriptors:
 *   Report 0x01: I2C Write  (Host → Device)
 *   Report 0x02: I2C Read   (Host → Device, Device → Host)
 *   Report 0x03: I2C Write+Read (Host → Device, Device → Host)
 */

#ifndef __HID_I2C_H__
#define __HID_I2C_H__

#include <stdint.h>

// HID Report IDs
#define HID_REPORT_ID_I2C_WRITE     0x01
#define HID_REPORT_ID_I2C_READ      0x02
#define HID_REPORT_ID_I2C_WRITEREAD 0x03

// HID I2C Commands
#define HID_I2C_MAX_TRANSFER    64  // Max bytes per transfer

// HID Report structures
#pragma pack(push, 1)

// HID I2C Write Report (Host → Device)
typedef struct {
    uint8_t reportId;       // 0x01
    uint8_t slaveAddr;       // 7-bit I2C slave address
    uint8_t length;         // Number of bytes to write (0-60)
    uint8_t data[60];       // Write data
} HID_I2C_WriteReport_t;

// HID I2C Read Report (Host → Device, triggers read)
typedef struct {
    uint8_t reportId;       // 0x02
    uint8_t slaveAddr;       // 7-bit I2C slave address
    uint8_t length;         // Number of bytes to read (1-64)
} HID_I2C_ReadReport_t;

// HID I2C Read Response Report (Device → Host)
typedef struct {
    uint8_t reportId;       // 0x02
    uint8_t status;         // 0x00=OK, 0x01=Error
    uint8_t length;         // Number of bytes returned
    uint8_t data[62];       // Read data
} HID_I2C_ReadResponse_t;

// HID I2C Write+Read Report (Host → Device)
typedef struct {
    uint8_t reportId;       // 0x03
    uint8_t slaveAddr;       // 7-bit I2C slave address
    uint8_t writeLen;       // Number of bytes to write
    uint8_t readLen;        // Number of bytes to read
    uint8_t writeData[60];  // Write data
} HID_I2C_WriteReadReport_t;

// HID I2C Write+Read Response Report (Device → Host)
typedef struct {
    uint8_t reportId;       // 0x03
    uint8_t status;         // 0x00=OK, 0x01=Error
    uint8_t length;         // Number of bytes returned
    uint8_t readData[62];  // Read data
} HID_I2C_WriteReadResponse_t;

#pragma pack(pop)

/**
 * Initialize HID I2C Bridge
 */
void HID_I2C_Init(void);

/**
 * Process HID report from host
 * @param reportData Pointer to HID report data (excluding report ID)
 * @param reportLen  Length of report data
 * @param response   Pointer to response buffer
 * @param responseLen Pointer to response length (filled by function)
 * @return           0=response sent, negative=no response needed
 */
int32_t HID_I2C_ProcessReport(const uint8_t* reportData, uint16_t reportLen,
                               uint8_t* response, uint16_t* responseLen);

#endif // __HID_I2C_H__
