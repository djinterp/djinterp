/******************************************************************************
* djinterp [test]                                   test_module_tests_sa_hooks.c
*
*   Unit tests for test_module.h stage hook functions.
*   Note: stage hooks on d_test_module require the module's internal config
* to have an initialized stage_hooks map. A default-constructed module does
* not support direct hook set/get; hooks are typically managed via the
* d_test_type wrapper configuration layer.
*
* path:      \test\test\test_module_tests_sa_hooks.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_module_tests_sa.h"


//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

static bool
hook_setup_fn(struct d_test* _test)
{
    (void)_test;

    return true;
}

static bool
hook_teardown_fn(struct d_test* _test)
{
    (void)_test;

    return true;
}


//=============================================================================
// STAGE HOOK TESTS
//=============================================================================

/*
d_tests_sa_test_module_set_stage_hook
  Tests setting a stage hook on a default-constructed module.
  Note: set_stage_hook returns false on a default module because the
  module's internal stage_hooks map is not initialized. This is by design;
  hooks are managed at the d_test_type wrapper configuration level.
  Tests the following:
  - set_stage_hook on a default module returns false
  - the module remains in a valid state after the failed set
*/
bool
d_tests_sa_test_module_set_stage_hook
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    bool                  set_result;

    printf("  --- Testing d_test_module_set_stage_hook ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // set_stage_hook on a default module returns false (no hooks map)
    set_result = d_test_module_set_stage_hook(mod,
                                              D_TEST_STAGE_SETUP,
                                              hook_setup_fn);

    if (!d_assert_standalone(!set_result,
                             "set_stage_hook on default module returns false",
                             "set_stage_hook on default module should "
                             "return false (no hooks map)",
                             _test_info))
    {
        all_passed = false;
    }

    // verify module is still valid (status unchanged)
    if (!d_assert_standalone(
            mod->status == D_TEST_MODULE_STATUS_PENDING,
            "module status still PENDING after failed set",
            "module status should remain PENDING after failed set",
            _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_set_stage_hook unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_set_stage_hook unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_get_stage_hook
  Tests retrieving hooks from a default module (none are set).
  Tests the following:
  - get_stage_hook for SETUP returns NULL on a default module
  - get_stage_hook for TEAR_DOWN returns NULL on a default module
*/
bool
d_tests_sa_test_module_get_stage_hook
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    fn_stage              got;

    printf("  --- Testing d_test_module_get_stage_hook ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // get on default module returns NULL (no hooks set)
    got = d_test_module_get_stage_hook(mod, D_TEST_STAGE_SETUP);

    if (!d_assert_standalone(got == NULL,
                             "SETUP hook is NULL on default module",
                             "SETUP hook should be NULL on default module",
                             _test_info))
    {
        all_passed = false;
    }

    got = d_test_module_get_stage_hook(mod, D_TEST_STAGE_TEAR_DOWN);

    if (!d_assert_standalone(got == NULL,
                             "TEAR_DOWN hook is NULL on default module",
                             "TEAR_DOWN hook should be NULL on default module",
                             _test_info))
    {
        all_passed = false;
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s d_test_module_get_stage_hook unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s d_test_module_get_stage_hook unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_get_stage_hook_unset
  Tests retrieving hooks for stages that were never set.
  Tests the following:
  - get_stage_hook returns NULL for all unset stages
  - each of the six DTestStage values returns NULL on a fresh module
*/
bool
d_tests_sa_test_module_get_stage_hook_unset
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    fn_stage              got;

    printf("  --- Testing d_test_module_get_stage_hook (unset) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        // no hooks set; all should return NULL
        got = d_test_module_get_stage_hook(mod, D_TEST_STAGE_SETUP);

        if (!d_assert_standalone(got == NULL,
                                 "unset SETUP hook is NULL",
                                 "unset SETUP hook should be NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        got = d_test_module_get_stage_hook(mod, D_TEST_STAGE_TEAR_DOWN);

        if (!d_assert_standalone(got == NULL,
                                 "unset TEAR_DOWN hook is NULL",
                                 "unset TEAR_DOWN hook should be NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        got = d_test_module_get_stage_hook(mod, D_TEST_STAGE_ON_SUCCESS);

        if (!d_assert_standalone(got == NULL,
                                 "unset ON_SUCCESS hook is NULL",
                                 "unset ON_SUCCESS hook should be NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s get_stage_hook (unset) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s get_stage_hook (unset) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_set_stage_hook_null
  Tests set_stage_hook with NULL arguments.
  Tests the following:
  - set_stage_hook on a NULL module returns false
  - set_stage_hook with a NULL hook function returns false
*/
bool
d_tests_sa_test_module_set_stage_hook_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    bool                  result;

    printf("  --- Testing set_stage_hook (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // NULL module
    result = d_test_module_set_stage_hook(NULL,
                                          D_TEST_STAGE_SETUP,
                                          hook_setup_fn);

    if (!d_assert_standalone(!result,
                             "set_stage_hook on NULL module returns false",
                             "set_stage_hook on NULL module should "
                             "return false",
                             _test_info))
    {
        all_passed = false;
    }

    // NULL hook function
    mod = d_test_module_new(NULL, 0);

    if (mod)
    {
        result = d_test_module_set_stage_hook(mod,
                                              D_TEST_STAGE_SETUP,
                                              NULL);

        if (!d_assert_standalone(!result,
                                 "set_stage_hook with NULL fn returns false",
                                 "set_stage_hook with NULL fn should "
                                 "return false",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_module_free(mod);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s set_stage_hook (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s set_stage_hook (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_module_stage_hooks_all_stages
  Tests that all six DTestStage values return NULL hooks on a fresh module
and that set attempts return false (hooks not supported on default modules).
  Tests the following:
  - set_stage_hook returns false for all six stages on a default module
  - get_stage_hook returns NULL for all six stages on a default module
*/
bool
d_tests_sa_test_module_stage_hooks_all_stages
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_module* mod;
    enum DTestStage       stages[6];
    fn_stage              got;
    bool                  set_ok;
    size_t                i;

    printf("  --- Testing stage hooks (all stages) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    stages[0] = D_TEST_STAGE_SETUP;
    stages[1] = D_TEST_STAGE_TEAR_DOWN;
    stages[2] = D_TEST_STAGE_ON_SUCCESS;
    stages[3] = D_TEST_STAGE_ON_FAILURE;
    stages[4] = D_TEST_STAGE_BEFORE;
    stages[5] = D_TEST_STAGE_AFTER;

    mod = d_test_module_new(NULL, 0);

    if (!mod)
    {
        printf("  %s module allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // verify set returns false for all stages on default module
    for (i = 0; i < 6; i++)
    {
        set_ok = d_test_module_set_stage_hook(mod,
                                               stages[i],
                                               hook_setup_fn);

        if (!d_assert_standalone(!set_ok,
                                 "set_stage_hook returns false on "
                                 "default module for stage",
                                 "set_stage_hook should return false on "
                                 "default module (no hooks map)",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    // verify get returns NULL for all stages
    for (i = 0; i < 6; i++)
    {
        got = d_test_module_get_stage_hook(mod, stages[i]);

        if (!d_assert_standalone(got == NULL,
                                 "get_stage_hook returns NULL for each stage",
                                 "get_stage_hook should return NULL for "
                                 "each stage on default module",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    d_test_module_free(mod);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s stage hooks (all stages) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s stage hooks (all stages) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
