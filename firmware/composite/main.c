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
#include "itm.h"
#include "msc_debug.h"
#include "uart_debug.h"
#include "self_test.h"
#include "flash_error.h"
#include "msc_ramdisk.h"

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

    /* I2C0 pins (PG0=SCL, PG1=SDA) are configured in I2C0_Init() */

    /* Configure PB8 as SWO (Single Wire Output) for ITM trace */
    /* NOTE: Value 0x07 is a common setting for SWO on Nuvoton M-series - verify with datasheet */
    /* SWO pin = PB8, alternate function for debug trace output */
    /* TODO: Verify PB8MFP value for M487. Common values: 0x07 or 0x08 */
    // SYS->GPB_MFPH = (SYS->GPB_MFPH & ~SYS_GPB_MFPH_PB8MFP_Msk) | (0x07 << SYS_GPB_MFPH_PB8MFP_Pos);
    /* Note: Uncomment the above line when PB8 SWO function is confirmed for your board.
     * For boards where PB8 is not available, ITM still works via SWO pin on dedicated debug header. */

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

    /* Initialize ITM SWO trace (call before other initializations) */
    /* Note: PB8 must be configured as SWO in SYS_Init() first */
    ITM_Init();
    ITM_LOG("=== System Boot ===\n");

    /* Init UART to 115200-8n1 for print message */
    UART_Open(UART0, 115200);

    /* Initialize UART Debug Log (structured logging via UART) */
    UART_DBG_Init(UART0, SystemCoreClock);

    printf("M487 USB Composite Device\n");
    printf("  - HID I2C Bridge (Interface 0)\n");
    printf("  - USB Mass Storage  (Interface 1)\n");
    printf("VID=0x%04X PID=0x%04X\n\n", USBD_VID, USBD_PID);

    MAIN_LOG("Device Info: VID=0x%04X PID=0x%04X\n", USBD_VID, USBD_PID);
    ITM_LOG("[MAIN] Device Info: VID=0x%04X PID=0x%04X\n", USBD_VID, USBD_PID);

    /* Run Boot-Time Self-Tests */
    printf("Running self-tests...\n");
    ITM_LOG("[MAIN] Running self-tests...\n");
    SelfTest_Init();
    SelfTest_Result_t stResult;
    uint32_t u32FailMask = SelfTest_RunAll(&stResult);
    printf("Self-test complete: FailedMask=0x%08X, PassedMask=0x%08X\n",
           (unsigned int)stResult.u32FailedMask,
           (unsigned int)stResult.u32PassedMask);
    ITM_LOG("[MAIN] Self-test: FailedMask=0x%08X, PassedMask=0x%08X\n",
           (unsigned int)stResult.u32FailedMask,
           (unsigned int)stResult.u32PassedMask);
    MAIN_LOG("SelfTest: FailMask=0x%08X, PassMask=0x%08X, DWT_cycles=%lu\n",
           (unsigned int)stResult.u32FailedMask,
           (unsigned int)stResult.u32PassedMask,
           (unsigned long)(stResult.u32Timestamp[1] - stResult.u32Timestamp[0]));

    /* Log individual test results */
    for (uint32_t i = 0; i < SelfTest_GetItemCount(); i++) {
        const char *pcName = SelfTest_GetItemName(i);
        uint32_t u32Mask = (1UL << i);
        if (stResult.u32FailedMask & u32Mask) {
            printf("  [FAIL] %s\n", pcName);
            ITM_LOG("[MAIN] [FAIL] %s\n", pcName);
        } else if (stResult.u32PassedMask & u32Mask) {
            printf("  [PASS] %s\n", pcName);
            ITM_LOG("[MAIN] [PASS] %s\n", pcName);
        } else if (stResult.u32SkippedMask & u32Mask) {
            printf("  [SKIP] %s\n", pcName);
            ITM_LOG("[MAIN] [SKIP] %s\n", pcName);
        } else {
            printf("  [----] %s (not run)\n", pcName);
            ITM_LOG("[MAIN] [----] %s\n", pcName);
        }
    }
    printf("\n");
    ITM_LOG("[MAIN] Self-test done, result RAM at 0x%08X\n", SELF_TEST_RAM_BASE);

    /* Initialize I2C */
    I2C0_Init();
    printf("I2C0 initialized (PE2=CLK, PE3=DAT0, 100kHz)\n");
    I2C_LOG("I2C0 initialized on PE2(CLK)/PE3(DAT0), 100kHz\n");
    I2C_TRACE("I2C0 initialized on PE2(CLK)/PE3(DAT0), 100kHz\n");

    /* Open USB device with HID class + MSC */
    HSUSBD_Open(&gsHSInfo, HID_ClassRequest, NULL);
    HSUSBD_SetVendorRequest(HID_VendorRequest);

    /* HID endpoint configuration + MSC BOT init */
    HID_Init();

    /* Initialize FAT12 RAM Disk (must be after HID_Init which sets g_u32StorageBase) */
    {
        extern uint32_t g_u32StorageBase;
        extern int32_t g_TotalSectors;
        RamDisk_Init((uint8_t *)g_u32StorageBase, (uint32_t)g_TotalSectors);

        /* Run I2C bus scan and write results to LOG.TXT */
        {
            char buf[80];
            uint8_t addrs[16];
            int32_t count, j;

            RamDisk_Log("=== M487 I2C Bus Scanner ===\r\n");
            RamDisk_Log("I2C0: PG0(SCL)/PG1(SDA) [Arduino D15/D14] @ 100kHz\r\n");
            RamDisk_Log("Scanning 0x03-0x77...\r\n\r\n");

            count = I2C_Scan(addrs, 16);

            snprintf(buf, sizeof(buf), "Found %d device(s):\r\n", (int)count);
            RamDisk_Log(buf);

            for (j = 0; j < count; j++) {
                snprintf(buf, sizeof(buf), "  [%d] 0x%02X\r\n", (int)j, addrs[j]);
                RamDisk_Log(buf);
            }

            if (count == 0) {
                RamDisk_Log("  (no devices found)\r\n");
            }

            RamDisk_Log("\r\n--- End of scan ---\r\n");
        }
    }

    /* Initialize MSC Vendor Debug Channel */
    MSC_Debug_Init();

    /* Enable USBD interrupt */
    NVIC_EnableIRQ(USBD20_IRQn);

    /* Start USB device */
    printf("Waiting for USB attach...\n");
    USB_LOG("USB stack initialized, waiting for attach\n");
    USB_TRACE("USB stack initialized, waiting for attach\n");
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
