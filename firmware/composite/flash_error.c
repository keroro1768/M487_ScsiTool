/**
 * @file     flash_error.c
 * @brief    Flash Error Log System Implementation
 * @version  1.0.0
 *
 * Persistent error storage in reserved flash region.
 * Uses RAM buffer with periodic flash flush for wear reduction.
 *
 * Flash Layout (4KB page at APROM end):
 *   [0x0007F000] Header (8 bytes)
 *   [0x0007F008] Entry[0] (12 bytes)
 *   ...
 */

#include "NuMicro.h"
#include "flash_error.h"
#include <string.h>

/*---------------------------------------------------------------------------------------------------------*/
/* RAM Buffer for Error Log (kept in SRAM for fast access)                                                */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  RAM copy of flash error log header
 * @note   Read from flash at init, write back periodically or on critical error
 */
static FlashErrorHeader_t s_sHeader = {
    .u32Magic = FLASH_ERROR_MAGIC,
    .u16Head  = 0,
    .u16Tail  = 0,
    .u16Count = 0,
    .u16Reserved = 0
};

/**
 * @brief  RAM buffer for error log entries
 * @note   Copied from flash at init, kept in SRAM for speed
 */
static uint8_t s_au8LogBuffer[FLASH_ERROR_PAGE_SIZE] __attribute__((aligned(4)));

/** @brief  Flag: true if log has been modified and needs flash sync */
static volatile uint8_t s_u8Dirty = 0;

/*---------------------------------------------------------------------------------------------------------*/
/* Local Helper Functions                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Read a 32-bit word from flash (via FMC)
 */
static uint32_t _ReadFlashWord(uint32_t u32Addr)
{
    return FMC_Read(u32Addr);
}

/**
 * @brief  Write a 32-bit word to flash (via FMC)
 * @note   Must call FMC_Erase() before writing to a page
 */
static int32_t _WriteFlashWord(uint32_t u32Addr, uint32_t u32Data)
{
    return FMC_Write(u32Addr, u32Data);
}

/**
 * @brief  Erase a flash page
 */
static int32_t _EraseFlashPage(uint32_t u32PageAddr)
{
    int32_t i32Result;
    FMC_Open();
    i32Result = FMC_Erase(u32PageAddr);
    FMC_Close();
    return i32Result;
}

/**
 * @brief  Write entire log buffer to flash (erase + write)
 * @return 0 on success, -1 on failure
 */
