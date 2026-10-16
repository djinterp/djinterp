/*******************************************************************************
* djinterp [core]                                     timer_tests_operations.cpp
*
*
* path:      /tests/djinterp/core/util/timer/timer_tests_operations.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_timer_start
  Verifies the successful start path.
  Tests the following:
  - start() on a fresh timer sets it running
  - elapsed() then tracks time advancing past the captured start point
*/
bool
tests_timer_start()
{
    reset_clock();

    ms_timer t;

    t.start();

    D_TM_CHECK(t.running() == true);

    test_clock::advance(msec(10));

    D_TM_CHECK(t.elapsed() == msec(10));

    return true;
}


/*
tests_timer_start_while_running
  Verifies the running guard: a second start() while already running is
  ignored and does not re-capture the start point.
  Tests the following:
  - after start, advance, start again, advance, elapsed() reflects one
    continuous interval (20 ms), proving the second start was a no-op
*/
bool
tests_timer_start_while_running()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(10));

    // second start while running must not reset the start point.
    t.start();
    test_clock::advance(msec(10));

    D_TM_CHECK(t.elapsed() == msec(20));

    return true;
}


/*
tests_timer_start_when_expired
  Verifies the expired guard in start() in isolation from the running guard:
  a bounded timer driven to its limit, then stopped, cannot be restarted.
  Tests the following:
  - after reaching the limit the timer is expired and not running
  - start() hits the expired guard and leaves the timer stopped
  - elapsed() is unchanged by the ignored start and a later clock advance
*/
bool
tests_timer_start_when_expired()
{
    reset_clock();

    ms_timer t(msec(50));

    t.start();
    test_clock::advance(msec(50));
    t.stop();

    D_TM_CHECK(t.expired() == true);
    D_TM_CHECK(t.running() == false);

    // expired and not running: start() must do nothing.
    t.start();

    D_TM_CHECK(t.running() == false);

    test_clock::advance(msec(10));

    D_TM_CHECK(t.elapsed() == msec(50));

    return true;
}


/*
tests_timer_start_when_born_expired
  Verifies the expired guard for a timer born expired (zero maximum).
  Tests the following:
  - start() on a zero-max timer is blocked by the expired guard
  - the timer never runs and accrues no elapsed time
*/
bool
tests_timer_start_when_born_expired()
{
    reset_clock();

    ms_timer t(msec(0));

    // born expired (elapsed 0 >= max 0): start() is blocked.
    t.start();

    D_TM_CHECK(t.running() == false);
    D_TM_CHECK(t.elapsed() == msec(0));

    return true;
}


/*
tests_timer_stop
  Verifies the successful stop path.
  Tests the following:
  - stop() after a running interval clears the running flag
  - the interval is accumulated into elapsed()
  - advancing the clock after stop does not change elapsed()
*/
bool
tests_timer_stop()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(25));
    t.stop();

    D_TM_CHECK(t.running() == false);
    D_TM_CHECK(t.elapsed() == msec(25));

    // a stopped timer does not accrue further time.
    test_clock::advance(msec(10));

    D_TM_CHECK(t.elapsed() == msec(25));

    return true;
}


/*
tests_timer_stop_when_not_running
  Verifies the not-running guard in stop().
  Tests the following:
  - stop() on a never-started timer is a no-op
  - the timer stays stopped with zero elapsed
*/
bool
tests_timer_stop_when_not_running()
{
    reset_clock();

    ms_timer t;

    // not running: stop() hits the guard and does nothing.
    t.stop();

    D_TM_CHECK(t.running() == false);
    D_TM_CHECK(t.elapsed() == msec(0));

    return true;
}


/*
tests_timer_accumulate_cycles
  Verifies that successive start/stop cycles accumulate.
  Tests the following:
  - two separate run intervals (10 ms then 15 ms) sum in elapsed() (25 ms)
*/
bool
tests_timer_accumulate_cycles()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(10));
    t.stop();

    t.start();
    test_clock::advance(msec(15));
    t.stop();

    D_TM_CHECK(t.elapsed() == msec(25));

    return true;
}


/*
tests_timer_exclude_stopped_intervals
  Verifies that time elapsing while stopped is not counted.
  Tests the following:
  - a large clock advance between stop and the next start is excluded
  - only the two running intervals (10 ms + 5 ms) appear in elapsed()
*/
bool
tests_timer_exclude_stopped_intervals()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(10));
    t.stop();

    // time passes while stopped; this gap must not be counted.
    test_clock::advance(msec(100));

    t.start();
    test_clock::advance(msec(5));
    t.stop();

    D_TM_CHECK(t.elapsed() == msec(15));

    return true;
}


