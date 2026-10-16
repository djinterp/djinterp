/*******************************************************************************
* djinterp [core]                                       lock_guard_tests_try.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests_try.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                         lock_guard_tests_try.cpp
//   Section III: scoped_try_lock - a non-blocking exclusive lock attempt.
//
//   Unlike the blocking guards, this one may FAIL to acquire, and the caller
// must ask owns_lock() before touching the protected resource.  It holds a
// mutex reference and a bool, tries the lock in its constructor, and unlocks in
// its destructor only if it owns.  So the section pins the two outcomes (free
// ->
// owned, held -> not owned), the manual release() path and its idempotence, and
// the move behaviour: the guard is move-CONSTRUCTIBLE (a custom move ctor
// transfers ownership and disowns the source) but not move-ASSIGNABLE, and a
// moved-from guard must not unlock.
//
//   The null-policy corner is the sharp one: no_op_mutex::try_lock() always
// returns true, so a scoped_try_lock<null_lock_policy> ALWAYS owns the lock -
// there is no exclusion to fail against.

// std
#include <mutex>
#include <utility>
// djinterp
#include "lock_guard_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_try_exposes_its_mutex_alias
  the guard republishes the policy's mutex type.
  Tests the following:
  - scoped_try_lock<P>::mutex_type is P::mutex_type
  - (the try guard names no lock_type - it holds the mutex directly and tracks
    ownership itself, rather than owning a std lock object)
*/
bool
tests_try_exposes_its_mutex_alias()
{
    D_CONSTEXPR bool alias_ok =
        std::is_same<scoped_try_lock<exclusive_lock_policy>::mutex_type,
                     exclusive_lock_policy::mutex_type>::value;

    static_assert(alias_ok,
                  "scoped_try_lock must republish the policy's mutex type");

    return alias_ok;
}

/*
tests_try_acquires_a_free_mutex
  a try-lock on a free mutex succeeds.
  Tests the following:
  - owns_lock() is true after constructing over an unlocked std::mutex
  - and the mutex really is held: a second try from the same thread fails
  - owns_lock() is const and noexcept
*/
bool
tests_try_acquires_a_free_mutex()
{
    std::mutex mutex;

    scoped_try_lock<exclusive_lock_policy> guard(mutex);

    D_CONSTEXPR bool is_noexcept =
        noexcept(std::declval<
                     const scoped_try_lock<exclusive_lock_policy>&>()
                         .owns_lock());

    static_assert(is_noexcept, "owns_lock() must be noexcept");

    const bool owned = guard.owns_lock();

    // the mutex is genuinely held
    const bool really_held = (!mutex.try_lock());

    return (owned && really_held && is_noexcept);
}

/*
tests_try_fails_on_a_held_mutex
  a try-lock on an already-held mutex fails, without blocking.
  Tests the following:
  - a first guard owns the lock; a second guard over the same mutex does NOT
  - the second guard's construction returns immediately (it does not block) -
    which the test implicitly confirms by completing at all
  - the failed guard owns nothing, so its destructor unlocks nothing, and the
    first guard's lock survives
*/
bool
tests_try_fails_on_a_held_mutex()
{
    std::mutex mutex;

    scoped_try_lock<exclusive_lock_policy> first(mutex);

    const bool first_owns = first.owns_lock();

    {
        scoped_try_lock<exclusive_lock_policy> second(mutex);

        const bool second_fails = (!second.owns_lock());

        if (!second_fails)
        {
            return false;
        }
    }   // second destructs; must NOT release first's lock

    // first still owns it: another try from here still fails
    const bool first_still_holds = (!mutex.try_lock());

    return (first_owns && first_still_holds);
}

