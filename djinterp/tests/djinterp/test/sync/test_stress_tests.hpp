/*******************************************************************************
* djinterp [tests]                                         test_stress_tests.hpp
*
*   DTest suite header for inc/djinterp/test/sync/test_stress.hpp - the
* high-volume stress harness (fixed-iteration, timed, and weighted chaos
* runners over a shared stress_report).
*
*   Two-faced per the DTest authoring guide (DTEST_SPEC_MODE).  Blocks map one
* to one onto section TUs:
*     report   - stress_report aggregation, throughput, lowering
*     stress_runner - the runner class (fixed iteration count)
*     timed    - timed_stress (run for a wall-clock duration)
*     op       - stress_op descriptor
*     chaos    - chaos_runner (weighted op mix)
*     factory  - make_stress / make_timed_stress  (see the gate below)
*
*   ------------------------------------------------------------------------
*   BUILD PREREQUISITE - make_stress / make_timed_stress do not compile
*   ------------------------------------------------------------------------
*   STATUS:  the factory block of this suite is gated OFF by default
*            (D_HS_FACTORY_RETURNABLE = 0) and the module under test is NOT
*            patched by this delivery.  Apply the substitution below, then
*            build with -DD_HS_FACTORY_RETURNABLE=1 to enable the block.
*
*   EXACT SUBSTITUTION, in inc/djinterp/test/sync/test_stress.hpp, after the
*   deleted-copy lines of each class:
*
*       stress_runner(const stress_runner&)            = delete;
*       stress_runner& operator=(const stress_runner&) = delete;
*   +   stress_runner(stress_runner&&)                 = default;
*   +   stress_runner& operator=(stress_runner&&)      = default;
*
*       timed_stress(const timed_stress&)             = delete;
*       timed_stress& operator=(const timed_stress&)  = delete;
*   +   timed_stress(timed_stress&&)                  = default;
*   +   timed_stress& operator=(timed_stress&&)       = default;
*
*   WHY:
*   make_stress() and make_timed_stress() end in `return runner;` / `return
* stress;` where the local is a stress_runner / timed_stress.  Both classes
* user-declare a deleted copy constructor and declare NO move constructor, so
* the implicit move is suppressed and returning by value selects the deleted
* copy constructor: the templates are ill-formed on instantiation.  (This is
* the same defect as make_concurrent_runner in test_concurrent.hpp.)
*
*   The factory block is gated so the suite compiles against the module as it
* stands.  The fix, in test_stress.hpp, is to declare the moves:
*
*       stress_runner(stress_runner&&)            = default;
*       stress_runner& operator=(stress_runner&&) = default;
*       timed_stress(timed_stress&&)              = default;
*       timed_stress& operator=(timed_stress&&)   = default;
*
*   With that in place, define D_HS_FACTORY_RETURNABLE to 1.
*
*
* path:      /tests/djinterp/test/sync/test_stress_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_STRESS_TESTS_
#define DJINTERP_TESTS_TEST_STRESS_TESTS_ 1

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
#include "../../../../inc/djinterp/test/sync/test_stress.hpp"                        // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"        // module_spec + run_module (spec)
#endif


// D_HS_FACTORY_RETURNABLE - see the header banner.
#ifndef D_HS_FACTORY_RETURNABLE
#define D_HS_FACTORY_RETURNABLE 0
#endif


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_HS_CHECK - unique two-letter suffix (hS = stress).
inline bool
hs_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

#define D_HS_CHECK(...)                                                        \
    ::djinterp::testing::hs_check(                                            \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// hs_type_id
//   fixture: a stable test_type_id for to_test_object probes.
inline dt::test_type_id hs_type_id() { return static_cast<dt::test_type_id>(2); }

// hs_tick
//   fixture: a shared operation sink counting invocations.
struct hs_tick
{
    std::atomic<long> calls;

    hs_tick()
        : calls(0)
    {}
};

// hs_bump_op
//   fixture: a stress operation that bumps the shared sink once per call.
inline dt::stress_runner::operation_fn
hs_bump_op(hs_tick& _sink)
{
    hs_tick* p = &_sink;

    return dt::stress_runner::operation_fn(
        [p](std::size_t /*tid*/, std::size_t /*iter*/)
        {
            p->calls.fetch_add(1, std::memory_order_relaxed);
        });
}

