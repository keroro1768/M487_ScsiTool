/**
 * @file     usb_hid.c
 * @brief    USB HID Device Layer Implementation
 * @version  1.0.0
 * 
 * Implements USB HID device for HID-over-I2C bridge.
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "hid_parser.h"

/*---------------------------------------------------------------------------------------------------------*/
/* External Symbols                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
extern uint32_t SystemCoreClock;

/*---------------------------------------------------------------------------------------------------------*/
/* USB Descriptor Definitions                                                                               */
/*---------------------------------------------------------------------------------------------------------*/

/** Device Descriptor */
static const uint8_t s_DeviceDescriptor[] = {
    LEN_DEVICE,               /* bLength */
    DESC_DEVICE,             /* bDescriptorType */
    0x10, 0x02,            /* bcdUSB: USB 2.0 */
    0x00,                  /* bDeviceClass: 0 (composite) */
    0x00,                  /* bDeviceSubClass */
    0x00,                  /* bDeviceProtocol */
    EP0_MAX_PKT_SIZE,      /* bMaxPacketSize0 */
    (USBD_VID & 0xFF), ((USBD_VID >> 8) & 0xFF),   /* idVendor */
    (USBD_PID & 0xFF), ((USBD_PID >> 8) & 0xFF),   /* idProduct */
    0x00, 0x01,            /* bcdDevice: 1.00 */
    0x01,                  /* iManufacturer */
    0x02,                  /* iProduct */
    0x00,                  /* iSerialNumber */
    0x01                   /* bNumConfigurations */
};

/** Configuration Descriptor */
static const uint8_t s_ConfigDescriptor[] = {
    /* Configuration Descriptor */
    LEN_CONFIG,             /* bLength */
    DESC_CONFIG,            /* bDescriptorType */
    (LEN_CONFIG + LEN_INTERFACE + LEN_HID + LEN_ENDPOINT * 2) & 0xFF,
    ((LEN_CONFIG + LEN_INTERFACE + LEN_HID + LEN_ENDPOINT * 2) >> 8) & 0xFF,
    0x01,                  /* bNumInterfaces */
    0x01,                  /* bConfigurationValue */
    0x00,                  /* iConfiguration */
    0x80,                  /* bmAttributes: Bus-powered, no remote wakeup */
    USBD_MAX_POWER,        /* MaxPower: 100mA */
    
    /* Interface Descriptor */
    LEN_INTERFACE,          /* bLength */
    DESC_INTERFACE,         /* bDescriptorType */
    0x00,                  /* bInterfaceNumber */
    0x00,                  /* bAlternateSetting */
    0x02,                  /* bNumEndpoints */
    0x03,                  /* bInterfaceClass: HID */
    0x00,                  /* bInterfaceSubClass: No boot */
    0x00,                  /* bInterfaceProtocol: None */
    0x00,                  /* iInterface */
    
    /* HID Descriptor */
    LEN_HID,                /* bLength */
    DESC_HID,               /* bDescriptorType */
    0x10, 0x01,           /* bcdHID: 1.10 */
    0x00,                  /* bCountryCode */
    0x01,                  /* bNumDescriptors */
    DESC_REPORT,           /* bDescriptorType: Report */
    (HID_IN_REPORT_SIZE + HID_OUT_REPORT_SIZE) & 0xFF,
    ((HID_IN_REPORT_SIZE + HID_OUT_REPORT_SIZE) >> 8) & 0xFF,
    
    /* EP1: Interrupt IN */
    LEN_ENDPOINT,           /* bLength */
    DESC_ENDPOINT,          /* bDescriptorType */
    (0x81),               /* bEndpointAddress: IN EP1 */
    0x03,                  /* bmAttributes: Interrupt */
    (EP1_MAX_PKT_SIZE & 0xFF), ((EP1_MAX_PKT_SIZE >> 8) & 0xFF), /* wMaxPacketSize */
    0x0A,                  /* bInterval: 10ms (FS) */
    
    /* EP2: Interrupt OUT */
    LEN_ENDPOINT,           /* bLength */
    DESC_ENDPOINT,          /* bDescriptorType */
    (0x02),               /* bEndpointAddress: OUT EP2 */
    0x03,                  /* bmAttributes: Interrupt */
    (EP2_MAX_PKT_SIZE & 0xFF), ((EP2_MAX_PKT_SIZE >> 8) & 0xFF), /* wMaxPacketSize */
    0x0A                   /* bInterval: 10ms (FS) */
};

