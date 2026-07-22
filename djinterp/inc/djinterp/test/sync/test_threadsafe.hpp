/******************************************************************************
* djinterp [test]                                          test_threadsafe.hpp
*
* Umbrella header for the DTest multithreading and threadsafe-behavior
* testing suite.  Includes every submodule of the multithreaded test
* harness in a single import:
*
*   test_thread.hpp           - exception-capturing thread wrapper
*                               (test_thread, test_thread_group);
*                               ALSO carries the threadsafe-testable
*                               SFINAE traits and C++20 concepts
*                               (formerly test_thread_traits.hpp /
*                                test_thread_concepts.hpp, folded in
*                                2026.07) in namespace
*                                djinterp::test::traits
*
*   test_sync.hpp             - coordination primitives for tests
*                               (test_latch, test_barrier, test_gate,
*                                test_rendezvous, simultaneous_start)
*
*   test_concurrent.hpp       - N-way concurrent execution drivers
*                               (concurrent_runner, run_simultaneous,
*                                run_reader_writer, pipeline_runner)
*
*   test_stress.hpp           - high-pressure repeated-operation drivers
*                               (stress_runner, timed_stress, chaos_runner,
*                                stress_op for weighted op-mix)
*
*   test_race.hpp             - fine-grained race observation
*                               (race_probe, atomicity_observer,
*                                linearization_log, consistency_check)
*
*   test_invariant.hpp        - concurrent invariant monitoring
*                               (invariant_monitor, monotonic_guard,
*                                bounded_guard, invariant_scope)
*
*   test_deadlock.hpp         - deadlock & timeout detection
*                               (deadlock_watchdog, lock_order_tracker,
*                                scoped_deadlock_watchdog)
*
*   test_thread_traits.hpp    - RETIRED: folded into test_thread.hpp.
*   test_thread_concepts.hpp    Both are gone as standalone headers;
*                               their contents (the SFINAE trait
*                               surface and the C++20 concept layer)
*                               now live in test_thread.hpp above.
*                               Include test_thread.hpp for them.
*
*   COMPLEMENTS:
*   This suite complements the threadsafe foundation module
* (../../core/sync/threadsafe.hpp) which provides the production-side
* primitives (lock_policy, atomic, condvar, cow, hazard_pointer, rcu).
* The test suite consumes those primitives to exercise threadsafe
* code under controlled concurrent load.
*
*   USAGE PATTERN:
*   Each module produces a *_report struct convertible to a basic_test
* via to_test_object(test_type_id, name).  Test cases adopt the
* resulting basic_test into their suite tree, integrating multithreaded
* assertions seamlessly with the rest of DTest.
*
*   Users who need only a subset can include the individual submodule
* headers directly.  Existing code that includes this umbrella header
* continues to work unchanged.
*
*   PORTABILITY:
*   All submodules require C++11 minimum (uses <thread>, <mutex>,
* <chrono>, <atomic>, <condition_variable>).  C++98 fallbacks are
* provided where feasible - typically as single-threaded degenerate
* stubs that always succeed.
*
*
* path:      /inc/djinterp/test/sync/test_threadsafe.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_THREADSAFE_
#define DJINTERP_TEST_THREADSAFE_ 1

// --- threadsafe foundation (production-side primitives) ---
//
//   Pulled in transitively so that callers who include this
// umbrella to test their threadsafe code do not also need to
// include <djinterp/core/sync/threadsafe.hpp> separately.  The
// individual test submodules also include the specific sync
// headers they consume.
#include "../../core/sync/threadsafe.hpp"

// --- DTest multithreading harness submodules ---

// foundational thread wrapper and group
#include "./test_thread.hpp"

// synchronization primitives
#include "./test_sync.hpp"

// concurrent execution drivers
#include "./test_concurrent.hpp"

// stress and chaos drivers
#include "./test_stress.hpp"

// race observation and linearization
#include "./test_race.hpp"

// invariant monitoring
#include "./test_invariant.hpp"

// deadlock and timeout detection
#include "./test_deadlock.hpp"

// NOTE: the SFINAE traits (formerly ./test_thread_traits.hpp) and the
// C++20 concepts (formerly ./test_thread_concepts.hpp) are now folded
// into ./test_thread.hpp (included above), so there is nothing to
// include here.  The standalone headers have been removed.


#endif  // DJINTERP_TEST_THREADSAFE_
