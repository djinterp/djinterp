/*******************************************************************************
* djinterp [test]                                             byte_array_tests.c
*
*   Module-level aggregation for byte_array.h unit tests.
*   Runs all test categories for the byte_array module.
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./byte_array_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_byte_array_run_all
    Module-level aggregation function that runs all byte_array tests.
    Executes tests for all categories:
    - Byte extraction macros
    - Byte expansion macros
    - 1D array builders
    - 2D array builders
    - Jagged array builders
*/
bool d_tests_byte_array_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_byte_array_extraction_all(_suite) && result;
    result = d_tests_byte_array_expansion_all(_suite) && result;
    result = d_tests_byte_array_1d_all(_suite) && result;
#if !defined(_MSC_VER)
    result = d_tests_byte_array_2d_all(_suite) && result;
    result = d_tests_byte_array_jagged_all(_suite) && result;
#endif

    return result;
}
