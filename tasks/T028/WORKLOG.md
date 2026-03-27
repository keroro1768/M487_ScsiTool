# T028 - UART Debug Log System

## 工作日誌 (WORKLOG.md)

### 状态
✅ Finish（2026-03-27, Giroro 执行）

### 工作内容（2026-03-27）
**新建文件：**
- irmware/composite/uart_debug.h — Header, log macros, module macros
- irmware/composite/uart_debug.c — UART log 实作

**核心功能：**
- UART_DBG_Init() — 初始化 UART Debug
- UART_DBG_Log() — 格式化日誌输出
- UART_DBG_HexDump() — Hex dump
- UART_DBG_Timestamp() — 时间戳记
- _UART_Vsnprintf() — 嵌入式 vsnprintf 实作
- 支援 Polling 模式（预设）和 IRQ 模式

**日誌格式：**
[HH:MM:SS.mmm][LVL][module] message

**Module-specific macros：**
- MAIN_LOG/ERR/WARN/DBG/TRC
- USB_LOG/ERR/WARN/DBG/TRC
- MSC_LOG/ERR/WARN/DBG/TRC
- I2C_LOG/ERR/WARN/DBG/TRC
- HID_LOG/ERR/WARN/DBG/TRC

**整合：**
- irmware/composite/main.c — UART_DBG_Init() 整合
- irmware/composite/build_gcc/Makefile — 加入 uart_debug.c

### 时间记录
| 日期 | 工作内容 | 负责人 | 备注 |
|------|---------|--------|------|
| 2026-03-27 | UART Debug Log System 实作 | Giroro | |

### Commit
265cb4c - Reorganize: unify workspace, add tasks/docs, new debug modules
