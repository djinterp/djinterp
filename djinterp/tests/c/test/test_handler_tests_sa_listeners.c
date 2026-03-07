/******************************************************************************
* djinterp [test]                              test_handler_tests_sa_listeners.c
*
*   Unit tests for test_handler event listener registration:
*     d_test_handler_register_listener, d_test_handler_unregister_listener,
*     d_test_handler_enable_listener, d_test_handler_disable_listener
*
*
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/

#include "./test_handler_tests_sa.h"


bool
d_tests_sa_handler_register_listener
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_register_listener ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Register on NULL handler fails
    if (!d_assert_standalone(
        d_test_handler_register_listener(NULL,
            D_TEST_EVENT_START, callback_start, true) == false,
        "register on NULL handler",
        "Returns false for NULL handler",
        _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Register on handler without events fails
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);
        if (handler)
        {
            if (!d_assert_standalone(
                d_test_handler_register_listener(handler,
                    D_TEST_EVENT_START, callback_start, true) == false,
                "register without event system",
                "Returns false on handler without events",
                _test_info))
            {
                all_assertions_passed = false;
            }
            d_test_handler_free(handler);
        }
    }

    // Test 3: Register NULL callback fails
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 10);
        if (handler)
        {
            if (!d_assert_standalone(
                d_test_handler_register_listener(handler,
                    D_TEST_EVENT_START, NULL, true) == false,
                "register NULL callback",
                "Returns false for NULL callback",
                _test_info))
            {
                all_assertions_passed = false;
            }
            d_test_handler_free(handler);
        }
    }

    // Test 4: Register valid listener succeeds
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 10);
        if (handler)
        {
            if (!d_assert_standalone(
                d_test_handler_register_listener(handler,
                    D_TEST_EVENT_START, callback_start, true) == true,
                "register valid listener",
                "Valid listener registration succeeds",
                _test_info))
            {
                all_assertions_passed = false;
            }
            d_test_handler_free(handler);
        }
    }

    // Test 5: Register listeners for all event types
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 20);
        if (handler)
        {
            bool all_ok;

            all_ok = d_test_handler_register_listener(handler,
                         D_TEST_EVENT_SETUP, callback_setup, true)
                  && d_test_handler_register_listener(handler,
                         D_TEST_EVENT_START, callback_start, true)
                  && d_test_handler_register_listener(handler,
                         D_TEST_EVENT_SUCCESS, callback_success, true)
                  && d_test_handler_register_listener(handler,
                         D_TEST_EVENT_FAILURE, callback_failure, true)
                  && d_test_handler_register_listener(handler,
                         D_TEST_EVENT_END, callback_end, true)
                  && d_test_handler_register_listener(handler,
                         D_TEST_EVENT_TEAR_DOWN, callback_teardown, true);

            if (!d_assert_standalone(all_ok,
                "register all event types",
                "All event type registrations succeed",
                _test_info))
            {
                all_assertions_passed = false;
            }
            d_test_handler_free(handler);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] register_listener\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] register_listener\n", D_INDENT);
    }
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}


bool
d_tests_sa_handler_unregister_listener
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_unregister_listener ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Unregister from NULL handler
    if (!d_assert_standalone(
        d_test_handler_unregister_listener(NULL,
            D_TEST_EVENT_START) == false,
        "unregister from NULL",
        "Returns false for NULL handler",
        _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Unregister from handler without events
    {
        struct d_test_handler* handler;
        handler = d_test_handler_new(NULL);
        if (handler)
        {
            if (!d_assert_standalone(
                d_test_handler_unregister_listener(handler,
                    D_TEST_EVENT_START) == false,
                "unregister without events",
                "Returns false without event system",
                _test_info))
            {
                all_assertions_passed = false;
            }
            d_test_handler_free(handler);
        }
    }

    // Test 3: Register then unregister
    {
        struct d_test_handler* handler;
        handler = d_test_handler_new_with_events(NULL, 10);
        if (handler)
        {
            d_test_handler_register_listener(handler,
                D_TEST_EVENT_START, callback_start, true);
            bool result = d_test_handler_unregister_listener(handler,
                              D_TEST_EVENT_START);

            if (!d_assert_standalone(result == true,
                "register then unregister",
                "Unregister after register succeeds",
                _test_info))
            {
                all_assertions_passed = false;
            }
            d_test_handler_free(handler);
        }
    }

    // Test 4: Non-existent listener does not crash
    {
        struct d_test_handler* handler;
        handler = d_test_handler_new_with_events(NULL, 10);
        if (handler)
        {
            d_test_handler_unregister_listener(handler,
                D_TEST_EVENT_START);
            if (!d_assert_standalone(true,
                "unregister non-existent",
                "Unregistering non-existent listener is safe",
                _test_info))
            {
                all_assertions_passed = false;
            }
            d_test_handler_free(handler);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] unregister_listener\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] unregister_listener\n", D_INDENT);
    }
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}


bool
d_tests_sa_handler_listener_enable_disable
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing listener enable/disable ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // Test 1: Enable on NULL handler returns false
    if (!d_assert_standalone(
        d_test_handler_enable_listener(NULL, D_TEST_EVENT_START) == false,
        "enable on NULL handler",
        "Enable returns false for NULL handler",
        _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Disable on NULL handler returns false
    if (!d_assert_standalone(
        d_test_handler_disable_listener(NULL, D_TEST_EVENT_START) == false,
        "disable on NULL handler",
        "Disable returns false for NULL handler",
        _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 3: Enable/disable on handler without events
    {
        struct d_test_handler* handler;
        handler = d_test_handler_new(NULL);
        if (handler)
        {
            if (!d_assert_standalone(
                (d_test_handler_enable_listener(handler,
                     D_TEST_EVENT_START) == false &&
                 d_test_handler_disable_listener(handler,
                     D_TEST_EVENT_START) == false),
                "enable/disable without events",
                "Both return false without event system",
                _test_info))
            {
                all_assertions_passed = false;
            }
            d_test_handler_free(handler);
        }
    }

    // Test 4: Enabled listener fires, disabled does not
    {
        struct d_test_handler* handler;
        handler = d_test_handler_new_with_events(NULL, 10);
        if (handler)
        {
            struct d_test* test;

            d_test_handler_register_listener(handler,
                D_TEST_EVENT_START, callback_start, true);

            // Disable it
            d_test_handler_disable_listener(handler, D_TEST_EVENT_START);

            reset_event_counters();
            test = helper_make_passing_test();
            if (test)
            {
                d_test_handler_run_test(handler, test, NULL);
                if (!d_assert_standalone(g_event_start_count == 0,
                    "disabled listener does not fire",
                    "Disabled listener callback not invoked",
                    _test_info))
                {
                    all_assertions_passed = false;
                }

                // Re-enable it
                d_test_handler_enable_listener(handler,
                    D_TEST_EVENT_START);
                reset_event_counters();
                d_test_handler_run_test(handler, test, NULL);

                if (!d_assert_standalone(g_event_start_count > 0,
                    "re-enabled listener fires",
                    "Re-enabled listener callback invoked",
                    _test_info))
                {
                    all_assertions_passed = false;
                }

                d_test_free(test);
            }
            d_test_handler_free(handler);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] listener enable/disable\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] listener enable/disable\n", D_INDENT);
    }
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}
