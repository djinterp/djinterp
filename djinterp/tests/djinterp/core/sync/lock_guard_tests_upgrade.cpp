/*******************************************************************************
* djinterp [core]                                   lock_guard_tests_upgrade.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests_upgrade.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                     lock_guard_tests_upgrade.cpp
//   Section V: upgrade_lock - a read lock promoted to a write lock (C++17+).
//
//   This guard takes an already-held read lock and turns it into an exclusive
// write lock on the same mutex.  The header is explicit that the promotion is
// NOT atomic: the constructor unlocks the read lock and THEN acquires the write
// lock, so there is a window in between where no lock is held and another
// writer
// could slip in.  The suite pins the observable end state - after construction
// the write lock is genuinely held and exclusive - and leaves the (by design)
// non-atomicity as a documented property rather than something to defend
// against.
// downgrade() releases the write lock; the caller must then re-acquire a read
// lock itself, so there is no read lock to check for afterwards.
//
//   The whole section exists only at C++17 and above, where std::shared_mutex
// and the shared policy first appear; below that it degrades to a stated no-op.
// Everything is exercised through shared_lock_policy, the policy the guard is
// actually meant for.

// std
#include <mutex>
#include <utility>
// djinterp
#include "lock_guard_tests.hpp"
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
    #include <shared_mutex>
#endif


NS_DJINTERP
NS_TESTING

/*
tests_upgrade_exposes_its_aliases
  the guard republishes the policy's mutex, read-lock and write-lock types.
  Tests the following:
  - upgrade_lock<P>::mutex_type is P::mutex_type
  - upgrade_lock<P>::read_lock is P::read_lock_type (a shared_lock under the
    shared policy)
  - upgrade_lock<P>::write_lock is P::write_lock_type (a unique_lock)
  Below C++17 this is a stated no-op.
*/
bool
tests_upgrade_exposes_its_aliases()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<upgrade_lock<shared_lock_policy>::mutex_type,
                       shared_lock_policy::mutex_type>::value     &&
          std::is_same<upgrade_lock<shared_lock_policy>::read_lock,
                       shared_lock_policy::read_lock_type>::value &&
          std::is_same<upgrade_lock<shared_lock_policy>::write_lock,
                       shared_lock_policy::write_lock_type>::value );

    static_assert(aliases_ok,
                  "upgrade_lock must republish the policy's lock types");

    return aliases_ok;
#else
    return true;
#endif
}

/*
tests_upgrade_acquires_the_write_lock
  after the (non-atomic) upgrade, the write lock is held and exclusive.
  Tests the following:
  - a read lock is taken, then handed to an upgrade_lock; afterwards owns_lock()
    is true
  - the underlying shared_mutex is now EXCLUSIVELY held: neither try_lock_shared
    (a reader) nor try_lock (a writer) can take it
  So the promotion really did land on an exclusive lock, whatever happened in
  the
  unlocked window during construction.
*/
bool
tests_upgrade_acquires_the_write_lock()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::shared_mutex mutex;

    std::shared_lock<std::shared_mutex> read(mutex);

    upgrade_lock<shared_lock_policy> upgraded(std::move(read), mutex);

    const bool owns = upgraded.owns_lock();

    // exclusive now: no reader and no writer may enter
    const bool reader_excluded = (!mutex.try_lock_shared());

    if (!reader_excluded)
    {
        mutex.unlock_shared();
    }

    const bool writer_excluded = (!mutex.try_lock());

    if (!writer_excluded)
    {
        mutex.unlock();
    }

    return (owns && reader_excluded && writer_excluded);
#else
    return true;
#endif
}

