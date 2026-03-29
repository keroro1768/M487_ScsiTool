/**
 * @file     self_test.h
 * @brief    M487 Self-Test Module - Boot-Time Hardware Verification
 * @version  1.0.0
 *
 * Executes hardware self-tests at boot to detect defects early.
 * Results are stored in a designated RAM region (MSC Debug Channel readable).
 *
 * Test Coverage:
 *   - Clock verification (HXT, PLL frequency via DWT cycle counter)
 *   - SRAM March test (full RAM walk pattern test)
 *   - I2C bus sanity check (bus not stuck, device ping)
 *   - USB PHY presence check (USB PHY registers accessible)
 *
 * Usage:
 *   #include "self_test.h"
 *   SelfTest_Result_t result;
 *   SelfTest_RunAll(&result);
 *   if (result.u32FailedMask != 0) { }  // handle failure
 *
 * Result RAM Location:
 *   SELF_TEST_RAM_BASE = 0x2000FFF0  (last 16 bytes of SRAM)
 *   Host can read via MSC DBG_READ_MEM (DBG_READ_MEM sub-cmd).
 *
 * Build Requirements:
 *   - self_test.c must be compiled with the project
 *   - Use gcc_arm_selftest.ld linker script (160 KB RAM + .selftest section)
 */

#ifndef SELF_TEST_H
#define SELF_TEST_H

#include <stdint.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------------------------------------*/
/* Self-Test Result RAM Location                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
/** @note These symbols must be placed at fixed addresses by the linker script. */
#define SELF_TEST_RAM_BASE       (0x2000FFF0UL)   /**< Last 16 bytes of SRAM */
#define SELF_TEST_RAM_SIZE       (16U)

/*---------------------------------------------------------------------------------------------------------*/
/* Self-Test Item IDs                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Self-Test Item IDs */
/** @{ */
#define SELF_TEST_ITEM_HXT          0   /**< HXT (External Crystal) clock test */
#define SELF_TEST_ITEM_PLL           1   /**< PLL clock test */
#define SELF_TEST_ITEM_SRAM          2   /**< SRAM March test */
#define SELF_TEST_ITEM_I2C           3   /**< I2C bus sanity check */
#define SELF_TEST_ITEM_USB_PHY       4   /**< USB PHY presence check */
#define SELF_TEST_ITEM_DWT          5   /**< DWT unit functional check */
#define SELF_TEST_ITEM_COUNT         6   /**< Total number of test items */
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Self-Test Status Codes                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/** @name Self-Test Status Codes */
/** @{ */
#define SELF_TEST_STATUS_NOT_RUN     0x00  /**< Test has not been run */
#define SELF_TEST_STATUS_PASS        0x01  /**< Test passed */
#define SELF_TEST_STATUS_FAIL        0x02  /**< Test failed */
#define SELF_TEST_STATUS_SKIP        0x03  /**< Test skipped (hardware not present) */
#define SELF_TEST_STATUS_TIMEOUT     0x04  /**< Test timed out */
/** @} */

/*---------------------------------------------------------------------------------------------------------*/
/* Self-Test Result Structure                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief  Self-test result structure
 *
 * Placed at a fixed RAM address (SELF_TEST_RAM_BASE) so the MSC Debug
 * Channel host can read results without any special commands.
 *
 * Layout (16 bytes total):
 *   Offset 0x00: u32FailedMask   - Bitmask of failed tests
 *   Offset 0x04: u32PassedMask   - Bitmask of passed tests
 *   Offset 0x08: u32SkippedMask   - Bitmask of skipped tests
 *   Offset 0x0C: u32Timestamp[0] - DWT cycle count at start
 *   Offset 0x10: u32Timestamp[1] - DWT cycle count at end
 *
 * Bit N of any mask corresponds to SELF_TEST_ITEM_N.
 */
#pragma pack(push, 1)
typedef struct {
    uint32_t    u32FailedMask;     /**< Bitmask of failed test items */
    uint32_t    u32PassedMask;     /**< Bitmask of passed test items */
    uint32_t    u32SkippedMask;    /**< Bitmask of skipped test items */
    uint32_t    u32Timestamp[2];  /**< DWT cycle count [0]=start [1]=end */
} SelfTest_Result_t;
#pragma pack(pop)

/*---------------------------------------------------------------------------------------------------------*/
/* Helper Macros                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
/** Helper to build the passed/skipped/failed masks from individual test results */
#define SELF_TEST_MASK_ITEM(id)          (1UL << (id))
#define SELF_TEST_MASK_ALL              ((1UL << SELF_TEST_ITEM_COUNT) - 1)

