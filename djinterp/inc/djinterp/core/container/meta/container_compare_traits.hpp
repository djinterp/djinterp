/******************************************************************************
* djinterp [container]                           container_compare_traits.hpp
*
* Container comparison traits for the djinterp framework.
*   Provides compile-time detection of comparison capabilities at three
* levels:
*
*   1. Element comparison:    can the container's value_type be
*      compared via ==, <, <=>, etc.?
*   2. Container comparison:  does the container itself support ==,
*      <, <=> as a whole?
*   3. Cross-container:       can two different container types be
*      compared element-wise?  Are their value_types compatible?
*
*   Classifies the highest "degree" of comparison available:
*     - none:           no comparison possible
*     - equality:       == and != only
*     - partial_order:  < but not guaranteed total
*     - total_order:    < with strict weak ordering
*     - three_way:      <=> returning an ordering type (C++20)
*
*   This module is the prerequisite for the conversion tier system
* (container_conversion_traits.hpp) — conversion between container
* types requires knowing whether their elements are comparable.
*
* DEPENDENCIES:
*   container_traits.hpp  — container classification
*   cpp_named98.hpp       — is_equality_comparable, is_less_than_comparable
*   cpp_named11.hpp       — is_compare, is_binary_predicate
*
* TABLE OF CONTENTS
* =================
* I.      Safe Value Type Helper
* II.     Element Same-Type Comparison Detection
* III.    Container-Level Comparison Detection
* IV.     Comparison Degree Classification
* V.      Cross-Container Element Compatibility
* VI.     Cross-Container Comparison Detection
* VII.    Convenience Predicates
* VIII.   Combined Classification
*
*
* path:      \inc\container\meta\container_compare_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_CONTAINER_COMPARE_TRAITS_
#define DJINTERP_CONTAINER_COMPARE_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "..\djinterp.hpp"
#include "..\type_traits.hpp"
#include "container_traits.hpp"

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    #include <compare>
#endif


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Safe Value Type Helper
// =============================================================================

NS_INTERNAL

    template<typename _Type, typename = void>
    struct cmp_safe_value_type
    {
        using type = void;
    };

    template<typename _Type>
    struct cmp_safe_value_type<_Type,
        std::void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    template<typename _Type>
    using cmp_safe_value_type_t =
        typename cmp_safe_value_type<_Type>::type;

NS_END  // internal


// =============================================================================
// II.  Element Same-Type Comparison Detection
// =============================================================================
// Detects which comparison operators the container's
// value_type supports when compared against itself.

NS_INTERNAL

    // --- a == b ---
    template<typename _E, typename = void>
    struct elem_has_eq : std::false_type
    {};

    template<typename _E>
    struct elem_has_eq<_E,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _E&>() ==
                std::declval<const _E&>()))>>
        : std::true_type
    {};

    // --- a != b ---
    template<typename _E, typename = void>
    struct elem_has_ne : std::false_type
    {};

    template<typename _E>
    struct elem_has_ne<_E,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _E&>() !=
                std::declval<const _E&>()))>>
        : std::true_type
    {};

    // --- a < b ---
    template<typename _E, typename = void>
    struct elem_has_lt : std::false_type
    {};

    template<typename _E>
    struct elem_has_lt<_E,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _E&>() <
                std::declval<const _E&>()))>>
        : std::true_type
    {};

    // --- a <= b ---
    template<typename _E, typename = void>
    struct elem_has_le : std::false_type
    {};

    template<typename _E>
    struct elem_has_le<_E,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _E&>() <=
                std::declval<const _E&>()))>>
        : std::true_type
    {};

    // --- a > b ---
    template<typename _E, typename = void>
    struct elem_has_gt : std::false_type
    {};

    template<typename _E>
    struct elem_has_gt<_E,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _E&>() >
                std::declval<const _E&>()))>>
        : std::true_type
    {};

    // --- a >= b ---
    template<typename _E, typename = void>
    struct elem_has_ge : std::false_type
    {};

    template<typename _E>
    struct elem_has_ge<_E,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _E&>() >=
                std::declval<const _E&>()))>>
        : std::true_type
    {};

    // --- a <=> b (C++20) ---
    template<typename _E, typename = void>
    struct elem_has_spaceship : std::false_type
    {};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    template<typename _E>
    struct elem_has_spaceship<_E,
        std::void_t<decltype(
            std::declval<const _E&>() <=>
            std::declval<const _E&>())>>
        : std::true_type
    {};
#endif

NS_END  // internal

// has_equality_comparable_elements
//   type trait: true if value_type supports == and !=.
template<typename _Type>
struct has_equality_comparable_elements
{
    using elem_type =
        internal::cmp_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        ( internal::elem_has_eq<elem_type>::value &&
          internal::elem_has_ne<elem_type>::value );
};

template<typename _Type>
inline constexpr bool
    has_equality_comparable_elements_v =
        has_equality_comparable_elements<
            _Type>::value;

// has_less_than_comparable_elements
//   type trait: true if value_type supports <.
template<typename _Type>
struct has_less_than_comparable_elements
{
    using elem_type =
        internal::cmp_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        internal::elem_has_lt<elem_type>::value;
};

template<typename _Type>
inline constexpr bool
    has_less_than_comparable_elements_v =
        has_less_than_comparable_elements<
            _Type>::value;

// has_totally_ordered_elements
//   type trait: true if value_type supports all four
// relational operators (< <= > >=) plus equality.
template<typename _Type>
struct has_totally_ordered_elements
{
    using elem_type =
        internal::cmp_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        ( internal::elem_has_eq<elem_type>::value &&
          internal::elem_has_ne<elem_type>::value &&
          internal::elem_has_lt<elem_type>::value &&
          internal::elem_has_le<elem_type>::value &&
          internal::elem_has_gt<elem_type>::value &&
          internal::elem_has_ge<elem_type>::value );
};

template<typename _Type>
inline constexpr bool
    has_totally_ordered_elements_v =
        has_totally_ordered_elements<_Type>::value;

// has_three_way_comparable_elements
//   type trait: true if value_type supports <=> (C++20).
template<typename _Type>
struct has_three_way_comparable_elements
{
    using elem_type =
        internal::cmp_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        internal::elem_has_spaceship<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool
    has_three_way_comparable_elements_v =
        has_three_way_comparable_elements<
            _Type>::value;


// =============================================================================
// III. Container-Level Comparison Detection
// =============================================================================
// Detects whether the container type itself has comparison
// operators (not just its elements).

NS_INTERNAL

    // --- C == C ---
    template<typename _C, typename = void>
    struct container_has_eq : std::false_type
    {};

    template<typename _C>
    struct container_has_eq<_C,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _C&>() ==
                std::declval<const _C&>()))>>
        : std::true_type
    {};

    // --- C != C ---
    template<typename _C, typename = void>
    struct container_has_ne : std::false_type
    {};

    template<typename _C>
    struct container_has_ne<_C,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _C&>() !=
                std::declval<const _C&>()))>>
        : std::true_type
    {};

    // --- C < C ---
    template<typename _C, typename = void>
    struct container_has_lt : std::false_type
    {};

    template<typename _C>
    struct container_has_lt<_C,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _C&>() <
                std::declval<const _C&>()))>>
        : std::true_type
    {};

    // --- C <=> C (C++20) ---
    template<typename _C, typename = void>
    struct container_has_spaceship : std::false_type
    {};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    template<typename _C>
    struct container_has_spaceship<_C,
        std::void_t<decltype(
            std::declval<const _C&>() <=>
            std::declval<const _C&>())>>
        : std::true_type
    {};
#endif

NS_END  // internal

// is_equality_comparable_container
//   type trait: true if container supports == and !=.
template<typename _Type>
struct is_equality_comparable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( internal::container_has_eq<
              clean_type>::value &&
          internal::container_has_ne<
              clean_type>::value );
};

template<typename _Type>
inline constexpr bool
    is_equality_comparable_container_v =
        is_equality_comparable_container<
            _Type>::value;

// is_less_than_comparable_container
//   type trait: true if container supports <.
template<typename _Type>
struct is_less_than_comparable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::container_has_lt<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool
    is_less_than_comparable_container_v =
        is_less_than_comparable_container<
            _Type>::value;

// is_three_way_comparable_container
//   type trait: true if container supports <=>.
template<typename _Type>
struct is_three_way_comparable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::container_has_spaceship<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool
    is_three_way_comparable_container_v =
        is_three_way_comparable_container<
            _Type>::value;


// =============================================================================
// IV.  Comparison Degree Classification
// =============================================================================

// DComparisonDegree
//   enum: classifies the highest comparison capability
// available.
enum class DComparisonDegree
{
    // no comparison possible
    none,

    // == and != only
    equality,

    // < available but not full relational set
    partial_order,

    // all relational operators (< <= > >= == !=)
    total_order,

    // <=> returning an ordering type (C++20)
    three_way
};

NS_INTERNAL

    template<typename _Type>
    struct element_comparison_degree_impl
    {
        using elem_type =
            cmp_safe_value_type_t<clean_t<_Type>>;

        static constexpr DComparisonDegree value =
            elem_has_spaceship<elem_type>::value
                ? DComparisonDegree::three_way

            : ( elem_has_eq<elem_type>::value &&
                elem_has_lt<elem_type>::value &&
                elem_has_le<elem_type>::value &&
                elem_has_gt<elem_type>::value &&
                elem_has_ge<elem_type>::value )
                ? DComparisonDegree::total_order

            : elem_has_lt<elem_type>::value
                ? DComparisonDegree::partial_order

            : elem_has_eq<elem_type>::value
                ? DComparisonDegree::equality

            : DComparisonDegree::none;
    };

    template<typename _Type>
    struct container_comparison_degree_impl
    {
        using C = clean_t<_Type>;

        static constexpr DComparisonDegree value =
            container_has_spaceship<C>::value
                ? DComparisonDegree::three_way

            : ( container_has_eq<C>::value &&
                container_has_lt<C>::value )
                ? DComparisonDegree::total_order

            : container_has_lt<C>::value
                ? DComparisonDegree::partial_order

            : container_has_eq<C>::value
                ? DComparisonDegree::equality

            : DComparisonDegree::none;
    };

NS_END  // internal

// element_comparison_degree
//   type trait: the highest comparison degree the
// container's value_type supports.
template<typename _Type>
struct element_comparison_degree
{
    static constexpr DComparisonDegree value =
        internal::element_comparison_degree_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DComparisonDegree
    element_comparison_degree_v =
        element_comparison_degree<_Type>::value;

// container_comparison_degree
//   type trait: the highest comparison degree the
// container itself supports.
template<typename _Type>
struct container_comparison_degree
{
    static constexpr DComparisonDegree value =
        internal::container_comparison_degree_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DComparisonDegree
    container_comparison_degree_v =
        container_comparison_degree<_Type>::value;


// =============================================================================
// V.   Cross-Container Element Compatibility
// =============================================================================
// Determines whether two different container types have
// compatible elements for comparison or conversion.

// elements_same_type
//   type trait: true if both containers have the same
// value_type.
template<typename _A,
         typename _B>
struct elements_same_type
{
    using elem_a =
        internal::cmp_safe_value_type_t<
            clean_t<_A>>;
    using elem_b =
        internal::cmp_safe_value_type_t<
            clean_t<_B>>;

    static constexpr bool value =
        std::is_same_v<elem_a, elem_b>;
};

template<typename _A,
         typename _B>
inline constexpr bool elements_same_type_v =
    elements_same_type<_A, _B>::value;

// elements_convertible
//   type trait: true if value_type of _A is implicitly
// convertible to value_type of _B.
template<typename _A,
         typename _B>
struct elements_convertible
{
    using elem_a =
        internal::cmp_safe_value_type_t<
            clean_t<_A>>;
    using elem_b =
        internal::cmp_safe_value_type_t<
            clean_t<_B>>;

    static constexpr bool value =
        std::is_convertible_v<elem_a, elem_b>;
};

template<typename _A,
         typename _B>
inline constexpr bool elements_convertible_v =
    elements_convertible<_A, _B>::value;

// elements_mutually_convertible
//   type trait: true if value_types are convertible in
// both directions.
template<typename _A,
         typename _B>
struct elements_mutually_convertible
{
    static constexpr bool value =
        ( elements_convertible_v<_A, _B> &&
          elements_convertible_v<_B, _A> );
};

template<typename _A,
         typename _B>
inline constexpr bool
    elements_mutually_convertible_v =
        elements_mutually_convertible<
            _A, _B>::value;


// =============================================================================
// VI.  Cross-Container Comparison Detection
// =============================================================================
// Detects whether elements of two different container types
// can be compared with each other (cross-type == and <).

NS_INTERNAL

    // --- elem_a == elem_b ---
    template<typename _EA,
             typename _EB,
             typename = void>
    struct cross_elem_has_eq : std::false_type
    {};

    template<typename _EA,
             typename _EB>
    struct cross_elem_has_eq<_EA, _EB,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _EA&>() ==
                std::declval<const _EB&>()))>>
        : std::true_type
    {};

    // --- elem_a < elem_b ---
    template<typename _EA,
             typename _EB,
             typename = void>
    struct cross_elem_has_lt : std::false_type
    {};

    template<typename _EA,
             typename _EB>
    struct cross_elem_has_lt<_EA, _EB,
        std::void_t<decltype(
            static_cast<bool>(
                std::declval<const _EA&>() <
                std::declval<const _EB&>()))>>
        : std::true_type
    {};

    // --- elem_a <=> elem_b (C++20) ---
    template<typename _EA,
             typename _EB,
             typename = void>
    struct cross_elem_has_spaceship : std::false_type
    {};

#if D_ENV_CPP_FEATURE_LANG_IMPL_THREE_WAY_COMPARISON
    template<typename _EA,
             typename _EB>
    struct cross_elem_has_spaceship<_EA, _EB,
        std::void_t<decltype(
            std::declval<const _EA&>() <=>
            std::declval<const _EB&>())>>
        : std::true_type
    {};
#endif

NS_END  // internal

// elements_cross_equality_comparable
//   type trait: true if value_type of _A can be compared
// with value_type of _B via ==.
template<typename _A,
         typename _B>
struct elements_cross_equality_comparable
{
    using elem_a =
        internal::cmp_safe_value_type_t<
            clean_t<_A>>;
    using elem_b =
        internal::cmp_safe_value_type_t<
            clean_t<_B>>;

    static constexpr bool value =
        internal::cross_elem_has_eq<
            elem_a, elem_b>::value;
};

template<typename _A,
         typename _B>
inline constexpr bool
    elements_cross_equality_comparable_v =
        elements_cross_equality_comparable<
            _A, _B>::value;

// elements_cross_less_than_comparable
//   type trait: true if value_type of _A can be compared
// with value_type of _B via <.
template<typename _A,
         typename _B>
struct elements_cross_less_than_comparable
{
    using elem_a =
        internal::cmp_safe_value_type_t<
            clean_t<_A>>;
    using elem_b =
        internal::cmp_safe_value_type_t<
            clean_t<_B>>;

    static constexpr bool value =
        internal::cross_elem_has_lt<
            elem_a, elem_b>::value;
};

template<typename _A,
         typename _B>
inline constexpr bool
    elements_cross_less_than_comparable_v =
        elements_cross_less_than_comparable<
            _A, _B>::value;

// elements_cross_three_way_comparable
//   type trait: true if value_type of _A can be compared
// with value_type of _B via <=>.
template<typename _A,
         typename _B>
struct elements_cross_three_way_comparable
{
    using elem_a =
        internal::cmp_safe_value_type_t<
            clean_t<_A>>;
    using elem_b =
        internal::cmp_safe_value_type_t<
            clean_t<_B>>;

    static constexpr bool value =
        internal::cross_elem_has_spaceship<
            elem_a, elem_b>::value;
};

template<typename _A,
         typename _B>
inline constexpr bool
    elements_cross_three_way_comparable_v =
        elements_cross_three_way_comparable<
            _A, _B>::value;

// cross_comparison_degree
//   type trait: the highest comparison degree between two
// containers' element types.
template<typename _A,
         typename _B>
struct cross_comparison_degree
{
    using elem_a =
        internal::cmp_safe_value_type_t<
            clean_t<_A>>;
    using elem_b =
        internal::cmp_safe_value_type_t<
            clean_t<_B>>;

    static constexpr DComparisonDegree value =
        internal::cross_elem_has_spaceship<
            elem_a, elem_b>::value
            ? DComparisonDegree::three_way

        : ( internal::cross_elem_has_eq<
                elem_a, elem_b>::value &&
            internal::cross_elem_has_lt<
                elem_a, elem_b>::value )
            ? DComparisonDegree::total_order

        : internal::cross_elem_has_lt<
              elem_a, elem_b>::value
            ? DComparisonDegree::partial_order

        : internal::cross_elem_has_eq<
              elem_a, elem_b>::value
            ? DComparisonDegree::equality

        : DComparisonDegree::none;
};

template<typename _A,
         typename _B>
inline constexpr DComparisonDegree
    cross_comparison_degree_v =
        cross_comparison_degree<_A, _B>::value;


// =============================================================================
// VII. Convenience Predicates
// =============================================================================

// has_any_element_comparison
//   type trait: true if elements support at least ==.
template<typename _Type>
struct has_any_element_comparison
{
    static constexpr bool value =
        ( element_comparison_degree_v<_Type> !=
          DComparisonDegree::none );
};

template<typename _Type>
inline constexpr bool has_any_element_comparison_v =
    has_any_element_comparison<_Type>::value;

// has_any_container_comparison
//   type trait: true if container supports at least ==.
template<typename _Type>
struct has_any_container_comparison
{
    static constexpr bool value =
        ( container_comparison_degree_v<_Type> !=
          DComparisonDegree::none );
};

template<typename _Type>
inline constexpr bool
    has_any_container_comparison_v =
        has_any_container_comparison<_Type>::value;

// containers_element_comparable
//   type trait: true if two containers' elements can be
// compared at any degree.
template<typename _A,
         typename _B>
struct containers_element_comparable
{
    static constexpr bool value =
        ( cross_comparison_degree_v<_A, _B> !=
          DComparisonDegree::none );
};

template<typename _A,
         typename _B>
inline constexpr bool
    containers_element_comparable_v =
        containers_element_comparable<
            _A, _B>::value;

// is_sortable_container
//   type trait: true if the container is iterable and its
// elements support < (minimum for std::sort).
template<typename _Type>
struct is_sortable_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_iterable_container_v<clean_type> &&
          has_less_than_comparable_elements_v<
              clean_type> );
};

template<typename _Type>
inline constexpr bool is_sortable_container_v =
    is_sortable_container<_Type>::value;

// has_custom_comparator_type
//   type trait: true if container exposes a key_compare or
// value_compare alias (associative containers).
D_TYPE_TRAIT_TRUE(has_key_compare_type,
    typename _Type::key_compare)

D_TYPE_TRAIT_TRUE(has_value_compare_type,
    typename _Type::value_compare)

template<typename _Type>
struct has_custom_comparator_type
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_key_compare_type_v<clean_type> ||
          has_value_compare_type_v<clean_type> );
};

template<typename _Type>
inline constexpr bool
    has_custom_comparator_type_v =
        has_custom_comparator_type<_Type>::value;

// has_hash_function_type
//   type trait: true if container exposes a hasher type
// (unordered associative containers).
D_TYPE_TRAIT_TRUE(has_hasher_type,
    typename _Type::hasher)

D_TYPE_TRAIT_TRUE(has_key_equal_type,
    typename _Type::key_equal)


// =============================================================================
// VIII. Combined Classification
// =============================================================================

// container_compare_class (single-container)
//   struct: complete comparison classification for one
// container type.
template<typename _Type>
struct container_compare_class
{
    // element-level
    static constexpr bool elems_eq =
        has_equality_comparable_elements_v<_Type>;
    static constexpr bool elems_lt =
        has_less_than_comparable_elements_v<_Type>;
    static constexpr bool elems_total =
        has_totally_ordered_elements_v<_Type>;
    static constexpr bool elems_three_way =
        has_three_way_comparable_elements_v<_Type>;
    static constexpr DComparisonDegree
        elem_degree =
            element_comparison_degree_v<_Type>;

    // container-level
    static constexpr bool container_eq =
        is_equality_comparable_container_v<_Type>;
    static constexpr bool container_lt =
        is_less_than_comparable_container_v<_Type>;
    static constexpr bool container_three_way =
        is_three_way_comparable_container_v<_Type>;
    static constexpr DComparisonDegree
        container_degree =
            container_comparison_degree_v<_Type>;

    // comparator types
    static constexpr bool has_comparator =
        has_custom_comparator_type_v<_Type>;
    static constexpr bool has_hasher =
        has_hasher_type_v<_Type>;

    // aggregate
    static constexpr bool is_comparable =
        has_any_element_comparison_v<_Type>;
    static constexpr bool is_sortable =
        is_sortable_container_v<_Type>;
};

// container_cross_compare_class (two containers)
//   struct: complete cross-container comparison
// classification.
template<typename _A,
         typename _B>
struct container_cross_compare_class
{
    // element compatibility
    static constexpr bool same_value_type =
        elements_same_type_v<_A, _B>;
    static constexpr bool a_to_b_convertible =
        elements_convertible_v<_A, _B>;
    static constexpr bool b_to_a_convertible =
        elements_convertible_v<_B, _A>;
    static constexpr bool mutually_convertible =
        elements_mutually_convertible_v<_A, _B>;

    // cross comparison
    static constexpr bool cross_eq =
        elements_cross_equality_comparable_v<
            _A, _B>;
    static constexpr bool cross_lt =
        elements_cross_less_than_comparable_v<
            _A, _B>;
    static constexpr bool cross_three_way =
        elements_cross_three_way_comparable_v<
            _A, _B>;
    static constexpr DComparisonDegree
        cross_degree =
            cross_comparison_degree_v<_A, _B>;

    // aggregate
    static constexpr bool is_comparable =
        containers_element_comparable_v<_A, _B>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_COMPARE_TRAITS_
