# I3C Hot-Join 完整教學

> 本文件詳細介紹 I3C Hot-Join 機制的原理與實作
> 參考來源：MIPI I3C Specification, MIPI Hot-Join Application Note

---

## 1. 什麼是 Hot-Join？

### 1.1 定義

**Hot-Join** 是 I3C 的一項重要功能，允許 I3C 裝置在匯流排已經被初始化之後，**熱插拔**加入匯流排，而無需重新啟動系統或匯流排。

### 1.2 與 I2C 的比較

| 特性 | I2C | I3C Hot-Join |
|------|-----|---------------|
| 熱插拔 | 不支援 | ✅ 支援 |
| 需要外部中斷 | 需要 GPIO | 不需要 |
| 動態位址 | 不支援 | ✅ 支援 |
| 系統重啟 | 需要 | 不需要 |

### 1.3 應用場景

| 場景 | 說明 |
|------|------|
| **模組化系統** | 可在系統運作時新增感測器模組 |
| **擴充底座** | laptop/手機底座熱插拔 |
| **感測器陣列** | 根據需求動態啟用/停用感測器 |
| **模組維修** | 可在系統運作時更換故障模組 |

---

## 2. Hot-Join 運作原理

### 2.1 硬體層面

```
┌─────────────────────────────────────────────────────────────┐
│                    I3C Bus (已初始化的匯流排)              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   SCL ──────────────────────────────────────────────────    │
│   SDA ──────────────────────────────────────────────────    │
│                                                             │
│   ┌─────────┐                                            │
│   │ Active  │ ◄── 已有位址的 I3C 裝置                    │
│   │ Controller│                                            │
│   └─────────┘                                            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ 熱插拔 新裝置
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Hot-Join 流程                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 偵測匯流排空閒 (Bus Idle)                              │
│  2. 發送 Hot-Join 請求 (SDA 低電位)                       │
│  3. 主控制器回應並時脈                                        │
│  4. 執行 ENTDAA 分配動態位址                               │
│  5. 完成 Hot-Join                                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 訊號時序

```
SCL ─┐    ┌─────────────────────────────────────
      │    │
      └────┘

SDA ──┐         ┌──────┐  ┌──────────────────
       │         │ START │  │ Hot-Join Request
       └─────────┘      └──┘ (SDA low by device)
       
              │         │
              │         │ Device pulls SDA low to request
              │         │
              ▼         ▼
         Bus Idle   Controller clocks
```

---

## 3. Hot-Join 完整流程

### 步驟流程圖

```
┌─────────────────────────────────────────────────────────────┐
│                   Hot-Join 流程                            │
└─────────────────────────────────────────────────────────────┘

  ┌─────────────┐
  │ 1. 裝置準備 │ ◄── 裝置已通電，準備加入匯流排
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ 2. 偵測匯流排 │ ◄── 確認匯流排處於 Idle 狀態
  │ (Bus Idle)  │
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ 3. 發送請求 │ ◄── 偵測 START + 0x7E (Reserved)
  │ (HJ Request)│     然後拉低 SDA 發送 Hot-Join
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ 4. 主控制器回應│ ◄── 主控制器偵測到 Hot-Join
  │              │     提供時脈
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ 5. 發送     │ ◄── 裝置發送 0x02 (Hot-Join
  │ Reserved   │     Broadcast Address)
  │ Address    │
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ 6. ENTDAA  │ ◄── 主控制器執行
  │ (Dynamic   │     Enter Dynamic Address
  │ Address    │     Assignment
  │ Assignment)│
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ 7. 分配位址 │ ◄── 裝置獲得動態位址
  │ (Assigned  │     (例如 0x30)
  │ Dynamic    │
  │ Address)   │
  └──────┬──────┘
         │
         ▼
  ┌─────────────┐
  │ 8. 正常通訊 │ ◄── Hot-Join 完成
  │             │     可開始標準 I3C 通訊
  └─────────────┘
```

### 3.1 詳細步驟說明

#### 步驟 1：裝置準備
- 裝置已通電
- 裝置支援 Hot-Join 功能 (HJCAP bit 需設定)
- 裝置處於未初始化狀態

#### 步驟 2：偵測匯流排 Idle
- 偵測 SCL 和 SDA 皆為 HIGH 狀態
- 確認匯流排處於空閒狀態

#### 步驟 3：發送 Hot-Join 請求
- 偵測到 START + 0x7E (I2C Reserved Address)
- 裝置將 SDA 拉低，發出 Hot-Join 請求訊號

#### 步驟 4：主控制器回應
- 主控制器偵測到 Hot-Join 請求
- 主控制器提供時脈信號

#### 步驟 5：發送 Reserved Address
- 裝置發送 0x02 (Hot-Join Broadcast Address)
- 格式：`7'b000_0010`

#### 步驟 6：ENTDAA 程序
- 主控制器執行 ENTDAA (Enter Dynamic Address Assignment)
- 這與一般初始化時的 DAA 程序相同

#### 步驟 7：位址分配
- 主控制器分配唯一的動態位址
- 裝置儲存位址並回應 ACK

#### 步驟 8：正常通訊
- Hot-Join 完成
- 裝置現在可以使用分配的動態位址進行通訊

---

## 4. 關鍵術語

| 術語 | 說明 |
|------|------|
| **HJREQ** | Hot-Join Request - 裝置發出的請求訊號 |
| **HJCAP** | Hot-Join Capable - 裝置是否支援 Hot-Join |
| **ENTDAA** | Enter Dynamic Address Assignment - 動態位址分配 |
| **DAA** | Dynamic Address Assignment - 動態位址分配程序 |
| **BCR** | Bus Characteristic Register - 匯流排特性暫存器 |
| **DCR** | Device Characteristic Register - 裝置特性暫存器 |
| **PID** | Provisional ID - 暫時識別碼 |

