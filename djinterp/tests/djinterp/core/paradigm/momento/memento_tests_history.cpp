/*******************************************************************************
* djinterp [core]                                      memento_tests_history.cpp
*
*
* path:      /tests/djinterp/core/paradigm/momento/memento_tests_history.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : memento_tests_history.cpp
//   The history eviction policies (section IV): unlimited_history,
// bounded_history (with its compile-time cap), and coalescing_history. Each is
// tested at the policy level - should_push / after_push over a plain vector of
// mementos - independent of the caretaker that composes them.

// std
#include <cstddef>
#include <vector>
// djinterp
#include "memento_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // small alias for a history container of int-snapshot mementos.
    typedef std::vector<memento<int> > int_history;
}


/*
tests_unlimited_history
  Verifies unlimited_history retains everything.
  Tests the following:
  - should_push always returns true
  - after_push leaves the container untouched
*/
bool
tests_unlimited_history()
{
    int_history h;
    h.push_back(memento<int>(1));
    h.push_back(memento<int>(2));

    bool ok = true;

    ok = ok && (unlimited_history::should_push(h, 3));
    unlimited_history::after_push(h);
    ok = ok && (h.size() == 2);

    return ok;
}

/*
tests_bounded_history_evicts
  Verifies bounded_history evicts oldest entries past the cap.
  Tests the following:
  - after_push trims the container down to the cap, dropping the oldest
*/
bool
tests_bounded_history_evicts()
{
    int_history h;
    for (int i = 1; i <= 5; ++i)
    {
        h.push_back(memento<int>(i));
    }

    bounded_history<3>::after_push(h);

    bool ok = true;

    ok = ok && (h.size() == 3);
    ok = ok && (h.front().state == 3);   // 1 and 2 evicted
    ok = ok && (h.back().state == 5);

    return ok;
}

/*
tests_bounded_history_max_size
  Verifies bounded_history exposes its compile-time cap.
  Tests the following:
  - the max_size constant equals the template argument
  - the default cap equals D_MEMENTO_DEFAULT_HISTORY_CAPACITY
*/
bool
tests_bounded_history_max_size()
{
    static_assert(bounded_history<8>::max_size == 8, "explicit cap");
    static_assert(bounded_history<>::max_size == D_MEMENTO_DEFAULT_HISTORY_CAPACITY,
                  "default cap");

    return ( bounded_history<8>::max_size == 8 &&
             bounded_history<>::max_size == 64 );
}

/*
tests_coalescing_history_suppresses
  Verifies coalescing_history suppresses a duplicate of the newest snapshot.
  Tests the following:
  - should_push is true for the first entry
  - should_push is false when the candidate equals the newest stored snapshot
*/
bool
tests_coalescing_history_suppresses()
{
    int_history h;

    bool ok = coalescing_history::should_push(h, 7);   // empty -> true

    h.push_back(memento<int>(7));
    ok = ok && (!coalescing_history::should_push(h, 7));   // duplicate -> false

    return ok;
}

/*
tests_coalescing_history_distinct
  Verifies coalescing_history keeps a snapshot that differs from the newest.
  Tests the following:
  - should_push is true when the candidate differs from the newest stored one
*/
bool
tests_coalescing_history_distinct()
{
    int_history h;
    h.push_back(memento<int>(7));

    return coalescing_history::should_push(h, 8);   // distinct -> true
}


NS_END  // testing
NS_END  // djinterp
