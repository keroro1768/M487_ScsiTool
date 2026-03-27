/**
 * @file    fake_i2c_bus.h
 * @brief   Mock I2C Bus HAL for unit testing
 * @version 1.0.0
 */

#ifndef FAKE_I2C_BUS_H
#define FAKE_I2C_BUS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Mock Configuration
 *============================================================================*/

#define FAKE_I2C_MAX_SLAVES      8
#define FAKE_I2C_MAX_XFER_SIZE   256
#define FAKE_I2C_LOG_MAX         128

/*==============================================================================
 * Mock Types
 *============================================================================*/

typedef enum {
    FAKE_I2C_STATE_IDLE = 0,
    FAKE_I2C_STATE_BUSY,
    FAKE_I2C_STATE_ERROR
} fake_i2c_state_t;

typedef enum {
    FAKE_I2C_OP_START = 0,
    FAKE_I2C_OP_STOP,
    FAKE_I2C_OP_SEND_ADDR,
    FAKE_I2C_OP_WRITE,
    FAKE_I2C_OP_READ,
    FAKE_I2C_OP_ACK,
    FAKE_I2C_OP_NACK
} fake_i2c_op_type_t;

typedef struct {
    uint8_t  addr;         /* I2C slave address */
    bool     is_read;      /* True for read, false for write */
    uint8_t  reg_addr;     /* Register address (if applicable) */
    uint8_t  data[FAKE_I2C_MAX_XFER_SIZE];
    uint16_t data_len;
    bool     nak;          /* Simulate NAK response */
} fake_i2c_xfer_t;

typedef struct {
    fake_i2c_op_type_t op;
    uint32_t            arg;
} fake_i2c_log_entry_t;

typedef struct {
    fake_i2c_log_entry_t entries[FAKE_I2C_LOG_MAX];
    int                  count;
    fake_i2c_xfer_t      last_xfer;
} fake_i2c_bus_t;

/*==============================================================================
 * Mock Slave Device (for simulation responses)
 *============================================================================*/

typedef struct {
    uint8_t  addr;
    bool     present;
    uint8_t  memory[256];       /* Simple register memory map */
    uint8_t  memory_size;
    bool     nak_on_read;       /* Simulate NAK on read */
    bool     nak_on_write;      /* Simulate NAK on write */
} fake_i2c_slave_t;

/*==============================================================================
 * Mock API
 *============================================================================*/

/**
 * @brief   Initialize the fake I2C bus
 * @param   pBus - Pointer to fake I2C bus
 */
void fake_i2c_bus_init(fake_i2c_bus_t *pBus);

/**
 * @brief   Reset the fake I2C bus to idle state
 * @param   pBus - Pointer to fake I2C bus
 */
void fake_i2c_bus_reset(fake_i2c_bus_t *pBus);

/**
 * @brief   Add a mock slave device on the bus
 * @param   pBus - Pointer to fake I2C bus
 * @param   addr - I2C slave address (7-bit)
 * @return  Slave index or -1 on failure
 */
int fake_i2c_bus_add_slave(fake_i2c_bus_t *pBus, uint8_t addr);

/**
 * @brief   Remove a mock slave device from the bus
 * @param   pBus - Pointer to fake I2C bus
 * @param   addr - I2C slave address (7-bit)
 */
void fake_i2c_bus_remove_slave(fake_i2c_bus_t *pBus, uint8_t addr);

/**
 * @brief   Simulate I2C master write operation
 * @param   pBus - Pointer to fake I2C bus
 * @param   addr - Slave address (7-bit)
 * @param   pData - Data to write
 * @param   len - Data length
 * @return  0 on success, negative on failure
 */
int fake_i2c_bus_write(fake_i2c_bus_t *pBus, uint8_t addr, const uint8_t *pData, uint16_t len);

/**
 * @brief   Simulate I2C master read operation
 * @param   pBus - Pointer to fake I2C bus
 * @param   addr - Slave address (7-bit)
 * @param   pData - Buffer to read into
 * @param   len - Bytes to read
 * @return  Bytes read, negative on failure
 */
int fake_i2c_bus_read(fake_i2c_bus_t *pBus, uint8_t addr, uint8_t *pData, uint16_t len);

/**
 * @brief   Simulate I2C register write
 * @param   pBus - Pointer to fake I2C bus
 * @param   addr - Slave address (7-bit)
 * @param   reg - Register address
 * @param   value - Value to write
 * @return  0 on success, negative on failure
 */
int fake_i2c_bus_write_reg(fake_i2c_bus_t *pBus, uint8_t addr, uint8_t reg, uint8_t value);

/**
 * @brief   Simulate I2C register read
 * @param   pBus - Pointer to fake I2C bus
 * @param   addr - Slave address (7-bit)
 * @param   reg - Register address
 * @param   pValue - Pointer to store read value
 * @return  0 on success, negative on failure
 */
int fake_i2c_bus_read_reg(fake_i2c_bus_t *pBus, uint8_t addr, uint8_t reg, uint8_t *pValue);

/**
 * @brief   Get operation log
 * @return  Pointer to log
 */
fake_i2c_log_entry_t *fake_i2c_bus_get_log(fake_i2c_bus_t *pBus);

/**
 * @brief   Clear operation log
 * @param   pBus - Pointer to fake I2C bus
 */
void fake_i2c_bus_clear_log(fake_i2c_bus_t *pBus);

/**
 * @brief   Get last transfer info
 * @param   pBus - Pointer to fake I2C bus
 * @return  Pointer to last transfer
 */
fake_i2c_xfer_t *fake_i2c_bus_get_last_xfer(fake_i2c_bus_t *pBus);

/**
 * @brief   Check if bus was idle before last operation
 * @param   pBus - Pointer to fake I2C bus
 * @return  true if bus was idle
 */
bool fake_i2c_bus_was_idle(fake_i2c_bus_t *pBus);

/**
 * @brief   Force NAK on next operation (for error simulation)
 * @param   pBus - Pointer to fake I2C bus
 * @param   enable - Enable NAK simulation
 */
void fake_i2c_bus_force_nak(fake_i2c_bus_t *pBus, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* FAKE_I2C_BUS_H */
