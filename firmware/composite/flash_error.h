/**
 * @file     flash_error.h
 * @brief    Flash Error Log System - Persistent error storage in reserved flash region
 * @version  1.0.0
 *
 * Stores error log entries in the last 4KB page of APROM flash.
 * Readable via MSC Debug Channel (DBG_READ_LOG extension).
 *
 * Flash Layout (4KB page at APROM end):
 *   [0x0007F000] Header (8 bytes)
 *                 - u32Magic: 0xDEADBEEF (valid marker)
 *                 - u16Head:  Log head index
 *                 - u16Tail:  Log tail index
 *   [0x0007F008] Entry[0] (12 bytes)
 *   [0x0007F014] Entry[1] (12 bytes)
 *   ... (341 entries total, max 4096/12)
 *
 * Hardware: M487 with 512KB APROM (last page at 0x0007F000)
 */

#ifndef __FLASH_ERROR_H__
#define __FLASH_ERROR_H__

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------*/
/* Configuration                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

/** @name Flash Error Log Configuration */
/** @{ */

/**
 * @brief  Flash reserved area base address (last 4KB page of 512KB APROM)
 * @note   For 256KB flash, use 0x0003F000. Adjust based on actual chip.
 */
#ifndef FLASH_ERROR_BASE
#define FLASH_ERROR_BASE         0x0007F000UL
#endif

/**
 * @brief  Flash page size (M480/M487 = 4KB)
 */
#ifndef FLASH_ERROR_PAGE_SIZE
#define FLASH_ERROR_PAGE_SIZE    0x1000UL   /* 4096 bytes */
#endif

/**
 * @brief  Flash page erase address (page-aligned base)
 */
#define FLASH_ERROR_PAGE_ADDR    (FLASH_ERROR_BASE & ~0xFFFUL)

/**
 * @brief  Maximum number of error log entries
 * @note   (4096 - 8) / 12 = 340 entries
 */
#define FLASH_ERROR_MAX_ENTRIES  ((FLASH_ERROR_PAGE_SIZE - sizeof(FlashErrorHeader_t)) / sizeof(FlashErrorEntry_t))

/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Error Log Entry Structure                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#pragma pack(push, 1)

/**
 * @brief  Error log entry structure (12 bytes)
 */
typedef struct {
    uint32_t    u32Timestamp;      /**< DWT cycle counter at time of error */
    uint16_t    u16ErrorCode;     /**< Error code (defined in i2c_error.h) */
    uint8_t     u8Module;          /**< Module ID (MOD_MAIN=0, MOD_USB=1, MOD_MSC=2, MOD_I2C=3, MOD_HID=4) */
    uint8_t     u8Flags;           /**< Flags (0x01=Critical, 0x02=Recovered) */
    uint32_t    u32Data;          /**< Additional data (address, count, etc.) */
} FlashErrorEntry_t;

/**
 * @brief  Flash error log header (8 bytes, at base of page)
 */
typedef struct {
    uint32_t    u32Magic;         /**< Magic number: 0xDEADBEEF (valid log marker) */
    uint16_t    u16Head;          /**< Head index (next write position) */
    uint16_t    u16Tail;          /**< Tail index (oldest entry) */
    uint16_t    u16Count;         /**< Total entries ever written */
    uint16_t    u16Reserved;      /**< Reserved */
} FlashErrorHeader_t;

#pragma pack(pop)

/*---------------------------------------------------------------------------------------------------------*/
/* Error Module IDs                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Error Module IDs */
/** @{ */
#define ERR_MOD_MAIN    0
#define ERR_MOD_USB     1
#define ERR_MOD_MSC     2
#define ERR_MOD_I2C     3
#define ERR_MOD_HID     4
#define ERR_MOD_UART    5
#define ERR_MOD_FMC     6
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Error Flags                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Error Flags */
/** @{ */
#define ERR_FLAGS_NONE      0x00
#define ERR_FLAGS_CRITICAL 0x01   /**< Critical error requiring immediate attention */
#define ERR_FLAGS_RECOVERED 0x02  /**< Error was recovered/retried successfully */
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Error Codes (shared with i2c_error.h)                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Error Codes (Flash-specific codes start at 0x1000) */
/** @{ */
#define ERR_OK                     0x0000
#define ERR_I2C_NACK               0x1001
#define ERR_I2C_TIMEOUT             0x1002
#define ERR_I2C_BUSY                0x1003
#define ERR_I2C_NACK_RETRY           0x1004
#define ERR_USB_ENUM_FAIL           0x2001
#define ERR_USB_EP_STALL            0x2002
#define ERR_USB_CRC_ERROR           0x2003
#define ERR_FMC_ERASE_FAIL          0x3001
#define ERR_FMC_WRITE_FAIL          0x3002
#define ERR_FMC_VERIFY_FAIL         0x3003
#define ERR_FLASH_LOG_FULL          0x3004
#define ERR_UNKNOWN                 0xFFFF
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Magic Number                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define FLASH_ERROR_MAGIC   0xDEADBEEFUL

