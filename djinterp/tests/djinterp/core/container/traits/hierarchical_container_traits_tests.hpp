/*******************************************************************************
* djinterp [testing]                     hierarchical_container_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* hierarchical_container_traits.hpp unit-test suite.  A container factors as
* T = tau + F[T]: a LEAF summand and an optional NODE summand F[T] (a sub-container);
* the depth d counts nodes, and a container is HIERARCHICAL when d >= 2.  This header
* owns the structure axis's shared machinery - the container-shape guard, the depth
* recursion, the node_type tell, the structure_category tag, and the structure_kind
* summary - reached by combining signals of differing strength: a container-shaped
* node_type (strong) or a hierarchical tag decides outright; otherwise, absent a flat
* tag, the value_type-chain depth heuristic (weak) decides.
*
*   MECHANISM.  The traits are compile-time facts; each check compares a constant
* reading against its expected result.  A failing check prints the offending
* expression and drives the function's bool result to false, which the runner turns
* into a non-zero exit code.
*
*   HELPER TYPES.  Custom container-shaped fixtures (a value_type + a const size())
* pin every signal path and precedence tie:
*     - flat_leaf         value_type=int                     -> flat, depth 1
*     - strlike           value_type=char (a container-shaped leaf, like a string)
*     - nested_box        value_type=flat_leaf               -> hierarchical, depth 2
*     - tagged_flat_str   value_type=strlike + flat tag       -> flat tag suppresses the
*                         depth-2 heuristic
*     - node_box          a container-shaped node_type        -> hierarchical outright
*                         (strong tell) even at value_type-depth 1
*     - node_box_flat_tagged  node_type + flat tag            -> STILL hierarchical
*                         (a flat tag cannot un-nest a declared node summand)
*     - hier_tagged       value_type=int + hierarchical tag   -> hierarchical at depth 1
*     - node_handle_box   a node_type that is NOT container-shaped (no size())
*                         -> has_node_summand false (the node-handle exclusion)
*   The standard containers and the vecN aliases supply the realistic and deep cases:
* std::vector/std::string/std::set/std::map are flat; vec2/vec3 nest to depth 2/3;
* std::forward_list (no size()) and int are non-containers.
*
*
* path:      /tests/djinterp/core/container/traits/hierarchical_container_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_HIERARCHICAL_CONTAINER_TRAITS_TESTS_
#define DJINTERP_HIERARCHICAL_CONTAINER_TRAITS_TESTS_ 1

// std
#include <array>
#include <cstddef>
#include <cstdio>
#include <forward_list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/core/container/structure)
#include "../../../../../inc/djinterp/core/container/traits/hierarchical_container_traits.hpp"


#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_HC_CHECK
//   macro: the per-suite runtime check (unique letters: HC = Hierarchical Container).
// Records a failure into the enclosing function's `_hc_ok` accumulator and prints the
// offending expression.  Always pass the condition already wrapped in parentheses so
// interior template commas (map<int,int>, container_depth<T, N>) survive macro
// argument splitting.
#define D_HC_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _hc_ok = false;                                                   \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// hc_cstr_eq
//   helper: constexpr C-string equality, so the kind name spellings can be checked
// directly (std::strcmp is not constexpr before C++23).
constexpr bool
hc_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : hc_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper container-shaped types + nested aliases
// ===========================================================================

// flat_leaf: a plain leaf container -> flat, depth 1
struct flat_leaf
{
    using value_type = int;
    std::size_t size() const;
};

// strlike: a container-shaped leaf (a value_type + size(), like std::string)
struct strlike
{
    using value_type = char;
    std::size_t size() const;
};

// nested_box: a container-shaped value_type -> depth 2, hierarchical
struct nested_box
{
    using value_type = flat_leaf;
    std::size_t size() const;
};

// tagged_flat_str: a container-shaped value_type (depth 2) with a flat tag that
// suppresses the depth heuristic -> not hierarchical
struct tagged_flat_str
{
    using value_type         = strlike;
    using structure_category = flat;
    std::size_t size() const;
};

// node_box: a container-shaped node_type - the strong hierarchy tell; hierarchical
// even at value_type-depth 1
struct node_box
{
    using value_type = int;
    using node_type  = flat_leaf;   // container-shaped -> node summand present
    std::size_t size() const;
};

// node_box_flat_tagged: node_type AND a flat tag - still hierarchical (the flat tag
// cannot override the strong node_type signal)
struct node_box_flat_tagged
{
    using value_type         = int;
    using node_type          = flat_leaf;
    using structure_category = flat;
    std::size_t size() const;
};

// hier_tagged: a hierarchical tag on a depth-1 container -> hierarchical outright
struct hier_tagged
{
    using value_type         = int;
    using structure_category = hierarchical;
    std::size_t size() const;
};

// nh: a node-handle-like leaf - a value_type but NO size(), so not container-shaped
struct nh
{
    using value_type = int;
};

// node_handle_box: a node_type that is NOT container-shaped -> has_node_summand false
// (the exclusion that keeps std::set / std::map node-handles from reading as nesting)
struct node_handle_box
{
    using value_type = int;
    using node_type  = nh;          // has value_type but no size() -> not a node summand
    std::size_t size() const;
};

// nested std::vector aliases (depths 1 / 2 / 3), for the depth + MaxDepth-cap tests
using vec1 = std::vector<int>;
using vec2 = std::vector<std::vector<int>>;
using vec3 = std::vector<std::vector<std::vector<int>>>;


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Structural detection signals --
bool tests_is_container_shape();
bool tests_has_node_summand();
bool tests_has_structure_category();

// -- II.  Recursive depth --
bool tests_container_depth();
bool tests_container_depth_maxdepth_cap();
bool tests_max_depth_of();

// -- III. Tag reading + verdict --
bool tests_is_hierarchical_container();
bool tests_is_depth_bounded_container();

// -- IV.  Structure-kind summary --
bool tests_structure_kind_name();
bool tests_structure_kind_of();

// -- V.   Aggregate snapshot + variable-template companions --
bool tests_hierarchical_container_class();
bool tests_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_HIERARCHICAL_CONTAINER_TRAITS_TESTS_
