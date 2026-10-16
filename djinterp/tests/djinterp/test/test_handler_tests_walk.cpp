/*******************************************************************************
* djinterp [test]                                    test_handler_tests_walk.cpp
*
*   Section IV (walk) - run() and the private walk it drives.  A node's status
* is authoritative; the walk reads it, fires the matching lifecycle events, and
* tallies - it evaluates nothing.  Leaf nodes (type_id <= 1) produce a
* test_start / status-event / test_end triple and increment a counter; interior
* nodes (type_id >= 2) produce a module_start / module_end pair and are not
* tallied; pending (and any out-of-range status code, via to_test_status'
* default) fires no status event; and run() brackets the whole walk with
* on_session_start / on_session_end (the latter carrying the pass/fail counts),
* resetting the counters up front.  The lifecycle_recorder witnesses which
* events fired, in what order; the counters are read directly.
*
*
* path:      /tests/djinterp/test/test_handler_tests_walk.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_handler_tests.hpp"

NS_DJINTERP
NS_TESTING


// tests_walk_leaf_status_counters
//   one leaf per status tallies each counter and the running total (covers
// to_test_status and increment_for across all five statuses).
bool
tests_walk_leaf_status_counters()
{
    bool ok = true;

    dt::test_handler h;
    std::vector<dt::basic_test> tree;
    tree.push_back(make_leaf(dt::test_status::passed));
    tree.push_back(make_leaf(dt::test_status::failed));
    tree.push_back(make_leaf(dt::test_status::skipped));
    tree.push_back(make_leaf(dt::test_status::pending));
    tree.push_back(make_leaf(dt::test_status::error));
    h.run(tree);

    ok = D_TH_CHECK(h.passed()  == 1) && ok;
    ok = D_TH_CHECK(h.failed()  == 1) && ok;
    ok = D_TH_CHECK(h.skipped() == 1) && ok;
    ok = D_TH_CHECK(h.pending() == 1) && ok;
    ok = D_TH_CHECK(h.errors()  == 1) && ok;
    ok = D_TH_CHECK(h.total()   == 5) && ok;

    return ok;
}

// tests_walk_leaf_events_order
//   a passed leaf fires start, its status event, then end - bracketed by the
// session events - in that order and nothing else.
bool
tests_walk_leaf_events_order()
{
    bool ok = true;

    lifecycle_recorder rec;
    dt::test_handler   h;
    rec.bind_all(h);

    std::vector<dt::basic_test> tree;
    tree.push_back(make_leaf(dt::test_status::passed));
    h.run(tree);

    ok = D_TH_CHECK(rec.test_start == 1) && ok;
    ok = D_TH_CHECK(rec.passed == 1)     && ok;
    ok = D_TH_CHECK(rec.test_end == 1)   && ok;
    ok = D_TH_CHECK(rec.failed == 0 && rec.skipped == 0 && rec.error == 0) && ok;
    ok = D_TH_CHECK(rec.module_start == 0 && rec.module_end == 0)          && ok;

    // order: session_start, test_start, passed, test_end, session_end
    ok = D_TH_CHECK(rec.order.size() == 5) && ok;
    if (rec.order.size() == 5)
    {
        ok = D_TH_CHECK(rec.order[0] == lifecycle_recorder::m_session_start) && ok;
        ok = D_TH_CHECK(rec.order[1] == lifecycle_recorder::m_test_start)    && ok;
        ok = D_TH_CHECK(rec.order[2] == lifecycle_recorder::m_passed)        && ok;
        ok = D_TH_CHECK(rec.order[3] == lifecycle_recorder::m_test_end)      && ok;
        ok = D_TH_CHECK(rec.order[4] == lifecycle_recorder::m_session_end)   && ok;
    }

    return ok;
}

// tests_walk_status_event_mapping
//   failed / skipped / error leaves each fire only their matching status event.
bool
tests_walk_status_event_mapping()
{
    bool ok = true;

    {
        lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
        std::vector<dt::basic_test> t; t.push_back(make_leaf(dt::test_status::failed));
        h.run(t);
        ok = D_TH_CHECK(rec.failed == 1 && rec.passed == 0 && rec.skipped == 0 && rec.error == 0) && ok;
    }
    {
        lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
        std::vector<dt::basic_test> t; t.push_back(make_leaf(dt::test_status::skipped));
        h.run(t);
        ok = D_TH_CHECK(rec.skipped == 1 && rec.passed == 0 && rec.failed == 0 && rec.error == 0) && ok;
    }
    {
        lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
        std::vector<dt::basic_test> t; t.push_back(make_leaf(dt::test_status::error));
        h.run(t);
        ok = D_TH_CHECK(rec.error == 1 && rec.passed == 0 && rec.failed == 0 && rec.skipped == 0) && ok;
    }

    return ok;
}

// tests_walk_pending_no_status_event
//   a pending leaf is bracketed by test start/end but fires no status event,
// yet is still counted.
bool
tests_walk_pending_no_status_event()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    std::vector<dt::basic_test> t; t.push_back(make_leaf(dt::test_status::pending));
    h.run(t);

    ok = D_TH_CHECK(rec.test_start == 1 && rec.test_end == 1) && ok;
    ok = D_TH_CHECK(rec.passed == 0 && rec.failed == 0 && rec.skipped == 0 && rec.error == 0) && ok;
    ok = D_TH_CHECK(h.pending() == 1) && ok;

    return ok;
}

// tests_walk_interior_module_events
//   an interior node fires module start/end, no test/status events, and is not
// tallied.
bool
tests_walk_interior_module_events()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    std::vector<dt::basic_test> t; t.push_back(make_interior());
    h.run(t);

    ok = D_TH_CHECK(rec.module_start == 1 && rec.module_end == 1) && ok;
    ok = D_TH_CHECK(rec.test_start == 0 && rec.test_end == 0)     && ok;
    ok = D_TH_CHECK(rec.passed == 0 && rec.failed == 0)           && ok;
    ok = D_TH_CHECK(h.total() == 0)                              && ok;   // interior not counted

    return ok;
}

// tests_walk_leaf_vs_interior_boundary
//   the leaf/interior boundary sits at type_id <= 1: ids 0 and 1 are leaves,
// id 2 is interior.
bool
tests_walk_leaf_vs_interior_boundary()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    std::vector<dt::basic_test> t;
    t.push_back(make_leaf_id(dt::test_type_id{0}));   // leaf, passed
    t.push_back(make_leaf_id(dt::test_type_id{1}));   // leaf, passed
    t.push_back(make_interior());                     // interior (type_id 2)
    h.run(t);

    ok = D_TH_CHECK(rec.test_start == 2)   && ok;   // two leaves entered
    ok = D_TH_CHECK(rec.module_start == 1) && ok;   // one interior entered
    ok = D_TH_CHECK(h.passed() == 2)       && ok;
    ok = D_TH_CHECK(h.total() == 2)        && ok;   // interior not counted

    return ok;
}

// tests_walk_to_test_status_default
//   an out-of-range status code falls through to_test_status' default and is
// treated as pending (no status event, pending counter bumped).
bool
tests_walk_to_test_status_default()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    std::vector<dt::basic_test> t;
    t.push_back(make_leaf_raw(static_cast<dt::basic_test::status_type>(99)));
    h.run(t);

    ok = D_TH_CHECK(h.pending() == 1) && ok;
    ok = D_TH_CHECK(rec.passed == 0 && rec.failed == 0 && rec.skipped == 0 && rec.error == 0) && ok;
    ok = D_TH_CHECK(rec.test_start == 1 && rec.test_end == 1) && ok;

    return ok;
}

// tests_walk_no_listeners_still_counts
//   with no listeners bound the walk fires nothing but still tallies (the
// counter side is unconditional; only events are listener-gated).
bool
tests_walk_no_listeners_still_counts()
{
    bool ok = true;

    dt::test_handler h;   // no recorder bound
    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed));
    t.push_back(make_leaf(dt::test_status::failed));
    h.run(t);

    ok = D_TH_CHECK(h.passed() == 1 && h.failed() == 1 && h.total() == 2) && ok;

    return ok;
}

// tests_walk_session_brackets
//   run() brackets the walk with session start (first) and end (last), the end
// carrying the final pass/fail counts.
bool
tests_walk_session_brackets()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed));
    t.push_back(make_leaf(dt::test_status::failed));
    h.run(t);

    ok = D_TH_CHECK(rec.session_start == 1) && ok;
    ok = D_TH_CHECK(rec.session_end == 1)   && ok;
    ok = D_TH_CHECK(!rec.order.empty() && rec.order.front() == lifecycle_recorder::m_session_start) && ok;
    ok = D_TH_CHECK(!rec.order.empty() && rec.order.back()  == lifecycle_recorder::m_session_end)   && ok;
    ok = D_TH_CHECK(rec.end_passed == 1 && rec.end_failed == 1) && ok;   // on_session_end payload

    return ok;
}

// tests_walk_mixed_tree
//   a mixed tree tallies each status, brackets each interior with module
// events, and rolls up to a failed verdict.
bool
tests_walk_mixed_tree()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    std::vector<dt::basic_test> t;
    t.push_back(make_interior());
    t.push_back(make_leaf(dt::test_status::passed));
    t.push_back(make_leaf(dt::test_status::passed));
    t.push_back(make_leaf(dt::test_status::failed));
    t.push_back(make_leaf(dt::test_status::skipped));
    t.push_back(make_interior());
    h.run(t);

    ok = D_TH_CHECK(h.passed() == 2 && h.failed() == 1 && h.skipped() == 1 && h.total() == 4) && ok;
    ok = D_TH_CHECK(rec.module_start == 2 && rec.module_end == 2) && ok;
    ok = D_TH_CHECK(rec.test_start == 4 && rec.test_end == 4)     && ok;
    ok = D_TH_CHECK(rec.passed == 2 && rec.failed == 1 && rec.skipped == 1) && ok;
    ok = D_TH_CHECK(h.result().verdict() == dt::session_verdict::failed) && ok;

    return ok;
}

// tests_walk_empty_tree
//   an empty walk fires only the session brackets and settles on the empty
// verdict.
bool
tests_walk_empty_tree()
{
    bool ok = true;

    lifecycle_recorder rec; dt::test_handler h; rec.bind_all(h);
    std::vector<dt::basic_test> t;   // empty
    h.run(t);

    ok = D_TH_CHECK(rec.session_start == 1 && rec.session_end == 1) && ok;
    ok = D_TH_CHECK(rec.test_start == 0 && rec.module_start == 0)   && ok;
    ok = D_TH_CHECK(h.total() == 0) && ok;
    ok = D_TH_CHECK(h.result().verdict() == dt::session_verdict::empty) && ok;
    ok = D_TH_CHECK(rec.end_passed == 0 && rec.end_failed == 0) && ok;

    return ok;
}

// tests_walk_start_resets_counters
//   run() resets counters before walking, so prior record()s do not leak into
// the session result.
bool
tests_walk_start_resets_counters()
{
    bool ok = true;

    dt::test_handler h;
    h.record(dt::test_status::failed);   // pre-existing state
    h.record(dt::test_status::failed);

    std::vector<dt::basic_test> t;
    t.push_back(make_leaf(dt::test_status::passed));
    h.run(t);                            // start_session zeroes first

    ok = D_TH_CHECK(h.failed() == 0) && ok;   // prior failures wiped
    ok = D_TH_CHECK(h.passed() == 1 && h.total() == 1) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
