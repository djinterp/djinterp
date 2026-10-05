/*******************************************************************************
* djinterp [core]                                    element_relation_traits.hpp
*
*   The element relation - the =tau (and <tau) the whole content hierarchy is
* parameterised over.  Every content equality of the comparison model is taken
* relative to a relation on the leaf type tau: =set and =bag rest on element
* equality, a sorted or ordered presentation on an element order.  Where the
* other container traits read a container's SHAPE, this reads what its
* elements
* admit.
*
*   Three levels, following the formal account:
*
*     ELEMENT     does the value_type admit ==, <, the full relational set, or
*                 <=> ?  The comparison DEGREE names the strongest it reaches.
*                 This is the relation the content equalities consume.
*
*     CONTAINER   does the container itself compare as a whole (a std::vector
*                 against a std::vector)? This is the observational reading -
*               an
*                 == on the container is an operation its interface exposes.
*
*     CROSS for two DIFFERENT container types, are their elements the same
*                 type, convertible, or cross-comparable? The prerequisite for
*                 conversion and for comparing unlike containers element-wise.
*
*   DEGREE. The element (or container) comparison degree is the strongest rung
* reached - none, then equality (==), then partial_order (<), then total_order
* (the full relational set), then three_way (<=>).  A degree is what a value-
* level comparison at a given content level requires: =set/=bag need equality,
* a sorted invariant needs an order.
*
*   PORTABILITY:
*   C++11 baseline; the <=> level is gated on three-way-comparison support and
* is
* simply unreachable (never true) below it.  `_v` companions degrade with the
* language, as elsewhere.
*
*
* path:      /inc/djinterp/core/container/traits/element_relation_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.30
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_ELEMENT_RELATION_TRAITS_HPP
#define DJINTERP_CONTAINER_TRAITS_ELEMENT_RELATION_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"              // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    // std
    #include <compare>
#endif


NS_DJINTERP


// ===========================================================================
// I.   Element type + relation probes
// ===========================================================================

NS_INTERNAL

    // element_type_helper
    //   helper: a container's value_type, or void where there is none.
    template<typename Container,
             typename = void>
    struct element_type_helper
    {
        using type = void;
    };

    // element_type_helper specialization
    //   helper: the detected case -- selected when the type declares the
    // nested type `value_type`.
    template<typename Container>
    struct element_type_helper<Container,
        D_VOID_T<typename Container::value_type>>
    {
        using type = typename Container::value_type;
    };

    template<typename Container>
    using element_type_of_helper =
        typename element_type_helper<Container>::type;

    // relation probes on an element type Elem. The gate below keeps a void
    // element from ever reaching these, so each may assume a usable type.
    template<typename Elem, typename = void>
    struct element_equality_helper : std::false_type {};
    template<typename Elem>
    struct element_equality_helper<Elem,
        D_VOID_T<decltype(std::declval<const Elem&>()
                       == std::declval<const Elem&>())>> : std::true_type {};

    template<typename Elem, typename = void>
    struct element_inequality_helper : std::false_type {};
    template<typename Elem>
    struct element_inequality_helper<Elem,
        D_VOID_T<decltype(std::declval<const Elem&>()
                       != std::declval<const Elem&>())>> : std::true_type {};

    template<typename Elem, typename = void>
    struct element_less_helper : std::false_type {};
    template<typename Elem>
    struct element_less_helper<Elem,
        D_VOID_T<decltype(std::declval<const Elem&>()
                       <  std::declval<const Elem&>())>> : std::true_type {};

    template<typename Elem, typename = void>
    struct element_less_equal_helper : std::false_type {};
    template<typename Elem>
    struct element_less_equal_helper<Elem,
        D_VOID_T<decltype(std::declval<const Elem&>()
                       <= std::declval<const Elem&>())>> : std::true_type {};

    template<typename Elem, typename = void>
    struct element_greater_helper : std::false_type {};
    template<typename Elem>
    struct element_greater_helper<Elem,
        D_VOID_T<decltype(std::declval<const Elem&>()
                       >  std::declval<const Elem&>())>> : std::true_type {};

    template<typename Elem, typename = void>
    struct element_greater_equal_helper : std::false_type {};
    template<typename Elem>
    struct element_greater_equal_helper<Elem,
        D_VOID_T<decltype(std::declval<const Elem&>()
                       >= std::declval<const Elem&>())>> : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    template<typename Elem, typename = void>
    struct element_three_way_helper : std::false_type {};
    template<typename Elem>
    struct element_three_way_helper<Elem,
        D_VOID_T<decltype(std::declval<const Elem&>()
                       <=> std::declval<const Elem&>())>> : std::true_type {};
#else
    template<typename Elem, typename = void>
    struct element_three_way_helper : std::false_type {};
#endif

    // element_relation_gate
    //   helper: apply a relation probe to a container's element, resolving to
    // false when the container has no element type - so a probe naming a void
    // element is never instantiated.
    template<template<typename, typename> class Probe,
             typename Container,
             typename Elem    = element_type_of_helper<Container>,
             bool     HasElem = !std::is_void<Elem>::value>
    struct element_relation_gate : std::false_type {};

    // element_relation_gate<Probe, Container, Elem, true>
    //   helper: the case where `!std::is_void<Elem>::value` is true.
    template<template<typename, typename> class Probe,
             typename Container,
             typename Elem>
    struct element_relation_gate<Probe, Container, Elem, true>
        : Probe<Elem, void> {};

NS_END  // internal


// ===========================================================================
// II.  Element relation (on the value_type)
// ===========================================================================

// element_type_of
//   trait: the container's element type, or void where there is none.
template<typename Type>
struct element_type_of
{
    using type = internal::element_type_of_helper<clean_t<Type>>;
};

// element_type_of_t
//   type: the carrier of element_type_of -- its `::type`, for use where a type
// rather than a value is wanted.
template<typename Type>
using element_type_of_t = typename element_type_of<Type>::type;

// has_equality_comparable_elements
//   trait: the value_type admits == - the =tau of the content equalities.
template<typename Type>
struct has_equality_comparable_elements
    : internal::element_relation_gate<
          internal::element_equality_helper, clean_t<Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_equality_comparable_elements)

// has_less_than_comparable_elements
//   trait: the value_type admits < - an order to sort or order by.
template<typename Type>
struct has_less_than_comparable_elements
    : internal::element_relation_gate<
          internal::element_less_helper, clean_t<Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_less_than_comparable_elements)

// has_totally_ordered_elements
//   trait: the value_type admits the full relational set (==, <, <=, >, >=) -
// a strict weak ordering usable both ways.
template<typename Type>
struct has_totally_ordered_elements
    : std::integral_constant<bool,
            internal::element_relation_gate<
                internal::element_equality_helper, clean_t<Type>>::value
         && internal::element_relation_gate<
                internal::element_less_helper, clean_t<Type>>::value
         && internal::element_relation_gate<
                internal::element_less_equal_helper, clean_t<Type>>::value
         && internal::element_relation_gate<
                internal::element_greater_helper, clean_t<Type>>::value
         && internal::element_relation_gate<
                internal::element_greater_equal_helper, clean_t<Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_totally_ordered_elements)

// has_three_way_comparable_elements
//   trait: the value_type admits <=> (always false below C++20).
template<typename Type>
struct has_three_way_comparable_elements
    : internal::element_relation_gate<
          internal::element_three_way_helper, clean_t<Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_three_way_comparable_elements)


// ===========================================================================
// III. Comparison degree
// ===========================================================================

// comparison_degree
//   enum: the strongest comparison a type (element or container) reaches.
enum class comparison_degree
{
    none,           // no comparison
    equality,       // == (and !=)
    partial_order,  // < present, not the full relational set
    total_order,    // the full relational set (a strict weak ordering)
    three_way       // <=> yielding an ordering (C++20)
};

// comparison_degree_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
comparison_degree_name(comparison_degree _d) noexcept
{
    return ( _d == comparison_degree::none          ? "none"
           : _d == comparison_degree::equality       ? "equality"
           : _d == comparison_degree::partial_order  ? "partial_order"
           : _d == comparison_degree::total_order    ? "total_order"
           :                                           "three_way" );
}

// comparison_degree_rank
//   function: strength as an integer (none weakest, three_way strongest); the
// enum is declared in that order, so the cast is the rank.
constexpr int
comparison_degree_rank(comparison_degree _d) noexcept
{
    return static_cast<int>(_d);
}

// comparison_degree_weaker
//   function: the weaker of two degrees - the strongest rung BOTH reach.
constexpr comparison_degree
comparison_degree_weaker(comparison_degree _a, comparison_degree _b) noexcept
{
    return ( comparison_degree_rank(_a) <= comparison_degree_rank(_b) )
               ? _a : _b;
}

// element_comparison_degree_of
//   trait: the strongest comparison the container's value_type supports.
template<typename Type>
struct element_comparison_degree_of
{
    static constexpr comparison_degree value =
        ( has_three_way_comparable_elements<Type>::value )
              ? comparison_degree::three_way
      : ( has_totally_ordered_elements<Type>::value )
              ? comparison_degree::total_order
      : ( has_less_than_comparable_elements<Type>::value )
              ? comparison_degree::partial_order
      : ( has_equality_comparable_elements<Type>::value )
              ? comparison_degree::equality
      :         comparison_degree::none;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr comparison_degree element_comparison_degree_of_v =
        element_comparison_degree_of<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr comparison_degree element_comparison_degree_of_v =
        element_comparison_degree_of<Type>::value;
#endif


// ===========================================================================
// IV.  Container-level comparison (the observational reading)
// ===========================================================================

// has_container_equality
//   trait: the container itself admits == as a whole.
D_TYPE_TRAIT_TRUE(has_container_equality,
    decltype(std::declval<const clean_t<Type>&>()
          == std::declval<const clean_t<Type>&>()))

// has_container_less
//   trait: the container itself admits < as a whole.
D_TYPE_TRAIT_TRUE(has_container_less,
    decltype(std::declval<const clean_t<Type>&>()
          <  std::declval<const clean_t<Type>&>()))

NS_INTERNAL

    // container relational probes, for the total-order rung.
    template<typename Type, typename = void>
    struct container_less_equal_helper : std::false_type {};
    template<typename Type>
    struct container_less_equal_helper<Type,
        D_VOID_T<decltype(std::declval<const Type&>()
                       <= std::declval<const Type&>())>> : std::true_type {};

    template<typename Type, typename = void>
    struct container_greater_helper : std::false_type {};
    template<typename Type>
    struct container_greater_helper<Type,
        D_VOID_T<decltype(std::declval<const Type&>()
                       >  std::declval<const Type&>())>> : std::true_type {};

    template<typename Type, typename = void>
    struct container_greater_equal_helper : std::false_type {};
    template<typename Type>
    struct container_greater_equal_helper<Type,
        D_VOID_T<decltype(std::declval<const Type&>()
                       >= std::declval<const Type&>())>> : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    template<typename Type, typename = void>
    struct container_three_way_helper : std::false_type {};
    template<typename Type>
    struct container_three_way_helper<Type,
        D_VOID_T<decltype(std::declval<const Type&>()
                       <=> std::declval<const Type&>())>> : std::true_type {};
#else
    template<typename Type, typename = void>
    struct container_three_way_helper : std::false_type {};
#endif

    // container_declared_degree_helper
    //   helper: the degree the container's own operators are DECLARED at. A
    // std operator template is declared unconditionally (a std::vector
    // declares == whatever its element), so this reports the declaration; it
    // is combined with the element degree below to yield actual comparability.
    template<typename Type>
    struct container_declared_degree_helper
    {
        static constexpr comparison_degree value =
            ( container_three_way_helper<Type>::value )
                  ? comparison_degree::three_way
          : ( has_container_equality<Type>::value
           && has_container_less<Type>::value
           && container_less_equal_helper<Type>::value
           && container_greater_helper<Type>::value
           && container_greater_equal_helper<Type>::value )
                  ? comparison_degree::total_order
          : ( has_container_less<Type>::value )
                  ? comparison_degree::partial_order
          : ( has_container_equality<Type>::value )
                  ? comparison_degree::equality
          :         comparison_degree::none;
    };

NS_END  // internal

// has_three_way_comparable_container
//   trait: the container itself admits <=> (always false below C++20).
template<typename Type>
struct has_three_way_comparable_container
    : internal::container_three_way_helper<clean_t<Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_three_way_comparable_container)

// container_comparison_degree_of
//   trait: the degree at which the container can ACTUALLY be compared as a
// whole
// - the weaker of the degree its operators are declared at and the degree its
// elements support. For a std container, whose comparison is its elements'
// lifted, this is the element degree; for one that declares no comparison,
// none.
template<typename Type>
struct container_comparison_degree_of
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr comparison_degree value =
        comparison_degree_weaker(
            internal::container_declared_degree_helper<clean_type>::value,
            element_comparison_degree_of<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr comparison_degree container_comparison_degree_of_v =
        container_comparison_degree_of<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr comparison_degree container_comparison_degree_of_v =
        container_comparison_degree_of<Type>::value;
#endif

// is_equality_comparable_container
//   trait: two of these can actually be == compared - the container declares
// == and its elements support it.
template<typename Type>
struct is_equality_comparable_container
    : std::integral_constant<bool,
          comparison_degree_rank(
              container_comparison_degree_of<clean_t<Type>>::value )
              >= comparison_degree_rank(comparison_degree::equality)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_equality_comparable_container)

// is_ordered_comparable_container
//   trait: two of these can actually be < compared.
template<typename Type>
struct is_ordered_comparable_container
    : std::integral_constant<bool,
          comparison_degree_rank(
              container_comparison_degree_of<clean_t<Type>>::value )
              >= comparison_degree_rank(comparison_degree::partial_order)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_ordered_comparable_container)


// ===========================================================================
// V.   Cross-container element compatibility
// ===========================================================================

// elements_same_type
//   trait: the two containers share a value_type.
template<typename Left,
         typename Right>
struct elements_same_type
    : std::is_same<internal::element_type_of_helper<clean_t<Left>>,
                   internal::element_type_of_helper<clean_t<Right>>>
{};

// elements_convertible
//   trait: the left container's element is convertible to the right's.
template<typename Left,
         typename Right>
struct elements_convertible
    : std::is_convertible<internal::element_type_of_helper<clean_t<Left>>,
                          internal::element_type_of_helper<clean_t<Right>>>
{};

// elements_mutually_convertible
//   trait: the element types convert in both directions.
template<typename Left,
         typename Right>
struct elements_mutually_convertible
    : std::integral_constant<bool,
            elements_convertible<Left, Right>::value
         && elements_convertible<Right, Left>::value>
{};

NS_INTERNAL

    // cross_equality_helper / cross_less_helper: unlike-element probes,
    // applied only when both element types are present (the gate below
    // enforces it).
    template<typename ElemL, typename ElemR, typename = void>
    struct cross_equality_helper : std::false_type {};
    template<typename ElemL, typename ElemR>
    struct cross_equality_helper<ElemL, ElemR,
        D_VOID_T<decltype(std::declval<const ElemL&>()
                       == std::declval<const ElemR&>())>> : std::true_type {};

    template<typename ElemL, typename ElemR, typename = void>
    struct cross_less_helper : std::false_type {};
    template<typename ElemL, typename ElemR>
    struct cross_less_helper<ElemL, ElemR,
        D_VOID_T<decltype(std::declval<const ElemL&>()
                       <  std::declval<const ElemR&>())>> : std::true_type {};

    // cross_relation_gate: apply an unlike-element probe, false unless both
    // containers carry an element type.
    template<template<typename, typename, typename> class Probe,
             typename Left,
             typename Right,
             typename ElemL = element_type_of_helper<clean_t<Left>>,
             typename ElemR = element_type_of_helper<clean_t<Right>>,
             bool     Both   = ( !std::is_void<ElemL>::value
                              && !std::is_void<ElemR>::value )>
    struct cross_relation_gate : std::false_type {};

    template<template<typename, typename, typename> class Probe,
             typename Left,
             typename Right,
             typename ElemL,
             typename ElemR>
    struct cross_relation_gate<Probe, Left, Right, ElemL, ElemR, true>
        : Probe<ElemL, ElemR, void> {};

NS_END  // internal

// cross_elements_equality_comparable
//   trait: an element of the left container can be == compared with an element
// of the right - the two may be compared element-wise for equality.
template<typename Left,
         typename Right>
struct cross_elements_equality_comparable
    : internal::cross_relation_gate<
          internal::cross_equality_helper, Left, Right>
{};

// cross_elements_ordered
//   trait: an element of the left can be < compared with an element of the
// right.
template<typename Left,
         typename Right>
struct cross_elements_ordered
    : internal::cross_relation_gate<
          internal::cross_less_helper, Left, Right>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Left, typename Right>
    inline constexpr bool elements_same_type_v =
        elements_same_type<Left, Right>::value;
    template<typename Left, typename Right>
    inline constexpr bool elements_convertible_v =
        elements_convertible<Left, Right>::value;
    template<typename Left, typename Right>
    inline constexpr bool elements_mutually_convertible_v =
        elements_mutually_convertible<Left, Right>::value;
    template<typename Left, typename Right>
    inline constexpr bool cross_elements_equality_comparable_v =
        cross_elements_equality_comparable<Left, Right>::value;
    template<typename Left, typename Right>
    inline constexpr bool cross_elements_ordered_v =
        cross_elements_ordered<Left, Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Left, typename Right>
    constexpr bool elements_same_type_v =
        elements_same_type<Left, Right>::value;
    template<typename Left, typename Right>
    constexpr bool elements_convertible_v =
        elements_convertible<Left, Right>::value;
    template<typename Left, typename Right>
    constexpr bool elements_mutually_convertible_v =
        elements_mutually_convertible<Left, Right>::value;
    template<typename Left, typename Right>
    constexpr bool cross_elements_equality_comparable_v =
        cross_elements_equality_comparable<Left, Right>::value;
    template<typename Left, typename Right>
    constexpr bool cross_elements_ordered_v =
        cross_elements_ordered<Left, Right>::value;
#endif


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TRAITS_ELEMENT_RELATION_TRAITS_HPP
