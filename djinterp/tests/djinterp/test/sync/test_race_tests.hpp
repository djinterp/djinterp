/*******************************************************************************
* djinterp [tests]                                           test_race_tests.hpp
*
*   DTest suite header for inc/djinterp/test/sync/test_race.hpp - the data-race
* detection toolkit (repeated-run race probe, atomicity observer,
* linearization log, and the sequential-consistency checker).
*
*   Two-faced per the DTest authoring guide (DTEST_SPEC_MODE).  Blocks map one
* to one onto section TUs:
*     probe          - race_probe_report + race_probe
*     atomicity      - atomicity_observer_report + atomicity_observer<T>
*     linearization  - linearization_event + linearization_log
*     consistency    - consistency_check_report + consistency_check
*     factory        - make_race_probe  (see the gate below)
*
*   ------------------------------------------------------------------------
*   BUILD PREREQUISITE - make_race_probe does not compile
*   ------------------------------------------------------------------------
*   STATUS:  the factory block of this suite is gated OFF by default
*            (D_HR_FACTORY_RETURNABLE = 0) and the module under test is NOT
*            patched by this delivery.  Apply the substitution below, then
*            build with -DD_HR_FACTORY_RETURNABLE=1 to enable the block.
*
*   EXACT SUBSTITUTION, in inc/djinterp/test/sync/test_race.hpp, in class
*   race_probe, immediately after the two deleted-copy lines:
*
*       race_probe(const race_probe&)            = delete;
*       race_probe& operator=(const race_probe&) = delete;
*   +   race_probe(race_probe&&)                 = default;
*   +   race_probe& operator=(race_probe&&)      = default;
*
*   WHY:
*   make_race_probe() ends in `return probe;` where `probe` is a local
* race_probe.  race_probe user-declares a deleted copy constructor and
* declares NO move constructor, so the implicit move is suppressed and the
* return selects the deleted copy constructor: the template is ill-formed on
* instantiation.  (Same defect as make_concurrent_runner in test_concurrent
* and make_stress / make_timed_stress in test_stress.)
*
*   The fix, in test_race.hpp:
*
*       race_probe(race_probe&&)            = default;
*       race_probe& operator=(race_probe&&) = default;
*
*   With that in place, define D_HR_FACTORY_RETURNABLE to 1.
*
*   ------------------------------------------------------------------------
*   A NOTE ON DETERMINISM
*   ------------------------------------------------------------------------
*   This module exists to detect races, which are by nature probabilistic.
*   Tests here therefore never assert that a race IS detected in racy code -
*   that would be flaky.  They assert the harness's own contract: that a
*   correct workload produces no evidence, that a deliberately-broken
*   invariant produces evidence on every run, that counters and reports
*   aggregate correctly, and that the log/consistency machinery classifies
*   hand-built event sequences exactly.
*
*
* path:      /tests/djinterp/test/sync/test_race_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_RACE_TESTS_
#define DJINTERP_TESTS_TEST_RACE_TESTS_ 1

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
#include "../../../../inc/djinterp/test/sync/test_race.hpp"                          // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"        // module_spec + run_module (spec)
#endif


// D_HR_FACTORY_RETURNABLE - see the header banner.
#ifndef D_HR_FACTORY_RETURNABLE
#define D_HR_FACTORY_RETURNABLE 0
#endif


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_HR_CHECK - unique two-letter suffix (hR = race).
inline bool
hr_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

#define D_HR_CHECK(...)                                                        \
    ::djinterp::testing::hr_check(                                            \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// hr_type_id
//   fixture: a stable test_type_id for to_test_object probes.
inline dt::test_type_id hr_type_id() { return static_cast<dt::test_type_id>(3); }

// hr_shared
//   fixture: a small shared state a race workload can hammer.  The atomic
// counter is used where correctness must hold; `plain` exists so a test can
// deliberately violate an invariant without relying on an actual race.
struct hr_shared
{
    std::atomic<long> counter;
    long              plain;

    hr_shared()
        : counter(0)
        , plain(0)
    {}

    void reset()
    {
        counter.store(0, std::memory_order_relaxed);
        plain = 0;
    }
};

// hr_make_snapshot_log
//   fixture: builds a log holding _n well-formed instantaneous events for a
// single thread, recorded in order.  record_event stamps invoked_at ==
// completed_at, so such a log is trivially interval-valid and in program
// order.
inline void
hr_fill_log(dt::linearization_log& _log,
            std::size_t            _thread_id,
            std::size_t            _n)
{
    for (std::size_t i = 0; i < _n; ++i)
    {
        _log.record_event(_thread_id, std::string("op"), std::string("ok"));
    }
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   RACE PROBE                      (test_race_tests_probe.cpp)
bool tests_rprobe_report_defaults();
bool tests_rprobe_report_probability_zero_runs();
bool tests_rprobe_report_probability_estimate();
bool tests_rprobe_report_success_predicate();
bool tests_rprobe_report_to_test_object();
bool tests_rprobe_run_zero_threads();
bool tests_rprobe_run_zero_iterations();
bool tests_rprobe_run_zero_runs();
bool tests_rprobe_run_without_operation();
bool tests_rprobe_run_without_invariant();
bool tests_rprobe_run_clean_workload();
bool tests_rprobe_run_counts_runs();
bool tests_rprobe_run_total_iterations();
bool tests_rprobe_run_setup_per_run();
bool tests_rprobe_run_failing_invariant();
bool tests_rprobe_run_throwing_operation();
bool tests_rprobe_run_throwing_invariant();

// II.  ATOMICITY OBSERVER              (test_race_tests_atomicity.cpp)
bool tests_ratomicity_report_defaults();
bool tests_ratomicity_report_success_predicate();
bool tests_ratomicity_report_to_test_object();
bool tests_ratomicity_run_zero_observers();
bool tests_ratomicity_run_zero_observations();
bool tests_ratomicity_run_without_reader();
bool tests_ratomicity_run_without_allowed();
bool tests_ratomicity_run_all_allowed();
bool tests_ratomicity_run_all_forbidden();
bool tests_ratomicity_run_counts_partition();
bool tests_ratomicity_run_reader_invoked();

// III. LINEARIZATION LOG               (test_race_tests_linearization.cpp)
bool tests_rlog_event_defaults();
bool tests_rlog_empty();
bool tests_rlog_record_event();
bool tests_rlog_begin_end_event();
bool tests_rlog_begin_without_end();
bool tests_rlog_event_fields();
bool tests_rlog_snapshot_is_copy();
bool tests_rlog_clear();
bool tests_rlog_ordering_preserved();
bool tests_rlog_concurrent_append_mt();

// IV.  CONSISTENCY CHECK               (test_race_tests_consistency.cpp)
bool tests_rconsistency_report_defaults();
bool tests_rconsistency_report_to_test_object();
bool tests_rconsistency_empty_log();
bool tests_rconsistency_well_formed_log();
bool tests_rconsistency_counts_events();
bool tests_rconsistency_interval_violation();
bool tests_rconsistency_ordering_violation();
bool tests_rconsistency_threads_independent();

// V.   FACTORY HELPERS                 (test_race_tests_factory.cpp)
#if D_HR_FACTORY_RETURNABLE
bool tests_rfactory_make_race_probe();
bool tests_rfactory_make_and_run();
#endif  // D_HR_FACTORY_RETURNABLE


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
race_spec()
{
    return dt::module_spec{
        "test_race",
        "Data-race detection toolkit: a repeated-run race probe with a "
        "post-run invariant, an atomicity observer classifying snapshots, an "
        "append-only linearization log, and a sequential-consistency checker.",
        {
            dt::block_spec{
                "probe",
                "race_probe_report accounting and the race_probe repeated-run "
                "workload.",
                {
                    { "tests_rprobe_report_defaults",
                      "a default probe report has zero runs and no evidence.",
                      &tests_rprobe_report_defaults },
                    { "tests_rprobe_report_probability_zero_runs",
                      "probability_estimate is zero when no run was performed.",
                      &tests_rprobe_report_probability_zero_runs },
                    { "tests_rprobe_report_probability_estimate",
                      "probability_estimate is evidence runs over total runs.",
                      &tests_rprobe_report_probability_estimate },
                    { "tests_rprobe_report_success_predicate",
                      "success() is true exactly when no evidence was seen.",
                      &tests_rprobe_report_success_predicate },
                    { "tests_rprobe_report_to_test_object",
                      "the report lowers to a test object reflecting its "
                      "verdict.",
                      &tests_rprobe_report_to_test_object },
                    { "tests_rprobe_run_zero_threads",
                      "running with zero threads performs no run.",
                      &tests_rprobe_run_zero_threads },
                    { "tests_rprobe_run_zero_iterations",
                      "running with zero iterations performs no run.",
                      &tests_rprobe_run_zero_iterations },
                    { "tests_rprobe_run_zero_runs",
                      "running with zero repetitions performs no run.",
                      &tests_rprobe_run_zero_runs },
                    { "tests_rprobe_run_without_operation",
                      "running without an operation performs no run.",
                      &tests_rprobe_run_without_operation },
                    { "tests_rprobe_run_without_invariant",
                      "running without an invariant performs no run.",
                      &tests_rprobe_run_without_invariant },
                    { "tests_rprobe_run_clean_workload",
                      "a correctly synchronised workload yields no race "
                      "evidence.",
                      &tests_rprobe_run_clean_workload },
                    { "tests_rprobe_run_counts_runs",
                      "runs_performed equals the configured repetition count.",
                      &tests_rprobe_run_counts_runs },
                    { "tests_rprobe_run_total_iterations",
                      "total_thread_iterations equals runs times threads times "
                      "iterations.",
                      &tests_rprobe_run_total_iterations },
                    { "tests_rprobe_run_setup_per_run",
                      "the setup callable runs once before every repetition.",
                      &tests_rprobe_run_setup_per_run },
                    { "tests_rprobe_run_failing_invariant",
                      "an invariant that always fails marks every run as "
                      "evidence.",
                      &tests_rprobe_run_failing_invariant },
                    { "tests_rprobe_run_throwing_operation",
                      "a throwing operation counts as race evidence for that "
                      "run.",
                      &tests_rprobe_run_throwing_operation },
                    { "tests_rprobe_run_throwing_invariant",
                      "an invariant that throws is treated as a failed "
                      "invariant.",
                      &tests_rprobe_run_throwing_invariant },
                }
            },
            dt::block_spec{
                "atomicity",
                "atomicity_observer snapshot classification and its report.",
                {
                    { "tests_ratomicity_report_defaults",
                      "a default observer report has zero observations and "
                      "passes.",
                      &tests_ratomicity_report_defaults },
                    { "tests_ratomicity_report_success_predicate",
                      "success() is false as soon as a forbidden state is "
                      "counted.",
                      &tests_ratomicity_report_success_predicate },
                    { "tests_ratomicity_report_to_test_object",
                      "the report lowers to a test object reflecting its "
                      "verdict.",
                      &tests_ratomicity_report_to_test_object },
                    { "tests_ratomicity_run_zero_observers",
                      "running with zero observers observes nothing.",
                      &tests_ratomicity_run_zero_observers },
                    { "tests_ratomicity_run_zero_observations",
                      "running with a zero observation count observes nothing.",
                      &tests_ratomicity_run_zero_observations },
                    { "tests_ratomicity_run_without_reader",
                      "running without a reader observes nothing.",
                      &tests_ratomicity_run_without_reader },
                    { "tests_ratomicity_run_without_allowed",
                      "running without a classifier observes nothing.",
                      &tests_ratomicity_run_without_allowed },
                    { "tests_ratomicity_run_all_allowed",
                      "a classifier that accepts everything yields no forbidden "
                      "observations.",
                      &tests_ratomicity_run_all_allowed },
                    { "tests_ratomicity_run_all_forbidden",
                      "a classifier that rejects everything fails the run.",
                      &tests_ratomicity_run_all_forbidden },
                    { "tests_ratomicity_run_counts_partition",
                      "allowed plus forbidden always equals the total "
                      "observations.",
                      &tests_ratomicity_run_counts_partition },
                    { "tests_ratomicity_run_reader_invoked",
                      "the reader is invoked once per observation per observer.",
                      &tests_ratomicity_run_reader_invoked },
                }
            },
            dt::block_spec{
                "linearization",
                "linearization_event fields and the append-only "
                "linearization_log.",
                {
                    { "tests_rlog_event_defaults",
                      "a default event has zero thread id and empty strings.",
                      &tests_rlog_event_defaults },
                    { "tests_rlog_empty",
                      "a fresh log is empty and snapshots to nothing.",
                      &tests_rlog_empty },
                    { "tests_rlog_record_event",
                      "record_event appends one instantaneous event.",
                      &tests_rlog_record_event },
                    { "tests_rlog_begin_end_event",
                      "begin_event then end_event records a closed interval "
                      "with a result.",
                      &tests_rlog_begin_end_event },
                    { "tests_rlog_begin_without_end",
                      "an unclosed event keeps its default completion stamp.",
                      &tests_rlog_begin_without_end },
                    { "tests_rlog_event_fields",
                      "thread id, op name, and args survive into the log.",
                      &tests_rlog_event_fields },
                    { "tests_rlog_snapshot_is_copy",
                      "snapshot returns a copy that does not alias the log.",
                      &tests_rlog_snapshot_is_copy },
                    { "tests_rlog_clear",
                      "clear empties the log.",
                      &tests_rlog_clear },
                    { "tests_rlog_ordering_preserved",
                      "events appear in the order they were begun.",
                      &tests_rlog_ordering_preserved },
                    { "tests_rlog_concurrent_append_mt",
                      "concurrent appends from many threads lose no events.",
                      &tests_rlog_concurrent_append_mt },
                }
            },
            dt::block_spec{
                "consistency",
                "consistency_check structural verification of a log.",
                {
                    { "tests_rconsistency_report_defaults",
                      "a default report is consistent with no violations.",
                      &tests_rconsistency_report_defaults },
                    { "tests_rconsistency_report_to_test_object",
                      "the report lowers to a test object reflecting its "
                      "verdict.",
                      &tests_rconsistency_report_to_test_object },
                    { "tests_rconsistency_empty_log",
                      "an empty log is trivially consistent.",
                      &tests_rconsistency_empty_log },
                    { "tests_rconsistency_well_formed_log",
                      "a log of ordered instantaneous events is consistent.",
                      &tests_rconsistency_well_formed_log },
                    { "tests_rconsistency_counts_events",
                      "events_examined equals the number of logged events.",
                      &tests_rconsistency_counts_events },
                    { "tests_rconsistency_interval_violation",
                      "an event completing before it was invoked is an interval "
                      "violation.",
                      &tests_rconsistency_interval_violation },
                    { "tests_rconsistency_ordering_violation",
                      "overlapping operations on one thread are an ordering "
                      "violation.",
                      &tests_rconsistency_ordering_violation },
                    { "tests_rconsistency_threads_independent",
                      "overlap between different threads is not a violation.",
                      &tests_rconsistency_threads_independent },
                }
            },
#if D_HR_FACTORY_RETURNABLE
            dt::block_spec{
                "factory",
                "make_race_probe - requires a movable race_probe (see the "
                "header banner).",
                {
                    { "tests_rfactory_make_race_probe",
                      "the factory returns a probe configured with the given "
                      "counts.",
                      &tests_rfactory_make_race_probe },
                    { "tests_rfactory_make_and_run",
                      "a factory-built probe runs its workload.",
                      &tests_rfactory_make_and_run },
                }
            },
#endif  // D_HR_FACTORY_RETURNABLE
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_RACE_TESTS_
