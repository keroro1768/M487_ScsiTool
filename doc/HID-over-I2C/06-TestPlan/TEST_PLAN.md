# HID-over-I²C Bridge — Test Plan

> Branch: `architecture/hid-over-i2c`  
> Status: Planned

---

## 1. Test Overview

### 1.1 Test Levels

| Level | Description | Target |
|-------|-------------|--------|
| Unit | Individual component functions | Developer |
| Integration | USB ↔ I²C translation | Developer |
| System | Full bridge with real device | QA |
| Compatibility | Different HID-over-I²C devices | QA |

### 1.2 Test Environment

```
PC (Windows) ←USB→ M487 ←I²C→ [Test Device]
```

- **M487:** NuLink2 debug interface
- **Test Device:** TBD (HID-over-I²C compatible)
- **PC:** Windows 10/11 with USB HID driver (built-in)

---

## 2. Unit Tests

### 2.1 I²C Driver Tests

| ID | Test | Method | Pass Criteria |
|----|------|--------|---------------|
| U-I2C-01 | I²C init | Check UI2C0 registers | Correct baudrate set |
| U-I2C-02 | Single byte write | Write 0xAA to register | ACK received |
| U-I2C-03 | Multi-byte write | Write 4 bytes | ACK for all bytes |
| U-I2C-04 | Single byte read | Read register | Correct data returned |
| U-I2C-05 | Multi-byte read | Read 30 bytes | Correct data, NACK on last |
| U-I2C-06 | NACK handling | Probe non-existent address | Retry behavior correct |
| U-I2C-07 | Timeout | Clock stretch exceeds limit | Timeout flag set |
| U-I2C-08 | Bus reset | After timeout | Bus recovered |

### 2.2 HID Descriptor Parser Tests

| ID | Test | Method | Pass Criteria |
|----|------|--------|---------------|
| U-DESC-01 | Valid descriptor | Parse known-good 30 bytes | All fields parsed |
| U-DESC-02 | Invalid version | bcdVersion != 0x0100 | Returns error |
| U-DESC-03 | Invalid length | wHIDDescLength != 30 | Returns error |
| U-DESC-04 | Zero register | Any register = 0 | Returns error |
| U-DESC-05 | Report desc read | Read wReportDescLength bytes | Full descriptor cached |

### 2.3 Command Builder Tests

| ID | Test | Input | Expected Output |
|----|------|-------|----------------|
| U-CMD-01 | GET_REPORT | opcode=2, type=1, id=3 | [0x21][0x13] |
| U-CMD-02 | SET_REPORT | opcode=3, type=2, id=5 | [0x32][0x25] |
| U-CMD-03 | RESET | opcode=1, type=0, id=0 | [0x10][0x10] |
| U-CMD-04 | SET_IDLE | opcode=5, rate=50 | [0x52][0x00] |

### 2.4 Translation Layer Tests

| ID | Test | Input | Expected |
|----|------|-------|----------|
| U-XLATE-01 | Build read seq | GET_REPORT(In,id=1) | Write cmd → Read data |
| U-XLATE-02 | Build write seq | SET_REPORT(Out,id=2) | Write data → Write cmd |
| U-XLATE-03 | Parse response | [len][data] | Strip length prefix |
| U-XLATE-04 | Overflow guard | Report > maxLen | Truncate + flag |

---

## 3. Integration Tests

### 3.1 USB Enumeration

| ID | Test | Method | Pass Criteria |
|----|------|--------|---------------|
| I-USB-01 | VID/PID | Check Device Descriptor | VID=0x0416, PID=0x5050 |
| I-USB-02 | EP configuration | Check Config Descriptor | EP0, EP1(IN), EP2(OUT) |
| I-USB-03 | Report Desc | Check HID Descriptor | Matches defined Report Desc |
| I-USB-04 | String Desc | Enumerate strings | Manufacturer, Product strings |

### 3.2 USB ↔ I²C Translation

