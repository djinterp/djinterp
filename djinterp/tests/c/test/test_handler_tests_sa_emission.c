#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_emit_event
  Tests d_test_handler_emit_event behavior.
  Tests the following:
  - emit on NULL handler does not crash
  - emit on handler without event system does not crash
  - emit fires registered callback
  - all lifecycle events fire
  - multiple emissions accumulate
  - emit without EMIT_EVENTS flag does not fire
*/
bool
d_tests_sa_handler_emit_event
(
    struct d_test_counter* _test_info
)
{
    size_t                 initial_tests_passed;
    bool                   all_assertions_passed;
    bool                   result;
    struct d_test_handler* handler;
    struct d_test_context  ctx;
    int                    i;

    printf("  --- Testing d_test_handler_emit_event ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // emit on NULL handler does not crash
    d_test_handler_emit_event(NULL, D_TEST_EVENT_START, NULL);

    all_assertions_passed &= d_assert_standalone(
        true,
        "emit on NULL handler",
        "Emitting on NULL handler does not crash",
        _test_info);

    // emit on handler without events does not crash
    handler = d_test_handler_new(NULL);

    if (handler)
    {
        d_test_context_init(&ctx, handler);
        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_START,
                                  &ctx);

        all_assertions_passed &= d_assert_standalone(
            true,
            "emit without event system",
            "Emit without event system does not crash",
            _test_info);

        d_test_handler_free(handler);
    }

    // emit fires registered callback
    reset_event_counters();
    handler = d_test_handler_new_with_events(NULL, 10);

    if (handler)
    {
        d_test_context_init(&ctx, handler);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_SETUP,
                                         callback_setup,
                                         true);
        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_SETUP,
                                  &ctx);

        all_assertions_passed &= d_assert_standalone(
            g_event_setup_count == 1,
            "SETUP callback fires",
            "SETUP event fires callback",
            _test_info);

        d_test_handler_free(handler);
    }

    // all lifecycle events fire
    reset_event_counters();
    handler = d_test_handler_new_with_events(NULL, 20);

    if (handler)
    {
        d_test_context_init(&ctx, handler);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_SETUP,
                                         callback_setup,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_START,
                                         callback_start,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_SUCCESS,
                                         callback_success,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_END,
                                         callback_end,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_TEAR_DOWN,
                                         callback_teardown,
                                         true);

        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_SETUP,
                                  &ctx);
        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_START,
                                  &ctx);
        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_SUCCESS,
                                  &ctx);
        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_END,
                                  &ctx);
        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_TEAR_DOWN,
                                  &ctx);

        result = (g_event_setup_count == 1)
              && (g_event_start_count == 1)
              && (g_event_success_count == 1)
              && (g_event_end_count == 1)
              && (g_event_teardown_count == 1);

        all_assertions_passed &= d_assert_standalone(
            result == true,
            "all lifecycle events fire",
            "All lifecycle event callbacks invoked once",
            _test_info);

        d_test_handler_free(handler);
    }

    // multiple emissions accumulate
    reset_event_counters();
    handler = d_test_handler_new_with_events(NULL, 10);

    if (handler)
    {
        d_test_context_init(&ctx, handler);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_START,
                                         callback_start,
                                         true);

        for (i = 0; i < 5; i++)
        {
            d_test_handler_emit_event(handler,
                                      D_TEST_EVENT_START,
                                      &ctx);
        }

        all_assertions_passed &= d_assert_standalone(
            g_event_start_count == 5,
            "5 emissions accumulate",
            "5 emissions fire callback 5 times",
            _test_info);

        d_test_handler_free(handler);
    }

    // emit without EMIT_EVENTS flag does not fire
    reset_event_counters();
    handler = d_test_handler_new_full(NULL,
                                      10,
                                      0,
                                      D_TEST_HANDLER_FLAG_NONE);

    if ( (handler) &&
         (handler->event_handler) )
    {
        d_test_context_init(&ctx, handler);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_START,
                                         callback_start,
                                         true);
        d_test_handler_emit_event(handler,
                                  D_TEST_EVENT_START,
                                  &ctx);

        all_assertions_passed &= d_assert_standalone(
            g_event_start_count == 0,
            "no fire without EMIT_EVENTS flag",
            "Event not fired when EMIT_EVENTS flag is not set",
            _test_info);
    }

    if (handler)
    {
        d_test_handler_free(handler);
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] emit_event\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] emit_event\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_handler_event_lifecycle
  Tests event lifecycle during test execution.
  Tests the following:
  - passing test fires SETUP,START,SUCCESS,END,TEARDOWN (not FAILURE)
  - failing test fires FAILURE instead of SUCCESS
  - multiple tests accumulate events
