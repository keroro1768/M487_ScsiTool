/**
 * @file     i2c_driver.c
 * @brief    M487 UI2C0 I2C Driver - BSP API Wrapper
 * @version  1.0.0
 * 
 * Hardware: UI2C0 (USCI_I2C)
 *   PE2 = CLK (UI2C0_CLK)
 *   PE3 = DAT0 (UI2C0_DAT0)
 *   Speed: 100 kHz (default)
 *   Mode: Master only
 * 
 * Uses BSP high-level functions for compatibility.
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "i2c_driver.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Private Variables                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/

static uint32_t s_u32CurrentSpeed = I2C0_SPEED;
static uint8_t s_u8DeviceAddr = 0xFF;  /* Current target device address */

/*---------------------------------------------------------------------------------------------------------*/
/* Internal Helper Functions                                                                               */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Wait for I2C transaction complete with timeout
 */
static int32_t WaitForComplete(uint32_t timeout)
{
    while (timeout-- > 0) {
        if (UI2C0->PROTSTS & UI2C_PROTSTS_STORIF_Msk)
            return I2C_OK;
        if (UI2C0->PROTSTS & UI2C_PROTSTS_NACKIF_Msk)
            return I2C_ERR_NACK;
        if (UI2C_GET_TIMEOUT_FLAG(UI2C0))
            return I2C_ERR_TIMEOUT;
    }
    return I2C_ERR_TIMEOUT;
}

/**
 * @brief   Clear protocol status flags
 */
static void ClearFlags(void)
{
    volatile uint32_t sts = UI2C0->PROTSTS;
    (void)sts;
    UI2C0->PROTSTS = sts;  /* Write 1 to clear */
}

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

int32_t I2C0_Init(void)
{
    /* Enable UI2C0 clock via USCI0 */
    CLK_EnableModuleClock(USCI0_MODULE);
    
    /* Configure pins: PE2=CLK, PE3=DAT0 */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE2MFP_USCI0_CLK | SYS_GPE_MFPL_PE3MFP_USCI0_DAT0);
    
    /* Enable Schmitt trigger for stable I2C signaling */
    PE->SMTEN |= (GPIO_SMTEN_SMTEN2_Msk | GPIO_SMTEN_SMTEN3_Msk);
    
    /* Open UI2C0 at specified speed using BSP function */
    UI2C_Open(UI2C0, I2C0_SPEED);
    
    /* Clear any pending flags */
    ClearFlags();
    
    s_u32CurrentSpeed = I2C0_SPEED;
    
    return I2C_OK;
}

void I2C0_DeInit(void)
{
    UI2C_Close(UI2C0);
    CLK_DisableModuleClock(USCI0_MODULE);
}

int32_t I2C0_SetSpeed(uint32_t speed)
{
    uint32_t actual;
    
    if (speed == 0)
        return I2C_ERR_PARAM;
    
    actual = UI2C_SetBusClockFreq(UI2C0, speed);
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
    
    s_u8DeviceAddr = addr;
    
    /* START condition */
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Send address + W */
    UI2C0->TXDAT = (addr << 1);
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Send register index */
    UI2C0->TXDAT = reg;
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Send data bytes */
    for (i = 0; i < len; i++) {
        UI2C0->TXDAT = data[i];
        UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
        ret = WaitForComplete(I2C0_TIMEOUT);
        if (ret == I2C_ERR_NACK) goto done;
        if (ret != I2C_OK) goto done;
        ClearFlags();
    }
    
done:
    /* STOP condition */
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | (UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_STO_Msk);
    ClearFlags();
    return ret;
}

int32_t I2C0_ReadReg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    int32_t ret;
    uint16_t i;
    
    if (len > 0 && data == NULL)
        return I2C_ERR_PARAM;
    
    s_u8DeviceAddr = addr;
    
    /* Write phase: START -> addr+W -> reg */
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    UI2C0->TXDAT = (addr << 1);
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    UI2C0->TXDAT = reg;
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Repeated START for read */
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Send address + R */
    UI2C0->TXDAT = (addr << 1) | 1;
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Receive data bytes */
    for (i = 0; i < len; i++) {
        /* ACK all except last byte */
        if (i < (len - 1))
            UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_AA_Msk;
        else
            UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;  /* NAK last byte */
        
        ret = WaitForComplete(I2C0_TIMEOUT);
        if (ret != I2C_OK) goto done;
        ClearFlags();
        
        data[i] = (uint8_t)(UI2C0->RXDAT & 0xFF);
    }
    
done:
    /* STOP condition */
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | (UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_STO_Msk);
    ClearFlags();
    return ret;
}

