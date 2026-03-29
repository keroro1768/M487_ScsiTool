# Task.md — T031

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T031 |
| 標題 | Flash Error Log System（錯誤持久化）|
| 狀態 | completed |
| 優先序 | P2 |
| 指派 | Giroro |
| 依賴 | T025 完成 |
| 截止 | - |

## 目標

將錯誤碼寫入 Flash 保留區，形成 persistent error log

## 需求

- [x] Flash 保留區使用（FMC 最後 4KB, 0x0007F000）
- [x] ErrorLogEntry_t 結構
- [x] 最大 64 筆錯誤記錄，環形覆蓋
- [x] 透過 MSC Debug Channel 讀取（DBG_READ_ERRLOG, 0xC0/0x09）
