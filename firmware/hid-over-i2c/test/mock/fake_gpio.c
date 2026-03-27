/**
 * @file    fake_gpio.c
 * @brief   Mock GPIO HAL implementation for unit testing
 */

#include <string.h>
#include "fake_gpio.h"

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

void fake_gpio_init(fake_gpio_device_t *pDev)
{
    memset(pDev, 0, sizeof(fake_gpio_device_t));
}

void fake_gpio_reset(fake_gpio_device_t *pDev)
{
    memset(pDev, 0, sizeof(fake_gpio_device_t));
}

void fake_gpio_set_mode(fake_gpio_device_t *pDev, fake_gpio_port_t port,
                        uint8_t pin, fake_gpio_mode_t mode)
{
    if (port >= FAKE_GPIO_PORT_NUM || pin >= 16) {
        return;
    }
    uint32_t bit = (1UL << pin);
    if (mode == FAKE_GPIO_MODE_OUTPUT || mode == FAKE_GPIO_MODE_OPEN_DRAIN) {
        pDev->dir[port] |= bit;
    } else {
        pDev->dir[port] &= ~bit;
    }
    (void)mode; /* Currently only input/output distinction is modeled */
}

void fake_gpio_write(fake_gpio_device_t *pDev, fake_gpio_port_t port,
                    uint8_t pin, uint8_t value)
{
    if (port >= FAKE_GPIO_PORT_NUM || pin >= 16) {
        return;
    }
    uint32_t bit = (1UL << pin);
    if (value) {
        pDev->out_vals[port] |= bit;
        pDev->pins[port] = (pDev->pins[port] & ~bit) | (bit & pDev->dir[port]);
    } else {
        pDev->out_vals[port] &= ~bit;
        pDev->pins[port] &= ~(bit & pDev->dir[port]);
    }
}

uint8_t fake_gpio_read(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin)
{
    if (port >= FAKE_GPIO_PORT_NUM || pin >= 16) {
        return 0;
    }
    uint32_t bit = (1UL << pin);
    /* If output, return output register; if input, return pin register */
    if (pDev->dir[port] & bit) {
        return (pDev->out_vals[port] & bit) ? 1 : 0;
    }
    return (pDev->pins[port] & bit) ? 1 : 0;
}

void fake_gpio_toggle(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin)
{
    if (port >= FAKE_GPIO_PORT_NUM || pin >= 16) {
        return;
    }
    uint32_t bit = (1UL << pin);
    pDev->out_vals[port] ^= bit;
    pDev->pins[port] = (pDev->pins[port] & ~bit) | (pDev->out_vals[port] & bit & pDev->dir[port]);
}

void fake_gpio_enable_pullup(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin)
{
    if (port >= FAKE_GPIO_PORT_NUM || pin >= 16) {
        return;
    }
    uint32_t bit = (1UL << pin);
    pDev->pull_up[port] |= bit;
    pDev->pull_down[port] &= ~bit;
}

void fake_gpio_enable_pulldown(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin)
{
    if (port >= FAKE_GPIO_PORT_NUM || pin >= 16) {
        return;
    }
    uint32_t bit = (1UL << pin);
    pDev->pull_down[port] |= bit;
    pDev->pull_up[port] &= ~bit;
}

void fake_gpio_trigger_interrupt(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin)
{
    if (port >= FAKE_GPIO_PORT_NUM || pin >= 16) {
        return;
    }
    uint32_t bit = (1UL << pin);
    if (pDev->interrupt_mask[port] & bit) {
        pDev->interrupt_status[port] |= bit;
    }
}

uint32_t fake_gpio_get_interrupt_status(fake_gpio_device_t *pDev, fake_gpio_port_t port)
{
    if (port >= FAKE_GPIO_PORT_NUM) {
        return 0;
    }
    return pDev->interrupt_status[port];
}

void fake_gpio_clear_interrupt(fake_gpio_device_t *pDev, fake_gpio_port_t port)
{
    if (port >= FAKE_GPIO_PORT_NUM) {
        return;
    }
    pDev->interrupt_status[port] = 0;
}
