# ARM Cortex-M4 燒錄與保護機制

## 概述

本文整理 ARM Cortex-M4 微控制器的燒錄方式與安全保護機制，特別針對 Nuvoton M487 系列說明。

---

## 一、燒錄方式

### 1.1 SWD / JTAG 燒錄

| 介面 | 針腳 | 說明 |
|------|------|------|
| SWDIO | SWDIO | 雙向資料 |
| SWCLK | SWCLK | 時脈 |
| SWO | SWO | 追蹤輸出 (可選) |
| NRST | NRST | 系統重設 |

#### 燒錄器

| 燒錄器 | 支援晶片 |
|--------|----------|
| J-Link | Cortex-M 全系列 |
| ST-Link | STM32 |
| Nu-Link | Nuvoton M0/M4 |
| CMSIS-DAP | ARM 標準 |

#### 燒錄流程

```
1. 連接 SWD 介面
2. 發送連線請求
3. 驗證晶片 ID
4. 擦除 Flash
5. 燒錄程式
6. 驗證燒錄結果
7. 設定選項位元
```

### 1.2 ISP (In-System Programming)

####  UART ISP

```
PC ──(UART)──► Bootloader ──► Flash
```

| 型號 | UART | Baud Rate |
|------|------|-----------|
| M487 | UART0 | 115200 |

#### 流程

```
1. 設定 Boot Pin (低電位)
2. 重置晶片
3. PC 傳送燒錄指令
4. bootloader 接收資料
5. 寫入 Flash
```

### 1.3 ICP (In-Circuit Programming)

#### NuMicro ICP

```
PC ──(Nu-Link)──► ICPC 介面 ──► Flash
```

| 特色 | 說明 |
|------|------|
| 線上燒錄 | 無需取出晶片 |
| 加密傳輸 | 資料加密傳送 |
| 自動驗證 | 燒錄後自動驗證 |

---

## 二、安全保護機制

### 2.1 Flash 鎖定 (Security Lock)

#### 運作方式

| 層級 | 保護內容 |
|------|----------|
| Level 1 | 禁止 SWD 讀取/除錯 |
| Level 2 | 禁止 SWD 讀取（需完全清除） |
| Level 3 | 禁止所有外部存取 |

#### Nuvoton 特色

- **Security Lock**：透過 ICP/ISP 工具設定
- **CRP (Code Read Protection)**：防止讀取
- **Secure Boot**：驗證開機程式完整性

### 2.2 選項位元 (Option Bytes)

| 位元 | 功能 |
|------|------|
| nBOOT0 | 選擇開機來源 |
| nSW Boot0 | 軟體控制開機來源 |
| WDTSEL | 看門狗時脈選擇 |
| Data Protection | 資料區域保護 |

### 2.3 MPU (Memory Protection Unit)

```c
// MPU 設定範例
void MPU_Config(void)
{
    // 設定 Flash 為唯讀
    MPU->RBAR = 0x00000000;  // Flash 起始位址
    MPU->RASR = (0x03 << 19) |  // AP: 讀取/寫入
                (0x06 << 16) |  // Region Size: 512KB
                (0x01 << 15) |  // Enable
                (0x01 << 12);   // SRD: 1 sub-region
}
```

### 2.4 安全開機 (Secure Boot)

#### 流程

```
┌─────────────────────────────────────┐
│ Power On                            │
└──────────────────┬──────────────────┘
                   ↓
┌─────────────────────────────────────┐
│ Bootloader 驗證 Application 簽章     │
└──────────────────┬──────────────────┘
                   ↓
        ┌──────────┴──────────┐
        ↓                    ↓
   簽章有效              簽章無效
        ↓                    ↓
   執行 App           進入安全模式
```

---

## 三、Nuvoton M487 特殊功能

### 3.1 內建保護機制

| 功能 | 說明 |
|------|------|
| Secure Boot ROM | 工廠預載安全開機 |
| Protection ROM | 安全儲存區域 |
| Encrypt Code | 程式碼加密 |

### 3.2 ISP/ICP 加密傳輸

```c
// 線上燒錄模式加密
void ISP_Encrypt(uint8_t *data, uint32_t len)
{
    // AES-128 加密
    AES_Encrypt(data, len, aes_key);
}
```

### 3.3 燒錄工具

