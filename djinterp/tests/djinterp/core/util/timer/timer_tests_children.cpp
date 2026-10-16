/*******************************************************************************
* djinterp [core]                                       timer_tests_children.cpp
*
*
* path:      /tests/djinterp/core/util/timer/timer_tests_children.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "timer_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_timer_add_child
  Verifies add_child() with no maximum.
  Tests the following:
  - the child count increments
  - the returned reference is usable and backed by the parent
  - the new child carries no maximum
*/
bool
tests_timer_add_child()
{
    reset_clock();

    ms_timer parent;

    D_TM_CHECK(parent.child_count() == 0);

    ms_timer& c = parent.add_child();

    D_TM_CHECK(parent.child_count() == 1);
    D_TM_CHECK(c.has_max()          == false);

    // the returned reference drives the same child the parent owns.
    c.start();
    test_clock::advance(msec(10));
    c.stop();

    D_TM_CHECK(parent.child(0).elapsed() == msec(10));

    return true;
}


/*
tests_timer_add_child_with_max
  Verifies add_child(max) sets the child's maximum.
  Tests the following:
  - the returned child reports has_max() true and the supplied maximum
  - the parent's stored child reflects the same maximum
*/
bool
tests_timer_add_child_with_max()
{
    reset_clock();

    ms_timer parent;

    ms_timer& c = parent.add_child(msec(50));

    D_TM_CHECK(c.has_max()           == true);
    D_TM_CHECK(c.max()               == msec(50));
    D_TM_CHECK(parent.child(0).max() == msec(50));

    return true;
}


/*
tests_timer_multiple_children
  Verifies several children coexist with distinct state.
  Tests the following:
  - the child count reflects all additions
  - each child retains its own maximum
*/
bool
tests_timer_multiple_children()
{
    reset_clock();

    ms_timer parent;

    parent.add_child(msec(10));
    parent.add_child(msec(20));
    parent.add_child(msec(30));

    D_TM_CHECK(parent.child_count() == 3);
    D_TM_CHECK(parent.child(0).max() == msec(10));
    D_TM_CHECK(parent.child(1).max() == msec(20));
    D_TM_CHECK(parent.child(2).max() == msec(30));

    return true;
}


/*
tests_timer_child_access
  Verifies the non-const child() accessor returns independent children.
  Tests the following:
  - driving child 0 leaves child 1 unaffected
*/
bool
tests_timer_child_access()
{
    reset_clock();

    ms_timer parent;

    parent.add_child();
    parent.add_child();

    // drive child 0 only.
    parent.child(0).start();
    test_clock::advance(msec(10));
    parent.child(0).stop();

    D_TM_CHECK(parent.child(0).elapsed() == msec(10));
    D_TM_CHECK(parent.child(1).elapsed() == msec(0));

    return true;
}


/*
tests_timer_child_access_const
  Verifies the const child() overload via a const reference.
  Tests the following:
  - the const overload returns a const child whose maximum reads correctly
  - child_count() reads correctly in a const context
*/
bool
tests_timer_child_access_const()
{
    reset_clock();

    ms_timer parent;

    parent.add_child(msec(99));

    const ms_timer& cp = parent;

    // the const child() overload returns a const reference.
    D_TM_CHECK(cp.child(0).max() == msec(99));
    D_TM_CHECK(cp.child_count()  == 1);

    return true;
}


/*
tests_timer_child_count
  Verifies child_count() tracks additions.
  Tests the following:
  - the count starts at zero and increments with each add_child()
*/
bool
tests_timer_child_count()
{
    reset_clock();

    ms_timer parent;

    D_TM_CHECK(parent.child_count() == 0);

    parent.add_child();

    D_TM_CHECK(parent.child_count() == 1);

    parent.add_child();

    D_TM_CHECK(parent.child_count() == 2);

    return true;
}


/*
tests_timer_children_independent
  Verifies children measure their own intervals independently.
  Tests the following:
  - two children driven over different intervals report different elapsed
*/
bool
tests_timer_children_independent()
{
    reset_clock();

    ms_timer parent;

    parent.add_child();
    parent.add_child();

    // run the two children over different intervals.
    parent.child(0).start();
    test_clock::advance(msec(10));
    parent.child(0).stop();

    parent.child(1).start();
    test_clock::advance(msec(25));
    parent.child(1).stop();

    D_TM_CHECK(parent.child(0).elapsed() == msec(10));
    D_TM_CHECK(parent.child(1).elapsed() == msec(25));

    return true;
}


/*
tests_timer_copy_deep_copies_children
  Verifies the owning contract: children are held by value, so a copied
  timer owns an independent copy of the subtree.
  Tests the following:
  - a copy reproduces the child's accumulated time
  - mutating the copy's child does not affect the original's child
*/
bool
tests_timer_copy_deep_copies_children()
{
    reset_clock();

    ms_timer parent;

    parent.add_child();

    parent.child(0).start();
    test_clock::advance(msec(10));
    parent.child(0).stop();

    // copy-construct: children held by value are deep-copied.
    ms_timer copy = parent;

    D_TM_CHECK(copy.child_count()      == 1);
    D_TM_CHECK(copy.child(0).elapsed() == msec(10));

    // mutating the copy's child must not affect the original.
    copy.child(0).reset();

    D_TM_CHECK(copy.child(0).elapsed()   == msec(0));
    D_TM_CHECK(parent.child(0).elapsed() == msec(10));

    return true;
}


NS_END  // testing
NS_END  // djinterp
