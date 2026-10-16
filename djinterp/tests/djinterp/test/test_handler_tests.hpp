/*******************************************************************************
* djinterp [test]                                         test_handler_tests.hpp
*
*   Declarations for the unit-test suite covering test_handler.hpp - the DTest
* session root that composes a typed event_dispatcher with per-session counters
* and a tree walker.  Each free function exercises one facet and returns true
* if every check inside it passed.  Tests are grouped into translation units by
* the section of the header they cover:
*
*   - test_handler_tests_result.cpp   -> III. session_verdict + session_result
*                                          (all_passed / any_failed / verdict)
*   - test_handler_tests_counters.cpp -> IV.  record / reset_counters / result +
*                                          the per-status counter accessors
*   - test_handler_tests_dispatch.cpp -> IV.  the event facade: on / off /
*                                          enable / disable / fire / queue /
*                                          process[_all] / has_listeners_for /
*                                          events() + the convenience macros
*   - test_handler_tests_walk.cpp     -> IV.  run() and the private walk: leaf
*                                          vs interior, status->event mapping,
*                                          to_test_status, counter tallying
*   - test_handler_tests_session.cpp  -> IV.  start/end_session, printer wiring
*                                          (set/clear/printer + install/uninstall
*                                          hooks), and the assertion sink
*
*   The lone shared check helper, test_handler_check, reports a failing check
* (with its stringized expression and source location) and forwards the
* boolean.  The D_TH_CHECK macro is the intended call site.
*
*   OBSERVATION.  Counters and session_result are read directly (public state /
* pure predicates).  The event side is observed by BINDING listeners: the
* lifecycle_recorder binds one recorder to every built-in lifecycle event and
* tallies invocations (plus an order log and the on_session_end payload), so a
* run() or fire() can be checked by what actually dispatched.  Nodes are built
* with make_leaf / make_leaf_raw / make_interior - basic_test values whose
* type_id selects leaf (<=1) vs interior and whose status drives the walk.
*
*   NOTE: the handler lives in djinterp::test; the event vocabulary (verdict,
* handler_id, event_dispatcher) lives in djinterp; the tests live, flat, in
* djinterp::testing.  Hence the `dt` alias plus unqualified verdict/handler_id.
*
*
* path:      /tests/djinterp/test/test_handler_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_HANDLER_TESTS_
#define DJINTERP_TEST_HANDLER_TESTS_ 1

// std
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>
// djinterp
// djinterp  -- framework header first, then the header under test.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/test/test_handler.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: authoring surface + run_module / run_suite
#endif


NS_DJINTERP
NS_TESTING

// dt names the handler surface (djinterp::test); the event vocabulary
// (verdict, handler_id, event_dispatcher) resolves unqualified from djinterp.
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures + check helper


// =========================================================================
//  shared check helper
// =========================================================================

// test_handler_check
//   function: reports the result of a single check.  When _condition is false,
// prints the stringized expression with its source location to stdout; always
// returns _condition so the caller can fold it into a running result.
inline bool
test_handler_check(
    bool        _condition,
    const char* _expression,
    const char* _file,
    int         _line
)
{
    if (!_condition)
    {
        std::printf("    [FAIL] %s:%d: %s\n", _file, _line, _expression);
    }

    return _condition;
}


// =========================================================================
//  node fixtures
// =========================================================================

// make_leaf
//   fixture: a leaf node (type_id 0, so node_is_leaf is true) carrying the
// requested status.
inline dt::basic_test
make_leaf(dt::test_status _status)
{
    dt::basic_test _n(dt::test_type_id{0});
    switch (_status)
    {
        case dt::test_status::passed:  _n.set_status(dt::basic_test::status_passed);  break;
        case dt::test_status::failed:  _n.set_status(dt::basic_test::status_failed);  break;
        case dt::test_status::skipped: _n.set_status(dt::basic_test::status_skipped); break;
        case dt::test_status::pending: _n.set_status(dt::basic_test::status_pending); break;
        case dt::test_status::error:   _n.set_status(dt::basic_test::status_error);   break;
    }
    return _n;
}

// make_leaf_raw
//   fixture: a leaf node carrying a raw status_type value directly (for probing
// the to_test_status default branch with an out-of-range code).
inline dt::basic_test
make_leaf_raw(dt::basic_test::status_type _raw)
{
    dt::basic_test _n(dt::test_type_id{0});
    _n.set_status(_raw);
    return _n;
}

// make_leaf_id
//   fixture: a leaf node with an explicit type_id (for the leaf/interior
// boundary at type_id <= 1) and a passed status.
inline dt::basic_test
make_leaf_id(dt::test_type_id _id)
{
    return dt::basic_test(_id, true);   // passed
}

// make_interior
//   fixture: an interior node (type_id 2, so node_is_leaf is false).
inline dt::basic_test
make_interior()
{
    return dt::basic_test(dt::test_type_id{2});
}


// =========================================================================
//  lifecycle recorder
// =========================================================================

// lifecycle_recorder
//   fixture: binds one recording listener to every built-in lifecycle event on
// a handler and tallies invocations, capturing the on_session_end payload and
// an order log (each event pushes a distinct marker as it fires).  Bind it,
// drive the handler, then read the counters.
struct lifecycle_recorder
{
    int         session_start;
    int         session_end;
    std::size_t end_passed;
    std::size_t end_failed;
    int         module_start;
    int         module_end;
    int         test_start;
    int         test_end;
    int         passed;
    int         failed;
    int         skipped;
    int         error;
    std::vector<int> order;

    lifecycle_recorder()
        : session_start(0), session_end(0), end_passed(0), end_failed(0),
          module_start(0), module_end(0), test_start(0), test_end(0),
          passed(0), failed(0), skipped(0), error(0), order()
    {}

    // markers used in the order log
    enum
    {
        m_session_start = 1, m_session_end = 2,
        m_module_start  = 3, m_module_end  = 4,
        m_test_start    = 5, m_test_end    = 6,
        m_passed        = 7, m_failed      = 8,
        m_skipped       = 9, m_error       = 10
    };

    void bind_all(dt::test_handler& _h)
    {
        _h.on<dt::on_session_start>(
            [this]() { ++session_start; order.push_back(m_session_start); return verdict::pass; });
        _h.on<dt::on_session_end>(
            [this](std::size_t _p, std::size_t _f)
            { ++session_end; end_passed = _p; end_failed = _f; order.push_back(m_session_end); return verdict::pass; });
        _h.on<dt::on_module_start>(
            [this](const dt::basic_test*) { ++module_start; order.push_back(m_module_start); return verdict::pass; });
        _h.on<dt::on_module_end>(
            [this](const dt::basic_test*) { ++module_end; order.push_back(m_module_end); return verdict::pass; });
        _h.on<dt::on_test_start>(
            [this](const dt::basic_test*) { ++test_start; order.push_back(m_test_start); return verdict::pass; });
        _h.on<dt::on_test_end>(
            [this](const dt::basic_test*) { ++test_end; order.push_back(m_test_end); return verdict::pass; });
        _h.on<dt::on_test_passed>(
            [this](const dt::basic_test*) { ++passed; order.push_back(m_passed); return verdict::pass; });
        _h.on<dt::on_test_failed>(
            [this](const dt::basic_test*) { ++failed; order.push_back(m_failed); return verdict::pass; });
        _h.on<dt::on_test_skipped>(
            [this](const dt::basic_test*) { ++skipped; order.push_back(m_skipped); return verdict::pass; });
        _h.on<dt::on_test_error>(
            [this](const dt::basic_test*, const char*) { ++error; order.push_back(m_error); return verdict::pass; });
    }
};

#endif  // !DTEST_SPEC_MODE


// =========================================================================
//  test declarations (visible in both modes; defined in the section TUs)
// =========================================================================

// session_result / session_verdict
bool tests_result_default_zero();
bool tests_result_all_passed();
bool tests_result_any_failed();
bool tests_result_verdict_empty();
bool tests_result_verdict_passed();
bool tests_result_verdict_pending();
bool tests_result_verdict_failed_dominates();

// counters
bool tests_counters_record_each_status();
bool tests_counters_total_accumulates();
bool tests_counters_reset();
bool tests_counters_accessors_reflect();
bool tests_counters_result_snapshot();

// dispatch (event facade)
bool tests_dispatch_on_fire_invoked();
bool tests_dispatch_fire_multiple();
bool tests_dispatch_fire_consume_halts();
bool tests_dispatch_off();
bool tests_dispatch_enable_disable();
bool tests_dispatch_has_listeners_for();
bool tests_dispatch_queue_process_all();
bool tests_dispatch_process_partial();
bool tests_dispatch_process_empty();
bool tests_dispatch_macros();
bool tests_dispatch_events_accessor();

// walk (run + private helpers)
bool tests_walk_leaf_status_counters();
bool tests_walk_leaf_events_order();
bool tests_walk_status_event_mapping();
bool tests_walk_pending_no_status_event();
bool tests_walk_interior_module_events();
bool tests_walk_leaf_vs_interior_boundary();
bool tests_walk_to_test_status_default();
bool tests_walk_no_listeners_still_counts();
bool tests_walk_session_brackets();
bool tests_walk_mixed_tree();
bool tests_walk_empty_tree();
bool tests_walk_start_resets_counters();

// session lifecycle / printer wiring / sink
bool tests_session_start_resets_and_fires();
bool tests_session_end_fires_with_counts();
bool tests_printer_set_get_clear();
bool tests_printer_replace();
bool tests_printer_clear_idempotent();
bool tests_printer_install_uninstall_hooks();
bool tests_printer_dtor_unbinds();
bool tests_sink_push_and_enumerate();
bool tests_sink_clear();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
// =========================================================================

inline dt::module_spec
handler_spec()
{
    return dt::module_spec{
        "test_handler",
        "The DTest session root: a typed event_dispatcher composed with "
        "per-session counters and a tree walker - session results, the counter "
        "surface, the event facade, the run() walk, and the session/printer/sink "
        "lifecycle.",
        {
            dt::block_spec{
                "result",
                "session_verdict + session_result: all_passed, any_failed, and "
                "the three-way verdict.",
                {
                    { "result_default_zero",
                      "a default result is all-zero, empty, not passed, not failed",
                      &tests_result_default_zero },
                    { "result_all_passed",
                      "all_passed iff total > 0 and passed == total",
                      &tests_result_all_passed },
                    { "result_any_failed",
                      "any_failed iff a failure or an error was seen",
                      &tests_result_any_failed },
                    { "result_verdict_empty",
                      "zero total yields the empty verdict",
                      &tests_result_verdict_empty },
                    { "result_verdict_passed",
                      "all-passed with at least one leaf yields passed",
                      &tests_result_verdict_passed },
                    { "result_verdict_pending",
                      "pending with no failure yields pending",
                      &tests_result_verdict_pending },
                    { "result_verdict_failed_dominates",
                      "a failure or error outranks any pending",
                      &tests_result_verdict_failed_dominates },
                }
            },
            dt::block_spec{
                "counters",
                "record / reset_counters / result and the per-status accessors.",
                {
                    { "counters_record_each_status",
                      "record routes each status to its counter and bumps total",
                      &tests_counters_record_each_status },
                    { "counters_total_accumulates",
                      "total is the running sum across records",
                      &tests_counters_total_accumulates },
                    { "counters_reset",
                      "reset_counters zeroes every counter",
                      &tests_counters_reset },
                    { "counters_accessors_reflect",
                      "the per-status accessors mirror result()",
                      &tests_counters_accessors_reflect },
                    { "counters_result_snapshot",
                      "result() returns a by-value snapshot",
                      &tests_counters_result_snapshot },
                }
            },
            dt::block_spec{
                "dispatch",
                "The event facade: bind/unbind/enable/disable, immediate + "
                "deferred dispatch, presence query, events(), and the macros.",
                {
                    { "dispatch_on_fire_invoked",
                      "fire invokes a bound listener and counts it; zero when none",
                      &tests_dispatch_on_fire_invoked },
                    { "dispatch_fire_multiple",
                      "fire invokes every enabled listener",
                      &tests_dispatch_fire_multiple },
                    { "dispatch_fire_consume_halts",
                      "a consuming listener halts the fold",
                      &tests_dispatch_fire_consume_halts },
                    { "dispatch_off",
                      "off removes a listener (found once, then not)",
                      &tests_dispatch_off },
                    { "dispatch_enable_disable",
                      "disable suspends dispatch; enable restores it",
                      &tests_dispatch_enable_disable },
                    { "dispatch_has_listeners_for",
                      "has_listeners_for tracks bound/unbound state",
                      &tests_dispatch_has_listeners_for },
                    { "dispatch_queue_process_all",
                      "queue defers; process_all dispatches and counts",
                      &tests_dispatch_queue_process_all },
                    { "dispatch_process_partial",
                      "process(n) dispatches a prefix, leaving the remainder",
                      &tests_dispatch_process_partial },
                    { "dispatch_process_empty",
                      "process_all on an empty queue dispatches nothing",
                      &tests_dispatch_process_empty },
                    { "dispatch_macros",
                      "D_TEST_ON / D_TEST_FIRE / D_TEST_QUEUE mirror the methods",
                      &tests_dispatch_macros },
                    { "dispatch_events_accessor",
                      "events() exposes the underlying dispatcher",
                      &tests_dispatch_events_accessor },
                }
            },
            dt::block_spec{
                "walk",
                "run() and the private walk: leaf vs interior, status->event "
                "mapping, to_test_status, and counter tallying.",
                {
                    { "walk_leaf_status_counters",
                      "a leaf per status tallies each counter and the total",
                      &tests_walk_leaf_status_counters },
                    { "walk_leaf_events_order",
                      "a leaf fires start, its status event, then end, in order",
                      &tests_walk_leaf_events_order },
                    { "walk_status_event_mapping",
                      "each status fires its matching status event",
                      &tests_walk_status_event_mapping },
                    { "walk_pending_no_status_event",
                      "a pending leaf fires start/end but no status event",
                      &tests_walk_pending_no_status_event },
                    { "walk_interior_module_events",
                      "an interior node fires module start/end and is not tallied",
                      &tests_walk_interior_module_events },
                    { "walk_leaf_vs_interior_boundary",
                      "type_id <= 1 is a leaf; >= 2 is interior",
                      &tests_walk_leaf_vs_interior_boundary },
                    { "walk_to_test_status_default",
                      "an out-of-range status code is treated as pending",
                      &tests_walk_to_test_status_default },
                    { "walk_no_listeners_still_counts",
                      "the walk tallies even with no listeners bound",
                      &tests_walk_no_listeners_still_counts },
                    { "walk_session_brackets",
                      "run() brackets the walk with session start/end",
                      &tests_walk_session_brackets },
                    { "walk_mixed_tree",
                      "a mixed tree tallies and dispatches consistently",
                      &tests_walk_mixed_tree },
                    { "walk_empty_tree",
                      "an empty walk fires only the session brackets",
                      &tests_walk_empty_tree },
                    { "walk_start_resets_counters",
                      "run() resets counters before walking",
                      &tests_walk_start_resets_counters },
                }
            },
            dt::block_spec{
                "session",
                "Session lifecycle, printer wiring (set/clear/printer + the "
                "install/uninstall hooks), and the assertion sink.",
                {
                    { "session_start_resets_and_fires",
                      "start_session zeroes counters and fires on_session_start",
                      &tests_session_start_resets_and_fires },
                    { "session_end_fires_with_counts",
                      "end_session fires on_session_end with the pass/fail counts",
                      &tests_session_end_fires_with_counts },
                    { "printer_set_get_clear",
                      "set_printer stores the pointer; clear_printer forgets it",
                      &tests_printer_set_get_clear },
                    { "printer_replace",
                      "re-attaching swaps the printer and its bundle",
                      &tests_printer_replace },
                    { "printer_clear_idempotent",
                      "clear_printer with no printer is a safe no-op",
                      &tests_printer_clear_idempotent },
                    { "printer_install_uninstall_hooks",
                      "the install/uninstall hooks bind and unbind the bundle",
                      &tests_printer_install_uninstall_hooks },
                    { "printer_dtor_unbinds",
                      "destruction unbinds an attached printer bundle",
                      &tests_printer_dtor_unbinds },
                    { "sink_push_and_enumerate",
                      "push_assertion appends; sink() enumerates",
                      &tests_sink_push_and_enumerate },
                    { "sink_clear",
                      "clear_sink empties the sink, leaving counters untouched",
                      &tests_sink_clear },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


// D_TH_CHECK
//   macro: evaluates its expression exactly once and routes the result through
// test_handler_check, capturing the expression text and source location.
// Variadic so expressions containing top-level commas need no defensive
// parentheses.
#define D_TH_CHECK(...)                                                        \
    ::djinterp::testing::test_handler_check(                                   \
        (__VA_ARGS__), #__VA_ARGS__, __FILE__, __LINE__)


#endif  // DJINTERP_TEST_HANDLER_TESTS_
