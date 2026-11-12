/*******************************************************************************
* djinterp [cpp][utility]                                  timer_tests_merge.cpp
*
* The timer suite's tests for the four operations a timer shares with the
* counters -- increment, decrement, compare and merge -- for timer and
* mutex_timer, on the hand-advanced test clock.
*
*
* path:      /tests/djinterp/cpp/util/timer/timer_tests_merge.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_timer_increment
  Adding time directly.
  Tests the following:
  - increment adds to a stopped timer's time
  - a negative amount removes time
  - while running, the open interval runs on top of the added time
*/
bool tests_timer_increment()
{
    reset_clock();

    DMsTimer t;

    D_TM_CHECK(t.increment(DMsec(50)));
    D_TM_CHECK(t.increment(DMsec(-10)));
    D_TM_CHECK(t.elapsed() == DMsec(40));

    t.start();
    test_clock::advance(DMsec(5));

    D_TM_CHECK(t.increment(DMsec(100)));
    D_TM_CHECK(t.elapsed() == DMsec(145));

    t.stop();

    D_TM_CHECK(t.elapsed() == DMsec(145));

    return true;
}

/*
tests_timer_decrement
  Removing time directly.
  Tests the following:
  - decrement removes from the accumulated time
  - a negative amount adds time
  - removing time can take an expired timer back under its limit
*/
bool tests_timer_decrement()
{
    reset_clock();

    DMsTimer t(DMsec(100));

    D_TM_CHECK(t.increment(DMsec(120)));
    D_TM_CHECK(t.expired());
    D_TM_CHECK(t.decrement(DMsec(30)));
    D_TM_CHECK(t.elapsed() == DMsec(90));
    D_TM_CHECK(!t.expired());
    D_TM_CHECK(t.decrement(DMsec(-5)));
    D_TM_CHECK(t.elapsed() == DMsec(95));

    return true;
}

/*
tests_timer_held_in_range
  The edges.
  Tests the following:
  - time never goes below zero, and the step that would take it there
    returns false
  - time saturates at Duration::max(), returning false
  - decrementing by Duration::min(), which has no negation, saturates too
*/
bool tests_timer_held_in_range()
{
    reset_clock();

    DMsTimer t;

    D_TM_CHECK(t.increment(DMsec(10)));
    D_TM_CHECK(!t.decrement(DMsec(25)));
    D_TM_CHECK(t.elapsed() == DMsec::zero());
    D_TM_CHECK(!t.increment(DMsec(-1)));

    D_TM_CHECK(t.increment(DMsec::max() - DMsec(1)));
    D_TM_CHECK(!t.increment(DMsec(2)));
    D_TM_CHECK(t.elapsed() == DMsec::max());

    t.reset();

    D_TM_CHECK(!t.decrement(DMsec::min()));
    D_TM_CHECK(t.elapsed() == DMsec::max());

    return true;
}

/*
tests_timer_compare
  Three-way, by elapsed time.
  Tests the following:
  - less, equal and more are -1, 0 and 1
  - a running timer's open interval counts
*/
bool tests_timer_compare()
{
    reset_clock();

    DMsTimer short_one;
    DMsTimer long_one;

    short_one.increment(DMsec(10));
    long_one.increment(DMsec(20));

    D_TM_CHECK(short_one.compare(long_one) == -1);
    D_TM_CHECK(long_one.compare(short_one) == 1);

    short_one.start();
    test_clock::advance(DMsec(10));

    D_TM_CHECK(short_one.compare(long_one) == 0);

    test_clock::advance(DMsec(1));

    D_TM_CHECK(short_one.compare(long_one) == 1);

    return true;
}

/*
tests_timer_merge
  Adding another timer's time.
  Tests the following:
  - merge adds the source's elapsed time and leaves the source alone
  - a running source contributes its open interval as of the merge
  - a timer merged into itself doubles once
*/
bool tests_timer_merge()
{
    reset_clock();

    DMsTimer dest;
    DMsTimer src;

    dest.increment(DMsec(100));
    src.increment(DMsec(40));

    D_TM_CHECK(dest.merge(src));
    D_TM_CHECK(dest.elapsed() == DMsec(140));
    D_TM_CHECK(src.elapsed() == DMsec(40));

    src.start();
    test_clock::advance(DMsec(5));

    D_TM_CHECK(dest.merge(src));
    D_TM_CHECK(dest.elapsed() == DMsec(185));
    D_TM_CHECK(src.running());

    D_TM_CHECK(dest.merge(dest));
    D_TM_CHECK(dest.elapsed() == DMsec(370));

    return true;
}

/*
tests_mutex_timer_operations
  mutex_timer.
  Tests the following:
  - increment, decrement, compare and merge act as timer's, under the lock
  - merge takes another mutex_timer or a plain timer
  - a mutex_timer merged into itself doubles once, without deadlock
*/
bool tests_mutex_timer_operations()
{
    reset_clock();

    mutex_timer<test_clock, DMsec> dest;
    mutex_timer<test_clock, DMsec> src;
    DMsTimer                       plain;

    D_TM_CHECK(dest.increment(DMsec(30)));
    D_TM_CHECK(dest.decrement(DMsec(10)));
    D_TM_CHECK(src.increment(DMsec(5)));
    D_TM_CHECK(plain.increment(DMsec(7)));

    D_TM_CHECK(dest.compare(src) == 1);
    D_TM_CHECK(dest.merge(src));
    D_TM_CHECK(dest.merge(plain));
    D_TM_CHECK(dest.elapsed() == DMsec(32));
    D_TM_CHECK(dest.merge(dest));
    D_TM_CHECK(dest.elapsed() == DMsec(64));

    return true;
}


NS_END  // testing
NS_END  // djinterp
