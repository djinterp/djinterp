/*******************************************************************************
* djinterp [core]                                      timer_tests_accessors.cpp
*
*
* path:      /tests/djinterp/core/util/timer/timer_tests_accessors.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_timer_elapsed_running
  Verifies elapsed() while running includes the live interval.
  Tests the following:
  - elapsed() reflects time since start while running
  - it grows as the clock advances without an intervening stop
*/
bool
tests_timer_elapsed_running()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(10));

    D_TM_CHECK(t.elapsed() == msec(10));

    test_clock::advance(msec(5));

    D_TM_CHECK(t.elapsed() == msec(15));

    return true;
}


/*
tests_timer_elapsed_stopped
  Verifies elapsed() while stopped returns only accumulated time.
  Tests the following:
  - after a stop, elapsed() equals the accumulated interval
*/
bool
tests_timer_elapsed_stopped()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(10));
    t.stop();

    D_TM_CHECK(t.elapsed() == msec(10));

    return true;
}


/*
tests_timer_elapsed_running_with_accumulation
  Verifies elapsed() combines prior accumulation with the live interval.
  Tests the following:
  - a stopped interval (10 ms) plus a live interval (7 ms) yields 17 ms
*/
bool
tests_timer_elapsed_running_with_accumulation()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(10));
    t.stop();                       // accumulated = 10 ms

    t.start();                      // running again
    test_clock::advance(msec(7));

    // elapsed() == accumulated (10) + live delta (7).
    D_TM_CHECK(t.elapsed() == msec(17));

    return true;
}


/*
tests_timer_max_value
  Verifies max() reporting.
  Tests the following:
  - a bounded timer returns its configured maximum
  - an unbounded timer returns zero
*/
bool
tests_timer_max_value()
{
    reset_clock();

    ms_timer bounded(msec(250));
    ms_timer unbounded;

    D_TM_CHECK(bounded.max()   == msec(250));
    D_TM_CHECK(unbounded.max() == msec(0));

    return true;
}


/*
tests_timer_has_max_flag
  Verifies has_max() reporting.
  Tests the following:
  - a bounded timer reports true
  - an unbounded timer reports false
*/
bool
tests_timer_has_max_flag()
{
    reset_clock();

    ms_timer with(msec(100));
    ms_timer without;

    D_TM_CHECK(with.has_max()    == true);
    D_TM_CHECK(without.has_max() == false);

    return true;
}


/*
tests_timer_running_flag
  Verifies running() tracks start/stop transitions.
  Tests the following:
  - false initially, true after start, false again after stop
*/
bool
tests_timer_running_flag()
{
    reset_clock();

    ms_timer t;

    D_TM_CHECK(t.running() == false);

    t.start();

    D_TM_CHECK(t.running() == true);

    t.stop();

    D_TM_CHECK(t.running() == false);

    return true;
}


/*
tests_timer_expired_without_max
  Verifies the no-maximum branch of expired().
  Tests the following:
  - with no maximum, expired() is false even after a long interval
*/
bool
tests_timer_expired_without_max()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(1000));

    // no maximum set: expired() always returns false.
    D_TM_CHECK(t.expired() == false);

    return true;
}


/*
tests_timer_expired_below_limit
  Verifies expired() below the limit.
  Tests the following:
  - elapsed strictly under the maximum is not expired
*/
bool
tests_timer_expired_below_limit()
{
    reset_clock();

    ms_timer t(msec(100));

    t.start();
    test_clock::advance(msec(50));

    D_TM_CHECK(t.expired() == false);

    return true;
}


/*
tests_timer_expired_at_limit
  Verifies the boundary case of expired() at exactly the limit.
  Tests the following:
  - elapsed equal to the maximum is expired (the comparison is >=)
*/
bool
tests_timer_expired_at_limit()
{
    reset_clock();

    ms_timer t(msec(100));

    t.start();
    test_clock::advance(msec(100));

    // elapsed meets the limit exactly: >= boundary makes it expired.
    D_TM_CHECK(t.expired() == true);

    return true;
}


/*
tests_timer_expired_above_limit
  Verifies expired() past the limit.
  Tests the following:
  - elapsed beyond the maximum is expired
*/
bool
tests_timer_expired_above_limit()
{
    reset_clock();

    ms_timer t(msec(100));

    t.start();
    test_clock::advance(msec(150));

    D_TM_CHECK(t.expired() == true);

    return true;
}


