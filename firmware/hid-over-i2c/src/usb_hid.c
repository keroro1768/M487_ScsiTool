/**
 * @file     usb_hid.c
 * @brief    USB HID Device Layer - STUB (研究完成，待完整整合)
 * @version  1.0.0
 * 
 * This is a STUB with preliminary implementation.
 * Full USB HID implementation requires careful integration with BSP's HSUSBD framework.
 * 
 * 研究記錄 (2026-03-27):
 * - 已研究 BSP HSUSBD_HID_Transfer_And_MSC 範例
 * - 已識別關鍵函式: HID_Init, HID_ClassRequest, EPA_Handler, EPB_Handler
 * - 已識別 Descriptor 結構和 endpoint 配置
 * - 完整整合需要參考 BSP 範例中的 USBD20_IRQHandler 實作
 *
 * 待完成:
 * - 整合 HSUSBD 中斷處理常式
 * - 實作完整 HID class request handler
 * - 驗證 endpoint 緩衝區配置
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "usb_hid.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Stub Variables                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
static uint8_t s_protocol = HID_PROTOCOL_REPORT;
static uint8_t s_idleRate = 0;
static uint8_t s_configuration = 0;
static uint8_t s_isAttached = 0;

static HID_GET_REPORT_CB s_onGetReport = NULL;
static HID_SET_REPORT_CB s_onSetReport = NULL;

/*---------------------------------------------------------------------------------------------------------*/
/* Endpoint Buffer Definitions                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_CEP_BUF_BASE   0x00
#define HID_CEP_BUF_LEN   EP0_MAX_PKT_SIZE
#define HID_EP1_BUF_BASE  (HID_CEP_BUF_BASE + HID_CEP_BUF_LEN)     /* 0x40 */
#define HID_EP1_BUF_LEN   EP1_MAX_PKT_SIZE
#define HID_EP2_BUF_BASE  (HID_EP1_BUF_BASE + HID_EP1_BUF_LEN)     /* 0x80 */
#define HID_EP2_BUF_LEN   EP2_MAX_PKT_SIZE

/*---------------------------------------------------------------------------------------------------------*/
/* Internal Report Buffers                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
static uint8_t s_au8InReport[HID_IN_REPORT_SIZE] = {0};
static uint8_t s_au8OutReport[HID_OUT_REPORT_SIZE] = {0};

/*---------------------------------------------------------------------------------------------------------*/
/* USB HID Report Descriptor (Standard Keyboard)                                                           */
/*---------------------------------------------------------------------------------------------------------*/
static const uint8_t s_au8HidReportDescriptor[] = {
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x06,        /* Usage (Keyboard) */
    0xA1, 0x01,        /* Collection (Application) */
    0x05, 0x07,        /*   Usage Page (Key Codes) */
    0x19, 0xE0,        /*   Usage Minimum (224) - Control */
    0x29, 0xE7,        /*   Usage Maximum (231) - Right GUI */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x01,        /*   Logical Maximum (1) */
    0x75, 0x01,        /*   Report Size (1) */
    0x95, 0x08,        /*   Report Count (8) */
    0x81, 0x02,        /*   Input (Data, Variable, Absolute) - Modifier byte */
    0x95, 0x01,        /*   Report Count (1) */
    0x75, 0x08,        /*   Report Size (8) */
    0x81, 0x01,        /*   Input (Constant) - Reserved byte */
    0x95, 0x05,        /*   Report Count (5) */
    0x75, 0x01,        /*   Report Size (1) */
    0x05, 0x08,        /*   Usage Page (LEDs) */
    0x19, 0x01,        /*   Usage Minimum (1) - Num Lock */
    0x29, 0x05,        /*   Usage Maximum (5) - Kana */
    0x91, 0x02,        /*   Output (Data, Variable, Absolute) - LED report */
    0x95, 0x01,        /*   Report Count (1) */
    0x75, 0x03,        /*   Report Size (3) */
    0x91, 0x01,        /*   Output (Constant) - LED report padding */
    0x95, 0x06,        /*   Report Count (6) */
    0x75, 0x08,        /*   Report Size (8) */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x65,        /*   Logical Maximum (101) */
    0x05, 0x07,        /*   Usage Page (Key Codes) */
    0x19, 0x00,        /*   Usage Minimum (0) */
    0x29, 0x65,        /*   Usage Maximum (101) */
    0x81, 0x00,        /*   Input (Data, Array) - Key array (6 bytes) */
    0xC0,              /* End Collection */
};