/*
tests_upgrade_empties_the_source_read_lock
  the read lock handed in is released by the constructor.
  Tests the following:
  - the constructor calls unlock() on the read lock it is given, so after the
    upgrade_lock is built, that read lock no longer owns anything
  - (the read lock is bound by rvalue reference, not moved-from, so it still
    exists as a named local - now unlocked)
  This is the "release the read lock" half of the promotion, observed directly
  on
  the caller's lock object.
*/
bool
tests_upgrade_empties_the_source_read_lock()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::shared_mutex mutex;

    std::shared_lock<std::shared_mutex> read(mutex);

    const bool read_owned_before = read.owns_lock();

    {
        upgrade_lock<shared_lock_policy> upgraded(std::move(read), mutex);

        // the constructor unlocked the read lock we passed
        const bool read_released = (!read.owns_lock());

        if (!(read_owned_before && read_released))
        {
            return false;
        }
    }

    return true;
#else
    return true;
#endif
}

/*
tests_upgrade_downgrade_releases_the_write_lock
  downgrade() drops the exclusive lock.
  Tests the following:
  - after downgrade(), owns_lock() is false
  - the underlying mutex is fully free: it can be both shared-locked AND
    exclusively locked again
  The header notes the caller must re-acquire a read lock itself after
  downgrade() - so there is deliberately no read lock left behind to check; the
  mutex simply returns to the unlocked state.
*/
bool
tests_upgrade_downgrade_releases_the_write_lock()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::shared_mutex mutex;

    std::shared_lock<std::shared_mutex> read(mutex);

    upgrade_lock<shared_lock_policy> upgraded(std::move(read), mutex);

    upgraded.downgrade();

    const bool disowned = (!upgraded.owns_lock());

    // fully free: a shared lock succeeds...
    const bool shared_ok = mutex.try_lock_shared();

    if (shared_ok)
    {
        mutex.unlock_shared();
    }

    // ...and so does an exclusive one
    const bool exclusive_ok = mutex.try_lock();

    if (exclusive_ok)
    {
        mutex.unlock();
    }

    return (disowned && shared_ok && exclusive_ok);
#else
    return true;
#endif
}

/*
tests_upgrade_releases_on_destruction
  an owning upgrade_lock frees the mutex when it leaves scope.
  Tests the following:
  - after the guard's scope ends (without an explicit downgrade), the mutex is
    fully free
  - the write lock is released by its member's destructor
*/
bool
tests_upgrade_releases_on_destruction()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::shared_mutex mutex;

    {
        std::shared_lock<std::shared_mutex> read(mutex);

        upgrade_lock<shared_lock_policy> upgraded(std::move(read), mutex);

        if (!upgraded.owns_lock())
        {
            return false;
        }
    }

    // fully free after scope
    const bool exclusive_ok = mutex.try_lock();

    if (exclusive_ok)
    {
        mutex.unlock();
    }

    return exclusive_ok;
#else
    return true;
#endif
}

/*
tests_upgrade_is_not_copyable
  the guard owns an exclusive lock and cannot be copied.
  Tests the following:
  - upgrade_lock is not copy-constructible or copy-assignable
  Below C++17 this is a stated no-op.
*/
bool
tests_upgrade_is_not_copyable()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<
              upgrade_lock<shared_lock_policy> >::value &&
          !std::is_copy_assignable<
              upgrade_lock<shared_lock_policy> >::value );

    static_assert(no_copy, "upgrade_lock must not be copyable");

    return no_copy;
#else
    return true;
#endif
}

/*
tests_upgrade_is_move_constructible
  the guard is movable on construction.
  Tests the following:
  - upgrade_lock is move-constructible (declared `= default`), since its
    unique_lock member moves
  - it is NOT move-assignable (none declared, copy deleted) - the same asymmetry
    as the try and timed guards
  Below C++17 this is a stated no-op.
*/
bool
tests_upgrade_is_move_constructible()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool move_ctor =
        std::is_move_constructible<
            upgrade_lock<shared_lock_policy> >::value;

    D_CONSTEXPR bool no_move_assign =
        !std::is_move_assignable<
            upgrade_lock<shared_lock_policy> >::value;

    static_assert(move_ctor,
                  "upgrade_lock must be move-constructible");

    return (move_ctor && no_move_assign);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
