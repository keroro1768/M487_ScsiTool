# I3C Host 與 Client (Slave) 開發資源

> 本文件整理 I3C Host (Master) 與 Client (Slave/Target) 的開發資源與範例
> 建立日期：2026-03-21

---

## 1. I3C 角色說明

### 1.1 術語對照

| 術語 | 說明 |
|------|------|
| **Host / Master** | I3C 主控制器 (主導匯流排) |
| **Client / Slave / Target** | I3C 從屬裝置 (回應主控制器) |

### 1.2 我們的專案角色

```
本專案 (USB to I3C Bridge)：
├── Host 端 (USB to I3C Bridge) ◄── 電腦控制 I3C 匯流排
│
└── Client 端 (I3C 感測器) ◄── 被控制的 I3C 裝置
```

---

## 2. Host (Master) 開發資源

### 2.1 STM32 官方範例

| 範例            | 型號      | 說明             |
| ------------- | ------- | -------------- |
| STM32CubeH5   | H563ZI  | 官方 I3C Host 範例 |
| NUCLEO-H563ZI | STM32H5 | 開發板範例          |

**取得方式：**
- STM32CubeIDE → New Project → Select NUCLEO-H563ZI
- Middleware → I3C → Enable

### 2.2 ST 官方教學

| 資源 | 連結 |
|------|------|
| ST Getting Started | https://wiki.st.com/stm32mcu/wiki/Getting_started_with_I3C |
| STM32H5 I3C | https://en.stamssolution.com/i3c-bus-using-in-stm32h5-series/ |

### 2.3 Host 程式碼範例

```c
// I3C Host (Master) 範例 - STM32H5
#include "stm32h5xx_hal.h"
#include "i3c.h"

I3C_HandleTypeDef hi3c1;

void I3C_Master_Init(void) {
    hi3c1.Instance = I3C1;
    hi3c1.Init.Timing = 0x00901362;  // 12.5MHz
    hi3c1.Init.AddressingMode = I3C_ADDRESSING_MODE_7BITS;
    hi3c1.Init.DualAddressingMode = I3C_DUALADDRESSING_DISABLE;
    
    if (HAL_I3C_Init(&hi3c1) != HAL_OK) {
        Error_Handler();
    }
}

// 掃描 I3C 裝置
void Scan_I3C_Devices(void) {
    printf("Scanning I3C bus...\n");
    
    // 執行 ENTDAA (Enter Dynamic Address Assignment)
    if (HAL_I3C_Ctrl_DynAddrAssign(&hi3c1, NULL, 
              I3C_RSTDAA_THEN_ENTDAA, 5000) == HAL_OK) {
        printf("Devices found!\n");
    }
}

// 發送資料到 Client
void I3C_Write(uint8_t addr, uint8_t *data, uint16_t size) {
    HAL_I3C_Master_Transmit(&hi3c1, addr << 1, data, size, 1000);
}

// 從 Client 讀取資料
void I3C_Read(uint8_t addr, uint8_t *data, uint16_t size) {
    HAL_I3C_Master_Receive(&hi3c1, addr << 1, data, size, 1000);
}
```

---

## 3. Client (Slave/Target) 開發資源

### 3.1 STM32 I3C Target 模式

STM32H5 支援作為 I3C Client (Target)。

### 3.2 NXP LPC86x I3C Slave

| 型號 | 特色 |
|------|------|
| LPC86x | 專為 I3C Slave 設計 |

**NXP App Note:**
- AN13952: How to Use I3C in LPC86x

### 3.3 Client 程式碼範例

