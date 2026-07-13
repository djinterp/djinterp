/******************************************************************************
* djinterp [test]                                 atomic_tests_flag_guard.cpp
*
*   Section III - atomic_flag_guard.  The RAII guard test-and-sets a
* std::atomic_flag on construction and clears it on destruction, reporting via
* was_set() whether the flag was already held.  These cases pin down all three
* observable behaviours: was_set() false on a clear flag, was_set() true when
* the flag is already set, that construction actually sets the flag, and that
* destruction actually clears it.  A final case documents that the guard is a
* SCOPED test-and-set (it always clears on the way out), not a persistent
* one-shot latch.
*
*
* path:      /tests/djinterp/core/sync/atomic_tests_flag_guard.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.13
******************************************************************************/

// std
#include <atomic>
#include <cstdio>

// djinterp
#include "atomic_tests.hpp"


#define D_AT_CHECK(_cond)                                                      \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            std::printf("      [check] FAILED: %s\n"                           \
                        "               at %s:%d\n",                           \
                        #_cond, __FILE__, __LINE__);                           \
            return false;                                                      \
        }                                                                      \
    }                                                                          \
    while (false)


NS_DJINTERP
NS_TESTING


bool
tests_flag_guard_was_set_false_on_clear_flag()
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;   // guaranteed clear

    atomic_flag_guard g(flag);
    D_AT_CHECK(!g.was_set());   // the flag was clear before this guard

    return true;
}


bool
tests_flag_guard_sets_flag_on_construct()
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

    atomic_flag_guard outer(flag);      // sets the flag
    D_AT_CHECK(!outer.was_set());

    {
        atomic_flag_guard inner(flag);  // the flag is already set by 'outer'
        D_AT_CHECK(inner.was_set());    // -> proves construction sets it
    }
    // (inner's destructor has now cleared the flag; that clear-on-destruct
    //  behaviour is asserted directly in the next test.)

    return true;
}


bool
tests_flag_guard_clears_on_destruct()
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

    {
        atomic_flag_guard g(flag);   // sets
        D_AT_CHECK(!g.was_set());
    }   // g destructs -> clears

    // a fresh guard now sees a CLEAR flag -> proves the destructor cleared it
    atomic_flag_guard again(flag);
    D_AT_CHECK(!again.was_set());

    return true;
}


bool
tests_flag_guard_was_set_true_when_already_held()
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

    // hold the flag directly, ahead of the guard
    (void)flag.test_and_set(std::memory_order_acquire);

    atomic_flag_guard g(flag);
    D_AT_CHECK(g.was_set());   // reports the contention

    return true;
}


bool
tests_flag_guard_one_shot_init_pattern()
{
    std::atomic_flag inited = ATOMIC_FLAG_INIT;
    int              init_count = 0;

    // first scoped pass: we are the initializer (flag was clear)
    {
        atomic_flag_guard g(inited);
        if (!g.was_set())
        {
            ++init_count;   // perform one-time initialization
        }
    }

    // Because the guard clears on destruct, the second scoped pass again finds
    // the flag clear.  This documents that atomic_flag_guard models a SCOPED
    // test-and-set, NOT a persistent latch - a persistent latch would not
    // clear, and init_count would stay 1.
    {
        atomic_flag_guard g(inited);
        if (!g.was_set())
        {
            ++init_count;
        }
    }

    D_AT_CHECK(init_count == 2);

    return true;
}


NS_END  // testing
NS_END  // djinterp

#undef D_AT_CHECK
