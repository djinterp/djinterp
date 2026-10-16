/*******************************************************************************
* djinterp [core]                                      rcu_tests_epoch_guard.cpp
*
*
* path:      /tests/djinterp/core/sync/rcu_tests_epoch_guard.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                    rcu_tests_epoch_guard.cpp
// Section III: epoch_guard - the RAII reader marker.  On construction it
// records the counter's current epoch into a caller-supplied slot (release
// store); on destruction it parks the `inactive` sentinel there.  Between those
// two points the registry can see, by reading the slot, that a reader is in
// that epoch and hold back reclamation accordingly.  The section pins the
// record-on- construction and reset-on-destruction behavior, that the value
// captured is the epoch at CONSTRUCTION time (not a live view of the counter),
// non-copyability, non-movability - the root cause of read_lock()'s C++17
// requirement - and that construction is noexcept.

// djinterp
#include "rcu_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_epoch_guard_records_epoch_on_construction
  entering a critical section stamps the slot with the current epoch.
  Tests the following:
  - a slot pre-set to inactive holds the counter's current epoch (0 here) while
    an epoch_guard built over it is alive
  This is how a reader announces "I am reading as of epoch E".
*/
bool
tests_epoch_guard_records_epoch_on_construction()
{
    epoch_counter              counter;
    std::atomic<std::uint64_t> slot(epoch_counter::inactive);

    epoch_guard guard(counter, slot);

    return (slot.load() == 0);
}

/*
tests_epoch_guard_resets_to_inactive_on_destruction
  leaving the critical section clears the slot back to inactive.
  Tests the following:
  - after the guard's scope ends, the slot reads epoch_counter::inactive again
  A reader that has exited must stop holding back reclamation, so the slot must
  return to the "no constraint" sentinel.
*/
bool
tests_epoch_guard_resets_to_inactive_on_destruction()
{
    epoch_counter              counter;
    std::atomic<std::uint64_t> slot(0);

    {
        epoch_guard guard(counter, slot);
        (void)guard;
    }

    return (slot.load() == epoch_counter::inactive);
}

/*
tests_epoch_guard_captures_current_epoch
  the guard records the epoch as of its construction, not a live counter view.
  Tests the following:
  - with the counter advanced to 3 before the guard is built, the slot holds 3
  - advancing the counter further AFTER construction does not change the slot
  The stamp is a snapshot taken once, at entry.
*/
bool
tests_epoch_guard_captures_current_epoch()
{
    epoch_counter counter;
    counter.advance();
    counter.advance();
    counter.advance();                       // current() == 3

    std::atomic<std::uint64_t> slot(epoch_counter::inactive);

    epoch_guard guard(counter, slot);
    const bool captured_three = (slot.load() == 3);

    counter.advance();                       // counter moves on to 4
    const bool still_three = (slot.load() == 3);

    return (captured_three && still_three);
}

/*
tests_epoch_guard_is_not_copyable
  a critical-section marker cannot be copied.
  Tests the following:
  - epoch_guard is not copy-constructible or copy-assignable (both deleted)
  Two guards over one slot would each try to reset it, corrupting the reader's
  announced epoch.
*/
bool
tests_epoch_guard_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<epoch_guard>::value &&
          !std::is_copy_assignable<epoch_guard>::value );

    static_assert(no_copy, "epoch_guard must not be copyable");

    return no_copy;
}

/*
tests_epoch_guard_is_not_movable
  the marker cannot be moved either - and this is the root of a version fact.
  Tests the following:
  - epoch_guard is neither move-constructible nor move-assignable
  It holds a reference member and declares a destructor, so no move is
  implicitly available and the deleted copy is the only candidate.  This is
  exactly why rcu_read_guard (which holds an epoch_guard and defaults its move)
  ends up non- movable, and thus why read_lock() needs C++17 - see the header
  section.
*/
bool
tests_epoch_guard_is_not_movable()
{
    D_CONSTEXPR bool no_move =
        ( !std::is_move_constructible<epoch_guard>::value &&
          !std::is_move_assignable<epoch_guard>::value );

    static_assert(no_move, "epoch_guard must not be movable");

    return no_move;
}

/*
tests_epoch_guard_construction_is_noexcept
  entering a critical section does not throw.
  Tests the following:
  - constructing an epoch_guard from a counter and a slot is a noexcept
    operation
  Read-side entry is on the fast path and must be usable where exceptions are
  not.
*/
bool
tests_epoch_guard_construction_is_noexcept()
{
    epoch_counter              counter;
    std::atomic<std::uint64_t> slot(epoch_counter::inactive);

    D_CONSTEXPR bool ctor_noexcept =
        noexcept(epoch_guard(counter, slot));

    static_assert(ctor_noexcept,
                  "epoch_guard construction must be noexcept");

    return ctor_noexcept;
}

/*
tests_epoch_guard_two_slots_are_independent
  guards over different slots each stamp only their own.
  Tests the following:
  - two guards, over two slots, built at epochs 1 and 2 respectively, hold 1 and
    2 - neither disturbs the other
  - when both have exited, both slots read inactive
  A registry holds many slots at once; each reader's marker must be local to its
  own slot.
*/
bool
tests_epoch_guard_two_slots_are_independent()
{
    epoch_counter counter;
    counter.advance();                       // current() == 1

    std::atomic<std::uint64_t> slot_a(epoch_counter::inactive);
    std::atomic<std::uint64_t> slot_b(epoch_counter::inactive);

    bool inner_ok = false;
    {
        epoch_guard guard_a(counter, slot_a);   // stamps 1
        counter.advance();                      // current() == 2
        epoch_guard guard_b(counter, slot_b);   // stamps 2

        inner_ok = ( (slot_a.load() == 1) &&
                     (slot_b.load() == 2) );
    }

    const bool both_cleared =
        ( (slot_a.load() == epoch_counter::inactive) &&
          (slot_b.load() == epoch_counter::inactive) );

    return (inner_ok && both_cleared);
}


NS_END  // testing
NS_END  // djinterp
