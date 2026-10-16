/*******************************************************************************
* djinterp [test]                                          byte_array_tests_sa.c
*
*   Module-level aggregation for byte_array.h unit tests.
*   Runs all test categories for the byte_array module.
*
*
* path:      /tests/djinterp/c/container/array/byte_array_tests_sa.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.04
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./byte_array_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_byte_array_run_all
    Module-level aggregation function that runs all byte_array tests.
    Executes tests for all categories:
    - Byte extraction macros
    - Byte expansion macros
    - 1D array builders
    - 2D array builders
    - Jagged array builders
*/
bool
d_tests_sa_byte_array_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_byte_array_extraction_all(_counter) && result;
    result = d_tests_sa_byte_array_expansion_all(_counter) && result;
    result = d_tests_sa_byte_array_1d_all(_counter) && result;
#if !defined(_MSC_VER)
    result = d_tests_sa_byte_array_2d_all(_counter) && result;
    result = d_tests_sa_byte_array_jagged_all(_counter) && result;
#endif

    return result;
}
