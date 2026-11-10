/*******************************************************************************
* djinterp [testing]                          container_overlay_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* container_overlay_traits.hpp unit-test suite.  The section translation units
* (container_overlay_traits_tests_*.cpp) DEFINE the tests_* functions declared
* here; the runner (in the config tree) calls them.  Every test function is flat
* in djinterp::testing and returns true on success.
*
*   MECHANISM.  The overlay traits are compile-time facts, so each check compares
* a constant trait reading against its expected result.  A failing check prints
* the offending expression and drives the function's bool result to false, which
* the runner reports and turns into a non-zero exit code - the runtime contract
* the DTest runner is built around.
*
*   OVERLAY SYNTHESIS.  overlay_of assembles five restrictions: mu (multiplicity,
* from the multiplicity axis), gamma / delta (capacity / domain, from
* boundedness), eta (keyed - a pair-shaped value_type, local), and sigma (sorted
* - an opt-in `sorted_invariant`, local).  The standard containers cover the
* canonical set / multiset / map / multimap overlays and the unkeyed/keyed and
* unique/repeatable splits, but not the bounded_multiset kind, the domain / opt-in
* capacity signals, the sigma opt-in, or the keyed-conjunction edge, so custom
* types supply them:
*     - DSortedSeq            opt-in sorted_invariant = true    -> sorted (NOT order-blind)
*     - DBoundedMultisetSeq  multiplicity = 5, unkeyed         -> bounded_multiset / multiset
*     - DBoundedMultimapSeq  multiplicity = 5, pair value_type -> bounded_multiset / multimap
*     - DDomainBoundedSeq    static lower/upper bounds          -> unique (interval), capacity
*                             AND domain bounded (an interval carrier: a bounded set)
*     - capacity_bounded_seq  capacity() but no reserve()        -> capacity bounded, sequence
*     - keyed_half            value_type has first_type but NO second_type
*                             -> NOT keyed (eta requires the full pair shape)
*   For eta on a genuine pair sequence, std::vector<std::pair<int,int>> is used
* directly: it is keyed yet - being a sequence - wears the `none` canonical.
*
*
* path:      /tests/djinterp/cpp/container/traits/container_overlay_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.16
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_OVERLAY_TRAITS_TESTS_
#define DJINTERP_CONTAINER_OVERLAY_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include <djinterp/cpp/container/buffer/byte_buffer.hpp>
#include <djinterp/cpp/container/traits/container_overlay_traits.hpp>
#include <djinterp/cpp/modules/test/test_defaults.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// D_OVL_CHECK
//   macro: the per-suite runtime check (unique letters: OVL = OVerLay).  Records
// a failure into the enclosing function's `_ovl_ok` accumulator and prints the
// offending expression.  Each `_cond` is a compile-time-constant reading, so the
// check is deterministic.  Always pass the condition already wrapped in
// parentheses so interior template commas (map<int,int>, array<int,N>,
// pair<int,int>) survive macro argument splitting.
#define D_OVL_CHECK(_cond)                                                    \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _ovl_ok = false;                                                  \
        }                                                                     \
    } while (false)


#endif  // !DTEST_SPEC_MODE
// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only


// ===========================================================================
// Shared helpers
// ===========================================================================

// ovl_cstr_eq
//   helper: constexpr C-string equality, so the *_name spellings can be checked
// directly (std::strcmp is not constexpr before C++23).
constexpr
bool ovl_cstr_eq(
    const char* _a,
    const char* _b
) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : ovl_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper container-shaped types (each isolates a signal / overlay branch)
// ===========================================================================

// DSortedSeq -> sigma (sorted): the only route to a non-order-blind overlay
struct DSortedSeq
{
    using value_type = int;
    static constexpr bool sorted_invariant = true;   // opt-in sigma
    std::size_t size() const noexcept { return 0; }
};

// DBoundedMultisetSeq -> bounded_multiset kind, unkeyed -> canonical multiset
struct DBoundedMultisetSeq
{
    using value_type = int;
    static constexpr std::size_t multiplicity = 5;   // 1 < m < inf
    std::size_t size() const noexcept { return 0; }
};

// DBoundedMultimapSeq -> bounded_multiset kind, keyed -> canonical multimap
struct DBoundedMultimapSeq
{
    using value_type = std::pair<int, int>;          // pair value_type -> eta
    static constexpr std::size_t multiplicity = 5;
    std::size_t size() const noexcept { return 0; }
};

// DDomainBoundedSeq -> static interval bounds: unique (interval) + capacity AND
// domain bounded (a bounded set) - the only route to domain_bounded
struct DDomainBoundedSeq
{
    using value_type = int;
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;
    std::size_t size() const noexcept { return 0; }
};

// capacity_bounded_seq -> capacity() without reserve(): capacity bounded, a
// plain sequence (the capacity/!reserve route to gamma)
struct capacity_bounded_seq
{
    using value_type = int;
    std::size_t size()     const noexcept { return 0; }
    std::size_t capacity() const noexcept { return 0; }
    // no reserve() -> the capacity is a FIXED bound
};

// DOnlyFirstPair -> a would-be pair MISSING second_type
struct DOnlyFirstPair
{
    using first_type = int;
    // no second_type
};

// keyed_half -> value_type has first_type but not second_type -> NOT keyed
struct keyed_half
{
    using value_type = DOnlyFirstPair;
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Restriction vocabulary --
bool tests_restriction_scope_name();
bool tests_restriction_kind_name();
bool tests_scope_of_restriction();

// -- II.  Overlay-specific signals (eta) --
bool tests_is_keyed_container();

// -- III. Overlay descriptor + algebra --
bool tests_overlay_top();
bool tests_is_order_blind();
bool tests_overlay_subsumes();
bool tests_overlay_meet();

// -- IV.  The overlay a container wears --
bool tests_overlay_of();
bool tests_is_order_blind_overlay();
bool tests_wears_overlay();

// -- V.   Canonical overlays --
bool tests_canonical_overlay_name();
bool tests_classify_canonical();
bool tests_canonical_overlay_of();

// -- VI.  Aggregate snapshot + variable-template companions --
bool tests_overlay_container_class();
bool tests_overlay_variable_templates();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// container_overlay_traits_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
container_overlay_traits_spec()
{
    return dt::module_spec{
        "container_overlay_traits",
        "container_overlay_traits.hpp",
        {
            dt::block_spec{
                "vocabulary",
                "Vocabulary",
                {
                    { "tests_restriction_scope_name",
                      "checks "
                      "(ovl_cstr_eq(restriction_scope_name(DRestrictionScope::static_scope",
                      &tests_restriction_scope_name },
                    { "tests_restriction_kind_name",
                      "checks "
                      "(ovl_cstr_eq(restriction_kind_name(DRestrictionKind::multiplicity),",
                      &tests_restriction_kind_name },
                    { "tests_scope_of_restriction",
                      "checks (scope_of_restriction(DRestrictionKind::keyed) "
                      "== DRestrictionScope",
                      &tests_scope_of_restriction },
                }
            },
            dt::block_spec{
                "signals",
                "Signals",
                {
                    { "tests_is_keyed_container",
                      "checks (is_keyed_container<std::map<int, int>>::value)",
                      &tests_is_keyed_container },
                }
            },
            dt::block_spec{
                "algebra",
                "Algebra",
                {
                    { "tests_overlay_top",
                      "checks (t.mult == multiplicity_kind::sequence)",
                      &tests_overlay_top },
                    { "tests_is_order_blind",
                      "checks (is_order_blind(ovl_top))",
                      &tests_is_order_blind },
                    { "tests_overlay_subsumes",
                      "checks (overlay_subsumes(ovl_set, ovl_set))",
                      &tests_overlay_subsumes },
                    { "tests_overlay_meet",
                      "checks (m.mult == multiplicity_kind::unique)",
                      &tests_overlay_meet },
                }
            },
            dt::block_spec{
                "overlay_of",
                "Overlay of",
                {
                    { "tests_overlay_of",
                      "checks (O::mult == multiplicity_kind::sequence)",
                      &tests_overlay_of },
                    { "tests_is_order_blind_overlay",
                      "checks "
                      "(is_order_blind_overlay<std::vector<int>>::value)",
                      &tests_is_order_blind_overlay },
                    { "tests_wears_overlay",
                      "checks (wears_overlay<std::set<int>>(target_top))",
                      &tests_wears_overlay },
                }
            },
            dt::block_spec{
                "canonical",
                "Canonical",
                {
                    { "tests_canonical_overlay_name",
                      "checks "
                      "(ovl_cstr_eq(canonical_overlay_name(canonical_overlay::none), "
                      "\"none",
                      &tests_canonical_overlay_name },
                    { "tests_classify_canonical",
                      "checks (classify_canonical(multiplicity_kind::unique, "
                      "false) == canonical_",
                      &tests_classify_canonical },
                    { "tests_canonical_overlay_of",
                      "checks (canonical_overlay_of<std::set<int>>::value == "
                      "canonical_overlay::s",
                      &tests_canonical_overlay_of },
                }
            },
            dt::block_spec{
                "classification",
                "Classification",
                {
                    { "tests_overlay_container_class",
                      "checks (C::mult == multiplicity_kind::sequence)",
                      &tests_overlay_container_class },
                    { "tests_overlay_variable_templates",
                      "checks (is_keyed_container_v<std::map<int, int>> == "
                      "is_keyed_container<std",
                      &tests_overlay_variable_templates },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OVERLAY_TRAITS_TESTS_