/*---------------------------------------------------------------------------------------------------------*/
/* USB Descriptors                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8DeviceDescriptor[18] = {
    18,             /* bLength */
    0x01,           /* bDescriptorType: Device */
    0x10, 0x02,    /* bcdUSB: 2.10 */
    0x00,           /* bDeviceClass: 0 (use interface) */
    0x00,           /* bDeviceSubClass */
    0x00,           /* bDeviceProtocol */
    EP0_MAX_PKT_SIZE,  /* bMaxPacketSize0 */
    (USBD_VID & 0xFF), ((USBD_VID >> 8) & 0xFF),   /* idVendor */
    (USBD_PID & 0xFF), ((USBD_PID >> 8) & 0xFF),   /* idProduct */
    0x00, 0x01,    /* bcdDevice: 1.00 */
    0x01,           /* iManufacturer */
    0x02,           /* iProduct */
    0x00,           /* iSerialNumber */
    0x01            /* bNumConfigurations */
};

const uint8_t gu8ConfigDescriptor[] = {
    /* Configuration Descriptor */
    9,              /* bLength */
    0x02,           /* bDescriptorType: Configuration */
    0x29, 0x00,    /* wTotalLength: 41 bytes */
    0x01,           /* bNumInterfaces */
    0x01,           /* bConfigurationValue */
    0x00,           /* iConfiguration */
    0x80,           /* bmAttributes: Bus-powered */
    USBD_MAX_POWER,/* bMaxPower: 100mA */
    
    /* Interface Descriptor */
    9,              /* bLength */
    0x04,           /* bDescriptorType: Interface */
    0x00,           /* bInterfaceNumber */
    0x00,           /* bAlternateSetting */
    0x02,           /* bNumEndpoints */
    0x03,           /* bInterfaceClass: HID */
    0x00,           /* bInterfaceSubClass */
    0x00,           /* bInterfaceProtocol */
    0x00,           /* iInterface */
    
    /* HID Descriptor */
    9,              /* bLength */
    0x21,           /* bDescriptorType: HID */
    0x11, 0x01,    /* bcdHID: 1.11 */
    0x00,           /* bCountryCode */
    0x01,           /* bNumDescriptors: 1 report descriptor */
    0x22,           /* bDescriptorType: Report */
    sizeof(s_au8HidReportDescriptor), 0x00,  /* wDescriptorLength */
    
    /* Endpoint Descriptor: EP1 IN */
    7,              /* bLength */
    0x05,           /* bDescriptorType: Endpoint */
    0x81,           /* bEndpointAddress: IN, EP1 */
    0x03,           /* bmAttributes: Interrupt */
    EP1_MAX_PKT_SIZE, 0x00,  /* wMaxPacketSize */
    0x01,           /* bInterval: 1ms */
    
    /* Endpoint Descriptor: EP2 OUT */
    7,              /* bLength */
    0x05,           /* bDescriptorType: Endpoint */
    0x02,           /* bEndpointAddress: OUT, EP2 */
    0x03,           /* bmAttributes: Interrupt */
    EP2_MAX_PKT_SIZE, 0x00,  /* wMaxPacketSize */
    0x01,           /* bInterval: 1ms */
};

/*---------------------------------------------------------------------------------------------------------*/
/* Stub Functions - Full implementation requires BSP HSUSBD framework integration                         */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Initialize USB HID device
 * @param   pConfig     USB HID configuration
 * @return  USBD_OK on success
 * 
 * STUB: Full implementation needs to:
 * 1. Enable HSUSBD clock
 * 2. Initialize USB PHY
 * 3. Configure endpoints with proper buffer addresses
 * 4. Set up USB interrupt handler
 */
