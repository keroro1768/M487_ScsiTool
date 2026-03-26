/**
 * @file     i2c_control.c
 * @brief    I2C Control Driver for M487 UI2C0
 * @version  2.1.0
 * 
 * Hardware: UI2C0 (USCI_I2C)
 *   PE2 = CLK (UI2C0_CLK)
 *   PE3 = DAT0 (UI2C0_DAT0)
 *   Speed: 100 kHz
 *   Mode: Master
 * 
 * Features:
 *   - NACK retry with exponential backoff
 *   - Unified error codes
 *   - Buffer boundary protection
 * 
 * Note: UI2C0 uses USCI0_IRQn which is separate from USBD20_IRQn,
 *       so USB and I2C interrupts can coexist.
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "hid_i2c.h"
#include "i2c_constants.h"
#include "i2c_error.h"

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Master State Machine                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
static volatile enum UI2C_MASTER_EVENT s_eI2cEvent = MASTER_STOP;
static volatile uint8_t s_u8MstDataLen = 0;
static volatile uint8_t s_u8MstEndFlag = 0;
static volatile uint8_t s_u8NackFlag = 0;

/* TX/RX buffers */
static uint8_t s_au8MstTxData[I2C_INTERNAL_TX_SIZE];
static uint8_t s_au8MstRxData[I2C_INTERNAL_RX_SIZE];
static uint8_t s_u8DeviceAddr;
static uint16_t s_u16TxLen;
static uint16_t s_u16RxLen;
static uint16_t s_u16RxCount;

/*---------------------------------------------------------------------------------------------------------*/
/* UI2C0 IRQ Handler                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void USCI0_IRQHandler(void)
{
    uint32_t u32Status = UI2C0->PROTSTS;

    if (UI2C_GET_TIMEOUT_FLAG(UI2C0)) {
        UI2C_ClearTimeoutFlag(UI2C0);
    }

    if ((u32Status & UI2C_PROTSTS_STARIF_Msk) == UI2C_PROTSTS_STARIF_Msk) {
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_STARIF_Msk);

        if (s_eI2cEvent == MASTER_SEND_START) {
            UI2C_SET_DATA(UI2C0, (s_u8DeviceAddr << 1) | I2C_ADDR_WRITE_BIT);
            s_eI2cEvent = MASTER_SEND_ADDRESS;
        } else if (s_eI2cEvent == MASTER_SEND_REPEAT_START) {
            UI2C_SET_DATA(UI2C0, (s_u8DeviceAddr << 1) | I2C_ADDR_READ_BIT);
            s_eI2cEvent = MASTER_SEND_H_RD_ADDRESS;
        }
        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_PTRG);
    }

    else if ((u32Status & UI2C_PROTSTS_ACKIF_Msk) == UI2C_PROTSTS_ACKIF_Msk) {
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_ACKIF_Msk);

        if (s_eI2cEvent == MASTER_SEND_ADDRESS) {
            if (s_u16TxLen > 0) {
                UI2C_SET_DATA(UI2C0, s_au8MstTxData[s_u8MstDataLen++]);
                s_eI2cEvent = MASTER_SEND_DATA;
            } else {
                /* Write-only transaction, but need to check if there's a restart+read */
                if (s_u16RxLen > 0) {
                    s_eI2cEvent = MASTER_SEND_REPEAT_START;
                    UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STA));
                } else {
                    s_eI2cEvent = MASTER_STOP;
                    UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
                }
            }
            UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_PTRG);
        }

        else if (s_eI2cEvent == MASTER_SEND_DATA) {
            if (s_u8MstDataLen < s_u16TxLen) {
                UI2C_SET_DATA(UI2C0, s_au8MstTxData[s_u8MstDataLen++]);
                UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_PTRG);
            } else {
                /* All TX data sent */
                if (s_u16RxLen > 0) {
                    s_eI2cEvent = MASTER_SEND_REPEAT_START;
                    UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STA));
                } else {
                    s_eI2cEvent = MASTER_STOP;
                    UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
                }
            }
        }

        else if (s_eI2cEvent == MASTER_SEND_H_RD_ADDRESS) {
            s_eI2cEvent = MASTER_READ_DATA;
            UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_PTRG);
        }
    }

    else if ((u32Status & UI2C_PROTSTS_NACKIF_Msk) == UI2C_PROTSTS_NACKIF_Msk) {
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_NACKIF_Msk);

        /* Mark NACK flag for retry logic */
        s_u8NackFlag = 1;

        if (s_eI2cEvent == MASTER_SEND_ADDRESS) {
            /* Device not responding - NAK on address */
            s_eI2cEvent = MASTER_STOP;
            UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
        } else if (s_eI2cEvent == MASTER_SEND_DATA) {
            /* NAK during data transmission */
            s_eI2cEvent = MASTER_STOP;
            UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
        } else if (s_eI2cEvent == MASTER_READ_DATA) {
            /* Last byte read - send NAK then STOP */
            s_au8MstRxData[s_u16RxCount++] = (uint8_t)(UI2C_GET_DATA(UI2C0) & 0xFF);
            s_eI2cEvent = MASTER_STOP;
            UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
        }
    }

    else if ((u32Status & UI2C_PROTSTS_STORIF_Msk) == UI2C_PROTSTS_STORIF_Msk) {
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_STORIF_Msk);
        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_PTRG);
        s_u8MstEndFlag = 1;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C0 Initialize                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_Init(void)
{
    /* Enable UI2C0 clock */
    CLK_EnableModuleClock(USCI0_MODULE);

    /* Configure pins: PE2=CLK, PE3=DAT0 */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE2MFP_USCI0_CLK | SYS_GPE_MFPL_PE3MFP_USCI0_DAT0);

    /* Enable Schmitt trigger for stable I2C signaling */
    PE->SMTEN |= (GPIO_SMTEN_SMTEN2_Msk | GPIO_SMTEN_SMTEN3_Msk);

    /* Open UI2C0 at 100kHz */
    UI2C_Open(UI2C0, I2C_BUS_SPEED_HZ);

    /* Enable I2C protocol interrupts */
    UI2C_ENABLE_PROT_INT(UI2C0, (UI2C_PROTIEN_ACKIEN_Msk | UI2C_PROTIEN_NACKIEN_Msk |
                                 UI2C_PROTIEN_STORIEN_Msk | UI2C_PROTIEN_STARIEN_Msk));

    /* Enable USCI0_IRQn */
    NVIC_EnableIRQ(USCI0_IRQn);

    printf("I2C0: UI2C0 @ %lu Hz on PE2(CLK)/PE3(DAT0)\n", (unsigned long)I2C_BUS_SPEED_HZ);
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C0 Start Transaction with NACK Retry (Internal)                                                       */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief   Start an I2C transaction with retry support
 * @param   devAddr Device address (7-bit)
 * @param   txBuf   Transmit buffer
 * @param   txLen   Transmit length
 * @param   rxBuf   Receive buffer
 * @param   rxLen   Receive length
 * @return  I2C_OK on success, error code on failure
 */
