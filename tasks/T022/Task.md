# Task.md — T022

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T022 |
| 標題 | Buffer 邊界檢查補全 |
| 狀態 | done |
| 優先序 | P1 |
| 指派 | Giroro |
| 依賴 | 無 |
| 截止 | - |

## 目標

所有 buffer 操作皆有明確的邊界驗證

## 需求

- [x] EPB_Handler 加入 len <= EPB_MAX_PKT_SIZE 檢查
- [x] I2C_Read 加入 buffer overflow 保護
- [x] 所有 memcpy/memset 加入長度驗證
- [x] HID_CmdI2CWrite/Read/WriteRead/Scan 加入邊界檢查

## 進度

✅ 完成
