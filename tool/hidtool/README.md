# hidtool - Unified HID + MSC Debug Channel CLI

## Overview

`hidtool` is a unified command-line tool providing both HID-over-I2C bridge access and MSC Debug Channel operations for M487-based USB devices on Windows.

## Requirements

- **Python 3.8+**
- **hidapi Python package** (for HID operations):
  ```batch
  pip install hidapi
  ```

## Quick Start

```batch
# List all commands
hidtool --help

# MSC Debug Channel commands
hidtool msc list              # List removable MSC drives
hidtool msc info              # Read device info (auto-detects drive)
hidtool msc readmem 0x20000000 64   # Read 64 bytes from SRAM
hidtool msc log 32            # Read 32 debug log entries
hidtool msc readreg 15        # Read register 15 (PRIMASK)
hidtool msc echo Hello        # Echo test

# HID commands (requires M487 HID-over-I2C Bridge connected)
hidtool device list           # List HID devices
hidtool device select 0      # Select device #0
hidtool read 0x50 0x10 16     # I2C read from addr 0x50, reg 0x10
hidtool write 0x50 0x10 0xAB 0xCD  # I2C write
hidtool reg read 0            # Read bridge register
hidtool monitor 100           # Monitor HID reports at 100ms interval
```

## Architecture

```
hidtool.py
├── HIDDebugChannel (hidapi)     → USB HID via WinUSB/hidapi
│   ├── device list/select
│   ├── i2c_read / i2c_write
│   └── reg_read / reg_write
│
└── MSCDebugChannel (ctypes)    → SCSI PassThrough for MSC BOT
    ├── msc list/select
    ├── get_info()
    ├── read_mem / write_mem
    ├── read_reg / write_reg
    ├── read_log()
    └── echo()
```

## MSC Debug Protocol

Uses vendor-specific SCSI CDB 0xC0 commands via IOCTL_SCSI_PASS_THROUGH_DIRECT.

| Sub-Cmd | Name | Description |
|---------|------|-------------|
| 0x01 | DBG_READ_MEM | Read ARM memory (addr + len in CDB) |
| 0x02 | DBG_WRITE_MEM | Write ARM memory |
| 0x03 | DBG_READ_REG | Read CPU register |
| 0x04 | DBG_WRITE_REG | Write CPU register |
| 0x07 | DBG_GET_INFO | Get device info (ChipID, FW version, etc.) |
| 0x08 | DBG_READ_LOG | Read debug log ring buffer |
| 0x0F | DBG_ECHO | Echo test |

## CPU Register IDs

| ID | Name | Description |
|----|------|-------------|
| 0-12 | R0-R12 | General purpose registers |
| 13 | SP | Stack Pointer |
| 14 | LR | Link Register |
| 15 | PC | Program Counter |
| 16 | xPSR | Program Status Register |
| 17 | MSP | Main Stack Pointer |
| 18 | PSP | Process Stack Pointer |
| 20 | PRIMASK | Priority Mask Register |
| 21 | CONTROL | Control Register |
| 22 | BASEPRI | Base Priority Register |

## Memory Map (M487)

| Region | Address Range | Size |
|--------|---------------|------|
| SRAM | 0x20000000 - 0x20027FFF | 160 KB |
| Flash | 0x00000000 - 0x000FFFFF | Up to 512 KB |

## Exit Codes

| Code | Meaning |
|------|--------|
| 0 | Success |
| 1 | General error / device not found |
| 2 | Invalid device index |
| 4 | USB/HID communication error |
| 99 | Unknown error |

## Auto-detection

`hidtool msc info|readmem|log|...` automatically detects the MSC drive letter from available removable drives. Use `hidtool msc select E:` to manually specify.