/** Report Descriptor */
static const uint8_t s_ReportDescriptor[] = {
    /* Report ID 1: Input Report (bridge -> host) */
    0x06, 0x00, 0xFF,     /* Usage Page: Vendor Defined */
    0x09, 0x01,            /* Usage: Vendor-defined */
    0xA1, 0x01,            /* Collection: Application */
    0x85, 0x01,            /*   Report ID 1 */
    0x09, 0x01,            /*   Usage */
    0x15, 0x00,            /*   Logical Min: 0 */
    0x26, 0xFF, 0x00,     /*   Logical Max: 255 */
    0x75, 0x08,            /*   Report Size: 8 */
    0x96, (HID_IN_REPORT_SIZE - 1) & 0xFF, ((HID_IN_REPORT_SIZE - 1) >> 8) & 0xFF,
                             /*   Report Count */
    0x81, 0x02,            /*   Input: Data, Variable, Absolute */
    0xC0,                  /* End Collection */
    
    /* Report ID 2: Output Report (host -> bridge) */
    0x06, 0x00, 0xFF,
    0x09, 0x02,
    0xA1, 0x01,
    0x85, 0x02,
    0x09, 0x02,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x96, (HID_OUT_REPORT_SIZE - 1) & 0xFF, ((HID_OUT_REPORT_SIZE - 1) >> 8) & 0xFF,
    0x91, 0x02,            /*   Output: Data, Variable, Absolute */
    0xC0,
};

/** String Descriptor: Language */
static const uint8_t s_StringLang[] = {
    0x04, 0x03,           /* bLength, bDescriptorType */
    0x09, 0x04            /* wLANGID: English (US) */
};

/** String Descriptor: Manufacturer */
static const uint8_t s_StringMfg[] = {
    14, 0x03,
    'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

/** String Descriptor: Product */
static const uint8_t s_StringProduct[] = {
    34, 0x03,
    'H', 0, 'I', 0, 'D', 0, '-', 0, 'o', 0, 'v', 0, 'e', 0,
    'r', 0, '-', 0, 'I', 0, '2', 0, 'C', 0, ' ', 0, 'B', 0,
    'r', 0, 'i', 0, 'd', 0, 'g', 0, 'e', 0
};

/*---------------------------------------------------------------------------------------------------------*/
/* USB HID Device State                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    uint8_t  state;
    uint8_t  protocol;
    uint8_t  idleRate;
    uint8_t  remoteWakeup;
    uint8_t  selfPowered;
    uint8_t  configuration;
    uint8_t  isAttached;
    
    /* Callbacks */
    HID_GET_REPORT_CB onGetReport;
    HID_SET_REPORT_CB onSetReport;
    
    /* Buffers */
    uint8_t  ep1TxBuf[EP1_MAX_PKT_SIZE];
    uint8_t  ep2RxBuf[EP2_MAX_PKT_SIZE];
    uint16_t ep1TxLen;
    uint16_t ep2RxLen;
} USB_HID_Dev_t;

static USB_HID_Dev_t s_USBD;

/*---------------------------------------------------------------------------------------------------------*/
/* HSUSBD Callbacks (weak functions to override)                                                           */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   HID Class Request handler
 */
