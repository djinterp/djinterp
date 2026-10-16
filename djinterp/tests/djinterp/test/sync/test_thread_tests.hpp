/*******************************************************************************
* djinterp [tests]                                         test_thread_tests.hpp
*
*   DTest suite header for inc/djinterp/test/sync/test_thread.hpp.
*
*   Two-faced per the DTest authoring guide:
*     - normal mode (section TUs): pulls in the header under test, exposes the
*       fixtures and the D_HT_CHECK macro used by the bodies.
*     - spec  mode  (runner):      pulls in test_defaults.hpp, exposes the
*       thread_spec() provider; no fixtures compiled.
*
*   The bool tests_*(); declarations are visible in BOTH modes: the section
* TUs define them, the provider references them by address.
*
*   Blocks (one section TU each):
*     thread    - test_thread class + test_thread_state enum
*     group     - test_thread_group
*     factory   - make_test_thread / make_named_test_thread
*     traits    - folded SFINAE trait surface (djinterp::test::traits)
*     concepts  - folded C++20 concept layer (gated on concepts support)
*
*
* path:      /tests/djinterp/test/sync/test_thread_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_THREAD_TESTS_
#define DJINTERP_TESTS_TEST_THREAD_TESTS_ 1

// std
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
// djinterp
// -- (part 1) mode-gated includes ------------------------------------------
// djinterp core is ALWAYS first and unconditional (NS_*, D_* qualifiers,
// language gates), so both faces have it.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/test/sync/test_thread.hpp"                        // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"        // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test).  Declared UNCONDITIONALLY,
// because the spec provider (spec mode) needs dt::module_spec too.
namespace dt = ::djinterp::test;


// D_HT_CHECK - evaluate once, print the failing expression + location on
// failure, and yield the condition.  Variadic so a top-level comma inside a
// trait/concept expression passes through whole.  Left OUTSIDE the fixture
// guard: it depends on nothing from the header under test.
inline bool
ht_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

#define D_HT_CHECK(...)                                                        \
    ::djinterp::testing::ht_check(                                            \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures - normal mode only

// ---------------------------------------------------------------------------
//  worker / probe fixtures for the thread + group + factory blocks
// ---------------------------------------------------------------------------

// ht_worker_ran
//   fixture: a thread-safe flag+id sink a worker can stamp so a test can prove
// the worker actually executed and received the right logical id.
struct ht_worker_ran
{
    std::atomic<bool>        ran;
    std::atomic<std::size_t> seen_id;
    std::atomic<int>         calls;

    ht_worker_ran()
        : ran(false)
        , seen_id(static_cast<std::size_t>(-1))
        , calls(0)
    {}
};

// ht_make_stamp
//   fixture: a worker that records that it ran and which id it was handed.
inline dt::test_thread::worker_fn
ht_make_stamp(ht_worker_ran& _sink)
{
    ht_worker_ran* p = &_sink;

    return dt::test_thread::worker_fn(
        [p](std::size_t _id)
        {
            p->seen_id.store(_id, std::memory_order_release);
            p->calls.fetch_add(1, std::memory_order_acq_rel);
            p->ran.store(true, std::memory_order_release);
        });
}

// ht_make_throwing
//   fixture: a worker that always throws a std::runtime_error carrying _msg.
inline dt::test_thread::worker_fn
ht_make_throwing(const char* _msg)
{
    std::string msg(_msg);

    return dt::test_thread::worker_fn(
        [msg](std::size_t /*id*/)
        {
            throw std::runtime_error(msg);
        });
}

// ht_make_sleeper
//   fixture: a worker that busy-idles for _ms milliseconds before returning,
// used to exercise timing and the try_join_for timeout path.
inline dt::test_thread::worker_fn
ht_make_sleeper(unsigned _ms)
{
    return dt::test_thread::worker_fn(
        [_ms](std::size_t /*id*/)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(_ms));
        });
}


