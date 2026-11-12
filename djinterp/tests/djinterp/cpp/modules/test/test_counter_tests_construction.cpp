/*******************************************************************************
* djinterp [cpp][test]                       test_counter_tests_construction.cpp
*
*
* path:      /tests/djinterp/cpp/modules/test/test_counter_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_counter_default_ctor
  Verifies the default constructor's initial state.
  Tests the following:
  - value and initial are zero
  - bounds default to the value type's extremes
  - it owns no children and observes nothing
*/
bool tests_test_counter_default_ctor()
{
    tc c;

    D_TC_CHECK(c.value()          == 0);
    D_TC_CHECK(c.initial()        == 0);
    D_TC_CHECK(c.min()            == std::numeric_limits<int>::lowest());
    D_TC_CHECK(c.max()            == std::numeric_limits<int>::max());
    D_TC_CHECK(c.child_count()    == 0);
    D_TC_CHECK(c.observed_count() == 0);

    return true;
}


/*
tests_test_counter_value_ctor
  Verifies the value-only constructor (bounds default).
  Tests the following:
  - value and initial take the supplied value
  - bounds default to the value type's extremes
*/
bool tests_test_counter_value_ctor()
{
    tc c(7);

    D_TC_CHECK(c.value()   == 7);
    D_TC_CHECK(c.initial() == 7);
    D_TC_CHECK(c.min()     == std::numeric_limits<int>::lowest());
    D_TC_CHECK(c.max()     == std::numeric_limits<int>::max());

    return true;
}


/*
tests_test_counter_value_bounds_ctor
  Verifies the value-plus-bounds constructor.
  Tests the following:
  - value, initial, min, and max are all stored
*/
bool tests_test_counter_value_bounds_ctor()
{
    tc c(5, 0, 100);

    D_TC_CHECK(c.value()   == 5);
    D_TC_CHECK(c.initial() == 5);
    D_TC_CHECK(c.min()     == 0);
    D_TC_CHECK(c.max()     == 100);

    return true;
}


NS_END  // testing
NS_END  // djinterp
