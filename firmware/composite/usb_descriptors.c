/**
 * @file     usb_descriptors.c
 * @brief    USB Descriptors for M487 Composite Device
 * @version  2.0.0
 * 
 * Composite Device Configuration:
 *   Interface 0: HID I2C Bridge (Interrupt EP1 IN + EP2 OUT)
 *   Interface 1: USB Mass Storage (Bulk EP3 IN + EP4 OUT)
 * 
 * VID = 0x0416, PID = 0x5020
 */

#include "NuMicro.h"
#include "hid_i2c.h"

/* Forward declarations for descriptors defined later */
/* HID Report Descriptor size (42 bytes - calculated from actual descriptor) */
#define HID_RPT_SIZE  42

/*---------------------------------------------------------------------------------------------------------*/
/* USB Device Descriptor                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8DeviceDescriptor[LEN_DEVICE] = {
    LEN_DEVICE,             /* bLength */
    DESC_DEVICE,            /* bDescriptorType */
    0x10, 0x02,           /* bcdUSB: USB 2.0 */
    0x00,                  /* bDeviceClass: Composite (0) */
    0x00,                  /* bDeviceSubClass */
    0x00,                  /* bDeviceProtocol */
    CEP_MAX_PKT_SIZE,      /* bMaxPacketSize0 */
    (USBD_VID & 0xFF), ((USBD_VID >> 8) & 0xFF),         /* idVendor */
    (USBD_PID & 0xFF), ((USBD_PID >> 8) & 0xFF),         /* idProduct */
    0x00, 0x00,           /* bcdDevice */
    0x01,                  /* iManufacturer */
    0x02,                  /* iProduct */
    0x00,                  /* iSerialNumber */
    0x01                   /* bNumConfigurations */
};

/*---------------------------------------------------------------------------------------------------------*/
/* USB Qualifier Descriptor                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8QualifierDescriptor[LEN_QUALIFIER] = {
    LEN_QUALIFIER,         /* bLength */
    DESC_QUALIFIER,        /* bDescriptorType */
    0x10, 0x01,           /* bcdUSB */
    0x00,                  /* bDeviceClass */
    0x00,                  /* bDeviceSubClass */
    0x00,                  /* bDeviceProtocol */
    CEP_MAX_PKT_SIZE,      /* bMaxPacketSize0 */
    0x01,                  /* bNumConfigurations */
    0x00                   /* bReserved */
};

