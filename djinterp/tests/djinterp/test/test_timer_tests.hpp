/******************************************************************************
* djinterp [test]                                         test_timer_tests.hpp
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
* author(s): TBA                                           created: 2026.06.24
******************************************************************************/

#ifndef DJINTERP_TEST_TIMER_TESTS_
#define DJINTERP_TEST_TIMER_TESTS_ 1

// std
#include <chrono>
#include <cstdio>
#include <cstring>
#include <tuple>
#include <type_traits>
// djinterp
#include "test_timer.hpp"


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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_TIMER_TESTS_
