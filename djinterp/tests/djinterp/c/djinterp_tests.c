/*******************************************************************************
* djinterp [c]                                                  djinterp_tests.c
*
* Master test suite implementation - aggregates all test modules.
*
* Test modules (each in separate .c file):
*   - djinterp_tests_sa_index.c          (dj_index functions)
*   - djinterp_tests_sa_index_macros.c   (index manipulation macros)
*   - djinterp_tests_sa_array.c          (array utility macros)
*   - djinterp_tests_sa_boolean.c        (boolean constants/macros)
*   - djinterp_tests_sa_function_ptr.c   (function pointer types)
*   - djinterp_tests_sa_edge.c           (edge cases and boundaries)
*
*
* path:      /tests/djinterp/c/djinterp_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.01.15
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "djinterp_tests.h"


/******************************************************************************
 * MODULE-LEVEL TEST RUNNERS
 *****************************************************************************/


bool d_tests_djinterp_run_index_macros(
    struct dj_test_suite* _suite
)
{
    // Calls master runner from djinterp_index_macros_tests_sa.c
    return d_tests_djinterp_index_macros_all(_suite);
}


/******************************************************************************
 * CATEGORY-LEVEL TEST RUNNERS
 *****************************************************************************/


bool d_tests_djinterp_run_all_macros(
    struct dj_test_suite* _suite
)
{
    d_tests_djinterp_run_index_macros(_suite);

    return true;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

bool d_tests_djinterp_run_all(
    struct dj_test_suite* _suite
)
{
    d_tests_djinterp_run_index_macros(_suite);

    return true;
}


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/


