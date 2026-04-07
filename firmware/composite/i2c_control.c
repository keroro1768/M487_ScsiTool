/**
 * @file     i2c_control.c
 * @brief    I2C Control Driver for M487 — Hardware I2C0 on PG0/PG1
 * @version  4.0.0
 *
 * Hardware: I2C0
 *   PG0 = SCL (Arduino D15)
 *   PG1 = SDA (Arduino D14)
 *   Speed: 100 kHz
 *   Mode: Master, polling
 *
 * Uses BSP I2C polling API (I2C_ReadMultiBytes / I2C_WriteMultiBytes).
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "hid_i2c.h"

/*---------------------------------------------------------------------------------------------------------*/
/* I2C0 Initialize — Hardware I2C on PG0(SCL) / PG1(SDA) (Arduino D15/D14)                                */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_Init(void)
{
    /* Enable I2C0 clock */
    CLK_EnableModuleClock(I2C0_MODULE);

    /* Configure pins: PG0=SCL, PG1=SDA (MFP=0x04) */
    SYS->GPG_MFPL &= ~(SYS_GPG_MFPL_PG0MFP_Msk | SYS_GPG_MFPL_PG1MFP_Msk);
    SYS->GPG_MFPL |= (SYS_GPG_MFPL_PG0MFP_I2C0_SCL | SYS_GPG_MFPL_PG1MFP_I2C0_SDA);

    /* Enable Schmitt trigger for stable I2C signaling */
    PG->SMTEN |= (GPIO_SMTEN_SMTEN0_Msk | GPIO_SMTEN_SMTEN1_Msk);

    /* Open I2C0 at 100kHz */
    I2C_Open(I2C0, 100000);

    printf("I2C0: Hardware I2C @ %d Hz on PG0(SCL)/PG1(SDA) [Arduino D15/D14]\n",
           (int)I2C_GetBusClockFreq(I2C0));
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Write                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
int32_t I2C_Write(uint8_t slaveAddr, const uint8_t *data, uint16_t len)
{
    uint32_t u32Written;

    if (len == 0) return 0;
    if (data == NULL) return -1;
    if (len > I2C_MAX_WRITE_LEN) len = I2C_MAX_WRITE_LEN;

    u32Written = I2C_WriteMultiBytes(I2C0, slaveAddr, (uint8_t *)data, len);

    return (u32Written == len) ? 0 : -1;
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Read (Pure read - no register address phase)                                                        */
/*---------------------------------------------------------------------------------------------------------*/
int32_t I2C_Read(uint8_t slaveAddr, uint8_t *data, uint16_t len)
{
    uint32_t u32Read;

    if (len == 0) return 0;
    if (data == NULL) return -1;
    if (len > I2C_MAX_READ_LEN) len = I2C_MAX_READ_LEN;

    u32Read = I2C_ReadMultiBytes(I2C0, slaveAddr, data, len);

    return (u32Read == len) ? 0 : -1;
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Write + Read (combined: write register addr, then repeated-start read)                              */
/*---------------------------------------------------------------------------------------------------------*/
int32_t I2C_WriteRead(uint8_t slaveAddr, const uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen)
{
    uint32_t u32Read;

    if (wdata == NULL || rdata == NULL) return -1;
    if (wlen > I2C_MAX_WRITE_LEN) wlen = I2C_MAX_WRITE_LEN;
    if (rlen > I2C_MAX_READ_LEN) rlen = I2C_MAX_READ_LEN;

    if (wlen == 1) {
        /* 1-byte register address */
        u32Read = I2C_ReadMultiBytesOneReg(I2C0, slaveAddr, wdata[0], rdata, rlen);
    } else if (wlen == 2) {
        /* 2-byte register address */
        uint16_t regAddr = ((uint16_t)wdata[0] << 8) | wdata[1];
        u32Read = I2C_ReadMultiBytesTwoRegs(I2C0, slaveAddr, regAddr, rdata, rlen);
    } else {
        /* Generic: write first, then separate read */
        uint32_t u32Written = I2C_WriteMultiBytes(I2C0, slaveAddr, (uint8_t *)wdata, wlen);
        if (u32Written != (uint32_t)wlen) return -1;
        u32Read = I2C_ReadMultiBytes(I2C0, slaveAddr, rdata, rlen);
    }

    return (u32Read == (uint32_t)rlen) ? 0 : -1;
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Scan - Probe for devices on the bus                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
int32_t I2C_Scan(uint8_t *foundAddrs, uint8_t maxCount)
{
    int32_t count = 0;
    uint8_t addr;
    uint8_t dummy;

    if (foundAddrs == NULL || maxCount == 0) return 0;

    for (addr = 0x03; addr <= 0x77 && count < maxCount; addr++) {
        /*
         * Probe: try to read 1 byte from the address.
         * If the device ACKs, ReadMultiBytes returns 1.
         * If NACK (no device), it returns 0.
         */
        if (I2C_ReadMultiBytes(I2C0, addr, &dummy, 1) == 1) {
            foundAddrs[count++] = addr;
        }
    }

    return count;
}