/*
tests_timer_expired_live
  Verifies expiry is observed live, while the timer is still running.
  Tests the following:
  - a running timer past its limit reports running() true and expired() true
  - remaining() is zero in that state
*/
bool
tests_timer_expired_live()
{
    reset_clock();

    ms_timer t(msec(100));

    t.start();
    test_clock::advance(msec(120));

    // expiry is detected without stopping the timer.
    D_TM_CHECK(t.running()   == true);
    D_TM_CHECK(t.expired()   == true);
    D_TM_CHECK(t.remaining() == msec(0));

    return true;
}


/*
tests_timer_remaining_without_max
  Verifies the no-maximum branch of remaining().
  Tests the following:
  - without a maximum, remaining() is defined to be zero
*/
bool
tests_timer_remaining_without_max()
{
    reset_clock();

    ms_timer t;

    t.start();
    test_clock::advance(msec(10));

    // without a maximum, remaining() is zero by definition.
    D_TM_CHECK(t.remaining() == msec(0));

    return true;
}


/*
tests_timer_remaining_before_start
  Verifies remaining() before any time elapses.
  Tests the following:
  - remaining() equals the full maximum when nothing has elapsed
*/
bool
tests_timer_remaining_before_start()
{
    reset_clock();

    ms_timer t(msec(100));

    // nothing elapsed yet: the full maximum remains.
    D_TM_CHECK(t.remaining() == msec(100));

    return true;
}


/*
tests_timer_remaining_partial
  Verifies remaining() with a partial interval elapsed.
  Tests the following:
  - remaining() equals maximum minus elapsed (70 ms of a 100 ms limit)
*/
bool
tests_timer_remaining_partial()
{
    reset_clock();

    ms_timer t(msec(100));

    t.start();
    test_clock::advance(msec(30));

    D_TM_CHECK(t.remaining() == msec(70));

    return true;
}


/*
tests_timer_remaining_at_limit
  Verifies remaining() reaching zero through the normal subtraction path.
  Tests the following:
  - at elapsed == maximum, remaining() is zero via (max - elapsed == 0),
    not via the negative-clamp branch
*/
bool
tests_timer_remaining_at_limit()
{
    reset_clock();

    ms_timer t(msec(100));

    t.start();
    test_clock::advance(msec(100));

    // remaining is exactly zero from the normal path, not the clamp.
    D_TM_CHECK(t.remaining() == msec(0));

    return true;
}


/*
tests_timer_remaining_past_limit
  Verifies the negative-clamp branch of remaining().
  Tests the following:
  - when elapsed exceeds maximum, the negative difference is clamped to zero
*/
bool
tests_timer_remaining_past_limit()
{
    reset_clock();

    ms_timer t(msec(100));

    t.start();
    test_clock::advance(msec(140));

    // the raw difference is negative and must be clamped to zero.
    D_TM_CHECK(t.remaining() == msec(0));

    return true;
}


/*
tests_timer_const_accessors
  Verifies every const accessor through a const reference, including the
  const child() and const observed() overloads.
  Tests the following:
  - elapsed/max/has_max/running/expired/remaining read correctly when const
  - child_count/observed_count read correctly when const
  - the const child() overload and observed() return the expected values
*/
bool
tests_timer_const_accessors()
{
    reset_clock();

    ms_timer base(msec(100));

    base.add_child();

    ms_timer external;

    base.observe(external);

    base.start();
    test_clock::advance(msec(40));
    base.stop();

    // exercise every const accessor through a const reference.
    const ms_timer& t = base;

    D_TM_CHECK(t.elapsed()        == msec(40));
    D_TM_CHECK(t.max()            == msec(100));
    D_TM_CHECK(t.has_max()        == true);
    D_TM_CHECK(t.running()        == false);
    D_TM_CHECK(t.expired()        == false);
    D_TM_CHECK(t.remaining()      == msec(60));
    D_TM_CHECK(t.child_count()    == 1);
    D_TM_CHECK(t.observed_count() == 1);

    // const child() overload and const-context observed().
    D_TM_CHECK(t.child(0).elapsed() == msec(0));
    D_TM_CHECK(t.observed(0)        == &external);

    return true;
}


/*
tests_timer_offset_independence
  Verifies elapsed math is purely delta-based and offset-independent.
  Tests the following:
  - starting from a large absolute clock baseline, a 10 ms interval still
    reports as 10 ms elapsed
*/
bool
tests_timer_offset_independence()
{
    reset_clock();

    // move the clock to a large absolute baseline before timing.
    test_clock::set(test_clock::time_point(msec(1000000000)));

    ms_timer t;

    t.start();
    test_clock::advance(msec(10));
    t.stop();

    D_TM_CHECK(t.elapsed() == msec(10));

    return true;
}


NS_END  // testing
NS_END  // djinterp
