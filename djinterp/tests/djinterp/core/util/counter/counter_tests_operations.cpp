/*******************************************************************************
* djinterp [core]                                   counter_tests_operations.cpp
*
*
* path:      /tests/djinterp/core/util/counter/counter_tests_operations.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_counter_increment
  Verifies a plain increment by the default amount.
  Tests the following:
  - increment() with no argument advances the value by one
  - it returns true because the upper bound was not reached
*/
bool
tests_counter_increment()
{
    ctr c(0, 0, 10);

    const bool ok = c.increment();   // default amount of one

    D_CTR_CHECK(ok        == true);
    D_CTR_CHECK(c.value() == 1);

    return true;
}


/*
tests_counter_increment_amount
  Verifies an increment by an explicit amount.
  Tests the following:
  - increment(n) advances the value by exactly n
  - it returns true when the result stays within bounds
*/
bool
tests_counter_increment_amount()
{
    ctr c(0, 0, 10);

    const bool ok = c.increment(5);

    D_CTR_CHECK(ok        == true);
    D_CTR_CHECK(c.value() == 5);

    return true;
}


/*
tests_counter_increment_to_max
  Verifies the boundary where an increment lands exactly on the maximum.
  Tests the following:
  - reaching the maximum exactly is the success path (not a clamp)
  - increment() returns true and the value equals the maximum
  - at_max() becomes true
*/
bool
tests_counter_increment_to_max()
{
    ctr c(8, 0, 10);

    // 8 + 2 == 10 does not exceed the maximum, so this is the normal path.
    const bool ok = c.increment(2);

    D_CTR_CHECK(ok         == true);
    D_CTR_CHECK(c.value()  == 10);
    D_CTR_CHECK(c.at_max() == true);

    return true;
}


/*
tests_counter_increment_past_max
  Verifies the clamping path when an increment would exceed the maximum.
  Tests the following:
  - increment() returns false and clamps the value to the maximum
  - a further increment from the clamped maximum also fails and stays put
*/
bool
tests_counter_increment_past_max()
{
    ctr c(8, 0, 10);

    // 8 + 5 would exceed the maximum: increment() clamps to max and fails.
    const bool ok = c.increment(5);

    D_CTR_CHECK(ok        == false);
    D_CTR_CHECK(c.value() == 10);

    const bool again = c.increment(1);

    D_CTR_CHECK(again     == false);
    D_CTR_CHECK(c.value() == 10);

    return true;
}


/*
tests_counter_decrement
  Verifies a plain decrement by the default amount.
  Tests the following:
  - decrement() with no argument lowers the value by one
  - it returns true because the lower bound was not reached
*/
bool
tests_counter_decrement()
{
    ctr c(5, 0, 10);

    const bool ok = c.decrement();   // default amount of one

    D_CTR_CHECK(ok        == true);
    D_CTR_CHECK(c.value() == 4);

    return true;
}


/*
tests_counter_decrement_amount
  Verifies a decrement by an explicit amount.
  Tests the following:
  - decrement(n) lowers the value by exactly n
  - it returns true when the result stays within bounds
*/
bool
tests_counter_decrement_amount()
{
    ctr c(5, 0, 10);

    const bool ok = c.decrement(3);

    D_CTR_CHECK(ok        == true);
    D_CTR_CHECK(c.value() == 2);

    return true;
}


/*
tests_counter_decrement_to_min
  Verifies the boundary where a decrement lands exactly on the minimum.
  Tests the following:
  - reaching the minimum exactly is the success path (not a clamp)
  - decrement() returns true and the value equals the minimum
  - at_min() becomes true
*/
bool
tests_counter_decrement_to_min()
{
    ctr c(2, 0, 10);

    // 2 - 2 == 0 does not fall below the minimum, so this is the normal path.
    const bool ok = c.decrement(2);

    D_CTR_CHECK(ok         == true);
    D_CTR_CHECK(c.value()  == 0);
    D_CTR_CHECK(c.at_min() == true);

    return true;
}


/*
tests_counter_decrement_past_min
  Verifies the clamping path when a decrement would exceed the minimum.
  Tests the following:
  - decrement() returns false and clamps the value to the minimum
  - a further decrement from the clamped minimum also fails and stays put
*/
bool
tests_counter_decrement_past_min()
{
    ctr c(2, 0, 10);

    // 2 - 5 would fall below the minimum: decrement() clamps to min and fails.
    const bool ok = c.decrement(5);

    D_CTR_CHECK(ok        == false);
    D_CTR_CHECK(c.value() == 0);

    const bool again = c.decrement(1);

    D_CTR_CHECK(again     == false);
    D_CTR_CHECK(c.value() == 0);

    return true;
}


