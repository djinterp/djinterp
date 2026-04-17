#include "./test_block_tests_sa.h"


/******************************************************************************
 * RUN-CONFIG STAGE HOOK TEST HELPERS
 *****************************************************************************/

// file-scope state for hook verification
static bool g_hook_setup_called    = false;
static bool g_hook_teardown_called = false;
static bool g_hook_test_fn_called  = false;


/*
d_internal_hook_setup
  Setup hook that records it was called.

Parameter(s):
  _context: unused context pointer.
Return:
  true always.
*/
static bool
d_internal_hook_setup
(
    void* _context
)
{
    (void)_context;

    g_hook_setup_called = true;

    return true;
}

/*
d_internal_hook_setup_fail
  Setup hook that always fails.

Parameter(s):
  _context: unused context pointer.
Return:
  false always.
*/
static bool
d_internal_hook_setup_fail
(
    void* _context
)
{
    (void)_context;

    g_hook_setup_called = true;

    return false;
}

/*
d_internal_hook_teardown
  Teardown hook that records it was called.

Parameter(s):
  _context: unused context pointer.
Return:
  true always.
*/
static bool
d_internal_hook_teardown
(
    void* _context
)
{
    (void)_context;

    g_hook_teardown_called = true;

    return true;
}

/*
d_internal_hook_replacement
  Replacement hook for testing overwrite behavior.

Parameter(s):
  _context: unused context pointer.
Return:
  true always.
*/
static bool
d_internal_hook_replacement
(
    void* _context
)
{
    (void)_context;

    return true;
}

/*
d_internal_hook_test_fn_pass
  Test function that records it was called and passes.

Return:
  true always.
*/
static bool
d_internal_hook_test_fn_pass
(
    void
)
{
    g_hook_test_fn_called = true;

    return true;
}

/*
d_internal_hook_reset_globals
  Resets all file-scope hook test state flags to false.

Parameter(s):
  none.
Return:
  none.
*/
static void
d_internal_hook_reset_globals
(
    void
)
{
    g_hook_setup_called    = false;
    g_hook_teardown_called = false;
    g_hook_test_fn_called  = false;

    return;
}

/*
d_internal_hook_make_config_with_hook
  Creates a d_test_options with a stage_hooks map containing a single hook.

Parameter(s):
  _stage: the stage to register the hook for.
  _hook:  the hook function pointer.
Return:
  Pointer to the new d_test_options, or NULL on failure.
*/
static struct d_test_options*
d_internal_hook_make_config_with_hook
(
    enum DTestStage _stage,
    fn_stage        _hook
)
{
    struct d_test_options* config;

    config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return NULL;
    }

    if (!config->stage_hooks)
    {
        config->stage_hooks = d_min_enum_map_new();

        if (!config->stage_hooks)
        {
            d_test_options_free(config);

            return NULL;
        }
    }

    d_min_enum_map_put(config->stage_hooks,
                       (int)_stage,
                       (void*)_hook);

    return config;
}

/******************************************************************************
 * RUN-CONFIG STAGE HOOK TESTS
 *****************************************************************************/

/*
d_tests_sa_test_block_run_config_setup_hook
  Tests that d_test_block_run invokes the setup hook from run config.
  Tests the following:
  - setup hook in run config's stage_hooks is called
  - block run succeeds when setup succeeds
*/
bool
d_tests_sa_test_block_run_config_setup_hook
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block*   block;
    struct d_test_options* config;
    bool                   result;
    bool                   all_passed;

    all_passed = true;

    d_internal_hook_reset_globals();

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    config = d_internal_hook_make_config_with_hook(
                 D_TEST_STAGE_SETUP,
                 (fn_stage)d_internal_hook_setup);

    if (!config)
    {
        d_test_block_free(block);

        return false;
    }

    result = d_test_block_run(block, config);

    all_passed &= d_assert_standalone(
        g_hook_setup_called == true,
        "run_config_setup: hook called",
        "setup hook from run config should be invoked",
        _test_info);

    all_passed &= d_assert_standalone(
        result == true,
        "run_config_setup: return value",
        "block should return true when setup hook succeeds",
        _test_info);

    d_test_options_free(config);
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_run_config_teardown_hook
  Tests that d_test_block_run invokes the teardown hook from run config.
  Tests the following:
  - teardown hook in run config's stage_hooks is called
