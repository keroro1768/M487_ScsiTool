# Task.md — T033

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T033 |
| 標題 | GDB RSP Server（Software ICE via USB MSC）|
| 狀態 | done |
| 優先序 | P2 |
| 指派 | Giroro |
| 依賴 | T025 完成 |
| 截止 | - |

## 目標

透過 USB MSC 接受 GDB 命令，實現 self-hosted debugging

## 研究結論

RSP Server 經 MSC Debug Channel 承載有雙向來回限制。更推薦 MSC Debug CLI 直接讀取狀態。如需真正 RSP，待有 OpenOCD 時用 SWD 直接對 DWT 操作。

## 進度

✅ Research Finish（2026-03-27 18:04）

## 研究文件

`T033_GDB_RSP_Research.md`
