# VERIFY — T025

## 基本資訊

| 欄位 | 內容 |
|------|------|
| Task ID | T025 |
| 驗收人 | Dororo（待指定）|
| 驗收日期 | 待填寫 |
| 狀態 | CONDITIONAL |

## 交付清單

- [x] msc_debug.h / msc_debug.c
- [x] CDB 0xC0-0xFF vendor commands
- [x] DBG_READ_MEM, DBG_WRITE_MEM, DBG_READ_REG, DBG_WRITE_REG
- [x] DBG_GET_INFO, DBG_READ_LOG, DBG_ECHO

## 軟體驗證（已完成）

| 項目 | 結果 |
|------|------|
| 程式碼審查 | ✅ 通過 |
| 編譯測試 | ✅ 成功 |
| CBW/CSW 解析邏輯 | ✅ 正確 |

## 硬體驗證（待執行）

| 項目 | 結果 |
|------|------|
| USB MSC 實際連線 | ⏸️ 待硬體 |
| MSC Debug Channel 讀寫 | ⏸️ 待硬體 |

## Blocks

⚠️ OpenOCD LIBUSB_ERROR_ACCESS — 無法燒錄韌體驗證