*/
bool
d_tests_sa_test_block_run_config_teardown_hook
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block*   block;
    struct d_test_options* config;
    bool                   all_passed;

    all_passed = true;

    d_internal_hook_reset_globals();

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    config = d_internal_hook_make_config_with_hook(
                 D_TEST_STAGE_TEAR_DOWN,
                 (fn_stage)d_internal_hook_teardown);

    if (!config)
    {
        d_test_block_free(block);

        return false;
    }

    d_test_block_run(block, config);

    all_passed &= d_assert_standalone(
        g_hook_teardown_called == true,
        "run_config_teardown: hook called",
        "teardown hook from run config should be invoked",
        _test_info);

    d_test_options_free(config);
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_run_config_setup_and_teardown
  Tests that both setup and teardown hooks from run config are invoked.
  Tests the following:
  - setup and teardown hooks are stored independently on config
  - both are invoked during a single block run
*/
bool
d_tests_sa_test_block_run_config_setup_and_teardown
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block*   block;
    struct d_test_options* config;
    bool                   result;
    bool                   all_passed;

    all_passed = true;

    d_internal_hook_reset_globals();

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // create config with both hooks
    config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        d_test_block_free(block);

        return false;
    }

    if (!config->stage_hooks)
    {
        config->stage_hooks = d_min_enum_map_new();

        if (!config->stage_hooks)
        {
            d_test_options_free(config);
            d_test_block_free(block);

            return false;
        }
    }

    d_min_enum_map_put(config->stage_hooks,
                       (int)D_TEST_STAGE_SETUP,
                       (void*)(fn_stage)d_internal_hook_setup);
    d_min_enum_map_put(config->stage_hooks,
                       (int)D_TEST_STAGE_TEAR_DOWN,
                       (void*)(fn_stage)d_internal_hook_teardown);

    result = d_test_block_run(block, config);

    all_passed &= d_assert_standalone(
        g_hook_setup_called == true,
        "run_config_both: setup called",
        "setup hook should be invoked",
        _test_info);

    all_passed &= d_assert_standalone(
        g_hook_teardown_called == true,
        "run_config_both: teardown called",
        "teardown hook should be invoked",
        _test_info);

    all_passed &= d_assert_standalone(
        result == true,
        "run_config_both: return value",
        "empty block with both hooks should return true",
        _test_info);

    d_test_options_free(config);
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_run_config_setup_fails
  Tests d_test_block_run when the setup hook in run config fails.
  Tests the following:
  - returns false immediately when setup hook fails
  - children are NOT executed
*/
bool
d_tests_sa_test_block_run_config_setup_fails
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block*   block;
    struct d_test_options* config;
    struct d_test_fn*      test_fn;
    struct d_test_type*    fn_type;
    struct d_test_type*    children[1];
    bool                   result;
    bool                   all_passed;

    all_passed = true;

    d_internal_hook_reset_globals();

    // create a block with a test_fn child to detect if children run
    test_fn = d_test_fn_new((fn_test)d_internal_hook_test_fn_pass);

    if (!test_fn)
    {
        return false;
    }

    fn_type = d_test_type_new(D_TEST_TYPE_TEST_FN, test_fn);

    if (!fn_type)
    {
        d_test_fn_free(test_fn);

        return false;
    }

    children[0] = fn_type;

    block = d_test_block_new(children, 1);

    if (!block)
    {
        d_test_type_free(fn_type);

        return false;
    }

    config = d_internal_hook_make_config_with_hook(
                 D_TEST_STAGE_SETUP,
                 (fn_stage)d_internal_hook_setup_fail);

    if (!config)
    {
        d_test_block_free(block);

        return false;
    }

    result = d_test_block_run(block, config);

    all_passed &= d_assert_standalone(
        g_hook_setup_called == true,
        "run_config_setup_fails: setup called",
        "setup hook should have been called",
        _test_info);

    all_passed &= d_assert_standalone(
        result == false,
        "run_config_setup_fails: return value",
        "block should return false when setup fails",
        _test_info);

    all_passed &= d_assert_standalone(
        g_hook_test_fn_called == false,
        "run_config_setup_fails: children skipped",
        "children should NOT execute when setup fails",
        _test_info);

    d_test_options_free(config);
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_run_config_null_hooks_map
  Tests d_test_block_run with a run config whose stage_hooks is NULL.
  Tests the following:
  - no crash when config exists but stage_hooks map is NULL
  - block runs normally without hooks
