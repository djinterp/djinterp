/*******************************************************************************
* djinterp [tests]                                      test_invariant_tests.hpp
*
*   DTest suite header for inc/djinterp/test/sync/test_invariant.hpp - the
* invariant monitoring toolkit (a polling background monitor, monotonic and
* bounded value guards, and an RAII monitoring scope).
*
*   Two-faced per the DTest authoring guide (DTEST_SPEC_MODE).  Blocks map one
* to one onto section TUs:
*     report     - invariant_violation + invariant_report
*     monitor    - invariant_monitor lifecycle and polling
*     monotonic  - monotonic_guard<T>
*     bounded    - bounded_guard<T>
*     scope      - invariant_scope (RAII)
*     factory    - make_invariant_monitor
*
*   ------------------------------------------------------------------------
*   NOTE: this module's factory is NOT gated
*   ------------------------------------------------------------------------
*   invariant_monitor also deletes its copy constructor without declaring a
* move constructor, but make_invariant_monitor() returns a PRVALUE
* (`return invariant_monitor(...)`) rather than a named local, so C++17's
* guaranteed copy elision applies and no constructor is required.  That is
* why this factory compiles while make_concurrent_runner / make_stress /
* make_timed_stress / make_race_probe (all `return named_local;`) do not.
*
*   ------------------------------------------------------------------------
*   A NOTE ON THE POLLING MONITOR
*   ------------------------------------------------------------------------
*   The background monitor polls on a cadence, so the NUMBER of checks it
* performs in a given window is inherently non-deterministic.  Tests here
* never assert an exact background poll count.  They assert what is
* contractual: check_now() is synchronous and exact, a failing predicate is
* recorded, the recorded-violation cap is honoured, lifecycle transitions are
* correct, and start() resets prior state.
*
*
* path:      /tests/djinterp/test/sync/test_invariant_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_INVARIANT_TESTS_
#define DJINTERP_TESTS_TEST_INVARIANT_TESTS_ 1

// std
#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
// djinterp
// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/test/sync/test_invariant.hpp"                     // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"        // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_HI_CHECK - unique two-letter suffix (hI = invariant).
inline bool
hi_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

#define D_HI_CHECK(...)                                                        \
    ::djinterp::testing::hi_check(                                            \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// hi_type_id
//   fixture: a stable test_type_id for to_test_object probes.
inline dt::test_type_id hi_type_id() { return static_cast<dt::test_type_id>(4); }

// hi_flag
//   fixture: a predicate-backing flag a test can flip to make an invariant
// start failing at a chosen moment.
struct hi_flag
{
    std::atomic<bool> ok;

    hi_flag()
        : ok(true)
    {}
};

// hi_predicate
//   fixture: a predicate reading the shared flag.
inline dt::invariant_monitor::predicate_fn
hi_predicate(hi_flag& _flag)
{
    hi_flag* p = &_flag;

    return dt::invariant_monitor::predicate_fn(
        [p]()
        {
            return p->ok.load(std::memory_order_acquire);
        });
}

// hi_settle
//   fixture: long enough for a 1ms-cadence monitor thread to have polled at
// least once.  Used only where the test asserts "at least one" - never an
// exact count.
inline void hi_settle()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   INVARIANT REPORT                (test_invariant_tests_report.cpp)
bool tests_ireport_violation_defaults();
bool tests_ireport_violation_value_construct();
bool tests_ireport_defaults();
bool tests_ireport_success_predicate();
bool tests_ireport_to_test_object_pass();
bool tests_ireport_to_test_object_fail();
bool tests_ireport_to_test_object_custom_name();

// II.  INVARIANT MONITOR               (test_invariant_tests_monitor.cpp)
bool tests_imonitor_default_construct();
bool tests_imonitor_predicate_construct();
bool tests_imonitor_check_now_without_predicate();
bool tests_imonitor_check_now_true();
bool tests_imonitor_check_now_false();
bool tests_imonitor_check_now_accumulates();
bool tests_imonitor_check_now_throwing_predicate();
bool tests_imonitor_detail_provider();
bool tests_imonitor_detail_provider_throws();
bool tests_imonitor_max_violations_recorded();
bool tests_imonitor_start_stop_running();
bool tests_imonitor_start_without_predicate();
bool tests_imonitor_start_twice();
bool tests_imonitor_stop_idempotent();
bool tests_imonitor_start_resets_report();
bool tests_imonitor_background_polls();
bool tests_imonitor_background_detects_violation();
bool tests_imonitor_success_and_violation_count();

// III. MONOTONIC GUARD                 (test_invariant_tests_monotonic.cpp)
bool tests_imonotonic_default_construct();
bool tests_imonotonic_initial_construct();
bool tests_imonotonic_observe_increasing();
bool tests_imonotonic_observe_equal_allowed();
bool tests_imonotonic_observe_decreasing();
bool tests_imonotonic_max_seen_tracks();
bool tests_imonotonic_max_unchanged_by_violation();
bool tests_imonotonic_observation_count();
bool tests_imonotonic_success_predicate();
bool tests_imonotonic_concurrent_observers_mt();

// IV.  BOUNDED GUARD                   (test_invariant_tests_bounded.cpp)
bool tests_ibounded_construct_range();
bool tests_ibounded_observe_inside();
bool tests_ibounded_observe_at_bounds();
bool tests_ibounded_observe_below();
bool tests_ibounded_observe_above();
bool tests_ibounded_current_tracks();
bool tests_ibounded_observation_count();
bool tests_ibounded_success_predicate();
bool tests_ibounded_concurrent_observers_mt();

// V.   INVARIANT SCOPE                 (test_invariant_tests_scope.cpp)
bool tests_iscope_reports_on_exit();
bool tests_iscope_interval_construct();
bool tests_iscope_monitor_access();
bool tests_iscope_check_now_through_monitor();
bool tests_iscope_detects_violation();

// VI.  FACTORY HELPERS                 (test_invariant_tests_factory.cpp)
bool tests_ifactory_make_invariant_monitor();
bool tests_ifactory_make_and_check();
bool tests_ifactory_make_and_run_background();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
invariant_spec()
{
    return dt::module_spec{
        "test_invariant",
        "Invariant monitoring toolkit: a violation/report model, a polling "
        "background monitor, monotonic and bounded value guards, an RAII "
        "monitoring scope, and the monitor factory.",
        {
            dt::block_spec{
                "report",
                "invariant_violation and invariant_report aggregation and "
                "lowering.",
                {
                    { "tests_ireport_violation_defaults",
                      "a default violation has an epoch stamp and empty detail.",
                      &tests_ireport_violation_defaults },
                    { "tests_ireport_violation_value_construct",
                      "the value ctor records the observation time and detail.",
                      &tests_ireport_violation_value_construct },
                    { "tests_ireport_defaults",
                      "a default report has no checks, no violations, and "
                      "passes.",
                      &tests_ireport_defaults },
                    { "tests_ireport_success_predicate",
                      "success() is false as soon as the violation count is "
                      "non-zero.",
                      &tests_ireport_success_predicate },
                    { "tests_ireport_to_test_object_pass",
                      "a clean report lowers to a passing test object.",
                      &tests_ireport_to_test_object_pass },
                    { "tests_ireport_to_test_object_fail",
                      "a violated report lowers to a failing test object with a "
                      "distinct message.",
                      &tests_ireport_to_test_object_fail },
                    { "tests_ireport_to_test_object_custom_name",
                      "the supplied name reaches the object's metadata.",
                      &tests_ireport_to_test_object_custom_name },
                }
            },
            dt::block_spec{
                "monitor",
                "invariant_monitor synchronous checking, violation recording, "
                "and background lifecycle.",
                {
                    { "tests_imonitor_default_construct",
                      "a default monitor is not running and has an empty "
                      "report.",
                      &tests_imonitor_default_construct },
                    { "tests_imonitor_predicate_construct",
                      "the predicate ctor binds a predicate usable immediately.",
                      &tests_imonitor_predicate_construct },
                    { "tests_imonitor_check_now_without_predicate",
                      "check_now with no predicate reports success and records "
                      "nothing.",
                      &tests_imonitor_check_now_without_predicate },
                    { "tests_imonitor_check_now_true",
                      "a holding predicate records a check and no violation.",
                      &tests_imonitor_check_now_true },
                    { "tests_imonitor_check_now_false",
                      "a failing predicate records a check and a violation.",
                      &tests_imonitor_check_now_false },
                    { "tests_imonitor_check_now_accumulates",
                      "repeated checks accumulate exactly in the report.",
                      &tests_imonitor_check_now_accumulates },
                    { "tests_imonitor_check_now_throwing_predicate",
                      "a predicate that throws is treated as a violation.",
                      &tests_imonitor_check_now_throwing_predicate },
                    { "tests_imonitor_detail_provider",
                      "the detail provider's string is attached to a recorded "
                      "violation.",
                      &tests_imonitor_detail_provider },
                    { "tests_imonitor_detail_provider_throws",
                      "a throwing detail provider leaves the violation recorded "
                      "with empty detail.",
                      &tests_imonitor_detail_provider_throws },
                    { "tests_imonitor_max_violations_recorded",
                      "the recorded-violation cap bounds stored violations but "
                      "not the count.",
                      &tests_imonitor_max_violations_recorded },
                    { "tests_imonitor_start_stop_running",
                      "start and stop drive the running flag.",
                      &tests_imonitor_start_stop_running },
                    { "tests_imonitor_start_without_predicate",
                      "start with no predicate is a no-op.",
                      &tests_imonitor_start_without_predicate },
                    { "tests_imonitor_start_twice",
                      "a second start while running has no effect.",
                      &tests_imonitor_start_twice },
                    { "tests_imonitor_stop_idempotent",
                      "stop is safe when the monitor was never started.",
                      &tests_imonitor_stop_idempotent },
                    { "tests_imonitor_start_resets_report",
                      "start clears the report accumulated before it.",
                      &tests_imonitor_start_resets_report },
                    { "tests_imonitor_background_polls",
                      "a running monitor performs checks on its own thread.",
                      &tests_imonitor_background_polls },
                    { "tests_imonitor_background_detects_violation",
                      "the background monitor records a violation once the "
                      "predicate starts failing.",
                      &tests_imonitor_background_detects_violation },
                    { "tests_imonitor_success_and_violation_count",
                      "the convenience accessors agree with the report.",
                      &tests_imonitor_success_and_violation_count },
                }
            },
            dt::block_spec{
                "monotonic",
                "monotonic_guard non-decreasing value enforcement.",
                {
                    { "tests_imonotonic_default_construct",
                      "a default guard starts with no observations or "
                      "violations.",
                      &tests_imonotonic_default_construct },
                    { "tests_imonotonic_initial_construct",
                      "the seeded ctor establishes the starting maximum.",
                      &tests_imonotonic_initial_construct },
                    { "tests_imonotonic_observe_increasing",
                      "a strictly increasing sequence is accepted throughout.",
                      &tests_imonotonic_observe_increasing },
                    { "tests_imonotonic_observe_equal_allowed",
                      "repeating the current maximum is allowed (non-strict "
                      "monotonicity).",
                      &tests_imonotonic_observe_equal_allowed },
                    { "tests_imonotonic_observe_decreasing",
                      "a value below the running maximum is rejected and "
                      "counted.",
                      &tests_imonotonic_observe_decreasing },
                    { "tests_imonotonic_max_seen_tracks",
                      "max_seen follows the largest accepted observation.",
                      &tests_imonotonic_max_seen_tracks },
                    { "tests_imonotonic_max_unchanged_by_violation",
                      "a rejected observation does not move the maximum.",
                      &tests_imonotonic_max_unchanged_by_violation },
                    { "tests_imonotonic_observation_count",
                      "every observation is counted, accepted or not.",
                      &tests_imonotonic_observation_count },
                    { "tests_imonotonic_success_predicate",
                      "success() is false once any violation is recorded.",
                      &tests_imonotonic_success_predicate },
                    { "tests_imonotonic_concurrent_observers_mt",
                      "a monotonically increasing multithreaded feed produces "
                      "no violations.",
                      &tests_imonotonic_concurrent_observers_mt },
                }
            },
            dt::block_spec{
                "bounded",
                "bounded_guard range enforcement.",
                {
                    { "tests_ibounded_construct_range",
                      "the range ctor records the low and high bounds.",
                      &tests_ibounded_construct_range },
                    { "tests_ibounded_observe_inside",
                      "a value strictly inside the range is accepted.",
                      &tests_ibounded_observe_inside },
                    { "tests_ibounded_observe_at_bounds",
                      "both bounds are inclusive.",
                      &tests_ibounded_observe_at_bounds },
                    { "tests_ibounded_observe_below",
                      "a value under the low bound is rejected and counted.",
                      &tests_ibounded_observe_below },
                    { "tests_ibounded_observe_above",
                      "a value over the high bound is rejected and counted.",
                      &tests_ibounded_observe_above },
                    { "tests_ibounded_current_tracks",
                      "current() reports the most recent observation.",
                      &tests_ibounded_current_tracks },
                    { "tests_ibounded_observation_count",
                      "every observation is counted, in range or not.",
                      &tests_ibounded_observation_count },
                    { "tests_ibounded_success_predicate",
                      "success() is false once any violation is recorded.",
                      &tests_ibounded_success_predicate },
                    { "tests_ibounded_concurrent_observers_mt",
                      "in-range observations from many threads produce no "
                      "violations.",
                      &tests_ibounded_concurrent_observers_mt },
                }
            },
            dt::block_spec{
                "scope",
                "invariant_scope RAII monitoring around a block.",
                {
                    { "tests_iscope_reports_on_exit",
                      "the report survives the scope and covers its lifetime.",
                      &tests_iscope_reports_on_exit },
                    { "tests_iscope_interval_construct",
                      "the cadence-taking ctor also starts monitoring.",
                      &tests_iscope_interval_construct },
                    { "tests_iscope_monitor_access",
                      "monitor() exposes the underlying running monitor.",
                      &tests_iscope_monitor_access },
                    { "tests_iscope_check_now_through_monitor",
                      "a synchronous check through the scope's monitor is "
                      "recorded.",
                      &tests_iscope_check_now_through_monitor },
                    { "tests_iscope_detects_violation",
                      "a predicate that fails during the scope is reported "
                      "afterwards.",
                      &tests_iscope_detects_violation },
                }
            },
            dt::block_spec{
                "factory",
                "make_invariant_monitor (prvalue return - no move constructor "
                "required).",
                {
                    { "tests_ifactory_make_invariant_monitor",
                      "the factory returns a monitor that is idle but armed.",
                      &tests_ifactory_make_invariant_monitor },
                    { "tests_ifactory_make_and_check",
                      "a factory-built monitor evaluates its predicate "
                      "synchronously.",
                      &tests_ifactory_make_and_check },
                    { "tests_ifactory_make_and_run_background",
                      "a factory-built monitor can be started and stopped.",
                      &tests_ifactory_make_and_run_background },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_INVARIANT_TESTS_
