/*******************************************************************************
* djinterp [core]                         reclamation_common_tests_published.cpp
*
*
* path:      /tests/djinterp/core/sync/reclamation_common_tests_published.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]      reclamation_common_tests_published.cpp
//   Section II: published_ptr - the single-writer / many-reader pointer that
// carries the publish half of the pattern (the reclaim_list carries the retire
// half).  A writer swaps a fully-built replacement in with release ordering
// and gets the old pointer back to retire; readers load with acquire ordering
// and are guaranteed to see a completely constructed pointee.
//
//   Ordering itself cannot be asserted from a single thread, so this section
// pins the OBSERVABLE contract - what publish returns, what load then sees,
// how the CAS reports failure, and the type's pinned, one-pointer shape - and
// finishes by driving the publish/retire cycle against a reclaim_list, which
// is how the two halves are meant to be used together.

// djinterp
#include "reclamation_common_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_published_default_is_null
  a default-constructed published_ptr holds nothing.
  Tests the following:
  - load() returns null before anything has been published
  A container that builds its value lazily starts here, so the empty state
  must be well-defined rather than indeterminate.
*/
bool
tests_published_default_is_null()
{
    published_ptr<int> pointer;

    return (pointer.load() == NULL);
}

/*
tests_published_constructs_from_a_pointer
  the initial value can be supplied up front.
  Tests the following:
  - a published_ptr built over a node loads back that exact node
*/
bool
tests_published_constructs_from_a_pointer()
{
    int* node = new int(42);

    published_ptr<int> pointer(node);

    const bool same = ( (pointer.load() == node) &&
                        (*pointer.load() == 42) );

    delete node;

    return same;
}

/*
tests_published_publish_returns_the_previous
  publishing hands the writer the value it replaced.
  Tests the following:
  - publish() returns the pointer that was current before the swap
  That returned pointer is precisely what the writer retires; if publish did
  not surrender it, the old node would be unreachable and leak.
*/
bool
tests_published_publish_returns_the_previous()
{
    int* first  = new int(1);
    int* second = new int(2);

    published_ptr<int> pointer(first);

    int* previous = pointer.publish(second);

    const bool handed_back = (previous == first);

    delete first;
    delete second;

    return handed_back;
}

/*
tests_published_load_sees_the_new_value
  after a publish, readers observe the replacement.
  Tests the following:
  - load() returns the newly published node, and its value
  - a second publish is visible in turn
*/
bool
tests_published_load_sees_the_new_value()
{
    int* first  = new int(1);
    int* second = new int(2);
    int* third  = new int(3);

    published_ptr<int> pointer(first);

    pointer.publish(second);
    const bool sees_second = ( (pointer.load() == second) &&
                               (*pointer.load() == 2) );

    pointer.publish(third);
    const bool sees_third = (*pointer.load() == 3);

    delete first;
    delete second;
    delete third;

    return (sees_second && sees_third);
}

/*
tests_published_publish_accepts_null
  a container may publish emptiness.
  Tests the following:
  - publishing null returns the previous node and leaves load() null
  Clearing a value is a legitimate update; the old node still needs retiring,
  so publish must behave identically for a null replacement.
*/
bool
tests_published_publish_accepts_null()
{
    int* node = new int(9);

    published_ptr<int> pointer(node);

    int* previous = pointer.publish(NULL);

    const bool cleared = ( (previous == node) &&
                           (pointer.load() == NULL) );

    delete node;

    return cleared;
}

/*
tests_published_compare_publish_succeeds_when_expected
  the CAS installs the replacement when nothing changed underneath.
  Tests the following:
  - compare_publish with the true current value returns true and installs the
    replacement
  This is the writer-side retry loop's success case: read, build, swap only if
  no other writer got there first.
*/
bool
tests_published_compare_publish_succeeds_when_expected()
{
    int* first  = new int(1);
    int* second = new int(2);

    published_ptr<int> pointer(first);

    int*       expected = first;
    const bool swapped  = pointer.compare_publish(expected, second);

    const bool installed = ( swapped &&
                             (pointer.load() == second) );

    delete first;
    delete second;

    return installed;
}

/*
tests_published_compare_publish_fails_and_reports
  a losing CAS leaves the value alone and tells the caller what it found.
  Tests the following:
  - compare_publish with a stale expectation returns false
  - the pointer is unchanged
  - the caller's expected variable is UPDATED to the actual current value, so
    a retry loop can rebuild against it without re-loading
  Reporting the observed value is the part that makes the loop efficient, and
  the part a hand-rolled CAS wrapper most often gets wrong.
*/
bool
tests_published_compare_publish_fails_and_reports()
{
    int* current = new int(1);
    int* stale   = new int(2);
    int* wanted  = new int(3);

    published_ptr<int> pointer(current);

    int*       expected = stale;                 // wrong expectation
    const bool swapped  = pointer.compare_publish(expected, wanted);

    const bool rejected =
        ( (!swapped)                    &&
          (pointer.load() == current)   &&
          (expected == current) );      // updated to what was really there

    delete current;
    delete stale;
    delete wanted;

    return rejected;
}