/*
tests_counter_increment_decrement_sequence
  Verifies that a mixed run of increments and decrements composes correctly.
  Tests the following:
  - a sequence of in-bounds operations accumulates to the expected value
*/
bool
tests_counter_increment_decrement_sequence()
{
    ctr c(0, 0, 100);

    c.increment(10);
    c.increment(5);
    c.decrement(3);
    c.increment(8);
    c.decrement(20);

    // 0 + 10 + 5 - 3 + 8 - 20 == 0
    D_CTR_CHECK(c.value() == 0);

    return true;
}


/*
tests_counter_reset
  Verifies that reset() returns the value to its zero initial.
  Tests the following:
  - after an increment the value reflects the change
  - reset() restores the construction initial value
*/
bool
tests_counter_reset()
{
    ctr c(0, 0, 10);

    c.increment(5);

    D_CTR_CHECK(c.value() == 5);

    c.reset();

    D_CTR_CHECK(c.value() == 0);

    return true;
}


/*
tests_counter_reset_nonzero_initial
  Verifies that reset() restores a non-zero initial rather than zero.
  Tests the following:
  - reset() returns the value to the initial supplied at construction
  - initial() continues to report that same construction value
*/
bool
tests_counter_reset_nonzero_initial()
{
    ctr c(5, 0, 10);

    c.increment(3);

    D_CTR_CHECK(c.value() == 8);

    c.reset();

    D_CTR_CHECK(c.value()   == 5);
    D_CTR_CHECK(c.initial() == 5);

    return true;
}


/*
tests_counter_reset_preserves_children
  Verifies that reset() touches only this counter, not its owned children.
  Tests the following:
  - reset() restores the parent's value
  - an owned child's value is left unchanged by the parent's reset()
*/
bool
tests_counter_reset_preserves_children()
{
    ctr parent(0, 0, 100);

    parent.add_child(10, 0, 100);

    parent.increment(5);
    parent.child(0).increment(7);

    D_CTR_CHECK(parent.value()          == 5);
    D_CTR_CHECK(parent.child(0).value() == 17);

    parent.reset();

    D_CTR_CHECK(parent.value()          == 0);
    D_CTR_CHECK(parent.child(0).value() == 17);

    return true;
}


/*
tests_counter_reset_all_recurses
  Verifies that reset_all() restores this counter and every owned descendant.
  Tests the following:
  - a child and a grandchild are each advanced, then all restored at once
  - reset_all() recurses through the full owned subtree
*/
bool
tests_counter_reset_all_recurses()
{
    ctr root(1, 0, 100);

    root.add_child(2, 0, 100);
    root.child(0).add_child(3, 0, 100);   // a grandchild

    root.increment(10);
    root.child(0).increment(10);
    root.child(0).child(0).increment(10);

    D_CTR_CHECK(root.value()                   == 11);
    D_CTR_CHECK(root.child(0).value()          == 12);
    D_CTR_CHECK(root.child(0).child(0).value() == 13);

    root.reset_all();

    D_CTR_CHECK(root.value()                   == 1);
    D_CTR_CHECK(root.child(0).value()          == 2);
    D_CTR_CHECK(root.child(0).child(0).value() == 3);

    return true;
}


/*
tests_counter_reset_all_leaf
  Verifies reset_all() on a childless counter (the empty-recursion case).
  Tests the following:
  - reset_all() restores the value with no children to iterate over
  - the counter still owns no children afterward
*/
bool
tests_counter_reset_all_leaf()
{
    ctr c(4, 0, 100);

    c.increment(20);

    D_CTR_CHECK(c.value() == 24);

    c.reset_all();

    D_CTR_CHECK(c.value()       == 4);
    D_CTR_CHECK(c.child_count() == 0);

    return true;
}


/*
tests_counter_reset_all_preserves_observed
  Verifies that reset_all() never resets a non-owning observed counter.
  Tests the following:
  - reset_all() restores the parent's value
  - an observed (external) counter keeps its own value across the parent's
    reset_all()
*/
bool
tests_counter_reset_all_preserves_observed()
{
    ctr parent(0, 0, 100);
    ctr external(50, 0, 100);

    parent.observe(external);

    parent.increment(5);
    external.increment(10);

    D_CTR_CHECK(parent.value()   == 5);
    D_CTR_CHECK(external.value() == 60);

    parent.reset_all();

    D_CTR_CHECK(parent.value()   == 0);
    D_CTR_CHECK(external.value() == 60);

    return true;
}


NS_END  // testing
NS_END  // djinterp
