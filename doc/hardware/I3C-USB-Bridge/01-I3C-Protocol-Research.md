# I3C 通訊協議研究 - USB to I3C Bridge

**專案：** USB 轉 I3C BridgeBoard  
**研究日期：** 2026-03-21  
**目標：** 實作前的深入技術研究

---

## 1. I3C 協議概述

### 1.1 什麼是 I3C？

**I3C (Improved Inter-Integrated Circuit)** 是由 MIPI Alliance 開發的改良型 I2C 匯流排標準。

| 特性 | I2C | I3C |
|------|-----|-----|
| 最大時脈 | 400kHz (Fast Mode) | 12.5 MHz |
| 地址分配 | 靜態（需手動設定） | 動態（自動分配） |
| 中斷 | 需額外線路 | In-Band Interrupt (IBI) |
| 功耗 | 較高 | 較低 |
| 熱插拔 | 不支援 | 支援 Hot-Join |

### 1.2 I3C 基本架構

```
┌─────────────┐         ┌─────────────┐
│  I3C Host   │◄───────►│ I3C Target  │
│ Controller  │  SCL    │   Device    │
│             │  SDA    │ (Sensor)    │
└─────────────┘         └─────────────┘
```

**關鍵訊號：**
- **SCL**：時脈線
- **SDA**：資料線
- **SDA/SCL 都有內建上拉電阻**

---

## 2. I3C 協議特性

### 2.1 通訊模式

| 模式 | 速率 | 說明 |
|------|------|------|
| SDR (Single Data Rate) | ~12.5 MHz | 預設模式 |
| HDR-DDR | ~25 MHz | 雙倍資料率 |
| HDR-TSL | ~30 MHz | 三星期的資料傳輸 |
| HDR-BRL | ~39 MHz | 脈衝回應模式 |

### 2.2 CCC (Common Command Codes)

I3C 定義了一組標準命令碼用於匯流排管理：

| CCC 代碼 | 功能 |
|----------|------|
| 0x00 | ENEC (Enable Events) |
| 0x01 | DISEC (Disable Events) |
| 0x02 | ENTAS0/1/2/3 (Enter Activity State) |
| 0x06 | SETDASA (Set Dynamic Address from Static) |
| 0x07 | SETNEWDA (Set New Dynamic Address) |
| 0x08 | GETMXDS (Get Max Data Speed) |
| 0x09 | GETPID (Get Provisional ID) |
| 0x0A | GETBCR (Get Bus Characteristic Register) |
| 0x0B | GETDCR (Get Device Characteristic Register) |

### 2.3 I3C 特色功能

1. **動態位址分配 (Dynamic Address Assignment)**
   - 控制器自動分配位址，減少衝突
   
2. **In-Band Interrupt (IBI)**
   - 目標裝置可直接透過 SDA/SCL 發出中斷
   
3. **Hot-Join**
   - 支援裝置熱插拔
   
4. **Multi-Controller**
   - 支援多個主控器

---

## 3. USB I3C Device Class

### 3.1 標準發布

**USB-IF 發布了 USB I3C Device Class Specification：**
- Version 1.0: 2022年1月
- Version 1.1: 2023年1月

**官網下載：** https://www.usb.org/sites/default/files/USB%20I3C%20Device%20Class%20Revision%201.1.pdf

### 3.2 規範重點

此規範定義了：
- 如何透過 USB 連接 I3C/I3C Basic 匯流排
- USB 與 I3C 之間的通訊協定
- 標準化的驅動程式介面

**優勢：**
- 無需自訂驅動程式
- 跨平台支援（Windows、Linux、macOS）
- 可直接與現有 USB 系統整合

---

## 4. 現有解決方案分析

### 4.1 商用 USB-I3C Bridge 產品

