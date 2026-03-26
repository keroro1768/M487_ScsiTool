/**
 * @file     i2c_control.c
 * @brief    M487 I2C Control via USB Vendor Command
 * @version  1.0.0
 */

#include "i2c_control.h"
#include "NuMicro.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define UI2C0_ADDR_TIMEOUT   50000

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static uint32_t s_u32I2CClk;

__weak uint32_t CLK_GetPCLK0Freq(void)
{
    return FREQ_192MHZ / 2;
}

__weak uint32_t CLK_GetPCLK1Freq(void)
{
    return FREQ_192MHZ / 2;
}

static __INLINE void UI2C0_Open(uint32_t u32BusClock)
{
    uint32_t u32ClkDiv;
    uint32_t u32Pclk = CLK_GetPCLK1Freq();
    u32ClkDiv = (uint32_t)((((((u32Pclk / 2U) * 10U) / (u32BusClock)) + 5U) / 10U) - 1U);
    
    /* Enable USCI_I2C protocol */
    UI2C0->CTL &= ~UI2C_CTL_FUNMODE_Msk;
    UI2C0->CTL = 4U << UI2C_CTL_FUNMODE_Pos;
    
    /* Data format: 8 bit data, MSB first */
    UI2C0->LINECTL &= ~UI2C_LINECTL_DWIDTH_Msk;
    UI2C0->LINECTL |= 8U << UI2C_LINECTL_DWIDTH_Pos;
    UI2C0->LINECTL &= ~UI2C_LINECTL_LSB_Msk;
    
    /* Set bus clock */
    UI2C0->BRGEN &= ~UI2C_BRGEN_CLKDIV_Msk;
    UI2C0->BRGEN |= (u32ClkDiv << UI2C_BRGEN_CLKDIV_Pos);
    
    /* Enable I2C protocol */
    UI2C0->PROTCTL |= UI2C_PROTCTL_PROTEN_Msk;
}

void I2C_Init(uint32_t u32BusClock)
{
    UI2C0_INIT();
    UI2C0_Open(u32BusClock);
    s_u32I2CClk = u32BusClock;
}

int32_t I2C_Write(uint8_t slaveAddr, const uint8_t *data, uint16_t len)
{
    uint32_t u32TimeOut;
    
    /* Send START condition */
    UI2C0->PROTCTL |= UI2C_PROTCTL_STAREN_Msk;
    u32TimeOut = UI2C0_ADDR_TIMEOUT;
    
    while((UI2C0->PROTSTS & UI2C_PROTSTS_STARIF_Msk) == 0) {
        if(--u32TimeOut == 0) return -1; // Timeout
    }
    UI2C0->PROTSTS = UI2C_PROTSTS_STARIF_Msk;
    
    /* Send slave address + W bit (0) */
    UI2C0->DEVADDR = (slaveAddr << 1); // 7-bit addr + W bit
    UI2C0->PROTCTL |= UI2C_PROTCTL_AA_Msk;
    u32TimeOut = UI2C0_ADDR_TIMEOUT;
    
    while((UI2C0->PROTSTS & (UI2C_PROTSTS_NACKIF_Msk | UI2C_PROTSTS_ONACKIF_Msk)) == 0) {
        if(--u32TimeOut == 0) return -2; // Timeout
    }
    
    if(UI2C0->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) {
        UI2C0->PROTSTS = UI2C_PROTSTS_NACKIF_Msk;
        UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk; // Send STOP
        return -3; // NACK
    }
    UI2C0->PROTSTS = UI2C_PROTSTS_ONACKIF_Msk;
    
    /* Send data */
    for(uint16_t i = 0; i < len; i++) {
        UI2C0->DAT = data[i];
        UI2C0->PROTCTL |= UI2C_PROTCTL_AA_Msk;
        u32TimeOut = UI2C0_ADDR_TIMEOUT;
        
        while((UI2C0->PROTSTS & (UI2C_PROTSTS_NACKIF_Msk | UI2C_PROTSTS_ONACKIF_Msk)) == 0) {
            if(--u32TimeOut == 0) return -4; // Timeout
        }
        
        if(UI2C0->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) {
            UI2C0->PROTSTS = UI2C_PROTSTS_NACKIF_Msk;
            UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk; // Send STOP
            return -5; // NACK
        }
        UI2C0->PROTSTS = UI2C_PROTSTS_ONACKIF_Msk;
    }
    
    /* Send STOP condition */
    UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk;
    
    return 0; // Success
}

