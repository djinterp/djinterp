/******************************************************************************
* djinterp [test]                          event_dispatcher_tests_drive.cpp
*
*   Section IV -- FUSED DRIVE.  drive runs a homogeneous trace through a
* precompiled fused_step as a single loop, reporting only the occurrence and
* consume counts (the fused path is intentionally not instrumented per
* handler).  Covers the drive_result fields; an empty trace (all zero); an
* ordinary drive (occurrences equal the trace length, side effects delivered);
* a drive of an empty step (occurrences still counted, none consumed); and the
* coherence law -- for a registry held fixed, driving the compiled step over a
* trace agrees with running the same trace through the registry on both
* occurrences and consumed_count, with identical per-occurrence side effects.
*
*
* path:      /tests/djinterp/core/event/event_dispatcher/event_dispatcher_tests_drive.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

// djinterp
#include "event_dispatcher_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_drive_result_fields
bool
tests_drive_result_fields()
{
    bool ok = true;

    // the drive aggregate is a plain pair; each field reads back what was
    // written.
    drive_result r;
    r.occurrences    = 7;
    r.consumed_count = 2;

    ok = D_ED_CHECK(r.occurrences == 7u) && ok;
    ok = D_ED_CHECK(r.consumed_count == 2u) && ok;

    return ok;
}


// tests_drive_empty_trace
bool
tests_drive_empty_trace()
{
    bool ok = true;

    event_dispatcher d;
    int sink = 0;
    d.bind<ev_int>(summing{&sink});
    fused_step<ev_int> step = d.compile<ev_int>();

    // an empty trace drives nothing.
    std::vector<int> trace;
    drive_result r = drive(step, trace.begin(), trace.end());

    ok = D_ED_CHECK(r.occurrences == 0u) && ok;
    ok = D_ED_CHECK(r.consumed_count == 0u) && ok;
    ok = D_ED_CHECK(sink == 0) && ok;

    return ok;
}


// tests_drive_counts_and_consumed
bool
tests_drive_counts_and_consumed()
{
    bool ok = true;

    // all-pass: occurrences counted, none consumed, side effects delivered.
    event_dispatcher d;
    int sink = 0;
    d.bind<ev_int>(summing{&sink});
    fused_step<ev_int> step = d.compile<ev_int>();

    int data[] = { 1, 2, 3, 4 };
    drive_result r = drive(step, data, data + 4);
    ok = D_ED_CHECK(r.occurrences == 4u) && ok;
    ok = D_ED_CHECK(r.consumed_count == 0u) && ok;
    ok = D_ED_CHECK(sink == 10) && ok;

    // a consuming handler makes every occurrence consume.
    event_dispatcher dc;
    int n = 0;
    dc.bind<ev_int>(consumer{&n});
    fused_step<ev_int> step_c = dc.compile<ev_int>();
    drive_result rc = drive(step_c, data, data + 4);
    ok = D_ED_CHECK(rc.occurrences == 4u) && ok;
    ok = D_ED_CHECK(rc.consumed_count == 4u) && ok;

    return ok;
}


// tests_drive_empty_step
bool
tests_drive_empty_step()
{
    bool ok = true;

    // a step compiled from a registry with no handlers folds to pass for every
    // occurrence: occurrences are still counted, none consumed.
    event_dispatcher d;
    fused_step<ev_int> empty_step = d.compile<ev_int>();
    ok = D_ED_CHECK(empty_step.size() == 0u) && ok;

    int data[] = { 1, 2, 3 };
    drive_result r = drive(empty_step, data, data + 3);
    ok = D_ED_CHECK(r.occurrences == 3u) && ok;
    ok = D_ED_CHECK(r.consumed_count == 0u) && ok;

    return ok;
}


// tests_drive_coherence_with_run
bool
tests_drive_coherence_with_run()
{
    bool ok = true;

    // COHERENCE LAW: for a fixed registry, drive(compile(), ...) agrees with
    // run(...) on occurrences and consumed_count, with identical side effects.
    // Two dispatchers with identical bindings keep the side-effect sinks
    // separate so they can be compared directly.
    int data[] = { 1, 2, 3 };

    event_dispatcher dd;
    int sd = 0;
    dd.bind<ev_int>(summing{&sd});
    dd.bind<ev_int>(consumer{0});                 // second handler consumes
    fused_step<ev_int> step = dd.compile<ev_int>();
    drive_result dr = drive(step, data, data + 3);

    event_dispatcher dr2;
    int sr = 0;
    dr2.bind<ev_int>(summing{&sr});
    dr2.bind<ev_int>(consumer{0});
    run_result rr = dr2.run<ev_int>(data, data + 3);

    ok = D_ED_CHECK(dr.occurrences == rr.occurrences) && ok;
    ok = D_ED_CHECK(dr.consumed_count == rr.consumed_count) && ok;
    ok = D_ED_CHECK(dr.consumed_count == 3u) && ok;   // each occurrence consumed
    ok = D_ED_CHECK(sd == sr) && ok;                  // identical side effects
    ok = D_ED_CHECK(sd == 6) && ok;                   // 1 + 2 + 3 (summing runs first)

    return ok;
}


NS_END  // testing
NS_END  // djinterp
