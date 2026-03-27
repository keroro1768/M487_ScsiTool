# T014 - I2C NACK Retry 機制

## 工作日誌 (WORKLOG.md)

### 任务描述
I2C NACK retry，3次重试 + exponential backoff

### 状态
✅ Finish

### 工作内容（2026-03-26, Giroro 执行）

- NACK 重试机制：NACK_RETRY_MAX = 3
- Exponential backoff：1ms → 2ms → 4ms（上限 100ms）
- 新错误码：I2C_ERR_NACK_RETRY_EXCEEDED
- 所有 Magic Numbers 替換為 #define 常量

### 相关文件
- irmware/composite/i2c_control.c

### 时间记录
| 日期 | 工作内容 | 负责人 | 备注 |
|------|---------|--------|------|
| 2026-03-26 | NACK Retry 实作 | Giroro | |

### Commit
7711083 firmware/composite: T014/T015/T021/T022 - NACK retry, unified error codes, buffer bounds
