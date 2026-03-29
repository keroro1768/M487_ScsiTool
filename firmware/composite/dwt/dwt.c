/**
 * @file     dwt.c
 * @brief    ARM CoreSight DWT (Data Watchpoint and Trace) Driver Implementation
 * @version  1.0.0
 * 
 * Implements hardware breakpoints and watchpoints using ARM Cortex-M4 DWT.
 */

#include <stddef.h>
#include "dwt.h"

/*---------------------------------------------------------------------------------------------------------*/
/* DWT Register Access                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define DWT_CTRL           (*(volatile uint32_t *)(DWT_BASE + DWT_CTRL_OFFSET))
#define DWT_CYCCNT         (*(volatile uint32_t *)(DWT_BASE + DWT_CYCCNT_OFFSET))
#define DWT_CPICNT         (*(volatile uint32_t *)(DWT_BASE + DWT_CPICNT_OFFSET))
#define DWT_EXCCNT         (*(volatile uint32_t *)(DWT_BASE + DWT_EXCCNT_OFFSET))
#define DWT_SLEEPCNT       (*(volatile uint32_t *)(DWT_BASE + DWT_SLEEPCNT_OFFSET))
#define DWT_LSUCNT         (*(volatile uint32_t *)(DWT_BASE + DWT_LSUCNT_OFFSET))
#define DWT_FOLDCNT        (*(volatile uint32_t *)(DWT_BASE + DWT_FOLDCNT_OFFSET))

/*---------------------------------------------------------------------------------------------------------*/
/* DWT Comparator Registers                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#define DWT_COMP(i)        (*(volatile uint32_t *)(DWT_BASE + DWT_COMP_BASE + (i) * 12))
#define DWT_MASK(i)        (*(volatile uint32_t *)(DWT_BASE + DWT_MASK_BASE + (i) * 12))
#define DWT_FUNCTION(i)    (*(volatile uint32_t *)(DWT_BASE + DWT_FUNCTION_BASE + (i) * 12))

/*---------------------------------------------------------------------------------------------------------*/
/* Internal State                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    bool    active;
    uint32_t addr;
    uint8_t  type;   /* 0=breakpoint, 1=watchpoint */
} DWT_Entry_t;

static DWT_Entry_t s_breakpoints[DWT_BREAKPOINT_COUNT] = {0};
static DWT_Entry_t s_watchpoints[DWT_WATCHPOINT_COUNT] = {0};

/*---------------------------------------------------------------------------------------------------------*/
/* Helper Functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Check if an address has an active breakpoint
 * @param   addr    Address to check
 * @return  >= 0: index of breakpoint, < 0: not found
 */
