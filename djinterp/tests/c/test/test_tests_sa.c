#include "./test_tests_sa.h"


/*
d_tests_sa_test_run_all
  Module-level aggregation function that runs all test.h tests.
  Executes tests for all categories:
  - Construction (d_test_new, d_test_new_args)
  - Validate args (d_test_validate_args, d_test_fn_validate_args)
  - Test function wrapper (d_test_fn_new)
  - Child management (add_child, add_assertion, add_function, child_count,
    get_child_at)
  - Stage hooks (set/get stage hooks for all lifecycle stages)
  - Test execution (d_test_run with various configurations)
  - Utility (d_test_print, d_test_type_flag_to_string)
  - Destruction (d_test_fn_free, d_test_free)
  - DTestTypeFlag enum validation
  - DTestStage enum validation
*/
bool
d_tests_sa_test_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_test_construction_all(_counter) && result;
    result = d_tests_sa_test_validate_args_all(_counter) && result;
    result = d_tests_sa_test_fn_wrapper_all(_counter) && result;
    result = d_tests_sa_test_child_mgmt_all(_counter) && result;
    result = d_tests_sa_test_stage_hooks_all(_counter) && result;
    result = d_tests_sa_test_execution_all(_counter) && result;
    result = d_tests_sa_test_utility_all(_counter) && result;
    result = d_tests_sa_test_destruction_all(_counter) && result;
    result = d_tests_sa_test_enum_all(_counter) && result;
    result = d_tests_sa_test_stage_enum_all(_counter) && result;

    return result;
}
