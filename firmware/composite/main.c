/**
 * @file     main.c
 * @brief    M487 USB Composite Device - Mass Storage (MSC) + HID I2C Bridge
 * @version  2.0.0
 * 
 * Composite Device:
 *   Interface 0: HID I2C Bridge (Interrupt EP)
 *   Interface 1: USB Mass Storage (Bulk EP)
 * 
 * Hardware: M487 (NuLink2 or custom board)
 * USB: High-Speed USB 2.0
 * I2C: UI2C0 (PE2=CLK, PE3=DAT0), 100 kHz default
 * 
 * Based on Nuvoton BSP HSUSBD_HID_Transfer_And_MSC example
 */

#include <stdio.h>
#include "NuMicro.h"
#include "hid_i2c.h"

/*---------------------------------------------------------------------------------------------------------*/
/* System Clock Configuration                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void SYS_Init(void)
{
    uint32_t volatile i;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable External XTAL (4~24 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Switch HCLK clock source to HXT */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HXT, CLK_CLKDIV0_HCLK(1));

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);

    /* Set both PCLK0 and PCLK1 as HCLK/2 */
    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2;

    /* Select HSUSBD */
    SYS->USBPHY &= ~SYS_USBPHY_HSUSBROLE_Msk;
    /* Enable USB PHY */
    SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSUSBEN_Msk;
    for (i = 0; i < 0x1000; i++);  /* delay > 10 us */
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;

    /* Enable HSUSBD IP clock */
    CLK_EnableModuleClock(HSUSBD_MODULE);

    /* Enable UART0 for debug messages */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    CLK_EnableModuleClock(UART0_MODULE);

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    /* Configure I2C pins: PE2=CLK, PE3=DAT0 (UI2C0) */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk);
    SYS->GPE_MFPL |= (SYS_GPE_MFPL_PE2MFP_USCI0_CLK | SYS_GPE_MFPL_PE3MFP_USCI0_DAT0);

    SystemCoreClockUpdate();
}

/*---------------------------------------------------------------------------------------------------------*/
/* External symbols (from hid_i2c.c)                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
extern uint8_t volatile g_u8MscStart;

/*---------------------------------------------------------------------------------------------------------*/
/* Main Entry                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{
    /* Init System, IP clock and multi-function I/O */
    SYS_Init();

    /* Init UART to 115200-8n1 for print message */
    UART_Open(UART0, 115200);

    printf("M487 USB Composite Device\n");
    printf("  - HID I2C Bridge (Interface 0)\n");
    printf("  - USB Mass Storage  (Interface 1)\n");
    printf("VID=0x%04X PID=0x%04X\n\n", USBD_VID, USBD_PID);

    /* Initialize I2C */
    I2C0_Init();
    printf("I2C0 initialized (PE2=CLK, PE3=DAT0, 100kHz)\n");

    /* Open USB device with HID class + MSC */
    HSUSBD_Open(&gsHSInfo, HID_ClassRequest, NULL);
    HSUSBD_SetVendorRequest(HID_VendorRequest);

    /* HID endpoint configuration + MSC BOT init */
    HID_Init();

    /* Enable USBD interrupt */
    NVIC_EnableIRQ(USBD20_IRQn);

    /* Start USB device */
    printf("Waiting for USB attach...\n");
    while (1) {
        if (HSUSBD_IS_ATTACHED()) {
            HSUSBD_Start();
            break;
        }
    }

    /* Main loop */
    while (1) {
        if (g_u8MscStart)
            MSC_ProcessCmd();
    }
}
