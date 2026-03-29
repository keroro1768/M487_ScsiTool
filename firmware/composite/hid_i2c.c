/**
 * @file     hid_i2c.c
 * @brief    M487 USB Composite Device - HID I2C Bridge + MSC Implementation
 * @version  2.0.0
 * 
 * HID I2C Bridge Protocol:
 *   Report ID 0x01: I2C Write  - [0x01][slave_addr][len][data...]
 *   Report ID 0x02: I2C Read   - [0x02][slave_addr][len] -> response [0x02][status][len][data]
 *   Report ID 0x03: I2C WriteRead - [0x03][slave_addr][wlen][rlen][wdata... -> [0x03][status][rlen][data]
 * 
 * Hardware: M487, UI2C0 (PE2=CLK, PE3=DAT0), 100 kHz
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "hid_i2c.h"
#include "msc_debug.h"

/*---------------------------------------------------------------------------------------------------------*/
/* UI2C0 (USCI_I2C) Macros                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define UI2C0_ADDR_TIMEOUT   50000

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

/* MSC global variables */
int32_t g_TotalSectors = 0;
uint8_t g_u8BulkState = BULK_NORMAL;
uint8_t g_u8Prevent = 0;
uint8_t volatile g_u8MscStart = 0;
uint8_t g_au8SenseKey[4];
uint32_t g_u32MSCMaxLun = 0;
uint32_t g_u32LbaAddress;
uint32_t g_u32MassBase, g_u32StorageBase;
uint32_t g_u32EpMaxPacketSize;
uint32_t g_u32EpAMaxPacketSize;
uint32_t g_u32EpBMaxPacketSize;

/* CBW/CSW */
CBW_t g_sCBW;
CSW_t g_sCSW;

/* HSUSBD device info (must be provided by application) */
extern const uint8_t gu8DeviceDescriptor[LEN_DEVICE];
extern const uint8_t gu8ConfigDescriptor[];
extern const uint8_t *gu8StringDescriptor[];
extern const uint8_t gu8QualifierDescriptor[LEN_QUALIFIER];
extern const uint8_t gu8HIDReportDescriptor[];
extern const uint32_t gu32HIDReportSize[1];
extern const uint32_t gu32ConfigHidDescIdx[2];

S_HSUSBD_INFO_T gsHSInfo =
{
    (uint8_t *)gu8DeviceDescriptor,      /* Device descriptor */
    (uint8_t *)gu8ConfigDescriptor,    /* Configuration descriptor */
    (uint8_t **)gu8StringDescriptor,  /* String descriptors */
    (uint8_t *)gu8QualifierDescriptor, /* Qualifier descriptor */
    (uint8_t *)gu8ConfigDescriptor,   /* Full speed config */
    (uint8_t *)gu8ConfigDescriptor,   /* High speed other config */
    (uint8_t *)gu8ConfigDescriptor,   /* Full speed other config */
    (uint8_t **)gu8HIDReportDescriptor,/* HID Report descriptors */
    (uint32_t *)gu32HIDReportSize,    /* HID Report sizes */
    (uint32_t *)gu32ConfigHidDescIdx, /* HID descriptor indices */
};

/* MSC Inquiry data */
uint8_t g_au8InquiryID[36] = {
    0x00, 0x80, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
    'N', 'u', 'v', 'o', 't', 'o', 'n', ' ',
    'U', 'S', 'B', ' ', 'M', 'a', 's', 's', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e',
    '1', '.', '0', '0'
};

