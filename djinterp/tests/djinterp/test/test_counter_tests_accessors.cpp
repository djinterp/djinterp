// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_counter_value
  Verifies value() reflects the current value.
  Tests the following:
  - value() returns the constructed value and tracks increments
*/
bool
tests_test_counter_value()
{
    tc c(42, 0, 100);

    D_TC_CHECK(c.value() == 42);

    c.increment(8);

    D_TC_CHECK(c.value() == 50);

    return true;
}


/*
tests_test_counter_initial
  Verifies initial() returns the construction value and is immutable.
  Tests the following:
  - initial() returns the constructed value
  - mutating the counter does not change initial()
*/
bool
tests_test_counter_initial()
{
    tc c(42, 0, 100);

    D_TC_CHECK(c.initial() == 42);

    c.increment(8);

    D_TC_CHECK(c.initial() == 42);

    return true;
}


/*
tests_test_counter_min
  Verifies min() returns the lower bound.
  Tests the following:
  - min() returns the supplied lower bound (including negatives)
*/
bool
tests_test_counter_min()
{
    tc c(5, -10, 100);

    D_TC_CHECK(c.min() == -10);

    return true;
}


/*
tests_test_counter_max
  Verifies max() returns the upper bound.
  Tests the following:
  - max() returns the supplied upper bound
*/
bool
tests_test_counter_max()
{
    tc c(5, 0, 250);

    D_TC_CHECK(c.max() == 250);

    return true;
}


/*
tests_test_counter_at_min
  Verifies at_min() reflects whether the value is at or below the lower bound.
  Tests the following:
  - at_min() is false above the bound and true once the value reaches it
*/
bool
tests_test_counter_at_min()
{
    tc c(5, 0, 100);

    D_TC_CHECK(c.at_min() == false);

    c.decrement(5);

    D_TC_CHECK(c.at_min() == true);

    return true;
}


/*
tests_test_counter_at_max
  Verifies at_max() reflects whether the value is at or above the upper bound.
  Tests the following:
  - at_max() is false below the bound and true once the value reaches it
*/
bool
tests_test_counter_at_max()
{
    tc c(95, 0, 100);

    D_TC_CHECK(c.at_max() == false);

    c.increment(5);

    D_TC_CHECK(c.at_max() == true);

    return true;
}


NS_END  // testing
NS_END  // djinterp
