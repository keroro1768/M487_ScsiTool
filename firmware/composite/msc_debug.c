/**
 * @file     msc_debug.c
 * @brief    MSC Vendor Debug Command Handler Implementation
 * @version  1.0.0
 * 
 * MSC Vendor-Specific Debug Channel via SCSI CDB 0xC0-0xFF
 * 
 * Hardware Requirements:
 *   - USB Mass Storage Interface (standard USB MSC BOT protocol)
 *   - No additional drivers needed on Windows/macOS/Linux
 * 
 * Usage from Host:
 *   1. Open USB MSC disk drive (e.g., E:)
 *   2. Send SCSI Vendor Command via IOCTL_SCSI_PASS_THROUGH
 *   3. Receive response via Data-In phase
 * 
 * Example (Windows):
 *   HANDLE h = CreateFile("\\\\.\\E:", GENERIC_READ | GENERIC_WRITE, ...);
 *   uint8_t cdb[6] = {0xC0, 0x07, 0, 0, sizeof(MSC_DeviceInfo_t), 0}; // GET_INFO
 *   DeviceIoControl(h, IOCTL_SCSI_PASS_THROUGH, &cdb, 6, &info, sizeof(info), &bytes, NULL);
 */

#include "NuMicro.h"
#include "hid_i2c.h"
#include "msc_debug.h"
#include "itm.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Debug Log Ring Buffer                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
static uint8_t  s_au8DebugLog[MSC_DEBUG_LOG_SIZE] __attribute__((aligned(4)));
static volatile uint32_t s_u32LogHead = 0;
static volatile uint32_t s_u32LogTail = 0;
static volatile uint32_t s_u32LogCount = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* Firmware Version / Build Info (can be set by build system)                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef FW_VERSION
#define FW_VERSION   0x0100   /* v1.00 */
#endif

#ifndef BUILD_DATE
#define BUILD_DATE   0x20260327  /* 2026-03-27 in BCD */
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Device Info (can be read from chip)                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
/* Chip ID for M487 */
#ifndef CHIP_ID
#define CHIP_ID   0xM4800000  /* M480 series */
#endif

#ifndef I2C_BUS_SPEED
#define I2C_BUS_SPEED   100   /* kHz */
#endif

#ifndef I2C_DEFAULT_ADDR
#define I2C_DEFAULT_ADDR   0x00
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Local Helper Functions                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Read a 32-bit value from memory with alignment handling
 */
static uint32_t _ReadMem32(uint32_t addr)
{
    /* Check alignment */
    if (addr & 0x03) {
        /* Unaligned - read byte by byte */
        uint8_t *p = (uint8_t *)addr;
        return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    }
    /* Aligned - direct word read */
    return *(volatile uint32_t *)addr;
}

/**
 * @brief  Write a 32-bit value to memory with alignment handling
 */
static int _WriteMem32(uint32_t addr, uint32_t val)
{
    /* Only allow word-aligned writes to prevent hard faults */
    if (addr < 0x20000000 || addr > 0x40000000)
        return -1;  /* Invalid memory region */
    
    if (addr & 0x03) {
        /* Unaligned - read-modify-write byte by byte */
        uint8_t *p = (uint8_t *)addr;
        p[0] = (uint8_t)(val & 0xFF);
        p[1] = (uint8_t)((val >> 8) & 0xFF);
        p[2] = (uint8_t)((val >> 16) & 0xFF);
        p[3] = (uint8_t)((val >> 24) & 0xFF);
        return 0;
    }
    /* Aligned - direct word write */
    *(volatile uint32_t *)addr = val;
    return 0;
}

/**
 * @brief  Read CPU register
 */
static uint32_t _ReadCPUReg(uint8_t regId)
{
    uint32_t val = 0;
    
    switch (regId) {
    case REG_R0 ... REG_R12:
        /* Access via assembly - save/restore manually */
        __asm volatile (
            "mov r0, %1\n"
            "push {r0-r3}\n"
            "pop {r0}\n"
            "mov %0, r0\n"
            : "=r"(val) : "r"(regId)
        );
        break;
    case REG_SP:
        __asm volatile ("mov %0, sp" : "=r"(val));
        break;
    case REG_LR:
        __asm volatile ("mov %0, lr" : "=r"(val));
        break;
    case REG_PC:
        /* PC shows the address of the next instruction */
        __asm volatile ("mov %0, pc" : "=r"(val));
        break;
    case REG_XPSR:
        __asm volatile ("mrs %0, xPSR" : "=r"(val));
        break;
    case REG_MSP:
        __asm volatile ("mrs %0, msp" : "=r"(val));
        break;
    case REG_PSP:
        __asm volatile ("mrs %0, psp" : "=r"(val));
        break;
    case REG_PRIMASK:
        __asm volatile ("mrs %0, primask" : "=r"(val));
        break;
    case REG_CONTROL:
        __asm volatile ("mrs %0, control" : "=r"(val));
        break;
    case REG_BASEPRI:
        __asm volatile ("mrs %0, basepri" : "=r"(val));
        break;
    default:
        val = 0xDEADBEEF;  /* Invalid register marker */
        break;
    }
    return val;
}

