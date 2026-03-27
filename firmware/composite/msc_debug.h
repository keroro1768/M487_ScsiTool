/**
 * @file     msc_debug.h
 * @brief    MSC Vendor Debug Command Handler
 * @version  1.0.0
 * 
 * MSC Vendor-Specific Debug Channel via SCSI CDB 0xC0-0xFF
 * 
 * Architecture:
 *   Host (Windows/macOS/Linux)  ───USB MSC───►  M487 MSC Handler
 *                                                 │
 *                                                 ├── Standard SCSI (0x00-0xBF)
 *                                                 └── Vendor CDB (0xC0-0xFF) ──► MSC_VendorCommand()
 *                                                                            │
 *                                                                            ├── DBG_READ_MEM
 *                                                                            ├── DBG_WRITE_MEM
 *                                                                            ├── DBG_GET_INFO
 *                                                                            └── DBG_READ_LOG
 * 
 * CDB Structure (6 bytes):
 *   Byte 0: Opcode    = 0xC0 (Vendor-specific)
 *   Byte 1: Sub-cmd   = Debug sub-command (see below)
 *   Byte 2: Addr[15:8] = Address high byte (for memory commands)
 *   Byte 3: Addr[7:0]  = Address low byte
 *   Byte 4: Length     = Transfer length in bytes
 *   Byte 5: Reserved   = 0x00
 * 
 * Data Stage:
 *   READ:  Device ──► Host (data payload)
 *   WRITE: Host ──► Device (command-specific data)
 */

#ifndef __MSC_DEBUG_H__
#define __MSC_DEBUG_H__

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------*/
/* Debug Vendor CDB Opcodes                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#define MSC_VENDOR_OPCODE   0xC0   /**< Vendor-specific CDB opcode range start */

/*---------------------------------------------------------------------------------------------------------*/
/* Debug Sub-Commands                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Debug Sub-Commands (CDB Byte 1) */
/** @{ */
#define DBG_READ_MEM        0x01   /**< Read memory (32-bit aligned) */
#define DBG_WRITE_MEM       0x02   /**< Write memory (32-bit aligned) */
#define DBG_READ_REG        0x03   /**< Read CPU register (R0-R15, xPSR) */
#define DBG_WRITE_REG       0x04   /**< Write CPU register */
#define DBG_GET_INFO       0x07   /**< Get device info (FW version, chip ID, etc.) */
#define DBG_READ_LOG       0x08   /**< Read debug log ring buffer */
#define DBG_ECHO           0x0F   /**< Echo test (loopback) */
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Debug Log Ring Buffer                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef MSC_DEBUG_LOG_SIZE
#define MSC_DEBUG_LOG_SIZE  512    /**< Debug log ring buffer size in bytes */
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Device Info Structure                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#pragma pack(push, 1)
typedef struct {
    uint32_t    u32ChipId;         /**< Chip ID (0xM487xxxx) */
    uint16_t    u16FwVersion;      /**< Firmware version (BCD, e.g. 0x0200 = v2.00) */
    uint16_t    u16BuildDate;      /**< Build date (YYYY-MM-DD encoded) */
    uint32_t    u32FlashSize;      /**< Flash size in bytes */
    uint32_t    u32RamSize;        /**< SRAM size in bytes */
    uint8_t     u8I2cSpeed;        /**< I2C bus speed in kHz (100 or 400) */
    uint8_t     u8I2cAddr;         /**< Default I2C target address (0 if none) */
    uint8_t     u8Reserved[2];     /**< Reserved */
    uint32_t    u32LogHead;        /**< Debug log ring buffer head pointer */
    uint32_t    u32LogTail;        /**< Debug log ring buffer tail pointer */
    uint32_t    u32LogCount;       /**< Number of log entries */
} MSC_DeviceInfo_t;
#pragma pack(pop)

/*---------------------------------------------------------------------------------------------------------*/
/* Debug Log Entry Structure                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#pragma pack(push, 1)
typedef struct {
    uint32_t    u32Timestamp;      /**< DWT cycle counter value */
    uint8_t     u8Level;           /**< Log level (0=ERR, 1=WARN, 2=INFO, 3=DBG, 4=TRC) */
    uint8_t     u8Module;           /**< Module ID (0=MAIN, 1=USB, 2=MSC, 3=I2C, 4=HID) */
    uint8_t     u8Code;            /**< Log code (module-specific) */
    uint8_t     u8Len;             /**< Extra data length */
    uint8_t     au8Data[4];        /**< Extra data (up to 4 bytes) */
} MSC_LogEntry_t;
#pragma pack(pop)

