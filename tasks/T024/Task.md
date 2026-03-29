# Task.md — T024

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T024 |
| 標題 | ITM/SWO Trace System |
| 狀態 | done |
| 優先序 | P0 |
| 指派 | Giroro |
| 依賴 | 無 |
| 截止 | - |

## 目標

提供零成本的即時執行追蹤能力，不需要 halt MCU

## 需求

- [x] itm.h / itm.c - ITM 追蹤系統
- [x] ITM_Init() / ITM_InitWithBaud()
- [x] ITM_Log / ITM_ERR / ITM_DBG / ITM_HEX_DUMP
- [x] DWT cycle counter timestamps
- [x] Module-specific macros (USB_TRACE, MSC_TRACE, I2C_TRACE, HID_TRACE)

## 進度

✅ 完成（程式碼完成，待硬體驗證）

## 備註

PB8 SWO pin 需確認 datasheet MFP 值
