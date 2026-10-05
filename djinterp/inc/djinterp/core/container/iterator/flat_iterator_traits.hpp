/*******************************************************************************
* djinterp [core]                                       flat_iterator_traits.hpp
*
* Flat iterator traits for the djinterp container framework.
*   Detects whether a container's structure can be flattened into a
* single linear sequence, and classifies the best flattening
* strategy.
*
*   Flattenable containers fall into two categories:
*
*     1. Nested:        value_type is itself iterable (e.g.
*                       vector<vector<int>>, deque<string>).
*                       Flattening concatenates inner ranges.
*
*     2. Hierarchical:  container exposes children() returning
*                       child nodes (tree, graph, DOM).
*                       Flattening traverses DFS or BFS.
*
*   A container that is already flat and non-nested is trivially
* "flattenable" (identity - just iterate normally).
*
* DEPENDENCIES:
*   container_traits.hpp   - hierarchy detection, iterability
*   iterator_traits.hpp    - iterator level detection
*
*
* path:      /inc/djinterp/core/container/iterator/flat_iterator_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.24
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Nested Iterable Detection
      -------------------------

II.   Hierarchical Flatten Detection
      ------------------------------

III.  Flatten Strategy Classification
      -------------------------------

IV.   Leaf Type Extraction
      --------------------

V.    Convenience Predicates
      ----------------------

VI.   Combined Classification
      -----------------------
*/

#ifndef DJINTERP_CONTAINER_ITERATOR_FLAT_ITERATOR_TRAITS_HPP
#define DJINTERP_CONTAINER_ITERATOR_FLAT_ITERATOR_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../traits/container_traits.hpp"
#include "./iterator_traits.hpp"
#include "../traits/hierarchical_container_traits.hpp"  // is_hierarchical_container_v
#include "../traits/flat_container_traits.hpp"          // is_flat_container_v


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// ===========================================================================
// I.   Nested Iterable Detection
// ===========================================================================
// Detects whether the container's value_type is itself an
// iterable range, enabling recursive flattening of nested
// containers (vector<vector<T>>, list<deque<T>>, etc.).

NS_INTERNAL

    template<typename Type, typename = void>
    struct flat_safe_value_type
    {
        using type = void;
    };

    template<typename Type>
    struct flat_safe_value_type<Type,
        std::void_t<typename Type::value_type>>
    {
        using type = typename Type::value_type;
    };

    template<typename Type>
    using flat_safe_value_type_t =
        typename flat_safe_value_type<Type>::type;

    // is_iterable_element
    //   helper: true if the container's value_type is itself iterable via
    // begin()/end().
    template<typename Elem, typename = void>
    struct is_iterable_element : std::false_type
    {};

    // is_iterable_element<Elem, std::void_t< decltype(std::begin(
    // std::declval<const Elem&>())), decltype(std::end( std::declval<const
    // Elem&>())) >>
    //   trait: the `std::void_t< decltype(std::begin( std::declval<const
    // Elem&>())), decltype(std::end( std::declval<const Elem&>())) >` case;
    // it reports true.
    template<typename Elem>
    struct is_iterable_element<Elem,
        std::void_t<
            decltype(std::begin(
                std::declval<const Elem&>())),
            decltype(std::end(
                std::declval<const Elem&>()))
        >> : std::true_type
    {};

    template<typename Elem, typename = void>
    struct nested_value_type
    {
        using type = void;
    };

    template<typename Elem>
    struct nested_value_type<Elem,
        std::void_t<decltype(
            *std::begin(
                std::declval<const Elem&>()))>>
    {
        using type = typename std::decay<
            decltype(*std::begin(
                std::declval<const Elem&>()))
        >::type;
    };

    template<typename Elem>
    using nested_value_type_t =
        typename nested_value_type<Elem>::type;

    // depth-2 nesting: value_type's value_type is
    template<typename Elem, typename = void>
    struct is_deeply_nested : std::false_type
    {};

    template<typename Elem>
    struct is_deeply_nested<Elem,
        std::enable_if_t<
            is_iterable_element<Elem>::value &&
            is_iterable_element<
                nested_value_type_t<Elem>
            >::value
        >> : std::true_type
    {};

