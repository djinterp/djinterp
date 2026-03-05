/******************************************************************************
* djinterp [test]                                      test_session_tests_sa.c
*
*   Aggregation function for all test_session.h standalone unit tests.
*
* path:      \test\test\test_session_tests_sa.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_session_tests_sa.h"


bool
d_tests_sa_test_session_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    printf("\n%s\n", D_TEST_SA_SEPARATOR_DOUBLE);
    printf("Running test_session Unit Test Suite\n");
    printf("%s\n\n", D_TEST_SA_SEPARATOR_DOUBLE);

    all_passed = true;

    //=========================================================================
    // CONSTRUCTOR / DESTRUCTOR TESTS
    //=========================================================================

    printf("  [SECTION] Constructor / Destructor\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_session_new(_test_info))                        { all_passed = false; }
    if (!d_tests_sa_test_session_new_with_config(_test_info))            { all_passed = false; }
    if (!d_tests_sa_test_session_new_with_modules(_test_info))           { all_passed = false; }
    if (!d_tests_sa_test_session_new_with_modules_and_config(_test_info)){ all_passed = false; }
    if (!d_tests_sa_test_session_validate_args(_test_info))              { all_passed = false; }
    if (!d_tests_sa_test_session_free(_test_info))                       { all_passed = false; }
    if (!d_tests_sa_test_session_free_null(_test_info))                  { all_passed = false; }

    printf("\n");

    //=========================================================================
    // CONFIGURATION TESTS
    //=========================================================================

    printf("  [SECTION] Configuration\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_session_set_get_option(_test_info))   { all_passed = false; }
    if (!d_tests_sa_test_session_option_null(_test_info))      { all_passed = false; }
    if (!d_tests_sa_test_session_set_output_format(_test_info)){ all_passed = false; }
    if (!d_tests_sa_test_session_set_verbosity(_test_info))    { all_passed = false; }
    if (!d_tests_sa_test_session_enable_color(_test_info))     { all_passed = false; }

    printf("\n");

    //=========================================================================
    // CHILD MANAGEMENT TESTS
    //=========================================================================

    printf("  [SECTION] Child Management\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_session_add_child(_test_info))            { all_passed = false; }
    if (!d_tests_sa_test_session_add_child_non_module(_test_info)) { all_passed = false; }
    if (!d_tests_sa_test_session_add_child_null(_test_info))       { all_passed = false; }
    if (!d_tests_sa_test_session_add_children(_test_info))         { all_passed = false; }
    if (!d_tests_sa_test_session_child_count(_test_info))          { all_passed = false; }
    if (!d_tests_sa_test_session_get_child_at(_test_info))         { all_passed = false; }
    if (!d_tests_sa_test_session_get_child_at_invalid(_test_info)) { all_passed = false; }
    if (!d_tests_sa_test_session_clear_children(_test_info))       { all_passed = false; }

    printf("\n");

    //=========================================================================
    // EXECUTION TESTS
    //=========================================================================

    printf("  [SECTION] Execution\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_session_run_empty(_test_info))         { all_passed = false; }
    if (!d_tests_sa_test_session_run_null(_test_info))          { all_passed = false; }
    if (!d_tests_sa_test_session_run_child_invalid(_test_info)) { all_passed = false; }
    if (!d_tests_sa_test_session_abort(_test_info))             { all_passed = false; }
    if (!d_tests_sa_test_session_reset(_test_info))             { all_passed = false; }
    if (!d_tests_sa_test_session_pause_resume(_test_info))      { all_passed = false; }

    printf("\n");

    //=========================================================================
    // OUTPUT TESTS
    //=========================================================================

    printf("  [SECTION] Output\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_session_write_header(_test_info))  { all_passed = false; }
    if (!d_tests_sa_test_session_write_footer(_test_info))  { all_passed = false; }
    if (!d_tests_sa_test_session_write_summary(_test_info)) { all_passed = false; }
    if (!d_tests_sa_test_session_flush(_test_info))         { all_passed = false; }
    if (!d_tests_sa_test_session_output_null(_test_info))   { all_passed = false; }

    printf("\n");

    //=========================================================================
    // STATISTICS TESTS
    //=========================================================================

    printf("  [SECTION] Statistics\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_session_get_stats(_test_info))      { all_passed = false; }
    if (!d_tests_sa_test_session_all_passed(_test_info))     { all_passed = false; }
    if (!d_tests_sa_test_session_total_counters(_test_info)) { all_passed = false; }
    if (!d_tests_sa_test_session_duration_ms(_test_info))    { all_passed = false; }
    if (!d_tests_sa_test_session_stats_null(_test_info))     { all_passed = false; }

    printf("\n");

    //=========================================================================
    // STATUS TESTS
    //=========================================================================

    printf("  [SECTION] Status\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_session_get_status(_test_info))          { all_passed = false; }
    if (!d_tests_sa_test_session_status_to_string(_test_info))    { all_passed = false; }
    if (!d_tests_sa_test_session_status_to_string_all(_test_info)){ all_passed = false; }
    if (!d_tests_sa_test_session_is_running(_test_info))          { all_passed = false; }
    if (!d_tests_sa_test_session_is_complete(_test_info))         { all_passed = false; }

    printf("\n");

    //=========================================================================
    // UTILITY TESTS
    //=========================================================================

    printf("  [SECTION] Utility\n");
    printf("  %s\n", D_TEST_SA_SEPARATOR_SINGLE);

    if (!d_tests_sa_test_session_print(_test_info))     { all_passed = false; }
    if (!d_tests_sa_test_session_exit_code(_test_info))  { all_passed = false; }

    //=========================================================================
    // SUMMARY
    //=========================================================================

    printf("\n%s\n", D_TEST_SA_SEPARATOR_DOUBLE);

    if (all_passed)
    {
        printf("%s test_session Unit Test Suite: PASSED\n",
               D_TEST_SYMBOL_SUCCESS);
    }
    else
    {
        printf("%s test_session Unit Test Suite: FAILED\n",
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
