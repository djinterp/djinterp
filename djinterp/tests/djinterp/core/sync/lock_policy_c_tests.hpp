/*******************************************************************************
* djinterp [test]                                        lock_policy_c_tests.hpp
*
*   Declarations and shared helpers for the lock_policy_c.hpp unit-test suite.
*
*   WHAT THIS HEADER IS, AND WHAT THAT MEANS FOR TESTING:
*   lock_policy_c.hpp is the C-backed sibling of lock_policy.hpp: instead of
* std::mutex and friends, its policies wrap the platform's own primitives -
* pthreads on POSIX (pthread_mutex, a RECURSIVE pthread_mutex, pthread_rwlock)
* and CRITICAL_SECTION / SRWLOCK on Windows - plus a std::atomic_flag spinlock.
* So unlike the base header, which is pure compile-time vocabulary, THIS one has
* real run-time behaviour: mutexes that actually exclude, an rwlock that admits
* concurrent readers, a recursive mutex a thread may re-lock, and a spinlock
* that busy-waits.  The suite therefore has two halves: static checks on the
* policy vocabulary (aliases, descriptors, levels - as for the base header), and
* genuine CONCURRENCY tests built from std::thread with a rendezvous, since the
* exclusion, reader-concurrency and recursion claims can only be shown with more
* than one thread.
*
*   PORTABILITY OF THE TESTS THEMSELVES: the concrete primitive classes are
* named per platform (c_mutex_posix vs c_mutex_win32), but the POLICY structs
* (c_exclusive, c_shared, c_recursive, c_spinlock) exist on every platform and
* expose the platform's type through mutex_type.  So the suite drives almost
* everything through the policy typedefs - c_exclusive::mutex_type, and so on -
* which keeps it building on any platform; only a single identity test names the
* concrete POSIX/Win32 classes, and it is guarded accordingly.
*
*   THE SHARP EDGES THE SUITE PINS:
*     - the guards are NON-MOVABLE.  c_lock_guard and c_shared_guard delete
*       their copy operations and declare a destructor, so no implicit move is
*       generated - the same trap no_op_guard springs in the base header.
*     - the C mutex classes are NON-COPYABLE the OLD way: a private, undeclared
*       copy constructor and assignment (the C++98 idiom), not `= delete`.  The
*       effect is the same; the suite checks the effect.
*     - c_recursive is genuinely REENTRANT: a thread may lock (or even try_lock)
*       its mutex while already holding it, which would DEADLOCK a plain
*       c_exclusive.  This is the one behaviour that distinguishes the two
*       otherwise identical exclusive policies.
*     - the spinlock is NOT reentrant: try_lock on a spinlock you already hold
*       returns false (it does not deadlock, but it does not re-enter either).
*     - THREE distinct policies - c_exclusive, c_recursive and c_spinlock - all
*       report level `exclusive`.  There is no `recursive` or `spin` level, so
*       the level does not distinguish them; only their behaviour does.
*     - c_shared is the one policy whose read and write locks DIFFER: its reader
*       is a c_shared_guard (a shared lock) and its writer a c_lock_guard (an
*       exclusive lock), so readers may overlap while writers do not.
*
*   A BANNER-VS-CODE DISCREPANCY, recorded rather than judged: the file's
* versioning banner lists c_shared as arriving at "C++17: + c_shared on all
* platforms", but the CODE gates c_shared only by PLATFORM, not by standard - on
* POSIX it is defined for every C++ version (its pthread_rwlock backing is
* C++98-clean).  The suite verifies that c_shared is in fact present at the
* floor it runs on, C++11, which the banner would not lead you to expect.  (The
* spinlock, by contrast, really is C++11+, matching its banner line.)
*
*   PORTABILITY: the suite floors at C++11 - the spinlock, the concurrency
* section's std::thread, and this header's helpers all need it - matching the
* sibling suites.  In the suite, every mention of c_spinlock / spinlock_mutex
* is gated to C++11 (they do not exist below it), and the sole concrete-class
* identity test is guarded by platform.
*
*
* path:      /tests/djinterp/core/sync/lock_policy_c_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED TEST HELPERS
      -------------------
      a. timing + rendezvous utilities

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. I.   platform mutex classes
      c.    II.  the C-backed RAII guards
            d. III. the C-backed policy structs
            e. IV.  the spinlock (mutex + policy)
            f. concurrency (what the locks are for)
*/

#ifndef DJINTERP_TESTS_LOCK_POLICY_C_
#define DJINTERP_TESTS_LOCK_POLICY_C_ 1

// std
#include <atomic>
#include <chrono>
#include <thread>
#include <type_traits>
// djinterp
//   lock_policy_c.hpp is self-sufficient - it includes ../djinterp.hpp and
// ./lock_policy.hpp itself, and so can be the first djinterp header in a TU -
// but the suite includes the core first anyway, for uniformity with the sibling
// suites and so the helpers below can use the D_ macros.
#include "djinterp/djinterp.hpp"
#include "../../../../inc/djinterp/core/sync/lock_policy_c.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "lock_policy_c_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED TEST HELPERS                                  ///
///////////////////////////////////////////////////////////////////////////////

// test_clock
//   type: the suite's clock - steady, never the wall clock.
typedef std::chrono::steady_clock test_clock;

