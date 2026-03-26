# HID-over-I²C Bridge — Firmware Register Map

> Branch: `architecture/hid-over-i2c`

---

## 1. USB HID Endpoint Configuration

| Symbol | Value | Description |
|--------|-------|-------------|
| `CEP_MAX_PKT_SIZE` | 64 | Control EP0 max packet |
| `EPA_MAX_PKT_SIZE` | 64 | Interrupt IN EP1 max packet |
| `EPB_MAX_PKT_SIZE` | 64 | Interrupt OUT EP2 max packet |

> **Note:** Using 64 bytes (FS-compatible) for interrupt endpoints. Can increase to 512 for HS.

---

## 2. USB Buffer Addresses

| Symbol | Address | Size | Purpose |
|--------|----------|------|---------|
| `CEP_BUF_BASE` | 0x00 | 64 | Control EP buffer |
| `EPA_BUF_BASE` | 0x40 | 64 | Interrupt IN buffer |
| `EPB_BUF_BASE` | 0x80 | 64 | Interrupt OUT buffer |

---

## 3. I²C Device Register Definitions

These are the register indices in the **I²C HID device** (per HID over I²C spec):

| Register | Index | Access | Size | Description |
|----------|-------|--------|------|-------------|
| `I2C_REG_HID_DESC` | 0x01 (typical) | Read | 30 bytes | HID Descriptor |
| `I2C_REG_REPORT_DESC` | 0x02 (typical) | Read | Variable | Report Descriptor |
| `I2C_REG_INPUT_REPORT` | 0x03 (typical) | Read | ≤wMaxInputLength | Input Report |
| `I2C_REG_OUTPUT_REPORT` | 0x04 (typical) | Write | ≤wMaxOutputLength | Output Report |
| `I2C_REG_COMMAND` | 0x05 (typical) | Write | 2 bytes | Command opcode |
| `I2C_REG_DATA` | 0x06 (typical) | Read/Write | Variable | Command data |

> **Note:** Actual register addresses are device-specific and read from the HID Descriptor.

---

## 4. Firmware Data Structures

### 4.1 HID Descriptor Cache

```c
typedef struct {
    uint16_t wHIDDescLength;       // = 30
    uint16_t bcdVersion;            // = 0x0100
    uint16_t wReportDescLength;     // Report Descriptor size
    uint16_t wReportDescRegister;   // Report Descriptor register index
    uint16_t wInputRegister;        // Input Report register index
    uint16_t wMaxInputLength;      // Max Input Report size (incl. length field)
    uint16_t wOutputRegister;       // Output Report register index
    uint16_t wMaxOutputLength;      // Max Output Report size
    uint16_t wCommandRegister;      // Command register index
    uint16_t wDataRegister;         // Data register index
    uint16_t wVendorID;             // VID
    uint16_t wProductID;            // PID
    uint16_t wVersionID;            // Firmware version
    uint8_t  reserved[4];           // Must be 0
} I2C_HID_Descriptor_t;
```

### 4.2 Bridge Device State

```c
typedef struct {
    uint8_t  deviceAddr;           // I²C device 7-bit address
    uint8_t  connected;            // 1 = device detected
    uint8_t  protocol;              // 0 = Boot, 1 = Report protocol
    uint8_t  idleRate;              // Idle rate (4ms units)
    uint16_t maxInputLength;        // Max Input Report size
    uint16_t maxOutputLength;       // Max Output Report size
    I2C_HID_Descriptor_t hidDesc;  // Cached HID Descriptor
    uint8_t  reportDesc[512];       // Cached Report Descriptor
    uint16_t reportDescLen;
} HIDBridge_Device_t;
```

### 4.3 I²C Transaction Buffer

```c
typedef struct {
    uint8_t  addr;                 // I²C device address
    uint8_t  reg;                   // Register index
    uint8_t  cmdOpcode;            // Command opcode (if applicable)
    uint8_t  cmdParam;             // Command parameter (ReportID etc.)
    uint16_t dataLen;              // Data length
    uint8_t  data[512];            // Data buffer (incl. length prefix)
    uint8_t  flags;                // Transaction flags
} I2C_Transaction_t;

// Flags
#define I2C_FLG_READ        0x01
#define I2C_FLG_WRITE       0x02
#define I2C_FLG_TWO_PHASE   0x04  // Write data then command
#define I2C_FLG_HAS_REPORT_ID 0x08
```

---

## 5. Command Opcodes (I²C HID Device Side)

