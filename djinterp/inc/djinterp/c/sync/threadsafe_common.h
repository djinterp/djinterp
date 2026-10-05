/*******************************************************************************
* djinterp [c]                                               threadsafe_common.h
*
* Cross-platform thread safety vocabulary and inline spinlock.
*   Composes primitives from datomic.h and dmutex.h into higher-level
* synchronization building blocks usable from both C and C++.
*
*   Neither datomic.h nor dmutex.h individually provides a spinlock --
* datomic.h has the atomic flag primitive, dmutex.h has full mutexes.
* This header bridges the gap with an inline spinlock that compiles
* down to a single atomic test-and-set in the spin loop: zero
* overhead beyond the atomic instruction itself.
*
*   Also provides the thread safety level enumeration in C, mirroring
* the C++ thread_safety_level enum class so that C containers can
* report their synchronization guarantees through the same vocabulary.
*
* ZERO-OVERHEAD GUARANTEE:
*   Every function in this header is static inline. No link-time
*   symbol is emitted. On platforms with stdatomic (C11, C++11+),
*   the spinlock compiles to a raw atomic_flag_test_and_set loop.
*   On other platforms, it delegates to the datomic.h shim which
*   maps to Interlocked* or __sync_* with the same cost.
*
* DEPENDENCIES:
*   datomic.h   -- d_atomic_flag, test_and_set, clear, memory orders
*   dmutex.h    -- d_thread_hardware_concurrency (re-exported)
*
*
* path:      /inc/djinterp/c/sync/threadsafe_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.28
*                                                            revised: 2026.09.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    THREAD SAFETY LEVEL (C ENUM)
      ----------------------------
      1.    d_thread_safety_levels

II.   SPINLOCK TYPE
      -------------
      1.    d_spinlock_t
      2.    D_SPINLOCK_INIT

III.  SPINLOCK OPERATIONS (STATIC INLINE)
      -----------------------------------
      1.    d_spinlock_init
      2.    d_spinlock_lock
      3.    d_spinlock_trylock
      4.    d_spinlock_unlock

IV.   HARDWARE CONCURRENCY
      --------------------
      1.    d_thread_hardware_concurrency  (re-exported from dmutex.h)
*/

#ifndef DJINTERP_C_SYNC_THREADSAFE_COMMON_H
#define DJINTERP_C_SYNC_THREADSAFE_COMMON_H 1

// djinterp
#include "./datomic.h"
#include "./dmutex.h"

D_EXTERN_C_BEGIN


// I     Thread safety level (c enum)
// Mirrors the C++ thread_safety_level enum class so that C
// containers can classify their synchronization guarantee
// through the same vocabulary as the C++ trait system.

// d_thread_safety_levels
//   enum: synchronization guarantee classification. Mirrors the C++
// thread_safety_level enum class in lock_policy.hpp.
enum d_thread_safety_levels
{
    D_THREAD_SAFETY_NONE         = 0,
    D_THREAD_SAFETY_ATOMIC_ONLY  = 1,
    D_THREAD_SAFETY_EXCLUSIVE    = 2,
    D_THREAD_SAFETY_SHARED       = 3,
    D_THREAD_SAFETY_TIMED        = 4,
    D_THREAD_SAFETY_SHARED_TIMED = 5
};


// II    Spinlock type
// A spinlock is the lightest possible mutex: a single
// atomic flag with acquire/release semantics. Suitable
// only for very short critical sections where the cost of
// a full mutex (kernel transition, wait queue) is
// disproportionate.

// d_spinlock_t
//   type: inline spinlock backed by d_atomic_flag. A single atomic flag with
// acquire/release semantics -- the lightest mutex there is.
struct d_spinlock
{
    d_atomic_flag flag;
};

typedef struct d_spinlock d_spinlock_t;

// D_SPINLOCK_INIT
//   macro: static initializer for a d_spinlock_t.
#define D_SPINLOCK_INIT                                                       \
    {                                                                         \
        D_ATOMIC_FLAG_INIT                                                    \
    }


// III   Spinlock operations (static inline)
// Every operation is static inline. On stdatomic platforms
// (C11, C++11+) the compiler inlines the atomic instruction
// directly. On Windows/GCC-sync fallback platforms, the
// call goes through the datomic.h shim at the same cost as
// using the Interlocked* / __sync_* primitive directly.

// d_spinlock_init
//   function: runtime initialization of a spinlock.
// Equivalent to D_SPINLOCK_INIT for stack/heap spinlocks.
static inline void
d_spinlock_init(d_spinlock_t* _lock)
{
    d_atomic_flag_clear(&_lock->flag);

    return;
}

// d_spinlock_lock
//   function: acquires the spinlock, spinning until the
// flag is cleared. Uses acquire ordering so that all
// memory operations after the lock are visible.
static inline void
d_spinlock_lock(d_spinlock_t* _lock)
{
    while (d_atomic_flag_test_and_set_explicit(
        &_lock->flag, D_MEMORY_ORDER_ACQUIRE))
    {
        // spin -- suitable only for very short
        // critical sections
    }

    return;
}

// d_spinlock_trylock
//   function: attempts to acquire the spinlock without
// blocking. Returns D_MUTEX_SUCCESS (0) if acquired,
// D_MUTEX_BUSY (-2) if already held.
static inline int
d_spinlock_trylock(d_spinlock_t* _lock)
{
    if (d_atomic_flag_test_and_set_explicit(
            &_lock->flag, D_MEMORY_ORDER_ACQUIRE))
    {
        return D_MUTEX_BUSY;
    }

    return D_MUTEX_SUCCESS;
}

// d_spinlock_unlock
//   function: releases the spinlock. Uses release
// ordering so that all memory operations before the
// unlock are visible to the next acquirer.
static inline void
d_spinlock_unlock(d_spinlock_t* _lock)
{
    d_atomic_flag_clear_explicit(
        &_lock->flag, D_MEMORY_ORDER_RELEASE);

    return;
}


// IV    Hardware concurrency
// Re-exports d_thread_hardware_concurrency from dmutex.h
// for convenience. No additional wrapper -- the dmutex.h
// declaration is sufficient. Documented here for
// discoverability.
//
// int d_thread_hardware_concurrency(void);
//   Returns the number of hardware threads available, or
//   0 if the value cannot be determined.


D_EXTERN_C_END

#endif  // DJINTERP_C_SYNC_THREADSAFE_COMMON_H
