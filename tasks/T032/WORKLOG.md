# T032 - Self-Test Mode

## 工作日誌 (WORKLOG.md)

### 任务描述
开机晶片内建 self-test

### 状态
✅ Implemented

### 相关文件
- `firmware/composite/self_test.h` — header
- `firmware/composite/self_test.c` — implementation
- `firmware/composite/gcc_arm_selftest.ld` — linker script (160 KB RAM variant)
- `firmware/composite/main.c` — SelfTest_Init + SelfTest_RunAll at boot
- `firmware/composite/dwt/dwt.h` — added `#include <stdbool.h>`
- `firmware/composite/build_gcc/Makefile` — added self_test.c, dwt.c, new linker script

### 时间记录
| 日期 | 工作内容 | 负责人 | 备注 |
|------|---------|--------|------|
| 2026-03-28 | 實作 self_test.h/c，含 HXT/PLL/SRAM/I2C/USB_PHY/DWT 測試 | Giroro |  |
| 2026-03-28 | 建立 gcc_arm_selftest.ld（160 KB RAM variant）| Giroro |  |
| 2026-03-28 | 更新 main.c 在開機時執行 self-test | Giroro |  |
| 2026-03-28 | 更新 Makefile 加入 self_test.c、dwt.c、新 linker script | Giroro |  |
| 2026-03-28 | 更新 Task.md 狀態 → implemented | Giroro |  |