```c
#define HID_I2C_OP_RESET        0x01
#define HID_I2C_OP_GET_REPORT   0x02
#define HID_I2C_OP_SET_REPORT   0x03
#define HID_I2C_OP_GET_IDLE     0x04
#define HID_I2C_OP_SET_IDLE     0x05
#define HID_I2C_OP_GET_PROTOCOL 0x06
#define HID_I2C_OP_SET_PROTOCOL 0x07
#define HID_I2C_OP_SET_POWER    0x08

// Report Type in Command Byte 1
#define HID_I2C_RPTYPE_INPUT    0x01
#define HID_I2C_RPTYPE_OUTPUT   0x02
#define HID_I2C_RPTYPE_FEATURE  0x03
```

---

## 6. USB HID Report IDs

The bridge uses Report IDs to multiplex different reports:

| Report ID | Direction | Source/Dest | Description |
|-----------|-----------|-------------|-------------|
| 0x01 | OUT | PC → Bridge → I²C | Output Report |
| 0x02 | IN | I²C → Bridge → PC | Input Report |
| 0x03 | OUT | PC → Bridge → I²C | Feature Report (SET) |
| 0x04 | IN | I²C → Bridge → PC | Feature Report (GET) |
| 0x05 | OUT | PC → Bridge | Control Command |
| 0x06 | IN | Bridge → PC | Status Response |

---

## 7. USB Report Descriptor

The M487's USB Report Descriptor describes what the PC can send/receive:

```c
// Report Descriptor for HID-over-I²C Bridge
const uint8_t HidOverI2C_ReportDescriptor[] = {
    // Report ID 1: Output Report (PC → I²C device)
    0x06, 0x00, 0xFF,     // Usage Page: Vendor Defined
    0x09, 0x01,            // Usage: Vendor-defined
    0xA1, 0x01,            // Collection: Application
    0x85, 0x01,            //   Report ID 1
    0x09, 0x01,            //   Usage
    0x15, 0x00,            //   Logical Min: 0
    0x26, 0xFF, 0x00,     //   Logical Max: 255
    0x75, 0x08,            //   Report Size: 8
    0x96, 0x00, 0x02,      //   Report Count: 512
    0x91, 0x02,            //   Output: Data, Var, Abs
    0xC0,                  // End Collection

    // Report ID 2: Input Report (I²C device → PC)
    0x06, 0x00, 0xFF,
    0x09, 0x02,
    0xA1, 0x01,
    0x85, 0x02,
    0x09, 0x02,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x96, 0x00, 0x02,
    0x81, 0x02,            // Input: Data, Var, Abs
    0xC0,

    // Report ID 3: Feature Output (PC → I²C device)
    0x06, 0x00, 0xFF,
    0x09, 0x03,
    0xA1, 0x01,
    0x85, 0x03,
    0x09, 0x03,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x96, 0x00, 0x02,
    0x91, 0x02,
    0xC0,

    // Report ID 4: Feature Input (I²C device → PC)
    0x06, 0x00, 0xFF,
    0x09, 0x04,
    0xA1, 0x01,
    0x85, 0x04,
    0x09, 0x04,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x96, 0x00, 0x02,
    0x81, 0x02,
    0xC0,
};
```

---

## 8. Memory Map

```
SRAM: 0x20000000
│
├── 0x20000000: I2C_TxBuffer[512]     // I²C TX buffer
├── 0x20000200: I2C_RxBuffer[512]     // I²C RX buffer  
├── 0x20000400: HID_DescCache[30]     // I²C device HID Descriptor
├── 0x20000420: ReportDescCache[512]  // I²C device Report Descriptor
├── 0x20000620: BridgeState           // HIDBridge_Device_t
├── 0x20000700: InputReportBuf[512]   // Current Input Report
├── 0x20000900: OutputReportBuf[512]  // Current Output Report
├── 0x20000B00: USB_EP0Buf[64]       // USB EP0 buffer
├── 0x20000B40: USB_EP1Buf[64]       // USB EP1 (Interrupt IN)
├── 0x20000B80: USB_EP2Buf[64]       // USB EP2 (Interrupt OUT)
└── 0x20001000: (available)
```

---

## 9. I²C Register Access Macros

```c
// Build command register value
#define HID_I2C_CMD(opcode, reportType, reportId) \
    (((opcode) << 4) | ((reportType) << 2) | ((reportId) & 0x0F))

// Parse command register
#define HID_I2C_CMD_OPCODE(cmd)     (((cmd) >> 4) & 0x0F)
#define HID_I2C_CMD_RPTYPE(cmd)     (((cmd) >> 2) & 0x03)
#define HID_I2C_CMD_REPORTID(cmd)    ((cmd) & 0x0F)

// Read length prefix from I²C response
#define I2C_READ_LEN(p)    (((uint16_t)(p)[0] << 8) | (p)[1])
```
