// djinterp
#include "test_timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_test_timer_add_child
  Verifies add_child() with no maximum, including dispatcher inheritance.
  Tests the following:
  - the child count increments
  - the new child has no maximum limit
  - the child inherits the parent's event dispatcher
*/
bool
tests_test_timer_add_child()
{
    reset_clock();

    event_dispatcher eh;
    tt parent(&eh);

    D_TT_CHECK(parent.child_count() == 0);

    tt& c = parent.add_child();

    D_TT_CHECK(parent.child_count() == 1);
    D_TT_CHECK(c.has_max()          == false);
    D_TT_CHECK(c.handler()          == &eh);

    return true;
}


/*
tests_test_timer_add_child_with_max
  Verifies add_child(max), including dispatcher inheritance.
  Tests the following:
  - the new child carries the supplied maximum limit
  - the child inherits the parent's event dispatcher
*/
bool
tests_test_timer_add_child_with_max()
{
    reset_clock();

    event_dispatcher eh;
    tt parent(&eh);

    tt& c = parent.add_child(msec(100));

    D_TT_CHECK(c.has_max() == true);
    D_TT_CHECK(c.max()     == msec(100));
    D_TT_CHECK(c.handler() == &eh);

    return true;
}


/*
tests_test_timer_multiple_children
  Verifies several children coexist with their own configuration.
  Tests the following:
  - the child count reflects all additions
  - each child retains its own max / no-max setting
*/
bool
tests_test_timer_multiple_children()
{
    reset_clock();

    tt parent;

    parent.add_child();
    parent.add_child(msec(50));
    parent.add_child();

    D_TT_CHECK(parent.child_count()      == 3);
    D_TT_CHECK(parent.child(0).has_max() == false);
    D_TT_CHECK(parent.child(1).has_max() == true);
    D_TT_CHECK(parent.child(1).max()     == msec(50));
    D_TT_CHECK(parent.child(2).has_max() == false);

    return true;
}


/*
tests_test_timer_child_access
  Verifies the non-const child() accessor returns a mutable reference.
  Tests the following:
  - timing a child through child() persists in the parent's stored child
*/
bool
tests_test_timer_child_access()
{
    reset_clock();

    tt parent;

    parent.add_child();

    parent.child(0).start();
    test_clock::advance(msec(15));
    parent.child(0).stop();

    D_TT_CHECK(parent.child(0).elapsed() == msec(15));

    return true;
}


/*
tests_test_timer_child_access_const
  Verifies the const child() overload via a const reference.
  Tests the following:
  - the const overload returns a const child whose state reads correctly
  - child_count() reads correctly in a const context
*/
bool
tests_test_timer_child_access_const()
{
    reset_clock();

    event_dispatcher eh;
    tt parent(&eh);

    parent.add_child(msec(80));

    const tt& cp = parent;

    D_TT_CHECK(cp.child(0).has_max() == true);
    D_TT_CHECK(cp.child(0).max()     == msec(80));
    D_TT_CHECK(cp.child_count()      == 1);

    return true;
}


/*
tests_test_timer_child_count
  Verifies child_count() tracks additions.
  Tests the following:
  - the count starts at zero and increments with each add_child()
*/
bool
tests_test_timer_child_count()
{
    reset_clock();

    tt parent;

    D_TT_CHECK(parent.child_count() == 0);

    parent.add_child();

    D_TT_CHECK(parent.child_count() == 1);

    parent.add_child();

    D_TT_CHECK(parent.child_count() == 2);

    return true;
}


/*
tests_test_timer_children_independent
  Verifies children hold independent state.
  Tests the following:
  - timing one child does not disturb its sibling
*/
bool
tests_test_timer_children_independent()
{
    reset_clock();

    tt parent;

    parent.add_child();
    parent.add_child();

    parent.child(0).start();
    test_clock::advance(msec(25));
    parent.child(0).stop();

    D_TT_CHECK(parent.child(0).elapsed() == msec(25));
    D_TT_CHECK(parent.child(1).elapsed() == msec(0));

    return true;
}


/*
tests_test_timer_copy_deep_copies_children
  Verifies the owning contract: children are held by value, so a copied timer
  owns an independent copy of the subtree.
  Tests the following:
  - a copy reproduces the child's accumulated time
  - timing the copy's child does not affect the original's child
*/
bool
tests_test_timer_copy_deep_copies_children()
{
    reset_clock();

    tt original;

    original.add_child();
    original.child(0).start();
    test_clock::advance(msec(10));
    original.child(0).stop();

    D_TT_CHECK(original.child(0).elapsed() == msec(10));

    tt copy = original;

    D_TT_CHECK(copy.child_count()      == 1);
    D_TT_CHECK(copy.child(0).elapsed() == msec(10));

    copy.child(0).start();
    test_clock::advance(msec(20));
    copy.child(0).stop();

    D_TT_CHECK(copy.child(0).elapsed()     == msec(30));
    D_TT_CHECK(original.child(0).elapsed() == msec(10));

    return true;
}


NS_END  // testing
NS_END  // djinterp
