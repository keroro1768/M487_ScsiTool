/**
 * @file     i2c_driver.c
 * @brief    M487 UI2C0 I2C Driver Implementation
 * @version  1.0.0
 * 
 * This is a clean I2C master driver for the HID-over-I2C bridge.
 * Uses polling mode for simplicity and determinism.
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "i2c_driver.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Macro Definitions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

/* UI2C0 register base */
#define UI2C0_BASE   ((UI2C_T *) ((uint32_t)UI2C0_BASE_ADDR))

/* Current bus speed (set by I2C0_SetSpeed) */
static uint32_t s_u32CurrentSpeed = I2C0_SPEED;

/*---------------------------------------------------------------------------------------------------------*/
/* Internal Functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Wait for UI2C protocol flag with timeout
 * @param   ui2c        UI2C peripheral base
 * @param   flag        Flag to wait for (e.g. UI2C_PROTSTS_STARIF_Msk)
 * @param   timeout     Timeout in iterations
 * @return  0 on success, -I2C_ERR_TIMEOUT on timeout
 */
static int32_t WaitForFlag(UI2C_T *ui2c, uint32_t flag, uint32_t timeout)
{
    while (timeout-- > 0) {
        if (ui2c->PROTSTS & flag)
            return I2C_OK;
        /* Also check timeout flag */
        if (ui2c->PROTSTS & UI2C_PROTSTS_TOIF_Msk) {
            ui2c->PROTSTS = UI2C_PROTSTS_TOIF_Msk;  /* Clear timeout flag */
            return I2C_ERR_TIMEOUT;
        }
    }
    return I2C_ERR_TIMEOUT;
}

/**
 * @brief   Clear all protocol status flags
 */
static void ClearProtocolFlags(UI2C_T *ui2c)
{
    volatile uint32_t sts = ui2c->PROTSTS;
    (void)sts;
    ui2c->PROTSTS = 0xFF;  /* Write 1 to clear */
}

/**
 * @brief   Send START condition
 */