/* MSC Mode pages */
static uint8_t g_au8ModePage_01[12] = {
    0x01, 0x0A, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00
};
static uint8_t g_au8ModePage_05[32] = {
    0x05, 0x1E, 0x13, 0x88, 0x08, 0x20, 0x02, 0x00, 0x01, 0xF4, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x1E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x68, 0x00, 0x00
};
static uint8_t g_au8ModePage_1B[12] = {
    0x1B, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static uint8_t g_au8ModePage_1C[8] = {
    0x1C, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00
};
static uint8_t g_au8ModePage[24] = {
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x1C, 0x0A, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

/* HID buffers */
static uint8_t  g_u8PageBuff[PAGE_SIZE] __attribute__((aligned(4))) = {0};
static uint32_t g_u32BytesInPageBuf __attribute__((aligned(4))) = 0;
uint8_t  g_u8OutBuff[EPB_MAX_PKT_SIZE] __attribute__((aligned(4))) = {0};

/* Feature Report buffer for GET_REPORT (S-05 fix) */
static uint8_t  g_u8FeatureReport[EPB_MAX_PKT_SIZE] __attribute__((aligned(4))) = {0};

/* HID I2C command state */
CMD_T gCmd;

/*---------------------------------------------------------------------------------------------------------*/
/* USB Interrupt Handler                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void USBD20_IRQHandler(void)
{
    __IO uint32_t IrqStL, IrqSt;

    IrqStL = HSUSBD->GINTSTS & HSUSBD->GINTEN;
    if (!IrqStL) return;

    if (IrqStL & HSUSBD_GINTSTS_USBIF_Msk) {
        IrqSt = HSUSBD->BUSINTSTS & HSUSBD->BUSINTEN;

        if (IrqSt & HSUSBD_BUSINTSTS_SOFIF_Msk)
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_SOFIF_Msk);

        if (IrqSt & HSUSBD_BUSINTSTS_RSTIF_Msk) {
            HSUSBD_SwReset();
            g_u8MscStart = 0;
            g_u8BulkState = BULK_NORMAL;

            HSUSBD_ResetDMA();
            HSUSBD->EP[EPA].EPRSPCTL = HSUSBD_EPRSPCTL_FLUSH_Msk;
            HSUSBD->EP[EPB].EPRSPCTL = HSUSBD_EPRSPCTL_FLUSH_Msk;
            HSUSBD->EP[EPC].EPRSPCTL = HSUSBD_EPRSPCTL_FLUSH_Msk;
            HSUSBD->EP[EPD].EPRSPCTL = HSUSBD_EPRSPCTL_FLUSH_Msk;

            if (HSUSBD->OPER & 0x04)
                HID_InitForHighSpeed();
            else
                HID_InitForFullSpeed();

            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk);
            HSUSBD_SET_ADDR(0);
            HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_RESUMEIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_RSTIF_Msk);
            HSUSBD_CLR_CEP_INT_FLAG(0x1ffc);
        }

        if (IrqSt & HSUSBD_BUSINTSTS_RESUMEIF_Msk) {
            HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_RESUMEIF_Msk);
        }

        if (IrqSt & HSUSBD_BUSINTSTS_SUSPENDIF_Msk) {
            HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_RESUMEIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_SUSPENDIF_Msk);
        }

        if (IrqSt & HSUSBD_BUSINTSTS_HISPDIF_Msk) {
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_HISPDIF_Msk);
        }

        if (IrqSt & HSUSBD_BUSINTSTS_DMADONEIF_Msk) {
            g_hsusbd_DmaDone = 1;
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_DMADONEIF_Msk);

            if (!(HSUSBD->DMACTL & HSUSBD_DMACTL_DMARD_Msk)) {
                if (g_u8BulkState == BULK_OUT)
                    g_u8BulkState = BULK_CBW;
                HSUSBD_ENABLE_EP_INT(EPD, HSUSBD_EPINTEN_RXPKIEN_Msk);
            }

            if (HSUSBD->DMACTL & HSUSBD_DMACTL_DMARD_Msk) {
                if (g_hsusbd_ShortPacket == 1) {
                    HSUSBD->EP[EPC].EPRSPCTL = (HSUSBD->EP[EPC].EPRSPCTL & 0x10) | HSUSBD_EP_RSPCTL_SHORTTXEN;
                    g_hsusbd_ShortPacket = 0;
                }
            }
        }

        if (IrqSt & HSUSBD_BUSINTSTS_VBUSDETIF_Msk) {
            if (HSUSBD_IS_ATTACHED())
                HSUSBD_ENABLE_USB();
            else
                HSUSBD_DISABLE_USB();
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_VBUSDETIF_Msk);
        }
    }

    /* CEP interrupt */
    if (IrqStL & HSUSBD_GINTSTS_CEPIF_Msk) {
        IrqSt = HSUSBD->CEPINTSTS & HSUSBD->CEPINTEN;

        if (IrqSt & HSUSBD_CEPINTSTS_SETUPTKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_SETUPTKIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_SETUPPKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_SETUPPKIF_Msk);
            HSUSBD_ProcessSetupPacket();
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_OUTTKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_OUTTKIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_INTKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
            if (!(IrqSt & HSUSBD_CEPINTSTS_STSDONEIF_Msk)) {
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_TXPKIF_Msk);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_TXPKIEN_Msk);
                HSUSBD_CtrlIn();
            } else {
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_TXPKIF_Msk);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_TXPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            }
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_TXPKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            if (g_hsusbd_CtrlInSize) {
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_INTKIEN_Msk);
            } else {
                if (g_hsusbd_CtrlZero == 1)
                    HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_ZEROLEN);
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            }
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_TXPKIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_RXPKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_RXPKIF_Msk);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_STSDONEIF_Msk) {
            HSUSBD_UpdateDeviceState();
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk);
            return;
        }
        if (IrqSt & (HSUSBD_CEPINTSTS_NAKIF_Msk | HSUSBD_CEPINTSTS_STALLIF_Msk | HSUSBD_CEPINTSTS_ERRIF_Msk |
                    HSUSBD_CEPINTSTS_BUFFULLIF_Msk | HSUSBD_CEPINTSTS_BUFEMPTYIF_Msk | HSUSBD_CEPINTSTS_PINGIF_Msk)) {
            HSUSBD_CLR_CEP_INT_FLAG(IrqSt);
            return;
        }
    }

    /* Endpoint interrupts */
    if (IrqStL & HSUSBD_GINTSTS_EPAIF_Msk) {
        IrqSt = HSUSBD->EP[EPA].EPINTSTS & HSUSBD->EP[EPA].EPINTEN;
        if (HSUSBD->EP[EPA].EPINTSTS & 0x02)
            EPA_Handler();
        HSUSBD_CLR_EP_INT_FLAG(EPA, IrqSt);
    }
    if (IrqStL & HSUSBD_GINTSTS_EPBIF_Msk) {
        IrqSt = HSUSBD->EP[EPB].EPINTSTS & HSUSBD->EP[EPB].EPINTEN;
        if (HSUSBD->EP[EPB].EPINTSTS & 0x01)
            EPB_Handler();
        HSUSBD_CLR_EP_INT_FLAG(EPB, IrqSt);
    }
    if (IrqStL & HSUSBD_GINTSTS_EPCIF_Msk) {
        IrqSt = HSUSBD->EP[EPC].EPINTSTS & HSUSBD->EP[EPC].EPINTEN;
        HSUSBD_ENABLE_EP_INT(EPC, 0);
        HSUSBD_CLR_EP_INT_FLAG(EPC, IrqSt);
    }
    if (IrqStL & HSUSBD_GINTSTS_EPDIF_Msk) {
        IrqSt = HSUSBD->EP[EPD].EPINTSTS & HSUSBD->EP[EPD].EPINTEN;
        HSUSBD_ENABLE_EP_INT(EPD, 0);
        HSUSBD_CLR_EP_INT_FLAG(EPD, IrqSt);
    }
    if (IrqStL & (HSUSBD_GINTSTS_EPEIF_Msk | HSUSBD_GINTSTS_EPFIF_Msk)) {
        IrqSt = HSUSBD->EP[(IrqStL & HSUSBD_GINTSTS_EPEIF_Msk) ? EPE : EPF].EPINTSTS;
        HSUSBD_CLR_EP_INT_FLAG((IrqStL & HSUSBD_GINTSTS_EPEIF_Msk) ? EPE : EPF, IrqSt);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* HID Endpoint Handlers                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void EPA_Handler(void)  /* Interrupt IN handler */
{
    HID_SetInReport();
}

void EPB_Handler(void)  /* Interrupt OUT handler */
{
    uint32_t len, i;
    len = HSUSBD->EP[EPB].EPDATCNT & 0xffff;
    /* Q-03: Boundary check - prevent buffer overflow */
    if (len > sizeof(g_u8OutBuff))
        len = sizeof(g_u8OutBuff);
    for (i = 0; i < len; i++)
        g_u8OutBuff[i] = HSUSBD->EP[EPB].EPDAT_BYTE;
    HID_GetOutReport(g_u8OutBuff, len);
}

/*---------------------------------------------------------------------------------------------------------*/
/* HID Initialization                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void HID_InitForHighSpeed(void)
{
    /* EPA ==> Interrupt IN, EP1 */
    HSUSBD_SetEpBufAddr(EPA, EPA_BUF_BASE, EPA_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPA, EPA_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPA, INT_IN_EP_NUM, HSUSBD_EP_CFG_TYPE_INT, HSUSBD_EP_CFG_DIR_IN);
    g_u32EpAMaxPacketSize = EPA_MAX_PKT_SIZE;

    /* EPB ==> Interrupt OUT, EP2 */
    HSUSBD_SetEpBufAddr(EPB, EPB_BUF_BASE, EPB_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPB, EPB_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPB, INT_OUT_EP_NUM, HSUSBD_EP_CFG_TYPE_INT, HSUSBD_EP_CFG_DIR_OUT);
    HSUSBD_ENABLE_EP_INT(EPB, HSUSBD_EPINTEN_RXPKIEN_Msk | HSUSBD_EPINTEN_BUFFULLIEN_Msk);
    g_u32EpBMaxPacketSize = EPB_MAX_PKT_SIZE;

    /* EPC ==> Bulk IN, EP3 */
    HSUSBD_SetEpBufAddr(EPC, EPC_BUF_BASE, EPC_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPC, EPC_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPC, BULK_IN_EP_NUM, HSUSBD_EP_CFG_TYPE_BULK, HSUSBD_EP_CFG_DIR_IN);

    /* EPD ==> Bulk OUT, EP4 */
    HSUSBD_SetEpBufAddr(EPD, EPD_BUF_BASE, EPD_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPD, EPD_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPD, BULK_OUT_EP_NUM, HSUSBD_EP_CFG_TYPE_BULK, HSUSBD_EP_CFG_DIR_OUT);
    HSUSBD_ENABLE_EP_INT(EPD, HSUSBD_EPINTEN_RXPKIEN_Msk);

    g_u32EpMaxPacketSize = EPC_MAX_PKT_SIZE;
}

