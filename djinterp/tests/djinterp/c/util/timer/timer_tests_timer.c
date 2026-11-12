/*******************************************************************************
* djinterp [c][utility]                                      timer_tests_timer.c
*
* The timer suite, block "timer": timer.h's timer and its five operations,
* on a hand-wound clock.
*
*
* path:      /tests/djinterp/c/util/timer/timer_tests_timer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "./timer_tests.h"  // corresponding header

// std
#include <string.h>  // memset


/*
d_tests_timer_zero
  A timer needs no init.
  Tests the following:
  - D_TIMER_INIT and a zero-filled struct are the same timer
  - it is stopped, at zero, with no limit, on the monotonic clock
*/
bool
d_tests_timer_zero(void)
{
    struct dj_timer from_macro = D_TIMER_INIT;
    struct dj_timer from_fill  = { 7, 7, 7, true, true, NULL, NULL };

    memset(&from_fill,
           0,
           sizeof(from_fill));

    D_TEST_REQUIRE(from_macro.elapsed == from_fill.elapsed);
    D_TEST_REQUIRE(from_macro.running == from_fill.running);
    D_TEST_REQUIRE(from_macro.has_limit == from_fill.has_limit);
    D_TEST_REQUIRE(from_macro.clock == NULL);

    D_TEST_REQUIRE(d_timer_elapsed(&from_fill) == 0);
    D_TEST_REQUIRE(!d_timer_running(&from_fill));
    D_TEST_REQUIRE(!d_timer_expired(&from_fill));
    D_TEST_REQUIRE(d_timer_remaining(&from_fill) == 0);

    return true;
}

/*
d_tests_timer_start_stop
  Intervals.
  Tests the following:
  - start opens an interval at the clock's reading; stop closes it
  - successive intervals accumulate, and time stopped is not counted
  - elapsed while running includes the open interval
  - a second start, or a stop while stopped, is refused and changes nothing
*/
bool
d_tests_timer_start_stop(void)
{
    struct dj_tests_timer_clock clock = { 1000 };
    struct dj_timer             timer = D_TIMER_INIT;

    d_timer_set_clock(&timer,
                      d_tests_timer_clock_read,
                      &clock);

    D_TEST_REQUIRE(!d_timer_stop(&timer));
    D_TEST_REQUIRE(d_timer_start(&timer));
    D_TEST_REQUIRE(d_timer_running(&timer));
    D_TEST_REQUIRE(!d_timer_start(&timer));

    clock.now = 1250;
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 250);
    D_TEST_REQUIRE(d_timer_stop(&timer));
    D_TEST_REQUIRE(!d_timer_running(&timer));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 250);

    clock.now = 9000;
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 250);
    D_TEST_REQUIRE(d_timer_start(&timer));
    clock.now = 9100;
    D_TEST_REQUIRE(d_timer_stop(&timer));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 350);

    return true;
}

/*
d_tests_timer_step
  Increment and decrement.
  Tests the following:
  - each adds or removes its amount, either sign, and reports a full move
  - elapsed time stops at zero and at INT64_MAX, reporting the cut
  - while running, the change is to the closed time and the open interval
    runs on top of it
*/
bool
d_tests_timer_step(void)
{
    struct dj_tests_timer_clock clock = { 0 };
    struct dj_timer             timer = D_TIMER_INIT;

    d_timer_set_clock(&timer,
                      d_tests_timer_clock_read,
                      &clock);

    D_TEST_REQUIRE(d_timer_increment(&timer,
                                     500));
    D_TEST_REQUIRE(d_timer_increment(&timer,
                                     -100));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 400);
    D_TEST_REQUIRE(d_timer_decrement(&timer,
                                     150));
    D_TEST_REQUIRE(d_timer_decrement(&timer,
                                     -50));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 300);

    D_TEST_REQUIRE(!d_timer_decrement(&timer,
                                      1000));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 0);
    D_TEST_REQUIRE(!d_timer_increment(&timer,
                                      INT64_MIN));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 0);

    D_TEST_REQUIRE(d_timer_increment(&timer,
                                     INT64_MAX - 1));
    D_TEST_REQUIRE(!d_timer_increment(&timer,
                                      2));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == INT64_MAX);

    d_timer_reset(&timer);
    D_TEST_REQUIRE(d_timer_start(&timer));
    clock.now = 70;
    D_TEST_REQUIRE(d_timer_increment(&timer,
                                     30));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 100);
    clock.now = 100;
    D_TEST_REQUIRE(d_timer_stop(&timer));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 130);

    return true;
}

