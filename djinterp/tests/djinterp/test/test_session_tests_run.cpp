// djinterp
#include "test_session_tests.hpp"


NS_DJINTERP
NS_TESTING


//   These tests drive the session against a live, default-constructed
// test_handler.  No listeners or printer are attached, so the handler walks
// the forest and tallies into its session_result while firing lifecycle
// events to nobody.  The session then syncs its own counters from that
// result.  See test_session_tests.hpp for the conjunctive-root accounting
// these expectations bake in (the pending root is counted as one leaf).


/*
tests_session_run_handler_empty_tree
  Verifies running an empty session: the handler observes nothing, the verdict
  is empty, and the lifecycle completes.
  Tests the following:
  - run(handler) on an empty tree returns session_verdict::empty
  - all counters and total remain 0 (sync from an all-zero result)
  - the session ends finished with the timer stopped
*/
bool
tests_session_run_handler_empty_tree()
{
    session_type  s;
    test_handler  h;

    const session_verdict v = s.run(h);

    return ( (v == session_verdict::empty) &&
             (s.passed().value()  == 0)    &&
             (s.failed().value()  == 0)    &&
             (s.skipped().value() == 0)    &&
             (s.pending().value() == 0)    &&
             (s.errors().value()  == 0)    &&
             (s.total()           == 0)    &&
             (s.is_finished()     == true) &&
             (s.timer().running() == false) );
}

/*
tests_session_run_handler_all_statuses
  Verifies the full sync path with one observed leaf of every status (plus the
  pending conjunctive root), which drives a failed verdict.
  Tests the following:
  - each per-status counter is synced from the handler result (all > 0 guards)
  - pending == 1 from the conjunctive root; total == 5
  - run(handler) returns session_verdict::failed (a failure outranks all)
  - the session ends finished
*/
bool
tests_session_run_handler_all_statuses()
{
    session_type  s;
    test_handler  h;

    // all child roots are leaves (type_id <= 1) so each is counted by status
    s.tree().add_root(make_status_test(0, test_status::passed));
    s.tree().add_root(make_status_test(1, test_status::failed));
    s.tree().add_root(make_status_test(0, test_status::skipped));
    s.tree().add_root(make_status_test(1, test_status::error));

    const session_verdict v = s.run(h);

    return ( (s.passed().value()  == 1)     &&
             (s.failed().value()  == 1)     &&
             (s.skipped().value() == 1)     &&
             (s.errors().value()  == 1)     &&
             (s.pending().value() == 1)     &&   // the conjunctive root
             (s.total()           == 5)     &&
             (v == session_verdict::failed) &&
             (s.is_finished()     == true) );
}

/*
tests_session_run_handler_all_pass_pending_root
  Verifies that an all-passing leaf set still yields a pending verdict, because
  the conjunctive root is counted as one pending leaf.  Also exercises the
  false branch of the failed / skipped / errors sync guards.
  Tests the following:
  - passed == 2; pending == 1 (conjunctive root); failed/skipped/errors == 0
  - run(handler) returns session_verdict::pending (no failure, work remains)
*/
bool
tests_session_run_handler_all_pass_pending_root()
{
    session_type  s;
    test_handler  h;

    s.tree().add_root(make_status_test(0, test_status::passed));
    s.tree().add_root(make_status_test(1, test_status::passed));

    const session_verdict v = s.run(h);

    return ( (s.passed().value()  == 2)      &&
             (s.failed().value()  == 0)      &&
             (s.skipped().value() == 0)      &&
             (s.errors().value()  == 0)      &&
             (s.pending().value() == 1)      &&   // the conjunctive root
             (v == session_verdict::pending) &&
             (s.is_finished()     == true) );
}

/*
tests_session_run_handler_and_tree_prebuilt_passed
  Verifies run(handler, tree): a pre-built forest is moved in and run.  Its
  root is an evaluated (non-pending) leaf, so a passed verdict is reachable.
  Tests the following:
  - the supplied tree is adopted (size reflects it) and walked
  - passed == 2, pending == 0 -> session_verdict::passed
  - the session ends finished
*/
bool
tests_session_run_handler_and_tree_prebuilt_passed()
{
    session_type  s;
    test_handler  h;

    // pre-build a forest whose root is an evaluated passed leaf (id 0), with a
    // passed leaf child - no pending conjunctive root is fabricated
    nary_tree<basic_test> f;
    f.emplace_root(make_status_test(0, test_status::passed));
    f.append_child(f.root(), make_status_test(0, test_status::passed));

    session_type::tree_type built(std::vector<test_kind>(),
                                  static_cast<nary_tree<basic_test>&&>(f));

    const session_verdict v =
        s.run(h, static_cast<session_type::tree_type&&>(built));

    return ( (s.tree().size()     == 2)     &&
             (s.passed().value()  == 2)     &&
             (s.pending().value() == 0)     &&
             (v == session_verdict::passed) &&
             (s.is_finished()     == true) );
}

/*
tests_session_run_handler_not_idle_skips_walk
  Verifies run(handler) is a no-op when the session is not idle: it returns the
  verdict from the CURRENT counters without walking the tree.
  Tests the following:
  - on a running session, run(handler) does not walk (a failing leaf in the
    tree is never counted)
  - it returns the verdict derived from the pre-set counters
  - the state is left unchanged (still running, not finished)
*/
bool
tests_session_run_handler_not_idle_skips_walk()
{
    session_type  s;
    test_handler  h;

    // a failing leaf sits in the tree; if the walk ran it would be counted
    s.tree().add_root(make_status_test(1, test_status::failed));

    // pre-set the counters to a passing tally
    s.passed().increment(5);

    // leave idle -> running so run(handler) takes the early-return path
    s.run();

    const session_verdict v = s.run(h);

    return ( (v == session_verdict::passed) &&   // from passed==5, not a walk
             (s.passed().value() == 5)      &&   // counters untouched
             (s.failed().value() == 0)      &&   // failing leaf NOT counted
             (s.is_running()     == true) );      // state unchanged
}

/*
tests_session_run_handler_and_tree_not_idle_no_move
  Verifies run(handler, tree) is a no-op when not idle: the supplied tree is
  NOT moved in and no walk occurs.
  Tests the following:
  - on a running session, the moved-in tree does not replace the owned tree
  - the verdict comes from current counters (empty here)
  - the state is left unchanged
*/
bool
tests_session_run_handler_and_tree_not_idle_no_move()
{
    session_type  s;
    test_handler  h;

    // make the session non-idle with an empty owned tree
    s.run();   // idle -> running

    nary_tree<basic_test> f;
    f.emplace_root(make_status_test(0, test_status::passed));
    f.append_child(f.root(), make_status_test(0, test_status::passed));

    session_type::tree_type built(std::vector<test_kind>(),
                                  static_cast<nary_tree<basic_test>&&>(f));

    const session_verdict v =
        s.run(h, static_cast<session_type::tree_type&&>(built));

    return ( (v == session_verdict::empty) &&   // counters still 0
             (s.tree().size() == 0)        &&   // built NOT moved in
             (s.is_running()  == true) );        // state unchanged
}


NS_END  // testing
NS_END  // djinterp