| ID | Test | Method | Pass Criteria |
|----|------|--------|---------------|
| I-XLATE-01 | GET_REPORT passthrough | Send GET_REPORT from PC | I²C transaction seen on bus |
| I-XLATE-02 | SET_REPORT passthrough | Send SET_REPORT from PC | I²C transaction seen on bus |
| I-XLATE-03 | Input report delivery | I²C device has data | PC receives via EP1 |
| I-XLATE-04 | Output report delivery | PC sends via EP2 | Data written to I²C device |
| I-XLATE-05 | RESET sequence | Send RESET request | I²C device re-initializes |

### 3.3 Interrupt Handling

| ID | Test | Method | Pass Criteria |
|----|------|--------|---------------|
| I-INT-01 | GPIO detection | I²C device asserts interrupt | M487 detects within 1ms |
| I-INT-02 | Input read on int | Device has data | Data forwarded to PC |
| I-INT-03 | Multi-packet | Multiple Input Reports | All delivered in order |
| I-INT-04 | Idle deassert | After read | Device deasserts interrupt |

---

## 4. System Tests

### 4.1 Functional Tests

| ID | Test | Scenario | Pass Criteria |
|----|------|----------|---------------|
| S-FUNC-01 | Basic read | Read sensor via HID | Correct data at PC |
| S-FUNC-02 | Basic write | Write config via HID | Config written to device |
| S-FUNC-03 | Continuous read | Poll sensor at 10Hz | Data updates correctly |
| S-FUNC-04 | Disconnect/reconnect | Remove/reconnect I²C device | Bridge recovers |
| S-FUNC-05 | Long duration | Run 1 hour | No memory leaks, stable |

### 4.2 Error Handling Tests

| ID | Test | Scenario | Pass Criteria |
|----|------|----------|---------------|
| S-ERR-01 | I²C NACK | Device doesn't respond | PC gets error response |
| S-ERR-02 | I²C timeout | Clock stretch timeout | Bridge retries, then error |
| S-ERR-03 | Invalid report | PC sends bad report | Report rejected, no crash |
| S-ERR-04 | Buffer overflow | Report exceeds buffer | Truncated, overflow flag |
| S-ERR-05 | USB disconnect | USB cable removed | Bridge state preserved |

### 4.3 Performance Tests

| ID | Test | Method | Pass Criteria |
|----|------|--------|---------------|
| S-PERF-01 | Latency | Time from I²C data to PC | < 5ms |
| S-PERF-02 | Throughput | Report exchanges per second | ≥ 100/sec |
| S-PERF-03 | Power | Current consumption | < 100mA @ 5V |

---

## 5. Compatibility Tests

| ID | Device | Test | Pass Criteria |
|----|--------|------|---------------|
| C-DEV-01 | Touchscreen | Report data | Correct multi-touch |
| C-DEV-02 | Keyboard | LED control | LED states update |
| C-DEV-03 | Mouse | Movement data | Accurate coordinates |
| C-DEV-04 | Generic HID | Any HID-over-I²C device | Works with driver |

---

## 6. Test Tools

### 6.1 Firmware Debug

- UART printf for state machine debug
- LED indicators for error states
- OpenOCD for register-level debugging

### 6.2 PC-Side Tools

- **HIDAPI** (C) or **hidapi-sharp** (C#) for HID communication
- **USBlyzer** or **Wireshark USB** for protocol capture
- **Windows Device Console** for basic HID queries

### 6.3 I²C Bus Monitor

- Saleae logic analyzer for I²C trace
- M487 as I²C master with debug output

---

## 7. Test Deliverables

- [ ] Unit test suite (C, assert-based)
- [ ] Integration test script (Python + HIDAPI)
- [ ] I²C protocol trace captures
- [ ] USB HID descriptor dump
- [ ] Performance benchmark results
- [ ] Bug reports for failures

---

## 8. Schedule

| Phase | Duration | Tasks |
|-------|----------|-------|
| Phase 1 | Week 1 | Unit tests: I²C driver, descriptor parser |
| Phase 2 | Week 2 | Integration: USB↔I²C translation |
| Phase 3 | Week 3 | System tests: with real HID-over-I²C device |
| Phase 4 | Week 4 | Compatibility, performance, bug fixes |
