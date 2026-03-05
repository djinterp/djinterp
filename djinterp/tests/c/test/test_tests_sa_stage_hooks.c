#include "./test_tests_sa.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

// dummy stage hook for testing
static bool
d_internal_stage_hook_pass(struct d_test* _test)
{
    (void)_test;

    return true;
}

// another dummy stage hook for uniqueness checks
static bool
d_internal_stage_hook_other(struct d_test* _test)
{
    (void)_test;

    return false;
}


/*
d_tests_sa_test_set_hook_null_test
  Tests d_test_set_stage_hook with NULL test.
  Tests the following:
  - d_test_set_stage_hook(NULL, stage, hook) returns false
*/
bool
d_tests_sa_test_set_hook_null_test
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
                 d_test_set_stage_hook(NULL,
                                       D_TEST_STAGE_SETUP,
                                       d_internal_stage_hook_pass) == false,
                 "d_test_set_hook_null_test",
                 "d_test_set_stage_hook(NULL, ...) returns false",
                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_set_hook_valid
  Tests d_test_set_stage_hook with valid parameters.
  Tests the following:
  - setting a setup hook returns true
  - setting a teardown hook returns true
*/
bool
d_tests_sa_test_set_hook_valid
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
                                   "d_test_set_hook_valid",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // set setup hook
    result = d_assert_standalone(
                 d_test_set_stage_hook(test,
                                       D_TEST_STAGE_SETUP,
                                       d_internal_stage_hook_pass) == true,
                 "d_test_set_hook_valid",
                 "setting SETUP hook returns true",
                 _counter) && result;

    // set teardown hook
    result = d_assert_standalone(
                 d_test_set_stage_hook(test,
                                       D_TEST_STAGE_TEAR_DOWN,
                                       d_internal_stage_hook_pass) == true,
                 "d_test_set_hook_valid",
                 "setting TEAR_DOWN hook returns true",
                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_get_hook_null_test
  Tests d_test_get_stage_hook with NULL test.
  Tests the following:
  - d_test_get_stage_hook(NULL, stage) returns NULL
*/
bool
d_tests_sa_test_get_hook_null_test
(
    struct d_test_counter* _counter
)
{
    fn_stage hook;
    bool     result;

    result = true;

    hook = d_test_get_stage_hook(NULL, D_TEST_STAGE_SETUP);

    result = d_assert_standalone(hook == NULL,
                                 "d_test_get_hook_null_test",
                                 "d_test_get_stage_hook(NULL, ...) returns NULL",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_get_hook_valid
  Tests d_test_get_stage_hook retrieves a previously set hook.
  Tests the following:
  - retrieved hook matches the hook that was set
  - different stages store different hooks independently
*/
bool
d_tests_sa_test_get_hook_valid
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    fn_stage       retrieved;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_get_hook_valid",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // set two different hooks for different stages
    d_test_set_stage_hook(test,
                          D_TEST_STAGE_SETUP,
                          d_internal_stage_hook_pass);
    d_test_set_stage_hook(test,
                          D_TEST_STAGE_TEAR_DOWN,
                          d_internal_stage_hook_other);

    // retrieve and verify setup hook
    retrieved = d_test_get_stage_hook(test, D_TEST_STAGE_SETUP);

    result = d_assert_standalone(retrieved == d_internal_stage_hook_pass,
                                 "d_test_get_hook_valid",
                                 "setup hook matches set value",
                                 _counter) && result;

    // retrieve and verify teardown hook
    retrieved = d_test_get_stage_hook(test, D_TEST_STAGE_TEAR_DOWN);

    result = d_assert_standalone(retrieved == d_internal_stage_hook_other,
                                 "d_test_get_hook_valid",
                                 "teardown hook matches set value",
                                 _counter) && result;

    // verify hooks are distinct
    result = d_assert_standalone(
                 d_test_get_stage_hook(test, D_TEST_STAGE_SETUP) !=
                 d_test_get_stage_hook(test, D_TEST_STAGE_TEAR_DOWN),
                 "d_test_get_hook_valid",
                 "different stages store independent hooks",
                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_get_hook_unset
  Tests d_test_get_stage_hook for a stage that has no hook set.
  Tests the following:
  - getting an unset stage returns NULL
*/
bool
d_tests_sa_test_get_hook_unset
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    fn_stage       retrieved;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_get_hook_unset",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // get hook for stage that was never set
    retrieved = d_test_get_stage_hook(test, D_TEST_STAGE_ON_SUCCESS);

    result = d_assert_standalone(retrieved == NULL,
                                 "d_test_get_hook_unset",
                                 "unset ON_SUCCESS stage returns NULL",
                                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_set_hook_all_stages
  Tests d_test_set_stage_hook for all DTestStage values.
  Tests the following:
  - SETUP, TEAR_DOWN, ON_SUCCESS, ON_FAILURE, BEFORE, AFTER all accept hooks
  - each hook can be retrieved independently
*/
bool
d_tests_sa_test_set_hook_all_stages
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;
    int            i;

    // all stages to test
    enum DTestStage stages[] =
    {
        D_TEST_STAGE_SETUP,
        D_TEST_STAGE_TEAR_DOWN,
        D_TEST_STAGE_ON_SUCCESS,
        D_TEST_STAGE_ON_FAILURE,
        D_TEST_STAGE_BEFORE,
        D_TEST_STAGE_AFTER
    };

    size_t stage_count;

    result      = true;
    stage_count = sizeof(stages) / sizeof(stages[0]);

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_set_hook_all_stages",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // set a hook for each stage
    for (i = 0; i < (int)stage_count; i++)
    {
        result = d_assert_standalone(
                     d_test_set_stage_hook(test,
                                           stages[i],
                                           d_internal_stage_hook_pass) == true,
                     "d_test_set_hook_all_stages",
                     "setting hook for stage succeeds",
                     _counter) && result;
    }

    // verify each stage has the hook set
    for (i = 0; i < (int)stage_count; i++)
    {
        result = d_assert_standalone(
                     d_test_get_stage_hook(test, stages[i]) ==
                         d_internal_stage_hook_pass,
                     "d_test_set_hook_all_stages",
                     "retrieving hook for stage returns correct value",
                     _counter) && result;
    }

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_stage_hooks_all
  Aggregation function for all stage hook tests.
*/
bool
d_tests_sa_test_stage_hooks_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_set_hook_null_test(_counter) && result;
    result = d_tests_sa_test_set_hook_valid(_counter) && result;
    result = d_tests_sa_test_get_hook_null_test(_counter) && result;
    result = d_tests_sa_test_get_hook_valid(_counter) && result;
    result = d_tests_sa_test_get_hook_unset(_counter) && result;
    result = d_tests_sa_test_set_hook_all_stages(_counter) && result;

    return result;
}
