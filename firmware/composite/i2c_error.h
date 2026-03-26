/**
 * @file     i2c_error.h
 * @brief    I2C Error Codes - Unified Error Return Values
 * @version  2.0.0
 * 
 * All I2C functions should return these standardized error codes.
 * This ensures consistent error handling across the codebase.
 */

#ifndef __I2C_ERROR_H__
#define __I2C_ERROR_H__

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Error Codes                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @name I2C Error Codes
 * @brief Standardized return values for I2C operations
 * 
 * @note All I2C functions should return one of these codes (or positive byte count for success)
 */
///@{
#define I2C_OK                       0       /**< Operation completed successfully */

#define I2C_ERR_NACK                -1       /**< NACK received (device not responding) */
#define I2C_ERR_TIMEOUT             -2       /**< Bus timeout */
#define I2C_ERR_BUSY                -3       /**< Bus is busy */
#define I2C_ERR_ARBITRATION_LOST    -4       /**< Arbitration lost */
#define I2C_ERR_NACK_RETRY_EXCEEDED -5       /**< NACK retry limit exceeded */
#define I2C_ERR_INVALID_PARAM       -6       /**< Invalid parameter */
#define I2C_ERR_NULL_PTR            -7       /**< NULL pointer passed */
#define I2C_ERR_LENGTH               -8       /**< Invalid length */
#define I2C_ERR_BUFF_OVERFLOW       -9       /**< Buffer overflow */
#define I2C_ERR_NOT_CONNECTED       -10      /**< Device not connected */
#define I2C_ERR_PROTOTYPE          -11      /**< Protocol error */
#define I2C_ERR_UNKNOWN             -99      /**< Unknown error */
///@}

/*---------------------------------------------------------------------------------------------------------*/
/* I2C Status Checks                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief Check if I2C result is an error
 * @param result I2C function return value
 * @return 1 if error, 0 if success
 */
#define I2C_IS_ERROR(result)         ((result) < I2C_OK)

/**
 * @brief Check if I2C result indicates success
 * @param result I2C function return value
 * @return 1 if success, 0 if error
 */
#define I2C_IS_OK(result)            ((result) >= I2C_OK)

/**
 * @brief Get I2C error string for debugging
 * @param err Error code
 * @return Human-readable error string
 */
static inline const char* I2C_ErrorString(int32_t err)
{
    switch (err) {
        case I2C_OK:                       return "OK";
        case I2C_ERR_NACK:                 return "NACK";
        case I2C_ERR_TIMEOUT:              return "Timeout";
        case I2C_ERR_BUSY:                 return "Busy";
        case I2C_ERR_ARBITRATION_LOST:    return "Arbitration Lost";
        case I2C_ERR_NACK_RETRY_EXCEEDED:  return "NACK Retry Exceeded";
        case I2C_ERR_INVALID_PARAM:         return "Invalid Parameter";
        case I2C_ERR_NULL_PTR:             return "NULL Pointer";
        case I2C_ERR_LENGTH:               return "Invalid Length";
        case I2C_ERR_BUFF_OVERFLOW:        return "Buffer Overflow";
        case I2C_ERR_NOT_CONNECTED:        return "Not Connected";
        case I2C_ERR_PROTOTYPE:            return "Protocol Error";
        default:                           return "Unknown Error";
    }
}

#endif /* __I2C_ERROR_H__ */
