/*******************************************************************************
* djinterp [c][utility]                                  counter_tests_counter.c
*
* The counter suite, block "counter": counter.h's counter and its five
* operations.
*
*
* path:      /tests/djinterp/c/util/counter/counter_tests_counter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./counter_tests.h"  // corresponding header

// std
#include <string.h>  // memset


/*
d_tests_counter_zero
  A counter needs no init.
  Tests the following:
  - D_COUNTER_INIT and a zero-filled struct are the same counter
  - it starts at 0, unbounded, and counts at once
*/
bool
d_tests_counter_zero(void)
{
    struct dj_counter from_macro = D_COUNTER_INIT;
    struct dj_counter from_fill  = { 7, 7, { 7, 7, 7, 7u }, true };

    memset(&from_fill,
           0,
           sizeof(from_fill));

    D_TEST_REQUIRE(from_macro.initial == from_fill.initial);
    D_TEST_REQUIRE(from_macro.value == from_fill.value);
    D_TEST_REQUIRE(from_macro.has_bounds == from_fill.has_bounds);
    D_TEST_REQUIRE(!from_fill.has_bounds);

    D_TEST_REQUIRE(d_counter_value(&from_fill) == 0);
    D_TEST_REQUIRE(!d_counter_at_min(&from_fill));
    D_TEST_REQUIRE(!d_counter_at_max(&from_fill));
    D_TEST_REQUIRE(d_counter_increment(&from_fill,
                                       3));
    D_TEST_REQUIRE(d_counter_value(&from_fill) == 3);

    return true;
}

/*
d_tests_counter_init
  The two init functions.
  Tests the following:
  - init sets the value and the start, and removes any bounds
  - init_bounded sets all three, and accepts a start on either bound
  - init_bounded refuses a start outside the bounds, and inverted, empty or
    discrete bounds, and leaves the counter as it was
  - init_bounded with NULL bounds is init
*/
bool
d_tests_counter_init(void)
{
    const struct d_interval_imax range  = d_tests_counter_range(0,
                                                                10);
    const struct d_interval_imax upside = d_tests_counter_range(5,
                                                                1);
    const struct d_interval_imax empty    =
        d_interval_imax_make(2,
                             3,
                             D_INTERVAL_OPEN,
                             0);
    const struct d_interval_imax discrete =
        d_interval_imax_make(0,
                             10,
                             D_INTERVAL_CLOSED,
                             2);
    struct dj_counter            counter  = D_COUNTER_INIT;

    counter.bounds     = range;
    counter.has_bounds = true;
    d_counter_init(&counter,
                   -4);
    D_TEST_REQUIRE(counter.value == -4);
    D_TEST_REQUIRE(counter.initial == -4);
    D_TEST_REQUIRE(!counter.has_bounds);

    D_TEST_REQUIRE(d_counter_init_bounded(&counter,
                                          10,
                                          &range));
    D_TEST_REQUIRE(counter.value == 10);
    D_TEST_REQUIRE(counter.initial == 10);
    D_TEST_REQUIRE(counter.has_bounds);
    D_TEST_REQUIRE(counter.bounds.upper == 10);
    D_TEST_REQUIRE(d_counter_init_bounded(&counter,
                                          0,
                                          &range));

    D_TEST_REQUIRE(!d_counter_init_bounded(&counter,
                                           11,
                                           &range));
    D_TEST_REQUIRE(!d_counter_init_bounded(&counter,
                                           -1,
                                           &range));
    D_TEST_REQUIRE(counter.value == 0);
    D_TEST_REQUIRE(counter.has_bounds);

    D_TEST_REQUIRE(!d_counter_init_bounded(&counter,
                                           3,
                                           &upside));
    D_TEST_REQUIRE(!d_counter_init_bounded(&counter,
                                           2,
                                           &empty));
    D_TEST_REQUIRE(!d_counter_init_bounded(&counter,
                                           4,
                                           &discrete));
    D_TEST_REQUIRE(counter.value == 0);

    D_TEST_REQUIRE(d_counter_init_bounded(&counter,
                                          99,
                                          NULL));
    D_TEST_REQUIRE(!counter.has_bounds);

    return true;
}