int32_t I2C_Read(uint8_t slaveAddr, uint8_t *data, uint16_t len)
{
    uint32_t u32TimeOut;
    
    /* Send START condition */
    UI2C0->PROTCTL |= UI2C_PROTCTL_STAREN_Msk;
    u32TimeOut = UI2C0_ADDR_TIMEOUT;
    
    while((UI2C0->PROTSTS & UI2C_PROTSTS_STARIF_Msk) == 0) {
        if(--u32TimeOut == 0) return -1; // Timeout
    }
    UI2C0->PROTSTS = UI2C_PROTSTS_STARIF_Msk;
    
    /* Send slave address + R bit (1) */
    UI2C0->DEVADDR = (slaveAddr << 1) | 0x01; // 7-bit addr + R bit
    UI2C0->PROTCTL |= UI2C_PROTCTL_AA_Msk;
    u32TimeOut = UI2C0_ADDR_TIMEOUT;
    
    while((UI2C0->PROTSTS & (UI2C_PROTSTS_NACKIF_Msk | UI2C_PROTSTS_ONACKIF_Msk)) == 0) {
        if(--u32TimeOut == 0) return -2; // Timeout
    }
    
    if(UI2C0->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) {
        UI2C0->PROTSTS = UI2C_PROTSTS_NACKIF_Msk;
        UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk; // Send STOP
        return -3; // NACK
    }
    UI2C0->PROTSTS = UI2C_PROTSTS_ONACKIF_Msk;
    
    /* Receive data */
    for(uint16_t i = 0; i < len; i++) {
        if(i < (len - 1)) {
            UI2C0->PROTCTL |= UI2C_PROTCTL_AA_Msk; // ACK for intermediate bytes
        } else {
            UI2C0->PROTCTL &= ~UI2C_PROTCTL_AA_Msk; // NACK for last byte
        }
        
        u32TimeOut = UI2C0_ADDR_TIMEOUT;
        while((UI2C0->PROTSTS & UI2C_PROTSTS_ONACKIF_Msk) == 0) {
            if(--u32TimeOut == 0) return -4; // Timeout
        }
        UI2C0->PROTSTS = UI2C_PROTSTS_ONACKIF_Msk;
        data[i] = UI2C0->DAT;
    }
    
    /* Send STOP condition */
    UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk;
    
    return 0; // Success
}

