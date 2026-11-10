/*******************************************************************************
* djinterp [functional]                                          compose_tests.c
*
*
* path:      /tests/djinterp/c/functional/compose_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./compose_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_compose_run_all
  Module-level aggregation function that runs all compose tests.
  Executes tests for all categories:
  - Transformer composition functions
  - Partial application functions
  - Convenience and template macros
*/
bool d_tests_compose_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_compose_transformer_all(_suite) && result;
    result = d_tests_compose_partial_all(_suite)     && result;
    result = d_tests_compose_macro_all(_suite)       && result;

    return result;
}