static int32_t I2C_StartTransaction(uint8_t devAddr, const uint8_t *txBuf, uint16_t txLen,
                                     uint8_t *rxBuf, uint16_t rxLen)
{
    int32_t retryCount;
    int32_t backoffMs;
    uint32_t timeout;

    for (retryCount = 0; retryCount <= NACK_RETRY_MAX; retryCount++) {
        /* Reset state */
        s_u8DeviceAddr = devAddr;
        s_u8MstDataLen = 0;
        s_u8MstEndFlag = 0;
        s_u8NackFlag = 0;
        s_u16TxLen = (txLen < I2C_INTERNAL_TX_SIZE) ? txLen : I2C_INTERNAL_TX_SIZE;
        s_u16RxLen = rxLen;
        s_u16RxCount = 0;

        /* Copy TX data with bounds check */
        if (s_u16TxLen > 0 && txBuf != NULL) {
            memcpy(s_au8MstTxData, txBuf, s_u16TxLen);
        }

        /* Send START */
        s_eI2cEvent = MASTER_SEND_START;
        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_STA);

        /* Wait for transaction complete with timeout */
        timeout = I2C_TIMEOUT_DEFAULT;
        while (!s_u8MstEndFlag && --timeout > 0);

        if (timeout == 0) {
            /* Timeout - reset I2C bus */
            UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
            UI2C_ClearTimeoutFlag(UI2C0);
            
            if (retryCount < NACK_RETRY_MAX) {
                /* Exponential backoff before retry */
                backoffMs = NACK_BACKOFF_BASE_MS << retryCount;
                if (backoffMs > NACK_BACKOFF_MAX_MS)
                    backoffMs = NACK_BACKOFF_MAX_MS;
                
                /* Simple delay loop (in practice, use a proper delay function) */
                volatile uint32_t delay;
                for (delay = 0; delay < (backoffMs * 1000); delay++);
                continue;
            }
            return I2C_ERR_TIMEOUT;
        }

        /* Check if NACK was received */
        if (s_u8NackFlag) {
            s_u8NackFlag = 0;
            
            if (retryCount < NACK_RETRY_MAX) {
                /* Exponential backoff before retry */
                backoffMs = NACK_BACKOFF_BASE_MS << retryCount;
                if (backoffMs > NACK_BACKOFF_MAX_MS)
                    backoffMs = NACK_BACKOFF_MAX_MS;
                
                volatile uint32_t delay;
                for (delay = 0; delay < (backoffMs * 1000); delay++);
                continue;
            }
            return I2C_ERR_NACK_RETRY_EXCEEDED;
        }

        /* Success - copy received data */
        if (rxLen > 0 && rxBuf != NULL && s_u16RxCount > 0) {
            uint16_t copyLen = (s_u16RxCount < rxLen) ? s_u16RxCount : rxLen;
            memcpy(rxBuf, s_au8MstRxData, copyLen);
        }

        return I2C_OK;
    }

    return I2C_ERR_NACK_RETRY_EXCEEDED;
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Write                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief   Write data to I2C device
 * @param   slaveAddr 7-bit slave address
 * @param   data      Data to write
 * @param   len       Number of bytes to write
 * @return  I2C_OK on success, error code on failure
 */
