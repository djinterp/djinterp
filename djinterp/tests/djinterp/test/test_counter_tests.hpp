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
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/core/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "test_counter.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
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


#endif  // !DTEST_SPEC_MODE  (fixtures)


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


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from each test's own /* Verifies ... */ block.
// =========================================================================

inline dt::module_spec
counter_spec()
{
    return dt::module_spec{
        "test_counter",
        "test_counter.hpp - util::counter wrapped with an optional event-dispatch "
        "layer, over a deterministic int instantiation: construction, the "
        "increment / decrement / reset operations with bounds clamping and their "
        "events, the accessors, the owning and observed child counters, and the "
        "event alphabet.",
        {
            dt::block_spec{
                "construction",
                "The four constructors and their initial state.",
                {
                    { "tests_test_counter_default_ctor",              "the default constructor's initial state", &tests_test_counter_default_ctor },
                    { "tests_test_counter_value_ctor",                "the value-only constructor (bounds and handler default)", &tests_test_counter_value_ctor },
                    { "tests_test_counter_value_bounds_ctor",         "the value-plus-bounds constructor (handler defaults)", &tests_test_counter_value_bounds_ctor },
                    { "tests_test_counter_value_bounds_handler_ctor", "the full constructor with an event dispatcher", &tests_test_counter_value_bounds_handler_ctor },
                }
            },
            dt::block_spec{
                "operations",
                "increment / decrement with bounds clamping, reset / reset_all, "
                "and the events they fire.",
                {
                    { "tests_test_counter_increment_within_bounds",    "a plain increment fires on_increment(old, new) and not on_limit", &tests_test_counter_increment_within_bounds },
                    { "tests_test_counter_increment_exact_max_no_limit","landing exactly on max is a success, not a clamp", &tests_test_counter_increment_exact_max_no_limit },
                    { "tests_test_counter_increment_overshoot_clamps", "an overshooting increment clamps to max and fires both events", &tests_test_counter_increment_overshoot_clamps },
                    { "tests_test_counter_increment_at_max_noop",      "incrementing an already-maxed counter fires on_limit but not on_increment", &tests_test_counter_increment_at_max_noop },
                    { "tests_test_counter_increment_zero_noop",        "a zero increment is neither a change nor a clamp", &tests_test_counter_increment_zero_noop },
                    { "tests_test_counter_decrement_within_bounds",    "a plain decrement fires on_decrement(old, new) and not on_limit", &tests_test_counter_decrement_within_bounds },
                    { "tests_test_counter_decrement_undershoot_clamps","an undershooting decrement clamps to min and fires both events", &tests_test_counter_decrement_undershoot_clamps },
                    { "tests_test_counter_decrement_at_min_noop",      "decrementing an already-minned counter fires on_limit but not on_decrement", &tests_test_counter_decrement_at_min_noop },
                    { "tests_test_counter_decrement_zero_noop",        "a zero decrement is neither a change nor a clamp", &tests_test_counter_decrement_zero_noop },
                    { "tests_test_counter_reset_fires_on_reset",       "reset() restores the initial value and fires on_reset with the prior value", &tests_test_counter_reset_fires_on_reset },
                    { "tests_test_counter_reset_all_leaf",             "reset_all() on a childless counter (the empty-recursion case)", &tests_test_counter_reset_all_leaf },
                    { "tests_test_counter_reset_all_recurses",         "reset_all() resets this counter and every owned descendant, each firing on_reset through the inherited dispatcher", &tests_test_counter_reset_all_recurses },
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
                "children_owning",
                "The owned child counters, held by value.",
                {
                    { "tests_test_counter_add_child",                "add_child() with defaults, including dispatcher inheritance", &tests_test_counter_add_child },
                    { "tests_test_counter_add_child_with_bounds",    "add_child(initial, min, max), including dispatcher inheritance", &tests_test_counter_add_child_with_bounds },
                    { "tests_test_counter_multiple_children",        "several children coexist with their own configuration", &tests_test_counter_multiple_children },
                    { "tests_test_counter_child_access",             "the non-const child() accessor returns a mutable reference", &tests_test_counter_child_access },
                    { "tests_test_counter_child_access_const",       "the const child() overload via a const reference", &tests_test_counter_child_access_const },
                    { "tests_test_counter_child_count",              "child_count() tracks additions", &tests_test_counter_child_count },
                    { "tests_test_counter_children_independent",     "children hold independent state", &tests_test_counter_children_independent },
                    { "tests_test_counter_copy_deep_copies_children","the owning contract - children are held by value, so a copy owns an independent subtree", &tests_test_counter_copy_deep_copies_children },
                }
            },
            dt::block_spec{
                "children_observed",
                "The non-owning observed counters.",
                {
                    { "tests_test_counter_observe",              "a single observe() registration", &tests_test_counter_observe },
                    { "tests_test_counter_observe_multiple",     "several observed counters are tracked in order", &tests_test_counter_observe_multiple },
                    { "tests_test_counter_observed_out_of_range","the bounds check in observed()", &tests_test_counter_observed_out_of_range },
                    { "tests_test_counter_observed_live_view",   "the observed pointer is a live view of the external counter", &tests_test_counter_observed_live_view },
                    { "tests_test_counter_observed_count",       "observed_count() tracks registrations", &tests_test_counter_observed_count },
                    { "tests_test_counter_observe_non_owning",   "observed counters are non-owning - reset_all() does not touch them", &tests_test_counter_observe_non_owning },
                }
            },
            dt::block_spec{
                "events",
                "The event alphabet, handler access, and dispatch on/off.",
                {
                    { "tests_test_counter_event_tag_names",         "each nested event tag reports its documented name", &tests_test_counter_event_tag_names },
                    { "tests_test_counter_event_tag_args",          "the argument tuple of each event tag (a compile-time contract)", &tests_test_counter_event_tag_args },
                    { "tests_test_counter_no_handler_noop",         "the zero-overhead path - with no dispatcher, every operation runs and dispatches nothing", &tests_test_counter_no_handler_noop },
                    { "tests_test_counter_handler_accessor",        "handler() reports the attached dispatcher", &tests_test_counter_handler_accessor },
                    { "tests_test_counter_set_handler_attach_detach","set_handler() attaches and detaches dispatch at runtime", &tests_test_counter_set_handler_attach_detach },
                    { "tests_test_counter_set_handler_swaps",       "set_handler() can redirect dispatch from one dispatcher to another", &tests_test_counter_set_handler_swaps },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_COUNTER_TESTS_
