/**
 * @file     uart_debug.h
 * @brief    UART Debug Log System - Structured logging via UART
 * @version  1.0.0
 * 
 * UART Debug Log provides structured logging via UART0.
 * This is a complementary to ITM/SWO trace for environments where
 * SWD debug header is not available.
 * 
 * Hardware:
 *   - UART0 (already initialized in main.c)
 *   - PB12 = UART0 RX, PB13 = UART0 TX
 *   - Baud rate: 115200 (matches UART_Open in main.c)
 *   - Default: 115200-8n1
 * 
 * Log Format:
 *   [LVL][HH:MM:SS.mmm][module] message\r\n
 *   Example: [ERR][14:23:01.123][I2C] NACK retry exceeded addr=0x3C\r\n
 * 
 * Usage:
 *   UART_DBG_Init();                          // Initialize (call once)
 *   UART_LOG("Hello world\n");                // Simple string
 *   UART_LOG("I2C addr=0x%02X\n", addr);     // Formatted
 *   I2C_ERR("NACK at addr=0x%02X\n", addr);  // Module-specific
 * 
 * Log Levels:
 *   0 = ERR   (always printed)
 *   1 = WARN
 *   2 = INFO
 *   3 = DBG
 *   4 = TRC   (most verbose)
 * 
 * Compile-time filter:
 *   Define UART_LOG_LEVEL to set maximum printed level.
 *   Lower levels are compiled out completely.
 * 
 * Runtime filter:
 *   UART_DBG_SetLevel(UART_DBG_LEVEL_DBG);
 *   UART_DBG_SetLevel(UART_DBG_LEVEL_ERR);  // Silent mode
 */

#ifndef __UART_DEBUG_H__
#define __UART_DEBUG_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*---------------------------------------------------------------------------------------------------------*/
/* Log Levels                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
    UART_DBG_LEVEL_ERR   = 0,   /**< Error only */
    UART_DBG_LEVEL_WARN  = 1,   /**< Warning and above */
    UART_DBG_LEVEL_INFO  = 2,   /**< Info and above (default) */
    UART_DBG_LEVEL_DBG   = 3,   /**< Debug and above */
    UART_DBG_LEVEL_TRC   = 4,   /**< Trace - most verbose */
} UART_DBG_Level_t;

/*---------------------------------------------------------------------------------------------------------*/
/* Configuration                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
/** Compile-time log level filter - set to 0-4, lower = less verbose */
/** Set to 0 (ERR only) in production, 3 (DBG) in development */
#ifndef UART_LOG_LEVEL
#define UART_LOG_LEVEL   UART_DBG_LEVEL_INFO
#endif

/** Enable/disable entire UART debug system */
#ifndef UART_DBG_ENABLE
#define UART_DBG_ENABLE  1
#endif

/** UART ring buffer size for interrupt-safe logging */
#ifndef UART_DBG_BUF_SIZE
#define UART_DBG_BUF_SIZE  256
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Core Logging Macros                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief  Core log macro - format and print via UART
 */