// hs_noop_op
//   fixture: an operation that does nothing (used where only the plumbing is
// under test).
inline dt::stress_runner::operation_fn
hs_noop_op()
{
    return dt::stress_runner::operation_fn(
        [](std::size_t /*tid*/, std::size_t /*iter*/) {});
}

// hs_throw_op
//   fixture: an operation that throws on the very first iteration.
inline dt::stress_runner::operation_fn
hs_throw_op()
{
    return dt::stress_runner::operation_fn(
        [](std::size_t /*tid*/, std::size_t _iter)
        {
            if (_iter == 0u)
            {
                throw std::runtime_error("stress op threw");
            }
        });
}

// hs_timed_span
//   fixture: a short but non-trivial wall-clock window for timed_stress.
inline std::chrono::milliseconds hs_timed_span() { return std::chrono::milliseconds(60); }

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   STRESS REPORT                   (test_stress_tests_report.cpp)
bool tests_sreport_defaults();
bool tests_sreport_success_follows_underlying();
bool tests_sreport_ops_per_second_zero_wall();
bool tests_sreport_ops_per_second_computed();
bool tests_sreport_to_test_object_success();
bool tests_sreport_to_test_object_failure();
bool tests_sreport_to_test_object_custom_name();

// II.  STRESS RUNNER                   (test_stress_tests_stress_runner.cpp)
bool tests_srunner_defaults();
bool tests_srunner_setters();
bool tests_srunner_run_zero_threads();
bool tests_srunner_run_zero_iterations();
bool tests_srunner_run_without_operation();
bool tests_srunner_run_total_ops();
bool tests_srunner_run_ops_per_thread();
bool tests_srunner_run_iteration_indices();
bool tests_srunner_run_thread_ids();
bool tests_srunner_run_success_outcome();
bool tests_srunner_run_operation_throws();
bool tests_srunner_run_wall_clock();
bool tests_srunner_run_repeatable();

// III. TIMED STRESS                    (test_stress_tests_timed.cpp)
bool tests_stimed_run_zero_threads();
bool tests_stimed_run_without_operation();
bool tests_stimed_run_performs_work();
bool tests_stimed_run_respects_duration();
bool tests_stimed_run_ops_per_thread_sizes();
bool tests_stimed_run_success_outcome();
bool tests_stimed_run_join_grace();

// IV.  STRESS OP DESCRIPTOR            (test_stress_tests_op.cpp)
bool tests_sop_default_construct();
bool tests_sop_value_construct();
bool tests_sop_action_invocable();
bool tests_sop_assignable();

// V.   CHAOS RUNNER                    (test_stress_tests_chaos.cpp)
bool tests_schaos_defaults();
bool tests_schaos_add_op_named();
bool tests_schaos_add_op_anonymous();
bool tests_schaos_run_without_ops();
bool tests_schaos_run_zero_threads();
bool tests_schaos_run_total_ops();
bool tests_schaos_run_op_names_reported();
bool tests_schaos_run_kind_counts_sum();
bool tests_schaos_run_weight_bias();
bool tests_schaos_run_zero_weight_treated_as_one();
bool tests_schaos_seed_determinism();
bool tests_schaos_seed_changes_mix();

