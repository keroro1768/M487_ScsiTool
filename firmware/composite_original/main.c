/**
 * @file     main.c
 * @brief    M487 USB Composite Device - Mass Storage + HID I2C Bridge
 * @version  1.0.0
 * 
 * This firmware combines:
 *   - USB Mass Storage (MSC) for Flash storage access
 *   - HID I2C Bridge for I2C device control
 * 
 * Hardware: M487 (NuLink2 or custom board)
 * USB: High-Speed USB 2.0
 * I2C: UI2C0 (PE2=CLK, PE3=DAT0), 100 kHz default
 */

#include <stdio.h>
#include "NuMicro.h"
#include "usb_descriptors.h"
#include "hid_i2c.h"
#include "i2c_control.h"

/*---------------------------------------------------------------------------------------------------------*/
/* System Clock Configuration                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef FREQ_192MHZ
#define FREQ_192MHZ   192000000
#endif

__weak uint32_t CLK_GetPLLClockFreq(void)
{
    return FREQ_192MHZ;
}

/*---------------------------------------------------------------------------------------------------------*/
/* MSC Global Variables                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define DATA_FLASH_STORAGE_SIZE  (64*1024)   // 64KB storage
#define UDC_SECTOR_SIZE          512         // 512 bytes per sector
#define STORAGE_BUFFER_SIZE      512         // Storage buffer

int32_t g_TotalSectors = DATA_FLASH_STORAGE_SIZE / UDC_SECTOR_SIZE;

volatile uint8_t g_u8EP1Ready = 0;
volatile uint8_t g_u8EP2Ready = 0;
volatile uint8_t g_u8Remove = 0;

uint8_t g_u8BulkState;
uint8_t g_u8Prevent = 0;
uint8_t g_u8Size;
uint8_t g_au8SenseKey[4];

uint32_t g_u32DataFlashStartAddr;
uint32_t g_u32Address;
uint32_t g_u32Length;
uint32_t g_u32LbaAddress;
uint32_t g_u32BytesInStorageBuf;

struct CBW {
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t  bmCBWFlags;
    uint8_t  bCBWLUN;
    uint8_t  bCBWCBLength;
    uint8_t  u8OPCode;
    uint8_t  u8LUN;
    uint8_t  au8Data[14];
} g_sCBW;

struct CSW {
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t  bCSWStatus;
} g_sCSW;

#define CBW_SIGNATURE   0x43425355
#define CSW_SIGNATURE   0x53425355

uint8_t g_au8InquiryID[36] = {
    0x00, 0x80, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
    'N', 'u', 'v', 'o', 't', 'o', 'n', ' ',
    'U', 'S', 'B', ' ', 'M', 'a', 's', 's', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e',
    '1', '.', '0', '0'
};

// SCSI Opcodes
#define UFI_TEST_UNIT_READY     0x00
#define UFI_REQUEST_SENSE      0x03
#define UFI_INQUIRY             0x12
#define UFI_READ_10             0x28
#define UFI_WRITE_10            0x2A
#define UFI_READ_CAPACITY       0x25
#define UFI_MODE_SENSE_6        0x1A

/*---------------------------------------------------------------------------------------------------------*/
/* USB Initialization                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
    
    /* Enable external XTAL (4~24 MHz) */
    CLK->PWRCTL |= CLK_PWRCTL_HXTEN_Msk;
    while ((CLK->STATUS & CLK_STATUS_HXTSTB_Msk) == 0);
    
    /* Configure PLL for 192 MHz */
    CLK->PLLCTL = 0x8842E;
    while (!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk));
    
    /* Set HCLK to PLL */
    CLK->CLKDIV0 = 0;
    CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_HCLKSEL_Msk) | CLK_CLKSEL0_HCLKSEL_PLL;
    
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2;
    
    /* Enable USB PHY */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_HSUSBROLE_Msk) | SYS_USBPHY_HSUSBEN_Msk;
    for (volatile uint32_t i = 0; i < 0x1000; i++);
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
    
    /* Enable USB and I2C clocks */
    CLK->AHBCLK |= CLK_AHBCLK_HSUSBDCKEN_Msk;
    CLK->APBCLK1 |= CLK_APBCLK1_USCI0CKEN_Msk;
    
    /* Configure I2C pins: PE2=CLK, PE3=DAT0 */
    SYS->GPE_MFPL = (SYS->GPE_MFPL & ~(SYS_GPE_MFPL_PE2MFP_Msk | SYS_GPE_MFPL_PE3MFP_Msk)) |
                    (SYS_GPE_MFPL_PE2MFP_USCI0_CLK | SYS_GPE_MFPL_PE3MFP_USCI0_DAT0);
    PE->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
    
    SystemCoreClock = FREQ_192MHZ;
}

/*---------------------------------------------------------------------------------------------------------*/
/* USB Device Functions                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void HID_I2C_MainFunction(void);

/*---------------------------------------------------------------------------------------------------------*/
/* Main Entry                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    SYS_Init();
    
    /* Initialize I2C at 100 kHz */
    I2C_Init(I2C_SPEED_STANDARD);
    
    /* Initialize USB device */
    HSUSBD_Open(NULL, NULL, NULL);
    
    /* Set USB descriptors */
    // Note: In real implementation, these would be set via HSUSBD_SetDescriptor callback
    
    /* Initialize MSC */
    // Note: MSC_Init() would be called here in full implementation
    
    NVIC_EnableIRQ(USBD20_IRQn);
    
    /* Start USB */
    while (1) {
        if (HSUSBD_IS_ATTACHED()) {
            HSUSBD_Start();
            break;
        }
    }
    
    /* Main loop */
    while (1) {
        /* Handle HID I2C commands */
        HID_I2C_MainFunction();
        
        /* Handle MSC commands (BOT protocol) */
        // MSC_ProcessCmd();
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* USB Interrupt Handler                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void USBD20_IRQHandler(void)
{
    uint32_t u32IntSts = HSUSBD->GINTSTS;
    uint32_t u32State = HSUSBD->BUSSTS;
    
    if (u32IntSts & HSUSBD_GINTSTS_WAKEUP) {
        HSUSBD_CLR_WAKEUP_INT_FLAG();
    }
    
    if (u32IntSts & HSUSBD_GINTSTS_BUS) {
        HSUSBD_CLR_BUS_INT_FLAG();
        if (u32State & HSUSBD_BUSSTS_USBRST) {
            /* USB bus reset */
            HSUSBD_ENABLE_USB();
            HSUSBD_SwReset();
        }
    }
    
    if (u32IntSts & HSUSBD_GINTSTS_USB) {
        if (u32IntSts & HSUSBD_GINTSTS_SETUP) {
            HSUSBD_ProcessSetupPacket();
        }
        
        /* EP events */
        if (u32IntSts & HSUSBD_GINTSTS_EP0) {
            HSUSBD_CLR_EP_INT_FLAG(0);
        }
        
        if (u32IntSts & HSUSBD_GINTSTS_EP1) {
            HSUSBD_CLR_EP_INT_FLAG(1);
        }
        
        if (u32IntSts & HSUSBD_GINTSTS_EP2) {
            HSUSBD_CLR_EP_INT_FLAG(2);
        }
        
        if (u32IntSts & HSUSBD_GINTSTS_EP3) {
            HSUSBD_CLR_EP_INT_FLAG(3);
        }
    }
}
