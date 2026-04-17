#include "./event_handler_tests_sa.h"


/******************************************************************************
 * III. EVENT OPERATIONS TESTS
 *****************************************************************************/

// d_tests_sa_event_handler_ops_fire_count
//   static: tracks callback invocations across fire/process tests.
static int d_tests_sa_event_handler_ops_fire_count = 0;

// d_tests_sa_event_handler_ops_cb
//   helper: callback that increments the fire counter.
static void
d_tests_sa_event_handler_ops_cb(size_t _count, void** _args)
{
    (void)_count;
    (void)_args;

    d_tests_sa_event_handler_ops_fire_count++;

    return;
}


/*
d_tests_sa_event_handler_fire_valid
  Tests d_event_handler_fire_event with a matching enabled listener.
  Tests the following:
  - Returns 1 (listener executed)
  - Callback was invoked
*/
bool
d_tests_sa_event_handler_fire_valid
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* lis;
    struct d_event*          event;
    ssize_t                  ret;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        d_tests_sa_event_handler_ops_fire_count = 0;
        lis   = d_event_listener_new(100, d_tests_sa_event_handler_ops_cb, true);
        event = d_event_new(100);

        d_event_handler_bind(handler, lis);
        ret = d_event_handler_fire_event(handler, event);

        result = d_assert_standalone(
            ret == 1,
            "eh_fire_valid_returns_1",
            "fire_event should return 1 when listener fires",
            _counter) && result;

        result = d_assert_standalone(
            d_tests_sa_event_handler_ops_fire_count == 1,
            "eh_fire_valid_callback",
            "Callback should have been invoked once",
            _counter) && result;

        d_event_free(event);
        d_event_handler_free(handler);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_fire_no_listener
  Tests d_event_handler_fire_event with no matching listener.
  Tests the following:
  - Returns 0
*/
bool
d_tests_sa_event_handler_fire_no_listener
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_event_handler* handler;
    struct d_event*         event;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        event = d_event_new(999);

        result = d_assert_standalone(
            d_event_handler_fire_event(handler, event) == 0,
            "eh_fire_no_listener",
            "fire_event with no listener should return 0",
            _counter) && result;

        d_event_free(event);
        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_fire_disabled
  Tests d_event_handler_fire_event with disabled listener.
  Tests the following:
  - Returns 0
  - Callback is NOT invoked
*/
bool
d_tests_sa_event_handler_fire_disabled
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* lis;
    struct d_event*          event;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        d_tests_sa_event_handler_ops_fire_count = 0;
        lis   = d_event_listener_new(100, d_tests_sa_event_handler_ops_cb, true);
        event = d_event_new(100);

        d_event_handler_bind(handler, lis);
        d_event_handler_disable_listener(handler, 100);

        result = d_assert_standalone(
            d_event_handler_fire_event(handler, event) == 0 &&
            d_tests_sa_event_handler_ops_fire_count == 0,
            "eh_fire_disabled",
            "Disabled listener should not fire (return 0, count 0)",
            _counter) && result;

        d_event_free(event);
        d_event_handler_free(handler);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_fire_null
  Tests d_event_handler_fire_event with NULL parameters.
  Tests the following:
  - Returns -1
*/
bool
d_tests_sa_event_handler_fire_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_handler_fire_event(NULL, NULL) == -1,
        "eh_fire_null",
        "fire_event(NULL, NULL) should return -1",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_queue_valid
  Tests d_event_handler_queue_event.
  Tests the following:
  - Returns true
  - pending_events increases
  - Multiple queues accumulate
