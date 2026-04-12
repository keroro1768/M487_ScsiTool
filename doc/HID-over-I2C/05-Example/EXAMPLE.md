# HID-over-I²C Bridge — Example Device: MLX90614 IR Sensor

> ⚠️ **注意：** MLX90614 本身是 **SMBus** 設備，不支援 HID-over-I²C 協議。本範例建立了一個**虛擬 HID-over-I²C 包裝層**來展示 bridge 與設備的互動模式，而非實際的 HID-over-I²C 設備範例。如需驗證 HID-over-I²C 實作，建議使用真正支援 HID-over-I²C 的設備（如特定觸控板或感測器）。

---

## 1. Example Device: IR Temperature Sensor (MLX90614)

While MLX90614 is not officially a HID-over-I²C device, this example shows the register access pattern similar to HID-over-I²C devices.

### 1.1 Device Parameters

| Parameter | Value |
|-----------|-------|
| I²C Address | 0x5A (7-bit) |
| Communication | SMBus (similar to HID-over-I²C) |
| Data format | 16-bit signed + PEC |

### 1.2 Register Map (SMBus-style)

| Register | Index | Access | Description |
|----------|-------|--------|-------------|
| RAM | 0x00–0x19 | Read | Temperature data, etc. |
| EEPROM | 0x20–0x3F | Read/Write | Configuration |
| Command | 0xE5–0xF7 | Write | Control commands |
| Read Error | 0xF8 | Read | Status |
| Sleep | 0xFF | Write | Enter sleep mode |

---

## 2. Simulated HID-over-I²C Wrapper

For testing, we create a **virtual HID-over-I²C device** that wraps MLX90614 data:

### 2.1 Virtual Device Register Map

| Register | Index | Access | Description |
|----------|-------|--------|-------------|
| HID_DESC | 0x01 | Read | 30-byte HID Descriptor |
| REPORT_DESC | 0x02 | Read | Report Descriptor |
| TEMP_DATA | 0x03 | Read | Object temperature (2 bytes) |
| AMBIENT_DATA | 0x04 | Read | Ambient temperature (2 bytes) |
| CMD | 0x05 | Write | Command register |
| DATA | 0x06 | Read/Write | Data exchange |
| CONFIG | 0x07 | Read/Write | Sensor config |

### 2.2 HID Descriptor (for Virtual Device)

```c
const uint8_t virtHIDDescriptor[30] = {
    0x1E, 0x00,       // wHIDDescLength = 30
    0x00, 0x01,       // bcdVersion = 1.00
    0x2C, 0x00,       // wReportDescLength = 44
    0x02, 0x00,       // wReportDescRegister = 2
    0x03, 0x00,       // wInputRegister = 3
    0x06, 0x00,       // wMaxInputLength = 6 (2 bytes temp + 2 bytes ambient + 2-byte length prefix)
    0x04, 0x00,       // wOutputRegister = 4
    0x04, 0x00,       // wMaxOutputLength = 4
    0x05, 0x00,       // wCommandRegister = 5
    0x06, 0x00,       // wDataRegister = 6
    0x16, 0x05,       // wVendorID = 0x0516 (Nuvoton)
    0x50, 0x50,       // wProductID = 0x5050
    0x01, 0x00,       // wVersionID = 0.01
    0x00, 0x00, 0x00, 0x00  // Reserved
};
```

### 2.3 Report Descriptor (for Virtual Device)

```
// Input Report: Temperature data
// [Length_MSB][Length_LSB][Object Temp H][Object Temp L][Ambient H][Ambient L]
// Total: 6 bytes

0x06, 0x00, 0xFF,     // Usage Page: Vendor
0x09, 0x01,            // Usage: Temperature
0xA1, 0x01,            // Collection: Application
0x85, 0x01,            //   Report ID 1
0x09, 0x01,            //   Usage: Temp Data
0x15, 0x00,            //   Logical Min: 0
0x26, 0xFF, 0x7F,     //   Logical Max: 32767
0x75, 0x10,            //   Report Size: 16
0x96, 0x04, 0x00,     //   Report Count: 4
0x81, 0x02,            //   Input: Data, Var, Abs
0xC0,                  // End Collection
```

---

## 3. Register Access Sequences

### 3.1 Read Temperature

```
1. Write Command Register: [0x02][0x11] (GET_REPORT, Input, ID=1)
2. Read Data Register: [0x04][0x00][T_obj_H][T_obj_L][T_amb_H][T_amb_L]
```

### 3.2 Set Configuration

```
1. Write Data Register: [0x03][0x00][config_H][config_L]
2. Write Command Register: [0x03][0x31] (SET_REPORT, Feature, ID=1)
```

### 3.3 Reset Device

```
1. Write Command Register: [0x01][0x10] (RESET)
```

---

## 4. Test Sequence

```c
void test_mlx90614_bridge(void)
{
    uint8_t addr = 0x5A;
    uint8_t data[6];
    uint16_t objectTemp, ambientTemp;
    
    // 1. Read HID Descriptor
    uint8_t hidDesc[30];
    I2C_ReadRegister(addr, 0x01, hidDesc, 30);
    
    // 2. Read Report Descriptor  
    uint8_t reportDesc[44];
    I2C_ReadRegister(addr, 0x02, reportDesc, 44);
    
    // 3. Reset
    I2C_WriteRegister(addr, 0x05, (uint8_t[]){0x01, 0x10}, 2);
    delay_ms(10);
    
    // 4. Get Temperature
    I2C_WriteRegister(addr, 0x05, (uint8_t[]){0x02, 0x11}, 2);
    delay_ms(5);
    I2C_ReadRegister(addr, 0x06, data, 6);
    
    objectTemp  = (data[2] << 8) | data[3];
    ambientTemp = (data[4] << 8) | data[5];
    
    // Convert: MLX90614 outputs 0.02°C per LSB
    printf("Object: %.2f C, Ambient: %.2f C\n",
           objectTemp * 0.02,
           ambientTemp * 0.02);
}
```

---

## 5. Expected Behavior

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1 | I²C scan | Device found at 0x5A |
| 2 | Read HID Desc | bcdVersion=0x0100, length=30 |
| 3 | Read Report Desc | 44 bytes received |
| 4 | Write RESET | Device re-initializes |
| 5 | GET_REPORT | Temperature data returned |
| 6 | SET_CONFIG | Configuration written |
