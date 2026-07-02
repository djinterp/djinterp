/******************************************************************************
* djinterp [container]                              element_relation_traits.hpp
*
*   The element relation - the =tau (and <tau) the whole content hierarchy is
* parameterised over.  Every content equality of the comparison model is taken
* relative to a relation on the leaf type tau: =set and =bag rest on element
* equality, a sorted or ordered presentation on an element order.  Where the
* other container traits read a container's SHAPE, this reads what its elements
* admit.
*
*   Three levels, following the formal account:
*
*     ELEMENT     does the value_type admit ==, <, the full relational set, or
*                 <=> ?  The comparison DEGREE names the strongest it reaches.
*                 This is the relation the content equalities consume.
*
*     CONTAINER   does the container itself compare as a whole (a std::vector
*                 against a std::vector)?  This is the observational reading - an
*                 == on the container is an operation its interface exposes.
*
*     CROSS       for two DIFFERENT container types, are their elements the same
*                 type, convertible, or cross-comparable?  The prerequisite for
*                 conversion and for comparing unlike containers element-wise.
*
*   DEGREE.  The element (or container) comparison degree is the strongest rung
* reached - none, then equality (==), then partial_order (<), then total_order
* (the full relational set), then three_way (<=>).  A degree is what a value-
* level comparison at a given content level requires: =set/=bag need equality,
* a sorted invariant needs an order.
*
*   PORTABILITY:
*   C++11 baseline; the <=> level is gated on three-way-comparison support and is
* simply unreachable (never true) below it.  `_v` companions degrade with the
* language, as elsewhere.
*
*
* path:      /inc/djinterp/core/container/traits/element_relation_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_ELEMENT_RELATION_TRAITS_
#define DJINTERP_ELEMENT_RELATION_TRAITS_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"                       // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"              // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    #include <compare>
#endif


NS_DJINTERP


// ===========================================================================
// I.   Element type + relation probes
// ===========================================================================

NS_INTERNAL

    // element_type_helper
    //   helper: a container's value_type, or void where there is none.
    template<typename _Container,
             typename = void>
    struct element_type_helper
    {
        using type = void;
    };

    template<typename _Container>
    struct element_type_helper<_Container,
        D_VOID_T<typename _Container::value_type>>
    {
        using type = typename _Container::value_type;
    };

    template<typename _Container>
    using element_type_of_helper =
        typename element_type_helper<_Container>::type;

    // relation probes on an element type _Elem.  The gate below keeps a void
    // element from ever reaching these, so each may assume a usable type.
    template<typename _Elem, typename = void>
    struct element_equality_helper : std::false_type {};
    template<typename _Elem>
    struct element_equality_helper<_Elem,
        D_VOID_T<decltype(std::declval<const _Elem&>()
                       == std::declval<const _Elem&>())>> : std::true_type {};

    template<typename _Elem, typename = void>
    struct element_inequality_helper : std::false_type {};
    template<typename _Elem>
    struct element_inequality_helper<_Elem,
        D_VOID_T<decltype(std::declval<const _Elem&>()
                       != std::declval<const _Elem&>())>> : std::true_type {};

    template<typename _Elem, typename = void>
    struct element_less_helper : std::false_type {};
    template<typename _Elem>
    struct element_less_helper<_Elem,
        D_VOID_T<decltype(std::declval<const _Elem&>()
                       <  std::declval<const _Elem&>())>> : std::true_type {};

    template<typename _Elem, typename = void>
    struct element_less_equal_helper : std::false_type {};
    template<typename _Elem>
    struct element_less_equal_helper<_Elem,
        D_VOID_T<decltype(std::declval<const _Elem&>()
                       <= std::declval<const _Elem&>())>> : std::true_type {};

    template<typename _Elem, typename = void>
    struct element_greater_helper : std::false_type {};
    template<typename _Elem>
    struct element_greater_helper<_Elem,
        D_VOID_T<decltype(std::declval<const _Elem&>()
                       >  std::declval<const _Elem&>())>> : std::true_type {};

    template<typename _Elem, typename = void>
    struct element_greater_equal_helper : std::false_type {};
    template<typename _Elem>
    struct element_greater_equal_helper<_Elem,
        D_VOID_T<decltype(std::declval<const _Elem&>()
                       >= std::declval<const _Elem&>())>> : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    template<typename _Elem, typename = void>
    struct element_three_way_helper : std::false_type {};
    template<typename _Elem>
    struct element_three_way_helper<_Elem,
        D_VOID_T<decltype(std::declval<const _Elem&>()
                       <=> std::declval<const _Elem&>())>> : std::true_type {};
#else
    template<typename _Elem, typename = void>
    struct element_three_way_helper : std::false_type {};
#endif

    // element_relation_gate
    //   helper: apply a relation probe to a container's element, resolving to
    // false when the container has no element type - so a probe naming a void
    // element is never instantiated.
    template<template<typename, typename> class _Probe,
             typename _Container,
             typename _Elem   = element_type_of_helper<_Container>,
             bool     _HasElem = !std::is_void<_Elem>::value>
    struct element_relation_gate : std::false_type {};

    template<template<typename, typename> class _Probe,
             typename _Container,
             typename _Elem>
    struct element_relation_gate<_Probe, _Container, _Elem, true>
        : _Probe<_Elem, void> {};

NS_END  // internal


// ===========================================================================
// II.  Element relation (on the value_type)
// ===========================================================================

// element_type_of
//   trait: the container's element type, or void where there is none.
template<typename _Type>
struct element_type_of
{
    using type = internal::element_type_of_helper<clean_t<_Type>>;
};

template<typename _Type>
using element_type_of_t = typename element_type_of<_Type>::type;

// has_equality_comparable_elements
//   trait: the value_type admits == - the =tau of the content equalities.
template<typename _Type>
struct has_equality_comparable_elements
    : internal::element_relation_gate<
          internal::element_equality_helper, clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_equality_comparable_elements)

// has_less_than_comparable_elements
//   trait: the value_type admits < - an order to sort or order by.
template<typename _Type>
struct has_less_than_comparable_elements
    : internal::element_relation_gate<
          internal::element_less_helper, clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_less_than_comparable_elements)

// has_totally_ordered_elements
//   trait: the value_type admits the full relational set (==, <, <=, >, >=) -
// a strict weak ordering usable both ways.
template<typename _Type>
struct has_totally_ordered_elements
    : std::integral_constant<bool,
            internal::element_relation_gate<
                internal::element_equality_helper, clean_t<_Type>>::value
         && internal::element_relation_gate<
                internal::element_less_helper, clean_t<_Type>>::value
         && internal::element_relation_gate<
                internal::element_less_equal_helper, clean_t<_Type>>::value
         && internal::element_relation_gate<
                internal::element_greater_helper, clean_t<_Type>>::value
         && internal::element_relation_gate<
                internal::element_greater_equal_helper, clean_t<_Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_totally_ordered_elements)

// has_three_way_comparable_elements
//   trait: the value_type admits <=> (always false below C++20).
template<typename _Type>
struct has_three_way_comparable_elements
    : internal::element_relation_gate<
          internal::element_three_way_helper, clean_t<_Type>>
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
template<typename _Type>
struct element_comparison_degree_of
{
    static constexpr comparison_degree value =
        ( has_three_way_comparable_elements<_Type>::value )
              ? comparison_degree::three_way
      : ( has_totally_ordered_elements<_Type>::value )
              ? comparison_degree::total_order
      : ( has_less_than_comparable_elements<_Type>::value )
              ? comparison_degree::partial_order
      : ( has_equality_comparable_elements<_Type>::value )
              ? comparison_degree::equality
      :         comparison_degree::none;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr comparison_degree element_comparison_degree_of_v =
        element_comparison_degree_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr comparison_degree element_comparison_degree_of_v =
        element_comparison_degree_of<_Type>::value;
#endif


// ===========================================================================
// IV.  Container-level comparison (the observational reading)
// ===========================================================================

// has_container_equality
//   trait: the container itself admits == as a whole.
D_TYPE_TRAIT_TRUE(has_container_equality,
    decltype(std::declval<const clean_t<_Type>&>()
          == std::declval<const clean_t<_Type>&>()))

// has_container_less
//   trait: the container itself admits < as a whole.
D_TYPE_TRAIT_TRUE(has_container_less,
    decltype(std::declval<const clean_t<_Type>&>()
          <  std::declval<const clean_t<_Type>&>()))

NS_INTERNAL

    // container relational probes, for the total-order rung.
    template<typename _Type, typename = void>
    struct container_less_equal_helper : std::false_type {};
    template<typename _Type>
    struct container_less_equal_helper<_Type,
        D_VOID_T<decltype(std::declval<const _Type&>()
                       <= std::declval<const _Type&>())>> : std::true_type {};

    template<typename _Type, typename = void>
    struct container_greater_helper : std::false_type {};
    template<typename _Type>
    struct container_greater_helper<_Type,
        D_VOID_T<decltype(std::declval<const _Type&>()
                       >  std::declval<const _Type&>())>> : std::true_type {};

    template<typename _Type, typename = void>
    struct container_greater_equal_helper : std::false_type {};
    template<typename _Type>
    struct container_greater_equal_helper<_Type,
        D_VOID_T<decltype(std::declval<const _Type&>()
                       >= std::declval<const _Type&>())>> : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    template<typename _Type, typename = void>
    struct container_three_way_helper : std::false_type {};
    template<typename _Type>
    struct container_three_way_helper<_Type,
        D_VOID_T<decltype(std::declval<const _Type&>()
                       <=> std::declval<const _Type&>())>> : std::true_type {};
#else
    template<typename _Type, typename = void>
    struct container_three_way_helper : std::false_type {};
#endif

    // container_declared_degree_helper
    //   helper: the degree the container's own operators are DECLARED at.  A std
    // operator template is declared unconditionally (a std::vector declares ==
    // whatever its element), so this reports the declaration; it is combined with
    // the element degree below to yield actual comparability.
    template<typename _Type>
    struct container_declared_degree_helper
    {
        static constexpr comparison_degree value =
            ( container_three_way_helper<_Type>::value )
                  ? comparison_degree::three_way
          : ( has_container_equality<_Type>::value
           && has_container_less<_Type>::value
           && container_less_equal_helper<_Type>::value
           && container_greater_helper<_Type>::value
           && container_greater_equal_helper<_Type>::value )
                  ? comparison_degree::total_order
          : ( has_container_less<_Type>::value )
                  ? comparison_degree::partial_order
          : ( has_container_equality<_Type>::value )
                  ? comparison_degree::equality
          :         comparison_degree::none;
    };

NS_END  // internal

// has_three_way_comparable_container
//   trait: the container itself admits <=> (always false below C++20).
template<typename _Type>
struct has_three_way_comparable_container
    : internal::container_three_way_helper<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_three_way_comparable_container)

// container_comparison_degree_of
//   trait: the degree at which the container can ACTUALLY be compared as a whole
// - the weaker of the degree its operators are declared at and the degree its
// elements support.  For a std container, whose comparison is its elements'
// lifted, this is the element degree; for one that declares no comparison, none.
template<typename _Type>
struct container_comparison_degree_of
{
private:
    using clean_type = clean_t<_Type>;

public:
    static constexpr comparison_degree value =
        comparison_degree_weaker(
            internal::container_declared_degree_helper<clean_type>::value,
            element_comparison_degree_of<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr comparison_degree container_comparison_degree_of_v =
        container_comparison_degree_of<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr comparison_degree container_comparison_degree_of_v =
        container_comparison_degree_of<_Type>::value;
#endif

// is_equality_comparable_container
//   trait: two of these can actually be == compared - the container declares ==
// and its elements support it.
template<typename _Type>
struct is_equality_comparable_container
    : std::integral_constant<bool,
          comparison_degree_rank(
              container_comparison_degree_of<clean_t<_Type>>::value )
              >= comparison_degree_rank(comparison_degree::equality)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_equality_comparable_container)

// is_ordered_comparable_container
//   trait: two of these can actually be < compared.
template<typename _Type>
struct is_ordered_comparable_container
    : std::integral_constant<bool,
          comparison_degree_rank(
              container_comparison_degree_of<clean_t<_Type>>::value )
              >= comparison_degree_rank(comparison_degree::partial_order)>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_ordered_comparable_container)


// ===========================================================================
// V.   Cross-container element compatibility
// ===========================================================================

// elements_same_type
//   trait: the two containers share a value_type.
template<typename _Left,
         typename _Right>
struct elements_same_type
    : std::is_same<internal::element_type_of_helper<clean_t<_Left>>,
                   internal::element_type_of_helper<clean_t<_Right>>>
{};

// elements_convertible
//   trait: the left container's element is convertible to the right's.
template<typename _Left,
         typename _Right>
struct elements_convertible
    : std::is_convertible<internal::element_type_of_helper<clean_t<_Left>>,
                          internal::element_type_of_helper<clean_t<_Right>>>
{};

// elements_mutually_convertible
//   trait: the element types convert in both directions.
template<typename _Left,
         typename _Right>
struct elements_mutually_convertible
    : std::integral_constant<bool,
            elements_convertible<_Left, _Right>::value
         && elements_convertible<_Right, _Left>::value>
{};

NS_INTERNAL

    // cross_equality_helper / cross_less_helper: unlike-element probes, applied
    // only when both element types are present (the gate below enforces it).
    template<typename _ElemL, typename _ElemR, typename = void>
    struct cross_equality_helper : std::false_type {};
    template<typename _ElemL, typename _ElemR>
    struct cross_equality_helper<_ElemL, _ElemR,
        D_VOID_T<decltype(std::declval<const _ElemL&>()
                       == std::declval<const _ElemR&>())>> : std::true_type {};

    template<typename _ElemL, typename _ElemR, typename = void>
    struct cross_less_helper : std::false_type {};
    template<typename _ElemL, typename _ElemR>
    struct cross_less_helper<_ElemL, _ElemR,
        D_VOID_T<decltype(std::declval<const _ElemL&>()
                       <  std::declval<const _ElemR&>())>> : std::true_type {};

    // cross_relation_gate: apply an unlike-element probe, false unless both
    // containers carry an element type.
    template<template<typename, typename, typename> class _Probe,
             typename _Left,
             typename _Right,
             typename _ElemL = element_type_of_helper<clean_t<_Left>>,
             typename _ElemR = element_type_of_helper<clean_t<_Right>>,
             bool     _Both  = ( !std::is_void<_ElemL>::value
                              && !std::is_void<_ElemR>::value )>
    struct cross_relation_gate : std::false_type {};

    template<template<typename, typename, typename> class _Probe,
             typename _Left,
             typename _Right,
             typename _ElemL,
             typename _ElemR>
    struct cross_relation_gate<_Probe, _Left, _Right, _ElemL, _ElemR, true>
        : _Probe<_ElemL, _ElemR, void> {};

NS_END  // internal

// cross_elements_equality_comparable
//   trait: an element of the left container can be == compared with an element
// of the right - the two may be compared element-wise for equality.
template<typename _Left,
         typename _Right>
struct cross_elements_equality_comparable
    : internal::cross_relation_gate<
          internal::cross_equality_helper, _Left, _Right>
{};

// cross_elements_ordered
//   trait: an element of the left can be < compared with an element of the right.
template<typename _Left,
         typename _Right>
struct cross_elements_ordered
    : internal::cross_relation_gate<
          internal::cross_less_helper, _Left, _Right>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Left, typename _Right>
    inline constexpr bool elements_same_type_v =
        elements_same_type<_Left, _Right>::value;
    template<typename _Left, typename _Right>
    inline constexpr bool elements_convertible_v =
        elements_convertible<_Left, _Right>::value;
    template<typename _Left, typename _Right>
    inline constexpr bool elements_mutually_convertible_v =
        elements_mutually_convertible<_Left, _Right>::value;
    template<typename _Left, typename _Right>
    inline constexpr bool cross_elements_equality_comparable_v =
        cross_elements_equality_comparable<_Left, _Right>::value;
    template<typename _Left, typename _Right>
    inline constexpr bool cross_elements_ordered_v =
        cross_elements_ordered<_Left, _Right>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Left, typename _Right>
    constexpr bool elements_same_type_v =
        elements_same_type<_Left, _Right>::value;
    template<typename _Left, typename _Right>
    constexpr bool elements_convertible_v =
        elements_convertible<_Left, _Right>::value;
    template<typename _Left, typename _Right>
    constexpr bool elements_mutually_convertible_v =
        elements_mutually_convertible<_Left, _Right>::value;
    template<typename _Left, typename _Right>
    constexpr bool cross_elements_equality_comparable_v =
        cross_elements_equality_comparable<_Left, _Right>::value;
    template<typename _Left, typename _Right>
    constexpr bool cross_elements_ordered_v =
        cross_elements_ordered<_Left, _Right>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_ELEMENT_RELATION_TRAITS_
