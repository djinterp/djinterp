/*******************************************************************************
* djinterp [core]                                    sorted_container_traits.hpp
*
*   The SORTEDNESS axis, container-side traits: does a container's TYPE
* promise
* its elements are kept in order (is_sorted_container), on what monotonicity
* footing (sortedness: unsorted / monotone / strictly-sorted), and may a
* holder
* therefore rely on ordered enumeration (admits_sorted_enumeration). These are
* the compile-time CLASSIFIERS of the sortedness axis; the instance-level
* check
* that walks a particular object's elements (is_sorted_range) is an OPERATION
* and
* lives in container/sorted_container.hpp, which includes this header.
*
*   Sortedness sits just above the order axis: a sorted container is an
* ordered
* one whose order is a maintained key invariant, so is_ordered_container /
* is_unordered_container are sourced from ordered_container_traits.hpp rather
* than
* re-derived here. A type opts in through a `sorted_invariant` marker
* (honoured
* first); otherwise the standard sorted associatives are recognised
* structurally.
*
*   PORTABILITY:
*   C++11 baseline.  Each `_v` companion is emitted through the trait_detect
* macros (inline variable on C++17+, variable template on C++14, absent on
* C++11).
*
*
* path:      /inc/djinterp/core/container/traits/sorted_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.01
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TRAITS_SORTED_CONTAINER_TRAITS_HPP
#define DJINTERP_CONTAINER_TRAITS_SORTED_CONTAINER_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"                 // clean_t, NS_*, D_ENV_* feature macros
#include "../../meta/trait_detect.hpp"        // D_TYPE_TRAIT_* detection macros
#include "./ordered_container_traits.hpp"     // is_ordered_container, is_unordered_container


NS_DJINTERP


// ===========================================================================
// I.   Sortedness signals
// ===========================================================================

NS_INTERNAL

    // has_key_compare_helper
    //   helper: detects a `key_compare` alias - the comparator of an ordered
    // associative container. Its presence is what lets an (unordered)
    // associative enumerate monotonically; a hash-ordered container has no
    // such comparator.
    template<typename Type,
             typename = void>
    struct has_key_compare_helper : std::false_type
    {};

    template<typename Type>
    struct has_key_compare_helper<Type,
        D_VOID_T<typename clean_t<Type>::key_compare>>
        : std::true_type
    {};

    // has_interval_bounds_helper
    //   helper: detects static `lower_bound` AND `upper_bound` - a
    // closed-interval carrier, whose arithmetic enumeration is monotone by
    // construction.
    template<typename Type,
             typename = void>
    struct has_interval_bounds_helper : std::false_type
    {};

    // has_interval_bounds_helper specialization
    //   helper: the detected case -- selected when the type declares the
    // static members `lower_bound` and `upper_bound`.
    template<typename Type>
    struct has_interval_bounds_helper<Type,
        D_VOID_T<decltype(clean_t<Type>::lower_bound),
                 decltype(clean_t<Type>::upper_bound)>>
        : std::true_type
    {};

    // sorted_invariant_helper
    //   helper: reads the opt-in static `sorted_invariant` constant, by which
    // an ordered sequence asserts it is maintained in sorted order.
    template<typename Type,
             typename = void>
    struct sorted_invariant_helper
    {
        static constexpr bool value = false;
    };

    template<typename Type>
    struct sorted_invariant_helper<Type,
        D_VOID_T<decltype(clean_t<Type>::sorted_invariant)>>
    {
        static constexpr bool value =
            static_cast<bool>(clean_t<Type>::sorted_invariant);
    };

NS_END  // internal


// ===========================================================================
// II.  Sortedness classification
// ===========================================================================

// sortedness
//   enum: a container's position on the sortedness axis.
enum class sortedness
{
    non_container,    // not an (iterable) container
    unordered,        // unordered, no comparator: sortedness not applicable
    monotone,         // unordered but comparator-equipped: sorted-by-construction enumeration
    order_dependent,  // ordered: sortedness is a property of the instance
    sorted            // ordered AND guaranteed in comparator order (interval / opt-in)
};

// sortedness_name
//   function: a stable spelling, for diagnostics and agent-facing summaries.
constexpr const char*
sortedness_name(sortedness _s) noexcept
{
    return ( _s == sortedness::non_container   ? "non_container"
           : _s == sortedness::unordered       ? "unordered"
           : _s == sortedness::monotone        ? "monotone"
           : _s == sortedness::order_dependent ? "order_dependent"
           :                                     "sorted" );
}

// sortedness_of
//   trait: classifies a type. An unordered container is monotone when it
// carries a comparator, else unordered; an ordered container is sorted when it
// guarantees comparator order (interval bounds or an opt-in invariant), else
// order_dependent.
template<typename Type>
struct sortedness_of
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr sortedness value =
        ( is_unordered_container<clean_type>::value )
              ? ( internal::has_key_compare_helper<clean_type>::value
                      ? sortedness::monotone
                      : sortedness::unordered )
      : ( is_ordered_container<clean_type>::value )
              ? ( (    internal::has_interval_bounds_helper<clean_type>::value
                    || internal::sorted_invariant_helper<clean_type>::value )
                      ? sortedness::sorted
                      : sortedness::order_dependent )
      :         sortedness::non_container;

    using type = std::integral_constant<sortedness, value>;
};

// sortedness_of_t / sortedness_of_v
//   type / value: the carrier and, where the language permits, the value
// companion of sortedness_of.
template<typename Type>
using sortedness_of_t = typename sortedness_of<Type>::type;

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename Type>
    inline constexpr sortedness sortedness_of_v =
        sortedness_of<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr sortedness sortedness_of_v =
        sortedness_of<Type>::value;
#endif


// ===========================================================================
// III. Classification predicates
// ===========================================================================

// is_sorted_container
//   trait: true iff the type guarantees comparator order along its positions -
// the ordered, sorted-by-construction case (the sorted restriction at type
// level).
template<typename Type>
struct is_sorted_container
    : std::integral_constant<bool,
          sortedness_of<clean_t<Type>>::value == sortedness::sorted>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_sorted_container)

// is_unsorted_container
//   trait: the complement of is_sorted_container over containers - a container
// whose positions are NOT guaranteed in comparator order. It is gated on being
// an (iterable) container so a non-container reports false rather than a
// vacuous true, mirroring the is_ordered / is_unordered complementary pair;
// the
// excluded
// non_container state is what distinguishes this from a bare
// !is_sorted_container.
// The three container states unordered, monotone, and order_dependent are all
// unsorted (only the sorted-by-construction sorted state is not).
template<typename Type>
struct is_unsorted_container
    : std::integral_constant<bool,
             sortedness_of<clean_t<Type>>::value != sortedness::non_container
          && sortedness_of<clean_t<Type>>::value != sortedness::sorted>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_unsorted_container)

// is_monotone_container
//   trait: true iff the type is a comparator-equipped unordered container - it
// has no positions, but its enumeration is sorted by construction.
template<typename Type>
struct is_monotone_container
    : std::integral_constant<bool,
          sortedness_of<clean_t<Type>>::value == sortedness::monotone>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_monotone_container)

// admits_sorted_enumeration
//   trait: true iff enumerating the container is guaranteed to yield
// comparator order - either a monotone (associative) or a sorted (ordered)
// type. This is the property that lets such a container be PRESENTED in sorted
// order.
template<typename Type>
struct admits_sorted_enumeration
    : std::integral_constant<bool,
            is_sorted_container<clean_t<Type>>::value
         || is_monotone_container<clean_t<Type>>::value>
{};

D_TYPE_TRAIT_VALUE_BOOL(admits_sorted_enumeration)


// ===========================================================================
// V.   Aggregate snapshot
// ===========================================================================

// sorted_container_class
//   trait: one-shot snapshot of the sortedness axis for `Type`. Gathers every
// signal and verdict this header computes into a single instantiation, so
// a caller that needs several of them pays for detection once rather than per
// query.
template<typename Type>
struct sorted_container_class
{
private:
    using clean_type = clean_t<Type>;

public:
    static constexpr bool has_comparator =
        internal::has_key_compare_helper<clean_type>::value;
    static constexpr bool interval_domain =
        internal::has_interval_bounds_helper<clean_type>::value;
    static constexpr bool sorted_invariant =
        internal::sorted_invariant_helper<clean_type>::value;

    static constexpr sortedness kind =
        sortedness_of<clean_type>::value;
    static constexpr const char* kind_name =
        sortedness_name(kind);

    static constexpr bool is_sorted_type =
        is_sorted_container<clean_type>::value;
    static constexpr bool is_monotone =
        is_monotone_container<clean_type>::value;
    static constexpr bool sorted_enumeration =
        admits_sorted_enumeration<clean_type>::value;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TRAITS_SORTED_CONTAINER_TRAITS_HPP
