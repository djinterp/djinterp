#include "./test_standalone_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS FOR RUNNER TESTS
 *****************************************************************************/

/*
test_helper_simple_module
  A simple test module function that returns a passing test tree.
*/
static struct d_test_object*
test_helper_simple_module
(
    void
)
{
    struct d_test_object* group;

    group = d_test_object_new_interior("Helper Module", 2);

    if (!group)
    {
        return NULL;
    }

    group->elements[0] = d_test_object_new_leaf("test1",
                                                 "passes", true);
    group->elements[1] = d_test_object_new_leaf("test2",
                                                 "passes", true);

    return group;
}

/*
test_helper_failing_module
  A test module function that returns a tree with failures.
*/
static struct d_test_object*
test_helper_failing_module
(
    void
)
{
    struct d_test_object* group;

    group = d_test_object_new_interior("Failing Module", 2);

    if (!group)
    {
        return NULL;
    }

    group->elements[0] = d_test_object_new_leaf("test1",
                                                 "passes", true);
    group->elements[1] = d_test_object_new_leaf("test2",
                                                 "fails", false);

    return group;
}

/*
test_helper_counter_module
  A counter-based test module function.
*/
static bool
test_helper_counter_module
(
    struct d_test_counter* _counter
)
{
    if (!_counter)
    {
        return false;
    }

    _counter->assertions_total  = 5;
    _counter->assertions_passed = 5;
    _counter->tests_total       = 2;
    _counter->tests_passed      = 2;

    return true;
}

/*
test_helper_counter_module_failing
  A counter-based test module function with failures.
*/
static bool
test_helper_counter_module_failing
(
    struct d_test_counter* _counter
)
{
    if (!_counter)
    {
        return false;
    }

    _counter->assertions_total  = 5;
    _counter->assertions_passed = 3;
    _counter->tests_total       = 2;
    _counter->tests_passed      = 1;

    return false;
}

/******************************************************************************
 * RUNNER INIT TESTS
 *****************************************************************************/

/*
d_tests_sa_tsa_runner_init
  Tests d_test_sa_runner_init function.
  Tests the following:
  - initializes suite_name correctly
  - initializes suite_description correctly
  - sets module_count to 0
  - sets wait_for_input to true (default)
  - sets show_notes to true (default)
  - initializes options to defaults
  - initializes failure list
  - handles NULL runner safely
*/
struct d_test_object*
d_tests_sa_tsa_runner_init
(
    void
)
{
    struct d_test_object*    group;
    struct d_test_sa_runner  runner;
    bool                     test_name;
    bool                     test_description;
    bool                     test_module_count;
    bool                     test_wait_default;
    bool                     test_notes_default;
    bool                     test_options_init;
    bool                     test_failures_init;
    bool                     test_null_safe;
    size_t                   idx;

    // test initialization
    d_test_sa_runner_init(&runner, "Test Suite", "Test Description");

    test_name         = (runner.suite_name != NULL) &&
                        (strcmp(runner.suite_name,
                                "Test Suite") == 0);
    test_description  = (runner.suite_description != NULL) &&
                        (strcmp(runner.suite_description,
                                "Test Description") == 0);
    test_module_count = (runner.module_count == 0);
    test_wait_default = (runner.wait_for_input == true);
    test_notes_default = (runner.show_notes == true);

    // verify options were initialized
    test_options_init = (runner.options.number_assertions == false) &&
                        (runner.options.show_info == true)          &&
                        (runner.options.show_module_footer == true) &&
                        (runner.options.list_failures == false)     &&
                        (runner.options.output_file == NULL);

    // verify failure list was initialized
    test_failures_init = (runner.failures.count == 0);

    // test NULL safety
    d_test_sa_runner_init(NULL, "Suite", "Desc");
    test_null_safe = true;

    // build result tree
    group = d_test_object_new_interior("d_test_sa_runner_init", 8);

    if (!group)
    {
        d_test_sa_failure_list_free(&runner.failures);

        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
        "name",
        test_name,
        "initializes suite_name correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
        "description",
        test_description,
        "initializes suite_description correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
        "module_count",
        test_module_count,
        "sets module_count to 0");
    group->elements[idx++] = D_ASSERT_TRUE(
        "wait_default",
        test_wait_default,
        "sets wait_for_input to true");
    group->elements[idx++] = D_ASSERT_TRUE(
        "notes_default",
        test_notes_default,
        "sets show_notes to true");
    group->elements[idx++] = D_ASSERT_TRUE(
        "options_init",
        test_options_init,
        "initializes options to defaults");
    group->elements[idx++] = D_ASSERT_TRUE(
        "failures_init",
        test_failures_init,
        "initializes failure list");
    group->elements[idx++] = D_ASSERT_TRUE(
        "null_safe",
        test_null_safe,
        "handles NULL runner safely");

    // cleanup failure list allocated by runner_init
    d_test_sa_failure_list_free(&runner.failures);

    return group;
}

