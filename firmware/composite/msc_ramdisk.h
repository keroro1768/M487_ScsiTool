/**
 * @file     msc_ramdisk.h
 * @brief    MSC RAM Disk with FAT12 filesystem and log file
 * @version  1.0.0
 *
 * Provides a 30KB FAT12 RAM disk visible to the PC as a USB drive.
 * Firmware can write text to LOG.TXT which the PC reads directly.
 */

#ifndef __MSC_RAMDISK_H__
#define __MSC_RAMDISK_H__

#include <stdint.h>

/**
 * @brief  Initialize the RAM disk with FAT12 filesystem and empty LOG.TXT
 * @param  pu8Storage  Pointer to RAM disk storage buffer
 * @param  u32Sectors  Total number of 512-byte sectors
 */
void RamDisk_Init(uint8_t *pu8Storage, uint32_t u32Sectors);

/**
 * @brief  Write a text string to LOG.TXT (append mode)
 * @param  szText  Null-terminated string to append
 * @return Number of bytes written, or -1 on error (disk full)
 */
int32_t RamDisk_Log(const char *szText);

/**
 * @brief  Clear LOG.TXT content (reset to empty)
 */
void RamDisk_LogClear(void);

#endif /* __MSC_RAMDISK_H__ */
