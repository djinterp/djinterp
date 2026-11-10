/*******************************************************************************
* djinterp [functional]                                        predicate_tests.c
*
*
* path:      /tests/djinterp/c/functional/predicate_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./predicate_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_predicate_run_all
  Module-level aggregation function that runs all predicate tests.
  Executes tests for all categories:
  - Constructor functions (_new variants)
  - Evaluation functions (_eval variants)
  - Utility predicates (is_null, is_not_null)
  - Compound literal macros (D_PREDICATE_* variants)
*/
bool d_tests_predicate_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_predicate_constructor_all(_suite) && result;
    result = d_tests_predicate_eval_all(_suite)        && result;
    result = d_tests_predicate_macro_all(_suite)       && result;

    return result;
}
