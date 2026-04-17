/******************************************************************************
* djinterp [test]                                      djinterp_tests_sa.c
*
* Master test suite implementation - aggregates all test modules.
*
* Test modules (each in separate .c file):
*   - djinterp_tests_sa_index.c          (d_index functions)
*   - djinterp_tests_sa_index_macros.c   (index manipulation macros)
*   - djinterp_tests_sa_array.c          (array utility macros)
*   - djinterp_tests_sa_boolean.c        (boolean constants/macros)
*   - djinterp_tests_sa_function_ptr.c   (function pointer types)
*   - djinterp_tests_sa_edge.c           (edge cases and boundaries)
*
* path:      \test\djinterp_tests_sa.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.15
******************************************************************************/

#include "djinterp_tests_sa.h"


/******************************************************************************
 * MODULE-LEVEL TEST RUNNERS
 *****************************************************************************/

bool
d_tests_sa_run_index_functions
(
    struct d_test_counter* _test_info
)
{
    // Calls master runner from djinterp_index_tests_sa.c
    return d_tests_sa_index_functions_all(_test_info);
}

bool
d_tests_sa_run_index_macros
(
    struct d_test_counter* _test_info
)
{
    // Calls master runner from djinterp_index_macros_tests_sa.c
    return d_tests_sa_index_macros_all(_test_info);
}

bool
d_tests_sa_run_array_macros
(
    struct d_test_counter* _test_info
)
{
    // Calls master runner from djinterp_array_tests_sa.c
    return d_tests_sa_array_macros_all(_test_info);
}

bool
d_tests_sa_run_boolean
(
    struct d_test_counter* _test_info
)
{
    // Calls master runner from djinterp_boolean_tests_sa.c
    return d_tests_sa_boolean_all(_test_info);
}

bool
d_tests_sa_run_function_pointers
(
    struct d_test_counter* _test_info
)
{
    // Calls master runner from djinterp_function_ptr_tests_sa.c
    return d_tests_sa_function_pointers_all(_test_info);
}

bool
d_tests_sa_run_edge_cases
(
    struct d_test_counter* _test_info
)
{
    // Calls master runner from djinterp_edge_tests_sa.c
    return d_tests_sa_edge_cases_all(_test_info);
}


/******************************************************************************
 * CATEGORY-LEVEL TEST RUNNERS
 *****************************************************************************/

bool
d_tests_sa_run_all_functions
(
    struct d_test_counter* _test_info
)
{
    printf("\n");
    printf("================================================================================\n");
    printf("                         FUNCTION TESTS\n");
    printf("================================================================================\n");

    struct d_test_counter category_counter = {0, 0, 0, 0};

    // Run d_index functions only
    d_tests_sa_run_index_functions(&category_counter);

    // Update totals
    _test_info->assertions_total  += category_counter.assertions_total;
    _test_info->assertions_passed += category_counter.assertions_passed;
    _test_info->tests_total       += category_counter.tests_total;
    _test_info->tests_passed      += category_counter.tests_passed;

    bool all_passed = (category_counter.tests_passed == category_counter.tests_total);

    printf("\n=== Function Tests Summary ===\n");
    printf("  Assertions: %zu/%zu passed\n",
           category_counter.assertions_passed, category_counter.assertions_total);
    printf("  Unit Tests: %zu/%zu passed\n",
           category_counter.tests_passed, category_counter.tests_total);
    printf("  Status: %s\n", all_passed ? "[PASS]" : "[FAIL]");
    printf("================================================================================\n\n");

    return all_passed;
}

bool
d_tests_sa_run_all_macros
(
    struct d_test_counter* _test_info
)
{
    printf("\n");
    printf("================================================================================\n");
    printf("                         MACRO TESTS\n");
    printf("================================================================================\n");

    struct d_test_counter category_counter = {0, 0, 0, 0};

    // Run all macro tests
    d_tests_sa_run_index_macros(&category_counter);
    d_tests_sa_run_array_macros(&category_counter);
    d_tests_sa_run_boolean(&category_counter);

    // Update totals
    _test_info->assertions_total  += category_counter.assertions_total;
    _test_info->assertions_passed += category_counter.assertions_passed;
    _test_info->tests_total       += category_counter.tests_total;
    _test_info->tests_passed      += category_counter.tests_passed;

    bool all_passed = (category_counter.tests_passed == category_counter.tests_total);

    printf("\n=== Macro Tests Summary ===\n");
    printf("  Assertions: %zu/%zu passed\n",
           category_counter.assertions_passed, category_counter.assertions_total);
    printf("  Unit Tests: %zu/%zu passed\n",
           category_counter.tests_passed, category_counter.tests_total);
    printf("  Status: %s\n", all_passed ? "[PASS]" : "[FAIL]");
    printf("================================================================================\n\n");

    return all_passed;
}

