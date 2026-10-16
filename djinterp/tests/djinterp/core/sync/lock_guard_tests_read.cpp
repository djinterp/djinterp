/*******************************************************************************
* djinterp [core]                                      lock_guard_tests_read.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests_read.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                        lock_guard_tests_read.cpp
//   Section I: scoped_read_lock - a read lock for exactly one scope.
//
//   The guard acquires the policy's read_lock_type on construction and releases
// it when the member lock is destroyed.  What "read lock" MEANS depends on the
// policy: under a shared policy it is a shared_lock (many readers may hold it
// at
// once), but under exclusive and null policies read_lock_type is the same
// exclusive lock as write_lock_type - "the user code doesn't need to know the
// difference", as the header puts it.  The acquire/release behaviour is
// therefore tested against the EXCLUSIVE policy, where holding the read lock is
// observable by trying to take the underlying std::mutex and failing; the
// shared nature of the shared-policy read lock gets its own test here and its
// concurrent proof in the concurrency section.
//
//   The move edge lives here too: the guard declares `= default` move members,
// which work for a real policy but are IMPLICITLY DELETED for null_lock_policy,
// whose no_op_guard member is non-movable.

// std
// std: the guards are constructed over real std mutexes in these tests, so this
// TU (unlike the header-contract one) names <mutex> directly
#include <mutex>
// djinterp
#include "lock_guard_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_read_exposes_its_policy_aliases
  the guard republishes the policy's lock and mutex types.
  Tests the following:
  - scoped_read_lock<P>::lock_type is P::read_lock_type
  - scoped_read_lock<P>::mutex_type is P::mutex_type
  - checked for both the exclusive policy (unique_lock over std::mutex) and, at
    C++17, the shared policy (shared_lock over std::shared_mutex)
  So a consumer can name the lock a guard holds without repeating the policy.
*/
bool
tests_read_exposes_its_policy_aliases()
{
    D_CONSTEXPR bool exclusive_aliases =
        ( std::is_same<scoped_read_lock<exclusive_lock_policy>::lock_type,
                       exclusive_lock_policy::read_lock_type>::value &&
          std::is_same<scoped_read_lock<exclusive_lock_policy>::mutex_type,
                       exclusive_lock_policy::mutex_type>::value );

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    D_CONSTEXPR bool shared_aliases =
        ( std::is_same<scoped_read_lock<shared_lock_policy>::lock_type,
                       shared_lock_policy::read_lock_type>::value &&
          std::is_same<scoped_read_lock<shared_lock_policy>::lock_type,
                       std::shared_lock<std::shared_mutex> >::value );
#else
    D_CONSTEXPR bool shared_aliases = true;
#endif

    static_assert(exclusive_aliases,
                  "scoped_read_lock must republish the policy's lock types");

    return (exclusive_aliases && shared_aliases);
}

/*
tests_read_lock_acquires_on_construction
  building the guard takes the lock.
  Tests the following:
  - while a scoped_read_lock<exclusive_lock_policy> is alive over a std::mutex,
    a
    try_lock on that mutex from the same thread FAILS - the guard is holding it
  - (std::mutex is non-recursive, so a failed try_lock from the owning thread is
    a sound proof that the lock is held)
  Under the exclusive policy the read lock is exclusive, which is exactly what
  makes this observable.
*/
bool
tests_read_lock_acquires_on_construction()
{
    std::mutex mutex;

    scoped_read_lock<exclusive_lock_policy> guard(mutex);

    // the guard holds the lock, so this must fail
    const bool held = (!mutex.try_lock());

    return held;
}

/*
tests_read_lock_releases_on_destruction
  the lock is gone once the guard leaves scope.
  Tests the following:
  - after the guard's scope ends, a try_lock on the mutex SUCCEEDS
  - so the lock is released automatically, with no explicit call
*/
bool
tests_read_lock_releases_on_destruction()
{
    std::mutex mutex;

    {
        scoped_read_lock<exclusive_lock_policy> guard(mutex);

        // check the guard really took it
        if (mutex.try_lock())
        {
            mutex.unlock();
            return false;
        }
    }

    // the guard is gone; the lock must be free
    const bool released = mutex.try_lock();

    if (released)
    {
        mutex.unlock();
    }

    return released;
}

/*
tests_read_lock_is_not_copyable
  a guard owns a lock and cannot be copied.
  Tests the following:
  - scoped_read_lock is not copy-constructible or copy-assignable (D_DELETE)
  - which is right: two guards over one lock would both release it
*/
bool
tests_read_lock_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<
              scoped_read_lock<exclusive_lock_policy> >::value &&
          !std::is_copy_assignable<
              scoped_read_lock<exclusive_lock_policy> >::value );

    static_assert(no_copy, "scoped_read_lock must not be copyable");

    return no_copy;
}

