/**
 * @file     hid_parser.c
 * @brief    HID-over-I2C Descriptor Parser Implementation
 * @version  1.0.0
 */

#include <stdio.h>
#include <string.h>
#include "NuMicro.h"
#include "i2c_driver.h"
#include "hid_parser.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Private Functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Swap bytes in 16-bit value (big-endian to little-endian)
 */
static uint16_t Swap16(uint16_t val)
{
    return (val >> 8) | (val << 8);
}

/**
 * @brief   Check if a register address is valid (non-zero)
 */
static int32_t CheckRegisterUnique(uint8_t r1, uint8_t r2, uint8_t r3, uint8_t r4, uint8_t r5)
{
    /* All must be non-zero and unique */
    if (r1 == 0 || r2 == 0 || r3 == 0 || r4 == 0 || r5 == 0)
        return HID_PARSER_ERR_ZERO_REG;
    
    if (r1 == r2 || r1 == r3 || r1 == r4 || r1 == r5 ||
        r2 == r3 || r2 == r4 || r2 == r5 ||
        r3 == r4 || r3 == r5 ||
        r4 == r5)
        return HID_PARSER_ERR_DUPLICATE_REG;
    
    return HID_PARSER_OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

int32_t HID_Parser_Init(HID_Context_t *ctx, uint8_t addr)
{
    if (ctx == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    memset(ctx, 0, sizeof(HID_Context_t));
    ctx->deviceAddr = addr;
    ctx->connected = 0;
    ctx->protocol = 1;  /* Default to Report protocol */
    ctx->idleRate = 0;
    
    return HID_PARSER_OK;
}

void HID_Parser_Reset(HID_Context_t *ctx)
{
    if (ctx == NULL)
        return;
    
    ctx->connected = 0;
    ctx->reportDescLen = 0;
    ctx->protocol = 1;
    ctx->idleRate = 0;
    memset(&ctx->hidDesc, 0, sizeof(I2C_HID_Descriptor_t));
}

int32_t HID_Parser_ValidateDescriptor(const uint8_t *buf, uint16_t len)
{
    uint16_t wHIDDescLen;
    uint16_t bcdVersion;
    uint16_t wReportDescReg, wInputReg, wOutputReg, wCmdReg, wDataReg;
    
    if (buf == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    if (len < HID_DESC_LENGTH)
        return HID_PARSER_ERR_DESC_LEN;
    
    /* Parse header */
    wHIDDescLen = Swap16(*(uint16_t*)&buf[0]);
    bcdVersion = Swap16(*(uint16_t*)&buf[2]);
    
    /* Validate length */
    if (wHIDDescLen != HID_DESC_LENGTH)
        return HID_PARSER_ERR_DESC_LEN;
    
    /* Validate version */
    if (bcdVersion != HID_DESC_VERSION)
        return HID_PARSER_ERR_VERSION;
    
    /* Parse register addresses */
    wReportDescReg = Swap16(*(uint16_t*)&buf[6]);
    wInputReg = Swap16(*(uint16_t*)&buf[8]);
    wOutputReg = Swap16(*(uint16_t*)&buf[12]);
    wCmdReg = Swap16(*(uint16_t*)&buf[16]);
    wDataReg = Swap16(*(uint16_t*)&buf[18]);
    
    /* Check registers are non-zero */
    if (wReportDescReg == 0 || wInputReg == 0 || wCmdReg == 0 || wDataReg == 0)
        return HID_PARSER_ERR_ZERO_REG;
    
    /* Note: wOutputReg can be 0 (optional) */
    
    /* Check for duplicate registers */
    if (wReportDescReg == wInputReg || wReportDescReg == wCmdReg ||
        wReportDescReg == wDataReg || wInputReg == wCmdReg ||
        wInputReg == wDataReg || wCmdReg == wDataReg)
        return HID_PARSER_ERR_DUPLICATE_REG;
    
    return HID_PARSER_OK;
}

int32_t HID_Parser_ParseDescriptor(HID_Context_t *ctx, const uint8_t *buf)
{
    int32_t ret;
    const uint8_t *p = buf;
    
    if (ctx == NULL || buf == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    /* Validate first */
    ret = HID_Parser_ValidateDescriptor(buf, HID_DESC_LENGTH);
    if (ret != HID_PARSER_OK)
        return ret;
    
    /* Parse into structure */
    ctx->hidDesc.wHIDDescLength = Swap16(*(uint16_t*)&p[0]);
    ctx->hidDesc.bcdVersion = Swap16(*(uint16_t*)&p[2]);
    ctx->hidDesc.wReportDescLength = Swap16(*(uint16_t*)&p[4]);
    ctx->hidDesc.wReportDescRegister = Swap16(*(uint16_t*)&p[6]);
    ctx->hidDesc.wInputRegister = Swap16(*(uint16_t*)&p[8]);
    ctx->hidDesc.wMaxInputLength = Swap16(*(uint16_t*)&p[10]);
    ctx->hidDesc.wOutputRegister = Swap16(*(uint16_t*)&p[12]);
    ctx->hidDesc.wMaxOutputLength = Swap16(*(uint16_t*)&p[14]);
    ctx->hidDesc.wCommandRegister = Swap16(*(uint16_t*)&p[16]);
    ctx->hidDesc.wDataRegister = Swap16(*(uint16_t*)&p[18]);
    ctx->hidDesc.wVendorID = Swap16(*(uint16_t*)&p[20]);
    ctx->hidDesc.wProductID = Swap16(*(uint16_t*)&p[22]);
    ctx->hidDesc.wVersionID = Swap16(*(uint16_t*)&p[24]);
    
    memcpy(ctx->hidDesc.reserved, &p[26], 4);
    
    /* Extract key fields to context (store full 16-bit register addresses) */
    ctx->regReportDesc = ctx->hidDesc.wReportDescRegister;
    ctx->regInput = ctx->hidDesc.wInputRegister;
    ctx->regOutput = ctx->hidDesc.wOutputRegister;
    ctx->regCommand = ctx->hidDesc.wCommandRegister;
    ctx->regData = ctx->hidDesc.wDataRegister;
    
    ctx->vendorID = ctx->hidDesc.wVendorID;
    ctx->productID = ctx->hidDesc.wProductID;
    ctx->versionID = ctx->hidDesc.wVersionID;
    
    ctx->connected = 1;
    
    return HID_PARSER_OK;
}

int32_t HID_Parser_ReadDescriptor(HID_Context_t *ctx)
{
    int32_t ret;
    uint8_t buf[HID_DESC_LENGTH];
    
    if (ctx == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    /* Read HID Descriptor from I2C device.
     * The HID-over-I2C spec doesn't define where the descriptor lives,
     * but convention is register 0x01 or just after power-on.
     * For most devices, we read from register 0x00 or the device
     * automatically provides the descriptor at first read.
     * 
     * We use a convention: read 30 bytes starting at register 0.
     * Some devices use register 0x01 as HID_DESC register.
     */
    
    /* Try reading from register 0x00 (common convention) */
    ret = I2C0_Read(ctx->deviceAddr, buf, HID_DESC_LENGTH);
    if (ret != I2C_OK) {
        /* Try register 0x01 */
        ret = I2C0_ReadReg(ctx->deviceAddr, 0x01, buf, HID_DESC_LENGTH);
        if (ret != I2C_OK)
            return HID_PARSER_ERR_TIMEOUT;
    }
    
    /* Parse and validate */
    ret = HID_Parser_ParseDescriptor(ctx, buf);
    if (ret != HID_PARSER_OK)
        return ret;
    
    return HID_PARSER_OK;
}

int32_t HID_Parser_ReadReportDescriptor(HID_Context_t *ctx)
{
    int32_t ret;
    uint16_t descLen;
    
    if (ctx == NULL)
        return HID_PARSER_ERR_NULL_PTR;
    
    if (!ctx->connected)
        return HID_PARSER_ERR_NULL_PTR;
    
    descLen = ctx->hidDesc.wReportDescLength;
    if (descLen == 0 || descLen > sizeof(ctx->reportDesc))
        return HID_PARSER_ERR_BUFFER_SMALL;
    
    /* Read Report Descriptor from the register specified in HID Descriptor */
    ret = I2C0_ReadReg(ctx->deviceAddr, 
                         ctx->hidDesc.wReportDescRegister,
                         ctx->reportDesc, 
                         descLen);
    
    if (ret != I2C_OK)
        return HID_PARSER_ERR_TIMEOUT;
    
    ctx->reportDescLen = descLen;
    
    return HID_PARSER_OK;
}

const char* HID_Parser_ErrorString(int32_t err)
{
    switch (err) {
        case HID_PARSER_OK:            return "OK";
        case HID_PARSER_ERR_DESC_LEN:  return "Invalid HID descriptor length";
        case HID_PARSER_ERR_VERSION:   return "Unsupported HID version (not 1.00)";
        case HID_PARSER_ERR_ZERO_REG:  return "Register address is zero";
        case HID_PARSER_ERR_DUPLICATE_REG: return "Duplicate register address";
        case HID_PARSER_ERR_NULL_PTR:  return "NULL pointer provided";
        case HID_PARSER_ERR_BUFFER_SMALL: return "Buffer too small";
        case HID_PARSER_ERR_TIMEOUT:   return "I2C timeout";
        default:                        return "Unknown error";
    }
}

void HID_Parser_DumpDescriptor(const HID_Context_t *ctx)
{
    const I2C_HID_Descriptor_t *d;
    
    if (ctx == NULL)
        return;
    
    d = &ctx->hidDesc;
    
    printf("=== HID Descriptor ===\n");
    printf("  Length:     %d bytes\n", d->wHIDDescLength);
    printf("  Version:     0x%04X\n", d->bcdVersion);
    printf("  ReportDesc:  %d bytes @ reg 0x%04X\n", 
           d->wReportDescLength, d->wReportDescRegister);
    printf("  Input:       max %d bytes @ reg 0x%04X\n", 
           d->wMaxInputLength, d->wInputRegister);
    printf("  Output:      max %d bytes @ reg 0x%04X\n", 
           d->wMaxOutputLength, d->wOutputRegister);
    printf("  Command:     @ reg 0x%04X\n", d->wCommandRegister);
    printf("  Data:        @ reg 0x%04X\n", d->wDataRegister);
    printf("  VID:         0x%04X\n", d->wVendorID);
    printf("  PID:         0x%04X\n", d->wProductID);
    printf("  Version:     0x%04X\n", d->wVersionID);
    printf("  Reserved:    %02X %02X %02X %02X\n",
           d->reserved[0], d->reserved[1], d->reserved[2], d->reserved[3]);
    printf("  I2C Addr:    0x%02X\n", ctx->deviceAddr);
    printf("  Report Desc: %d bytes (cached)\n", ctx->reportDescLen);
}
