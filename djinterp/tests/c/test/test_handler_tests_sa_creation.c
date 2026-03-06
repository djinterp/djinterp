#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_new
  Tests d_test_handler_new with various configurations.
  Tests the following:
  - creating handler with NULL config succeeds
  - fields initialized correctly (event_handler, depth, abort, results,
    output_stream)
  - creating handler with custom config stores reference
  - multiple sequential creations succeed
*/
bool
d_tests_sa_handler_new
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_new ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // create handler with NULL config
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (!d_assert_standalone(handler != NULL,
                                 "new with NULL config",
                                 "d_test_handler_new(NULL) returns non-NULL",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (handler)
        {
            if (!d_assert_standalone(
                    handler->event_handler == NULL,
                    "no event system on basic handler",
                    "Handler created without event system",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                    handler->current_depth == 0,
                    "current_depth starts at 0",
                    "Handler depth initialized to 0",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                    handler->abort_on_failure == false,
                    "abort_on_failure starts false",
                    "Handler abort_on_failure initialized to false",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                    ( (handler->results.tests_run         == 0) &&
                      (handler->results.tests_passed      == 0) &&
                      (handler->results.tests_failed      == 0) &&
                      (handler->results.assertions_run    == 0) &&
                      (handler->results.assertions_passed == 0) &&
                      (handler->results.assertions_failed == 0) &&
                      (handler->results.blocks_run        == 0) &&
                      (handler->results.max_depth         == 0) ),
                    "all result fields zero",
                    "All handler result fields initialized to 0",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                    handler->output_stream == stdout,
                    "output_stream defaults to stdout",
                    "Handler output_stream set to stdout",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // create handler with custom config stores reference
    {
        struct d_test_options* config;
        struct d_test_handler* handler;

        config  = d_test_options_new(D_TEST_MODE_VERBOSE);
        handler = d_test_handler_new(config);

        if (handler)
        {
            if (!d_assert_standalone(
                    handler->default_config == config,
                    "stores config reference",
                    "Handler stores the default_config pointer",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }

        d_test_options_free(config);
    }

    // multiple sequential creations
    {
        int  i;
        bool all_ok;

        all_ok = true;

        for (i = 0; i < 10; i++)
        {
            struct d_test_handler* temp;

            temp = d_test_handler_new(NULL);

            if (!temp)
            {
                all_ok = false;
            }
            else
            {
                d_test_handler_free(temp);
            }
        }

        if (!d_assert_standalone(all_ok,
                                 "10 sequential creations",
                                 "10 sequential handler creations all succeed",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_new\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_new\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_new_with_events
  Tests d_test_handler_new_with_events with different event capacities.
  Tests the following:
  - standard capacity creates event handler
  - EMIT_EVENTS flag set automatically
  - handler fields properly initialized alongside events
  - various capacities (1, 5, 50, 100) all work
*/
bool
d_tests_sa_handler_new_with_events
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_new_with_events ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // standard capacity
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 10);

        if (!d_assert_standalone(handler != NULL,
                                 "new_with_events(NULL, 10)",
                                 "Handler created with event capacity 10",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (handler)
        {
            if (!d_assert_standalone(
                    handler->event_handler != NULL,
                    "event_handler allocated",
                    "Event handler is non-NULL",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                    d_test_handler_has_flag(handler,
                        D_TEST_HANDLER_FLAG_EMIT_EVENTS),
                    "EMIT_EVENTS flag auto-set",
                    "EMIT_EVENTS flag set automatically",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            if (!d_assert_standalone(
                    ( (handler->current_depth   == 0)     &&
                      (handler->abort_on_failure == false) ),
                    "fields initialized with events",
                    "Handler fields properly initialized",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // small and large capacities
    {
        size_t caps[4];
        int    i;
        bool   all_ok;

        caps[0] = 1;
        caps[1] = 5;
        caps[2] = 50;
        caps[3] = 100;
        all_ok  = true;

        for (i = 0; i < 4; i++)
        {
            struct d_test_handler* h;

            h = d_test_handler_new_with_events(NULL, caps[i]);

            if ( (!h) || (!h->event_handler) )
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }

        if (!d_assert_standalone(all_ok,
                                 "various event capacities",
                                 "All event capacities (1,5,50,100) work",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_new_with_events\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_new_with_events\n",
               D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_new_full
  Tests d_test_handler_new_full with all parameter combinations.
  Tests the following:
  - all parameters zero/NULL creates minimal handler
  - event capacity > 0 allocates event handler
  - stack capacity > 0 allocates both stacks
  - TRACK_STACK flag allocates stacks with default capacity
  - ABORT_ON_FAIL flag sets abort_on_failure
  - combined flags work correctly
*/
bool
d_tests_sa_handler_new_full
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_new_full ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // minimal handler (all zeros)
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(NULL,
                                          0,
                                          0,
                                          D_TEST_HANDLER_FLAG_NONE);

        if (!d_assert_standalone(handler != NULL,
                                 "new_full minimal",
                                 "Minimal handler created successfully",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (handler)
        {
            if (!d_assert_standalone(
                    ( (handler->event_handler   == NULL)  &&
                      (handler->result_stack    == NULL)  &&
                      (handler->context_stack   == NULL)  &&
                      (handler->abort_on_failure == false) ),
                    "minimal handler fields",
                    "Minimal handler has NULL stacks and no events",
                    _test_info))
            {
                all_assertions_passed = false;
            }

            d_test_handler_free(handler);
        }
    }

    // event capacity allocates event handler
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      16,
                      0,
                      D_TEST_HANDLER_FLAG_EMIT_EVENTS);

        if (!d_assert_standalone(
                ( (handler != NULL) &&
                  (handler->event_handler != NULL) ),
                "event capacity allocates handler",
                "Event capacity > 0 allocates event handler",
                _test_info))
        {
            all_assertions_passed = false;
        }

        if (handler)
        {
            d_test_handler_free(handler);
        }
    }

    // stack capacity allocates both stacks
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_NONE);

        if (!d_assert_standalone(
                ( (handler != NULL)              &&
                  (handler->result_stack  != NULL) &&
                  (handler->context_stack != NULL) ),
                "stack capacity allocates stacks",
                "Stack capacity > 0 allocates both stacks",
                _test_info))
        {
            all_assertions_passed = false;
        }

        if (handler)
        {
            d_test_handler_free(handler);
        }
    }

    // TRACK_STACK flag allocates with default capacity
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      0,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if (!d_assert_standalone(
                ( (handler != NULL)              &&
                  (handler->result_stack  != NULL) &&
                  (handler->context_stack != NULL) ),
                "TRACK_STACK allocates stacks",
                "TRACK_STACK flag allocates stacks even with cap=0",
                _test_info))
        {
            all_assertions_passed = false;
        }

        if (handler)
        {
            d_test_handler_free(handler);
        }
    }

    // ABORT_ON_FAIL flag
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      0,
                      D_TEST_HANDLER_FLAG_ABORT_ON_FAIL);

        if (!d_assert_standalone(
                ( (handler != NULL) &&
                  (handler->abort_on_failure == true) ),
                "ABORT_ON_FAIL flag",
                "ABORT_ON_FAIL flag sets abort_on_failure to true",
                _test_info))
        {
            all_assertions_passed = false;
        }

        if (handler)
        {
            d_test_handler_free(handler);
        }
    }

    // combined flags
    {
        struct d_test_handler* handler;
        uint32_t               flags;

        flags = D_TEST_HANDLER_FLAG_EMIT_EVENTS
              | D_TEST_HANDLER_FLAG_ABORT_ON_FAIL
              | D_TEST_HANDLER_FLAG_TRACK_STACK;

        handler = d_test_handler_new_full(NULL, 10, 16, flags);

        if (!d_assert_standalone(
                ( (handler != NULL)               &&
                  (handler->event_handler  != NULL) &&
                  (handler->result_stack   != NULL) &&
                  (handler->context_stack  != NULL) &&
                  (handler->abort_on_failure == true) ),
                "combined flags",
                "All combined flags produce correct handler state",
                _test_info))
        {
            all_assertions_passed = false;
        }

        if (handler)
        {
            d_test_handler_free(handler);
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_new_full\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_new_full\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_free
  Tests d_test_handler_free with valid and NULL handlers.
  Tests the following:
  - freeing NULL handler does not crash
  - freeing basic handler succeeds
  - freeing handler with event system succeeds
  - freeing handler with stacks succeeds
  - freeing fully-loaded handler succeeds
  - multiple sequential create/free cycles succeed
*/
bool
d_tests_sa_handler_free
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_assertions_passed;

    printf("  --- Testing d_test_handler_free ---\n");
    initial_tests_passed  = _test_info->tests_passed;
    all_assertions_passed = true;

    // free NULL handler
    d_test_handler_free(NULL);

    if (!d_assert_standalone(true,
                             "free NULL handler",
                             "Freeing NULL handler does not crash",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // free basic handler
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new(NULL);

        if (handler)
        {
            d_test_handler_free(handler);

            if (!d_assert_standalone(true,
                                     "free basic handler",
                                     "Freeing basic handler succeeds",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }
    }

    // free handler with events
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_with_events(NULL, 10);

        if (handler)
        {
            d_test_handler_free(handler);

            if (!d_assert_standalone(true,
                                     "free handler with events",
                                     "Freeing handler with events succeeds",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }
    }

    // free handler with stacks
    {
        struct d_test_handler* handler;

        handler = d_test_handler_new_full(
                      NULL,
                      0,
                      32,
                      D_TEST_HANDLER_FLAG_TRACK_STACK);

        if (handler)
        {
            d_test_handler_free(handler);

            if (!d_assert_standalone(true,
                                     "free handler with stacks",
                                     "Freeing handler with stacks succeeds",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }
    }

    // free fully-loaded handler
    {
        struct d_test_handler* handler;
        uint32_t               flags;

        flags = D_TEST_HANDLER_FLAG_EMIT_EVENTS
              | D_TEST_HANDLER_FLAG_TRACK_STACK
              | D_TEST_HANDLER_FLAG_ABORT_ON_FAIL;

        handler = d_test_handler_new_full(NULL, 20, 32, flags);

        if (handler)
        {
            d_test_handler_free(handler);

            if (!d_assert_standalone(true,
                                     "free fully-loaded handler",
                                     "Freeing fully-loaded handler succeeds",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }
    }

    // multiple sequential frees
    {
        int  i;
        bool all_ok;

        all_ok = true;

        for (i = 0; i < 10; i++)
        {
            struct d_test_handler* h;

            h = d_test_handler_new_with_events(NULL, 5);

            if (h)
            {
                d_test_handler_free(h);
            }
            else
            {
                all_ok = false;
            }
        }

        if (!d_assert_standalone(all_ok,
                                 "10 sequential frees",
                                 "10 sequential create/free cycles succeed",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] d_test_handler_free\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] d_test_handler_free\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
