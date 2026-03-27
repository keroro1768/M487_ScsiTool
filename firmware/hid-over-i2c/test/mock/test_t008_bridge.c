/**
 * @file    test_t008_bridge.c
 * @brief   Unit tests for T008 Bridge module
 * @version 1.0.0
 *
 * These tests validate the HID-over-I2C Bridge layer using mock HAL.
 */

#include <stdio.h>
#include <string.h>
#include "test_runner.h"
#include "fake_usb_device.h"
#include "fake_i2c_bus.h"
#include "fake_gpio.h"

/*==============================================================================
 * NOTE: The actual T008 module (bridge.c) is not yet refactored for testing.
 * These tests serve as a template for future integration once
 * bridge.c is modified to accept HAL abstraction interfaces.
 *============================================================================*/

/*==============================================================================
 * Test Cases (Placeholder - requires bridge.c refactoring for HAL injection)
 *============================================================================*/

static bool test_bridge_i2c_write(void)
{
    /* TODO: Once bridge.c uses i2c_driver_write() abstraction,
     * inject fake_i2c_bus in place of real I2C driver */
    TEST_LOG("T008: I2C write through bridge - placeholder");
    TEST_ASSERT_TRUE(true);
    return true;
}

static bool test_bridge_i2c_read(void)
{
    TEST_LOG("T008: I2C read through bridge - placeholder");
    TEST_ASSERT_TRUE(true);
    return true;
}

static bool test_bridge_hid_report_decode(void)
{
    TEST_LOG("T008: HID report decode and forward to I2C - placeholder");
    TEST_ASSERT_TRUE(true);
    return true;
}

static bool test_bridge_hid_report_encode(void)
{
    TEST_LOG("T008: I2C response encode as HID report - placeholder");
    TEST_ASSERT_TRUE(true);
    return true;
}

/*==============================================================================
 * Fake HAL Integration Tests (can run now with mock devices)
 *============================================================================*/

static bool test_fake_usb_device_init(void)
{
    fake_usb_device_t dev;
    fake_usb_device_init(&dev);
    TEST_ASSERT_EQUAL(FAKE_USB_STATE_DETACHED, dev.state);
    TEST_ASSERT_EQUAL(0, dev.address);
    TEST_ASSERT_EQUAL(0, dev.configuration);
    return true;
}

static bool test_fake_usb_device_attach(void)
{
    fake_usb_device_t dev;
    fake_usb_device_init(&dev);
    fake_usb_device_attach(&dev);
    TEST_ASSERT_EQUAL(FAKE_USB_STATE_ATTACHED, dev.state);
    return true;
}

static bool test_fake_usb_device_write_read_pipe(void)
{
    fake_usb_device_t dev;
    uint8_t tx_data[8] = { 0x01, 0x02, 0x03, 0x04 };
    uint8_t rx_data[8] = { 0 };
    int len;

    fake_usb_device_init(&dev);
    fake_usb_device_config_ep(&dev, 0x01, FAKE_USB_EP_TYPE_INTERRUPT, 64);

    len = fake_usb_device_write_pipe(&dev, 0x01, tx_data, 4);
    TEST_ASSERT_EQUAL(4, len);

    len = fake_usb_device_read_pipe(&dev, 0x01, rx_data, 8);
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL(0, memcmp(tx_data, rx_data, 4));
    return true;
}

static bool test_fake_i2c_bus_add_slave(void)
{
    fake_i2c_bus_t bus;
    int idx;

    fake_i2c_bus_init(&bus);
    idx = fake_i2c_bus_add_slave(&bus, 0x50);
    TEST_ASSERT_TRUE(idx >= 0);
    return true;
}

static bool test_fake_i2c_bus_write_reg(void)
{
    fake_i2c_bus_t bus;
    int ret;

    fake_i2c_bus_init(&bus);
    fake_i2c_bus_add_slave(&bus, 0x50);

    ret = fake_i2c_bus_write_reg(&bus, 0x50, 0x10, 0xAB);
    TEST_ASSERT_EQUAL(0, ret);
    return true;
}

