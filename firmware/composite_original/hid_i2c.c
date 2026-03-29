/**
 * @file     hid_i2c.c
 * @brief    HID I2C Bridge implementation
 * @version  1.0.0
 */

#include "hid_i2c.h"
#include "i2c_control.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static volatile int s_iI2CInitialized = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Initialization (lazy init)                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
static void EnsureI2CInit(void)
{
    if (!s_iI2CInitialized) {
        I2C_Init(I2C_SPEED_STANDARD);  // 100 kHz
        s_iI2CInitialized = 1;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Process HID I2C Reports                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
int32_t HID_I2C_ProcessReport(const uint8_t* reportData, uint16_t reportLen,
                               uint8_t* response, uint16_t* responseLen)
{
    EnsureI2CInit();
    
    uint8_t reportId = reportData[0];
    int32_t ret = 0;
    
    switch (reportId) {
        case HID_REPORT_ID_I2C_WRITE:
            {
                // HID I2C Write Report
                const HID_I2C_WriteReport_t* pWrite = (const HID_I2C_WriteReport_t*)reportData;
                
                if (reportLen < 3) {
                    return -1;  // Invalid report
                }
                
                uint8_t slaveAddr = pWrite->slaveAddr & 0x7F;  // Ensure 7-bit
                uint8_t len = pWrite->length;
                
                if (len > 60) len = 60;
                if (reportLen < (3 + len)) len = reportLen - 3;
                
                ret = I2C_Write(slaveAddr, pWrite->data, len);
                
                // Response: Status only
                response[0] = HID_REPORT_ID_I2C_WRITE;
                response[1] = (ret == 0) ? 0x00 : 0x01;  // 0=OK, 1=Error
                *responseLen = 2;
            }
            break;
            
        case HID_REPORT_ID_I2C_READ:
            {
                // HID I2C Read Report
                const HID_I2C_ReadReport_t* pRead = (const HID_I2C_ReadReport_t*)reportData;
                HID_I2C_ReadResponse_t* pResp = (HID_I2C_ReadResponse_t*)response;
                
                if (reportLen < 3) {
                    return -1;  // Invalid report
                }
                
                uint8_t slaveAddr = pRead->slaveAddr & 0x7F;
                uint8_t len = pRead->length;
                
                if (len > 62) len = 62;
                
                uint8_t readData[64];
                ret = I2C_Read(slaveAddr, readData, len);
                
                // Response: Read data
                pResp->reportId = HID_REPORT_ID_I2C_READ;
                pResp->status = (ret == 0) ? 0x00 : 0x01;
                pResp->length = (ret == 0) ? len : 0;
                
                if (ret == 0) {
                    memcpy(pResp->data, readData, len);
                }
                
                *responseLen = 3 + len;
            }
            break;
            
        case HID_REPORT_ID_I2C_WRITEREAD:
            {
                // HID I2C Write+Read Report
                const HID_I2C_WriteReadReport_t* pWR = (const HID_I2C_WriteReadReport_t*)reportData;
                HID_I2C_WriteReadResponse_t* pResp = (HID_I2C_WriteReadResponse_t*)response;
                
                if (reportLen < 4) {
                    return -1;  // Invalid report
                }
                
                uint8_t slaveAddr = pWR->slaveAddr & 0x7F;
                uint8_t wlen = pWR->writeLen;
                uint8_t rlen = pWR->readLen;
                
                if (wlen > 60) wlen = 60;
                if (rlen > 62) rlen = 62;
                if (reportLen < (4 + wlen)) wlen = reportLen - 4;
                
                uint8_t readData[64];
                ret = I2C_WriteRead(slaveAddr, pWR->writeData, wlen, readData, rlen);
                
                // Response: Read data
                pResp->reportId = HID_REPORT_ID_I2C_WRITEREAD;
                pResp->status = (ret == 0) ? 0x00 : 0x01;
                pResp->length = (ret == 0) ? rlen : 0;
                
                if (ret == 0) {
                    memcpy(pResp->readData, readData, rlen);
                }
                
                *responseLen = 3 + rlen;
            }
            break;
            
        default:
            return -1;  // Unknown report
    }
    
    return 0;  // Response sent
}
