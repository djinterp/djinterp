/******************************************************************************
* djinterp [test]                                       test_counter_tests.hpp
*
* djinterp test_counter unit-test header:
*   Declarations, helper types, and a helper macro for the test_counter.hpp
* unit suite. Each test is a parameterless `bool tests_*()` living flat in
* `djinterp::testing`; a test returns true on success and false on the first
* failed check.
*
*   test_counter wraps util::counter and adds an optional event-dispatch layer,
* so the suite is driven over a single deterministic instantiation,
*     tc = test_counter<int>                                  (aliased `tc`),
* whose integer values make every assertion exact. Event behaviour is observed
* by binding an event_log to a dispatcher via bind_log() and triggering counter
* operations; the log counts the on_increment / on_decrement / on_limit /
* on_reset events and captures their argument values.
*
*   PORTABILITY:
*   Requires C++17 or later (the helper event_dispatcher stand-in and the test
* harness use C++17 library facilities). test_counter.hpp itself targets C++11.
*
*
* path:      /tests/djinterp/test/test_counter_tests.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.06.24
******************************************************************************/

#ifndef DJINTERP_TEST_COUNTER_TESTS_
#define DJINTERP_TEST_COUNTER_TESTS_ 1

// std
#include <cstdio>
#include <cstring>
#include <limits>
#include <tuple>
#include <type_traits>
// djinterp
#include "test_counter.hpp"


// D_TC_CHECK
//   macro: evaluates a boolean condition inside a test_counter test. on failure
// it prints the failing expression and source location, then returns false from
// the enclosing test (which the runner reports as a FAIL). a trailing
// semicolon is expected at each call site.
#define D_TC_CHECK(_condition)                                          \
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
// counter-under-test instantiation
// -----------------------------------------------------------------

// tc
//   type: the test_counter specialization exercised throughout the suite. It
// fixes the value type to int, so the whole suite shares a single instantiation
// with small, exact integer values.
using tc = test::test_counter<int>;


// -----------------------------------------------------------------
// event observation
// -----------------------------------------------------------------

// event_log
//   helper: tallies the events a dispatcher receives from a tc, and captures
// the argument values carried by the last event of each kind.
struct event_log
{
    int increments = 0;
    int decrements = 0;
    int limits     = 0;
    int resets     = 0;

    tc::value_type last_inc_old = 0;
    tc::value_type last_inc_new = 0;
    tc::value_type last_dec_old = 0;
    tc::value_type last_dec_new = 0;
    tc::value_type last_limit   = 0;
    tc::value_type last_reset   = 0;
};

// bind_log
//   helper: wires an event_log to a dispatcher for tc's four event tags, so
// that firing any tc event updates the log. Mirrors the documented usage,
// e.g. `disp.bind<tc::on_increment>(...)`.
inline void bind_log(
    event_dispatcher& _disp,
    event_log&        _log
)
{
    _disp.bind<tc::on_increment>(
        [&_log](tc::value_type _old, tc::value_type _new)
        {
            ++_log.increments;
            _log.last_inc_old = _old;
            _log.last_inc_new = _new;
        });

    _disp.bind<tc::on_decrement>(
        [&_log](tc::value_type _old, tc::value_type _new)
        {
            ++_log.decrements;
            _log.last_dec_old = _old;
            _log.last_dec_new = _new;
        });

    _disp.bind<tc::on_limit>(
        [&_log](tc::value_type _value)
        {
            ++_log.limits;
            _log.last_limit = _value;
        });

    _disp.bind<tc::on_reset>(
        [&_log](tc::value_type _old)
        {
            ++_log.resets;
            _log.last_reset = _old;
        });

    return;
}


// -----------------------------------------------------------------
// test declarations
// -----------------------------------------------------------------

// construction (test_counter_tests_construction.cpp)
bool tests_test_counter_default_ctor();
bool tests_test_counter_value_ctor();
bool tests_test_counter_value_bounds_ctor();
bool tests_test_counter_value_bounds_handler_ctor();

// operations: increment / decrement / reset / reset_all + event firing
// (test_counter_tests_operations.cpp)
bool tests_test_counter_increment_within_bounds();
bool tests_test_counter_increment_exact_max_no_limit();
bool tests_test_counter_increment_overshoot_clamps();
bool tests_test_counter_increment_at_max_noop();
bool tests_test_counter_increment_zero_noop();
bool tests_test_counter_decrement_within_bounds();
bool tests_test_counter_decrement_undershoot_clamps();
bool tests_test_counter_decrement_at_min_noop();
bool tests_test_counter_decrement_zero_noop();
bool tests_test_counter_reset_fires_on_reset();
bool tests_test_counter_reset_all_leaf();
bool tests_test_counter_reset_all_recurses();

// accessors: value / initial / min / max / at_min / at_max
// (test_counter_tests_accessors.cpp)
bool tests_test_counter_value();
bool tests_test_counter_initial();
bool tests_test_counter_min();
bool tests_test_counter_max();
bool tests_test_counter_at_min();
bool tests_test_counter_at_max();

// children, owning (test_counter_tests_children.cpp)
bool tests_test_counter_add_child();
bool tests_test_counter_add_child_with_bounds();
bool tests_test_counter_multiple_children();
bool tests_test_counter_child_access();
bool tests_test_counter_child_access_const();
bool tests_test_counter_child_count();
bool tests_test_counter_children_independent();
bool tests_test_counter_copy_deep_copies_children();

// children, non-owning / observed (test_counter_tests_observed.cpp)
bool tests_test_counter_observe();
bool tests_test_counter_observe_multiple();
bool tests_test_counter_observed_out_of_range();
bool tests_test_counter_observed_live_view();
bool tests_test_counter_observed_count();
bool tests_test_counter_observe_non_owning();

// events: tags, handler access, dispatch on/off (test_counter_tests_events.cpp)
bool tests_test_counter_event_tag_names();
bool tests_test_counter_event_tag_args();
bool tests_test_counter_no_handler_noop();
bool tests_test_counter_handler_accessor();
bool tests_test_counter_set_handler_attach_detach();
bool tests_test_counter_set_handler_swaps();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_COUNTER_TESTS_
