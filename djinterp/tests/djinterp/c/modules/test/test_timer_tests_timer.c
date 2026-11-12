/*******************************************************************************
* djinterp [c][test]                                    test_timer_tests_timer.c
*
* The test_timer suite, block "timer": the test timer, timer.h's timer as
* DTest's.
*
*
* path:      /tests/djinterp/c/modules/test/test_timer_tests_timer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.11
*                                                            revised: 2026.11.11
*******************************************************************************/
#include "./test_timer_tests.h"  // corresponding header

// std
#include <string.h>  // memset


/*
d_tests_test_timer_init
  Setting up a test timer.
  Tests the following:
  - init gives a stopped timer at zero, with no limit, on the clock
    D_TEST_TIMER_INIT gives
  - set_clock changes the clock, and NULL restores the monotonic one
*/
bool
d_tests_test_timer_init(void)
{
    struct dj_tests_test_timer_clock clock = { 0 };
    struct dj_test_timer             macro = D_TEST_TIMER_INIT;
    struct dj_test_timer             timer = { 0 };

    memset(&timer,
           0x5a,
           sizeof(timer));
    d_test_timer_init(&timer);
    D_TEST_REQUIRE(!d_test_timer_running(&timer));
    D_TEST_REQUIRE(d_test_timer_elapsed(&timer) == 0);
    D_TEST_REQUIRE(!timer.timer.has_limit);
    D_TEST_REQUIRE(timer.timer.clock == macro.timer.clock);
    D_TEST_REQUIRE(macro.timer.elapsed == 0);
    D_TEST_REQUIRE(!macro.timer.running);

    d_test_timer_set_clock(&timer,
                           d_tests_test_timer_clock_read,
                           &clock);
    D_TEST_REQUIRE(timer.timer.clock == d_tests_test_timer_clock_read);
    D_TEST_REQUIRE(timer.timer.context == &clock);
    d_test_timer_set_clock(&timer,
                           NULL,
                           NULL);
    D_TEST_REQUIRE(timer.timer.clock != d_tests_test_timer_clock_read);

    return true;
}

/*
d_tests_test_timer_start_stop
  Intervals, on a hand-wound clock.
  Tests the following:
  - start opens an interval at the clock's reading; elapsed includes it
  - stop closes it into the elapsed time
  - a second start, or a stop while stopped, is refused
  - intervals accumulate
*/
bool
d_tests_test_timer_start_stop(void)
{
    struct dj_tests_test_timer_clock clock = { 100 };
    struct dj_test_timer             timer = D_TEST_TIMER_INIT;

    d_test_timer_init(&timer);
    d_test_timer_set_clock(&timer,
                           d_tests_test_timer_clock_read,
                           &clock);

    D_TEST_REQUIRE(!d_test_timer_stop(&timer));
    D_TEST_REQUIRE(d_test_timer_start(&timer));
    D_TEST_REQUIRE(!d_test_timer_start(&timer));
    D_TEST_REQUIRE(d_test_timer_running(&timer));
    clock.now = 160;
    D_TEST_REQUIRE(d_test_timer_elapsed(&timer) == 60);
    D_TEST_REQUIRE(d_test_timer_stop(&timer));
    D_TEST_REQUIRE(!d_test_timer_running(&timer));

    clock.now = 500;
    D_TEST_REQUIRE(d_test_timer_start(&timer));
    clock.now = 540;
    D_TEST_REQUIRE(d_test_timer_stop(&timer));
    D_TEST_REQUIRE(d_test_timer_elapsed(&timer) == 100);

    return true;
}

