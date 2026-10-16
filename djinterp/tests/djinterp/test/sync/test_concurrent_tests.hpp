/*******************************************************************************
* djinterp [tests]                                     test_concurrent_tests.hpp
*
*   DTest suite header for inc/djinterp/test/sync/test_concurrent.hpp - the
* concurrent execution harness (run reports, the runner, execution patterns,
* the phase pipeline, and the factory).
*
*   Two-faced per the DTest authoring guide (DTEST_SPEC_MODE).  Blocks map one
* to one onto section TUs:
*     report    - thread_report / concurrent_run_report / outcome enum
*     concurrent_runner - the runner class: configuration and run()
*     patterns  - run_simultaneous / _with_timeout / run_reader_writer
*     pipeline  - pipeline_runner phase execution
*     factory   - make_concurrent_runner  (see the gate below)
*
*   ------------------------------------------------------------------------
*   BUILD PREREQUISITE - make_concurrent_runner does not compile
*   ------------------------------------------------------------------------
*   STATUS:  the factory block of this suite is gated OFF by default
*            (D_HC_FACTORY_RETURNABLE = 0) and the module under test is NOT
*            patched by this delivery.  Apply the substitution below, then
*            build with -DD_HC_FACTORY_RETURNABLE=1 to enable the block.
*
*   EXACT SUBSTITUTION, in inc/djinterp/test/sync/test_concurrent.hpp, in
*   class concurrent_runner, immediately after the two deleted-copy lines:
*
*       concurrent_runner(const concurrent_runner&)            = delete;
*       concurrent_runner& operator=(const concurrent_runner&) = delete;
*   +   concurrent_runner(concurrent_runner&&)                 = default;
*   +   concurrent_runner& operator=(concurrent_runner&&)      = default;
*
*   WHY:
*   make_concurrent_runner() ends in `return runner;` where `runner` is a
* local concurrent_runner.  concurrent_runner user-declares a deleted copy
* constructor and declares NO move constructor, so the implicit move
* constructor is suppressed; returning by value selects the deleted copy
* constructor and the template is ill-formed on instantiation.  (Compare
* test_thread and test_thread_group, which both explicitly declare moves and
* whose factories therefore work.)
*
*   The factory block is gated so this suite compiles against the module as it
* stands.  The one-line fix in test_concurrent.hpp is:
*
*       concurrent_runner(concurrent_runner&&)            = default;
*       concurrent_runner& operator=(concurrent_runner&&) = default;
*
*   With that in place, define D_HC_FACTORY_RETURNABLE to 1 (here or on the
* command line) and the factory block compiles and runs.
*
*
* path:      /tests/djinterp/test/sync/test_concurrent_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_CONCURRENT_TESTS_
#define DJINTERP_TESTS_TEST_CONCURRENT_TESTS_ 1

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
#include "../../../../inc/djinterp/test/sync/test_concurrent.hpp"                    // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"        // module_spec + run_module (spec)
#endif


// D_HC_FACTORY_RETURNABLE
//   set to 1 once concurrent_runner declares a move constructor (see the
// header banner).  Gates the factory block's declarations AND its spec rows
// on the same macro, per the authoring guide.
#ifndef D_HC_FACTORY_RETURNABLE
#define D_HC_FACTORY_RETURNABLE 0
#endif


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_HC_CHECK - unique two-letter suffix (hC = concurrent).
inline bool
hc_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

#define D_HC_CHECK(...)                                                        \
    ::djinterp::testing::hc_check(                                            \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// hc_type_id
//   fixture: an arbitrary but stable test_type_id for to_test_object probes.
inline dt::test_type_id hc_type_id() { return static_cast<dt::test_type_id>(1); }

// hc_short / hc_long_ms
//   fixtures: a join timeout that is certain to expire, and the worker sleep
// that outlasts it by a wide margin.
inline std::chrono::milliseconds hc_short()   { return std::chrono::milliseconds(20); }
inline unsigned                  hc_long_ms() { return 400u; }

// hc_counter
//   fixture: a shared invocation sink for workers.
struct hc_counter
{
    std::atomic<int> calls;

    hc_counter()
        : calls(0)
    {}
};

// hc_bump
//   fixture: worker that bumps a counter once per invocation.
inline dt::test_thread::worker_fn
hc_bump(hc_counter& _sink)
{
    hc_counter* p = &_sink;

    return dt::test_thread::worker_fn(
        [p](std::size_t /*id*/)
        {
            p->calls.fetch_add(1, std::memory_order_acq_rel);
        });
}

