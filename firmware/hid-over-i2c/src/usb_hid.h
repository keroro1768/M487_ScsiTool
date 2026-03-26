/**
 * @file     usb_hid.h
 * @brief    USB HID Device Layer for HID-over-I2C Bridge
 * @version  1.0.0
 * 
 * Implements USB HID device with:
 *   - EP0: Control endpoint (HID class requests)
 *   - EP1: Interrupt IN (Input Reports from I2C device)
 *   - EP2: Interrupt OUT (Output Reports to I2C device)
 * 
 * VID = 0x0416 (Nuvoton)
 * PID = 0x5050 (HID-over-I2C Bridge)
 */

#ifndef __USB_HID_H__
#define __USB_HID_H__

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------*/
/* USB Configuration                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define USBD_VID                0x0416
#define USBD_PID                0x5050
#define USBD_MAX_POWER          50     /* 100mA */
#define EP0_MAX_PKT_SIZE        64
#define EP1_MAX_PKT_SIZE        64     /* Interrupt IN (FS) */
#define EP2_MAX_PKT_SIZE        64     /* Interrupt OUT (FS) */
#define HS_EP1_MAX_PKT_SIZE     512    /* Interrupt IN (HS) */
#define HS_EP2_MAX_PKT_SIZE     512    /* Interrupt OUT (HS) */

/*---------------------------------------------------------------------------------------------------------*/
/* HID Class Requests                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_GET_REPORT           0x01
#define HID_GET_IDLE            0x02
#define HID_GET_PROTOCOL        0x03
#define HID_SET_REPORT          0x09
#define HID_SET_IDLE            0x0A
#define HID_SET_PROTOCOL        0x0B

/*---------------------------------------------------------------------------------------------------------*/
/* Report Types                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_RPT_TYPE_INPUT      0x01
#define HID_RPT_TYPE_OUTPUT     0x02
#define HID_RPT_TYPE_FEATURE    0x03

/*---------------------------------------------------------------------------------------------------------*/
/* Protocol                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_PROTOCOL_BOOT       0x00
#define HID_PROTOCOL_REPORT     0x01

/*---------------------------------------------------------------------------------------------------------*/
/* USB Return Codes                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define USBD_OK                 0
#define USBD_ERR_BUSY          -1
#define USBD_ERR_STALL         -2
#define USBD_ERR_TIMEOUT       -3
#define USBD_ERR_PARAM         -4

/*---------------------------------------------------------------------------------------------------------*/
/* USB Descriptor Lengths                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define LEN_DEVICE              18
#define LEN_CONFIG              9
#define LEN_INTERFACE           9
#define LEN_ENDPOINT           7
#define LEN_HID                9
#define LEN_STRING              4

/*---------------------------------------------------------------------------------------------------------*/
/* USB Descriptor Types                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define DESC_DEVICE             0x01
#define DESC_CONFIG            0x02
#define DESC_INTERFACE         0x04
#define DESC_ENDPOINT          0x05
#define DESC_HID               0x21
#define DESC_REPORT            0x22
#define DESC_STRING            0x03
#define DESC_QUALIFIER         0x06

/*---------------------------------------------------------------------------------------------------------*/
/* USB Device States                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
    USBD_STATE_DETACHED = 0,
    USBD_STATE_ATTACHED,
    USBD_STATE_POWERED,
    USBD_STATE_DEFAULT,
    USBD_STATE_ADDRESS,
    USBD_STATE_CONFIGURED
} USBD_STATE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* HID Report Buffer                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_IN_REPORT_SIZE      64
#define HID_OUT_REPORT_SIZE     64

/*---------------------------------------------------------------------------------------------------------*/
/* USB HID Callbacks (implemented by upper layer)                                                           */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Callback: Host requests Input Report
 * @param   reportID    Report ID (0 if not used)
 * @param   reportType  HID_RPT_TYPE_INPUT
 * @param   pBuf       Buffer to fill with report data
 * @param   pLen       [in/out] Input: max size, Output: actual size
 * @return  0 on success, negative on error
 * 
 * Called when host sends GET_REPORT (Input) request.
 * Bridge should fetch Input Report from I2C device and return it.
 */
typedef int32_t (*HID_GET_REPORT_CB)(uint8_t reportID, uint8_t reportType, 
                                      uint8_t *pBuf, uint16_t *pLen);

/**
 * @brief   Callback: Host sends Output/Feature Report
 * @param   reportID    Report ID (0 if not used)
 * @param   reportType  HID_RPT_TYPE_OUTPUT or HID_RPT_TYPE_FEATURE
 * @param   pBuf       Report data from host
 * @param   len        Report length
 * @return  0 on success, negative on error
 * 
 * Called when host sends SET_REPORT request.
 * Bridge should forward report to I2C device.
 */
typedef int32_t (*HID_SET_REPORT_CB)(uint8_t reportID, uint8_t reportType,
                                      const uint8_t *pBuf, uint16_t len);

/*---------------------------------------------------------------------------------------------------------*/
/* USB HID Configuration                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint16_t maxPower;        /* In mA */
    HID_GET_REPORT_CB onGetReport;
    HID_SET_REPORT_CB onSetReport;
} USB_HID_Config_t;

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Initialize USB HID device
 * @param   pConfig     USB HID configuration
 * @return  USBD_OK on success
 */
int32_t USB_HID_Init(const USB_HID_Config_t *pConfig);

/**
 * @brief   Start USB device (enable pull-up)
 * @return  None
 */
void USB_HID_Start(void);

/**
 * @brief   Stop USB device (disable pull-up)
 * @return  None
 */
void USB_HID_Stop(void);

/**
 * @brief   Check if USB is attached
 * @return  1 if attached, 0 if not
 */
uint8_t USB_HID_IsAttached(void);

/**
 * @brief   Send Input Report to host via EP1
 * @param   pBuf       Report data
 * @param   len        Report length (max EP1_MAX_PKT_SIZE)
 * @return  USBD_OK on success
 * 
 * Called by bridge when I2C device has Input Report ready.
 */
int32_t USB_HID_SendInputReport(const uint8_t *pBuf, uint16_t len);

/**
 * @brief   Get current protocol (boot or report)
 * @return  Current protocol (HID_PROTOCOL_BOOT or HID_PROTOCOL_REPORT)
 */
uint8_t USB_HID_GetProtocol(void);

/**
 * @brief   Set current protocol
 * @param   protocol   HID_PROTOCOL_BOOT or HID_PROTOCOL_REPORT
 * @return  None
 */
void USB_HID_SetProtocol(uint8_t protocol);

/**
 * @brief   Get current idle rate
 * @return  Idle rate in 4ms units
 */
uint8_t USB_HID_GetIdleRate(void);

/**
 * @brief   Set idle rate
 * @param   idle       Idle rate in 4ms units
 * @return  None
 */
void USB_HID_SetIdleRate(uint8_t idle);

#endif /* __USB_HID_H__ */