/*
d_tests_counter_step
  Increment and decrement.
  Tests the following:
  - each moves by its amount, either sign, and reports a full move
  - a move past a bound stops on it, reports the cut, and at_max or at_min
    turns true
  - a move away from a bound is exact again
  - an unbounded counter saturates at the int64_t range
*/
bool
d_tests_counter_step(void)
{
    const struct d_interval_imax range   = d_tests_counter_range(0,
                                                                 10);
    struct dj_counter            counter = D_COUNTER_INIT;

    D_TEST_REQUIRE(d_counter_init_bounded(&counter,
                                          5,
                                          &range));

    D_TEST_REQUIRE(d_counter_increment(&counter,
                                       2));
    D_TEST_REQUIRE(counter.value == 7);
    D_TEST_REQUIRE(d_counter_increment(&counter,
                                       -1));
    D_TEST_REQUIRE(counter.value == 6);
    D_TEST_REQUIRE(d_counter_decrement(&counter,
                                       1));
    D_TEST_REQUIRE(counter.value == 5);
    D_TEST_REQUIRE(d_counter_decrement(&counter,
                                       -2));
    D_TEST_REQUIRE(counter.value == 7);

    D_TEST_REQUIRE(!d_counter_increment(&counter,
                                        100));
    D_TEST_REQUIRE(counter.value == 10);
    D_TEST_REQUIRE(d_counter_at_max(&counter));
    D_TEST_REQUIRE(d_counter_decrement(&counter,
                                       3));
    D_TEST_REQUIRE(!d_counter_at_max(&counter));

    D_TEST_REQUIRE(!d_counter_decrement(&counter,
                                        INT64_MIN));
    D_TEST_REQUIRE(counter.value == 10);
    D_TEST_REQUIRE(!d_counter_decrement(&counter,
                                        INT64_MAX));
    D_TEST_REQUIRE(counter.value == 0);
    D_TEST_REQUIRE(d_counter_at_min(&counter));

    d_counter_init(&counter,
                   INT64_MAX - 1);
    D_TEST_REQUIRE(!d_counter_increment(&counter,
                                        2));
    D_TEST_REQUIRE(counter.value == INT64_MAX);
    D_TEST_REQUIRE(!d_counter_at_max(&counter));

    return true;
}

/*
d_tests_counter_compare
  Three-way, by value.
  Tests the following:
  - less, equal and greater are -1, 0 and 1
  - starts and bounds play no part
  - the extremes compare without overflow
  - NULL sorts before any counter and equals itself
*/
bool
d_tests_counter_compare(void)
{
    struct dj_counter low  = D_COUNTER_INIT;
    struct dj_counter high = D_COUNTER_INIT;
    struct dj_counter same = D_COUNTER_INIT;

    d_counter_init(&low,
                   1);
    d_counter_init(&high,
                   2);
    d_counter_init(&same,
                   0);
    D_TEST_REQUIRE(d_counter_increment(&same,
                                       1));

    D_TEST_REQUIRE(d_counter_compare(&low,
                                     &high) == -1);
    D_TEST_REQUIRE(d_counter_compare(&high,
                                     &low) == 1);
    D_TEST_REQUIRE(d_counter_compare(&low,
                                     &same) == 0);

    d_counter_init(&low,
                   INT64_MIN);
    d_counter_init(&high,
                   INT64_MAX);
    D_TEST_REQUIRE(d_counter_compare(&low,
                                     &high) == -1);
    D_TEST_REQUIRE(d_counter_compare(&high,
                                     &low) == 1);

    D_TEST_REQUIRE(d_counter_compare(NULL,
                                     &low) == -1);
    D_TEST_REQUIRE(d_counter_compare(&low,
                                     NULL) == 1);
    D_TEST_REQUIRE(d_counter_compare(NULL,
                                     NULL) == 0);

    return true;
}