int32_t USB_HID_Init(const USB_HID_Config_t *pConfig)
{
    // STUB: TODO - Initialize HSUSBD hardware, configure endpoints, set up interrupts
    if (pConfig != NULL) {
        s_onGetReport = pConfig->onGetReport;
        s_onSetReport = pConfig->onSetReport;
    }
    return USBD_OK;
}

/**
 * @brief   Start USB device (connect pull-up)
 */
void USB_HID_Start(void)
{
    // STUB: TODO - Enable USB D+ pull-up to signal device attachment
    s_isAttached = 1;
}

/**
 * @brief   Stop USB device (disconnect pull-up)
 */
void USB_HID_Stop(void)
{
    // STUB: TODO - Disable USB D+ pull-up, halt all endpoints
    s_isAttached = 0;
}

/**
 * @brief   Check if USB is attached
 */
uint8_t USB_HID_IsAttached(void)
{
    // STUB: TODO - Return actual VBUS detection state
    return s_isAttached;
}

/**
 * @brief   Send Input Report via EP1 Interrupt IN
 * @param   pBuf       Report data
 * @param   len        Report length
 * @return  USBD_OK on success
 * 
 * STUB: Should use HSUSBD->EP[EPA].EPDAT_BYTE to load data
 */
int32_t USB_HID_SendInputReport(const uint8_t *pBuf, uint16_t len)
{
    // STUB: TODO - Send via HSUSBD interrupt IN endpoint
    (void)pBuf;
    (void)len;
    return USBD_OK;
}

/**
 * @brief   Get current protocol
 */
uint8_t USB_HID_GetProtocol(void)
{
    return s_protocol;
}

/**
 * @brief   Set protocol
 */
void USB_HID_SetProtocol(uint8_t protocol)
{
    s_protocol = protocol;
}

/**
 * @brief   Get idle rate
 */
uint8_t USB_HID_GetIdleRate(void)
{
    return s_idleRate;
}

/**
 * @brief   Set idle rate
 */
void USB_HID_SetIdleRate(uint8_t idle)
{
    s_idleRate = idle;
}

/*---------------------------------------------------------------------------------------------------------*/
/* USB Interrupt Handlers (STUB - need BSP integration)                                                   */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   USB Interrupt Handler - STUB
 * 
 * STUB: Should follow BSP HSUSBD_HID_Transfer_And_MSC example:
 * - Check GINTSTS for USB, CEP, EPA, EPB interrupts
 * - Handle USB reset, suspend, resume
 * - Handle CEP setup packets, control transfers
 * - Handle EPA (IN) and EPB (OUT) endpoint transfers
 */
void USBD20_IRQHandler(void)
{
    // STUB: TODO - Implement full interrupt handling per BSP example
}

/**
 * @brief   EPA Interrupt Handler - EP1 IN
 * 
 * Called when interrupt IN transfer completes.
 * Should call HID_SetInReport() to prepare next report.
 */
void EPA_Handler(void)
{
    // STUB: TODO - Implement EP1 IN handler
}

/**
 * @brief   EPB Interrupt Handler - EP2 OUT
 * 
 * Called when host sends output report.
 * Should read data from EP2 buffer and pass to callback.
 */
void EPB_Handler(void)
{
    // STUB: TODO - Implement EP2 OUT handler
}

/*---------------------------------------------------------------------------------------------------------*/
/* HID Class Request Handlers (STUB)                                                                       */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   HID Class Request Handler - STUB
 * 
 * STUB: Should handle:
 * - GET_REPORT: Return input report via CEP
 * - SET_REPORT: Read output report from host
 * - GET_IDLE / SET_IDLE: Idle rate control
 * - GET_PROTOCOL / SET_PROTOCOL: Boot vs Report protocol
 */
void HID_ClassRequest(void)
{
    // STUB: TODO - Implement HID class-specific requests
}