/**
 * @brief  Write CPU register
 */
static int _WriteCPUReg(uint8_t regId, uint32_t val)
{
    switch (regId) {
    case REG_PRIMASK:
        __asm volatile ("msr primask, %0" : : "r"(val));
        return 0;
    case REG_CONTROL:
        __asm volatile ("msr control, %0" : : "r"(val));
        return 0;
    case REG_BASEPRI:
        __asm volatile ("msr basepri, %0" : : "r"(val));
        return 0;
    case REG_MSP:
        __asm volatile ("msr msp, %0" : : "r"(val));
        return 0;
    case REG_PSP:
        __asm volatile ("msr psp, %0" : : "r"(val));
        return 0;
    default:
        /* Most registers are not writable (R0-R12, PC, LR, xPSR) */
        return -1;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void MSC_Debug_Init(void)
{
    s_u32LogHead = 0;
    s_u32LogTail = 0;
    s_u32LogCount = 0;
    memset(s_au8DebugLog, 0, sizeof(s_au8DebugLog));
    
    MSC_TRACE("[MSC_DEBUG] Vendor debug channel initialized\n");
}

void MSC_Debug_GetInfo(MSC_DeviceInfo_t *pInfo)
{
    if (pInfo == NULL)
        return;
    
    memset(pInfo, 0, sizeof(MSC_DeviceInfo_t));
    
    pInfo->u32ChipId = CHIP_ID;
    pInfo->u16FwVersion = FW_VERSION;
    pInfo->u16BuildDate = BUILD_DATE;
    pInfo->u32FlashSize = FMC->DFSDZ & 0x1F;  /* Read flash size from FMC */
    pInfo->u32FlashSize = (pInfo->u32FlashSize == 0) ? 512 * 1024 : (1 << (pInfo->u32FlashSize + 1)) * 1024;
    pInfo->u32RamSize = 160 * 1024;  /* M487 has 160KB SRAM */
    pInfo->u8I2cSpeed = I2C_BUS_SPEED;
    pInfo->u8I2cAddr = I2C_DEFAULT_ADDR;
    pInfo->u32LogHead = s_u32LogHead;
    pInfo->u32LogTail = s_u32LogTail;
    pInfo->u32LogCount = s_u32LogCount;
}

uint32_t MSC_Debug_ReadLog(uint8_t *pBuf, uint32_t u32Len)
{
    uint32_t u32Available;
    uint32_t u32Read = 0;
    uint32_t u32Tail = s_u32LogTail;
    uint32_t u32Head = s_u32LogHead;
    
    if (pBuf == NULL || u32Len == 0)
        return 0;
    
    /* Calculate available data */
    if (u32Head >= u32Tail) {
        u32Available = u32Head - u32Tail;
    } else {
        u32Available = MSC_DEBUG_LOG_SIZE - u32Tail + u32Head;
    }
    
    u32Read = (u32Len < u32Available) ? u32Len : u32Available;
    
    /* Copy from ring buffer to output */
    uint32_t u32First = MSC_DEBUG_LOG_SIZE - u32Tail;
    if (u32First >= u32Read) {
        /* Continuous segment */
        memcpy(pBuf, &s_au8DebugLog[u32Tail], u32Read);
        s_u32LogTail = (s_u32LogTail + u32Read) % MSC_DEBUG_LOG_SIZE;
    } else {
        /* Wrapped: copy tail portion, then head portion */
        memcpy(pBuf, &s_au8DebugLog[u32Tail], u32First);
        memcpy(&pBuf[u32First], s_au8DebugLog, u32Read - u32First);
        s_u32LogTail = u32Read - u32First;
    }
    
    s_u32LogCount -= u32Read;
    
    return u32Read;
}

void MSC_Debug_Log(uint8_t u8Level, uint8_t u8Module, uint8_t u8Code,
                   const uint8_t *pData, uint8_t u8Len)
{
    MSC_LogEntry_t entry;
    uint32_t mask;
    uint8_t *pDest;
    
    /* Get current cycle count for timestamp */
    entry.u32Timestamp = DWT->CYCCNT;
    entry.u8Level = u8Level;
    entry.u8Module = u8Module;
    entry.u8Code = u8Code;
    entry.u8Len = (u8Len > 4) ? 4 : u8Len;
    
    if (pData && entry.u8Len > 0) {
        memcpy(entry.au8Data, pData, entry.u8Len);
    } else {
        memset(entry.au8Data, 0, 4);
    }
    
    /* Enter critical section */
    mask = __get_PRIMASK();
    __disable_irq();
    
    /* Write entry to ring buffer */
    pDest = &s_au8DebugLog[s_u32LogHead];
    memcpy(pDest, &entry, sizeof(MSC_LogEntry_t));
    
    s_u32LogHead = (s_u32LogHead + sizeof(MSC_LogEntry_t)) % MSC_DEBUG_LOG_SIZE;
    
    if (s_u32LogCount < (MSC_DEBUG_LOG_SIZE / sizeof(MSC_LogEntry_t))) {
        s_u32LogCount++;
    } else {
        /* Buffer full - advance tail */
        s_u32LogTail = (s_u32LogTail + sizeof(MSC_LogEntry_t)) % MSC_DEBUG_LOG_SIZE;
    }
    
    /* Exit critical section */
    __set_PRIMASK(mask);
}

uint32_t MSC_Debug_ReadMem(uint32_t u32Addr, uint8_t *pBuf, uint32_t u32Len)
{
    uint32_t u32Read = 0;
    uint32_t u32Word;
    
    if (pBuf == NULL || u32Len == 0)
        return 0;
    
    /* Limit transfer length to prevent buffer overflow */
    if (u32Len > 508)
        u32Len = 508;
    
    /* Validate address range (allow SRAM and flash only) */
    if (u32Addr < 0x20000000 || u32Addr > 0x400FFFFF) {
        pBuf[0] = 0xFF;
        return 0;
    }
    
    while (u32Read < u32Len) {
        /* Handle alignment */
        if ((u32Addr & 0x03) && (u32Len - u32Read < 4)) {
            /* Unaligned start, less than 4 bytes remaining */
            uint8_t *p = (uint8_t *)u32Addr;
            pBuf[u32Read++] = *p++;
            u32Addr++;
        } else if (u32Addr & 0x03) {
            /* Unaligned - read one byte at a time */
            pBuf[u32Read++] = *(volatile uint8_t *)u32Addr;
            u32Addr++;
        } else if (u32Len - u32Read >= 4) {
            /* Aligned, 4+ bytes remaining - read word */
            u32Word = *(volatile uint32_t *)u32Addr;
            pBuf[u32Read++] = (uint8_t)(u32Word & 0xFF);
            pBuf[u32Read++] = (uint8_t)((u32Word >> 8) & 0xFF);
            pBuf[u32Read++] = (uint8_t)((u32Word >> 16) & 0xFF);
            pBuf[u32Read++] = (uint8_t)((u32Word >> 24) & 0xFF);
            u32Addr += 4;
        } else {
            /* Aligned, less than 4 bytes remaining */
            uint8_t *p = (uint8_t *)u32Addr;
            pBuf[u32Read++] = *p++;
            u32Addr++;
        }
    }
    
    return u32Read;
}

uint32_t MSC_Debug_WriteMem(uint32_t u32Addr, const uint8_t *pData, uint32_t u32Len)
{
    uint32_t u32Written = 0;
    
    if (pData == NULL || u32Len == 0)
        return 0;
    
    /* Limit transfer length */
    if (u32Len > 508)
        u32Len = 508;
    
    /* Validate address range (SRAM only for safety) */
    if (u32Addr < 0x20000000 || u32Addr > 0x20027FFF) {
        /* Writing to flash requires FMC, which is more complex */
        /* For safety, only allow SRAM writes via debug channel */
        return 0;
    }
    
    while (u32Written < u32Len) {
        if ((u32Addr & 0x03) == 0 && (u32Len - u32Written >= 4)) {
            /* Word-aligned write */
            uint32_t word = pData[u32Written]
                | (pData[u32Written + 1] << 8)
                | (pData[u32Written + 2] << 16)
                | (pData[u32Written + 3] << 24);
            _WriteMem32(u32Addr, word);
            u32Addr += 4;
            u32Written += 4;
        } else {
            /* Byte write */
            *(volatile uint8_t *)u32Addr = pData[u32Written];
            u32Addr++;
            u32Written++;
        }
    }
    
    return u32Written;
}

uint32_t MSC_Debug_ReadReg(uint8_t u8RegId)
{
    if (u8RegId > REG_MAX)
        return 0xDEADBEEF;
    return _ReadCPUReg(u8RegId);
}

int32_t MSC_Debug_WriteReg(uint8_t u8RegId, uint32_t u32Val)
{
    if (u8RegId > REG_MAX)
        return -1;
    return _WriteCPUReg(u8RegId, u32Val);
}

uint8_t* MSC_Debug_Echo(const uint8_t *pData, uint32_t u32Len)
{
    /* Echo is handled inline in MSC_VendorCommand - this is a placeholder */
    return (uint8_t *)pData;
}

/*---------------------------------------------------------------------------------------------------------*/
/* MSC Vendor Command Handler                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void MSC_VendorCommand(CBW_t *pCBW)
{
    uint8_t u8SubCmd;
    uint32_t u32Addr;
    uint32_t u32Len;
    uint32_t u32Result;
    uint8_t *pData;
    uint8_t au8Response[32];
    
    if (pCBW == NULL)
        return;
    
    /* Extract sub-command from CDB */
    u8SubCmd = pCBW->au8Data[0];  /* Byte 1 of CDB */
    
    /* Extract address and length */
    u32Addr = ((uint32_t)pCBW->au8Data[1] << 8) | ((uint32_t)pCBW->au8Data[2] & 0xFF);
    u32Len = pCBW->au8Data[3];
    
    memset(au8Response, 0, sizeof(au8Response));
    
    switch (u8SubCmd) {
    /*--------------------------------------------------------*/
    case DBG_GET_INFO: {
        /* Get device info - no data stage needed, info in response */
        MSC_DeviceInfo_t info;
        MSC_Debug_GetInfo(&info);
        memcpy(au8Response, &info, sizeof(info));
        u32Result = sizeof(info);
        
        MSC_TRACE("[MSC_DEBUG] GET_INFO request\n");
        
        /* Send response via Bulk IN */
        MSC_BulkIn((uint32_t)&au8Response, u32Result);
        MSC_AckCmd(0);
        break;
    }
    
    /*--------------------------------------------------------*/
    case DBG_READ_LOG: {
        /* Read debug log - response is binary log data */
        uint8_t abLog[MSC_DEBUG_LOG_SIZE];
        uint32_t u32Read = MSC_Debug_ReadLog(abLog, u32Len);
        
        MSC_TRACE("[MSC_DEBUG] READ_LOG request, len=%lu\n", (unsigned long)u32Read);
        
        MSC_BulkIn((uint32_t)abLog, u32Read);
        MSC_AckCmd(0);
        break;
    }
    
    /*--------------------------------------------------------*/
    case DBG_ECHO: {
        /* Echo test - read data from Bulk OUT, echo back via Bulk IN */
        /* First read data from Bulk OUT, then send back via Bulk IN */
        uint8_t abEcho[64];
        uint32_t u32ReadLen = (u32Len > 64) ? 64 : u32Len;
        
        /* Read echo data from Bulk OUT (host already sent it) */
        MSC_BulkOut((uint32_t)abEcho, u32ReadLen);
        
        /* Echo back via Bulk IN */
        MSC_BulkIn((uint32_t)abEcho, u32ReadLen);
        MSC_AckCmd(0);
        
        MSC_TRACE("[MSC_DEBUG] ECHO test, len=%lu\n", (unsigned long)u32ReadLen);
        break;
    }
    
    /*--------------------------------------------------------*/
    case DBG_READ_REG: {
        /* Read CPU register */
        uint32_t u32Val = MSC_Debug_ReadReg((uint8_t)u32Addr);  /* addr byte = reg ID */
        
        MSC_TRACE("[MSC_DEBUG] READ_REG r%lu = 0x%08lX\n", (unsigned long)u32Addr, (unsigned long)u32Val);
        
        memcpy(au8Response, &u32Val, 4);
        MSC_BulkIn((uint32_t)&au8Response, 4);
        MSC_AckCmd(0);
        break;
    }
    
    /*--------------------------------------------------------*/
    case DBG_WRITE_REG: {
        /* Write CPU register - data in Bulk OUT stage */
        uint8_t abVal[4];
        int32_t i32Result;
        
        /* Read value from Bulk OUT */
        MSC_BulkOut((uint32_t)abVal, 4);
        
        i32Result = MSC_Debug_WriteReg((uint8_t)u32Addr,
                                        ((uint32_t)abVal[0]) |
                                        ((uint32_t)abVal[1] << 8) |
                                        ((uint32_t)abVal[2] << 16) |
                                        ((uint32_t)abVal[3] << 24));
        
        au8Response[0] = (i32Result == 0) ? 0x00 : 0x01;  /* 0=success, 1=fail */
        
        MSC_TRACE("[MSC_DEBUG] WRITE_REG r%lu = 0x%02X%02X%02X%02X, result=%d\n",
                  (unsigned long)u32Addr,
                  abVal[3], abVal[2], abVal[1], abVal[0], (int)i32Result);
        
        MSC_BulkIn((uint32_t)&au8Response, 1);
        MSC_AckCmd(0);
        break;
    }
    
    /*--------------------------------------------------------*/
    case DBG_READ_MEM: {
        /* Read memory - response is memory data */
        uint32_t u32MemAddr = ((uint32_t)pCBW->au8Data[1] << 16) |
                              ((uint32_t)pCBW->au8Data[2] << 8) |
                              ((uint32_t)pCBW->au8Data[3] & 0xFF);
        uint32_t u32MemLen = pCBW->dCBWDataTransferLength;
        
        if (u32MemLen > 512)
            u32MemLen = 512;
        
        /* Use static buffer for memory read result */
        static uint8_t s_au8MemBuf[512] __attribute__((aligned(4)));
        u32Result = MSC_Debug_ReadMem(u32MemAddr, s_au8MemBuf, u32MemLen);
        
        MSC_TRACE("[MSC_DEBUG] READ_MEM addr=0x%06lX len=%lu\n",
                  (unsigned long)u32MemAddr, (unsigned long)u32Result);
        
        MSC_BulkIn((uint32_t)s_au8MemBuf, u32Result);
        MSC_AckCmd(0);
        break;
    }
    
    /*--------------------------------------------------------*/
    case DBG_WRITE_MEM: {
        /* Write memory - data in Bulk OUT, then response via Bulk IN */
        uint32_t u32MemAddr = ((uint32_t)pCBW->au8Data[1] << 16) |
                              ((uint32_t)pCBW->au8Data[2] << 8) |
                              ((uint32_t)pCBW->au8Data[3] & 0xFF);
        uint32_t u32MemLen = pCBW->dCBWDataTransferLength;
        
        if (u32MemLen > 512)
            u32MemLen = 512;
        
        /* Read data from Bulk OUT */
        MSC_BulkOut((uint32_t)s_au8MemBuf, u32MemLen);  /* reuse static buffer */
        
        /* Write to memory */
        u32Result = MSC_Debug_WriteMem(u32MemAddr, s_au8MemBuf, u32MemLen);
        
        au8Response[0] = (uint8_t)(u32Result & 0xFF);
        au8Response[1] = (uint8_t)((u32Result >> 8) & 0xFF);
        au8Response[2] = (uint8_t)((u32Result >> 16) & 0xFF);
        au8Response[3] = (uint8_t)((u32Result >> 24) & 0xFF);
        
        MSC_TRACE("[MSC_DEBUG] WRITE_MEM addr=0x%06lX len=%lu written=%lu\n",
                  (unsigned long)u32MemAddr, (unsigned long)u32MemLen, (unsigned long)u32Result);
        
        MSC_BulkIn((uint32_t)&au8Response, 4);
        MSC_AckCmd(0);
        break;
    }
    
    /*--------------------------------------------------------*/
    default:
        /* Unknown sub-command - STALL to indicate error */
        MSC_TRACE("[MSC_DEBUG] Unknown sub-cmd 0x%02X\n", u8SubCmd);
        
        g_au8SenseKey[0] = 0x05;  /* ILLEGAL REQUEST */
        g_au8SenseKey[1] = 0x24;  /* ASC: ASCQ: Invalid field in CDB */
        g_au8SenseKey[2] = 0x00;
        
        if (pCBW->dCBWDataTransferLength > 0)
            MSC_AckCmd(pCBW->dCBWDataTransferLength);
        else
            MSC_AckCmd(0);
        break;
    }
}
