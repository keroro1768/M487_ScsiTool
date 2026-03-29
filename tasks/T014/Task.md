# Task.md — T014

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T014 |
| 標題 | I2C NACK Retry 機制實作 |
| 狀態 | done |
| 優先序 | P0 |
| 指派 | Giroro |
| 依賴 | 無 |
| 截止 | 2026-03-27 |

## 目標

消除 I2C 通訊單次失敗即中斷的脆弱設計

## 需求

- [x] 分析 NACK 發生情境
- [x] 實作可配置次數的 NACK retry（NACK_RETRY_MAX = 3）
- [x] 建立指數 backoff 策略
- [x] 回傳統一的錯誤碼

## 進度

✅ 完成。NACK retry 已實作，指數 backoff 上限 100ms。