*/
bool
d_tests_sa_handler_event_lifecycle
(
    struct d_test_counter* _test_info
)
{
    size_t                 initial_tests_passed;
    bool                   all_assertions_passed;
    bool                   result;
    struct d_test_handler* handler;
    struct d_test*         test;
    int                    i;

    printf("  --- Testing event lifecycle ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // passing test fires SETUP,START,SUCCESS,END,TEARDOWN (not FAILURE)
    reset_event_counters();
    handler = d_test_handler_new_with_events(NULL, 20);

    if (handler)
    {
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_SETUP,
                                         callback_setup,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_START,
                                         callback_start,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_SUCCESS,
                                         callback_success,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_FAILURE,
                                         callback_failure,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_END,
                                         callback_end,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_TEAR_DOWN,
                                         callback_teardown,
                                         true);

        test = d_test_new(handler_test_passing, NULL);

        if (test)
        {
            d_test_handler_run_test(handler, test, NULL);

            result = (g_event_setup_count == 1)
                  && (g_event_start_count == 1)
                  && (g_event_success_count == 1)
                  && (g_event_failure_count == 0)
                  && (g_event_end_count == 1)
                  && (g_event_teardown_count == 1);

            all_assertions_passed &= d_assert_standalone(
                result == true,
                "passing test lifecycle events",
                "Passing test fires SETUP,START,SUCCESS,END,TEARDOWN",
                _test_info);

            d_test_free(test);
        }

        d_test_handler_free(handler);
    }

    // failing test fires FAILURE instead of SUCCESS
    reset_event_counters();
    handler = d_test_handler_new_with_events(NULL, 20);

    if (handler)
    {
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_SUCCESS,
                                         callback_success,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_FAILURE,
                                         callback_failure,
                                         true);

        test = d_test_new(handler_test_failing, NULL);

        if (test)
        {
            d_test_handler_run_test(handler, test, NULL);

            result = (g_event_success_count == 0)
                  && (g_event_failure_count == 1);

            all_assertions_passed &= d_assert_standalone(
                result == true,
                "failing test fires FAILURE",
                "Failing test fires FAILURE not SUCCESS",
                _test_info);

            d_test_free(test);
        }

        d_test_handler_free(handler);
    }

    // multiple tests accumulate events
    reset_event_counters();
    handler = d_test_handler_new_with_events(NULL, 20);

    if (handler)
    {
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_START,
                                         callback_start,
                                         true);
        d_test_handler_register_listener(handler,
                                         D_TEST_EVENT_END,
                                         callback_end,
                                         true);

        for (i = 0; i < 3; i++)
        {
            test = d_test_new(handler_test_passing, NULL);

            if (test)
            {
                d_test_handler_run_test(handler, test, NULL);
                d_test_free(test);
            }
        }

        result = (g_event_start_count == 3)
              && (g_event_end_count == 3);

        all_assertions_passed &= d_assert_standalone(
            result == true,
            "3 tests fire 3 START and 3 END",
            "Events accumulate across test runs",
            _test_info);

        d_test_handler_free(handler);
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] event_lifecycle\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] event_lifecycle\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