| 廠商 | 型號 | 介面 | 支援 |
|------|------|------|------|
| Renesas | USB-I3C Converter | USB-C | UART, I2C, I3C |
| Binho | nPA-Pro | USB | I3C, I2C, SPI |
| FTDI | FT4222H | USB 2.0 | I2C/SPI (需確認 I3C) |

### 4.2 內建 I3C 的 MCU

| 廠商 | 系列 | CPU 架構 | I3C 支援 |
|------|------|----------|----------|
| STMicroelectronics | STM32H5 | ARM Cortex-M7 | 完整支援 |
| STMicroelectronics | STM32U5 | ARM Cortex-M33 | 完整支援 |
| Microchip | PIC18 | 8-bit AVR | 有限支援 |
| NXP | i.MX RT | ARM Cortex-M7 | 需確認 |
| Nordic | nRF52 | ARM Cortex-M4 | 有限支援 |

### 4.3 評估建議

**首選方案：STM32H5 系列**
- ARM Cortex-M7 @ 280MHz
- 完整 I3C 支援 (MIPI I3C v1.0)
- USB Device/Host 內建
- 大量參考代碼和函式庫
- 價格合理

**參考應用筆記：**
- ST AN5879: Introduction to I3C for STM32H5

---

## 5. 實作架構建議

### 5.1 目標硬體架構

```
┌──────────────────┐      USB      ┌──────────────────┐
│   Host PC        │◄─────────────►│  STM32H5 MCU     │
│   (Python/API)   │               │  (USB Device)    │
└──────────────────┘               └────────┬─────────┘
                                           │
                                           │ I3C Bus
                                           ▼
                                    ┌──────────────┐
                                    │ I3C Sensors  │
                                    │ (IMU, Temp,  │
                                    │  Pressure)   │
                                    └──────────────┘
```

### 5.2 軟體層次

| 層次 | 技術選擇 |
|------|----------|
| USB Layer | TinyUSB (開源) |
| I3C Driver | STM32 HAL / LL |
| API | USB CDC (Virtual COM) |
| Host Software | Python (pyserial) |

### 5.3 開發工具推薦

- **IDE:** STM32CubeIDE (免費)
- **燒錄器:** ST-Link V3
- **協定分析:** Totalphase Beagle I3C/SPI

---

## 6. 研究結論

### 6.1 可行性評估

| 項目 | 狀態 | 備註 |
|------|------|------|
| I3C 協議成熟度 | ✅ 成熟 | MIPI 標準化 |
| USB I3C Class | ✅ 可用 | v1.1 已發布 |
| 硬體選擇 | ✅ ARM STM32H5 | 完整支援 |
| 開源參考 | ⚠️ 有限 | 需自行開發 |
| 開發難度 | 中等 | 需熟悉 USB 和 I3C |

### 6.2 建議行動項目

1. **購買評估板** - STM32H5 Nucleo-144
2. **研究 USB I3C Class 規範** - 下載並閱讀 spec
3. **評估 TinyUSB 整合** - 檢查 I3C Device Class 支援
4. **建立基本通訊** - 先實現 USB CDC + I3C 基本讀寫
5. **擴展功能** - 支援 HDR 模式、IBI 等進階功能

---

## 7. 參考資源

### 官方標準
- MIPI I3C Spec: https://www.mipi.org/specifications/i3c-sensor-specification
- USB I3C Device Class: https://www.usb.org/sites/default/files/USB%20I3C%20Device%20Class%20Revision%201.1.pdf

### 技術文檔
- Linux Kernel I3C: https://docs.kernel.org/driver-api/i3c/protocol.html
- ST AN5879 (STM32H5 I3C): https://www.st.com/resource/en/application_note/an5879

### 開發工具
- Renesas USB Converter: https://www.renesas.com/en/applications/industrial/industrial-automation/universal-usb-converter
- STM32H5 Nucleo: https://www.st.com/en/evaluation-tools/stm32h5-nucleo.html

---

*此文件為初步技術研究，後續將根據實際開發經驗更新*