static int32_t SendStart(UI2C_T *ui2c)
{
    int32_t ret;
    
    /* Check if bus is free */
    if (!(ui2c->PROTSTS & UI2C_PROTSTS_BUSYIF_Msk)) {
        /* Bus might be stuck, try to recover */
        ClearProtocolFlags(ui2c);
    }
    
    /* Generate START */
    ui2c->PROTCTL = (ui2c->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    
    ret = WaitForFlag(ui2c, UI2C_PROTSTS_STARIF_Msk, I2C0_TIMEOUT);
    if (ret != I2C_OK) {
        ClearProtocolFlags(ui2c);
        return ret;
    }
    
    /* Clear STARIF */
    ui2c->PROTSTS = UI2C_PROTSTS_STARIF_Msk;
    
    return I2C_OK;
}

/**
 * @brief   Send STOP condition
 */
static int32_t SendStop(UI2C_T *ui2c)
{
    int32_t ret;
    
    ui2c->PROTCTL = (ui2c->PROTCTL & ~0x2E) | (UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_STO_Msk);
    
    ret = WaitForFlag(ui2c, UI2C_PROTSTS_STORIF_Msk, I2C0_TIMEOUT);
    if (ret != I2C_OK) {
        ClearProtocolFlags(ui2c);
        return ret;
    }
    
    /* Clear STORIF */
    ui2c->PROTSTS = UI2C_PROTSTS_STORIF_Msk;
    
    return I2C_OK;
}

/**
 * @brief   Send address + R/W bit, wait for ACK/NACK
 * @param   ui2c    UI2C peripheral
 * @param   addr    7-bit address
 * @param   read    1 = read, 0 = write
 * @return  I2C_OK if ACK, I2C_ERR_NACK if NACK
 */
static int32_t SendAddress(UI2C_T *ui2c, uint8_t addr, uint8_t read)
{
    int32_t ret;
    uint8_t addrByte = (uint8_t)((addr << 1) | (read & 0x01));
    
    /* Send address byte */
    ui2c->TXDAT = addrByte;
    ui2c->PROTCTL = (ui2c->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    
    /* Wait for ACK/NACK */
    ret = WaitForFlag(ui2c, UI2C_PROTSTS_ACKIF_Msk | UI2C_PROTSTS_NACKIF_Msk, I2C0_TIMEOUT);
    if (ret != I2C_OK)
        return ret;
    
    /* Check if NACK */
    if (ui2c->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) {
        ui2c->PROTSTS = UI2C_PROTSTS_NACKIF_Msk;  /* Clear NACK flag */
        return I2C_ERR_NACK;
    }
    
    /* Clear ACK flag */
    ui2c->PROTSTS = UI2C_PROTSTS_ACKIF_Msk;
    
    return I2C_OK;
}

/**
 * @brief   Send one data byte, wait for ACK/NACK
 * @param   ui2c    UI2C peripheral
 * @param   data    Byte to send
 * @return  I2C_OK if ACK, I2C_ERR_NACK if NACK
 */
static int32_t SendByte(UI2C_T *ui2c, uint8_t data)
{
    int32_t ret;
    
    ui2c->TXDAT = data;
    ui2c->PROTCTL = (ui2c->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    
    ret = WaitForFlag(ui2c, UI2C_PROTSTS_ACKIF_Msk | UI2C_PROTSTS_NACKIF_Msk, I2C0_TIMEOUT);
    if (ret != I2C_OK)
        return ret;
    
    if (ui2c->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) {
        ui2c->PROTSTS = UI2C_PROTSTS_NACKIF_Msk;
        return I2C_ERR_NACK;
    }
    
    ui2c->PROTSTS = UI2C_PROTSTS_ACKIF_Msk;
    
    return I2C_OK;
}

/**
 * @brief   Receive one byte and send ACK or NACK
 * @param   ui2c    UI2C peripheral
 * @param   ack     1 = send ACK, 0 = send NACK (for last byte)
 * @return  Received byte
 */
static uint8_t ReceiveByte(UI2C_T *ui2c, uint8_t ack)
{
    /* Trigger receive */
    ui2c->PROTCTL = (ui2c->PROTCTL & ~0x2E) | 
                     UI2C_PROTCTL_PTRG_Msk | 
                     (ack ? UI2C_PROTCTL_AA_Msk : 0);
    
    /* Wait for data */
    uint32_t timeout = I2C0_TIMEOUT;
    while (timeout-- > 0) {
        if (ui2c->PROTSTS & UI2C_PROTSTS_ACKIF_Msk)
            break;
        if (ui2c->PROTSTS & UI2C_PROTSTS_TOIF_Msk) {
            ui2c->PROTSTS = UI2C_PROTSTS_TOIF_Msk;
            return 0;
        }
    }
    
    ui2c->PROTSTS = UI2C_PROTSTS_ACKIF_Msk;
    
    return (uint8_t)(ui2c->RXDAT & 0xFF);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

int32_t I2C0_Init(void)
{
    /* Enable UI2C0 clock */
    CLK_EnableModuleClock(USCI0_MODULE);
    
    /* Configure pins: PE2=CLK, PE3=DAT0 */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE2MFP_USCI0_CLK | SYS_GPE_MFPL_PE3MFP_USCI0_DAT0);
    
    /* Enable Schmitt trigger for stable I2C signaling */
    PE->SMTEN |= (GPIO_SMTEN_SMTEN2_Msk | GPIO_SMTEN_SMTEN3_Msk);
    
    /* Configure PE2, PE3 as open-drain output */
    PE->MODE = (PE->MODE & ~0xF0) | (0x3 << 8) | (0x3 << 12);  /* PE2, PE3 open-drain */
    
    /* Open UI2C0 at specified speed */
    UI2C_Open(UI2C0_BASE, I2C0_SPEED);
    
    /* Enable clock stretch */
    UI2C0_BASE->BRGEN |= UI2C_BRGEN_CLKSEL_Msk;
    
    /* Clear any pending flags */
    ClearProtocolFlags(UI2C0_BASE);
    
    s_u32CurrentSpeed = I2C0_SPEED;
    
    return I2C_OK;
}

void I2C0_DeInit(void)
{
    UI2C_Close(UI2C0_BASE);
    CLK_DisableModuleClock(USCI0_MODULE);
}

int32_t I2C0_SetSpeed(uint32_t speed)
{
    uint32_t actual;
    
    if (speed == 0)
        return I2C_ERR_PARAM;
    
    actual = UI2C_SetBusClockFreq(UI2C0_BASE, speed);
    s_u32CurrentSpeed = actual;
    
    return I2C_OK;
}

uint32_t I2C0_GetSpeed(void)
{
    return s_u32CurrentSpeed;
}

int32_t I2C0_WriteReg(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    int32_t ret;
    uint16_t i;
    
    if (len > 0 && data == NULL)
        return I2C_ERR_PARAM;
    
    /* START */
    ret = SendStart(UI2C0_BASE);
    if (ret != I2C_OK) goto done;
    
    /* Address + Write */
    ret = SendAddress(UI2C0_BASE, addr, 0);
    if (ret != I2C_OK) goto done;
    
    /* Register index */
    ret = SendByte(UI2C0_BASE, reg);
    if (ret != I2C_OK) goto done;
    
    /* Data bytes */
    for (i = 0; i < len; i++) {
        ret = SendByte(UI2C0_BASE, data[i]);
        if (ret != I2C_OK) goto done;
    }
    
done:
    SendStop(UI2C0_BASE);
    ClearProtocolFlags(UI2C0_BASE);
    return ret;
}

int32_t I2C0_ReadReg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    int32_t ret;
    uint16_t i;
    
    if (len > 0 && data == NULL)
        return I2C_ERR_PARAM;
    
    /* START */
    ret = SendStart(UI2C0_BASE);
    if (ret != I2C_OK) goto done;
    
    /* Address + Write */
    ret = SendAddress(UI2C0_BASE, addr, 0);
    if (ret != I2C_OK) goto done;
    
    /* Register index */
    ret = SendByte(UI2C0_BASE, reg);
    if (ret != I2C_OK) goto done;
    
    /* Repeated START */
    ret = SendStart(UI2C0_BASE);
    if (ret != I2C_OK) goto done;
    
    /* Address + Read */
    ret = SendAddress(UI2C0_BASE, addr, 1);
    if (ret != I2C_OK) goto done;
    
    /* Receive data bytes */
    for (i = 0; i < len; i++) {
        uint8_t ack = (i < (len - 1)) ? 1 : 0;  /* ACK all except last */
        data[i] = ReceiveByte(UI2C0_BASE, ack);
    }
    
done:
    SendStop(UI2C0_BASE);
    ClearProtocolFlags(UI2C0_BASE);
    return ret;
}

int32_t I2C0_Write(uint8_t addr, const uint8_t *data, uint16_t len)
{
    int32_t ret;
    uint16_t i;
    
    if (len > 0 && data == NULL)
        return I2C_ERR_PARAM;
    
    ret = SendStart(UI2C0_BASE);
    if (ret != I2C_OK) goto done;
    
    ret = SendAddress(UI2C0_BASE, addr, 0);
    if (ret != I2C_OK) goto done;
    
    for (i = 0; i < len; i++) {
        ret = SendByte(UI2C0_BASE, data[i]);
        if (ret != I2C_OK) goto done;
    }
    
done:
    SendStop(UI2C0_BASE);
    ClearProtocolFlags(UI2C0_BASE);
    return ret;
}

int32_t I2C0_Read(uint8_t addr, uint8_t *data, uint16_t len)
{
    int32_t ret;
    uint16_t i;
    
    if (len > 0 && data == NULL)
        return I2C_ERR_PARAM;
    
    ret = SendStart(UI2C0_BASE);
    if (ret != I2C_OK) goto done;
    
    ret = SendAddress(UI2C0_BASE, addr, 1);
    if (ret != I2C_OK) goto done;
    
    for (i = 0; i < len; i++) {
        uint8_t ack = (i < (len - 1)) ? 1 : 0;
        data[i] = ReceiveByte(UI2C0_BASE, ack);
    }
    
done:
    SendStop(UI2C0_BASE);
    ClearProtocolFlags(UI2C0_BASE);
    return ret;
}

int32_t I2C0_WriteRead(uint8_t addr, uint8_t reg, const uint8_t *wdata, uint16_t wlen,
                         uint8_t *rdata, uint16_t rlen)
{
    int32_t ret;
    uint16_t i;
    
    /* Write phase: register + optional data */
    ret = SendStart(UI2C0_BASE);
    if (ret != I2C_OK) goto done;
    
    ret = SendAddress(UI2C0_BASE, addr, 0);
    if (ret != I2C_OK) goto done;
    
    ret = SendByte(UI2C0_BASE, reg);
    if (ret != I2C_OK) goto done;
    
    for (i = 0; i < wlen; i++) {
        ret = SendByte(UI2C0_BASE, wdata[i]);
        if (ret != I2C_OK) goto done;
    }
    
    /* Read phase */
    ret = SendStart(UI2C0_BASE);
    if (ret != I2C_OK) goto done;
    
    ret = SendAddress(UI2C0_BASE, addr, 1);
    if (ret != I2C_OK) goto done;
    
    for (i = 0; i < rlen; i++) {
        uint8_t ack = (i < (rlen - 1)) ? 1 : 0;
        rdata[i] = ReceiveByte(UI2C0_BASE, ack);
    }
    
done:
    SendStop(UI2C0_BASE);
    ClearProtocolFlags(UI2C0_BASE);
    return ret;
}

int32_t I2C0_Scan(uint8_t *foundAddrs, uint8_t maxCount)
{
    int32_t count = 0;
    uint8_t addr;
    
    for (addr = 0x03; addr <= 0x77 && count < maxCount; addr++) {
        if (I2C0_Probe(addr) == I2C_OK) {
            foundAddrs[count++] = addr;
        }
        /* Inter-byte delay for bus stability */
        volatile uint32_t d = 100; while (--d > 0);
    }
    
    return count;
}

int32_t I2C0_Probe(uint8_t addr)
{
    int32_t ret;
    
    ret = SendStart(UI2C0_BASE);
    if (ret != I2C_OK) goto done;
    
    ret = SendAddress(UI2C0_BASE, addr, 0);
    
done:
    SendStop(UI2C0_BASE);
    ClearProtocolFlags(UI2C0_BASE);
    return ret;
}

uint8_t I2C0_IsBusIdle(void)
{
    /* Bus is idle if no START condition detected */
    return !(UI2C0_BASE->PROTSTS & UI2C_PROTSTS_STARTF_Msk);
}

void I2C0_ResetBus(void)
{
    int32_t i;
    
    /* Disable I2C temporarily */
    UI2C0_BASE->PROTCTL &= ~UI2C_PROTCTL_PROTEN_Msk;
    
    /* Toggle clock 9 times to clear stuck slave */
    PE->MODE = (PE->MODE & ~0xF0) | (0x1 << 8) | (0x1 << 12);  /* Output mode */
    
    for (i = 0; i < 9; i++) {
        PE->DOUT &= ~0x04;  /* CLK low */
        volatile uint32_t d = 50; while (--d > 0);
        PE->DOUT |= 0x04;   /* CLK high */
        volatile uint32_t d2 = 50; while (--d2 > 0);
    }
    
    /* Restore open-drain mode */
    PE->MODE = (PE->MODE & ~0xF0) | (0x3 << 8) | (0x3 << 12);
    
    /* Re-enable I2C */
    ClearProtocolFlags(UI2C0_BASE);
    UI2C0_BASE->PROTCTL |= UI2C_PROTCTL_PROTEN_Msk;
}
