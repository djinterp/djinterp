/*******************************************************************************
* djinterp [test]                                           lock_guard_tests.hpp
*
*   Declarations for the lock_guard.hpp unit-test suite, plus the helper types
* the sections share.
*
*   WHAT THESE GUARDS ARE, AND WHAT THAT MEANS FOR TESTING:
*   lock_guard.hpp is the RAII layer over lock_policy.hpp: a handful of guards
* that acquire a policy's lock on construction and release it on destruction,
* so that call sites never name a concrete mutex or lock type.  Each guard is a
* thin wrapper whose whole contract is "hold the right lock for exactly this
* scope", so the suite tests two things about each: that it acquires the lock it
* claims to (observable by trying to take the underlying mutex and failing), and
* that it gives the lock back when it leaves scope (observable by then taking
* the mutex and succeeding).  The genuinely concurrent claims - mutual exclusion
* across threads, a timed lock that actually times out under contention, shared
* readers that overlap - get their own section, built from std::thread with a
* rendezvous so the threads truly race rather than lining up by luck.
*
*   THE SHARP EDGES THE SUITE PINS:
*     - scoped_read_lock and scoped_write_lock declare `= default` move members,
*       but for null_lock_policy those moves are IMPLICITLY DELETED: the guard
*       holds a no_op_guard, and no_op_guard is non-movable (deleted copy plus a
*       user-declared destructor).  So a guard that moves fine for every real
*       policy will not compile if moved under the null policy - a portability
*       trap for generic code (tests_read_lock_is_not_movable_for_the_null_
*       policy, and its write twin).
*     - scoped_try_lock and scoped_timed_lock are move-CONSTRUCTIBLE but not
*       move-ASSIGNABLE: they declare a move constructor and no move assignment,
*       and the deleted copy suppresses the implicit one.  A move on
*       construction transfers ownership; the moved-from guard does NOT unlock
*       (tests_try_move_transfers_ownership).
*     - the null policy provides NO real exclusion: no_op_mutex::try_lock()
*       always returns true, so scoped_try_lock<null_lock_policy> ALWAYS owns
* the
*       lock, even a second one over the same mutex.  Single-threaded null-mode
*       code proceeds unconditionally (tests_try_under_the_null_policy_always_
*       succeeds).
*     - upgrade_lock is NOT atomic: it unlocks the read lock and then takes the
*       write lock, and there is a window in between where NO lock is held.  The
*       header says so; the suite pins that after the (non-atomic) upgrade the
*       write lock really is held exclusively (tests_upgrade_holds_the_write_
*       lock_exclusively).
*     - release() (on try and timed guards) is idempotent: calling it twice does
*       not double-unlock, and a released guard's destructor unlocks nothing
*       (tests_try_release_is_idempotent).
*
*   A NON-BUG WORTH RECORDING: lock_guard.hpp's own "include env.h first"
* precondition guards are COMMENTED OUT, exactly as cow.hpp's are.  Unlike
* cow.hpp, this is harmless here, because lock_guard.hpp includes
* ../djinterp.hpp
* BEFORE ./lock_policy.hpp - so the environment is always in scope by the time
* lock_policy.hpp's own LIVE #error is reached.  The header is therefore
* self-sufficient and CAN be the first djinterp header in a TU
* (tests_header_is_self_sufficient).
*
*   PORTABILITY: the suite floors at C++11 (its concurrency section needs
* std::thread), matching the sibling suites.  lock_guard.hpp itself reaches
* lower - the blocking guards work in C++98 with null_lock_policy - but that
* path is out of this harness's scope.  Within the suite, scoped_timed_lock is
* exercised from C++11, and upgrade_lock and the shared_lock_policy paths are
* gated to C++17 (shared_timed to C++14), because that is where the standard
* library types they need first appear.
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.14
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      -------
      a. deadlines and spin-waits (concurrency section)

II.   TEST DECLARATIONS
      -----------------
      a. the header contract
      b. I.   scoped_read_lock
      c.    II.  scoped_write_lock
            d. III. scoped_try_lock
            e. IV.  scoped_timed_lock
            f. V.   upgrade_lock
            g. concurrency
*/

#ifndef DJINTERP_TESTS_LOCK_GUARD_
#define DJINTERP_TESTS_LOCK_GUARD_ 1

