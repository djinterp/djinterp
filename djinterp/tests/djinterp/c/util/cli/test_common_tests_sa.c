#include "./test_common_tests_sa.h"


/*
d_tests_sa_test_common_run_all
  Module-level aggregation function that runs all test_common tests.
*/
bool
d_tests_sa_test_common_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_common_macro_all(_counter)        && result;
    result = d_tests_sa_test_common_type_all(_counter)         && result;
    result = d_tests_sa_test_common_arg_all(_counter)          && result;
    result = d_tests_sa_test_common_fn_wrapper_all(_counter)   && result;
    result = d_tests_sa_test_common_lifecycle_all(_counter)    && result;
    result = d_tests_sa_test_common_discriminator_all(_counter) && result;
    result = d_tests_sa_test_cli_option_all(_counter)          && result;

    return result;
}
