/*******************************************************************************
* djinterp [testing]                           container_filter_traits_tests.hpp
*
*   Declarations, shared helpers, and helper container TYPES for the
* container_filter_traits.hpp unit-test suite.  The section translation units
* (container_filter_traits_tests_*.cpp) DEFINE the tests_* functions declared
* here; the runner (in the config tree) calls them.  Every test function is
* flat in djinterp::testing and returns true on success.
*
*   MECHANISM.  The Filterability traits are compile-time facts, so each check
* compares a constant trait reading against its expected result.  A failing
* check prints the offending expression and drives the function's bool result
* to false, which the runner reports and turns into a non-zero exit code - the
* runtime contract the DTest runner is built around.
*
*   HELPER TYPES.  Five custom container-shaped types exercise branches the
* standard library cannot reach on its own:
*     - native_filterable   a filterable source that exposes its OWN
*                           filter(bool(*)(const value_type&)) primitive; the
*                           only way to reach has_native_filter / the `native`
*                           strategy (no std container has a filter() member).
*     - native_wrong_sig    a filterable source whose filter() has the WRONG
*                           signature, confirming native detection is signature-
*                           precise (has_native_filter is FALSE, and the
*                           strategy falls through to `indexed`).
*     - sorted_seq          an ordered, filterable sequence that opts into the
*                           sorted invariant, the only way to make
*                           filter_preserves_sortedness TRUE (a std::set is
*                           `monotone`, not `sorted`, on the sortedness axis).
*     - value_only          exposes a value_type but is NOT iterable, so it is
*                           not a filter source (the iterability conjunct fails).
*     - iterable_no_value   iterable (begin()/end()) but exposes NO value_type,
*                           so it is not a source either (the value_type
*                           conjunct fails) - isolating the other half of the
*                           is_filter_source conjunction.
*
*
* path:      /tests/djinterp/core/container/traits/container_filter_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.15
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_FILTER_TRAITS_TESTS_
#define DJINTERP_CONTAINER_FILTER_TRAITS_TESTS_ 1

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
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/core/container/traits)
#include "../../../../../inc/djinterp/core/container/traits/container_filter_traits.hpp"


// NS_TESTING / D_KEYWORD_TESTING may not be defined by the core headers; mirror
// the defensive guard test_common.hpp uses so this header stands alone.
#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_CF_CHECK
//   macro: the per-suite runtime check (unique letters: CF = Container Filter).
// Records a failure into the enclosing function's `_cf_ok` accumulator and
// prints the offending expression, so the runner's FAIL line is actionable.
// Each `_cond` is a compile-time-constant trait reading, so the check is
// deterministic.  Always pass the condition already wrapped in parentheses so
// interior template commas (map<int,int>, array<int,N>, is_same<A,B>) survive
// macro argument splitting.
#define D_CF_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _cf_ok = false;                                                   \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// cf_cstr_eq
//   helper: constexpr C-string equality, so the *_name spellings can be checked
// directly (std::strcmp is not constexpr before C++23).  Single-return
// recursive form, valid under the C++11 constexpr rules.
constexpr bool
cf_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : cf_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper container-shaped types
// ===========================================================================

// native_filterable
//   type: a filterable source (iterable + value_type + push_back) that ALSO
// exposes its own filter(bool(*)(const value_type&)) primitive.  It is the only
// way to reach has_native_filter == true and the `native` strategy, which is
// selected ahead of every iteration-based realization.  The user-provided
// destructor and default-initialised size member keep it a runtime-lifetime
// type (so its filter stage is `runtime`), and size() is deliberately not
// constexpr.
struct native_filterable
{
    using value_type = int;

    int         m_buf[4];
    std::size_t m_size = 0;

    ~native_filterable() noexcept {}

    std::size_t size() const noexcept { return m_size; }

    int*        begin()       noexcept { return m_buf; }
    int*        end()         noexcept { return m_buf + m_size; }
    const int*  begin() const noexcept { return m_buf; }
    const int*  end()   const noexcept { return m_buf + m_size; }

    void        push_back(const int&) {}                     // build capability
    void        filter(bool(*)(const int&)) const {}         // native primitive (exact shape)
};

// native_wrong_sig
//   type: a filterable source whose filter() member takes the WRONG argument
// (an int, not a bool(*)(const value_type&)).  has_native_filter must be FALSE
// - a filter() member of the wrong shape is not the native primitive - so the
// strategy falls through to `indexed` (it is contiguous and random-access).
struct native_wrong_sig
{
    using value_type = int;

    int         m_buf[4];
    std::size_t m_size = 0;

    ~native_wrong_sig() noexcept {}

    std::size_t size() const noexcept { return m_size; }

    int*        begin()       noexcept { return m_buf; }
    int*        end()         noexcept { return m_buf + m_size; }
    const int*  begin() const noexcept { return m_buf; }
    const int*  end()   const noexcept { return m_buf + m_size; }
    const int*  data()  const noexcept { return m_buf; }     // -> indexed with random-access iterator

    void        push_back(const int&) {}
    void        filter(int) const {}                         // WRONG signature: not detected as native
};

// sorted_seq
//   type: an ordered, filterable, contiguous int sequence that opts into the
// sorted invariant (static sorted_invariant == true).  On the sortedness axis
// this makes it `sorted` (not merely `order_dependent`), so
// filter_preserves_sortedness is TRUE - a verdict no standard container yields
// (a std::set is classified `monotone`).  Runtime-lifetime, like the others.
struct sorted_seq
{
    using value_type = int;

    static constexpr bool sorted_invariant = true;           // opt-in sigma

    int         m_buf[4];
    std::size_t m_size = 0;

    ~sorted_seq() noexcept {}

    std::size_t size() const noexcept { return m_size; }

    int*        begin()       noexcept { return m_buf; }
    int*        end()         noexcept { return m_buf + m_size; }
    const int*  begin() const noexcept { return m_buf; }
    const int*  end()   const noexcept { return m_buf + m_size; }
    const int*  data()  const noexcept { return m_buf; }

    void        push_back(const int&) {}
};

// value_only
//   type: exposes a value_type but no begin()/end() - it LOOKS container-ish
// yet is not iterable, so is_filter_source is FALSE (the iterability conjunct
// fails even though the value_type conjunct holds).
struct value_only
{
    using value_type = int;
};

// iterable_no_value
//   type: iterable (begin()/end()) but exposes NO value_type alias, so
// element_type_of is void and is_filter_source is FALSE (the value_type
// conjunct fails even though the iterability conjunct holds) - the complement
// of value_only.
struct iterable_no_value
{
    int m_buf[4];

    int*        begin()       noexcept { return m_buf; }
    int*        end()         noexcept { return m_buf + 4; }
    const int*  begin() const noexcept { return m_buf; }
    const int*  end()   const noexcept { return m_buf + 4; }
};


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Capability detection --
bool tests_is_filter_source();
bool tests_is_container_filterable();
bool tests_is_filter_input_only();
bool tests_has_native_filter();

// -- II.  Strategy (the realization) --
bool tests_filter_strategy_name();
bool tests_container_filter_strategy();

// -- III. Stage (when a selection may run) --
bool tests_filter_stage_name();
bool tests_filter_stage_of();

// -- IV.  Preservation --
bool tests_filter_preserves_order();
bool tests_filter_preserves_sortedness();
bool tests_filter_preserves_uniqueness();
bool tests_filter_preserves_capacity_bounds();

// -- V.   Result type --
bool tests_filter_result_type();

// -- VI.  Combined classification + variable-template companions --
bool tests_container_filter_class();
bool tests_filter_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_FILTER_TRAITS_TESTS_
