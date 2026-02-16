#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS FOR RUNNER TESTS
 *****************************************************************************/

/*
helper_runner_module_tree
  Helper function that returns a test object tree for runner testing.
*/
static struct d_test_object*
helper_runner_module_tree
(
    void
)
{
    return d_test_object_new_leaf("runner_test", "runner message", true);
}


/*
helper_runner_module_counter
  Helper function that updates a counter for runner testing.
*/
static bool
helper_runner_module_counter
(
    struct d_test_counter* _counter
)
{
    if (_counter)
    {
        _counter->assertions_total++;
        _counter->assertions_passed++;
    }

    return true;
}


/******************************************************************************
 * XI. RUNNER FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_runner_init
  Tests the d_test_sa_runner_init function.
  Tests the following:
  - NULL runner is handled safely
  - Suite name is stored
  - Suite description is stored
  - module_count is initialized to 0
  - Default settings are applied
*/
bool
d_tests_sa_standalone_runner_init
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_test_sa_runner runner;

    result = true;

    // test 1: NULL runner is handled safely
    d_test_sa_runner_init(NULL, "Test", "Description");

    result = d_assert_standalone(
        true,
        "runner_init_null_safe",
        "d_test_sa_runner_init(NULL) should not crash",
        _counter) && result;

    // test 2: suite name is stored
    d_test_sa_runner_init(&runner, "Test Suite", "Suite Description");

    result = d_assert_standalone(
        runner.suite_name != NULL,
        "runner_init_name_stored",
        "suite_name should be stored",
        _counter) && result;

    result = d_assert_standalone(
        d_strcasecmp(runner.suite_name, "Test Suite") == 0,
        "runner_init_name_value",
        "suite_name should match input",
        _counter) && result;

    // test 3: suite description is stored
    result = d_assert_standalone(
        runner.suite_description != NULL,
        "runner_init_description_stored",
        "suite_description should be stored",
        _counter) && result;

    result = d_assert_standalone(
        d_strcasecmp(runner.suite_description, "Suite Description") == 0,
        "runner_init_description_value",
        "suite_description should match input",
        _counter) && result;

    // test 4: module_count is initialized to 0
    result = d_assert_standalone(
        runner.module_count == 0,
        "runner_init_module_count_zero",
        "module_count should be initialized to 0",
        _counter) && result;

    // test 5: wait_for_input defaults to true
    result = d_assert_standalone(
        runner.wait_for_input == true,
        "runner_init_wait_default",
        "wait_for_input should default to true",
        _counter) && result;

    // test 6: show_notes defaults to true
    result = d_assert_standalone(
        runner.show_notes == true,
        "runner_init_notes_default",
        "show_notes should default to true",
        _counter) && result;

    // test 7: results are initialized
    result = d_assert_standalone(
        runner.results.modules_total == 0,
        "runner_init_results_modules",
        "results.modules_total should be 0",
        _counter) && result;

    result = d_assert_standalone(
        runner.results.totals.assertions_total == 0,
        "runner_init_results_assertions",
        "results.totals should be reset",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_runner_add_module
  Tests the d_test_sa_runner_add_module function.
  Tests the following:
  - NULL runner is handled safely
  - NULL run_fn is handled safely
  - Module is added correctly
  - module_count is incremented
*/
bool
d_tests_sa_standalone_runner_add_module
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_test_sa_runner runner;

    result = true;

    // test 1: NULL runner is handled safely
    d_test_sa_runner_add_module(NULL, "test", "desc", helper_runner_module_tree, 0, NULL);

    result = d_assert_standalone(
        true,
        "runner_add_module_null_runner_safe",
        "add_module with NULL runner should not crash",
        _counter) && result;

    // test 2: NULL run_fn is handled safely
    d_test_sa_runner_init(&runner, "Test", "Desc");
    d_test_sa_runner_add_module(&runner, "test", "desc", NULL, 0, NULL);

    result = d_assert_standalone(
        runner.module_count == 0,
        "runner_add_module_null_fn_safe",
        "add_module with NULL run_fn should not add module",
        _counter) && result;

    // test 3: module is added correctly
    d_test_sa_runner_init(&runner, "Test", "Desc");
    d_test_sa_runner_add_module(&runner, "module1", "Module 1",
                                 helper_runner_module_tree, 0, NULL);

    result = d_assert_standalone(
        runner.module_count == 1,
        "runner_add_module_count",
        "module_count should be 1 after adding module",
        _counter) && result;

    // test 4: module name is stored
    result = d_assert_standalone(
        d_strcasecmp(runner.modules[0].name, "module1") == 0,
        "runner_add_module_name",
        "Module name should be stored",
        _counter) && result;

    // test 5: module description is stored
    result = d_assert_standalone(
        d_strcasecmp(runner.modules[0].description, "Module 1") == 0,
        "runner_add_module_description",
        "Module description should be stored",
        _counter) && result;

    // test 6: run_fn is stored
    result = d_assert_standalone(
        runner.modules[0].run_fn == helper_runner_module_tree,
        "runner_add_module_run_fn",
        "run_fn should be stored",
        _counter) && result;

    // test 7: run_counter is NULL for tree-based module
    result = d_assert_standalone(
        runner.modules[0].run_counter == NULL,
        "runner_add_module_run_counter_null",
        "run_counter should be NULL for tree-based module",
        _counter) && result;

    // test 8: multiple modules can be added
    d_test_sa_runner_add_module(&runner, "module2", "Module 2",
                                 helper_runner_module_tree, 0, NULL);

    result = d_assert_standalone(
        runner.module_count == 2,
        "runner_add_module_multiple",
        "Should be able to add multiple modules",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_runner_add_module_counter
  Tests the d_test_sa_runner_add_module_counter function.
  Tests the following:
  - NULL runner is handled safely
  - NULL run_fn is handled safely
  - Module is added correctly with counter-based function
*/
bool
d_tests_sa_standalone_runner_add_module_counter
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_test_sa_runner runner;

    result = true;

    // test 1: NULL runner is handled safely
    d_test_sa_runner_add_module_counter(NULL, "test", "desc",
                                         helper_runner_module_counter, 0, NULL);

    result = d_assert_standalone(
        true,
        "runner_add_counter_null_runner_safe",
        "add_module_counter with NULL runner should not crash",
        _counter) && result;

    // test 2: NULL run_fn is handled safely
    d_test_sa_runner_init(&runner, "Test", "Desc");
    d_test_sa_runner_add_module_counter(&runner, "test", "desc", NULL, 0, NULL);

    result = d_assert_standalone(
        runner.module_count == 0,
        "runner_add_counter_null_fn_safe",
        "add_module_counter with NULL run_fn should not add module",
        _counter) && result;

    // test 3: module is added correctly
    d_test_sa_runner_init(&runner, "Test", "Desc");
    d_test_sa_runner_add_module_counter(&runner, "counter_module", "Counter Module",
                                         helper_runner_module_counter, 0, NULL);

    result = d_assert_standalone(
        runner.module_count == 1,
        "runner_add_counter_count",
        "module_count should be 1 after adding counter module",
        _counter) && result;

    // test 4: run_fn is NULL for counter-based module
    result = d_assert_standalone(
        runner.modules[0].run_fn == NULL,
        "runner_add_counter_run_fn_null",
        "run_fn should be NULL for counter-based module",
        _counter) && result;

    // test 5: run_counter is stored
    result = d_assert_standalone(
        runner.modules[0].run_counter == helper_runner_module_counter,
        "runner_add_counter_run_counter",
        "run_counter should be stored",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_runner_set_wait
  Tests the d_test_sa_runner_set_wait_for_input function.
  Tests the following:
  - NULL runner is handled safely
  - wait_for_input is set to true
  - wait_for_input is set to false
*/
bool
d_tests_sa_standalone_runner_set_wait
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_test_sa_runner runner;

    result = true;

    // test 1: NULL runner is handled safely
    d_test_sa_runner_set_wait_for_input(NULL, true);

    result = d_assert_standalone(
        true,
        "runner_set_wait_null_safe",
        "set_wait_for_input with NULL runner should not crash",
        _counter) && result;

    // test 2: wait_for_input can be set to false
    d_test_sa_runner_init(&runner, "Test", "Desc");
    d_test_sa_runner_set_wait_for_input(&runner, false);

    result = d_assert_standalone(
        runner.wait_for_input == false,
        "runner_set_wait_false",
        "wait_for_input should be set to false",
        _counter) && result;

    // test 3: wait_for_input can be set to true
    d_test_sa_runner_set_wait_for_input(&runner, true);

    result = d_assert_standalone(
        runner.wait_for_input == true,
        "runner_set_wait_true",
        "wait_for_input should be set to true",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_runner_set_notes
  Tests the d_test_sa_runner_set_show_notes function.
  Tests the following:
  - NULL runner is handled safely
  - show_notes is set to true
  - show_notes is set to false
*/
bool
d_tests_sa_standalone_runner_set_notes
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_test_sa_runner runner;

    result = true;

    // test 1: NULL runner is handled safely
    d_test_sa_runner_set_show_notes(NULL, true);

    result = d_assert_standalone(
        true,
        "runner_set_notes_null_safe",
        "set_show_notes with NULL runner should not crash",
        _counter) && result;

    // test 2: show_notes can be set to false
    d_test_sa_runner_init(&runner, "Test", "Desc");
    d_test_sa_runner_set_show_notes(&runner, false);

    result = d_assert_standalone(
        runner.show_notes == false,
        "runner_set_notes_false",
        "show_notes should be set to false",
        _counter) && result;

    // test 3: show_notes can be set to true
    d_test_sa_runner_set_show_notes(&runner, true);

    result = d_assert_standalone(
        runner.show_notes == true,
        "runner_set_notes_true",
        "show_notes should be set to true",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_runner_cleanup
  Tests the d_test_sa_runner_cleanup function.
  Tests the following:
  - NULL runner is handled safely
  - Cleanup with no allocated resources is safe
*/
bool
d_tests_sa_standalone_runner_cleanup
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_test_sa_runner runner;

    result = true;

    // test 1: NULL runner is handled safely
    d_test_sa_runner_cleanup(NULL);

    result = d_assert_standalone(
        true,
        "runner_cleanup_null_safe",
        "d_test_sa_runner_cleanup(NULL) should not crash",
        _counter) && result;

    // test 2: cleanup with no allocated resources is safe
    d_test_sa_runner_init(&runner, "Test", "Desc");
    d_test_sa_runner_cleanup(&runner);

    result = d_assert_standalone(
        true,
        "runner_cleanup_no_alloc_safe",
        "Cleanup with no allocations should not crash",
        _counter) && result;

    // test 3: modules pointer is NULL after cleanup
    result = d_assert_standalone(
        runner.results.modules == NULL,
        "runner_cleanup_modules_null",
        "results.modules should be NULL after cleanup",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_runner_fn_all
  Aggregation function that runs all runner function tests.
*/
bool
d_tests_sa_standalone_runner_fn_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Runner Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_standalone_runner_init(_counter) && result;
    result = d_tests_sa_standalone_runner_add_module(_counter) && result;
    result = d_tests_sa_standalone_runner_add_module_counter(_counter) && result;
    result = d_tests_sa_standalone_runner_set_wait(_counter) && result;
    result = d_tests_sa_standalone_runner_set_notes(_counter) && result;
    result = d_tests_sa_standalone_runner_cleanup(_counter) && result;

    return result;
}
