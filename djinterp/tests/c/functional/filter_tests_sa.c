#include ".\filter_tests_sa.h"


/*
d_tests_sa_filter_run_all
  Module-level aggregation function that runs all filter tests.
  Executes tests for all categories:
  - Filter operation creation
  - Filter chain management
  - Filter combinators
  - Execution and application
  - Utility functions
  - Iterator interface
  - Fluent filter builder
  - Convenience macros
*/
bool
d_tests_sa_filter_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_filter_operations_all(_counter)  && result;
    result = d_tests_sa_filter_chain_all(_counter)       && result;
    result = d_tests_sa_filter_combinators_all(_counter)  && result;
    result = d_tests_sa_filter_execution_all(_counter)   && result;
    result = d_tests_sa_filter_utility_all(_counter)     && result;
    result = d_tests_sa_filter_iterator_all(_counter)    && result;
    result = d_tests_sa_filter_builder_all(_counter)     && result;
    result = d_tests_sa_filter_macros_all(_counter)      && result;

    return result;
}
