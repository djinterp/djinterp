/*******************************************************************************
* djinterp [testing]                           element_relation_traits_tests.hpp
*
*   Declarations, shared helpers, and helper TYPES for the
* element_relation_traits.hpp unit-test suite.  Where the other container traits
* read a container's SHAPE, this reads what its ELEMENTS admit: the =tau (and <tau)
* the content hierarchy is parameterised over.  Three levels - ELEMENT (does the
* value_type admit ==, <, the full relational set, or <=> ?), CONTAINER (does the
* container compare as a whole?), and CROSS (for two container types, are their
* elements the same / convertible / cross-comparable?) - each summarised by a
* comparison DEGREE (none < equality < partial_order < total_order < three_way).
*
*   MECHANISM.  The traits are compile-time facts; each check compares a constant
* reading against its expected result.  A failing check prints the offending
* expression and drives the function's bool result to false, which the runner turns
* into a non-zero exit code.
*
*   HELPER TYPES.  Because the standard fundamental element types reach three_way
* under C++20 (int / char admit <=>), the intermediate rungs are pinned by custom
* ELEMENT types with controlled operator sets, wrapped in box<E> (a container-shaped
* type whose value_type is E):
*     - elem_none   no operators                       -> element degree none
*     - elem_eq     operator== only                    -> equality
*     - elem_lt     operator== and operator<           -> partial_order
*     - elem_total  ==, <, <=, >, >= (no <=>)           -> total_order
*     - elem_3way   defaulted <=> (C++20)               -> three_way
*   and custom CONTAINER types with explicitly declared comparison operators pin the
* container-degree ladder independently of the library's C++20 comparison quirks:
*     - cmp_eq / cmp_partial / cmp_total / cmp_3way  (int elements)
*     - cmp_total_eqelem  (declares all five, but elem_eq elements -> element caps)
*   The standard containers (std::vector, std::string) and a non-container (int)
* supply the realistic and boundary cases.
*
*   NOTE (three-way).  The <=> level and the elem_3way / cmp_3way fixtures are gated
* on D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON; below C++20 the level is
* simply unreachable and those checks are compiled out.  (Also: the header's
* internal element_inequality_helper has no public consumer, so it is not reachable
* through these tests.)
*
*
* path:      /tests/djinterp/core/container/traits/element_relation_traits_tests.hpp
* link(s):   TBA
* author(s): unit-test suite                                 created: 2026.07.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_ELEMENT_RELATION_TRAITS_TESTS_
#define DJINTERP_ELEMENT_RELATION_TRAITS_TESTS_ 1

// std
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
// djinterp (bare include; the leaf CMakeLists adds inc/djinterp/core/container/traits)
#include "../../../../../inc/djinterp/core/container/traits/element_relation_traits.hpp"


#ifndef D_KEYWORD_TESTING
    #define D_KEYWORD_TESTING   testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
    #define NS_TESTING          D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


// D_ER_CHECK
//   macro: the per-suite runtime check (unique letters: ER = Element Relation).
// Records a failure into the enclosing function's `_er_ok` accumulator and prints
// the offending expression.  Always pass the condition already wrapped in
// parentheses so interior template commas (elements_same_type<A, B>, etc.) survive
// macro argument splitting.
#define D_ER_CHECK(_cond)                                                     \
    do {                                                                      \
        if (!(_cond)) {                                                       \
            std::printf("    - check failed: %s\n", #_cond);                  \
            _er_ok = false;                                                   \
        }                                                                     \
    } while (false)


NS_DJINTERP
NS_TESTING


// ===========================================================================
// Shared helpers
// ===========================================================================

// er_cstr_eq
//   helper: constexpr C-string equality, so the degree name spellings can be
// checked directly (std::strcmp is not constexpr before C++23).
constexpr bool
er_cstr_eq(const char* _a, const char* _b) noexcept
{
    return (*_a == *_b) && (*_a == '\0' ? true : er_cstr_eq(_a + 1, _b + 1));
}


// ===========================================================================
// Helper element types (controlled operator sets)
// ===========================================================================

// elem_none: no comparison operators -> element degree none
struct elem_none
{};

// elem_eq: operator== only -> equality
struct elem_eq
{
    bool operator==(const elem_eq&) const;
};

// elem_lt: operator== and operator< (but not <=, >, >=) -> partial_order
struct elem_lt
{
    bool operator==(const elem_lt&) const;
    bool operator< (const elem_lt&) const;
};

// elem_total: the full relational set, but NO <=> -> total_order
struct elem_total
{
    bool operator==(const elem_total&) const;
    bool operator< (const elem_total&) const;
    bool operator<=(const elem_total&) const;
    bool operator> (const elem_total&) const;
    bool operator>=(const elem_total&) const;
};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
// elem_3way: a defaulted <=> (which also yields ==) -> three_way
struct elem_3way
{
    auto operator<=>(const elem_3way&) const = default;
};
#endif  // D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON


// ===========================================================================
// Helper container types
// ===========================================================================

// box<E>: a container-shaped type carrying value_type = E, with NO container-level
// comparison operators of its own (so its container degree is capped at none)
template<typename _Elem>
struct box
{
    using value_type = _Elem;
};

// convert_from_int: constructible FROM int but not convertible back TO int - an
// asymmetric conversion, so box<int> -> box<convert_from_int> converts one way only
struct convert_from_int
{
    convert_from_int(int);
};

// container types with explicitly declared comparison operators + int elements
// (whose element degree is the strongest, so the container degree tracks the
// declaration)
struct cmp_eq
{
    using value_type = int;
    bool operator==(const cmp_eq&) const;
};

struct cmp_partial
{
    using value_type = int;
    bool operator==(const cmp_partial&) const;
    bool operator< (const cmp_partial&) const;
};

struct cmp_total
{
    using value_type = int;
    bool operator==(const cmp_total&) const;
    bool operator< (const cmp_total&) const;
    bool operator<=(const cmp_total&) const;
    bool operator> (const cmp_total&) const;
    bool operator>=(const cmp_total&) const;
};

// declares the full relational set, but its ELEMENTS (elem_eq) support only == -
// so container_comparison_degree_of is capped at equality by the elements
struct cmp_total_eqelem
{
    using value_type = elem_eq;
    bool operator==(const cmp_total_eqelem&) const;
    bool operator< (const cmp_total_eqelem&) const;
    bool operator<=(const cmp_total_eqelem&) const;
    bool operator> (const cmp_total_eqelem&) const;
    bool operator>=(const cmp_total_eqelem&) const;
};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
struct cmp_3way
{
    using value_type = int;
    auto operator<=>(const cmp_3way&) const = default;
};
#endif  // D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON


// ===========================================================================
// Test-function declarations (defined in the section .cpp files)
// ===========================================================================

// -- II.  Element relation (on the value_type) --
bool tests_element_type_of();
bool tests_has_equality_comparable_elements();
bool tests_has_less_than_comparable_elements();
bool tests_has_totally_ordered_elements();
bool tests_has_three_way_comparable_elements();

// -- III. Comparison degree --
bool tests_comparison_degree_name();
bool tests_comparison_degree_rank_and_weaker();
bool tests_element_comparison_degree_of();

// -- IV.  Container-level comparison --
bool tests_has_container_equality_and_less();
bool tests_has_three_way_comparable_container();
bool tests_container_comparison_degree_of();
bool tests_is_equality_comparable_container();
bool tests_is_ordered_comparable_container();

// -- V.   Cross-container element compatibility --
bool tests_elements_same_type();
bool tests_elements_convertible();
bool tests_elements_mutually_convertible();
bool tests_cross_elements_equality_comparable();
bool tests_cross_elements_ordered();
bool tests_variable_templates();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_ELEMENT_RELATION_TRAITS_TESTS_
