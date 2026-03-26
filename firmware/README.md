# M487 USB Storage + I2C Bridge Firmware

本資料夾包含可整合進 Nuvoton M480 BSP 的 I2C 控制程式碼。

## 檔案

- `i2c_control.h` - I2C 控制函式介面
- `i2c_control.c` - I2C 驅動實作（UI2C0，PE2=CLK, PE3=DAT0）

## 整合步驟

### 1. 複製檔案

將 `i2c_control.h` 和 `i2c_control.c` 複製到您的 M480BSP 專案資料夾，例如：
```
D:\AiWorkSpace\KM\M480BSP\SampleCode\StdDriver\USBD_Mass_Storage_Flash\
```

### 2. 修改 MassStorage.c

在 `MSC_ProcessCmd()` 函式中，找到 `default:` case，在 unsupported command 處理之前加入 vendor command 處理：

```c
case UFI_VENDOR_READ:  // 0xC0
{
    uint8_t response[65];  // Max 64 bytes + status
    uint16_t respLen = 0;
    
    // Initialize I2C on first use
    static int i2c_initialized = 0;
    if (!i2c_initialized) {
        I2C_Init(I2C_SPEED_STANDARD);  // 100 kHz
        i2c_initialized = 1;
    }
    
    int32_t ret = ProcessVendorCommand(g_sCBW.au8Data, 
                                        (uint8_t*)STORAGE_DATA_BUF,
                                        response, &respLen);
    
    // Send response via EP2
    if (respLen > 0) {
        uint32_t u32CopyLen = (respLen < EP2_MAX_PKT_SIZE) ? respLen : EP2_MAX_PKT_SIZE;
        USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + g_u32BulkBuf1), response, u32CopyLen);
        USBD_SET_PAYLOAD_LEN(EP2, u32CopyLen);
        g_u32Length = respLen - u32CopyLen;
        g_u32Address = STORAGE_DATA_BUF;
        if (g_u32Length > 0) {
            memcpy((uint8_t*)STORAGE_DATA_BUF, response + u32CopyLen, g_u32Length);
            g_u8BulkState = BULK_IN;
            MSC_Read();
        } else {
            g_sCSW.dCSWDataResidue = 0;
            g_sCSW.bCSWStatus = (ret == 0) ? 0 : 1;
            g_u8BulkState = BULK_IN;
            MSC_AckCmd();
        }
    } else {
        g_sCSW.dCSWDataResidue = g_sCBW.dCBWDataTransferLength;
        g_sCSW.bCSWStatus = 1;  // Command failed
        g_u8BulkState = BULK_IN;
        MSC_AckCmd();
    }
    return;
}
```

### 3. 在 massstorage.h 中加入

```c
#include "i2c_control.h"
```

### 4. 在 massstorage.h 中加入 vendor opcode

```c
#define UFI_VENDOR_READ    0xC0
```

### 5. 在 MSC_Init() 中初始化 I2C

```c
void MSC_Init(void)
{
    // ... existing initialization ...
    
    // Initialize I2C at 100 kHz
    I2C_Init(I2C_SPEED_STANDARD);
}
```

## Vendor Command (0xC0) 規格

### Sub-cmd 0x00: Get Device String
- **CDB**: `[0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]`
- **Response**: `"ELAN-USB-I2C-BRIDGE-V0.1\0"` (24 bytes)

### Sub-cmd 0x01: I2C Write
- **CDB**: `[0xC0, 0x01, slave_addr, len_H, len_L, 0x00, 0x00, 0x00, 0x00, 0x00]`
- **Data Stage**: `len` bytes to write
- **Response**: `[status]` (1 byte, 0=OK, 1=Error)

### Sub-cmd 0x02: I2C Read
- **CDB**: `[0xC0, 0x02, slave_addr, len_H, len_L, 0x00, 0x00, 0x00, 0x00, 0x00]`
- **Response**: `[status, data[0], data[1], ...]` (1 + len bytes)

### Sub-cmd 0x03: I2C Write+Read (Repeated Start)
- **CDB**: `[0xC0, 0x03, slave_addr, wlen_H, wlen_L, rlen_H, rlen_L, 0x00, 0x00, 0x00]`
- **Data Stage**: `wlen` bytes write data
- **Response**: `[status, read_data[0], read_data[1], ...]` (1 + rlen bytes)

## I2C 硬體接線

| M487 Pin | Function | Connect to |
|----------|----------|------------|
| PE2 | UI2C0 CLK | I2C Slave SCL |
| PE3 | UI2C0 DAT0 | I2C Slave SDA |
| 3.3V | Power | I2C Slave VCC |
| GND | Ground | I2C Slave GND |

## 韌體建置

1. 使用 Keil MDK 開啟 `USBD_Mass_Storage_Flash.uvprojx`
2. 將 `i2c_control.c` 加入專案
3. 編譯並燒錄到 M487

## Windows 主機端工具

M487_ScsiTool 已支援 I2C 命令，請參考上層資料夾的 `src/main.cpp`。
