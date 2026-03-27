# T022 - Buffer 邊界檢查補全

## 工作日誌 (WORKLOG.md)

### 状态
✅ Finish（2026-03-26, Giroro 执行）

### 工作内容
- EPB_Handler()：加入 len <= EPB_MAX_PKT_SIZE 验证
- HID_CmdI2CWrite/Read/WriteRead/Scan：所有 memcpy 操作加入边界检查
- I2C_Read()：buffer overflow 保护

### Commit
7711083 firmware/composite: T014/T015/T021/T022 - NACK retry, unified error codes, buffer bounds