/*
tests_timer_reset_while_running
  Verifies reset() on a running timer.
  Tests the following:
  - reset() clears both the running flag and accumulated time
  - the timer is usable again afterward and measures fresh intervals
*/
bool
tests_timer_reset_while_running()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(30));

    // reset while running clears running state and accumulated time.
    t.reset();

    D_TM_CHECK(t.running() == false);
    D_TM_CHECK(t.elapsed() == msec(0));

    // usable again after reset.
    t.start();
    test_clock::advance(msec(5));

    D_TM_CHECK(t.elapsed() == msec(5));

    return true;
}


/*
tests_timer_reset_while_stopped
  Verifies reset() on a stopped timer with accumulated time.
  Tests the following:
  - accumulated time is cleared to zero
  - the timer remains stopped
*/
bool
tests_timer_reset_while_stopped()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(20));
    t.stop();

    D_TM_CHECK(t.elapsed() == msec(20));

    t.reset();

    D_TM_CHECK(t.elapsed() == msec(0));
    D_TM_CHECK(t.running() == false);

    return true;
}


/*
tests_timer_reset_preserves_children
  Verifies reset() does not disturb owned children.
  Tests the following:
  - a child with accumulated time keeps it after the parent is reset
  - the parent's child count is unchanged
*/
bool
tests_timer_reset_preserves_children()
{
    reset_clock();

    ms_timer parent;

    parent.add_child();

    // give the child some accumulated time.
    parent.child(0).start();
    test_clock::advance(msec(10));
    parent.child(0).stop();

    // reset() affects only this timer, not its children.
    parent.reset();

    D_TM_CHECK(parent.child_count()      == 1);
    D_TM_CHECK(parent.child(0).elapsed() == msec(10));

    return true;
}


/*
tests_timer_reset_all_recurses
  Verifies reset_all() recurses through the whole owned subtree.
  Tests the following:
  - a root/child/grandchild tree each accumulate time
  - reset_all() clears elapsed() at every level (the recursion loop runs)
*/
bool
tests_timer_reset_all_recurses()
{
    reset_clock();

    ms_timer root;

    root.add_child();
    root.child(0).add_child();

    // accumulate time independently at each level.
    root.start();
    test_clock::advance(msec(10));
    root.stop();

    root.child(0).start();
    test_clock::advance(msec(10));
    root.child(0).stop();

    root.child(0).child(0).start();
    test_clock::advance(msec(10));
    root.child(0).child(0).stop();

    D_TM_CHECK(root.elapsed()                     == msec(10));
    D_TM_CHECK(root.child(0).elapsed()            == msec(10));
    D_TM_CHECK(root.child(0).child(0).elapsed()   == msec(10));

    // reset_all() recurses into children and grandchildren.
    root.reset_all();

    D_TM_CHECK(root.elapsed()                     == msec(0));
    D_TM_CHECK(root.child(0).elapsed()            == msec(0));
    D_TM_CHECK(root.child(0).child(0).elapsed()   == msec(0));

    return true;
}


/*
tests_timer_reset_all_leaf
  Verifies reset_all() on a childless timer (the recursion loop is skipped).
  Tests the following:
  - accumulated time is cleared and the timer is stopped
  - the timer still has no children
*/
bool
tests_timer_reset_all_leaf()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(40));
    t.stop();

    // no children: the recursion loop body never executes.
    t.reset_all();

    D_TM_CHECK(t.elapsed()     == msec(0));
    D_TM_CHECK(t.running()     == false);
    D_TM_CHECK(t.child_count() == 0);

    return true;
}


/*
tests_timer_reset_all_preserves_observed
  Verifies reset_all() leaves observed (non-owning) timers untouched.
  Tests the following:
  - an observed external timer keeps its accumulated time after reset_all()
  - the observed-count is unchanged and the view still reflects the external
*/
bool
tests_timer_reset_all_preserves_observed()
{
    reset_clock();

    ms_timer root;
    ms_timer external;

    // external accrues time and is observed (non-owning) by root.
    external.start();
    test_clock::advance(msec(10));
    external.stop();

    root.observe(external);

    // reset_all() resets owned state only; observed timers are left alone.
    root.reset_all();

    D_TM_CHECK(root.observed_count()        == 1);
    D_TM_CHECK(external.elapsed()           == msec(10));
    D_TM_CHECK(root.observed(0)->elapsed()  == msec(10));

    return true;
}


NS_END  // testing
NS_END  // djinterp