// hc_thrower
//   fixture: worker that always throws with the given message.
inline dt::test_thread::worker_fn
hc_thrower(const char* _msg)
{
    std::string msg(_msg);

    return dt::test_thread::worker_fn(
        [msg](std::size_t /*id*/)
        {
            throw std::runtime_error(msg);
        });
}

// hc_sleeper
//   fixture: worker that outlives a short join timeout.
inline dt::test_thread::worker_fn
hc_sleeper(unsigned _ms)
{
    return dt::test_thread::worker_fn(
        [_ms](std::size_t /*id*/)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(_ms));
        });
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   RUN REPORT                      (test_concurrent_tests_report.cpp)
bool tests_creport_outcome_enum_values();
bool tests_creport_thread_report_defaults();
bool tests_creport_thread_report_fields();
bool tests_creport_run_report_defaults();
bool tests_creport_success_predicate();
bool tests_creport_to_test_object_success();
bool tests_creport_to_test_object_thread_threw();
bool tests_creport_to_test_object_timed_out();
bool tests_creport_to_test_object_not_started();
bool tests_creport_to_test_object_custom_name();

// II.  CONCURRENT RUNNER               (test_concurrent_tests_concurrent_runner.cpp)
bool tests_crunner_default_construct();
bool tests_crunner_explicit_construct();
bool tests_crunner_set_thread_count();
bool tests_crunner_thread_count_from_heterogeneous();
bool tests_crunner_clear_workers();
bool tests_crunner_join_timeout_accessors();
bool tests_crunner_run_zero_threads_not_started();
bool tests_crunner_run_without_worker_not_started();
bool tests_crunner_run_homogeneous_success();
bool tests_crunner_run_invokes_each_thread_once();
bool tests_crunner_run_thread_ids_unique();
bool tests_crunner_run_heterogeneous();
bool tests_crunner_run_per_thread_reports();
bool tests_crunner_run_thread_threw();
bool tests_crunner_run_first_exception();
bool tests_crunner_run_partial_failure_counts();
bool tests_crunner_run_timeout_outcome();
bool tests_crunner_simultaneous_start();
bool tests_crunner_wall_clock_nonnegative();

// III. EXECUTION PATTERNS              (test_concurrent_tests_patterns.cpp)
bool tests_cpatterns_run_simultaneous();
bool tests_cpatterns_run_simultaneous_reports();
bool tests_cpatterns_run_simultaneous_failure();
bool tests_cpatterns_with_timeout_success();
bool tests_cpatterns_with_timeout_expires();
bool tests_cpatterns_reader_writer_counts();
bool tests_cpatterns_reader_writer_id_split();

// IV.  PHASE PIPELINE                  (test_concurrent_tests_pipeline.cpp)
bool tests_cpipeline_construct();
bool tests_cpipeline_add_phase();
bool tests_cpipeline_run_no_phases();
bool tests_cpipeline_run_all_phases();
bool tests_cpipeline_phase_indices();
bool tests_cpipeline_barrier_ordering();