/******************************************************************************
 * RUNNER SET OPTIONS TESTS
 *****************************************************************************/

/*
d_tests_sa_tsa_runner_set_options
  Tests d_test_sa_runner_set_options function.
  Tests the following:
  - copies options to runner
  - handles NULL runner safely
  - handles NULL options safely
*/
struct d_test_object*
d_tests_sa_tsa_runner_set_options
(
    void
)
{
    struct d_test_object*      group;
    struct d_test_sa_runner    runner;
    struct d_test_sa_options   opts;
    bool                       test_copies_options;
    bool                       test_null_runner;
    bool                       test_null_options;
    size_t                     idx;

    // initialize runner and set custom options
    d_test_sa_runner_init(&runner, "Suite", "Desc");

    d_test_sa_options_init(&opts);
    opts.number_assertions = true;
    opts.list_failures     = true;
    opts.show_info         = false;

    d_test_sa_runner_set_options(&runner, &opts);

    test_copies_options = (runner.options.number_assertions == true) &&
                          (runner.options.list_failures == true)     &&
                          (runner.options.show_info == false);

    // test NULL safety
    d_test_sa_runner_set_options(NULL, &opts);
    test_null_runner = true;

    d_test_sa_runner_set_options(&runner, NULL);
    test_null_options = true;

    // build result tree
    group = d_test_object_new_interior(
                "d_test_sa_runner_set_options", 3);

    if (!group)
    {
        d_test_sa_failure_list_free(&runner.failures);

        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
        "copies_options",
        test_copies_options,
        "copies all option fields to runner");
    group->elements[idx++] = D_ASSERT_TRUE(
        "null_runner",
        test_null_runner,
        "handles NULL runner safely");
    group->elements[idx++] = D_ASSERT_TRUE(
        "null_options",
        test_null_options,
        "handles NULL options safely");

    // cleanup failure list allocated by runner_init
    d_test_sa_failure_list_free(&runner.failures);

    return group;
}

/******************************************************************************
 * RUNNER ADD MODULE TESTS
 *****************************************************************************/

/*
d_tests_sa_tsa_runner_add_module
  Tests d_test_sa_runner_add_module function.
  Tests the following:
  - adds module with tree-based function
  - increments module_count
  - stores module name
  - stores module description
  - stores run function
  - handles NULL runner safely
  - handles NULL function safely
*/
struct d_test_object*
d_tests_sa_tsa_runner_add_module
(
    void
)
{
    struct d_test_object*    group;
    struct d_test_sa_runner  runner;
    bool                     test_adds_module;
    bool                     test_increments_count;
    bool                     test_stores_name;
    bool                     test_stores_desc;
    bool                     test_stores_fn;
    bool                     test_null_runner;
    bool                     test_null_fn;
    size_t                   idx;

    // initialize runner
    d_test_sa_runner_init(&runner, "Suite", "Desc");

    // test adding module
    d_test_sa_runner_add_module(&runner,
                                "module1",
                                "Module Description",
                                test_helper_simple_module,
                                0,
                                NULL);

    test_adds_module      = (runner.module_count == 1);
    test_increments_count = test_adds_module;
    test_stores_name      = (runner.modules[0].name != NULL) &&
                            (strcmp(runner.modules[0].name,
                                    "module1") == 0);
    test_stores_desc      = (runner.modules[0].description != NULL) &&
                            (strcmp(runner.modules[0].description,
                                    "Module Description") == 0);
    test_stores_fn        = (runner.modules[0].run_fn ==
                             test_helper_simple_module);

    // test NULL runner
    d_test_sa_runner_add_module(NULL,
                                "module",
                                "desc",
                                test_helper_simple_module,
                                0,
                                NULL);
    test_null_runner = true;

    // test NULL function
    d_test_sa_runner_add_module(&runner, "module", "desc",
                                NULL, 0, NULL);
    test_null_fn = (runner.module_count == 1);

    // build result tree
    group = d_test_object_new_interior(
                "d_test_sa_runner_add_module", 7);

    if (!group)
    {
        d_test_sa_failure_list_free(&runner.failures);

        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
        "adds_module",
        test_adds_module,
        "adds module successfully");
    group->elements[idx++] = D_ASSERT_TRUE(
        "increments_count",
        test_increments_count,
        "increments module_count");
    group->elements[idx++] = D_ASSERT_TRUE(
        "stores_name",
        test_stores_name,
        "stores module name");
    group->elements[idx++] = D_ASSERT_TRUE(
        "stores_desc",
        test_stores_desc,
        "stores module description");
    group->elements[idx++] = D_ASSERT_TRUE(
        "stores_fn",
        test_stores_fn,
        "stores run function");
    group->elements[idx++] = D_ASSERT_TRUE(
        "null_runner",
        test_null_runner,
        "handles NULL runner safely");
    group->elements[idx++] = D_ASSERT_TRUE(
        "null_fn",
        test_null_fn,
        "handles NULL function safely");

    // cleanup failure list allocated by runner_init
    d_test_sa_failure_list_free(&runner.failures);

    return group;
}

