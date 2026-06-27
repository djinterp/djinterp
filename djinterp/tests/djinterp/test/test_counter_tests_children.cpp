// djinterp
#include "test_counter_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_counter_add_child
  Verifies add_child() with defaults, including dispatcher inheritance.
  Tests the following:
  - the child count increments
  - the new child takes the default initial value
  - the child inherits the parent's event dispatcher
*/
bool
tests_test_counter_add_child()
{
    event_dispatcher eh;
    tc parent(0, 0, 100, &eh);

    D_TC_CHECK(parent.child_count() == 0);

    tc& c = parent.add_child();

    D_TC_CHECK(parent.child_count() == 1);
    D_TC_CHECK(c.value()            == 0);
    D_TC_CHECK(c.handler()          == &eh);

    return true;
}


/*
tests_test_counter_add_child_with_bounds
  Verifies add_child(initial, min, max), including dispatcher inheritance.
  Tests the following:
  - the new child carries the supplied value and bounds
  - the child inherits the parent's event dispatcher
*/
bool
tests_test_counter_add_child_with_bounds()
{
    event_dispatcher eh;
    tc parent(0, 0, 100, &eh);

    tc& c = parent.add_child(5, 0, 50);

    D_TC_CHECK(c.value()   == 5);
    D_TC_CHECK(c.initial() == 5);
    D_TC_CHECK(c.min()     == 0);
    D_TC_CHECK(c.max()     == 50);
    D_TC_CHECK(c.handler() == &eh);

    return true;
}


/*
tests_test_counter_multiple_children
  Verifies several children coexist with their own configuration.
  Tests the following:
  - the child count reflects all additions
  - each child retains its own value and bounds
*/
bool
tests_test_counter_multiple_children()
{
    tc parent;

    parent.add_child(1, 0, 10);
    parent.add_child(2, 0, 20);
    parent.add_child(3, 0, 30);

    D_TC_CHECK(parent.child_count()    == 3);
    D_TC_CHECK(parent.child(0).value() == 1);
    D_TC_CHECK(parent.child(1).value() == 2);
    D_TC_CHECK(parent.child(1).max()   == 20);
    D_TC_CHECK(parent.child(2).value() == 3);

    return true;
}


/*
tests_test_counter_child_access
  Verifies the non-const child() accessor returns a mutable reference.
  Tests the following:
  - incrementing a child through child() persists in the stored child
*/
bool
tests_test_counter_child_access()
{
    tc parent;

    parent.add_child(0, 0, 100);

    parent.child(0).increment(15);

    D_TC_CHECK(parent.child(0).value() == 15);

    return true;
}


/*
tests_test_counter_child_access_const
  Verifies the const child() overload via a const reference.
  Tests the following:
  - the const overload returns a const child whose state reads correctly
  - child_count() reads correctly in a const context
*/
bool
tests_test_counter_child_access_const()
{
    tc parent;

    parent.add_child(7, 0, 50);

    const tc& cp = parent;

    D_TC_CHECK(cp.child(0).value() == 7);
    D_TC_CHECK(cp.child(0).max()   == 50);
    D_TC_CHECK(cp.child_count()    == 1);

    return true;
}


/*
tests_test_counter_child_count
  Verifies child_count() tracks additions.
  Tests the following:
  - the count starts at zero and increments with each add_child()
*/
bool
tests_test_counter_child_count()
{
    tc parent;

    D_TC_CHECK(parent.child_count() == 0);

    parent.add_child();

    D_TC_CHECK(parent.child_count() == 1);

    parent.add_child();

    D_TC_CHECK(parent.child_count() == 2);

    return true;
}


/*
tests_test_counter_children_independent
  Verifies children hold independent state.
  Tests the following:
  - mutating one child does not disturb its sibling
*/
bool
tests_test_counter_children_independent()
{
    tc parent;

    parent.add_child(0, 0, 100);
    parent.add_child(0, 0, 100);

    parent.child(0).increment(25);

    D_TC_CHECK(parent.child(0).value() == 25);
    D_TC_CHECK(parent.child(1).value() == 0);

    return true;
}


/*
tests_test_counter_copy_deep_copies_children
  Verifies the owning contract: children are held by value, so a copied counter
  owns an independent copy of the subtree.
  Tests the following:
  - a copy reproduces the child's value
  - mutating the copy's child does not affect the original's child
*/
bool
tests_test_counter_copy_deep_copies_children()
{
    tc original;

    original.add_child(0, 0, 100);
    original.child(0).increment(10);

    D_TC_CHECK(original.child(0).value() == 10);

    tc copy = original;

    D_TC_CHECK(copy.child_count()    == 1);
    D_TC_CHECK(copy.child(0).value() == 10);

    copy.child(0).increment(20);

    D_TC_CHECK(copy.child(0).value()     == 30);
    D_TC_CHECK(original.child(0).value() == 10);

    return true;
}


NS_END  // testing
NS_END  // djinterp
