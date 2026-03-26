/**
 * @file     i2c_control.h
 * @brief    M487 USB Storage + I2C Bridge Firmware
 * @version  1.0.0
 * 
 * Vendor Command 0xC0:
 *   Sub-cmd 0x00: Get Device String "ELAN-USB-I2C-BRIDGE-V0.1"
 *   Sub-cmd 0x01: I2C Write (slave_addr + len + data)
 *   Sub-cmd 0x02: I2C Read  (slave_addr + len)
 *   Sub-cmd 0x03: I2C Write+Read (repeated start, slave_addr + wlen + wdata + rlen)
 */

#ifndef __I2C_CONTROL_H__
#define __I2C_CONTROL_H__

#include <stdint.h>

// I2C speed settings (Hz)
#define I2C_SPEED_STANDARD   100000  // 100 kHz
#define I2C_SPEED_FAST       400000  // 400 kHz

// UI2C0 pins: PE2=CLK, PE3=DAT0
#define UI2C0_INIT()         do { \
    CLK->APBCLK1 |= CLK_APBCLK1_USCI0CKEN_Msk; \
    SYS->GPE_MFPL = (SYS->GPE_MFPL & ~(SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk)) | \
                    (SYS_GPE_MFPL_PE2MFP_USCI0_CLK | SYS_GPE_MFPL_PE3MFP_USCI0_DAT0); \
    PE->SMTEN |= GPIO_SMTEN_SMTEN2_Msk; \
} while(0)

/**
 * Initialize UI2C0 at specified clock speed
 */
void I2C_Init(uint32_t u32BusClock);

/**
 * I2C write data to slave
 * @param slaveAddr 7-bit I2C slave address
 * @param data      Pointer to data buffer
 * @param len       Number of bytes to write
 * @return          0=success, negative=error
 */
int32_t I2C_Write(uint8_t slaveAddr, const uint8_t *data, uint16_t len);

/**
 * I2C read data from slave
 * @param slaveAddr 7-bit I2C slave address
 * @param data      Pointer to receive buffer
 * @param len       Number of bytes to read
 * @return          0=success, negative=error
 */
int32_t I2C_Read(uint8_t slaveAddr, uint8_t *data, uint16_t len);

/**
 * I2C write then read (repeated start)
 * @param slaveAddr 7-bit I2C slave address
 * @param wdata     Pointer to write data buffer
 * @param wlen      Number of bytes to write
 * @param rdata     Pointer to receive buffer
 * @param rlen      Number of bytes to read
 * @return          0=success, negative=error
 */
int32_t I2C_WriteRead(uint8_t slaveAddr, const uint8_t *wdata, uint16_t wlen, uint8_t *rdata, uint16_t rlen);

/**
 * Process vendor command from CBW
 * @param cdb Pointer to CDB data
 * @param cbwData Pointer to CBW data buffer
 * @param response Pointer to response buffer
 * @param responseLen Pointer to response length (updated by function)
 * @return 0=success, negative=error
 */
int32_t ProcessVendorCommand(const uint8_t *cdb, const uint8_t *cbwData, 
                              uint8_t *response, uint16_t *responseLen);

#endif // __I2C_CONTROL_H__
