# T015 - Magic Numbers 消除 + 統一錯誤碼

## 工作日誌 (WORKLOG.md)

### 任务描述
统一错误码，i2c_constants.h, i2c_error.h

### 状态
✅ Finish

### 工作内容（2026-03-26, Giroro 执行）

**新建文件：**
- irmware/composite/i2c_constants.h：timeout、retry、buffer size 等常量
- irmware/composite/i2c_error.h：统一错误码定义

**错误码定义：**
- I2C_OK = 0
- I2C_ERR_NACK = -1
- I2C_ERR_TIMEOUT = -2
- I2C_ERR_BUSY = -3
- I2C_ERR_NACK_RETRY_EXCEEDED = -4

**巨集：**
- I2C_IS_ERROR()
- I2C_IS_OK()
- I2C_ErrorString()

### 相关文件
- irmware/composite/i2c_constants.h
- irmware/composite/i2c_error.h
- irmware/composite/i2c_control.c

### Commit
7711083 firmware/composite: T014/T015/T021/T022 - NACK retry, unified error codes, buffer bounds
