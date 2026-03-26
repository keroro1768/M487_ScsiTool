/**
 * @file     usb_hid.c
 * @brief    USB HID Device Layer - STUB
 * @version  1.0.0
 * 
 * This is a STUB implementation.
 * Full USB HID implementation requires integration with BSP's HSUSBD framework.
 * 
 * The actual implementation should follow the BSP's HSUSBD HID Transfer example.
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
/* Stub Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

int32_t USB_HID_Init(const USB_HID_Config_t *pConfig)
{
    if (pConfig != NULL) {
        s_onGetReport = pConfig->onGetReport;
        s_onSetReport = pConfig->onSetReport;
    }
    return USBD_OK;
}

void USB_HID_Start(void)
{
    s_isAttached = 1;
}

void USB_HID_Stop(void)
{
    s_isAttached = 0;
}

uint8_t USB_HID_IsAttached(void)
{
    return s_isAttached;
}

int32_t USB_HID_SendInputReport(const uint8_t *pBuf, uint16_t len)
{
    /* Stub: would send via HSUSBD EP1 */
    (void)pBuf;
    (void)len;
    return USBD_OK;
}

uint8_t USB_HID_GetProtocol(void)
{
    return s_protocol;
}

void USB_HID_SetProtocol(uint8_t protocol)
{
    s_protocol = protocol;
}

uint8_t USB_HID_GetIdleRate(void)
{
    return s_idleRate;
}

void USB_HID_SetIdleRate(uint8_t idle)
{
    s_idleRate = idle;
}

/*---------------------------------------------------------------------------------------------------------*/
/* USB Descriptors (placeholders)                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
const uint8_t gu8DeviceDescriptor[18] = {
    18, 1, 0x10, 0x02, 0, 0, 0, 64, 0x16, 0x04, 0x50, 0x50, 0, 0, 1, 0, 2, 0
};
