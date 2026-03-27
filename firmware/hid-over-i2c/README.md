# HID-over-I2C Bridge (M487)

## 概述

本專案實作 M487 微控制器上的 HID-over-I2C 橋接器，允許主機透過 USB HID 介面與 I2C 從屬裝置通訊。

### 核心功能

- **USB HID 介面**：作為 USB HID 裝置連接至主機
- **I2C 從屬模式**：作為 I2C master 控制外部 I2C 從屬晶片
- **Bridge 層**：解析 HID report 並轉發至 I2C bus

## 架構

```
┌─────────────────┐     USB HID      ┌──────────────────┐     I2C      ┌─────────────┐
│   Host PC       │◄───────────────►│   M487           │◄───────────►│ I2C Slave   │
│   (hidtool)     │   HID Report    │  USB_HID         │  I2C Read/   │ Device      │
│                 │                 │  Bridge          │  Write       │ (Sensor,    │
│                 │                 │  HID_Parser      │              │ EEPROM...)  │
│                 │                 │  I2C_Driver      │              │             │
└─────────────────┘                 └──────────────────┘              └─────────────┘
```

### 原始碼結構

```
hid-over-i2c/
├── src/
│   ├── main.c          # 系統初始化、main loop
│   ├── bridge.c/h      # HID Report ↔ I2C 轉發邏輯
│   ├── hid_parser.c/h # HID Report 解析器
│   ├── i2c_driver.c/h # I2C Master 驅動程式
│   └── usb_hid.c/h    # USB HID 裝置層 (STUB)
├── build/              # 編譯產出
├── Makefile
├── README.md
└── TEST_PLAN.md
```

### BSP 依賴

本專案依賴 Nuvoton M480 BSP：
- `Library/CMSIS/` - ARM CMSIS headers
- `Library/Device/Nuvoton/M480/` - M480 device headers & linker script
- `Library/StdDriver/src/` - 標準驅動程式 (HSUSBD, GPIO, CLK, SYS, etc.)

## 編譯

### 前置需求

1. **xPack ARM GNU Toolchain** (arm-none-eabi-gcc)
2. **Nuvoton M480 BSP** - 需設定 `BSP_DIR` 路徑

### 編譯步驟

```bash
# 使用預設路徑編譯
mingw32-make all

# 清除編譯產出
mingw32-make clean

# 燒錄（需要 Nu-Link 或相容燒錄器）
mingw32-make flash
```

### 自訂路徑

```bash
# 透過環境變數指定路徑
mingw32-make all BSP_DIR=D:/MyBSP/M480BSP XPKG_ROOT=D:/Tools/xpack-arm-none-eabi-gcc

# 或建立 Makefile.config（參考 Makefile.config.example）
```

## 燒錄

### 方式一：使用 Make flash

```bash
cd hid-over-i2c
mingw32-make flash
```

### 方式二：使用 OpenOCD 手動燒錄

```bash
openocd.exe -s <scripts> -f interface/nulink.cfg -f target/numicroM4.cfg \
  -c "init" -c "reset halt" -c "flash write_image erase build/firmware.bin 0" \
  -c "reset run" -c "shutdown"
```

### 方式三：使用 flash.bat（composite 版本）

```batch
cd firmware\composite\build_gcc
flash.bat
```

## USB 描述符

本專案目前使用 STUB 描述符，需要替換為實際的 HID 描述符：

- `usb_hid.c` 中的 `gu8DeviceDescriptor` - 裝置描述符
- `usb_hid.c` 中的 `gu8ConfigDescriptor` - 配置描述符（含 HID 描述符與端點）

## 已知限制

- `usb_hid.c` 為 STUB 實作，需要整合 BSP 的 HSUSBD 框架
- I2C driver 需要根據實際從屬裝置調整
- 橋接層 Protocol 需要根據 HID over I2C 規範完整實作

## 參考資源

- [M480 BSP](https://github.com/Nuvoton-Israel/M480_BSP)
- [USB HID 1.11 規範](https://www.usb.org/document-library/device-class-definition-hid-111)
- [HID Over I2C Protocol](https://docs.microsoft.com/en-us/windows-hardware/design/component-guides/hid-over-i2c)
