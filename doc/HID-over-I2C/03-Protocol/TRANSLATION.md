# HID-over-I²C Bridge — USB ↔ I²C Translation Protocol

> Branch: `architecture/hid-over-i2c`  
> Date: 2026-03-26

---

## 1. USB HID Request to HID-over-I²C Operation Mapping

### 1.1 GET_REPORT (Input Report)

**USB:** Host sends `GET_REPORT` via USB Control Transfer (bRequest=0x01)  
**I²C:** M487 reads report from device via register access

```
USB Host                          M487 Bridge                         I²C HID Device
  │                                    │                                    │
  │ GET_REPORT (Input, ReportID=x)     │                                    │
  │ ──────────────────────────────────►│                                    │
  │                                    │                                    │
  │                                    │ 1. Prepare command:                │
  │                                    │    CmdReg = [0x01][0x10|ReportID] │
  │                                    │    (Opcode=GET_REPORT, Type=Input) │
  │                                    │                                    │
  │                                    │ 2. Write Command Register          │
  │                                    │ I²C Write: [CmdReg_Hi][CmdReg_Lo]  │
  │                                    │───────────────────────────────────►│
  │                                    │                                    │
  │                                    │ 3. Read Data Register              │
  │                                    │ I²C Read: [len_MSB][len_LSB][data]│
  │                                    │◄───────────────────────────────────│
  │                                    │                                    │
  │                                    │ 4. Return report to USB            │
  │ Input Report                       │                                    │
  │◄───────────────────────────────────│                                    │
  │                                    │                                    │
```

### 1.2 GET_REPORT (Feature Report)

```
USB Host                          M487 Bridge                         I²C HID Device
  │                                    │                                    │
  │ GET_REPORT (Feature, ReportID=x)   │                                    │
  │ ──────────────────────────────────►│                                    │
  │                                    │                                    │
  │                                    │ 1. Write Command Register          │
  │                                    │    CmdReg = [0x01][0x30|ReportID] │
  │                                    │    (Opcode=GET_REPORT, Type=Feat)  │
  │                                    │ I²C Write: [CmdReg_Hi][CmdReg_Lo] │
  │                                    │───────────────────────────────────►│
  │                                    │                                    │
  │                                    │ 2. Read Data Register              │
  │                                    │ I²C Read: [len_MSB][len_LSB][data]│
  │                                    │◄───────────────────────────────────│
  │                                    │                                    │
  │ Feature Report                     │                                    │
  │◄───────────────────────────────────│                                    │
```

### 1.3 SET_REPORT (Output Report)

**USB:** Host sends `SET_REPORT` via USB Control Transfer (bRequest=0x09)  
**I²C:** M487 writes report to device via register access

```
USB Host                          M487 Bridge                         I²C HID Device
  │                                    │                                    │
  │ SET_REPORT (Output, ReportID=x)    │                                    │
  │ + Output Report data               │                                    │
  │ ──────────────────────────────────►│                                    │
  │                                    │                                    │
  │                                    │ 1. Write Data Register first:       │
  │                                    │    [len_MSB][len_LSB][data...]     │
  │                                    │ I²C Write (no STOP between)       │
  │                                    │───────────────────────────────────►│
  │                                    │                                    │
  │                                    │ 2. Write Command Register:         │
  │                                    │    CmdReg = [0x01][0x20|ReportID] │
  │                                    │    (Opcode=SET_REPORT, Type=Out)   │
  │                                    │ I²C Write (RESTART then write)     │
  │                                    │───────────────────────────────────►│
  │                                    │                                    │
  │ ACK                                │                                    │
  │◄───────────────────────────────────│                                    │
  │                                    │                                    │
```

### 1.4 SET_REPORT (Feature Report)

