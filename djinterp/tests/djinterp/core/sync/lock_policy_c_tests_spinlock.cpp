/*******************************************************************************
* djinterp [core]                               lock_policy_c_tests_spinlock.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_c_tests_spinlock.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]                 lock_policy_c_tests_spinlock.cpp
//   Section IV: the spinlock - spinlock_mutex and the c_spinlock policy
//   (C++11+).
//
// spinlock_mutex is a test-and-set lock over std::atomic_flag: lock() busy-
// waits on test_and_set until it wins, unlock() clears the flag, try_lock()
// makes a single attempt.  It is the one C-backed primitive built from a C++
// standard type rather than a platform API, and its operations are all
// noexcept.  The section pins construction, lock/unlock, try_lock on a free
// lock, the fact that try_lock on a HELD lock fails (the spinlock is NOT
// reentrant - a contrast with c_recursive), the noexcept-ness, and
// non-copyability/non-movability (it holds an atomic_flag, which is itself
// non-copyable).  c_spinlock, the policy, gets its aliases, descriptors and
// level.
//
// The whole section is gated to C++11 - std::atomic_flag and thus the spinlock
// do not exist below it - so every reference is inside the guard and each test
// degrades to a stated no-op.  (Single-thread here; the spinlock's mutual
// exclusion under contention is in the concurrency section.)

// djinterp
#include "lock_policy_c_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_spinlock_mutex_constructs
  a spinlock_mutex can be created, starting unlocked.
  Tests the following:
  - a spinlock_mutex is default-constructed (its constructor clears the flag)
    and is immediately acquirable: a try_lock succeeds
  The constructor's explicit clear matters: a default std::atomic_flag is not
  guaranteed clear before C++20, so the constructor clears it to be sure.  Below
  C++11 this is a stated no-op.
*/
bool
tests_spinlock_mutex_constructs()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    spinlock_mutex lock;

    // starts unlocked -> immediately acquirable
    const bool acquirable = lock.try_lock();

    if (acquirable)
    {
        lock.unlock();
    }

    return acquirable;
#else
    return true;
#endif
}

/*
tests_spinlock_mutex_lock_then_unlock
  the spinlock can be locked and unlocked.
  Tests the following:
  - lock() then unlock() completes, and afterwards a try_lock succeeds
  Below C++11 this is a stated no-op.
*/
bool
tests_spinlock_mutex_lock_then_unlock()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    spinlock_mutex lock;

    lock.lock();
    lock.unlock();

    const bool free_after = lock.try_lock();

    if (free_after)
    {
        lock.unlock();
    }

    return free_after;
#else
    return true;
#endif
}

/*
tests_spinlock_mutex_try_lock_when_free
  a try-lock on a free spinlock succeeds and takes it.
  Tests the following:
  - try_lock() returns true on an unlocked spinlock
  - and it is now held: a SECOND try_lock (same thread) returns false (see the
    next test)
  Below C++11 this is a stated no-op.
*/
bool
tests_spinlock_mutex_try_lock_when_free()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    spinlock_mutex lock;

    const bool got = lock.try_lock();

    if (got)
    {
        lock.unlock();
    }

    return got;
#else
    return true;
#endif
}

/*
tests_spinlock_mutex_try_lock_when_held_fails
  the spinlock is NOT reentrant: try_lock on a held lock fails.
  Tests the following:
  - after lock(), a try_lock() on the SAME spinlock returns false - it does not
    re-enter, but it also does not deadlock (try_lock never spins)
  This is the direct contrast with c_recursive, whose try_lock re-enters and
  returns true.  A thread must not try to re-lock a spinlock it already holds
  (calling lock() again WOULD spin forever); the test uses try_lock, which is
  safe, to show the non-reentrancy.  Below C++11 this is a stated no-op.
*/
bool
tests_spinlock_mutex_try_lock_when_held_fails()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    spinlock_mutex lock;

    lock.lock();

    // already held by this thread: a try must fail (no re-entry, no deadlock)
    const bool re_entry_refused = (!lock.try_lock());

    lock.unlock();

    return re_entry_refused;
#else
    return true;
#endif
}

