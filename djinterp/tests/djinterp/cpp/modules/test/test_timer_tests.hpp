/*******************************************************************************
* djinterp [cpp][test]                                      test_timer_tests.hpp
*
* djinterp DTestTimer unit-test header:
*   Declarations, helper types, and a helper macro for the test_timer.hpp
* unit suite. Each test is a parameterless `bool tests_*()` living flat in
* `djinterp::testing`; a test returns true on success and false on the first
* failed check.
*
*   DTestTimer is the generic timer with elapsed_ms(), so the suite is driven
* over a single deterministic instantiation,
*     tt = DTestTimer<test_clock, std::chrono::milliseconds>   (aliased `tt`),
* where test_clock is a manually-advanced fake clock (no real sleeping). The
* event block binds test_timer_event.hpp's handlers to the suite's own events
* (DTtOpen, DTtClose), prefixed for the one-process umbrella, which links
* every suite together.
*
* PORTABILITY
* -----------
*   Requires C++17 or later (the tie-in and the test harness use C++17).
* test_timer.hpp itself targets C++11.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_timer_tests.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.24
*                                                            revised: 2026.11.10
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
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_timer.hpp"
#include "../../../../../inc/djinterp/cpp/modules/test/test_timer_event.hpp"
#include "../../../../../inc/djinterp/cpp/event/event_dispatcher.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/cpp/modules/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


// D_TT_CHECK
//   macro: evaluates a boolean condition inside a DTestTimer test. on failure it
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
using DMsec = std::chrono::milliseconds;

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
    static
    time_point now() D_NOEXCEPT
    {
        return s_now;
    }

    // advance
    //   moves the simulated time forward by the given duration.
    template<typename _Rep,
             typename _Period>
    static
    void advance(
        std::chrono::duration<_Rep, _Period> _by
    ) D_NOEXCEPT
    {
        s_now += std::chrono::duration_cast<duration>(_by);

        return;
    }

    // reset
    //   returns the simulated time to the epoch (zero).
    static
    void reset() D_NOEXCEPT
    {
        s_now = time_point();

        return;
    }

    inline static time_point s_now{};
};

// tt
//   type: the DTestTimer specialization exercised throughout the suite. It
// fixes the clock to test_clock and the duration to milliseconds, so the
// whole suite shares a single instantiation with deterministic timing.
using tt = test::DTestTimer<test_clock, DMsec>;


// -----------------------------------------------------------------
// the suite's events
// -----------------------------------------------------------------

// DTtOpen
//   event: what starts a timer, through the start handler.
D_EVENT_EMPTY(DTtOpen);

// DTtClose
//   event: what stops it.
D_EVENT_EMPTY(DTtClose);

// reset_clock
//   helper: returns the shared simulated clock to zero. Called at the start
// of each test so tests are order-independent.
inline
void reset_clock() D_NOEXCEPT
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

// operations: the generic timer's, as the test timer's
// (test_timer_tests_operations.cpp)
bool tests_test_timer_start_stop_accumulate();

bool tests_test_timer_start_twice_noop();

bool tests_test_timer_start_when_expired_noop();

bool tests_test_timer_stop_not_running_noop();

bool tests_test_timer_reset();

bool tests_test_timer_reset_all_recurses();

// accessors: elapsed / max / has_max / running / expired / remaining
// (test_timer_tests_accessors.cpp)
bool tests_test_timer_elapsed();

bool tests_test_timer_max();

bool tests_test_timer_has_max();

bool tests_test_timer_running();

bool tests_test_timer_expired();

bool tests_test_timer_remaining();

// DTest's addition (test_timer_tests_reports.cpp)
bool tests_test_timer_elapsed_ms();

// the event tie-in (test_timer_tests_event.cpp)
bool tests_test_timer_event_span();

bool tests_test_timer_event_unbind();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from each test's own /* Verifies ... */ block.
// =========================================================================

inline
dt::module_spec timer_spec()
{
    return dt::module_spec{
        "test_timer",
        "test_timer.hpp - the generic timer as DTest's, with elapsed_ms() for "
        "reports, and test_timer_event.hpp's handlers timing the span between "
        "two events, driven over a deterministic fake clock.",
        {
            dt::block_spec{
                "construction",
                "The generic timer's constructors, as the test timer's.",
                {
                    { "tests_test_timer_default_ctor", "the default constructor's initial state", &tests_test_timer_default_ctor },
                    { "tests_test_timer_max_ctor",     "the max-duration constructor", &tests_test_timer_max_ctor },
                }
            },
            dt::block_spec{
                "operations",
                "start / stop / reset / reset_all.",
                {
                    { "tests_test_timer_start_stop_accumulate",   "start/stop cycles accumulate elapsed time", &tests_test_timer_start_stop_accumulate },
                    { "tests_test_timer_start_twice_noop",        "starting a running timer changes nothing", &tests_test_timer_start_twice_noop },
                    { "tests_test_timer_start_when_expired_noop", "an expired timer does not start", &tests_test_timer_start_when_expired_noop },
                    { "tests_test_timer_stop_not_running_noop",   "stopping a stopped timer changes nothing", &tests_test_timer_stop_not_running_noop },
                    { "tests_test_timer_reset",                   "reset() stops the timer and clears its time", &tests_test_timer_reset },
                    { "tests_test_timer_reset_all_recurses",      "reset_all() resets this timer and every owned descendant", &tests_test_timer_reset_all_recurses },
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
                "reports",
                "DTest's addition: milliseconds for reports.",
                {
                    { "tests_test_timer_elapsed_ms", "elapsed_ms() is elapsed() in milliseconds", &tests_test_timer_elapsed_ms },
                }
            },
            dt::block_spec{
                "event",
                "test_timer_event.hpp: start and stop handlers timing a span between two events.",
                {
                    { "tests_test_timer_event_span",   "the handler pair times the span between the two events", &tests_test_timer_event_span },
                    { "tests_test_timer_event_unbind", "the dispatcher's handler ids unbind both handlers", &tests_test_timer_event_unbind },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_TIMER_TESTS_
