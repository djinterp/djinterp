/*******************************************************************************
* djinterp [functional]                                           filter_tests.c
*
*
* path:      /tests/djinterp/c/functional/filter_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./filter_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_filter_op_release
  An operation's contents (its name and indices) go with
d_filter_operation_free, which leaves the operation itself to the caller.
*/
void d_tests_filter_op_release(
    struct dj_filter_operation* _op
)
{
    d_filter_operation_free(_op);
    free(_op);

    return;
}


/*
d_tests_filter_run_all
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
bool d_tests_filter_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_filter_operations_all(_suite)  && result;
    result = d_tests_filter_chain_all(_suite)       && result;
    result = d_tests_filter_combinators_all(_suite)  && result;
    result = d_tests_filter_execution_all(_suite)   && result;
    result = d_tests_filter_utility_all(_suite)     && result;
    result = d_tests_filter_iterator_all(_suite)    && result;
    result = d_tests_filter_builder_all(_suite)     && result;
    result = d_tests_filter_macros_all(_suite)      && result;

    return result;
}