// k_deadline_ms
//   constant: how long a concurrency test waits for something that MUST happen
// before it calls the operation broken.  A liveness bound, not a performance
// one.
D_STATIC D_CONSTEXPR unsigned k_deadline_ms = 3000;

// k_agents
//   constant: how many threads a contention test starts.
D_STATIC D_CONSTEXPR int k_agents = 4;

// k_iterations
//   constant: how many locked increments each worker performs.
D_STATIC D_CONSTEXPR int k_iterations = 10000;

// sleep_ms
//   function: park this thread for _ms milliseconds.
D_INLINE void
sleep_ms(
    unsigned _ms
)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(_ms));

    return;
}

// elapsed_ms
//   function: milliseconds since _start.
D_INLINE long long
elapsed_ms(
    const test_clock::time_point& _start
)
{
    return static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            test_clock::now() - _start).count());
}

// spin_until
//   function: wait for a flag to go true, up to _ms.  Returns false if the
// deadline passed first.
D_INLINE bool
spin_until(
    const std::atomic<bool>& _flag,
    unsigned                 _ms
)
{
    const test_clock::time_point start = test_clock::now();

    while (!_flag.load())
    {
        // check if the deadline has passed
        if (elapsed_ms(start) >= static_cast<long long>(_ms))
        {
            return false;
        }

        sleep_ms(1);
    }

    return true;
}

// spin_until_count
//   function: as spin_until, for a counter reaching _target.
D_INLINE bool
spin_until_count(
    const std::atomic<int>& _counter,
    int                     _target,
    unsigned                _ms
)
{
    const test_clock::time_point start = test_clock::now();

    while (_counter.load() < _target)
    {
        // check if the deadline has passed
        if (elapsed_ms(start) >= static_cast<long long>(_ms))
        {
            return false;
        }

        sleep_ms(1);
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST DECLARATIONS                                    ///
///////////////////////////////////////////////////////////////////////////////

// a.   the header contract
//////////////////////////////////////////
bool tests_header_include_guard();
bool tests_header_is_idempotent();
bool tests_header_is_self_sufficient();
bool tests_header_pulls_in_the_base_lock_policy();
bool tests_header_provides_the_platform_threading_facilities();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_header_defines_no_stray_macros();
bool tests_the_spinlock_is_gated_to_cpp11();
bool tests_c_shared_is_available_at_this_standard();

// b.   I.   platform mutex classes
//////////////////////////////////////////
bool tests_exclusive_mutex_constructs_and_destructs();
bool tests_exclusive_mutex_lock_then_unlock();
bool tests_exclusive_mutex_try_lock_when_free();
bool tests_exclusive_mutex_is_not_copyable();
bool tests_exclusive_mutex_is_not_movable();
bool tests_recursive_mutex_allows_reentrant_locking();
bool tests_recursive_mutex_try_lock_is_reentrant();
bool tests_rwlock_shared_ops_are_callable();
bool tests_rwlock_try_lock_when_free();
bool tests_the_concrete_classes_back_the_policies();

// c.   II.  the C-backed RAII guards
//////////////////////////////////////////
bool tests_lock_guard_acquires_on_construction();
bool tests_lock_guard_releases_on_destruction();
bool tests_lock_guard_is_not_copyable();
bool tests_lock_guard_is_not_movable();
bool tests_shared_guard_acquires_shared_on_construction();
bool tests_shared_guard_releases_on_destruction();
bool tests_shared_guard_is_not_copyable();
bool tests_shared_guard_is_not_movable();
bool tests_the_guards_are_templates_over_any_c_mutex();

// d.   III. the C-backed policy structs
//////////////////////////////////////////
bool tests_c_exclusive_aliases();
bool tests_c_exclusive_read_and_write_are_identical();
bool tests_c_exclusive_descriptors();
bool tests_c_exclusive_level_is_exclusive();
bool tests_c_shared_aliases();
bool tests_c_shared_read_and_write_differ();
bool tests_c_shared_descriptors();
bool tests_c_shared_level_is_shared();
bool tests_c_recursive_aliases_and_descriptors();
bool tests_c_recursive_level_is_exclusive();
bool tests_three_policies_share_the_exclusive_level();

// e.   IV.  the spinlock (mutex + policy)
//////////////////////////////////////////
bool tests_spinlock_mutex_constructs();
bool tests_spinlock_mutex_lock_then_unlock();
bool tests_spinlock_mutex_try_lock_when_free();
bool tests_spinlock_mutex_try_lock_when_held_fails();
bool tests_spinlock_mutex_operations_are_noexcept();
bool tests_spinlock_mutex_is_not_copyable();
bool tests_spinlock_mutex_is_not_movable();
bool tests_c_spinlock_aliases();
bool tests_c_spinlock_descriptors_and_level();

// f.   concurrency (what the locks are for)
//////////////////////////////////////////
bool tests_c_exclusive_serializes_writers();
bool tests_c_shared_admits_concurrent_readers();
bool tests_c_shared_writer_excludes_readers();
bool tests_c_recursive_is_exclusive_across_threads();
bool tests_c_spinlock_serializes_writers();
bool tests_c_exclusive_try_lock_excludes_a_contending_thread();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_LOCK_POLICY_C_
