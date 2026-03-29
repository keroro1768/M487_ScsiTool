# Task.md — T032

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T032 |
| 標題 | Self-Test Mode（開機自我檢測）|
| 狀態 | implemented |
| 優先序 | P2 |
| 指派 | Giroro |
| 依賴 | T024/T025 完成 |
| 截止 | - |

## 目標

開機時執行晶片內建 self-test，及早發現硬體問題

## 需求

- [x] Clock verification (HXT 12MHz ±5%, PLL 192MHz ±10%)
- [x] SRAM March test (MATS+ algorithm, tests last 32 KB of SRAM)
- [x] USB PHY presence check (USBIPHCT register accessibility)
- [x] I2C bus sanity check (register accessibility)
- [x] DWT unit functional check (unit present, cycle counter works, breakpoint settable)
- [x] MSC Debug Channel result access (results at fixed RAM 0x2000FFF0)

## 實作檔案

- `firmware/composite/self_test.h` — header with SelfTest_Result_t, test IDs, API declarations
- `firmware/composite/self_test.c` — implementation of all tests
- `firmware/composite/gcc_arm_selftest.ld` — linker script (160 KB RAM, gcc_arm_selftest variant)
- `firmware/composite/main.c` — SelfTest_Init() + SelfTest_RunAll() called at boot (after SYS_Init, before I2C/USB)

## 使用方式

```c
#include "self_test.h"
SelfTest_Init();
SelfTest_Result_t result;
SelfTest_RunAll(&result);
// Results printed to UART + ITM
// Results also at fixed RAM 0x2000FFF0 (16 bytes)
// Host can read via MSC DBG_READ_MEM(0x2000FFF0, 16)
```

## 測試項目

| ID | 名稱 | 方法 | 備註 |
|----|------|------|------|
| 0 | HXT Clock | CLK_GetHXTFreq() vs 12 MHz ±5% | 外部晶體 |
| 1 | PLL Clock | CLK_GetHCLKFreq() vs 192 MHz ±10% | 主時脈 |
| 2 | SRAM March | MATS+ algorithm, 0x20020000-0x20028000 | 32 KB 區域 |
| 3 | I2C Bus | Register accessibility check | UI2C0 |
| 4 | USB PHY | USBIPHCT register read | 不做loopback |
| 5 | DWT Unit | NUMCOMP>0, cycle counter, breakpoint | CoreSight |

## 限制/已知問題

- USB PHY loopback 未實作（M487 USB PHY 不支援 internal loopback，需外部設備）
- I2C bus test 僅檢查暫存器可及性，不傳送實際資料框架
- SRAM March test 使用最後 32 KB（避開 .data/.bss/.heap 區段）
- 結果 RAM 位址 0x2000FFF0 由程式直接定址，不依賴 linker section
