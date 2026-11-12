/*******************************************************************************
* djinterp [cpp][test]                         test_counter_tests_operations.cpp
*
* The test_counter suite, block "operations": the generic counter's increment,
* decrement, reset and reset_all, as the test counter's own.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_counter_tests_operations.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.10
*******************************************************************************/
// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_counter_increment_within_bounds
  Verifies a plain increment moves the value and succeeds.
  Tests the following:
  - an increment inside the bounds returns true and adds its amount
  - a second increment adds again
*/
bool tests_test_counter_increment_within_bounds()
{
    tc c(0, 0, 100);

    D_TC_CHECK(c.increment(5));
    D_TC_CHECK(c.value() == 5);
    D_TC_CHECK(c.increment());
    D_TC_CHECK(c.value() == 6);

    return true;
}


/*
tests_test_counter_increment_overshoot_clamps
  Verifies an overshooting increment clamps to max and reports it.
  Tests the following:
  - the value stops at max and the increment returns false
  - landing exactly on max is a success
*/
bool tests_test_counter_increment_overshoot_clamps()
{
    tc c(50, 0, 100);

    D_TC_CHECK(!c.increment(80));
    D_TC_CHECK(c.value() == 100);
    D_TC_CHECK(c.at_max());

    tc exact(90, 0, 100);

    D_TC_CHECK(exact.increment(10));
    D_TC_CHECK(exact.value() == 100);

    return true;
}


/*
tests_test_counter_decrement_within_bounds
  Verifies a plain decrement moves the value and succeeds.
  Tests the following:
  - a decrement inside the bounds returns true and subtracts its amount
*/
bool tests_test_counter_decrement_within_bounds()
{
    tc c(50, 0, 100);

    D_TC_CHECK(c.decrement(20));
    D_TC_CHECK(c.value() == 30);

    return true;
}


/*
tests_test_counter_decrement_undershoot_clamps
  Verifies an undershooting decrement clamps to min and reports it.
  Tests the following:
  - the value stops at min and the decrement returns false
*/
bool tests_test_counter_decrement_undershoot_clamps()
{
    tc c(50, 0, 100);

    D_TC_CHECK(!c.decrement(80));
    D_TC_CHECK(c.value() == 0);
    D_TC_CHECK(c.at_min());

    return true;
}


/*
tests_test_counter_reset
  Verifies reset() restores the initial value.
  Tests the following:
  - the value returns to its initial after moving
*/
bool tests_test_counter_reset()
{
    tc c(10, 0, 100);

    (void)c.increment(20);
    c.reset();

    D_TC_CHECK(c.value() == 10);

    return true;
}


/*
tests_test_counter_reset_all_recurses
  Verifies reset_all() resets this counter and every owned descendant.
  Tests the following:
  - the root and a grandchild both return to their initial values
*/
bool tests_test_counter_reset_all_recurses()
{
    tc c(0, 0, 100);

    (void)c.add_child();
    (void)c.add_child().add_child();
    (void)c.increment(7);
    (void)c.child(1).child(0).increment(3);

    c.reset_all();

    D_TC_CHECK(c.value() == 0);
    D_TC_CHECK(c.child(1).child(0).value() == 0);

    return true;
}


NS_END  // testing
NS_END  // djinterp