NS_END  // internal

// has_nested_iterable_elements
//   type trait: true if container's value_type is itself iterable (depth-1
// nesting).
template<typename Type>
struct has_nested_iterable_elements
{
    using elem_type =
        internal::flat_safe_value_type_t<
            clean_t<Type>>;

    static constexpr bool value =
        internal::is_iterable_element<
            elem_type>::value;
};

template<typename Type>
inline constexpr bool
    has_nested_iterable_elements_v =
        has_nested_iterable_elements<Type>::value;

// has_deeply_nested_elements
//   type trait: true if nesting depth >= 2 (value_type's value_type is also
// iterable).
template<typename Type>
struct has_deeply_nested_elements
{
    using elem_type =
        internal::flat_safe_value_type_t<
            clean_t<Type>>;

    static constexpr bool value =
        internal::is_deeply_nested<
            elem_type>::value;
};

template<typename Type>
inline constexpr bool
    has_deeply_nested_elements_v =
        has_deeply_nested_elements<Type>::value;

// nesting_depth
//   type trait: compile-time nesting depth. 0 = flat, 1 = vector<vector<T>>, 2
// = vector<vector<vector<T>>>
NS_INTERNAL

    template<typename Type,
             bool = is_iterable_element<Type>::value>
    struct nesting_depth_impl
    {
        static constexpr std::size_t value = 0;
    };

    template<typename Type>
    struct nesting_depth_impl<Type, true>
    {
        static constexpr std::size_t value =
            1 + nesting_depth_impl<
                    nested_value_type_t<Type>
                >::value;
    };

NS_END  // internal

template<typename Type>
struct nesting_depth
{
    using elem_type =
        internal::flat_safe_value_type_t<
            clean_t<Type>>;

    static constexpr std::size_t value =
        internal::nesting_depth_impl<
            elem_type>::value;
};

template<typename Type>
inline constexpr std::size_t nesting_depth_v =
    nesting_depth<Type>::value;


// ===========================================================================
// II.  Hierarchical Flatten Detection
// ===========================================================================
// Detects whether a hierarchical container can be
// flattened via DFS or BFS traversal of its children().

// is_hierarchy_flattenable
//   type trait: true if container is hierarchical and its children() returns
// an iterable of the same type (or a compatible node type).
template<typename Type>
struct is_hierarchy_flattenable
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( is_hierarchical_container_v<clean_type> &&
          has_children_accessor_v<clean_type> );
};

template<typename Type>
inline constexpr bool is_hierarchy_flattenable_v =
    is_hierarchy_flattenable<Type>::value;

// has_sibling_accessor
//   type trait: true if container/node has a next_sibling() method for
// sibling-order traversal.
D_TYPE_TRAIT_DETECTED(has_sibling_accessor,
    decltype(
        std::declval<const Type&>().next_sibling()))

// has_prev_sibling_accessor
D_TYPE_TRAIT_DETECTED(has_prev_sibling_accessor,
    decltype(std::declval<const Type&>()
        .prev_sibling()))

// has_is_leaf_method
D_TYPE_TRAIT_DETECTED(has_is_leaf_method,
    decltype(
        std::declval<const Type&>().is_leaf()))

// has_is_root_method
D_TYPE_TRAIT_DETECTED(has_is_root_method,
    decltype(
        std::declval<const Type&>().is_root()))

// has_child_count_method
D_TYPE_TRAIT_DETECTED(has_child_count_method,
    decltype(
        std::declval<const Type&>().child_count()))


// ===========================================================================
// III. Flatten Strategy Classification
// ===========================================================================

// flatten_strategy
//   enum: compile-time flattening approach.
enum class flatten_strategy
{
    // already flat - identity iteration
    identity,

    // depth-1 nested - concatenate inner ranges
    concat,

    // depth-2+ nested - recursive concatenation
    recursive,

    // hierarchical - DFS traversal via children()
    dfs,

    // hierarchical - BFS traversal via children()
    bfs,

    // not flattenable
    unsupported
};

