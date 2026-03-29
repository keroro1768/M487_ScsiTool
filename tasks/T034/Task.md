# Task.md — T034

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T034 |
| 標題 | DWT Breakpoint/Watchpoint Debug |
| 狀態 | done |
| 優先序 | P2 |
| 指派 | Giroro |
| 依賴 | T024 完成 |
| 截止 | - |

## 目標

利用 ARM CoreSight DWT 實現硬體 breakpoint/watchpoint

## 研究發現

M487 M4 core 有 6 HW breakpoints + 4 watchpoints。DWT base = 0xE0001000。ITM 可整合 DWT event 輸出至 SWO。

## 進度

✅ Research Finish（2026-03-27 18:04）

## 研究文件

`T034_DWT_Research.md`