*/
bool
d_tests_sa_test_block_run_config_null_hooks_map
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block*   block;
    struct d_test_options* config;
    bool                   result;
    bool                   all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // create config without initializing stage_hooks
    config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        d_test_block_free(block);

        return false;
    }

    // ensure stage_hooks is NULL for this test
    if (config->stage_hooks)
    {
        d_min_enum_map_free(config->stage_hooks);
        config->stage_hooks = NULL;
    }

    result = d_test_block_run(block, config);

    all_passed &= d_assert_standalone(
        result == true,
        "run_config_null_hooks: return value",
        "block with NULL stage_hooks should run without crash",
        _test_info);

    d_test_options_free(config);
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_run_config_no_hook_set
  Tests d_test_block_run with a run config whose stage_hooks map exists
but has no hooks registered.
  Tests the following:
  - no crash when map exists but requested stage has no entry
  - block runs normally
*/
bool
d_tests_sa_test_block_run_config_no_hook_set
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block*   block;
    struct d_test_options* config;
    bool                   result;
    bool                   all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        d_test_block_free(block);

        return false;
    }

    // create empty stage_hooks map (no hooks registered)
    if (!config->stage_hooks)
    {
        config->stage_hooks = d_min_enum_map_new();

        if (!config->stage_hooks)
        {
            d_test_options_free(config);
            d_test_block_free(block);

            return false;
        }
    }

    result = d_test_block_run(block, config);

    all_passed &= d_assert_standalone(
        result == true,
        "run_config_no_hook: return value",
        "block with empty stage_hooks map should run normally",
        _test_info);

    d_test_options_free(config);
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_run_config_overwrite_hook
  Tests that overwriting a hook in the run config's stage_hooks map
works correctly.
  Tests the following:
  - second put to the same stage key overwrites the first
  - only the replacement hook is invoked
*/
bool
d_tests_sa_test_block_run_config_overwrite_hook
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block*   block;
    struct d_test_options* config;
    bool                   result;
    bool                   all_passed;

    all_passed = true;

    d_internal_hook_reset_globals();

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // create config with setup hook, then overwrite it
    config = d_internal_hook_make_config_with_hook(
                 D_TEST_STAGE_SETUP,
                 (fn_stage)d_internal_hook_setup);

    if (!config)
    {
        d_test_block_free(block);

        return false;
    }

    // overwrite with replacement that does NOT set the global flag
    d_min_enum_map_put(config->stage_hooks,
                       (int)D_TEST_STAGE_SETUP,
                       (void*)(fn_stage)d_internal_hook_replacement);

    result = d_test_block_run(block, config);

    // the replacement hook does NOT set g_hook_setup_called
    all_passed &= d_assert_standalone(
        g_hook_setup_called == false,
        "run_config_overwrite: original not called",
        "original setup hook should not be invoked after overwrite",
        _test_info);

    all_passed &= d_assert_standalone(
        result == true,
        "run_config_overwrite: return value",
        "block should succeed with replacement hook",
        _test_info);

    d_test_options_free(config);
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_run_config_null_config
  Tests d_test_block_run with NULL run config.
  Tests the following:
  - no crash when run config is NULL
  - block runs normally without any hooks
*/
bool
d_tests_sa_test_block_run_config_null_config
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    result = d_test_block_run(block, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "run_config_null: return value",
        "block with NULL run config should run without crash",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_stage_hooks_all
  Aggregation function for all run-config stage hook tests.
  Tests the following:
  - setup hook via run config
  - teardown hook via run config
  - both hooks together
  - setup failure early-exit via run config
  - NULL hooks map, empty hooks map, overwrite, NULL config
*/
bool
d_tests_sa_test_block_stage_hooks_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_test_block_run_config_setup_hook(_test_info);
    all_passed &= d_tests_sa_test_block_run_config_teardown_hook(_test_info);
    all_passed &= d_tests_sa_test_block_run_config_setup_and_teardown(_test_info);
    all_passed &= d_tests_sa_test_block_run_config_setup_fails(_test_info);
    all_passed &= d_tests_sa_test_block_run_config_null_hooks_map(_test_info);
    all_passed &= d_tests_sa_test_block_run_config_no_hook_set(_test_info);
    all_passed &= d_tests_sa_test_block_run_config_overwrite_hook(_test_info);
    all_passed &= d_tests_sa_test_block_run_config_null_config(_test_info);

    return all_passed;
}
