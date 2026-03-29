# hidlog.exe - M487Filter HID Packet Log Viewer

A command-line tool for reading and displaying captured HID packets from the M487 USB Filter Driver.

## Features

- **Hexdump format output** - 16 bytes per line with ASCII sidebar
- **Report type filtering** - Show only specific HID operations
- **Color-coded output** - Distinguish packet types visually
- **Follow mode** - Real-time tail (-f) like `tail -f`
- **Statistics** - View ring buffer utilization
- **Quiet mode** - Compact single-line hex output

## Building

```batch
cd app
build.cmd hidlog
```

Or manually with MSVC:
```batch
cl /EHsc /W4 hidlog.c /Fehidlog.exe /link advapi32.lib
```

## Usage

```
hidlog [options]

Options:
  -f, --follow          Continuous tail mode (Ctrl+C to stop)
  -n, --lines N         Show last N entries (default: 16, max: 256)
  --filter TYPE         Filter by type: set-feature, get-feature,
                        write-report, read-report, all (default: all)
  --no-color            Disable ANSI color output
  -q, --quiet           Quiet mode (only hexdump output)
  -s, --stats           Show ring buffer statistics only
  -d, --device PATH     Device path (default: first found M487Filter device)
  -h, --help            Show this help
```

## Examples

```powershell
# Show last 16 captured packets
hidlog.exe

# Show last 100 packets
hidlog.exe -n 100

# Only show SET_FEATURE (host → device feature reports)
hidlog.exe --filter set-feature

# Follow WRITE_REPORT packets in real-time
hidlog.exe -f --filter write-report

# Show only ring buffer statistics
hidlog.exe -s

# Quiet mode with all packets (for piping)
hidlog.exe -q

# Disable colors
hidlog.exe --no-color
```

## Report Types

| Type | IOCTL | Direction | Color |
|------|-------|-----------|-------|
| SET_FEATURE | IOCTL_HID_SET_FEATURE | Host → Device | Green |
| GET_FEATURE | IOCTL_HID_GET_FEATURE | Device → Host | Yellow |
| WRITE_REPORT | IOCTL_HID_WRITE_REPORT | Host → Device | Cyan |
| READ_REPORT | IOCTL_HID_READ_REPORT | Device → Host | Magenta |

## Output Format

```
[   1] 2026-03-28 14:30:01.234 WREP IsWrite=T Len=8 ReportId=0x01 IOCTL=0x000D010B
  0000: 01 02 03 04 05 06 07 08  ........
  0008: 09 0A 0B 0C 0D 0E 0F 10  ........
```

- `[1]` - Entry index
- `2026-03-28 14:30:01.234` - Timestamp (HID report capture time)
- `WREP` - Report type code (4-letter abbreviation)
- `IsWrite=T` - Write operation (T=true, F=false)
- `Len=8` - Data length in bytes
- `ReportId=0x01` - HID Report ID
- `IOCTL=0x000D010B` - Windows IOCTL code
- Hexdump - 16 bytes per line with ASCII sidebar

## Requirements

- M487Filter driver installed
- M487 USB device present
- Windows 10/11 with Windows Terminal (for colors)

## Exit Codes

- `0` - Success
- `1` - Error (device not found, driver not installed, etc.)
