/*******************************************************************************
* djinterp [cpp][test]                                    test_counter_tests.hpp
*
* djinterp test_counter unit-test header:
*   Declarations, helper types, and a helper macro for the test_counter.hpp
* unit suite. Each test is a parameterless `bool tests_*()` living flat in
* `djinterp::testing`; a test returns true on success and false on the first
* failed check.
*
*   test_counter is the generic counter with a step, and test_tally keeps one
* per key type, so the suite is driven over a single deterministic
* instantiation,
*     tc = test_counter<int>, tl = test_tally<int>,
* whose integer values make every assertion exact. The event block binds
* test_counter_event.hpp's handlers; its events are the suite's own tags
* (DTcTick, DTcBytes, DTcMiss), which also serve the tally as keys, prefixed
* so that the one-process umbrella (test_all), which links every suite
* together, sees no other suite's tag of the same name.
*
* PORTABILITY
* -----------
*   Requires C++17 or later, as test_counter.hpp does.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_counter_tests.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.24
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_TEST_COUNTER_TESTS_
#define DJINTERP_TEST_COUNTER_TESTS_ 1

// std
#include <cstdio>
#include <cstring>
#include <limits>
#include <tuple>
#include <type_traits>
// djinterp
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_counter.hpp"
#include "../../../../../inc/djinterp/cpp/modules/test/test_counter_event.hpp"
#include "../../../../../inc/djinterp/cpp/event/event_dispatcher.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/cpp/modules/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


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

// dt names the entities under test (djinterp::test); the tests live here in
// djinterp::testing.  (Spec mode also uses it for dt::module_spec / run_module.)
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // fixtures + check-macro support: normal (section-file) mode only


// -----------------------------------------------------------------
// types under test
// -----------------------------------------------------------------

// tc
//   type: the test_counter specialization exercised throughout the suite. It
// fixes the value type to int, so the whole suite shares a single instantiation
// with small, exact integer values.
using tc = test::test_counter<int>;

// tl
//   type: the test_tally over tc.
using tl = test::test_tally<int>;


// -----------------------------------------------------------------
// the suite's events
// -----------------------------------------------------------------

// DTcTick
//   event: an occurrence with no payload; what most tests count.
D_EVENT_EMPTY(DTcTick);

// DTcBytes
//   event: an occurrence carrying an amount, for counting by payload.
D_EVENT(DTcBytes, int);

// DTcMiss
//   event: an event nothing counts, to show a counter ignores it.
D_EVENT_EMPTY(DTcMiss);


#endif  // !DTEST_SPEC_MODE  (fixtures)


// -----------------------------------------------------------------
// test declarations
// -----------------------------------------------------------------

// construction (test_counter_tests_construction.cpp)
bool tests_test_counter_default_ctor();

bool tests_test_counter_value_ctor();

bool tests_test_counter_value_bounds_ctor();

// operations: the generic counter's, as the test counter's
// (test_counter_tests_operations.cpp)
bool tests_test_counter_increment_within_bounds();

bool tests_test_counter_increment_overshoot_clamps();

bool tests_test_counter_decrement_within_bounds();

bool tests_test_counter_decrement_undershoot_clamps();

bool tests_test_counter_reset();

bool tests_test_counter_reset_all_recurses();

// accessors: value / initial / min / max / at_min / at_max
// (test_counter_tests_accessors.cpp)
bool tests_test_counter_value();

bool tests_test_counter_initial();

bool tests_test_counter_min();

bool tests_test_counter_max();

bool tests_test_counter_at_min();

bool tests_test_counter_at_max();

// the tally (test_counter_tests_tally.cpp)
bool tests_test_tally_track();

bool tests_test_tally_increment_and_count();

bool tests_test_tally_merge();

bool tests_test_tally_reset_and_clear();

// the event tie-in (test_counter_tests_event.cpp)
bool tests_test_counter_event_each_occurrence();

bool tests_test_counter_event_operations();

bool tests_test_counter_event_unbind();

bool tests_test_tally_event();


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from each test's own /* Verifies ... */ block.
// =========================================================================