/*---------------------------------------------------------------------------------------------------------*/
/* Function Declarations                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief  Run all self-tests
 * @param  pResult  Pointer to result structure (must be in RAM)
 * @return 0 if all tests passed, non-zero (u32FailedMask) if any failed
 *
 * @note Results are also written to SELF_TEST_RAM_BASE for MSC Debug Channel access.
 *
 * Test execution order:
 *   1. DWT cycle counter init (prerequisite for clock tests)
 *   2. HXT clock verification
 *   3. PLL clock verification
 *   4. SRAM March test
 *   5. I2C bus sanity check
 *   6. USB PHY presence check
 *   7. DWT functional check
 */
uint32_t SelfTest_RunAll(SelfTest_Result_t *pResult);

/**
 * @brief  Run individual HXT clock verification test
 * @param  pResult  Pointer to result structure
 *
 * Verifies the HXT (external crystal) is running by measuring
 * CLK_GetHXTCLK() against expected 12 MHz.
 *
 * Tolerance: ±5% (11.4 MHz - 12.6 MHz)
 * Method: DWT cycle counter with known HCLK divisor
 */
void SelfTest_TestHXT(SelfTest_Result_t *pResult);

/**
 * @brief  Run individual PLL clock verification test
 * @param  pResult  Pointer to result structure
 *
 * Verifies PLL is locked and generating expected frequency.
 * On M487, PLL should be 192 MHz when configured.
 *
 * Tolerance: ±10% of target PLL frequency
 * Method: DWT cycle counter measurement
 */
void SelfTest_TestPLL(SelfTest_Result_t *pResult);

/**
 * @brief  Run individual SRAM March test
 * @param  pResult  Pointer to result structure
 *
 * March test algorithm: MATS+ (Marching Addresses  Test)
 * - Tests all SRAM locations: ~160 KB (0x2000_0000 - 0x2002_8000)
 * - Preserves existing RAM contents (backup/restore)
 * - Checks for stuck-at faults, coupling faults, address decoder faults
 *
 * Time estimate: ~50 ms for 160 KB @ 192 MHz
 */
void SelfTest_TestSRAM(SelfTest_Result_t *pResult);

/**
 * @brief  Run individual I2C bus sanity check
 * @param  pResult  Pointer to result structure
 *
 * Checks:
 *   1. Bus not stuck (SDA/SCL lines can toggle)
 *   2. Bus can generate START condition
 *   3. Bus can generate STOP condition
 *   4. (Optional) Device presence scan on known addresses
 *
 * Requires I2C0 to be initialized before calling.
 */
void SelfTest_TestI2C(SelfTest_Result_t *pResult);

/**
 * @brief  Run individual USB PHY presence check
 * @param  pResult  Pointer to result structure
 *
 * Checks:
 *   1. USB PHY register accessibility (SYS->USBPHY)
 *   2. USB PHY ID register reads expected value
 *
 * Does NOT perform loopback test (requires USB host/device connection).
 */
void SelfTest_TestUSB_PHY(SelfTest_Result_t *pResult);

/**
 * @brief  Run individual DWT functional check
 * @param  pResult  Pointer to result structure
 *
 * Checks:
 *   1. DWT unit is present (NUMCOMP > 0)
 *   2. Cycle counter can start/stop/reset
 *   3. Breakpoint comparator can be set and fires correctly
 *
 * Must run first before using DWT timestamps in other tests.
 */
void SelfTest_TestDWT(SelfTest_Result_t *pResult);

/**
 * @brief  Initialize self-test module
 *
 * Initializes DWT for timestamp generation.
 * Call once at boot before any SelfTest_* functions.
 */
void SelfTest_Init(void);

/**
 * @brief  Get human-readable name for a test item
 * @param  id  Test item ID (SELF_TEST_ITEM_*)
 * @return Static string with test name (never NULL)
 */
const char* SelfTest_GetItemName(uint32_t id);

/**
 * @brief  Check if any test has failed
 * @param  pResult  Pointer to result structure
 * @return true if any test failed
 */
static inline bool SelfTest_HasFailed(const SelfTest_Result_t *pResult) {
    return (pResult->u32FailedMask != 0U);
}

/**
 * @brief  Get number of test items
 * @return Number of test items
 */
static inline uint32_t SelfTest_GetItemCount(void) {
    return SELF_TEST_ITEM_COUNT;
}

/*---------------------------------------------------------------------------------------------------------*/
/* External Symbols                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
extern uint32_t SystemCoreClock; /**< Current system core clock in Hz */

#endif /* SELF_TEST_H */