int32_t I2C_WriteRead(uint8_t slaveAddr, const uint8_t *wdata, uint16_t wlen, uint8_t *rdata, uint16_t rlen)
{
    uint32_t u32TimeOut;
    
    /* Phase 1: Write */
    if(wlen > 0) {
        /* Send START condition */
        UI2C0->PROTCTL |= UI2C_PROTCTL_STAREN_Msk;
        u32TimeOut = UI2C0_ADDR_TIMEOUT;
        
        while((UI2C0->PROTSTS & UI2C_PROTSTS_STARIF_Msk) == 0) {
            if(--u32TimeOut == 0) return -1;
        }
        UI2C0->PROTSTS = UI2C_PROTSTS_STARIF_Msk;
        
        /* Send slave address + W bit */
        UI2C0->DEVADDR = (slaveAddr << 1);
        UI2C0->PROTCTL |= UI2C_PROTCTL_AA_Msk;
        u32TimeOut = UI2C0_ADDR_TIMEOUT;
        
        while((UI2C0->PROTSTS & (UI2C_PROTSTS_NACKIF_Msk | UI2C_PROTSTS_ONACKIF_Msk)) == 0) {
            if(--u32TimeOut == 0) return -2;
        }
        
        if(UI2C0->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) {
            UI2C0->PROTSTS = UI2C_PROTSTS_NACKIF_Msk;
            UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk;
            return -3;
        }
        UI2C0->PROTSTS = UI2C_PROTSTS_ONACKIF_Msk;
        
        /* Send write data */
        for(uint16_t i = 0; i < wlen; i++) {
            UI2C0->DAT = wdata[i];
            UI2C0->PROTCTL |= UI2C_PROTCTL_AA_Msk;
            u32TimeOut = UI2C0_ADDR_TIMEOUT;
            
            while((UI2C0->PROTSTS & (UI2C_PROTSTS_NACKIF_Msk | UI2C_PROTSTS_ONACKIF_Msk)) == 0) {
                if(--u32TimeOut == 0) return -4;
            }
            
            if(UI2C0->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) {
                UI2C0->PROTSTS = UI2C_PROTSTS_NACKIF_Msk;
                UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk;
                return -5;
            }
            UI2C0->PROTSTS = UI2C_PROTSTS_ONACKIF_Msk;
        }
    }
    
    /* Phase 2: Repeated START + Read */
    if(rlen > 0) {
        /* Send repeated START */
        UI2C0->PROTCTL |= UI2C_PROTCTL_STAREN_Msk;
        u32TimeOut = UI2C0_ADDR_TIMEOUT;
        
        while((UI2C0->PROTSTS & UI2C_PROTSTS_STARIF_Msk) == 0) {
            if(--u32TimeOut == 0) return -6;
        }
        UI2C0->PROTSTS = UI2C_PROTSTS_STARIF_Msk;
        
        /* Send slave address + R bit */
        UI2C0->DEVADDR = (slaveAddr << 1) | 0x01;
        UI2C0->PROTCTL |= UI2C_PROTCTL_AA_Msk;
        u32TimeOut = UI2C0_ADDR_TIMEOUT;
        
        while((UI2C0->PROTSTS & (UI2C_PROTSTS_NACKIF_Msk | UI2C_PROTSTS_ONACKIF_Msk)) == 0) {
            if(--u32TimeOut == 0) return -7;
        }
        
        if(UI2C0->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) {
            UI2C0->PROTSTS = UI2C_PROTSTS_NACKIF_Msk;
            UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk;
            return -8;
        }
        UI2C0->PROTSTS = UI2C_PROTSTS_ONACKIF_Msk;
        
        /* Receive data */
        for(uint16_t i = 0; i < rlen; i++) {
            if(i < (rlen - 1)) {
                UI2C0->PROTCTL |= UI2C_PROTCTL_AA_Msk;
            } else {
                UI2C0->PROTCTL &= ~UI2C_PROTCTL_AA_Msk;
            }
            
            u32TimeOut = UI2C0_ADDR_TIMEOUT;
            while((UI2C0->PROTSTS & UI2C_PROTSTS_ONACKIF_Msk) == 0) {
                if(--u32TimeOut == 0) return -9;
            }
            UI2C0->PROTSTS = UI2C_PROTSTS_ONACKIF_Msk;
            rdata[i] = UI2C0->DAT;
        }
    }
    
    /* Send STOP condition */
    UI2C0->PROTCTL |= UI2C_PROTCTL_STOEN_Msk;
    
    return 0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Vendor Command Handler                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/

// Device string for ELAN-USB-I2C-BRIDGE
static const char s_acDeviceString[] = "ELAN-USB-I2C-BRIDGE-V0.1";

int32_t ProcessVendorCommand(const uint8_t *cdb, const uint8_t *cbwData, 
                              uint8_t *response, uint16_t *responseLen)
{
    uint8_t subCmd = cdb[1];
    int32_t ret = 0;
    
    switch(subCmd) {
        case 0x00: // Get Device String
            *responseLen = sizeof(s_acDeviceString);
            memcpy(response, s_acDeviceString, sizeof(s_acDeviceString));
            break;
            
        case 0x01: // I2C Write
            {
                uint8_t slaveAddr = cdb[2];
                uint16_t dataLen = (cdb[3] << 8) | cdb[4];
                if(dataLen > 64) dataLen = 64; // Limit to 64 bytes
                ret = I2C_Write(slaveAddr, cbwData, dataLen);
                response[0] = (ret == 0) ? 0x00 : 0x01; // 0=OK, 1=Error
                *responseLen = 1;
            }
            break;
            
        case 0x02: // I2C Read
            {
                uint8_t slaveAddr = cdb[2];
                uint16_t readLen = (cdb[3] << 8) | cdb[4];
                if(readLen > 64) readLen = 64; // Limit to 64 bytes
                ret = I2C_Read(slaveAddr, &response[1], readLen);
                response[0] = (ret == 0) ? 0x00 : 0x01; // Status byte first
                *responseLen = readLen + 1;
            }
            break;
            
        case 0x03: // I2C Write+Read (for sensors that need register address)
            {
                uint8_t slaveAddr = cdb[2];
                uint16_t wlen = (cdb[3] << 8) | cdb[4];
                uint16_t rlen = (cdb[5] << 8) | cdb[6];
                if(wlen > 16) wlen = 16;
                if(rlen > 64) rlen = 64;
                
                // cbwData format: [write_data[0], write_data[1], ...]
                ret = I2C_WriteRead(slaveAddr, cbwData, wlen, &response[1], rlen);
                response[0] = (ret == 0) ? 0x00 : 0x01;
                *responseLen = rlen + 1;
            }
            break;
            
        default:
            *responseLen = 0;
            ret = -1;
            break;
    }
    
    return ret;
}
