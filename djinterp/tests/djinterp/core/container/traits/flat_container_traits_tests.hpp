/*******************************************************************************
* djinterp [testing]                             flat_container_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* flat_container_traits.hpp unit-test suite.  Flatness is the COMPLEMENT of
* hierarchy over container-shaped types: a container is flat exactly when it is
* container-shaped and NOT hierarchical (depth 1, leaves only).  The header is thin
* - the heavy lifting (the container-shape guard, the depth recursion, the
* node_type tell, the structure_category tag) lives in hierarchical_container_traits.hpp
* and is inherited here - so these tests exercise how each of those signals
* propagates through the flat verdict, plus the one place flatness and the depth
* measure can diverge.
*
*   MECHANISM.  The traits are compile-time facts; each check compares a constant
* reading against its expected result.  A failing check prints the offending
* expression and drives the function's bool result to false, which the runner turns
* into a non-zero exit code.
*
*   HELPER TYPES.  Custom container-shaped fixtures (a value_type + a const size())
* pin every signal path:
*     - flat_leaf         value_type=int                     -> flat, depth 1, strict
*     - strlike           value_type=char (a string-like leaf, container-shaped)
*     - nested_box        value_type=flat_leaf               -> hierarchical (depth 2)
*     - tagged_flat_str   value_type=strlike + flat tag       -> flat by TAG, but depth 2,
*                         so is_flat is true while is_strictly_flat is FALSE
*     - node_box          a container-shaped node_type        -> hierarchical (strong tell,
*                         even at value_type-depth 1)
*     - node_box_flat_tagged  node_type + flat tag            -> STILL hierarchical
*                         (a flat tag cannot un-nest a declared node summand)
*     - hier_tagged       value_type=int + hierarchical tag   -> hierarchical at depth 1
*   The standard containers supply the realistic cases: std::vector/std::string/
* std::array/std::set/std::map are flat; std::vector<std::vector<int>> and
* std::vector<std::string> are hierarchical (the string element reads as a nested
* level); std::set/std::map stay flat despite their node-handle node_type (it is not
* container-shaped); and std::forward_list (no size()) and int are non-containers.
*
*
* path:      /tests/djinterp/core/container/traits/flat_container_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_FLAT_CONTAINER_TRAITS_TESTS_
#define DJINTERP_FLAT_CONTAINER_TRAITS_TESTS_ 1

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
#include "../../../../../inc/djinterp/core/container/traits/flat_container_traits.hpp"


#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_FC_CHECK
//   macro: the per-suite runtime check (unique letters: FC = Flat Container).
// Records a failure into the enclosing function's `_fc_ok` accumulator and prints
// the offending expression.  Always pass the condition already wrapped in
// parentheses so interior template commas (map<int,int>, array<int,N>) survive
// macro argument splitting.
#define D_FC_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _fc_ok = false;                                                   \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// fc_cstr_eq
//   helper: constexpr C-string equality, so the kind name spellings can be checked
// directly (std::strcmp is not constexpr before C++23).
constexpr bool
fc_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : fc_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper container-shaped types
// ===========================================================================

// flat_leaf: a plain leaf container -> flat, depth 1, strictly flat
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

// nested_box: a value_type that is itself container-shaped -> depth 2, hierarchical
struct nested_box
{
    using value_type = flat_leaf;
    std::size_t size() const;
};

// tagged_flat_str: a container-shaped value_type (depth 2) asserted flat via the tag;
// is_flat is true (tag suppresses the depth heuristic) but is_strictly_flat is FALSE
// (container_depth is unaffected by the tag and still reads 2)
struct tagged_flat_str
{
    using value_type         = strlike;
    using structure_category = flat;
    std::size_t size() const;
};

// node_box: a container-shaped node_type - the strong hierarchy tell; hierarchical
// even though the value_type chain is depth 1
struct node_box
{
    using value_type = int;
    using node_type  = flat_leaf;   // container-shaped -> node summand present
    std::size_t size() const;
};

// node_box_flat_tagged: node_type AND a flat tag - still hierarchical, since the flat
// tag cannot override the strong node_type signal
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


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.  Flat classification --
bool tests_is_flat_container();
bool tests_is_strictly_flat_container();

// -- II. Aggregate snapshot + variable-template companions --
bool tests_flat_container_class();
bool tests_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_FLAT_CONTAINER_TRAITS_TESTS_
