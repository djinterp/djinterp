#include ".\test_standalone_tests_sa.h"


/*
d_tests_sa_standalone_run_all
  Module-level aggregation function that runs all test_standalone tests.
  Executes tests for all categories:
  - Assertion macros (TRUE, FALSE, NULL, NOT_NULL, EQUAL, STR_EQUAL)
  - Constant macros (LEAF, INTERIOR, LINE_WIDTH, separators, MAX_MODULES)
  - Test counter operations (struct, reset, add)
  - Test object operations (struct, new_leaf, new_interior, add_child, free)
  - Results structures (module_results, suite_results)
  - Note structures (note_item, note_section)
  - Module entry and runner structures
  - Function pointer types (fn_print_object, fn_print_object_file)
  - Assertion function (d_assert_standalone)
  - Template substitution (d_test_substitute_template)
  - Runner functions (init, add_module, set_wait, set_notes, cleanup)
  - Utility functions (get_elapsed_time)
*/
bool
d_tests_sa_standalone_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_standalone_assertion_macro_all(_counter) && result;
    result = d_tests_sa_standalone_constant_all(_counter) && result;
    result = d_tests_sa_standalone_counter_all(_counter) && result;
    result = d_tests_sa_standalone_object_all(_counter) && result;
    result = d_tests_sa_standalone_results_all(_counter) && result;
    result = d_tests_sa_standalone_note_all(_counter) && result;
    result = d_tests_sa_standalone_runner_struct_all(_counter) && result;
    result = d_tests_sa_standalone_fn_ptr_all(_counter) && result;
    result = d_tests_sa_standalone_assert_fn_all(_counter) && result;
    result = d_tests_sa_standalone_template_all(_counter) && result;
    result = d_tests_sa_standalone_runner_fn_all(_counter) && result;
    result = d_tests_sa_standalone_utility_all(_counter) && result;

    return result;
}