/*
tests_read_lock_is_movable_for_a_real_policy
  for a policy whose lock is movable, the guard is movable.
  Tests the following:
  - scoped_read_lock<exclusive_lock_policy> is move-constructible and
    move-assignable: its member is a std::unique_lock, which moves
  - so a guard over a real mutex can be returned from a function or stored by
    value
  The header declares these moves `= default`; for a real policy the default is
  a working move.  Contrast the null policy, next test.
*/
bool
tests_read_lock_is_movable_for_a_real_policy()
{
    D_CONSTEXPR bool movable =
        ( std::is_move_constructible<
              scoped_read_lock<exclusive_lock_policy> >::value &&
          std::is_move_assignable<
              scoped_read_lock<exclusive_lock_policy> >::value );

    static_assert(movable,
                  "scoped_read_lock over a real policy must be movable");

    return movable;
}

/*
tests_read_lock_is_not_movable_for_the_null_policy
  the sharp edge: the SAME guard is NOT movable under null_lock_policy.
  Tests the following:
  - scoped_read_lock<null_lock_policy> is neither move-constructible nor
    move-assignable
  - because its member is a no_op_guard, and no_op_guard is non-movable: it
    deletes its copy operations AND declares a destructor, so no implicit move
    is
    generated, which in turn makes the guard's `= default` move IMPLICITLY
    DELETED
  This is a genuine portability trap.  Generic code that does `auto g =
  std::move(other);` on a scoped_read_lock compiles for every real policy and
  fails to compile for the null policy - the one policy meant to be the
  zero-overhead default.  Pinned so the asymmetry is on record.
*/
bool
tests_read_lock_is_not_movable_for_the_null_policy()
{
    D_CONSTEXPR bool not_movable =
        ( !std::is_move_constructible<
              scoped_read_lock<null_lock_policy> >::value &&
          !std::is_move_assignable<
              scoped_read_lock<null_lock_policy> >::value );

    // and it is not copyable either, so it is a pin-in-place type
    D_CONSTEXPR bool not_copyable =
        !std::is_copy_constructible<
            scoped_read_lock<null_lock_policy> >::value;

    static_assert(not_movable,
                  "scoped_read_lock<null_lock_policy> must not be movable");

    return (not_movable && not_copyable);
}

/*
tests_read_lock_under_the_null_policy_is_a_no_op
  under the null policy the guard constructs and destructs, and locks nothing.
  Tests the following:
  - a scoped_read_lock<null_lock_policy> over a no_op_mutex builds and tears
    down
    without error
  - no_op_mutex's operations are all no-ops, so nothing is actually locked - the
    guard is pure scaffolding the optimizer can erase
  This is the single-threaded path: the same call site that locks under a real
  policy costs nothing under the null one.
*/
bool
tests_read_lock_under_the_null_policy_is_a_no_op()
{
    no_op_mutex mutex;

    {
        scoped_read_lock<null_lock_policy> guard(mutex);

        // nothing to observe - the point is that this compiles and runs
        (void)guard;
    }

    // a no_op_mutex try_lock is always true; there is no real state to check
    return mutex.try_lock();
}

/*
tests_read_lock_is_shared_under_the_shared_policy
  under a shared policy the read lock is a SHARED lock, not an exclusive one.
  Tests the following:
  - scoped_read_lock<shared_lock_policy>::lock_type is
    std::shared_lock<std::shared_mutex>
  - while such a guard is held, the underlying shared_mutex can STILL be
    shared-locked (try_lock_shared succeeds) - readers do not exclude readers
  - but it CANNOT be exclusively locked (try_lock fails) - a reader excludes a
    writer
  This is the whole reason the read/write split exists, and it is only visible
  under a policy that actually distinguishes them.  Below C++17 there is no
  shared policy and this is a stated no-op.
*/
bool
tests_read_lock_is_shared_under_the_shared_policy()
{
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    std::shared_mutex mutex;

    scoped_read_lock<shared_lock_policy> guard(mutex);

    // another reader may join: shared does not exclude shared
    const bool another_reader_ok = mutex.try_lock_shared();

    if (another_reader_ok)
    {
        mutex.unlock_shared();
    }

    // but a writer may not: shared excludes exclusive
    const bool writer_excluded = (!mutex.try_lock());

    if (!writer_excluded)
    {
        mutex.unlock();
    }

    return (another_reader_ok && writer_excluded);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
