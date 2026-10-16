/*******************************************************************************
* djinterp [core]                                lock_policy_c_tests_mutexes.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_c_tests_mutexes.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                  lock_policy_c_tests_mutexes.cpp
// Section I: the platform mutex classes - the raw primitives the C policies
// wrap.  On POSIX these are c_mutex_posix (pthread_mutex), c_recursive_mutex_
// posix (a RECURSIVE pthread_mutex) and c_rwlock_posix (pthread_rwlock); on
// Windows, the CRITICAL_SECTION / SRWLOCK equivalents.  The section drives them
// through the policy typedefs (c_exclusive::mutex_type and friends) so it
// builds on any platform, and pins: construction/destruction, lock/unlock and
// try_lock on a free mutex, non-copyability, the RECURSIVE mutex's defining
// ability to be re-locked by one thread, and the rwlock's shared operations.  A
// single platform-guarded test names the concrete classes to confirm the
// mapping.
//
// Single-thread only here - what one thread can observe of a lock.  The claims
// that need contention (real exclusion, reader overlap) are in the concurrency
// section.

// djinterp
#include "lock_policy_c_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_exclusive_mutex_constructs_and_destructs
  a c_exclusive mutex can be created and destroyed.
  Tests the following:
  - a c_exclusive::mutex_type object is built (its constructor initializes the
    platform primitive) and torn down (its destructor releases it) without error
  The lifecycle is the base guarantee everything else rests on: the platform
  init/destroy calls are wired to the C++ constructor/destructor.
*/
bool
tests_exclusive_mutex_constructs_and_destructs()
{
    {
        c_exclusive::mutex_type mutex;
        (void)mutex;
    }   // destructor runs here

    return true;
}

/*
tests_exclusive_mutex_lock_then_unlock
  the mutex can be locked and unlocked.
  Tests the following:
  - lock() then unlock() completes, and afterwards the mutex is free: a
    following try_lock() succeeds (and is released)
  So the lock/unlock pair leaves the mutex usable, not stuck.
*/
bool
tests_exclusive_mutex_lock_then_unlock()
{
    c_exclusive::mutex_type mutex;

    mutex.lock();
    mutex.unlock();

    // it is free again
    const bool free_after = mutex.try_lock();

    if (free_after)
    {
        mutex.unlock();
    }

    return free_after;
}

/*
tests_exclusive_mutex_try_lock_when_free
  a try-lock on a free mutex succeeds and actually takes it.
  Tests the following:
  - try_lock() returns true on an uncontended mutex
  - and it really holds it: this is confirmed cross-thread in the concurrency
    section (a second thread's try_lock then fails); here we simply take and
    release it
*/
bool
tests_exclusive_mutex_try_lock_when_free()
{
    c_exclusive::mutex_type mutex;

    const bool got = mutex.try_lock();

    if (got)
    {
        mutex.unlock();
    }

    return got;
}

/*
tests_exclusive_mutex_is_not_copyable
  the mutex cannot be copied.
  Tests the following:
  - c_exclusive::mutex_type is not copy-constructible or copy-assignable
  It achieves this the C++98 way - a private, undeclared copy constructor and
  assignment operator - rather than with `= delete`, but the observable effect,
  which is what this checks, is the same: copying does not compile.
*/
bool
tests_exclusive_mutex_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<c_exclusive::mutex_type>::value &&
          !std::is_copy_assignable<c_exclusive::mutex_type>::value );

    static_assert(no_copy, "a C mutex must not be copyable");

    return no_copy;
}

/*
tests_exclusive_mutex_is_not_movable
  the mutex cannot be moved either.
  Tests the following:
  - c_exclusive::mutex_type is neither move-constructible nor move-assignable
  Declaring the copy operations (privately) and a destructor suppresses the
  implicit move.  A mutex owns a platform primitive tied to its address, so this
  is correct: moving one would leave the primitive behind.
*/
bool
tests_exclusive_mutex_is_not_movable()
{
    D_CONSTEXPR bool no_move =
        ( !std::is_move_constructible<c_exclusive::mutex_type>::value &&
          !std::is_move_assignable<c_exclusive::mutex_type>::value );

    static_assert(no_move, "a C mutex must not be movable");

    return no_move;
}

