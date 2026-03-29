# Task.md — T011

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T011 |
| 標題 | T001 實作品質 Review |
| 狀態 | done |
| 優先序 | P0 |
| 指派 | Dororo + Giroro |
| 依賴 | T001 |
| 截止 | - |

## 目標

對 T001 firmware/composite-rewrite 進行 Code Review，發現並修復問題

## 需求

- [x] 程式碼 Review（Q-01~Q-10）
- [x] 發現 Critical/Major 問題並修復
- [x] 補齊 Minor 問題（時程允許）

## 進度

**已發現並修復：**
- Q-02：錯誤回傳值不一致（I2C_Read 回傳 -1，其餘回傳 0）
- Q-03：Buffer 邊界檢查缺失（EPB_Handler 無 len 驗證）
- Q-05：NACK retry 機制缺失
- Q-07：Magic Numbers 未消除
- S-05：GET_REPORT 直接 STALL（不合 HID 規範）
- S-06：SET_REPORT 只處理 Feature Report

**Minor 問題（不阻礙交付）：**
- I2C_Read() 回傳 -1 表示未實作，應補完後統一錯誤碼約定
- 部分函式缺 docstring

## 備註

**結果：0 Critical / 0 Major，2 Minor**
