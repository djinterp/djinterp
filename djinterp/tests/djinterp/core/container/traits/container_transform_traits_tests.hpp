/*******************************************************************************
* djinterp [testing]                        container_transform_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* container_transform_traits.hpp unit-test suite.  The Transformability axis is
* the twin of Filterability: a READ capability (iterability + a value_type) aligned
* with a BUILD capability (push_back or associative insert), exercised over a map
* f : tau -> sigma.  A type is a SOURCE if it can be read, TRANSFORMABLE if it can
* also gather the images, and INPUT-ONLY if it can be read but not built into.  The
* STRATEGY is the strongest realization its iterability grants; the STAGE is when a
* mapping may run; and PRESERVATION records that mapping keeps arrangement and size
* (order, both capacity bounds) but may break sortedness and uniqueness - the exact
* complement of selection.
*
*   MECHANISM.  The traits are compile-time facts; each check compares a constant
* reading against its expected result.  A failing check prints the offending
* expression and drives the function's bool result to false, which the runner turns
* into a non-zero exit code.
*
*   HELPER TYPES.  Custom fixtures pin the branches the standard containers do not
* reach on their own:
*     - native_box     exposes a transform(map) member  -> native strategy
*     - read_only_seq  iterable + typed, cannot build    -> input-only, external
*     - Box<E>         a clean element-parametric ctor    -> cleanly rebindable,
*                      transformable (bidirectional strategy)
*     - pair_box<A,B>  a two-type-parameter template      -> rebind rest-preservation
*   The standard containers supply the rest: std::vector/std::string (indexed),
* std::deque/std::list/std::set/std::map (bidirectional), std::unordered_set
* (forward), std::forward_list/std::array (input-only source, external), and a
* non-container (int, unsupported).  std::array also anchors compile-time STAGE
* (a literal type), where the growable containers are runtime.
*
*
* path:      /tests/djinterp/core/container/traits/container_transform_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRANSFORM_TRAITS_TESTS_
#define DJINTERP_CONTAINER_TRANSFORM_TRAITS_TESTS_ 1

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
#include <type_traits>
#include <unordered_set>
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/core/container/traits)
#include "../../../../../inc/djinterp/core/container/traits/container_transform_traits.hpp"


#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_TX_CHECK
//   macro: the per-suite runtime check (unique letters: TX = TransXform).  Records
// a failure into the enclosing function's `_tx_ok` accumulator and prints the
// offending expression.  Always pass the condition already wrapped in parentheses
// so interior template commas (map<int,int>, array<int,N>, pair_box<A,B>) survive
// macro argument splitting.
#define D_TX_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _tx_ok = false;                                                   \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// tx_cstr_eq
//   helper: constexpr C-string equality, so the strategy/stage name spellings can
// be checked directly (std::strcmp is not constexpr before C++23).
constexpr bool
tx_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : tx_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper types
// ===========================================================================

// native_box: exposes its own transform(map) primitive (and can build) -> native
struct native_box
{
    using value_type = int;
    int*       begin();
    int*       end();
    void       push_back(int);
    native_box transform(int (*)(const int&)) const;
};

// read_only_seq: iterable and typed, but cannot grow -> transform source, input-only
struct read_only_seq
{
    using value_type = int;
    const int* begin() const;
    const int* end()   const;
};

// Box<E>: a clean element-parametric constructor (type-pack, no allocator) that can
// be read and built -> transformable AND cleanly rebindable
template<typename _Elem>
struct Box
{
    using value_type = _Elem;
    _Elem* begin();
    _Elem* end();
    void   push_back(_Elem);
};

// pair_box<A,B>: a two-type-parameter template (not a container), used to check that
// rebind_element replaces only the FIRST argument and preserves the rest
template<typename _First, typename _Second>
struct pair_box
{};


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- I.   Capability detection --
bool tests_is_transform_source();
bool tests_is_container_transformable();
bool tests_is_transform_input_only();
bool tests_has_native_transform();

// -- II.  Strategy --
bool tests_transform_strategy_name();
bool tests_container_transform_strategy();

// -- III. Stage --
bool tests_transform_stage_name();
bool tests_transform_stage_of();

// -- IV.  Preservation --
bool tests_transform_preserves_order();
bool tests_transform_preserves_size_and_capacity();
bool tests_transform_preserves_sortedness_and_uniqueness();

// -- V.   Result type and element rebind --
bool tests_rebind_element();
bool tests_transform_result_type();

// -- VI.  Combined classification + variable-template companions --
bool tests_container_transform_class();
bool tests_transform_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TRANSFORM_TRAITS_TESTS_
