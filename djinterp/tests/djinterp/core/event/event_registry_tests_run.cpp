/******************************************************************************
* djinterp [test]                              event_registry_tests_run.cpp
*
*   Section III (run) -- the outer fold over a homogeneous trace.  run folds
* dispatch across [first, last), building one payload per element, and returns
* the aggregate (occurrences, handlers_invoked, consumed_count).  Covers the
* empty trace (all zero); a trace with an all-pass handler (occurrences and
* handlers_invoked both equal the trace length, none consumed); a consuming
* handler (every occurrence consumed); a trace with no handlers (occurrences
* counted, nothing invoked); and per-occurrence accumulation of handlers_invoked
* under a mid-word consume (each occurrence contributes only the handlers it
* actually ran before the left zero).
*
*
* path:      /tests/djinterp/core/event/event_registry/event_registry_tests_run.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_registry_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_run_empty_trace
bool
tests_run_empty_trace()
{
    bool ok = true;

    event_registry reg;
    int sink = 0;
    reg.bind<ev_int>(summing{&sink});

    // an empty trace yields a fully-zero aggregate and invokes nothing.
    std::vector<int> trace;
    run_result r = reg.run<ev_int>(trace.begin(), trace.end());

    ok = D_ER_CHECK(r.occurrences == 0u) && ok;
    ok = D_ER_CHECK(r.handlers_invoked == 0u) && ok;
    ok = D_ER_CHECK(r.consumed_count == 0u) && ok;
    ok = D_ER_CHECK(sink == 0) && ok;

    return ok;
}


// tests_run_counts_pass
bool
tests_run_counts_pass()
{
    bool ok = true;

    // one all-pass handler over a four-element trace: four occurrences, four
    // invocations, none consumed; each element's payload is delivered.
    event_registry reg;
    int sink = 0;
    reg.bind<ev_int>(summing{&sink});

    int data[] = { 1, 2, 3, 4 };
    run_result r = reg.run<ev_int>(data, data + 4);

    ok = D_ER_CHECK(r.occurrences == 4u) && ok;
    ok = D_ER_CHECK(r.handlers_invoked == 4u) && ok;
    ok = D_ER_CHECK(r.consumed_count == 0u) && ok;
    ok = D_ER_CHECK(sink == 10) && ok;   // 1+2+3+4

    return ok;
}


// tests_run_consumed_count
bool
tests_run_consumed_count()
{
    bool ok = true;

    // a consuming handler ends every occurrence in a consume.
    event_registry reg;
    reg.bind<ev_int>(unary_rec{0, 1, verdict::consume});

    int data[] = { 1, 2, 3 };
    run_result r = reg.run<ev_int>(data, data + 3);

    ok = D_ER_CHECK(r.occurrences == 3u) && ok;
    ok = D_ER_CHECK(r.handlers_invoked == 3u) && ok;   // one per occurrence
    ok = D_ER_CHECK(r.consumed_count == 3u) && ok;

    return ok;
}


// tests_run_no_handlers
bool
tests_run_no_handlers()
{
    bool ok = true;

    // with no handlers registered, occurrences are still counted but nothing
    // is invoked or consumed.
    event_registry reg;

    int data[] = { 1, 2, 3, 4, 5 };
    run_result r = reg.run<ev_int>(data, data + 5);

    ok = D_ER_CHECK(r.occurrences == 5u) && ok;
    ok = D_ER_CHECK(r.handlers_invoked == 0u) && ok;
    ok = D_ER_CHECK(r.consumed_count == 0u) && ok;

    return ok;
}


// tests_run_short_circuit_accumulation
bool
tests_run_short_circuit_accumulation()
{
    bool ok = true;

    // first handler passes, second consumes, third would run but is cut off;
    // so each occurrence contributes exactly two invocations and one consume.
    event_registry reg;
    reg.bind<ev_int>(unary_rec{0, 1, verdict::pass});
    reg.bind<ev_int>(unary_rec{0, 2, verdict::consume});
    reg.bind<ev_int>(unary_rec{0, 3, verdict::pass});

    int data[] = { 10, 20, 30 };
    run_result r = reg.run<ev_int>(data, data + 3);

    ok = D_ER_CHECK(r.occurrences == 3u) && ok;
    ok = D_ER_CHECK(r.handlers_invoked == 6u) && ok;   // 2 per occurrence * 3
    ok = D_ER_CHECK(r.consumed_count == 3u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
