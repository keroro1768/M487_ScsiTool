/**
 * @file    test_runner.c
 * @brief   Simple unit test framework implementation
 */

#include <stdio.h>
#include "test_runner.h"

/*==============================================================================
 * Public API Implementation
 *============================================================================*/

bool test_runner_run_case(const test_case_t *pCase)
{
    printf("[RUN]  %s\n", pCase->name);
    bool result = pCase->fn();
    if (result) {
        printf("[PASS] %s\n", pCase->name);
    }
    return result;
}

int test_runner_run(const test_suite_t *pSuite)
{
    int passed = 0;
    int failed = 0;

    printf("\n========================================\n");
    printf("Test Suite: %s\n", pSuite->name);
    printf("========================================\n");

    for (int i = 0; i < pSuite->count; i++) {
        if (test_runner_run_case(&pSuite->cases[i])) {
            passed++;
        } else {
            failed++;
        }
    }

    printf("\n----------------------------------------\n");
    printf("Results: %d passed, %d failed, %d total\n", passed, failed, pSuite->count);
    printf("----------------------------------------\n");

    return failed;
}
