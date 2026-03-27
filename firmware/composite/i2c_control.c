/**
 * @file     i2c_control.c
 * @brief    I2C Control Driver for M487 UI2C0
 * @version  2.0.0
 * 
 * Hardware: UI2C0 (USCI_I2C)
 *   PE2 = CLK (UI2C0_CLK)
 *   PE3 = DAT0 (UI2C0_DAT0)
 *   Speed: 100 kHz
 *   Mode: Master
 * 
 * Note: UI2C0 uses USCI0_IRQn which is separate from USBD20_IRQn,
 *       so USB and I2C interrupts can coexist.
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "hid_i2c.h"

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Master State Machine                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
static volatile enum UI2C_MASTER_EVENT s_eI2cEvent = MASTER_STOP;
static volatile uint8_t s_u8MstDataLen = 0;
static volatile uint8_t s_u8MstEndFlag = 0;

/* NACK retry tracking - Q-05 */
static volatile uint8_t s_u8NackRetryCount = 0;
#define NACK_RETRY_MAX       3
#define I2C_TIMEOUT_COUNT    1000000UL  /* Q-07: Timeout for I2C transaction */
#define I2C_SCAN_TIMEOUT     1000       /* Q-07: Short timeout for bus scan probe */
#define I2C_SCAN_DELAY       100        /* Q-07: Inter-byte delay between scan probes */