NS_INTERNAL

    template<typename Type>
    struct flatten_strategy_impl
    {
        using C = clean_t<Type>;

        static constexpr flatten_strategy value =
            // hierarchical with children()
            is_hierarchy_flattenable_v<C>
                ? flatten_strategy::dfs

            // deeply nested (depth >= 2)
            : has_deeply_nested_elements_v<C>
                ? flatten_strategy::recursive

            // depth-1 nested
            : has_nested_iterable_elements_v<C>
                ? flatten_strategy::concat

            // already flat and iterable
            : is_iterable_container_v<C>
                ? flatten_strategy::identity

            : flatten_strategy::unsupported;
    };

NS_END  // internal

template<typename Type>
struct container_flatten_strategy
{
    static constexpr flatten_strategy value =
        internal::flatten_strategy_impl<
            Type>::value;
};

template<typename Type>
inline constexpr flatten_strategy
    container_flatten_strategy_v =
        container_flatten_strategy<Type>::value;


// ===========================================================================
// IV.  Leaf Type Extraction
// ===========================================================================
// Extracts the innermost (leaf) element type after full
// recursive flattening.

NS_INTERNAL

    template<typename Type,
             bool = is_iterable_element<Type>::value>
    struct leaf_type_impl
    {
        using type = Type;
    };

    template<typename Type>
    struct leaf_type_impl<Type, true>
    {
        using type =
            typename leaf_type_impl<
                nested_value_type_t<Type>
            >::type;
    };

NS_END  // internal

// leaf_element_type
//   type trait: the innermost element type after recursive flattening. For
// vector<vector<int>>, this is int.
template<typename Type>
struct leaf_element_type
{
    using elem_type =
        internal::flat_safe_value_type_t<
            clean_t<Type>>;

    using type =
        typename internal::leaf_type_impl<
            elem_type>::type;
};

// leaf_element_type_t
//   type: the carrier of leaf_element_type -- its `::type`, for use where a
// type rather than a value is wanted.
template<typename Type>
using leaf_element_type_t =
    typename leaf_element_type<Type>::type;


// ===========================================================================
// V.   Convenience Predicates
// ===========================================================================

// is_flattenable
//   type trait: true if the container can be flattened via any mechanism.
template<typename Type>
struct is_flattenable
{
    static constexpr bool value =
        ( container_flatten_strategy_v<Type> !=
          flatten_strategy::unsupported  &&
          container_flatten_strategy_v<Type> !=
          flatten_strategy::identity );
};

template<typename Type>
inline constexpr bool is_flattenable_v =
    is_flattenable<Type>::value;

// needs_recursive_flatten
//   type trait: true if flattening requires recursion (depth >= 2 nesting or
// hierarchical traversal).
template<typename Type>
struct needs_recursive_flatten
{
    static constexpr flatten_strategy s =
        container_flatten_strategy_v<Type>;

    static constexpr bool value =
        ( s == flatten_strategy::recursive ||
          s == flatten_strategy::dfs       ||
          s == flatten_strategy::bfs );
};

template<typename Type>
inline constexpr bool needs_recursive_flatten_v =
    needs_recursive_flatten<Type>::value;


// ===========================================================================
// VI.  Combined Classification
// ===========================================================================

template<typename Type>
struct flat_iterator_class
{
    // nesting
    static constexpr bool nested_elements =
        has_nested_iterable_elements_v<Type>;
    static constexpr bool deeply_nested =
        has_deeply_nested_elements_v<Type>;
    static constexpr std::size_t depth =
        nesting_depth_v<Type>;

    // hierarchical
    static constexpr bool hierarchy_flattenable =
        is_hierarchy_flattenable_v<Type>;
    static constexpr bool has_siblings =
        has_sibling_accessor_v<Type>;
    static constexpr bool has_is_leaf =
        has_is_leaf_method_v<Type>;
    static constexpr bool has_child_count =
        has_child_count_method_v<Type>;

    // strategy
    static constexpr flatten_strategy strategy =
        container_flatten_strategy_v<Type>;

    // aggregate
    static constexpr bool is_flattenable =
        is_flattenable_v<Type>;
    static constexpr bool recursive =
        needs_recursive_flatten_v<Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ITERATOR_FLAT_ITERATOR_TRAITS_HPP
