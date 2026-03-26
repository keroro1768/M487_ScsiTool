#pragma once
#ifndef SCSI_H
#define SCSI_H

#include <cstdint>
#include <cstring>

// SCSI Operation Codes
#define SCSI_OP_READ10        0x28
#define SCSI_OP_WRITE10       0x2A
#define SCSI_OP_READ_CAPACITY 0x25
#define SCSI_OP_INQUIRY       0x12
#define SCSI_OP_TEST_UNIT_READY 0x00

// Vendor-specific opcode (custom for M487)
#define SCSI_OP_VENDOR_READ   0xC0

// CBW Signature
#define CBW_SIGNATURE         0x43425355
#define CSW_SIGNATURE         0x53425355

// CBW structure (31 bytes)
#pragma pack(push, 1)
struct CBW {
    uint32_t dCBWSignature;      // 'USBC' = 0x43425355
    uint32_t dCBWTag;            // Host-generated tag
    uint32_t dCBWDataTransferLength; // Bytes to transfer
    uint8_t  bmCBWFlags;         // Bit 7: Data direction (1=IN, 0=OUT)
    uint8_t  bCBWLUN;            // Logical Unit Number
    uint8_t  bCBWCBLength;       // Length of CDB (5..16)
    uint8_t  CDB[16];            // SCSI Command Descriptor Block
};
#pragma pack(pop)

// CSW structure (13 bytes)
#pragma pack(push, 1)
struct CSW {
    uint32_t dCSWSignature;      // 'USBS' = 0x53425355
    uint32_t dCSWTag;            // Must match CBW Tag
    uint32_t dCSWDataResidue;    // Bytes not transferred
    uint8_t  bCSWStatus;         // 0x00=Good, 0x01=Failed, 0x02=Phase Error
};
#pragma pack(pop)

// SCSI READ CAPACITY response (8 bytes)
#pragma pack(push, 1)
struct SCSI_READ_CAPACITY_10 {
    uint32_t returnedLogicalBlockAddress;  // Last LBA (MSB first)
    uint32_t blockLengthInBytes;            // Block size (MSB first, usually 0x00000200 = 512)
};
#pragma pack(pop)

// SCSI INQUIRY response (36 bytes)
#pragma pack(push, 1)
struct SCSI_INQUIRY_DATA {
    uint8_t  peripheralDeviceType;   // b5-b0
    uint8_t  peripheralQualifier;      // b7-b5
    uint8_t  deviceTypeModifier;     // RMB, version
    uint8_t  version;               // ANSI version
    uint8_t  responseDataFormat;     // 
    uint8_t  additionalLength;       // = 31
    uint8_t  reserved[2];
    uint8_t  vendorID[8];            // 'Nuvoton '
    uint8_t  productID[16];          // 'USB Mass Storage'
    uint8_t  productRevision[4];     // '1.00'
};
#pragma pack(pop)

class ScsiCommands {
public:
    // Build CDB for SCSI READ(10)
    static void buildRead10(uint8_t* cdb, uint32_t lba, uint16_t transferLength);

    // Build CDB for SCSI WRITE(10)
    static void buildWrite10(uint8_t* cdb, uint32_t lba, uint16_t transferLength);

    // Build CDB for SCSI READ CAPACITY(10)
    static void buildReadCapacity10(uint8_t* cdb);

    // Build CDB for SCSI INQUIRY
    static void buildInquiry(uint8_t* cdb, uint8_t allocationLength);

    // Build CDB for Vendor-specific READ command
    // Sub-cmd 0x00: Read device string (e.g., "ELAN-USB-I2C-BRIDGE")
    static void buildVendorRead(uint8_t* cdb, uint8_t subCmd, uint16_t transferLength);

    // Byte-swap buffer (16-bit words) - for verifying vendor command response
    static void byteSwapBuffer(uint8_t* buffer, size_t length);

    // Verify byte-swap pattern: expected original pattern was 0x55 0xAA repeating
    static bool verifyByteSwapPattern(const uint8_t* buffer, size_t length);
};

#endif // SCSI_H
