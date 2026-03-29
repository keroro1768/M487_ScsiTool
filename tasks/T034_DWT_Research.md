# T034 研究：DWT Breakpoint / Watchpoint Debug

## ARM CoreSight DWT Overview

M487 的 ARM Cortex-M4F core 內建 Debug Watchpoint and Trigger (DWT) 單元。

### 基本資訊

- **Base Address**: 0xE0001000
- **文件**: ARMv7-M Architecture Reference Manual (DWT 章節)
- **供電**: Core power domain（debug power island）

### DWT 功能

| 功能 | 數量 | 說明 |
|------|------|------|
| Hardware Breakpoint (IWM) | 最多 6 個 | Instruction Fetch matching |
| Hardware Watchpoint (DWT Comparator) | 最多 4 個 | Data access matching |
| PC Sampling | 1 個 | 週期性 PC 取樣 |
| Exception Tracing | - | Configurable |

## DWT Registers

所有暫存器皆為 32-bit，可透過 APB 或 processor core register access。

### 1. DWT_CTRL (0xE0001000) - Control

```
Bit[0]    NOCMP     - No comparator match (write 1 to clear)
Bit[1]    NOCY      - No cycle counter overflow
Bit[2]    NOPRFCNT  - No profiling counter overflow
Bit[3]    NOTRCPKT  - No trace packet output
Bit[16]   EXCEVTENA - Exception trace enable
Bit[17]   PCSAMPLENA - PC sample enable
Bit[20]   CYCDBGENA - Clock cycle debug enable
Bit[21]   POSTCNTENA - Postfinal counter enable
Bit[22]   POSTPRESET - Postfinal counter preset
Bit[23]   POSTINIT   - Postfinal counter initial
Bit[24]   TRCENA     - Trace enable (MUST set to 1)
```

### 2. DWT_CYCCNT (0xE0001004) - Cycle Counter

- 32-bit up counter
- Counts every clock cycle
- 需要 DWT_CTRL.CYCDBGENA = 1

### 3. DWT_COMPn (0xE0001010 + n*16) - Comparator n

用於 breakpoint 或 watchpoint 的 address/data matching。

### 4. DWT_MASKn (0xE0001014 + n*16) - Mask n

決定 address matching 的粒度。

### 5. DWT_FUNCTIONn (0xE0001018 + n*16) - Function n

設定 comparator 的行為：

```
Function = 0x00: Disabled
Function = 0x01: Instruction fetch address match (breakpoint)
Function = 0x02: Data address watchpoint (load/store)
Function = 0x03: Data address watchpoint (load only)
Function = 0x04: Data address watchpoint (store only)
Function = 0x05: Instruction + Data address (breakpoint)
```

### 6. DWT_PCSR (0xE000101C) - Program Counter Sample

唯讀，取樣當前 PC 值。

## 使用方式：設定 Hardware Breakpoint

```c
// 假設要在 addr 0x10000200 設定 breakpoint
#define DWT_BASE   0xE0001000
#define DWT_COMP0  (*(volatile uint32_t *)(DWT_BASE + 0x10))
#define DWT_MASK0  (*(volatile uint32_t *)(DWT_BASE + 0x14))
#define DWT_FNCT0  (*(volatile uint32_t *)(DWT_BASE + 0x18))
#define DWT_CTRL   (*(volatile uint32_t *)(DWT_BASE + 0x00))

void set_breakpoint(uint32_t addr) {
    // 確認 DWT 已啟用
    DWT_CTRL |= (1 << 24) | (1 << 21);  // TRCENA | CYCDBGENA
    
    // 設定 comparator 0
    DWT_COMP0 = addr;
    DWT_MASK0 = 0;  // exact address match (0 = no masking)
    DWT_FNCT0 = 0x01;  // instruction fetch match = breakpoint
    
    // 等待完成
    __asm volatile ("dsb");
    __asm volatile ("isb");
}

void clear_breakpoint(void) {
    DWT_FNCT0 = 0x00;
    __asm volatile ("dsb");
}
```

## 使用方式：設定 Hardware Watchpoint

```c
// 假設要 watch address 0x20001000 的寫入
void set_watchpoint_write(uint32_t addr) {
    DWT_CTRL |= (1 << 24) | (1 << 21);
    
    DWT_COMP1 = addr;
    DWT_MASK1 = 0;
    DWT_FNCT1 = 0x04;  // store address match (write watchpoint)
    
    __asm volatile ("dsb");
    __asm volatile ("isb");
}

// watch read 或 write
void set_watchpoint_read_write(uint32_t addr) {
    DWT_CTRL |= (1 << 24) | (1 << 21);
    
    DWT_COMP2 = addr;
    DWT_MASK2 = 0;
    DWT_FNCT2 = 0x02;  // data address watchpoint (load or store)
    
    __asm volatile ("dsb");
    __asm volatile ("isb");
}
```

## M487 ITM 整合

ITM (Instrumentation Trace Macrocell) 和 DWT 共用同一個 clock domain。

ITM 可輸出 DWT event：
- DWT exception trace
- PC samples
- Data watchpoint events

```c
// 啟用 DWT cycle counter + ITM trace
DWT_CTRL = (1 << 24) | (1 << 21) | (1 << 20);  // TRCENA | CYCDBGENA | ...

// ITM 啟用（見 itm.c）
// ITM_Log() 現在可以使用 DWT cycle counter 作為 timestamp
```

## ITM Software Trace vs DWT Hardware Trace

| 特性 | ITM SWO | DWT |
|------|---------|-----|
| 輸出 | PB8 SWO pin | DWT 單元（硬體觸發）|
| 用途 | 軟體驅動 log | Breakpoint / Watchpoint |
| 觸發 | ITM_Log() call | Address / Data match |
| PC 追蹤 | N/A | Yes (PC Sampling) |

## M487 DWT + MSC Debug Channel 整合

MSC Debug Channel 可擴展：
- `DBG_BREAK_SET = 0x10` - 設定 HW breakpoint
- `DBG_BREAK_CLR = 0x11` - 清除 HW breakpoint
- `DBG_WATCH_SET = 0x12` - 設定 HW watchpoint
- `DBG_WATCH_CLR = 0x13` - 清除 HW watchpoint
- `DBG_PC_SAMPLE = 0x14` - 讀取 PC sample

實作時需要：
1. 在 `msc_debug.c` 加入 DWT register 操作
2. firmware 保留 DWT function table
3. Host tool 呼叫對應 MSC command

## 文件參考

- ARMv7-M ARM: Section C1 (Debug Architecture)
- ARMv7-M ARM: Section C20 (DWT)
- M487 Datasheet: Section on ARM CoreSight
