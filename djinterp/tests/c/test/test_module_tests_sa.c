/******************************************************************************
* djinterp [test]                                       test_module_tests_sa.c
*
*   Aggregation function for all test_module.h standalone unit tests.
*
* path:      \test\test\test_module_tests_sa.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_module_tests_sa.h"


//=============================================================================
// ROOT TEST FUNCTION
//=============================================================================

/*
d_tests_sa_test_module_all
  Runs all test_module unit tests across all sections.
  Tests the following:
  - constructor / destructor tests
  - child management tests
  - configuration tests
  - stage hook tests
  - execution tests
  - result tests
  - output tests
*/
bool
d_tests_sa_test_module_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    printf("\n%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
    printf("Running test_module Unit Test Suite\n");
    printf("%s\n\n", D_TEST_SA_SEPARATOR_DOUBLE);

    all_passed = true;

    //=========================================================================
    // CONSTRUCTOR / DESTRUCTOR TESTS
    //=========================================================================

    printf("  [SECTION] Constructor / Destructor\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_module_new(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_new_with_children(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_new_args(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_validate_args(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_free(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_free_null(_test_info))
    {
        all_passed = false;
    }

    printf("\n");

    //=========================================================================
    // CHILD MANAGEMENT TESTS
    //=========================================================================

    printf("  [SECTION] Child Management\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_module_add_child(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_add_child_null(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_child_count(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_child_count_empty(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_child_at(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_child_at_invalid(_test_info))
    {
        all_passed = false;
    }

    printf("\n");

    //=========================================================================
    // CONFIGURATION TESTS
    //=========================================================================

    printf("  [SECTION] Configuration\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_module_get_effective_settings(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_effective_settings_null_parent(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_name(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_name_null(_test_info))
    {
        all_passed = false;
    }

    printf("\n");

    //=========================================================================
    // STAGE HOOK TESTS
    //=========================================================================

    printf("  [SECTION] Stage Hooks\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_module_set_stage_hook(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_stage_hook(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_stage_hook_unset(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_set_stage_hook_null(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_stage_hooks_all_stages(_test_info))
    {
        all_passed = false;
    }

    printf("\n");

    //=========================================================================
    // EXECUTION TESTS
    //=========================================================================

    printf("  [SECTION] Execution\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_module_run(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_run_empty(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_run_with_failing_child(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_run_child(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_run_child_invalid_index(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_run_null(_test_info))
    {
        all_passed = false;
    }

    printf("\n");

    //=========================================================================
    // RESULT TESTS
    //=========================================================================

    printf("  [SECTION] Results\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_module_get_result(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_result_null(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_reset_result(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_pass_rate(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_get_pass_rate_empty(_test_info))
    {
        all_passed = false;
    }

    printf("\n");

    //=========================================================================
    // OUTPUT TESTS
    //=========================================================================

    printf("  [SECTION] Output\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_module_print(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_print_null(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_print_result(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_status_to_string(_test_info))
    {
        all_passed = false;
    }

    if (!d_tests_sa_test_module_status_to_string_all(_test_info))
    {
        all_passed = false;
    }

    //=========================================================================
    // SUMMARY
    //=========================================================================

    printf("\n%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    if (all_passed)
    {
        printf("%s test_module Unit Test Suite: PASSED\n",
               D_TEST_SYMBOL_SUCCESS);
    }
    else
    {
        printf("%s test_module Unit Test Suite: FAILED\n",
               D_TEST_SYMBOL_FAIL);
    }

    printf("  assertions: %zu / %zu passed\n",
           _test_info->assertions_passed,
           _test_info->assertions_total);
    printf("  tests:      %zu / %zu passed\n",
           _test_info->tests_passed,
           _test_info->tests_total);

    printf("%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    return all_passed;
}
