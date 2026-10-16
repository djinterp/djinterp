/*******************************************************************************
* djinterp [testing]                       container_conversion_traits_tests.hpp
*
*   Declarations, shared helper functions, and helper container TYPES for the
* container_conversion_traits.hpp unit-test suite.  The section translation
* units (container_conversion_traits_tests_*.cpp) DEFINE the tests_* functions
* declared here; the runner (in the config tree) calls them.  Every test
* function is flat in djinterp::testing and returns true on success.
*
*   MECHANISM.  The traits under test are compile-time facts, so each check
* compares a constant trait value against its expected result.  A failing
* check prints the offending expression and drives the function's bool result
* to false, which the runner reports and turns into a non-zero exit code - the
* runtime contract the DTest runner is built around.
*
*   HELPER TYPES.  Two custom container-shaped types exercise branches the
* standard library cannot reach on its own:
*     - `opaque`      an element type with NO conversion to/from int, so a
*                     std::vector<opaque> is element-INCOMPATIBLE with a
*                     std::vector<int> (the `none`-via-incompatible path).
*     - `sorted_vec`  a contiguous, runtime-lifetime, unbounded int sequence
*                     that differs from std::vector<int> on EXACTLY one axis -
*                     discipline - via an opt-in `sorted_invariant`.  It is the
*                     only way to drive is_reinterpretation_helper's
*                     "discipline is strictly GREATER" branch (a strictly
*                     stronger From viewed through a weaker To).
*
*
* path:      /tests/djinterp/core/container/traits/container_conversion_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.15
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_CONVERSION_TRAITS_TESTS_
#define DJINTERP_CONTAINER_CONVERSION_TRAITS_TESTS_ 1

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
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/core/container/traits)
#include "../../../../../inc/djinterp/core/container/traits/container_conversion_traits.hpp"


// NS_TESTING / D_KEYWORD_TESTING may not be defined by the core headers; mirror
// the defensive guard test_common.hpp uses so this header stands alone.
#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_CCV_CHECK
//   macro: the per-suite runtime check (unique letters: CCV = Container
// Conversion traits).  Records a failure into the enclosing function's
// `_ccv_ok` accumulator and prints the offending expression, so the runner's
// FAIL line is actionable.  Each `_cond` is a compile-time-constant trait
// reading, so the check is deterministic.
#define D_CCV_CHECK(_cond)                                                    \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _ccv_ok = false;                                                  \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// ccv_cstr_eq
//   helper: constexpr C-string equality, so the conversion_tier_name spellings
// can be checked directly (std::strcmp is not constexpr before C++23).  Single-
// return recursive form, valid under the C++11 constexpr rules.
constexpr bool
ccv_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : ccv_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper container-shaped types
// ===========================================================================

// opaque
//   type: an element with no conversion to or from int.  A std::vector<opaque>
// is therefore element-INCOMPATIBLE with a std::vector<int>, and stays flat
// (opaque is not container-shaped: no value_type, no size()).
struct opaque
{
    int m_value;
};

// sorted_vec
//   type: a contiguous, mutable, runtime-lifetime, UNBOUNDED sequence of int
// that agrees with std::vector<int> on every conversion axis EXCEPT discipline.
//
//   - value_type / size() / begin()/end() / data(): a container-shaped,
//     order-carrying (no key_type) sequence with an array_like layout
//     (data() + random-access int* iterator) and non-const iteration.
//   - the user-provided destructor makes it a non-literal (runtime-lifetime)
//     type, and size() is deliberately NOT constexpr, so BOTH its size- and
//     contents-lifetimes are runtime - matching std::vector<int>.
//   - `sorted_invariant` opts the type into the sequence-level `sorted`
//     restriction, so its overlay strictly SUBSUMES a plain sequence's: the
//     discipline relation against std::vector<int> is `greater`.
//
//   The net effect: sorted_vec -> std::vector<int> is a VIEW driven by the
// discipline-greater branch, while std::vector<int> -> sorted_vec is `none`
// (compatible elements, but neither a reinterpretation nor a construction path,
// as sorted_vec exposes no range constructor or range insert).
struct sorted_vec
{
    using value_type = int;

    static constexpr bool sorted_invariant = true;   // opt-in sigma

    int         m_buf[8];
    std::size_t m_size;

    ~sorted_vec() noexcept {}                         // non-trivial -> runtime lifetime

    std::size_t size() const noexcept { return m_size; }

    int*        begin()       noexcept { return m_buf; }
    int*        end()         noexcept { return m_buf + m_size; }
    const int*  begin() const noexcept { return m_buf; }
    const int*  end()   const noexcept { return m_buf + m_size; }
    const int*  data()  const noexcept { return m_buf; }
};


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Tier vocabulary (enum + conversion_tier_name) --
bool tests_conversion_tier_vocabulary();

// -- II.  Construction-path detection --
bool tests_range_constructible();
bool tests_range_insertable();
bool tests_has_constructive_path();

// -- III. Per-concern analysis --
bool tests_elements_compatible();
bool tests_conversion_forgets_content();
bool tests_conversion_clamps_capacity();
bool tests_conversion_changes_topology();

// -- IV.  Conversion classification --
bool tests_conversion_tier_of_view();
bool tests_conversion_tier_of_constructive();
bool tests_conversion_tier_of_lossy();
bool tests_conversion_tier_of_structural();
bool tests_conversion_tier_of_none();
bool tests_conversion_tier_of_cvref();
bool tests_conversion_predicates();

// -- IV.  Aggregate + variable-template companions --
bool tests_conversion_class();
bool tests_conversion_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONVERSION_TRAITS_TESTS_
