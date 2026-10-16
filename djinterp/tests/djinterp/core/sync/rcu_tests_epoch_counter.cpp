/*******************************************************************************
* djinterp [core]                                    rcu_tests_epoch_counter.cpp
*
*
* path:      /tests/djinterp/core/sync/rcu_tests_epoch_counter.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                  rcu_tests_epoch_counter.cpp
// Section II: epoch_counter - the monotonic global epoch that the whole scheme
// is timed against.  Writers advance() it after a modification; readers
// snapshot current() to stamp the start of a critical section.  The section
// pins its starting value, the fetch_add semantics of advance() (it returns the
// PREVIOUS epoch and then increments), the `inactive` sentinel, non-copyability
// and non-movability (it holds an atomic), and that its operations are
// noexcept.
//
//   Single-thread here - the atomic's own thread-safety is the standard
// library's to guarantee; the concurrency section exercises it under load.

// djinterp
#include "rcu_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_epoch_counter_starts_at_zero
  a fresh counter reads epoch 0.
  Tests the following:
  - a default-constructed epoch_counter reports current() == 0
  Epoch 0 is the initial generation; every retired node and reader stamp is
  measured from here.
*/
bool
tests_epoch_counter_starts_at_zero()
{
    epoch_counter counter;

    return (counter.current() == 0);
}

/*
tests_epoch_counter_advance_returns_previous
  advance() hands back the epoch it moved AWAY from.
  Tests the following:
  - the first advance() on a fresh counter returns 0 (the previous epoch), not 1
  - the second advance() returns 1
  This is fetch_add semantics, and it matters: the reclaimer records the return
  of advance() as a retired node's epoch, so the boundary arithmetic downstream
  depends on it being the OLD value.
*/
bool
tests_epoch_counter_advance_returns_previous()
{
    epoch_counter counter;

    const bool first_is_zero = (counter.advance() == 0);
    const bool second_is_one = (counter.advance() == 1);

    return (first_is_zero && second_is_one);
}

/*
tests_epoch_counter_advance_increments
  advance() moves the current epoch forward by one each time.
  Tests the following:
  - after one advance(), current() == 1
  - after three advances total, current() == 3
  So the visible epoch tracks the number of advances.
*/
bool
tests_epoch_counter_advance_increments()
{
    epoch_counter counter;

    counter.advance();
    const bool after_one = (counter.current() == 1);

    counter.advance();
    counter.advance();
    const bool after_three = (counter.current() == 3);

    return (after_one && after_three);
}

/*
tests_epoch_counter_inactive_is_uint64_max
  the inactive sentinel is the largest uint64.
  Tests the following:
  - epoch_counter::inactive == UINT64_MAX
  A thread not in a critical section parks this value in its slot; because it is
  the MAXIMUM, an idle slot never lowers the registry's minimum-active-epoch
  scan (nothing real is >= it), which is exactly how "idle" reads as "no
  constraint".
*/
bool
tests_epoch_counter_inactive_is_uint64_max()
{
    return (epoch_counter::inactive == UINT64_MAX);
}

/*
tests_epoch_counter_inactive_is_constexpr
  the sentinel is a compile-time constant.
  Tests the following:
  - epoch_counter::inactive is usable in a constant expression (it is declared
    `static constexpr std::uint64_t`)
*/
bool
tests_epoch_counter_inactive_is_constexpr()
{
    static_assert(epoch_counter::inactive == UINT64_MAX,
                  "inactive must be a constexpr equal to UINT64_MAX");

    D_CONSTEXPR std::uint64_t sentinel = epoch_counter::inactive;

    return (sentinel == UINT64_MAX);
}

/*
tests_epoch_counter_is_not_copyable
  the counter cannot be copied.
  Tests the following:
  - epoch_counter is not copy-constructible or copy-assignable (both deleted)
  A shared epoch must be one object; copying it would fork the timeline.
*/
bool
tests_epoch_counter_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<epoch_counter>::value &&
          !std::is_copy_assignable<epoch_counter>::value );

    static_assert(no_copy, "epoch_counter must not be copyable");

    return no_copy;
}

/*
tests_epoch_counter_is_not_movable
  the counter cannot be moved either.
  Tests the following:
  - epoch_counter is neither move-constructible nor move-assignable
  It is suppressed by the deleted copy operations, and in any case its
  std::atomic member is itself non-movable.
*/
bool
tests_epoch_counter_is_not_movable()
{
    D_CONSTEXPR bool no_move =
        ( !std::is_move_constructible<epoch_counter>::value &&
          !std::is_move_assignable<epoch_counter>::value );

    static_assert(no_move, "epoch_counter must not be movable");

    return no_move;
}

/*
tests_epoch_counter_operations_are_noexcept
  the counter promises not to throw.
  Tests the following:
  - the default constructor, current() and advance() are all noexcept
  Epoch bookkeeping sits on the reader/writer fast path, so it must be usable in
  noexcept contexts.
*/
bool
tests_epoch_counter_operations_are_noexcept()
{
    D_CONSTEXPR bool ctor_noexcept =
        std::is_nothrow_default_constructible<epoch_counter>::value;

    epoch_counter counter;

    D_CONSTEXPR bool ops_noexcept =
        ( noexcept(counter.current()) &&
          noexcept(counter.advance()) );

    static_assert(ctor_noexcept && ops_noexcept,
                  "epoch_counter operations must be noexcept");

    return (ctor_noexcept && ops_noexcept);
}


NS_END  // testing
NS_END  // djinterp
