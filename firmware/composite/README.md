# M487 USB Composite Device Firmware

USB Composite Device: **Mass Storage (MSC)** + **HID I2C Bridge**

## 架構

```
┌─────────────────────────────────────────────────────────────┐
│  M487 USB Composite Device                                  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ Interface 0: USB Mass Storage (MSC)                  │  │
│  │   Class: Mass Storage (0x08)                         │  │
│  │   Subclass: SCSI Transparent (0x06)                  │  │
│  │   Protocol: Bulk-Only Transport (0x50)             │  │
│  │                                                     │  │
│  │   EP1 IN (Bulk):  Read data from Flash             │  │
│  │   EP2 OUT (Bulk): Write data to Flash               │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ Interface 1: HID I2C Bridge                         │  │
│  │   Class: HID (0x03)                                 │  │
│  │                                                     │  │
│  │   EP3 IN (Interrupt): HID Input Reports            │  │
│  │   EP5 OUT (Bulk): HID Output Reports (Commands)    │  │
│  └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## USB 參數

| 參數 | 值 |
|------|-----|
| VID | 0x04F3 (Nuvoton, registered) |
| PID | 0x0732 (Composite Device, registered) |
| USB Speed | High-Speed (480 Mbps) |
| Max Power | 100 mA |

## HID I2C Bridge Reports

### Report 0x01: I2C Write (Host → Device)

| Offset | Size | 說明 |
|--------|------|------|
| 0 | 1 | Report ID = 0x01 |
| 1 | 1 | I2C Slave Address (7-bit) |
| 2 | 1 | Length (要寫入的位元組數) |
| 3 | N | Write Data (最多 60 bytes) |

### Report 0x02: I2C Read (Host → Device, Device → Host)

**Request (Host → Device)**:
| Offset | Size | 說明 |
|--------|------|------|
| 0 | 1 | Report ID = 0x02 |
| 1 | 1 | I2C Slave Address (7-bit) |
| 2 | 1 | Length (要讀取的位元組數) |

**Response (Device → Host)**:
| Offset | Size | 說明 |
|--------|------|------|
| 0 | 1 | Report ID = 0x02 |
| 1 | 1 | Status (0=OK, 1=Error) |
| 2 | 1 | Length (實際讀取的位元組數) |
| 3 | N | Read Data |

### Report 0x03: I2C Write+Read (Host → Device, Device → Host)

用於讀取需要先寫入暫存器位址的 I2C 感測器。

**Request (Host → Device)**:
| Offset | Size | 說明 |
|--------|------|------|
| 0 | 1 | Report ID = 0x03 |
| 1 | 1 | I2C Slave Address (7-bit) |
| 2 | 1 | Write Length |
| 3 | 1 | Read Length |
| 4 | N | Write Data |

**Response (Device → Host)**:
| Offset | Size | 說明 |
|--------|------|------|
| 0 | 1 | Report ID = 0x03 |
| 1 | 1 | Status (0=OK, 1=Error) |
| 2 | 1 | Length (實際讀取的位元組數) |
| 3 | N | Read Data |

## 硬體接線

| M487 Pin | Function | Connect to |
|----------|----------|------------|
| PE2 | UI2C0 CLK | I2C Slave SCL |
| PE3 | UI2C0 DAT0 | I2C Slave SDA |
| 3.3V | Power | I2C Slave VCC |
| GND | Ground | I2C Slave GND |

## 檔案列表

| 檔案 | 說明 |
|------|------|
| `main.c` | 主程式，初始化和 main loop |
| `usb_descriptors.c` | USB Descriptors (Device, Config, HID Report) |
| `hid_i2c.c` | HID I2C Bridge 實作 |
| `hid_i2c.h` | HID I2C Bridge 介面 |
| `i2c_control.c` | I2C 底層驅動 (blocking, with NACK retry) |
| `i2c_control.h` | I2C 底層驅動介面 |
| `itm.c` / `itm.h` | ITM/SWO trace system |
| `msc_debug.c` / `msc_debug.h` | MSC Vendor Debug Channel (CDB 0xC0-0xFF) |
| `uart_debug.c` / `uart_debug.h` | UART structured debug log |

## 建置方式

此 firmware 需要整合進 Nuvoton M480 BSP 環境：

1. 複製 `composite/` 資料夾到 M480BSP 專案
2. 使用 Keil MDK 或 IAR EWARM 開啟
3. 確認 BSP 版本支援 HSUSBD (High-Speed USB)
4. 編譯並燒錄到 M487

## Windows 主機端工具

請參考上層資料夾的 `src/main.cpp`，已支援 HID I2C 命令。

或使用 `hidconsole.exe` (獨立工具，尚未實作)。

## 預期行為

1. 插入 M487 後，Windows 會識別為：
   - USB Storage (可掛載磁碟機, RAM Disk 64KB)
   - HID 相容裝置 (HID I2C Bridge)

2. HID I2C Bridge 可以直接通訊，不需要額外驅動

3. MSC Vendor Debug Channel (CDB 0xC0-0xFF): 可透過 `msc_debug.exe` 工具讀取裝置資訊、記憶體、CPU 暫存器

## 限制

- MSC RAM Disk 大小：64 KB (128 sectors × 512 bytes, SRAM)
- HID I2C 傳輸限制：
  - Write: 最多 60 bytes (不含 Report ID + addr + len header)
  - Read: 最多 62 bytes (不含 Report ID + status + len)
  - Write+Read: Write 最多 60 bytes, Read 最多 62 bytes

## 授權

Nuvoton M480 BSP License
