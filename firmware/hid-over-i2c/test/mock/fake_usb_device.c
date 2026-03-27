/**
 * @file    fake_usb_device.c
 * @brief   Mock USB Device HAL implementation for unit testing
 */

#include <string.h>
#include "fake_usb_device.h"

/*==============================================================================
 * Static Variables
 *============================================================================*/

static fake_usb_call_log_t s_call_log = { .count = 0 };

/*==============================================================================
 * Internal Helpers
 *============================================================================*/

static void log_call(fake_usb_call_type_t type, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    if (s_call_log.count < FAKE_USB_LOG_MAX) {
        s_call_log.entries[s_call_log.count].type = type;
        s_call_log.entries[s_call_log.count].arg0 = arg0;
        s_call_log.entries[s_call_log.count].arg1 = arg1;
        s_call_log.entries[s_call_log.count].arg2 = arg2;
        s_call_log.count++;
    }
}

static fake_usb_ep_t *find_ep(fake_usb_device_t *pDev, uint8_t ep_addr)
{
    for (int i = 0; i < FAKE_USB_NUM_ENDPOINTS; i++) {
        if (pDev->eps[i].ep_addr == ep_addr) {
            return &pDev->eps[i];
        }
    }
    return NULL;
}

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

void fake_usb_device_init(fake_usb_device_t *pDev)
{
    memset(pDev, 0, sizeof(fake_usb_device_t));
    pDev->state = FAKE_USB_STATE_DETACHED;
    pDev->max_packet_size = 64;
    log_call(FAKE_USB_CALL_INIT, 0, 0, 0);
}

void fake_usb_device_reset(fake_usb_device_t *pDev)
{
    memset(pDev->eps, 0, sizeof(pDev->eps));
    pDev->address = 0;
    pDev->configuration = 0;
    pDev->state = FAKE_USB_STATE_DEFAULT;
}

void fake_usb_device_attach(fake_usb_device_t *pDev)
{
    pDev->state = FAKE_USB_STATE_ATTACHED;
    log_call(FAKE_USB_CALL_START, 0, 0, 0);
}

void fake_usb_device_detach(fake_usb_device_t *pDev)
{
    pDev->state = FAKE_USB_STATE_DETACHED;
    log_call(FAKE_USB_CALL_STOP, 0, 0, 0);
}

int fake_usb_device_config_ep(fake_usb_device_t *pDev, uint8_t ep_addr,
                               fake_usb_ep_type_t type, uint16_t max_pkt_size)
{
    (void)max_pkt_size;
    fake_usb_ep_t *pEp = find_ep(pDev, ep_addr);
    if (pEp == NULL) {
        /* Find empty slot */
        for (int i = 0; i < FAKE_USB_NUM_ENDPOINTS; i++) {
            if (pDev->eps[i].ep_addr == 0) {
                pEp = &pDev->eps[i];
                break;
            }
        }
    }
    if (pEp == NULL) {
        return -1; /* No free endpoint */
    }
    pEp->ep_addr = ep_addr;
    pEp->type = type;
    pEp->stalled = false;
    pEp->busy = false;
    return 0;
}

int fake_usb_device_write_pipe(fake_usb_device_t *pDev, uint8_t ep_addr,
                               const uint8_t *pData, uint16_t len)
{
    (void)pDev;
    fake_usb_ep_t *pEp = find_ep(pDev, ep_addr);
    if (pEp == NULL) {
        return -1;
    }
    if (pEp->stalled) {
        return -2;
    }
    uint16_t copy_len = (len > pEp->buffer_size) ? pEp->buffer_size : len;
    memcpy(pEp->buffer, pData, copy_len);
    pEp->data_len = copy_len;
    pEp->busy = false;
    log_call(FAKE_USB_CALL_WRITE_PIPE, ep_addr, (uint32_t)pData, len);
    return copy_len;
}

int fake_usb_device_read_pipe(fake_usb_device_t *pDev, uint8_t ep_addr,
                              uint8_t *pData, uint16_t max_len)
{
    (void)pDev;
    fake_usb_ep_t *pEp = find_ep(pDev, ep_addr);
    if (pEp == NULL) {
        return -1;
    }
    if (pEp->stalled) {
        return -2;
    }
    uint16_t copy_len = (pEp->data_len > max_len) ? max_len : pEp->data_len;
    memcpy(pData, pEp->buffer, copy_len);
    log_call(FAKE_USB_CALL_READ_PIPE, ep_addr, (uint32_t)pData, max_len);
    return copy_len;
}

void fake_usb_device_set_address(fake_usb_device_t *pDev, uint8_t addr)
{
    pDev->address = addr;
    pDev->state = (addr > 0) ? FAKE_USB_STATE_ADDRESS : FAKE_USB_STATE_DEFAULT;
    log_call(FAKE_USB_CALL_SET_ADDRESS, addr, 0, 0);
}

void fake_usb_device_set_configuration(fake_usb_device_t *pDev, uint8_t config)
{
    pDev->configuration = config;
    pDev->state = (config > 0) ? FAKE_USB_STATE_CONFIGURED : FAKE_USB_STATE_ADDRESS;
    log_call(FAKE_USB_CALL_SET_CONFIGURATION, config, 0, 0);
}

fake_usb_call_log_t *fake_usb_device_get_log(void)
{
    return &s_call_log;
}

void fake_usb_device_clear_log(void)
{
    s_call_log.count = 0;
    memset(s_call_log.entries, 0, sizeof(s_call_log.entries));
}

bool fake_usb_device_was_called(fake_usb_call_type_t call_type, uint32_t arg0)
{
    for (int i = 0; i < s_call_log.count; i++) {
        if (s_call_log.entries[i].type == call_type) {
            if (arg0 == 0 || s_call_log.entries[i].arg0 == arg0) {
                return true;
            }
        }
    }
    return false;
}
