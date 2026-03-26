/**
 * @file     bridge.c
 * @brief    HID-over-I2C Bridge Core Implementation
 * @version  1.0.0
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "i2c_driver.h"
#include "hid_parser.h"
#include "usb_hid.h"
#include "bridge.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Private Variables                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/

static HID_Context_t s_HIDCtx;
static Bridge_Config_t s_Config;
static Bridge_Stats_t s_Stats;
static BRIDGE_STATE_T s_State = BRIDGE_STATE_DETACHED;

/*---------------------------------------------------------------------------------------------------------*/
/* Forward Declarations                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
static int32_t Bridge_ResetI2CDevice(void);
static int32_t BuildCommandRegister(uint8_t opcode, uint8_t reportType, uint8_t reportID, uint8_t *pCmd);

/*---------------------------------------------------------------------------------------------------------*/
/* Command Register Helper                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Build command register value per HID-over-I2C spec
 * 
 * Command Register format (2 bytes):
 *   Byte 0: [Reserved:4][Opcode:4]
 *   Byte 1: [Reserved:2][ReportType:2][ReportID:4]
 */
static int32_t BuildCommandRegister(uint8_t opcode, uint8_t reportType, uint8_t reportID, uint8_t *pCmd)
{
    uint8_t byte0, byte1;
    
    if (pCmd == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    /* Byte 0: Opcode */
    byte0 = (opcode & 0x0F) << 4;
    
    /* Byte 1: Report Type + Report ID */
    byte1 = ((reportType & 0x03) << 2) | (reportID & 0x0F);
    
    pCmd[0] = byte0;
    pCmd[1] = byte1;
    
    return HID_PARSER_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Bridge Implementation                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/

int32_t Bridge_Init(const Bridge_Config_t *pConfig)
{
    USB_HID_Config_t usbConfig;
    
    if (pConfig == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    memset(&s_Stats, 0, sizeof(s_Stats));
    s_State = BRIDGE_STATE_DETACHED;
    
    /* Save config */
    s_Config = *pConfig;
    
    /* Initialize I2C */
    I2C0_Init();
    
    /* Initialize HID context */
    HID_Parser_Init(&s_HIDCtx, s_Config.i2cDeviceAddr);
    
    /* Initialize USB HID with callbacks */
    memset(&usbConfig, 0, sizeof(usbConfig));
    usbConfig.vid = USBD_VID;
    usbConfig.pid = USBD_PID;
    usbConfig.maxPower = 100;
    usbConfig.onGetReport = Bridge_GetReport;
    usbConfig.onSetReport = Bridge_SetReport;
    
    USB_HID_Init(&usbConfig);
    
    /* Configure GPIO for I2C device interrupt if enabled */
    if (s_Config.enableInterrupt) {
        /* Configure GPIO as input with pull-up */
        uint32_t gpioBase;
        uint32_t pinMask;
        
        /* Map pin number to GPIO base and mask */
        gpioBase = 0x40040000; /* Default to GPIOA */
        pinMask = (1 << s_Config.gpioIntPin);
        
        /* Set as input */
        *(volatile uint32_t *)(gpioBase + 0x04) &= ~pinMask;
        
        /* Enable pull-up */
        *(volatile uint32_t *)(gpioBase + 0x0C) |= pinMask;
        
        /* Enable falling edge interrupt */
        *(volatile uint32_t *)(gpioBase + 0x18) &= ~pinMask; /* Falling edge */
        *(volatile uint32_t *)(gpioBase + 0x20) |= pinMask;   /* Enable interrupt */
    }
    
    return HID_PARSER_OK;
}

int32_t Bridge_Connect(void)
{
    int32_t ret;
    
    s_State = BRIDGE_STATE_CONNECTING;
    
    /* Probe I2C device */
    ret = I2C0_Probe(s_Config.i2cDeviceAddr);
    if (ret != I2C_OK) {
        s_State = BRIDGE_STATE_ERROR;
        s_Stats.i2cErrors++;
        return ret;
    }
    
    /* Read HID Descriptor */
    ret = HID_Parser_ReadDescriptor(&s_HIDCtx);
    if (ret != HID_PARSER_OK) {
        s_State = BRIDGE_STATE_ERROR;
        s_Stats.i2cErrors++;
        return ret;
    }
    
    /* Read Report Descriptor */
    ret = HID_Parser_ReadReportDescriptor(&s_HIDCtx);
    if (ret != HID_PARSER_OK) {
        s_State = BRIDGE_STATE_ERROR;
        s_Stats.i2cErrors++;
        return ret;
    }
    
    /* Reset I2C device */
    ret = Bridge_ResetI2CDevice();
    if (ret != HID_PARSER_OK) {
        s_State = BRIDGE_STATE_ERROR;
        s_Stats.i2cErrors++;
        return ret;
    }
    
    s_State = BRIDGE_STATE_CONNECTED;
    
    return HID_PARSER_OK;
}

void Bridge_Disconnect(void)
{
    s_State = BRIDGE_STATE_DETACHED;
    HID_Parser_Reset(&s_HIDCtx);
}

BRIDGE_STATE_T Bridge_GetState(void)
{
    return s_State;
}

void Bridge_GetStats(Bridge_Stats_t *pStats)
{
    if (pStats != NULL)
        memcpy(pStats, &s_Stats, sizeof(Bridge_Stats_t));
}

void Bridge_ResetStats(void)
{
    memset(&s_Stats, 0, sizeof(s_Stats));
}

int32_t Bridge_ResetI2CDevice(void)
{
    uint8_t cmd[2];
    int32_t ret;
    
    /* Build RESET command */
    ret = BuildCommandRegister(HID_I2C_OP_RESET, 0, 0, cmd);
    if (ret != HID_PARSER_OK)
        return ret;
    
    /* Write to Command Register */
    ret = I2C0_WriteReg(s_Config.i2cDeviceAddr, 
                         s_HIDCtx.regCommand, cmd, 2);
    if (ret != I2C_OK) {
        s_Stats.i2cErrors++;
        return ret;
    }
    
    /* Wait for device to reset */
    volatile uint32_t delay;
    for (delay = 0; delay < 10000; delay++);
    
    return HID_PARSER_OK;
}

int32_t Bridge_GetReport(uint8_t reportID, uint8_t reportType, uint8_t *pBuf, uint16_t *pLen)
{
    uint8_t cmd[2];
    uint8_t dataBuf[64];
    uint16_t maxLen;
    int32_t ret, readLen;
    
    if (pLen == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    maxLen = *pLen;
    *pLen = 0;
    
    if (s_State != BRIDGE_STATE_CONNECTED)
        return HID_PARSER_ERR_NULL_PTR;
    
    s_Stats.getReportCount++;
    
    /* Build GET_REPORT command */
    ret = BuildCommandRegister(HID_I2C_OP_GET_REPORT, reportType, reportID, cmd);
    if (ret != HID_PARSER_OK)
        return ret;
    
    /* Write Command Register */
    ret = I2C0_WriteReg(s_Config.i2cDeviceAddr, s_HIDCtx.regCommand, cmd, 2);
    if (ret != I2C_OK) {
        s_Stats.i2cErrors++;
        return ret;
    }
    
    /* Small delay for device to prepare data */
    volatile uint32_t d;
    for (d = 0; d < 1000; d++);
    
    /* Read Data Register (with length prefix) */
    readLen = I2C0_ReadReg(s_Config.i2cDeviceAddr, s_HIDCtx.regData, dataBuf, 64);
    if (readLen < 0) {
        s_Stats.i2cErrors++;
        return readLen;
    }
    
    /* Skip length prefix (2 bytes) and copy data */
    if (readLen > 2) {
        readLen -= 2;  /* Subtract length prefix */
        if ((uint16_t)readLen > maxLen)
            readLen = maxLen;
        memcpy(pBuf, &dataBuf[2], readLen);
        *pLen = (uint16_t)readLen;
    }
    
    s_Stats.inputReportsSent++;
    
    return HID_PARSER_OK;
}

int32_t Bridge_SetReport(uint8_t reportID, uint8_t reportType, const uint8_t *pBuf, uint16_t len)
{
    uint8_t cmd[2];
    uint8_t dataBuf[66];
    int32_t ret;
    
    if (s_State != BRIDGE_STATE_CONNECTED)
        return HID_PARSER_ERR_NULL_PTR;
    
    if (len > HID_OUT_REPORT_SIZE)
        len = HID_OUT_REPORT_SIZE;
    
    s_Stats.setReportCount++;
    
    /* Prepare data: [Length_MSB][Length_LSB][report...] */
    dataBuf[0] = (uint8_t)((len + 2) >> 8);   /* MSB of length */
    dataBuf[1] = (uint8_t)((len + 2) & 0xFF);  /* LSB of length */
    memcpy(&dataBuf[2], pBuf, len);
    
    /* Write Data Register first */
    ret = I2C0_WriteReg(s_Config.i2cDeviceAddr, s_HIDCtx.regData, dataBuf, len + 2);
    if (ret != I2C_OK) {
        s_Stats.i2cErrors++;
        return ret;
    }
    
    /* Build SET_REPORT command */
    ret = BuildCommandRegister(HID_I2C_OP_SET_REPORT, reportType, reportID, cmd);
    if (ret != HID_PARSER_OK)
        return ret;
    
    /* Write Command Register */
    ret = I2C0_WriteReg(s_Config.i2cDeviceAddr, s_HIDCtx.regCommand, cmd, 2);
    if (ret != I2C_OK) {
        s_Stats.i2cErrors++;
        return ret;
    }
    
    s_Stats.outputReportsReceived++;
    
    return HID_PARSER_OK;
}

void Bridge_OnI2CInterrupt(void)
{
    uint8_t inputBuf[64];
    int32_t len;
    
    if (s_State != BRIDGE_STATE_CONNECTED)
        return;
    
    /* Read Input Register from I2C device */
    len = I2C0_ReadReg(s_Config.i2cDeviceAddr, s_HIDCtx.regInput, inputBuf, 64);
    if (len < 0) {
        s_Stats.i2cErrors++;
        return;
    }
    
    /* Skip length prefix and send to USB */
    if (len > 2) {
        len -= 2;
        USB_HID_SendInputReport(&inputBuf[2], (uint16_t)len);
        s_Stats.inputReportsSent++;
    }
}

int32_t Bridge_SendCommand(uint8_t opcode, uint8_t reportType, uint8_t reportID)
{
    uint8_t cmd[2];
    int32_t ret;
    
    if (s_State != BRIDGE_STATE_CONNECTED)
        return HID_PARSER_ERR_NULL_PTR;
    
    ret = BuildCommandRegister(opcode, reportType, reportID, cmd);
    if (ret != HID_PARSER_OK)
        return ret;
    
    ret = I2C0_WriteReg(s_Config.i2cDeviceAddr, s_HIDCtx.regCommand, cmd, 2);
    if (ret != I2C_OK)
        s_Stats.i2cErrors++;
    
    return ret;
}

int32_t Bridge_ReadData(uint8_t *pBuf, uint16_t maxLen)
{
    int32_t len;
    
    if (pBuf == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    if (s_State != BRIDGE_STATE_CONNECTED)
        return HID_PARSER_ERR_NULL_PTR;
    
    len = I2C0_ReadReg(s_Config.i2cDeviceAddr, s_HIDCtx.regData, pBuf, maxLen);
    if (len < 0)
        s_Stats.i2cErrors++;
    
    return len;
}

int32_t Bridge_WriteData(const uint8_t *pBuf, uint16_t len)
{
    int32_t ret;
    
    if (s_State != BRIDGE_STATE_CONNECTED)
        return HID_PARSER_ERR_NULL_PTR;
    
    ret = I2C0_WriteReg(s_Config.i2cDeviceAddr, s_HIDCtx.regData, pBuf, len);
    if (ret != I2C_OK)
        s_Stats.i2cErrors++;
    
    return ret;
}
