/*******************************************************************************
* djinterp [core]                                 lock_policy_c_tests_guards.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_c_tests_guards.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                   lock_policy_c_tests_guards.cpp
//   Section II: the C-backed RAII guards - c_lock_guard and c_shared_guard.
//
//   Both are class templates parameterized on the mutex type.  c_lock_guard
// takes an exclusive lock in its constructor (m.lock()) and releases it in its
// destructor (m.unlock()); c_shared_guard does the same with the shared pair
// (lock_shared / unlock_shared) for a read-write lock.  The section pins, for
// each, that it acquires on construction (observed by trying the underlying
// primitive and seeing the expected refusal) and releases on destruction, that
// it is non-copyable AND - the sharp edge - non-movable, and that c_lock_guard
// really is generic over every C mutex type.
//
// Single-thread here: the acquire/release is observed on one thread by probing
// the underlying mutex around the guard's lifetime.  The cross-thread exclusion
// the guards provide is in the concurrency section.

// djinterp
#include "lock_policy_c_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_lock_guard_acquires_on_construction
  building a c_lock_guard takes the underlying lock.
  Tests the following:
  - while a c_lock_guard over a c_exclusive mutex is alive, a try_lock directly
    on that mutex FAILS - the guard is holding it
  - the guard held the lock for the whole scope
*/
bool
tests_lock_guard_acquires_on_construction()
{
    c_exclusive::mutex_type mutex;

    c_lock_guard<c_exclusive::mutex_type> guard(mutex);

    // the guard holds it, so a direct try fails
    const bool held = (!mutex.try_lock());

    return held;
}

/*
tests_lock_guard_releases_on_destruction
  the lock is gone once the guard leaves scope.
  Tests the following:
  - after the guard's scope ends, a try_lock on the mutex SUCCEEDS
  So the destructor released what the constructor took.
*/
bool
tests_lock_guard_releases_on_destruction()
{
    c_exclusive::mutex_type mutex;

    {
        c_lock_guard<c_exclusive::mutex_type> guard(mutex);

        if (mutex.try_lock())
        {
            mutex.unlock();
            return false;   // should have been held
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
tests_lock_guard_is_not_copyable
  a guard owns a lock and cannot be copied.
  Tests the following:
  - c_lock_guard is not copy-constructible or copy-assignable (D_DELETE)
*/
bool
tests_lock_guard_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<
              c_lock_guard<c_exclusive::mutex_type> >::value &&
          !std::is_copy_assignable<
              c_lock_guard<c_exclusive::mutex_type> >::value );

    static_assert(no_copy, "c_lock_guard must not be copyable");

    return no_copy;
}

/*
tests_lock_guard_is_not_movable
  the guard cannot be moved either - the recurring sharp edge.
  Tests the following:
  - c_lock_guard is neither move-constructible nor move-assignable
  - because it deletes its copy operations AND declares a destructor, so no
    implicit move is generated
  This is the same shape as no_op_guard and the std lock guards: a guard that
  looks like it should move does not.  Generic code that tries to move one will
  not compile - worth knowing.
*/
bool
tests_lock_guard_is_not_movable()
{
    D_CONSTEXPR bool no_move =
        ( !std::is_move_constructible<
              c_lock_guard<c_exclusive::mutex_type> >::value &&
          !std::is_move_assignable<
              c_lock_guard<c_exclusive::mutex_type> >::value );

    static_assert(no_move, "c_lock_guard must not be movable");

    return no_move;
}

/*
tests_shared_guard_acquires_shared_on_construction
  building a c_shared_guard takes a SHARED lock.
  Tests the following:
  - while a c_shared_guard over a c_shared rwlock is alive, a WRITE try_lock on
    the rwlock FAILS (a writer cannot join a held read lock)...
  - ...but the read lock is genuinely shared, which the concurrency section
    confirms by overlapping two readers
  Here we show the reader is holding SOMETHING that excludes a writer.
*/
bool
tests_shared_guard_acquires_shared_on_construction()
{
    c_shared::mutex_type rwlock;

    c_shared_guard<c_shared::mutex_type> guard(rwlock);

    // a writer cannot acquire while a reader holds the shared lock
    const bool writer_blocked = (!rwlock.try_lock());

    if (!writer_blocked)
    {
        rwlock.unlock();
    }

    return writer_blocked;
}

/*
tests_shared_guard_releases_on_destruction
  the shared lock is released when the guard leaves scope.
  Tests the following:
  - after the guard's scope ends, a WRITE try_lock on the rwlock SUCCEEDS - the
    reader is gone
*/
bool
tests_shared_guard_releases_on_destruction()
{
    c_shared::mutex_type rwlock;

    {
        c_shared_guard<c_shared::mutex_type> guard(rwlock);
        (void)guard;
    }

    const bool released = rwlock.try_lock();

    if (released)
    {
        rwlock.unlock();
    }

    return released;
}

/*
tests_shared_guard_is_not_copyable
  the shared guard cannot be copied.
  Tests the following:
  - c_shared_guard is not copy-constructible or copy-assignable (D_DELETE)
*/
bool
tests_shared_guard_is_not_copyable()
{
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<
              c_shared_guard<c_shared::mutex_type> >::value &&
          !std::is_copy_assignable<
              c_shared_guard<c_shared::mutex_type> >::value );

    static_assert(no_copy, "c_shared_guard must not be copyable");

    return no_copy;
}

/*
tests_shared_guard_is_not_movable
  nor can the shared guard be moved.
  Tests the following:
  - c_shared_guard is neither move-constructible nor move-assignable, for the
    same reason as c_lock_guard (deleted copy + declared destructor)
  Pinned separately because the two guards are distinct templates.
*/
bool
tests_shared_guard_is_not_movable()
{
    D_CONSTEXPR bool no_move =
        ( !std::is_move_constructible<
              c_shared_guard<c_shared::mutex_type> >::value &&
          !std::is_move_assignable<
              c_shared_guard<c_shared::mutex_type> >::value );

    static_assert(no_move, "c_shared_guard must not be movable");

    return no_move;
}

/*
tests_the_guards_are_templates_over_any_c_mutex
  c_lock_guard works over every C mutex type, not just one.
  Tests the following:
  - c_lock_guard can be instantiated and used over c_exclusive's, c_recursive's
    AND (at C++11+) c_spinlock's mutex types, acquiring and releasing each
  - so the guard is genuinely generic scaffolding, matching how the policies
    reuse it as their read/write lock type
  Each instantiation is exercised by taking and dropping the lock; the recursive
  one is locked once (not re-entered) to keep the check about the guard, not the
  recursion.
*/
bool
tests_the_guards_are_templates_over_any_c_mutex()
{
    bool exclusive_ok = false;
    {
        c_exclusive::mutex_type mutex;
        {
            c_lock_guard<c_exclusive::mutex_type> guard(mutex);
            exclusive_ok = (!mutex.try_lock());
        }
    }

    bool recursive_ok = false;
    {
        c_recursive::mutex_type mutex;
        {
            c_lock_guard<c_recursive::mutex_type> guard(mutex);
            (void)guard;
            recursive_ok = true;   // constructed and will destruct cleanly
        }
    }

    bool spin_ok = true;
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    spin_ok = false;
    {
        c_spinlock::mutex_type mutex;
        {
            c_lock_guard<c_spinlock::mutex_type> guard(mutex);
            spin_ok = (!mutex.try_lock());   // guard holds the spinlock
        }
    }
#endif

    return (exclusive_ok && recursive_ok && spin_ok);
}


NS_END  // testing
NS_END  // djinterp