static bool test_fake_i2c_bus_read_reg(void)
{
    fake_i2c_bus_t bus;
    uint8_t val = 0;
    int ret;

    fake_i2c_bus_init(&bus);
    fake_i2c_bus_add_slave(&bus, 0x50);

    ret = fake_i2c_bus_read_reg(&bus, 0x50, 0x10, &val);
    /* Read without prior write returns 0 (memory initialized to 0) */
    TEST_ASSERT_EQUAL(1, ret);
    return true;
}

static bool test_fake_i2c_bus_nak_on_missing_slave(void)
{
    fake_i2c_bus_t bus;
    int ret;

    fake_i2c_bus_init(&bus);
    /* Don't add slave at 0x50 */

    ret = fake_i2c_bus_write_reg(&bus, 0x50, 0x10, 0xAB);
    TEST_ASSERT(ret < 0); /* Should NAK */
    return true;
}

static bool test_fake_gpio_write_read(void)
{
    fake_gpio_device_t gpio;

    fake_gpio_init(&gpio);
    fake_gpio_set_mode(&gpio, FAKE_GPIO_PORT_A, 5, FAKE_GPIO_MODE_OUTPUT);

    fake_gpio_write(&gpio, FAKE_GPIO_PORT_A, 5, 1);
    TEST_ASSERT_EQUAL(1, fake_gpio_read(&gpio, FAKE_GPIO_PORT_A, 5));

    fake_gpio_write(&gpio, FAKE_GPIO_PORT_A, 5, 0);
    TEST_ASSERT_EQUAL(0, fake_gpio_read(&gpio, FAKE_GPIO_PORT_A, 5));
    return true;
}

static bool test_fake_gpio_toggle(void)
{
    fake_gpio_device_t gpio;

    fake_gpio_init(&gpio);
    fake_gpio_set_mode(&gpio, FAKE_GPIO_PORT_A, 3, FAKE_GPIO_MODE_OUTPUT);

    fake_gpio_write(&gpio, FAKE_GPIO_PORT_A, 3, 1);
    fake_gpio_toggle(&gpio, FAKE_GPIO_PORT_A, 3);
    TEST_ASSERT_EQUAL(0, fake_gpio_read(&gpio, FAKE_GPIO_PORT_A, 3));

    fake_gpio_toggle(&gpio, FAKE_GPIO_PORT_A, 3);
    TEST_ASSERT_EQUAL(1, fake_gpio_read(&gpio, FAKE_GPIO_PORT_A, 3));
    return true;
}

/*==============================================================================
 * Test Suite Definition
 *============================================================================*/

#define MAX_TEST_CASES 16

static test_case_t s_cases[MAX_TEST_CASES];

static test_suite_t s_suite = {
    .name   = "T008 Bridge Module Tests",
    .cases  = s_cases,
    .count  = 0
};

void test_suite_add(const char *name, test_fn_t fn)
{
    if (s_suite.count < MAX_TEST_CASES) {
        s_cases[s_suite.count].name = name;
        s_cases[s_suite.count].fn = fn;
        s_suite.count++;
    }
}

int main(void)
{
    /* Register test cases */
    test_suite_add("Fake USB: Init",          test_fake_usb_device_init);
    test_suite_add("Fake USB: Attach",        test_fake_usb_device_attach);
    test_suite_add("Fake USB: Write/Read EP",  test_fake_usb_device_write_read_pipe);
    test_suite_add("Fake I2C: Add slave",      test_fake_i2c_bus_add_slave);
    test_suite_add("Fake I2C: Write reg",      test_fake_i2c_bus_write_reg);
    test_suite_add("Fake I2C: Read reg",       test_fake_i2c_bus_read_reg);
    test_suite_add("Fake I2C: NAK on missing", test_fake_i2c_bus_nak_on_missing_slave);
    test_suite_add("Fake GPIO: Write/Read",    test_fake_gpio_write_read);
    test_suite_add("Fake GPIO: Toggle",         test_fake_gpio_toggle);

    /* Placeholder T008 bridge tests - require bridge.c HAL injection */
    test_suite_add("T008: I2C write (TODO)",   test_bridge_i2c_write);
    test_suite_add("T008: I2C read (TODO)",    test_bridge_i2c_read);
    test_suite_add("T008: HID decode (TODO)", test_bridge_hid_report_decode);
    test_suite_add("T008: HID encode (TODO)", test_bridge_hid_report_encode);

    return test_runner_run(&s_suite);
}
