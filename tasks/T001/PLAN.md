# PLAN.md — T001

## 執行計畫

### Step 1：修復 OpenOCD 連線（前置準備）
1. 使用 Zadig 置換 Nu-Link 驅動為 WinUSB
2. 驗證 OpenOCD 可正常連線
3. 燒錄最新韌體到 M487

### Step 2：I2C Read 功能實作
1. 完成 `I2C_Read()` 函式
2. 加入 NACK retry 機制
3. 測試 I2C write + read 流程

### Step 3：USB HID Layer 整合
1. 整合 BSP HSUSBD 框架（T008）
2. 驗證 HID Class Request（GET_REPORT/SET_REPORT）
3. 驗證 EP1/EP2 Interrupt 傳輸

### Step 4：MSC 功能驗證
1. 測試 RAM Disk 識別
2. 測試檔案讀寫
3. 測試複合裝置同時運作

### Step 5：整合測試
1. HID + MSC 同時運作不衝突
2. Windows Tool 完整測試

## 子任務

| 子任務 | 說明 |
|--------|------|
| T007 | I2C Read 功能實作 |
| T008 | BSP HSUSBD 框架整合 |
| T010 | 實體測試 |

## 預估工時

| 階段 | 預估 |
|------|------|
| OpenOCD 修復 | 1 hr |
| I2C Read 實作 | 2 hr |
| USB HID 整合 | 4 hr |
| MSC 驗證 | 2 hr |
| 整合測試 | 2 hr |
| **合計** | **~11 hr** |