/******************************************************************************
 * RUNNER ADD MODULE COUNTER TESTS
 *****************************************************************************/

/*
d_tests_sa_tsa_runner_add_module_counter
  Tests d_test_sa_runner_add_module_counter function.
  Tests the following:
  - adds module with counter-based function
  - increments module_count
  - stores counter function correctly
  - run_fn is NULL for counter modules
  - handles NULL runner safely
*/
struct d_test_object*
d_tests_sa_tsa_runner_add_module_counter
(
    void
)
{
    struct d_test_object*    group;
    struct d_test_sa_runner  runner;
    bool                     test_adds_module;
    bool                     test_increments_count;
    bool                     test_stores_counter_fn;
    bool                     test_run_fn_null;
    bool                     test_null_runner;
    size_t                   idx;

    // initialize runner
    d_test_sa_runner_init(&runner, "Suite", "Desc");

    // test adding counter module
    d_test_sa_runner_add_module_counter(
        &runner,
        "counter_module",
        "Counter Module Description",
        test_helper_counter_module,
        0,
        NULL);

    test_adds_module       = (runner.module_count == 1);
    test_increments_count  = test_adds_module;
    test_stores_counter_fn = (runner.modules[0].run_counter ==
                              test_helper_counter_module);
    test_run_fn_null       = (runner.modules[0].run_fn == NULL);

    // test NULL runner
    d_test_sa_runner_add_module_counter(
        NULL,
        "module",
        "desc",
        test_helper_counter_module,
        0,
        NULL);
    test_null_runner = true;

    // build result tree
    group = d_test_object_new_interior(
                "d_test_sa_runner_add_module_counter", 5);

    if (!group)
    {
        d_test_sa_failure_list_free(&runner.failures);

        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
        "adds_module",
        test_adds_module,
        "adds counter module successfully");
    group->elements[idx++] = D_ASSERT_TRUE(
        "increments_count",
        test_increments_count,
        "increments module_count");
    group->elements[idx++] = D_ASSERT_TRUE(
        "stores_counter_fn",
        test_stores_counter_fn,
        "stores counter function correctly");
    group->elements[idx++] = D_ASSERT_TRUE(
        "run_fn_null",
        test_run_fn_null,
        "run_fn is NULL for counter modules");
    group->elements[idx++] = D_ASSERT_TRUE(
        "null_runner",
        test_null_runner,
        "handles NULL runner safely");

    // cleanup failure list allocated by runner_init
    d_test_sa_failure_list_free(&runner.failures);

    return group;
}

/******************************************************************************
 * RUNNER SETTINGS TESTS
 *****************************************************************************/

