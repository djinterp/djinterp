/*******************************************************************************
* djinterp [core]                                   rcu_tests_epoch_registry.cpp
*
*
* path:      /tests/djinterp/core/sync/rcu_tests_epoch_registry.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                 rcu_tests_epoch_registry.cpp
// Section IV: epoch_registry - the fixed pool of per-thread epoch slots.  A
// thread claims a slot with a lock-free CAS on its active flag; the writer
// scans active slots for the minimum epoch, which is the reclamation frontier
// (a node retired at an epoch strictly below the minimum is safe to free).  The
// section pins default and custom capacity, slot claiming (fresh slot is
// inactive, claim until full, then nullptr), releasing a slot (frees it for
// reuse, null-safe), the min-active-epoch scan (no readers -> inactive; the
// true minimum across several; an idle slot never lowers it), the capacity-0
// corner, and that the registry is neither copyable nor movable (it owns a raw
// slot array).
//
//   Single-thread here; the concurrent claim path is exercised in the
// concurrency section.

// djinterp
#include "rcu_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_registry_default_capacity_is_64
  the default registry holds 64 slots.
  Tests the following:
  - a default-constructed epoch_registry reports capacity() == 64
  This is the documented default thread count.
*/
bool
tests_registry_default_capacity_is_64()
{
    epoch_registry registry;

    return (registry.capacity() == 64);
}

/*
tests_registry_custom_capacity
  the registry honors a requested capacity.
  Tests the following:
  - epoch_registry(7) reports capacity() == 7
*/
bool
tests_registry_custom_capacity()
{
    epoch_registry registry(7);

    return (registry.capacity() == 7);
}

/*
tests_registry_register_returns_a_slot
  claiming a slot yields a usable pointer.
  Tests the following:
  - register_thread() on a fresh registry returns a non-null slot pointer
  The returned pointer is what a reader hands to an epoch_guard.
*/
bool
tests_registry_register_returns_a_slot()
{
    epoch_registry registry(4);

    std::atomic<std::uint64_t>* slot = registry.register_thread();

    return (slot != NULL);
}

/*
tests_registry_fresh_slot_is_inactive
  a just-claimed slot starts outside any critical section.
  Tests the following:
  - the local_epoch of a freshly registered slot reads epoch_counter::inactive
  A reader that has registered but not yet entered a critical section must not
  hold back reclamation.
*/
bool
tests_registry_fresh_slot_is_inactive()
{
    epoch_registry registry(4);

    std::atomic<std::uint64_t>* slot = registry.register_thread();
    if (!slot)
    {
        return false;
    }

    return (slot->load() == epoch_counter::inactive);
}

/*
tests_registry_registers_until_full
  the pool hands out exactly capacity slots, then reports exhaustion.
  Tests the following:
  - on a capacity-3 registry, three register_thread() calls all succeed
  - the fourth returns nullptr (all slots in use)
*/
bool
tests_registry_registers_until_full()
{
    epoch_registry registry(3);

    std::atomic<std::uint64_t>* a = registry.register_thread();
    std::atomic<std::uint64_t>* b = registry.register_thread();
    std::atomic<std::uint64_t>* c = registry.register_thread();
    std::atomic<std::uint64_t>* d = registry.register_thread();

    const bool three_ok = (a && b && c);
    const bool full      = (d == NULL);

    return (three_ok && full);
}

/*
tests_registry_unregister_frees_the_slot
  releasing a slot returns it to the pool.
  Tests the following:
  - a full (capacity-1) registry hands back nullptr, but after unregistering the
    one slot a fresh register_thread() succeeds again
  Slot reuse is what lets a bounded pool serve threads that come and go.
*/
bool
tests_registry_unregister_frees_the_slot()
{
    epoch_registry registry(1);

    std::atomic<std::uint64_t>* first = registry.register_thread();
    std::atomic<std::uint64_t>* full  = registry.register_thread();

    if (!first || full)
    {
        return false;
    }

    registry.unregister_thread(first);

    std::atomic<std::uint64_t>* again = registry.register_thread();

    return (again != NULL);
}

