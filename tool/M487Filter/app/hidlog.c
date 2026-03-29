/*
 * hidlog.c
 *
 * User-mode CLI tool for reading and displaying captured HID packets
 * from the M487Filter ring buffer.
 *
 * Features:
 *   - Hexdump format output
 *   - Filter by report type (SET_FEATURE, GET_FEATURE, WRITE_REPORT, READ_REPORT)
 *   - Continuous tail mode
 *   - Statistics summary
 *
 * Build:
 *   cl /EHsc /W4 hidlog.c uuid.lib ole32.lib oleaut32.lib
 *
 * Usage:
 *   hidlog.exe [options]
 *
 * Options:
 *   -f, --follow        Continuous tail mode (like tail -f)
 *   -n, --lines N       Show last N entries (default: 16)
 *   --filter TYPE       Filter by type: set-feature, get-feature,
 *                       write-report, read-report, all (default: all)
 *   --no-color          Disable ANSI color output
 *   -q, --quiet         Quiet mode (only hexdump output)
 *   -s, --stats         Show ring buffer statistics only
 *   -h, --help          Show this help
 *
 * Examples:
 *   hidlog.exe                           # Show last 16 entries
 *   hidlog.exe -n 100                    # Show last 100 entries
 *   hidlog.exe --filter set-feature      # Only show SET_FEATURE packets
 *   hidlog.exe -f                        # Follow new entries continuously
 *   hidlog.exe -f --filter write-report  # Follow WRITE_REPORT only
 *   hidlog.exe -s                        # Show statistics only
 *
 * Environment:
 *   Windows user-mode
 *
 * Note:
 *   Requires the M487Filter driver to be installed and the device to be
 *   present. Uses IOCTL_M487FILTER_GET_RING_ADDR to get the ring buffer
 *   virtual address directly from the driver.
 *
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#pragma comment(lib, "advapi32.lib")

//
// IOCTL codes from the driver (must match M487FilterIoctl.h)
//
#define FILE_DEVICE_HID                     0x0000ff12
#define IOCTL_HID_SET_FEATURE               CTL_CODE(FILE_DEVICE_HID, 0x00b, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_HID_GET_FEATURE               CTL_CODE(FILE_DEVICE_HID, 0x00c, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_HID_WRITE_REPORT              CTL_CODE(FILE_DEVICE_HID, 0x00d, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_HID_READ_REPORT               CTL_CODE(FILE_DEVICE_HID, 0x00e, METHOD_NEITHER, FILE_ANY_ACCESS)

#define FILE_DEVICE_M487FILTER              0x00009420
#define IOCTL_M487FILTER_BASE               0x9000

// IOCTL_M487FILTER_GET_INFO = BASE + 0 = 0x9000
#define IOCTL_M487FILTER_GET_INFO           CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL_M487FILTER_SET_MODE = BASE + 1 = 0x9001
#define IOCTL_M487FILTER_SET_MODE           CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL_M487FILTER_SEND_FEATURE = BASE + 2 = 0x9002
#define IOCTL_M487FILTER_SEND_FEATURE      CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL_M487FILTER_GET_FEATURE = BASE + 3 = 0x9003
#define IOCTL_M487FILTER_GET_FEATURE        CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL_M487FILTER_GET_RING_STATS = BASE + 4 = 0x9004
#define IOCTL_M487FILTER_GET_RING_STATS     CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL_M487FILTER_GET_RING_ADDR = BASE + 5 = 0x9005
#define IOCTL_M487FILTER_GET_RING_ADDR      CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 5, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL_M487FILTER_SET_INTERCEPT_CONFIG = BASE + 6 = 0x9006
#define IOCTL_M487FILTER_SET_INTERCEPT_CONFIG CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL_M487FILTER_GET_INTERCEPT_CONFIG = BASE + 7 = 0x9007
#define IOCTL_M487FILTER_GET_INTERCEPT_CONFIG CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL_M487FILTER_MAP_RING_BUFFER = BASE + 8 = 0x9008
#define IOCTL_M487FILTER_MAP_RING_BUFFER   CTL_CODE(FILE_DEVICE_M487FILTER, IOCTL_M487FILTER_BASE + 8, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Ring buffer mapping info returned by IOCTL_M487FILTER_MAP_RING_BUFFER
//
typedef struct _M487_FILTER_RING_MAPPING_INFO
{
    ULONG64 RingBufferUserVa;  // User-mode virtual address
    ULONG RingBufferSize;      // Total size in bytes
    ULONG SlotSize;           // Size of each slot
    ULONG SlotCount;          // Number of slots
    USHORT MaxDataSize;       // Max data per slot
    USHORT Reserved;

} M487_FILTER_RING_MAPPING_INFO, *PM487_FILTER_RING_MAPPING_INFO;

//
// Ring buffer constants (must match M487FilterRingBuffer.h)
//
#define M487_RING_SLOT_COUNT     256
#define M487_MAX_REPORT_SIZE     64
#define M487_RING_MAGIC          0x4D343837  // 'M487'

//
// IOCTL types (must match M487FilterRingBuffer.h)
//
typedef enum _M487_IOCTL_TYPE
{
    IoctlType_Unknown = 0,
    IoctlType_HID_SET_FEATURE,
    IoctlType_HID_GET_FEATURE,
    IoctlType_HID_WRITE_REPORT,
    IoctlType_HID_READ_REPORT,
    IoctlType_HID_GET_COLLECTION_DESCRIPTOR,
    IoctlType_HID_GET_COLLECTION_INFORMATION,
    IoctlType_HID_GET_HARDWARE_ID,
    IoctlType_HID_GETManufacturerString,
    IoctlType_HID_GETProductString,
    IoctlType_HID_GETSerialNumberString,
    IoctlType_HID_GETIndexedString,
    IoctlType_HID_GET_MS_GENRE_DESCRIPTOR

} M487_IOCTL_TYPE;

//
// Ring buffer entry header (must match driver)
//
typedef struct _M487_RING_ENTRY
{
    LARGE_INTEGER Timestamp;
    M487_IOCTL_TYPE IoctlType;
    ULONG IoctlCode;
    BOOLEAN IsWrite;
    USHORT DataLength;
    UCHAR ReportId;
    UCHAR Reserved[2];
    UCHAR Data[1];

} M487_RING_ENTRY, *PM487_RING_ENTRY;

//
// Ring buffer control block (must match driver)
//
typedef struct _M487_RING_CONTROL
{
    volatile ULONG RingByteSize;
    volatile ULONG WriteIndex;
    volatile ULONG ReadIndex;
    volatile ULONG64 TotalCaptured;
    volatile ULONG DroppedCount;
    volatile ULONG Magic;
    UCHAR Reserved[4];
    UCHAR DataStart[1];

} M487_RING_CONTROL, *PM487_RING_CONTROL;

//
// Ring buffer info returned by IOCTL
//
typedef struct _M487_RING_ADDR_INFO
{
    ULONG64 RingBufferVa;     // Virtual address in user-mode
    ULONG64 RingBufferSize;  // Total size in bytes
    ULONG SlotSize;          // Size of each slot
    ULONG SlotCount;         // Number of slots
    USHORT MaxDataSize;      // Max data per slot
    USHORT Reserved;

} M487_RING_ADDR_INFO, *PM487_RING_ADDR_INFO;

//
// Ring buffer stats returned by IOCTL
//
typedef struct _M487_RING_STATS
{
    ULONG64 TotalCaptured;
    ULONG DroppedCount;
    ULONG CurrentUsedSlots;
    ULONG TotalSlots;
    ULONG SlotSize;
    BOOLEAN RingInitialized;

} M487_RING_STATS, *PM487_RING_STATS;

//
// Intercept config (must match driver)
//
typedef struct _M487_INTERCEPT_CONFIG
{
    BOOLEAN CaptureSetFeature;
    BOOLEAN CaptureGetFeature;
    BOOLEAN CaptureWriteReport;
    BOOLEAN CaptureReadReport;
    BOOLEAN CaptureStringRequests;
    BOOLEAN CaptureCollectionInfo;
    ULONG Reserved[3];

} M487_INTERCEPT_CONFIG;

//
// Options
//
typedef struct _OPTIONS
{
    int lines;
    BOOL follow;
    BOOL quiet;
    BOOL statsOnly;
    BOOL noColor;
    BOOL filterAll;
    BOOL filterSetFeature;
    BOOL filterGetFeature;
    BOOL filterWriteReport;
    BOOL filterReadReport;
    WCHAR* devicePath;

} OPTIONS;

//
// Globals
//
static OPTIONS g_opt;
static HANDLE g_hDevice = INVALID_HANDLE_VALUE;
static M487_RING_CONTROL* g_pRing = NULL;
static M487_RING_ADDR_INFO g_ringInfo;
static int g_entryIndex = 0;

//
// ANSI color codes (narrow strings, use %hs in fwprintf)
// All fwprintf with colors uses %hs for these.
//
static const char* COLOR_RESET    = "";
static const char* COLOR_GREEN    = "";
static const char* COLOR_YELLOW   = "";
static const char* COLOR_CYAN    = "";
static const char* COLOR_MAGENTA  = "";
static const char* COLOR_DIM      = "";

static void InitColor(VOID)
{
    if (g_opt.noColor) return;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(hOut, &mode))
        {
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            COLOR_GREEN    = "\x1b[32m";
            COLOR_YELLOW   = "\x1b[33m";
            COLOR_CYAN     = "\x1b[36m";
            COLOR_MAGENTA  = "\x1b[35m";
            COLOR_DIM      = "\x1b[2m";
            COLOR_RESET    = "\x1b[0m";
        }
    }
}

static void PrintHelp(VOID)
{
    printf(
        "hidlog - M487Filter HID packet log viewer\n"
        "\n"
        "Usage: hidlog [options]\n"
        "\n"
        "Options:\n"
        "  -f, --follow          Continuous tail mode (Ctrl+C to stop)\n"
        "  -n, --lines N         Show last N entries (default: 16, max: %d)\n"
        "  --filter TYPE         Filter by report type:\n"
        "                          set-feature  - IOCTL_HID_SET_FEATURE\n"
        "                          get-feature  - IOCTL_HID_GET_FEATURE\n"
        "                          write-report - IOCTL_HID_WRITE_REPORT\n"
        "                          read-report  - IOCTL_HID_READ_REPORT\n"
        "                          all          - All types (default)\n"
        "  --no-color            Disable ANSI color output\n"
        "  -q, --quiet           Quiet mode (only hexdump output)\n"
        "  -s, --stats           Show ring buffer statistics only\n"
        "  -d, --device PATH     Device path (default: first found M487Filter device)\n"
        "  -h, --help            Show this help\n"
        "\n"
        "Examples:\n"
        "  hidlog                           # Show last 16 entries\n"
        "  hidlog -n 100                    # Show last 100 entries\n"
        "  hidlog --filter set-feature      # Only SET_FEATURE packets\n"
        "  hidlog -f                        # Follow continuously\n"
        "  hidlog -f --filter write-report  # Follow WRITE_REPORT only\n"
        "  hidlog -s                        # Statistics only\n"
        "\n",
        M487_RING_SLOT_COUNT
    );
}

static WCHAR* IoctlTypeToString(M487_IOCTL_TYPE type)
{
    switch (type) {
        case IoctlType_HID_SET_FEATURE:            return L"SET_FEATURE";
        case IoctlType_HID_GET_FEATURE:            return L"GET_FEATURE";
        case IoctlType_HID_WRITE_REPORT:           return L"WRITE_REPORT";
        case IoctlType_HID_READ_REPORT:            return L"READ_REPORT";
        case IoctlType_HID_GET_COLLECTION_DESCRIPTOR: return L"GET_COLLECTION_DESCRIPTOR";
        case IoctlType_HID_GET_COLLECTION_INFORMATION: return L"GET_COLLECTION_INFO";
        case IoctlType_HID_GET_HARDWARE_ID:        return L"GET_HARDWARE_ID";
        case IoctlType_HID_GETManufacturerString:   return L"GET_MANUFACTURER";
        case IoctlType_HID_GETProductString:       return L"GET_PRODUCT";
        case IoctlType_HID_GETSerialNumberString:  return L"GET_SERIAL";
        case IoctlType_HID_GETIndexedString:       return L"GET_INDEXED_STRING";
        case IoctlType_HID_GET_MS_GENRE_DESCRIPTOR: return L"GET_MS_GENRE";
        default:                                   return L"UNKNOWN";
    }
}

static WCHAR* IoctlTypeToShortString(M487_IOCTL_TYPE type)
{
    switch (type) {
        case IoctlType_HID_SET_FEATURE:    return L"SETF";
        case IoctlType_HID_GET_FEATURE:    return L"GETF";
        case IoctlType_HID_WRITE_REPORT:   return L"WREP";
        case IoctlType_HID_READ_REPORT:    return L"RREP";
        default:                           return L"????";
    }
}

static const char* IoctlTypeToColor(M487_IOCTL_TYPE type)
{
    switch (type) {
        case IoctlType_HID_SET_FEATURE:    return COLOR_GREEN;
        case IoctlType_HID_GET_FEATURE:     return COLOR_YELLOW;
        case IoctlType_HID_WRITE_REPORT:    return COLOR_CYAN;
        case IoctlType_HID_READ_REPORT:     return COLOR_MAGENTA;
        default:                             return COLOR_DIM;
    }
}

static BOOL ShouldShowEntry(M487_IOCTL_TYPE type)
{
    if (g_opt.filterAll) return TRUE;

    switch (type) {
        case IoctlType_HID_SET_FEATURE:
            return g_opt.filterSetFeature;
        case IoctlType_HID_GET_FEATURE:
            return g_opt.filterGetFeature;
        case IoctlType_HID_WRITE_REPORT:
            return g_opt.filterWriteReport;
        case IoctlType_HID_READ_REPORT:
            return g_opt.filterReadReport;
        default:
            return FALSE;
    }
}

static void TimestampToString(LARGE_INTEGER* ts, WCHAR* buf, size_t bufsize)
{
    LARGE_INTEGER local;
    FILETIME ft;

    // Windows epoch 1601-01-01 to Unix epoch 1970-01-01 = 369 years = 11644473600 seconds
    // In 100-nanosecond intervals: 11644473600 * 10000000 = 116444736000000000
    static const ULONGLONG EPOCH_OFFSET = 116444736000000000ULL;

    local.QuadPart = ts->QuadPart - (LONGLONG)EPOCH_OFFSET;

    // Get as Unix timestamp (seconds)
    ULONGLONG unixSecs = local.QuadPart / 10000000ULL;

    // Convert to FILETIME
    ft.dwLowDateTime = (DWORD)(unixSecs * 10000000ULL);
    ft.dwHighDateTime = (DWORD)((unixSecs * 10000000ULL) >> 32);

    // Convert to local time
    SYSTEMTIME st;
    FileTimeToLocalFileTime(&ft, &ft);
    FileTimeToSystemTime(&ft, &st);

    // Format timestamp with milliseconds
    swprintf(buf, bufsize,
             L"%04d-%02d-%02d %02d:%02d:%02d.%03d",
             (int)st.wYear, (int)st.wMonth, (int)st.wDay,
             (int)st.wHour, (int)st.wMinute, (int)st.wSecond,
             (int)((local.QuadPart % 10000000ULL) / 10000ULL));
}

static void HexdumpLine(const UCHAR* data, USHORT length, USHORT lineOffset, FILE* out)
{
    int i;
    WCHAR hexBuf[128];
    WCHAR asciiBuf[17];
    int pos = 0;

    // Build hex part manually
    pos = 0;
    for (i = 0; i < 16; i++) {
        if (i < length) {
            pos += swprintf(hexBuf + pos, 128 - pos, L"%02X ", data[lineOffset + i]);
        } else {
            pos += swprintf(hexBuf + pos, 128 - pos, L"   ");
        }
        if (i == 7) {
            hexBuf[pos++] = L' ';
        }
    }
    hexBuf[pos] = L'\0';

    // Build ASCII part
    for (i = 0; i < 16 && (lineOffset + i) < length; i++) {
        UCHAR c = data[lineOffset + i];
        asciiBuf[i] = (c >= 0x20 && c <= 0x7e) ? (WCHAR)c : L'.';
    }
    asciiBuf[i] = L'\0';

    if (!g_opt.quiet) {
        fwprintf(out, L"  %04X: %-51ws %-16ws\n", lineOffset, hexBuf, asciiBuf);
    } else {
        fwprintf(out, L"%04X: %-51ws %-16ws\n", lineOffset, hexBuf, asciiBuf);
    }
}

static void PrintEntry(PM487_RING_ENTRY entry, FILE* out)
{
    WCHAR tsBuf[64];
    const char* color = IoctlTypeToColor(entry->IoctlType);
    WCHAR* shortStr = IoctlTypeToShortString(entry->IoctlType);
    USHORT dlen = entry->DataLength;
    UCHAR* data = (UCHAR*)entry->Data;

    TimestampToString(&entry->Timestamp, tsBuf, _countof(tsBuf));

    if (!g_opt.quiet) {
        if (!g_opt.noColor && color && *color) {
            fwprintf(out, L"\n%hs[%4d]%hs ", COLOR_DIM, g_entryIndex, COLOR_RESET);
            fwprintf(out, L"%hs%ws %hs%hs[%ws]%hs",
                    color, tsBuf, COLOR_RESET,
                    color, shortStr, COLOR_RESET);
        } else {
            fwprintf(out, L"\n[%4d] ", g_entryIndex);
            fwprintf(out, L"%ws %ws", tsBuf, shortStr);
        }

        fwprintf(out, L" IsWrite=%ws Len=%hu ReportId=0x%02X IOCTL=0x%08X",
                entry->IsWrite ? L"T" : L"F",
                dlen,
                entry->ReportId,
                entry->IoctlCode);

        if (dlen == 0 || data == NULL) {
            fwprintf(out, L"\n  (no data)");
        } else {
            USHORT offset = 0;
            while (offset < dlen) {
                HexdumpLine(data, dlen, offset, out);
                offset += 16;
            }
        }
    } else {
        // Quiet mode: compact hexdump
        fwprintf(out, L"[%4d] %ws %ws IsWrite=%ws Len=%hu ReportId=0x%02X: ",
                g_entryIndex, tsBuf, shortStr,
                entry->IsWrite ? L"T" : L"F",
                dlen,
                entry->ReportId);

        for (USHORT i = 0; i < dlen && i < M487_MAX_REPORT_SIZE; i++) {
            fwprintf(out, L"%02X", data[i]);
        }
        fwprintf(out, L"\n");
    }
}

static void PrintHeader(FILE* out)
{
    if (g_opt.quiet) return;

    fwprintf(out, L"\n");
    fwprintf(out, L"%hs========================================"
            L"==========================\n", COLOR_CYAN);
    fwprintf(out, L"%hs  hidlog - M487Filter HID Packet Log"
            L"                              \n", COLOR_RESET);
    fwprintf(out, L"%hs========================================"
            L"==========================\n", COLOR_CYAN);

    fwprintf(out, L"  Ring Buffer: VA=0x%016llX  Size=%llu bytes\n",
            (ULONG64)g_ringInfo.RingBufferVa,
            (ULONG64)g_ringInfo.RingBufferSize);
    fwprintf(out, L"  Slots: %lu x %lu bytes  MaxData: %hu bytes\n",
            g_ringInfo.SlotCount,
            (ULONG)g_ringInfo.SlotSize,
            g_ringInfo.MaxDataSize);
    fwprintf(out, L"  Filter: %hs%hs%hs%hs%hs\n",
            g_opt.filterAll ? "ALL" : "",
            g_opt.filterSetFeature ? "SET_FEATURE " : "",
            g_opt.filterGetFeature ? "GET_FEATURE " : "",
            g_opt.filterWriteReport ? "WRITE_REPORT " : "",
            g_opt.filterReadReport ? "READ_REPORT " : "");
    fwprintf(out, L"%hs========================================"
            L"==========================\n", COLOR_CYAN);
}

static void PrintStats(FILE* out)
{
    M487_RING_STATS stats;
    DWORD bytesReturned = 0;

    if (!DeviceIoControl(g_hDevice,
                        IOCTL_M487FILTER_GET_RING_STATS,
                        NULL, 0,
                        &stats, sizeof(stats),
                        &bytesReturned, NULL)) {
        fwprintf(stderr, L"[ERROR] Failed to get ring stats: %lu\n", GetLastError());
        return;
    }

    fwprintf(out, L"\n");
    fwprintf(out, L"%hs========================================\n", COLOR_CYAN);
    fwprintf(out, L"%hs       M487Filter Ring Buffer Stats      \n", COLOR_RESET);
    fwprintf(out, L"%hs========================================\n", COLOR_CYAN);
    fwprintf(out, L"  Ring Initialized : %ws\n", stats.RingInitialized ? L"YES" : L"NO");
    fwprintf(out, L"  Total Slots      : %lu\n", stats.TotalSlots);
    fwprintf(out, L"  Used Slots       : %lu\n", stats.CurrentUsedSlots);
    fwprintf(out, L"  Slot Size        : %lu bytes\n", stats.SlotSize);
    fwprintf(out, L"  Total Captured   : %I64u\n", stats.TotalCaptured);
    fwprintf(out, L"  Dropped Count    : %lu\n", stats.DroppedCount);

    if (stats.TotalSlots > 0) {
        double usage = (double)stats.CurrentUsedSlots / (double)stats.TotalSlots * 100.0;
        fwprintf(out, L"  Usage            : %.1f%%\n", usage);
    }
    fwprintf(out, L"%hs========================================\n", COLOR_CYAN);
}

static HANDLE OpenDevice(WCHAR* path)
{
    if (path && *path) {
        return CreateFileW(path,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, OPEN_EXISTING,
                           FILE_FLAG_OVERLAPPED, NULL);
    }

    // Try to find the device via device interface GUID
    // For now, try common device paths
    static const WCHAR* devicePaths[] = {
        L"\\\\.\\M487Filter0",
        L"\\\\.\\M487Filter1",
        L"\\\\.\\M487Filter",
    };

    for (int i = 0; i < 3; i++) {
        HANDLE h = CreateFileW(devicePaths[i],
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING,
                               FILE_FLAG_OVERLAPPED, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            return h;
        }
    }

    return INVALID_HANDLE_VALUE;
}

static BOOL MapRingBuffer(VOID)
{
    DWORD bytesReturned = 0;
    M487_FILTER_RING_MAPPING_INFO mappingInfo = { 0 };

    if (!DeviceIoControl(g_hDevice,
                        IOCTL_M487FILTER_MAP_RING_BUFFER,
                        NULL, 0,
                        &mappingInfo, sizeof(mappingInfo),
                        &bytesReturned, NULL)) {
        fwprintf(stderr, L"[ERROR] IOCTL_M487FILTER_MAP_RING_BUFFER failed: %lu\n", GetLastError());
        fwprintf(stderr, L"[INFO]  Make sure the M487Filter driver is installed and the device is present.\n");
        return FALSE;
    }

    if (mappingInfo.RingBufferUserVa == 0 || mappingInfo.RingBufferSize == 0) {
        fwprintf(stderr, L"[ERROR] Invalid ring buffer address or size from driver.\n");
        return FALSE;
    }

    // Fill in g_ringInfo for display purposes
    g_ringInfo.RingBufferVa = mappingInfo.RingBufferUserVa;
    g_ringInfo.RingBufferSize = mappingInfo.RingBufferSize;
    g_ringInfo.SlotSize = mappingInfo.SlotSize;
    g_ringInfo.SlotCount = mappingInfo.SlotCount;
    g_ringInfo.MaxDataSize = mappingInfo.MaxDataSize;

    g_pRing = (PM487_RING_CONTROL)mappingInfo.RingBufferUserVa;

    // Validate magic
    if (g_pRing->Magic != M487_RING_MAGIC) {
        fwprintf(stderr, L"[ERROR] Ring buffer magic mismatch: expected 0x%X, got 0x%X\n",
                M487_RING_MAGIC, g_pRing->Magic);
        return FALSE;
    }

    return TRUE;
}

static void ReadAndDisplayEntries(FILE* out)
{
    ULONG slotSize = g_ringInfo.SlotSize;
    ULONG slotCount = g_ringInfo.SlotCount;
    ULONG64 totalCaptured = g_pRing->TotalCaptured;

    // ReadIndex is where user-mode will read next
    ULONG readIdx = g_pRing->ReadIndex;
    ULONG writeIdx = g_pRing->WriteIndex;

    // Count available entries
    ULONG usedSlots;
    if (writeIdx >= readIdx) {
        usedSlots = writeIdx - readIdx;
    } else {
        usedSlots = slotCount - readIdx + writeIdx;
    }

    if (!g_opt.quiet) {
        fwprintf(stderr, L"[INFO] Ring: read=%lu write=%lu used=%lu total=%llu\n",
                readIdx, writeIdx, usedSlots, totalCaptured);
    }

    // Show at most 'lines' entries
    ULONG toShow = (g_opt.lines > 0) ? (ULONG)g_opt.lines : 16UL;
    if (usedSlots < toShow) {
        toShow = usedSlots;
    }

    if (toShow == 0) {
        if (!g_opt.quiet) {
            fwprintf(out, L"\n  (no entries in ring buffer)\n");
        }
        return;
    }

    // Start from entry that would be shown first
    ULONG startIdx;
    if (writeIdx >= toShow) {
        startIdx = writeIdx - toShow;
    } else {
        startIdx = slotCount - (toShow - writeIdx);
    }

    // Traverse circular buffer
    ULONG idx = startIdx;
    ULONG shown = 0;
    g_entryIndex = 0;

    while (shown < toShow) {
        UCHAR* slotBase = (UCHAR*)g_pRing->DataStart + (idx * slotSize);
        PM487_RING_ENTRY entry = (PM487_RING_ENTRY)slotBase;

        // Validate entry has data or is marked
        // Entries with DataLength=0 are empty slots
        if (entry->DataLength > 0 || entry->IoctlType != IoctlType_Unknown) {
            if (ShouldShowEntry(entry->IoctlType)) {
                PrintEntry(entry, out);
                shown++;
            }
            g_entryIndex++;
        }

        idx = (idx + 1) % slotCount;

        // Safety: don't loop forever
        if (idx == startIdx) break;
    }
}

static void TailLoop(FILE* out)
{
    ULONG64 lastTotal = 0;

    while (1) {
        ULONG64 totalCaptured = g_pRing->TotalCaptured;

        if (totalCaptured != lastTotal) {
            if (totalCaptured > lastTotal) {
                ULONG64 newEntries = totalCaptured - lastTotal;
                if (!g_opt.quiet) {
                    fwprintf(stderr, L"[INFO] %llu new entries\n", newEntries);
                }

                // Show new entries
                ULONG slotSize = g_ringInfo.SlotSize;
                ULONG slotCount = g_ringInfo.SlotCount;
                ULONG writeIdx = g_pRing->WriteIndex;

                int toShow = (g_opt.lines > 0) ? (int)g_opt.lines : 16;
                if ((ULONG)toShow > newEntries) toShow = (int)newEntries;

                // Walk back from write index
                ULONG idx = (writeIdx + slotCount - toShow) % slotCount;
                int shown = 0;
                g_entryIndex = 0;

                while (shown < toShow) {
                    UCHAR* slotBase = (UCHAR*)g_pRing->DataStart + (idx * slotSize);
                    PM487_RING_ENTRY entry = (PM487_RING_ENTRY)slotBase;

                    if (entry->DataLength > 0 || entry->IoctlType != IoctlType_Unknown) {
                        if (ShouldShowEntry(entry->IoctlType)) {
                            PrintEntry(entry, out);
                            shown++;
                        }
                        g_entryIndex++;
                    }

                    idx = (idx + 1) % slotCount;

                    if (idx == (writeIdx + slotCount - toShow) % slotCount) break;
                }
            }
            lastTotal = totalCaptured;
        }

        Sleep(100); // 100ms polling interval
    }
}

static void ParseCommandLine(int argc, WCHAR* argv[])
{
    int i;

    memset(&g_opt, 0, sizeof(g_opt));
    g_opt.lines = 16;
    g_opt.filterAll = TRUE;
    g_opt.devicePath = NULL;

    for (i = 1; i < argc; i++) {
        WCHAR* arg = argv[i];

        if (wcscmp(arg, L"-f") == 0 || wcscmp(arg, L"--follow") == 0) {
            g_opt.follow = TRUE;
        }
        else if (wcscmp(arg, L"-n") == 0 || wcscmp(arg, L"--lines") == 0) {
            if (i + 1 < argc) {
                g_opt.lines = _wtoi(argv[++i]);
                if (g_opt.lines <= 0) g_opt.lines = 16;
                if (g_opt.lines > M487_RING_SLOT_COUNT) g_opt.lines = M487_RING_SLOT_COUNT;
            }
        }
        else if (wcscmp(arg, L"--filter") == 0) {
            if (i + 1 < argc) {
                WCHAR* filter = argv[++i];
                g_opt.filterAll = FALSE;

                if (wcscmp(filter, L"all") == 0) {
                    g_opt.filterAll = TRUE;
                }
                else if (wcscmp(filter, L"set-feature") == 0) {
                    g_opt.filterSetFeature = TRUE;
                }
                else if (wcscmp(filter, L"get-feature") == 0) {
                    g_opt.filterGetFeature = TRUE;
                }
                else if (wcscmp(filter, L"write-report") == 0) {
                    g_opt.filterWriteReport = TRUE;
                }
                else if (wcscmp(filter, L"read-report") == 0) {
                    g_opt.filterReadReport = TRUE;
                }
                else {
                    fwprintf(stderr, L"[ERROR] Unknown filter type: %s\n", filter);
                    fwprintf(stderr, L"Valid: set-feature, get-feature, write-report, read-report, all\n");
                    exit(1);
                }
            }
        }
        else if (wcscmp(arg, L"--no-color") == 0) {
            g_opt.noColor = TRUE;
        }
        else if (wcscmp(arg, L"-q") == 0 || wcscmp(arg, L"--quiet") == 0) {
            g_opt.quiet = TRUE;
        }
        else if (wcscmp(arg, L"-s") == 0 || wcscmp(arg, L"--stats") == 0) {
            g_opt.statsOnly = TRUE;
        }
        else if (wcscmp(arg, L"-d") == 0 || wcscmp(arg, L"--device") == 0) {
            if (i + 1 < argc) {
                g_opt.devicePath = argv[++i];
            }
        }
        else if (wcscmp(arg, L"-h") == 0 || wcscmp(arg, L"--help") == 0 || wcscmp(arg, L"/?") == 0) {
            PrintHelp();
            exit(0);
        }
        else {
            fwprintf(stderr, L"[ERROR] Unknown option: %s\n", arg);
            PrintHelp();
            exit(1);
        }
    }
}

int wmain(int argc, WCHAR* argv[])
{
    FILE* out = stdout;

    ParseCommandLine(argc, argv);
    InitColor();

    if (!g_opt.quiet) {
        wprintf(L"M487Filter HID Packet Log Viewer\n");
        wprintf(L"================================\n\n");
    }

    // Open device
    g_hDevice = OpenDevice(g_opt.devicePath);
    if (g_hDevice == INVALID_HANDLE_VALUE) {
        fwprintf(stderr, L"[ERROR] Cannot open M487Filter device.\n");
        fwprintf(stderr, L"[ERROR] GetLastError = %lu\n", GetLastError());
        fwprintf(stderr, L"[INFO]  Make sure the driver is installed.\n");
        fwprintf(stderr, L"[INFO]  You can specify device path with --device PATH\n");
        return 1;
    }

    // Map ring buffer
    if (!MapRingBuffer()) {
        CloseHandle(g_hDevice);
        return 1;
    }

    if (!g_opt.quiet) {
        PrintHeader(out);
    }

    if (g_opt.statsOnly) {
        PrintStats(out);
    } else {
        ReadAndDisplayEntries(out);
    }

    if (g_opt.follow && !g_opt.statsOnly) {
        TailLoop(out);
    }

    CloseHandle(g_hDevice);
    return 0;
}
