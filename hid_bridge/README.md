# M487 HID I2C Bridge - Windows Tool

**Executable:** `build\M487HidTool.exe`

## Overview

A Windows CLI tool for communicating with the M487 USB Composite Device's HID I2C Bridge interface.

- **Target Device:** M487 USB Composite Device (VID=0x04F3, PID=0x0732)
- **Interface:** HID I2C Bridge (Interface 1, Interrupt Endpoint EP3)
- **API:** Windows HID API (`hid.dll`, `setupapi.dll`)

## Build

### Requirements
- Windows SDK (10.0.26100.0 or later)
- Visual Studio 2022 with C++ Build Tools
- MSVC 14.44+ compiler

### Build Steps

```powershell
cd D:\AiWorkSpace\M487_ScsiTool\hid_bridge\build
build.bat
```

The executable will be at `build\M487HidTool.exe`.

### Clean Rebuild

```powershell
Remove-Item D:\AiWorkSpace\M487_ScsiTool\hid_bridge\build\*.obj -Force
Remove-Item D:\AiWorkSpace\M487_ScsiTool\hid_bridge\build\M487HidTool.exe -Force
build.bat
```

## Usage

```
D:\AiWorkSpace\M487_ScsiTool\hid_bridge\build\M487HidTool.exe
```

### Command Reference

| Command | Description |
|---------|-------------|
| `list` | Enumerate all M487 HID devices |
| `open [idx]` | Connect to device (default: index 0) |
| `close` | Disconnect from device |
| `info` | Show device VID/PID/version |
| `test` | Test connection with device |
| `scan` | Scan I2C bus for devices |
| `write <addr> <hex>` | Write hex data to I2C slave |
| `read <addr> <len>` | Read N bytes from I2C slave |
| `writeread <addr> <whex> <rlen>` | Write hex, then read N bytes |
| `help` | Show help |
| `exit` | Exit |

### Examples

```
M487-HID> list
  Searching for M487 HID devices (VID=0x04F3, PID=0x0732)...
  Found 1 device(s):
    [0] M487 USB Device
        VID=0x04F3 PID=0x0732

M487-HID> open
  Connecting to device[0]...
  [OK] Connected successfully.

M487-HID> scan
  Scanning I2C bus...
       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
  00x: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  10x: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  ...
  30x: -- -- -- -- -- -- -- -- -- -- -- -- 3C -- -- -- --
  ...

M487-HID> writeread 0x3C 0x00 1
  Write+Read: 1 bytes -> 1 bytes from 0x3C
  [OK] Write+Read successful.
  Write Data: 00
  Read Data:  5A

M487-HID> exit
```

## Architecture

```
M487HidDevice (hid_device.h / hid_device.cpp)
  ├── enumerate()        → SetupDiGetClassDevs + HidD_GetAttributes
  ├── connect()          → CreateFile (FILE_FLAG_OVERLAPPED)
  ├── sendOutputReport() → WriteFile (interrupt OUT)
  ├── receiveInputReport() → ReadFile (interrupt IN, overlapped)
  ├── i2cWrite()        → HID Report 0x01
  ├── i2cRead()         → HID Report 0x02
  ├── i2cWriteRead()    → HID Report 0x03
  └── i2cScan()         → Probe all 0x01-0x7F addresses
```

## HID Report Protocol

Matches the firmware in `firmware/composite/hid_i2c.c`:

| Report ID | Direction | Structure | Description |
|-----------|-----------|-----------|-------------|
| 0x01 | Host→Device | `HID_I2C_WriteReport_t` | I2C write |
| 0x02 | Host→Device | `HID_I2C_ReadRequest_t` | I2C read request |
| 0x02 | Device→Host | `HID_I2C_ReadResponse_t` | I2C read response |
| 0x03 | Host→Device | `HID_I2C_WriteRead_t` | I2C write+read |
| 0x03 | Device→Host | `HID_I2C_WriteReadResponse_t` | Write+read response |

## File Structure

```
hid_bridge/
├── CMakeLists.txt           # CMake build (requires CMake)
├── README.md                # This file
├── include/
│   └── hid_device.h         # M487HidDevice class header
└── src/
    ├── hid_device.cpp       # M487HidDevice class implementation
    └── main.cpp             # CLI application
    └── build/
        ├── build.bat        # MSVC build script
        └── M487HidTool.exe # Output executable
```

## Troubleshooting

**"No M487 HID devices found"**
- Verify device is connected and firmware is loaded
- Check Device Manager for HID device (VID=0x04F3, PID=0x0732)
- For composite device: make sure MSC driver doesn't claim the HID interface

**"WriteFile failed: 87"**
- Invalid parameter in WriteFile (likely buffer size mismatch)
- The HID driver requires a specific buffer alignment

**"ReadFile timeout"**
- Device not sending response
- Wrong endpoint configuration
- Try increasing the timeout in `receiveInputReport()`

**Device not enumerating**
- Windows may have loaded a different driver for the composite device
- Use Zadig or `pnputil` to verify HID interface driver