// V.   FACTORY HELPERS                 (test_concurrent_tests_factory.cpp)
#if D_HC_FACTORY_RETURNABLE
bool tests_cfactory_make_concurrent_runner();
bool tests_cfactory_make_and_run();
#endif  // D_HC_FACTORY_RETURNABLE


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
concurrent_spec()
{
    return dt::module_spec{
        "test_concurrent",
        "Concurrent execution harness: per-thread and aggregate run reports, "
        "the configurable concurrent_runner, the simultaneous / reader-writer "
        "execution patterns, and the barrier-separated phase pipeline.",
        {
            dt::block_spec{
                "report",
                "thread_report and concurrent_run_report defaults, the outcome "
                "enum, and lowering a report into a test object.",
                {
                    { "tests_creport_outcome_enum_values",
                      "concurrent_run_outcome enumerators are the five distinct "
                      "documented values.",
                      &tests_creport_outcome_enum_values },
                    { "tests_creport_thread_report_defaults",
                      "a default thread_report is idle, unfailed, zero-elapsed, "
                      "with empty strings.",
                      &tests_creport_thread_report_defaults },
                    { "tests_creport_thread_report_fields",
                      "thread_report fields are plain and independently "
                      "assignable.",
                      &tests_creport_thread_report_fields },
                    { "tests_creport_run_report_defaults",
                      "a default run report is not_started with zero counts and "
                      "no captured exception.",
                      &tests_creport_run_report_defaults },
                    { "tests_creport_success_predicate",
                      "success() is true only for the success outcome.",
                      &tests_creport_success_predicate },
                    { "tests_creport_to_test_object_success",
                      "a success report lowers to a passing test object.",
                      &tests_creport_to_test_object_success },
                    { "tests_creport_to_test_object_thread_threw",
                      "a thread_threw report lowers to a failing object naming "
                      "the throw.",
                      &tests_creport_to_test_object_thread_threw },
                    { "tests_creport_to_test_object_timed_out",
                      "a timed_out report lowers to a failing object naming the "
                      "timeout.",
                      &tests_creport_to_test_object_timed_out },
                    { "tests_creport_to_test_object_not_started",
                      "a not_started report lowers to a failing object.",
                      &tests_creport_to_test_object_not_started },
                    { "tests_creport_to_test_object_custom_name",
                      "the name argument is stored in the object's metadata.",
                      &tests_creport_to_test_object_custom_name },
                }
            },
            dt::block_spec{
                "concurrent_runner",
                "concurrent_runner configuration, worker resolution, and the "
                "outcomes produced by run().",
                {
                    { "tests_crunner_default_construct",
                      "a default runner has zero threads and no join timeout.",
                      &tests_crunner_default_construct },
                    { "tests_crunner_explicit_construct",
                      "the sized ctor records the thread count.",
                      &tests_crunner_explicit_construct },
                    { "tests_crunner_set_thread_count",
                      "set_thread_count replaces the configured count.",
                      &tests_crunner_set_thread_count },
                    { "tests_crunner_thread_count_from_heterogeneous",
                      "added per-thread workers override the configured count.",
                      &tests_crunner_thread_count_from_heterogeneous },
                    { "tests_crunner_clear_workers",
                      "clear_workers drops both worker faces and restores the "
                      "configured count.",
                      &tests_crunner_clear_workers },
                    { "tests_crunner_join_timeout_accessors",
                      "the join timeout defaults to zero and round-trips.",
                      &tests_crunner_join_timeout_accessors },
                    { "tests_crunner_run_zero_threads_not_started",
                      "running with zero threads yields not_started.",
                      &tests_crunner_run_zero_threads_not_started },
                    { "tests_crunner_run_without_worker_not_started",
                      "running without a bound worker yields not_started.",
                      &tests_crunner_run_without_worker_not_started },
                    { "tests_crunner_run_homogeneous_success",
                      "a clean homogeneous run reports success with every "
                      "thread completed.",
                      &tests_crunner_run_homogeneous_success },
                    { "tests_crunner_run_invokes_each_thread_once",
                      "the homogeneous worker runs exactly once per thread.",
                      &tests_crunner_run_invokes_each_thread_once },
                    { "tests_crunner_run_thread_ids_unique",
                      "each thread is handed a distinct sequential id.",
                      &tests_crunner_run_thread_ids_unique },
                    { "tests_crunner_run_heterogeneous",
                      "per-thread workers each run on their own thread.",
                      &tests_crunner_run_heterogeneous },
                    { "tests_crunner_run_per_thread_reports",
                      "the report carries one entry per thread with matching "
                      "ids and states.",
                      &tests_crunner_run_per_thread_reports },
                    { "tests_crunner_run_thread_threw",
                      "a throwing worker yields the thread_threw outcome.",
                      &tests_crunner_run_thread_threw },
                    { "tests_crunner_run_first_exception",
                      "the report captures the first thrown exception.",
                      &tests_crunner_run_first_exception },
                    { "tests_crunner_run_partial_failure_counts",
                      "completed and failed counts split correctly on a mixed "
                      "run.",
                      &tests_crunner_run_partial_failure_counts },
                    { "tests_crunner_run_timeout_outcome",
                      "a join timeout shorter than the work yields timed_out.",
                      &tests_crunner_run_timeout_outcome },
                    { "tests_crunner_simultaneous_start",
                      "a run with the simultaneous-start handshake still "
                      "completes cleanly.",
                      &tests_crunner_simultaneous_start },
                    { "tests_crunner_wall_clock_nonnegative",
                      "the reported wall clock is non-negative.",
                      &tests_crunner_wall_clock_nonnegative },
                }
            },
            dt::block_spec{
                "patterns",
                "the convenience execution patterns layered over the runner.",
                {
                    { "tests_cpatterns_run_simultaneous",
                      "run_simultaneous runs the worker on every thread and "
                      "succeeds.",
                      &tests_cpatterns_run_simultaneous },
                    { "tests_cpatterns_run_simultaneous_reports",
                      "run_simultaneous reports one thread entry per thread.",
                      &tests_cpatterns_run_simultaneous_reports },
                    { "tests_cpatterns_run_simultaneous_failure",
                      "a throwing worker surfaces as thread_threw through the "
                      "pattern.",
                      &tests_cpatterns_run_simultaneous_failure },
                    { "tests_cpatterns_with_timeout_success",
                      "run_simultaneous_with_timeout succeeds when the work "
                      "fits the window.",
                      &tests_cpatterns_with_timeout_success },
                    { "tests_cpatterns_with_timeout_expires",
                      "run_simultaneous_with_timeout reports timed_out when the "
                      "work overruns.",
                      &tests_cpatterns_with_timeout_expires },
                    { "tests_cpatterns_reader_writer_counts",
                      "run_reader_writer spawns readers plus writers threads.",
                      &tests_cpatterns_reader_writer_counts },
                    { "tests_cpatterns_reader_writer_id_split",
                      "readers take the low id range and writers the high "
                      "range.",
                      &tests_cpatterns_reader_writer_id_split },
                }
            },
            dt::block_spec{
                "pipeline",
                "pipeline_runner phase registration and barrier-separated "
                "phase execution.",
                {
                    { "tests_cpipeline_construct",
                      "a fresh pipeline has no phases.",
                      &tests_cpipeline_construct },
                    { "tests_cpipeline_add_phase",
                      "add_phase appends and phase_count tracks it.",
                      &tests_cpipeline_add_phase },
                    { "tests_cpipeline_run_no_phases",
                      "running with no phases still completes every thread.",
                      &tests_cpipeline_run_no_phases },
                    { "tests_cpipeline_run_all_phases",
                      "every phase runs once on every thread.",
                      &tests_cpipeline_run_all_phases },
                    { "tests_cpipeline_phase_indices",
                      "each phase callable receives its own phase index.",
                      &tests_cpipeline_phase_indices },
                    { "tests_cpipeline_barrier_ordering",
                      "no thread enters a phase before every thread has left "
                      "the previous one.",
                      &tests_cpipeline_barrier_ordering },
                }
            },
#if D_HC_FACTORY_RETURNABLE
            dt::block_spec{
                "factory",
                "make_concurrent_runner - requires a movable concurrent_runner "
                "(see the header banner).",
                {
                    { "tests_cfactory_make_concurrent_runner",
                      "the factory returns a runner configured with the given "
                      "thread count.",
                      &tests_cfactory_make_concurrent_runner },
                    { "tests_cfactory_make_and_run",
                      "a factory-built runner runs its worker on every thread.",
                      &tests_cfactory_make_and_run },
                }
            },
#endif  // D_HC_FACTORY_RETURNABLE
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_CONCURRENT_TESTS_
