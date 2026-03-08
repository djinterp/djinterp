/******************************************************************************
* djinterp [test]                                  test_handler_tests_sa_edge.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_null_parameters
  Tests all public API functions with NULL parameters for safety.
  Tests the following:
  - new(NULL) succeeds
  - free(NULL) is safe
  - run_test, run_block, run_test_type, run_module with NULL
  - run_tree, run_session with NULL
  - record_assertion, get_results, reset, print with NULL
  - pass_rate, assertion_rate with NULL
  - register/unregister listener with NULL
  - set/clear/has flag with NULL
  - emit_event with NULL
*/
bool
d_tests_sa_handler_null_parameters
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   ok;
    struct d_test_handler* tmp;
    struct d_test_result   r;

    printf("  --- Testing NULL parameters ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // new(NULL) succeeds
    tmp = d_test_handler_new(NULL);

    if (!d_assert_standalone(
            tmp != NULL,
            "new(NULL)",
            "succeeds",
            _test_info))
    {
        ok = false;
    }

    if (tmp)
    {
        d_test_handler_free(tmp);
    }

    // free(NULL) is safe
    d_test_handler_free(NULL);

    if (!d_assert_standalone(
            true,
            "free(NULL)",
            "safe",
            _test_info))
    {
        ok = false;
    }

    // run functions with NULL
    if (!d_assert_standalone(
            d_test_handler_run_test(NULL, NULL, NULL) == false,
            "run_test(NULL,NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    if (!d_assert_standalone(
            d_test_handler_run_block(NULL, NULL, NULL) == false,
            "run_block(NULL,NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    if (!d_assert_standalone(
            d_test_handler_run_test_type(NULL, NULL, NULL) == false,
            "run_test_type(NULL,NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    if (!d_assert_standalone(
            d_test_handler_run_module(NULL, NULL, NULL) == false,
            "run_module(NULL,NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    if (!d_assert_standalone(
            d_test_handler_run_tree(NULL, NULL, NULL) == false,
            "run_tree(NULL,NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    if (!d_assert_standalone(
            d_test_handler_run_session(NULL, NULL) == false,
            "run_session(NULL,NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    // record assertion with NULL is safe
    d_test_handler_record_assertion(NULL, NULL);

    if (!d_assert_standalone(
            true,
            "record(NULL,NULL)",
            "safe",
            _test_info))
    {
        ok = false;
    }

    // get_results(NULL) returns zeroed
    r = d_test_handler_get_results(NULL);

    if (!d_assert_standalone(
            r.tests_run == 0,
            "get_results(NULL)",
            "zeroed",
            _test_info))
    {
        ok = false;
    }

    // reset and print with NULL are safe
    d_test_handler_reset_results(NULL);

    if (!d_assert_standalone(
            true,
            "reset(NULL)",
            "safe",
            _test_info))
    {
        ok = false;
    }

    d_test_handler_print_results(NULL, "test");

    if (!d_assert_standalone(
            true,
            "print(NULL)",
            "safe",
            _test_info))
    {
        ok = false;
    }

    // rate functions with NULL return 0.0
    if (!d_assert_standalone(
            d_test_handler_get_pass_rate(NULL) == 0.0,
            "pass_rate(NULL)",
            "0.0",
            _test_info))
    {
        ok = false;
    }

    if (!d_assert_standalone(
            d_test_handler_get_assertion_rate(NULL) == 0.0,
            "assert_rate(NULL)",
            "0.0",
            _test_info))
    {
        ok = false;
    }

    // listener registration with NULL
    if (!d_assert_standalone(
            d_test_handler_register_listener(NULL,
                                             0,
                                             callback_start,
                                             true) == false,
            "register(NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    if (!d_assert_standalone(
            d_test_handler_unregister_listener(NULL, 0) == false,
            "unregister(NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    // flag operations with NULL
    if (!d_assert_standalone(
            d_test_handler_has_flag(NULL, 0) == false,
            "has_flag(NULL)",
            "false",
            _test_info))
    {
        ok = false;
    }

    // set/clear/emit with NULL are safe (no-ops)
    d_test_handler_set_flag(NULL, 0);
    d_test_handler_clear_flag(NULL, 0);
    d_test_handler_emit_event(NULL, 0, NULL);

    if (!d_assert_standalone(
            true,
            "set/clear/emit(NULL)",
            "all safe",
            _test_info))
    {
        ok = false;
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] null_parameters\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] null_parameters\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}


/*
d_tests_sa_handler_empty_blocks
  Tests handler behavior with empty test blocks.
  Tests the following:
  - empty block succeeds with blocks_run=1, tests_run=0
  - nested empty blocks: blocks=2, depth=2
*/
bool
d_tests_sa_handler_empty_blocks
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   ok;
    struct d_test_handler* h;
    struct d_test_block*   b;
    struct d_test_block*   outer;
    struct d_test_block*   inner;
    struct d_test_result   res;
    bool                   r;

    printf("  --- Testing empty blocks ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    h = d_test_handler_new(NULL);

    if (h)
    {
        // single empty block
        b = d_test_block_new(NULL, 0);

        if (b)
        {
            r   = d_test_handler_run_block(h, b, NULL);
            res = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r == true)           &&
                      (res.blocks_run == 1) &&
                      (res.tests_run == 0) ),
                    "empty block",
                    "succeeds, blocks=1, tests=0",
                    _test_info))
            {
                ok = false;
            }

            d_test_block_free(b);
        }

        // nested empty blocks
        outer = d_test_block_new(NULL, 0);
        inner = d_test_block_new(NULL, 0);

        if (outer && inner)
        {
            helper_add_block_child_to_block(outer, inner);
            d_test_handler_reset_results(h);
            d_test_handler_run_block(h, outer, NULL);
            res = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (res.blocks_run == 2) &&
                      (res.max_depth == 2) ),
                    "nested empty",
                    "blocks=2, depth=2",
                    _test_info))
            {
                ok = false;
            }

            d_test_block_free(outer);
        }

        d_test_handler_free(h);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] empty_blocks\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] empty_blocks\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}


/*
d_tests_sa_handler_no_events
  Tests handler behavior without event system.
  Tests the following:
  - handler created without events has NULL event_handler
  - test runs fine without events
  - register_listener fails without events
  - emit_event is safe without events
*/
bool
d_tests_sa_handler_no_events
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   ok;
    struct d_test_handler* h;
    struct d_test*         t;
    struct d_test_context  ctx;

    printf("  --- Testing handler with no events ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    h = d_test_handler_new(NULL);

    if (h)
    {
        // no event_handler
        if (!d_assert_standalone(
                h->event_handler == NULL,
                "no event_handler",
                "NULL",
                _test_info))
        {
            ok = false;
        }

        // test runs fine without events
        t = helper_make_passing_test();

        if (t)
        {
            if (!d_assert_standalone(
                    d_test_handler_run_test(h, t, NULL) == true,
                    "test without events",
                    "Runs fine",
                    _test_info))
            {
                ok = false;
            }

            d_test_free(t);
        }

        // register_listener fails without events
        if (!d_assert_standalone(
                d_test_handler_register_listener(
                    h,
                    D_TEST_EVENT_START,
                    callback_start,
                    true) == false,
                "register fails",
                "Without events",
                _test_info))
        {
            ok = false;
        }

        // emit_event is safe without events
        d_test_context_init(&ctx, h);
        d_test_handler_emit_event(h, D_TEST_EVENT_START, &ctx);

        if (!d_assert_standalone(
                true,
                "emit safe",
                "Without events",
                _test_info))
        {
            ok = false;
        }

        d_test_handler_free(h);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] no_events\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] no_events\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}
