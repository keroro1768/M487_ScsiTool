# HID over I2C Protocol Specification Summary

> Source: Microsoft "HID over I2C Protocol Specification" v1.0  
> Date: March 21, 2012  
> Location: `D:\AiWorkSpace\KM\KM-collect\Hardware\I3C-USB-Bridge\Microsoft-HID-over-I2C-spec-full.md`

---

## 1. Overview

HID over I2C is a protocol that allows HID (Human Interface Device) class devices to communicate over an I²C bus instead of USB. This enables HID devices (keyboards, mice, sensors, touchscreens) to be connected via I²C to a host controller.

**Key benefit:** A single HID driver on the host can work with devices over USB, Bluetooth, or I²C — the HID protocol is bus-agnostic.

---

## 2. System Architecture

```
┌─────────────────┐      I²C       ┌─────────────────────────┐     USB      ┌─────────────┐
│  HID over I2C   │◄─────────────►│        M487             │◄────────────►│   Windows   │
│     DEVICE      │   (I2C Host)  │   (I2C Host +          │  (USB HID   │     PC      │
│ (e.g. Touchpad, │               │    USB Device Bridge)   │   Device)   │  (USB Host) │
│   Sensor)       │               │                         │             │             │
└─────────────────┘               └─────────────────────────┘             └─────────────┘
```

- **I²C side:** M487 acts as I²C host controller. The peripheral device is a HID DEVICE that implements HID over I²C protocol.
- **USB side:** M487 appears as a USB HID device to the PC. The PC sends standard HID requests over USB interrupt endpoints.
- **Bridge:** M487 translates between USB HID class requests and HID over I²C register operations.

---

## 3. HID Descriptor (30 bytes)

Every HID over I²C device MUST expose a HID Descriptor at a known register address.

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0 | wHIDDescLength | 2 | Total length = 30 (0x1E) for v1.00 |
| 2 | bcdVersion | 2 | Protocol version, BCD. Set to 0x0100 |
| 4 | wReportDescLength | 2 | Length of Report Descriptor in bytes |
| 6 | wReportDescRegister | 2 | Register index to read Report Descriptor |
| 8 | wInputRegister | 2 | Register index to read Input Report |
| 10 | wMaxInputLength | 2 | Max Input Report size (incl. 2-byte length field) |
| 12 | wOutputRegister | 2 | Register index to write Output Report (0 if none) |
| 14 | wMaxOutputLength | 2 | Max Output Report size (0 if none) |
| 16 | wCommandRegister | 2 | Register index for command requests |
| 18 | wDataRegister | 2 | Register index for data exchange |
| 20 | wVendorID | 2 | Vendor ID (must be non-zero) |
| 22 | wProductID | 2 | Product ID |
| 24 | wVersionID | 2 | Firmware revision (BCD) |
| 26 | RESERVED | 4 | Must be 0 |

**All registers must have unique non-zero indices.**

---

## 4. Register Model

The HID over I²C device exposes the following registers (accessed via I²C read/write):

| Register | Purpose | Access |
|----------|---------|--------|
| wReportDescRegister | Report Descriptor | Read only |
| wInputRegister | Input Report data (device → host) | Read only |
| wOutputRegister | Output Report data (host → device) | Write only |
| wCommandRegister | Command opcode | Write only |
| wDataRegister | Command data / response | Read/Write |

### Data Register Format

The first 2 bytes of any data register read/write ALWAYS contain the length:

```
[Length_MSB] [Length_LSB] [data...]
```

Length = 2 (length field itself) + actual data bytes.

---

## 5. Command Protocol

Commands are written to `wCommandRegister` (1 byte opcode + 1 byte params), then data is exchanged via `wDataRegister`.

### Command Register Format

```
Byte 1 (High): [Reserved 4 bits][Opcode 4 bits]
Byte 2 (Low):  [Reserved 2 bits][Report Type 2 bits][Report ID 4 bits]
```

Report Type: 01=Input, 10=Output, 11=Feature

### Opcode Table

| Opcode | Name | Host Mandatory | Device Mandatory |
|--------|------|---------------|----------------|
| 0001 | RESET | Yes | Yes |
| 0010 | GET_REPORT | Yes | Yes |
| 0011 | SET_REPORT | Yes | Yes |
| 0100 | GET_IDLE | No | No |
| 0101 | SET_IDLE | No | No |
| 0110 | GET_PROTOCOL | No | No |
| 0111 | SET_PROTOCOL | No | No |
| 1000 | SET_POWER | No | Yes |
| 1110 | VENDOR RESERVED | N/A | N/A |

