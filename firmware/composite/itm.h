/**
 * @file     itm.h
 * @brief    ITM (Instrumentation Trace Macrocell) SWO Trace Header
 * @version  1.0.0
 * 
 * ITM Trace System for M487 Cortex-M4
 * SWO pin: PB8 (Single Wire Output)
 * 
 * Usage:
 *   ITM_Init();                           // Initialize SWO trace
 *   ITM_LOG("Hello world\n");             // Simple string output
 *   ITM_LOG("I2C addr=0x%02X\n", addr);   // Formatted output
 *   ITM_HEX_DUMP(LOG_DBG, "RX", buf, len); // Hex dump
 */

#ifndef __ITM_H__
#define __ITM_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Log Levels                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
    LOG_LEVEL_ERR   = 0,    /**< Error - always printed */
    LOG_LEVEL_WARN  = 1,    /**< Warning */
    LOG_LEVEL_INFO  = 2,    /**< Information */
    LOG_LEVEL_DBG   = 3,    /**< Debug */
    LOG_LEVEL_TRC   = 4,    /**< Trace - most verbose */
} ITM_LogLevel_t;

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Configuration                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
/** Default log level - set to ERR in production, DBG in development */
#ifndef ITM_LOG_LEVEL
#define ITM_LOG_LEVEL   LOG_LEVEL_DBG
#endif

/** ITM Stimulus Port used for trace output */
#define ITM_STIM_PORT   0

/** ITM enable flag - set to 0 to disable all ITM output */
#ifndef ITM_ENABLE
#define ITM_ENABLE      1
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Core Macros                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Check if ITM stimulus port is enabled and ready
 * @note   Returns 1 if ITM is enabled and the FIFO is not full
 */
#if ITM_ENABLE
#define ITM_STIM_FIFO_READY()   (ITM->PORT[ITM_STIM_PORT].u32 != 0)
#else
#define ITM_STIM_FIFO_READY()   (0)
#endif

/**
 * @brief  Send a single character via ITM Stimulus Port 0
 * @param  c  Character to send
 * @note   Blocking wait if FIFO is full
 */
#if ITM_ENABLE
#define ITM_SendChar(c) \
    do { \
        if (ITM_STIM_FIFO_READY()) \
            ITM->PORT[ITM_STIM_PORT].u8 = (uint8_t)(c); \
    } while (0)
#else
#define ITM_SendChar(c)   ((void)0)
#endif

/**
 * @brief  Send a 32-bit word via ITM
 */
#if ITM_ENABLE
#define ITM_SendWord(w) \
    do { \
        if (ITM_STIM_FIFO_READY()) \
            ITM->PORT[ITM_STIM_PORT].u32 = (uint32_t)(w); \
    } while (0)
#else
#define ITM_SendWord(w)   ((void)0)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Logging Macros                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/

