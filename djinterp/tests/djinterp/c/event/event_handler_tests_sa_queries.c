#include "./event_handler_tests_sa.h"


/******************************************************************************
 * IV. QUERY FUNCTION TESTS
 *****************************************************************************/

// d_tests_sa_event_handler_query_cb
//   helper: minimal callback for listener construction.
static void
d_tests_sa_event_handler_query_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    return;
}


/*
d_tests_sa_event_handler_queries_empty
  Tests all query functions on an empty handler.
  Tests the following:
  - listener_count returns 0
  - enabled_count returns 0
  - pending_events returns 0
*/
bool
d_tests_sa_event_handler_queries_empty
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
        result = d_assert_standalone(
            d_event_handler_listener_count(handler) == 0 &&
            d_event_handler_enabled_count(handler) == 0 &&
            d_event_handler_pending_events(handler) == 0,
            "eh_queries_empty",
            "All query functions should return 0 for empty handler",
            _counter) && result;

        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_queries_populated
  Tests all query functions on a populated handler.
  Tests the following:
  - listener_count reflects bound listeners
  - enabled_count counts only enabled listeners
  - pending_events reflects queued events
*/
bool
d_tests_sa_event_handler_queries_populated
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* l1;
    struct d_event_listener* l2;
    struct d_event*          e1;
    struct d_event*          e2;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        l1 = d_event_listener_new(100, d_tests_sa_event_handler_query_cb, true);
        l2 = d_event_listener_new(200, d_tests_sa_event_handler_query_cb, false);

        d_event_handler_bind(handler, l1);
        d_event_handler_bind(handler, l2);

        result = d_assert_standalone(
            d_event_handler_listener_count(handler) == 2,
            "eh_queries_listener_count",
            "listener_count should be 2",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_enabled_count(handler) == 1,
            "eh_queries_enabled_count",
            "enabled_count should be 1 (only l1 enabled)",
            _counter) && result;

        e1 = d_event_new(100);
        e2 = d_event_new(200);

        d_event_handler_queue_event(handler, e1);
        d_event_handler_queue_event(handler, e2);

        result = d_assert_standalone(
            d_event_handler_pending_events(handler) == 2,
            "eh_queries_pending",
            "pending_events should be 2",
            _counter) && result;

        d_event_free(e1);
        d_event_free(e2);
        d_event_handler_free(handler);

        if (l1)
        {
            d_event_listener_free(l1);
        }

        if (l2)
        {
            d_event_listener_free(l2);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_queries_null
  Tests query functions with NULL handler.
  Tests the following:
  - All return 0
*/
bool
d_tests_sa_event_handler_queries_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_handler_listener_count(NULL) == 0 &&
        d_event_handler_enabled_count(NULL) == 0 &&
        d_event_handler_pending_events(NULL) == 0,
        "eh_queries_null",
        "All queries on NULL handler should return 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_query_all
  Runs all query function tests.
*/
bool
d_tests_sa_event_handler_query_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Query Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_event_handler_queries_empty(_counter)     && result;
    result = d_tests_sa_event_handler_queries_populated(_counter)  && result;
    result = d_tests_sa_event_handler_queries_null(_counter)       && result;

    return result;
}