void HID_ClassRequest(void)
{
    uint8_t bmRequestType = HSUSBD->SETUP1_0;
    uint8_t bRequest = HSUSBD->SETUP3_4 & 0xFF;
    uint16_t wValue = HSUSBD->SETUP5_6;
    uint16_t wLength = HSUSBD->SETUP7_8;
    
    /* Handle HID class requests */
    if ((bmRequestType & 0x80) == 0x80) {
        /* Host-to-device (data to device) */
        switch (bRequest) {
        case HID_GET_REPORT:
            /* Device sends report to host */
            break;
        case HID_GET_IDLE:
            HSUSBD->CEPData = s_USBD.idleRate;
            HSUSBD->CEPTXCNT = 1;
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
        case HID_GET_PROTOCOL:
            HSUSBD->CEPData = s_USBD.protocol;
            HSUSBD->CEPTXCNT = 1;
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
        default:
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            break;
        }
    } else {
        /* Device-to-host (data to host) */
        switch (bRequest) {
        case HID_SET_REPORT:
            /* Host sends report to device - prepare to receive */
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
        case HID_SET_IDLE:
            s_USBD.idleRate = (uint8_t)(wValue >> 8);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_INTKIEN_Msk);
            break;
        case HID_SET_PROTOCOL:
            s_USBD.protocol = (uint8_t)(wValue & 0xFF);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
        default:
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            break;
        }
    }
}

/**
 * @brief   HID Vendor Request handler
 */
void HID_VendorRequest(void)
{
    HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
}

/*---------------------------------------------------------------------------------------------------------*/
/* USB Device Handler                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   EP0 Setup packet handler
 */