static int32_t _SyncToFlash(void)
{
    uint32_t u32Addr;
    uint32_t *pu32Src;
    int32_t i32Result = 0;

    /* Erase page */
    if (_EraseFlashPage(FLASH_ERROR_PAGE_ADDR) != 0) {
        return -1;
    }

    /* Write header + entries word by word */
    FMC_Open();
    pu32Src = (uint32_t *)s_au8LogBuffer;
    for (u32Addr = FLASH_ERROR_PAGE_ADDR;
         u32Addr < FLASH_ERROR_PAGE_ADDR + FLASH_ERROR_PAGE_SIZE;
         u32Addr += 4, pu32Src++) {
        if (FMC_Write(u32Addr, *pu32Src) != 0) {
            i32Result = -1;
            break;
        }
    }
    FMC_Close();

    if (i32Result == 0) {
        s_u8Dirty = 0;
    }

    return i32Result;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Public Functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

int32_t FlashError_Init(void)
{
    uint32_t u32Word;
    uint32_t u32Addr;
    uint8_t *pu8Dst;

    /* Read header from flash */
    FMC_Open();
    u32Word = FMC_Read(FLASH_ERROR_BASE);
    FMC_Close();

    if (u32Word == FLASH_ERROR_MAGIC) {
        /* Valid magic - header exists, read entire page into RAM */
        s_sHeader.u32Magic = u32Word;

        FMC_Open();
        u32Word = FMC_Read(FLASH_ERROR_BASE + 4);
        FMC_Close();
        s_sHeader.u16Head  = (uint16_t)((u32Word >> 0)  & 0xFFFF);
        s_sHeader.u16Tail  = (uint16_t)((u32Word >> 16) & 0xFFFF);

        FMC_Open();
        u32Word = FMC_Read(FLASH_ERROR_BASE + 8);
        FMC_Close();
        s_sHeader.u16Count    = (uint16_t)((u32Word >> 0)  & 0xFFFF);
        s_sHeader.u16Reserved = (uint16_t)((u32Word >> 16) & 0xFFFF);

        /* Read entries into RAM buffer */
        pu8Dst = s_au8LogBuffer + sizeof(FlashErrorHeader_t);
        for (u32Addr = FLASH_ERROR_BASE + sizeof(FlashErrorHeader_t);
             u32Addr < FLASH_ERROR_PAGE_ADDR + FLASH_ERROR_PAGE_SIZE;
             u32Addr += 4, pu8Dst += 4) {
            FMC_Open();
            *(uint32_t *)pu8Dst = FMC_Read(u32Addr);
            FMC_Close();
        }

        s_u8Dirty = 0;

    } else {
        /* No valid log - initialize empty log in RAM, format flash */
        s_sHeader.u32Magic = FLASH_ERROR_MAGIC;
        s_sHeader.u16Head  = 0;
        s_sHeader.u16Tail  = 0;
        s_sHeader.u16Count = 0;
        s_sHeader.u16Reserved = 0;

        /* Clear RAM buffer */
        memset(s_au8LogBuffer, 0xFF, sizeof(s_au8LogBuffer));
        memcpy(s_au8LogBuffer, &s_sHeader, sizeof(FlashErrorHeader_t));

        /* Write empty log to flash */
        s_u8Dirty = 1;
        if (_SyncToFlash() != 0) {
            /* Flash write failed - log will be in RAM only */
            s_u8Dirty = 0;
        }
    }

    return 0;
}

int32_t FlashError_Add(uint16_t u16ErrorCode, uint8_t u8Module,
                       uint8_t u8Flags, uint32_t u32Data)
{
    FlashErrorEntry_t *pEntry;
    uint16_t u16Head;

    /* Check if log is full (head == tail and count > 0 means full) */
    if (s_sHeader.u16Head == s_sHeader.u16Tail && s_sHeader.u16Count > 0) {
        /* Log full - advance tail (overwrite oldest) */
        s_sHeader.u16Tail = (s_sHeader.u16Tail + 1) % FLASH_ERROR_MAX_ENTRIES;
    }

    /* Get entry pointer */
    u16Head = s_sHeader.u16Head;
    pEntry = (FlashErrorEntry_t *)(s_au8LogBuffer + sizeof(FlashErrorHeader_t)
                                   + u16Head * sizeof(FlashErrorEntry_t));

    /* Fill entry */
    pEntry->u32Timestamp = DWT->CYCCNT;
    pEntry->u16ErrorCode = u16ErrorCode;
    pEntry->u8Module     = u8Module;
    pEntry->u8Flags      = u8Flags;
    pEntry->u32Data      = u32Data;

    /* Update header */
    s_sHeader.u16Head = (u16Head + 1) % FLASH_ERROR_MAX_ENTRIES;
    s_sHeader.u16Count++;

    /* Copy updated header to RAM buffer */
    memcpy(s_au8LogBuffer, &s_sHeader, sizeof(FlashErrorHeader_t));

    /* Mark dirty and sync to flash */
    s_u8Dirty = 1;
    if (_SyncToFlash() != 0) {
        /* Flash sync failed - entry is in RAM but not persisted */
        s_u8Dirty = 0;
        return -1;
    }

    return 0;
}

uint16_t FlashError_Read(FlashErrorEntry_t *pEntries, uint16_t u16Max)
{
    uint16_t u16Count;
    uint16_t u16Tail;
    uint16_t u16Read = 0;
    FlashErrorEntry_t *pSrc;

    if (pEntries == NULL || u16Max == 0)
        return 0;

    /* Calculate actual count */
    if (s_sHeader.u16Head >= s_sHeader.u16Tail) {
        u16Count = s_sHeader.u16Head - s_sHeader.u16Tail;
    } else {
        u16Count = FLASH_ERROR_MAX_ENTRIES - s_sHeader.u16Tail + s_sHeader.u16Head;
    }

    if (u16Count == 0)
        return 0;

    /* Limit to requested count */
    if (u16Max < u16Count)
        u16Count = u16Max;

    u16Tail = s_sHeader.u16Tail;
    pSrc = (FlashErrorEntry_t *)(s_au8LogBuffer + sizeof(FlashErrorHeader_t));

    while (u16Read < u16Count) {
        memcpy(&pEntries[u16Read], &pSrc[u16Tail], sizeof(FlashErrorEntry_t));
        u16Tail = (u16Tail + 1) % FLASH_ERROR_MAX_ENTRIES;
        u16Read++;
    }

    return u16Read;
}

uint16_t FlashError_Count(void)
{
    uint16_t u16Count;

    if (s_sHeader.u16Head >= s_sHeader.u16Tail) {
        u16Count = s_sHeader.u16Head - s_sHeader.u16Tail;
    } else {
        u16Count = FLASH_ERROR_MAX_ENTRIES - s_sHeader.u16Tail + s_sHeader.u16Head;
    }

    return u16Count;
}

int32_t FlashError_Clear(void)
{
    /* Reset header */
    s_sHeader.u16Head  = 0;
    s_sHeader.u16Tail  = 0;
    s_sHeader.u16Count = 0;

    /* Copy to RAM buffer */
    memcpy(s_au8LogBuffer, &s_sHeader, sizeof(FlashErrorHeader_t));

    /* Write to flash */
    s_u8Dirty = 1;
    if (_SyncToFlash() != 0) {
        s_u8Dirty = 0;
        return -1;
    }

    return 0;
}

void FlashError_GetInfo(uint16_t *pHead, uint16_t *pTail, uint16_t *pCount)
{
    if (pHead)  *pHead  = s_sHeader.u16Head;
    if (pTail)  *pTail  = s_sHeader.u16Tail;
    if (pCount) *pCount = s_sHeader.u16Count;
}
