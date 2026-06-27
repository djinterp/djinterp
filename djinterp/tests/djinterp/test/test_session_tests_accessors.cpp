// djinterp
#include "test_session_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_session_tree_accessor_mutable_and_const
  Verifies both overloads of tree() expose the one owned tree.
  Tests the following:
  - tree() (non-const) is mutable: add_root grows the owned tree
  - tree() const observes the same tree (size reflects the insertion)
*/
bool
tests_session_tree_accessor_mutable_and_const()
{
    session_type s;

    // mutate through the non-const accessor (first add_root also creates the
    // implied conjunctive root, so size becomes 2)
    s.tree().add_root(make_test(0, true));

    const session_type& cs = s;

    return ( (s.tree().size()  == 2) &&
             (cs.tree().size() == 2) );
}

/*
tests_session_counter_accessors_mutable_and_const
  Verifies every status counter is reachable through both overloads, and that
  the mutable overload actually mutates the owned counter.
  Tests the following:
  - passed / failed / skipped / pending / errors (non-const) mutate
  - the const overloads observe the mutated values
*/
bool
tests_session_counter_accessors_mutable_and_const()
{
    session_type s;

    // mutate through the non-const accessors
    s.passed().increment(1);
    s.failed().increment(2);
    s.skipped().increment(3);
    s.pending().increment(4);
    s.errors().increment(5);

    const session_type& cs = s;

    return ( (cs.passed().value()  == 1) &&
             (cs.failed().value()  == 2) &&
             (cs.skipped().value() == 3) &&
             (cs.pending().value() == 4) &&
             (cs.errors().value()  == 5) );
}

/*
tests_session_total_sums_counters
  Verifies total() returns the sum of all five status counters.
  Tests the following:
  - total() == passed + failed + skipped + pending + errors
*/
bool
tests_session_total_sums_counters()
{
    session_type s;

    s.passed().increment(10);
    s.failed().increment(1);
    s.skipped().increment(2);
    s.pending().increment(3);
    s.errors().increment(4);

    // 10 + 1 + 2 + 3 + 4
    return (s.total() == 20);
}

/*
tests_session_timer_accessor_mutable_and_const
  Verifies both overloads of timer() expose the one owned timer.
  Tests the following:
  - timer() (non-const) drives the owned timer (start makes it run)
  - timer() const observes the running state
*/
bool
tests_session_timer_accessor_mutable_and_const()
{
    session_type s;

    const bool idle_ok = (s.timer().running() == false);

    // drive the owned timer through the mutable accessor
    s.timer().start();

    const session_type& cs = s;

    const bool running_ok =
        ( (s.timer().running()  == true) &&
          (cs.timer().running() == true) );

    return ( idle_ok &&
             running_ok );
}

/*
tests_session_elapsed_initial_zero
  Verifies elapsed() forwards the timer's accumulated duration, which is zero
  on a fresh session.
  Tests the following:
  - elapsed().count() == 0 before any timing
*/
bool
tests_session_elapsed_initial_zero()
{
    session_type s;

    return (s.elapsed().count() == 0);
}

/*
tests_session_save_load_are_noops
  Verifies the stubbed save() / load() are inert: callable and leave the
  session's observable state untouched.
  Tests the following:
  - save() (const) and load() change neither state nor counters nor tree
*/
bool
tests_session_save_load_are_noops()
{
    session_type s;
    s.passed().increment(7);
    s.tree().add_root(make_test(0, true));

    s.save();
    s.load();

    return ( (s.is_idle()         == true) &&
             (s.passed().value()  == 7)    &&
             (s.tree().size()     == 2) );
}


NS_END  // testing
NS_END  // djinterp
