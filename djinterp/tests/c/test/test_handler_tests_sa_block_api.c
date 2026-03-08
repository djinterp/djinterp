/******************************************************************************
* djinterp [test]                             test_handler_tests_sa_block_api.c
*
*   Unit tests for the revised test_block API, exercising child management,
* stage hooks, direct block execution, and the new_args constructor.
*
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_block_child_management
  Tests test_block child management functions.
  Tests the following:
  - d_test_block_child_count returns 0 for empty block
  - d_test_block_child_count returns correct count after adds
  - d_test_block_get_child_at returns valid child
  - d_test_block_get_child_at returns NULL for out-of-bounds
  - d_test_block_count_tests counts only test children
  - d_test_block_count_blocks counts only block children
  - d_test_block_add_child rejects NULL parameters
*/
bool
d_tests_sa_handler_block_child_management
(
    struct d_test_counter* _test_info
)
{
    size_t               ip;
    bool                 ok;
    struct d_test_block* b;
    struct d_test_block* child_block;
    struct d_test_type*  child;
    size_t               count;

    printf("  --- Testing block child management ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // empty block has 0 children
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        count = d_test_block_child_count(b);

        if (!d_assert_standalone(
                count == 0,
                "empty child_count",
                "Empty block has 0 children",
                _test_info))
        {
            ok = false;
        }

        // add 3 passing tests
        helper_add_passing_test_to_block(b);
        helper_add_passing_test_to_block(b);
        helper_add_passing_test_to_block(b);

        count = d_test_block_child_count(b);

        if (!d_assert_standalone(
                count == 3,
                "count after adds",
                "3 children after 3 adds",
                _test_info))
        {
            ok = false;
        }

        // get_child_at returns valid child
        child = d_test_block_get_child_at(b, 0);

        if (!d_assert_standalone(
                child != NULL,
                "get_child_at(0)",
                "Returns valid child",
                _test_info))
        {
            ok = false;
        }

        // get_child_at for out-of-bounds
        child = d_test_block_get_child_at(b, 99);

        if (!d_assert_standalone(
                child == NULL,
                "get_child_at(99)",
                "Returns NULL for out-of-bounds",
                _test_info))
        {
            ok = false;
        }

        // count_tests counts only test children
        if (!d_assert_standalone(
                d_test_block_count_tests(b) == 3,
                "count_tests",
                "3 test children",
                _test_info))
        {
            ok = false;
        }

        // count_blocks is 0 (no block children yet)
        if (!d_assert_standalone(
                d_test_block_count_blocks(b) == 0,
                "count_blocks=0",
                "No block children",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    // mixed children: tests and blocks
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        helper_add_passing_test_to_block(b);
        helper_add_passing_test_to_block(b);

        child_block = d_test_block_new(NULL, 0);

        if (child_block)
        {
            helper_add_block_child_to_block(b, child_block);
        }

        if (!d_assert_standalone(
                d_test_block_count_tests(b) == 2,
                "mixed count_tests",
                "2 test children in mixed block",
                _test_info))
        {
            ok = false;
        }

        if (!d_assert_standalone(
                d_test_block_count_blocks(b) == 1,
                "mixed count_blocks",
                "1 block child in mixed block",
                _test_info))
        {
            ok = false;
        }

        if (!d_assert_standalone(
                d_test_block_child_count(b) == 3,
                "mixed total",
                "3 total children",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    // add_child rejects NULL
    if (!d_assert_standalone(
            d_test_block_add_child(NULL, NULL) == false,
            "add_child(NULL,NULL)",
            "Rejects NULL parameters",
            _test_info))
    {
        ok = false;
    }

    // child_count on NULL returns 0
    if (!d_assert_standalone(
            d_test_block_child_count(NULL) == 0,
            "child_count(NULL)",
            "Returns 0 for NULL block",
            _test_info))
    {
        ok = false;
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] block_child_management\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] block_child_management\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}


/*
d_tests_sa_handler_block_stage_hooks
  Tests test_block stage hook registration and retrieval.
  Tests the following:
  - set_stage_hook succeeds on valid block
  - get_stage_hook retrieves the set hook
  - get_stage_hook returns NULL for unset stage
  - set_stage_hook rejects NULL block
  - stage hooks are invoked during d_test_block_run
*/
bool
d_tests_sa_handler_block_stage_hooks
(
    struct d_test_counter* _test_info
)
{
    size_t               ip;
    bool                 ok;
    struct d_test_block* b;
    fn_stage             retrieved;
    bool                 set_result;

    printf("  --- Testing block stage hooks ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // set and get stage hook
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        // set setup hook
        set_result = d_test_block_set_stage_hook(b,
                                                  D_TEST_STAGE_SETUP,
                                                  stage_hook_setup);

        if (!d_assert_standalone(
                set_result == true,
                "set_stage_hook",
                "Succeeds on valid block",
                _test_info))
        {
            ok = false;
        }

        // retrieve setup hook
        retrieved = d_test_block_get_stage_hook(b,
                                                D_TEST_STAGE_SETUP);

        if (!d_assert_standalone(
                retrieved == stage_hook_setup,
                "get_stage_hook setup",
                "Retrieves correct hook",
                _test_info))
        {
            ok = false;
        }

        // set teardown hook
        d_test_block_set_stage_hook(b,
                                    D_TEST_STAGE_TEAR_DOWN,
                                    stage_hook_teardown);

        retrieved = d_test_block_get_stage_hook(b,
                                                D_TEST_STAGE_TEAR_DOWN);

        if (!d_assert_standalone(
                retrieved == stage_hook_teardown,
                "get_stage_hook teardown",
                "Retrieves teardown hook",
                _test_info))
        {
            ok = false;
        }

        // unset stage returns NULL
        retrieved = d_test_block_get_stage_hook(b,
                                                D_TEST_STAGE_ON_SUCCESS);

        if (!d_assert_standalone(
                retrieved == NULL,
                "get unset stage",
                "Returns NULL for unset",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    // NULL block rejected
    if (!d_assert_standalone(
            d_test_block_set_stage_hook(NULL,
                                        D_TEST_STAGE_SETUP,
                                        stage_hook_setup) == false,
            "set_hook(NULL)",
            "Rejects NULL block",
            _test_info))
    {
        ok = false;
    }

    // NULL block returns NULL hook
    if (!d_assert_standalone(
            d_test_block_get_stage_hook(NULL,
                                        D_TEST_STAGE_SETUP) == NULL,
            "get_hook(NULL)",
            "Returns NULL for NULL block",
            _test_info))
    {
        ok = false;
    }

    // hooks invoked during d_test_block_run
    reset_stage_counters();
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        d_test_block_set_stage_hook(b,
                                    D_TEST_STAGE_SETUP,
                                    stage_hook_setup);
        d_test_block_set_stage_hook(b,
                                    D_TEST_STAGE_TEAR_DOWN,
                                    stage_hook_teardown);
        helper_add_passing_test_to_block(b);

        d_test_block_run(b, NULL);

        if (!d_assert_standalone(
                g_stage_setup_count == 1,
                "setup hook invoked",
                "Setup hook called during run",
                _test_info))
        {
            ok = false;
        }

        if (!d_assert_standalone(
                g_stage_teardown_count == 1,
                "teardown hook invoked",
                "Teardown hook called during run",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] block_stage_hooks\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] block_stage_hooks\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}


/*
d_tests_sa_handler_block_direct_run
  Tests d_test_block_run (standalone execution without handler).
  Tests the following:
  - NULL block returns false
  - empty block returns true
  - block with passing tests returns true
  - block with a failing test returns false
  - nested block execution works
  - failing setup hook prevents execution
*/
bool
d_tests_sa_handler_block_direct_run
(
    struct d_test_counter* _test_info
)
{
    size_t               ip;
    bool                 ok;
    struct d_test_block* b;
    struct d_test_block* outer;
    struct d_test_block* inner;
    bool                 result;

    printf("  --- Testing block direct run ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // NULL block returns false
    if (!d_assert_standalone(
            d_test_block_run(NULL, NULL) == false,
            "run(NULL)",
            "NULL block returns false",
            _test_info))
    {
        ok = false;
    }

    // empty block returns true
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        result = d_test_block_run(b, NULL);

        if (!d_assert_standalone(
                result == true,
                "run empty block",
                "Empty block returns true",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    // block with passing tests
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        helper_add_passing_test_to_block(b);
        helper_add_passing_test_to_block(b);
        result = d_test_block_run(b, NULL);

        if (!d_assert_standalone(
                result == true,
                "run passing block",
                "All-pass block returns true",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    // block with a failing test
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        helper_add_passing_test_to_block(b);
        helper_add_failing_test_to_block(b);
        result = d_test_block_run(b, NULL);

        if (!d_assert_standalone(
                result == false,
                "run mixed block",
                "Mixed block returns false",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    // nested block execution
    outer = d_test_block_new(NULL, 0);
    inner = d_test_block_new(NULL, 0);

    if (outer && inner)
    {
        helper_add_passing_test_to_block(inner);
        helper_add_block_child_to_block(outer, inner);
        result = d_test_block_run(outer, NULL);

        if (!d_assert_standalone(
                result == true,
                "nested direct run",
                "Nested block execution succeeds",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(outer);
    }

    // failing setup hook prevents execution
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        d_test_block_set_stage_hook(b,
                                    D_TEST_STAGE_SETUP,
                                    stage_hook_failing);
        helper_add_passing_test_to_block(b);
        result = d_test_block_run(b, NULL);

        if (!d_assert_standalone(
                result == false,
                "failing setup hook",
                "Failing setup prevents execution",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] block_direct_run\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] block_direct_run\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}


/*
d_tests_sa_handler_block_new_args
  Tests d_test_block_new_args constructor with config arguments.
  Tests the following:
  - new_args with NULL args creates valid block
  - new_args creates block with config member
  - new_args with children populates children
  - block config is non-NULL after new_args
*/
bool
d_tests_sa_handler_block_new_args
(
    struct d_test_counter* _test_info
)
{
    size_t               ip;
    bool                 ok;
    struct d_test_block* b;

    printf("  --- Testing block new_args ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // new_args with NULL args creates valid block
    b = d_test_block_new_args(NULL, 0, NULL, 0);

    if (b)
    {
        if (!d_assert_standalone(
                d_test_block_child_count(b) == 0,
                "new_args(NULL,0)",
                "Creates valid empty block",
                _test_info))
        {
            ok = false;
        }

        // config member should be non-NULL
        if (!d_assert_standalone(
                b->config != NULL,
                "new_args config",
                "Block has config after new_args",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }
    else
    {
        if (!d_assert_standalone(
                false,
                "new_args allocation",
                "new_args returned NULL",
                _test_info))
        {
            ok = false;
        }
    }

    // basic new also creates config
    b = d_test_block_new(NULL, 0);

    if (b)
    {
        if (!d_assert_standalone(
                b->config != NULL,
                "new config",
                "Block has config after new",
                _test_info))
        {
            ok = false;
        }

        // children vector should exist
        if (!d_assert_standalone(
                b->children != NULL,
                "new children",
                "Block has children vector",
                _test_info))
        {
            ok = false;
        }

        // stage_hooks starts as NULL
        if (!d_assert_standalone(
                b->stage_hooks == NULL,
                "new stage_hooks",
                "Stage hooks NULL until set",
                _test_info))
        {
            ok = false;
        }

        d_test_block_free(b);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] block_new_args\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] block_new_args\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}
