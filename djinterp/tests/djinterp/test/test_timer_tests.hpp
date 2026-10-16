/*******************************************************************************
* djinterp [test]                                           test_timer_tests.hpp
*
* djinterp test_timer unit-test header:
*   Declarations, helper types, and a helper macro for the test_timer.hpp
* unit suite. Each test is a parameterless `bool tests_*()` living flat in
* `djinterp::testing`; a test returns true on success and false on the first
* failed check.
*
*   test_timer wraps util::timer and adds an optional event-dispatch layer, so
* the suite is driven over a single deterministic instantiation,
*     tt = test_timer<test_clock, std::chrono::milliseconds>   (aliased `tt`),
* where test_clock is a manually-advanced fake clock (no real sleeping). Event
* behaviour is observed by binding an event_log to a dispatcher via bind_log()
* and triggering timer operations; the log counts the on_start / on_stop /
* on_expire / on_reset events and captures their argument counts.
*
*   PORTABILITY:
*   Requires C++17 or later (the helper event_dispatcher stand-in and the test
* harness use C++17 library facilities). test_timer.hpp itself targets C++11.
*
*
* path:      /tests/djinterp/test/test_timer_tests.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.24
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_TIMER_TESTS_
#define DJINTERP_TEST_TIMER_TESTS_ 1

// std
#include <chrono>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <type_traits>
// djinterp
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../inc/djinterp/test/test_timer.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


// D_TT_CHECK
//   macro: evaluates a boolean condition inside a test_timer test. on failure it
// prints the failing expression and source location, then returns false from
// the enclosing test (which the runner reports as a FAIL). a trailing
// semicolon is expected at each call site.
#define D_TT_CHECK(_condition)                                          \
    do                                                                  \
    {                                                                   \
        if (!(_condition))                                             \
        {                                                               \
            std::printf("    [check] %s\n             at %s:%d\n",       \
                        #_condition,                                    \
                        __FILE__,                                       \
                        __LINE__);                                      \
                                                                        \
            return false;                                               \
        }                                                               \
    }                                                                   \
    while (0)


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); the tests live here in
// djinterp::testing.  (Spec mode also uses it for dt::module_spec / run_module.)
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // fixtures + check-macro support: normal (section-file) mode only


// -----------------------------------------------------------------
// deterministic clock + timer-under-test instantiation
// -----------------------------------------------------------------

// msec
//   type: millisecond duration alias used throughout the suite.
using msec = std::chrono::milliseconds;

// test_clock
//   helper: a manually-advanced steady clock satisfying the Clock named
// requirement. Time only moves when a test calls advance()/reset(), making
// every elapsed/expiry/event-count assertion exact and free of real waiting.
// Its tick is a millisecond, matching the suite's duration so accumulated
// counts come out in round figures.
struct test_clock
{
    using duration   = std::chrono::milliseconds;
    using rep        = duration::rep;
    using period     = duration::period;
    using time_point = std::chrono::time_point<test_clock, duration>;

    static constexpr bool is_steady = true;

    // now
    //   returns the current simulated time point.
    static time_point now() D_NOEXCEPT
    {
        return s_now;
    }

    // advance
    //   moves the simulated time forward by the given duration.
    template<typename _Rep,
             typename _Period>
    static void advance(
        std::chrono::duration<_Rep, _Period> _by
    ) D_NOEXCEPT
    {
        s_now += std::chrono::duration_cast<duration>(_by);

        return;
    }

    // reset
    //   returns the simulated time to the epoch (zero).
    static void reset() D_NOEXCEPT
    {
        s_now = time_point();

        return;
    }

    inline static time_point s_now{};
};

// tt
//   type: the test_timer specialization exercised throughout the suite. It
// fixes the clock to test_clock and the duration to milliseconds, so the
// whole suite shares a single instantiation with deterministic timing.
using tt = test::test_timer<test_clock, msec>;


// -----------------------------------------------------------------
// event observation
// -----------------------------------------------------------------

// event_log
//   helper: tallies the events a dispatcher receives from a tt, and captures
// the argument count carried by the last on_stop / on_reset.
struct event_log
{
    int starts  = 0;
    int stops   = 0;
    int expires = 0;
    int resets  = 0;

    tt::rep_type last_stop_count  = -1;
    tt::rep_type last_reset_count = -1;
};

// bind_log
//   helper: wires an event_log to a dispatcher for tt's four event tags, so
// that firing any tt event updates the log. Mirrors the documented usage,
// e.g. `disp.bind<tt::on_stop>(...)`.
inline void bind_log(
    event_dispatcher& _disp,
    event_log&        _log
)
{
    _disp.bind<tt::on_start>(
        [&_log]()
        {
            ++_log.starts;
        });

    _disp.bind<tt::on_stop>(
        [&_log](tt::rep_type _count)
        {
            ++_log.stops;
            _log.last_stop_count = _count;
        });

    _disp.bind<tt::on_expire>(
        [&_log]()
        {
            ++_log.expires;
        });

    _disp.bind<tt::on_reset>(
        [&_log](tt::rep_type _count)
        {
            ++_log.resets;
            _log.last_reset_count = _count;
        });

    return;
}

// reset_clock
//   helper: returns the shared simulated clock to zero. Called at the start
// of each test so tests are order-independent.
inline void reset_clock() D_NOEXCEPT
{
    test_clock::reset();

    return;
}


#endif  // !DTEST_SPEC_MODE  (fixtures)


// -----------------------------------------------------------------
// test declarations
// -----------------------------------------------------------------

// construction (test_timer_tests_construction.cpp)
bool tests_test_timer_default_ctor();
bool tests_test_timer_max_ctor();
bool tests_test_timer_max_handler_ctor();
bool tests_test_timer_handler_ctor();

// operations: start / stop / reset / reset_all + event firing
// (test_timer_tests_operations.cpp)
bool tests_test_timer_start_fires_on_start();
bool tests_test_timer_start_twice_one_event();
bool tests_test_timer_start_when_expired_noop();
bool tests_test_timer_stop_fires_on_stop();
bool tests_test_timer_stop_fires_on_expire();
bool tests_test_timer_stop_not_running_noop();
bool tests_test_timer_reset_fires_on_reset();
bool tests_test_timer_reset_all_leaf();
bool tests_test_timer_reset_all_recurses();
bool tests_test_timer_multiple_cycles_accumulate();

// accessors: elapsed / max / has_max / running / expired / remaining
// (test_timer_tests_accessors.cpp)
bool tests_test_timer_elapsed();
bool tests_test_timer_max();
bool tests_test_timer_has_max();
bool tests_test_timer_running();
bool tests_test_timer_expired();
bool tests_test_timer_remaining();

// children, owning (test_timer_tests_children.cpp)
bool tests_test_timer_add_child();
bool tests_test_timer_add_child_with_max();
bool tests_test_timer_multiple_children();
bool tests_test_timer_child_access();
bool tests_test_timer_child_access_const();
bool tests_test_timer_child_count();
bool tests_test_timer_children_independent();
bool tests_test_timer_copy_deep_copies_children();

// children, non-owning / observed (test_timer_tests_observed.cpp)
bool tests_test_timer_observe();
bool tests_test_timer_observe_multiple();
bool tests_test_timer_observed_out_of_range();
bool tests_test_timer_observed_live_view();
bool tests_test_timer_observed_count();
bool tests_test_timer_observe_non_owning();

// events: tags, handler access, dispatch on/off (test_timer_tests_events.cpp)
bool tests_test_timer_event_tag_names();
bool tests_test_timer_event_tag_args();
bool tests_test_timer_no_handler_noop();
bool tests_test_timer_handler_accessor();
bool tests_test_timer_set_handler_attach_detach();
bool tests_test_timer_set_handler_swaps();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from each test's own /* Verifies ... */ block.
// =========================================================================

