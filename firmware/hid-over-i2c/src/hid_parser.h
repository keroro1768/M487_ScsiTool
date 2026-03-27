/**
 * @file     hid_parser.h
 * @brief    HID-over-I2C Descriptor Parser
 * @version  1.0.0
 * 
 * Parses HID Descriptor (30 bytes) and Report Descriptor
 * from a HID-over-I2C device.
 * 
 * Reference: Microsoft HID-over-I2C Protocol Specification v1.0
 */

#ifndef __HID_PARSER_H__
#define __HID_PARSER_H__

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------*/
/* HID Descriptor (30 bytes, per HID-over-I2C spec)                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_DESC_LENGTH         30
#define HID_DESC_VERSION       0x0100  /* bcdVersion must be 1.00 */

/**
 * @brief   HID Descriptor structure (30 bytes)
 * 
 * All multi-byte fields are little-endian.
 */
typedef struct {
    uint16_t wHIDDescLength;         /* Offset 0: Total length = 30 */
    uint16_t bcdVersion;              /* Offset 2: Protocol version = 0x0100 */
    uint16_t wReportDescLength;       /* Offset 4: Report Descriptor length */
    uint16_t wReportDescRegister;     /* Offset 6: Register index for Report Desc */
    uint16_t wInputRegister;         /* Offset 8: Register index for Input Report */
    uint16_t wMaxInputLength;         /* Offset 10: Max Input Report size */
    uint16_t wOutputRegister;         /* Offset 12: Register index for Output Report */
    uint16_t wMaxOutputLength;         /* Offset 14: Max Output Report size */
    uint16_t wCommandRegister;         /* Offset 16: Register index for Command */
    uint16_t wDataRegister;           /* Offset 18: Register index for Data */
    uint16_t wVendorID;               /* Offset 20: Vendor ID */
    uint16_t wProductID;              /* Offset 22: Product ID */
    uint16_t wVersionID;             /* Offset 24: Firmware version */
    uint8_t  reserved[4];             /* Offset 26: Must be 0 */
} I2C_HID_Descriptor_t;

/*---------------------------------------------------------------------------------------------------------*/
/* HID-over-I2C Command Opcodes                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_I2C_OP_RESET        0x01
#define HID_I2C_OP_GET_REPORT   0x02
#define HID_I2C_OP_SET_REPORT   0x03
#define HID_I2C_OP_GET_IDLE    0x04
#define HID_I2C_OP_SET_IDLE    0x05
#define HID_I2C_OP_GET_PROTOCOL 0x06
#define HID_I2C_OP_SET_PROTOCOL 0x07
#define HID_I2C_OP_SET_POWER    0x08

/*---------------------------------------------------------------------------------------------------------*/
/* Report Type (bits in command register byte 1)                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_RPTYPE_INPUT       0x01
#define HID_RPTYPE_OUTPUT      0x02
#define HID_RPTYPE_FEATURE     0x03

/*---------------------------------------------------------------------------------------------------------*/
/* Power Commands                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_POWER_ON           0x00
#define HID_POWER_SLEEP       0x01

/*---------------------------------------------------------------------------------------------------------*/
/* Return Codes                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_PARSER_OK          0
#define HID_PARSER_ERR_DESC_LEN     -1   /* Invalid HID descriptor length */
#define HID_PARSER_ERR_VERSION      -2   /* Unsupported HID version */
#define HID_PARSER_ERR_ZERO_REG     -3   /* Register address is zero */
#define HID_PARSER_ERR_DUPLICATE_REG -4  /* Duplicate register address */
#define HID_PARSER_ERR_NULL_PTR     -5   /* NULL pointer provided */
#define HID_PARSER_ERR_BUFFER_SMALL -6   /* Buffer too small */
#define HID_PARSER_ERR_TIMEOUT      -7   /* I2C timeout */

/*---------------------------------------------------------------------------------------------------------*/
/* Parser Context                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   HID-over-I2C device context
 * 
 * Contains cached descriptor information and device state.
 */
typedef struct {
    /* I2C device address (7-bit) */
    uint8_t  deviceAddr;
    
    /* Connection state */
    uint8_t  connected;
    
    /* HID Descriptor */
    I2C_HID_Descriptor_t hidDesc;
    
    /* Report Descriptor */
    uint8_t  reportDesc[512];   /* Cached Report Descriptor */
    uint16_t reportDescLen;
    
    /* Protocol state */
    uint8_t  protocol;          /* 0=Boot, 1=Report */
    uint8_t  idleRate;          /* Idle rate in 4ms units */
    
    /* Device info */
    uint16_t vendorID;
    uint16_t productID;
    uint16_t versionID;
    
    /* Register addresses (extracted from HID Descriptor)
     * HID-over-I2C spec defines these as 16-bit values (wReportDescRegister,
     * wInputRegister, etc.). Use uint16_t to avoid truncation.
     * Note: Most HID-over-I2C devices only use 8-bit registers (0x00-0xFF),
     * but devices requiring >0xFF would fail with uint8_t. */
    uint16_t regReportDesc;
    uint16_t regInput;
    uint16_t regOutput;
    uint16_t regCommand;
    uint16_t regData;
} HID_Context_t;

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Initialize HID parser context
 * @param   ctx     Pointer to HID context
 * @param   addr    I2C device 7-bit address
 * @return  HID_PARSER_OK on success
 */
int32_t HID_Parser_Init(HID_Context_t *ctx, uint8_t addr);

/**
 * @brief   Read and parse HID Descriptor from I2C device
 * @param   ctx     Pointer to HID context (must be initialized)
 * @return  HID_PARSER_OK on success, error code on failure
 * 
 * Reads 30 bytes from the device and validates:
 *   - bcdVersion == 0x0100
 *   - wHIDDescLength == 30
 *   - All register addresses are non-zero and unique
 */
int32_t HID_Parser_ReadDescriptor(HID_Context_t *ctx);

/**
 * @brief   Read and cache Report Descriptor from I2C device
 * @param   ctx     Pointer to HID context
 * @return  HID_PARSER_OK on success, error code on failure
 * 
 * Uses wReportDescRegister and wReportDescLength from HID Descriptor.
 */
int32_t HID_Parser_ReadReportDescriptor(HID_Context_t *ctx);

/**
 * @brief   Validate a HID Descriptor buffer (without I2C access)
 * @param   buf     Pointer to 30-byte HID Descriptor
 * @param   len     Buffer length (must be 30)
 * @return  HID_PARSER_OK if valid, error code if invalid
 * 
 * Use this for static validation of a known descriptor.
 */
int32_t HID_Parser_ValidateDescriptor(const uint8_t *buf, uint16_t len);

/**
 * @brief   Parse HID Descriptor into context structure
 * @param   ctx     Pointer to HID context
 * @param   buf     Pointer to 30-byte HID Descriptor
 * @return  HID_PARSER_OK on success, error code on failure
 */
int32_t HID_Parser_ParseDescriptor(HID_Context_t *ctx, const uint8_t *buf);

/**
 * @brief   Reset HID context
 * @param   ctx     Pointer to HID context
 * @return  None
 */
void HID_Parser_Reset(HID_Context_t *ctx);

/**
 * @brief   Get human-readable error string
 * @param   err     Error code
 * @return  Static error string
 */
const char* HID_Parser_ErrorString(int32_t err);

/**
 * @brief   Dump HID Descriptor to stdout (for debugging)
 * @param   ctx     Pointer to HID context
 * @return  None
 */
void HID_Parser_DumpDescriptor(const HID_Context_t *ctx);

#endif /* __HID_PARSER_H__ */
