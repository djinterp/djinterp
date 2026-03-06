#include "./test_common_tests_sa.h"


/*
d_tests_sa_test_common_run_all
  Module-level aggregation function that runs all test_common tests
(counter-based). Executes tests for all categories:
  - Macro definitions (keywords, pass/fail, symbols)
  - Type definitions (d_test_id, fn_test, fn_stage)
  - Argument structures (d_test_arg, d_test_arg_list)
  - Test function wrappers (d_test_fn)
  - Lifecycle stages (DTestStage)
  - Type discriminators (DTestTypeFlag)

Parameter(s):
  _counter: test counter to update with results
Return:
  true if all tests passed, false otherwise.
*/
bool
d_tests_sa_test_common_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all counter-based test categories
    result = d_tests_sa_test_common_macro_all(_counter) && result;
    result = d_tests_sa_test_common_type_all(_counter) && result;
    result = d_tests_sa_test_common_arg_all(_counter) && result;
    result = d_tests_sa_test_common_fn_wrapper_all(_counter) && result;
    result = d_tests_sa_test_common_lifecycle_all(_counter) && result;
    result = d_tests_sa_test_common_discriminator_all(_counter) && result;

    return result;
}

/*
d_tests_sa_test_common_run_all_tree
  Module-level aggregation function that runs all tree-based
test_common tests. Builds a d_test_object tree containing:
  - Symbols and constants
  - Function pointer types (fn_test, fn_stage)
  - d_test_fn structure

Parameter(s):
  none.
Return:
  Pointer to a d_test_object tree containing all tree-based
test results, or NULL on allocation failure.
*/
struct d_test_object*
d_tests_sa_test_common_run_all_tree
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("test_common (tree)", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_tc_symbols_all();
    group->elements[idx++] = d_tests_sa_tc_fn_all();
    group->elements[idx++] = d_tests_sa_tc_test_fn_all();

    return group;
}
