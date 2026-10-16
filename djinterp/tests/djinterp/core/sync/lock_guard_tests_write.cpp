/*******************************************************************************
* djinterp [core]                                     lock_guard_tests_write.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests_write.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                       lock_guard_tests_write.cpp
//   Section II: scoped_write_lock - an exclusive lock for exactly one scope.
//
//   The write guard acquires the policy's write_lock_type, which is ALWAYS an
// exclusive lock, whatever the policy - even under a shared policy, where the
// read lock is shared, the write lock is a std::unique_lock.  So the acquire
// and
// release behaviour is uniform across policies and directly observable: while
// the guard is held, the underlying mutex cannot be taken by anyone, reader or
// writer.  The section mirrors the read section's structure, and pins the one
// thing that distinguishes write from read - that a write lock under a shared
// policy excludes even other readers.
//
//   The null-policy move edge is identical to the read guard's and is pinned
// again here, because the two guards are separate types and a fix to one would
// not touch the other.

// std
#include <mutex>
// djinterp
#include "lock_guard_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_write_exposes_its_policy_aliases
  the guard republishes the policy's write-lock and mutex types.
  Tests the following:
  - scoped_write_lock<P>::lock_type is P::write_lock_type
  - scoped_write_lock<P>::mutex_type is P::mutex_type
  - under the shared policy (C++17), write_lock_type is a unique_lock even
    though
    the policy's READ lock is a shared_lock
*/
bool
tests_write_exposes_its_policy_aliases()
{
    D_CONSTEXPR bool exclusive_aliases =
        ( std::is_same<scoped_write_lock<exclusive_lock_policy>::lock_type,
                       exclusive_lock_policy::write_lock_type>::value &&
          std::is_same<scoped_write_lock<exclusive_lock_policy>::mutex_type,
                       exclusive_lock_policy::mutex_type>::value );

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool shared_write_is_exclusive =
        std::is_same<scoped_write_lock<shared_lock_policy>::lock_type,
                     std::unique_lock<std::shared_mutex> >::value;
#else
    D_CONSTEXPR bool shared_write_is_exclusive = true;
#endif

    static_assert(exclusive_aliases,
                  "scoped_write_lock must republish the policy's lock types");

    return (exclusive_aliases && shared_write_is_exclusive);
}

/*
tests_write_lock_acquires_exclusively
  building the guard takes the lock, exclusively.
  Tests the following:
  - while a scoped_write_lock<exclusive_lock_policy> is alive, a try_lock on the
    underlying std::mutex FAILS
  - the guard holds the exclusive lock for its whole scope
*/
bool
tests_write_lock_acquires_exclusively()
{
    std::mutex mutex;

    scoped_write_lock<exclusive_lock_policy> guard(mutex);

    const bool held = (!mutex.try_lock());

    return held;
}

/*
tests_write_lock_releases_on_destruction
  the lock is gone once the guard leaves scope.
  Tests the following:
  - after the guard's scope ends, a try_lock on the mutex SUCCEEDS
*/
bool
tests_write_lock_releases_on_destruction()
{
    std::mutex mutex;

    {
        scoped_write_lock<exclusive_lock_policy> guard(mutex);

        if (mutex.try_lock())
        {
            mutex.unlock();
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
tests_write_lock_is_not_copyable
  a guard owns a lock and cannot be copied.
  Tests the following:
  - scoped_write_lock is not copy-constructible or copy-assignable
*/
bool
tests_write_lock_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<
              scoped_write_lock<exclusive_lock_policy> >::value &&
          !std::is_copy_assignable<
              scoped_write_lock<exclusive_lock_policy> >::value );

    static_assert(no_copy, "scoped_write_lock must not be copyable");

    return no_copy;
}

/*
tests_write_lock_is_movable_for_a_real_policy
  for a policy whose lock is movable, the guard is movable.
  Tests the following:
  - scoped_write_lock<exclusive_lock_policy> is move-constructible and
    move-assignable - its std::unique_lock member moves
*/
bool
tests_write_lock_is_movable_for_a_real_policy()
{
    D_CONSTEXPR bool movable =
        ( std::is_move_constructible<
              scoped_write_lock<exclusive_lock_policy> >::value &&
          std::is_move_assignable<
              scoped_write_lock<exclusive_lock_policy> >::value );

    static_assert(movable,
                  "scoped_write_lock over a real policy must be movable");

    return movable;
}

/*
tests_write_lock_is_not_movable_for_the_null_policy
  the same sharp edge as the read guard: NOT movable under null_lock_policy.
  Tests the following:
  - scoped_write_lock<null_lock_policy> is neither move-constructible nor
    move-assignable, because its no_op_guard member is non-movable and the
    guard's `= default` move is therefore implicitly deleted
  - nor is it copyable
  Pinned separately from the read guard because they are distinct types.
*/
bool
tests_write_lock_is_not_movable_for_the_null_policy()
{
    D_CONSTEXPR bool not_movable =
        ( !std::is_move_constructible<
              scoped_write_lock<null_lock_policy> >::value &&
          !std::is_move_assignable<
              scoped_write_lock<null_lock_policy> >::value );

    D_CONSTEXPR bool not_copyable =
        !std::is_copy_constructible<
            scoped_write_lock<null_lock_policy> >::value;

    static_assert(not_movable,
                  "scoped_write_lock<null_lock_policy> must not be movable");

    return (not_movable && not_copyable);
}

/*
tests_write_lock_is_exclusive_even_under_the_shared_policy
  a write lock excludes EVERYONE, readers included.
  Tests the following:
  - while a scoped_write_lock<shared_lock_policy> is held, the underlying
    shared_mutex cannot be shared-locked (try_lock_shared fails) NOR exclusively
    locked (try_lock fails)
  - so the write guard is a true exclusive lock even on the policy that
    otherwise
    admits concurrent readers
  This is the counterpart to the read guard's shared test: read admits readers,
  write admits no one.  Below C++17 there is no shared policy and this is a
  stated no-op.
*/
bool
tests_write_lock_is_exclusive_even_under_the_shared_policy()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::shared_mutex mutex;

    scoped_write_lock<shared_lock_policy> guard(mutex);

    // no reader may join
    const bool reader_excluded = (!mutex.try_lock_shared());

    if (!reader_excluded)
    {
        mutex.unlock_shared();
    }

    // and no writer may join
    const bool writer_excluded = (!mutex.try_lock());

    if (!writer_excluded)
    {
        mutex.unlock();
    }

    return (reader_excluded && writer_excluded);
#else
    return true;
#endif
}

/*
tests_write_lock_under_the_null_policy_is_a_no_op
  under the null policy the write guard constructs and destructs, locking
  nothing.
  Tests the following:
  - a scoped_write_lock<null_lock_policy> over a no_op_mutex builds and tears
    down without error
  - the same call site that takes an exclusive lock under a real policy costs
    nothing under the null one
*/
bool
tests_write_lock_under_the_null_policy_is_a_no_op()
{
    no_op_mutex mutex;

    {
        scoped_write_lock<null_lock_policy> guard(mutex);

        (void)guard;
    }

    return mutex.try_lock();
}


NS_END  // testing
NS_END  // djinterp
