/*******************************************************************************
* djinterp [test]                                                timer_tests.hpp
*
* djinterp timer unit-test header:
*   Declarations, helper types, and helper macros for the timer.hpp unit
* suite. Each test is a parameterless `bool tests_*()` living flat in
* `djinterp::testing`; a test returns true on success and false on the first
* failed check.
*
*   To keep every timing-dependent assertion deterministic, the suite never
* uses real wall-clock time or sleeps. Instead it drives `timer` over
* `test_clock`: a manually-advanced clock that satisfies the Clock named
* requirement but only moves when a test tells it to.
*
*   PORTABILITY:
*   This header pulls in timer.hpp (which provides the namespace macros and the
* portable noexcept specifier). Requires C++17 or later (uses an inline static
* data member).
*
*
* path:      /tests/djinterp/core/util/timer/timer_tests.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.23
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_UTILITY_TIMER_TESTS_
#define DJINTERP_UTILITY_TIMER_TESTS_ 1

// std
#include <chrono>
#include <cstdio>
// djinterp
#include "../../../../../inc/djinterp/core/util/timer/timer.hpp"


// D_TM_CHECK
//   macro: evaluates a boolean condition inside a timer test. on failure it
// prints the failing expression and source location, then returns false from
// the enclosing test (which the runner reports as a FAIL). a trailing
// semicolon is expected at each call site.
#define D_TM_CHECK(_condition)                                          \
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
// duration aliases (readability)
// -----------------------------------------------------------------

// msec
//   type: millisecond duration alias used throughout the suite.
using msec = std::chrono::milliseconds;

// usec
//   type: microsecond duration alias (finer-than-clock duration test).
using usec = std::chrono::microseconds;

// nsec
//   type: nanosecond duration alias (finer-than-clock duration test).
using nsec = std::chrono::nanoseconds;


// -----------------------------------------------------------------
// test_clock
// -----------------------------------------------------------------

// test_clock
//   helper: a manually-advanced steady clock satisfying the Clock named
// requirement. time never passes on its own; a test moves it forward with
// advance()/set(), which makes every elapsed/expiry/remaining assertion
// fully deterministic and free of real sleeps.
//
//   Resolution is intentionally milliseconds so the bulk of the suite reads
// in round millisecond figures. Because the timer assigns `now()` into a
// `time_point<_Clock, _Duration>`, a timer's `_Duration` must be no coarser
// than this clock's tick; millisecond resolution still permits exercising
// finer microsecond/nanosecond timers (see tests_timer_ctor_custom_duration).
struct test_clock
{
    using duration   = std::chrono::milliseconds;
    using rep        = duration::rep;
    using period     = duration::period;
    using time_point = std::chrono::time_point<test_clock, duration>;

    // is_steady
    //   member: the clock only ever moves forward (on request), so it
    // models a steady clock.
    static constexpr bool is_steady = true;

    // now
    //   returns the current simulated time point.
    static time_point now() D_NOEXCEPT
    {
        return s_now;
    }

    // set
    //   sets the simulated time to an absolute time point.
    static void set(
        time_point _tp
    ) D_NOEXCEPT
    {
        s_now = _tp;

        return;
    }

    // advance
    //   moves the simulated time forward by the given duration. the amount
    // may be any chrono duration; it is converted into this clock's tick.
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


// -----------------------------------------------------------------
// timer aliases over the deterministic clock
// -----------------------------------------------------------------

// ms_timer
//   type: the primary timer-under-test specialization (millisecond
// resolution over the manual test clock). equivalent to the default
// timer<test_clock> since test_clock::duration is milliseconds.
using ms_timer = timer<test_clock, msec>;

// us_timer
//   type: microsecond-resolution timer over the test clock; used to confirm
// the template behaves with a duration finer than the clock's tick.
using us_timer = timer<test_clock, usec>;

// ns_timer
//   type: nanosecond-resolution timer over the test clock; used to confirm
// the template behaves with the finest standard duration.
using ns_timer = timer<test_clock, nsec>;


// -----------------------------------------------------------------
// helpers
// -----------------------------------------------------------------

// reset_clock
//   helper: returns the shared test clock to a known baseline (the epoch)
// so each test begins from a deterministic point. every test calls this
// first because the clock's state is static and therefore shared across the
// whole test program.
inline void reset_clock() D_NOEXCEPT
{
    test_clock::reset();

    return;
}


// -----------------------------------------------------------------
// test declarations
// -----------------------------------------------------------------

// constructors (timer_tests_constructors.cpp)
bool tests_timer_default_ctor();
bool tests_timer_max_ctor();
bool tests_timer_zero_max_ctor();
bool tests_timer_ctor_custom_duration();
bool tests_timer_ns_lifecycle();
bool tests_timer_us_lifecycle();

// operations: start / stop / reset / reset_all (timer_tests_operations.cpp)
bool tests_timer_start();
bool tests_timer_start_while_running();
bool tests_timer_start_when_expired();
bool tests_timer_start_when_born_expired();
bool tests_timer_stop();
bool tests_timer_stop_when_not_running();
bool tests_timer_accumulate_cycles();
bool tests_timer_exclude_stopped_intervals();
bool tests_timer_reset_while_running();
bool tests_timer_reset_while_stopped();
bool tests_timer_reset_preserves_children();
bool tests_timer_reset_all_recurses();
bool tests_timer_reset_all_leaf();
bool tests_timer_reset_all_preserves_observed();

// accessors: elapsed / max / has_max / running / expired / remaining
// (timer_tests_accessors.cpp)
bool tests_timer_elapsed_running();
bool tests_timer_elapsed_stopped();
bool tests_timer_elapsed_running_with_accumulation();
bool tests_timer_max_value();
bool tests_timer_has_max_flag();
bool tests_timer_running_flag();
bool tests_timer_expired_without_max();
bool tests_timer_expired_below_limit();
bool tests_timer_expired_at_limit();
bool tests_timer_expired_above_limit();
bool tests_timer_expired_live();
bool tests_timer_remaining_without_max();
bool tests_timer_remaining_before_start();
bool tests_timer_remaining_partial();
bool tests_timer_remaining_at_limit();
bool tests_timer_remaining_past_limit();
bool tests_timer_const_accessors();
bool tests_timer_offset_independence();

// children, owning (timer_tests_children.cpp)
bool tests_timer_add_child();
bool tests_timer_add_child_with_max();
bool tests_timer_multiple_children();
bool tests_timer_child_access();
bool tests_timer_child_access_const();
bool tests_timer_child_count();
bool tests_timer_children_independent();
bool tests_timer_copy_deep_copies_children();

// children, non-owning / observed (timer_tests_observed.cpp)
bool tests_timer_observe();
bool tests_timer_observe_multiple();
bool tests_timer_observed_out_of_range();
bool tests_timer_observed_live_view();
bool tests_timer_observed_count();
bool tests_timer_observe_non_owning();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_UTILITY_TIMER_TESTS_
