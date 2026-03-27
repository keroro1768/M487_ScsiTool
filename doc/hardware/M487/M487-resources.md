# Nuvoton M487 USB I3C Bridge 專案資源

## 晶片規格

### M487 系列概述

| 型號 | 主要特色 |
|------|----------|
| M487 Ethernet Series | Ethernet, USB 2.0 HS OTG, Crypto |
| M484 USB HS OTG Series | USB 2.0 HS OTG |
| M485 Crypto Series | 加密功能 |

### 關鍵規格

- **核心**: ARM Cortex-M4 @ 192MHz
- **Flash**: 512KB / 2MB
- **SRAM**: 160KB + 32KB
- **USB**: USB 2.0 HS OTG + FS OTG
- **I2C**: 最多 4 組
- **I3C**: 支援（從 M480 系列繼承）

---

## 官方文件

### 資料手冊

| 文件 | 連結 | 說明 |
|------|------|------|
| M480 Series Datasheet (Rev 3.08) | [PDF](https://www.nuvoton.com/export/resource-files/en-us--DS_M480_Series_EN_Rev3.08.pdf) | 完整規格書 |
| M480 Series Datasheet (Rev 3.07) | [PDF](https://www.nuvoton.com/export/resource-files/en-us--DS_M480_Series_EN_Rev3.07.pdf) | 舊版 |

### 開發板手冊

| 文件 | 連結 | 說明 |
|------|------|------|
| NuMaker-IoT-M487 User Manual | [PDF](https://www.nuvoton.com/resource-files/UM_NuMaker-IoT-M487_User_Manual_EN_Rev1.01.pdf) | IoT 開發板 |
| NuMaker-PFM-M487 User Manual | [PDF](https://www.nuvoton.com/export/resource-files/UM_NuMaker-PFM-M487_User_Manual_EN_Rev1.01.pdf) | 平台模組 |
| NuMaker-ETM-M487 User Manual | [PDF](https://www.nuvoton.com/export/resource-files/UM_NuMaker-ETM-M487_User_Manual_EN_Rev1.01.pdf) | ETM 開發板 |
| NuMaker-HMI-M487 User Manual | [PDF](https://www.nuvoton.com/export/resource-files/en-us--UM_NuMaker-HMI-M487_User_Manual_EN_Rev_2.00.pdf) | HMI 開發板 |

---

## 範例程式碼

### USB 範例

| 範例                   | 說明                 | 連結                                                                                       |
| -------------------- | ------------------ | ---------------------------------------------------------------------------------------- |
| USB Device HID       | USB 轉 SPI 橋接       | [NuForum](https://forum.nuvoton.com/en/mcu-nudeveloper-ecosystem/bsp-example-code/11197) |
| USB Composite Device | 複合裝置 (UAC+HID+MSC) | [NuForum](https://forum.nuvoton.com/en/mcu-nudeveloper-ecosystem/bsp-example-code/10972) |
| USB CDC              | 虛擬序列埠              | SDK 內建                                                                                   |
| USB Mass Storage     | 大容量儲存              | SDK 內建                                                                                   |

### I2C 範例

| 範例 | 說明 | 連結 |
|------|------|------|
| I2C EEPROM Access | 存取 EEPROM | [NuForum](http://forum.nuvoton.com/viewtopic.php?t=8274) |
| I2C Master | 主模式下讀寫 | SDK 內建 |

### USB OTG 電路

```
┌─────────────────────────────────────┐
│         USB HS OTG 電路              │
├─────────────────────────────────────┤
│  M487 USB D+ (PB.12)  ──────┐        │
│  M487 USB D- (PB.13)  ──────┤        │
│                      ▼    ▼        │
│                     ┌─────┐         │
│                     │ USB │         │
│                     │ ID  │         │
│                     └─────┘         │
└─────────────────────────────────────┘
```

---

## SDK 與開發工具

### BSP (Board Support Package)

| 資源 | 連結 |
|------|------|
| Nuvoton GitHub | https://github.com/OpenNUVOTON |
| NuMaker IoT M487 BSP | https://github.com/wosayttn/sdk-bsp-numaker-iot-m487 |
| Mbed Platform | https://os.mbed.com/platforms/NUMAKER-IOT-M487/ |

### 開發環境

| 工具 | 說明 |
|------|------|
| Keil MDK | ARM MDK |
| IAR EWARM | IAR Embedded Workbench |
| NuEclipse | Nuoton 專用 Eclipse |
| GCC | ARM GCC |

---

## USB HID 範例程式碼結構

### 初始化

```c
void USBD_Init(void)
{
    /* 開啟 USB 時脈 */
    CLK_EnableModuleClock(USBD_MODULE);

    /* 設定 USB 為 Device 模式 */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTG;

    /* 初始化 USBD 驅動 */
    USBD_Open(&gUsbdParam, HID_ClassRequest, NULL);
}
```

### Endpoint 設定

```c
/* HID IN Endpoint (Endpoint 1) */
USBD_SET_EP(1, USBD_EP_CFG_DIR_IN, USBD_EP_CFG_TYPE_INT, 64);

/* HID OUT Endpoint (Endpoint 2) */
USBD_SET_EP(2, USBD_EP_CFG_DIR_OUT, USBD_EP_CFG_TYPE_INT, 64);
```

### 資料傳輸

```c
/* 傳送資料到主機 */
void HID_SendReport(uint8_t *buf, uint32_t len)
{
    USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(1)), buf, len);
    USBD_SET_PAYLOAD_LEN(1, len);
}

/* 從主機接收資料 */
void HID_GetReport(void)
{
    uint32_t len = USBD_GET_PAYLOAD_LEN(2);
    if (len > 0) {
        USBD_MemCopy(gRxBuf, (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(2)), len);
    }
}
```

---

## I2C 範例程式碼

### I2C Master 傳送

```c
void I2C_Master_Transmit(uint8_t u8SlaveAddr, uint8_t *pu8Buf, uint32_t u32BufLen)
{
    /* 設定為 Master 模式 */
    I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);

    /* 傳送 START */
    I2C_START(I2C0);
    while (!(I2C0->CTL & I2C_CTL_SI));

    /* 傳送 SLA+W */
    I2C_SET_DATA(I2C0, u8SlaveAddr << 1);
    I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
    while (!(I2C0->CTL & I2C_CTL_SI));

    /* 傳送資料 */
    for (uint32_t i = 0; i < u32BufLen; i++) {
        I2C_SET_DATA(I2C0, pu8Buf[i]);
        I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
        while (!(I2C0->CTL & I2C_CTL_SI));
    }

    /* 傳送 STOP */
    I2C_STOP(I2C0);
    while (I2C0->CTL & I2C_CTL_SI);
}
```

### I2C Master 接收

```c
void I2C_Master_Receive(uint8_t u8SlaveAddr, uint8_t *pu8Buf, uint32_t u32BufLen)
{
    /* 傳送 START */
    I2C_START(I2C0);
    while (!(I2C0->CTL & I2C_CTL_SI));

    /* 傳送 SLA+W */
    I2C_SET_DATA(I2C0, (u8SlaveAddr << 1) | 0x00);
    I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI);
    while (!(I2C0->CTL & I2C_CTL_SI));

    /* 接收資料 */
    for (uint32_t i = 0; i < u32BufLen; i++) {
        if (i == u32BufLen - 1) {
            I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI | I2C_CTL_AA);  // NACK
        } else {
            I2C_SET_CONTROL_REG(I2C0, I2C_CTL_SI | I2C_CTL_AA);  // ACK
        }
        while (!(I2C0->CTL & I2C_CTL_SI));
        pu8Buf[i] = I2C_GET_DATA(I2C0);
    }

    /* 傳送 STOP */
    I2C_STOP(I2C0);
    while (I2C0->CTL & I2C_CTL_SI);
}
```

---

## USB OTG 功能切換

### 硬體線路

```
USB ID Pin:
- 短路到 GND → 為 OTG A 裝置 (供電)
- 浮空 → 為 OTG B 裝置 (受電)
```

### 軟體控制

```c
void USB_OTG_Init(void)
{
    /* 啟用 USB PHY */
    SYS->USBPHY |= SYS_USBPHY_OTG;

    /* 檢測 ID pin 狀態 */
    if (USBD_IS_ID_ACTIVE()) {
        /* OTG A 裝置 */
        USBD_SetRole(USBD_ROLE_OTG | USBD_ROLE_HOST);
    } else {
        /* OTG B 裝置 / 一般 Device */
        USBD_SetRole(USBD_ROLE_DEVICE);
    }
}
```

---

## I3C 相容性

**⚠️ 重要修正（2026-03-26）：**

M480/M487 系列**沒有原生 I3C 硬體控制器**。根據 BSP register header 確認：

- ✅ 有 `i2c_reg.h` — 標準 I2C（SMBus/PMBus 相容）
- ✅ 有 `ui2c_reg.h` — USCI 介面（可設為 I2C 模式）
- ❌ **沒有 `i3c_reg.h`** — 沒有 I3C 控制器

| 功能                | I2C  | I3C（需軟體模擬） |
| ----------------- | ---- | ---------- |
| 標準模式              | ✅ 原生 | ⚠️ 軟體模擬    |
| 快速模式              | ✅ 原生 | ⚠️ 軟體模擬    |
| 高速模式              | ✅ 原生 | ❌ 不支援      |
| CCC commands      | ❌    | ⚠️ 軟體模擬    |
| In-band interrupt | ❌    | ⚠️ 軟體模擬    |
| 動態位址分配            | ❌    | ⚠️ 軟體模擬    |

M487 只能透過 **USB HID 橋接外部 I3C 控制晶片**（如 STM32H5、I3C Hub）來實現 I3C 功能，不能作為 I3C Host。

---

## 參考資源

| 資源 | 連結 |
|------|------|
| NuForum USB 範例 | https://forum.nuvoton.com/en/mcu-nudeveloper-ecosystem/bsp-example-code/ |
| FreeRTOS M487 | https://docs.aws.amazon.com/freertos/latest/userguide/getting-started-nuvoton-m487.html |
| Zephyr M487 | https://docs.zephyrproject.org/latest/boards/nuvoton/numaker_pfm_m487/doc/index.html |
| Nuvoton GitHub | https://github.com/OpenNUVOTON |

---

*建立時間: 2026-03-25*
