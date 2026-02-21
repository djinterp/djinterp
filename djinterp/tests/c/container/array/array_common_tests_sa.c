/******************************************************************************
* djinterp [container]                                  array_common_tests_sa.c
*
*   Comprehensive test runner for `array_common.h` unit tests.
*   Contains helper functions and the main test suite runners.
*
*   This file provides:
*   - Helper functions used across all test files (comparators, free functions)
*   - Wrapper functions for testing variadic functions
*   - Array verification utilities
*   - Main test suite entry points and result aggregation
*
*
* path:      \tests\container\array\array_common_tests_sa.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.25
******************************************************************************/

#include ".\array_common_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

/*
d_test_init_from_args_wrapper
  Helper function for testing d_array_common_init_from_args.
  This wrapper handles the va_list conversion locally without modifying
  the test_standalone framework.

Parameter(s):
  _elements:     pointer to elements pointer to be initialized
  _count:        pointer to count variable to be set
  _element_size: size of each element in bytes
  _arg_count:    number of variadic arguments to process
  ...:           variadic arguments to populate the array
Return:
  A boolean value corresponding to either:
  - true, if initialization was successful, or
  - false, if memory allocation failed.
*/
bool
d_test_init_from_args_wrapper
(
    void**  _elements,
    size_t* _count,
    size_t  _element_size,
    size_t  _arg_count,
    ...
)
{
    va_list args;
    bool    result;

    va_start(args, _arg_count);

    result = d_array_common_init_from_args(_elements,
                                           _count,
                                           _element_size,
                                           _arg_count,
                                           args);
    va_end(args);

    return result;
}

/*
d_test_int_comparator
  Helper comparator function for testing integer comparisons.
  Follows standard comparator semantics for use with sort/search functions.

Parameter(s):
  _a: pointer to first integer value
  _b: pointer to second integer value
Return:
  - negative value if *_a < *_b
  - zero if *_a == *_b
  - positive value if *_a > *_b
*/
int
d_test_int_comparator
(
    const void* _a,
    const void* _b
)
{
    int ia;
    int ib;

    // extract integer values from void pointers
    ia = *(const int*)_a;
    ib = *(const int*)_b;

    // standard three-way comparison
    if (ia < ib)
    {
        return -1;
    }

    if (ia > ib)
    {
        return 1;
    }

    return 0;
}

/*
d_test_free_int_ptr
  Helper free function for testing deep free operations.
  Safely frees a dynamically allocated pointer with NULL check.

Parameter(s):
  _ptr: pointer to free; may be NULL
Return:
  none.
*/
void
d_test_free_int_ptr
(
    void* _ptr
)
{
    if (_ptr)
    {
        free(_ptr);
    }

    return;
}

/*
d_test_verify_array_contents
  Helper function to verify that an integer array matches expected values.
  Performs element-by-element comparison.

Parameter(s):
  _array:    pointer to the array to verify
  _count:    number of elements to check
  _expected: pointer to array of expected values
Return:
  A boolean value corresponding to either:
  - true, if all elements match, or
  - false, if any element differs or parameters are invalid.
*/
bool
d_test_verify_array_contents
(
    const int* _array,
    size_t     _count,
    const int* _expected
)
{
    size_t i;

    // validate parameters
    if ( (!_array)    ||
         (!_expected) ||
         (_count == 0) )
    {
        return false;
    }

    // compare each element
    for (i = 0; i < _count; i++)
    {
        if (_array[i] != _expected[i])
        {
            return false;
        }
    }

    return true;
}

/*
d_test_verify_array_reversed
  Helper function to verify that an array is the reverse of original values.
  Checks that array[i] == original[count - 1 - i] for all elements.

Parameter(s):
  _array:    pointer to the array to verify (should be reversed)
  _count:    number of elements
  _original: pointer to the original array (non-reversed order)
Return:
  A boolean value corresponding to either:
  - true, if array is correctly reversed, or
  - false, if reversal is incorrect or parameters are invalid.
*/
bool
d_test_verify_array_reversed
(
    const int* _array,
    size_t     _count,
    const int* _original
)
{
    size_t i;

    // validate parameters
    if ( (!_array)    ||
         (!_original) ||
         (_count == 0) )
    {
        return false;
    }

    // verify reversed order
    for (i = 0; i < _count; i++)
    {
        if (_array[i] != _original[_count - 1 - i])
        {
            return false;
        }
    }

    return true;
}


/******************************************************************************
 * COMPREHENSIVE TEST SUITE RUNNERS
 *****************************************************************************/