| 工具 | 用途 |
|------|------|
| ICP Programming Tool | Nu-Link 專用 |
| NuMicro ISP Programming Tool | UART ISP |
| NuMaker Provisioning Tool | 大量生產 |

---

## 四、燒錄流程詳細說明

### 4.1 SWD 燒錄時序

```
     ┌─────┐     ┌─────┐     ┌─────┐
SWDIO│     │     │     │     │     │
     │ START│ ID  │ ACK │ DATA│ STOP│
     └─────┘     └─────┘     └─────┘
           ←───►
           單向傳輸
```

### 4.2 JTAG vs SWD

| 特性 | JTAG | SWD |
|------|------|-----|
| 針腳數 | 4-5 | 2-3 |
| 速度 | 較慢 | 較快 |
| 相容性 | 廣泛 | Cortex-M |
| SWO | 需額外 | 可選 |

---

## 五、常見問題

### Q1: 忘記 Security Lock 密碼？

**解答**：
- 執行全晶片擦除 (Chip Erase)
- 需使用 ICP 工具
- 會清除所有資料

### Q2: 如何防止盜拷？

| 方法 | 等級 | 說明 |
|------|------|------|
| 關閉 SWD | 高 | 無法除錯 |
| 程式碼加密 | 高 | 燒錄加密過的 binary |
| 簽章驗證 | 高 | Secure Boot |
| 唯一 ID | 中 | 綁定特定晶片 |

### Q3: 保護後還能燒錄嗎？

| 保護層級 | 可燒錄 | 可擦除 |
|----------|--------|--------|
| Level 1 | ✅ | ✅ |
| Level 2 | ❌ | 需完全擦除 |
| Level 3 | ❌ | ❌ |

---

## 六、程式碼保護實作

### 6.1 CRP 設定 (Nuvoton)

```c
// config.c
// 設定 CRP Level 1
__attribute__((section("CRP")))
const uint32_t CRP_Level1 = 0xFFFFFFFF;
```

### 6.2 MPU 保護

```c
void EnableReadoutProtection(void)
{
    // 設定 MPU 保護 Flash
    MPU->CTRL = 0x00;  // disable MPU
    MPU->RNR  = 0;    // region 0
    MPU->RBAR = 0x00000000;  // Flash base
    MPU->RASR = (0x03 << 19) |  // Read-only
                (0x14 << 1);    // 512KB
    MPU->CTRL = 0x05;  // enable MPU
}
```

### 6.3 加密燒錄流程

```
┌──────────────┐     ┌──────────────┐
│  原始 Code   │────►│ AES 加密     │
└──────────────┘     └──────────────┘
                              │
                              ▼
┌──────────────┐     ┌──────────────┐
│   M487 Flash │◄────│  加密 Binary │
└──────────────┘     └──────────────┘
```

---

## 七、總結比較表

| 燒錄方式 | 速度 | 需要硬體 | 安全性 |
|----------|------|----------|--------|
| SWD/JTAG | 快 | 燒錄器 | 中 |
| UART ISP | 慢 | 序列埠 | 低 |
| ICP | 快 | Nu-Link | 高 |
| USB ISP | 中 | USB | 中 |
| **pyOCD** | 快 | CMSIS-DAP | 高 |

### 燒錄器比較

| 燒錄器 | 類型 | 支援晶片 | 價格 |
|--------|------|----------|------|
| **J-Link** | 商業 | 全面 | 高 |
| **ST-Link** | 官方 | STM32 | 中 |
| **pyOCD** | 開源 | Cortex-M | 免費 |
| **CMSIS-DAP** | 開源 | Cortex-M | 免費/便宜 |
| **Nu-Link** | 官方 | Nuvoton | 中 |

| 保護機制 | 等級 | 可逆 |
|----------|------|------|
| Flash Lock | 高 | 是 |
| CRP | 高 | 需擦除 |
| Secure Boot | 极高 | 否 |
| MPU | 中 | 是 |

---

## 參考資源

| 資源 | 連結 |
|------|------|
| Nuvoton ICP Tool | https://www.nuvoton.com/resource-files/UM_ICP_Programming_Tool_EN_Rev2.06.pdf |
| Nuvoton Code Protection | https://www.nuvoton.com/export/resource-files/AN0001_NuMicro_Cortex-M_Code_Protection_EN_V1.00.pdf |
| ARM Cortex-M4 | https://en.wikipedia.org/wiki/ARM_Cortex-M |

---

*建立時間: 2026-03-25*
