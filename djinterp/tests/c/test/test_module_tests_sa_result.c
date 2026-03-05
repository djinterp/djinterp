/******************************************************************************
* djinterp [test]                                  test_module_tests_sa_result.c
*
*   Unit tests for test_module.h result functions.
*   Note: children that must be executed via d_test_module_run are passed
* through the constructor array (d_test_type**), NOT through add_child.
*   d_test_module_result is pre-allocated by the constructor and always
* non-NULL. reset_result zeroes the fields but does not free/NULL the
* pointer.
*
* path:      \test\test\test_module_tests_sa_result.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_module_tests_sa.h"


//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

static bool
result_passing_fn(void)
{
    return true;
}


//=============================================================================
// RESULT TESTS
//=============================================================================

/*
d_tests_sa_test_module_get_result
  Tests retrieving the result from a module after execution.
  Children are supplied via the constructor array.
  Tests the following:
  - get_result on a module that has been run returns non-NULL
  - the result status reflects PASSED for a passing module
  - duration_ms is non-negative
  - tests_passed <= tests_total
*/
bool
d_tests_sa_test_module_get_result
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
    struct d_test_module_result* res;

    printf("  --- Testing d_test_module_get_result ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create and run a module with one passing test fn
    tfn = d_test_fn_new(result_passing_fn);

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
    d_test_module_run(mod, NULL);

    // get result
    res = d_test_module_get_result(mod);

    if (!d_assert_standalone(res != NULL,
                             "get_result returned non-NULL after run",
                             "get_result should return non-NULL after run",
                             _test_info))
    {
        all_passed = false;
    }

    if (res)
    {
        // verify status
        if (!d_assert_standalone(
                res->status == D_TEST_MODULE_STATUS_PASSED,
                "result status is PASSED",
                "result status should be PASSED",
                _test_info))
        {
            all_passed = false;
        }

        // verify duration is non-negative
        if (!d_assert_standalone(res->duration_ms >= 0.0,
                                 "duration_ms is non-negative",
                                 "duration_ms should be non-negative",
                                 _test_info))
        {
            all_passed = false;
        }

        // verify passed <= total
        if (!d_assert_standalone(
                res->tests_passed <= res->tests_total,
                "tests_passed <= tests_total",
                "tests_passed should be <= tests_total",
                _test_info))
        {
            all_passed = false;
        }
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_get_result unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_get_result unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_get_result_null
  Tests get_result with NULL module and pre-run module.
  Note: the result is pre-allocated by the constructor, so get_result
  on an unrun module returns non-NULL with default/zeroed values.
  Tests the following:
  - get_result(NULL) returns NULL
  - get_result on an unrun module returns non-NULL (pre-allocated)
  - the pre-allocated result has PENDING status
*/
bool
d_tests_sa_test_module_get_result_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module*        mod;
    struct d_test_module_result* res;

    printf("  --- Testing d_test_module_get_result (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // NULL module
    res = d_test_module_get_result(NULL);

    if (!d_assert_standalone(res == NULL,
                             "get_result(NULL) returns NULL",
                             "get_result(NULL) should return NULL",
                             _test_info))
    {
        all_passed = false;
    }

    // unrun module — result is pre-allocated with default values
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        res = d_test_module_get_result(mod);

        if (!d_assert_standalone(res != NULL,
                                 "get_result on unrun module is non-NULL "
                                 "(pre-allocated)",
                                 "get_result on unrun module should be "
                                 "non-NULL (pre-allocated)",
                                 _test_info))
        {
            all_passed = false;
        }

        // verify the pre-allocated result has UNKNOWN status (zero-init)
        if (res)
        {
            if (!d_assert_standalone(
                    res->status == D_TEST_MODULE_STATUS_UNKNOWN,
                    "pre-allocated result status is UNKNOWN (zero-init)",
                    "pre-allocated result status should be UNKNOWN",
                    _test_info))
            {
                all_passed = false;
            }
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_get_result (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_get_result (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_reset_result
  Tests clearing a module's result state.
  Children are supplied via the constructor array for the run step.
  Note: reset_result zeroes the result fields and resets the module status
  to PENDING, but does not free/NULL the result pointer.
  Tests the following:
  - reset_result clears the result fields after a run
  - module status returns to PENDING after reset
  - result pointer is still non-NULL after reset (pre-allocated)
*/
bool
d_tests_sa_test_module_reset_result
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
    struct d_test_module_result* res;

    printf("  --- Testing d_test_module_reset_result ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create a passing test fn via constructor array
    tfn = d_test_fn_new(result_passing_fn);

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
        _test_info->tests_total++;

        return false;
    }

    // pass children via constructor array
    children[0] = child;
    mod = d_test_module_new(children, 1);

    if (!mod)
    {
        d_test_type_free(child);
        _test_info->tests_total++;

        return false;
    }

    // run to populate result
    d_test_module_run(mod, NULL);

    // verify result exists before reset
    res = d_test_module_get_result(mod);

    if (!d_assert_standalone(res != NULL,
                             "result exists before reset",
                             "result should exist before reset",
                             _test_info))
    {
        all_passed = false;
    }

    // reset the result
    d_test_module_reset_result(mod);

    // result pointer is still non-NULL (reset zeroes fields, not pointer)
    res = d_test_module_get_result(mod);

    if (!d_assert_standalone(res != NULL,
                             "result is non-NULL after reset "
                             "(pre-allocated, zeroed)",
                             "result should be non-NULL after reset "
                             "(pre-allocated, zeroed)",
                             _test_info))
    {
        all_passed = false;
    }

    // verify status returns to PENDING
    if (!d_assert_standalone(
            mod->status == D_TEST_MODULE_STATUS_PENDING,
            "status is PENDING after reset",
            "status should be PENDING after reset",
            _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_reset_result unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_reset_result unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_get_pass_rate
  Tests computing the pass rate after running a module.
  Children are supplied via the constructor array.
  Note: fn_test children are not tracked in the module result counters.
  A module with only fn_test children will report 0% pass rate.
  Tests the following:
  - pass rate is 0.0% for a module with only fn_test children
  - pass rate is in [0, 100] range
*/
bool
d_tests_sa_test_module_get_pass_rate
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
    double                rate;

    printf("  --- Testing d_test_module_get_pass_rate ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // create module with one fn_test child via constructor array
    tfn = d_test_fn_new(result_passing_fn);

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
    d_test_module_run(mod, NULL);

    // get pass rate
    rate = d_test_module_get_pass_rate(mod);

    // rate should be in valid range
    if (!d_assert_standalone(rate >= 0.0 && rate <= 100.0,
                             "pass rate is in [0, 100] range",
                             "pass rate should be in [0, 100] range",
                             _test_info))
    {
        all_passed = false;
    }

    // fn_test children are not tracked in counters, so pass rate is 0%
    if (!d_assert_standalone(rate == 0.0,
                             "pass rate is 0% (fn_test not tracked "
                             "in counters)",
                             "pass rate should be 0% (fn_test not tracked)",
                             _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_get_pass_rate unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_get_pass_rate unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_module_get_pass_rate_empty
  Tests pass rate on an empty or NULL module.
  Tests the following:
  - pass rate on an unrun module is 0.0
  - pass rate on a NULL module is 0.0
*/
bool
d_tests_sa_test_module_get_pass_rate_empty
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    double                rate;

    printf("  --- Testing d_test_module_get_pass_rate (empty) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // unrun module
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        rate = d_test_module_get_pass_rate(mod);

        if (!d_assert_standalone(rate == 0.0,
                                 "unrun module has 0% pass rate",
                                 "unrun module should have 0% pass rate",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    // NULL module
    rate = d_test_module_get_pass_rate(NULL);

    if (!d_assert_standalone(rate == 0.0,
                             "NULL module has 0% pass rate",
                             "NULL module should have 0% pass rate",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s get_pass_rate (empty) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s get_pass_rate (empty) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