bool
d_tests_sa_run_all_types
(
    struct d_test_counter* _test_info
)
{
    printf("\n");
    printf("================================================================================\n");
    printf("                         TYPE TESTS\n");
    printf("================================================================================\n");

    struct d_test_counter category_counter = {0, 0, 0, 0};

    // Run type-related tests
    d_tests_sa_run_function_pointers(&category_counter);

    // Update totals
    _test_info->assertions_total  += category_counter.assertions_total;
    _test_info->assertions_passed += category_counter.assertions_passed;
    _test_info->tests_total       += category_counter.tests_total;
    _test_info->tests_passed      += category_counter.tests_passed;

    bool all_passed = (category_counter.tests_passed == category_counter.tests_total);

    printf("\n=== Type Tests Summary ===\n");
    printf("  Assertions: %zu/%zu passed\n",
           category_counter.assertions_passed, category_counter.assertions_total);
    printf("  Unit Tests: %zu/%zu passed\n",
           category_counter.tests_passed, category_counter.tests_total);
    printf("  Status: %s\n", all_passed ? "[PASS]" : "[FAIL]");
    printf("================================================================================\n\n");

    return all_passed;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

bool
d_tests_sa_run_all
(
    struct d_test_counter* _test_info
)
{
    printf("\n");
    printf("================================================================================\n");
    printf("================================================================================\n");
    printf("                         djinterp TEST SUITE\n");
    printf("                      Comprehensive Test Coverage\n");
    printf("================================================================================\n");
    printf("================================================================================\n");

    struct d_test_counter master_counter = {0, 0, 0, 0};

    // Run all test modules
    printf("\n[1/6] Running d_index Function Tests...\n");
    d_tests_sa_run_index_functions(&master_counter);

    printf("\n[2/6] Running Index Manipulation Macro Tests...\n");
    d_tests_sa_run_index_macros(&master_counter);

    printf("\n[3/6] Running Array Utility Macro Tests...\n");
    d_tests_sa_run_array_macros(&master_counter);

    printf("\n[4/6] Running Boolean Constant/Macro Tests...\n");
    d_tests_sa_run_boolean(&master_counter);

    printf("\n[5/6] Running Function Pointer Tests...\n");
    d_tests_sa_run_function_pointers(&master_counter);

    printf("\n[6/6] Running Edge Case Tests...\n");
    d_tests_sa_run_edge_cases(&master_counter);

    // Update totals
    _test_info->assertions_total  += master_counter.assertions_total;
    _test_info->assertions_passed += master_counter.assertions_passed;
    _test_info->tests_total       += master_counter.tests_total;
    _test_info->tests_passed      += master_counter.tests_passed;

    // Print final summary
    d_tests_sa_print_summary(&master_counter);

    return (master_counter.tests_passed == master_counter.tests_total);
}


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

void
d_tests_sa_print_module_results
(
    const char*                  _module_name,
    const struct d_test_counter* _counter,
    bool                         _passed
)
{
    printf("\n--- %s Results ---\n", _module_name);
    printf("  Assertions: %zu/%zu passed (%.1f%%)\n",
           _counter->assertions_passed, _counter->assertions_total,
           (_counter->assertions_total > 0) ?
           (100.0 * _counter->assertions_passed / _counter->assertions_total) : 0.0);
    printf("  Unit Tests: %zu/%zu passed (%.1f%%)\n",
           _counter->tests_passed, _counter->tests_total,
           (_counter->tests_total > 0) ?
           (100.0 * _counter->tests_passed / _counter->tests_total) : 0.0);
    printf("  Status: %s\n", _passed ? "[PASS]" : "[FAIL]");
}

void
d_tests_sa_print_summary
(
    const struct d_test_counter* _counter
)
{
    printf("\n");
    printf("================================================================================\n");
    printf("                            FINAL TEST SUMMARY\n");
    printf("================================================================================\n");
    printf("\n");

    // Assertions summary
    printf("ASSERTIONS:\n");
    printf("  Total:  %zu\n", _counter->assertions_total);
    printf("  Passed: %zu\n", _counter->assertions_passed);
    printf("  Failed: %zu\n", _counter->assertions_total - _counter->assertions_passed);
    if (_counter->assertions_total > 0)
    {
        printf("  Rate:   %.2f%%\n",
               100.0 * _counter->assertions_passed / _counter->assertions_total);
    }
    printf("\n");

    // Unit tests summary
    printf("UNIT TESTS:\n");
    printf("  Total:  %zu\n", _counter->tests_total);
    printf("  Passed: %zu\n", _counter->tests_passed);
    printf("  Failed: %zu\n", _counter->tests_total - _counter->tests_passed);
    if (_counter->tests_total > 0)
    {
        printf("  Rate:   %.2f%%\n",
               100.0 * _counter->tests_passed / _counter->tests_total);
    }
    printf("\n");

    // Overall status
    bool all_passed = (_counter->tests_passed == _counter->tests_total);
    printf("OVERALL STATUS: %s\n", all_passed ? "✓ ALL TESTS PASSED" : "✗ SOME TESTS FAILED");

    if (!all_passed)
    {
        printf("\nWARNING: %zu test(s) failed. Review output above for details.\n",
               _counter->tests_total - _counter->tests_passed);
    }

    printf("\n");
    printf("================================================================================\n");
    printf("================================================================================\n");
    printf("\n");
}
