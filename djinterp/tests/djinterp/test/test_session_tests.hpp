/*******************************************************************************
* djinterp [test]                                         test_session_tests.hpp
*
*   Unit-test declarations and shared helpers for test_session.hpp.
*
*   Every test is a nullary `bool tests_*()` predicate (true == pass) living
* flat in djinterp::testing.  Declarations are grouped to mirror the sections
* of test_session.hpp; each group is defined in its own .cpp section TU, and
* the runner (test_session_tests_runner.cpp) links and drives them.
*
*   HANDLER WALK / CONJUNCTIVE-ROOT ACCOUNTING (read this once):
*   The handler-driven run path hands the session's backing forest to a live
* default test_handler.  The handler walks every node of that forest
* (begin()/end(), which includes the implied conjunctive root) and tallies
* ONLY leaf nodes - those whose type_id() <= 1 - by status; interior nodes
* (type_id > 1) fire module events but are not counted.  Because the
* conjunctive root is a default-constructed element (type id 0, status
* pending), it is itself a leaf and is counted as one PENDING observation.
* Consequently, a session populated through tree().add_root(...) and run
* against a handler always reports at least one pending node, so its verdict
* is never `passed`.  To obtain a passed verdict from a real walk, the
* run(handler, tree) overload is fed a pre-built forest whose root is an
* evaluated (non-pending) leaf.
*
*   VERDICT DECISION LOGIC:
*   current_verdict() is private; it is reached through the non-idle
* early-return of run(handler), which returns the verdict computed from the
* session's current counters WITHOUT walking.  The verdict tests drive each
* branch by setting counters directly and then calling run() on a
* non-idle session - no handler walk involved.
*
*
* path:      /tests/djinterp/test/test_session_tests.hpp
* link(s):   TBA
* author(s): djinterp test-suite                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_SESSION_TESTS_
#define DJINTERP_TEST_SESSION_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../inc/djinterp/test/test_session.hpp"
#include "../../../inc/djinterp/test/test_handler.hpp"
#include "../../../inc/djinterp/test/test_object.hpp"
#include "../../../inc/djinterp/test/test_common.hpp"
#include "../../../inc/djinterp/test/test_kind.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); the tests live here in
// djinterp::testing.  (Spec mode also uses it for dt::module_spec / run_module.)
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // fixtures + brought-in names: normal (section-file) mode only


// ---------------------------------------------------------------------------
//  names under test (brought in from djinterp::test for brevity)
// ---------------------------------------------------------------------------

using ::djinterp::test::test_session;
using ::djinterp::test::test_tree;
using ::djinterp::test::session_state;
using ::djinterp::test::session_verdict;
using ::djinterp::test::test_handler;
using ::djinterp::test::basic_test;
using ::djinterp::test::test_kind;
using ::djinterp::test::test_status;
using ::djinterp::test::test_type_id;
using ::djinterp::test::make_test;
using ::djinterp::nary_tree;


// ---------------------------------------------------------------------------
//  shared aliases and helpers
// ---------------------------------------------------------------------------

// session_type
//   alias: the session instantiation under test - basic_test elements over
// the default n-ary tree backing.
using session_type = test_session<basic_test, nary_tree<basic_test>>;

// make_status_test
//   helper: builds a basic_test of the given type id whose status is forced
// to _status.  For passed / failed the result flag is set consistently (via
// evaluate); the other states set only the status.
inline basic_test
make_status_test(
    test_type_id _id,
    test_status  _status
)
{
    basic_test t(_id);

    // drive the node into the requested status
    if (_status == test_status::passed)
    {
        t.evaluate(true);
    }
    else if (_status == test_status::failed)
    {
        t.evaluate(false);
    }
    else if (_status == test_status::skipped)
    {
        t.skip();
    }
    else if (_status == test_status::error)
    {
        t.set_status(basic_test::status_error);
    }
    else
    {
        // test_status::pending - the default-constructed state
        t.set_status(basic_test::status_pending);
    }

    return t;
}


#endif  // !DTEST_SPEC_MODE  (fixtures)


// ---------------------------------------------------------------------------
//  I.  construction, type aliases, and initial state
//      (test_session_tests_construction.cpp)
// ---------------------------------------------------------------------------

bool tests_session_default_state();
bool tests_session_default_counters_zero();
bool tests_session_default_tree_and_timer();
bool tests_session_type_aliases();


// ---------------------------------------------------------------------------
//  II.  accessors: tree, counters, total, timer, elapsed, save/load
//       (test_session_tests_accessors.cpp)
// ---------------------------------------------------------------------------

bool tests_session_tree_accessor_mutable_and_const();
bool tests_session_counter_accessors_mutable_and_const();
bool tests_session_total_sums_counters();
bool tests_session_timer_accessor_mutable_and_const();
bool tests_session_elapsed_initial_zero();
bool tests_session_save_load_are_noops();


// ---------------------------------------------------------------------------
//  III.  state machine: run() / pause / resume / finish / reset
//        (test_session_tests_state.cpp)
// ---------------------------------------------------------------------------

bool tests_session_run_transitions_and_noops();
bool tests_session_pause_transitions_and_noops();
bool tests_session_resume_transitions_and_noops();
bool tests_session_finish_transitions_and_noops();
bool tests_session_reset_clears_state();
bool tests_session_lifecycle_sequence();
bool tests_session_timer_tracks_state();


// ---------------------------------------------------------------------------
//  IV.  handler-driven run: run(handler) / run(handler, tree)
//       (test_session_tests_run.cpp)
// ---------------------------------------------------------------------------

bool tests_session_run_handler_empty_tree();
bool tests_session_run_handler_all_statuses();
bool tests_session_run_handler_all_pass_pending_root();
bool tests_session_run_handler_and_tree_prebuilt_passed();
bool tests_session_run_handler_not_idle_skips_walk();
bool tests_session_run_handler_and_tree_not_idle_no_move();


// ---------------------------------------------------------------------------
//  V.  verdict decision logic (current_verdict via non-idle early return)
//      (test_session_tests_verdict.cpp)
// ---------------------------------------------------------------------------

bool tests_session_current_verdict_empty();
bool tests_session_current_verdict_failed_on_failed();
bool tests_session_current_verdict_failed_on_error();
bool tests_session_current_verdict_pending();
bool tests_session_current_verdict_passed();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from each test's own /* Verifies ... */ block.
// =========================================================================

