#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_null_parameters
  Tests NULL parameter handling across all handler API functions.
  Tests the following:
  - new(NULL) succeeds
  - free(NULL) is safe
  - run_test/block/test_type/module/tree/session(NULL) return false
  - record_assertion(NULL) is safe
  - get_results(NULL) returns zeroed struct
  - reset_results(NULL) is safe
  - print_results(NULL) is safe
  - pass_rate(NULL) returns 0.0
  - assertion_rate(NULL) returns 0.0
  - register/unregister/has_flag(NULL) return false
  - set_flag/clear_flag/emit_event(NULL) are safe
*/
bool
d_tests_sa_handler_null_parameters
(
    struct d_test_counter* _test_info
)
{
    size_t                 initial_tests_passed;
    bool                   all_assertions_passed;
    bool                   result;
    struct d_test_handler* handler;
    struct d_test_result   res;

    printf("  --- Testing NULL parameters ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // new(NULL) succeeds
    result = (d_test_handler_new(NULL) != NULL);

    all_assertions_passed &= d_assert_standalone(
        result == true,
        "new(NULL)",
        "new(NULL) succeeds",
        _test_info);

    if (result)
    {
        handler = d_test_handler_new(NULL);
        d_test_handler_free(handler);
    }

    // free(NULL) is safe
    d_test_handler_free(NULL);

    all_assertions_passed &= d_assert_standalone(
        true,
        "free(NULL)",
        "free(NULL) safe",
        _test_info);

    // run functions return false on NULL
    result = d_test_handler_run_test(NULL, NULL, NULL);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "run_test(NULL,NULL)",
        "returns false",
        _test_info);

    result = d_test_handler_run_block(NULL, NULL, NULL);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "run_block(NULL,NULL)",
        "returns false",
        _test_info);

    result = d_test_handler_run_test_type(NULL, NULL, NULL);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "run_test_type(NULL,NULL)",
        "returns false",
        _test_info);

    result = d_test_handler_run_module(NULL, NULL, NULL);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "run_module(NULL,NULL)",
        "returns false",
        _test_info);

    result = d_test_handler_run_tree(NULL, NULL, NULL);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "run_tree(NULL,NULL)",
        "returns false",
        _test_info);

    result = d_test_handler_run_session(NULL, NULL);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "run_session(NULL,NULL)",
        "returns false",
        _test_info);

    // record_assertion(NULL) is safe
    d_test_handler_record_assertion(NULL, NULL);

    all_assertions_passed &= d_assert_standalone(
        true,
        "record_assertion(NULL,NULL)",
        "safe",
        _test_info);

    // get_results(NULL) returns zeroed struct
    res = d_test_handler_get_results(NULL);

    all_assertions_passed &= d_assert_standalone(
        res.tests_run == 0,
        "get_results(NULL)",
        "returns zeroed",
        _test_info);

    // reset_results(NULL) is safe
    d_test_handler_reset_results(NULL);

    all_assertions_passed &= d_assert_standalone(
        true,
        "reset_results(NULL)",
        "safe",
        _test_info);

    // print_results(NULL) is safe
    d_test_handler_print_results(NULL, "test");

    all_assertions_passed &= d_assert_standalone(
        true,
        "print_results(NULL)",
        "safe",
        _test_info);

    // rate functions return 0.0 on NULL
    all_assertions_passed &= d_assert_standalone(
        d_test_handler_get_pass_rate(NULL) == 0.0,
        "pass_rate(NULL)",
        "0.0",
        _test_info);

    all_assertions_passed &= d_assert_standalone(
        d_test_handler_get_assertion_rate(NULL) == 0.0,
        "assert_rate(NULL)",
        "0.0",
        _test_info);

    // register/unregister/has_flag return false on NULL
    result = d_test_handler_register_listener(NULL,
                 0,
                 callback_start,
                 true);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "register(NULL)",
        "false",
        _test_info);

    result = d_test_handler_unregister_listener(NULL, 0);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "unregister(NULL)",
        "false",
        _test_info);

    result = d_test_handler_has_flag(NULL, 0);

    all_assertions_passed &= d_assert_standalone(
        result == false,
        "has_flag(NULL)",
        "false",
        _test_info);

    // set_flag/clear_flag/emit_event on NULL are safe
    d_test_handler_set_flag(NULL, 0);
    d_test_handler_clear_flag(NULL, 0);
    d_test_handler_emit_event(NULL, 0, NULL);

    all_assertions_passed &= d_assert_standalone(
        true,
        "set/clear/emit(NULL)",
        "all safe",
        _test_info);

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] null_parameters\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] null_parameters\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_empty_blocks
  Tests handler behavior with empty test blocks.
  Tests the following:
  - empty block succeeds with blocks_run=1, tests_run=0
  - nested empty blocks report blocks=2, depth=2
