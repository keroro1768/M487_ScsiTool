# VERIFY — T030

## 基本資訊

| 欄位 | 內容 |
|------|------|
| Task ID | T030 |
| 驗收人 | Dororo（待指定）|
| 驗收日期 | 待填寫 |
| 狀態 | CONDITIONAL |

## 交付清單

- [x] tool/itm_trace_viewer.py (32KB)
- [x] UARTReceiver / FileReceiver / JLinkReceiver
- [x] ITMParser + TPIU frame decode
- [x] ANSI 彩色輸出

## 軟體驗證（已完成）

| 項目 | 結果 |
|------|------|
| Python 語法檢查 | ✅ 通過 |
| --help 測試 | ✅ 正常 |

## 硬體驗證（待執行）

| 項目 | 結果 |
|------|------|
| 實際 UART-to-USB bridge 接收 | ⏸️ 待硬體 |
| SWO trace 解析 | ⏸️ 待硬體 |

## Blocks

⚠️ 需要 UART-to-USB bridge 硬體（M487 PB8 SWO → FTDI → PC）