/*
d_tests_sa_tsa_runner_settings
  Tests d_test_sa_runner_set_* functions.
  Tests the following:
  - set_wait_for_input changes value
  - set_show_notes changes value
  - settings can be toggled
  - handles NULL runner safely
*/
struct d_test_object*
d_tests_sa_tsa_runner_settings
(
    void
)
{
    struct d_test_object*    group;
    struct d_test_sa_runner  runner;
    bool                     test_set_wait_false;
    bool                     test_set_wait_true;
    bool                     test_set_notes_false;
    bool                     test_set_notes_true;
    bool                     test_null_safe;
    size_t                   idx;

    // initialize runner
    d_test_sa_runner_init(&runner, "Suite", "Desc");

    // test set_wait_for_input
    d_test_sa_runner_set_wait_for_input(&runner, false);
    test_set_wait_false = (runner.wait_for_input == false);

    d_test_sa_runner_set_wait_for_input(&runner, true);
    test_set_wait_true = (runner.wait_for_input == true);

    // test set_show_notes
    d_test_sa_runner_set_show_notes(&runner, false);
    test_set_notes_false = (runner.show_notes == false);

    d_test_sa_runner_set_show_notes(&runner, true);
    test_set_notes_true = (runner.show_notes == true);

    // test NULL safety
    d_test_sa_runner_set_wait_for_input(NULL, false);
    d_test_sa_runner_set_show_notes(NULL, false);
    test_null_safe = true;

    // build result tree
    group = d_test_object_new_interior("Runner Settings", 5);

    if (!group)
    {
        d_test_sa_failure_list_free(&runner.failures);

        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
        "set_wait_false",
        test_set_wait_false,
        "set_wait_for_input sets to false");
    group->elements[idx++] = D_ASSERT_TRUE(
        "set_wait_true",
        test_set_wait_true,
        "set_wait_for_input sets to true");
    group->elements[idx++] = D_ASSERT_TRUE(
        "set_notes_false",
        test_set_notes_false,
        "set_show_notes sets to false");
    group->elements[idx++] = D_ASSERT_TRUE(
        "set_notes_true",
        test_set_notes_true,
        "set_show_notes sets to true");
    group->elements[idx++] = D_ASSERT_TRUE(
        "null_safe",
        test_null_safe,
        "handles NULL runner safely");

    // cleanup failure list allocated by runner_init
    d_test_sa_failure_list_free(&runner.failures);

    return group;
}

/******************************************************************************
 * RUNNER CLEANUP TESTS
 *****************************************************************************/

/*
d_tests_sa_tsa_runner_cleanup
  Tests d_test_sa_runner_cleanup function.
  Tests the following:
  - handles NULL runner safely
  - cleanup after init does not crash
  - failure list is freed
  - results.modules is NULL after cleanup
*/
struct d_test_object*
d_tests_sa_tsa_runner_cleanup
(
    void
)
{
    struct d_test_object*    group;
    struct d_test_sa_runner  runner;
    bool                     test_null_safe;
    bool                     test_init_cleanup;
    bool                     test_modules_null;
    size_t                   idx;

    // test NULL safety
    d_test_sa_runner_cleanup(NULL);
    test_null_safe = true;

    // test cleanup after init
    d_test_sa_runner_init(&runner, "Suite", "Desc");
    d_test_sa_runner_cleanup(&runner);
    test_init_cleanup = true;

    // verify results.modules is NULL
    test_modules_null = (runner.results.modules == NULL);

    // build result tree
    group = d_test_object_new_interior(
                "d_test_sa_runner_cleanup", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE(
        "null_safe",
        test_null_safe,
        "handles NULL runner safely");
    group->elements[idx++] = D_ASSERT_TRUE(
        "init_cleanup",
        test_init_cleanup,
        "cleanup after init does not crash");
    group->elements[idx++] = D_ASSERT_TRUE(
        "modules_null",
        test_modules_null,
        "results.modules is NULL after cleanup");

    return group;
}

/******************************************************************************
 * RUNNER MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_tsa_runner_all
  Runs all runner tests.
  Tests the following:
  - d_test_sa_runner_init
  - d_test_sa_runner_set_options
  - d_test_sa_runner_add_module
  - d_test_sa_runner_add_module_counter
  - runner settings
  - d_test_sa_runner_cleanup
*/
struct d_test_object*
d_tests_sa_tsa_runner_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior(
                "Unified Runner Operations", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_tsa_runner_init();
    group->elements[idx++] = d_tests_sa_tsa_runner_set_options();
    group->elements[idx++] = d_tests_sa_tsa_runner_add_module();
    group->elements[idx++] = d_tests_sa_tsa_runner_add_module_counter();
    group->elements[idx++] = d_tests_sa_tsa_runner_settings();
    group->elements[idx++] = d_tests_sa_tsa_runner_cleanup();

    return group;
}
