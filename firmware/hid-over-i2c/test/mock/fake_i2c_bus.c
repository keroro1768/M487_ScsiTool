/**
 * @file    fake_i2c_bus.c
 * @brief   Mock I2C Bus HAL implementation for unit testing
 */

#include <string.h>
#include "fake_i2c_bus.h"

/*==============================================================================
 * Static Variables
 *============================================================================*/

static fake_i2c_slave_t s_slaves[FAKE_I2C_MAX_SLAVES];
static int s_slave_count = 0;
static bool s_force_nak = false;

/*==============================================================================
 * Internal Helpers
 *============================================================================*/

static fake_i2c_slave_t *find_slave(uint8_t addr)
{
    for (int i = 0; i < s_slave_count; i++) {
        if (s_slaves[i].addr == addr && s_slaves[i].present) {
            return &s_slaves[i];
        }
    }
    return NULL;
}

static void log_op(fake_i2c_bus_t *pBus, fake_i2c_op_type_t op, uint32_t arg)
{
    if (pBus->count < FAKE_I2C_LOG_MAX) {
        pBus->entries[pBus->count].op = op;
        pBus->entries[pBus->count].arg = arg;
        pBus->count++;
    }
}

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

void fake_i2c_bus_init(fake_i2c_bus_t *pBus)
{
    memset(pBus, 0, sizeof(fake_i2c_bus_t));
    pBus->last_xfer.addr = 0xFF;
    s_slave_count = 0;
    s_force_nak = false;
}

void fake_i2c_bus_reset(fake_i2c_bus_t *pBus)
{
    memset(pBus->entries, 0, sizeof(pBus->entries));
    pBus->count = 0;
    memset(&pBus->last_xfer, 0, sizeof(pBus->last_xfer));
    s_force_nak = false;
}

int fake_i2c_bus_add_slave(fake_i2c_bus_t *pBus, uint8_t addr)
{
    (void)pBus;
    if (s_slave_count >= FAKE_I2C_MAX_SLAVES) {
        return -1;
    }
    /* Check if already exists */
    if (find_slave(addr) != NULL) {
        return -2;
    }
    s_slaves[s_slave_count].addr = addr;
    s_slaves[s_slave_count].present = true;
    s_slaves[s_slave_count].memory_size = 256;
    s_slaves[s_slave_count].nak_on_read = false;
    s_slaves[s_slave_count].nak_on_write = false;
    s_slave_count++;
    return s_slave_count - 1;
}

void fake_i2c_bus_remove_slave(fake_i2c_bus_t *pBus, uint8_t addr)
{
    (void)pBus;
    for (int i = 0; i < s_slave_count; i++) {
        if (s_slaves[i].addr == addr) {
            s_slaves[i].present = false;
            return;
        }
    }
}

int fake_i2c_bus_write(fake_i2c_bus_t *pBus, uint8_t addr, const uint8_t *pData, uint16_t len)
{
    fake_i2c_slave_t *pSlave = find_slave(addr);

    log_op(pBus, FAKE_I2C_OP_START, 0);
    log_op(pBus, FAKE_I2C_OP_SEND_ADDR, addr << 1); /* Write bit = 0 */

    if (pSlave == NULL) {
        log_op(pBus, FAKE_I2C_OP_NACK, 0);
        return -1; /* NAK - slave not found */
    }

    if (s_force_nak || pSlave->nak_on_write) {
        log_op(pBus, FAKE_I2C_OP_NACK, 0);
        return -2; /* Simulated NAK */
    }

    log_op(pBus, FAKE_I2C_OP_ACK, 0);

    /* Write data */
    for (uint16_t i = 0; i < len; i++) {
        log_op(pBus, FAKE_I2C_OP_WRITE, pData[i]);
        /* First byte may be register address */
        if (i == 0 && pData[i] < pSlave->memory_size) {
            /* Store as "register" for later read */
        }
        log_op(pBus, FAKE_I2C_OP_ACK, 0);
    }

    log_op(pBus, FAKE_I2C_OP_STOP, 0);

    /* Store last transfer */
    pBus->last_xfer.addr = addr;
    pBus->last_xfer.is_read = false;
    memcpy(pBus->last_xfer.data, pData, (len > FAKE_I2C_MAX_XFER_SIZE) ? FAKE_I2C_MAX_XFER_SIZE : len);
    pBus->last_xfer.data_len = len;

    return 0;
}

int fake_i2c_bus_read(fake_i2c_bus_t *pBus, uint8_t addr, uint8_t *pData, uint16_t len)
{
    fake_i2c_slave_t *pSlave = find_slave(addr);

    log_op(pBus, FAKE_I2C_OP_START, 0);
    log_op(pBus, FAKE_I2C_OP_SEND_ADDR, (addr << 1) | 1); /* Read bit = 1 */

    if (pSlave == NULL) {
        log_op(pBus, FAKE_I2C_OP_NACK, 0);
        return -1;
    }

    if (s_force_nak || pSlave->nak_on_read) {
        log_op(pBus, FAKE_I2C_OP_NACK, 0);
        return -2;
    }

    log_op(pBus, FAKE_I2C_OP_ACK, 0);

    /* Read data from slave memory */
    for (uint16_t i = 0; i < len; i++) {
        pData[i] = pSlave->memory[i];
        log_op(pBus, FAKE_I2C_OP_READ, pData[i]);
        /* ACK all bytes except last */
        if (i < len - 1) {
            log_op(pBus, FAKE_I2C_OP_ACK, 0);
        } else {
            log_op(pBus, FAKE_I2C_OP_NACK, 0); /* NAK last byte */
        }
    }

    log_op(pBus, FAKE_I2C_OP_STOP, 0);

    /* Store last transfer */
    pBus->last_xfer.addr = addr;
    pBus->last_xfer.is_read = true;
    memcpy(pBus->last_xfer.data, pData, (len > FAKE_I2C_MAX_XFER_SIZE) ? FAKE_I2C_MAX_XFER_SIZE : len);
    pBus->last_xfer.data_len = len;

    return len;
}

int fake_i2c_bus_write_reg(fake_i2c_bus_t *pBus, uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = { reg, value };
    return fake_i2c_bus_write(pBus, addr, data, 2);
}

int fake_i2c_bus_read_reg(fake_i2c_bus_t *pBus, uint8_t addr, uint8_t reg, uint8_t *pValue)
{
    /* Write register address */
    if (fake_i2c_bus_write(pBus, addr, &reg, 1) != 0) {
        return -1;
    }
    /* Read value */
    return fake_i2c_bus_read(pBus, addr, pValue, 1);
}

fake_i2c_log_entry_t *fake_i2c_bus_get_log(fake_i2c_bus_t *pBus)
{
    return pBus->entries;
}

void fake_i2c_bus_clear_log(fake_i2c_bus_t *pBus)
{
    memset(pBus->entries, 0, sizeof(pBus->entries));
    pBus->count = 0;
}

fake_i2c_xfer_t *fake_i2c_bus_get_last_xfer(fake_i2c_bus_t *pBus)
{
    return &pBus->last_xfer;
}

bool fake_i2c_bus_was_idle(fake_i2c_bus_t *pBus)
{
    return (pBus->last_xfer.addr == 0xFF);
}

void fake_i2c_bus_force_nak(fake_i2c_bus_t *pBus, bool enable)
{
    (void)pBus;
    s_force_nak = enable;
}
