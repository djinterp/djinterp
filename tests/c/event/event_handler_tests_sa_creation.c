#include "./event_handler_tests_sa.h"


/******************************************************************************
 * I. CREATION AND DESTRUCTION TESTS
 *****************************************************************************/

// d_tests_sa_event_handler_create_cb
//   helper: minimal callback for listener construction.
static void
d_tests_sa_event_handler_create_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_event_handler_new_valid
  Tests d_event_handler_new with valid parameters.
  Tests the following:
  - Returns non-NULL
  - events (circular array) is allocated
  - listeners (hash table) is allocated
*/
bool
d_tests_sa_event_handler_new_valid
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_event_handler* handler;

    result  = true;
    handler = d_event_handler_new(10, 20);

    result = d_assert_standalone(
        handler != NULL,
        "eh_new_valid_non_null",
        "d_event_handler_new(10, 20) should return non-NULL",
        _counter) && result;

    if (handler)
    {
        result = d_assert_standalone(
            handler->events != NULL &&
            handler->listeners != NULL,
            "eh_new_valid_members",
            "events and listeners should both be allocated",
            _counter) && result;

        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_new_zero
  Tests d_event_handler_new with zero capacities.
  Tests the following:
  - Returns NULL (circular_array rejects zero capacity)
*/
bool
d_tests_sa_event_handler_new_zero
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_event_handler* handler;

    result  = true;
    handler = d_event_handler_new(0, 0);

    result = d_assert_standalone(
        handler == NULL,
        "eh_new_zero",
        "new(0, 0) should return NULL",
        _counter) && result;

    if (handler)
    {
        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_new_large
  Tests d_event_handler_new with large capacities.
  Tests the following:
  - Returns non-NULL
*/
bool
d_tests_sa_event_handler_new_large
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_event_handler* handler;

    result  = true;
    handler = d_event_handler_new(1000, 1000);

    result = d_assert_standalone(
        handler != NULL,
        "eh_new_large",
        "new(1000, 1000) should succeed",
        _counter) && result;

    if (handler)
    {
        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_free_valid
  Tests d_event_handler_free with a valid handler.
  Tests the following:
  - Does not crash
*/
bool
d_tests_sa_event_handler_free_valid
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_event_handler* handler;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        d_event_handler_free(handler);
    }

    result = d_assert_standalone(
        true,
        "eh_free_valid",
        "Freeing valid handler should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_free_null
  Tests d_event_handler_free with NULL.
  Tests the following:
  - Does not crash
*/
bool
d_tests_sa_event_handler_free_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    d_event_handler_free(NULL);

    result = d_assert_standalone(
        true,
        "eh_free_null",
        "free(NULL) should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_free_with_listeners
  Tests d_event_handler_free on handler containing bound listeners.
  Tests the following:
  - Does not crash
  - Listeners survive handler free (managed externally)
*/
bool
d_tests_sa_event_handler_free_with_listeners
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* lis;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        lis = d_event_listener_new(100,
                                   d_tests_sa_event_handler_create_cb,
                                   true);

        if (lis)
        {
            d_event_handler_bind(handler, lis);
        }

        d_event_handler_free(handler);

        result = d_assert_standalone(
            true,
            "eh_free_with_listeners",
            "Freeing handler with bound listeners should not crash",
            _counter) && result;

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_creation_all
  Runs all creation and destruction tests.
*/
bool
d_tests_sa_event_handler_creation_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Creation and Destruction\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_event_handler_new_valid(_counter)          && result;
    result = d_tests_sa_event_handler_new_zero(_counter)           && result;
    result = d_tests_sa_event_handler_new_large(_counter)          && result;
    result = d_tests_sa_event_handler_free_valid(_counter)         && result;
    result = d_tests_sa_event_handler_free_null(_counter)          && result;
    result = d_tests_sa_event_handler_free_with_listeners(_counter) && result;

    return result;
}