*/
bool
d_tests_sa_handler_empty_blocks
(
    struct d_test_counter* _test_info
)
{
    size_t                 initial_tests_passed;
    bool                   all_assertions_passed;
    bool                   result;
    struct d_test_handler* handler;
    struct d_test_block*   block;
    struct d_test_block*   outer;
    struct d_test_block*   inner;
    struct d_test_result   res;

    printf("  --- Testing empty blocks ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    handler = d_test_handler_new(NULL);

    if (handler)
    {
        // single empty block
        block = d_test_block_new(NULL, 0);

        if (block)
        {
            result = d_test_handler_run_block(handler, block, NULL);
            res    = d_test_handler_get_results(handler);

            all_assertions_passed &= d_assert_standalone(
                ( (result == true)      &&
                  (res.blocks_run == 1) &&
                  (res.tests_run == 0) ),
                "empty block",
                "Empty block: succeeds, blocks=1, tests=0",
                _test_info);

            d_test_block_free(block);
        }

        // nested empty blocks: use add_block to parent the inner
        outer = d_test_block_new(NULL, 0);
        inner = d_test_block_new(NULL, 0);

        if ( (outer) &&
             (inner) )
        {
            // wraps inner in d_test_type and adds via ptr_vector
            helper_add_block_child_to_block(outer, inner);

            d_test_handler_reset_results(handler);
            d_test_handler_run_block(handler, outer, NULL);
            res = d_test_handler_get_results(handler);

            all_assertions_passed &= d_assert_standalone(
                ( (res.blocks_run == 2) &&
                  (res.max_depth == 2) ),
                "nested empty blocks",
                "Nested empty: blocks=2, depth=2",
                _test_info);

            // outer owns inner through the d_test_type wrapper
            d_test_block_free(outer);
        }

        d_test_handler_free(handler);
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] empty_blocks\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] empty_blocks\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_no_events
  Tests handler behavior when created without an event system.
  Tests the following:
  - event_handler field is NULL
  - tests still run without event system
  - register_listener fails without event system
  - emit_event is safe without event system
*/
bool
d_tests_sa_handler_no_events
(
    struct d_test_counter* _test_info
)
{
    size_t                 initial_tests_passed;
    bool                   all_assertions_passed;
    bool                   result;
    struct d_test_handler* handler;
    struct d_test*         test;
    struct d_test_context  ctx;

    printf("  --- Testing handler with no events ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    handler = d_test_handler_new(NULL);

    if (handler)
    {
        // event_handler should be NULL
        all_assertions_passed &= d_assert_standalone(
            handler->event_handler == NULL,
            "no event_handler",
            "event_handler is NULL",
            _test_info);

        // tests still work without events
        test = d_test_new(handler_test_passing, NULL);

        if (test)
        {
            result = d_test_handler_run_test(handler, test, NULL);

            all_assertions_passed &= d_assert_standalone(
                result == true,
                "test without events",
                "Test runs without event system",
                _test_info);

            d_test_free(test);
        }

        // register fails without event system
        result = d_test_handler_register_listener(
                     handler,
                     D_TEST_EVENT_START,
                     callback_start,
                     true);

        all_assertions_passed &= d_assert_standalone(
            result == false,
            "register fails",
            "Register fails without events",
            _test_info);

        // emit is safe without event system
        d_test_context_init(&ctx, handler);
        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_START,
                                  &ctx);

        all_assertions_passed &= d_assert_standalone(
            true,
            "emit safe",
            "Emit safe without events",
            _test_info);

        d_test_handler_free(handler);
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] no_events\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] no_events\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
