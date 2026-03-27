# T017 - flash.bat Error Handling 強化

## 工作日誌 (WORKLOG.md)

### 状态
✅ Finish（2026-03-26, Tamama 执行）

### 工作内容
- 路径改为 OPENOCD_ROOT 环境变数 + 相对路径
- 加入 tool 存在性检查（OpenOCD binary、scripts、目录）
- 加入 firmware binary 存在性验证
- 加入 --verify 烧录验证（预设启用，VERIFY=0 可停用）
- 加入错误码检查与 early exit
- 详细 [INFO] / [ERROR] / [SUCCESS] 区分输出

### Commit
7711083 (part of Tamama Sprint)