int32_t I2C0_Write(uint8_t addr, const uint8_t *data, uint16_t len)
{
    int32_t ret;
    uint16_t i;
    
    if (len > 0 && data == NULL)
        return I2C_ERR_PARAM;
    
    s_u8DeviceAddr = addr;
    
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    UI2C0->TXDAT = (addr << 1);
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    for (i = 0; i < len; i++) {
        UI2C0->TXDAT = data[i];
        UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
        ret = WaitForComplete(I2C0_TIMEOUT);
        if (ret == I2C_ERR_NACK) goto done;
        if (ret != I2C_OK) goto done;
        ClearFlags();
    }
    
done:
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | (UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_STO_Msk);
    ClearFlags();
    return ret;
}

int32_t I2C0_Read(uint8_t addr, uint8_t *data, uint16_t len)
{
    int32_t ret;
    uint16_t i;
    
    if (len > 0 && data == NULL)
        return I2C_ERR_PARAM;
    
    s_u8DeviceAddr = addr;
    
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    UI2C0->TXDAT = (addr << 1) | 1;
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    for (i = 0; i < len; i++) {
        if (i < (len - 1))
            UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_AA_Msk;
        else
            UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
        
        ret = WaitForComplete(I2C0_TIMEOUT);
        if (ret != I2C_OK) goto done;
        ClearFlags();
        
        data[i] = (uint8_t)(UI2C0->RXDAT & 0xFF);
    }
    
done:
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | (UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_STO_Msk);
    ClearFlags();
    return ret;
}

int32_t I2C0_WriteRead(uint8_t addr, uint8_t reg, const uint8_t *wdata, uint16_t wlen,
                         uint8_t *rdata, uint16_t rlen)
{
    int32_t ret;
    uint16_t i;
    
    /* Write phase */
    s_u8DeviceAddr = addr;
    
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    UI2C0->TXDAT = (addr << 1);
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Register index */
    UI2C0->TXDAT = reg;
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Write data */
    for (i = 0; i < wlen; i++) {
        UI2C0->TXDAT = wdata[i];
        UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
        ret = WaitForComplete(I2C0_TIMEOUT);
        if (ret == I2C_ERR_NACK) goto done;
        if (ret != I2C_OK) goto done;
        ClearFlags();
    }
    
    /* Repeated START for read */
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    UI2C0->TXDAT = (addr << 1) | 1;
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) goto done;
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Read data */
    for (i = 0; i < rlen; i++) {
        if (i < (rlen - 1))
            UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_AA_Msk;
        else
            UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
        
        ret = WaitForComplete(I2C0_TIMEOUT);
        if (ret != I2C_OK) goto done;
        ClearFlags();
        
        rdata[i] = (uint8_t)(UI2C0->RXDAT & 0xFF);
    }
    
done:
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | (UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_STO_Msk);
    ClearFlags();
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
        /* Short delay between probes */
        volatile uint32_t d = 50; while (--d > 0);
    }
    
    return count;
}

int32_t I2C0_Probe(uint8_t addr)
{
    int32_t ret;
    
    s_u8DeviceAddr = addr;
    
    /* START */
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_STA_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret != I2C_OK) goto done;
    ClearFlags();
    
    /* Address + W */
    UI2C0->TXDAT = (addr << 1);
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | UI2C_PROTCTL_PTRG_Msk;
    ret = WaitForComplete(I2C0_TIMEOUT);
    if (ret == I2C_ERR_NACK) {
        ret = I2C_ERR_NACK;  /* Device not found */
    }
    
done:
    /* STOP */
    UI2C0->PROTCTL = (UI2C0->PROTCTL & ~0x2E) | (UI2C_PROTCTL_PTRG_Msk | UI2C_PROTCTL_STO_Msk);
    ClearFlags();
    return (ret == I2C_ERR_NACK) ? I2C_ERR_NACK : ret;
}

uint8_t I2C0_IsBusIdle(void)
{
    /* Check if bus is free (no START condition in progress) */
    return !(UI2C0->PROTSTS & UI2C_PROTSTS_STARIF_Msk);
}

void I2C0_ResetBus(void)
{
    int32_t i;
    uint32_t gpioBase;
    uint32_t pinMask;
    
    /* Get GPIO base for PE */
    gpioBase = 0x40040000UL;  /* GPIO base */
    pinMask = (1 << 2);  /* PE.2 = CLK */
    
    /* Disable I2C temporarily */
    UI2C0->PROTCTL &= ~UI2C_PROTCTL_PROTEN_Msk;
    
    /* Toggle clock manually to clear stuck slave */
    /* Set PE.2 as output */
    *(volatile uint32_t *)(gpioBase + 0x04) &= ~pinMask;  /* DIR out */
    
    for (i = 0; i < 9; i++) {
        *(volatile uint32_t *)(gpioBase + 0x0C) &= ~pinMask;  /* CLK low */
        volatile uint32_t d = 50; while (--d > 0);
        *(volatile uint32_t *)(gpioBase + 0x0C) |= pinMask;   /* CLK high */
        volatile uint32_t d2 = 50; while (--d2 > 0);
    }
    
    /* Re-enable I2C */
    ClearFlags();
    UI2C0->PROTCTL |= UI2C_PROTCTL_PROTEN_Msk;
}