---

## 6. Report Protocol

### Input Reports (Device → Host)

Triggered by DEVICE interrupt pin. Host reads `wInputRegister`:

```
[Length_MSB] [Length_LSB] [Report Data...]
```

- Length field always included (minimum 2).
- If multiple TLC: `[Length_MSB][Length_LSB][ReportID][Report Data...]`
- Device keeps interrupt asserted until all data read.

### Output Reports (Host → Device)

Host writes to `wOutputRegister`:

```
[Length_MSB] [Length_LSB] [Report Data...]
```

- No interrupt asserted for output.
- Length must be > 2.

### Feature Reports

Accessed via GET_REPORT / SET_REPORT commands through Command+Data registers.

---

## 7. Interrupt Handling

- DEVICE has a dedicated interrupt pin (level-triggered, active low typically).
- When DEVICE has Input Report ready: asserts interrupt → Host reads Input Register.
- DEVICE keeps interrupt asserted until Host reads complete report.
- After read: DEVICE de-asserts interrupt.
- If more data: DEVICE re-asserts interrupt.

---

## 8. HID Descriptor Retrieval Sequence

```
Host: READ I2C register [HID Descriptor Address], 30 bytes
Device: Returns 30-byte HID Descriptor
Host: Validate bcdVersion=0x0100, wHIDDescLength=30
Host: READ I2C register [wReportDescRegister], wReportDescLength bytes
Device: Returns Report Descriptor
```

---

## 9. Typical Enumeration Flow

```
1. Host reads HID Descriptor from device
2. Host validates version and length
3. Host reads Report Descriptor
4. Host sends RESET command
5. Device initializes, writes 0x0000 to Input Register, asserts interrupt
6. Host sends GET_REPORT (Input) to get current state
7. Device responds with current Input Report via Data Register
8. Normal operation begins
```

---

## 10. GET_REPORT / SET_REPORT Sequence

### GET_REPORT (e.g. Input Report):

```
1. Host writes Command Register: [0x00][0x10] (Opcode=GET_REPORT, Type=Input)
2. Device prepares report in Data Register: [len_MSB][len_LSB][report data...]
3. Host reads Data Register (repeated start I2C read)
4. Device returns [len_MSB][len_LSB][report...]
```

### SET_REPORT (e.g. Feature Report):

```
1. Host writes Data Register first: [len_MSB][len_LSB][report data...]
2. Host writes Command Register: [0x00][0x31] (Opcode=SET_REPORT, Type=Feature)
3. Device accepts and processes report (no response needed)
```

---

## 11. Power Management

- **DIPO (Device Initiated):** Device optimizes its own power (e.g., reduce sampling when idle).
- **HIPO (Host Initiated):**
  - SET_POWER(ON) = 0x00 → Device must be fully operational
  - SET_POWER(SLEEP) = 0x01 → Device enters low power, de-asserts interrupt

---

## 12. Error Handling

- **Timeout:** Host resets device if no response within 5 seconds.
- **Clock Stretching:** Max 10ms per I²C transaction.
- **Short Packet:** Host discards malformed reports.
- **Protocol Errors:** Device can issue DEVICE-Initiated Reset (DIR) by writing 0x0000 to Input Register and asserting interrupt.

---

## 13. I²C Bus Parameters

| Parameter | Value |
|-----------|-------|
| Addressing | 7-bit or 10-bit |
| Speed | Standard (100kHz), Fast (400kHz), Fast Mode+ (1MHz), High Speed (3.4MHz) |
| Byte Order | Little-endian (LSB first) |
| Interrupt | Level-triggered, dedicated GPIO per device |

---

## 14. Report ID Optimization

- Protocol optimized for Report ID < 15.
- If Report ID >= 15: Low byte = 0x0F (sentinel), third byte carries actual Report ID.

---

## 15. Key Differences from USB HID

| Aspect | USB HID | HID over I²C |
|--------|---------|-------------|
| Bus | USB | I²C |
| Control | USB Control Transfers | Command+Data Registers |
| Input Data | Interrupt IN endpoint | Read Input Register |
| Output Data | Interrupt OUT / Control Write | Write Output Register |
| Device Discovery | USB enumeration | ACPI + HID Descriptor |
| Interrupt | USB IN token | GPIO interrupt pin |

---

## 16. Reference Documents

- USB HID Class Specification v1.11
- I²C Specification (NXP UM10204)
- ACPI 5.0 Specification
- HID Usage Tables