---

## 5. 程式碼範例 (STM32)

### 5.1 初始化支援 Hot-Join

```c
// 設定 BCR - 宣告支援 Hot-Join
#define BCR_HOT_JOIN_CAPABLE   (1 << 6)  // Bit 6: Hot-Join Capable

void i3c_device_init(void) {
    // 設定 BCR (Bus Characteristic Register)
    // 宣告此裝置支援 Hot-Join
    device_bcr = BCR_HOT_JOIN_CAPABLE | 
                 BCR_I3C_DEVICE |       // I3C 裝置
                 BCR_ADVANCED;           // 進階功能
    
    // 設定 DCR (Device Characteristic Register)
    device_dcr = DCR_GENERIC_SENSOR;    // 根據裝置類型
    
    // 設定 PID (Provisional ID)
    device_pid = (uint64_t)0x1234567890ABCD; // 64-bit PID
}
```

### 5.2 主控制器處理 Hot-Join

```c
// 主控制器端處理 Hot-Join 請求
void HAL_I3C_HotJoinCallback(I3C_HandleTypeDef *hi3c) {
    printf("Hot-Join request detected!\n");
    
    // 執行 ENTDAA 程序
    if (HAL_I3C_EnterDynAddrAssign(hi3c, I3C_RSTDAA_THEN_ENTDAA) == HAL_OK) {
        printf("Dynamic address assigned successfully\n");
        
        // 列印已發現的裝置
        for (int i = 0; i < hi3c->deviceCount; i++) {
            printf("Device[%d]: Address = 0x%02X\n", 
                   i, hi3c->devices[i].dynamicAddr);
        }
    }
}
```

### 5.3 完整 ENTDAA 程序

```c
HAL_StatusTypeDef I3C_ProcessHotJoin(I3C_HandleTypeDef *hi3c) {
    uint8_t dev_count = 0;
    I3C_DeviceInfo devices[8];
    
    // 1. 執行 ENTDAA (Enter Dynamic Address Assignment)
    HAL_StatusTypeDef status = HAL_I3C_Ctrl_DynAddrAssign(
        hi3c, 
        NULL,               // 使用預設搜尋
        I3C_RSTDAA_THEN_ENTDAA,  // 先重置再進行 DAA
        5000               // Timeout 5s
    );
    
    if (status != HAL_OK) {
        printf("ENTDAA failed!\n");
        return status;
    }
    
    // 2. 列印新增的裝置
    printf("Hot-Join device added!\n");
    
    return HAL_OK;
}
```

---

## 6. 電氣特性要求

### 6.1 Hot-Join 時序要求

| 參數 | 最小值 | 最大值 | 說明 |
|------|--------|--------|------|
| tIDLE | 80μs | - | 匯流排空閒時間 |
| tHIGH | 200ns | - | SCL HIGH 時間 |
| tLOW | 200ns | - | SCL LOW 時間 |

### 6.2 電源要求

- 支援 Hot-Join 的裝置必須在匯流排初始化時已經通電
- 建議在 SDA/SCL 上使用低電容線路

---

## 7. 與 In-Band Interrupt (IBI) 的比較

| 特性 | Hot-Join | IBI |
|------|----------|-----|
| **用途** | 裝置加入匯流排 | 裝置發出中斷 |
| **發起者** | 裝置 | 裝置 |
| **時機** | 需要獲得位址 | 需要主動通知 |
| **位址** | 尚未分配 | 已有動態位址 |

---

## 8. 應用範例

### 8.1 感測器模組熱插拔

```
場景：系統運作中，使用者新增一個溫度感測器模組

步驟：
1. 使用者插入感測器模組
2. 模組通電
3. 模組偵測 I3C 匯流排 Idle
4. 模組發送 Hot-Join 請求
5. 主控制器偵測並回應
6. 執行 ENTDAA 分配位址 (例如 0x35)
7. 主動讀取感測器資料
```

### 8.2 多功能底座

```
場景：Laptop 底座熱插拔

1. 使用者將底座連接到 Laptop
2. 底座內的 I3C 裝置通電
3. 發送 Hot-Join 請求
4. Laptop 主控制器分配位址
5. 自動識別底座內的裝置 (觸控板、鍵盤等)
6. 安裝對應的驅動程式
```

---

## 9. 總結

### Hot-Join 重點

| 項目 | 說明 |
|------|------|
| **定義** | 允許 I3C 裝置在匯流排初始化後加入 |
| **優點** | 熱插拔、無需外部中斷、動態位址分配 |
| **流程** | 偵測 Idle → 發送請求 → ENTDAA → 完成 |
| **必備條件** | 裝置需支援 HJCAP、主控制器需處理 |

### 與傳統方案的優勢

- ✅ **無需重啟** - 系統運作中直接新增裝置
- ✅ **節省 GPIO** - 不需要額外中斷線
- ✅ **彈性配置** - 可動態調整匯流排上的裝置
- ✅ **節省功耗** - 裝置可在不使用時關閉電源

---

## 10. 參考資源

| 資源 | 連結 |
|------|------|
| MIPI I3C Hot-Join App Note | https://www.mipi.org/hubfs/I3C-Resources/MIPI-Alliance-I3C-v1-2-Hot-Join-App-Note-public-edition.pdf |
| Microchip I3C Documentation | https://onlinedocs.microchip.com/ |
| Linux Kernel I3C | https://docs.kernel.org/driver-api/i3c/protocol.html |
| ST I3C Application Note | https://www.st.com/resource/en/application_note/an5988-tsc1641-i3c-capabilities-stmicroelectronics.pdf |

---

*最後更新：2026-03-21*
