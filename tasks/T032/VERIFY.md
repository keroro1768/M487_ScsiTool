# VERIFY — T032

## 基本資訊

| 欄位 | 內容 |
|------|------|
| Task ID | T032 |
| 驗收人 | Dororo（待指定）|
| 驗收日期 | 待填寫 |
| 狀態 | CONDITIONAL |

## 交付清單

- [x] self_test.h / self_test.c
- [x] 6 項硬體測試（HXT/PLL/SRAM/I2C/USB PHY/DWT）
- [x] gcc_arm_selftest.ld

## 軟體驗證（已完成）

| 項目 | 結果 |
|------|------|
| 程式碼審查 | ✅ 通過 |
| 編譯測試 | ✅ 成功 |

## 硬體驗證（待執行）

| 項目 | 結果 |
|------|------|
| HXT Clock 驗證 | ⏸️ 待硬體 |
| SRAM March test | ⏸️ 待硬體 |
| MSC Debug Channel 讀取結果 | ⏸️ 待硬體 |

## Blocks

⚠️ OpenOCD LIBUSB_ERROR_ACCESS — 無法燒錄韌體驗證

## 備註

USB PHY loopback 未實作（M487 PHY 不支援 internal loopback）