int32_t I2C_Write(uint8_t slaveAddr, const uint8_t *data, uint16_t len)
{
    if (data == NULL)
        return I2C_ERR_NULL_PTR;
    
    if (len == 0)
        return I2C_OK;
    
    /* Clamp length to maximum */
    if (len > I2C_MAX_WRITE_LEN)
        len = I2C_MAX_WRITE_LEN;

    /* Make sure previous transaction is done */
    volatile uint32_t waitCount = I2C_TIMEOUT_SHORT;
    while (!s_u8MstEndFlag && (UI2C0->PROTCTL & UI2C_PROTCTL_PROTEN_Msk)) {
        if (--waitCount == 0)
            return I2C_ERR_BUSY;
    }

    return I2C_StartTransaction(slaveAddr, data, len, NULL, 0);
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Read                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief   Read data from I2C device
 * @param   slaveAddr 7-bit slave address
 * @param   data      Buffer to store read data
 * @param   len       Number of bytes to read
 * @return  Number of bytes read on success, error code on failure
 * @note    This function is a stub - pure read is not yet implemented
 */
int32_t I2C_Read(uint8_t slaveAddr, uint8_t *data, uint16_t len)
{
    if (data == NULL)
        return I2C_ERR_NULL_PTR;
    
    if (len == 0)
        return I2C_OK;
    
    /* Clamp length to buffer size */
    if (len > I2C_MAX_READ_LEN)
        len = I2C_MAX_READ_LEN;
    
    /* For pure read, we need to send START + addr+R, then read.
     * The state machine currently requires a write phase first.
     * TODO: Implement pure read using separate state. */
    (void)slaveAddr;
    
    return I2C_ERR_PROTOTYPE;  /* Not yet implemented */
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Write + Read (combined protocol)                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief   Write then read from I2C device (repeated start)
 * @param   slaveAddr 7-bit slave address
 * @param   wdata     Data to write
 * @param   wlen      Number of bytes to write
 * @param   rdata     Buffer to store read data
 * @param   rlen      Number of bytes to read
 * @return  I2C_OK on success, error code on failure
 */
int32_t I2C_WriteRead(uint8_t slaveAddr, const uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen)
{
    if (wdata == NULL || rdata == NULL)
        return I2C_ERR_NULL_PTR;
    
    /* Clamp lengths */
    if (wlen > I2C_MAX_WRITE_LEN)
        wlen = I2C_MAX_WRITE_LEN;
    if (rlen > I2C_MAX_READ_LEN)
        rlen = I2C_MAX_READ_LEN;

    /* Wait for bus to be free */
    volatile uint32_t waitCount = I2C_TIMEOUT_SHORT;
    while (!s_u8MstEndFlag && (UI2C0->PROTCTL & UI2C_PROTCTL_PROTEN_Msk)) {
        if (--waitCount == 0)
            return I2C_ERR_BUSY;
    }

    return I2C_StartTransaction(slaveAddr, wdata, wlen, rdata, rlen);
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Scan - Probe for devices on the bus                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief   Scan I2C bus for devices
 * @param   foundAddrs Buffer to store found addresses
 * @param   maxCount   Maximum number of addresses to store
 * @return  Number of devices found
 */
int32_t I2C_Scan(uint8_t *foundAddrs, uint8_t maxCount)
{
    int32_t count = 0;
    uint8_t addr;

    if (foundAddrs == NULL)
        return I2C_ERR_NULL_PTR;
    
    if (maxCount == 0)
        return I2C_ERR_INVALID_PARAM;

    for (addr = I2C_SLAVE_ADDR_MIN; addr <= I2C_SLAVE_ADDR_MAX && count < maxCount; addr++) {
        /* Try to send address and check for ACK */
        s_u8DeviceAddr = addr;
        s_u8MstDataLen = 0;
        s_u8MstEndFlag = 0;
        s_u8NackFlag = 0;
        s_u16TxLen = 0;
        s_u16RxLen = 0;
        s_u16RxCount = 0;

        /* Send START + address write */
        s_eI2cEvent = MASTER_SEND_START;
        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_STA);

        /* Wait short time for NACK/ACK */
        volatile uint32_t t = I2C_TIMEOUT_SCAN;
        while (!s_u8MstEndFlag && --t > 0);

        /* Reset for next probe */
        if (!s_u8MstEndFlag) {
            UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
            UI2C_ClearTimeoutFlag(UI2C0);
        }
        s_u8MstEndFlag = 0;

        /* If transaction succeeded without NACK on address, it's present */
        if (s_u8MstEndFlag && !s_u8NackFlag) {
            foundAddrs[count++] = addr;
        }

        /* Inter-byte delay */
        volatile uint32_t d = I2C_SCAN_INTERBYTE_DELAY;
        while (--d > 0);
    }

    return count;
}
