/**
 * @file     usb_descriptors.h
 * @brief    USB Descriptors for M487 Composite Device
 * @version  1.0.0
 * 
 * Composite Device:
 *   Interface 0: Mass Storage (MSC) - Bulk endpoints
 *   Interface 1: HID I2C Bridge    - Interrupt endpoint
 */

#ifndef __USB_DESCRIPTORS_H__
#define __USB_DESCRIPTORS_H__

#include <stdint.h>

// Device parameters
#define USB_VID             0x0416  // Nuvoton
#define USB_PID             0x5020  // Composite Device
#define USB_MAX_POWER       100     // 100 mA

// Endpoint parameters
#define EP0_MAX_PKT_SIZE    64
#define MSC_BULK_IN_EP      0x81    // EP1 IN (Bulk)
#define MSC_BULK_OUT_EP     0x02    // EP2 OUT (Bulk)
#define HID_INTERRUPT_EP    0x83    // EP3 IN/OUT (Interrupt)
#define HID_INTERRUPT_EP_ADDR 0x03  // EP3 address

#define HID_BULK_IN_EP      0x84    // EP4 IN (Bulk for HID data)
#define HID_BULK_OUT_EP     0x05    // EP5 OUT (Bulk for HID commands)

#define HID_REPORT_SIZE     64      // HID report max size

// Class codes
#define CLASS_MSC           0x08    // Mass Storage
#define CLASS_HID           0x03    // HID
#define CLASS_VENDOR        0xFF    // Vendor-specific

// Subclass codes
#define SUBCLASS_MSC_SCSI   0x06    // SCSI transparent command set
#define PROTOCOL_MSC_BOT    0x50    // Bulk-Only Transport

// HID Protocol
#define HID_PROTO_NONE      0x00    // No protocol

/*---------------------------------------------------------------------------------------------------------*/
/* USB Descriptors                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

// Device Descriptor
extern const uint8_t gu8DeviceDescriptor[LEN_DEVICE];

// Configuration Descriptor (with all interfaces)
extern const uint8_t gu8ConfigDescriptor[];

// HID Report Descriptor (for I2C Bridge)
extern const uint8_t gu8HidReportDescriptor[];

// String Descriptors
extern const uint8_t gu8StringLang[];
extern const uint8_t gu8VendorString[];
extern const uint8_t gu8ProductString[];
extern const uint8_t gu8SerialNumberString[];

// Get descriptor functions
void* GetStringDescriptor(uint8_t idx);
uint16_t GetHidReportDescriptorSize(void);

#endif // __USB_DESCRIPTORS_H__
