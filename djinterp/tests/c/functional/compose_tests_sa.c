#include ".\compose_tests_sa.h"


/*
d_tests_sa_compose_run_all
  Module-level aggregation function that runs all compose tests.
  Executes tests for all categories:
  - Transformer composition functions
  - Partial application functions
  - Convenience and template macros
*/
bool
d_tests_sa_compose_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_compose_transformer_all(_counter) && result;
    result = d_tests_sa_compose_partial_all(_counter)     && result;
    result = d_tests_sa_compose_macro_all(_counter)       && result;

    return result;
}