/*
tests_try_releases_on_destruction
  an owning guard unlocks when it leaves scope.
  Tests the following:
  - after an owning guard's scope ends, the mutex is free
*/
bool
tests_try_releases_on_destruction()
{
    std::mutex mutex;

    {
        scoped_try_lock<exclusive_lock_policy> guard(mutex);

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
tests_try_release_unlocks_early
  release() drops the lock before the destructor.
  Tests the following:
  - after guard.release(), owns_lock() is false and the mutex is free
  - so a caller can hand the lock back mid-scope without waiting for the guard
    to
    die
*/
bool
tests_try_release_unlocks_early()
{
    std::mutex mutex;

    scoped_try_lock<exclusive_lock_policy> guard(mutex);

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
tests_try_release_is_idempotent
  calling release() twice is safe.
  Tests the following:
  - a second release() does nothing - it does not double-unlock the mutex
  - and the guard's destructor, seeing no ownership, also unlocks nothing
  A double unlock of a std::mutex is undefined behaviour, so the guard's
  ownership flag must gate the unlock.  This pins that it does.
*/
bool
tests_try_release_is_idempotent()
{
    std::mutex mutex;

    {
        scoped_try_lock<exclusive_lock_policy> guard(mutex);

        guard.release();
        guard.release();   // must be a no-op, not a second unlock

        const bool disowned = (!guard.owns_lock());

        if (!disowned)
        {
            return false;
        }
    }   // destructor must also not unlock

    // the mutex is free and in a sound state: we can take and drop it
    const bool sound = mutex.try_lock();

    if (sound)
    {
        mutex.unlock();
    }

    return sound;
}

/*
tests_try_is_not_copyable
  a guard tracking unique ownership cannot be copied.
  Tests the following:
  - scoped_try_lock is not copy-constructible or copy-assignable
*/
bool
tests_try_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<
              scoped_try_lock<exclusive_lock_policy> >::value &&
          !std::is_copy_assignable<
              scoped_try_lock<exclusive_lock_policy> >::value );

    static_assert(no_copy, "scoped_try_lock must not be copyable");

    return no_copy;
}

/*
tests_try_is_move_constructible_only
  the guard moves on construction but not on assignment.
  Tests the following:
  - scoped_try_lock is move-CONSTRUCTIBLE (it declares a move constructor)
  - but NOT move-ASSIGNABLE: no move assignment is declared, and the deleted
    copy
    suppresses the implicit one
  So a guard can be handed off when a new one is built, but not reseated onto an
  existing variable.  An asymmetry worth knowing, pinned.
*/
bool
tests_try_is_move_constructible_only()
{
    D_CONSTEXPR bool move_ctor =
        std::is_move_constructible<
            scoped_try_lock<exclusive_lock_policy> >::value;

    D_CONSTEXPR bool no_move_assign =
        !std::is_move_assignable<
            scoped_try_lock<exclusive_lock_policy> >::value;

    static_assert(move_ctor && no_move_assign,
                  "scoped_try_lock must be move-constructible but not "
                  "move-assignable");

    return (move_ctor && no_move_assign);
}

/*
tests_try_move_transfers_ownership
  moving a guard hands the lock to the destination and disowns the source.
  Tests the following:
  - after moving an owning guard, the source owns_lock() is false and the
    destination owns_lock() is true
  - and there is no double unlock: when both die, the mutex ends free and sound,
    because only the destination unlocks
  The custom move constructor sets the source's ownership flag to false, which
  is
  what makes this safe.  A move that left both owning would unlock twice.
*/
bool
tests_try_move_transfers_ownership()
{
    std::mutex mutex;

    {
        scoped_try_lock<exclusive_lock_policy> source(mutex);

        const bool source_owned = source.owns_lock();

        scoped_try_lock<exclusive_lock_policy> dest(std::move(source));

        const bool transferred =
            ( (!source.owns_lock()) && dest.owns_lock() );

        if (!(source_owned && transferred))
        {
            return false;
        }
    }   // dest unlocks once; source unlocks nothing

    const bool free_and_sound = mutex.try_lock();

    if (free_and_sound)
    {
        mutex.unlock();
    }

    return free_and_sound;
}

/*
tests_try_under_the_null_policy_always_succeeds
  under the null policy a try-lock always owns the lock.
  Tests the following:
  - a scoped_try_lock<null_lock_policy> owns the lock: no_op_mutex::try_lock()
    returns true
  - a SECOND one over the same mutex ALSO owns it - there is no real exclusion
    to
    fail against
  So single-threaded null-mode code that branches on owns_lock() always takes
  the
  "acquired" branch.  This is the null policy's promise (zero overhead) and its
  hazard (no actual mutual exclusion), pinned together.
*/
bool
tests_try_under_the_null_policy_always_succeeds()
{
    no_op_mutex mutex;

    scoped_try_lock<null_lock_policy> first(mutex);
    scoped_try_lock<null_lock_policy> second(mutex);

    // both "own" it - the null mutex never refuses
    return ( first.owns_lock() && second.owns_lock() );
}


NS_END  // testing
NS_END  // djinterp
