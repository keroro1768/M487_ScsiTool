# M487 HID over I2C Bridge — Architecture

> Branch: `architecture/hid-over-i2c`  
> Date: 2026-03-26  
> Status: **Phase 1 Complete**

---

## 1. System Overview

The M487 bridges a **Windows PC** (USB HID host) and an **I²C HID device** (e.g. touchpad, sensor) using the **HID over I²C** protocol.

```
┌─────────────┐      USB HID       ┌─────────────────┐      I²C HID      ┌──────────────────┐
│   Windows   │◄──────────────────►│      M487        │◄─────────────────►│  HID over I²C     │
│   PC       │   Standard HID      │  USB Device      │  HID over I²C    │  Device           │
│  (USB Host)│   Class Requests    │  + I²C Host      │  Protocol        │  (e.g. Touchpad)  │
│            │                     │  Bridge          │                  │                   │
└─────────────┘                     └─────────────────┘                  └──────────────────┘
```

**Key constraint:** M487 appears as a **standard USB HID device** to the PC. The bridge translates USB HID class requests into HID over I²C register operations.

---

## 2. Bridge Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                        M487 Firmware                         │
│                                                               │
│  ┌──────────────┐    ┌────────────────┐    ┌───────────────┐ │
│  │  USB HID     │◄──►│  HID-over-I²C  │◄──►│    I²C Host   │ │
│  │  Device      │    │    Bridge      │    │   (UI2C0)     │ │
│  │  Layer       │    │   Translator   │    │               │ │
│  │              │    │                │    │  PE2=CLK      │ │
│  │  - HSUSBD    │    │  - Protocol    │    │  PE3=DAT0     │ │
│  │  - Endpoints │    │    Translation │    │               │ │
│  │  - HID Req   │    │  - Register    │    │               │ │
│  │    Handler   │    │    Access      │    │               │ │
│  └──────────────┘    └────────────────┘    └───────────────┘ │
│                              │                    │          │
│                              ▼                    ▼          │
│                       ┌────────────────┐    ┌──────────────┐  │
│                       │   HID Device   │    │  I²C Bus    │  │
│                       │   Properties   │    │  Scan       │  │
│                       │   (Desc, Rpts) │    │             │  │
│                       └────────────────┘    └──────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. Firmware Layer Structure

### Layer 1: USB HID Device Interface
- Handles USB enumeration as a HID device
- Exposes Interrupt IN/OUT endpoints for HID reports
- Handles HID class requests: `GET_REPORT`, `SET_REPORT`, `GET_IDLE`, `SET_IDLE`, `GET_PROTOCOL`, `SET_PROTOCOL`
- No vendor-specific HID commands — the protocol is fully HID-compliant

### Layer 2: HID-over-I²C Bridge Translator
- Translates USB HID class requests → HID over I²C register operations
- Maintains connection state to the I²C HID device
- Manages command protocol (RESET, GET_REPORT, SET_REPORT, etc.)
- Handles Input Report buffering from I²C device
- Manages power state

### Layer 3: I²C Host Driver (UI2C0)
- Hardware: UI2C0 on PE2(CLK)/PE3(DAT0)
- Speed: 100kHz (default), up to 400kHz
- Handles I²C read/write transactions with the HID device
- Manages clock stretching and bus errors

### Layer 4: Device Discovery & Properties
- I²C bus scanning to detect HID over I²C devices
- Reads and caches HID Descriptor from device
- Parses Report Descriptor
- Maintains device information: VID, PID, version, register addresses

---

## 4. USB HID Interface Design

### 4.1 USB Configuration

| Parameter | Value |
|-----------|-------|
| VID | 0x0416 (Nuvoton) |
| PID | 0x5050 (TBD) |
| Speed | High-Speed |
| Power | Self-powered, 100mA max |
| Configuration | Single |

### 4.2 Endpoints

| EP | Type | Direction | Purpose |
|----|------|----------|---------|
| EP0 | Control | IN/OUT | Standard HID requests |
| EP1 | Interrupt | IN | Input Reports (device → PC) |
| EP2 | Interrupt | OUT | Output Reports (PC → device) |

**Note:** HID over I²C devices use Interrupt OUT for output reports, matching our HID bridge design.

### 4.3 Report Descriptor

The M487's own Report Descriptor describes the reports it can exchange with the host. This can be:
- A simple pass-through design: whatever reports the I²C device supports, the M487 bridges
- Or a custom wrapper: M487 defines its own report IDs that map to specific I²C device functions

**Design decision:** The bridge operates in **transparent mode** — it bridges the I²C device's native Report Descriptor. The M487 acts as a pass-through.

### 4.4 HID Class Requests

All standard HID class requests are supported:

| Request | Direction | Description |
|---------|-----------|-------------|
| GET_REPORT | Host→Device | Get report from I²C device |
| SET_REPORT | Host→Device | Send report to I²C device |
| GET_IDLE | Host→Device | Read idle rate |
| SET_IDLE | Host→Device | Set idle rate |
| GET_PROTOCOL | Host→Device | Read boot/report protocol |
| SET_PROTOCOL | Host→Device | Set boot/report protocol |

