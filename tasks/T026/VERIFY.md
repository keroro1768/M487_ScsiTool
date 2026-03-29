# VERIFY — T026

## 基本資訊

| 欄位 | 內容 |
|------|------|
| Task ID | T026 |
| 驗收人 | Dororo（待指定）|
| 驗收日期 | 待填寫 |
| 狀態 | CONDITIONAL |

## 交付清單

- [x] msc_debug.exe (292KB)
- [x] info / readmem / writemem / readreg / writereg / log / echo commands
- [x] IOCTL_SCSI_PASS_THROUGH_DIRECT

## 軟體驗證（已完成）

| 項目 | 結果 |
|------|------|
| 程式碼審查 | ✅ 通過 |
| 編譯測試 | ✅ 成功 |
| CLI --help | ✅ 正常 |

## 硬體驗證（待執行）

| 項目 | 結果 |
|------|------|
| 與 M487 MSC 實際通訊 | ⏸️ 待硬體 |
| MSC Debug Channel 指令 | ⏸️ 待硬體 |

## Blocks

⚠️ M487 USB 未連接 — 無法驗證

## 備註

Minor Issue：ECHO 命令 firmware 端 buffer offset 問題，不影響主要除錯功能
