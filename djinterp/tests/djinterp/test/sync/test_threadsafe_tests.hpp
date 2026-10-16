/*******************************************************************************
* djinterp [tests]                                     test_threadsafe_tests.hpp
*
*   DTest suite header for inc/djinterp/test/sync/test_threadsafe.hpp - the
* umbrella that pulls the whole test/sync subframework (and the production
* threadsafe subframework it sits on) into one include.
*
*   The umbrella declares nothing of its own, so this suite is a REACHABILITY
* contract rather than a behavioural one: after a single include of
* test_threadsafe.hpp, is every public entity of every submodule visible,
* correctly namespaced, and usable?  A regression here means an include was
* dropped, reordered into a cycle, or moved between namespaces - all of which
* are silent breakages for downstream users who include only the umbrella.
*
*   This TU therefore deliberately does NOT include any submodule header
* directly.  Everything it names must arrive through the umbrella alone.
*
*   Two-faced per the DTest authoring guide (DTEST_SPEC_MODE).  Blocks map one
* to one onto section TUs:
*     reachability  - every submodule's public entities resolve and construct
*     traits        - the folded trait/concept surface arrives via the umbrella
*     production    - the production threadsafe types the harness consumes
*     integration   - a short end-to-end flow spanning several submodules
*
*   NOTE ON THE FOLD: test_thread_traits.hpp and test_thread_concepts.hpp were
* folded into test_thread.hpp, and the umbrella's two #include lines for them
* were removed.  The traits block below is precisely the regression test for
* that change: the trait surface must still be reachable through the umbrella,
* now by way of test_thread.hpp.
*
*
* path:      /tests/djinterp/test/sync/test_threadsafe_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_THREADSAFE_TESTS_
#define DJINTERP_TESTS_TEST_THREADSAFE_TESTS_ 1

// std
#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <type_traits>
// djinterp
// -- (part 1) mode-gated includes ------------------------------------------
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/test/sync/test_threadsafe.hpp"                    // THE UMBRELLA - and nothing else
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"        // module_spec + run_module (spec)
#endif


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_HU_CHECK - unique two-letter suffix (hU = umbrella).
inline bool
hu_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

#define D_HU_CHECK(...)                                                        \
    ::djinterp::testing::hu_check(                                            \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// hu_probe
//   fixture: a synthetic type carrying a strategy tag, used to prove the
// folded trait surface arrived through the umbrella.
struct hu_probe
{
    using concurrency_strategy_tag = int;
};

// hu_plain
//   fixture: a marker-free counterpart for the negative side of the same
// check.
struct hu_plain
{
    int x;
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   REACHABILITY                    (test_threadsafe_tests_reachability.cpp)
bool tests_ureach_thread_module();
bool tests_ureach_sync_module();
bool tests_ureach_concurrent_module();
bool tests_ureach_stress_module();
bool tests_ureach_race_module();
bool tests_ureach_invariant_module();
bool tests_ureach_deadlock_module();
bool tests_ureach_enums();
bool tests_ureach_factories();

// II.  FOLDED TRAIT SURFACE            (test_threadsafe_tests_traits.cpp)
bool tests_utraits_namespace_reachable();
bool tests_utraits_positive_and_negative();
bool tests_utraits_composites_reachable();
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
bool tests_utraits_concepts_reachable();
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

// III. PRODUCTION SUBFRAMEWORK         (test_threadsafe_tests_production.cpp)
bool tests_uprod_atomic_size();
bool tests_uprod_exclusive_lock_policy();
bool tests_uprod_condvar_and_once();
bool tests_uprod_thread_helpers();

// IV.  INTEGRATION                     (test_threadsafe_tests_integration.cpp)
bool tests_uintegration_thread_and_sync();
bool tests_uintegration_concurrent_and_invariant();
bool tests_uintegration_stress_over_concurrent();
bool tests_uintegration_race_over_concurrent();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
threadsafe_spec()
{
    return dt::module_spec{
        "test_threadsafe",
        "Umbrella header for the test/sync subframework: verifies that a "
        "single include makes every submodule, the folded trait surface, and "
        "the production threadsafe types reachable and usable together.",
        {
            dt::block_spec{
                "reachability",
                "every submodule's public entities resolve and construct "
                "through the umbrella alone.",
                {
                    { "tests_ureach_thread_module",
                      "test_thread and test_thread_group are reachable and "
                      "constructible.",
                      &tests_ureach_thread_module },
                    { "tests_ureach_sync_module",
                      "the latch, barrier, gate, rendezvous, and "
                      "simultaneous-start primitives are reachable.",
                      &tests_ureach_sync_module },
                    { "tests_ureach_concurrent_module",
                      "the concurrent runner, pipeline, and run reports are "
                      "reachable.",
                      &tests_ureach_concurrent_module },
                    { "tests_ureach_stress_module",
                      "the stress runners, operation descriptor, and stress "
                      "report are reachable.",
                      &tests_ureach_stress_module },
                    { "tests_ureach_race_module",
                      "the race probe, atomicity observer, linearization log, "
                      "and consistency checker are reachable.",
                      &tests_ureach_race_module },
                    { "tests_ureach_invariant_module",
                      "the invariant monitor and the monotonic and bounded "
                      "guards are reachable.",
                      &tests_ureach_invariant_module },
                    { "tests_ureach_deadlock_module",
                      "the deadlock watchdog and lock-order tracker are "
                      "reachable.",
                      &tests_ureach_deadlock_module },
                    { "tests_ureach_enums",
                      "every public enumeration in the subframework is "
                      "reachable.",
                      &tests_ureach_enums },
                    { "tests_ureach_factories",
                      "the thread factory helpers and execution patterns are "
                      "reachable and callable.",
                      &tests_ureach_factories },
                }
            },
            dt::block_spec{
                "traits",
                "the folded threadsafe-testable trait and concept surface "
                "arrives through the umbrella (fold regression guard).",
                {
                    { "tests_utraits_namespace_reachable",
                      "the traits namespace resolves through the umbrella "
                      "without including the retired standalone headers.",
                      &tests_utraits_namespace_reachable },
                    { "tests_utraits_positive_and_negative",
                      "a marked type is classified and a marker-free type is "
                      "not.",
                      &tests_utraits_positive_and_negative },
                    { "tests_utraits_composites_reachable",
                      "the composite classifiers and the summary class "
                      "template are reachable.",
                      &tests_utraits_composites_reachable },
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
                    { "tests_utraits_concepts_reachable",
                      "the C++20 concept layer is reachable and agrees with "
                      "the traits.",
                      &tests_utraits_concepts_reachable },
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS
                }
            },
            dt::block_spec{
                "production",
                "the production threadsafe types the harness is built on "
                "arrive with the umbrella.",
                {
                    { "tests_uprod_atomic_size",
                      "atomic_size is reachable and its load/store/fetch_add "
                      "interface works.",
                      &tests_uprod_atomic_size },
                    { "tests_uprod_exclusive_lock_policy",
                      "exclusive_lock_policy exposes its mutex and lock types "
                      "and its policy flags.",
                      &tests_uprod_exclusive_lock_policy },
                    { "tests_uprod_condvar_and_once",
                      "portable_condvar and portable_once are reachable and "
                      "usable.",
                      &tests_uprod_condvar_and_once },
                    { "tests_uprod_thread_helpers",
                      "the portable thread yield and hardware-concurrency "
                      "helpers are reachable.",
                      &tests_uprod_thread_helpers },
                }
            },
            dt::block_spec{
                "integration",
                "short end-to-end flows spanning several submodules at once.",
                {
                    { "tests_uintegration_thread_and_sync",
                      "a thread group coordinated by a latch completes "
                      "cleanly.",
                      &tests_uintegration_thread_and_sync },
                    { "tests_uintegration_concurrent_and_invariant",
                      "an invariant guard observes a concurrent run without "
                      "violations.",
                      &tests_uintegration_concurrent_and_invariant },
                    { "tests_uintegration_stress_over_concurrent",
                      "a stress run reports through the concurrent run report "
                      "it wraps.",
                      &tests_uintegration_stress_over_concurrent },
                    { "tests_uintegration_race_over_concurrent",
                      "a race probe drives the concurrent runner and finds no "
                      "evidence on correct code.",
                      &tests_uintegration_race_over_concurrent },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_THREADSAFE_TESTS_