inline dt::module_spec
timer_spec()
{
    return dt::module_spec{
        "test_timer",
        "test_timer.hpp - util::timer wrapped with an optional event-dispatch "
        "layer, driven over a deterministic fake clock: construction, the "
        "start / stop / reset operations and their events, the accessors, the "
        "owning and observed child timers, and the event alphabet.",
        {
            dt::block_spec{
                "construction",
                "The four constructors and their initial state.",
                {
                    { "tests_test_timer_default_ctor",       "the default constructor's initial state", &tests_test_timer_default_ctor },
                    { "tests_test_timer_max_ctor",           "the max-duration constructor, whose handler argument defaults to null", &tests_test_timer_max_ctor },
                    { "tests_test_timer_max_handler_ctor",   "the max-plus-handler constructor", &tests_test_timer_max_handler_ctor },
                    { "tests_test_timer_handler_ctor",       "the handler-only constructor (no maximum limit)", &tests_test_timer_handler_ctor },
                }
            },
            dt::block_spec{
                "operations",
                "start / stop / reset / reset_all and the events they fire.",
                {
                    { "tests_test_timer_start_fires_on_start",       "start() transitions to running and fires on_start once", &tests_test_timer_start_fires_on_start },
                    { "tests_test_timer_start_twice_one_event",      "starting an already-running timer is a no-op for events", &tests_test_timer_start_twice_one_event },
                    { "tests_test_timer_start_when_expired_noop",    "starting an expired timer does not transition or fire", &tests_test_timer_start_when_expired_noop },
                    { "tests_test_timer_stop_fires_on_stop",         "stop() accumulates elapsed time and fires on_stop with the count", &tests_test_timer_stop_fires_on_stop },
                    { "tests_test_timer_stop_fires_on_expire",       "stopping at or beyond the limit additionally fires on_expire", &tests_test_timer_stop_fires_on_expire },
                    { "tests_test_timer_stop_not_running_noop",      "stopping a non-running timer is a no-op", &tests_test_timer_stop_not_running_noop },
                    { "tests_test_timer_reset_fires_on_reset",       "reset() clears elapsed time and fires on_reset with the prior count", &tests_test_timer_reset_fires_on_reset },
                    { "tests_test_timer_reset_all_leaf",             "reset_all() on a childless timer (the empty-recursion case)", &tests_test_timer_reset_all_leaf },
                    { "tests_test_timer_reset_all_recurses",         "reset_all() resets this timer and every owned descendant, each firing on_reset through the inherited dispatcher", &tests_test_timer_reset_all_recurses },
                    { "tests_test_timer_multiple_cycles_accumulate", "successive start/stop cycles accumulate elapsed time", &tests_test_timer_multiple_cycles_accumulate },
                }
            },
            dt::block_spec{
                "accessors",
                "elapsed / max / has_max / running / expired / remaining.",
                {
                    { "tests_test_timer_elapsed",   "elapsed() reflects accumulated time, including live time while running", &tests_test_timer_elapsed },
                    { "tests_test_timer_max",       "max() returns the configured limit", &tests_test_timer_max },
                    { "tests_test_timer_has_max",   "has_max() distinguishes limited from unlimited timers", &tests_test_timer_has_max },
                    { "tests_test_timer_running",   "running() tracks the running state", &tests_test_timer_running },
                    { "tests_test_timer_expired",   "expired() reflects whether accumulated time met the limit", &tests_test_timer_expired },
                    { "tests_test_timer_remaining", "remaining() returns time left before expiry, clamped at zero", &tests_test_timer_remaining },
                }
            },
            dt::block_spec{
                "children_owning",
                "The owned child timers, held by value.",
                {
                    { "tests_test_timer_add_child",                "add_child() with no maximum, including dispatcher inheritance", &tests_test_timer_add_child },
                    { "tests_test_timer_add_child_with_max",       "add_child(max), including dispatcher inheritance", &tests_test_timer_add_child_with_max },
                    { "tests_test_timer_multiple_children",        "several children coexist with their own configuration", &tests_test_timer_multiple_children },
                    { "tests_test_timer_child_access",             "the non-const child() accessor returns a mutable reference", &tests_test_timer_child_access },
                    { "tests_test_timer_child_access_const",       "the const child() overload via a const reference", &tests_test_timer_child_access_const },
                    { "tests_test_timer_child_count",              "child_count() tracks additions", &tests_test_timer_child_count },
                    { "tests_test_timer_children_independent",     "children hold independent state", &tests_test_timer_children_independent },
                    { "tests_test_timer_copy_deep_copies_children","the owning contract - children are held by value, so a copy owns an independent subtree", &tests_test_timer_copy_deep_copies_children },
                }
            },
            dt::block_spec{
                "children_observed",
                "The non-owning observed timers.",
                {
                    { "tests_test_timer_observe",              "a single observe() registration", &tests_test_timer_observe },
                    { "tests_test_timer_observe_multiple",     "several observed timers are tracked in order", &tests_test_timer_observe_multiple },
                    { "tests_test_timer_observed_out_of_range","the bounds check in observed()", &tests_test_timer_observed_out_of_range },
                    { "tests_test_timer_observed_live_view",   "the observed pointer is a live view of the external timer", &tests_test_timer_observed_live_view },
                    { "tests_test_timer_observed_count",       "observed_count() tracks registrations", &tests_test_timer_observed_count },
                    { "tests_test_timer_observe_non_owning",   "observed timers are non-owning - reset_all() does not touch them", &tests_test_timer_observe_non_owning },
                }
            },
            dt::block_spec{
                "events",
                "The event alphabet, handler access, and dispatch on/off.",
                {
                    { "tests_test_timer_event_tag_names",         "each nested event tag reports its documented name", &tests_test_timer_event_tag_names },
                    { "tests_test_timer_event_tag_args",          "the argument tuple of each event tag (a compile-time contract)", &tests_test_timer_event_tag_args },
                    { "tests_test_timer_no_handler_noop",         "the zero-overhead path - with no dispatcher, every operation runs and dispatches nothing", &tests_test_timer_no_handler_noop },
                    { "tests_test_timer_handler_accessor",        "handler() reports the attached dispatcher", &tests_test_timer_handler_accessor },
                    { "tests_test_timer_set_handler_attach_detach","set_handler() attaches and detaches dispatch at runtime", &tests_test_timer_set_handler_attach_detach },
                    { "tests_test_timer_set_handler_swaps",       "set_handler() can redirect dispatch from one dispatcher to another", &tests_test_timer_set_handler_swaps },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_TIMER_TESTS_