/*---------------------------------------------------------------------------------------------------------*/
/* USB Configuration Descriptor                                                                              */
/*  Total length = config + 2*interface + hid + 4*endpoint                                               */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8ConfigDescriptor[] = {
    /* Configuration Descriptor */
    LEN_CONFIG,                                /* bLength */
    DESC_CONFIG,                               /* bDescriptorType */
    /* wTotalLength (calculated) */
    (LEN_CONFIG + LEN_INTERFACE * 2 + LEN_HID + LEN_ENDPOINT * 4) & 0xFF,
    ((LEN_CONFIG + LEN_INTERFACE * 2 + LEN_HID + LEN_ENDPOINT * 4) >> 8) & 0xFF,
    0x02,                                      /* bNumInterfaces */
    0x01,                                      /* bConfigurationValue */
    0x00,                                      /* iConfiguration */
    0x80 | (USBD_SELF_POWERED << 6) | (USBD_REMOTE_WAKEUP << 5), /* bmAttributes */
    USBD_MAX_POWER,                            /* MaxPower */

    /* Interface 0: HID (I2C Bridge) */
    LEN_INTERFACE,                              /* bLength */
    DESC_INTERFACE,                             /* bDescriptorType */
    0x00,                                      /* bInterfaceNumber */
    0x00,                                      /* bAlternateSetting */
    0x02,                                      /* bNumEndpoints */
    0x03,                                      /* bInterfaceClass: HID */
    0x00,                                      /* bInterfaceSubClass */
    0x00,                                      /* bInterfaceProtocol */
    0x00,                                      /* iInterface */

    /* HID Descriptor */
    LEN_HID,                                   /* bLength */
    DESC_HID,                                  /* bDescriptorType */
    0x10, 0x01,                               /* bcdHID */
    0x00,                                      /* bCountryCode */
    0x01,                                      /* bNumDescriptors */
    DESC_HID_RPT,                              /* bDescriptorType */
    /* wDescriptorLength */
    HID_RPT_SIZE & 0xFF,
    (HID_RPT_SIZE >> 8) & 0xFF,

    /* EP1: Interrupt IN */
    LEN_ENDPOINT,                              /* bLength */
    DESC_ENDPOINT,                             /* bDescriptorType */
    (INT_IN_EP_NUM | 0x80),                   /* bEndpointAddress: IN EP1 */
    0x03,                                      /* bmAttributes: Interrupt */
    (EPA_MAX_PKT_SIZE & 0xFF), ((EPA_MAX_PKT_SIZE >> 8) & 0xFF), /* wMaxPacketSize */
    HID_DEFAULT_INT_IN_INTERVAL,               /* bInterval */

    /* EP2: Interrupt OUT */
    LEN_ENDPOINT,                              /* bLength */
    DESC_ENDPOINT,                             /* bDescriptorType */
    (INT_OUT_EP_NUM | 0x00),                   /* bEndpointAddress: OUT EP2 */
    0x03,                                      /* bmAttributes: Interrupt */
    (EPB_MAX_PKT_SIZE & 0xFF), ((EPB_MAX_PKT_SIZE >> 8) & 0xFF), /* wMaxPacketSize */
    HID_DEFAULT_INT_IN_INTERVAL,               /* bInterval */

    /* Interface 1: Mass Storage (BOT) */
    LEN_INTERFACE,                              /* bLength */
    DESC_INTERFACE,                             /* bDescriptorType */
    0x01,                                       /* bInterfaceNumber */
    0x00,                                       /* bAlternateSetting */
    0x02,                                       /* bNumEndpoints */
    0x08,                                       /* bInterfaceClass: Mass Storage */
    0x05,                                       /* bInterfaceSubClass: SCSI */
    0x50,                                       /* bInterfaceProtocol: BOT */
    0x00,                                       /* iInterface */

    /* EP3: Bulk IN */
    LEN_ENDPOINT,                              /* bLength */
    DESC_ENDPOINT,                             /* bDescriptorType */
    (BULK_IN_EP_NUM | 0x80),                  /* bEndpointAddress: IN EP3 */
    0x02,                                      /* bmAttributes: Bulk */
    (EPC_MAX_PKT_SIZE & 0xFF), ((EPC_MAX_PKT_SIZE >> 8) & 0xFF), /* wMaxPacketSize */
    0x00,                                       /* bInterval */

    /* EP4: Bulk OUT */
    LEN_ENDPOINT,                              /* bLength */
    DESC_ENDPOINT,                             /* bDescriptorType */
    (BULK_OUT_EP_NUM | 0x00),                  /* bEndpointAddress: OUT EP4 */
    0x02,                                      /* bmAttributes: Bulk */
    (EPD_MAX_PKT_SIZE & 0xFF), ((EPD_MAX_PKT_SIZE >> 8) & 0xFF), /* wMaxPacketSize */
    0x00                                        /* bInterval */
};

