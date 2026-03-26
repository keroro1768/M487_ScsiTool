/**
 * @file     usb_descriptors.c
 * @brief    USB Descriptors for M487 Composite Device
 * @version  1.0.0
 */

#include "NuMicro.h"
#include "usb_descriptors.h"
#include "hid_i2c.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Device Descriptor                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8DeviceDescriptor[LEN_DEVICE] = {
    LEN_DEVICE,            // bLength
    DESC_DEVICE,            // bDescriptorType
    0x10, 0x02,           // bcdUSB: USB 2.1
    0x00,                  // bDeviceClass: Composite
    0x00,                  // bDeviceSubClass
    0x00,                  // bDeviceProtocol
    EP0_MAX_PKT_SIZE,      // bMaxPacketSize0
    USB_VID & 0xFF,        // idVendor LSB
    (USB_VID >> 8) & 0xFF, // idVendor MSB
    USB_PID & 0xFF,        // idProduct LSB
    (USB_PID >> 8) & 0xFF, // idProduct MSB
    0x00, 0x00,           // bcdDevice
    0x01,                  // iManufacturer
    0x02,                  // iProduct
    0x03,                  // iSerialNumber
    0x01                   // bNumConfigurations
};

/*---------------------------------------------------------------------------------------------------------*/
/* Configuration Descriptor                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
// Configuration = Interface 0 (MSC) + Interface 1 (HID)
const uint8_t gu8ConfigDescriptor[] = {
    // Configuration Descriptor
    LEN_CONFIG,                                    // bLength
    DESC_CONFIG,                                   // bDescriptorType
    (LEN_CONFIG + LEN_INTERFACE + LEN_ENDPOINT * 2 + // wTotalLength
     LEN_INTERFACE + LEN_HID + LEN_ENDPOINT * 2), 0x00,
    0x02,                                          // bNumInterfaces
    0x01,                                          // bConfigurationValue
    0x00,                                          // iConfiguration
    0xC0,                                          // bmAttributes: Self-powered
    USB_MAX_POWER / 2,                             // MaxPower

    /* Interface 0: Mass Storage (MSC) */
    LEN_INTERFACE,                                 // bLength
    DESC_INTERFACE,                                // bDescriptorType
    0x00,                                          // bInterfaceNumber
    0x00,                                          // bAlternateSetting
    0x02,                                          // bNumEndpoints
    CLASS_MSC,                                     // bInterfaceClass: Mass Storage
    SUBCLASS_MSC_SCSI,                             // bInterfaceSubClass: SCSI
    PROTOCOL_MSC_BOT,                              // bInterfaceProtocol: BOT
    0x00,                                          // iInterface

    /* Endpoint 2: Bulk OUT (MSC Command/Data) */
    LEN_ENDPOINT,                                  // bLength
    DESC_ENDPOINT,                                 // bDescriptorType
    MSC_BULK_OUT_EP,                              // bEndpointAddress: OUT EP2
    0x02,                                          // bmAttributes: Bulk
    0x40, 0x00,                                   // wMaxPacketSize: 64 bytes
    0x00,                                          // bInterval

    /* Endpoint 1: Bulk IN (MSC Response/Data) */
    LEN_ENDPOINT,                                  // bLength
    DESC_ENDPOINT,                                 // bDescriptorType
    MSC_BULK_IN_EP,                               // bEndpointAddress: IN EP1
    0x02,                                          // bmAttributes: Bulk
    0x40, 0x00,                                   // wMaxPacketSize: 64 bytes
    0x00,                                          // bInterval

    /* Interface 1: HID I2C Bridge */
    LEN_INTERFACE,                                 // bLength
    DESC_INTERFACE,                                // bDescriptorType
    0x01,                                          // bInterfaceNumber
    0x00,                                          // bAlternateSetting
    0x02,                                          // bNumEndpoints (EP3 IN + EP5 OUT)
    CLASS_HID,                                     // bInterfaceClass: HID
    0x00,                                          // bInterfaceSubClass
    0x00,                                          // bInterfaceProtocol
    0x00,                                          // iInterface

    /* HID Descriptor */
    LEN_HID,                                       // bLength
    DESC_HID,                                      // bDescriptorType
    0x11, 0x01,                                   // bcdHID: HID 1.11
    0x00,                                          // bCountryCode
    0x01,                                          // bNumDescriptors: 1 report
    DESC_HID_REPORT,                               // bDescriptorType
    sizeof(gu8HidReportDescriptor), 0x00,          // wDescriptorLength

    /* Endpoint 3: Interrupt IN (HID Input Report) */
    LEN_ENDPOINT,                                  // bLength
    DESC_ENDPOINT,                                 // bDescriptorType
    HID_INTERRUPT_EP,                             // bEndpointAddress: IN EP3
    0x03,                                          // bmAttributes: Interrupt
    HID_REPORT_SIZE, 0x00,                         // wMaxPacketSize
    0x01,                                          // bInterval: 1 ms

    /* Endpoint 5: Bulk OUT (HID Output Report / I2C commands) */
    LEN_ENDPOINT,                                  // bLength
    DESC_ENDPOINT,                                 // bDescriptorType
    HID_BULK_OUT_EP,                              // bEndpointAddress: OUT EP5
    0x02,                                          // bmAttributes: Bulk
    0x40, 0x00,                                   // wMaxPacketSize: 64 bytes
    0x00,                                          // bInterval
};

