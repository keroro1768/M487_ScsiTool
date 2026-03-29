# Task.md — T026

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T026 |
| 標題 | MSC Debug CLI Tool (msc_debug.exe) |
| 狀態 | done |
| 優先序 | P1 |
| 指派 | Tamama |
| 依賴 | T025 完成 |
| 截止 | - |

## 目標

Windows CLI 工具，透過 USB MSC 發送 Vendor CDB，讀取除錯資訊

## 需求

- [x] Windows CLI 完整實作（info, readmem, writemem, readreg, writereg, log, echo）
- [x] IOCTL_SCSI_PASS_THROUGH_DIRECT
- [x] msc_debug.exe 編譯完成（292KB）

## 進度

✅ 完成（待硬體驗證）