/*
d_tests_timer_compare
  Three-way, by elapsed time.
  Tests the following:
  - less, equal and greater are -1, 0 and 1
  - a running timer's open interval counts
  - NULL sorts before any timer and equals itself
*/
bool
d_tests_timer_compare(void)
{
    struct dj_tests_timer_clock clock     = { 0 };
    struct dj_timer             short_one = D_TIMER_INIT;
    struct dj_timer             long_one  = D_TIMER_INIT;

    d_timer_set_clock(&short_one,
                      d_tests_timer_clock_read,
                      &clock);
    d_timer_set_clock(&long_one,
                      d_tests_timer_clock_read,
                      &clock);

    D_TEST_REQUIRE(d_timer_increment(&short_one,
                                     10));
    D_TEST_REQUIRE(d_timer_increment(&long_one,
                                     20));
    D_TEST_REQUIRE(d_timer_compare(&short_one,
                                   &long_one) == -1);
    D_TEST_REQUIRE(d_timer_compare(&long_one,
                                   &short_one) == 1);

    D_TEST_REQUIRE(d_timer_start(&short_one));
    clock.now = 10;
    D_TEST_REQUIRE(d_timer_compare(&short_one,
                                   &long_one) == 0);
    clock.now = 11;
    D_TEST_REQUIRE(d_timer_compare(&short_one,
                                   &long_one) == 1);

    D_TEST_REQUIRE(d_timer_compare(NULL,
                                   &long_one) == -1);
    D_TEST_REQUIRE(d_timer_compare(&long_one,
                                   NULL) == 1);
    D_TEST_REQUIRE(d_timer_compare(NULL,
                                   NULL) == 0);

    return true;
}

/*
d_tests_timer_merge
  Adding another timer's time.
  Tests the following:
  - merge adds the source's elapsed time, and leaves the source alone
  - a running source contributes its open interval as of the merge
  - a timer merged into itself doubles once
*/
bool
d_tests_timer_merge(void)
{
    struct dj_tests_timer_clock clock = { 0 };
    struct dj_timer             dest  = D_TIMER_INIT;
    struct dj_timer             src   = D_TIMER_INIT;

    d_timer_set_clock(&src,
                      d_tests_timer_clock_read,
                      &clock);

    D_TEST_REQUIRE(d_timer_increment(&dest,
                                     100));
    D_TEST_REQUIRE(d_timer_increment(&src,
                                     40));
    D_TEST_REQUIRE(d_timer_merge(&dest,
                                 &src));
    D_TEST_REQUIRE(d_timer_elapsed(&dest) == 140);
    D_TEST_REQUIRE(d_timer_elapsed(&src) == 40);

    D_TEST_REQUIRE(d_timer_start(&src));
    clock.now = 5;
    D_TEST_REQUIRE(d_timer_merge(&dest,
                                 &src));
    D_TEST_REQUIRE(d_timer_elapsed(&dest) == 185);
    D_TEST_REQUIRE(d_timer_running(&src));

    D_TEST_REQUIRE(d_timer_merge(&dest,
                                 &dest));
    D_TEST_REQUIRE(d_timer_elapsed(&dest) == 370);

    return true;
}

/*
d_tests_timer_reset
  Back to zero.
  Tests the following:
  - reset stops a running timer and zeroes it
  - the limit and the clock stay
*/
bool
d_tests_timer_reset(void)
{
    struct dj_tests_timer_clock clock = { 0 };
    struct dj_timer             timer = D_TIMER_INIT;

    D_TEST_REQUIRE(d_timer_init_limited(&timer,
                                        1000));
    d_timer_set_clock(&timer,
                      d_tests_timer_clock_read,
                      &clock);
    D_TEST_REQUIRE(d_timer_increment(&timer,
                                     300));
    D_TEST_REQUIRE(d_timer_start(&timer));
    clock.now = 50;

    d_timer_reset(&timer);
    D_TEST_REQUIRE(!d_timer_running(&timer));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 0);
    D_TEST_REQUIRE(timer.has_limit);
    D_TEST_REQUIRE(timer.limit == 1000);
    D_TEST_REQUIRE(timer.clock == d_tests_timer_clock_read);
    D_TEST_REQUIRE(timer.context == &clock);

    return true;
}

