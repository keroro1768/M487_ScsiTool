# M487JIDAE 晶片規格

## 基本資訊

| 項目 | 規格 |
|------|------|
| **型號** | M487JIDAE |
| **CPU Core** | ARM 32-bit Cortex-M4 |
| **封裝** | LQFP144 |
| **I/O** | up to 115 |

---

## 記憶體

| 類型 | 大小 | 位址範圍 |
|------|------|----------|
| **SRAM** | 160KB | 0x2000_0000 起 |
| **App Flash (APROM)** | 512KB | 0x0000_0000 起 |
| **ISP Flash** | 4KB | - |
| **Data Flash** | 0~512KB (可配置) | 0x0001_F000 起 |

---

## 通訊介面

| 介面 | 數量 |
|------|------|
| UART | 6 |
| SPI | 4 |
| I2C | 3 |
| USB Device | FS/HS |
| CAN | 2 |
| I2S | 1 |
| SD Host | 2 |

---

## 類比周邊

| 類型 | 規格 |
|------|------|
| ADC | 16 通道 x 12-bit |
| DAC | 2 通道 x 12-bit |
| Comparator | 2 |
| OP/Amp | 3 |

---

## 計時器與 PWM

| 類型 | 規格 |
|------|------|
| Timer | 4 x 32-bit |
| PWM | 24 通道 |

---

## 其他功能

- **Crypto**: 支援
- **VAI (Voice Activity Intelligence)**: 支援
- **EBI**: 1
- **PDMA**: 16 通道
- **QEI**: 2
- **RTC + Vbat**: 1

---

## SWD 調試資訊

| 項目 | 數值 |
|------|------|
| **SWD IDCODE** | 0x2BA01477 |
| **Target** | NuMicro.cpu |
| **Breakpoints** | 6 |
| **Watchpoints** | 4 |
| **預設時脈** | 1000 kHz |

---

## Flash 組織 (根據實際驗證)

| 區域 | 大小 | 用途 |
|------|------|------|
| APROM | 512KB | 應用程式 |
| LDROM | - | ISP bootloader |
| Data Flash | 可配置 | 參數儲存 |

---

## 供電與功率

- **工作電壓**: 需查閱 datasheet
- **LDO**: 內建

---

## 參考資源

- Nuvoton M487 官網: https://www.nuvoton.com/products/microcontrollers/arm-cortex-m4-mcus/m480-series/m487-series/
- BSP 下載: https://www.nuvoton.com/tool-and-software/board-support-package/
- 線上文件: https://www.nuvoton.com/tool-and-software/middleware-and-drivers/