inline dt::module_spec
session_spec()
{
    return dt::module_spec{
        "test_session",
        "test_session.hpp - the top-level run context: the owned six-kind tree, "
        "the per-status counters, the timer, the lifecycle state machine, the "
        "handler-driven walk, and the verdict decision.",
        {
            dt::block_spec{
                "construction",
                "Construction, type aliases, and initial state.",
                {
                    { "tests_session_default_state",
                      "a default-constructed session starts idle",
                      &tests_session_default_state },
                    { "tests_session_default_counters_zero",
                      "every status counter (and the total) starts at zero",
                      &tests_session_default_counters_zero },
                    { "tests_session_default_tree_and_timer",
                      "the owned tree is empty and the timer idle on construction",
                      &tests_session_default_tree_and_timer },
                    { "tests_session_type_aliases",
                      "the public type aliases resolve as documented",
                      &tests_session_type_aliases },
                }
            },
            dt::block_spec{
                "accessors",
                "The tree / counter / total / timer / elapsed accessors and the "
                "stubbed save / load.",
                {
                    { "tests_session_tree_accessor_mutable_and_const",
                      "both overloads of tree() expose the one owned tree",
                      &tests_session_tree_accessor_mutable_and_const },
                    { "tests_session_counter_accessors_mutable_and_const",
                      "every status counter is reachable through both the const and mutable overloads",
                      &tests_session_counter_accessors_mutable_and_const },
                    { "tests_session_total_sums_counters",
                      "total() returns the sum of all five status counters",
                      &tests_session_total_sums_counters },
                    { "tests_session_timer_accessor_mutable_and_const",
                      "both overloads of timer() expose the one owned timer",
                      &tests_session_timer_accessor_mutable_and_const },
                    { "tests_session_elapsed_initial_zero",
                      "elapsed() forwards the timer's accumulated duration, zero on a fresh session",
                      &tests_session_elapsed_initial_zero },
                    { "tests_session_save_load_are_noops",
                      "the stubbed save() / load() are inert - callable and state-preserving",
                      &tests_session_save_load_are_noops },
                }
            },
            dt::block_spec{
                "state",
                "The lifecycle state machine: run / pause / resume / finish / "
                "reset, their legal transitions, and their no-ops.",
                {
                    { "tests_session_run_transitions_and_noops",
                      "run(): idle -> running, and a no-op from every other state",
                      &tests_session_run_transitions_and_noops },
                    { "tests_session_pause_transitions_and_noops",
                      "pause(): running -> paused, and a no-op from every other state",
                      &tests_session_pause_transitions_and_noops },
                    { "tests_session_resume_transitions_and_noops",
                      "resume(): paused -> running, and a no-op from every other state",
                      &tests_session_resume_transitions_and_noops },
                    { "tests_session_finish_transitions_and_noops",
                      "finish(): valid from running or paused, a no-op from idle or finished",
                      &tests_session_finish_transitions_and_noops },
                    { "tests_session_reset_clears_state",
                      "reset() returns the session to idle and clears the tree, counters, and timer",
                      &tests_session_reset_clears_state },
                    { "tests_session_lifecycle_sequence",
                      "a full lifecycle walk through every state in order",
                      &tests_session_lifecycle_sequence },
                    { "tests_session_timer_tracks_state",
                      "the owned timer's running flag follows the lifecycle",
                      &tests_session_timer_tracks_state },
                }
            },
            dt::block_spec{
                "run",
                "The handler-driven run: run(handler) and run(handler, tree), the "
                "conjunctive-root accounting, and the not-idle early return.",
                {
                    { "tests_session_run_handler_empty_tree",
                      "running an empty session observes nothing and the verdict stays empty",
                      &tests_session_run_handler_empty_tree },
                    { "tests_session_run_handler_all_statuses",
                      "the full sync path counts one observed leaf of every status",
                      &tests_session_run_handler_all_statuses },
                    { "tests_session_run_handler_all_pass_pending_root",
                      "an all-passing leaf set still yields pending - the conjunctive root is itself pending",
                      &tests_session_run_handler_all_pass_pending_root },
                    { "tests_session_run_handler_and_tree_prebuilt_passed",
                      "run(handler, tree) moves in a pre-built forest and runs it to a passed verdict",
                      &tests_session_run_handler_and_tree_prebuilt_passed },
                    { "tests_session_run_handler_not_idle_skips_walk",
                      "run(handler) is a no-op when not idle - it early-returns the verdict without walking",
                      &tests_session_run_handler_not_idle_skips_walk },
                    { "tests_session_run_handler_and_tree_not_idle_no_move",
                      "run(handler, tree) is a no-op when not idle - the supplied tree is left untouched",
                      &tests_session_run_handler_and_tree_not_idle_no_move },
                }
            },
            dt::block_spec{
                "verdict",
                "The verdict decision logic, reached through the non-idle early "
                "return with counters set directly.",
                {
                    { "tests_session_current_verdict_empty",
                      "the verdict is empty when nothing has been observed (total == 0)",
                      &tests_session_current_verdict_empty },
                    { "tests_session_current_verdict_failed_on_failed",
                      "a non-zero failed count yields a failed verdict",
                      &tests_session_current_verdict_failed_on_failed },
                    { "tests_session_current_verdict_failed_on_error",
                      "a non-zero error count (with failed == 0) yields a failed verdict",
                      &tests_session_current_verdict_failed_on_error },
                    { "tests_session_current_verdict_pending",
                      "a non-zero pending count (no failures or errors) yields pending",
                      &tests_session_current_verdict_pending },
                    { "tests_session_current_verdict_passed",
                      "passed when work was observed and nothing failed, errored, or is pending",
                      &tests_session_current_verdict_passed },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_SESSION_TESTS_
