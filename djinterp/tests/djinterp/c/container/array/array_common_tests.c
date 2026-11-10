/*******************************************************************************
* djinterp [container]                                      array_common_tests.c
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
* path:      /tests/djinterp/c/container/array/array_common_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.10.25
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./array_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER FUNCTIONS
 *****************************************************************************/

/*
d_test_init_from_args_wrapper
  Helper function for testing d_array_common_init_from_args.
  This wrapper handles the va_list conversion locally, so the test calls
  the function as a caller with a va_list would.

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
bool d_test_init_from_args_wrapper(
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
int d_test_int_comparator(
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
void d_test_free_int_ptr(
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
bool d_test_verify_array_contents(
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
bool d_test_verify_array_reversed(
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
d_tests_array_common_all
  Runs all basic array_common test functions and provides a comprehensive
  summary of test results.

Parameter(s):
  _suite: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all tests passed, or
  - false, if any test failed.
*/
bool d_tests_array_common_all(
    struct dj_test_suite* _suite
)
{
    d_tests_array_common_run_init_tests(_suite);
    d_tests_array_common_run_util_tests(_suite);
    d_tests_array_common_run_manip_tests(_suite);
    d_tests_array_common_run_free_tests(_suite);

    return true;
}

/*
d_tests_array_common_all_extended
  Runs ALL implemented array_common test functions including all available
  functionality with detailed reporting.

Parameter(s):
  _suite: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all tests passed, or
  - false, if any test failed.
*/
bool d_tests_array_common_all_extended(
    struct dj_test_suite* _suite
)
{
    d_tests_array_common_all(_suite);

    return true;
}

/*
d_tests_array_common_run_all
  Main entry point for the test module runner. Returns pass/fail status for
  use with the unified test runner.

Parameter(s):
  _suite: pointer to test counter for tracking results
Return:
  A boolean value corresponding to either:
  - true, if all tests passed, or
  - false, if any test failed.
*/
bool d_tests_array_common_run_all(
    struct dj_test_suite* _suite
)
{
    return d_tests_array_common_all(_suite);
}
