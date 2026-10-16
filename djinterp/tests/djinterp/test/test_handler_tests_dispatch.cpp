/*******************************************************************************
* djinterp [test]                                test_handler_tests_dispatch.cpp
*
*   Section IV (dispatch) - the typed event facade over the owned dispatcher.
* on() binds a listener and hands back a handler_id; off/enable/disable act on
* that id (and report whether it was found); fire() dispatches immediately and
* returns the invoked count (halting the fold on a consuming listener); queue()
* defers and process[_all]() drain the queue; has_listeners_for() is the O(1)
* presence probe; events() exposes the underlying dispatcher; and the
* D_TEST_ON / D_TEST_FIRE / D_TEST_QUEUE macros mirror the methods.  Custom
* event tags (below) keep these checks off the built-in lifecycle events.
*
*
* path:      /tests/djinterp/test/test_handler_tests_dispatch.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_handler_tests.hpp"

NS_DJINTERP
NS_TESTING

// custom event tags for the facade tests (kept off the lifecycle events).
D_EVENT(ev_val, int);      // one-int payload
D_EVENT_EMPTY(ev_ping);    // empty payload


// tests_dispatch_on_fire_invoked
//   fire invokes a bound listener and reports one invocation; an event with no
// listeners reports zero.
bool
tests_dispatch_on_fire_invoked()
{
    bool ok = true;

    dt::test_handler h;
    int n = 0;
    handler_id id = h.on<ev_ping>([&n]() { ++n; return verdict::pass; });
    (void)id;

    ok = D_TH_CHECK(h.fire<ev_ping>() == 1) && ok;
    ok = D_TH_CHECK(n == 1) && ok;
    ok = D_TH_CHECK(h.fire<ev_val>(7) == 0) && ok;   // no listeners for ev_val

    return ok;
}

// tests_dispatch_fire_multiple
//   fire invokes every enabled listener.
bool
tests_dispatch_fire_multiple()
{
    bool ok = true;

    dt::test_handler h;
    int n = 0;
    h.on<ev_ping>([&n]() { ++n; return verdict::pass; });
    h.on<ev_ping>([&n]() { ++n; return verdict::pass; });
    h.on<ev_ping>([&n]() { ++n; return verdict::pass; });

    ok = D_TH_CHECK(h.fire<ev_ping>() == 3) && ok;
    ok = D_TH_CHECK(n == 3) && ok;

    return ok;
}

// tests_dispatch_fire_consume_halts
//   a consuming listener halts the fold, so a later listener is not reached.
bool
tests_dispatch_fire_consume_halts()
{
    bool ok = true;

    dt::test_handler h;
    int first = 0;
    int second = 0;
    h.on<ev_ping>([&first]()  { ++first;  return verdict::consume; });   // consumes
    h.on<ev_ping>([&second]() { ++second; return verdict::pass;    });   // should not run

    ok = D_TH_CHECK(h.fire<ev_ping>() == 1) && ok;   // fold halted after the consumer
    ok = D_TH_CHECK(first == 1)  && ok;
    ok = D_TH_CHECK(second == 0) && ok;

    return ok;
}

// tests_dispatch_off
//   off removes a listener: found the first time, absent the second, and no
// longer invoked.
bool
tests_dispatch_off()
{
    bool ok = true;

    dt::test_handler h;
    int n = 0;
    handler_id id = h.on<ev_ping>([&n]() { ++n; return verdict::pass; });

    ok = D_TH_CHECK(h.off(id))  && ok;    // found + removed
    ok = D_TH_CHECK(!h.off(id)) && ok;    // already gone
    ok = D_TH_CHECK(h.fire<ev_ping>() == 0) && ok;
    ok = D_TH_CHECK(n == 0) && ok;

    return ok;
}

// tests_dispatch_enable_disable
//   disable suspends a listener from dispatch; enable restores it; both report
// whether the id was found.
bool
tests_dispatch_enable_disable()
{
    bool ok = true;

    dt::test_handler h;
    int n = 0;
    handler_id id = h.on<ev_ping>([&n]() { ++n; return verdict::pass; });

    ok = D_TH_CHECK(h.fire<ev_ping>() == 1) && ok;   // enabled by default
    ok = D_TH_CHECK(h.disable(id))          && ok;
    ok = D_TH_CHECK(h.fire<ev_ping>() == 0) && ok;   // suspended
    ok = D_TH_CHECK(h.enable(id))           && ok;
    ok = D_TH_CHECK(h.fire<ev_ping>() == 1) && ok;   // restored

    h.off(id);
    ok = D_TH_CHECK(!h.disable(id)) && ok;   // gone -> not found
    ok = D_TH_CHECK(!h.enable(id))  && ok;

    return ok;
}

// tests_dispatch_has_listeners_for
//   has_listeners_for tracks whether any listener is bound for an event.
bool
tests_dispatch_has_listeners_for()
{
    bool ok = true;

    dt::test_handler h;
    ok = D_TH_CHECK(!h.has_listeners_for<ev_ping>()) && ok;   // none yet

    handler_id id = h.on<ev_ping>([]() { return verdict::pass; });
    ok = D_TH_CHECK(h.has_listeners_for<ev_ping>())  && ok;   // bound
    ok = D_TH_CHECK(!h.has_listeners_for<ev_val>())  && ok;   // different event

    h.off(id);
    ok = D_TH_CHECK(!h.has_listeners_for<ev_ping>()) && ok;   // unbound

    return ok;
}

// tests_dispatch_queue_process_all
//   queue defers dispatch; process_all delivers every queued occurrence (by
// value) and returns the count.
bool
tests_dispatch_queue_process_all()
{
    bool ok = true;

    dt::test_handler h;
    int sum = 0;
    h.on<ev_val>([&sum](int _x) { sum += _x; return verdict::pass; });

    h.queue<ev_val>(10);
    h.queue<ev_val>(20);
    ok = D_TH_CHECK(sum == 0) && ok;                 // nothing dispatched yet

    ok = D_TH_CHECK(h.process_all() == 2) && ok;
    ok = D_TH_CHECK(sum == 30) && ok;                // both payloads delivered

    return ok;
}

// tests_dispatch_process_partial
//   process(n) dispatches a prefix of n; the remainder stays queued.
bool
tests_dispatch_process_partial()
{
    bool ok = true;

    dt::test_handler h;
    int cnt = 0;
    h.on<ev_val>([&cnt](int) { ++cnt; return verdict::pass; });

    h.queue<ev_val>(1);
    h.queue<ev_val>(2);
    h.queue<ev_val>(3);

    ok = D_TH_CHECK(h.process(2) == 2 && cnt == 2) && ok;   // prefix
    ok = D_TH_CHECK(h.process_all() == 1 && cnt == 3) && ok; // remainder

    return ok;
}

// tests_dispatch_process_empty
//   draining an empty queue dispatches nothing.
bool
tests_dispatch_process_empty()
{
    bool ok = true;

    dt::test_handler h;
    ok = D_TH_CHECK(h.process_all() == 0) && ok;
    ok = D_TH_CHECK(h.process(5) == 0)   && ok;

    return ok;
}

// tests_dispatch_macros
//   the convenience macros mirror on / fire / queue.
bool
tests_dispatch_macros()
{
    bool ok = true;

    dt::test_handler h;
    int fired = 0;
    int queued_sum = 0;

    handler_id id = D_TEST_ON(h, ev_val, ([&fired](int) { ++fired; return verdict::pass; }));
    (void)id;
    ok = D_TH_CHECK(D_TEST_FIRE(h, ev_val, 1) == 1) && ok;   // immediate dispatch with a payload
    ok = D_TH_CHECK(fired == 1) && ok;

    D_TEST_ON(h, ev_val, ([&queued_sum](int _x) { queued_sum += _x; return verdict::pass; }));
    D_TEST_QUEUE(h, ev_val, 5);                              // deferred dispatch with a payload
    h.process_all();
    ok = D_TH_CHECK(queued_sum == 5) && ok;

    return ok;
}

// tests_dispatch_events_accessor
//   events() exposes the one underlying dispatcher (mutable and const yield the
// same object); binding through it is visible via the facade.
bool
tests_dispatch_events_accessor()
{
    bool ok = true;

    dt::test_handler h;
    event_dispatcher&       d  = h.events();
    const dt::test_handler&  ch = h;
    const event_dispatcher& cd = ch.events();
    ok = D_TH_CHECK(&d == &cd) && ok;   // same object

    int n = 0;
    d.bind<ev_ping>([&n]() { ++n; return verdict::pass; });
    ok = D_TH_CHECK(h.fire<ev_ping>() == 1 && n == 1) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