static int32_t find_breakpoint(uint32_t addr) {
    int i;
    for (i = 0; i < DWT_BREAKPOINT_COUNT; i++) {
        if (s_breakpoints[i].active && s_breakpoints[i].addr == addr) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief   Find free breakpoint slot
 * @return  >= 0: free slot index, < 0: no free slot
 */
static int32_t find_free_breakpoint_slot(void) {
    int i;
    for (i = 0; i < DWT_BREAKPOINT_COUNT; i++) {
        if (!s_breakpoints[i].active) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief   Find watchpoint by address
 * @param   addr    Address to check
 * @return  >= 0: index of watchpoint, < 0: not found
 */
static int32_t find_watchpoint(uint32_t addr) {
    int i;
    for (i = 0; i < DWT_WATCHPOINT_COUNT; i++) {
        if (s_watchpoints[i].active && s_watchpoints[i].addr == addr) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief   Find free watchpoint slot
 * @return  >= 0: free slot index, < 0: no free slot
 */
static int32_t find_free_watchpoint_slot(void) {
    int i;
    for (i = 0; i < DWT_WATCHPOINT_COUNT; i++) {
        if (!s_watchpoints[i].active) {
            return i;
        }
    }
    return -1;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Public API Implementation                                                                                */
/*---------------------------------------------------------------------------------------------------------*/

int32_t DWT_Init(void) {
    int i;
    
    /* Reset state */
    for (i = 0; i < DWT_BREAKPOINT_COUNT; i++) {
        s_breakpoints[i].active = false;
        s_breakpoints[i].addr = 0;
        s_breakpoints[i].type = 0;
        DWT_FUNCTION(i) = 0;  /* Disable */
    }
    
    for (i = 0; i < DWT_WATCHPOINT_COUNT; i++) {
        s_watchpoints[i].active = false;
        s_watchpoints[i].addr = 0;
        s_watchpoints[i].type = 0;
    }
    
    for (i = 0; i < DWT_WATCHPOINT_COUNT; i++) {
        s_watchpoints[i].active = false;
        s_watchpoints[i].addr = 0;
        s_watchpoints[i].type = 0;
    }
    
    /* Enable DWT */
    DWT_CTRL |= DWT_CTRL_DWTENA_Msk;
    
    /* Reset counters */
    DWT_CYCCNT = 0;
    
    return 0;
}

int32_t DWT_SetBreakpoint(uint32_t addr) {
    int32_t slot;
    
    /* Check if breakpoint already exists at this address */
    slot = find_breakpoint(addr);
    if (slot >= 0) {
        return slot;  /* Already set */
    }
    
    /* Find free slot */
    slot = find_free_breakpoint_slot();
    if (slot < 0) {
        return -1;  /* No free slot */
    }
    
    /* Configure comparator */
    DWT_COMP(slot) = addr;
    DWT_MASK(slot) = 0;  /* No masking - match exact address */
    DWT_FUNCTION(slot) = DWT_FUNCTION_IBP;  /* Instruction breakpoint */
    
    /* Update state */
    s_breakpoints[slot].active = true;
    s_breakpoints[slot].addr = addr;
    s_breakpoints[slot].type = 0;  /* Breakpoint */
    
    return slot;
}

int32_t DWT_ClearBreakpoint(uint32_t addr) {
    int32_t slot;
    
    slot = find_breakpoint(addr);
    if (slot < 0) {
        return -1;  /* Not found */
    }
    
    /* Disable comparator */
    DWT_FUNCTION(slot) = 0;
    
    /* Update state */
    s_breakpoints[slot].active = false;
    s_breakpoints[slot].addr = 0;
    
    return 0;
}

int32_t DWT_SetWatchpoint(uint32_t addr, uint32_t size, int type) {
    int32_t slot;
    uint32_t function;
    uint32_t mask;
    
    /* Check if watchpoint already exists */
    slot = find_watchpoint(addr);
    if (slot >= 0) {
        return slot;
    }
    
    /* Find free slot (use comparators 0-3 for watchpoints) */
    slot = find_free_watchpoint_slot();
    if (slot < 0) {
        return -1;
    }
    
    /* Calculate mask based on size */
    /* For simplicity, we use mask=0 (exact match) for all sizes */
    mask = 0;
    
    /* Select function based on type */
    switch (type) {
        case 0: function = DWT_FUNCTION_DWP_WRITE; break;    /* Write */
        case 1: function = DWT_FUNCTION_DWP_READ; break;    /* Read */
        case 2: function = DWT_FUNCTION_DWP_ACCESS; break; /* Read/Write */
        default: return -1;
    }
    
    /* Configure comparator */
    DWT_COMP(slot) = addr;
    DWT_MASK(slot) = mask;
    DWT_FUNCTION(slot) = function;
    
    /* Update state */
    s_watchpoints[slot].active = true;
    s_watchpoints[slot].addr = addr;
    s_watchpoints[slot].type = (uint8_t)(type + 1);
    
    return slot;
}

int32_t DWT_ClearWatchpoint(uint32_t addr) {
    int32_t slot;
    
    slot = find_watchpoint(addr);
    if (slot < 0) {
        return -1;
    }
    
    /* Disable comparator */
    DWT_FUNCTION(slot) = 0;
    
    /* Update state */
    s_watchpoints[slot].active = false;
    s_watchpoints[slot].addr = 0;
    
    return 0;
}

uint32_t DWT_GetCycleCount(void) {
    return DWT_CYCCNT;
}

void DWT_ResetCycleCount(void) {
    DWT_CYCCNT = 0;
}

void DWT_EnableCycleCounter(void) {
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void DWT_DisableCycleCounter(void) {
    DWT_CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t DWT_IsAvailable(void) {
    /* Check if DWT is present by checking NUMCOMP field */
    uint32_t ctrl = DWT_CTRL;
    uint32_t numcomp = (ctrl >> 28) & 0xF;
    return (numcomp > 0) ? 1 : 0;
}

uint32_t DWT_GetExceptionTrace(void) {
    /* Exception trace is in the first 4 comparators' function registers */
    return 0;  /* Simplified - full implementation would track exception entry/exit */
}

void DWT_EnableDebugEvents(uint32_t enable) {
    if (enable) {
        DWT_CTRL |= DWT_CTRL_DWTENA_Msk | DWT_CTRL_EXCTRCENA_Msk;
    } else {
        DWT_CTRL &= ~(DWT_CTRL_DWTENA_Msk | DWT_CTRL_EXCTRCENA_Msk);
    }
}