/*
tests_recursive_mutex_allows_reentrant_locking
  the recursive mutex may be locked more than once by one thread.
  Tests the following:
  - one thread calls lock() twice on a c_recursive mutex and then unlock()
    twice, with no deadlock
  - so the mutex counts ownership rather than blocking its owner
  This is THE behaviour that separates c_recursive from c_exclusive: the same
  two lock() calls on a c_exclusive mutex would deadlock the thread.  The
  reentrancy is exercised here on a single thread (its whole point); the
  concurrency section separately confirms the recursive mutex still excludes
  OTHER threads.
*/
bool
tests_recursive_mutex_allows_reentrant_locking()
{
    c_recursive::mutex_type mutex;

    mutex.lock();
    mutex.lock();     // would deadlock a non-recursive mutex
    mutex.unlock();
    mutex.unlock();

    // and it ends free
    const bool free_after = mutex.try_lock();

    if (free_after)
    {
        mutex.unlock();
    }

    return free_after;
}

/*
tests_recursive_mutex_try_lock_is_reentrant
  a try-lock on a recursive mutex the thread already holds succeeds.
  Tests the following:
  - a thread locks a c_recursive mutex, then try_lock() on the SAME mutex
    returns true (it re-enters, bumping the recursion count) - so two unlocks
    are then needed
  On a non-recursive mutex the same try_lock would fail (or, for lock(),
  deadlock).  This pins that the recursion applies to try_lock too, not just
  lock.
*/
bool
tests_recursive_mutex_try_lock_is_reentrant()
{
    c_recursive::mutex_type mutex;

    mutex.lock();

    const bool re_entered = mutex.try_lock();

    if (re_entered)
    {
        mutex.unlock();   // release the reentrant acquisition
    }

    mutex.unlock();       // release the original

    return re_entered;
}

/*
tests_rwlock_shared_ops_are_callable
  the read-write lock's shared and exclusive operations both work.
  Tests the following:
  - lock_shared() / unlock_shared() complete (the reader path)
  - lock() / unlock() complete (the writer path)
  - and the lock ends free (a try_lock succeeds afterwards)
  So the rwlock backing c_shared exposes both halves of a reader/writer lock.
  Reader CONCURRENCY - the point of a shared lock - is shown in the concurrency
  section; here we confirm the operations exist and leave the lock usable.
*/
bool
tests_rwlock_shared_ops_are_callable()
{
    c_shared::mutex_type rwlock;

    rwlock.lock_shared();
    rwlock.unlock_shared();

    rwlock.lock();
    rwlock.unlock();

    const bool free_after = rwlock.try_lock();

    if (free_after)
    {
        rwlock.unlock();
    }

    return free_after;
}

/*
tests_rwlock_try_lock_when_free
  a write try-lock on a free rwlock succeeds.
  Tests the following:
  - try_lock() (a WRITE / exclusive try) returns true on an uncontended rwlock
  - (the rwlock exposes only a write try_lock, not a shared one - matching its
    pthread_rwlock_trywrlock backing)
*/
bool
tests_rwlock_try_lock_when_free()
{
    c_shared::mutex_type rwlock;

    const bool got = rwlock.try_lock();

    if (got)
    {
        rwlock.unlock();
    }

    return got;
}

/*
tests_the_concrete_classes_back_the_policies
  the policies' mutex_type is the platform's concrete class.
  Tests the following:
  - on POSIX: c_exclusive::mutex_type is c_mutex_posix, c_recursive::mutex_type
    is c_recursive_mutex_posix, and c_shared::mutex_type is c_rwlock_posix
  - on Windows: c_exclusive::mutex_type is c_mutex_win32 and
    c_shared::mutex_type is c_rwlock_win32
  This is the one test that names the platform-specific classes, so it is
  guarded by platform; everywhere else the suite goes through the typedefs.  It
  documents which concrete primitive each policy resolves to on the build's
  platform.
*/
bool
tests_the_concrete_classes_back_the_policies()
{
#if defined(D_HAS_PTHREADS) && D_HAS_PTHREADS
    D_CONSTEXPR bool mapped =
        ( std::is_same<c_exclusive::mutex_type, c_mutex_posix>::value  &&
          std::is_same<c_recursive::mutex_type,
                       c_recursive_mutex_posix>::value                 &&
          std::is_same<c_shared::mutex_type, c_rwlock_posix>::value );

    static_assert(mapped,
                  "POSIX policies must be backed by the pthread classes");

    return mapped;
#elif D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    D_CONSTEXPR bool mapped =
        ( std::is_same<c_exclusive::mutex_type, c_mutex_win32>::value &&
          std::is_same<c_shared::mutex_type, c_rwlock_win32>::value );

    static_assert(mapped,
                  "Windows policies must be backed by the Win32 classes");

    return mapped;
#else
    // no platform primitives on this target; nothing to map
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
