# VERIFY — T017

## 基本資訊

| 欄位 | 內容 |
|------|------|
| Task ID | T017 |
| 驗收人 | Tamama |
| 驗收日期 | 2026-03-28 |
| 狀態 | PASS |

## 交付清單

- [x] OpenOCD 路徑改為環境變數（OPENOCD_ROOT）
- [x] tool 存在性檢查
- [x] --verify 燒錄驗證
- [x] 錯誤碼檢查與 early exit

## 驗證結果

flash.bat 已強化錯誤處理

## Blocks（若有）

無

## 備註

-

## Commit

`7711083`（含 T017 flash.bat Error Handling）
