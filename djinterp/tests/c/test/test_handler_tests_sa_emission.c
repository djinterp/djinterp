/******************************************************************************
* djinterp [test]                               test_handler_tests_sa_emission.c
*
*   Unit tests for test_handler event emission:
*     d_test_handler_emit_event
*
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"

bool d_tests_sa_handler_emit_event(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;
    printf("  --- Testing d_test_handler_emit_event ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Emit on NULL handler does not crash
    d_test_handler_emit_event(NULL, D_TEST_EVENT_START, NULL);
    if (!d_assert_standalone(true, "emit on NULL handler",
        "Emitting on NULL handler does not crash", _test_info))
        all_assertions_passed = false;

    // Test 2: Emit on handler without events does not crash
    {
        struct d_test_handler* h = d_test_handler_new(NULL);
        if (h) {
            struct d_test_context ctx;
            d_test_context_init(&ctx, h);
            d_test_handler_emit_event(h, D_TEST_EVENT_START, &ctx);
            if (!d_assert_standalone(true, "emit without event system",
                "Emit without event system does not crash", _test_info))
                all_assertions_passed = false;
            d_test_handler_free(h);
        }
    }

    // Test 3: Emit fires registered callback
    {
        reset_event_counters();
        struct d_test_handler* h = d_test_handler_new_with_events(NULL, 10);
        if (h) {
            struct d_test_context ctx;
            d_test_context_init(&ctx, h);
            d_test_handler_register_listener(h, D_TEST_EVENT_SETUP,
                callback_setup, true);
            d_test_handler_emit_event(h, D_TEST_EVENT_SETUP, &ctx);
            if (!d_assert_standalone(g_event_setup_count == 1,
                "SETUP callback fires", "SETUP event fires callback", _test_info))
                all_assertions_passed = false;
            d_test_handler_free(h);
        }
    }

    // Test 4: All lifecycle events fire
    {
        reset_event_counters();
        struct d_test_handler* h = d_test_handler_new_with_events(NULL, 20);
        if (h) {
            struct d_test_context ctx;
            d_test_context_init(&ctx, h);
            d_test_handler_register_listener(h, D_TEST_EVENT_SETUP, callback_setup, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_START, callback_start, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_SUCCESS, callback_success, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_END, callback_end, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);

            d_test_handler_emit_event(h, D_TEST_EVENT_SETUP, &ctx);
            d_test_handler_emit_event(h, D_TEST_EVENT_START, &ctx);
            d_test_handler_emit_event(h, D_TEST_EVENT_SUCCESS, &ctx);
            d_test_handler_emit_event(h, D_TEST_EVENT_END, &ctx);
            d_test_handler_emit_event(h, D_TEST_EVENT_TEAR_DOWN, &ctx);

            if (!d_assert_standalone(
                (g_event_setup_count == 1 && g_event_start_count == 1 &&
                 g_event_success_count == 1 && g_event_end_count == 1 &&
                 g_event_teardown_count == 1),
                "all lifecycle events fire",
                "All lifecycle event callbacks invoked once", _test_info))
                all_assertions_passed = false;
            d_test_handler_free(h);
        }
    }

    // Test 5: Multiple emissions accumulate
    {
        reset_event_counters();
        struct d_test_handler* h = d_test_handler_new_with_events(NULL, 10);
        if (h) {
            struct d_test_context ctx;
            int i;
            d_test_context_init(&ctx, h);
            d_test_handler_register_listener(h, D_TEST_EVENT_START, callback_start, true);
            for (i = 0; i < 5; i++)
                d_test_handler_emit_event(h, D_TEST_EVENT_START, &ctx);
            if (!d_assert_standalone(g_event_start_count == 5,
                "5 emissions accumulate", "5 emissions fire callback 5 times", _test_info))
                all_assertions_passed = false;
            d_test_handler_free(h);
        }
    }

    // Test 6: Emit without EMIT_EVENTS flag does not fire
    {
        reset_event_counters();
        struct d_test_handler* h = d_test_handler_new_full(NULL, 10, 0, D_TEST_HANDLER_FLAG_NONE);
        if (h && h->event_handler) {
            struct d_test_context ctx;
            d_test_context_init(&ctx, h);
            d_test_handler_register_listener(h, D_TEST_EVENT_START, callback_start, true);
            d_test_handler_emit_event(h, D_TEST_EVENT_START, &ctx);
            if (!d_assert_standalone(g_event_start_count == 0,
                "no fire without EMIT_EVENTS flag",
                "Event not fired when EMIT_EVENTS flag is not set", _test_info))
                all_assertions_passed = false;
        }
        if (h) d_test_handler_free(h);
    }

    if (all_assertions_passed) { _test_info->tests_passed++; printf("%s[PASS] emit_event\n", D_INDENT); }
    else printf("%s[FAIL] emit_event\n", D_INDENT);
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

bool d_tests_sa_handler_event_lifecycle(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;
    printf("  --- Testing event lifecycle ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Passing test fires SETUP,START,SUCCESS,END,TEARDOWN (not FAILURE)
    {
        reset_event_counters();
        struct d_test_handler* h = d_test_handler_new_with_events(NULL, 20);
        if (h) {
            d_test_handler_register_listener(h, D_TEST_EVENT_SETUP, callback_setup, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_START, callback_start, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_SUCCESS, callback_success, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_FAILURE, callback_failure, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_END, callback_end, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);

            struct d_test* test = helper_make_passing_test();
            if (test) {
                d_test_handler_run_test(h, test, NULL);
                if (!d_assert_standalone(
                    (g_event_setup_count == 1 && g_event_start_count == 1 &&
                     g_event_success_count == 1 && g_event_failure_count == 0 &&
                     g_event_end_count == 1 && g_event_teardown_count == 1),
                    "passing test lifecycle events",
                    "Passing test fires SETUP,START,SUCCESS,END,TEARDOWN", _test_info))
                    all_assertions_passed = false;
                d_test_free(test);
            }
            d_test_handler_free(h);
        }
    }

    // Test 2: Failing test fires FAILURE instead of SUCCESS
    {
        reset_event_counters();
        struct d_test_handler* h = d_test_handler_new_with_events(NULL, 20);
        if (h) {
            d_test_handler_register_listener(h, D_TEST_EVENT_SUCCESS, callback_success, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_FAILURE, callback_failure, true);
            struct d_test* test = helper_make_failing_test();
            if (test) {
                d_test_handler_run_test(h, test, NULL);
                if (!d_assert_standalone(
                    (g_event_success_count == 0 && g_event_failure_count == 1),
                    "failing test fires FAILURE",
                    "Failing test fires FAILURE not SUCCESS", _test_info))
                    all_assertions_passed = false;
                d_test_free(test);
            }
            d_test_handler_free(h);
        }
    }

    // Test 3: Multiple tests accumulate events
    {
        int i;
        reset_event_counters();
        struct d_test_handler* h = d_test_handler_new_with_events(NULL, 20);
        if (h) {
            d_test_handler_register_listener(h, D_TEST_EVENT_START, callback_start, true);
            d_test_handler_register_listener(h, D_TEST_EVENT_END, callback_end, true);
            for (i = 0; i < 3; i++) {
                struct d_test* t = helper_make_passing_test();
                if (t) { d_test_handler_run_test(h, t, NULL); d_test_free(t); }
            }
            if (!d_assert_standalone(
                (g_event_start_count == 3 && g_event_end_count == 3),
                "3 tests fire 3 START and 3 END",
                "Events accumulate across test runs", _test_info))
                all_assertions_passed = false;
            d_test_handler_free(h);
        }
    }

    if (all_assertions_passed) { _test_info->tests_passed++; printf("%s[PASS] event_lifecycle\n", D_INDENT); }
    else printf("%s[FAIL] event_lifecycle\n", D_INDENT);
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}