/*
tests_published_is_pinned
  a published pointer never moves.
  Tests the following:
  - published_ptr is neither copyable nor movable, in either construction or
    assignment
  Readers load from its ADDRESS; relocating it would leave them reading a
  stale location, so it inherits the pinned contract rather than the move-only
  one the reclaim_list uses.
*/
bool
tests_published_is_pinned()
{
    typedef published_ptr<int> pointer_type;

    const bool pinned =
        ( !std::is_copy_constructible<pointer_type>::value &&
          !std::is_copy_assignable<pointer_type>::value    &&
          !std::is_move_constructible<pointer_type>::value &&
          !std::is_move_assignable<pointer_type>::value );

    static_assert(!std::is_move_constructible<pointer_type>::value,
                  "published_ptr must be pinned in place");

    return pinned;
}

/*
tests_published_costs_one_pointer
  the wrapper adds nothing to the atomic it holds.
  Tests the following:
  - sizeof(published_ptr<int>) equals sizeof(std::atomic<int*>)
  It sits in the hot path of every read, so it must be exactly the atomic
  pointer it wraps - no bookkeeping, no padding of its own.
*/
bool
tests_published_costs_one_pointer()
{
    return (sizeof(published_ptr<int>) ==
            sizeof(std::atomic<int*>));
}

/*
tests_published_store_installs_without_returning
  store() sets the pointer for INITIALIZATION, handing nothing back.
  Tests the following:
  - after store(), load() reports the stored pointer
  - store() over an existing value replaces it (and, by design, does NOT
    return the old one - which is exactly why it is initialization-only)
  A constructor that must build its pointee before it can install it cannot
  use the pointer constructor; store() is that path.  Callers REPLACING a live
  value must use publish() instead, or the old pointer is lost unretired.
*/
bool
tests_published_store_installs_without_returning()
{
    int* first  = new int(1);
    int* second = new int(2);

    published_ptr<int> pointer;

    const bool starts_null = (pointer.load() == NULL);

    pointer.store(first);
    const bool took_first = ( (pointer.load() == first) &&
                              (*pointer.load() == 1) );

    // storing again simply overwrites - the previous pointer is NOT returned
    pointer.store(second);
    const bool took_second = (pointer.load() == second);

    delete first;
    delete second;

    return (starts_null && took_first && took_second);
}

/*
tests_published_store_is_noexcept
  installing a pointer cannot throw.
  Tests the following:
  - store() is a noexcept expression
  It is called from constructors, which frequently promise noexcept
  themselves; a throwing store would break that promise.
*/
bool
tests_published_store_is_noexcept()
{
    published_ptr<int> pointer;

    const bool is_noexcept = noexcept(pointer.store(NULL));

    static_assert(noexcept(std::declval<published_ptr<int>&>()
                               .store(NULL)),
                  "published_ptr::store must be noexcept");

    return is_noexcept;
}

/*
tests_published_pairs_with_the_reclaim_list
  the two halves compose into the publish/retire cycle.
  Tests the following:
  - five successive updates - build a replacement, publish it, retire the
    pointer publish handed back - leave the current value correct at each step
  - a final drain reclaims exactly the five superseded nodes, in publication
    order, leaving only the live one
  This is the shape both rcu_protected and the hazard-pointer containers use,
  reduced to its two shared pieces.
*/
bool
tests_published_pairs_with_the_reclaim_list()
{
    reclaim_log log;

    published_ptr<int> pointer(new int(0));
    reclaim_list<int>  list;

    bool every_step_ok = true;
    for (int i = 1; i <= 5; ++i)
    {
        int* replacement = new int(i);
        int* superseded  = pointer.publish(replacement);

        list.retire(superseded, log.deleter_for(),
                    static_cast<std::uint64_t>(i));

        if (*pointer.load() != i)
        {
            every_step_ok = false;
        }
    }

    const bool accumulated = (list.pending() == 5);

    const std::size_t drained = list.force_reclaim();

    const bool freed_in_order =
        ( (drained == 5)      &&
          (log.count() == 5)  &&
          (log.freed[0] == 0) &&
          (log.freed[4] == 4) );

    // the live node is not the list's to free
    delete pointer.load();

    return (every_step_ok && accumulated && freed_in_order);
}


NS_END  // testing
NS_END  // djinterp