void HID_InitForFullSpeed(void)
{
    /* EPA ==> Interrupt IN */
    HSUSBD_SetEpBufAddr(EPA, EPA_BUF_BASE, EPA_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPA, EPA_OTHER_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPA, INT_IN_EP_NUM, HSUSBD_EP_CFG_TYPE_INT, HSUSBD_EP_CFG_DIR_IN);
    g_u32EpAMaxPacketSize = EPA_OTHER_MAX_PKT_SIZE;

    /* EPB ==> Interrupt OUT */
    HSUSBD_SetEpBufAddr(EPB, EPB_BUF_BASE, EPB_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPB, EPB_OTHER_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPB, INT_OUT_EP_NUM, HSUSBD_EP_CFG_TYPE_INT, HSUSBD_EP_CFG_DIR_OUT);
    HSUSBD_ENABLE_EP_INT(EPB, HSUSBD_EPINTEN_RXPKIEN_Msk | HSUSBD_EPINTEN_BUFFULLIEN_Msk);
    g_u32EpBMaxPacketSize = EPB_OTHER_MAX_PKT_SIZE;

    /* EPC ==> Bulk IN */
    HSUSBD_SetEpBufAddr(EPC, EPC_BUF_BASE, EPC_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPC, EPC_OTHER_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPC, BULK_IN_EP_NUM, HSUSBD_EP_CFG_TYPE_BULK, HSUSBD_EP_CFG_DIR_IN);

    /* EPD ==> Bulk OUT */
    HSUSBD_SetEpBufAddr(EPD, EPD_BUF_BASE, EPD_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPD, EPD_OTHER_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPD, BULK_OUT_EP_NUM, HSUSBD_EP_CFG_TYPE_BULK, HSUSBD_EP_CFG_DIR_OUT);
    HSUSBD_ENABLE_EP_INT(EPD, HSUSBD_EPINTEN_RXPKIEN_Msk);

    g_u32EpMaxPacketSize = EPC_OTHER_MAX_PKT_SIZE;
}

