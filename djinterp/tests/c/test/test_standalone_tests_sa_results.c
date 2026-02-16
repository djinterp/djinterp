#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * V. RESULTS STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_module_results_struct
  Tests the d_test_sa_module_results structure.
  Tests the following:
  - Structure has expected members
  - Members are accessible
  - Can represent module test results
*/
bool
d_tests_sa_standalone_module_results_struct
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_test_sa_module_results mod_results;

    result = true;

    // test 1: name member is accessible
    mod_results.name = "test_module";

    result = d_assert_standalone(
        mod_results.name != NULL,
        "module_results_name_accessible",
        "name member should be accessible",
        _counter) && result;

    // test 2: description member is accessible
    mod_results.description = "Test module description";

    result = d_assert_standalone(
        mod_results.description != NULL,
        "module_results_description_accessible",
        "description member should be accessible",
        _counter) && result;

    // test 3: counter member is accessible
    mod_results.counter.assertions_total  = 100;
    mod_results.counter.assertions_passed = 95;
    mod_results.counter.tests_total       = 20;
    mod_results.counter.tests_passed      = 18;

    result = d_assert_standalone(
        mod_results.counter.assertions_total == 100,
        "module_results_counter_accessible",
        "counter member should be accessible",
        _counter) && result;

    // test 4: passed member is accessible
    mod_results.passed = true;

    result = d_assert_standalone(
        mod_results.passed == true,
        "module_results_passed_accessible",
        "passed member should be accessible",
        _counter) && result;

    // test 5: elapsed_time member is accessible
    mod_results.elapsed_time = 1.234;

    result = d_assert_standalone(
        mod_results.elapsed_time == 1.234,
        "module_results_elapsed_time_accessible",
        "elapsed_time member should be accessible",
        _counter) && result;

    // test 6: elapsed_time can hold fractional seconds
    mod_results.elapsed_time = 0.001;

    result = d_assert_standalone(
        mod_results.elapsed_time < 0.01,
        "module_results_elapsed_time_precision",
        "elapsed_time should support millisecond precision",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_suite_results_struct
  Tests the d_test_sa_suite_results structure.
  Tests the following:
  - Structure has expected members
  - Members are accessible
  - Can represent suite test results
*/
bool
d_tests_sa_standalone_suite_results_struct
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_suite_results suite_results;

    result = true;

    // test 1: modules_total member is accessible
    suite_results.modules_total = 10;

    result = d_assert_standalone(
        suite_results.modules_total == 10,
        "suite_results_modules_total_accessible",
        "modules_total member should be accessible",
        _counter) && result;

    // test 2: modules_passed member is accessible
    suite_results.modules_passed = 8;

    result = d_assert_standalone(
        suite_results.modules_passed == 8,
        "suite_results_modules_passed_accessible",
        "modules_passed member should be accessible",
        _counter) && result;

    // test 3: totals counter member is accessible
    suite_results.totals.assertions_total  = 500;
    suite_results.totals.assertions_passed = 480;
    suite_results.totals.tests_total       = 100;
    suite_results.totals.tests_passed      = 95;

    result = d_assert_standalone(
        suite_results.totals.assertions_total == 500,
        "suite_results_totals_accessible",
        "totals member should be accessible",
        _counter) && result;

    // test 4: modules pointer member is accessible
    suite_results.modules = NULL;

    result = d_assert_standalone(
        suite_results.modules == NULL,
        "suite_results_modules_ptr_accessible",
        "modules pointer member should be accessible",
        _counter) && result;

    // test 5: total_time member is accessible
    suite_results.total_time = 5.678;

    result = d_assert_standalone(
        suite_results.total_time == 5.678,
        "suite_results_total_time_accessible",
        "total_time member should be accessible",
        _counter) && result;

    // test 6: modules_total can be zero
    suite_results.modules_total = 0;

    result = d_assert_standalone(
        suite_results.modules_total == 0,
        "suite_results_modules_zero",
        "modules_total should be able to be zero",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_results_all
  Aggregation function that runs all results structure tests.
*/
bool
d_tests_sa_standalone_results_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Results Structures\n");
    printf("  -----------------------------\n");

    result = d_tests_sa_standalone_module_results_struct(_counter) && result;
    result = d_tests_sa_standalone_suite_results_struct(_counter) && result;

    return result;
}
