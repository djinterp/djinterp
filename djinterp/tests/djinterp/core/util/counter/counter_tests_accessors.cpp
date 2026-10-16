/*******************************************************************************
* djinterp [core]                                    counter_tests_accessors.cpp
*
*
* path:      /tests/djinterp/core/util/counter/counter_tests_accessors.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_counter_value
  Verifies that value() tracks the live counter value.
  Tests the following:
  - value() returns the construction value initially
  - value() reflects a subsequent increment
*/
bool
tests_counter_value()
{
    ctr c(3, 0, 10);

    D_CTR_CHECK(c.value() == 3);

    c.increment(4);

    D_CTR_CHECK(c.value() == 7);

    return true;
}


/*
tests_counter_initial
  Verifies that initial() reports the construction value and never changes.
  Tests the following:
  - initial() returns the value supplied at construction
  - mutating the live value leaves initial() unchanged
*/
bool
tests_counter_initial()
{
    ctr c(3, 0, 10);

    D_CTR_CHECK(c.initial() == 3);

    c.increment(4);

    D_CTR_CHECK(c.initial() == 3);
    D_CTR_CHECK(c.value()   == 7);

    return true;
}


/*
tests_counter_min
  Verifies that min() returns the configured lower bound.
  Tests the following:
  - min() returns the supplied (here negative) minimum
*/
bool
tests_counter_min()
{
    ctr c(5, -3, 10);

    D_CTR_CHECK(c.min() == -3);

    return true;
}


/*
tests_counter_max
  Verifies that max() returns the configured upper bound.
  Tests the following:
  - max() returns the supplied maximum
*/
bool
tests_counter_max()
{
    ctr c(5, -3, 10);

    D_CTR_CHECK(c.max() == 10);

    return true;
}


/*
tests_counter_at_min_equal
  Verifies at_min() at the boundary where the value equals the minimum.
  Tests the following:
  - at_min() is true when value == min
  - at_max() is false at the same time
*/
bool
tests_counter_at_min_equal()
{
    ctr c(0, 0, 10);

    D_CTR_CHECK(c.at_min() == true);
    D_CTR_CHECK(c.at_max() == false);

    return true;
}


/*
tests_counter_at_min_below
  Verifies at_min() when the value sits below the minimum (the constructor
  does not clamp the initial value).
  Tests the following:
  - an initial below the minimum is stored verbatim
  - at_min() (a <= comparison) reports true below the bound, not only at it
*/
bool
tests_counter_at_min_below()
{
    ctr c(-5, 0, 10);

    D_CTR_CHECK(c.value()  == -5);
    D_CTR_CHECK(c.at_min() == true);

    return true;
}


/*
tests_counter_at_min_above
  Verifies at_min() when the value is strictly above the minimum.
  Tests the following:
  - at_min() is false when value > min
*/
bool
tests_counter_at_min_above()
{
    ctr c(5, 0, 10);

    D_CTR_CHECK(c.at_min() == false);

    return true;
}


/*
tests_counter_at_max_equal
  Verifies at_max() at the boundary where the value equals the maximum.
  Tests the following:
  - at_max() is true when value == max
  - at_min() is false at the same time
*/
bool
tests_counter_at_max_equal()
{
    ctr c(10, 0, 10);

    D_CTR_CHECK(c.at_max() == true);
    D_CTR_CHECK(c.at_min() == false);

    return true;
}


/*
tests_counter_at_max_above
  Verifies at_max() when the value sits above the maximum (the constructor
  does not clamp the initial value).
  Tests the following:
  - an initial above the maximum is stored verbatim
  - at_max() (a >= comparison) reports true above the bound, not only at it
*/
bool
tests_counter_at_max_above()
{
    ctr c(15, 0, 10);

    D_CTR_CHECK(c.value()  == 15);
    D_CTR_CHECK(c.at_max() == true);

    return true;
}


/*
tests_counter_at_max_below
  Verifies at_max() when the value is strictly below the maximum.
  Tests the following:
  - at_max() is false when value < max
*/
bool
tests_counter_at_max_below()
{
    ctr c(5, 0, 10);

    D_CTR_CHECK(c.at_max() == false);

    return true;
}


/*
tests_counter_const_accessors
  Verifies that every query is callable through a const reference, including
  the const child() overload and observed().
  Tests the following:
  - value/initial/min/max/at_min/at_max read correctly on a const counter
  - child_count(), the const child(), observed(), and observed_count() all
    work through the const reference
*/
bool
tests_counter_const_accessors()
{
    ctr c(5, 0, 10);
    ctr external(2, 0, 10);

    c.add_child(7, 0, 10);
    c.observe(external);

    const ctr& cc = c;

    D_CTR_CHECK(cc.value()          == 5);
    D_CTR_CHECK(cc.initial()        == 5);
    D_CTR_CHECK(cc.min()            == 0);
    D_CTR_CHECK(cc.max()            == 10);
    D_CTR_CHECK(cc.at_min()         == false);
    D_CTR_CHECK(cc.at_max()         == false);
    D_CTR_CHECK(cc.child_count()    == 1);
    D_CTR_CHECK(cc.child(0).value() == 7);
    D_CTR_CHECK(cc.observed_count() == 1);
    D_CTR_CHECK(cc.observed(0)      == &external);

    return true;
}


/*
tests_counter_default_bounds_extremes
  Verifies the bound queries and at_min/at_max for a default counter whose
  bounds are the value_type extremes.
  Tests the following:
  - min()/max() return the lowest/largest value_type
  - zero lies strictly between the extremes, so at_min() and at_max() are both
    false
*/
bool
tests_counter_default_bounds_extremes()
{
    ctr c;

    D_CTR_CHECK(c.min()    == k_lowest);
    D_CTR_CHECK(c.max()    == k_highest);
    D_CTR_CHECK(c.at_min() == false);
    D_CTR_CHECK(c.at_max() == false);

    return true;
}


NS_END  // testing
NS_END  // djinterp