/*---------------------------------------------------------------------------------------------------------*/
/* HID Report Descriptor                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8HidReportDescriptor[] = {
    // I2C Write Report (Host → Device)
    0x06, 0x00, 0xFF,        // Usage Page: Vendor Defined (0xFF00)
    0x09, 0x01,               // Usage: I2C Write
    0xA1, 0x01,               // Collection: Application
    0x85, HID_REPORT_ID_I2C_WRITE,  // Report ID
    0x09, 0x01,               //   Usage: Write
    0x15, 0x00,               //   Logical Minimum: 0
    0x26, 0xFF, 0x00,         //   Logical Maximum: 255
    0x75, 0x08,               //   Report Size: 8 bits
    0x95, 0x01,               //   Report Count: 1
    0x09, 0x01,               //   Usage: (I2C Address)
    0x81, 0x02,               //   Input: Data, Var, Abs (slaveAddr)
    0x09, 0x01,               //   Usage: (Length)
    0x81, 0x02,               //   Input: Data, Var, Abs (length)
    0x95, 0x3C,               //   Report Count: 60 (data)
    0x09, 0x01,               //   Usage: (Data)
    0x81, 0x02,               //   Input: Data, Var, Abs
    0xC0,                     // End Collection

    // I2C Read Report (Host → Device, triggers Device → Host response)
    0x06, 0x00, 0xFF,        // Usage Page: Vendor Defined
    0x09, 0x02,               // Usage: I2C Read
    0xA1, 0x01,               // Collection: Application
    0x85, HID_REPORT_ID_I2C_READ,  // Report ID
    0x09, 0x02,               //   Usage: Read
    0x15, 0x00,               //   Logical Minimum: 0
    0x26, 0xFF, 0x00,         //   Logical Maximum: 255
    0x75, 0x08,               //   Report Size: 8 bits
    0x95, 0x01,               //   Report Count: 1
    0x09, 0x02,               //   Usage: (I2C Address)
    0x81, 0x02,               //   Input: Data, Var, Abs (slaveAddr)
    0x09, 0x02,               //   Usage: (Length)
    0x81, 0x02,               //   Input: Data, Var, Abs (length)
    0x95, 0x3E,               //   Report Count: 62 (response data)
    0x09, 0x02,               //   Usage: (Response Data)
    0x91, 0x02,               //   Output: Data, Var, Abs
    0xC0,                     // End Collection

    // I2C Write+Read Report (Host → Device, triggers Device → Host response)
    0x06, 0x00, 0xFF,        // Usage Page: Vendor Defined
    0x09, 0x03,               // Usage: I2C Write+Read
    0xA1, 0x01,               // Collection: Application
    0x85, HID_REPORT_ID_I2C_WRITEREAD,  // Report ID
    0x09, 0x03,               //   Usage: Write+Read
    0x15, 0x00,               //   Logical Minimum: 0
    0x26, 0xFF, 0x00,         //   Logical Maximum: 255
    0x75, 0x08,               //   Report Size: 8 bits
    0x95, 0x01,               //   Report Count: 1
    0x09, 0x03,               //   Usage: (I2C Address)
    0x81, 0x02,               //   Input: Data, Var, Abs (slaveAddr)
    0x09, 0x03,               //   Usage: (Write Length)
    0x81, 0x02,               //   Input: Data, Var, Abs
    0x09, 0x03,               //   Usage: (Read Length)
    0x81, 0x02,               //   Input: Data, Var, Abs
    0x95, 0x3C,               //   Report Count: 60 (write data)
    0x09, 0x03,               //   Usage: (Write Data)
    0x81, 0x02,               //   Input: Data, Var, Abs
    0x95, 0x3E,               //   Report Count: 62 (response data)
    0x09, 0x03,               //   Usage: (Response Data)
    0x91, 0x02,               //   Output: Data, Var, Abs
    0xC0,                     // End Collection
};

/*---------------------------------------------------------------------------------------------------------*/
/* String Descriptors                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8StringLang[] = {
    4,                       // bLength
    DESC_STRING,              // bDescriptorType
    0x09, 0x04               // Language ID: English (US)
};

const uint8_t gu8VendorString[] = {
    16,                      // bLength
    DESC_STRING,              // bDescriptorType
    'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

const uint8_t gu8ProductString[] = {
    38,                      // bLength
    DESC_STRING,              // bDescriptorType
    'M', 0, '4', 0, '8', 0, '7', 0, ' ', 0,
    'U', 0, 'S', 0, 'B', 0, ' ', 0,
    'C', 0, 'o', 0, 'm', 0, 'p', 0, 'o', 0, 's', 0, 'i', 0, 't', 0, 'e', 0
};

const uint8_t gu8SerialNumberString[] = {
    26,                      // bLength
    DESC_STRING,              // bDescriptorType
    'A', 0, '0', 0, '2', 0, '0', 0, '0', 0, '8', 0,
    '0', 0, '4', 0, '0', 0, '1', 0, '1', 0, '4', 0
};

/*---------------------------------------------------------------------------------------------------------*/
/* Helper Functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void* GetStringDescriptor(uint8_t idx)
{
    switch (idx) {
        case 0: return (void*)gu8StringLang;
        case 1: return (void*)gu8VendorString;
        case 2: return (void*)gu8ProductString;
        case 3: return (void*)gu8SerialNumberString;
        default: return NULL;
    }
}

uint16_t GetHidReportDescriptorSize(void)
{
    return sizeof(gu8HidReportDescriptor);
}
