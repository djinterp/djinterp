#include "./test_tests_sa.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

// test function that always passes
static bool
d_internal_exec_pass_fn(void)
{
    return true;
}

// test function that always fails
static bool
d_internal_exec_fail_fn(void)
{
    return false;
}

// counter for tracking hook invocations
static int g_setup_hook_count    = 0;
static int g_teardown_hook_count = 0;

// setup hook that passes and increments counter
static bool
d_internal_exec_setup_pass(struct d_test* _test)
{
    (void)_test;
    g_setup_hook_count++;

    return true;
}

// setup hook that fails
static bool
d_internal_exec_setup_fail(struct d_test* _test)
{
    (void)_test;
    g_setup_hook_count++;

    return false;
}

// teardown hook that increments counter
static bool
d_internal_exec_teardown(struct d_test* _test)
{
    (void)_test;
    g_teardown_hook_count++;

    return true;
}


/*
d_tests_sa_test_run_null
  Tests d_test_run with NULL test.
  Tests the following:
  - d_test_run(NULL, NULL) returns false
*/
bool
d_tests_sa_test_run_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(d_test_run(NULL, NULL) == false,
                                 "d_test_run_null",
                                 "d_test_run(NULL, NULL) returns false",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_run_empty
  Tests d_test_run with an empty test (no children).
  Tests the following:
  - d_test_run with no children returns true (vacuously passes)
*/
bool
d_tests_sa_test_run_empty
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_run_empty",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // empty test should pass (no failures possible)
    result = d_assert_standalone(d_test_run(test, NULL) == true,
                                 "d_test_run_empty",
                                 "empty test run returns true",
                                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_run_pass_fn
  Tests d_test_run with a single passing test function.
  Tests the following:
  - test with one passing function returns true
*/
bool
d_tests_sa_test_run_pass_fn
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_run_pass_fn",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // add passing function
    d_test_add_function(test, (fn_test)d_internal_exec_pass_fn);

    result = d_assert_standalone(d_test_run(test, NULL) == true,
                                 "d_test_run_pass_fn",
                                 "test with passing fn returns true",
                                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_run_fail_fn
  Tests d_test_run with a single failing test function.
  Tests the following:
  - test with one failing function returns false
*/
bool
d_tests_sa_test_run_fail_fn
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_run_fail_fn",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // add failing function
    d_test_add_function(test, (fn_test)d_internal_exec_fail_fn);

    result = d_assert_standalone(d_test_run(test, NULL) == false,
                                 "d_test_run_fail_fn",
                                 "test with failing fn returns false",
                                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_run_mixed
  Tests d_test_run with a mix of passing and failing children.
  Tests the following:
  - test with both pass and fail functions returns false
  - all children are still evaluated (not short-circuited)
*/
bool
d_tests_sa_test_run_mixed
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_run_mixed",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // add mix of passing and failing functions
    d_test_add_function(test, (fn_test)d_internal_exec_pass_fn);
    d_test_add_function(test, (fn_test)d_internal_exec_fail_fn);
    d_test_add_function(test, (fn_test)d_internal_exec_pass_fn);

    // should fail because one child fails
    result = d_assert_standalone(d_test_run(test, NULL) == false,
                                 "d_test_run_mixed",
                                 "mixed pass/fail test returns false",
                                 _counter) && result;

    // verify all children were present
    result = d_assert_standalone(d_test_child_count(test) == 3,
                                 "d_test_run_mixed",
                                 "all 3 children are present",
                                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_run_setup_hook
  Tests d_test_run invokes the setup hook before running children.
  Tests the following:
  - setup hook is invoked during test execution
  - setup hook count increments
*/
bool
d_tests_sa_test_run_setup_hook
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    int            initial_count;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_run_setup_hook",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // set setup hook
    d_test_set_stage_hook(test,
                          D_TEST_STAGE_SETUP,
                          d_internal_exec_setup_pass);

    // add a passing function
    d_test_add_function(test, (fn_test)d_internal_exec_pass_fn);

    // record initial count
    initial_count = g_setup_hook_count;

    // run test
    d_test_run(test, NULL);

    // verify setup hook was called
    result = d_assert_standalone(g_setup_hook_count == initial_count + 1,
                                 "d_test_run_setup_hook",
                                 "setup hook was invoked during test run",
                                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_run_teardown_hook
  Tests d_test_run invokes the teardown hook after running children.
  Tests the following:
  - teardown hook is invoked during test execution
  - teardown hook count increments
*/
bool
d_tests_sa_test_run_teardown_hook
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    int            initial_count;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_run_teardown_hook",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // set teardown hook
    d_test_set_stage_hook(test,
                          D_TEST_STAGE_TEAR_DOWN,
                          d_internal_exec_teardown);

    // add a passing function
    d_test_add_function(test, (fn_test)d_internal_exec_pass_fn);

    // record initial count
    initial_count = g_teardown_hook_count;

    // run test
    d_test_run(test, NULL);

    // verify teardown hook was called
    result = d_assert_standalone(g_teardown_hook_count == initial_count + 1,
                                 "d_test_run_teardown_hook",
                                 "teardown hook was invoked during test run",
                                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_run_setup_fail
  Tests d_test_run when setup hook fails.
  Tests the following:
  - d_test_run returns false when setup hook fails
  - teardown hook is still called after setup failure
*/
bool
d_tests_sa_test_run_setup_fail
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    int            initial_teardown;
    bool           run_result;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_run_setup_fail",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // set failing setup hook and teardown hook
    d_test_set_stage_hook(test,
                          D_TEST_STAGE_SETUP,
                          d_internal_exec_setup_fail);
    d_test_set_stage_hook(test,
                          D_TEST_STAGE_TEAR_DOWN,
                          d_internal_exec_teardown);

    // add a passing function (should never run)
    d_test_add_function(test, (fn_test)d_internal_exec_pass_fn);

    // record initial teardown count
    initial_teardown = g_teardown_hook_count;

    // run test -- should fail due to setup hook failure
    run_result = d_test_run(test, NULL);

    result = d_assert_standalone(run_result == false,
                                 "d_test_run_setup_fail",
                                 "test run returns false when setup fails",
                                 _counter) && result;

    // verify teardown was still called (cleanup guarantee)
    result = d_assert_standalone(
                 g_teardown_hook_count == initial_teardown + 1,
                 "d_test_run_setup_fail",
                 "teardown hook called even after setup failure",
                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_execution_all
  Aggregation function for all test execution tests.
*/
bool
d_tests_sa_test_execution_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_run_null(_counter) && result;
    result = d_tests_sa_test_run_empty(_counter) && result;
    result = d_tests_sa_test_run_pass_fn(_counter) && result;
    result = d_tests_sa_test_run_fail_fn(_counter) && result;
    result = d_tests_sa_test_run_mixed(_counter) && result;
    result = d_tests_sa_test_run_setup_hook(_counter) && result;
    result = d_tests_sa_test_run_teardown_hook(_counter) && result;
    result = d_tests_sa_test_run_setup_fail(_counter) && result;

    return result;
}
