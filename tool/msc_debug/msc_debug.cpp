/**
 * @file     msc_debug.cpp
 * @brief    MSC Vendor Debug Channel CLI Tool for Windows
 * @version  1.0.0
 *
 * Uses Windows SCSI PassThrough (IOCTL_SCSI_PASS_THROUGH_DIRECT) to send
 * vendor-specific CDB 0xC0 commands to the M487 USB MSC device.
 *
 * Usage:
 *   msc_debug.exe info
 *   msc_debug.exe readmem <addr> <len>
 *   msc_debug.exe writemem <addr> <hex-data>
 *   msc_debug.exe readreg <reg-id>
 *   msc_debug.exe writereg <reg-id> <value>
 *   msc_debug.exe log [count]
 *   msc_debug.exe echo <text>
 *   msc_debug.exe list
 *
 * Build:
 *   g++ -O2 -s -static -o msc_debug.exe msc_debug.cpp -lwinapi
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>

// ============================================================================
// MSC Vendor Debug Protocol
// ============================================================================
#define MSC_VENDOR_OPCODE   0xC0

// Sub-commands (CDB byte 1)
#define DBG_GET_INFO        0x07
#define DBG_READ_LOG        0x08
#define DBG_ECHO            0x0F
#define DBG_READ_MEM        0x01
#define DBG_WRITE_MEM       0x02
#define DBG_READ_REG        0x03
#define DBG_WRITE_REG       0x04

// MSC Device Info structure (packed, 32 bytes)
#pragma pack(push, 1)
struct MSC_DeviceInfo {
    uint32_t    u32ChipId;         // Chip ID
    uint16_t    u16FwVersion;      // FW version (BCD)
    uint16_t    u16BuildDate;      // Build date
    uint32_t    u32FlashSize;      // Flash size (bytes)
    uint32_t    u32RamSize;        // SRAM size (bytes)
    uint8_t     u8I2cSpeed;        // I2C speed (kHz)
    uint8_t     u8I2cAddr;        // Default I2C addr
    uint8_t     u8Reserved[2];
    uint32_t    u32LogHead;        // Log buffer head
    uint32_t    u32LogTail;        // Log buffer tail
    uint32_t    u32LogCount;       // Log entries count
};
#pragma pack(pop)

// Log entry structure (packed, 12 bytes)
#pragma pack(push, 1)
struct MSC_LogEntry {
    uint32_t    u32Timestamp;
    uint8_t     u8Level;
    uint8_t     u8Module;
    uint8_t     u8Code;
    uint8_t     u8Len;
    uint8_t     au8Data[4];
};
#pragma pack(pop)

// ============================================================================
// SCSI PassThrough Structures
// ============================================================================
#pragma pack(push, 1)
typedef struct {
    uint8_t     au8CDB[16];        // CDB (max 16 bytes)
    uint8_t     u8CDBLength;       // CDB length
    uint8_t     u8SenseLength;
    uint8_t     u8ScsiStatus;
    uint8_t     bResetSenseInfo;
    uint8_t     bQueueTagEnable;
    uint16_t    u16QueueTag;
    uint32_t    u32QueueAlgorithm;
    uint32_t    u32DataTransferLength;
    uint32_t    u32TimeOutValue;
    void       *pvDataPointer;
    uint32_t    u32SenseInfoOffset;
    uint8_t     u8Direction;       // 1 = IN, 2 = OUT, 0 = NONE
    uint8_t     u8DeviceAddress;
    uint16_t    u16BusAddress;
    uint32_t    u32BusType;
    uint32_t    u32SecurityProtocol;
    uint16_t    u16SecurityProtocolLength;
    uint32_t    u32AlignMask;
    uint8_t     ucToken;
    uint32_t    dwSystemIoSize;
    uint8_t     ucSystemIoArch;
    uint8_t     ucHashForToken;
    uint8_t     ucReserved38;
    uint8_t     ucReserved39;
} SCSI_PASS_THROUGH_DIRECT_BUFFER_V2, *PSCSI_PASS_THROUGH_DIRECT_BUFFER_V2;
#pragma pack(pop)

// IOCTL for SCSI PassThrough Direct
#ifndef IOCTL_SCSI_PASS_THROUGH_DIRECT
#define IOCTL_SCSI_PASS_THROUGH_DIRECT  0x4D014
#endif

// ============================================================================
// Global
// ============================================================================
static const char *g_pszProgName = "msc_debug";

// Device path (will auto-detect if empty)
static char g_szDevicePath[MAX_PATH] = {0};

// Detect MSC device (drive letter or PID match)
static bool FindMSCDevice(char *pszDriveLetter, size_t cbDrive)
{
    char szDrives[256] = {0};
    char szDevicePath[MAX_PATH] = {0};

    if (!GetLogicalDriveStringsA(sizeof(szDrives) - 1, szDrives)) {
        fprintf(stderr, "[ERROR] GetLogicalDriveStrings failed\n");
        return false;
    }

    char *pDrive = szDrives;
    while (*pDrive) {
        // Check removable drives only
        if (GetDriveTypeA(pDrive) == DRIVE_REMOVABLE) {
            char szDeviceName[MAX_PATH];
            snprintf(szDeviceName, sizeof(szDeviceName), "\\\\.\\%s", pDrive);
            szDeviceName[strlen(szDeviceName) - 1] = '\0'; // strip trailing backslash

            HANDLE hDevice = CreateFileA(szDeviceName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

            if (hDevice != INVALID_HANDLE_VALUE) {
                CloseHandle(hDevice);
                strncpy(pszDriveLetter, pDrive, cbDrive - 1);
                pszDriveLetter[cbDrive - 1] = '\0';
                // Remove trailing backslash
                size_t len = strlen(pszDriveLetter);
                if (len > 0 && pszDriveLetter[len - 1] == '\\')
                    pszDriveLetter[len - 1] = '\0';
                return true;
            }
        }
        pDrive += strlen(pDrive) + 1;
    }

    return false;
}

// Open MSC device handle
static HANDLE OpenMSCDevice(const char *pszDrive)
{
    char szDeviceName[MAX_PATH];
    snprintf(szDeviceName, sizeof(szDeviceName), "\\\\.\\%s", pszDrive);
    size_t len = strlen(szDeviceName);
    if (szDeviceName[len - 1] == '\\')
        szDeviceName[len - 1] = '\0';

    HANDLE h = CreateFileA(szDeviceName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[ERROR] Cannot open device %s (error=%lu)\n",
                szDeviceName, GetLastError());
        return INVALID_HANDLE_VALUE;
    }
    return h;
}

// Send SCSI vendor command
static bool SendVendorCommand(HANDLE hDevice, uint8_t u8SubCmd,
                               uint32_t u32Addr, uint32_t u32Len,
                               uint8_t *pDataIn, uint32_t cbDataIn,
                               uint8_t *pDataOut, uint32_t cbDataOut,
                               uint8_t *pu8ScsiStatus)
{
    uint8_t cdb[16] = {0};
    cdb[0] = MSC_VENDOR_OPCODE;  // Vendor opcode
    cdb[1] = u8SubCmd;
    cdb[2] = (uint8_t)((u32Addr >> 16) & 0xFF);
    cdb[3] = (uint8_t)((u32Addr >> 8) & 0xFF);
    cdb[4] = (uint8_t)(u32Addr & 0xFF);
    cdb[5] = (uint8_t)(u32Len & 0xFF);

    // Build pass-through structure
    uint8_t buffer[256 + sizeof(SCSI_PASS_THROUGH_DIRECT_BUFFER_V2)];
    memset(buffer, 0, sizeof(buffer));

    SCSI_PASS_THROUGH_DIRECT_BUFFER_V2 *pPT = (SCSI_PASS_THROUGH_DIRECT_BUFFER_V2 *)buffer;

    memcpy(pPT->au8CDB, cdb, 16);
    pPT->u8CDBLength = 6;
    pPT->u8SenseLength = 32;
    pPT->u32DataTransferLength = (cbDataIn > 0) ? cbDataIn : cbDataOut;
    pPT->u32TimeOutValue = 3000; // 3s timeout
    pPT->pvDataPointer = (cbDataIn > 0) ? pDataIn : pDataOut;
    pPT->u32SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT_BUFFER_V2);
    pPT->u8Direction = (cbDataIn > 0) ? 2 : 1;  // 2=OUT, 1=IN

    DWORD dwBytesReturned = 0;
    BOOL bResult = DeviceIoControl(hDevice,
        IOCTL_SCSI_PASS_THROUGH_DIRECT,
        pPT, sizeof(SCSI_PASS_THROUGH_DIRECT_BUFFER_V2),
        pPT, sizeof(buffer),
        &dwBytesReturned, NULL);

    if (!bResult) {
        fprintf(stderr, "[ERROR] DeviceIoControl failed (error=%lu)\n", GetLastError());
        return false;
    }

    if (pu8ScsiStatus)
        *pu8ScsiStatus = pPT->u8ScsiStatus;

    return true;
}

// ============================================================================
// Commands
// ============================================================================

// msc_debug.exe list
static int cmd_list(void)
{
    char szDrives[256] = {0};
    printf("Removable drives found:\n");

    if (!GetLogicalDriveStringsA(sizeof(szDrives) - 1, szDrives)) {
        fprintf(stderr, "[ERROR] GetLogicalDriveStrings failed\n");
        return 1;
    }

    char *pDrive = szDrives;
    bool bAny = false;
    while (*pDrive) {
        if (GetDriveTypeA(pDrive) == DRIVE_REMOVABLE) {
            printf("  %s (removable)\n", pDrive);
            bAny = true;
        }
        pDrive += strlen(pDrive) + 1;
    }

    if (!bAny) {
        printf("  (none)\n");
    }

    return 0;
}

// msc_debug.exe info
static int cmd_info(HANDLE h)
{
    uint8_t dataOut[sizeof(MSC_DeviceInfo)] = {0};
    uint8_t status = 0;

    bool ok = SendVendorCommand(h, DBG_GET_INFO, 0, 0, NULL, 0, dataOut, sizeof(MSC_DeviceInfo), &status);
    if (!ok) return 1;

    if (status != 0) {
        fprintf(stderr, "[WARN] SCSI status = 0x%02X\n", status);
    }

    MSC_DeviceInfo *pInfo = (MSC_DeviceInfo *)dataOut;
    printf("=== M487 Device Info ===\n");
    printf("  Chip ID    : 0x%08lX\n", (unsigned long)pInfo->u32ChipId);
    printf("  FW Version : %d.%02d (0x%04X)\n",
           (pInfo->u16FwVersion >> 8) & 0xFF, pInfo->u16FwVersion & 0xFF,
           pInfo->u16FwVersion);
    printf("  Build Date : 0x%04X\n", pInfo->u16BuildDate);
    printf("  Flash Size : %lu KB\n", (unsigned long)pInfo->u32FlashSize / 1024);
    printf("  SRAM Size  : %lu KB\n", (unsigned long)pInfo->u32RamSize / 1024);
    printf("  I2C Speed  : %d kHz\n", pInfo->u8I2cSpeed);
    printf("  I2C Addr   : 0x%02X\n", pInfo->u8I2cAddr);
    printf("  Log Head   : %lu\n", (unsigned long)pInfo->u32LogHead);
    printf("  Log Tail   : %lu\n", (unsigned long)pInfo->u32LogTail);
    printf("  Log Count  : %lu\n", (unsigned long)pInfo->u32LogCount);

    return 0;
}

// msc_debug.exe log [count]
static int cmd_log(HANDLE h, int count)
{
    if (count <= 0) count = 64;
    if (count > 256) count = 256;

    uint8_t *pBuf = (uint8_t *)malloc(count * sizeof(MSC_LogEntry));
    if (!pBuf) {
        fprintf(stderr, "[ERROR] Out of memory\n");
        return 1;
    }

    uint8_t status = 0;
    bool ok = SendVendorCommand(h, DBG_READ_LOG, 0, count * sizeof(MSC_LogEntry),
                                NULL, 0, pBuf, count * sizeof(MSC_LogEntry), &status);
    if (!ok) {
        free(pBuf);
        return 1;
    }

    const char *apszLevel[] = {"ERR", "WARN", "INFO", "DBG", "TRC"};
    const char *apszModule[] = {"MAIN", "USB", "MSC", "I2C", "HID"};

    int nEntries = (count * sizeof(MSC_LogEntry)) / sizeof(MSC_LogEntry);
    printf("=== Debug Log (%d entries) ===\n", nEntries);

    MSC_LogEntry *pEntry = (MSC_LogEntry *)pBuf;
    for (int i = 0; i < nEntries; i++) {
        const char *pszLevel = (pEntry[i].u8Level < 5) ? apszLevel[pEntry[i].u8Level] : "???";
        const char *pszModule = (pEntry[i].u8Module < 5) ? apszModule[pEntry[i].u8Module] : "???";
        printf("  [%s][%s][0x%02X] TS=0x%08lX",
               pszLevel, pszModule, pEntry[i].u8Code,
               (unsigned long)pEntry[i].u32Timestamp);
        if (pEntry[i].u8Len > 0) {
            printf(" DATA=");
            for (int j = 0; j < pEntry[i].u8Len && j < 4; j++)
                printf("%02X ", pEntry[i].au8Data[j]);
        }
        printf("\n");
    }

    free(pBuf);
    return 0;
}

// msc_debug.exe echo <text>
static int cmd_echo(HANDLE h, const char *pszText)
{
    uint8_t buf[64] = {0};
    size_t cbText = strlen(pszText);
    if (cbText > 63) cbText = 63;
    memcpy(buf, pszText, cbText);

    uint8_t resp[64] = {0};
    uint8_t status = 0;

    bool ok = SendVendorCommand(h, DBG_ECHO, 0, (uint32_t)cbText,
                                buf, (uint32_t)cbText, resp, sizeof(resp), &status);
    if (!ok) return 1;

    // Print echoed data as text
    printf("Echo back: %.*s\n", (int)cbText, resp);
    return 0;
}

// msc_debug.exe readmem <addr> <len>
static int cmd_readmem(HANDLE h, uint32_t u32Addr, uint32_t u32Len)
{
    if (u32Len == 0) u32Len = 16;
    if (u32Len > 512) u32Len = 512;

    uint8_t *pBuf = (uint8_t *)malloc(u32Len);
    if (!pBuf) {
        fprintf(stderr, "[ERROR] Out of memory\n");
        return 1;
    }

    uint8_t status = 0;
    bool ok = SendVendorCommand(h, DBG_READ_MEM, u32Addr, u32Len,
                                NULL, 0, pBuf, u32Len, &status);
    if (!ok) {
        free(pBuf);
        return 1;
    }

    printf("Memory @ 0x%08lX (%lu bytes):\n", (unsigned long)u32Addr, (unsigned long)u32Len);

    // Hex dump
    for (uint32_t i = 0; i < u32Len; i += 16) {
        printf("  %08lX: ", (unsigned long)(u32Addr + i));
        // Hex
        for (int j = 0; j < 16; j++) {
            if (i + j < u32Len)
                printf("%02X ", pBuf[i + j]);
            else
                printf("   ");
        }
        printf(" ");
        // ASCII
        for (int j = 0; j < 16 && i + j < u32Len; j++) {
            uint8_t c = pBuf[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("\n");
    }

    free(pBuf);
    return 0;
}

// msc_debug.exe writemem <addr> <hex-data>
static int cmd_writemem(HANDLE h, uint32_t u32Addr, const uint8_t *pData, uint32_t cbData)
{
    uint8_t *pBuf = (uint8_t *)malloc(cbData);
    if (!pBuf) {
        fprintf(stderr, "[ERROR] Out of memory\n");
        return 1;
    }
    memcpy(pBuf, pData, cbData);

    uint8_t resp[4] = {0};
    uint8_t status = 0;

    bool ok = SendVendorCommand(h, DBG_WRITE_MEM, u32Addr, cbData,
                                pBuf, cbData, resp, sizeof(resp), &status);
    if (!ok) {
        free(pBuf);
        return 1;
    }

    uint32_t u32Written = (uint32_t)resp[0] | ((uint32_t)resp[1] << 8) |
                          ((uint32_t)resp[2] << 16) | ((uint32_t)resp[3] << 24);
    printf("Write memory @ 0x%08lX: %lu bytes written\n", (unsigned long)u32Addr, (unsigned long)u32Written);
    free(pBuf);
    return 0;
}

// msc_debug.exe readreg <reg-id>
static int cmd_readreg(HANDLE h, uint8_t u8RegId)
{
    uint8_t resp[4] = {0};
    uint8_t status = 0;

    bool ok = SendVendorCommand(h, DBG_READ_REG, u8RegId, 0,
                                NULL, 0, resp, 4, &status);
    if (!ok) return 1;

    uint32_t u32Val = (uint32_t)resp[0] | ((uint32_t)resp[1] << 8) |
                      ((uint32_t)resp[2] << 16) | ((uint32_t)resp[3] << 24);

    const char *apszRegs[] = {
        "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10",
        "R11", "R12", "SP", "LR", "PC", "xPSR", "MSP", "PSP",
        "???", "PRIMASK", "CONTROL", "BASEPRI"
    };
    const char *pszReg = (u8RegId < 23) ? apszRegs[u8RegId] : "???";

    printf("%s (reg %d) = 0x%08lX\n", pszReg, u8RegId, (unsigned long)u32Val);
    return 0;
}

// msc_debug.exe writereg <reg-id> <value>
static int cmd_writereg(HANDLE h, uint8_t u8RegId, uint32_t u32Val)
{
    uint8_t buf[4] = {
        (uint8_t)(u32Val & 0xFF),
        (uint8_t)((u32Val >> 8) & 0xFF),
        (uint8_t)((u32Val >> 16) & 0xFF),
        (uint8_t)((u32Val >> 24) & 0xFF)
    };
    uint8_t resp[1] = {0};
    uint8_t status = 0;

    bool ok = SendVendorCommand(h, DBG_WRITE_REG, u8RegId, 0,
                                buf, 4, resp, 1, &status);
    if (!ok) return 1;

    printf("Write %s (reg %d) = 0x%08lX -> %s\n",
           (u8RegId < 23) ? (const char*[]){"R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","R10","R11","R12","SP","LR","PC","xPSR","MSP","PSP","???","PRIMASK","CONTROL","BASEPRI"}[u8RegId] : "???",
           u8RegId, (unsigned long)u32Val,
           (resp[0] == 0) ? "OK" : "FAILED");
    return 0;
}

// ============================================================================
// Hex string parser
// ============================================================================
static bool ParseHexString(const char *pszHex, uint8_t *pBuf, size_t cbBuf, size_t *pcbOut)
{
    size_t cbHex = strlen(pszHex);
    size_t cbOut = 0;

    // Skip leading "0x" or "0X"
    if (cbHex >= 2 && pszHex[0] == '0' && (pszHex[1] == 'x' || pszHex[1] == 'X')) {
        pszHex += 2;
        cbHex -= 2;
    }

    if (cbHex == 0) {
        *pcbOut = 0;
        return true;
    }

    // Pad with leading zero if odd length
    if (cbHex % 2 != 0) {
        cbHex++;
    }

    cbOut = cbHex / 2;
    if (cbOut > cbBuf) cbOut = cbBuf;

    memset(pBuf, 0, cbOut);

    for (size_t i = 0; i < cbOut * 2; i++) {
        char c = pszHex[i];
        uint8_t nibble;
        if (c >= '0' && c <= '9') nibble = c - '0';
        else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
        else {
            fprintf(stderr, "[ERROR] Invalid hex character '%c'\n", c);
            return false;
        }
        if ((i & 1) == 0)
            pBuf[i / 2] = nibble << 4;
        else
            pBuf[i / 2] |= nibble;
    }

    *pcbOut = cbOut;
    return true;
}

// ============================================================================
// Usage
// ============================================================================
static void usage(void)
{
    printf("Usage:\n");
    printf("  %s list\n", g_pszProgName);
    printf("  %s info\n", g_pszProgName);
    printf("  %s log [count]\n", g_pszProgName);
    printf("  %s echo <text>\n", g_pszProgName);
    printf("  %s readmem <addr> <len>\n", g_pszProgName);
    printf("  %s writemem <addr> <hex-data>\n", g_pszProgName);
    printf("  %s readreg <reg-id>\n", g_pszProgName);
    printf("  %s writereg <reg-id> <value>\n", g_pszProgName);
    printf("\n");
    printf("Examples:\n");
    printf("  %s info\n", g_pszProgName);
    printf("  %s log 32\n", g_pszProgName);
    printf("  %s readmem 0x20000000 64\n", g_pszProgName);
    printf("  %s writemem 0x20000000 0x01020304\n", g_pszProgName);
    printf("  %s readreg 15\n", g_pszProgName);
    printf("  %s echo Hello\n", g_pszProgName);
    printf("\n");
    printf("Auto-detects MSC device drive letter (removable drive).\n");
    printf("Device: VID=0x04F3 PID=0x0732\n");
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage();
        return 0;
    }

    const char *pszCmd = argv[1];

    // Find MSC device automatically
    char szDrive[8] = {0};
    if (FindMSCDevice(szDrive, sizeof(szDrive))) {
        printf("[INFO] Auto-detected MSC device: %s\n", szDrive);
    } else {
        fprintf(stderr, "[ERROR] No MSC device found. Is M487 connected?\n");
        return 1;
    }

    HANDLE hDevice = OpenMSCDevice(szDrive);
    if (hDevice == INVALID_HANDLE_VALUE)
        return 1;

    int nResult = 0;

    if (strcmp(pszCmd, "list") == 0) {
        nResult = cmd_list();
    }
    else if (strcmp(pszCmd, "info") == 0) {
        nResult = cmd_info(hDevice);
    }
    else if (strcmp(pszCmd, "log") == 0) {
        int count = (argc > 2) ? atoi(argv[2]) : 64;
        nResult = cmd_log(hDevice, count);
    }
    else if (strcmp(pszCmd, "echo") == 0) {
        if (argc < 3) {
            fprintf(stderr, "[ERROR] echo requires <text> argument\n");
            nResult = 1;
        } else {
            nResult = cmd_echo(hDevice, argv[2]);
        }
    }
    else if (strcmp(pszCmd, "readmem") == 0) {
        if (argc < 4) {
            fprintf(stderr, "[ERROR] readmem requires <addr> and <len>\n");
            nResult = 1;
        } else {
            uint32_t addr = strtoul(argv[2], NULL, 0);
            uint32_t len = strtoul(argv[3], NULL, 0);
            nResult = cmd_readmem(hDevice, addr, len);
        }
    }
    else if (strcmp(pszCmd, "writemem") == 0) {
        if (argc < 4) {
            fprintf(stderr, "[ERROR] writemem requires <addr> and <hex-data>\n");
            nResult = 1;
        } else {
            uint32_t addr = strtoul(argv[2], NULL, 0);
            uint8_t buf[256] = {0};
            size_t cb = 0;
            if (ParseHexString(argv[3], buf, sizeof(buf), &cb))
                nResult = cmd_writemem(hDevice, addr, buf, (uint32_t)cb);
            else
                nResult = 1;
        }
    }
    else if (strcmp(pszCmd, "readreg") == 0) {
        if (argc < 3) {
            fprintf(stderr, "[ERROR] readreg requires <reg-id>\n");
            nResult = 1;
        } else {
            uint8_t reg = (uint8_t)atoi(argv[2]);
            nResult = cmd_readreg(hDevice, reg);
        }
    }
    else if (strcmp(pszCmd, "writereg") == 0) {
        if (argc < 4) {
            fprintf(stderr, "[ERROR] writereg requires <reg-id> and <value>\n");
            nResult = 1;
        } else {
            uint8_t reg = (uint8_t)atoi(argv[2]);
            uint32_t val = strtoul(argv[3], NULL, 0);
            nResult = cmd_writereg(hDevice, reg, val);
        }
    }
    else {
        fprintf(stderr, "[ERROR] Unknown command: %s\n", pszCmd);
        usage();
        nResult = 1;
    }

    CloseHandle(hDevice);
    return nResult;
}
