# VERIFY — T034

## 基本資訊

| 欄位 | 內容 |
|------|------|
| Task ID | T034 |
| 驗收人 | Dororo（待指定）|
| 驗收日期 | 待填寫 |
| 狀態 | CONDITIONAL |

## 交付清單

- [x] DWT 研究文件
- [x] M487 M4 Core 6 HW breakpoints + 4 watchpoints 分析
- [x] DWT base = 0xE0001000

## 研究結論

✅ Research Finish — 可在 msc_debug.c 加入 DBG_BREAK_SET / DBG_WATCH_SET 擴展 MSC command

## 軟體驗證（已完成）

| 項目 | 結果 |
|------|------|
| 研究文件審查 | ✅ 通過 |
| 寄存器位址 | ✅ 正確 |

## 硬體驗證（待執行）

| 項目 | 結果 |
|------|------|
| DWT breakpoint 實際運作 | ⏸️ 待硬體 |
| DWT watchpoint 實際運作 | ⏸️ 待硬體 |

## Blocks

無（研究階段已完成）
