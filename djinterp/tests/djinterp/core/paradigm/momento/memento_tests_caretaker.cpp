/*******************************************************************************
* djinterp [core]                                    memento_tests_caretaker.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_caretaker.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_caretaker.cpp
//   The memento_caretaker (section V): the history stack that composes a
// snapshot type with a history eviction policy. Covers push/size, pop, peek,
// indexed at(), empty/clear, the two for_each traversals, the optional try_pop,
// and the interaction with the coalescing and bounded policies.

// std
#include <cstddef>
#include <vector>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_caretaker_push_size
  Verifies push grows the history under the default (unlimited) policy.
  Tests the following:
  - size reflects the number pushed
  - a fresh caretaker is empty
*/
bool
tests_caretaker_push_size()
{
    memento_caretaker<int> ct;

    bool ok = (ct.empty() && ct.size() == 0);

    ct.push(memento<int>(1));
    ct.push(memento<int>(2));
    ct.push(memento<int>(3));

    ok = ok && (ct.size() == 3);
    ok = ok && (!ct.empty());

    return ok;
}

/*
tests_caretaker_pop
  Verifies pop returns and removes the most recent memento.
  Tests the following:
  - pop yields the newest entry
  - the history shrinks by one
*/
bool
tests_caretaker_pop()
{
    memento_caretaker<int> ct;
    ct.push(memento<int>(1));
    ct.push(memento<int>(2));

    memento<int> m = ct.pop();

    bool ok = true;

    ok = ok && (m.state == 2);
    ok = ok && (ct.size() == 1);

    return ok;
}

/*
tests_caretaker_peek
  Verifies peek returns the newest memento without removing it.
  Tests the following:
  - peek yields the newest entry
  - the history is unchanged
*/
bool
tests_caretaker_peek()
{
    memento_caretaker<int> ct;
    ct.push(memento<int>(5));
    ct.push(memento<int>(9));

    bool ok = true;

    ok = ok && (ct.peek().state == 9);
    ok = ok && (ct.size() == 2);

    return ok;
}

/*
tests_caretaker_at
  Verifies indexed access with 0 = oldest.
  Tests the following:
  - at(0) is the oldest, at(size-1) the newest
*/
bool
tests_caretaker_at()
{
    memento_caretaker<int> ct;
    ct.push(memento<int>(10));
    ct.push(memento<int>(20));
    ct.push(memento<int>(30));

    bool ok = true;

    ok = ok && (ct.at(0).state == 10);
    ok = ok && (ct.at(1).state == 20);
    ok = ok && (ct.at(2).state == 30);

    return ok;
}

/*
tests_caretaker_empty_clear
  Verifies clear discards all mementos.
  Tests the following:
  - clear empties a populated history
*/
bool
tests_caretaker_empty_clear()
{
    memento_caretaker<int> ct;
    ct.push(memento<int>(1));
    ct.push(memento<int>(2));

    ct.clear();

    return (ct.empty() && ct.size() == 0);
}

/*
tests_caretaker_for_each
  Verifies for_each iterates oldest to newest.
  Tests the following:
  - the callback visits every entry
  - the first-visited entry is the oldest
*/
bool
tests_caretaker_for_each()
{
    memento_caretaker<int> ct;
    ct.push(memento<int>(1));
    ct.push(memento<int>(2));
    ct.push(memento<int>(3));

    int sum   = 0;
    int first = -1;
    ct.for_each([&](const memento<int>& _m)
    {
        if (first < 0) { first = _m.state; }
        sum += _m.state;
    });

    bool ok = true;

    ok = ok && (sum == 6);
    ok = ok && (first == 1);   // oldest first

    return ok;
}

/*
tests_caretaker_for_each_reverse
  Verifies for_each_reverse iterates newest to oldest.
  Tests the following:
  - the callback visits every entry
  - the first-visited entry is the newest
*/
bool
tests_caretaker_for_each_reverse()
{
    memento_caretaker<int> ct;
    ct.push(memento<int>(1));
    ct.push(memento<int>(2));
    ct.push(memento<int>(3));

    int count = 0;
    int first = -1;
    ct.for_each_reverse([&](const memento<int>& _m)
    {
        if (first < 0) { first = _m.state; }
        ++count;
    });

    bool ok = true;

    ok = ok && (count == 3);
    ok = ok && (first == 3);   // newest first

    return ok;
}

/*
tests_caretaker_try_pop
  Verifies the optional try_pop.
  Tests the following:
  - try_pop yields the newest entry when non-empty
  - try_pop yields nullopt when empty (C++17+)
*/
bool
tests_caretaker_try_pop()
{
#if D_MEMENTO_HAS_OPTIONAL
    memento_caretaker<int> ct;
    ct.push(memento<int>(8));

    std::optional<memento<int> > got = ct.try_pop();

    bool ok = true;

    ok = ok && (got.has_value() && got->state == 8);

    std::optional<memento<int> > none = ct.try_pop();
    ok = ok && (!none.has_value());

    return ok;
#else
    return true;
#endif
}

/*
tests_caretaker_coalescing
  Verifies the caretaker honours the coalescing policy on push.
  Tests the following:
  - a duplicate of the newest snapshot is suppressed
  - distinct snapshots are retained
*/
bool
tests_caretaker_coalescing()
{
    memento_caretaker<int, coalescing_history> ct;

    ct.push(memento<int>(7));
    ct.push(memento<int>(7));   // suppressed
    ct.push(memento<int>(8));
    ct.push(memento<int>(8));   // suppressed

    bool ok = true;

    ok = ok && (ct.size() == 2);
    ok = ok && (ct.at(0).state == 7);
    ok = ok && (ct.at(1).state == 8);

    return ok;
}

/*
tests_caretaker_bounded
  Verifies the caretaker honours the bounded policy on push.
  Tests the following:
  - the history never exceeds the cap
  - the oldest entries are evicted
*/
bool
tests_caretaker_bounded()
{
    memento_caretaker<int, bounded_history<3> > ct;

    for (int i = 1; i <= 6; ++i)
    {
        ct.push(memento<int>(i));
    }

    bool ok = true;

    ok = ok && (ct.size() == 3);
    ok = ok && (ct.at(0).state == 4);   // 1..3 evicted
    ok = ok && (ct.peek().state == 6);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
