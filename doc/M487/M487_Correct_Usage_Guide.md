# M487 正確使用方法研究

> 最後更新：2026-03-28
> 來源：Nuvoton 官網、GitHub BSP、論壇、AWS FreeRTOS 文件

---

## 1. M487 晶片概述

| 項目 | 規格 |
|------|------|
| 核心 | ARM Cortex-M4F |
| 時脈 | 192 MHz（max）|
| SRAM | 160 KB |
| Flash | 512 KB |
| USB | USB 2.0 High-Speed OTG（內建 PHY）|
| USB 介面 | USB Device + USB Host（支援 HSUSBD）|

**你使用的型號：M487JIDAE**（根據 `firmware/composite/main.c` 中的 `#if defined(__M487JIDAE__)`）

---

## 2. M487 USB 架構

M487 內建 **USB 2.0 High-Speed OTG** 控制器，有兩種模式：

### USB Host vs Device

| 模式 | 用途 | 你需要這個 |
|------|------|-----------|
| **USB Device** | M487 作為 USB 裝置（被電腦辨識）| ✅ |
| USB Host | M487 作為 USB 主機（接隨身碟等）| ❌ |

### HSUSBD（High-Speed USB Device）

M487 的 USB Device 控制器代號為 **HSUSBD**，有獨立的周邊驅動程式。

**BSP 位置：**
```
D:\AiWorkSpace\KM\M480BSP\StdDriver\src\hsusbd.c
D:\AiWorkSpace\KM\M480BSP\StdDriver\inc\hsusbd.h
```

---

## 3. Nuvoton BSP 標準範例

BSP 中與 USB 相關的範例（按類型分）：

### USB Device 範例

| 範例資料夾 | 說明 | 與你的專案相關性 |
|-----------|------|----------------|
| `HSUSBD_HID_Transfer` | HID Interrupt IN/OUT 傳輸 | ✅ 參考 |
| `HSUSBD_HID_Transfer_And_MSC` | HID + MSC 複合裝置 | ✅ **直接相關** |
| `HSUSBD_Mass_Storage_SRAM` | USB MSC（SRAM 作為儲存）| ✅ 參考 |
| `HSUSBD_Mass_Storage_SD` | USB MSC + SD 卡 | 一般參考 |
| `HSUSBD_VENDOR_LBK` | Vendor 自訂命令 | ✅ 參考 |
| `HSUSBD_VCOM_SerialEmulator` | USB 轉 UART | 一般參考 |

### 正確的參考順序（推薦）

```
1. HSUSBD_HID_Transfer_And_MSC  ← 最接近你的需求
   ↓
2. HSUSBD_VENDOR_LBK            ← MSC 自訂命令參考
   ↓  
3. HSUSBD_HID_Transfer          ← HID 底層參考
```

---

## 4. USB 複合裝置實作方法

### 你的目標：MSC + HID I2C Bridge

這是一個 **USB 複合裝置**，包含兩個獨立的 USB 介面：

```
USB Composite Device（M487）
├── Interface 0：HID（Interrupt EP）
│   ├── EP1 IN（HID Input Report）
│   └── EP2 OUT（HID Output Report）
└── Interface 1：MSC（Bulk EP）
    ├── EP3 IN（MSC Data）
    └── EP4 OUT（MSC Data）
```

### Nuvoton 官方複合裝置範例

根據 Nuvoton 論壇（2022/07/13）的一個範例：

> *"This example code implements a composite device that is made up of three devices — UAC (Audio), HID (Interrupt Transfer), and MSC (SD card)"*

**下載連結：**
```
https://www.nuvoton.com/resource-download.jsp?tp_GUID=EC01-2022070601090438
```

**BSP 版本需求：** `M480_Series_BSP_CMSIS_V3.05.001` 以上

---

## 5. 正確的驅動程式使用方式

### 使用 BSP 標準驅動，不要自己從零寫

| 層次 | 說明 |
|------|------|
| **你的應用程式** | `main.c` 中的 MSC+HID 處理邏輯 |
| **BSP 驅動** | `hsusbd.c`, `usbd.c`, `fmc.c`, `gpio.c`... |
| **硬體抽象層** | Nuvoton 已經幫你寫好了 |

**你需要做的：** 呼叫 BSP 提供的 API，不要直接碰觸寄存器。

### BSP 驅動分類

| 類別 | 驅動檔案 | 說明 |
|------|---------|------|
| USB Device | `hsusbd.c`, `usbd.c` | USB 控制器 |
| I2C | `i2c.c`, `usci_i2c.c` | UI2C0（你的 I2C Bridge）|
| Clock | `clk.c` | 系統時脈設定 |
| Flash | `fmc.c` | Flash ISP 燒錄 |
| Debug | `uart.c` | UART 除錯輸出 |

---

## 6. 正確的開發流程

### Step 1：用 BSP 範例驗證硬體

**在直接用 OpenOCD 燒錄自己的程式前，先燒錄官方範例確認硬體正常：**

