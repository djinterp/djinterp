/******************************************************************************
* djinterp [test]                                  test_handler_tests_sa_edge.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"

bool d_tests_sa_handler_null_parameters(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing NULL parameters ---\n");
    ip = _test_info->tests_passed; ok = true;

    { struct d_test_handler* tmp = d_test_handler_new(NULL);
      if (!d_assert_standalone(tmp != NULL, "new(NULL)", "succeeds", _test_info)) ok = false;
      if (tmp) d_test_handler_free(tmp); }

    d_test_handler_free(NULL);
    if (!d_assert_standalone(true, "free(NULL)", "safe", _test_info)) ok = false;

    if (!d_assert_standalone(d_test_handler_run_test(NULL, NULL, NULL) == false,
        "run_test(NULL,NULL)", "false", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_run_block(NULL, NULL, NULL) == false,
        "run_block(NULL,NULL)", "false", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_run_test_type(NULL, NULL, NULL) == false,
        "run_test_type(NULL,NULL)", "false", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_run_module(NULL, NULL, NULL) == false,
        "run_module(NULL,NULL)", "false", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_run_tree(NULL, NULL, NULL) == false,
        "run_tree(NULL,NULL)", "false", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_run_session(NULL, NULL) == false,
        "run_session(NULL,NULL)", "false", _test_info)) ok = false;

    d_test_handler_record_assertion(NULL, NULL);
    if (!d_assert_standalone(true, "record(NULL,NULL)", "safe", _test_info)) ok = false;

    { struct d_test_result r = d_test_handler_get_results(NULL);
      if (!d_assert_standalone(r.tests_run == 0, "get_results(NULL)", "zeroed", _test_info)) ok = false; }

    d_test_handler_reset_results(NULL);
    if (!d_assert_standalone(true, "reset(NULL)", "safe", _test_info)) ok = false;
    d_test_handler_print_results(NULL, "test");
    if (!d_assert_standalone(true, "print(NULL)", "safe", _test_info)) ok = false;

    if (!d_assert_standalone(d_test_handler_get_pass_rate(NULL) == 0.0,
        "pass_rate(NULL)", "0.0", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_get_assertion_rate(NULL) == 0.0,
        "assert_rate(NULL)", "0.0", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_register_listener(NULL, 0, callback_start, true) == false,
        "register(NULL)", "false", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_unregister_listener(NULL, 0) == false,
        "unregister(NULL)", "false", _test_info)) ok = false;
    if (!d_assert_standalone(d_test_handler_has_flag(NULL, 0) == false,
        "has_flag(NULL)", "false", _test_info)) ok = false;

    d_test_handler_set_flag(NULL, 0);
    d_test_handler_clear_flag(NULL, 0);
    d_test_handler_emit_event(NULL, 0, NULL);
    if (!d_assert_standalone(true, "set/clear/emit(NULL)", "all safe", _test_info)) ok = false;

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] null_parameters\n", D_INDENT); }
    else printf("%s[FAIL] null_parameters\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_empty_blocks(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing empty blocks ---\n");
    ip = _test_info->tests_passed; ok = true;

    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        struct d_test_block* b = d_test_block_new(NULL, 0);
        if (b) {
            bool r = d_test_handler_run_block(h, b, NULL);
            struct d_test_result res = d_test_handler_get_results(h);
            if (!d_assert_standalone(
                (r == true && res.blocks_run == 1 && res.tests_run == 0),
                "empty block", "succeeds, blocks=1, tests=0", _test_info)) ok = false;
            d_test_block_free(b); }

        // Nested empty blocks
        { struct d_test_block* o = d_test_block_new(NULL, 0);
          struct d_test_block* in = d_test_block_new(NULL, 0);
          if (o && in) {
            helper_add_block_child_to_block(o, in);
            d_test_handler_reset_results(h);
            d_test_handler_run_block(h, o, NULL);
            struct d_test_result r2 = d_test_handler_get_results(h);
            if (!d_assert_standalone(
                (r2.blocks_run == 2 && r2.max_depth == 2),
                "nested empty", "blocks=2, depth=2", _test_info)) ok = false;
            d_test_block_free(o); } }

        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] empty_blocks\n", D_INDENT); }
    else printf("%s[FAIL] empty_blocks\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}

bool d_tests_sa_handler_no_events(struct d_test_counter* _test_info)
{
    size_t ip; bool ok;
    printf("  --- Testing handler with no events ---\n");
    ip = _test_info->tests_passed; ok = true;

    { struct d_test_handler* h = d_test_handler_new(NULL);
      if (h) {
        if (!d_assert_standalone(h->event_handler == NULL,
            "no event_handler", "NULL", _test_info)) ok = false;
        { struct d_test* t = helper_make_passing_test();
          if (t) {
            if (!d_assert_standalone(d_test_handler_run_test(h, t, NULL) == true,
                "test without events", "Runs fine", _test_info)) ok = false;
            d_test_free(t); } }
        if (!d_assert_standalone(
            d_test_handler_register_listener(h, D_TEST_EVENT_START, callback_start, true) == false,
            "register fails", "Without events", _test_info)) ok = false;
        { struct d_test_context ctx;
          d_test_context_init(&ctx, h);
          d_test_handler_emit_event(h, D_TEST_EVENT_START, &ctx);
          if (!d_assert_standalone(true, "emit safe", "Without events", _test_info)) ok = false; }
        d_test_handler_free(h); } }

    if (ok) { _test_info->tests_passed++; printf("%s[PASS] no_events\n", D_INDENT); }
    else printf("%s[FAIL] no_events\n", D_INDENT);
    _test_info->tests_total++; return (_test_info->tests_passed > ip);
}