```
USB Host                          M487 Bridge                         I²C HID Device
  │                                    │                                    │
  │ SET_REPORT (Feature, ReportID=x)   │                                    │
  │ + Feature Report data               │                                    │
  │ ──────────────────────────────────►│                                    │
  │                                    │                                    │
  │                                    │ 1. Write Data Register             │
  │                                    │    [len_MSB][len_LSB][data...]     │
  │                                    │ I²C Write                          │
  │                                    │───────────────────────────────────►│
  │                                    │                                    │
  │                                    │ 2. Write Command Register          │
  │                                    │    CmdReg = [0x01][0x30|ReportID] │
  │                                    │    (Opcode=SET_REPORT, Type=Feat) │
  │                                    │ I²C Write                          │
  │                                    │───────────────────────────────────►│
  │                                    │                                    │
  │ ACK                                │                                    │
  │◄───────────────────────────────────│                                    │
```

### 1.5 RESET Command

```
USB Host                          M487 Bridge                         I²C HID Device
  │                                    │                                    │
  │ SET_REPORT (Feature, ReportID=0)   │                                    │
  │ + [0x00, 0x00] (RESET command)    │                                    │
  │ ──────────────────────────────────►│                                    │
  │                                    │                                    │
  │                                    │ Write Command Register:            │
  │                                    │   [0x01][0x10] (Opcode=RESET)     │
  │                                    │ I²C Write                          │
  │                                    │───────────────────────────────────►│
  │                                    │                                    │
  │                                    │ Device initializes,                 │
  │                                    │ writes 0x0000 to Input Register,   │
  │                                    │ asserts interrupt                   │
  │                                    │◄───────────────────────────────────│
  │                                    │                                    │
  │ ACK                                │                                    │
  │◄───────────────────────────────────│                                    │
```

### 1.6 SET_IDLE

```
USB Host                          M487 Bridge                         I²C HID Device
  │                                    │                                    │
  │ SET_IDLE (Duration=50ms)          │                                    │
  │ ──────────────────────────────────►│                                    │
  │                                    │                                    │
  │                                    │ Write Command Register:           │
  │                                    │   [0x02][0x00|Duration]            │
  │                                    │ I²C Write                          │
  │                                    │───────────────────────────────────►│
  │                                    │                                    │
  │ ACK                                │                                    │
  │◄───────────────────────────────────│                                    │
```

---

## 2. Interrupt-Driven Input Report Flow

When the I²C HID device has data ready, it asserts its interrupt pin (connected to M487 GPIO).

```
I²C Device                    M487 Bridge                         USB Host
   │                               │                                  │
   │ [Input Report Ready]          │                                  │
   │ GPIO interrupt ───────────────►│                                  │
   │                               │                                  │
   │                               │ Read Input Register:              │
   │                               │   I²C Read → [len][data]         │
   │◄──────────────────────────────│                                  │
   │                               │                                  │
   │                               │ De-assert interrupt (device)     │
   │                               │◄─────────────────────────────────│
   │                               │                                  │
   │                               │ Send via Interrupt IN EP1:        │
   │                               │   USB Interrupt Transfer          │
   │                               │──────────────────────────────────►│
   │                               │                                  │
```

---

## 3. Register Access Protocol

### 3.1 Reading a Register

```
M487 → I²C: [S][DeviceAddr_W][REG_ADDR][Sr][DeviceAddr_R][DATA...][P]
```

- **S** = START condition
- **DeviceAddr_W** = Device address with R/W bit = 0
- **REG_ADDR** = Register index to read
- **Sr** = Repeated START
- **DeviceAddr_R** = Device address with R/W bit = 1
- **DATA** = [Length_MSB][Length_LSB][payload...] (for data registers)
- **P** = STOP condition

### 3.2 Writing a Register

```
M487 → I²C: [S][DeviceAddr_W][REG_ADDR][DATA...][P]
```

- **DATA** = [Length_MSB][Length_LSB][payload...] (for data registers)
- **DATA** = [Cmd_Hi][Cmd_Lo] (for command register)

### 3.3 Two-Phase Write (Set Report)

Some operations require writing data first, then command:

```
M487 → I²C: [S][DeviceAddr_W][DataReg][DATA...][Sr][DeviceAddr_W][CmdReg][Cmd][P]
                                                           ▲
                                                           │
                                            Repeated START (no STOP between)
```