void HID_Init(void)
{
    /* Enable USB BUS, CEP and EPA~EPD global interrupt */
    HSUSBD_ENABLE_USB_INT(HSUSBD_GINTEN_USBIEN_Msk | HSUSBD_GINTEN_CEPIEN_Msk |
                          HSUSBD_GINTEN_EPAIEN_Msk | HSUSBD_GINTEN_EPBIEN_Msk |
                          HSUSBD_GINTEN_EPCIEN_Msk | HSUSBD_GINTEN_EPDIEN_Msk);
    /* Enable BUS interrupt */
    HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_DMADONEIEN_Msk | HSUSBD_BUSINTEN_RESUMEIEN_Msk |
                          HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_VBUSDETIEN_Msk);
    HSUSBD_SET_ADDR(0);

    /* Control endpoint */
    HSUSBD_SetEpBufAddr(CEP, CEP_BUF_BASE, CEP_BUF_LEN);
    HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);

    HID_InitForHighSpeed();

    /* MSC init */
    g_sCSW.dCSWSignature = CSW_SIGNATURE;
    g_TotalSectors = 60;  /* 60 sectors x 512 = 30KB RAM disk */
    g_u32MassBase = 0x20001000;
    g_u32StorageBase = 0x20002000;
}

/*---------------------------------------------------------------------------------------------------------*/
/* USB Class Request Handler                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void HID_ClassRequest(void)
{
    if (gUsbCmd.bmRequestType & 0x80) {
        switch (gUsbCmd.bRequest) {
        case GET_MAX_LUN:
            g_u8MscStart = 1;
            HSUSBD_PrepareCtrlIn((uint8_t *)&g_u32MSCMaxLun, 1);
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_INTKIEN_Msk);
            break;
        case GET_REPORT: {
            /* S-05 FIX: GET_REPORT must return report data per HID spec */
            /* wValue: high byte = report type (1=Input, 2=Output, 3=Feature), low byte = report ID */
            uint8_t u8ReportType = (gUsbCmd.wValue >> 8) & 0xFF;
            uint8_t u8ReportID = gUsbCmd.wValue & 0xFF;
            uint16_t u16Len = gUsbCmd.wLength;
            
            if (u16Len == 0) {
                HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_ZEROLEN);
                break;
            }
            
            /* Build feature report response: [ReportID][status][data...] */
            memset(g_u8FeatureReport, 0, sizeof(g_u8FeatureReport));
            g_u8FeatureReport[0] = u8ReportID;  /* Report ID */
            g_u8FeatureReport[1] = 0x00;         /* Status: OK */
            g_u8FeatureReport[2] = 0x00;         /* I2C state: idle */
            
            /* Limit transfer length */
            if (u16Len > EPB_MAX_PKT_SIZE)
                u16Len = EPB_MAX_PKT_SIZE;
            
            HSUSBD_PrepareCtrlIn(g_u8FeatureReport, u16Len);
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_INTKIEN_Msk);
            break;
        }
        case GET_IDLE:
            /* Return current idle rate (0 = only on change) */
            HSUSBD_PrepareCtrlIn((uint8_t *)"\x00", 1);
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_INTKIEN_Msk);
            break;
        case GET_PROTOCOL:
            /* Return 0 = Boot Protocol */
            HSUSBD_PrepareCtrlIn((uint8_t *)"\x00", 1);
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_INTKIEN_Msk);
            break;
        default:
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            break;
        }
    } else {
        switch (gUsbCmd.bRequest) {
        case BULK_ONLY_MASS_STORAGE_RESET:
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            break;
        case SET_REPORT: {
            /* S-06 FIX: Handle both Feature (type=3) and Output (type=2) reports */
            uint8_t u8ReportType = (gUsbCmd.wValue >> 8) & 0xFF;
            /* For Output or Feature report, we accept and discard data */
            if (u8ReportType == 0x02 || u8ReportType == 0x03) {
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
                HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            } else {
                HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            }
            break;
        }
        case SET_IDLE:
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            break;
        case SET_PROTOCOL:
        default:
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            break;
        }
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Vendor Request Handler                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void HID_VendorRequest(void)
{
    /* No vendor-specific requests needed */
    HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Helper: convert big-endian uint32                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
static uint32_t get_be32(uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/*---------------------------------------------------------------------------------------------------------*/
/* MSC Command Handlers                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void MSC_RequestSense(void)
{
    memset((uint8_t *)(g_u32MassBase), 0, 18);
    if (g_u8Prevent) {
        g_u8Prevent = 0;
        *(uint8_t *)(g_u32MassBase) = 0x70;
    } else {
        *(uint8_t *)(g_u32MassBase) = 0xf0;
    }
    *(uint8_t *)(g_u32MassBase + 2) = g_au8SenseKey[0];
    *(uint8_t *)(g_u32MassBase + 7) = 0x0a;
    *(uint8_t *)(g_u32MassBase + 12) = g_au8SenseKey[1];
    *(uint8_t *)(g_u32MassBase + 13) = g_au8SenseKey[2];
    MSC_BulkIn(g_u32MassBase, g_sCBW.dCBWDataTransferLength);
    g_au8SenseKey[0] = g_au8SenseKey[1] = g_au8SenseKey[2] = 0;
}

void MSC_ReadCapacity(void)
{
    uint32_t tmp;
    memset((uint8_t *)g_u32MassBase, 0, 8);
    tmp = g_TotalSectors - 1;
    *((uint8_t *)(g_u32MassBase + 0)) = (tmp >> 24) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 1)) = (tmp >> 16) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 2)) = (tmp >> 8) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 3)) = tmp & 0xFF;
    *((uint8_t *)(g_u32MassBase + 6)) = 0x02;
    MSC_BulkIn(g_u32MassBase, g_sCBW.dCBWDataTransferLength);
}

void MSC_ReadFormatCapacity(void)
{
    memset((uint8_t *)g_u32MassBase, 0, 36);
    *((uint8_t *)(g_u32MassBase + 3)) = 0x10;
    *((uint8_t *)(g_u32MassBase + 4)) = (g_TotalSectors >> 24) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 5)) = (g_TotalSectors >> 16) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 6)) = (g_TotalSectors >> 8) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 7)) = g_TotalSectors & 0xFF;
    *((uint8_t *)(g_u32MassBase + 10)) = 0x02;
    *((uint8_t *)(g_u32MassBase + 12)) = (g_TotalSectors >> 24) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 13)) = (g_TotalSectors >> 16) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 14)) = (g_TotalSectors >> 8) & 0xFF;
    *((uint8_t *)(g_u32MassBase + 15)) = g_TotalSectors & 0xFF;
    *((uint8_t *)(g_u32MassBase + 18)) = 0x02;
    MSC_BulkIn(g_u32MassBase, g_sCBW.dCBWDataTransferLength);
}

void MSC_ModeSense10(void)
{
    uint8_t i, j, NumHead = 2, NumSector = 64;
    uint16_t NumCyl = g_TotalSectors / 128;
    *((uint32_t *)g_u32MassBase) = 0;
    *((uint32_t *)(g_u32MassBase + 4)) = 0;

    switch (g_sCBW.au8Data[0]) {
    case 0x01:
        *((uint8_t *)g_u32MassBase) = 19; i = 8;
        for (j = 0; j < 12; j++, i++) *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage_01[j];
        break;
    case 0x05:
        *((uint8_t *)g_u32MassBase) = 39; i = 8;
        for (j = 0; j < 32; j++, i++) *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage_05[j];
        *((uint8_t *)(g_u32MassBase + 12)) = NumHead;
        *((uint8_t *)(g_u32MassBase + 13)) = NumSector;
        *((uint8_t *)(g_u32MassBase + 16)) = (NumCyl >> 8) & 0xFF;
        *((uint8_t *)(g_u32MassBase + 17)) = NumCyl & 0xFF;
        break;
    case 0x1B:
        *((uint8_t *)g_u32MassBase) = 19; i = 8;
        for (j = 0; j < 12; j++, i++) *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage_1B[j];
        break;
    case 0x1C:
        *((uint8_t *)g_u32MassBase) = 15; i = 8;
        for (j = 0; j < 8; j++, i++) *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage_1C[j];
        break;
    case 0x3F:
        *((uint8_t *)g_u32MassBase) = 0x47; i = 8;
        for (j = 0; j < 12; j++, i++) *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage_01[j];
        for (j = 0; j < 32; j++, i++) *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage_05[j];
        for (j = 0; j < 12; j++, i++) *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage_1B[j];
        for (j = 0; j < 8; j++, i++) *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage_1C[j];
        *((uint8_t *)(g_u32MassBase + 24)) = NumHead;
        *((uint8_t *)(g_u32MassBase + 25)) = NumSector;
        *((uint8_t *)(g_u32MassBase + 28)) = (NumCyl >> 8) & 0xFF;
        *((uint8_t *)(g_u32MassBase + 29)) = NumCyl & 0xFF;
        break;
    default:
        g_au8SenseKey[0] = 0x05; g_au8SenseKey[1] = 0x24; g_au8SenseKey[2] = 0x00;
    }
    MSC_BulkIn(g_u32MassBase, g_sCBW.dCBWDataTransferLength);
}

void MSC_ModeSense6(void)
{
    uint8_t i;
    for (i = 0; i < 4; i++)
        *((uint8_t *)(g_u32MassBase + i)) = g_au8ModePage[i];
    MSC_BulkIn(g_u32MassBase, g_sCBW.dCBWDataTransferLength);
}

void MSC_BulkOut(uint32_t u32Addr, uint32_t u32Len)
{
    uint32_t u32Loop, i;
    HSUSBD_SET_DMA_WRITE(BULK_OUT_EP_NUM);
    g_hsusbd_ShortPacket = 0;
    u32Loop = u32Len / USBD_MAX_DMA_LEN;
    for (i = 0; i < u32Loop; i++)
        MSC_ActiveDMA(u32Addr + i * USBD_MAX_DMA_LEN, USBD_MAX_DMA_LEN);
    u32Loop = u32Len % USBD_MAX_DMA_LEN;
    if (u32Loop)
        MSC_ActiveDMA(u32Addr + i * USBD_MAX_DMA_LEN, u32Loop);
}

void MSC_BulkIn(uint32_t u32Addr, uint32_t u32Len)
{
    uint32_t u32Loop, i, addr, count;
    HSUSBD_SET_DMA_READ(BULK_IN_EP_NUM);
    u32Loop = u32Len / USBD_MAX_DMA_LEN;
    for (i = 0; i < u32Loop; i++) {
        HSUSBD_ENABLE_EP_INT(EPC, HSUSBD_EPINTEN_TXPKIEN_Msk);
        g_hsusbd_ShortPacket = 0;
        while (1)
            if (HSUSBD_GET_EP_INT_FLAG(EPC) & HSUSBD_EPINTSTS_BUFEMPTYIF_Msk) {
                MSC_ActiveDMA(u32Addr + i * USBD_MAX_DMA_LEN, USBD_MAX_DMA_LEN);
                break;
            }
    }
    addr = u32Addr + i * USBD_MAX_DMA_LEN;
    u32Loop = u32Len % USBD_MAX_DMA_LEN;
    if (u32Loop) {
        count = u32Loop / g_u32EpMaxPacketSize;
        if (count) {
            HSUSBD_ENABLE_EP_INT(EPC, HSUSBD_EPINTEN_TXPKIEN_Msk);
            g_hsusbd_ShortPacket = 0;
            while (1)
                if (HSUSBD_GET_EP_INT_FLAG(EPC) & HSUSBD_EPINTSTS_BUFEMPTYIF_Msk) {
                    MSC_ActiveDMA(addr, count * g_u32EpMaxPacketSize);
                    break;
                }
            addr += (count * g_u32EpMaxPacketSize);
        }
        count = u32Loop % g_u32EpMaxPacketSize;
        if (count) {
            HSUSBD_ENABLE_EP_INT(EPC, HSUSBD_EPINTEN_TXPKIEN_Msk);
            g_hsusbd_ShortPacket = 1;
            while (1)
                if (HSUSBD_GET_EP_INT_FLAG(EPC) & HSUSBD_EPINTSTS_BUFEMPTYIF_Msk) {
                    MSC_ActiveDMA(addr, count);
                    break;
                }
        }
    }
}

void MSC_ReceiveCBW(uint32_t u32Buf)
{
    HSUSBD_SET_DMA_WRITE(BULK_OUT_EP_NUM);
    HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_DMADONEIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk |
                          HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_VBUSDETIEN_Msk);
    HSUSBD_SET_DMA_ADDR(u32Buf);
    HSUSBD_SET_DMA_LEN(31);
    g_hsusbd_DmaDone = 0;
    HSUSBD_ENABLE_DMA();
}

void MSC_ActiveDMA(uint32_t u32Addr, uint32_t u32Len)
{
    HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_DMADONEIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk |
                          HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_VBUSDETIEN_Msk);
    HSUSBD_SET_DMA_ADDR(u32Addr);
    HSUSBD_SET_DMA_LEN(u32Len);
    g_hsusbd_DmaDone = 0;
    HSUSBD_ENABLE_DMA();
    while (g_u8MscStart) {
        if (g_hsusbd_DmaDone) break;
        if (!HSUSBD_IS_ATTACHED()) break;
    }
}

void MSC_AckCmd(uint32_t u32Residue)
{
    g_sCSW.dCSWDataResidue = u32Residue;
    g_sCSW.bCSWStatus = g_u8Prevent;
    HSUSBD_MemCopy((uint8_t *)g_u32MassBase, (uint8_t *)&g_sCSW.dCSWSignature, 16);
    MSC_BulkIn(g_u32MassBase, 13);
    g_u8BulkState = BULK_NORMAL;
}

void MSC_ProcessCmd(void)
{
    uint32_t i;
    if (g_u8BulkState == BULK_NORMAL) {
        g_u8BulkState = BULK_OUT;
        MSC_ReceiveCBW(g_u32MassBase);
    }
    if (g_u8BulkState == BULK_CBW) {
        if (*(uint32_t *)(g_u32MassBase) != CBW_SIGNATURE) {
            g_u8BulkState = BULK_NORMAL;
            return;
        }
        for (i = 0; i < 31; i++)
            *((uint8_t *)(&g_sCBW.dCBWSignature) + i) = *(uint8_t *)(g_u32MassBase + i);
        g_sCSW.dCSWTag = g_sCBW.dCBWTag;

        switch (g_sCBW.u8OPCode) {
        case UFI_READ_10:
            g_u32LbaAddress = get_be32(&g_sCBW.au8Data[0]) * USBD_SECTOR_SIZE;
            MSC_BulkIn(g_u32StorageBase + g_u32LbaAddress, g_sCBW.dCBWDataTransferLength);
            MSC_AckCmd(0);
            break;
        case UFI_WRITE_10:
            g_u32LbaAddress = get_be32(&g_sCBW.au8Data[0]) * USBD_SECTOR_SIZE;
            MSC_BulkOut(g_u32StorageBase + g_u32LbaAddress, g_sCBW.dCBWDataTransferLength);
            MSC_AckCmd(0);
            break;
        case UFI_PREVENT_ALLOW_MEDIUM_REMOVAL:
            if (g_sCBW.au8Data[2] & 0x01) {
                g_au8SenseKey[0] = 0x05; g_au8SenseKey[1] = 0x24; g_au8SenseKey[2] = 0x00;
                g_u8Prevent = 1;
            } else {
                g_u8Prevent = 0;
            }
            MSC_AckCmd(0);
            break;
        case UFI_VERIFY_10:
        case UFI_START_STOP:
        case UFI_TEST_UNIT_READY:
            MSC_AckCmd(0);
            break;
        case UFI_REQUEST_SENSE:
            MSC_RequestSense();
            MSC_AckCmd(0);
            break;
        case UFI_READ_FORMAT_CAPACITY:
            MSC_ReadFormatCapacity();
            MSC_AckCmd(0);
            break;
        case UFI_READ_CAPACITY:
            MSC_ReadCapacity();
            MSC_AckCmd(0);
            break;
        case UFI_MODE_SELECT_10:
            MSC_BulkOut(g_u32StorageBase, g_sCBW.dCBWDataTransferLength);
            MSC_AckCmd(0);
            break;
        case UFI_MODE_SENSE_10:
            MSC_ModeSense10();
            MSC_AckCmd(0);
            break;
        case UFI_MODE_SENSE_6:
            MSC_ModeSense6();
            MSC_AckCmd(0);
            break;
        case UFI_INQUIRY:
            HSUSBD_MemCopy((uint8_t *)(g_u32MassBase), (uint8_t *)g_au8InquiryID, 36);
            MSC_BulkIn(g_u32MassBase, g_sCBW.dCBWDataTransferLength);
            MSC_AckCmd(0);
            break;

        /* Vendor-specific CDB (0xC0-0xFF) - MSC Debug Channel */
        case 0xC0 ... 0xFF:
            MSC_VendorCommand(&g_sCBW);
            break;

        default:
            g_au8SenseKey[0] = 0x05; g_au8SenseKey[1] = 0x20; g_au8SenseKey[2] = 0x00;
            if (g_sCBW.dCBWDataTransferLength > 0)
                MSC_AckCmd(g_sCBW.dCBWDataTransferLength);
            else
                MSC_AckCmd(0);
        }
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* HID I2C Command Processing                                                                               */
/*---------------------------------------------------------------------------------------------------------*/

/* HID Report ID: I2C Write */
int32_t HID_CmdI2CWrite(CMD_T *pCmd)
{
    uint8_t slaveAddr = pCmd->u8Data[0] & 0x7F;
    uint8_t len = pCmd->u8Data[1];
    if (len > I2C_MAX_WRITE_LEN) len = I2C_MAX_WRITE_LEN;

    int32_t ret = I2C_Write(slaveAddr, &pCmd->u8Data[2], len);

    pCmd->u8Cmd = HID_CMD_NONE;
    pCmd->u8Data[0] = (ret == 0) ? 0x00 : 0x01;  /* status */
    pCmd->u8Data[1] = len;

    /* Send response via HID IN */
    {
        uint32_t txLen = 2;
        uint32_t i;
        for (i = 0; i < txLen; i++)
            HSUSBD->EP[EPA].EPDAT_BYTE = pCmd->u8Data[i];
        HSUSBD->EP[EPA].EPTXCNT = txLen;
        HSUSBD_ENABLE_EP_INT(EPA, HSUSBD_EPINTEN_INTKIEN_Msk);
    }
    return ret;
}

/* HID Report ID: I2C Read */
int32_t HID_CmdI2CRead(CMD_T *pCmd)
{
    uint8_t slaveAddr = pCmd->u8Data[0] & 0x7F;
    uint8_t len = pCmd->u8Data[1];
    if (len > I2C_MAX_READ_LEN) len = I2C_MAX_READ_LEN;

    uint8_t readData[64];
    int32_t ret = I2C_Read(slaveAddr, readData, len);

    pCmd->u8Cmd = HID_CMD_NONE;
    pCmd->u8Data[0] = (ret == 0) ? 0x00 : 0x01;  /* status */
    pCmd->u8Data[1] = (ret == 0) ? len : 0;

    /* Send response via HID IN */
    {
        uint32_t txLen = 2 + ((ret == 0) ? len : 0);
        uint32_t i;
        for (i = 0; i < txLen && i < 66; i++)
            HSUSBD->EP[EPA].EPDAT_BYTE = pCmd->u8Data[i];
        if (ret == 0 && len > 0)
            for (i = 0; i < len && (i + 2) < 66; i++)
                HSUSBD->EP[EPA].EPDAT_BYTE = readData[i];
        HSUSBD->EP[EPA].EPTXCNT = txLen;
        HSUSBD_ENABLE_EP_INT(EPA, HSUSBD_EPINTEN_INTKIEN_Msk);
    }
    return ret;
}

/* HID Report ID: I2C Write+Read (combined) */
int32_t HID_CmdI2CWriteRead(CMD_T *pCmd)
{
    uint8_t slaveAddr = pCmd->u8Data[0] & 0x7F;
    uint8_t wlen = pCmd->u8Data[1];
    uint8_t rlen = pCmd->u8Data[2];
    if (wlen > I2C_MAX_WRITE_LEN) wlen = I2C_MAX_WRITE_LEN;
    if (rlen > I2C_MAX_READ_LEN) rlen = I2C_MAX_READ_LEN;

    uint8_t readData[64];
    int32_t ret = I2C_WriteRead(slaveAddr, &pCmd->u8Data[3], wlen, readData, rlen);

    pCmd->u8Cmd = HID_CMD_NONE;
    pCmd->u8Data[0] = (ret == 0) ? 0x00 : 0x01;
    pCmd->u8Data[1] = (ret == 0) ? rlen : 0;

    /* Send response via HID IN */
    {
        uint32_t txLen = 2 + ((ret == 0) ? rlen : 0);
        uint32_t i;
        for (i = 0; i < txLen && i < 66; i++)
            HSUSBD->EP[EPA].EPDAT_BYTE = pCmd->u8Data[i];
        if (ret == 0 && rlen > 0)
            for (i = 0; i < rlen && (i + 2) < 66; i++)
                HSUSBD->EP[EPA].EPDAT_BYTE = readData[i];
        HSUSBD->EP[EPA].EPTXCNT = txLen;
        HSUSBD_ENABLE_EP_INT(EPA, HSUSBD_EPINTEN_INTKIEN_Msk);
    }
    return ret;
}

/* HID Report ID: I2C Scan */
int32_t HID_CmdI2CScan(CMD_T *pCmd)
{
    uint8_t results[8] = {0};
    int32_t count = I2C_Scan(results, 8);
    
    pCmd->u8Cmd = HID_CMD_NONE;
    pCmd->u8Data[0] = 0x00;  /* status OK */
    pCmd->u8Data[1] = (uint8_t)count;
    memcpy(&pCmd->u8Data[2], results, count > 8 ? 8 : count);

    /* Send response via HID IN */
    {
        uint32_t txLen = 2 + count;
        uint32_t i;
        for (i = 0; i < txLen && i < 66; i++)
            HSUSBD->EP[EPA].EPDAT_BYTE = pCmd->u8Data[i];
        HSUSBD->EP[EPA].EPTXCNT = txLen;
        HSUSBD_ENABLE_EP_INT(EPA, HSUSBD_EPINTEN_INTKIEN_Msk);
    }
    return 0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Process HID command packet from host                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
int32_t ProcessCommand(uint8_t *pu8Buffer, uint32_t u32BufferLen)
{
    uint32_t u32sum;
    CMD_T cmd;

    /* Copy and validate */
    if (u32BufferLen > sizeof(CMD_T)) u32BufferLen = sizeof(CMD_T);
    HSUSBD_MemCopy((uint8_t *)&cmd, pu8Buffer, u32BufferLen);

    /* Check size and signature */
    if ((cmd.u8Size > sizeof(CMD_T)) || (cmd.u8Size > u32BufferLen))
        return -1;
    if (cmd.u32Signature != HID_CMD_SIGNATURE)
        return -1;

    /* Checksum */
    u32sum = CalCheckSum((uint8_t *)&cmd, cmd.u8Size);
    if (u32sum != cmd.u32Checksum)
        return -1;

    /* Save command for async processing */
    gCmd = cmd;

    /* Process command */
    switch (cmd.u8Cmd) {
    case HID_CMD_I2C_WRITE:
        HID_CmdI2CWrite(&gCmd);
        break;
    case HID_CMD_I2C_READ:
        HID_CmdI2CRead(&gCmd);
        break;
    case HID_CMD_I2C_WRITEREAD:
        HID_CmdI2CWriteRead(&gCmd);
        break;
    case HID_CMD_I2C_SCAN:
        HID_CmdI2CScan(&gCmd);
        break;
    default:
        return -1;
    }
    return 0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* HID OUT report handler (data from host)                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void HID_GetOutReport(uint8_t *pu8EpBuf, uint32_t u32Size)
{
    /* Check and process the HID command packet */
    if (ProcessCommand(pu8EpBuf, u32Size)) {
        /* Unknown command - send NAK by doing nothing */
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* HID IN report handler (data to host)                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
void HID_SetInReport(void)
{
    /* For HID I2C bridge, responses are sent synchronously in the command handlers.
       This handler is called for EP0 IN (not EPA). No-op for our protocol. */
}

/*---------------------------------------------------------------------------------------------------------*/
/* Checksum calculation                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
uint32_t CalCheckSum(uint8_t *buf, uint32_t size)
{
    uint32_t sum = 0;
    uint32_t i;
    for (i = 0; i < size; i++)
        sum += buf[i];
    return sum;
}
