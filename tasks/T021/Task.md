# Task.md — T021

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T021 |
| 標題 | BuildCommandRegister 截斷風險修復（16-bit register）|
| 狀態 | done |
| 優先序 | P1 |
| 指派 | Giroro |
| 依賴 | T004 HID Descriptor Parser 確認 |
| 截止 | - |

## 目標

16-bit register 位址不再被截斷為 8-bit

## 需求

- [x] HID-over-I2C 規範確認（需 16-bit）
- [x] HID_Context_t reg* 改為 uint16_t
- [x] I2C0_ReadReg 等函式支援 uint16_t reg

## 進度

✅ 完成（2026-03-27 08:01）
