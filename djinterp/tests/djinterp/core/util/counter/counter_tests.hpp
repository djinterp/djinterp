/*******************************************************************************
* djinterp [test]                                              counter_tests.hpp
*
* djinterp counter unit-test header:
*   Declarations, helper types, and helper macros for the counter.hpp unit
* suite. Each test is a parameterless `bool tests_*()` living flat in
* `djinterp::testing`; a test returns true on success and false on the first
* failed check.
*
*   The suite drives a single `counter<std::int64_t>` specialization (aliased
* `ctr`) through every branch: increment/decrement bound clamping, reset and
* recursive reset_all, the bound-query accessors, and both the owning
* (by-value) and observing (by-pointer) child models.
*
*   PORTABILITY:
*   This header pulls in counter.hpp (which provides the namespace macros and
* the portable noexcept specifier). Requires C++17 or later.
*
*
* path:      /tests/djinterp/core/util/counter/counter_tests.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.06.24
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_UTILITY_COUNTER_TESTS_
#define DJINTERP_UTILITY_COUNTER_TESTS_ 1

// std
#include <cstdint>
#include <cstdio>
#include <limits>
#include <type_traits>
// djinterp
#include "../../../../../inc/djinterp/core/util/counter/counter.hpp"


// D_CTR_CHECK
//   macro: evaluates a boolean condition inside a counter test. on failure it
// prints the failing expression and source location, then returns false from
// the enclosing test (which the runner reports as a FAIL). a trailing
// semicolon is expected at each call site.
#define D_CTR_CHECK(_condition)                                          \
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
// counter-under-test specialization
// -----------------------------------------------------------------

// ctr
//   type: the counter specialization exercised throughout the suite. it fixes
// the value type to std::int64_t (the template's own default), so the entire
// suite shares a single instantiation and the bound arithmetic stays in a
// signed 64-bit domain.
using ctr = counter<std::int64_t>;


// -----------------------------------------------------------------
// bound sentinels (readability)
// -----------------------------------------------------------------

// k_lowest
//   value: the lower sentinel a default-constructed counter reports from
// min() (the lowest representable value_type).
constexpr std::int64_t k_lowest = std::numeric_limits<std::int64_t>::lowest();

// k_highest
//   value: the upper sentinel a default-constructed counter reports from
// max() (the largest representable value_type).
constexpr std::int64_t k_highest = std::numeric_limits<std::int64_t>::max();


// -----------------------------------------------------------------
// test declarations
// -----------------------------------------------------------------

// constructors (counter_tests_constructors.cpp)
bool tests_counter_default_ctor();
bool tests_counter_value_ctor();
bool tests_counter_value_ctor_default_bounds();
bool tests_counter_value_ctor_min_only();
bool tests_counter_negative_bounds();

// operations: increment / decrement / reset / reset_all
// (counter_tests_operations.cpp)
bool tests_counter_increment();
bool tests_counter_increment_amount();
bool tests_counter_increment_to_max();
bool tests_counter_increment_past_max();
bool tests_counter_decrement();
bool tests_counter_decrement_amount();
bool tests_counter_decrement_to_min();
bool tests_counter_decrement_past_min();
bool tests_counter_increment_decrement_sequence();
bool tests_counter_reset();
bool tests_counter_reset_nonzero_initial();
bool tests_counter_reset_preserves_children();
bool tests_counter_reset_all_recurses();
bool tests_counter_reset_all_leaf();
bool tests_counter_reset_all_preserves_observed();

// accessors: value / initial / min / max / at_min / at_max
// (counter_tests_accessors.cpp)
bool tests_counter_value();
bool tests_counter_initial();
bool tests_counter_min();
bool tests_counter_max();
bool tests_counter_at_min_equal();
bool tests_counter_at_min_below();
bool tests_counter_at_min_above();
bool tests_counter_at_max_equal();
bool tests_counter_at_max_above();
bool tests_counter_at_max_below();
bool tests_counter_const_accessors();
bool tests_counter_default_bounds_extremes();

// children, owning (counter_tests_children.cpp)
bool tests_counter_add_child();
bool tests_counter_add_child_with_bounds();
bool tests_counter_multiple_children();
bool tests_counter_child_access();
bool tests_counter_child_access_const();
bool tests_counter_child_count();
bool tests_counter_children_independent();
bool tests_counter_copy_deep_copies_children();

// children, non-owning / observed (counter_tests_observed.cpp)
bool tests_counter_observe();
bool tests_counter_observe_multiple();
bool tests_counter_observed_out_of_range();
bool tests_counter_observed_live_view();
bool tests_counter_observed_count();
bool tests_counter_observe_non_owning();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_UTILITY_COUNTER_TESTS_