/*
d_tests_counter_merge
  Movement, not value.
  Tests the following:
  - two counters from 10 that each count 3 merge to 16
  - a source that moved down takes the destination down
  - the destination's bounds hold, and a cut merge is reported
  - a source whose movement saturates makes the merge inexact
  - a counter merged into itself adds its own movement once
*/
bool
d_tests_counter_merge(void)
{
    const struct d_interval_imax range = d_tests_counter_range(0,
                                                               20);
    struct dj_counter            dest  = D_COUNTER_INIT;
    struct dj_counter            src   = D_COUNTER_INIT;

    d_counter_init(&dest,
                   10);
    d_counter_init(&src,
                   10);
    D_TEST_REQUIRE(d_counter_increment(&dest,
                                       3));
    D_TEST_REQUIRE(d_counter_increment(&src,
                                       3));
    D_TEST_REQUIRE(d_counter_merge(&dest,
                                   &src));
    D_TEST_REQUIRE(dest.value == 16);
    D_TEST_REQUIRE(src.value == 13);

    D_TEST_REQUIRE(d_counter_decrement(&src,
                                       8));
    D_TEST_REQUIRE(d_counter_merge(&dest,
                                   &src));
    D_TEST_REQUIRE(dest.value == 11);

    D_TEST_REQUIRE(d_counter_init_bounded(&dest,
                                          18,
                                          &range));
    D_TEST_REQUIRE(d_counter_increment(&src,
                                       10));
    D_TEST_REQUIRE(!d_counter_merge(&dest,
                                    &src));
    D_TEST_REQUIRE(dest.value == 20);

    d_counter_init(&dest,
                   0);
    d_counter_init(&src,
                   INT64_MIN);
    src.value = INT64_MAX;
    D_TEST_REQUIRE(!d_counter_merge(&dest,
                                    &src));
    D_TEST_REQUIRE(dest.value == INT64_MAX);

    d_counter_init(&dest,
                   5);
    D_TEST_REQUIRE(d_counter_increment(&dest,
                                       4));
    D_TEST_REQUIRE(d_counter_merge(&dest,
                                   &dest));
    D_TEST_REQUIRE(dest.value == 13);

    return true;
}

/*
d_tests_counter_reset
  Back to the start.
  Tests the following:
  - reset returns the value to the start, not to zero
  - the bounds stay
  - delta is the movement since the start, and 0 after a reset
*/
bool
d_tests_counter_reset(void)
{
    const struct d_interval_imax range   = d_tests_counter_range(-5,
                                                                 5);
    struct dj_counter            counter = D_COUNTER_INIT;

    D_TEST_REQUIRE(d_counter_init_bounded(&counter,
                                          2,
                                          &range));
    D_TEST_REQUIRE(!d_counter_increment(&counter,
                                        9));
    D_TEST_REQUIRE(d_counter_delta(&counter) == 3);

    d_counter_reset(&counter);
    D_TEST_REQUIRE(counter.value == 2);
    D_TEST_REQUIRE(d_counter_delta(&counter) == 0);
    D_TEST_REQUIRE(counter.has_bounds);
    D_TEST_REQUIRE(counter.bounds.upper == 5);

    return true;
}

/*
d_tests_counter_null
  Every function, given NULL.
  Tests the following:
  - the operations return false, and init and reset do nothing
  - the queries answer as an empty counter
  - merge refuses a NULL on either side
*/
bool
d_tests_counter_null(void)
{
    struct dj_counter counter = D_COUNTER_INIT;

    d_counter_init(NULL,
                   1);
    d_counter_reset(NULL);
    D_TEST_REQUIRE(!d_counter_init_bounded(NULL,
                                           0,
                                           NULL));
    D_TEST_REQUIRE(!d_counter_increment(NULL,
                                        1));
    D_TEST_REQUIRE(!d_counter_decrement(NULL,
                                        1));
    D_TEST_REQUIRE(!d_counter_merge(NULL,
                                    &counter));
    D_TEST_REQUIRE(!d_counter_merge(&counter,
                                    NULL));

    D_TEST_REQUIRE(d_counter_value(NULL) == 0);
    D_TEST_REQUIRE(d_counter_delta(NULL) == 0);
    D_TEST_REQUIRE(!d_counter_at_min(NULL));
    D_TEST_REQUIRE(!d_counter_at_max(NULL));

    return true;
}
