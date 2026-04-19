#include "./event_handler_tests_sa.h"


/******************************************************************************
 * V. INTEGRATION, STRESS, AND EDGE CASE TESTS
 *****************************************************************************/

// d_tests_sa_event_handler_adv_fire_count
//   static: tracks callback invocations in advanced tests.
static int d_tests_sa_event_handler_adv_fire_count = 0;

// d_tests_sa_event_handler_adv_cb
//   helper: callback that increments the fire counter.
D_STATIC void
d_tests_sa_event_handler_adv_cb
(
    void* _context
)
{
    (void)_context;

    d_tests_sa_event_handler_adv_fire_count++;

    return;
}

/*
d_tests_sa_event_handler_integration
  Tests a complete workflow: bind listeners, queue events, process.
  Tests the following:
  - 3 listeners bound, 3 events queued, 3 processed with 3 callbacks
  - Disabled listener is skipped during processing
*/
bool
d_tests_sa_event_handler_integration
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* listeners[3];
    struct d_event*          event;
    size_t                   processed;
    int                      i;

    result  = true;
    handler = d_event_handler_new(20, 20);

    if (handler)
    {
        d_tests_sa_event_handler_adv_fire_count = 0;

        // bind 3 listeners
        for (i = 0; i < 3; i++)
        {
            listeners[i] = d_event_listener_new(
                               (i * 100),
                               d_tests_sa_event_handler_adv_cb,
                               true);
            d_event_handler_bind(handler, listeners[i]);
        }

        // queue 3 events (one per listener)
        for (i = 0; i < 3; i++)
        {
            event = d_event_new(i * 100);
            d_event_handler_queue_event(handler, event);
            d_event_free(event);
        }

        processed = d_event_handler_process_events(handler, 10);

        result = d_assert_standalone(
            processed == 3 && d_tests_sa_event_handler_adv_fire_count == 3,
            "eh_integ_full_workflow",
            "Full workflow: 3 events processed, 3 callbacks fired",
            _counter) && result;

        // disable one listener, queue 2 events
        d_event_handler_disable_listener(handler, 100);
        d_tests_sa_event_handler_adv_fire_count = 0;

        {
            struct d_event* e1;
            struct d_event* e2;

            e1 = d_event_new(100);
            e2 = d_event_new(200);

            d_event_handler_queue_event(handler, e1);
            d_event_handler_queue_event(handler, e2);
            d_event_handler_process_events(handler, 10);

            result = d_assert_standalone(
                d_tests_sa_event_handler_adv_fire_count == 1,
                "eh_integ_disabled_skipped",
                "Disabled listener should be skipped (1 of 2 fires)",
                _counter) && result;

            d_event_free(e1);
            d_event_free(e2);
        }

        for (i = 0; i < 3; i++)
        {
            if (listeners[i])
            {
                d_event_listener_free(listeners[i]);
            }
        }

        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_stress
  Stress-tests with 50 listeners and 50 events.
  Tests the following:
  - All 50 events processed
  - All 50 callbacks fired
*/
bool
d_tests_sa_event_handler_stress
(
    struct d_test_counter* _counter
)
{
    bool                     result;
    struct d_event_handler*  handler;
    struct d_event_listener* listeners[50];
    struct d_event*          event;
    size_t                   processed;
    int                      i;

    result  = true;
    handler = d_event_handler_new(100, 100);

    if (handler)
    {
        d_tests_sa_event_handler_adv_fire_count = 0;

        for (i = 0; i < 50; i++)
        {
            listeners[i] = d_event_listener_new(i,
                                                d_tests_sa_event_handler_adv_cb,
                                                true);
            d_event_handler_bind(handler, listeners[i]);
        }

        for (i = 0; i < 50; i++)
        {
            event = d_event_new(i);
            d_event_handler_queue_event(handler, event);
            d_event_free(event);
        }

        processed = d_event_handler_process_events(handler, 100);

        result = d_assert_standalone(
            processed == 50 && d_tests_sa_event_handler_adv_fire_count == 50,
            "eh_stress",
            "50 listeners, 50 events: all processed and fired",
            _counter) && result;

        for (i = 0; i < 50; i++)
        {
            if (listeners[i])
            {
                d_event_listener_free(listeners[i]);
            }
        }

        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_edge_empty_queue
  Tests processing an empty queue and firing with no listener.
  Tests the following:
  - process_events on empty queue returns 0
  - fire_event with no matching listener returns 0
*/
bool
d_tests_sa_event_handler_edge_empty_queue
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
        result = d_assert_standalone(
            d_event_handler_process_events(handler, 10) == 0,
            "eh_edge_empty_process",
            "Processing empty queue should return 0",
            _counter) && result;

        event = d_event_new(999);

        result = d_assert_standalone(
            d_event_handler_fire_event(handler, event) == 0,
            "eh_edge_no_match",
            "Firing with no matching listener should return 0",
            _counter) && result;

        d_event_free(event);
        d_event_handler_free(handler);
    }

    return result;
}

/*
d_tests_sa_event_handler_edge_idempotent
  Tests that repeated enable/disable is idempotent.
  Tests the following:
  - Double disable keeps enabled_count at 0
  - Double enable keeps enabled_count at 1
*/
bool
d_tests_sa_event_handler_edge_idempotent
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
        lis = d_event_listener_new(100, d_tests_sa_event_handler_adv_cb, true);
        d_event_handler_bind(handler, lis);

        // double disable
        d_event_handler_disable_listener(handler, 100);
        d_event_handler_disable_listener(handler, 100);

        result = d_assert_standalone(
            d_event_handler_enabled_count(handler) == 0,
            "eh_edge_double_disable",
            "Double disable should keep enabled_count at 0",
            _counter) && result;

        // double enable
        d_event_handler_enable_listener(handler, 100);
        d_event_handler_enable_listener(handler, 100);

        result = d_assert_standalone(
            d_event_handler_enabled_count(handler) == 1,
            "eh_edge_double_enable",
            "Double enable should keep enabled_count at 1",
            _counter) && result;

        d_event_handler_free(handler);

        if (lis)
        {
            d_event_listener_free(lis);
        }
    }

    return result;
}

/*
d_tests_sa_event_handler_advanced_all
  Runs all integration, stress, and edge case tests.
*/
bool
d_tests_sa_event_handler_advanced_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Integration, Stress, and Edge Cases\n");
    printf("  ------------------------------------------------\n");

    result = d_tests_sa_event_handler_integration(_counter)     && result;
    result = d_tests_sa_event_handler_stress(_counter)          && result;
    result = d_tests_sa_event_handler_edge_empty_queue(_counter) && result;
    result = d_tests_sa_event_handler_edge_idempotent(_counter)  && result;

    return result;
}
