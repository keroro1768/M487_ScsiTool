/**
 * @file     i2c_constants.h
 * @brief    I2C Constants and Configuration Values
 * @version  2.0.0
 * 
 * Centralized definitions for all I2C-related magic numbers and constants.
 * Use these instead of raw values throughout the codebase.
 */

#ifndef __I2C_CONSTANTS_H__
#define __I2C_CONSTANTS_H__

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Timeout Values (in iterations)                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @name I2C Timeout Values
 * @brief Timeout values for I2C operations. These are architecture-dependent
 *        and should be tuned for your system clock.
 */
///@{
#define I2C_TIMEOUT_DEFAULT     1000000UL  /**< Default timeout for I2C transactions */
#define I2C_TIMEOUT_SHORT        50000UL    /**< Short timeout for quick operations */
#define I2C_TIMEOUT_ADDR         50000UL    /**< Address phase timeout */
#define I2C_TIMEOUT_BYTE         10000UL    /**< Byte transfer timeout */
#define I2C_TIMEOUT_SCAN         1000UL     /**< Scan probe timeout */
///@}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Retry Configuration                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @name I2C Retry Configuration
 * @brief NACK retry and backoff settings for robust I2C communication
 */
///@{
#define NACK_RETRY_MAX           3          /**< Maximum number of NACK retries */
#define NACK_BACKOFF_BASE_MS     1          /**< Base delay for exponential backoff (ms) */
#define NACK_BACKOFF_MAX_MS      100        /**< Maximum backoff delay (ms) */
#define NACK_SCAN_RETRY_MAX      2          /**< NACK retry for scan operations */
///@}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Buffer Sizes                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @name I2C Buffer Sizes
 * @brief Buffer size limits for I2C operations
 */
///@{
#define I2C_MAX_WRITE_LEN        60         /**< Maximum write data length */
#define I2C_MAX_READ_LEN         62         /**< Maximum read data length */
#define I2C_INTERNAL_TX_SIZE     68         /**< Internal TX buffer size */
#define I2C_INTERNAL_RX_SIZE     64         /**< Internal RX buffer size */
#define I2C_SCAN_RESULT_MAX      8          /**< Maximum scan results */
///@}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Timing Delays                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @name I2C Timing Delays
 * @brief Inter-byte and inter-transaction delays
 */
///@{
#define I2C_SCAN_INTERBYTE_DELAY 100        /**< Delay between scan probes (iterations) */
#define I2C_RESET_DELAY          10000      /**< Device reset wait time (iterations) */
#define I2C_CMD_RESPONSE_DELAY   1000       /**< Command response wait (iterations) */
///@}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Bus Configuration                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @name I2C Bus Configuration
 * @brief I2C bus speed and addressing
 */
///@{
#define I2C_BUS_SPEED_HZ         100000     /**< I2C bus speed: 100kHz */
#define I2C_SLAVE_ADDR_MIN       0x03       /**< Minimum valid slave address */
#define I2C_SLAVE_ADDR_MAX       0x77       /**< Maximum valid slave address */
#define I2C_ADDR_WRITE_BIT       0x00       /**< I2C address write bit (LSB) */
#define I2C_ADDR_READ_BIT        0x01       /**< I2C address read bit (LSB) */
///@}

#endif /* __I2C_CONSTANTS_H__ */