/*---------------------------------------------------------------------------------------------------------*/
/* Function Declarations                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Initialize flash error log system
 * @note   Reads header from flash, validates magic number
 * @return 0 on success, -1 if log area corrupted/unformatted
 */
int32_t FlashError_Init(void);

/**
 * @brief  Add an error entry to the log
 * @param  u16ErrorCode  Error code (ERR_*)
 * @param  u8Module      Module ID (ERR_MOD_*)
 * @param  u8Flags       Flags (ERR_FLAGS_*)
 * @param  u32Data       Additional data
 * @return 0 on success, -1 on flash write failure
 */
int32_t FlashError_Add(uint16_t u16ErrorCode, uint8_t u8Module,
                       uint8_t u8Flags, uint32_t u32Data);

/**
 * @brief  Read error log entries
 * @param  pEntries  Output array of entries
 * @param  u16Max   Maximum number of entries to read
 * @return Number of entries actually read
 * @note   Reads entries from tail (oldest) to head (newest)
 */
uint16_t FlashError_Read(FlashErrorEntry_t *pEntries, uint16_t u16Max);

/**
 * @brief  Get current error log entry count
 * @return Number of entries currently in log
 */
uint16_t FlashError_Count(void);

/**
 * @brief  Clear all error log entries
 * @return 0 on success, -1 on flash erase failure
 * @note   Erases the entire flash page
 */
int32_t FlashError_Clear(void);

/**
 * @brief  Get header info
 * @param  pHead  Pointer to output variable for head index (can be NULL)
 * @param  pTail  Pointer to output variable for tail index (can be NULL)
 * @param  pCount Pointer to output variable for total count (can be NULL)
 */
void FlashError_GetInfo(uint16_t *pHead, uint16_t *pTail, uint16_t *pCount);

/*---------------------------------------------------------------------------------------------------------*/
/* Convenience Macros                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Log an I2C error
 */
#define FlashError_I2C(err, flags, data) \
    FlashError_Add((err), ERR_MOD_I2C, (flags), (data))

/**
 * @brief  Log a USB error
 */
#define FlashError_USB(err, flags, data) \
    FlashError_Add((err), ERR_MOD_USB, (flags), (data))

/**
 * @brief  Log an FMC error
 */
#define FlashError_FMC(err, flags, data) \
    FlashError_Add((err), ERR_MOD_FMC, (flags), (data))

/**
 * @brief  Log a critical error
 */
#define FlashError_Critical(err, module, data) \
    FlashError_Add((err), (module), ERR_FLAGS_CRITICAL, (data))

/*---------------------------------------------------------------------------------------------------------*/
/* Backward Compatibility Aliases (for ErrorLog_* API)                                                    */
/*---------------------------------------------------------------------------------------------------------*/
/** @name ErrorLog Compatibility Aliases (original API name) */
/** @{ */

#define ErrorLogEntry_t         FlashErrorEntry_t
#define ErrorLog_Init()         FlashError_Init()
#define ErrorLog_Add(e,m,f,d)   FlashError_Add((e),(m),(f),(d))
#define ErrorLog_Read(b,n)      FlashError_Read((b),(n))
#define ErrorLog_Count()        FlashError_Count()
#define ErrorLog_Clear()        FlashError_Clear()
#define ErrorLog_GetInfo(h,t,c) FlashError_GetInfo((h),(t),(c))

/** @brief  Backward compatible: get next write slot (head index) */
static inline uint16_t ErrorLog_GetSlot(void) {
    uint16_t h, t, c;
    FlashError_GetInfo(&h, &t, &c);
    return h;
}

/** @brief  Backward compatible: get entry count */
static inline uint16_t ErrorLog_GetCount(void) {
    return FlashError_Count();
}

#define ERR_LOG_MAX_ENTRIES     FLASH_ERROR_MAX_ENTRIES
#define ERROR_LOG_MAX_ENTRIES   FLASH_ERROR_MAX_ENTRIES
#define ERROR_LOG_ENTRY_SIZE    sizeof(FlashErrorEntry_t)

/** @} */

#endif /* __FLASH_ERROR_H__ */
