/**
 * @file    fake_usb_device.h
 * @brief   Mock USB Device HAL for unit testing
 * @version 1.0.0
 */

#ifndef FAKE_USB_DEVICE_H
#define FAKE_USB_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Mock Configuration
 *============================================================================*/

#define FAKE_USB_MAX_EP_SIZE    64
#define FAKE_USB_NUM_ENDPOINTS  4

/*==============================================================================
 * Mock Types
 *============================================================================*/

typedef enum {
    FAKE_USB_STATE_DETACHED = 0,
    FAKE_USB_STATE_ATTACHED,
    FAKE_USB_STATE_POWERED,
    FAKE_USB_STATE_DEFAULT,
    FAKE_USB_STATE_ADDRESS,
    FAKE_USB_STATE_CONFIGURED,
    FAKE_USB_STATE_SUSPENDED
} fake_usb_state_t;

typedef enum {
    FAKE_USB_EP_TYPE_CONTROL = 0,
    FAKE_USB_EP_TYPE_ISOCHRONOUS,
    FAKE_USB_EP_TYPE_BULK,
    FAKE_USB_EP_TYPE_INTERRUPT
} fake_usb_ep_type_t;

typedef struct {
    uint8_t  ep_addr;
    fake_usb_ep_type_t type;
    uint8_t *buffer;
    uint16_t buffer_size;
    uint16_t data_len;
    bool     stalled;
    bool     busy;
} fake_usb_ep_t;

typedef struct {
    fake_usb_state_t state;
    uint8_t  address;
    uint8_t  configuration;
    bool     remote_wakeup;
    bool     self_powered;
    uint16_t max_packet_size;
    fake_usb_ep_t eps[FAKE_USB_NUM_ENDPOINTS];
} fake_usb_device_t;

/*==============================================================================
 * Mock Call Log (for test assertions)
 *============================================================================*/

typedef enum {
    FAKE_USB_CALL_INIT,
    FAKE_USB_CALL_START,
    FAKE_USB_CALL_STOP,
    FAKE_USB_CALL_WRITE_PIPE,
    FAKE_USB_CALL_READ_PIPE,
    FAKE_USB_CALL_SET_ADDRESS,
    FAKE_USB_CALL_SET_CONFIGURATION,
    FAKE_USB_CALL_STALL_EP,
    FAKE_USB_CALL_CLEAR_STALL,
    FAKE_USB_CALL_IS_ATTACHED,
    FAKE_USB_CALL_COUNT
} fake_usb_call_type_t;

#define FAKE_USB_LOG_MAX 64

typedef struct {
    fake_usb_call_type_t type;
    uint32_t              arg0;
    uint32_t              arg1;
    uint32_t              arg2;
} fake_usb_call_log_entry_t;

typedef struct {
    fake_usb_call_log_entry_t entries[FAKE_USB_LOG_MAX];
    int                       count;
} fake_usb_call_log_t;

/*==============================================================================
 * Mock API
 *============================================================================*/

/**
 * @brief   Initialize the fake USB device
 * @param   pDev - Pointer to fake USB device
 */
void fake_usb_device_init(fake_usb_device_t *pDev);

/**
 * @brief   Reset the fake USB device to default state
 * @param   pDev - Pointer to fake USB device
 */
void fake_usb_device_reset(fake_usb_device_t *pDev);

/**
 * @brief   Simulate USB attach event
 * @param   pDev - Pointer to fake USB device
 */
void fake_usb_device_attach(fake_usb_device_t *pDev);

/**
 * @brief   Simulate USB detach event
 * @param   pDev - Pointer to fake USB device
 */
void fake_usb_device_detach(fake_usb_device_t *pDev);

/**
 * @brief   Configure an endpoint
 * @param   pDev - Pointer to fake USB device
 * @param   ep_addr - Endpoint address (IN: 0x80+ep, OUT: ep)
 * @param   type - Endpoint type
 * @param   max_pkt_size - Maximum packet size
 */
int fake_usb_device_config_ep(fake_usb_device_t *pDev, uint8_t ep_addr,
                              fake_usb_ep_type_t type, uint16_t max_pkt_size);

/**
 * @brief   Write data to an endpoint (simulate host OUT transfer)
 * @param   pDev - Pointer to fake USB device
 * @param   ep_addr - Endpoint address
 * @param   pData - Data to write
 * @param   len - Data length
 * @return  Bytes written
 */
int fake_usb_device_write_pipe(fake_usb_device_t *pDev, uint8_t ep_addr,
                              const uint8_t *pData, uint16_t len);

/**
 * @brief   Read data from an endpoint (simulate host IN transfer)
 * @param   pDev - Pointer to fake USB device
 * @param   ep_addr - Endpoint address
 * @param   pData - Buffer to read into
 * @param   max_len - Maximum bytes to read
 * @return  Bytes read
 */
int fake_usb_device_read_pipe(fake_usb_device_t *pDev, uint8_t ep_addr,
                              uint8_t *pData, uint16_t max_len);

/**
 * @brief   Set device address
 * @param   pDev - Pointer to fake USB device
 * @param   addr - New device address
 */
void fake_usb_device_set_address(fake_usb_device_t *pDev, uint8_t addr);

/**
 * @brief   Set device configuration
 * @param   pDev - Pointer to fake USB device
 * @param   config - Configuration value
 */
void fake_usb_device_set_configuration(fake_usb_device_t *pDev, uint8_t config);

/**
 * @brief   Get call log
 * @return  Pointer to call log
 */
fake_usb_call_log_t *fake_usb_device_get_log(void);

/**
 * @brief   Clear call log
 */
void fake_usb_device_clear_log(void);

/**
 * @brief   Assert that a specific call was made
 * @param   call_type - Type of call
 * @param   arg0 - Expected arg0 (0 = don't care)
 * @return  true if call was found
 */
bool fake_usb_device_was_called(fake_usb_call_type_t call_type, uint32_t arg0);

#ifdef __cplusplus
}
#endif

#endif /* FAKE_USB_DEVICE_H */
