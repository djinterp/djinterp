/******************************************************************************
* djinterp [test]                event_dispatcher_tests_dispatcher_defer.cpp
*
*   Section III (deferred surface) -- the event_dispatcher facade's
* asynchronous half and its integration with the registry.  queue enqueues an
* occurrence (reflected in pending_events and the events() sub-queue); process
* and process_all drain the queue against the dispatcher's own registry,
* returning the count processed; a max bounds a partial drain.  The defining
* integration test pairs the two halves: queue an occurrence, bind a handler
* afterward, then process -- the handler fires, because the queue is processed
* against the registry as it stands at process() time.  Finally, fire and queue
* are contrasted: fire delivers immediately, queue defers until processed.
*
*
* path:      /tests/djinterp/core/event/event_dispatcher/event_dispatcher_tests_dispatcher_defer.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

// djinterp
#include "event_dispatcher_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_dispatcher_queue_pending
bool
tests_dispatcher_queue_pending()
{
    bool ok = true;

    event_dispatcher d;
    ok = D_ED_CHECK(d.pending_events() == 0u) && ok;

    d.queue<ev_int>(1);
    d.queue<ev_int>(2);
    ok = D_ED_CHECK(d.pending_events() == 2u) && ok;
    // the same count is visible through the events() sub-queue.
    ok = D_ED_CHECK(d.events().pending() == 2u) && ok;

    return ok;
}


// tests_dispatcher_process_dispatches
bool
tests_dispatcher_process_dispatches()
{
    bool ok = true;

    // queued occurrences dispatch against the dispatcher's own registry.
    event_dispatcher d;
    int sink = 0;
    d.bind<ev_int>(summing{&sink});

    d.queue<ev_int>(3);
    d.queue<ev_int>(4);
    std::size_t n = d.process();
    ok = D_ED_CHECK(n == 2u) && ok;
    ok = D_ED_CHECK(sink == 7) && ok;
    ok = D_ED_CHECK(d.pending_events() == 0u) && ok;

    return ok;
}


// tests_dispatcher_queue_then_bind_then_process
bool
tests_dispatcher_queue_then_bind_then_process()
{
    bool ok = true;

    // delivery-time binding through the facade: queue first, bind second,
    // process third -- the late-bound handler still fires.
    event_dispatcher d;
    d.queue<ev_int>(8);

    int sink = 0;
    d.bind<ev_int>(summing{&sink});

    std::size_t n = d.process_all();
    ok = D_ED_CHECK(n == 1u) && ok;
    ok = D_ED_CHECK(sink == 8) && ok;

    return ok;
}


// tests_dispatcher_process_partial_and_all
bool
tests_dispatcher_process_partial_and_all()
{
    bool ok = true;

    event_dispatcher d;
    int sink = 0;
    d.bind<ev_int>(summing{&sink});

    d.queue<ev_int>(1);
    d.queue<ev_int>(2);
    d.queue<ev_int>(3);

    // process(max) drains a bounded prefix.
    std::size_t n = d.process(2);
    ok = D_ED_CHECK(n == 2u) && ok;
    ok = D_ED_CHECK(d.pending_events() == 1u) && ok;
    ok = D_ED_CHECK(sink == 3) && ok;           // 1 + 2

    // process_all drains the rest.
    std::size_t m = d.process_all();
    ok = D_ED_CHECK(m == 1u) && ok;
    ok = D_ED_CHECK(d.pending_events() == 0u) && ok;
    ok = D_ED_CHECK(sink == 6) && ok;           // + 3

    return ok;
}


// tests_dispatcher_fire_vs_queue
bool
tests_dispatcher_fire_vs_queue()
{
    bool ok = true;

    // fire delivers now; queue defers until processed.
    event_dispatcher d;
    int sink = 0;
    d.bind<ev_int>(summing{&sink});

    d.fire<ev_int>(3);
    ok = D_ED_CHECK(sink == 3) && ok;           // immediate

    d.queue<ev_int>(4);
    ok = D_ED_CHECK(sink == 3) && ok;           // not yet delivered
    ok = D_ED_CHECK(d.pending_events() == 1u) && ok;

    d.process();
    ok = D_ED_CHECK(sink == 7) && ok;           // delivered on process

    return ok;
}


NS_END  // testing
NS_END  // djinterp
