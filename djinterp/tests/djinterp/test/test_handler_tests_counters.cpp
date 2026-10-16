/*******************************************************************************
* djinterp [test]                                test_handler_tests_counters.cpp
*
*   Section IV (counters) - the pull-based bookkeeping side of the handler.
* record() routes a status through increment_for into the matching per-session
* counter and always bumps the running total, without firing any event; the
* per-status accessors and result() surface those counters, reset_counters()
* zeroes them, and result() hands back an independent by-value snapshot.  Every
* status arm of increment_for is exercised here.
*
*
* path:      /tests/djinterp/test/test_handler_tests_counters.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_handler_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_counters_record_each_status
//   record routes each of the five statuses to its own counter and bumps the
// total each time (covers every increment_for arm).
bool
tests_counters_record_each_status()
{
    bool ok = true;

    dt::test_handler h;
    h.record(dt::test_status::passed);
    h.record(dt::test_status::failed);
    h.record(dt::test_status::skipped);
    h.record(dt::test_status::error);
    h.record(dt::test_status::pending);

    ok = D_TH_CHECK(h.passed()  == 1) && ok;
    ok = D_TH_CHECK(h.failed()  == 1) && ok;
    ok = D_TH_CHECK(h.skipped() == 1) && ok;
    ok = D_TH_CHECK(h.errors()  == 1) && ok;
    ok = D_TH_CHECK(h.pending() == 1) && ok;
    ok = D_TH_CHECK(h.total()   == 5) && ok;

    return ok;
}

// tests_counters_total_accumulates
//   repeated records accumulate into their counters and the total.
bool
tests_counters_total_accumulates()
{
    bool ok = true;

    dt::test_handler h;
    h.record(dt::test_status::passed);
    h.record(dt::test_status::passed);
    h.record(dt::test_status::failed);

    ok = D_TH_CHECK(h.passed() == 2) && ok;
    ok = D_TH_CHECK(h.failed() == 1) && ok;
    ok = D_TH_CHECK(h.total()  == 3) && ok;

    return ok;
}

// tests_counters_reset
//   reset_counters zeroes every counter (and the snapshot reflects it).
bool
tests_counters_reset()
{
    bool ok = true;

    dt::test_handler h;
    h.record(dt::test_status::passed);
    h.record(dt::test_status::error);
    h.reset_counters();

    ok = D_TH_CHECK(h.total()   == 0) && ok;
    ok = D_TH_CHECK(h.passed()  == 0) && ok;
    ok = D_TH_CHECK(h.errors()  == 0) && ok;

    dt::session_result r = h.result();
    ok = D_TH_CHECK(r.total == 0) && ok;

    return ok;
}

// tests_counters_accessors_reflect
//   the per-status accessors mirror the corresponding result() fields.
bool
tests_counters_accessors_reflect()
{
    bool ok = true;

    dt::test_handler h;
    h.record(dt::test_status::skipped);
    h.record(dt::test_status::pending);
    h.record(dt::test_status::pending);

    dt::session_result r = h.result();
    ok = D_TH_CHECK(h.passed()  == r.passed)  && ok;
    ok = D_TH_CHECK(h.failed()  == r.failed)  && ok;
    ok = D_TH_CHECK(h.skipped() == r.skipped && r.skipped == 1) && ok;
    ok = D_TH_CHECK(h.errors()  == r.errors)  && ok;
    ok = D_TH_CHECK(h.pending() == r.pending && r.pending == 2) && ok;
    ok = D_TH_CHECK(h.total()   == r.total   && r.total   == 3) && ok;

    return ok;
}

// tests_counters_result_snapshot
//   result() returns a by-value snapshot: later mutation of the handler does
// not disturb a previously captured result.
bool
tests_counters_result_snapshot()
{
    bool ok = true;

    dt::test_handler h;
    h.record(dt::test_status::passed);
    dt::session_result snap = h.result();   // captured snapshot

    h.record(dt::test_status::passed);      // handler advances afterward
    ok = D_TH_CHECK(snap.passed == 1) && ok;   // snapshot is frozen
    ok = D_TH_CHECK(h.passed()  == 2) && ok;   // handler moved on

    return ok;
}


NS_END  // testing
NS_END  // djinterp