/*
tests_registry_unregister_null_is_safe
  releasing a null slot is a harmless no-op.
  Tests the following:
  - unregister_thread(nullptr) does nothing and does not crash
  - a subsequent register_thread() still works
  Callers may unregister defensively without first checking for null.
*/
bool
tests_registry_unregister_null_is_safe()
{
    epoch_registry registry(2);

    registry.unregister_thread(NULL);

    std::atomic<std::uint64_t>* slot = registry.register_thread();

    return (slot != NULL);
}

/*
tests_registry_min_active_epoch_with_no_readers
  with nobody reading, everything is reclaimable.
  Tests the following:
  - min_active_epoch() on a registry with no active slots returns
    epoch_counter::inactive
  inactive is the maximum epoch, so `retire_epoch < inactive` is true for every
  real retirement - the frontier is wide open.
*/
bool
tests_registry_min_active_epoch_with_no_readers()
{
    epoch_registry registry(4);

    return (registry.min_active_epoch() == epoch_counter::inactive);
}

/*
tests_registry_min_active_epoch_is_the_minimum
  the frontier is the smallest epoch any active reader is holding.
  Tests the following:
  - with three active slots stamped 5, 3 and 9, min_active_epoch() returns 3
  The oldest reader determines how far back reclamation may proceed.
*/
bool
tests_registry_min_active_epoch_is_the_minimum()
{
    epoch_registry registry(4);

    std::atomic<std::uint64_t>* a = registry.register_thread();
    std::atomic<std::uint64_t>* b = registry.register_thread();
    std::atomic<std::uint64_t>* c = registry.register_thread();
    if (!a || !b || !c)
    {
        return false;
    }

    a->store(5);
    b->store(3);
    c->store(9);

    return (registry.min_active_epoch() == 3);
}

/*
tests_registry_idle_slot_does_not_lower_the_minimum
  a registered-but-idle reader imposes no constraint.
  Tests the following:
  - with one slot stamped 4 and another left at inactive, min_active_epoch()
    returns 4 (the idle slot does not drag it down)
  - with every active slot idle, min_active_epoch() returns inactive
  Because inactive is the maximum, an idle slot's epoch is never below a real
  one, so it never becomes the minimum unless it is the only kind present.
*/
bool
tests_registry_idle_slot_does_not_lower_the_minimum()
{
    epoch_registry registry(4);

    std::atomic<std::uint64_t>* busy = registry.register_thread();
    std::atomic<std::uint64_t>* idle = registry.register_thread();
    if (!busy || !idle)
    {
        return false;
    }

    busy->store(4);
    // idle keeps its fresh value, inactive
    const bool busy_wins = (registry.min_active_epoch() == 4);

    busy->store(epoch_counter::inactive);          // now both idle
    const bool all_idle_is_inactive =
        (registry.min_active_epoch() == epoch_counter::inactive);

    return (busy_wins && all_idle_is_inactive);
}

/*
tests_registry_capacity_zero_is_handled
  a zero-capacity registry is inert but well-behaved.
  Tests the following:
  - capacity() reports 0
  - register_thread() returns nullptr (there is nothing to hand out)
  - min_active_epoch() returns inactive (no active readers)
  A degenerate size must not misbehave.
*/
bool
tests_registry_capacity_zero_is_handled()
{
    epoch_registry registry(0);

    const bool cap_zero   = (registry.capacity() == 0);
    const bool no_slot    = (registry.register_thread() == NULL);
    const bool min_is_inf =
        (registry.min_active_epoch() == epoch_counter::inactive);

    return (cap_zero && no_slot && min_is_inf);
}

/*
tests_registry_is_not_copyable_or_movable
  the registry owns its slot array outright.
  Tests the following:
  - epoch_registry is not copy-constructible or copy-assignable
  - epoch_registry is not move-constructible or move-assignable
  It holds a raw owning pointer and a destructor that deletes it; copying would
  double-free, and no move is offered, so the type is pinned in place.
*/
bool
tests_registry_is_not_copyable_or_movable()
{
    D_CONSTEXPR bool pinned =
        ( !std::is_copy_constructible<epoch_registry>::value &&
          !std::is_copy_assignable<epoch_registry>::value    &&
          !std::is_move_constructible<epoch_registry>::value &&
          !std::is_move_assignable<epoch_registry>::value );

    static_assert(pinned,
                  "epoch_registry must be neither copyable nor movable");

    return pinned;
}


NS_END  // testing
NS_END  // djinterp