/*
d_tests_timer_limit
  Expiry.
  Tests the following:
  - a negative limit is refused
  - remaining counts down while running and stops at zero
  - the timer expires on reaching its limit, running or not
  - an expired timer refuses to start until decrement or reset takes it back
    under its limit
  - a limit of zero is expired at once
*/
bool
d_tests_timer_limit(void)
{
    struct dj_tests_timer_clock clock = { 0 };
    struct dj_timer             timer = D_TIMER_INIT;

    D_TEST_REQUIRE(!d_timer_init_limited(&timer,
                                         -1));
    D_TEST_REQUIRE(d_timer_init_limited(&timer,
                                        100));
    d_timer_set_clock(&timer,
                      d_tests_timer_clock_read,
                      &clock);

    D_TEST_REQUIRE(d_timer_remaining(&timer) == 100);
    D_TEST_REQUIRE(d_timer_start(&timer));
    clock.now = 60;
    D_TEST_REQUIRE(d_timer_remaining(&timer) == 40);
    D_TEST_REQUIRE(!d_timer_expired(&timer));
    clock.now = 100;
    D_TEST_REQUIRE(d_timer_expired(&timer));
    clock.now = 130;
    D_TEST_REQUIRE(d_timer_remaining(&timer) == 0);
    D_TEST_REQUIRE(d_timer_stop(&timer));
    D_TEST_REQUIRE(d_timer_expired(&timer));

    D_TEST_REQUIRE(!d_timer_start(&timer));
    D_TEST_REQUIRE(d_timer_decrement(&timer,
                                     50));
    D_TEST_REQUIRE(!d_timer_expired(&timer));
    D_TEST_REQUIRE(d_timer_start(&timer));

    D_TEST_REQUIRE(d_timer_init_limited(&timer,
                                        0));
    D_TEST_REQUIRE(d_timer_expired(&timer));

    return true;
}

/*
d_tests_timer_clock
  The clock.
  Tests the following:
  - a clock that reads earlier than the start counts no time, open or closed
  - set_clock with NULL restores the monotonic clock
  - on the monotonic clock, a started and stopped timer has elapsed time
    that is not negative, and later readings are not earlier
*/
bool
d_tests_timer_clock(void)
{
    struct dj_tests_timer_clock clock = { 500 };
    struct dj_timer             timer = D_TIMER_INIT;

    d_timer_set_clock(&timer,
                      d_tests_timer_clock_read,
                      &clock);
    D_TEST_REQUIRE(d_timer_start(&timer));
    clock.now = 200;
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 0);
    D_TEST_REQUIRE(d_timer_stop(&timer));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) == 0);

    d_timer_set_clock(&timer,
                      NULL,
                      NULL);
    D_TEST_REQUIRE(timer.clock == NULL);

    const int64_t first = d_timer_clock_monotonic(NULL);

    D_TEST_REQUIRE(d_timer_start(&timer));
    D_TEST_REQUIRE(d_timer_stop(&timer));
    D_TEST_REQUIRE(d_timer_elapsed(&timer) >= 0);
    D_TEST_REQUIRE(d_timer_clock_monotonic(NULL) >= first);

    return true;
}

/*
d_tests_timer_null
  Every function, given NULL.
  Tests the following:
  - the operations return false, and init, set_clock and reset do nothing
  - the queries answer as a stopped timer at zero
  - merge refuses a NULL on either side
*/
bool
d_tests_timer_null(void)
{
    struct dj_timer timer = D_TIMER_INIT;

    d_timer_init(NULL);
    d_timer_set_clock(NULL,
                      NULL,
                      NULL);
    d_timer_reset(NULL);
    D_TEST_REQUIRE(!d_timer_init_limited(NULL,
                                         1));
    D_TEST_REQUIRE(!d_timer_start(NULL));
    D_TEST_REQUIRE(!d_timer_stop(NULL));
    D_TEST_REQUIRE(!d_timer_increment(NULL,
                                      1));
    D_TEST_REQUIRE(!d_timer_decrement(NULL,
                                      1));
    D_TEST_REQUIRE(!d_timer_merge(NULL,
                                  &timer));
    D_TEST_REQUIRE(!d_timer_merge(&timer,
                                  NULL));

    D_TEST_REQUIRE(d_timer_elapsed(NULL) == 0);
    D_TEST_REQUIRE(d_timer_remaining(NULL) == 0);
    D_TEST_REQUIRE(!d_timer_running(NULL));
    D_TEST_REQUIRE(!d_timer_expired(NULL));

    return true;
}
