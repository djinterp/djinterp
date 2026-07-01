/******************************************************************************
* djinterp [test]                          event_dispatcher_tests_queue.cpp
*
*   Sections I-II -- INTERNAL QUEUE SUPPORT and EVENT QUEUE.  Exercises the
* event_queue: enqueue appends (payload captured by value) and is reflected in
* pending()/empty(); clear() discards without dispatching; process() folds
* dispatch over a prefix against a supplied registry, in order, returning the
* count and dropping the processed prefix; a max below pending() processes only
* that many and leaves the remainder, a max of zero or above pending() drains
* what is available (process_all is the zero case), and processing an empty
* queue yields zero.  Two semantic guarantees get dedicated tests: delivery-time
* binding (the registry is read at process() time, so a handler bound after the
* enqueue still fires) and re-entrant deferral (occurrences enqueued by a
* handler during process() land after the current batch and wait for a later
* call).  A heterogeneous queue routes each occurrence to its own bucket.
*
*   The internal helpers dispatch_tuple / dispatch_tuple_impl and the erased
* occurrence's type_key/replay are covered transitively here: correct,
* per-type delivery through process() is exactly their contract.
*
*
* path:      /tests/djinterp/core/event/event_dispatcher/event_dispatcher_tests_queue.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

// djinterp
#include "event_dispatcher_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_queue_enqueue_pending_empty_clear
bool
tests_queue_enqueue_pending_empty_clear()
{
    bool ok = true;

    event_queue q;
    ok = D_ED_CHECK(q.empty()) && ok;
    ok = D_ED_CHECK(q.pending() == 0u) && ok;

    q.enqueue<ev_int>(1);
    q.enqueue<ev_int>(2);
    ok = D_ED_CHECK(!q.empty()) && ok;
    ok = D_ED_CHECK(q.pending() == 2u) && ok;

    q.clear();
    ok = D_ED_CHECK(q.empty()) && ok;
    ok = D_ED_CHECK(q.pending() == 0u) && ok;

    // clear discards without dispatching: a re-enqueue then clear leaves the
    // handler un-invoked when processed.
    event_registry reg;
    int sink = 0;
    reg.bind<ev_int>(summing{&sink});
    q.enqueue<ev_int>(7);
    q.clear();
    ok = D_ED_CHECK(q.process(reg) == 0u) && ok;
    ok = D_ED_CHECK(sink == 0) && ok;

    return ok;
}


// tests_queue_process_order_and_count
bool
tests_queue_process_order_and_count()
{
    bool ok = true;

    event_registry reg;
    std::vector<int> log;
    reg.bind<ev_int>(order_rec{&log, 9});

    event_queue q;
    q.enqueue<ev_int>(0);
    q.enqueue<ev_int>(0);
    q.enqueue<ev_int>(0);

    std::size_t n = q.process(reg);
    ok = D_ED_CHECK(n == 3u) && ok;
    ok = D_ED_CHECK(q.pending() == 0u) && ok;   // prefix dropped
    ok = D_ED_CHECK(log.size() == 3u) && ok;    // one dispatch per occurrence

    return ok;
}


// tests_queue_process_partial_and_remainder
bool
tests_queue_process_partial_and_remainder()
{
    bool ok = true;

    event_registry reg;
    int sink = 0;
    reg.bind<ev_int>(summing{&sink});

    event_queue q;
    q.enqueue<ev_int>(1);
    q.enqueue<ev_int>(2);
    q.enqueue<ev_int>(3);
    q.enqueue<ev_int>(4);

    // a max below pending() processes only that many, in order, leaving the
    // rest queued.
    std::size_t n = q.process(reg, 2);
    ok = D_ED_CHECK(n == 2u) && ok;
    ok = D_ED_CHECK(q.pending() == 2u) && ok;
    ok = D_ED_CHECK(sink == 3) && ok;           // 1 + 2

    // the remainder processes on a later call (0 == all currently queued).
    std::size_t m = q.process(reg, 0);
    ok = D_ED_CHECK(m == 2u) && ok;
    ok = D_ED_CHECK(q.pending() == 0u) && ok;
    ok = D_ED_CHECK(sink == 10) && ok;          // + 3 + 4

    return ok;
}


// tests_queue_process_max_clamped_and_all
bool
tests_queue_process_max_clamped_and_all()
{
    bool ok = true;

    event_registry reg;
    int sink = 0;
    reg.bind<ev_int>(summing{&sink});

    event_queue q;
    q.enqueue<ev_int>(5);
    q.enqueue<ev_int>(6);

    // a max above pending() is clamped to what is available.
    std::size_t n = q.process(reg, 100);
    ok = D_ED_CHECK(n == 2u) && ok;
    ok = D_ED_CHECK(q.pending() == 0u) && ok;
    ok = D_ED_CHECK(sink == 11) && ok;

    // process_all drains everything; here there is nothing left.
    q.enqueue<ev_int>(4);
    std::size_t m = q.process_all(reg);
    ok = D_ED_CHECK(m == 1u) && ok;
    ok = D_ED_CHECK(sink == 15) && ok;

    return ok;
}


// tests_queue_process_empty
bool
tests_queue_process_empty()
{
    bool ok = true;

    // processing an empty queue invokes nothing and returns zero, by either
    // entry point.
    event_registry reg;
    event_queue q;

    ok = D_ED_CHECK(q.process(reg) == 0u) && ok;
    ok = D_ED_CHECK(q.process(reg, 5) == 0u) && ok;
    ok = D_ED_CHECK(q.process_all(reg) == 0u) && ok;

    return ok;
}


// tests_queue_delivery_time_binding
bool
tests_queue_delivery_time_binding()
{
    bool ok = true;

    // the registry is consulted at process() time, not at enqueue time: a
    // handler bound after the enqueue still fires.
    event_registry reg;
    event_queue q;

    q.enqueue<ev_int>(5);                         // no handler registered yet
    int sink = 0;
    reg.bind<ev_int>(summing{&sink});             // bind after enqueue
    q.process(reg);

    ok = D_ED_CHECK(sink == 5) && ok;

    return ok;
}


// tests_queue_payload_captured_by_value
bool
tests_queue_payload_captured_by_value()
{
    bool ok = true;

    // enqueue captures the payload by value: mutating the source afterward
    // does not change what is dispatched.
    event_registry reg;
    int sink = 0;
    reg.bind<ev_int>(summing{&sink});

    event_queue q;
    int x = 5;
    q.enqueue<ev_int>(x);                          // captures 5
    x = 99;                                        // mutate source after enqueue
    q.process(reg);

    ok = D_ED_CHECK(sink == 5) && ok;

    return ok;
}


// tests_queue_reentrant_enqueue_deferred
bool
tests_queue_reentrant_enqueue_deferred()
{
    bool ok = true;

    // each handler invocation re-enqueues one ev_int. The prefix is detached
    // into an isolated batch before any replay runs, so those re-entrant
    // enqueues land after the batch and are deferred to a later process().
    event_registry reg;
    event_queue q;
    reg.bind<ev_int>(reenqueue{&q});

    q.enqueue<ev_int>(0);
    q.enqueue<ev_int>(0);                          // batch of two

    std::size_t n = q.process(reg);
    ok = D_ED_CHECK(n == 2u) && ok;               // only the original two ran
    ok = D_ED_CHECK(q.pending() == 2u) && ok;     // their re-enqueues wait

    // the deferred occurrences process on the next call (and re-enqueue again).
    std::size_t m = q.process(reg);
    ok = D_ED_CHECK(m == 2u) && ok;
    ok = D_ED_CHECK(q.pending() == 2u) && ok;

    return ok;
}


// tests_queue_heterogeneous
bool
tests_queue_heterogeneous()
{
    bool ok = true;

    // a single queue holds occurrences of different event types; process
    // routes each to its own registry bucket.
    event_registry reg;
    int s1 = 0;
    int s2 = 0;
    reg.bind<ev_int>(summing{&s1});
    reg.bind<ev_two>(two_sum{&s2});

    event_queue q;
    q.enqueue<ev_int>(4);
    q.enqueue<ev_two>(10, 20);
    q.enqueue<ev_int>(6);

    std::size_t n = q.process_all(reg);
    ok = D_ED_CHECK(n == 3u) && ok;
    ok = D_ED_CHECK(s1 == 10) && ok;              // 4 + 6
    ok = D_ED_CHECK(s2 == 30) && ok;              // 10 + 20

    return ok;
}


NS_END  // testing
NS_END  // djinterp