/** ITM Log - formatted output with level check */
#define ITM_LOG(fmt, ...) \
    do { \
        if (ITM_ENABLE && (ITM_LOG_LEVEL) >= (LOG_LEVEL_INFO)) \
            ITM_Log(fmt, ##__VA_ARGS__); \
    } while (0)

/** ITM Log Error - always printed regardless of level */
#define ITM_ERR(fmt, ...) \
    do { \
        if (ITM_ENABLE) \
            ITM_Log(fmt, ##__VA_ARGS__); \
    } while (0)

/** ITM Log Debug - only printed if LOG_LEVEL >= DBG */
#define ITM_DBG(fmt, ...) \
    do { \
        if (ITM_ENABLE && (ITM_LOG_LEVEL) >= (LOG_LEVEL_DBG)) \
            ITM_Log(fmt, ##__VA_ARGS__); \
    } while (0)

/** ITM Log Trace - only printed if LOG_LEVEL >= TRC */
#define ITM_TRC(fmt, ...) \
    do { \
        if (ITM_ENABLE && (ITM_LOG_LEVEL) >= (LOG_LEVEL_TRC)) \
            ITM_Log(fmt, ##__VA_ARGS__); \
    } while (0)

/**
 * @brief  ITM Hex Dump - output memory region as hex
 * @param  level  Log level (LOG_LEVEL_*)
 * @param  label  Label/name for this dump
 * @param  data   Pointer to data buffer
 * @param  len    Number of bytes to dump
 */
#define ITM_HEX_DUMP(level, label, data, len) \
    do { \
        if (ITM_ENABLE && (ITM_LOG_LEVEL) >= (level)) \
            ITM_HexDump(label, (const uint8_t *)(data), (int)(len)); \
    } while (0)

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Function Declarations                                                                               */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Initialize ITM (Instrumentation Trace Macrocell) for SWO output
 * @note   Call this after SystemInit() / SYS_Init()
 *         PB8 must be configured as SWO in SYS_Init() before calling this
 * 
 * SWO Configuration:
 *   - PB8 = SWO (Single Wire Output)
 *   - Baud rate: determined by TPI->ACPR divider
 *   - Protocol: 4B/5B encoded SWO Manchester or NRZ
 */
void ITM_Init(void);

/**
 * @brief  Initialize ITM with custom SWO baud rate
 * @param  swo_freq_hz  Desired SWO frequency in Hz (e.g., 2000000 for 2MHz)
 * @note   Actual baud rate may be rounded to nearest dividable value
 */
void ITM_InitWithBaud(uint32_t swo_freq_hz);

/**
 * @brief  Formatted log output via ITM
 * @param  fmt  Printf-style format string
 * @param  ...  Arguments
 * @note   Thread-safe on ARM Cortex-M (using ITM atomic stimulus port)
 */
void ITM_Log(const char *fmt, ...);

/**
 * @brief  Hex dump output via ITM
 * @param  label  Label for the dump
 * @param  data   Data buffer
 * @param  len    Buffer length in bytes
 */
void ITM_HexDump(const char *label, const uint8_t *data, int len);

/**
 * @brief  Get current system tick timestamp string
 * @param  buf   Output buffer (at least 12 bytes)
 * @param  size  Buffer size
 * @note   Format: "HH:MM:SS.mmm"
 */
void ITM_Timestamp(char *buf, int size);

/**
 * @brief  Enable/Disable ITM at runtime
 * @param  enable  1 = enable, 0 = disable
 */
void ITM_Enable(uint8_t enable);

/**
 * @brief  Set runtime log level
 * @param  level  New log level (LOG_LEVEL_*)
 */
void ITM_SetLevel(ITM_LogLevel_t level);

/**
 * @brief  Get current runtime log level
 * @return Current log level
 */
ITM_LogLevel_t ITM_GetLevel(void);

/*---------------------------------------------------------------------------------------------------------*/
/* Application-Level Trace Macros (Module-Specific)                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Module-Specific Trace Macros */
/** @{ */

/** USB Module Trace */
#define USB_TRACE(fmt, ...)   ITM_LOG("[USB] " fmt, ##__VA_ARGS__)
#define USB_ERR(fmt, ...)     ITM_ERR("[USB] ERR: " fmt, ##__VA_ARGS__)
#define USB_DBG(fmt, ...)     ITM_DBG("[USB] " fmt, ##__VA_ARGS__)

/** MSC Module Trace */
#define MSC_TRACE(fmt, ...)   ITM_LOG("[MSC] " fmt, ##__VA_ARGS__)
#define MSC_ERR(fmt, ...)     ITM_ERR("[MSC] ERR: " fmt, ##__VA_ARGS__)
#define MSC_DBG(fmt, ...)     ITM_DBG("[MSC] " fmt, ##__VA_ARGS__)

/** I2C Module Trace */
#define I2C_TRACE(fmt, ...)  ITM_LOG("[I2C] " fmt, ##__VA_ARGS__)
#define I2C_ERR(fmt, ...)     ITM_ERR("[I2C] ERR: " fmt, ##__VA_ARGS__)
#define I2C_DBG(fmt, ...)     ITM_DBG("[I2C] " fmt, ##__VA_ARGS__)

/** HID Module Trace */
#define HID_TRACE(fmt, ...)   ITM_LOG("[HID] " fmt, ##__VA_ARGS__)
#define HID_ERR(fmt, ...)     ITM_ERR("[HID] ERR: " fmt, ##__VA_ARGS__)
#define HID_DBG(fmt, ...)     ITM_DBG("[HID] " fmt, ##__VA_ARGS__)

/** @} */

#endif /* __ITM_H__ */
