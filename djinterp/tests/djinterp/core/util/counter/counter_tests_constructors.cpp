/*******************************************************************************
* djinterp [core]                                 counter_tests_constructors.cpp
*
*
* path:      /tests/djinterp/core/util/counter/counter_tests_constructors.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_counter_default_ctor
  Verifies the default constructor's initial state.
  Tests the following:
  - value() and initial() are both zero
  - min() is the lowest representable value_type, max() the largest
  - the counter owns no children and observes nothing
  - the template's default value_type is std::int64_t
*/
bool
tests_counter_default_ctor()
{
    static_assert(std::is_same<counter<>::value_type, std::int64_t>::value,
                  "default counter value_type should be std::int64_t");

    ctr c;

    D_CTR_CHECK(c.value()          == 0);
    D_CTR_CHECK(c.initial()        == 0);
    D_CTR_CHECK(c.min()            == k_lowest);
    D_CTR_CHECK(c.max()            == k_highest);
    D_CTR_CHECK(c.child_count()    == 0);
    D_CTR_CHECK(c.observed_count() == 0);

    return true;
}


/*
tests_counter_value_ctor
  Verifies the value/bounds constructor stores every field it is given.
  Tests the following:
  - value() and initial() both equal the supplied initial
  - min() and max() return the supplied bounds
*/
bool
tests_counter_value_ctor()
{
    ctr c(5, 0, 10);

    D_CTR_CHECK(c.value()   == 5);
    D_CTR_CHECK(c.initial() == 5);
    D_CTR_CHECK(c.min()     == 0);
    D_CTR_CHECK(c.max()     == 10);

    return true;
}


/*
tests_counter_value_ctor_default_bounds
  Verifies that supplying only an initial value leaves the bounds at their
  widest defaults.
  Tests the following:
  - value() and initial() equal the supplied initial
  - min() falls back to the lowest value_type and max() to the largest
*/
bool
tests_counter_value_ctor_default_bounds()
{
    ctr c(7);

    D_CTR_CHECK(c.value()   == 7);
    D_CTR_CHECK(c.initial() == 7);
    D_CTR_CHECK(c.min()     == k_lowest);
    D_CTR_CHECK(c.max()     == k_highest);

    return true;
}


/*
tests_counter_value_ctor_min_only
  Verifies the middle default argument: supplying an initial and a minimum
  but no maximum.
  Tests the following:
  - min() returns the supplied lower bound
  - max() falls back to the largest value_type
*/
bool
tests_counter_value_ctor_min_only()
{
    ctr c(7, 2);

    D_CTR_CHECK(c.value()   == 7);
    D_CTR_CHECK(c.initial() == 7);
    D_CTR_CHECK(c.min()     == 2);
    D_CTR_CHECK(c.max()     == k_highest);

    return true;
}


/*
tests_counter_negative_bounds
  Verifies construction with negative bounds and a negative initial value,
  confirming the signed domain is handled correctly.
  Tests the following:
  - a negative initial value is stored verbatim
  - negative minimum and positive maximum bounds are stored verbatim
*/
bool
tests_counter_negative_bounds()
{
    ctr c(-5, -10, 10);

    D_CTR_CHECK(c.value()   == -5);
    D_CTR_CHECK(c.initial() == -5);
    D_CTR_CHECK(c.min()     == -10);
    D_CTR_CHECK(c.max()     == 10);

    return true;
}


NS_END  // testing
NS_END  // djinterp