---

## 5. HID-over-I²C Translation Layer

### 5.1 Translation Table

| USB HID Request | HID-over-I²C Operation |
|-----------------|------------------------|
| GET_REPORT (Input) | 1. Write Command Register (Opcode=GET_REPORT, Type=Input) 2. Read Data Register |
| GET_REPORT (Feature) | 1. Write Command Register (Opcode=GET_REPORT, Type=Feature) 2. Read Data Register |
| SET_REPORT (Output) | 1. Write Data Register (report) 2. Write Command Register (Opcode=SET_REPORT, Type=Output) |
| SET_REPORT (Feature) | 1. Write Data Register (report) 2. Write Command Register (Opcode=SET_REPORT, Type=Feature) |
| RESET | Write Command Register (Opcode=RESET) |
| GET_IDLE | Write Command Register, Read Data Register |
| SET_IDLE | Write Command Register (no data response) |
| GET_PROTOCOL | Write Command Register, Read Data Register |
| SET_PROTOCOL | Write Command Register |

### 5.2 Input Report Flow (I²C Device → PC)

```
I²C Device asserts interrupt (GPIO)
         │
         ▼
M487 detects GPIO interrupt
         │
         ▼
M487 reads Input Register from I²C device (I²C read, length prefixed)
         │
         ▼
M487 sends Input Report to PC via USB Interrupt IN EP1
         │
         ▼
PC receives Input Report
```

### 5.3 Output Report Flow (PC → I²C Device)

```
PC sends Output Report via USB Interrupt OUT EP2
         │
         ▼
M487 receives Output Report
         │
         ▼
M487 writes report data to Output Register (I²C write, length prefixed)
         │
         ▼
M487 writes Command Register (Opcode=SET_REPORT, Type=Output)
         │
         ▼
I²C device processes Output Report
```

---

## 6. HID Descriptor Caching

On device detection, the bridge reads and caches:

```
1. HID Descriptor (30 bytes from device register)
   - Validates bcdVersion = 0x0100
   - Extracts register addresses
   - Extracts wMaxInputLength, wMaxOutputLength
   - Extracts VID, PID, VersionID

2. Report Descriptor (variable length from wReportDescRegister)
   - Caches for GET_REPORT passthrough
   - Used to build M487's own Report Descriptor
```

---

## 7. I²C Device Discovery

### 7.1 Automatic Discovery (On Connect)

```
1. Scan I²C addresses 0x03–0x77
2. For each responding address:
   a. Read first 2 bytes (HID descriptor length hint)
   b. If valid HID device:
      - Read full HID Descriptor
      - Cache properties
      - Add to device list
```

### 7.2 Device Address Configuration

Default HID over I²C device address: **0x2E (7-bit)**
Other common addresses: 0x10, 0x11, 0x2E, 0x3E

---

## 8. Error Handling

| Error | Response |
|-------|----------|
| I²C device NACK | Retry up to 3 times, then return error to USB host |
| I²C timeout | Abort transaction, reset I²C bus |
| USB timeout | NAK the request |
| Invalid HID Descriptor | Reject device, log error |
| Unsupported report | Return error response |
| Buffer overflow | Truncate to max length, flag overflow |

---

## 9. Memory Layout

```
Flash:     0x00000000 (APROM)
SRAM:      0x20000000
  ├── 0x20000000: I²C RX/TX buffers (2KB)
  ├── 0x20000800: HID Descriptor cache (256B)
  ├── 0x20000900: Report Descriptor cache (4KB)
  ├── 0x20001900: Input Report buffer (512B)
  ├── 0x20001D00: Output Report buffer (512B)
  ├── 0x20002100: MSC RAM Disk (30KB) — optional
  └── 0x20008000: Stack / Heap
```

---

## 10. Interrupt Priorities

| IRQ | Priority | Handler |
|-----|----------|---------|
| USBD20_IRQn | 2 (High) | USB bus events, control transfers |
| USCI0_IRQn | 3 | I²C protocol events |
| GPIOCP0_IRQn | 4 | GPIO interrupt from I²C device |

---

## 11. Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| USB HID vs Custom | HID class | OS-native driver, no custom driver needed |
| Report Descriptor | Pass-through | Bridge is transparent to report format |
| I²C Speed | 100kHz default | Maximizes compatibility |
| Max I²C devices | 1 (expandable) | Single device per bridge initially |
| Memory model | Static buffers | Simpler, deterministic |
| Input Report delivery | Interrupt-driven | I²C device's GPIO triggers read |
| Power management | Host-initiated only | Simpler implementation |

---

## 12. Future Extensions

- Multiple I²C HID devices (multi-TLC)
- Flash storage passthrough (MSC overlay)
- Boot protocol support
- HID over I²C Low Power Mode
- ACPI integration descriptors
