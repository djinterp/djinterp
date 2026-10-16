/*******************************************************************************
* djinterp [testing]                          container_storage_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* container_storage_traits.hpp unit-test suite.  The section translation units
* (container_storage_traits_tests_*.cpp) DEFINE the tests_* functions declared
* here; the runner (in the config tree) calls them.  Every test function is flat
* in djinterp::testing and returns true on success.
*
*   MECHANISM.  The storage traits are compile-time facts, so each check compares
* a constant trait reading against its expected result.  A failing check prints
* the offending expression and drives the function's bool result to false, which
* the runner reports and turns into a non-zero exit code - the runtime contract
* the DTest runner is built around.
*
*   SIGNALS AND SITING.  storage_duration_of resolves a container's siting from
* two structural signals - static (a member `extent` or a std::tuple_size
* specialisation) and dynamic (an allocator_type or a reserve()) - fed to
* make_storage_duration (static-only -> static, dynamic-only -> dynamic, BOTH ->
* hybrid, NEITHER -> unknown), UNLESS the type opts in via a
* storage_duration_category member (highest priority).  The standard containers
* only ever raise one signal, so custom types cover the rest:
*     - extent_only            a static `extent`            -> static (extent route)
*     - reserve_only           a reserve(), no allocator     -> dynamic (reserve route)
*     - hybrid_shape           extent AND reserve            -> hybrid (SHAPE-synthesised)
*     - optin_dynamic_override extent + opt-in = dynamic      -> dynamic (opt-in beats shape)
*     - optin_hybrid           extent + opt-in = hybrid       -> hybrid (via opt-in)
*     - optin_static_no_size   opt-in = static, NO size       -> static, but its size is
*                              runtime-only, so it is the ONE type whose
*                              static_storage_implies_constexpr_size is FALSE
*     - no_signal              a value_type, no storage signal -> unknown
*   std::array / std::pair / std::tuple supply the tuple_size static route.
*
*
* path:      /tests/djinterp/core/container/traits/container_storage_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.16
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_STORAGE_TRAITS_TESTS_
#define DJINTERP_CONTAINER_STORAGE_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/core/container/traits)
#include "../../../../../inc/djinterp/core/container/traits/container_storage_traits.hpp"


// NS_TESTING / D_KEYWORD_TESTING may not be defined by the core headers; mirror
// the defensive guard test_common.hpp uses so this header stands alone.
#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_STG_CHECK
//   macro: the per-suite runtime check (unique letters: STG = SToraGe).  Records
// a failure into the enclosing function's `_stg_ok` accumulator and prints the
// offending expression.  Each `_cond` is a compile-time-constant reading, so the
// check is deterministic.  Always pass the condition already wrapped in
// parentheses so interior template commas (map<int,int>, array<int,N>,
// pair<int,int>) survive macro argument splitting.
#define D_STG_CHECK(_cond)                                                    \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _stg_ok = false;                                                  \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// stg_cstr_eq
//   helper: constexpr C-string equality, so the duration_name spellings can be
// checked directly (std::strcmp is not constexpr before C++23).
constexpr bool
stg_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : stg_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper container-shaped types (each isolates a signal / siting branch)
// ===========================================================================

// extent_only -> static (the `extent` static-signal route)
struct extent_only
{
    static constexpr std::size_t extent = 8;
};

// reserve_only -> dynamic (the reserve() dynamic-signal route, no allocator)
struct reserve_only
{
    using value_type = int;
    void reserve(std::size_t) {}
};

// hybrid_shape -> hybrid, SYNTHESISED from BOTH shape signals (extent + reserve)
struct hybrid_shape
{
    static constexpr std::size_t extent = 4;
    void reserve(std::size_t) {}
};

// optin_dynamic_override -> dynamic: the opt-in overrides a static shape signal
struct optin_dynamic_override
{
    static constexpr std::size_t extent = 4;
    static constexpr storage_duration storage_duration_category =
        storage_duration::dynamic_storage;
};

// optin_hybrid -> hybrid via the opt-in (extent keeps its size compile-time)
struct optin_hybrid
{
    static constexpr std::size_t extent = 4;
    static constexpr storage_duration storage_duration_category =
        storage_duration::hybrid_storage;
};

// optin_static_no_size -> static via the opt-in, but with a RUNTIME-only size
// (no extent / tuple_size / size()): the one type whose siting<->size coherence
// fails - static_storage_implies_constexpr_size is FALSE.
struct optin_static_no_size
{
    static constexpr storage_duration storage_duration_category =
        storage_duration::static_storage;
};

// no_signal -> unknown (looks container-ish, but raises no storage signal)
struct no_signal
{
    using value_type = int;
};


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Storage-shape signals --
bool tests_has_static_storage_signal();
bool tests_has_dynamic_storage_signal();

// -- II.  The container siting classifier --
bool tests_storage_duration_of();
bool tests_storage_duration_of_companions();

// -- III. Container siting predicates --
bool tests_is_static_storage_container();
bool tests_is_dynamic_storage_container();
bool tests_is_hybrid_storage_container();
bool tests_has_static_storage_component_container();
bool tests_has_dynamic_storage_component_container();

// -- IV.  Cross-axis entailment --
bool tests_static_storage_implies_constexpr_size();

// -- V.   Aggregate snapshot + variable-template companions --
bool tests_container_storage_class();
bool tests_storage_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_STORAGE_TRAITS_TESTS_
