#include "scsi.h"
#include <cstring>
#include <algorithm>

void ScsiCommands::buildRead10(uint8_t* cdb, uint32_t lba, uint16_t transferLength)
{
    memset(cdb, 0, 16);
    cdb[0] = SCSI_OP_READ10;                     // Opcode = 0x28
    cdb[1] = 0;                                  // Obsolete
    cdb[2] = (lba >> 24) & 0xFF;                // LBA byte 3 (MSB)
    cdb[3] = (lba >> 16) & 0xFF;                // LBA byte 2
    cdb[4] = (lba >> 8) & 0xFF;                 // LBA byte 1
    cdb[5] = lba & 0xFF;                         // LBA byte 0 (LSB)
    cdb[6] = 0;                                  // Reserved
    cdb[7] = (transferLength >> 8) & 0xFF;      // Transfer length MSB
    cdb[8] = transferLength & 0xFF;             // Transfer length LSB
    cdb[9] = 0;                                  // Control
}

void ScsiCommands::buildWrite10(uint8_t* cdb, uint32_t lba, uint16_t transferLength)
{
    memset(cdb, 0, 16);
    cdb[0] = SCSI_OP_WRITE10;                    // Opcode = 0x2A
    cdb[1] = 0;                                  // Obsolete
    cdb[2] = (lba >> 24) & 0xFF;                // LBA byte 3 (MSB)
    cdb[3] = (lba >> 16) & 0xFF;                // LBA byte 2
    cdb[4] = (lba >> 8) & 0xFF;                 // LBA byte 1
    cdb[5] = lba & 0xFF;                         // LBA byte 0 (LSB)
    cdb[6] = 0;                                  // Reserved
    cdb[7] = (transferLength >> 8) & 0xFF;      // Transfer length MSB
    cdb[8] = transferLength & 0xFF;             // Transfer length LSB
    cdb[9] = 0;                                  // Control
}

void ScsiCommands::buildReadCapacity10(uint8_t* cdb)
{
    memset(cdb, 0, 16);
    cdb[0] = SCSI_OP_READ_CAPACITY;              // Opcode = 0x25
    cdb[1] = 0;                                  // Obsolete
    cdb[2] = 0;                                  // Reserved
    cdb[3] = 0;
    cdb[4] = 0;                                  // Reserved
    cdb[5] = 0;
    cdb[6] = 0;                                  // Reserved / PMI (Partial Medium Indicator)
    cdb[7] = 0;
    cdb[8] = 0;                                  // Reserved / Control
    cdb[9] = 0;                                  // Control
}

void ScsiCommands::buildInquiry(uint8_t* cdb, uint8_t allocationLength)
{
    memset(cdb, 0, 16);
    cdb[0] = SCSI_OP_INQUIRY;                    // Opcode = 0x12
    cdb[1] = 0;                                  // EVPD (Enable Vital Product Data) = 0
    cdb[2] = 0;                                  // Page code = 0
    cdb[3] = allocationLength;                   // Allocation length LSB
    cdb[4] = 0;                                  // (MSB is 0 since allocation length is 8-bit)
    cdb[5] = 0;                                  // Control
}

void ScsiCommands::buildVendorRead(uint8_t* cdb, uint8_t subCmd, uint16_t transferLength)
{
    memset(cdb, 0, 16);
    cdb[0] = SCSI_OP_VENDOR_READ;                // Opcode = 0xC0 (vendor-specific)
    cdb[1] = subCmd;                             // Sub-command (e.g., 0x00 = Get Device String)
    cdb[2] = (transferLength >> 8) & 0xFF;      // Transfer length MSB
    cdb[3] = transferLength & 0xFF;             // Transfer length LSB
    cdb[4] = 0;                                  // Reserved
    cdb[5] = 0;                                  // Reserved
    cdb[6] = 0;                                  // Reserved
    cdb[7] = 0;                                  // Reserved
    cdb[8] = 0;                                  // Reserved
    cdb[9] = 0;                                  // Control
}

void ScsiCommands::byteSwapBuffer(uint8_t* buffer, size_t length)
{
    // Swap adjacent bytes: 0x55 0xAA 0x55 0xAA -> 0xAA 0x55 0xAA 0x55
    for (size_t i = 0; i + 1 < length; i += 2) {
        uint8_t temp = buffer[i];
        buffer[i] = buffer[i + 1];
        buffer[i + 1] = temp;
    }
}

bool ScsiCommands::verifyByteSwapPattern(const uint8_t* buffer, size_t length)
{
    // Expected pattern: 0xAA 0x55 0xAA 0x55 ... (byte-swapped version of 0x55 0xAA)
    for (size_t i = 0; i + 1 < length; i += 2) {
        if (buffer[i] != 0xAA || buffer[i + 1] != 0x55) {
            return false;
        }
    }
    return true;
}
