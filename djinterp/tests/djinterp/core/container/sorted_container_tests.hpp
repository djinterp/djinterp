/*******************************************************************************
* djinterp [testing]                                  sorted_container_tests.hpp
*
*   DTest suite header for sorted_container.hpp - the SORTEDNESS axis.  Two
* faces gated by DTEST_SPEC_MODE: normal mode exposes the SFINAE-probe trait
* and
* the check macro; spec mode exposes sorted_container_spec().
*
*   sorted_container.hpp re-exports the sortedness classifiers from its traits
* header and defines exactly one thing of its own: is_sorted_range, the
* instance-level check for an ORDERED container (both the default-comparator
* and
* the supplied-comparator overloads).  That function is what this suite tests -
* its runtime verdict across container kinds and edge cases, and the enable_if
* gate that restricts it to ordered containers.  The re-exported classifier
* surface belongs to the traits header's own suite and is not duplicated here.
*
*
* path:      /tests/djinterp/core/container/sorted_container_tests.hpp
* link(s):   TBA
* author(s): DTest suite                                     created: 2026.07.24
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORTED_CONTAINER_TESTS_
#define DJINTERP_TESTS_SORTED_CONTAINER_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
// djinterp core is ALWAYS first and unconditional.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/container/sorted_container.hpp"                     // the header under test
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"          // module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); unconditional so the spec
// provider can name dt::module_spec in spec mode.
namespace dt = ::djinterp::test;


// D_SR_CHECK
//   macro: evaluate a predicate once; on failure print the failing expression
// and its location and return false from the enclosing body.  Variadic so a
// top-level comma in a trait expression passes through whole.
#define D_SR_CHECK(...)                                                  \
    do                                                                   \
    {                                                                    \
        if (!(__VA_ARGS__))                                              \
        {                                                                \
            std::printf("    check failed: %s\n    at: %s:%d\n",         \
                        #__VA_ARGS__, __FILE__, __LINE__);               \
            return false;                                                \
        }                                                                \
    }                                                                    \
    while (0)


#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// can_sorted_range
//   trait: true iff is_sorted_range is callable with a single argument of the
// given container type.  It probes the enable_if gate (on
// is_ordered_container),
// so an associative or a non-container reads false without a hard error.  Both
// is_sorted_range overloads share that gate, so the single-argument probe
// characterises the availability of the pair.
template<typename _C,
         typename = void>
struct can_sorted_range
    : std::false_type
{
};

template<typename _C>
struct can_sorted_range<_C,
    std::void_t<decltype(is_sorted_range(std::declval<const _C&>()))>>
    : std::true_type
{
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- declarations - visible in BOTH modes ----------------------------------

// I.   range   (sorted_container_tests_range.cpp)
bool tests_sorted_default_basic();
bool tests_sorted_default_duplicates_and_boundary();
bool tests_sorted_custom_comparator();
bool tests_sorted_across_containers();

// II.  gating   (sorted_container_tests_gating.cpp)
bool tests_sorted_gating_ordered_enabled();
bool tests_sorted_gating_associative_disabled();
bool tests_sorted_gating_non_container_disabled();


// -- spec provider - spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
sorted_container_spec()
{
    return dt::module_spec{
        "sorted_container",
        "The SORTEDNESS axis: the instance-level is_sorted_range check for an "
        "ordered container (default and supplied comparator) and the "
        "enable_if "
        "gate restricting it to ordered containers.",
        {
            dt::block_spec{
                "range",
                "the runtime is_sorted_range verdict across container kinds, "
                "comparators, and edge cases.",
                {
                    { "tests_sorted_default_basic",
                      "is_sorted_range is true for a non-descending sequence, "
                      "empty, and single; false for an out-of-order one.",
                      &tests_sorted_default_basic },
                    { "tests_sorted_default_duplicates_and_boundary",
                      "equal adjacent elements count as non-descending; a "
                      "single "
                      "inversion at either end is rejected.",
                      &tests_sorted_default_duplicates_and_boundary },
                    { "tests_sorted_custom_comparator",
                      "the comparator overload accepts a descending sequence "
                      "under std::greater and rejects an ascending one.",
                      &tests_sorted_custom_comparator },
                    { "tests_sorted_across_containers",
                      "the check runs over list, deque, array, and string, "
                      "distinguishing sorted from unsorted contents.",
                      &tests_sorted_across_containers },
                }
            },
            dt::block_spec{
                "gating",
                "the enable_if gate: available for ordered containers, "
                "removed "
                "for associatives and non-containers.",
                {
                    { "tests_sorted_gating_ordered_enabled",
                      "is_sorted_range is callable on ordered containers "
                      "(vector/list/deque/string/array).",
                      &tests_sorted_gating_ordered_enabled },
                    { "tests_sorted_gating_associative_disabled",
                      "it is NOT callable on any associative - the gate "
                      "follows "
                      "is_ordered_container, which excludes them all.",
                      &tests_sorted_gating_associative_disabled },
                    { "tests_sorted_gating_non_container_disabled",
                      "it is NOT callable on a scalar or a pointer.",
                      &tests_sorted_gating_non_container_disabled },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_SORTED_CONTAINER_TESTS_
