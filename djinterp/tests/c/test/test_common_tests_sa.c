#include ".\test_common_tests_sa.h"


/*
d_tests_sa_test_common_run_all
  Module-level aggregation function that runs all test_common tests.
  Executes tests for all categories:
  - Macro definitions (keywords, pass/fail, symbols)
  - Type definitions (d_test_id, fn_test, fn_stage)
  - Argument structures (d_test_arg, d_test_arg_list)
  - Test function wrappers (d_test_fn)
  - Lifecycle stages (DTestStage)
  - Type discriminators (DTestTypeFlag)
*/
bool
d_tests_sa_test_common_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_test_common_macro_all(_counter) && result;
    result = d_tests_sa_test_common_type_all(_counter) && result;
    result = d_tests_sa_test_common_arg_all(_counter) && result;
    result = d_tests_sa_test_common_fn_wrapper_all(_counter) && result;
    result = d_tests_sa_test_common_lifecycle_all(_counter) && result;
    result = d_tests_sa_test_common_discriminator_all(_counter) && result;

    return result;
}
