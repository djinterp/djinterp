/******************************************************************************
* djinterp [test]                            event_registry_tests_results.cpp
*
*   Section I -- DISPATCH AND RUN RESULTS.  Covers the two result aggregates:
* dispatch_result (the enriched (invoked, outcome) pair, with consumed() true
* exactly when the outcome is verdict::consume) and run_result (the trace
* aggregate of occurrences / handlers_invoked / consumed_count -- a plain data
* holder whose fields are independently stored).
*
*
* path:      /tests/djinterp/core/event/event_registry/event_registry_tests_results.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

// djinterp
#include "event_registry_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_dispatch_result_fields_and_consumed
bool
tests_dispatch_result_fields_and_consumed()
{
    bool ok = true;

    // fields are independently stored.
    dispatch_result r;
    r.invoked = 4;
    r.outcome = verdict::consume;
    ok = D_ER_CHECK(r.invoked == 4u) && ok;

    // consumed() is true exactly for verdict::consume.
    ok = D_ER_CHECK(r.consumed()) && ok;

    r.outcome = verdict::pass;
    ok = D_ER_CHECK(!r.consumed()) && ok;

    return ok;
}


// tests_run_result_aggregate
bool
tests_run_result_aggregate()
{
    bool ok = true;

    // the run aggregate is a plain triple; each field reads back what was
    // written.
    run_result a;
    a.occurrences      = 10;
    a.handlers_invoked = 27;
    a.consumed_count   = 3;

    ok = D_ER_CHECK(a.occurrences == 10u) && ok;
    ok = D_ER_CHECK(a.handlers_invoked == 27u) && ok;
    ok = D_ER_CHECK(a.consumed_count == 3u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