*/
bool
d_tests_sa_event_handler_queue_valid
(
    struct d_test_counter* _counter
)
{
    bool                    result;
    struct d_event_handler* handler;
    struct d_event*         e1;
    struct d_event*         e2;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        e1 = d_event_new(100);
        e2 = d_event_new(200);

        result = d_assert_standalone(
            d_event_handler_queue_event(handler, e1) == true,
            "eh_queue_valid_returns_true",
            "queue_event should return true",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_pending_events(handler) == 1,
            "eh_queue_valid_count_1",
            "pending_events should be 1 after first queue",
            _counter) && result;

        d_event_handler_queue_event(handler, e2);

        result = d_assert_standalone(
            d_event_handler_pending_events(handler) == 2,
            "eh_queue_valid_count_2",
            "pending_events should be 2 after second queue",
            _counter) && result;

        d_event_free(e1);
        d_event_free(e2);
        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_queue_null
  Tests d_event_handler_queue_event with NULL parameters.
  Tests the following:
  - Returns false
*/
bool
d_tests_sa_event_handler_queue_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_handler_queue_event(NULL, NULL) == false,
        "eh_queue_null",
        "queue_event(NULL, NULL) should return false",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_process_all
  Tests d_event_handler_process_events processing entire queue.
  Tests the following:
  - Returns correct processed count
  - All callbacks invoked
  - Queue is empty after processing
*/
bool
d_tests_sa_event_handler_process_all
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* lis;
    struct d_event*          e1;
    struct d_event*          e2;
    struct d_event*          e3;
    size_t                   processed;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        d_tests_sa_event_handler_ops_fire_count = 0;
        lis = d_event_listener_new(100, d_tests_sa_event_handler_ops_cb, true);

        d_event_handler_bind(handler, lis);

        e1 = d_event_new(100);
        e2 = d_event_new(100);
        e3 = d_event_new(100);

        d_event_handler_queue_event(handler, e1);
        d_event_handler_queue_event(handler, e2);
        d_event_handler_queue_event(handler, e3);

        processed = d_event_handler_process_events(handler, 10);

        result = d_assert_standalone(
            processed == 3 && d_tests_sa_event_handler_ops_fire_count == 3,
            "eh_process_all_count",
            "Should process 3 events with 3 callbacks",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_pending_events(handler) == 0,
            "eh_process_all_empty",
            "Queue should be empty after processing all",
            _counter) && result;

        d_event_free(e1);
        d_event_free(e2);
        d_event_free(e3);
        d_event_handler_free(handler);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_process_limited
  Tests d_event_handler_process_events with max_events limit.
  Tests the following:
  - Only processes up to max_events
  - Remaining events stay in queue
*/
bool
d_tests_sa_event_handler_process_limited
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* lis;
    struct d_event*          events[3];
    size_t                   processed;
    int                      i;

    result  = true;
    handler = d_event_handler_new(10, 10);

    if (handler)
    {
        d_tests_sa_event_handler_ops_fire_count = 0;
        lis = d_event_listener_new(100, d_tests_sa_event_handler_ops_cb, true);

        d_event_handler_bind(handler, lis);

        for (i = 0; i < 3; i++)
        {
            events[i] = d_event_new(100);
            d_event_handler_queue_event(handler, events[i]);
        }

        processed = d_event_handler_process_events(handler, 2);

        result = d_assert_standalone(
            processed == 2 && d_tests_sa_event_handler_ops_fire_count == 2,
            "eh_process_limited",
            "Should process only 2 of 3 events",
            _counter) && result;

        result = d_assert_standalone(
            d_event_handler_pending_events(handler) == 1,
            "eh_process_limited_remaining",
            "1 event should remain in queue",
            _counter) && result;

        for (i = 0; i < 3; i++)
        {
            d_event_free(events[i]);
        }

        d_event_handler_free(handler);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_process_null
  Tests d_event_handler_process_events with NULL handler.
  Tests the following:
  - Returns 0
*/
bool
d_tests_sa_event_handler_process_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_event_handler_process_events(NULL, 10) == 0,
        "eh_process_null",
        "process_events(NULL) should return 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_event_handler_event_ops_all
  Runs all event operations tests.
*/
bool
d_tests_sa_event_handler_event_ops_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Event Operations\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_event_handler_fire_valid(_counter)       && result;
    result = d_tests_sa_event_handler_fire_no_listener(_counter) && result;
    result = d_tests_sa_event_handler_fire_disabled(_counter)    && result;
    result = d_tests_sa_event_handler_fire_null(_counter)        && result;
    result = d_tests_sa_event_handler_queue_valid(_counter)      && result;
    result = d_tests_sa_event_handler_queue_null(_counter)       && result;
    result = d_tests_sa_event_handler_process_all(_counter)      && result;
    result = d_tests_sa_event_handler_process_limited(_counter)  && result;
    result = d_tests_sa_event_handler_process_null(_counter)     && result;

    return result;
}
