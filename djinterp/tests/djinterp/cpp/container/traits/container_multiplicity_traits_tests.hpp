/*******************************************************************************
* djinterp [testing]                     container_multiplicity_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* container_multiplicity_traits.hpp unit-test suite.  The section translation
* units (container_multiplicity_traits_tests_*.cpp) DEFINE the tests_* functions
* declared here; the runner (in the config tree) calls them.  Every test
* function is flat in djinterp::testing and returns true on success.
*
*   MECHANISM.  The multiplicity traits are compile-time facts, so each check
* compares a constant trait reading against its expected result.  A failing
* check prints the offending expression and drives the function's bool result to
* false, which the runner reports and turns into a non-zero exit code - the
* runtime contract the DTest runner is built around.
*
*   HELPER TYPES.  multiplicity_kind_of resolves by a precedence: a non-countable
* type is unknown; an opt-in `multiplicity` bound is authoritative; static
* interval bounds are unique; a comparator-less container is a sequence; and a
* keyed container is unique or an unbounded multiset by its insert signature.
* No standard container reaches the bounded_multiset kind, the opt-in path, or
* the interval path, so custom types supply them:
*     - DIntervalSeq        static lower/upper bounds, non-keyed  -> unique
*                           (interval wins over the sequence default)
*     - DIntervalKeyed      interval bounds AND a key_type        -> unique
*                           (interval wins over the keyed branches)
*     - DOptinUnique        multiplicity = 1                      -> unique
*     - DOptinBounded       multiplicity = 5   (1 < m < inf)      -> bounded_multiset
*     - DOptinUnbounded     multiplicity = inf                    -> unbounded_multiset
*     - DOptinZero          multiplicity = 0   (below the floor)  -> unknown, bound 0
*     - DOptinOverInterval multiplicity = 5 AND interval bounds  -> bounded_multiset
*                           (the opt-in wins over the interval)
*     - DKeyedNoSize       key_type + value_type but NO size()   -> unknown
*                           (the countability guard precedes the keyed logic)
*
*
* path:      /tests/djinterp/cpp/container/traits/container_multiplicity_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.16
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_TESTS_
#define DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_TESTS_ 1

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
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/cpp/container/traits)
#include "../../../../../inc/djinterp/cpp/container/traits/container_multiplicity_traits.hpp"


// NS_TESTING / D_KEYWORD_TESTING may not be defined by the core headers; mirror
// the defensive guard test_common.hpp uses so this header stands alone.
#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_CMP_CHECK
//   macro: the per-suite runtime check (unique letters: CMP = Container
// MultiPlicity).  Records a failure into the enclosing function's `_cmp_ok`
// accumulator and prints the offending expression.  Each `_cond` is a
// compile-time-constant trait reading, so the check is deterministic.  Always
// pass the condition already wrapped in parentheses so interior template commas
// (map<int,int>, array<int,N>, is_same<A,B>) survive macro argument splitting.
#define D_CMP_CHECK(_cond)                                                    \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _cmp_ok = false;                                                  \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// cmp_cstr_eq
//   helper: constexpr C-string equality, so the kind_name spellings can be
// checked directly (std::strcmp is not constexpr before C++23).
constexpr
bool cmp_cstr_eq(
    const char* _a,
    const char* _b
) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : cmp_cstr_eq(_a + 1, _b + 1));
}

// the inf sentinel, spelled locally so the helper types below do not depend on
// the name's visibility (it equals djinterp::unbounded_multiplicity).
constexpr std::size_t cmp_inf = static_cast<std::size_t>(-1);


// ===========================================================================
// Helper container-shaped types (each isolates one verdict branch / edge)
// ===========================================================================

// DIntervalSeq -> unique  (static interval bounds win over the sequence default)
struct DIntervalSeq
{
    using value_type = int;
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;
    std::size_t size() const noexcept { return 0; }
};

// DIntervalKeyed -> unique  (interval bounds win over the keyed branches)
struct DIntervalKeyed
{
    using value_type = int;
    using key_type   = int;
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;
    std::size_t size() const noexcept { return 0; }
};

// DOptinUnique -> unique  (opt-in multiplicity = 1)
struct DOptinUnique
{
    using value_type = int;
    static constexpr std::size_t multiplicity = 1;
    std::size_t size() const noexcept { return 0; }
};

// DOptinBounded -> bounded_multiset  (opt-in multiplicity = 5, the only route)
struct DOptinBounded
{
    using value_type = int;
    static constexpr std::size_t multiplicity = 5;
    std::size_t size() const noexcept { return 0; }
};

// DOptinUnbounded -> unbounded_multiset  (opt-in multiplicity = inf)
struct DOptinUnbounded
{
    using value_type = int;
    static constexpr std::size_t multiplicity = cmp_inf;   // = unbounded_multiplicity
    std::size_t size() const noexcept { return 0; }
};

// DOptinZero -> unknown, bound 0  (opt-in multiplicity = 0, below the floor)
struct DOptinZero
{
    using value_type = int;
    static constexpr std::size_t multiplicity = 0;
    std::size_t size() const noexcept { return 0; }
};

// DOptinOverInterval -> bounded_multiset  (opt-in wins over interval bounds)
struct DOptinOverInterval
{
    using value_type = int;
    static constexpr std::size_t multiplicity = 5;
    static constexpr int lower_bound = 0;
    static constexpr int upper_bound = 9;
    std::size_t size() const noexcept { return 0; }
};

// DKeyedNoSize -> unknown  (the countability guard precedes the keyed logic)
struct DKeyedNoSize
{
    using value_type = int;
    using key_type   = int;
    // no size() -> not countable
};


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Structural signals --
bool tests_is_countable_container();
bool tests_has_interval_bounds_signal();
bool tests_has_unique_insert();
bool tests_has_multiplicity_bound();

// -- II.  Verdict (kind + numeric bound) --
bool tests_multiplicity_kind_of();
bool tests_multiplicity_kind_companions();
bool tests_multiplicity_bound_of();

// -- III. Classification predicates --
bool tests_is_sequence_container();
bool tests_is_unique_container();
bool tests_is_multiset_container();

// -- IV.  Aggregate snapshot + variable-template companions --
bool tests_multiplicity_container_class();
bool tests_multiplicity_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_TESTS_
