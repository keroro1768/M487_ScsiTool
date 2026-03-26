/**
 * @file     bridge.h
 * @brief    HID-over-I2C Bridge Core - Translation Layer
 * @version  1.0.0
 * 
 * Bridges USB HID requests to HID-over-I2C register operations.
 */

#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#include "hid_parser.h"
#include "usb_hid.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Bridge State                                                                                            */
/*---------------------------------------------------------------------------------------------------------*/

typedef enum {
    BRIDGE_STATE_DETACHED = 0,
    BRIDGE_STATE_CONNECTING,
    BRIDGE_STATE_CONNECTED,
    BRIDGE_STATE_ERROR
} BRIDGE_STATE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Bridge Statistics                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    uint32_t inputReportsSent;
    uint32_t outputReportsReceived;
    uint32_t getReportCount;
    uint32_t setReportCount;
    uint32_t i2cErrors;
    uint32_t usbErrors;
} Bridge_Stats_t;

/*---------------------------------------------------------------------------------------------------------*/
/* Bridge Configuration                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    uint8_t  i2cDeviceAddr;          /* 7-bit I2C address */
    uint8_t  enableInterrupt;         /* Use GPIO interrupt for input reports */
    uint8_t  gpioIntPin;              /* GPIO pin for I2C device interrupt */
} Bridge_Config_t;

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Initialize HID-over-I2C bridge
 * @param   pConfig    Bridge configuration
 * @return  0 on success, negative on error
 */
int32_t Bridge_Init(const Bridge_Config_t *pConfig);

/**
 * @brief   Connect to I2C HID device and enumerate
 * @return  0 on success, negative on error
 * 
 * Performs:
 * 1. I2C device probe
 * 2. Read HID Descriptor
 * 3. Read Report Descriptor
 * 4. Reset device
 */
int32_t Bridge_Connect(void);

/**
 * @brief   Disconnect from I2C HID device
 * @return  None
 */
void Bridge_Disconnect(void);

/**
 * @brief   Get current bridge state
 * @return  Current state
 */
BRIDGE_STATE_T Bridge_GetState(void);

/**
 * @brief   Get device statistics
 * @param   pStats    Pointer to statistics structure
 * @return  None
 */
void Bridge_GetStats(Bridge_Stats_t *pStats);

/**
 * @brief   Reset statistics
 * @return  None
 */
void Bridge_ResetStats(void);

/**
 * @brief   Handle GET_REPORT from USB host
 * @param   reportID    Report ID
 * @param   reportType  Report type (INPUT/OUTPUT/FEATURE)
 * @param   pBuf       Buffer for report data
 * @param   pLen       [in/out] Buffer size / report length
 * @return  0 on success
 * 
 * Called by USB_HID when host sends GET_REPORT.
 * Bridges to I2C device: Write Command Register -> Read Data Register
 */
int32_t Bridge_GetReport(uint8_t reportID, uint8_t reportType, uint8_t *pBuf, uint16_t *pLen);

/**
 * @brief   Handle SET_REPORT from USB host
 * @param   reportID    Report ID
 * @param   reportType  Report type (INPUT/OUTPUT/FEATURE)
 * @param   pBuf       Report data
 * @param   len        Report length
 * @return  0 on success
 * 
 * Called by USB_HID when host sends SET_REPORT.
 * Bridges to I2C device: Write Data Register -> Write Command Register
 */
int32_t Bridge_SetReport(uint8_t reportID, uint8_t reportType, const uint8_t *pBuf, uint16_t len);

/**
 * @brief   Handle Input Report from I2C device (called from GPIO interrupt)
 * @return  None
 * 
 * Reads Input Register from I2C device and forwards to USB host via EP1.
 */
void Bridge_OnI2CInterrupt(void);

/**
 * @brief   Send raw command to I2C device
 * @param   opcode      HID-I2C opcode
 * @param   reportType  Report type
 * @param   reportID    Report ID
 * @return  0 on success
 */
int32_t Bridge_SendCommand(uint8_t opcode, uint8_t reportType, uint8_t reportID);

/**
 * @brief   Read data from I2C device Data Register
 * @param   pBuf       Buffer for data
 * @param   maxLen    Maximum bytes to read
 * @return  Actual bytes read, negative on error
 */
int32_t Bridge_ReadData(uint8_t *pBuf, uint16_t maxLen);

/**
 * @brief   Write data to I2C device Data Register
 * @param   pBuf       Data to write
 * @param   len        Number of bytes
 * @return  0 on success
 */
int32_t Bridge_WriteData(const uint8_t *pBuf, uint16_t len);

#endif /* __BRIDGE_H__ */