/*
d_tests_sa_array_common_all
  Runs all basic array_common test functions and provides a comprehensive
  summary of test results.

Parameter(s):
  _test_info: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all tests passed, or
  - false, if any test failed.
*/
bool
d_tests_sa_array_common_all
(
    struct d_test_counter* _test_info
)
{
    bool                   overall_result;
    struct d_test_counter  local_counter;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    overall_result = true;

    // initialize local counter
    local_counter.assertions_total  = 0;
    local_counter.assertions_passed = 0;
    local_counter.tests_total       = 0;
    local_counter.tests_passed      = 0;

    printf("\n========================================\n");
    printf("Running Array Common Utility Tests\n");
    printf("========================================\n");

    // run initialization function tests
    overall_result &= d_tests_sa_array_common_run_init_tests(&local_counter);

    // run utility function tests
    overall_result &= d_tests_sa_array_common_run_util_tests(&local_counter);

    // run manipulation function tests
    overall_result &= d_tests_sa_array_common_run_manip_tests(&local_counter);

    // run cleanup/free function tests
    overall_result &= d_tests_sa_array_common_run_free_tests(&local_counter);

    // update main counter with local results
    _test_info->assertions_total  += local_counter.assertions_total;
    _test_info->assertions_passed += local_counter.assertions_passed;
    _test_info->tests_total       += local_counter.tests_total;
    _test_info->tests_passed      += local_counter.tests_passed;

    // print comprehensive results
    printf("\n===========================================\n");
    printf("`array_common` Unit Test Results (Standalone)\n");
    printf("===========================================\n");
    printf("Function Categories Tested:\n");
    printf("%s%s Initialization Functions (12)\n",
           D_INDENT, D_TEST_SYMBOL_PASS);
    printf("%s%s Utility Functions (9)\n",
           D_INDENT, D_TEST_SYMBOL_PASS);
    printf("%s%s Manipulation Functions (14)\n",
           D_INDENT, D_TEST_SYMBOL_PASS);
    printf("%s%s Cleanup Functions (2)\n",
           D_INDENT, D_TEST_SYMBOL_PASS);
    printf("----------------------------------------\n");
    printf("Total Function Tests: 37\n");
    printf("Tests: %zu/%zu passed (%.2f%%)\n",
           local_counter.tests_passed,
           local_counter.tests_total,
           (local_counter.tests_total > 0)
               ? ( (double)local_counter.tests_passed /
                   (double)local_counter.tests_total * 100.0 )
               : 0.0);

    printf("Assertions: %zu/%zu passed (%.2f%%)\n",
           local_counter.assertions_passed,
           local_counter.assertions_total,
           (local_counter.assertions_total > 0)
               ? ( (double)local_counter.assertions_passed /
                   (double)local_counter.assertions_total * 100.0 )
               : 0.0);

    printf("Overall Status: %s\n",
           overall_result ? "PASSED" : "FAILED");
    printf("========================================\n");

    return overall_result;
}

/*
d_tests_sa_array_common_all_extended
  Runs ALL implemented array_common test functions including all available
  functionality with detailed reporting.

Parameter(s):
  _test_info: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all tests passed, or
  - false, if any test failed.
*/
bool
d_tests_sa_array_common_all_extended
(
    struct d_test_counter* _test_info
)
{
    bool                  overall_result;
    struct d_test_counter local_counter;

    // validate parameters
    if (!_test_info)
    {
        return false;
    }

    overall_result = true;

    // initialize local counter
    local_counter.assertions_total  = 0;
    local_counter.assertions_passed = 0;
    local_counter.tests_total       = 0;
    local_counter.tests_passed      = 0;

    printf("\n========================================\n");
    printf("Running COMPLETE `array_common` Test Suite\n");
    printf("========================================\n");

    // run the complete test suite with all implemented functions
    overall_result = d_tests_sa_array_common_all(&local_counter);

    // update main counter
    _test_info->assertions_total  += local_counter.assertions_total;
    _test_info->assertions_passed += local_counter.assertions_passed;
    _test_info->tests_total       += local_counter.tests_total;
    _test_info->tests_passed      += local_counter.tests_passed;

    // print comprehensive extended results
    printf("\n========================================\n");
    printf("COMPLETE `array_common` Test Results\n");
    printf("========================================\n");
    printf("All functions from array_common.h tested:\n");
    printf("%s- 12 initialization functions\n",           D_INDENT);
    printf("%s- 9 utility functions\n",                   D_INDENT);
    printf("%s- 14 manipulation functions\n",             D_INDENT);
    printf("%s- 2 cleanup functions\n",                   D_INDENT);
    printf("Enhanced features tested:\n");
    printf("%s- d_index support with negative indexing\n", D_INDENT);
    printf("%s- Proper validation via d_index_is_valid\n", D_INDENT);
    printf("%s- Safe conversion via d_index_convert_safe\n", D_INDENT);
    printf("%s- Boundary condition handling\n",            D_INDENT);
    printf("%s- NULL parameter handling\n",                D_INDENT);
    printf("========================================\n");
    printf("Tests: %zu/%zu passed (%.2f%%)\n",
           local_counter.tests_passed,
           local_counter.tests_total,
           (local_counter.tests_total > 0)
               ? ( (double)local_counter.tests_passed /
                   (double)local_counter.tests_total * 100.0 )
               : 0.0);

    printf("Assertions: %zu/%zu passed (%.2f%%)\n",
           local_counter.assertions_passed,
           local_counter.assertions_total,
           (local_counter.assertions_total > 0)
               ? ( (double)local_counter.assertions_passed /
                   (double)local_counter.assertions_total * 100.0 )
               : 0.0);

    printf("Overall Status: %s\n",
           overall_result ? "PASSED" : "FAILED");
    printf("========================================\n");

    return overall_result;
}

/*
d_tests_sa_array_common_run_all
  Main entry point for the test module runner. Returns pass/fail status for
  use with the unified test runner.

Parameter(s):
  _test_info: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all tests passed, or
  - false, if any test failed.
*/
D_INLINE bool
d_tests_sa_array_common_run_all
(
    struct d_test_counter* _test_info
)
{
    return d_tests_sa_array_common_all(_test_info);
}