// VI.  FACTORY HELPERS                 (test_stress_tests_factory.cpp)
#if D_HS_FACTORY_RETURNABLE
bool tests_sfactory_make_stress();
bool tests_sfactory_make_stress_runs();
bool tests_sfactory_make_timed_stress();
#endif  // D_HS_FACTORY_RETURNABLE


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
stress_spec()
{
    return dt::module_spec{
        "test_stress",
        "High-volume stress harness: a shared stress report with throughput "
        "accounting, a fixed-iteration runner, a wall-clock timed runner, an "
        "operation descriptor, and a weighted chaos runner.",
        {
            dt::block_spec{
                "report",
                "stress_report defaults, verdict delegation, throughput, and "
                "lowering into a test object.",
                {
                    { "tests_sreport_defaults",
                      "a default stress report has zero ops, empty vectors, and "
                      "zero wall clock.",
                      &tests_sreport_defaults },
                    { "tests_sreport_success_follows_underlying",
                      "success() mirrors the underlying concurrent run report.",
                      &tests_sreport_success_follows_underlying },
                    { "tests_sreport_ops_per_second_zero_wall",
                      "ops_per_second is zero when no wall-clock time elapsed.",
                      &tests_sreport_ops_per_second_zero_wall },
                    { "tests_sreport_ops_per_second_computed",
                      "ops_per_second divides total ops by elapsed seconds.",
                      &tests_sreport_ops_per_second_computed },
                    { "tests_sreport_to_test_object_success",
                      "a successful report lowers to a passing test object.",
                      &tests_sreport_to_test_object_success },
                    { "tests_sreport_to_test_object_failure",
                      "a failed underlying run lowers to a failing test object.",
                      &tests_sreport_to_test_object_failure },
                    { "tests_sreport_to_test_object_custom_name",
                      "the supplied name reaches the object's metadata.",
                      &tests_sreport_to_test_object_custom_name },
                }
            },
            dt::block_spec{
                "stress_runner",
                "stress_runner configuration and the fixed-iteration workload.",
                {
                    { "tests_srunner_defaults",
                      "a fresh runner defaults to one thread, 1000 iterations, "
                      "no timeout.",
                      &tests_srunner_defaults },
                    { "tests_srunner_setters",
                      "thread count, iterations, and join timeout round-trip.",
                      &tests_srunner_setters },
                    { "tests_srunner_run_zero_threads",
                      "running with zero threads does no work.",
                      &tests_srunner_run_zero_threads },
                    { "tests_srunner_run_zero_iterations",
                      "running with zero iterations does no work.",
                      &tests_srunner_run_zero_iterations },
                    { "tests_srunner_run_without_operation",
                      "running without an operation does no work.",
                      &tests_srunner_run_without_operation },
                    { "tests_srunner_run_total_ops",
                      "total_ops equals threads times iterations.",
                      &tests_srunner_run_total_ops },
                    { "tests_srunner_run_ops_per_thread",
                      "every thread records exactly the configured iteration "
                      "count.",
                      &tests_srunner_run_ops_per_thread },
                    { "tests_srunner_run_iteration_indices",
                      "each thread sees iteration indices 0..n-1 exactly once.",
                      &tests_srunner_run_iteration_indices },
                    { "tests_srunner_run_thread_ids",
                      "the operation receives every thread id in range.",
                      &tests_srunner_run_thread_ids },
                    { "tests_srunner_run_success_outcome",
                      "a clean run reports success through the underlying "
                      "report.",
                      &tests_srunner_run_success_outcome },
                    { "tests_srunner_run_operation_throws",
                      "a throwing operation surfaces as a failed underlying "
                      "run.",
                      &tests_srunner_run_operation_throws },
                    { "tests_srunner_run_wall_clock",
                      "the report carries a non-negative wall clock.",
                      &tests_srunner_run_wall_clock },
                    { "tests_srunner_run_repeatable",
                      "a runner can be run more than once with the same "
                      "configuration.",
                      &tests_srunner_run_repeatable },
                }
            },
            dt::block_spec{
                "timed",
                "timed_stress duration-bounded execution.",
                {
                    { "tests_stimed_run_zero_threads",
                      "running with zero threads does no work.",
                      &tests_stimed_run_zero_threads },
                    { "tests_stimed_run_without_operation",
                      "running without an operation does no work.",
                      &tests_stimed_run_without_operation },
                    { "tests_stimed_run_performs_work",
                      "the operation runs repeatedly for the configured window.",
                      &tests_stimed_run_performs_work },
                    { "tests_stimed_run_respects_duration",
                      "the run lasts at least the configured duration.",
                      &tests_stimed_run_respects_duration },
                    { "tests_stimed_run_ops_per_thread_sizes",
                      "one per-thread counter is reported per thread.",
                      &tests_stimed_run_ops_per_thread_sizes },
                    { "tests_stimed_run_success_outcome",
                      "a clean timed run reports success.",
                      &tests_stimed_run_success_outcome },
                    { "tests_stimed_run_join_grace",
                      "a join grace period still permits a clean run.",
                      &tests_stimed_run_join_grace },
                }
            },
            dt::block_spec{
                "op",
                "the stress_op descriptor: name, weight, and action.",
                {
                    { "tests_sop_default_construct",
                      "a default op has an empty name, weight 1, and no action.",
                      &tests_sop_default_construct },
                    { "tests_sop_value_construct",
                      "the value ctor records name, weight, and action.",
                      &tests_sop_value_construct },
                    { "tests_sop_action_invocable",
                      "the stored action is invocable with (tid, iter).",
                      &tests_sop_action_invocable },
                    { "tests_sop_assignable",
                      "op fields are independently assignable.",
                      &tests_sop_assignable },
                }
            },
            dt::block_spec{
                "chaos",
                "chaos_runner op menu, weighted selection, and seed behaviour.",
                {
                    { "tests_schaos_defaults",
                      "a fresh chaos runner has an empty op menu.",
                      &tests_schaos_defaults },
                    { "tests_schaos_add_op_named",
                      "the named add_op overload appends with the given weight.",
                      &tests_schaos_add_op_named },
                    { "tests_schaos_add_op_anonymous",
                      "the anonymous add_op overload appends a default-named "
                      "op.",
                      &tests_schaos_add_op_anonymous },
                    { "tests_schaos_run_without_ops",
                      "running with an empty menu does no work.",
                      &tests_schaos_run_without_ops },
                    { "tests_schaos_run_zero_threads",
                      "running with zero threads does no work.",
                      &tests_schaos_run_zero_threads },
                    { "tests_schaos_run_total_ops",
                      "total_ops equals threads times iterations.",
                      &tests_schaos_run_total_ops },
                    { "tests_schaos_run_op_names_reported",
                      "the report lists one name per registered op, in order.",
                      &tests_schaos_run_op_names_reported },
                    { "tests_schaos_run_kind_counts_sum",
                      "per-kind counts sum to the total op count.",
                      &tests_schaos_run_kind_counts_sum },
                    { "tests_schaos_run_weight_bias",
                      "a heavily weighted op is selected far more often than a "
                      "light one.",
                      &tests_schaos_run_weight_bias },
                    { "tests_schaos_run_zero_weight_treated_as_one",
                      "a zero-weight op still participates in the draw.",
                      &tests_schaos_run_zero_weight_treated_as_one },
                    { "tests_schaos_seed_determinism",
                      "the same seed reproduces the same op-mix.",
                      &tests_schaos_seed_determinism },
                    { "tests_schaos_seed_changes_mix",
                      "the seed is actually consumed by the selection draw.",
                      &tests_schaos_seed_changes_mix },
                }
            },
#if D_HS_FACTORY_RETURNABLE
            dt::block_spec{
                "factory",
                "make_stress / make_timed_stress - require movable runners "
                "(see the header banner).",
                {
                    { "tests_sfactory_make_stress",
                      "make_stress returns a runner configured with the given "
                      "threads and iterations.",
                      &tests_sfactory_make_stress },
                    { "tests_sfactory_make_stress_runs",
                      "a factory-built stress runner performs the expected "
                      "work.",
                      &tests_sfactory_make_stress_runs },
                    { "tests_sfactory_make_timed_stress",
                      "make_timed_stress returns a runnable duration-bounded "
                      "runner.",
                      &tests_sfactory_make_timed_stress },
                }
            },
#endif  // D_HS_FACTORY_RETURNABLE
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_STRESS_TESTS_
