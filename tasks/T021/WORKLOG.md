# T021 - BuildCommandRegister 截斷修復

## 工作日誌 (WORKLOG.md)

### 状态
✅ Finish（2026-03-26, Giroro 执行）

### 工作内容
- 分析：HID-over-I2C 规范明确定义 register 位址为 16-bit
- 发现：HID_Context_t 中 egReportDesc、egInput 等栏位为 uint8_t，会截断高位元组
- 修复：I2C0_ReadReg() 等函式支援 uint16_t reg

### Commit
7711083 firmware/composite: T014/T015/T021/T022 - NACK retry, unified error codes, buffer bounds