/* TX/RX buffers */
static uint8_t s_au8MstTxData[68];
static uint8_t s_au8MstRxData[64];
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
            UI2C_SET_DATA(UI2C0, (s_u8DeviceAddr << 1) | 0x00);
            s_eI2cEvent = MASTER_SEND_ADDRESS;
        } else if (s_eI2cEvent == MASTER_SEND_REPEAT_START) {
            UI2C_SET_DATA(UI2C0, (s_u8DeviceAddr << 1) | 0x01);
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
        
        else if (s_eI2cEvent == MASTER_READ_DATA) {
            /* Read data byte and ACK to continue reading (unless last byte) */
            uint8_t byte = (uint8_t)(UI2C_GET_DATA(UI2C0) & 0xFF);
            s_au8MstRxData[s_u16RxCount++] = byte;
            
            if (s_u16RxCount < s_u16RxLen) {
                /* More bytes to read - send ACK and continue */
                /* AA=1 means ACK for next byte */
                UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_PTRG | UI2C_CTL_AA);
            } else {
                /* Last byte (or only byte) - send NACK to tell slave we're done */
                UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_PTRG);  /* No AA = NACK */
            }
        }
    }

    else if ((u32Status & UI2C_PROTSTS_NACKIF_Msk) == UI2C_PROTSTS_NACKIF_Msk) {
        UI2C_CLR_PROT_INT_FLAG(UI2C0, UI2C_PROTSTS_NACKIF_Msk);

        if (s_eI2cEvent == MASTER_SEND_ADDRESS) {
            /* Q-05: Device NACKed address - retry with backoff */
            s_u8NackRetryCount++;
            if (s_u8NackRetryCount < NACK_RETRY_MAX) {
                /* Exponential backoff: delay proportional to retry count */
                volatile uint32_t delay = 50 * s_u8NackRetryCount;
                while (delay-- > 0);
                /* Retry: resend START + address */
                s_eI2cEvent = MASTER_SEND_START;
                UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_STA);
            } else {
                /* Max retries exceeded - give up */
                s_eI2cEvent = MASTER_STOP;
                UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
            }
        } else if (s_eI2cEvent == MASTER_SEND_DATA) {
            /* Q-05: Device NACKed data - retry sending same byte */
            s_u8NackRetryCount++;
            if (s_u8NackRetryCount < NACK_RETRY_MAX) {
                /* Resend same data byte */
                volatile uint32_t delay = 50 * s_u8NackRetryCount;
                while (delay-- > 0);
                s_eI2cEvent = MASTER_SEND_DATA;
                UI2C_SET_DATA(UI2C0, s_au8MstTxData[s_u8MstDataLen]);
                UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_PTRG);
            } else {
                /* Max retries exceeded - give up */
                s_eI2cEvent = MASTER_STOP;
                UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
            }
        } else if (s_eI2cEvent == MASTER_READ_DATA) {
            /* NACK on read - this shouldn't normally happen since we NACK ourselves
             * but handle it gracefully by ending the transfer */
            /* Data was already read on ACKIF */
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
    UI2C_Open(UI2C0, 100000);

    /* Enable I2C protocol interrupts */
    UI2C_ENABLE_PROT_INT(UI2C0, (UI2C_PROTIEN_ACKIEN_Msk | UI2C_PROTIEN_NACKIEN_Msk |
                                 UI2C_PROTIEN_STORIEN_Msk | UI2C_PROTIEN_STARIEN_Msk));

    /* Enable USCI0_IRQn */
    NVIC_EnableIRQ(USCI0_IRQn);

    printf("I2C0: UI2C0 @ 100kHz on PE2(CLK)/PE3(DAT0)\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Start I2C transaction (internal)                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
static int32_t I2C_StartTransaction(uint8_t devAddr, const uint8_t *txBuf, uint16_t txLen,
                                     uint8_t *rxBuf, uint16_t rxLen)
{
    s_u8DeviceAddr = devAddr;
    s_u8MstDataLen = 0;
    s_u8MstEndFlag = 0;
    s_u8NackRetryCount = 0;  /* Q-05: Reset NACK retry count */
    s_u16TxLen = txLen;
    s_u16RxLen = rxLen;
    s_u16RxCount = 0;

    if (txLen > 0 && txLen < sizeof(s_au8MstTxData))
        memcpy(s_au8MstTxData, txBuf, txLen);

    /* Choose handler: if we have RX, use combined handler, else TX only */
    /* (In this implementation, we use the same state machine for both) */

    /* Send START */
    s_eI2cEvent = MASTER_SEND_START;
    UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_STA);

    /* Wait for transaction complete */
    volatile uint32_t timeout = I2C_TIMEOUT_COUNT;
    while (!s_u8MstEndFlag && --timeout > 0);
    if (timeout == 0) {
        /* Timeout - reset I2C bus */
        UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
        UI2C_ClearTimeoutFlag(UI2C0);
        return -1;
    }

    /* Copy received data */
    if (rxLen > 0 && rxBuf != NULL && s_u16RxCount > 0) {
        uint16_t copyLen = (s_u16RxCount < rxLen) ? s_u16RxCount : rxLen;
        memcpy(rxBuf, s_au8MstRxData, copyLen);
    }

    return 0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Write                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
int32_t I2C_Write(uint8_t slaveAddr, const uint8_t *data, uint16_t len)
{
    if (len == 0) return 0;
    if (len > I2C_MAX_WRITE_LEN) len = I2C_MAX_WRITE_LEN;

    /* Make sure previous transaction is done */
    while (!s_u8MstEndFlag && (UI2C0->PROTCTL & UI2C_PROTCTL_PROTEN_Msk)) { }

    return I2C_StartTransaction(slaveAddr, data, len, NULL, 0);
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Read (Pure read - no write before)                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief   Read data from I2C device (pure read, no write address phase)
 * @param   slaveAddr   7-bit I2C device address
 * @param   data       Buffer to receive data
 * @param   len        Number of bytes to read (max I2C_MAX_READ_LEN)
 * @return  0 on success, negative on error
 * 
 * Protocol: START -> ADDR+R -> [DATA...] -> STOP
 * 
 * Note: For devices that require a register address before reading,
 *       use I2C_WriteRead() instead.
 */
int32_t I2C_Read(uint8_t slaveAddr, uint8_t *data, uint16_t len)
{
    if (len == 0) return 0;
    if (len > I2C_MAX_READ_LEN) len = I2C_MAX_READ_LEN;
    if (data == NULL) return -1;
    
    /* Wait for previous transaction to complete */
    while (!s_u8MstEndFlag && (UI2C0->PROTCTL & UI2C_PROTCTL_PROTEN_Msk)) { }
    
    /* Initialize transaction */
    s_u8DeviceAddr = slaveAddr;
    s_u8MstEndFlag = 0;
    s_u8NackRetryCount = 0;
    s_u16RxLen = len;
    s_u16RxCount = 0;
    
    /* For pure read: Set event to MASTER_SEND_REPEAT_START so that when 
     * START condition is detected, it sends address+R directly instead of address+W */
    s_eI2cEvent = MASTER_SEND_REPEAT_START;
    
    /* Send START - when STARIF fires, ISR will send ADDR+R (because event is MASTER_SEND_REPEAT_START) */
    UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_STA);
    
    /* Wait for transaction complete */
    volatile uint32_t timeout = I2C_TIMEOUT_COUNT;
    while (!s_u8MstEndFlag && --timeout > 0);
    if (timeout == 0) {
        /* Timeout - reset I2C bus */
        UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
        UI2C_ClearTimeoutFlag(UI2C0);
        return -1;
    }
    
    /* Copy received data to output buffer */
    if (s_u16RxCount > 0 && data != NULL) {
        uint16_t copyLen = (s_u16RxCount < len) ? s_u16RxCount : len;
        memcpy(data, s_au8MstRxData, copyLen);
    }
    
    return 0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Write + Read (combined protocol)                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
int32_t I2C_WriteRead(uint8_t slaveAddr, const uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen)
{
    if (wlen > I2C_MAX_WRITE_LEN) wlen = I2C_MAX_WRITE_LEN;
    if (rlen > I2C_MAX_READ_LEN) rlen = I2C_MAX_READ_LEN;

    while (!s_u8MstEndFlag && (UI2C0->PROTCTL & UI2C_PROTCTL_PROTEN_Msk)) { }

    return I2C_StartTransaction(slaveAddr, wdata, wlen, rdata, rlen);
}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Scan - Probe for devices on the bus                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
int32_t I2C_Scan(uint8_t *foundAddrs, uint8_t maxCount)
{
    int32_t count = 0;
    uint8_t addr;

    for (addr = 0x03; addr <= 0x77 && count < maxCount; addr++) {
        /* Try to send address and check for ACK */
        s_u8DeviceAddr = addr;
        s_u8MstDataLen = 0;
        s_u8MstEndFlag = 0;
        s_u16TxLen = 0;
        s_u16RxLen = 0;
        s_u16RxCount = 0;

        /* Send START + address write */
        s_eI2cEvent = MASTER_SEND_START;
        UI2C_SET_CONTROL_REG(UI2C0, UI2C_CTL_STA);

        /* Wait short time for NACK/ACK */
        volatile uint32_t t = I2C_SCAN_TIMEOUT;
        while (!s_u8MstEndFlag && --t > 0);

        if (t > 0 && s_eI2cEvent != MASTER_STOP) {
            /* If we got here without timeout, check if device responded */
            /* A device that NACKs will cause NACKIF */
        }

        /* If transaction succeeded without NACK on address, it's present */
        if (s_u8MstEndFlag && (UI2C0->PROTSTS & UI2C_PROTSTS_NACKIF_Msk) == 0) {
            /* Check if it was an ACK */
            /* Actually let's just check the timeout - if it completed quickly 
             * with END flag, the device likely ACKed */
            foundAddrs[count++] = addr;
        }

        /* Reset for next probe */
        if (!s_u8MstEndFlag) {
            UI2C_SET_CONTROL_REG(UI2C0, (UI2C_CTL_PTRG | UI2C_CTL_STO));
            UI2C_ClearTimeoutFlag(UI2C0);
        }
        s_u8MstEndFlag = 0;

        /* Inter-byte delay */
        volatile uint32_t d = I2C_SCAN_DELAY; while (--d > 0);
    }

    return count;
}
