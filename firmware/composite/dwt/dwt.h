/**
 * @file     dwt.h
 * @brief    ARM CoreSight DWT (Data Watchpoint and Trace) Driver
 * @version  1.0.0
 * 
 * DWT provides hardware breakpoints and watchpoints for ARM Cortex-M4.
 * 
 * Features:
 * - 6 hardware breakpoints (instruction fetch matching)
 * - 4 data watchpoints (read/write/access watching)
 * - Cycle counter
 * - Exception tracing
 */

#ifndef DWT_H
#define DWT_H

#include <stdint.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------------------------------------*/
/* DWT Register Base Address                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define DWT_BASE              (0xE0001000UL)

/*---------------------------------------------------------------------------------------------------------*/
/* DWT Register Offsets                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define DWT_CTRL_OFFSET      0x000   /* Control register */
#define DWT_CYCCNT_OFFSET    0x004   /* Cycle counter */
#define DWT_CPICNT_OFFSET    0x008   /* CPI counter */
#define DWT_EXCCNT_OFFSET    0x00C   /* Exception overhead counter */
#define DWT_SLEEPCNT_OFFSET  0x010   /* Sleep counter */
#define DWT_LSUCNT_OFFSET    0x014   /* LSU counter */
#define DWT_FOLDCNT_OFFSET   0x018   /* Folded instruction counter */
#define DWT_COMP_BASE        0x020   /* Comparator base (0-5) */
#define DWT_MASK_BASE        0x024   /* Mask base (0-5) */
#define DWT_FUNCTION_BASE    0x028   /* Function base (0-5) */

/*---------------------------------------------------------------------------------------------------------*/
/* DWT Control Register Bits                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define DWT_CTRL_DWTENA_Pos      16  /* DWT enable */
#define DWT_CTRL_DWTENA_Msk      (1UL << DWT_CTRL_DWTENA_Pos)
#define DWT_CTRL_HALTENA_Pos     18  /* Halt enable */
#define DWT_CTRL_HALTENA_Msk     (1UL << DWT_CTRL_HALTENA_Pos)
#define DWT_CTRL_CYCCNTENA_Pos   0   /* Cycle counter enable */
#define DWT_CTRL_CYCCNTENA_Msk   (1UL << DWT_CTRL_CYCCNTENA_Pos)
#define DWT_CTRL_EXCTRCENA_Pos   16  /* Exception trace enable */
#define DWT_CTRL_EXCTRCENA_Msk   (1UL << DWT_CTRL_EXCTRCENA_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/* DWT Function Register Values                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define DWT_FUNCTION_DISABLED    0x00000000  /* Disabled */
#define DWT_FUNCTION_IBP        0x00000001  /* Instruction fetch breakpoint */
#define DWT_FUNCTION_DWP_WRITE   0x00000002  /* Data write watchpoint */
#define DWT_FUNCTION_DWP_READ     0x00000004  /* Data read watchpoint */
#define DWT_FUNCTION_DWP_ACCESS  0x00000008  /* Data access watchpoint */
#define DWT_FUNCTION_LINK        0x00000100  /* Link to ITM */

/*---------------------------------------------------------------------------------------------------------*/
/* DWT Comparator Count                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define DWT_BREAKPOINT_COUNT   6   /* Hardware breakpoints */
#define DWT_WATCHPOINT_COUNT   4   /* Data watchpoints */
#define DWT_TOTAL_COMP         6   /* Total comparators */

/*---------------------------------------------------------------------------------------------------------*/
/* DWT API                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Initialize DWT unit
 * @return  0 on success, negative on error
 */
int32_t DWT_Init(void);

/**
 * @brief   Set hardware breakpoint at address
 * @param   addr    Target address (must be word-aligned for Thumb mode)
 * @return  >= 0: breakpoint ID, < 0: error (no free comparator)
 */
int32_t DWT_SetBreakpoint(uint32_t addr);

/**
 * @brief   Clear hardware breakpoint
 * @param   addr    Target address
 * @return  0 on success, negative if not found
 */
int32_t DWT_ClearBreakpoint(uint32_t addr);

/**
 * @brief   Set data watchpoint
 * @param   addr    Target address
 * @param   size    Size in bytes (1, 2, or 4)
 * @param   type    Watchpoint type: 0=write, 1=read, 2=read/write
 * @return  >= 0: watchpoint ID, < 0: error
 */
int32_t DWT_SetWatchpoint(uint32_t addr, uint32_t size, int type);

/**
 * @brief   Clear data watchpoint
 * @param   addr    Target address
 * @return  0 on success, negative if not found
 */
int32_t DWT_ClearWatchpoint(uint32_t addr);

/**
 * @brief   Get current cycle count
 * @return  Cycle count value
 */
uint32_t DWT_GetCycleCount(void);

/**
 * @brief   Reset cycle counter
 */
void DWT_ResetCycleCount(void);

/**
 * @brief   Enable cycle counter
 */
void DWT_EnableCycleCounter(void);

/**
 * @brief   Disable cycle counter
 */
void DWT_DisableCycleCounter(void);

/**
 * @brief   Check if DWT is available (for debugging)
 * @return  true if DWT is present
 */
uint32_t DWT_IsAvailable(void);

/**
 * @brief   Get DWT exception trace
 * @return  Exception trace value
 */
uint32_t DWT_GetExceptionTrace(void);

/**
 * @brief   Enable DWT debug event generation
 * @param   enable  true to enable, false to disable
 */
void DWT_EnableDebugEvents(uint32_t enable);

#endif /* DWT_H */
