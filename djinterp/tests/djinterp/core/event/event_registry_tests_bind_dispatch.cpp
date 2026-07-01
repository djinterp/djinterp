/******************************************************************************
* djinterp [test]                      event_registry_tests_bind_dispatch.cpp
*
*   Section III (bind + dispatch) -- typed registration and the inner fold.
* bind type-erases a compatible callable, stores it under the event's key, and
* returns a managing handler_id (valid, distinct per bind, reflected in the
* registry's counts and typed queries).  dispatch builds one occurrence's
* payload and folds seq over the bucket's enabled entries: it returns
* (invoked, outcome); yields (0, pass) when no handler is registered; invokes
* handlers in insertion order; counts invocations; stops early on a consume
* (the left zero), leaving later handlers un-invoked; masks out disabled
* entries; delivers the dispatched arguments to handlers (arity one, two, and
* zero); and normalizes a void-returning handler to pass.  Finally, distinct
* event types are isolated by key, and cv/ref-qualified event spellings
* normalize to the same key (so const ev& dispatch hits ev's handlers).
*
*
* path:      /tests/djinterp/core/event/event_registry/event_registry_tests_bind_dispatch.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_registry_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_bind_returns_valid_id_and_registers
bool
tests_bind_returns_valid_id_and_registers()
{
    bool ok = true;

    event_registry reg;
    std::vector<int> log;

    // a successful bind yields a valid id and registers the handler under the
    // event's key, advancing the counts and typed queries.
    handler_id a = reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    ok = D_ER_CHECK(a.is_valid()) && ok;
    ok = D_ER_CHECK(reg.contains(a)) && ok;
    ok = D_ER_CHECK(reg.is_enabled(a)) && ok;
    ok = D_ER_CHECK(reg.handler_count() == 1u) && ok;
    ok = D_ER_CHECK(reg.enabled_count() == 1u) && ok;
    ok = D_ER_CHECK(reg.handler_count_for<ev_int>() == 1u) && ok;
    ok = D_ER_CHECK(reg.has_handlers_for<ev_int>()) && ok;
    ok = D_ER_CHECK(reg.type_count() == 1u) && ok;

    return ok;
}


// tests_bind_distinct_ids
bool
tests_bind_distinct_ids()
{
    bool ok = true;

    event_registry reg;
    std::vector<int> log;

    // each bind hands out a distinct id, even within the same event bucket.
    handler_id a = reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    handler_id b = reg.bind<ev_int>(unary_rec{&log, 2, verdict::pass});
    handler_id c = reg.bind<ev_two>(binary_rec{0, 0});

    ok = D_ER_CHECK(a.value != b.value) && ok;
    ok = D_ER_CHECK(b.value != c.value) && ok;
    ok = D_ER_CHECK(a.value != c.value) && ok;
    ok = D_ER_CHECK(reg.handler_count() == 3u) && ok;

    return ok;
}


// tests_dispatch_no_handlers
bool
tests_dispatch_no_handlers()
{
    bool ok = true;

    // dispatching an event with no registered handlers invokes nothing and
    // passes.
    event_registry reg;
    dispatch_result r = reg.dispatch<ev_int>(7);

    ok = D_ER_CHECK(r.invoked == 0u) && ok;
    ok = D_ER_CHECK(r.outcome == verdict::pass) && ok;
    ok = D_ER_CHECK(!r.consumed()) && ok;

    return ok;
}


// tests_dispatch_single_pass
bool
tests_dispatch_single_pass()
{
    bool ok = true;

    event_registry reg;
    std::vector<int> log;
    reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});

    dispatch_result r = reg.dispatch<ev_int>(0);
    ok = D_ER_CHECK(r.invoked == 1u) && ok;
    ok = D_ER_CHECK(r.outcome == verdict::pass) && ok;
    ok = D_ER_CHECK(log.size() == 1u && log[0] == 1) && ok;

    return ok;
}


// tests_dispatch_single_consume
bool
tests_dispatch_single_consume()
{
    bool ok = true;

    event_registry reg;
    std::vector<int> log;
    reg.bind<ev_int>(unary_rec{&log, 1, verdict::consume});

    dispatch_result r = reg.dispatch<ev_int>(0);
    ok = D_ER_CHECK(r.invoked == 1u) && ok;
    ok = D_ER_CHECK(r.outcome == verdict::consume) && ok;
    ok = D_ER_CHECK(r.consumed()) && ok;

    return ok;
}


// tests_dispatch_order_and_count
bool
tests_dispatch_order_and_count()
{
    bool ok = true;

    // three all-pass handlers run in insertion order; all are counted.
    event_registry reg;
    std::vector<int> log;
    reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    reg.bind<ev_int>(unary_rec{&log, 2, verdict::pass});
    reg.bind<ev_int>(unary_rec{&log, 3, verdict::pass});

    dispatch_result r = reg.dispatch<ev_int>(0);
    ok = D_ER_CHECK(r.invoked == 3u) && ok;
    ok = D_ER_CHECK(!r.consumed()) && ok;
    ok = D_ER_CHECK(log.size() == 3u) && ok;
    if (log.size() == 3u)
    {
        ok = D_ER_CHECK(log[0] == 1 && log[1] == 2 && log[2] == 3) && ok;
    }

    return ok;
}


// tests_dispatch_consume_short_circuit
bool
tests_dispatch_consume_short_circuit()
{
    bool ok = true;

    // the second handler consumes, so the third never runs and only two are
    // counted.
    event_registry reg;
    std::vector<int> log;
    reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    reg.bind<ev_int>(unary_rec{&log, 2, verdict::consume});
    reg.bind<ev_int>(unary_rec{&log, 3, verdict::pass});

    dispatch_result r = reg.dispatch<ev_int>(0);
    ok = D_ER_CHECK(r.invoked == 2u) && ok;
    ok = D_ER_CHECK(r.consumed()) && ok;
    ok = D_ER_CHECK(log.size() == 2u) && ok;
    if (log.size() == 2u)
    {
        ok = D_ER_CHECK(log[0] == 1 && log[1] == 2) && ok;
    }

    return ok;
}


// tests_dispatch_disabled_masked
bool
tests_dispatch_disabled_masked()
{
    bool ok = true;

    // a disabled handler is masked out of the fold: it is neither invoked nor
    // counted, while the remaining enabled handler runs.
    event_registry reg;
    std::vector<int> log;
    handler_id a = reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    reg.bind<ev_int>(unary_rec{&log, 2, verdict::pass});
    reg.disable(a);

    dispatch_result r = reg.dispatch<ev_int>(0);
    ok = D_ER_CHECK(r.invoked == 1u) && ok;
    ok = D_ER_CHECK(log.size() == 1u && log[0] == 2) && ok;

    // re-enabling restores it to the fold.
    reg.enable(a);
    log.clear();
    dispatch_result r2 = reg.dispatch<ev_int>(0);
    ok = D_ER_CHECK(r2.invoked == 2u) && ok;

    return ok;
}


// tests_dispatch_payload_delivery
bool
tests_dispatch_payload_delivery()
{
    bool ok = true;

    // arity-one: the dispatched value reaches the handler.
    event_registry reg;
    int sink = 0;
    reg.bind<ev_int>(summing{&sink});
    reg.dispatch<ev_int>(5);
    ok = D_ER_CHECK(sink == 5) && ok;
    reg.dispatch<ev_int>(37);
    ok = D_ER_CHECK(sink == 42) && ok;

    // arity-two: both dispatched values reach the handler in order.
    event_registry reg2;
    int a = 0;
    int b = 0;
    reg2.bind<ev_two>(binary_rec{&a, &b});
    reg2.dispatch<ev_two>(11, 22);
    ok = D_ER_CHECK(a == 11 && b == 22) && ok;

    return ok;
}


// tests_dispatch_void_handler_and_nullary
bool
tests_dispatch_void_handler_and_nullary()
{
    bool ok = true;

    // a void-returning handler is normalized to pass and still counted.
    event_registry reg;
    int vn = 0;
    reg.bind<ev_int>(void_h{&vn});
    dispatch_result r = reg.dispatch<ev_int>(0);
    ok = D_ER_CHECK(r.invoked == 1u) && ok;
    ok = D_ER_CHECK(r.outcome == verdict::pass) && ok;
    ok = D_ER_CHECK(vn == 1) && ok;

    // an empty-payload event dispatches with no arguments.
    event_registry reg2;
    int nn = 0;
    reg2.bind<ev_none>(nullary_rec{&nn});
    dispatch_result r2 = reg2.dispatch<ev_none>();
    ok = D_ER_CHECK(r2.invoked == 1u) && ok;
    ok = D_ER_CHECK(nn == 1) && ok;

    return ok;
}


// tests_dispatch_multi_event_isolation_and_cvref
bool
tests_dispatch_multi_event_isolation_and_cvref()
{
    bool ok = true;

    event_registry reg;
    std::vector<int> log;
    int a = 0;
    int b = 0;
    reg.bind<ev_int>(unary_rec{&log, 1, verdict::pass});
    reg.bind<ev_two>(binary_rec{&a, &b});

    // two distinct event types, isolated by key.
    ok = D_ER_CHECK(reg.type_count() == 2u) && ok;

    // dispatching ev_int fires only ev_int's handler (not ev_two's).
    dispatch_result r = reg.dispatch<ev_int>(9);
    ok = D_ER_CHECK(r.invoked == 1u) && ok;
    ok = D_ER_CHECK(a == 0 && b == 0) && ok;   // ev_two handler untouched

    // a cv/ref-qualified event spelling normalizes (clean_t) to the same key,
    // so it reaches the same handler and the same typed query bucket.
    log.clear();
    dispatch_result r2 = reg.dispatch<const ev_int&>(3);
    ok = D_ER_CHECK(r2.invoked == 1u) && ok;
    ok = D_ER_CHECK(log.size() == 1u && log[0] == 1) && ok;
    ok = D_ER_CHECK(reg.handler_count_for<const ev_int&>() == 1u) && ok;
    ok = D_ER_CHECK(reg.has_handlers_for<ev_int&>()) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