#if UART_DBG_ENABLE
#define UART_LOG(fmt, ...) \
    do { \
        if (UART_DBG_LEVEL_INFO <= UART_LOG_LEVEL) \
            UART_DBG_Log(fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define UART_LOG(fmt, ...)   ((void)0)
#endif

/**
 * @brief  Error log - always printed regardless of level
 */
#if UART_DBG_ENABLE
#define UART_ERR(fmt, ...) \
    do { \
        if (UART_DBG_LEVEL_ERR <= UART_LOG_LEVEL) \
            UART_DBG_Log("[ERR] " fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define UART_ERR(fmt, ...)   ((void)0)
#endif

/**
 * @brief  Warning log
 */
#if UART_DBG_ENABLE
#define UART_WARN(fmt, ...) \
    do { \
        if (UART_DBG_LEVEL_WARN <= UART_LOG_LEVEL) \
            UART_DBG_Log("[WARN] " fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define UART_WARN(fmt, ...)  ((void)0)
#endif

/**
 * @brief  Debug log
 */
#if UART_DBG_ENABLE
#define UART_DBG(fmt, ...) \
    do { \
        if (UART_DBG_LEVEL_DBG <= UART_LOG_LEVEL) \
            UART_DBG_Log("[DBG] " fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define UART_DBG(fmt, ...)   ((void)0)
#endif

/**
 * @brief  Trace log - most verbose
 */
#if UART_DBG_ENABLE
#define UART_TRC(fmt, ...) \
    do { \
        if (UART_DBG_LEVEL_TRC <= UART_LOG_LEVEL) \
            UART_DBG_Log("[TRC] " fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define UART_TRC(fmt, ...)   ((void)0)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Module-Specific Macros                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Module-specific logging macros */
/** @{ */

/** USB Module */
#define USB_LOG(fmt, ...)   UART_LOG("[USB] " fmt, ##__VA_ARGS__)
#define USB_ERR(fmt, ...)  UART_ERR("[USB] " fmt, ##__VA_ARGS__)
#define USB_WARN(fmt, ...) UART_WARN("[USB] " fmt, ##__VA_ARGS__)
#define USB_DBG(fmt, ...)  UART_DBG("[USB] " fmt, ##__VA_ARGS__)
#define USB_TRC(fmt, ...)  UART_TRC("[USB] " fmt, ##__VA_ARGS__)

/** MSC Module */
#define MSC_LOG(fmt, ...)   UART_LOG("[MSC] " fmt, ##__VA_ARGS__)
#define MSC_ERR(fmt, ...)  UART_ERR("[MSC] " fmt, ##__VA_ARGS__)
#define MSC_WARN(fmt, ...) UART_WARN("[MSC] " fmt, ##__VA_ARGS__)
#define MSC_DBG(fmt, ...)  UART_DBG("[MSC] " fmt, ##__VA_ARGS__)
#define MSC_TRC(fmt, ...)  UART_TRC("[MSC] " fmt, ##__VA_ARGS__)

/** I2C Module */
#define I2C_LOG(fmt, ...)   UART_LOG("[I2C] " fmt, ##__VA_ARGS__)
#define I2C_ERR(fmt, ...)  UART_ERR("[I2C] " fmt, ##__VA_ARGS__)
#define I2C_WARN(fmt, ...) UART_WARN("[I2C] " fmt, ##__VA_ARGS__)
#define I2C_DBG(fmt, ...)  UART_DBG("[I2C] " fmt, ##__VA_ARGS__)
#define I2C_TRC(fmt, ...)  UART_TRC("[I2C] " fmt, ##__VA_ARGS__)

/** HID Module */
#define HID_LOG(fmt, ...)   UART_LOG("[HID] " fmt, ##__VA_ARGS__)
#define HID_ERR(fmt, ...)  UART_ERR("[HID] " fmt, ##__VA_ARGS__)
#define HID_WARN(fmt, ...) UART_WARN("[HID] " fmt, ##__VA_ARGS__)
#define HID_DBG(fmt, ...)  UART_DBG("[HID] " fmt, ##__VA_ARGS__)
#define HID_TRC(fmt, ...)  UART_TRC("[HID] " fmt, ##__VA_ARGS__)

/** MAIN Module */
#define MAIN_LOG(fmt, ...)  UART_LOG("[MAIN] " fmt, ##__VA_ARGS__)
#define MAIN_ERR(fmt, ...)  UART_ERR("[MAIN] " fmt, ##__VA_ARGS__)
#define MAIN_WARN(fmt, ...) UART_WARN("[MAIN] " fmt, ##__VA_ARGS__)
#define MAIN_DBG(fmt, ...)  UART_DBG("[MAIN] " fmt, ##__VA_ARGS__)
#define MAIN_TRC(fmt, ...)  UART_TRC("[MAIN] " fmt, ##__VA_ARGS__)

/** DBG Module (MSC Debug Channel) */
#define DBG_LOG(fmt, ...)   UART_LOG("[DBG] " fmt, ##__VA_ARGS__)
#define DBG_ERR(fmt, ...)  UART_ERR("[DBG] " fmt, ##__VA_ARGS__)
#define DBG_WARN(fmt, ...) UART_WARN("[DBG] " fmt, ##__VA_ARGS__)
#define DBG_DBG(fmt, ...)  UART_DBG("[DBG] " fmt, ##__VA_ARGS__)
#define DBG_TRC(fmt, ...)  UART_TRC("[DBG] " fmt, ##__VA_ARGS__)

/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Hex Dump Macro                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief  Hex dump via UART
 * @param  level  Log level (UART_DBG_LEVEL_*)
 * @param  label  Label/name for this dump
 * @param  data   Pointer to data buffer
 * @param  len    Number of bytes to dump
 */
#define UART_HEX_DUMP(level, label, data, len) \
    do { \
        if ((level) <= UART_LOG_LEVEL) \
            UART_DBG_HexDump(label, (const uint8_t *)(data), (int)(len)); \
    } while (0)

/*---------------------------------------------------------------------------------------------------------*/
/* Function Declarations                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Initialize UART Debug Log System
 * @note   Call once after UART_Open() in main()
 *         Uses UART0 at 115200-8n1 (must be pre-initialized)
 * 
 * @param  uartBase  UART peripheral base address (e.g., UART0)
 * @param  sysclk    System clock frequency in Hz (e.g., 192000000 for 192MHz)
 */
void UART_DBG_Init(void *uartBase, uint32_t sysclk);

/**
 * @brief  Set runtime log level filter
 * @param  level  New log level (UART_DBG_LEVEL_*)
 * @note   Can be changed at runtime
 */
void UART_DBG_SetLevel(UART_DBG_Level_t level);

/**
 * @brief  Get current runtime log level
 * @return Current log level
 */
UART_DBG_Level_t UART_DBG_GetLevel(void);

/**
 * @brief  Formatted log output via UART
 * @param  fmt  Printf-style format string
 * @param  ...  Arguments
 * @note   Uses UART IRQ for thread safety if buffer enabled
 */
void UART_DBG_Log(const char *fmt, ...);

/**
 * @brief  Hex dump output via UART
 * @param  label  Label for the dump
 * @param  data   Data buffer
 * @param  len    Buffer length in bytes
 */
void UART_DBG_HexDump(const char *label, const uint8_t *data, int len);

/**
 * @brief  Enable/disable UART debug at runtime
 * @param  enable  1 = enable, 0 = disable
 */
void UART_DBG_Enable(uint8_t enable);

/**
 * @brief  Flush debug buffer (force output)
 * @note   Call this in main loop or idle to flush pending data
 */
void UART_DBG_Flush(void);

/**
 * @brief  Get timestamp string
 * @param  buf   Output buffer (at least 12 bytes)
 * @param  size  Buffer size
 * @note   Format: "HH:MM:SS.mmm"
 */
void UART_DBG_Timestamp(char *buf, int size);

/**
 * @brief  Direct UART putchar (low-level)
 * @param  c  Character to send
 */
void UART_DBG_PutChar(char c);

/**
 * @brief  Direct UART puts (low-level)
 * @param  s  String to send
 */
void UART_DBG_PutString(const char *s);

/*---------------------------------------------------------------------------------------------------------*/
/* Assert / Fatal Error                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief  Assert macro - print assertion failure and hang
 * @param  expr  Expression that should be true
 * @param  file  Source file name (__FILE__)
 * @param  line  Source line number (__LINE__)
 */
#define UART_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            UART_ERR("ASSERT FAILED: %s at %s:%d\n", #expr, __FILE__, __LINE__); \
            while (1); /* Hang for debugger */ \
        } \
    } while (0)

/**
 * @brief  Fatal error - print message and hang
 */
#define UART_FATAL(fmt, ...) \
    do { \
        UART_ERR("[FATAL] " fmt "\n", ##__VA_ARGS__); \
        while (1); \
    } while (0)

#endif /* __UART_DEBUG_H__ */
