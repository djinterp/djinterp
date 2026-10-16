/*******************************************************************************
* djinterp [tests]                                       test_deadlock_tests.hpp
*
*   DTest suite header for inc/djinterp/test/sync/test_deadlock.hpp - the
* deadlock detection toolkit (a one-shot watchdog timer, a lock-order graph
* tracker with cycle detection, and an RAII watchdog scope).
*
*   Two-faced per the DTest authoring guide (DTEST_SPEC_MODE).  Blocks map one
* to one onto section TUs:
*     report      - deadlock_event_kind / deadlock_event / deadlock_report
*     watchdog    - deadlock_watchdog arm / disarm / fire
*     lock_order  - lock_order_tracker edges and cycle detection
*     scoped      - scoped_deadlock_watchdog (RAII)
*     factory     - make_deadlock_watchdog  (see the gate below)
*
*   ------------------------------------------------------------------------
*   BUILD PREREQUISITE - make_deadlock_watchdog does not compile
*   ------------------------------------------------------------------------
*   STATUS:  the factory block of this suite is gated OFF by default
*            (D_HD_FACTORY_RETURNABLE = 0) and the module under test is NOT
*            patched by this delivery.  Apply the substitution below, then
*            build with -DD_HD_FACTORY_RETURNABLE=1 to enable the block.
*
*   NOTE: unlike the sibling defects in test_concurrent / test_stress /
*   test_race, this one CANNOT be fixed by declaring a defaulted move.
*   deadlock_watchdog holds a std::mutex and a std::condition_variable,
*   neither of which is movable, so `= default` would itself be deleted.  A
*   configured watchdog cannot be returned by value at all while the return
*   statement names a local.  The fix is to make the return a PRVALUE, which
*   C++17 elides with no constructor required - and that needs a configuring
*   constructor to exist.
*
*   EXACT SUBSTITUTION, part 1, in inc/djinterp/test/sync/test_deadlock.hpp,
*   in class deadlock_watchdog, added to the public section:
*
*   +   template<typename _Rep, typename _Period>
*   +   explicit deadlock_watchdog(
*   +       const std::chrono::duration<_Rep, _Period>& _timeout,
*   +       std::string _description = std::string())
*   +       : deadlock_watchdog()
*   +   {
*   +       set_timeout(_timeout);
*   +       set_description(static_cast<std::string&&>(_description));
*   +   }
*
*   EXACT SUBSTITUTION, part 2, in the same header, the body of
*   make_deadlock_watchdog - replace the three configuring lines and the
*   named return with one prvalue return:
*
*   -   deadlock_watchdog wd;
*   -   wd.set_timeout(_timeout);
*   -   wd.set_description(std::move(_description));
*   -   return wd;
*   +   return deadlock_watchdog(
*   +       _timeout, static_cast<std::string&&>(_description));
*
*   WHY:
*   make_deadlock_watchdog() ends in `return wd;` where `wd` is a named local
* deadlock_watchdog.  deadlock_watchdog user-declares a deleted copy
* constructor and declares NO move constructor, so the implicit move is
* suppressed and the return selects the deleted copy constructor: the
* template is ill-formed on instantiation.  (Same defect as
* make_concurrent_runner, make_stress, make_timed_stress, and
* make_race_probe.  Contrast make_invariant_monitor, which already returns a
* prvalue and therefore compiles as shipped.)
*
*   ------------------------------------------------------------------------
*   A NOTE ON TIMING
*   ------------------------------------------------------------------------
*   The watchdog is a real timer.  Tests that must observe a FIRE use a short
* timeout and then wait several times that long; tests that must observe NO
* fire use a long timeout and disarm promptly.  Both directions have an
* order-of-magnitude margin, so neither is a race.
*
*   set_fatal_on_fire(true) is deliberately NEVER exercised with a firing
* watchdog: the fire path calls std::abort(), which would take the whole test
* process down.  Only the setter's acceptance of the flag is covered.
*
*
* path:      /tests/djinterp/test/sync/test_deadlock_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_DEADLOCK_TESTS_
#define DJINTERP_TESTS_TEST_DEADLOCK_TESTS_ 1

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
#include "../../../../inc/djinterp/test/sync/test_deadlock.hpp"                      // the header under test (normal)
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"        // module_spec + run_module (spec)
#endif


// D_HD_FACTORY_RETURNABLE - see the header banner.
#ifndef D_HD_FACTORY_RETURNABLE
#define D_HD_FACTORY_RETURNABLE 0
#endif


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_HD_CHECK - unique two-letter suffix (hD = deadlock).
inline bool
hd_check(bool _cond, const char* _expr, const char* _file, int _line)
{
    if (!_cond)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expr);
    }

    return _cond;
}

#define D_HD_CHECK(...)                                                        \
    ::djinterp::testing::hd_check(                                            \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// hd_type_id
//   fixture: a stable test_type_id for to_test_object probes.
inline dt::test_type_id hd_type_id() { return static_cast<dt::test_type_id>(5); }

// hd_fire_timeout
//   fixture: a watchdog deadline short enough that a test can wait it out.
inline std::chrono::milliseconds hd_fire_timeout()
{
    return std::chrono::milliseconds(30);
}

// hd_await_fire
//   fixture: waits well past hd_fire_timeout so a watchdog that was left
// armed is certain to have fired.
inline void hd_await_fire()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

// hd_long_timeout
//   fixture: a deadline no test will ever reach, used where the watchdog must
// NOT fire.
inline std::chrono::seconds hd_long_timeout()
{
    return std::chrono::seconds(30);
}

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------

// I.   EVENTS AND REPORT               (test_deadlock_tests_report.cpp)
bool tests_dreport_event_kind_values();
bool tests_dreport_event_defaults();
bool tests_dreport_event_fields();
bool tests_dreport_defaults();
bool tests_dreport_ok_predicate();
bool tests_dreport_ok_requires_all_clear();
bool tests_dreport_event_count();
bool tests_dreport_to_test_object_pass();
bool tests_dreport_to_test_object_fail();
bool tests_dreport_to_test_object_custom_name();

// II.  DEADLOCK WATCHDOG               (test_deadlock_tests_watchdog.cpp)
bool tests_dwatchdog_default_construct();
bool tests_dwatchdog_set_timeout_records();
bool tests_dwatchdog_set_fatal_flag_accepted();
bool tests_dwatchdog_arm_disarm_no_fire();
bool tests_dwatchdog_arm_twice();
bool tests_dwatchdog_disarm_without_arm();
bool tests_dwatchdog_disarm_twice();
bool tests_dwatchdog_fires_on_timeout();
bool tests_dwatchdog_fire_records_event();
bool tests_dwatchdog_fire_sets_stop_requested();
bool tests_dwatchdog_fire_invokes_callback();
bool tests_dwatchdog_description_in_event();
bool tests_dwatchdog_default_description();
bool tests_dwatchdog_reset_clears_report();
bool tests_dwatchdog_reset_ignored_while_armed();
bool tests_dwatchdog_rearm_after_fire();
bool tests_dwatchdog_destructor_disarms();

// III. LOCK ORDER TRACKER              (test_deadlock_tests_lock_order.cpp)
bool tests_dorder_empty();
bool tests_dorder_single_acquire_no_edge();
bool tests_dorder_nested_acquire_adds_edge();
bool tests_dorder_edges_deduplicated();
bool tests_dorder_release_pops_held();
bool tests_dorder_release_unknown_thread();
bool tests_dorder_sequential_no_edge();
bool tests_dorder_consistent_order_no_cycle();
bool tests_dorder_ab_ba_detected();
bool tests_dorder_ab_ba_event_fields();
bool tests_dorder_analyze_clean_graph();
bool tests_dorder_analyze_finds_long_cycle();
bool tests_dorder_analyze_preserves_prior();
bool tests_dorder_reset();
bool tests_dorder_concurrent_recording_mt();

// IV.  SCOPED WATCHDOG                 (test_deadlock_tests_scoped.cpp)
bool tests_dscoped_arms_and_disarms();
bool tests_dscoped_no_fire_when_prompt();
bool tests_dscoped_sets_timeout();
bool tests_dscoped_fire_inside_scope();

// V.   FACTORY HELPERS                 (test_deadlock_tests_factory.cpp)
#if D_HD_FACTORY_RETURNABLE
bool tests_dfactory_make_deadlock_watchdog();
bool tests_dfactory_make_with_description();
#endif  // D_HD_FACTORY_RETURNABLE


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
deadlock_spec()
{
    return dt::module_spec{
        "test_deadlock",
        "Deadlock detection toolkit: an event/report model, a one-shot "
        "watchdog timer, a lock-order graph tracker with AB-BA and long-cycle "
        "detection, and an RAII watchdog scope.",
        {
            dt::block_spec{
                "report",
                "deadlock_event_kind, deadlock_event, and deadlock_report "
                "aggregation and lowering.",
                {
                    { "tests_dreport_event_kind_values",
                      "deadlock_event_kind enumerators are the four distinct "
                      "documented values.",
                      &tests_dreport_event_kind_values },
                    { "tests_dreport_event_defaults",
                      "a default event is user_reported with empty description "
                      "and zeroed ids.",
                      &tests_dreport_event_defaults },
                    { "tests_dreport_event_fields",
                      "event fields are plain and independently assignable.",
                      &tests_dreport_event_fields },
                    { "tests_dreport_defaults",
                      "a default report is unfired with zero counts and no "
                      "events.",
                      &tests_dreport_defaults },
                    { "tests_dreport_ok_predicate",
                      "ok() is true only for an unfired, zero-count report.",
                      &tests_dreport_ok_predicate },
                    { "tests_dreport_ok_requires_all_clear",
                      "any one of fired, a timeout, or a cycle is enough to "
                      "fail ok().",
                      &tests_dreport_ok_requires_all_clear },
                    { "tests_dreport_event_count",
                      "event_count reflects the number of recorded events.",
                      &tests_dreport_event_count },
                    { "tests_dreport_to_test_object_pass",
                      "a clean report lowers to a passing test object.",
                      &tests_dreport_to_test_object_pass },
                    { "tests_dreport_to_test_object_fail",
                      "a fired report lowers to a failing test object.",
                      &tests_dreport_to_test_object_fail },
                    { "tests_dreport_to_test_object_custom_name",
                      "the supplied kind and name reach the test object.",
                      &tests_dreport_to_test_object_custom_name },
                }
            },
            dt::block_spec{
                "watchdog",
                "deadlock_watchdog configuration, the arm/disarm cycle, and "
                "the fire path.",
                {
                    { "tests_dwatchdog_default_construct",
                      "a fresh watchdog is unarmed, unfired, and not requesting "
                      "stop.",
                      &tests_dwatchdog_default_construct },
                    { "tests_dwatchdog_set_timeout_records",
                      "set_timeout is echoed into the report's configured "
                      "timeout.",
                      &tests_dwatchdog_set_timeout_records },
                    { "tests_dwatchdog_set_fatal_flag_accepted",
                      "the fatal-on-fire flag can be set and cleared without "
                      "arming.",
                      &tests_dwatchdog_set_fatal_flag_accepted },
                    { "tests_dwatchdog_arm_disarm_no_fire",
                      "a watchdog disarmed well inside its deadline never "
                      "fires.",
                      &tests_dwatchdog_arm_disarm_no_fire },
                    { "tests_dwatchdog_arm_twice",
                      "a second arm while armed has no effect.",
                      &tests_dwatchdog_arm_twice },
                    { "tests_dwatchdog_disarm_without_arm",
                      "disarm on a never-armed watchdog is a safe no-op.",
                      &tests_dwatchdog_disarm_without_arm },
                    { "tests_dwatchdog_disarm_twice",
                      "a second disarm is a safe no-op.",
                      &tests_dwatchdog_disarm_twice },
                    { "tests_dwatchdog_fires_on_timeout",
                      "a watchdog left armed past its deadline fires.",
                      &tests_dwatchdog_fires_on_timeout },
                    { "tests_dwatchdog_fire_records_event",
                      "firing records exactly one timeout event and bumps the "
                      "timeout count.",
                      &tests_dwatchdog_fire_records_event },
                    { "tests_dwatchdog_fire_sets_stop_requested",
                      "firing raises the stop-requested flag for the system "
                      "under test.",
                      &tests_dwatchdog_fire_sets_stop_requested },
                    { "tests_dwatchdog_fire_invokes_callback",
                      "the on-fire callback runs exactly once when the watchdog "
                      "fires.",
                      &tests_dwatchdog_fire_invokes_callback },
                    { "tests_dwatchdog_description_in_event",
                      "a configured description is carried into the fire event.",
                      &tests_dwatchdog_description_in_event },
                    { "tests_dwatchdog_default_description",
                      "with no description set the event carries a non-empty "
                      "default.",
                      &tests_dwatchdog_default_description },
                    { "tests_dwatchdog_reset_clears_report",
                      "reset clears the report and the stop flag while "
                      "preserving the configured timeout.",
                      &tests_dwatchdog_reset_clears_report },
                    { "tests_dwatchdog_reset_ignored_while_armed",
                      "reset is refused while the watchdog is armed.",
                      &tests_dwatchdog_reset_ignored_while_armed },
                    { "tests_dwatchdog_rearm_after_fire",
                      "a fired watchdog can be reset and armed again without "
                      "firing.",
                      &tests_dwatchdog_rearm_after_fire },
                    { "tests_dwatchdog_destructor_disarms",
                      "destroying an armed watchdog joins its watcher cleanly.",
                      &tests_dwatchdog_destructor_disarms },
                }
            },
            dt::block_spec{
                "lock_order",
                "lock_order_tracker edge recording, AB-BA detection, and "
                "graph cycle analysis.",
                {
                    { "tests_dorder_empty",
                      "a fresh tracker has no edges and a clean report.",
                      &tests_dorder_empty },
                    { "tests_dorder_single_acquire_no_edge",
                      "acquiring one lock with nothing held records no edge.",
                      &tests_dorder_single_acquire_no_edge },
                    { "tests_dorder_nested_acquire_adds_edge",
                      "acquiring B while holding A records the ordering edge "
                      "A->B.",
                      &tests_dorder_nested_acquire_adds_edge },
                    { "tests_dorder_edges_deduplicated",
                      "repeating the same acquisition order does not duplicate "
                      "the edge.",
                      &tests_dorder_edges_deduplicated },
                    { "tests_dorder_release_pops_held",
                      "releasing a lock removes it from the thread's held set.",
                      &tests_dorder_release_pops_held },
                    { "tests_dorder_release_unknown_thread",
                      "releasing on an unseen thread is a safe no-op.",
                      &tests_dorder_release_unknown_thread },
                    { "tests_dorder_sequential_no_edge",
                      "locks taken and released one at a time create no "
                      "ordering edge.",
                      &tests_dorder_sequential_no_edge },
                    { "tests_dorder_consistent_order_no_cycle",
                      "two threads taking locks in the same order raise no "
                      "cycle.",
                      &tests_dorder_consistent_order_no_cycle },
                    { "tests_dorder_ab_ba_detected",
                      "two threads taking the same pair in opposite orders "
                      "raise a cycle immediately.",
                      &tests_dorder_ab_ba_detected },
                    { "tests_dorder_ab_ba_event_fields",
                      "the AB-BA event names the offending thread and both "
                      "locks.",
                      &tests_dorder_ab_ba_event_fields },
                    { "tests_dorder_analyze_clean_graph",
                      "analysing an acyclic graph reports no cycle.",
                      &tests_dorder_analyze_clean_graph },
                    { "tests_dorder_analyze_finds_long_cycle",
                      "analyse detects a three-lock cycle that no single "
                      "acquisition revealed.",
                      &tests_dorder_analyze_finds_long_cycle },
                    { "tests_dorder_analyze_preserves_prior",
                      "analyse keeps cycles already recorded during "
                      "acquisition.",
                      &tests_dorder_analyze_preserves_prior },
                    { "tests_dorder_reset",
                      "reset clears edges, thread stacks, and the report.",
                      &tests_dorder_reset },
                    { "tests_dorder_concurrent_recording_mt",
                      "concurrent recording from many threads stays "
                      "self-consistent.",
                      &tests_dorder_concurrent_recording_mt },
                }
            },
            dt::block_spec{
                "scoped",
                "scoped_deadlock_watchdog RAII arming around a block.",
                {
                    { "tests_dscoped_arms_and_disarms",
                      "the scope arms on entry and disarms on exit.",
                      &tests_dscoped_arms_and_disarms },
                    { "tests_dscoped_no_fire_when_prompt",
                      "a scope that closes inside its deadline leaves the "
                      "watchdog unfired.",
                      &tests_dscoped_no_fire_when_prompt },
                    { "tests_dscoped_sets_timeout",
                      "the scope's timeout argument is applied to the watchdog.",
                      &tests_dscoped_sets_timeout },
                    { "tests_dscoped_fire_inside_scope",
                      "a scope that outlives its deadline records a fire.",
                      &tests_dscoped_fire_inside_scope },
                }
            },
#if D_HD_FACTORY_RETURNABLE
            dt::block_spec{
                "factory",
                "make_deadlock_watchdog - requires a returnable watchdog (see "
                "the header banner).",
                {
                    { "tests_dfactory_make_deadlock_watchdog",
                      "the factory returns an unarmed watchdog carrying the "
                      "given timeout.",
                      &tests_dfactory_make_deadlock_watchdog },
                    { "tests_dfactory_make_with_description",
                      "the factory's description reaches a recorded fire event.",
                      &tests_dfactory_make_with_description },
                }
            },
#endif  // D_HD_FACTORY_RETURNABLE
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_TEST_DEADLOCK_TESTS_
