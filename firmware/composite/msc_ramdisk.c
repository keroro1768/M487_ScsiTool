/**
 * @file     msc_ramdisk.c
 * @brief    MSC RAM Disk with FAT12 filesystem and log file
 * @version  1.0.0
 *
 * Layout (60 sectors x 512 bytes = 30KB):
 *   Sector 0:    Boot sector (BPB)
 *   Sector 1:    FAT12 table
 *   Sector 2:    Root directory (16 entries)
 *   Sectors 3-59: Data area (57 sectors, cluster 2 starts at sector 3)
 *
 * LOG.TXT occupies the entire data area (max ~29KB of text).
 */

#include <stdio.h>
#include <string.h>
#include "msc_ramdisk.h"

#define SECTOR_SIZE     512
#define FAT_SECTOR      1
#define ROOTDIR_SECTOR  2
#define DATA_START      3       /* First data sector (cluster 2) */
#define MAX_DIR_ENTRIES 16

/* Module state */
static uint8_t  *s_pu8Storage;
static uint32_t  s_u32TotalSectors;
static uint32_t  s_u32DataSectors;  /* Sectors available for file data */
static uint32_t  s_u32LogSize;      /* Current LOG.TXT file size in bytes */

/* FAT12 Boot Sector (BPB) */
static const uint8_t s_au8BootSector[62] = {
    0xEB, 0x3C, 0x90,                  /* Jump + NOP */
    'M', 'S', 'D', 'O', 'S', '5', '.', '0', /* OEM Name */
    0x00, 0x02,                         /* BytsPerSec = 512 */
    0x01,                               /* SecPerClus = 1 */
    0x01, 0x00,                         /* RsvdSecCnt = 1 */
    0x01,                               /* NumFATs = 1 */
    0x10, 0x00,                         /* RootEntCnt = 16 */
    0x3C, 0x00,                         /* TotSec16 = 60 */
    0xF0,                               /* Media = removable */
    0x01, 0x00,                         /* FATSz16 = 1 */
    0x01, 0x00,                         /* SecPerTrk = 1 */
    0x01, 0x00,                         /* NumHeads = 1 */
    0x00, 0x00, 0x00, 0x00,            /* HiddSec = 0 */
    0x00, 0x00, 0x00, 0x00,            /* TotSec32 = 0 */
    0x00,                               /* DrvNum = 0 */
    0x00,                               /* Reserved1 */
    0x29,                               /* BootSig = 0x29 */
    0x78, 0x56, 0x34, 0x12,            /* VolID */
    'M', '4', '8', '7', ' ', 'L', 'O', 'G', ' ', ' ', ' ', /* VolLab (11 bytes) */
    'F', 'A', 'T', '1', '2', ' ', ' ', ' ',  /* FilSysType (8 bytes) */
};

/*---------------------------------------------------------------------------*/
/* Helper: Write a FAT12 entry                                               */
/*---------------------------------------------------------------------------*/
static void fat12_set_entry(uint8_t *fat, uint32_t idx, uint16_t val)
{
    uint32_t off = idx + (idx / 2);   /* byte offset: idx * 1.5 */
    if (idx & 1) {
        /* Odd: high nibble of fat[off], all of fat[off+1] */
        fat[off] = (fat[off] & 0x0F) | ((val & 0x0F) << 4);
        fat[off + 1] = (val >> 4) & 0xFF;
    } else {
        /* Even: all of fat[off], low nibble of fat[off+1] */
        fat[off] = val & 0xFF;
        fat[off + 1] = (fat[off + 1] & 0xF0) | ((val >> 8) & 0x0F);
    }
}

