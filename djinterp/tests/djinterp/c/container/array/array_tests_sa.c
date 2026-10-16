/*******************************************************************************
* djinterp [container]                                          array_tests_sa.c
*
* Comprehensive test runner for `array.h` unit tests.
* Contains aggregate test suite runners only.
*
*
* path:      /tests/djinterp/c/container/array/array_tests_sa.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.30
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_array_all
  Runs all core array tests.
*/
bool
d_tests_sa_array_all
(
    struct d_test_counter* _test_info
)
{
    bool                   overall_result = true;
    struct d_test_counter  local_counter = { 0, 0, 0, 0 };

    printf("\n========================================\n");
    printf("Running Core Array Module Tests\n");
    printf("========================================\n");

    // run d_array creation function tests
    overall_result &= d_tests_sa_array_run_creation_tests(&local_counter);

    // run d_array_s creation function tests
    overall_result &= d_tests_sa_array_run_creation_s_tests(&local_counter);

    // run manipulation function tests
    overall_result &= d_tests_sa_array_run_manipulation_tests(&local_counter);

    // run memory management tests
    overall_result &= d_tests_sa_array_run_memory_tests(&local_counter);

    // update main counter
    _test_info->assertions_total  += local_counter.assertions_total;
    _test_info->assertions_passed += local_counter.assertions_passed;
    _test_info->tests_total       += local_counter.tests_total;
    _test_info->tests_passed      += local_counter.tests_passed;

    // print results
    printf("\n========================================\n");
    printf("Core Array Test Results\n");
    printf("========================================\n");
    printf("Tests: %zu/%zu passed (%.2f%%)\n",
        local_counter.tests_passed, local_counter.tests_total,
        local_counter.tests_total > 0 ?
            (double)local_counter.tests_passed / local_counter.tests_total * 100.0 :
            0.0);
    printf("Assertions: %zu/%zu passed (%.2f%%)\n",
        local_counter.assertions_passed, local_counter.assertions_total,
        local_counter.assertions_total > 0 ?
            (double)local_counter.assertions_passed / local_counter.assertions_total * 100.0 :
            0.0);
    printf("Overall Status: %s\n", overall_result ? "PASSED" : "FAILED");
    printf("========================================\n");

    return overall_result;
}

/*
d_tests_sa_array_all_extended
  Runs all array tests including integration and edge cases.
*/
bool
d_tests_sa_array_all_extended
(
    struct d_test_counter* _test_info
)
{
    bool                   overall_result = true;
    struct d_test_counter  local_counter = { 0, 0, 0, 0 };

    printf("\n========================================\n");
    printf("Running COMPLETE Array Module Test Suite\n");
    printf("========================================\n");

    // run all core tests
    overall_result &= d_tests_sa_array_run_creation_tests(&local_counter);
    overall_result &= d_tests_sa_array_run_creation_s_tests(&local_counter);
    overall_result &= d_tests_sa_array_run_manipulation_tests(&local_counter);
    overall_result &= d_tests_sa_array_run_memory_tests(&local_counter);

    // run integration tests
    overall_result &= d_tests_sa_array_run_integration_tests(&local_counter);

    // update main counter
    _test_info->assertions_total  += local_counter.assertions_total;
    _test_info->assertions_passed += local_counter.assertions_passed;
    _test_info->tests_total       += local_counter.tests_total;
    _test_info->tests_passed      += local_counter.tests_passed;

    // print comprehensive results
    printf("\n========================================\n");
    printf("COMPLETE Array Test Results\n");
    printf("========================================\n");
    printf("Tests: %zu/%zu passed (%.2f%%)\n",
        local_counter.tests_passed, local_counter.tests_total,
        local_counter.tests_total > 0 ?
            (double)local_counter.tests_passed / local_counter.tests_total * 100.0 :
            0.0);
    printf("Assertions: %zu/%zu passed (%.2f%%)\n",
        local_counter.assertions_passed, local_counter.assertions_total,
        local_counter.assertions_total > 0 ?
            (double)local_counter.assertions_passed / local_counter.assertions_total * 100.0 :
            0.0);
    printf("Overall Status: %s\n", overall_result ? "PASSED" : "FAILED");
    printf("========================================\n");

    return overall_result;
}

/*
d_tests_sa_array_run_all
  Main entry point for the test module runner.
*/
bool
d_tests_sa_array_run_all
(
    struct d_test_counter* _test_info
)
{
    return d_tests_sa_array_all_extended(_test_info);
}
