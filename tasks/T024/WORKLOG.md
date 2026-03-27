# T024 - ITM/SWO Trace System

## 工作日誌 (WORKLOG.md)

### 状态
✅ Finish（2026-03-27, Giroro 执行）

### 工作内容（2026-03-27）
**新建文件：**
- irmware/composite/itm.h — ITM log level, macros
- irmware/composite/itm.c — ITM_Init, ITM_Log, ITM_HexDump

**核心功能：**
- ITM_Init() / ITM_InitWithBaud() — TPI/SWO 设定
- ITM_Log() / ITM_ERR() / ITM_DBG() / ITM_HEX_DUMP()
- DWT cycle counter — 时间戳记
- TPI SWO baud rate config (default 2MHz)
- PB8 SWO pin 预留（需确认正确 MFP 值）

**Module-specific macros：**
- USB_TRACE, MSC_TRACE, I2C_TRACE, HID_TRACE

**整合：**
- irmware/composite/main.c — ITM_Init() 整合
- irmware/composite/build_gcc/Makefile — 加入 itm.c

### 时间记录
| 日期 | 工作内容 | 负责人 | 备注 |
|------|---------|--------|------|
| 2026-03-27 | ITM/SWO Trace System 实作 | Giroro | |

### Commit
265cb4c - Reorganize: unify workspace, add tasks/docs, new debug modules