// ---------------------------------------------------------------------------
//  synthetic trait/concept probe types (traits + concepts blocks)
// ---------------------------------------------------------------------------
//   Each carries exactly the marker(s) a probe keys on, so classification can
// be asserted axis by axis.  `plain_probe` carries none.

struct plain_probe        { int x; };

struct level_probe        { using thread_safety_level      = int; };
struct strategy_probe     { using concurrency_strategy_tag = int; };
struct lock_policy_probe  { using lock_policy_type         = int; };
struct mutex_probe        { using mutex_type               = int; };
struct cow_state_probe    { using cow_state_type           = int; };
struct rcu_probe          { using rcu_protected_type       = int; };
struct hazard_probe       { using hazard_domain_type       = int; };

// atomic_probe: value_type + the unindexed load/store/CAS interface the
// test-specific atomic probes key on.
struct atomic_probe
{
    using value_type = int;

    int  load() const                     { return 0; }
    void store(int)                       {}
    bool compare_exchange_weak(int&, int) { return true; }
};

// cow_iface_probe: snapshot() + publish() - the method-shaped COW lifecycle.
struct cow_iface_probe
{
    void snapshot() {}
    void publish()  {}
};

// try_lock_probe: bare try_lock().
struct try_lock_probe
{
    bool try_lock() { return true; }
};

// full_lock_probe: read_lock() const + write_lock() - the reader/writer pair.
struct full_lock_probe
{
    void read_lock()  const {}
    void write_lock()       {}
};

// rich_probe: a strategy tag AND the atomic interface, i.e. both a nested
// alias axis and a method axis - the shape fully_described_threadsafe_type
// asks for.
struct rich_probe
{
    using value_type               = int;
    using concurrency_strategy_tag = int;

