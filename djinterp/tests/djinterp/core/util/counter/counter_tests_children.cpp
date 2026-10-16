/*******************************************************************************
* djinterp [core]                                     counter_tests_children.cpp
*
*
* path:      /tests/djinterp/core/util/counter/counter_tests_children.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_counter_add_child
  Verifies add_child() with default arguments.
  Tests the following:
  - the child count increments
  - the returned reference is usable and backed by the parent
  - the new child carries a zero value and the widest default bounds
*/
bool
tests_counter_add_child()
{
    ctr parent;

    D_CTR_CHECK(parent.child_count() == 0);

    ctr& c = parent.add_child();

    D_CTR_CHECK(parent.child_count() == 1);
    D_CTR_CHECK(c.value()            == 0);
    D_CTR_CHECK(c.initial()          == 0);
    D_CTR_CHECK(c.min()              == k_lowest);
    D_CTR_CHECK(c.max()              == k_highest);

    // the returned reference drives the same child the parent owns.
    c.increment(3);

    D_CTR_CHECK(parent.child(0).value() == 3);

    return true;
}


/*
tests_counter_add_child_with_bounds
  Verifies add_child(initial, min, max) sets the child's fields.
  Tests the following:
  - the returned child reports the supplied initial value and bounds
  - the parent's stored child reflects the same values
*/
bool
tests_counter_add_child_with_bounds()
{
    ctr parent;

    ctr& c = parent.add_child(5, 0, 10);

    D_CTR_CHECK(c.value()              == 5);
    D_CTR_CHECK(c.initial()            == 5);
    D_CTR_CHECK(c.min()                == 0);
    D_CTR_CHECK(c.max()                == 10);
    D_CTR_CHECK(parent.child(0).max()  == 10);

    return true;
}


/*
tests_counter_multiple_children
  Verifies several children coexist with distinct state.
  Tests the following:
  - the child count reflects all additions
  - each child retains its own value
*/
bool
tests_counter_multiple_children()
{
    ctr parent;

    parent.add_child(1, 0, 10);
    parent.add_child(2, 0, 10);
    parent.add_child(3, 0, 10);

    D_CTR_CHECK(parent.child_count()    == 3);
    D_CTR_CHECK(parent.child(0).value() == 1);
    D_CTR_CHECK(parent.child(1).value() == 2);
    D_CTR_CHECK(parent.child(2).value() == 3);

    return true;
}


/*
tests_counter_child_access
  Verifies the non-const child() accessor returns a mutable reference.
  Tests the following:
  - a change made through child() persists in the parent's stored child
*/
bool
tests_counter_child_access()
{
    ctr parent;

    parent.add_child(0, 0, 10);

    // mutate through the non-const child() reference.
    parent.child(0).increment(4);

    D_CTR_CHECK(parent.child(0).value() == 4);

    return true;
}


/*
tests_counter_child_access_const
  Verifies the const child() overload via a const reference.
  Tests the following:
  - the const overload returns a const child whose value reads correctly
  - child_count() reads correctly in a const context
*/
bool
tests_counter_child_access_const()
{
    ctr parent;

    parent.add_child(9, 0, 10);

    const ctr& cp = parent;

    D_CTR_CHECK(cp.child(0).value()   == 9);
    D_CTR_CHECK(cp.child(0).initial() == 9);
    D_CTR_CHECK(cp.child_count()      == 1);

    return true;
}


/*
tests_counter_child_count
  Verifies child_count() tracks additions.
  Tests the following:
  - the count starts at zero and increments with each add_child()
*/
bool
tests_counter_child_count()
{
    ctr parent;

    D_CTR_CHECK(parent.child_count() == 0);

    parent.add_child();

    D_CTR_CHECK(parent.child_count() == 1);

    parent.add_child();

    D_CTR_CHECK(parent.child_count() == 2);

    return true;
}


/*
tests_counter_children_independent
  Verifies children hold independent state.
  Tests the following:
  - incrementing one child does not disturb its sibling
*/
bool
tests_counter_children_independent()
{
    ctr parent;

    parent.add_child(0, 0, 100);
    parent.add_child(0, 0, 100);

    // change child 0 only.
    parent.child(0).increment(10);

    D_CTR_CHECK(parent.child(0).value() == 10);
    D_CTR_CHECK(parent.child(1).value() == 0);

    return true;
}


/*
tests_counter_copy_deep_copies_children
  Verifies the owning contract: children are held by value, so a copied
  counter owns an independent copy of the subtree.
  Tests the following:
  - a copy reproduces the child's value
  - mutating the copy's child does not affect the original's child
*/
bool
tests_counter_copy_deep_copies_children()
{
    ctr original(0, 0, 100);

    original.add_child(5, 0, 100);

    // copy-construct: children held by value are deep-copied.
    ctr copy = original;

    D_CTR_CHECK(copy.child_count()    == 1);
    D_CTR_CHECK(copy.child(0).value() == 5);

    // mutating the copy's child must not affect the original.
    copy.child(0).increment(20);

    D_CTR_CHECK(copy.child(0).value()     == 25);
    D_CTR_CHECK(original.child(0).value()  == 5);

    return true;
}


NS_END  // testing
NS_END  // djinterp
