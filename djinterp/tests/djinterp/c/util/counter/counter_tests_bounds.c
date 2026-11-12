/*******************************************************************************
* djinterp [c][utility]                                   counter_tests_bounds.c
*
* The counter suite, block "bounds": the saturating arithmetic every counter
* shares.
*
*
* path:      /tests/djinterp/c/util/counter/counter_tests_bounds.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./counter_tests.h"  // corresponding header


/*
d_tests_counter_bounds_open
  Math's intervals, read inclusively.
  Tests the following:
  - an open end holds a sum one integer inside it, inexact
  - a one-sided interval limits that side and leaves the other to int64_t
*/
bool
d_tests_counter_bounds_open(void)
{
    const struct d_interval_imax open     =
        d_interval_imax_make(0,
                             10,
                             D_INTERVAL_OPEN,
                             0);
    const struct d_interval_imax floor    =
        d_interval_imax_make(-3,
                             D_MATH_IMAX_MAX,
                             D_INTERVAL_CLOSED,
                             0);
    bool                         is_exact = false;

    D_TEST_REQUIRE(d_counter_bounds_add(&open,
                                        5,
                                        10,
                                        &is_exact) == 9);
    D_TEST_REQUIRE(!is_exact);
    D_TEST_REQUIRE(d_counter_bounds_sub(&open,
                                        5,
                                        10,
                                        &is_exact) == 1);
    D_TEST_REQUIRE(!is_exact);

    D_TEST_REQUIRE(d_counter_bounds_sub(&floor,
                                        0,
                                        10,
                                        &is_exact) == -3);
    D_TEST_REQUIRE(!is_exact);
    D_TEST_REQUIRE(d_counter_bounds_add(&floor,
                                        INT64_MAX - 1,
                                        5,
                                        &is_exact) == INT64_MAX);
    D_TEST_REQUIRE(!is_exact);

    return true;
}

/*
d_tests_counter_bounds_add
  The sum, held twice.
  Tests the following:
  - an in-range sum is exact
  - a sum past INT64_MAX or INT64_MIN stops there, inexact, with no
    overflow on the way (UBSan builds see it)
  - a sum past a bound stops at it, inexact
  - a value already outside its bounds is pulled in by any step
  - NULL bounds and a NULL exactness are both accepted
*/
bool
d_tests_counter_bounds_add(void)
{
    const struct d_interval_imax range = d_tests_counter_range(0,
                                                               10);
    bool                         exact = false;

    D_TEST_REQUIRE(d_counter_bounds_add(&range,
                                        3,
                                        4,
                                        &exact) == 7);
    D_TEST_REQUIRE(exact);

    D_TEST_REQUIRE(d_counter_bounds_add(NULL,
                                        INT64_MAX - 1,
                                        5,
                                        &exact) ==
                   INT64_MAX);
    D_TEST_REQUIRE(!exact);
    D_TEST_REQUIRE(d_counter_bounds_add(NULL,
                                        INT64_MIN + 1,
                                        -5,
                                        &exact) ==
                   INT64_MIN);
    D_TEST_REQUIRE(!exact);
    D_TEST_REQUIRE(d_counter_bounds_add(NULL,
                                        INT64_MIN,
                                        INT64_MAX,
                                        &exact) ==
                   -1);
    D_TEST_REQUIRE(exact);

    D_TEST_REQUIRE(d_counter_bounds_add(&range,
                                        8,
                                        5,
                                        &exact) == 10);
    D_TEST_REQUIRE(!exact);
    D_TEST_REQUIRE(d_counter_bounds_add(&range,
                                        2,
                                        -5,
                                        &exact) == 0);
    D_TEST_REQUIRE(!exact);

    D_TEST_REQUIRE(d_counter_bounds_add(&range,
                                        20,
                                        0,
                                        &exact) == 10);
    D_TEST_REQUIRE(!exact);

    D_TEST_REQUIRE(d_counter_bounds_add(&range,
                                        1,
                                        1,
                                        NULL) == 2);

    return true;
}

/*
d_tests_counter_bounds_sub
  The difference, held the same way.
  Tests the following:
  - an in-range difference is exact
  - INT64_MIN, which has no negation, moves up by 2^63 and saturates
  - from below zero, subtracting INT64_MIN fits, and is exact
  - a negative amount moves the value up
  - a difference past a bound stops at it, inexact
*/
bool
d_tests_counter_bounds_sub(void)
{
    const struct d_interval_imax range = d_tests_counter_range(0,
                                                               10);
    bool                         exact = false;

    D_TEST_REQUIRE(d_counter_bounds_sub(NULL,
                                        9,
                                        4,
                                        &exact) == 5);
    D_TEST_REQUIRE(exact);

    D_TEST_REQUIRE(d_counter_bounds_sub(NULL,
                                        0,
                                        INT64_MIN,
                                        &exact) ==
                   INT64_MAX);
    D_TEST_REQUIRE(!exact);
    D_TEST_REQUIRE(d_counter_bounds_sub(NULL,
                                        -5,
                                        INT64_MIN,
                                        &exact) ==
                   INT64_MAX - 4);
    D_TEST_REQUIRE(exact);

    D_TEST_REQUIRE(d_counter_bounds_sub(NULL,
                                        1,
                                        -2,
                                        &exact) == 3);
    D_TEST_REQUIRE(exact);

    D_TEST_REQUIRE(d_counter_bounds_sub(&range,
                                        3,
                                        9,
                                        &exact) == 0);
    D_TEST_REQUIRE(!exact);
    D_TEST_REQUIRE(d_counter_bounds_sub(&range,
                                        3,
                                        INT64_MIN,
                                        &exact) == 10);
    D_TEST_REQUIRE(!exact);

    return true;
}
