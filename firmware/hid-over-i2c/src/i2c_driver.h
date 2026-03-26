/**
 * @file     i2c_driver.h
 * @brief    M487 UI2C0 I2C Driver for HID-over-I2C Bridge
 * @version  1.0.0
 * 
 * Hardware: UI2C0 (USCI_I2C)
 *   PE2 = CLK (UI2C0_CLK)
 *   PE3 = DAT0 (UI2C0_DAT0)
 *   Speed: 100 kHz (default), up to 400 kHz
 *   Mode: Master only
 * 
 * Features:
 *   - Blocking and interrupt-driven I2C transactions
 *   - Bus scanning
 *   - Timeout protection
 *   - NACK retry (up to 3 attempts)
 */

#ifndef __I2C_DRIVER_H__
#define __I2C_DRIVER_H__

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Configuration                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define I2C0_SPEED           100000   /* 100 kHz default */
#define I2C0_TIMEOUT        10000    /* 10ms timeout per transaction */
#define I2C0_NACK_RETRY     3        /* Retry count on NACK */
#define I2C0_CLOCK_STRETCH  10000    /* Max clock stretch in us */

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Return Codes                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define I2C_OK              0        /* Success */
#define I2C_ERR_NACK       -1       /* NACK received */
#define I2C_ERR_TIMEOUT    -2       /* Transaction timeout */
#define I2C_ERR_ARBLOST    -3       /* Arbitration lost */
#define I2C_ERR_BUS        -4       /* Bus error */
#define I2C_ERR_PARAM      -5       /* Invalid parameter */
#define I2C_ERR_BUSY       -6       /* I2C bus busy */

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Bus Scan Results                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define I2C_SCAN_MAX_DEVICES  8      /* Maximum devices to report */

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Initialize UI2C0 as I2C master
 * @return  I2C_OK on success, error code on failure
 * 
 * Configures:
 *   - PE2 = UI2C0_CLK (open-drain, pull-up)
 *   - PE3 = UI2C0_DAT0 (open-drain, pull-up)
 *   - Clock: PCLK0 / (2 * (CLKDIV + 1))
 *   - Default speed: 100 kHz
 *   - 7-bit address mode
 */
int32_t I2C0_Init(void);

/**
 * @brief   Deinitialize UI2C0
 * @return  None
 */
void I2C0_DeInit(void);

/**
 * @brief   Set I2C bus clock frequency
 * @param   speed  Target frequency in Hz (e.g. 100000, 400000)
 * @return  I2C_OK on success, error code on failure
 */
int32_t I2C0_SetSpeed(uint32_t speed);

/**
 * @brief   Get current I2C bus speed
 * @return  Current bus frequency in Hz
 */
uint32_t I2C0_GetSpeed(void);

/**
 * @brief   Write data to I2C device register (blocking)
 * @param   addr     7-bit I2C device address
 * @param   reg      Register index to write
 * @param   data     Pointer to data buffer
 * @param   len      Number of bytes to write
 * @return  I2C_OK on success, error code on failure
 * 
 * Sequence: START -> ADDR+W -> REG -> DATA... -> STOP
 */
int32_t I2C0_WriteReg(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len);

/**
 * @brief   Read data from I2C device register (blocking)
 * @param   addr     7-bit I2C device address
 * @param   reg      Register index to read
 * @param   data     Pointer to receive buffer
 * @param   len      Number of bytes to read
 * @return  I2C_OK on success, error code on failure
 * 
 * Sequence: START -> ADDR+W -> REG -> START -> ADDR+R -> DATA... -> STOP
 */
int32_t I2C0_ReadReg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

/**
 * @brief   Write data to I2C device without register address (blocking)
 * @param   addr     7-bit I2C device address
 * @param   data     Pointer to data buffer
 * @param   len      Number of bytes to write
 * @return  I2C_OK on success, error code on failure
 * 
 * Sequence: START -> ADDR+W -> DATA... -> STOP
 */
int32_t I2C0_Write(uint8_t addr, const uint8_t *data, uint16_t len);

/**
 * @brief   Read data from I2C device without register address (blocking)
 * @param   addr     7-bit I2C device address
 * @param   data     Pointer to receive buffer
 * @param   len      Number of bytes to read
 * @return  I2C_OK on success, error code on failure
 * 
 * Sequence: START -> ADDR+R -> DATA... -> STOP
 */
int32_t I2C0_Read(uint8_t addr, uint8_t *data, uint16_t len);

/**
 * @brief   Combined Write+Read (Write register address, then read data)
 * @param   addr     7-bit I2C device address
 * @param   reg      Register index
 * @param   wdata    Pointer to write buffer (can be NULL if wlen=0)
 * @param   wlen     Number of bytes to write
 * @param   rdata    Pointer to receive buffer
 * @param   rlen     Number of bytes to read
 * @return  I2C_OK on success, error code on failure
 * 
 * Sequence: START -> ADDR+W -> reg -> wdata... -> START -> ADDR+R -> rdata... -> STOP
 */
int32_t I2C0_WriteRead(uint8_t addr, uint8_t reg, const uint8_t *wdata, uint16_t wlen,
                        uint8_t *rdata, uint16_t rlen);

/**
 * @brief   Scan I2C bus for responding devices
 * @param   foundAddrs   Pointer to array to store found addresses
 * @param   maxCount     Maximum number of addresses to store
 * @return  Number of devices found (0 to maxCount)
 * 
 * Probes addresses 0x03 to 0x77.
 * A device is considered found if it ACKs its address.
 */
int32_t I2C0_Scan(uint8_t *foundAddrs, uint8_t maxCount);

/**
 * @brief   Check if I2C bus is idle
 * @return  1 if idle, 0 if busy
 */
uint8_t I2C0_IsBusIdle(void);

/**
 * @brief   Reset I2C bus (recover from stuck state)
 * @return  None
 * 
 * Toggles clock line 9 times to clear stuck slaves.
 */
void I2C0_ResetBus(void);

/**
 * @brief   Probe a single I2C address
 * @param   addr     7-bit I2C device address
 * @return  I2C_OK if device responds with ACK, error code otherwise
 */
int32_t I2C0_Probe(uint8_t addr);

#endif /* __I2C_DRIVER_H__ */