inline
dt::module_spec counter_spec()
{
    return dt::module_spec{
        "test_counter",
        "test_counter.hpp - the generic counter as DTest's, with a step, and "
        "test_tally keeping one per key type; test_counter_event.hpp's handlers "
        "binding them to events; over a deterministic int instantiation.",
        {
            dt::block_spec{
                "construction",
                "The generic counter's constructors, as the test counter's.",
                {
                    { "tests_test_counter_default_ctor",      "the default constructor's initial state", &tests_test_counter_default_ctor },
                    { "tests_test_counter_value_ctor",        "the value-only constructor (bounds default)", &tests_test_counter_value_ctor },
                    { "tests_test_counter_value_bounds_ctor", "the value-plus-bounds constructor", &tests_test_counter_value_bounds_ctor },
                }
            },
            dt::block_spec{
                "operations",
                "increment / decrement with bounds clamping, reset / reset_all.",
                {
                    { "tests_test_counter_increment_within_bounds",     "a plain increment moves the value and succeeds", &tests_test_counter_increment_within_bounds },
                    { "tests_test_counter_increment_overshoot_clamps",  "an overshooting increment clamps to max and reports it", &tests_test_counter_increment_overshoot_clamps },
                    { "tests_test_counter_decrement_within_bounds",     "a plain decrement moves the value and succeeds", &tests_test_counter_decrement_within_bounds },
                    { "tests_test_counter_decrement_undershoot_clamps", "an undershooting decrement clamps to min and reports it", &tests_test_counter_decrement_undershoot_clamps },
                    { "tests_test_counter_reset",                       "reset() restores the initial value", &tests_test_counter_reset },
                    { "tests_test_counter_reset_all_recurses",          "reset_all() resets this counter and every owned descendant", &tests_test_counter_reset_all_recurses },
                }
            },
            dt::block_spec{
                "accessors",
                "value / initial / min / max / at_min / at_max.",
                {
                    { "tests_test_counter_value",   "value() reflects the current value", &tests_test_counter_value },
                    { "tests_test_counter_initial", "initial() returns the construction value and is immutable", &tests_test_counter_initial },
                    { "tests_test_counter_min",     "min() returns the lower bound", &tests_test_counter_min },
                    { "tests_test_counter_max",     "max() returns the upper bound", &tests_test_counter_max },
                    { "tests_test_counter_at_min",  "at_min() reflects whether the value is at or below the lower bound", &tests_test_counter_at_min },
                    { "tests_test_counter_at_max",  "at_max() reflects whether the value is at or above the upper bound", &tests_test_counter_at_max },
                }
            },
            dt::block_spec{
                "tally",
                "test_tally: test counters kept by key type.",
                {
                    { "tests_test_tally_track",               "track() makes one counter per key, at zero, at a stable address", &tests_test_tally_track },
                    { "tests_test_tally_increment_and_count", "record() and increment() track as they must; count() and find() read without tracking", &tests_test_tally_increment_and_count },
                    { "tests_test_tally_merge",               "merge() adds another tally's movement key by key, tracking new keys", &tests_test_tally_merge },
                    { "tests_test_tally_reset_and_clear",     "reset() returns counts to zero and keeps keys; clear() forgets them; names report", &tests_test_tally_reset_and_clear },
                }
            },
            dt::block_spec{
                "event",
                "test_counter_event.hpp: handlers moving a test counter or a tally key per occurrence.",
                {
                    { "tests_test_counter_event_each_occurrence", "a test counter's handler moves it by its step per occurrence, and no other event's", &tests_test_counter_event_each_occurrence },
                    { "tests_test_counter_event_operations",      "counter_event.hpp's handler on test counters: decrement, add-the-payload, reset", &tests_test_counter_event_operations },
                    { "tests_test_counter_event_unbind",          "the dispatcher's handler id unbinds a test counter's handler", &tests_test_counter_event_unbind },
                    { "tests_test_tally_event",                   "a tally handler records its key per occurrence, tracking it as it must", &tests_test_tally_event },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_COUNTER_TESTS_
