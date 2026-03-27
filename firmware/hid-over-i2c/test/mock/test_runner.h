/**
 * @file    test_runner.h
 * @brief   Simple unit test framework for T008 module testing
 * @version 1.0.0
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Test Framework Macros
 *============================================================================*/

#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("[FAIL] %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("[FAIL] %s:%d: Expected %d, got %d (%s)\n", \
                   __FILE__, __LINE__, (int)(expected), (int)(actual), #actual); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_TRUE(actual)    TEST_ASSERT_EQUAL(true, (actual))
#define TEST_ASSERT_FALSE(actual)   TEST_ASSERT_EQUAL(false, (actual))
#define TEST_ASSERT_NULL(ptr)       TEST_ASSERT((ptr) == NULL)
#define TEST_ASSERT_NOT_NULL(ptr)   TEST_ASSERT((ptr) != NULL)

#define TEST_LOG(msg)               printf("[LOG]  %s\n", msg)

/*==============================================================================
 * Test Suite Types
 *============================================================================*/

typedef bool (*test_fn_t)(void);

typedef struct {
    const char *name;
    test_fn_t   fn;
} test_case_t;

typedef struct {
    const char  *name;
    test_case_t *cases;
    int         count;
} test_suite_t;

/*==============================================================================
 * Test Runner API
 *============================================================================*/

/**
 * @brief   Run a test suite and report results
 * @param   pSuite - Pointer to test suite
 * @return  Number of failed tests
 */
int test_runner_run(const test_suite_t *pSuite);

/**
 * @brief   Run a single test case
 * @param   pCase - Pointer to test case
 * @return  true if passed
 */
bool test_runner_run_case(const test_case_t *pCase);

#ifdef __cplusplus
}
#endif

#endif /* TEST_RUNNER_H */
