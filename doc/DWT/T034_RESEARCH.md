# T034 - DWT Breakpoint/Watchpoint Debug 研究

**狀態:** 🔄 Ongoing
**起始:** 2026-03-27
**優先:** P2
**負責:** 🐱 Giroro

## 目標

利用 ARM CoreSight DWT (Data Watchpoint and Trace) 單元實現硬體 breakpoint/watchpoint。

## DWT 概述

DWT (Data Watchpoint and Trace) 是 ARM Cortex-M4 內建的調試單元，提供：

- **6 個硬體斷點 (I breakpoint comparators)**
- **4 個 watchpoint (DWT comparators)**
- **PC sampling**
- **Exception tracing**
- **Cycle counter**

## M487 DWT 規格

根據 ARM Cortex-M4 TRM:
- DWT 基址: `0xE0001000`
- 6 個指令觀測點 (IWM/IWRS)
- 4 個數據觀測點 (DWT Comparator)

## 寄存器

### DWT Control Register (0xE0001000)
```
31:28 - NUMCOMP: Number of comparators (4 for M4)
17    - CYCDBG: Cycle Debug enable
16    - CYCCNTENA: Cycle counter enable
1     - DWTRESET: DWT reset
0     - DWTENA: DWT enable
```

### Comparator Registers (per comparator, 0-5)
- `DWT_COMP0-5`: 比較的地址
- `DWT_MASK0-5`: 地址屏蔽位 (0-31)
- `DWT_FUNCTION0-5`: 比較功能

### Function Codes
```
0x00000000 - Disabled
0x00000001 - Instruction fetch breakpoint
0x00000002 - Data write watchpoint
0x00000004 - Data read watchpoint
0x00000008 - Data read/write watchpoint
```

## 實作

### 1. DWT 初始化

```c
void DWT_Init(void) {
    // Enable DWT
    DWT->CTRL |= DWT_CTRL_DWTENA_Msk;
    
    // Enable cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // Clear any pending matches
    DWT->FUNCTION0 = 0;
}
```

### 2. 設置硬體斷點

```c
int32_t DWT_SetBreakpoint(uint32_t addr) {
    int i;
    
    // Find free comparator
    for (i = 0; i < 6; i++) {
        if ((DWT->FUNCTION[i] & 0xF) == 0) {
            DWT->COMP[i] = addr;
            DWT->MASK[i] = 0;  // No masking
            DWT->FUNCTION[i] = 0x01;  // Instruction fetch breakpoint
            return 0;
        }
    }
    return -1;  // No free comparator
}
```

### 3. 清除斷點

```c
int32_t DWT_ClearBreakpoint(uint32_t addr) {
    int i;
    
    for (i = 0; i < 6; i++) {
        if (DWT->COMP[i] == addr && (DWT->FUNCTION[i] & 0xF) == 0x01) {
            DWT->FUNCTION[i] = 0;
            return 0;
        }
    }
    return -1;
}
```

### 4. 設置 Watchpoint

```c
int32_t DWT_SetWatchpoint(uint32_t addr, uint32_t size, int type) {
    int i;
    uint32_t func;
    
    // Find free comparator
    for (i = 0; i < 4; i++) {
        if ((DWT->FUNCTION[i] & 0xF) == 0) {
            DWT->COMP[i] = addr;
            DWT->MASK[i] = 0;  // Compare full address
            
            // type: 0=write, 1=read, 2=read/write
            func = (type == 0) ? 0x02 : (type == 1) ? 0x04 : 0x08;
            DWT->FUNCTION[i] = func;
            return 0;
        }
    }
    return -1;
}
```

## 整合 ITM/SWO

DWT 事件可透過 ITM/SWO 輸出：

```c
// Enable ITM and SWO output
ITM->LAR = 0xC5ACCE55;  // Unlock ITM
ITM->TER |= (1 << 0);    // Enable stimulus port 0
ITM->TCR |= ITM_TCR_ITMENA_Msk;

// DWT event to ITM
DWT->FUNCTION1 = 0x01;  // Link to ITM
```

## 與 GDB RSP 整合

GDB RSP 的 `Z1` (hardware breakpoint) 命令可以使用 DWT：

```
Z1,ADDR,LENGTH - Set hardware breakpoint using DWT
z1,ADDR,LENGTH - Clear hardware breakpoint
```

## 實作文件

待創建:
- `firmware/composite/dwt.h` - DWT API
- `firmware/composite/dwt.c` - DWT 實作

## 依賴

- T024: ITM/SWO System ✅ (已完成)
- T033: GDB RSP (可並行)