---

## 4. HID Descriptor Retrieval

### 4.1 Step 1: Read HID Descriptor (Fixed 30 bytes)

```
M487 → I²C: [S][Addr_W][HID_DESC_REG][Sr][Addr_R]
I²C Device → M487: [30 bytes HID Descriptor]
```

Validation:
- `bcdVersion` must be 0x0100
- `wHIDDescLength` must be 30
- All register addresses must be non-zero and unique

### 4.2 Step 2: Read Report Descriptor

```
M487 → I²C: [S][Addr_W][wReportDescRegister][Sr][Addr_R]
I²C Device → M487: [wReportDescLength bytes]
```

Cache this for:
- Device capability discovery
- M487's own Report Descriptor generation

### 4.3 Step 3: Reset Device

```
M487 → I²C: [S][Addr_W][wCommandRegister][0x01][0x10][P]
```

Wait for interrupt, then read initial state.

---

## 5. Report Format Translation

### 5.1 I²C Device Report Format

```
[Length_MSB] [Length_LSB] [Report Data...]
 ↑ minimum 2 bytes (length field itself)
```

### 5.2 USB HID Report Format

Depends on Report Descriptor. Typically:

```
[ReportID] [Report Data...]  (if ReportID used)
```

Or raw data without ReportID.

### 5.3 Translation Rule

```
I²C → USB: 
  If Report Descriptor includes ReportID:
    Strip first 2 length bytes, prepend ReportID from command
  Else:
    Strip first 2 length bytes, send as-is

USB → I²C:
  If Report Descriptor includes ReportID:
    Use ReportID from report
  Else:
    Length = len(report) + 2
    Send [Length_MSB][Length_LSB][report data]
```

---

## 6. HID over I²C Command Reference

### 6.1 Command Register Format

```
Byte 0: [Reserved:4][Opcode:4]
Byte 1: [Reserved:2][ReportType:2][ReportID:4]
```

**Report Type:** 01=Input, 10=Output, 11=Feature

### 6.2 Opcode Encoding

| Opcode | Name | Flow |
|--------|------|------|
| 0x1 | RESET | Write only |
| 0x2 | GET_REPORT | Write cmd → Read data |
| 0x3 | SET_REPORT | Write data → Write cmd |
| 0x4 | GET_IDLE | Write cmd → Read data |
| 0x5 | SET_IDLE | Write cmd only |
| 0x6 | GET_PROTOCOL | Write cmd → Read data |
| 0x7 | SET_PROTOCOL | Write cmd only |
| 0x8 | SET_POWER | Write cmd only |
| 0xE | VENDOR RESERVED | Device-specific |

### 6.3 Report Type Encoding

| Type | Value | Usage |
|------|-------|-------|
| Input | 01 | GET_REPORT |
| Output | 10 | SET_REPORT |
| Feature | 11 | GET_REPORT / SET_REPORT |

---

## 7. Timing Requirements

| Parameter | Value | Notes |
|-----------|-------|-------|
| Command Register write | < 10ms | Per I²C spec |
| Data Register read | < 10ms | Per I²C spec |
| Interrupt pulse width | > 100µs | From device |
| Interrupt response | < 1ms | M487 GPIO latency |
| USB Interrupt IN | Every 1ms (min) | USB HS interrupt EP |
| Report complete | < 5ms total | End-to-end |

---

## 8. Error Recovery

| Condition | Detection | Recovery |
|-----------|-----------|----------|
| I²C NACK on address | No ACK after address byte | Retry 3x, return USB STALL |
| I²C NACK on data | No ACK after data byte | Abort, return USB error |
| I²C timeout (>10ms) | Clock stretch timeout | Reset I²C bus, retry |
| Invalid HID Descriptor | Version != 0x0100 or length != 30 | Reject device |
| Report too long | Report > wMaxInputLength | Truncate, flag overflow |
| Device disconnect | GPIO interrupt lost | Re-scan I²C bus |