/*---------------------------------------------------------------------------------------------------------*/
/* CPU Register IDs (for DBG_READ_REG/DBG_WRITE_REG)                                                       */
/*---------------------------------------------------------------------------------------------------------*/
/** @name CPU Register IDs */
/** @{ */
#define REG_R0      0
#define REG_R1      1
#define REG_R2      2
#define REG_R3      3
#define REG_R4      4
#define REG_R5      5
#define REG_R6      6
#define REG_R7      7
#define REG_R8      8
#define REG_R9      9
#define REG_R10     10
#define REG_R11     11
#define REG_R12     12
#define REG_SP      13   /**< Stack Pointer */
#define REG_LR      14   /**< Link Register */
#define REG_PC      15   /**< Program Counter */
#define REG_XPSR    16   /**< Application Program Status Register */
#define REG_MSP     17   /**< Main Stack Pointer */
#define REG_PSP     18   /**< Process Stack Pointer */
#define REG_PRIMASK 20   /**< PRIMASK register */
#define REG_CONTROL  21   /**< CONTROL register */
#define REG_BASEPRI  22   /**< BASEPRI register */
#define REG_MAX     22
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Debug Log Module IDs                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Debug Module IDs */
/** @{ */
#define MOD_MAIN    0
#define MOD_USB     1
#define MOD_MSC     2
#define MOD_I2C     3
#define MOD_HID     4
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Debug Log Levels                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Debug Log Levels */
/** @{ */
#define LVL_ERR     0
#define LVL_WARN    1
#define LVL_INFO    2
#define LVL_DBG     3
#define LVL_TRC     4
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Function Declarations                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Initialize MSC Vendor Debug Command Handler
 * @note   Call this during MSC initialization
 */
void MSC_Debug_Init(void);

/**
 * @brief  MSC Vendor Debug Command Handler
 * @param  pCBW  Pointer to parsed CBW structure
 * @note   Called from MSC_ProcessCmd() when CDB opcode >= 0xC0
 */
void MSC_VendorCommand(CBW_t *pCBW);

/**
 * @brief  Get device information
 * @param  pInfo  Pointer to MSC_DeviceInfo_t structure to fill
 */
void MSC_Debug_GetInfo(MSC_DeviceInfo_t *pInfo);

/**
 * @brief  Read debug log ring buffer
 * @param  pBuf   Pointer to output buffer
 * @param  u32Len Maximum bytes to read
 * @return Actual bytes read
 */
uint32_t MSC_Debug_ReadLog(uint8_t *pBuf, uint32_t u32Len);

/**
 * @brief  Add a log entry to the debug ring buffer
 * @param  u8Level   Log level (LVL_*)
 * @param  u8Module  Module ID (MOD_*)
 * @param  u8Code    Log code
 * @param  pData     Pointer to extra data (can be NULL)
 * @param  u8Len     Extra data length (max 4)
 * @note   Thread-safe on ARM Cortex-M (uses IRQ mask)
 */
void MSC_Debug_Log(uint8_t u8Level, uint8_t u8Module, uint8_t u8Code,
                   const uint8_t *pData, uint8_t u8Len);

/**
 * @brief  Read memory via debug channel
 * @param  u32Addr  Memory address (32-bit aligned recommended)
 * @param  pBuf     Output buffer
 * @param  u32Len   Bytes to read (max 508 due to CBW structure)
 * @return Actual bytes read
 */
uint32_t MSC_Debug_ReadMem(uint32_t u32Addr, uint8_t *pBuf, uint32_t u32Len);

/**
 * @brief  Write memory via debug channel
 * @param  u32Addr  Memory address (32-bit aligned recommended)
 * @param  pData    Input data
 * @param  u32Len   Bytes to write (max 508)
 * @return Actual bytes written
 */
uint32_t MSC_Debug_WriteMem(uint32_t u32Addr, const uint8_t *pData, uint32_t u32Len);

/**
 * @brief  Read CPU register
 * @param  u8RegId  Register ID (REG_*)
 * @return Register value (0 if invalid register)
 */
uint32_t MSC_Debug_ReadReg(uint8_t u8RegId);

/**
 * @brief  Write CPU register
 * @param  u8RegId  Register ID (REG_*)
 * @param  u32Val   Value to write
 * @return 0 on success, -1 on failure (invalid register or write-protected)
 */
int32_t MSC_Debug_WriteReg(uint8_t u8RegId, uint32_t u32Val);

/**
 * @brief  Echo test - loopback data
 * @param  pData   Data to echo back
 * @param  u32Len  Data length
 * @return Same pointer as pData (for chaining)
 */
uint8_t* MSC_Debug_Echo(const uint8_t *pData, uint32_t u32Len);

/*---------------------------------------------------------------------------------------------------------*/
/* Helper Macros for Debug Logging                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Debug Logging Macros */
/** @{ */

/**
 * @brief  Log an error message
 * @param  code   Log code (module-specific error code)
 * @param  data   Optional 4-byte data (or NULL)
 */
#define DBG_LOG_ERR(module, code, data) \
    MSC_Debug_Log(LVL_ERR, MOD_##module, code, (const uint8_t *)(data), (data) ? 4 : 0)

/**
 * @brief  Log a warning message
 */
#define DBG_LOG_WARN(module, code, data) \
    MSC_Debug_Log(LVL_WARN, MOD_##module, code, (const uint8_t *)(data), (data) ? 4 : 0)

/**
 * @brief  Log an info message
 */
#define DBG_LOG_INFO(module, code, data) \
    MSC_Debug_Log(LVL_INFO, MOD_##module, code, (const uint8_t *)(data), (data) ? 4 : 0)

/**
 * @brief  Log a debug message
 */
#define DBG_LOG_DBG(module, code, data) \
    MSC_Debug_Log(LVL_DBG, MOD_##module, code, (const uint8_t *)(data), (data) ? 4 : 0)

/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* External variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
extern uint32_t g_u32FlashSize;
extern uint32_t g_u32RamSize;

#endif /* __MSC_DEBUG_H__ */
