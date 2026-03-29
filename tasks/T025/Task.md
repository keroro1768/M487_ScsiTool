# Task.md — T025

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T025 |
| 標題 | MSC Vendor Debug Channel |
| 狀態 | done |
| 優先序 | P0 |
| 指派 | Giroro |
| 依賴 | 無 |
| 截止 | - |

## 目標

透過 USB MSC 存取任意記憶體/CPU暫存器，不需要任何額外驅動

## 需求

- [x] msc_debug.h / msc_debug.c - MSC Vendor Command Handler
- [x] CDB 0xC0-0xFF (Vendor-specific range)
- [x] DBG_READ_MEM, DBG_WRITE_MEM, DBG_READ_REG, DBG_WRITE_REG
- [x] DBG_GET_INFO, DBG_READ_LOG, DBG_ECHO

## 進度

✅ 完成（程式碼完成，待硬體驗證）
