/**
 * @file     main.c
 * @brief    HID-over-I2C Bridge - Main Entry Point
 * @version  1.0.0
 * 
 * M487 HID-over-I2C Bridge Firmware
 * 
 * Architecture:
 *   PC (USB HID Host) ←USB→ M487 ←I2C→ HID-over-I2C Device
 * 
 * This firmware implements a USB HID device that bridges to an I2C HID device
 * using the Microsoft HID-over-I2C protocol specification.
 */

#include <stdio.h>
#include "NuMicro.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Local Headers                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#include "i2c_driver.h"
#include "hid_parser.h"
#include "usb_hid.h"
#include "bridge.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Configuration                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

/** I2C Device Address (default: 0x2E for many HID-over-I2C devices) */
#ifndef I2C_DEVICE_ADDRESS
#define I2C_DEVICE_ADDRESS      0x2E
#endif

/** Enable GPIO interrupt for Input Reports */
#ifndef ENABLE_I2C_INTERRUPT
#define ENABLE_I2C_INTERRUPT     1
#endif

/** GPIO pin for I2C device interrupt */
#ifndef I2C_DEVICE_INT_PIN
#define I2C_DEVICE_INT_PIN     4   /* PC.4 */
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Forward Declarations                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
static void System_Init(void);
static void GPIO_Init(void);

/*---------------------------------------------------------------------------------------------------------*/
/* System Initialization                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/

static void System_Init(void)
{
    uint32_t volatile i;
    
    /* Unlock protected registers */
    SYS_UnlockReg();
    
    /* Enable external crystal (12 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    while (!CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk));
    
    /* Set HCLK to HXT */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT, CLK_CLKDIV0_HCLK(1));
    
    /* Set core clock to 96 MHz */
    CLK_SetCoreClock(FREQ_96MHZ);
    
    /* PCLK0, PCLK1 = HCLK/2 */
    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2;
    
    /* Enable USB PHY */
    SYS->USBPHY &= ~SYS_USBPHY_HSUSBROLE_Msk;
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_HSUSBROLE_Msk) | SYS_USBPHY_HSUSBEN_Msk;
    for (i = 0; i < 1000; i++);
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
    
    /* Enable module clocks */
    CLK_EnableModuleClock(HSUSBD_MODULE);
    CLK_EnableModuleClock(USCI0_MODULE);
    
    /* UART0 for debug */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    CLK_EnableModuleClock(UART0_MODULE);
    
    /* Configure UART0 pins */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);
    
    /* Configure I2C pins: PE2=CLK, PE3=DAT0 */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE2MFP_USCI0_CLK | SYS_GPE_MFPL_PE3MFP_USCI0_DAT0);
    
    SystemCoreClockUpdate();
}

/**
 * @brief   GPIO Initialization
 */
static void GPIO_Init(void)
{
#if ENABLE_I2C_INTERRUPT
    /* Configure GPIO for I2C device interrupt input */
    /* PC.4 as input with pull-up */
    PC->MODE &= ~(0x3 << (4 * 2));   /* Input mode */
    PC->PUSEL |= (0x1 << (4 * 2));   /* Pull-up */
    
    /* Enable GPIO C interrupt */
    PC->INTCFG |= (1 << 4);   /* Rising edge trigger */
    PC->INTEN |= (1 << 4);    /* Enable interrupt */
    
    /* NVIC for GPIOC */
    NVIC_EnableIRQ(GPIOC_IRQn);
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* Main Entry                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/

int main(void)
{
    Bridge_Config_t bridgeConfig;
    
    /* Initialize system */
    System_Init();
    
    /* Initialize UART for debug output */
    UART_Open(UART0, 115200);
    
    printf("\n");
    printf("===========================================\n");
    printf("  HID-over-I2C Bridge\n");
    printf("  M487 Firmware v1.0.0\n");
    printf("===========================================\n");
    printf("\n");
    
    /* Configure GPIO */
    GPIO_Init();
    
    /* Initialize bridge */
    memset(&bridgeConfig, 0, sizeof(bridgeConfig));
    bridgeConfig.i2cDeviceAddr = I2C_DEVICE_ADDRESS;
    bridgeConfig.enableInterrupt = ENABLE_I2C_INTERRUPT;
    bridgeConfig.gpioIntPin = I2C_DEVICE_INT_PIN;
    
    Bridge_Init(&bridgeConfig);
    
    printf("Bridge initialized\n");
    printf("  I2C Address: 0x%02X\n", I2C_DEVICE_ADDRESS);
    printf("  Interrupt:   %s\n", ENABLE_I2C_INTERRUPT ? "Enabled" : "Disabled");
    printf("\n");
    
    /* Start USB */
    USB_HID_Start();
    printf("USB HID device started\n");
    printf("Waiting for USB connection...\n");
    
    /* Main loop */
    while (1) {
        /* Poll for USB connection */
        if (USB_HID_IsAttached()) {
            /* Try to connect to I2C device */
            Bridge_Connect();
            
            if (Bridge_GetState() == BRIDGE_STATE_CONNECTED) {
                printf("Connected to I2C HID device!\n");
                printf("Bridge ready.\n");
                
                /* Main bridge loop - handle data */
                while (Bridge_GetState() == BRIDGE_STATE_CONNECTED) {
                    /* In interrupt mode, data is handled in ISR */
                    /* In polling mode, we could poll here if needed */
                    
                    /* Toggle LED to indicate connection */
                    volatile uint32_t d;
                    for (d = 0; d < 100000; d++);
                    
                    /* Check for disconnect (USB detach) */
                    if (!USB_HID_IsAttached()) {
                        Bridge_Disconnect();
                        break;
                    }
                }
            }
            
            if (Bridge_GetState() == BRIDGE_STATE_ERROR) {
                printf("Connection failed. Retrying...\n");
            }
        }
        
        /* Toggle LED when waiting for connection */
        volatile uint32_t d;
        for (d = 0; d < 500000; d++);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Interrupt Handlers                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

#if ENABLE_I2C_INTERRUPT
/**
 * @brief   GPIO C Interrupt Handler (I2C device interrupt)
 */
void GPIOC_IRQHandler(void)
{
    /* Check if PC.4 caused interrupt */
    if (PC->INTSTS & (1 << 4)) {
        /* Clear interrupt flag */
        PC->INTSTS = (1 << 4);
        
        /* Handle I2C device Input Report */
        Bridge_OnI2CInterrupt();
    }
}
#endif