/*---------------------------------------------------------------------------------------------------------*/
/* HID Report Descriptor                                                                                     */
/*  Report format:
 *    Report ID 0x01: I2C Write    [0x01][slave_addr][len][data...]
 *    Report ID 0x02: I2C Read     [0x02][slave_addr][len]
 *    Report ID 0x03: I2C WriteRead [0x03][slave_addr][wlen][rlen][wdata...]
 *    Report ID 0x04: I2C Scan     [0x04]
 *---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8HIDReportDescriptor[] = {
    /* Report ID 1: I2C Write (host -> device) */
    0x06, 0x00, 0xFF,     /* Usage Page: Vendor Defined */
    0x09, 0x01,            /* Usage: 0x01 */
    0xA1, 0x01,            /* Collection: Application */
    0x85, 0x01,            /*   Report ID 1 */
    0x09, 0x01,            /*   Usage: Write command */
    0x15, 0x00,            /*   Logical Min: 0 */
    0x26, 0xFF, 0x00,      /*   Logical Max: 255 */
    0x75, 0x08,            /*   Report Size: 8 */
    0x96, 0x40, 0x00,      /*   Report Count: 64 bytes (EPA_MAX_PKT_SIZE) */
    0x81, 0x02,            /*   Input: Data, Variable, Absolute */
    0xC0,                  /* End Collection */

    /* Report ID 2: I2C Read (device -> host) */
    0x06, 0x00, 0xFF,
    0x09, 0x02,
    0xA1, 0x01,
    0x85, 0x02,
    0x09, 0x02,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x96, 0x40, 0x00,      /* Report Count: 64 bytes */
    0x81, 0x02,            /* Input */
    0xC0,

    /* Report ID 3: I2C WriteRead (host -> device) */
    0x06, 0x00, 0xFF,
    0x09, 0x03,
    0xA1, 0x01,
    0x85, 0x03,
    0x09, 0x03,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x96, 0x40, 0x00,
    0x81, 0x02,
    0xC0,

    /* Report ID 4: I2C Scan (host -> device) */
    0x06, 0x00, 0xFF,
    0x09, 0x04,
    0xA1, 0x01,
    0x85, 0x04,
    0x09, 0x04,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x96, 0x01, 0x00,      /* Report Count: 1 */
    0x81, 0x02,
    0xC0
};

/*---------------------------------------------------------------------------------------------------------*/
/* String Descriptors                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8LangIDDescriptor[] = {
    0x04, 0x03,           /* bLength, bDescriptorType */
    0x09, 0x04            /* wLANGID: English (US) */
};

const uint8_t gu8VendorStringDescriptor[] = {
    14, 0x03,            /* bLength, bDescriptorType */
    'N', 0x00,
    'u', 0x00,
    'v', 0x00,
    'o', 0x00,
    't', 0x00,
    'o', 0x00,
    'n', 0x00
};

const uint8_t gu8ProductStringDescriptor[] = {
    30, 0x03,            /* bLength, bDescriptorType */
    'M', 0x00,
    '4', 0x00,
    '8', 0x00,
    '7', 0x00,
    ' ', 0x00,
    'U', 0x00,
    'S', 0x00,
    'B', 0x00,
    ' ', 0x00,
    'C', 0x00,
    'o', 0x00,
    'm', 0x00,
    'p', 0x00,
    'o', 0x00,
    's', 0x00,
    'i', 0x00,
    't', 0x00,
    'e', 0x00
};

/* High Speed and Full Speed Config Descriptors (same as standard config for composite device) */
/* For HS: use EPA=EP1(512), EPB=EP2(512), EPC=EP3(512), EPD=EP4(512) */
/* For FS: use EPA=EP1(64), EPB=EP2(64), EPC=EP3(64), EPD=EP4(64) */
extern const uint8_t gu8ConfigDescriptor[];  /* Forward reference - defined above */

/* String descriptor pointer array (for S_HSUSBD_INFO_T.gu8StringDesc) */
const uint8_t *gu8StringDescriptor[] = {
    (const uint8_t *)gu8LangIDDescriptor,          /* String 0: Language ID */
    (const uint8_t *)gu8VendorStringDescriptor,   /* String 1: Manufacturer */
    (const uint8_t *)gu8ProductStringDescriptor,  /* String 2: Product */
    NULL                                          /* String 3: Serial Number (none) */
};

/* HID Report Descriptor sizes (for S_HSUSBD_INFO_T.gu32HidReportSize) */
const uint32_t gu32HIDReportSize[1] = {
    HID_RPT_SIZE   /* HID Report Descriptor size for Interface 0 */
};

/* HID Descriptor indices within Configuration Descriptor (for S_HSUSBD_INFO_T.gu32ConfigHidDescIdx) */
const uint32_t gu32ConfigHidDescIdx[2] = {
    25,  /* Interface 0: HID - HID descriptor starts at byte 25 of config descriptor */
    0    /* Interface 1: MSC - no HID descriptor */
};
