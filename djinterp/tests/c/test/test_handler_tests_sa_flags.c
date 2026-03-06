#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_set_flag
  Tests d_test_handler_set_flag for all flag types.
  Tests the following:
  - setting flag on NULL handler does not crash
  - setting D_TEST_HANDLER_FLAG_EMIT_EVENTS
  - setting ABORT_ON_FAIL updates abort_on_failure field
  - setting multiple flags preserves all
  - setting an already-set flag is idempotent
*/
bool
d_tests_sa_handler_set_flag
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_set_flag ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // setting flag on NULL handler does not crash
    d_test_handler_set_flag(NULL, D_TEST_HANDLER_FLAG_EMIT_EVENTS);

    if (!d_assert_standalone(true,
                             "set_flag on NULL handler",
                             "Setting flag on NULL handler does not crash",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // setting D_TEST_HANDLER_FLAG_EMIT_EVENTS
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            d_test_handler_set_flag(handler,
                                    D_TEST_HANDLER_FLAG_EMIT_EVENTS);

            if (!d_assert_standalone(
                    d_test_handler_has_flag(
                        handler,
                        D_TEST_HANDLER_FLAG_EMIT_EVENTS),
                    "set EMIT_EVENTS flag",
                    "EMIT_EVENTS flag set and readable",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // setting ABORT_ON_FAIL updates abort_on_failure field
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            if (!d_assert_standalone(
                    handler->abort_on_failure == false,
                    "abort_on_failure initially false",
                    "Handler starts with abort_on_failure == false",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_set_flag(handler,
                                    D_TEST_HANDLER_FLAG_ABORT_ON_FAIL);

            if (!d_assert_standalone(
                    handler->abort_on_failure == true,
                    "set ABORT_ON_FAIL updates field",
                    "Setting ABORT_ON_FAIL sets abort_on_failure to true",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                    d_test_handler_has_flag(
                        handler,
                        D_TEST_HANDLER_FLAG_ABORT_ON_FAIL),
                    "ABORT_ON_FAIL flag readable",
                    "ABORT_ON_FAIL flag set and readable via has_flag",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // setting multiple flags preserves all
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            d_test_handler_set_flag(handler,
                                    D_TEST_HANDLER_FLAG_EMIT_EVENTS);
            d_test_handler_set_flag(handler,
                                    D_TEST_HANDLER_FLAG_ABORT_ON_FAIL);
            d_test_handler_set_flag(handler,
                                    D_TEST_HANDLER_FLAG_TRACK_STACK);

            if (!d_assert_standalone(
                    ( (d_test_handler_has_flag(
                           handler,
                           D_TEST_HANDLER_FLAG_EMIT_EVENTS))  &&
                      (d_test_handler_has_flag(
                           handler,
                           D_TEST_HANDLER_FLAG_ABORT_ON_FAIL)) &&
                      (d_test_handler_has_flag(
                           handler,
                           D_TEST_HANDLER_FLAG_TRACK_STACK)) ),
                    "multiple flags preserved",
                    "All three flags set and readable",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // setting already-set flag is idempotent
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            d_test_handler_set_flag(handler,
                                    D_TEST_HANDLER_FLAG_EMIT_EVENTS);
            d_test_handler_set_flag(handler,
                                    D_TEST_HANDLER_FLAG_EMIT_EVENTS);

            if (!d_assert_standalone(
                    d_test_handler_has_flag(
                        handler,
                        D_TEST_HANDLER_FLAG_EMIT_EVENTS),
                    "idempotent set_flag",
                    "Double-setting same flag is idempotent",
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
        printf("%s[PASS] d_test_handler_set_flag\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_set_flag\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_clear_flag
  Tests d_test_handler_clear_flag for all flag types.
  Tests the following:
  - clearing flag on NULL handler does not crash
  - clearing EMIT_EVENTS flag
  - clearing ABORT_ON_FAIL updates abort_on_failure to false
  - clearing one flag preserves others
  - clearing unset flag is a no-op
*/
bool
d_tests_sa_handler_clear_flag
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_clear_flag ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // clearing flag on NULL handler does not crash
    d_test_handler_clear_flag(NULL, D_TEST_HANDLER_FLAG_EMIT_EVENTS);

    if (!d_assert_standalone(true,
                             "clear_flag on NULL handler",
                             "Clearing flag on NULL handler does not crash",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // clearing EMIT_EVENTS flag
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      0,
                      D_TEST_HANDLER_FLAG_EMIT_EVENTS);

        if (handler)
        {
            d_test_handler_clear_flag(handler,
                                      D_TEST_HANDLER_FLAG_EMIT_EVENTS);

            if (!d_assert_standalone(
                    !d_test_handler_has_flag(
                         handler,
                         D_TEST_HANDLER_FLAG_EMIT_EVENTS),
                    "clear EMIT_EVENTS flag",
                    "EMIT_EVENTS flag cleared successfully",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // clearing ABORT_ON_FAIL updates abort_on_failure
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      0,
                      D_TEST_HANDLER_FLAG_ABORT_ON_FAIL);

        if (handler)
        {
            if (!d_assert_standalone(
                    handler->abort_on_failure == true,
                    "abort_on_failure initially true via flag",
                    "Handler starts with abort_on_failure true from flag",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_clear_flag(handler,
                                      D_TEST_HANDLER_FLAG_ABORT_ON_FAIL);

            if (!d_assert_standalone(
                    handler->abort_on_failure == false,
                    "clear ABORT_ON_FAIL updates field",
                    "Clearing ABORT_ON_FAIL sets abort_on_failure false",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // clearing one flag preserves others
    {
        struct d_test_handler* handler;
        uint32_t               combined;

        combined = D_TEST_HANDLER_FLAG_EMIT_EVENTS
                 | D_TEST_HANDLER_FLAG_TRACK_STACK;

        handler = d_test_handler_new_full(NULL,
                                          10,
                                          16,
                                          combined);

        if (handler)
        {
            d_test_handler_clear_flag(handler,
                                      D_TEST_HANDLER_FLAG_EMIT_EVENTS);

            if (!d_assert_standalone(
                    ( (!d_test_handler_has_flag(
                            handler,
                            D_TEST_HANDLER_FLAG_EMIT_EVENTS)) &&
                      (d_test_handler_has_flag(
                            handler,
                            D_TEST_HANDLER_FLAG_TRACK_STACK)) ),
                    "clear one preserves others",
                    "Clearing one flag does not affect other flags",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // clearing unset flag is no-op
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            d_test_handler_clear_flag(handler,
                                      D_TEST_HANDLER_FLAG_EMIT_EVENTS);

            if (!d_assert_standalone(
                    !d_test_handler_has_flag(
                         handler,
                         D_TEST_HANDLER_FLAG_EMIT_EVENTS),
                    "clear unset flag is no-op",
                    "Clearing an unset flag does not break anything",
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
        printf("%s[PASS] d_test_handler_clear_flag\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_clear_flag\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_has_flag
  Tests d_test_handler_has_flag with various flag combinations.
  Tests the following:
  - has_flag on NULL handler returns false
  - fresh handler does not have EMIT_EVENTS or ABORT_ON_FAIL
  - has_flag detects flag after set and absence after clear
  - has_flag with FLAG_NONE always returns true (identity)
*/
bool
d_tests_sa_handler_has_flag
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_has_flag ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // has_flag on NULL handler returns false
    {
        bool result;

        result = d_test_handler_has_flag(NULL,
                     D_TEST_HANDLER_FLAG_EMIT_EVENTS);

        if (!d_assert_standalone(result == false,
                                 "has_flag on NULL handler",
                                 "has_flag returns false for NULL handler",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // fresh handler has no flags
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            if (!d_assert_standalone(
                    !d_test_handler_has_flag(
                         handler,
                         D_TEST_HANDLER_FLAG_EMIT_EVENTS),
                    "no EMIT_EVENTS on fresh handler",
                    "Fresh handler does not have EMIT_EVENTS",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                    !d_test_handler_has_flag(
                         handler,
                         D_TEST_HANDLER_FLAG_ABORT_ON_FAIL),
                    "no ABORT_ON_FAIL on fresh handler",
                    "Fresh handler does not have ABORT_ON_FAIL",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // has_flag after set/clear cycle
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            d_test_handler_set_flag(handler,
                                    D_TEST_HANDLER_FLAG_EMIT_EVENTS);

            if (!d_assert_standalone(
                    d_test_handler_has_flag(
                        handler,
                        D_TEST_HANDLER_FLAG_EMIT_EVENTS),
                    "has_flag after set",
                    "has_flag detects flag after set",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_clear_flag(handler,
                                      D_TEST_HANDLER_FLAG_EMIT_EVENTS);

            if (!d_assert_standalone(
                    !d_test_handler_has_flag(
                         handler,
                         D_TEST_HANDLER_FLAG_EMIT_EVENTS),
                    "has_flag after clear",
                    "has_flag returns false after clear",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // has_flag with FLAG_NONE always returns true (identity)
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            // FLAG_NONE (0) is always a subset via bitwise AND
            if (!d_assert_standalone(
                    d_test_handler_has_flag(
                        handler,
                        D_TEST_HANDLER_FLAG_NONE),
                    "has_flag with FLAG_NONE",
                    "has_flag with FLAG_NONE returns true",
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
        printf("%s[PASS] d_test_handler_has_flag\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_has_flag\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