```c
// I3C Client (Slave/Target) 範例 - STM32H5
#include "stm32h5xx_hal.h"
#include "i3c.h"

I3C_HandleTypeDef hi3c1;

// 設定為 I3C Target (Slave)
void I3C_Target_Init(void) {
    hi3c1.Instance = I3C1;
    hi3c1.Init.AddressingMode = I3C_ADDRESSING_MODE_7BITS;
    hi3c1.Init.TargetAddress = 0x30;  // 靜態位址
    
    // 設定為 Target 模式
    hi3c1.Mode = I3C_MODE_TARGET;
    
    if (HAL_I3C_Init(&hi3c1) != HAL_OK) {
        Error_Handler();
    }
}

// 設定回調函式
void HAL_I3C_Target_AddrCallback(I3C_HandleTypeDef *hi3c, 
                                  uint16_t addrMatchCode) {
    printf("Address matched: 0x%02X\n", addrMatchCode);
}

// 接收資料回調
void HAL_I3C_Target_RxCpltCallback(I3C_HandleTypeDef *hi3c) {
    printf("Data received: %d bytes\n", hi3c->RxXferSize);
}

// 發送資料回調
void HAL_I3C_Target_TxCpltCallback(I3C_HandleTypeDef *hi3c) {
    printf("Data sent: %d bytes\n", hi3c->TxXferSize);
}
```

---

## 4. I3C 通訊流程

### 4.1 Host 發起通訊

```
Host                           Client
  │                               │
  │──── ENTDAA (Scan) ──────────►│
  │◄─── PID/BCR/DCR ────────────│
  │                               │
  │──── SETDASA (Set Dynamic) ──►│
  │◄─── ACK ────────────────────│
  │                               │
  │──── Write (0x30) ────────────►│
  │◄─── ACK ────────────────────│
  │──── Data ────────────────────►│
  │◄─── ACK ────────────────────│
  │                               │
  │──── Read (0x30) ─────────────►│
  │◄─── Data ───────────────────│
  │──── ACK ─────────────────────►│
```

### 4.2 Client 發起中斷 (IBI)

```
Host                           Client
  │                               │
  │   (正常通訊結束)              │
  │                               │
  │◄─── IBI + Address ─────────│
  │──── ACK ────────────────────►│
  │◄─── IBI Data ───────────────│
  │──── ACK ────────────────────►│
```

---

## 5. 常見 I3C Client 裝置

| 裝置 | 型號 | 功能 |
|------|------|------|
| 加速度計 | BMI260 | 6軸加速度計 |
| 陀螺儀 | BMI260 | 3軸陀螺儀 |
| 氣壓計 | BMP580 | 氣壓/溫度 |
| 環境光 | APDS-9960 | RGB/proximity |

---

## 6. 開發板選擇

### 6.1 Host 開發板

| 型號 | 用途 |
|------|------|
| NUCLEO-H563ZI | I3C Host 主控制器 |
| Blue Pill | 實驗用 Host (需改裝) |

### 6.2 Client 開發

| 型號 | 用途 |
|------|------|
| NUCLEO-H563ZI | 可做為 I3C Target |
| 其他 MCU + I3C 感知 | 作為 Client 測試 |

---

## 7. 範例程式位置

### 7.1 ST 官方範例

```
STM32CubeH5/
├── Projects/
│   └── NUCLEO-H563ZI/
│       └── Examples/
│           └── I3C/
│               ├── I3C_Master                # Host 範例
│               ├── I3C_Slave                # Client 範例
│               └── I3C_IBI                  # 中斷範例
```

### 7.2 GitHub 範例

| 範例 | 網址 |
|------|------|
| STM32 I2C Slave | https://github.com/tunerok/stm32_i2c_slave_examlpe |
| STM32CubeF4 I2C | https://github.com/STMicroelectronics/STM32CubeF4 |

---

## 8. 總結

### 8.1 開發需求

| 角色 | 我們的用途 |
|------|-----------|
| **Host** | USB to I3C Bridge (電腦控制) |
| **Client** | I3C 感測器/裝置 |

### 8.2 下一步

1. 購買 NUCLEO-H563ZI 作為 Host
2. 測試 I3C Host 程式
3. 連接 I3C 感測器 (如 BMI260)
4. 實現完整的 USB to I3C Bridge

---

## 9. 參考資源

| 資源 | 連結 |
|------|------|
| ST Getting Started | https://wiki.st.com/stm32mcu/wiki/Getting_started_with_I3C |
| NXP AN13952 | https://www.nxp.com/docs/en/application-note/AN13952.pdf |
| I3C Protocol | https://docs.kernel.org/driver-api/i3c/protocol.html |

---

*最後更新：2026-03-21*