    int  load() const                     { return 0; }
    void store(int)                       {}
    bool compare_exchange_weak(int&, int) { return true; }
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   TEST THREAD + STATE ENUM        (test_thread_tests_thread.cpp)
bool tests_thread_state_enum_values();
bool tests_thread_default_construct();
bool tests_thread_construct_id_name_worker();
bool tests_thread_construct_id_worker();
bool tests_thread_set_name();
bool tests_thread_set_worker_before_start();
bool tests_thread_start_runs_worker();
bool tests_thread_start_without_worker_noop();
bool tests_thread_start_twice_noop();
bool tests_thread_state_query_transitions();
bool tests_thread_join_idempotent();
bool tests_thread_worker_receives_id();
bool tests_thread_exception_capture();
bool tests_thread_exception_what();
bool tests_thread_exception_what_empty_on_success();
bool tests_thread_rethrow_if_failed();
bool tests_thread_rethrow_noop_on_success();
bool tests_thread_timing_monotonic();
bool tests_thread_elapsed_zero_when_idle();
bool tests_thread_detach_state();
bool tests_thread_try_join_for_success();
bool tests_thread_try_join_for_timeout();
bool tests_thread_move_construct();

// II.  THREAD GROUP                    (test_thread_tests_group.cpp)
bool tests_group_default_empty();
bool tests_group_emplace_id_name_worker();
bool tests_group_emplace_id_worker();
bool tests_group_emplace_auto_id();
bool tests_group_at_access();
bool tests_group_start_join_all();
bool tests_group_any_failed_false_on_success();
bool tests_group_any_failed_true_on_throw();
bool tests_group_all_completed_requires_nonempty();
bool tests_group_failure_count();
bool tests_group_first_exception_in_id_order();
bool tests_group_rethrow_first_failure();
bool tests_group_rethrow_noop_when_clean();
bool tests_group_clear();
bool tests_group_range_iteration();
bool tests_group_try_join_all_for();

// III. FACTORY HELPERS                 (test_thread_tests_factory.cpp)
bool tests_factory_make_test_thread();
bool tests_factory_make_named_test_thread();
bool tests_factory_make_and_run();
bool tests_factory_worker_forwarding();

// IV.  SFINAE TRAITS                   (test_thread_tests_traits.cpp)
bool tests_traits_negative_all_false();
bool tests_traits_has_thread_safety_level();
bool tests_traits_has_concurrency_strategy_tag();
bool tests_traits_has_lock_policy_type();
bool tests_traits_has_mutex_type();
bool tests_traits_has_cow_state_type();
bool tests_traits_has_rcu_protected_type();
bool tests_traits_has_hazard_domain_type();
bool tests_traits_has_try_lock_method();
bool tests_traits_has_atomic_load_store_cas();
bool tests_traits_has_atomic_interface();
bool tests_traits_has_publish_and_cow_interface();
bool tests_traits_full_lock_interface();
bool tests_traits_is_rcu_testable();
bool tests_traits_is_hazard_testable();
bool tests_traits_is_cow_testable();
bool tests_traits_is_locked_testable();
bool tests_traits_is_lock_free_testable();
bool tests_traits_is_threadsafe_testable();
bool tests_traits_thread_test_class_flags();

// V.   C++20 CONCEPTS                  (test_thread_tests_concepts.cpp)
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
bool tests_concepts_negative();
bool tests_concepts_safety_level_aware();
bool tests_concepts_strategy_tagged();
bool tests_concepts_lock_policy_aware();
bool tests_concepts_mutex_aware();
bool tests_concepts_cow_state_aware();
bool tests_concepts_rcu_protected_aware();
bool tests_concepts_hazard_domain_aware();
bool tests_concepts_try_lockable();
bool tests_concepts_atomic_capable();
bool tests_concepts_publishable_and_cow_capable();
bool tests_concepts_lock_free_testable();
bool tests_concepts_threadsafe_testable();
bool tests_concepts_classified_thread_test();
bool tests_concepts_race_testable();
bool tests_concepts_fully_described();
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
thread_spec()
{
    return dt::module_spec{
        "test_thread",
        "Exception-capturing test-thread wrapper: managed std::thread with "
        "captured exceptions, timing, identity, a thread group, factory "
        "helpers, and the folded threadsafe-testable trait/concept surface.",
        {
            dt::block_spec{
                "thread",
                "test_thread lifecycle, state machine, exception capture, "
                "timing, move, and try-join.",
                {
                    { "tests_thread_state_enum_values",
                      "test_thread_state enumerators are the five distinct "
                      "documented lifecycle values.",
                      &tests_thread_state_enum_values },
                    { "tests_thread_default_construct",
                      "default ctor yields id 0, empty name, idle, not "
                      "joinable, no captured exception.",
                      &tests_thread_default_construct },
                    { "tests_thread_construct_id_name_worker",
                      "(id,name,worker) ctor records id and name and starts "
                      "idle.",
                      &tests_thread_construct_id_name_worker },
                    { "tests_thread_construct_id_worker",
                      "(id,worker) ctor records id with an empty name.",
                      &tests_thread_construct_id_worker },
                    { "tests_thread_set_name",
                      "set_name replaces the human-readable name.",
                      &tests_thread_set_name },
                    { "tests_thread_set_worker_before_start",
                      "set_worker binds a worker that a later start runs.",
                      &tests_thread_set_worker_before_start },
                    { "tests_thread_start_runs_worker",
                      "start spawns the worker; join then reports completed.",
                      &tests_thread_start_runs_worker },
                    { "tests_thread_start_without_worker_noop",
                      "start with no bound worker is a no-op: stays idle, not "
                      "joinable.",
                      &tests_thread_start_without_worker_noop },
                    { "tests_thread_start_twice_noop",
                      "a second start once non-idle has no effect (single "
                      "spawn).",
                      &tests_thread_start_twice_noop },
                    { "tests_thread_state_query_transitions",
                      "is_idle/is_running/completed/failed track the state "
                      "across a run.",
                      &tests_thread_state_query_transitions },
                    { "tests_thread_join_idempotent",
                      "join is safe when not joinable and after completion.",
                      &tests_thread_join_idempotent },
                    { "tests_thread_worker_receives_id",
                      "the worker is invoked with the thread's logical id.",
                      &tests_thread_worker_receives_id },
                    { "tests_thread_exception_capture",
                      "a throwing worker leaves the thread failed with a "
                      "non-null exception_ptr.",
                      &tests_thread_exception_capture },
                    { "tests_thread_exception_what",
                      "exception_what surfaces the thrown message.",
                      &tests_thread_exception_what },
                    { "tests_thread_exception_what_empty_on_success",
                      "exception_what is empty after a clean run.",
                      &tests_thread_exception_what_empty_on_success },
                    { "tests_thread_rethrow_if_failed",
                      "rethrow_if_failed rethrows the captured exception.",
                      &tests_thread_rethrow_if_failed },
                    { "tests_thread_rethrow_noop_on_success",
                      "rethrow_if_failed is a no-op after a clean run.",
                      &tests_thread_rethrow_noop_on_success },
                    { "tests_thread_timing_monotonic",
                      "started_at <= stopped_at and elapsed is non-negative "
                      "after a run.",
                      &tests_thread_timing_monotonic },
                    { "tests_thread_elapsed_zero_when_idle",
                      "elapsed is zero for a never-started thread.",
                      &tests_thread_elapsed_zero_when_idle },
                    { "tests_thread_detach_state",
                      "detach moves the thread to the detached state and it is "
                      "no longer joinable.",
                      &tests_thread_detach_state },
                    { "tests_thread_try_join_for_success",
                      "try_join_for returns true when the worker finishes "
                      "inside the window.",
                      &tests_thread_try_join_for_success },
                    { "tests_thread_try_join_for_timeout",
                      "try_join_for returns false when the worker outlasts the "
                      "window; a following join still succeeds.",
                      &tests_thread_try_join_for_timeout },
                    { "tests_thread_move_construct",
                      "move transfers id/name/state and resets the source to "
                      "idle.",
                      &tests_thread_move_construct },
                }
            },
            dt::block_spec{
                "group",
                "test_thread_group membership, bulk lifecycle, aggregate "
                "verdict queries, and iteration.",
                {
                    { "tests_group_default_empty",
                      "a fresh group is empty with size 0.",
                      &tests_group_default_empty },
                    { "tests_group_emplace_id_name_worker",
                      "emplace(id,name,worker) appends and returns a usable "
                      "reference.",
                      &tests_group_emplace_id_name_worker },
                    { "tests_group_emplace_id_worker",
                      "emplace(id,worker) appends an anonymously-named thread.",
                      &tests_group_emplace_id_worker },
                    { "tests_group_emplace_auto_id",
                      "emplace(worker) assigns the next sequential id.",
                      &tests_group_emplace_auto_id },
                    { "tests_group_at_access",
                      "at() reaches members through both mutable and const "
                      "references.",
                      &tests_group_at_access },
                    { "tests_group_start_join_all",
                      "start_all/join_all run every member to completion.",
                      &tests_group_start_join_all },
                    { "tests_group_any_failed_false_on_success",
                      "any_failed is false when every member completed.",
                      &tests_group_any_failed_false_on_success },
                    { "tests_group_any_failed_true_on_throw",
                      "any_failed is true when a member threw.",
                      &tests_group_any_failed_true_on_throw },
                    { "tests_group_all_completed_requires_nonempty",
                      "all_completed is true only for a non-empty, all-clean "
                      "group.",
                      &tests_group_all_completed_requires_nonempty },
                    { "tests_group_failure_count",
                      "failure_count equals the number of throwing members.",
                      &tests_group_failure_count },
                    { "tests_group_first_exception_in_id_order",
                      "first_exception returns the earliest failure in member "
                      "order.",
                      &tests_group_first_exception_in_id_order },
                    { "tests_group_rethrow_first_failure",
                      "rethrow_first_failure rethrows the first captured "
                      "exception.",
                      &tests_group_rethrow_first_failure },
                    { "tests_group_rethrow_noop_when_clean",
                      "rethrow_first_failure is a no-op with no failures.",
                      &tests_group_rethrow_noop_when_clean },
                    { "tests_group_clear",
                      "clear joins members and empties the group.",
                      &tests_group_clear },
                    { "tests_group_range_iteration",
                      "range-for visits every member.",
                      &tests_group_range_iteration },
                    { "tests_group_try_join_all_for",
                      "try_join_all_for reports whether every member joined in "
                      "the window.",
                      &tests_group_try_join_all_for },
                }
            },
            dt::block_spec{
                "factory",
                "make_test_thread / make_named_test_thread convenience "
                "constructors.",
                {
                    { "tests_factory_make_test_thread",
                      "make_test_thread binds a worker with the given id and an "
                      "empty name.",
                      &tests_factory_make_test_thread },
                    { "tests_factory_make_named_test_thread",
                      "make_named_test_thread binds a worker with id and name.",
                      &tests_factory_make_named_test_thread },
                    { "tests_factory_make_and_run",
                      "a factory-built thread starts and runs its worker.",
                      &tests_factory_make_and_run },
                    { "tests_factory_worker_forwarding",
                      "the callable is forwarded so its captured state is "
                      "observed on run.",
                      &tests_factory_worker_forwarding },
                }
            },
            dt::block_spec{
                "traits",
                "folded threadsafe-testable SFINAE traits: presence probes, "
                "method probes, and strategy composites.",
                {
                    { "tests_traits_negative_all_false",
                      "a marker-free type trips no probe and is not "
                      "threadsafe-testable.",
                      &tests_traits_negative_all_false },
                    { "tests_traits_has_thread_safety_level",
                      "has_thread_safety_level detects the nested "
                      "thread_safety_level alias.",
                      &tests_traits_has_thread_safety_level },
                    { "tests_traits_has_concurrency_strategy_tag",
                      "has_concurrency_strategy_tag detects the nested "
                      "concurrency_strategy_tag alias.",
                      &tests_traits_has_concurrency_strategy_tag },
                    { "tests_traits_has_lock_policy_type",
                      "has_lock_policy_type detects the nested lock_policy_type "
                      "alias.",
                      &tests_traits_has_lock_policy_type },
                    { "tests_traits_has_mutex_type",
                      "has_mutex_type (renamed re-export) detects the nested "
                      "mutex_type alias.",
                      &tests_traits_has_mutex_type },
                    { "tests_traits_has_cow_state_type",
                      "has_cow_state_type detects the nested cow_state_type "
                      "alias.",
                      &tests_traits_has_cow_state_type },
                    { "tests_traits_has_rcu_protected_type",
                      "has_rcu_protected_type detects the nested "
                      "rcu_protected_type alias.",
                      &tests_traits_has_rcu_protected_type },
                    { "tests_traits_has_hazard_domain_type",
                      "has_hazard_domain_type detects the nested "
                      "hazard_domain_type alias.",
                      &tests_traits_has_hazard_domain_type },
                    { "tests_traits_has_try_lock_method",
                      "has_try_lock_method detects a callable try_lock().",
                      &tests_traits_has_try_lock_method },
                    { "tests_traits_has_atomic_load_store_cas",
                      "the load/store/compare_exchange method probes each fire "
                      "on the scalar atomic interface.",
                      &tests_traits_has_atomic_load_store_cas },
                    { "tests_traits_has_atomic_interface",
                      "has_atomic_interface is the load+store composite.",
                      &tests_traits_has_atomic_interface },
                    { "tests_traits_has_publish_and_cow_interface",
                      "has_publish_method and the snapshot+publish COW "
                      "composite fire on the method-shaped COW type.",
                      &tests_traits_has_publish_and_cow_interface },
                    { "tests_traits_full_lock_interface",
                      "has_full_lock_interface is the read_lock+write_lock "
                      "composite.",
                      &tests_traits_full_lock_interface },
                    { "tests_traits_is_rcu_testable",
                      "is_rcu_testable follows the rcu_protected_type marker.",
                      &tests_traits_is_rcu_testable },
                    { "tests_traits_is_hazard_testable",
                      "is_hazard_testable follows the hazard_domain_type "
                      "marker.",
                      &tests_traits_is_hazard_testable },
                    { "tests_traits_is_cow_testable",
                      "is_cow_testable is satisfied by the cow_state_type "
                      "marker.",
                      &tests_traits_is_cow_testable },
                    { "tests_traits_is_locked_testable",
                      "is_locked_testable is satisfied by the lock_policy_type "
                      "marker.",
                      &tests_traits_is_locked_testable },
                    { "tests_traits_is_lock_free_testable",
                      "is_lock_free_testable holds for the atomic interface and "
                      "for RCU/hazard markers.",
                      &tests_traits_is_lock_free_testable },
                    { "tests_traits_is_threadsafe_testable",
                      "is_threadsafe_testable holds for any recognised marker "
                      "and is false for none.",
                      &tests_traits_is_threadsafe_testable },
                    { "tests_traits_thread_test_class_flags",
                      "thread_test_class exposes per-axis flags consistent with "
                      "the individual probes.",
                      &tests_traits_thread_test_class_flags },
                }
            },
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
            dt::block_spec{
                "concepts",
                "folded C++20 concept layer mirroring the trait surface.",
                {
                    { "tests_concepts_negative",
                      "a marker-free type satisfies none of the concepts.",
                      &tests_concepts_negative },
                    { "tests_concepts_safety_level_aware",
                      "safety_level_aware_type holds for a thread_safety_level "
                      "alias.",
                      &tests_concepts_safety_level_aware },
                    { "tests_concepts_strategy_tagged",
                      "strategy_tagged_type holds for a concurrency_strategy_tag "
                      "alias.",
                      &tests_concepts_strategy_tagged },
                    { "tests_concepts_lock_policy_aware",
                      "lock_policy_aware_type holds for a lock_policy_type "
                      "alias.",
                      &tests_concepts_lock_policy_aware },
                    { "tests_concepts_mutex_aware",
                      "mutex_aware_type holds for a mutex_type alias.",
                      &tests_concepts_mutex_aware },
                    { "tests_concepts_cow_state_aware",
                      "cow_state_aware_type holds for a cow_state_type alias.",
                      &tests_concepts_cow_state_aware },
                    { "tests_concepts_rcu_protected_aware",
                      "rcu_protected_aware_type holds for an rcu_protected_type "
                      "alias.",
                      &tests_concepts_rcu_protected_aware },
                    { "tests_concepts_hazard_domain_aware",
                      "hazard_domain_aware_type holds for a hazard_domain_type "
                      "alias.",
                      &tests_concepts_hazard_domain_aware },
                    { "tests_concepts_try_lockable",
                      "try_lockable_type holds for a try_lock() method.",
                      &tests_concepts_try_lockable },
                    { "tests_concepts_atomic_capable",
                      "atomic_capable_type holds for the scalar atomic "
                      "interface.",
                      &tests_concepts_atomic_capable },
                    { "tests_concepts_publishable_and_cow_capable",
                      "publishable_type and cow_capable_type hold for the "
                      "snapshot+publish type.",
                      &tests_concepts_publishable_and_cow_capable },
                    { "tests_concepts_lock_free_testable",
                      "lock_free_testable_type holds for the atomic interface.",
                      &tests_concepts_lock_free_testable },
                    { "tests_concepts_threadsafe_testable",
                      "threadsafe_testable_type holds for any recognised "
                      "marker.",
                      &tests_concepts_threadsafe_testable },
                    { "tests_concepts_classified_thread_test",
                      "classified_thread_test_type follows thread_test_class's "
                      "is_threadsafe flag.",
                      &tests_concepts_classified_thread_test },
                    { "tests_concepts_race_testable",
                      "race_testable_type holds for a reader/writer, snapshot, "
                      "or atomic-load type.",
                      &tests_concepts_race_testable },
                    { "tests_concepts_fully_described",
                      "fully_described_threadsafe_type holds for a type with "
                      "both an alias axis and a method axis.",
                      &tests_concepts_fully_described },
                }
            },
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_THREAD_TESTS_
