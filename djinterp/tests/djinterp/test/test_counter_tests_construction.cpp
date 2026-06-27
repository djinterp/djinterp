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
  - it owns no children, observes nothing, and has no event handler
*/
bool
tests_test_counter_default_ctor()
{
    tc c;

    D_TC_CHECK(c.value()          == 0);
    D_TC_CHECK(c.initial()        == 0);
    D_TC_CHECK(c.min()            == std::numeric_limits<int>::lowest());
    D_TC_CHECK(c.max()            == std::numeric_limits<int>::max());
    D_TC_CHECK(c.child_count()    == 0);
    D_TC_CHECK(c.observed_count() == 0);
    D_TC_CHECK(c.handler()        == nullptr);

    return true;
}


/*
tests_test_counter_value_ctor
  Verifies the value-only constructor (bounds and handler default).
  Tests the following:
  - value and initial take the supplied value
  - bounds default to the value type's extremes
  - the handler defaults to nullptr
*/
bool
tests_test_counter_value_ctor()
{
    tc c(7);

    D_TC_CHECK(c.value()   == 7);
    D_TC_CHECK(c.initial() == 7);
    D_TC_CHECK(c.min()     == std::numeric_limits<int>::lowest());
    D_TC_CHECK(c.max()     == std::numeric_limits<int>::max());
    D_TC_CHECK(c.handler() == nullptr);

    return true;
}


/*
tests_test_counter_value_bounds_ctor
  Verifies the value-plus-bounds constructor (handler defaults).
  Tests the following:
  - value, initial, min, and max are all stored
  - the handler defaults to nullptr when only bounds are supplied
*/
bool
tests_test_counter_value_bounds_ctor()
{
    tc c(5, 0, 100);

    D_TC_CHECK(c.value()   == 5);
    D_TC_CHECK(c.initial() == 5);
    D_TC_CHECK(c.min()     == 0);
    D_TC_CHECK(c.max()     == 100);
    D_TC_CHECK(c.handler() == nullptr);

    return true;
}


/*
tests_test_counter_value_bounds_handler_ctor
  Verifies the full constructor with an event dispatcher.
  Tests the following:
  - value and bounds are stored
  - the supplied dispatcher is stored
*/
bool
tests_test_counter_value_bounds_handler_ctor()
{
    event_dispatcher eh;
    tc c(5, 0, 100, &eh);

    D_TC_CHECK(c.value()   == 5);
    D_TC_CHECK(c.min()     == 0);
    D_TC_CHECK(c.max()     == 100);
    D_TC_CHECK(c.handler() == &eh);

    return true;
}


NS_END  // testing
NS_END  // djinterp
