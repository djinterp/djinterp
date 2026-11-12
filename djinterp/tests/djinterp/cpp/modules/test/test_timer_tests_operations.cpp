/*******************************************************************************
* djinterp [cpp][test]                           test_timer_tests_operations.cpp
*
* The test_timer suite, block "operations": the generic timer's start, stop,
* reset and reset_all, as the test timer's own.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_timer_tests_operations.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_start_stop_accumulate
  Verifies successive start/stop cycles accumulate elapsed time.
  Tests the following:
  - one cycle of 30ms gives 30ms
  - time while stopped does not count
  - a second cycle of 20ms gives 50ms in all
*/
bool tests_test_timer_start_stop_accumulate()
{
    reset_clock();

    tt t;

    t.start();
    test_clock::advance(DMsec(30));
    t.stop();
    D_TT_CHECK(t.elapsed() == DMsec(30));

    test_clock::advance(DMsec(100));
    t.start();
    test_clock::advance(DMsec(20));
    t.stop();
    D_TT_CHECK(t.elapsed() == DMsec(50));

    return true;
}


/*
tests_test_timer_start_twice_noop
  Verifies starting a running timer changes nothing.
  Tests the following:
  - a second start() does not restart the open interval
*/
bool tests_test_timer_start_twice_noop()
{
    reset_clock();

    tt t;

    t.start();
    test_clock::advance(DMsec(10));
    t.start();
    test_clock::advance(DMsec(5));
    t.stop();

    D_TT_CHECK(t.elapsed() == DMsec(15));

    return true;
}


/*
tests_test_timer_start_when_expired_noop
  Verifies an expired timer does not start.
  Tests the following:
  - a timer that met its limit reports expired and stays stopped on start()
*/
bool tests_test_timer_start_when_expired_noop()
{
    reset_clock();

    tt t(DMsec(50));

    t.start();
    test_clock::advance(DMsec(50));
    t.stop();
    D_TT_CHECK(t.expired());

    t.start();

    D_TT_CHECK(!t.running());

    return true;
}


/*
tests_test_timer_stop_not_running_noop
  Verifies stopping a stopped timer changes nothing.
  Tests the following:
  - stop() on a fresh timer leaves it stopped at zero
*/
bool tests_test_timer_stop_not_running_noop()
{
    reset_clock();

    tt t;

    t.stop();

    D_TT_CHECK(!t.running());
    D_TT_CHECK(t.elapsed() == DMsec(0));

    return true;
}


/*
tests_test_timer_reset
  Verifies reset() stops the timer and clears its time.
  Tests the following:
  - a running timer with time on it is stopped at zero
*/
bool tests_test_timer_reset()
{
    reset_clock();

    tt t;

    t.start();
    test_clock::advance(DMsec(30));
    t.reset();

    D_TT_CHECK(!t.running());
    D_TT_CHECK(t.elapsed() == DMsec(0));

    return true;
}


/*
tests_test_timer_reset_all_recurses
  Verifies reset_all() resets this timer and every owned descendant.
  Tests the following:
  - the root and a grandchild both return to zero
*/
bool tests_test_timer_reset_all_recurses()
{
    reset_clock();

    tt t;

    (void)t.add_child();
    (void)t.add_child().add_child();
    t.start();
    t.child(1).child(0).start();
    test_clock::advance(DMsec(25));
    t.stop();
    t.child(1).child(0).stop();

    t.reset_all();

    D_TT_CHECK(t.elapsed() == DMsec(0));
    D_TT_CHECK(t.child(1).child(0).elapsed() == DMsec(0));

    return true;
}


NS_END  // testing
NS_END  // djinterp
