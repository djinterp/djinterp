#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_register_listener
  Tests d_test_handler_register_listener behavior.
  Tests the following:
  - register on NULL handler fails
  - register on handler without event system fails
  - register with NULL callback fails
  - register valid listener succeeds
  - register listeners for all event types succeeds
*/
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

    // register on NULL handler fails
    if (!d_assert_standalone(
            d_test_handler_register_listener(NULL,
                                             D_TEST_EVENT_START,
                                             callback_start,
                                             true) == false,
            "register on NULL handler",
            "Returns false for NULL handler",
            _test_info))
    {
        all_assertions_passed = false;
    }

    // register on handler without event system fails
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            if (!d_assert_standalone(
                    d_test_handler_register_listener(
                        handler,
                        D_TEST_EVENT_START,
                        callback_start,
                        true) == false,
                    "register without event system",
                    "Returns false on handler without events",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // register NULL callback fails
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 10);

        if (handler)
        {
            if (!d_assert_standalone(
                    d_test_handler_register_listener(
                        handler,
                        D_TEST_EVENT_START,
                        NULL,
                        true) == false,
                    "register NULL callback",
                    "Returns false for NULL callback",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // register valid listener succeeds
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 10);

        if (handler)
        {
            if (!d_assert_standalone(
                    d_test_handler_register_listener(
                        handler,
                        D_TEST_EVENT_START,
                        callback_start,
                        true) == true,
                    "register valid listener",
                    "Valid listener registration succeeds",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // register listeners for all event types
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 20);

        if (handler)
        {
            bool all_ok;

            all_ok = d_test_handler_register_listener(
                         handler,
                         D_TEST_EVENT_SETUP,
                         callback_setup,
                         true)
                  && d_test_handler_register_listener(
                         handler,
                         D_TEST_EVENT_START,
                         callback_start,
                         true)
                  && d_test_handler_register_listener(
                         handler,
                         D_TEST_EVENT_SUCCESS,
                         callback_success,
                         true)
                  && d_test_handler_register_listener(
                         handler,
                         D_TEST_EVENT_FAILURE,
                         callback_failure,
                         true)
                  && d_test_handler_register_listener(
                         handler,
                         D_TEST_EVENT_END,
                         callback_end,
                         true)
                  && d_test_handler_register_listener(
                         handler,
                         D_TEST_EVENT_TEAR_DOWN,
                         callback_teardown,
                         true);

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


/*
d_tests_sa_handler_unregister_listener
  Tests d_test_handler_unregister_listener behavior.
  Tests the following:
  - unregister from NULL handler returns false
  - unregister from handler without events returns false
  - register then unregister succeeds
  - unregister non-existent listener is safe
*/
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

    // unregister from NULL handler
    if (!d_assert_standalone(
            d_test_handler_unregister_listener(
                NULL,
                D_TEST_EVENT_START) == false,
            "unregister from NULL",
            "Returns false for NULL handler",
            _test_info))
    {
        all_assertions_passed = false;
    }

    // unregister from handler without events
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            if (!d_assert_standalone(
                    d_test_handler_unregister_listener(
                        handler,
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

    // register then unregister
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 10);

        if (handler)
        {
            bool result;

            d_test_handler_register_listener(handler,
                                             D_TEST_EVENT_START,
                                             callback_start,
                                             true);
            result = d_test_handler_unregister_listener(
                         handler,
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

    // non-existent listener does not crash
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


/*
d_tests_sa_handler_listener_enable_disable
  Tests listener enable and disable behavior.
  Tests the following:
  - enable on NULL handler returns false
  - disable on NULL handler returns false
  - enable/disable without event system return false
  - disabled listener does not fire callback
  - re-enabled listener fires callback
*/
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

    // enable on NULL handler returns false
    if (!d_assert_standalone(
            d_test_handler_enable_listener(NULL,
                                           D_TEST_EVENT_START) == false,
            "enable on NULL handler",
            "Enable returns false for NULL handler",
            _test_info))
    {
        all_assertions_passed = false;
    }

    // disable on NULL handler returns false
    if (!d_assert_standalone(
            d_test_handler_disable_listener(NULL,
                                            D_TEST_EVENT_START) == false,
            "disable on NULL handler",
            "Disable returns false for NULL handler",
            _test_info))
    {
        all_assertions_passed = false;
    }

    // enable/disable without event system
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            if (!d_assert_standalone(
                    ( (d_test_handler_enable_listener(
                           handler,
                           D_TEST_EVENT_START) == false) &&
                      (d_test_handler_disable_listener(
                           handler,
                           D_TEST_EVENT_START) == false) ),
                    "enable/disable without events",
                    "Both return false without event system",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // enabled listener fires, disabled does not
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 10);

        if (handler)
        {
            struct d_test* test;

            d_test_handler_register_listener(handler,
                                             D_TEST_EVENT_START,
                                             callback_start,
                                             true);

            // disable the listener
            d_test_handler_disable_listener(handler,
                                            D_TEST_EVENT_START);

            reset_event_counters();
            test = d_test_new(handler_test_passing, NULL);

            if (test)
            {
                // verify disabled listener does not fire
                d_test_handler_run_test(handler, test, NULL);

                if (!d_assert_standalone(g_event_start_count == 0,
                                         "disabled listener does not fire",
                                         "Disabled listener callback not invoked",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }

                // re-enable the listener
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