/*---------------------------------------------------------------------------*/
/* Update the directory entry file size and FAT chain                        */
/*---------------------------------------------------------------------------*/
static void update_dir_and_fat(void)
{
    uint8_t *rootdir = s_pu8Storage + (ROOTDIR_SECTOR * SECTOR_SIZE);
    uint8_t *fat     = s_pu8Storage + (FAT_SECTOR * SECTOR_SIZE);
    uint32_t clusters_needed, i;

    /* Update file size in directory entry (offset 28, 4 bytes LE) */
    rootdir[28] = (s_u32LogSize >>  0) & 0xFF;
    rootdir[29] = (s_u32LogSize >>  8) & 0xFF;
    rootdir[30] = (s_u32LogSize >> 16) & 0xFF;
    rootdir[31] = (s_u32LogSize >> 24) & 0xFF;

    /* Rebuild FAT chain for LOG.TXT */
    /* Clear FAT (keep entries 0 and 1) */
    memset(fat + 3, 0, SECTOR_SIZE - 3);

    /* Re-set reserved entries */
    fat12_set_entry(fat, 0, 0xFF0);
    fat12_set_entry(fat, 1, 0xFFF);

    if (s_u32LogSize == 0) {
        /* Empty file — first cluster = 0 in dir entry */
        rootdir[26] = 0;
        rootdir[27] = 0;
        return;
    }

    /* First cluster = 2 */
    rootdir[26] = 2;
    rootdir[27] = 0;

    clusters_needed = (s_u32LogSize + SECTOR_SIZE - 1) / SECTOR_SIZE;
    if (clusters_needed > s_u32DataSectors)
        clusters_needed = s_u32DataSectors;

    /* Build chain: 2 -> 3 -> 4 -> ... -> (2+N-1) -> 0xFFF */
    for (i = 0; i < clusters_needed - 1; i++) {
        fat12_set_entry(fat, 2 + i, 3 + i);
    }
    fat12_set_entry(fat, 2 + clusters_needed - 1, 0xFFF);  /* End of chain */
}

/*---------------------------------------------------------------------------*/
/* Public API                                                                */
/*---------------------------------------------------------------------------*/

void RamDisk_Init(uint8_t *pu8Storage, uint32_t u32Sectors)
{
    uint8_t *rootdir;

    s_pu8Storage     = pu8Storage;
    s_u32TotalSectors = u32Sectors;
    s_u32DataSectors  = u32Sectors - DATA_START;
    s_u32LogSize      = 0;

    /* Zero entire storage */
    memset(pu8Storage, 0, u32Sectors * SECTOR_SIZE);

    /* Write boot sector */
    memcpy(pu8Storage, s_au8BootSector, sizeof(s_au8BootSector));
    /* Boot signature */
    pu8Storage[510] = 0x55;
    pu8Storage[511] = 0xAA;

    /* Initialize FAT */
    {
        uint8_t *fat = pu8Storage + (FAT_SECTOR * SECTOR_SIZE);
        fat12_set_entry(fat, 0, 0xFF0);
        fat12_set_entry(fat, 1, 0xFFF);
    }

    /* Create LOG.TXT directory entry */
    rootdir = pu8Storage + (ROOTDIR_SECTOR * SECTOR_SIZE);
    memcpy(rootdir, "LOG     TXT", 11);  /* 8.3 filename */
    rootdir[11] = 0x20;                   /* Archive attribute */
    /* Time/date: 2026-03-31 12:00:00 */
    rootdir[22] = 0x00; rootdir[23] = 0x60;  /* Time: 12:00 */
    rootdir[24] = 0x9F; rootdir[25] = 0x5C;  /* Date: 2026-03-31 */
    /* First cluster = 0 (empty file) */
    rootdir[26] = 0; rootdir[27] = 0;
    /* File size = 0 */
    rootdir[28] = 0; rootdir[29] = 0; rootdir[30] = 0; rootdir[31] = 0;

    /* Create volume label entry */
    rootdir += 32;  /* Second entry */
    memcpy(rootdir, "M487 LOG   ", 11);
    rootdir[11] = 0x08;  /* Volume label attribute */
}

int32_t RamDisk_Log(const char *szText)
{
    uint32_t len, maxSize, space;
    uint8_t *dataArea;

    if (szText == NULL) return -1;
    len = strlen(szText);
    if (len == 0) return 0;

    maxSize = s_u32DataSectors * SECTOR_SIZE;
    space = maxSize - s_u32LogSize;

    if (space == 0) return -1;
    if (len > space) len = space;

    dataArea = s_pu8Storage + (DATA_START * SECTOR_SIZE);
    memcpy(dataArea + s_u32LogSize, szText, len);
    s_u32LogSize += len;

    update_dir_and_fat();

    return (int32_t)len;
}

void RamDisk_LogClear(void)
{
    uint8_t *dataArea = s_pu8Storage + (DATA_START * SECTOR_SIZE);
    memset(dataArea, 0, s_u32DataSectors * SECTOR_SIZE);
    s_u32LogSize = 0;
    update_dir_and_fat();
}
