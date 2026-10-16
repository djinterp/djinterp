/*******************************************************************************
* djinterp [functional]                                     predicate_tests_sa.c
*
*
* path:      /tests/djinterp/c/functional/predicate_tests_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./predicate_tests_sa.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


/*
d_tests_sa_predicate_run_all
  Module-level aggregation function that runs all predicate tests.
  Executes tests for all categories:
  - Constructor functions (_new variants)
  - Evaluation functions (_eval variants)
  - Utility predicates (is_null, is_not_null)
  - Compound literal macros (D_PREDICATE_* variants)
*/
bool
d_tests_sa_predicate_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_predicate_constructor_all(_counter) && result;
    result = d_tests_sa_predicate_eval_all(_counter)        && result;
    result = d_tests_sa_predicate_macro_all(_counter)       && result;

    return result;
}