1. 燒錄 `HSUSBD_HID_Transfer_And_MSC` 範例
2. 接上 USB，看電腦是否出現 HID + MSC 複合裝置
3. 如果不行，先確認這個可以，再燒自己的程式

**原因：** 排除硬體問題。

### Step 2：修改官方範例，不要從零開始

把你的功能加到 `HSUSBD_HID_Transfer_And_MSC` 專案裡，而不是另外建立一個新專案。

**好處：**
- USB 描述符、端點設定都已經正確
- 中斷處理、列舉流程都已經驗證過
- 減少除錯時間

### Step 3：用 GCC 編譯

```
D:\AiWorkSpace\KM\M480BSP\Library\StdDriver\src\   ← 驅動程式碼
D:\AiWorkSpace\KM\M480BSP\SampleCode\StdDriver\    ← 範例
```

BSP 已經支援 GCC：
- `system_M480.c`
- `startup_M480.S`
- CMSIS header files

### Step 4：用 OpenOCD + Nu-Link 燒錄

**燒錄前確認：**
1. Zadig 設定好 WinUSB 驅動
2. 關閉所有 Keil/NuStudio
3. 管理員執行 OpenOCD

---

## 7. 你的韌體與 BSP 的關係

```
你的 firmware/composite/
├── main.c              → 系統初始化（CLK_Init, USB_Init）
├── hid_i2c.c           → HID + MSC 處理邏輯
├── i2c_control.c        → UI2C0 驅動（I2C Bridge）
├── usb_descriptors.c    → USB 描述符（很重要！）
├── msc_debug.c         → MSC Debug Channel（自己加的）
├── itm.c               → ITM/SWO Trace（自己加的）
├── uart_debug.c        → UART Debug Log（自己加的）
└── flash_error.c        → Flash Error Log（自己加的）
        ↓
呼叫 BSP 驅動（位於 D:\AiWorkSpace\KM\M480BSP\StdDriver\）
├── hsusbd.c / hsusbd.h
├── usbd.c / usbd.h
├── i2c.c / i2c.h 或 usci_i2c.c
├── clk.c / clk.h
└── fmc.c / fmc.h
```

---

## 8. 常見錯誤與正確做法

### ❌ 錯誤：自己寫 USB 寄存器操作

### ✅ 正確：使用 `HSUSBD_Open()` + `HSUSBD_Start()`

```c
// 錯誤：自己寫寄存器
USB->DEVICE.FADDR = 0x00;

// 正確：用 BSP API
HSUSBD_Open(&gsHSInfo, NULL, NULL);
HSUSBD_Start();
```

### ❌ 錯誤：自己設定 USB Endpoint

### ✅ 正確：在描述符中定義，讓 BSP 自動設定

```c
// 正確：在 usb_descriptors.c 中定義
// BSP 會根據描述符自動設定端點
```

### ❌ 錯誤：燒錄後 USB 沒反應，先懷疑驅動

### ✅ 正確：先燒錄官方範例確認硬體

官方範例如果 USB 也沒反應，才是硬體問題。

---

## 9. BSP 版本與更新

| 版本 | 日期 | 更新內容 |
|------|------|---------|
| V3.05.001 | 2022/07 | 複合裝置範例新增 |
| 更早版本 | - | 基礎 HSUSBD 驅動 |

**確認你的 BSP 版本：**
```c
// 在 main.c 中
printf("BSP Version: %s\n", BSP_VERSION);
```

**下載最新 BSP：**
```
https://www.nuvoton.com/tool-and-software/bsp-software-package/
```

---

## 10. 參考文件

| 文件 | 說明 |
|------|------|
| [M487 Datasheet (PDF)](https://www.alldatasheet.com/datasheet-pdf/pdf/1133525/NUVOTON/M487.html) | 完整電氣規格 |
| [M480 Series TRM (PDF)](http://www.nuvoton-mcu.com/userupload/TRM_M480_Series_SC_Rev1.00.pdf) | 技術參考手冊 |
| [NuMaker HMI M487 User Manual](https://www.nuvoton.com/export/resource-files/UM_NuMaker-HMI-M487_User_Manual_EN_Rev1.00.pdf) | 開發板手冊 |
| [AWS FreeRTOS M487 Getting Started](https://docs.aws.amazon.com/freertos/latest/userguide/getting-started-nuvoton-m487.html) | 官方入門指南 |
| [Nuvoton BSP GitHub](https://github.com/OpenNuvoton/M480BSP) | BSP 原始碼 |

---

## 11. 建議的下一步

1. **燒錄官方 `HSUSBD_HID_Transfer_And_MSC` 範例**到 M487，確認 USB 正常識別
2. 確認後，以這個範例為基礎，加入你的 HID I2C Bridge 功能
3. 用 GCC 編譯燒錄驗證
4. 最後替換成你目前的 `firmware/composite` 程式

**核心原則：先用官方範例確認硬體，再用 GCC編譯自己的程式。**
