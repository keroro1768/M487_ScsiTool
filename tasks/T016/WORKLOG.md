# T016 - Makefile 環境變數重構

## 工作日誌 (WORKLOG.md)

### 状态
✅ Finish（2026-03-26, Tamama 执行）

### 工作内容
- 将 C:/Users/rinry/Tool/xpack-... 替換為 $(XPKG_ROOT)
- 将 D:/AiWorkSpace/KM/M480BSP 替換為 $(BSP_DIR)
- 新增 Makefile.config.example 供新环境参考
- 新增 $(OPENOCD_ROOT) 统一管理 OpenOCD 路径

### Commit
7711083 (part of Tamama Sprint)