/*
tests_spinlock_mutex_operations_are_noexcept
  every spinlock operation is noexcept.
  Tests the following:
  - the default constructor, lock(), unlock() and try_lock() are all noexcept
  A spinlock is a low-level primitive for the tightest critical sections; it
  promises never to throw, which lets it sit in noexcept paths.  Below C++11
  this is a stated no-op.
*/
bool
tests_spinlock_mutex_operations_are_noexcept()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_CONSTEXPR bool ctor_noexcept =
        std::is_nothrow_default_constructible<spinlock_mutex>::value;

    spinlock_mutex lock;

    D_CONSTEXPR bool ops_noexcept =
        ( noexcept(lock.lock())     &&
          noexcept(lock.unlock())   &&
          noexcept(lock.try_lock()) );

    static_assert(ctor_noexcept && ops_noexcept,
                  "spinlock_mutex operations must be noexcept");

    return (ctor_noexcept && ops_noexcept);
#else
    return true;
#endif
}

/*
tests_spinlock_mutex_is_not_copyable
  the spinlock cannot be copied.
  Tests the following:
  - spinlock_mutex is not copy-constructible or copy-assignable
  - it gets this from its std::atomic_flag member, which is itself non-copyable,
    rather than from an explicit deletion - but the effect is what matters
  Below C++11 this is a stated no-op.
*/
bool
tests_spinlock_mutex_is_not_copyable()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_CONSTEXPR bool no_copy =
        ( !std::is_copy_constructible<spinlock_mutex>::value &&
          !std::is_copy_assignable<spinlock_mutex>::value );

    static_assert(no_copy, "spinlock_mutex must not be copyable");

    return no_copy;
#else
    return true;
#endif
}

/*
tests_spinlock_mutex_is_not_movable
  nor can the spinlock be moved.
  Tests the following:
  - spinlock_mutex is neither move-constructible nor move-assignable, again
    because std::atomic_flag is non-movable
  Correct for a lock: its identity (the flag's address) must not move.  Below
  C++11 this is a stated no-op.
*/
bool
tests_spinlock_mutex_is_not_movable()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_CONSTEXPR bool no_move =
        ( !std::is_move_constructible<spinlock_mutex>::value &&
          !std::is_move_assignable<spinlock_mutex>::value );

    static_assert(no_move, "spinlock_mutex must not be movable");

    return no_move;
#else
    return true;
#endif
}

/*
tests_c_spinlock_aliases
  the c_spinlock policy names the spinlock and a c_lock_guard over it.
  Tests the following:
  - mutex_type is spinlock_mutex
  - read_lock_type and write_lock_type are both c_lock_guard<spinlock_mutex>,
    and identical
  So the spinlock policy is exclusive, reusing the same c_lock_guard as the
  other exclusive C policies.  Below C++11 this is a stated no-op.
*/
bool
tests_c_spinlock_aliases()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_CONSTEXPR bool aliases_ok =
        ( std::is_same<c_spinlock::mutex_type, spinlock_mutex>::value      &&
          std::is_same<c_spinlock::read_lock_type,
                       c_lock_guard<spinlock_mutex> >::value               &&
          std::is_same<c_spinlock::write_lock_type,
                       c_lock_guard<spinlock_mutex> >::value               &&
          std::is_same<c_spinlock::read_lock_type,
                       c_spinlock::write_lock_type>::value );

    static_assert(aliases_ok,
                  "c_spinlock must name spinlock_mutex and its c_lock_guard");

    return aliases_ok;
#else
    return true;
#endif
}

/*
tests_c_spinlock_descriptors_and_level
  the spinlock policy is exclusive and thread-safe.
  Tests the following:
  - is_threadsafe is true, is_shared is false, is_timed is false
  - level is thread_safety_level::exclusive
  - unlike the other C policies, c_spinlock spells its descriptors
    `static constexpr bool` rather than `static const bool` (it is a C++11-only
    policy, so it can) - both forms are compile-time constants, checked here by
    static_assert
  Below C++11 this is a stated no-op.
*/
bool
tests_c_spinlock_descriptors_and_level()
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static_assert(c_spinlock::is_threadsafe, "c_spinlock is thread-safe");
    static_assert(!c_spinlock::is_shared,    "c_spinlock is not shared");
    static_assert(!c_spinlock::is_timed,     "c_spinlock is not timed");
    static_assert(c_spinlock::level == thread_safety_level::exclusive,
                  "c_spinlock's level must be exclusive");

    return ( c_spinlock::is_threadsafe    &&
             (!c_spinlock::is_shared)     &&
             (!c_spinlock::is_timed)      &&
             (c_spinlock::level == thread_safety_level::exclusive) );
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