void USBD_SetupHandler(void)
{
    uint8_t bmRequestType = HSUSBD->SETUP1_0;
    uint8_t bRequest = HSUSBD->SETUP3_4 & 0xFF;
    uint16_t wValue = HSUSBD->SETUP5_6;
    uint16_t wIndex = HSUSBD->SETUP9_10;
    uint16_t wLength = HSUSBD->SETUP7_8;
    
    /* Standard requests */
    if ((bmRequestType & 0x60) == 0x00) {
        switch (bRequest) {
        case 0x00:  /* GET_STATUS */
            HSUSBD->CEPData = 0;
            HSUSBD->CEPData = 0;
            HSUSBD->CEPTXCNT = 2;
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x01:  /* CLEAR_FEATURE */
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x02:  /* Reserved, not used */
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            break;
            
        case 0x03:  /* SET_FEATURE */
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x05:  /* SET_ADDRESS */
            HSUSBD_SET_ADDR(wValue & 0xFF);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x06:  /* GET_DESCRIPTOR */
            if (wValue >> 8 == DESC_DEVICE) {
                HSUSBD_PrepareCtrlIn(s_DeviceDescriptor, LEN_DEVICE);
                HSUSBD->CEPTXCNT = LEN_DEVICE;
            } else if (wValue >> 8 == DESC_CONFIG) {
                HSUSBD_PrepareCtrlIn(s_ConfigDescriptor, sizeof(s_ConfigDescriptor));
                HSUSBD->CEPTXCNT = sizeof(s_ConfigDescriptor);
            } else if (wValue >> 8 == DESC_STRING) {
                if ((wValue & 0xFF) == 0) {
                    HSUSBD_PrepareCtrlIn(s_StringLang, 4);
                    HSUSBD->CEPTXCNT = 4;
                } else if ((wValue & 0xFF) == 1) {
                    HSUSBD_PrepareCtrlIn(s_StringMfg, 14);
                    HSUSBD->CEPTXCNT = 14;
                } else if ((wValue & 0xFF) == 2) {
                    HSUSBD_PrepareCtrlIn(s_StringProduct, 34);
                    HSUSBD->CEPTXCNT = 34;
                } else {
                    HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
                }
            } else {
                HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            }
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x07:  /* SET_DESCRIPTOR */
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            break;
            
        case 0x08:  /* GET_CONFIG */
            HSUSBD->CEPData = s_USBD.configuration;
            HSUSBD->CEPTXCNT = 1;
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x09:  /* SET_CONFIG */
            s_USBD.configuration = (uint8_t)(wValue & 0xFF);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x0A:  /* GET_INTERFACE */
            HSUSBD->CEPData = 0;
            HSUSBD->CEPTXCNT = 1;
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x0B:  /* SET_INTERFACE */
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            break;
            
        case 0x0C:  /* SYNCH_FRAME */
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            break;
            
        default:
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
            break;
        }
    }
    /* Class requests */
    else if ((bmRequestType & 0x60) == 0x20) {
        HID_ClassRequest();
    }
    /* Vendor requests */
    else if ((bmRequestType & 0x60) == 0x40) {
        HID_VendorRequest();
    }
    else {
        HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Endpoint Handlers                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   EP1 (Interrupt IN) handler
 */
void EP1_Handler(void)
{
    /* Send data to host - TXPKIF when buffer empty */
    if (HSUSBD->EP[1].EPINTSTS & 0x01) {
        /* Buffer empty, can send more data if needed */
    }
}

/**
 * @brief   EP2 (Interrupt OUT) handler
 */
void EP2_Handler(void)
{
    uint32_t len, i;
    
    /* Read received data */
    len = HSUSBD->EP[2].EPDATCNT & 0xFFFF;
    for (i = 0; i < len; i++) {
        s_USBD.ep2RxBuf[i] = HSUSBD->EP[2].EPDAT_BYTE;
    }
    s_USBD.ep2RxLen = len;
    
    /* Forward to upper layer (bridge) */
    if (s_USBD.onSetReport != NULL) {
        uint8_t reportID = (len > 0) ? s_USBD.ep2RxBuf[0] : 0;
        uint8_t reportType = HID_RPT_TYPE_OUTPUT;
        const uint8_t *pData = (len > 1) ? &s_USBD.ep2RxBuf[1] : NULL;
        uint16_t dataLen = (len > 1) ? (len - 1) : 0;
        
        s_USBD.onSetReport(reportID, reportType, pData, dataLen);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* USB Interrupt Handler                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   USB ISR
 */
void USBD20_IRQHandler(void)
{
    uint32_t u32INTSTS = HSUSBD->GINTSTS & HSUSBD->GINTEN;
    
    if (u32INTSTS & HSUSBD_GINTSTS_USBIF_Msk) {
        uint32_t u32BusINT = HSUSBD->BUSINTSTS & HSUSBD->BUSINTEN;
        
        if (u32BusINT & HSUSBD_BUSINTSTS_SOFIF_Msk) {
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_SOFIF_Msk);
        }
        
        if (u32BusINT & HSUSBD_BUSINTSTS_RSTIF_Msk) {
            /* USB reset */
            HSUSBD_SwReset();
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk);
            HSUSBD_SET_ADDR(0);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_RSTIF_Msk);
            s_USBD.configuration = 0;
        }
        
        if (u32BusINT & HSUSBD_BUSINTSTS_RESUMEIF_Msk) {
            HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_RESUMEIF_Msk);
        }
        
        if (u32BusINT & HSUSBD_BUSINTSTS_SUSPENDIF_Msk) {
            HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_RESUMEIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_SUSPENDIF_Msk);
        }
        
        if (u32BusINT & HSUSBD_BUSINTSTS_VBUSDETIF_Msk) {
            if (HSUSBD_IS_ATTACHED()) {
                s_USBD.isAttached = 1;
                HSUSBD_ENABLE_USB();
            } else {
                s_USBD.isAttached = 0;
            }
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_VBUSDETIF_Msk);
        }
    }
    
    /* Control endpoint */
    if (u32INTSTS & HSUSBD_GINTSTS_CEPIF_Msk) {
        HSUSBD_CtrlHandler();
    }
    
    /* EP1 (IN) */
    if (u32INTSTS & HSUSBD_GINTSTS_EPAIF_Msk) {
        EP1_Handler();
        HSUSBD_CLR_EP_INT_FLAG(1, HSUSBD->EP[1].EPINTSTS);
    }
    
    /* EP2 (OUT) */
    if (u32INTSTS & HSUSBD_GINTSTS_EPBIF_Msk) {
        EP2_Handler();
        HSUSBD_CLR_EP_INT_FLAG(2, HSUSBD->EP[2].EPINTSTS);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

int32_t USB_HID_Init(const USB_HID_Config_t *pConfig)
{
    memset(&s_USBD, 0, sizeof(s_USBD));
    
    if (pConfig != NULL) {
        s_USBD.onGetReport = pConfig->onGetReport;
        s_USBD.onSetReport = pConfig->onSetReport;
    }
    
    /* Enable USB clock */
    SYS->USBPHY &= ~SYS_USBPHY_HSUSBROLE_Msk;
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_HSUSBROLE_Msk) | SYS_USBPHY_HSUSBEN_Msk;
    
    /* Delay > 10us */
    volatile uint32_t i;
    for (i = 0; i < 1000; i++);
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
    
    CLK_EnableModuleClock(HSUSBD_MODULE);
    
    /* Init USB device */
    HSUSBD->GINTEN = HSUSBD_GINTEN_USBIEN_Msk | HSUSBD_GINTEN_CEPIEN_Msk |
                     HSUSBD_GINTEN_EPAIEN_Msk | HSUSBD_GINTEN_EPBIEN_Msk;
    
    HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_VBUSDETIEN_Msk |
                          HSUSBD_BUSINTEN_RESUMEIEN_Msk | HSUSBD_BUSINTEN_SUSPENDIEN_Msk);
    
    /* EP0 */
    HSUSBD_SET_EP_BUF_ADDR(0, 0);
    HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk | HSUSBD_CEPINTEN_STSDONEIEN_Msk);
    
    /* EP1: Interrupt IN */
    HSUSBD_SET_EP_BUF_ADDR(1, 0x100);
    HSUSBD_SET_MAX_PAYLOAD(1, EP1_MAX_PKT_SIZE);
    HSUSBD_CONFIG_EP(1, 0x81, HSUSBD_EP_CFG_TYPE_INT, HSUSBD_EP_CFG_DIR_IN);
    
    /* EP2: Interrupt OUT */
    HSUSBD_SET_EP_BUF_ADDR(2, 0x200);
    HSUSBD_SET_MAX_PAYLOAD(2, EP2_MAX_PKT_SIZE);
    HSUSBD_CONFIG_EP(2, 0x02, HSUSBD_EP_CFG_TYPE_INT, HSUSBD_EP_CFG_DIR_OUT);
    HSUSBD_ENABLE_EP_INT(2, HSUSBD_EPINTEN_RXPKIEN_Msk);
    
    /* Enable USB interrupt */
    NVIC_EnableIRQ(USBD20_IRQn);
    
    return USBD_OK;
}

void USB_HID_Start(void)
{
    HSUSBD_START();
}

void USB_HID_Stop(void)
{
    HSUSBD_DISABLE_USB();
}

uint8_t USB_HID_IsAttached(void)
{
    return s_USBD.isAttached;
}

int32_t USB_HID_SendInputReport(const uint8_t *pBuf, uint16_t len)
{
    uint16_t i;
    
    if (len > HID_IN_REPORT_SIZE)
        len = HID_IN_REPORT_SIZE;
    
    /* Copy to buffer */
    for (i = 0; i < len; i++) {
        HSUSBD->EP[1].EPDAT_BYTE = pBuf[i];
    }
    HSUSBD->EP[1].EPTXCNT = len;
    
    return USBD_OK;
}

uint8_t USB_HID_GetProtocol(void)
{
    return s_USBD.protocol;
}

void USB_HID_SetProtocol(uint8_t protocol)
{
    s_USBD.protocol = protocol;
}

uint8_t USB_HID_GetIdleRate(void)
{
    return s_USBD.idleRate;
}

void USB_HID_SetIdleRate(uint8_t idle)
{
    s_USBD.idleRate = idle;
}