/*
d_tests_test_timer_operations
  The five operations and as_timer.
  Tests the following:
  - increment and decrement add and remove nanoseconds, held at zero
  - compare orders by elapsed time
  - merge adds the source's elapsed time
  - reset stops and zeroes
  - as_timer is the timer the struct wraps, for timer.h
*/
bool
d_tests_test_timer_operations(void)
{
    struct dj_tests_test_timer_clock clock = { 0 };
    struct dj_test_timer             a     = D_TEST_TIMER_INIT;
    struct dj_test_timer             b     = D_TEST_TIMER_INIT;

    d_test_timer_init(&a);
    d_test_timer_init(&b);
    d_test_timer_set_clock(&a,
                           d_tests_test_timer_clock_read,
                           &clock);

    D_TEST_REQUIRE(d_test_timer_increment(&a,
                                          30));
    D_TEST_REQUIRE(d_test_timer_decrement(&a,
                                          10));
    D_TEST_REQUIRE(d_test_timer_elapsed(&a) == 20);
    D_TEST_REQUIRE(!d_test_timer_decrement(&a,
                                           50));
    D_TEST_REQUIRE(d_test_timer_elapsed(&a) == 0);
    D_TEST_REQUIRE(d_test_timer_increment(&a,
                                          20));
    D_TEST_REQUIRE(d_test_timer_increment(&b,
                                          45));

    D_TEST_REQUIRE(d_test_timer_compare(&a,
                                        &b) == -1);
    D_TEST_REQUIRE(d_test_timer_compare(&b,
                                        &a) == 1);
    D_TEST_REQUIRE(d_test_timer_compare(&a,
                                        &a) == 0);

    D_TEST_REQUIRE(d_test_timer_merge(&a,
                                      &b));
    D_TEST_REQUIRE(d_test_timer_elapsed(&a) == 65);
    D_TEST_REQUIRE(d_test_timer_elapsed(&b) == 45);

    D_TEST_REQUIRE(d_test_timer_start(&a));
    d_test_timer_reset(&a);
    D_TEST_REQUIRE(!d_test_timer_running(&a));
    D_TEST_REQUIRE(d_test_timer_elapsed(&a) == 0);

    D_TEST_REQUIRE(d_test_timer_as_timer(&a) == &a.timer);
    D_TEST_REQUIRE(d_timer_increment(d_test_timer_as_timer(&a),
                                     9));
    D_TEST_REQUIRE(d_test_timer_elapsed(&a) == 9);

    return true;
}

/*
d_tests_test_timer_elapsed_ms
  The report's unit.
  Tests the following:
  - elapsed_ms is the elapsed nanoseconds divided by one million
  - an open interval counts
*/
bool
d_tests_test_timer_elapsed_ms(void)
{
    struct dj_tests_test_timer_clock clock = { 0 };
    struct dj_test_timer             timer = D_TEST_TIMER_INIT;

    d_test_timer_init(&timer);
    d_test_timer_set_clock(&timer,
                           d_tests_test_timer_clock_read,
                           &clock);

    D_TEST_REQUIRE(d_test_timer_elapsed_ms(&timer) == 0.0);
    D_TEST_REQUIRE(d_test_timer_increment(&timer,
                                          2500000));
    D_TEST_REQUIRE(d_test_timer_elapsed_ms(&timer) == 2.5);

    D_TEST_REQUIRE(d_test_timer_start(&timer));
    clock.now = 1000000;
    D_TEST_REQUIRE(d_test_timer_elapsed_ms(&timer) == 3.5);

    return true;
}

/*
d_tests_test_timer_null
  Every function, given NULL.
  Tests the following:
  - the operations return false, and init, set_clock and reset do nothing
  - the queries answer as a stopped timer at zero, and as_timer NULL
*/
bool
d_tests_test_timer_null(void)
{
    struct dj_test_timer timer = D_TEST_TIMER_INIT;

    d_test_timer_init(NULL);
    d_test_timer_set_clock(NULL,
                           NULL,
                           NULL);
    d_test_timer_reset(NULL);
    D_TEST_REQUIRE(!d_test_timer_start(NULL));
    D_TEST_REQUIRE(!d_test_timer_stop(NULL));
    D_TEST_REQUIRE(!d_test_timer_increment(NULL,
                                           1));
    D_TEST_REQUIRE(!d_test_timer_decrement(NULL,
                                           1));
    D_TEST_REQUIRE(!d_test_timer_merge(NULL,
                                       &timer));
    D_TEST_REQUIRE(!d_test_timer_merge(&timer,
                                       NULL));

    D_TEST_REQUIRE(d_test_timer_elapsed(NULL) == 0);
    D_TEST_REQUIRE(d_test_timer_elapsed_ms(NULL) == 0.0);
    D_TEST_REQUIRE(!d_test_timer_running(NULL));
    D_TEST_REQUIRE(d_test_timer_compare(NULL,
                                        NULL) == 0);
    D_TEST_REQUIRE(d_test_timer_compare(NULL,
                                        &timer) == -1);
    D_TEST_REQUIRE(d_test_timer_as_timer(NULL) == NULL);

    return true;
}