// std
// std  (NOT <mutex>, <shared_mutex> or <chrono>: lock_guard.hpp's own include
//       chain supplies those, and tests_header_provides_the_std_facilities is
//       only an honest test of that from a TU that could not have gotten them
//       elsewhere)
#include <atomic>
#include <cstddef>
#include <thread>
#include <type_traits>
#include <utility>
// djinterp
//   lock_guard.hpp CAN be included first - it pulls ../djinterp.hpp before
// ./lock_policy.hpp - but the suite still includes the core explicitly, both to
// match the sibling suites and so that removing lock_guard.hpp's core include
// would surface here rather than silently.  tests_header_is_self_sufficient
// records that the header does not actually need this line.
#include "djinterp/djinterp.hpp"
#include "../../../../inc/djinterp/core/sync/lock_guard.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "lock_guard_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                             ///
///////////////////////////////////////////////////////////////////////////////

// a.   deadlines and spin-waits (concurrency section)
//////////////////////////////////////////
//   The concurrency section runs real threads.  A test that must observe two
// threads overlapping needs a way to wait for that overlap that becomes a
// FAILED test rather than a hung runner if it never happens - hence the
// deadline on every wait.  The waits sleep between polls, because a hot spin on
// a single-core box can starve the very thread being waited on.

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
//   constant: how many locked increments each writer performs.
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
bool tests_header_pulls_its_dependencies();
bool tests_header_provides_the_std_facilities();
bool tests_header_precondition_guards_are_commented_out();
bool tests_entities_live_in_the_djinterp_namespace();
bool tests_header_defines_no_stray_macros();
bool tests_the_guards_are_gated_by_standard();

// b.   I.  scoped_read_lock
//////////////////////////////////////////
bool tests_read_exposes_its_policy_aliases();
bool tests_read_lock_acquires_on_construction();
bool tests_read_lock_releases_on_destruction();
bool tests_read_lock_is_not_copyable();
bool tests_read_lock_is_movable_for_a_real_policy();
bool tests_read_lock_is_not_movable_for_the_null_policy();
bool tests_read_lock_under_the_null_policy_is_a_no_op();
bool tests_read_lock_is_shared_under_the_shared_policy();

// c.   II. scoped_write_lock
//////////////////////////////////////////
bool tests_write_exposes_its_policy_aliases();
bool tests_write_lock_acquires_exclusively();
bool tests_write_lock_releases_on_destruction();
bool tests_write_lock_is_not_copyable();
bool tests_write_lock_is_movable_for_a_real_policy();
bool tests_write_lock_is_not_movable_for_the_null_policy();
bool tests_write_lock_is_exclusive_even_under_the_shared_policy();
bool tests_write_lock_under_the_null_policy_is_a_no_op();

// d.   III. scoped_try_lock
//////////////////////////////////////////
bool tests_try_exposes_its_mutex_alias();
bool tests_try_acquires_a_free_mutex();
bool tests_try_fails_on_a_held_mutex();
bool tests_try_releases_on_destruction();
bool tests_try_release_unlocks_early();
bool tests_try_release_is_idempotent();
bool tests_try_is_not_copyable();
bool tests_try_is_move_constructible_only();
bool tests_try_move_transfers_ownership();
bool tests_try_under_the_null_policy_always_succeeds();

// e.   IV. scoped_timed_lock
//////////////////////////////////////////
bool tests_timed_exposes_its_mutex_alias();
bool tests_timed_acquires_a_free_mutex();
bool tests_timed_releases_on_destruction();
bool tests_timed_release_unlocks_early();
bool tests_timed_release_is_idempotent();
bool tests_timed_is_not_copyable();
bool tests_timed_is_move_constructible_only();
bool tests_timed_move_transfers_ownership();

// f.   V.  upgrade_lock
//////////////////////////////////////////
bool tests_upgrade_exposes_its_aliases();
bool tests_upgrade_acquires_the_write_lock();
bool tests_upgrade_empties_the_source_read_lock();
bool tests_upgrade_downgrade_releases_the_write_lock();
bool tests_upgrade_releases_on_destruction();
bool tests_upgrade_is_not_copyable();
bool tests_upgrade_is_move_constructible();

// g.   concurrency
//////////////////////////////////////////
bool tests_write_lock_serializes_writers();
bool tests_read_lock_admits_concurrent_readers();
bool tests_try_lock_excludes_a_contending_thread();
bool tests_timed_lock_times_out_under_contention();
bool tests_upgrade_holds_the_write_lock_exclusively();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_LOCK_GUARD_
