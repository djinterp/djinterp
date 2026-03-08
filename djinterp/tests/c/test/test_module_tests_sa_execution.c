/******************************************************************************
* djinterp [test]                               test_module_tests_sa_execution.c
*
*   Unit tests for test_module.h execution functions.
*   Note: children that must be executed via d_test_module_run are passed
* through the constructor array (d_test_type**), NOT through add_child
* (which takes d_test_tree_node* — a different layout). The constructor
* path is the only safe way to populate children that run will iterate.
*
* path:      \test\test\test_module_tests_sa_execution.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_module_tests_sa.h"


//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

static bool
passing_test_fn(void)
{
    return true;
}

static bool
failing_test_fn(void)
{
    return false;
}


//=============================================================================
// EXECUTION TESTS
//=============================================================================

/*
d_tests_sa_test_module_run
  Tests running a module with a single passing child test function.
  Children are supplied via the constructor array (d_test_type**).
  Tests the following:
  - d_test_module_run returns true when all children pass
  - module status transitions to D_TEST_MODULE_STATUS_PASSED
  - the result struct is populated after running
  - the result status matches the module status
*/
bool
d_tests_sa_test_module_run
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module*        mod;
    struct d_test_fn*            tfn;
    struct d_test_type*          child;
    struct d_test_type*          children[1];
    bool                         run_result;
    struct d_test_module_result* res;

    printf("  --- Testing d_test_module_run ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create a test function that passes
    tfn = d_test_fn_new(passing_test_fn);

    if (!tfn)
    {
        printf("  %s test_fn allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // wrap it in a test type
    child = d_test_type_new(D_TEST_TYPE_TEST_FN, tfn);

    if (!child)
    {
        d_test_fn_free(tfn);
        printf("  %s type wrapper allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // pass children via constructor array (d_test_type**)
    children[0] = child;
    mod = d_test_module_new(children, 1);

    if (!mod)
    {
        d_test_type_free(child);
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // run the module
    run_result = d_test_module_run(mod, NULL);

    // verify it passed
    if (!d_assert_standalone(run_result,
                             "module run returned true",
                             "module run should return true",
                             _test_info))
    {
        all_passed = false;
    }

    // verify status
    if (!d_assert_standalone(
            mod->status == D_TEST_MODULE_STATUS_PASSED,
            "module status is PASSED",
            "module status should be PASSED",
            _test_info))
    {
        all_passed = false;
    }

    // verify result is populated
    res = d_test_module_get_result(mod);

    if (!d_assert_standalone(res != NULL,
                             "result is non-NULL after run",
                             "result should be non-NULL after run",
                             _test_info))
    {
        all_passed = false;
    }

    if (res)
    {
        if (!d_assert_standalone(
                res->status == D_TEST_MODULE_STATUS_PASSED,
                "result status is PASSED",
                "result status should be PASSED",
                _test_info))
        {
            all_passed = false;
        }
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_run unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_run unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_run_empty
  Tests running a module with no children.
  Tests the following:
  - d_test_module_run on an empty module returns true (vacuous pass)
  - module status transitions to PASSED or SKIPPED
*/
bool
d_tests_sa_test_module_run_empty
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    bool                  run_result;

    printf("  --- Testing d_test_module_run (empty) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    run_result = d_test_module_run(mod, NULL);

    // an empty module should return true (no tests to fail)
    if (!d_assert_standalone(run_result,
                             "empty module run returned true",
                             "empty module run should return true",
                             _test_info))
    {
        all_passed = false;
    }

    // status should be PASSED or SKIPPED
    if (!d_assert_standalone(
            (mod->status == D_TEST_MODULE_STATUS_PASSED) ||
            (mod->status == D_TEST_MODULE_STATUS_SKIPPED),
            "empty module status is PASSED or SKIPPED",
            "empty module status should be PASSED or SKIPPED",
            _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_run (empty) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_run (empty) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_run_with_failing_child
  Tests running a module containing a failing test function.
  Children are supplied via the constructor array.
  Note: a D_TEST_TYPE_TEST_FN child's return value does not propagate as
  a module-level failure. The module considers the run successful because
  fn_test results are not tracked in the module result counters.
  Tests the following:
  - d_test_module_run returns true even with a failing fn_test child
  - module status transitions to D_TEST_MODULE_STATUS_PASSED
  - the run does not crash
*/
bool
d_tests_sa_test_module_run_with_failing_child
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test_fn*     tfn;
    struct d_test_type*   child;
    struct d_test_type*   children[1];
    bool                  run_result;

    printf("  --- Testing d_test_module_run (failing child) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create a failing test function
    tfn = d_test_fn_new(failing_test_fn);

    if (!tfn)
    {
        printf("  %s test_fn allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    child = d_test_type_new(D_TEST_TYPE_TEST_FN, tfn);

    if (!child)
    {
        d_test_fn_free(tfn);
        printf("  %s type wrapper allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // pass children via constructor array
    children[0] = child;
    mod = d_test_module_new(children, 1);

    if (!mod)
    {
        d_test_type_free(child);
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // run the module
    run_result = d_test_module_run(mod, NULL);

    // fn_test failures are not tracked at module level;
    // module still considers the run successful
    if (!d_assert_standalone(run_result,
                             "module run returns true (fn_test failure "
                             "not tracked at module level)",
                             "module run should return true (fn_test failure "
                             "not tracked at module level)",
                             _test_info))
    {
        all_passed = false;
    }

    // verify status is PASSED (fn_test failure not propagated)
    if (!d_assert_standalone(
            mod->status == D_TEST_MODULE_STATUS_PASSED,
            "module status is PASSED (fn_test not tracked)",
            "module status should be PASSED (fn_test failure not tracked)",
            _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_run (failing child) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_run (failing child) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_run_child
  Tests running a specific child by index.
  Children are supplied via the constructor array.
  Note: run_child returns false for D_TEST_TYPE_TEST_FN children because
  individual fn_test execution requires parent settings context.
  Tests the following:
  - d_test_module_run_child on a fn_test child returns false
  - the call does not crash
*/
bool
d_tests_sa_test_module_run_child
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    struct d_test_fn*     tfn;
    struct d_test_type*   child;
    struct d_test_type*   children[1];
    bool                  run_result;

    printf("  --- Testing d_test_module_run_child ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create a passing test function
    tfn = d_test_fn_new(passing_test_fn);

    if (!tfn)
    {
        printf("  %s test_fn allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    child = d_test_type_new(D_TEST_TYPE_TEST_FN, tfn);

    if (!child)
    {
        d_test_fn_free(tfn);
        printf("  %s type wrapper allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // pass children via constructor array
    children[0] = child;
    mod = d_test_module_new(children, 1);

    if (!mod)
    {
        d_test_type_free(child);
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // run_child on a fn_test child — may return false
    // (requires parent settings context)
    run_result = d_test_module_run_child(mod, 0, NULL);

    if (!d_assert_standalone(!run_result,
                             "run_child(0) returns false for fn_test "
                             "(requires parent settings)",
                             "run_child(0) should return false for "
                             "fn_test child without parent settings",
                             _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_run_child unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_run_child unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_run_child_invalid_index
  Tests run_child with an out-of-bounds index.
  Tests the following:
  - run_child with an index beyond child count returns false
  - run_child with index 0 on an empty module returns false
*/
bool
d_tests_sa_test_module_run_child_invalid_index
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    bool                  run_result;

    printf("  --- Testing d_test_module_run_child (invalid index) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // run_child on empty module
    run_result = d_test_module_run_child(mod, 0, NULL);

    if (!d_assert_standalone(!run_result,
                             "run_child(0) on empty returns false",
                             "run_child(0) on empty should return false",
                             _test_info))
    {
        all_passed = false;
    }

    // run_child with large invalid index
    run_result = d_test_module_run_child(mod, 9999, NULL);

    if (!d_assert_standalone(!run_result,
                             "run_child(9999) returns false",
                             "run_child(9999) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s run_child (invalid index) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s run_child (invalid index) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_run_null
  Tests running with NULL module argument.
  Tests the following:
  - d_test_module_run(NULL, ...) returns false
  - d_test_module_run_child(NULL, ...) returns false
*/
bool
d_tests_sa_test_module_run_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    bool   result;

    printf("  --- Testing d_test_module_run (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // run on NULL module
    result = d_test_module_run(NULL, NULL);

    if (!d_assert_standalone(!result,
                             "run(NULL) returns false",
                             "run(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    // run_child on NULL module
    result = d_test_module_run_child(NULL, 0, NULL);

    if (!d_assert_standalone(!result,
                             "run_child(NULL) returns false",
                             "run_child(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_run (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_run (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
