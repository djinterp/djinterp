/*******************************************************************************
* djinterp [cpp]                                    timer_tests_constructors.cpp
*
*
* path:      /tests/djinterp/cpp/util/timer/timer_tests_constructors.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_timer_default_ctor
  Verifies the default constructor's initial state.
  Tests the following:
  - a default-constructed timer is not running
  - it carries no maximum (has_max() is false, max() is zero)
  - elapsed() is zero
  - with no maximum it is never expired and reports zero remaining
*/
bool tests_timer_default_ctor()
{
    reset_clock();

    DMsTimer t;

    D_TM_CHECK(t.running()   == false);
    D_TM_CHECK(t.has_max()   == false);
    D_TM_CHECK(t.elapsed()   == DMsec(0));
    D_TM_CHECK(t.max()       == DMsec(0));
    D_TM_CHECK(t.expired()   == false);
    D_TM_CHECK(t.remaining() == DMsec(0));

    return true;
}


/*
tests_timer_max_ctor
  Verifies the maximum-duration constructor's initial state.
  Tests the following:
  - has_max() is true and max() returns the supplied limit
  - the timer is not running and has zero elapsed
  - it is not yet expired (zero is below the limit)
  - remaining() equals the full maximum before any time elapses
*/
bool tests_timer_max_ctor()
{
    reset_clock();

    DMsTimer t(DMsec(100));

    D_TM_CHECK(t.has_max()   == true);
    D_TM_CHECK(t.max()       == DMsec(100));
    D_TM_CHECK(t.running()   == false);
    D_TM_CHECK(t.elapsed()   == DMsec(0));
    D_TM_CHECK(t.expired()   == false);
    D_TM_CHECK(t.remaining() == DMsec(100));

    return true;
}


/*
tests_timer_zero_max_ctor
  Verifies the boundary case of a zero maximum (a timer born expired).
  Tests the following:
  - has_max() is true and max() is zero
  - elapsed (zero) meets the limit (zero), so expired() is true immediately
  - remaining() is zero
  - start() has no effect because the expired guard blocks it
*/
bool tests_timer_zero_max_ctor()
{
    reset_clock();

    DMsTimer t(DMsec(0));

    D_TM_CHECK(t.has_max()   == true);
    D_TM_CHECK(t.max()       == DMsec(0));
    D_TM_CHECK(t.expired()   == true);
    D_TM_CHECK(t.remaining() == DMsec(0));

    // born expired: start() must be a no-op.
    t.start();

    D_TM_CHECK(t.running() == false);

    return true;
}


/*
tests_timer_ctor_custom_duration
  Verifies the template works with durations finer than the clock tick.
  Tests the following:
  - a nanosecond-resolution timer constructs, runs, and reports elapsed and
    remaining time correctly (whole-millisecond advances convert exactly)
  - a microsecond-resolution timer likewise reports elapsed time correctly
  - the duration_cast path in stop()/elapsed() yields the expected finer units
*/
bool tests_timer_ctor_custom_duration()
{
    reset_clock();

    // nanosecond timer; the clock ticks in milliseconds, so advance by whole
    // milliseconds (which convert exactly into nanoseconds).
    DNsTimer tns(DNsec(5000000));   // 5 ms expressed in ns

    D_TM_CHECK(tns.has_max() == true);
    D_TM_CHECK(tns.max()     == DNsec(5000000));

    tns.start();
    test_clock::advance(DMsec(2));
    tns.stop();

    D_TM_CHECK(tns.elapsed()   == DNsec(2000000));   // 2 ms in ns
    D_TM_CHECK(tns.remaining() == DNsec(3000000));

    // microsecond timer over the same clock.
    DUsTimer tus(DUsec(5000));      // 5 ms expressed in us

    tus.start();
    test_clock::advance(DMsec(3));
    tus.stop();

    D_TM_CHECK(tus.elapsed()   == DUsec(3000));      // 3 ms in us
    D_TM_CHECK(tus.remaining() == DUsec(2000));      // positive remaining path

    return true;
}


/*
tests_timer_ns_lifecycle
  Drives a nanosecond-resolution timer through every guarded path, verifying
  the full contract holds identically at the finest resolution.
  Tests the following:
  - the stop() not-running guard on a fresh ns timer
  - elapsed() while running, and expired()/remaining() with no maximum
  - a bounded ns timer driven past its limit: expired() true, remaining()
    clamped to zero, and start() blocked by the expired guard
*/
bool tests_timer_ns_lifecycle()
{
    reset_clock();

    // unbounded ns timer: not-running stop guard, live elapsed, and the
    // no-maximum branches of expired()/remaining().
    DNsTimer un;

    un.stop();

    D_TM_CHECK(un.running() == false);

    un.start();

    // start while already running: the running guard makes this a no-op.
    un.start();

    test_clock::advance(DMsec(1));

    D_TM_CHECK(un.elapsed()   == DNsec(1000000));    // 1 ms in ns
    D_TM_CHECK(un.expired()   == false);
    D_TM_CHECK(un.remaining() == DNsec(0));

    un.stop();

    // bounded ns timer driven past its maximum: expiry, the remaining()
    // negative-clamp, and the start() expired guard.
    DNsTimer bd(DNsec(1000000));    // 1 ms in ns

    bd.start();
    test_clock::advance(DMsec(2));   // 2 ms exceeds the 1 ms limit
    bd.stop();

    D_TM_CHECK(bd.expired()   == true);
    D_TM_CHECK(bd.remaining() == DNsec(0));

    bd.start();

    D_TM_CHECK(bd.running() == false);

    return true;
}


/*
tests_timer_us_lifecycle
  Drives a microsecond-resolution timer through every guarded path, verifying
  the full contract holds identically at microsecond resolution.
  Tests the following:
  - the stop() not-running guard on a fresh us timer
  - elapsed() while running, and expired()/remaining() with no maximum
  - a bounded us timer driven past its limit: expired() true, remaining()
    clamped to zero, and start() blocked by the expired guard
*/
bool tests_timer_us_lifecycle()
{
    reset_clock();

    // unbounded us timer: not-running stop guard, live elapsed, and the
    // no-maximum branches of expired()/remaining().
    DUsTimer un;

    un.stop();

    D_TM_CHECK(un.running() == false);

    un.start();

    // start while already running: the running guard makes this a no-op.
    un.start();

    test_clock::advance(DMsec(1));

    D_TM_CHECK(un.elapsed()   == DUsec(1000));       // 1 ms in us
    D_TM_CHECK(un.expired()   == false);
    D_TM_CHECK(un.remaining() == DUsec(0));

    un.stop();

    // bounded us timer driven past its maximum: expiry, the remaining()
    // negative-clamp, and the start() expired guard.
    DUsTimer bd(DUsec(1000));       // 1 ms in us

    bd.start();
    test_clock::advance(DMsec(2));   // 2 ms exceeds the 1 ms limit
    bd.stop();

    D_TM_CHECK(bd.expired()   == true);
    D_TM_CHECK(bd.remaining() == DUsec(0));

    bd.start();

    D_TM_CHECK(bd.running() == false);

    return true;
}


NS_END  // testing
NS_END  // djinterp
