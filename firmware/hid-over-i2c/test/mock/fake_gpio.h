/**
 * @file    fake_gpio.h
 * @brief   Mock GPIO HAL for unit testing
 * @version 1.0.0
 */

#ifndef FAKE_GPIO_H
#define FAKE_GPIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Mock Types
 *============================================================================*/

typedef enum {
    FAKE_GPIO_MODE_INPUT = 0,
    FAKE_GPIO_MODE_OUTPUT,
    FAKE_GPIO_MODE_OPEN_DRAIN,
    FAKE_GPIO_MODE_QUASI
} fake_gpio_mode_t;

typedef enum {
    FAKE_GPIO_PORT_A = 0,
    FAKE_GPIO_PORT_B,
    FAKE_GPIO_PORT_C,
    FAKE_GPIO_PORT_D,
    FAKE_GPIO_PORT_E,
    FAKE_GPIO_PORT_NUM
} fake_gpio_port_t;

typedef struct {
    fake_gpio_port_t port;
    uint8_t         pin;
} fake_gpio_pin_t;

typedef struct {
    uint32_t pins[FAKE_GPIO_PORT_NUM];        /* Current pin states */
    uint32_t out_vals[FAKE_GPIO_PORT_NUM];     /* Output values */
    uint32_t dir[FAKE_GPIO_PORT_NUM];          /* Direction: 1=output, 0=input */
    uint32_t pull_up[FAKE_GPIO_PORT_NUM];      /* Pull-up enabled */
    uint32_t pull_down[FAKE_GPIO_PORT_NUM];    /* Pull-down enabled */
    uint32_t interrupt_mask[FAKE_GPIO_PORT_NUM];
    uint32_t interrupt_status[FAKE_GPIO_PORT_NUM];
} fake_gpio_device_t;

/*==============================================================================
 * Mock API
 *============================================================================*/

/**
 * @brief   Initialize the fake GPIO device
 * @param   pDev - Pointer to fake GPIO device
 */
void fake_gpio_init(fake_gpio_device_t *pDev);

/**
 * @brief   Reset GPIO to default state
 * @param   pDev - Pointer to fake GPIO device
 */
void fake_gpio_reset(fake_gpio_device_t *pDev);

/**
 * @brief   Set pin mode
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 * @param   pin - Pin number (0-15)
 * @param   mode - Pin mode
 */
void fake_gpio_set_mode(fake_gpio_device_t *pDev, fake_gpio_port_t port,
                        uint8_t pin, fake_gpio_mode_t mode);

/**
 * @brief   Set pin output value
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 * @param   pin - Pin number
 * @param   value - 0 = low, 1 = high
 */
void fake_gpio_write(fake_gpio_device_t *pDev, fake_gpio_port_t port,
                    uint8_t pin, uint8_t value);

/**
 * @brief   Read pin input value
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 * @param   pin - Pin number
 * @return  Pin value (0 or 1)
 */
uint8_t fake_gpio_read(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin);

/**
 * @brief   Toggle pin output (write inverted value)
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 * @param   pin - Pin number
 */
void fake_gpio_toggle(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin);

/**
 * @brief   Enable pull-up on pin
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 * @param   pin - Pin number
 */
void fake_gpio_enable_pullup(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin);

/**
 * @brief   Enable pull-down on pin
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 * @param   pin - Pin number
 */
void fake_gpio_enable_pulldown(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin);

/**
 * @brief   Simulate external interrupt on pin
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 * @param   pin - Pin number
 */
void fake_gpio_trigger_interrupt(fake_gpio_device_t *pDev, fake_gpio_port_t port, uint8_t pin);

/**
 * @brief   Get interrupt status for port
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 * @return  Interrupt status bits
 */
uint32_t fake_gpio_get_interrupt_status(fake_gpio_device_t *pDev, fake_gpio_port_t port);

/**
 * @brief   Clear interrupt status for port
 * @param   pDev - Pointer to fake GPIO device
 * @param   port - GPIO port
 */
void fake_gpio_clear_interrupt(fake_gpio_device_t *pDev, fake_gpio_port_t port);

#ifdef __cplusplus
}
#endif

#endif /* FAKE_GPIO_H */
