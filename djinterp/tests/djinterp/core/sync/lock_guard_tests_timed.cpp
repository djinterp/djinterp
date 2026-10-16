/*******************************************************************************
* djinterp [core]                                     lock_guard_tests_timed.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests_timed.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                       lock_guard_tests_timed.cpp
//   Section IV: scoped_timed_lock - an exclusive lock with a deadline (C++11+).
//
//   The timed guard tries to acquire the lock within a caller-supplied duration
// via the mutex's try_lock_for; if it cannot, it gives up and owns nothing.  It
// is only valid for policies whose mutex is timed (timed_lock_policy,
// shared_timed_lock_policy) - a std::mutex has no try_lock_for, so
// scoped_timed_lock<exclusive_lock_policy> would not compile.  The suite uses
// timed_lock_policy throughout.  Its structure mirrors the try-lock section:
// the
// single-threaded tests pin acquisition of a free mutex, release and its
// idempotence, and the move behaviour; the actual TIMEOUT under contention is a
// concurrency test, since it needs a second thread holding the lock.
//
//   This whole section exists only at C++11 and above, which is the suite's
// floor, so no gating is needed within it - but scoped_timed_lock is itself a
// C++11 feature of the header.

// std
#include <mutex>
#include <utility>
// djinterp
#include "lock_guard_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_timed_exposes_its_mutex_alias
  the guard republishes the policy's mutex type.
  Tests the following:
  - scoped_timed_lock<timed_lock_policy>::mutex_type is std::timed_mutex
*/
bool
tests_timed_exposes_its_mutex_alias()
{
    D_CONSTEXPR bool alias_ok =
        ( std::is_same<scoped_timed_lock<timed_lock_policy>::mutex_type,
                       timed_lock_policy::mutex_type>::value &&
          std::is_same<scoped_timed_lock<timed_lock_policy>::mutex_type,
                       std::timed_mutex>::value );

    static_assert(alias_ok,
                  "scoped_timed_lock must republish the policy's mutex type");

    return alias_ok;
}

/*
tests_timed_acquires_a_free_mutex
  a timed attempt on a free mutex succeeds well within its deadline.
  Tests the following:
  - owns_lock() is true after constructing over an unlocked std::timed_mutex
    with
    a generous timeout
  - the mutex really is held: a plain try_lock from the same thread fails
  - owns_lock() is noexcept
*/
bool
tests_timed_acquires_a_free_mutex()
{
    std::timed_mutex mutex;

    scoped_timed_lock<timed_lock_policy> guard(
        mutex, std::chrono::milliseconds(200));

    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<
                     const scoped_timed_lock<timed_lock_policy>&>()
                         .owns_lock());

    static_assert(is_noexcept, "owns_lock() must be noexcept");

    const bool owned       = guard.owns_lock();
    const bool really_held = (!mutex.try_lock());

    return (owned && really_held && is_noexcept);
}

/*
tests_timed_releases_on_destruction
  an owning timed guard unlocks when it leaves scope.
  Tests the following:
  - after an owning guard's scope ends, the mutex is free
*/
bool
tests_timed_releases_on_destruction()
{
    std::timed_mutex mutex;

    {
        scoped_timed_lock<timed_lock_policy> guard(
            mutex, std::chrono::milliseconds(200));

        if (!guard.owns_lock())
        {
            return false;
        }
    }

    const bool released = mutex.try_lock();

    if (released)
    {
        mutex.unlock();
    }

    return released;
}

/*
tests_timed_release_unlocks_early
  release() drops the lock before the destructor.
  Tests the following:
  - after guard.release(), owns_lock() is false and the mutex is free
*/
bool
tests_timed_release_unlocks_early()
{
    std::timed_mutex mutex;

    scoped_timed_lock<timed_lock_policy> guard(
        mutex, std::chrono::milliseconds(200));

    const bool owned_before = guard.owns_lock();

    guard.release();

    const bool disowned = (!guard.owns_lock());
    const bool freed    = mutex.try_lock();

    if (freed)
    {
        mutex.unlock();
    }

    return (owned_before && disowned && freed);
}

/*
tests_timed_release_is_idempotent
  calling release() twice is safe.
  Tests the following:
  - a second release() does not double-unlock, and the destructor unlocks
    nothing afterwards
  - the mutex ends free and sound
*/
bool
tests_timed_release_is_idempotent()
{
    std::timed_mutex mutex;

    {
        scoped_timed_lock<timed_lock_policy> guard(
            mutex, std::chrono::milliseconds(200));

        guard.release();
        guard.release();

        if (guard.owns_lock())
        {
            return false;
        }
    }

    const bool sound = mutex.try_lock();

    if (sound)
    {
        mutex.unlock();
    }

    return sound;
}

/*
tests_timed_is_not_copyable
  the guard cannot be copied.
  Tests the following:
  - scoped_timed_lock is not copy-constructible or copy-assignable
*/
bool
tests_timed_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<
              scoped_timed_lock<timed_lock_policy> >::value &&
          !std::is_copy_assignable<
              scoped_timed_lock<timed_lock_policy> >::value );

    static_assert(no_copy, "scoped_timed_lock must not be copyable");

    return no_copy;
}

/*
tests_timed_is_move_constructible_only
  the guard moves on construction but not on assignment.
  Tests the following:
  - scoped_timed_lock is move-constructible (declared move ctor) but NOT
    move-assignable (none declared, copy deleted)
  Same asymmetry as scoped_try_lock, pinned separately.
*/
bool
tests_timed_is_move_constructible_only()
{
    D_CONSTEXPR bool move_ctor =
        std::is_move_constructible<
            scoped_timed_lock<timed_lock_policy> >::value;

    D_CONSTEXPR bool no_move_assign =
        !std::is_move_assignable<
            scoped_timed_lock<timed_lock_policy> >::value;

    static_assert(move_ctor && no_move_assign,
                  "scoped_timed_lock must be move-constructible but not "
                  "move-assignable");

    return (move_ctor && no_move_assign);
}

/*
tests_timed_move_transfers_ownership
  moving a timed guard hands the lock over and disowns the source.
  Tests the following:
  - after moving an owning guard, the source owns nothing and the destination
    owns the lock
  - only the destination unlocks, so the mutex ends free and sound
*/
bool
tests_timed_move_transfers_ownership()
{
    std::timed_mutex mutex;

    {
        scoped_timed_lock<timed_lock_policy> source(
            mutex, std::chrono::milliseconds(200));

        const bool source_owned = source.owns_lock();

        scoped_timed_lock<timed_lock_policy> dest(std::move(source));

        const bool transferred =
            ( (!source.owns_lock()) && dest.owns_lock() );

        if (!(source_owned && transferred))
        {
            return false;
        }
    }

    const bool free_and_sound = mutex.try_lock();

    if (free_and_sound)
    {
        mutex.unlock();
    }

    return free_and_sound;
}


NS_END  // testing
NS_END  // djinterp
