/*******************************************************************************
* djinterp [core]                               hierarchical_iterator_traits.hpp
*
* Hierarchical iterator traits for the djinterp container framework.
*   Detects the tree navigation capabilities of a hierarchical
* container or node type, classifying the supported traversal orders
* and navigation primitives.
*
*   A hierarchical container is one where elements have parent/child
* relationships (trees, DOM, AST, file systems, nested option
* groups, etc.).  This module detects which navigation operations
* the container or its node type supports:
*
*   Topology:    parent(), children(), root()
*   Siblings:    next_sibling(), prev_sibling()
*   Leaf/root:   is_leaf(), is_root()
*   Depth:       depth(), max_depth()
*   Child count: child_count(), child_at(index)
*
*   Based on the detected capabilities, classifies which traversal
* orders are available (pre-order, post-order, in-order, level-
* order, leaf-only) and the best iteration strategy.
*
* DEPENDENCIES:
*   container_traits.hpp   - base hierarchy detection
*
*
*            hierarchical_iterator_traits.hpp
*
*
* path:      /inc/djinterp/core/container/iterator/hierarchical_iterator_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.24
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Node Topology Detection
      -----------------------

II.   Sibling Navigation Detection
      ----------------------------

III.  Child Access Detection
      ----------------------

IV.   Depth and Path Detection
      ------------------------

V.    Traversal Order Classification
      ------------------------------

VI.   Iteration Strategy Classification
      ---------------------------------

VII.  Convenience Predicates
      ----------------------

VIII. Combined Classification
      -----------------------
*/

#ifndef DJINTERP_CONTAINER_ITERATOR_HIERARCHICAL_ITERATOR_TRAITS_HPP
#define DJINTERP_CONTAINER_ITERATOR_HIERARCHICAL_ITERATOR_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../traits/container_traits.hpp"
#include "../traits/hierarchical_container_traits.hpp"  // is_hierarchical_container_v
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint8_t
                                                   //   (used below; was
                                                   // neither included nor
                                                   // qualified)


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// ===========================================================================
// I.   Node Topology Detection
// ===========================================================================
// Core tree navigation: parent, children, root.
// These delegate to the existing container_traits
// detectors where available.

// (has_parent_accessor, has_children_accessor,
//  has_root_accessor, has_node_type, has_depth_type
//  are already defined in container_traits.hpp)

// has_first_child_accessor
D_TYPE_TRAIT_DETECTED(has_first_child_accessor,
    decltype(std::declval<const Type&>()
        .first_child()))

// has_last_child_accessor
D_TYPE_TRAIT_DETECTED(has_last_child_accessor,
    decltype(std::declval<const Type&>()
        .last_child()))

// has_value_accessor
//   node has .value() to access the stored datum (distinct from the node
// structure itself).
D_TYPE_TRAIT_DETECTED(has_node_value_accessor,
    decltype(std::declval<const Type&>()
        .value()))

// has_key_accessor
//   node has .key() for keyed trees (e.g. JSON, XML).
D_TYPE_TRAIT_DETECTED(has_node_key_accessor,
    decltype(std::declval<const Type&>()
        .key()))

// is_navigable_node
//   type trait: true if the type supports basic tree navigation (has parent +
// children + root).
template<typename Type>
struct is_navigable_node
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( has_parent_accessor_v<clean_type>   &&
          has_children_accessor_v<clean_type> );
};

template<typename Type>
inline constexpr bool is_navigable_node_v =
    is_navigable_node<Type>::value;


// ===========================================================================
// II.  Sibling Navigation Detection
// ===========================================================================

// (has_sibling_accessor and has_prev_sibling_accessor
//  are defined in flat_iterator_traits.hpp - re-detect
//  here for independence)

NS_INTERNAL

    template<typename Type, typename = void>
    struct has_next_sibling_check : std::false_type
    {};

    template<typename Type>
    struct has_next_sibling_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>()
                .next_sibling())>>
        : std::true_type
    {};

    template<typename Type, typename = void>
    struct has_prev_sibling_check : std::false_type
    {};

    template<typename Type>
    struct has_prev_sibling_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>()
                .prev_sibling())>>
        : std::true_type
    {};

NS_END  // internal

// has_next_sibling
template<typename Type>
struct has_next_sibling
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        internal::has_next_sibling_check<
            clean_type>::value;
};

template<typename Type>
inline constexpr bool has_next_sibling_v =
    has_next_sibling<Type>::value;

// has_prev_sibling
template<typename Type>
struct has_prev_sibling
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        internal::has_prev_sibling_check<
            clean_type>::value;
};

template<typename Type>
inline constexpr bool has_prev_sibling_v =
    has_prev_sibling<Type>::value;

// has_bidirectional_siblings
//   type trait: true if both next and prev sibling navigation are available.
template<typename Type>
struct has_bidirectional_siblings
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( has_next_sibling_v<clean_type> &&
          has_prev_sibling_v<clean_type> );
};

template<typename Type>
inline constexpr bool
    has_bidirectional_siblings_v =
        has_bidirectional_siblings<Type>::value;


// ===========================================================================
// III. Child Access Detection
// ===========================================================================

NS_INTERNAL

    // child_at(index) - positional child access
    template<typename Type, typename = void>
    struct has_child_at_check : std::false_type
    {};

    template<typename Type>
    struct has_child_at_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>().child_at(
                std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // child_count()
    template<typename Type, typename = void>
    struct has_child_count_check : std::false_type
    {};

    template<typename Type>
    struct has_child_count_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>()
                .child_count())>>
        : std::true_type
    {};

    // is_leaf()
    template<typename Type, typename = void>
    struct has_is_leaf_check : std::false_type
    {};

    template<typename Type>
    struct has_is_leaf_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>()
                .is_leaf())>>
        : std::true_type
    {};

    // is_root()
    template<typename Type, typename = void>
    struct has_is_root_check : std::false_type
    {};

    template<typename Type>
    struct has_is_root_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>()
                .is_root())>>
        : std::true_type
    {};

NS_END  // internal

// has_child_at
template<typename Type>
struct has_child_at
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        internal::has_child_at_check<
            clean_type>::value;
};

template<typename Type>
inline constexpr bool has_child_at_v =
    has_child_at<Type>::value;

// has_child_count
template<typename Type>
struct has_child_count
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        internal::has_child_count_check<
            clean_type>::value;
};

template<typename Type>
inline constexpr bool has_child_count_v =
    has_child_count<Type>::value;

// has_is_leaf
template<typename Type>
struct has_is_leaf
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        internal::has_is_leaf_check<
            clean_type>::value;
};

template<typename Type>
inline constexpr bool has_is_leaf_v =
    has_is_leaf<Type>::value;

// has_is_root
template<typename Type>
struct has_is_root
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        internal::has_is_root_check<
            clean_type>::value;
};

template<typename Type>
inline constexpr bool has_is_root_v =
    has_is_root<Type>::value;

// has_random_access_children
//   type trait: true if children can be accessed by index (child_at +
// child_count).
template<typename Type>
struct has_random_access_children
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( has_child_at_v<clean_type> &&
          has_child_count_v<clean_type> );
};

template<typename Type>
inline constexpr bool
    has_random_access_children_v =
        has_random_access_children<Type>::value;


// ===========================================================================
// IV.  Depth and Path Detection
// ===========================================================================

NS_INTERNAL

    // depth()
    template<typename Type, typename = void>
    struct has_depth_method_check : std::false_type
    {};

    template<typename Type>
    struct has_depth_method_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>().depth())>>
        : std::true_type
    {};

    // path()
    template<typename Type, typename = void>
    struct has_path_check : std::false_type
    {};

    template<typename Type>
    struct has_path_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>().path())>>
        : std::true_type
    {};

    // level() (synonym for depth in some APIs)
    template<typename Type, typename = void>
    struct has_level_check : std::false_type
    {};

    template<typename Type>
    struct has_level_check<Type,
        std::void_t<decltype(
            std::declval<const Type&>().level())>>
        : std::true_type
    {};

NS_END  // internal

// has_depth_method
template<typename Type>
struct has_depth_method
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        ( internal::has_depth_method_check<
              clean_type>::value ||
          internal::has_level_check<
              clean_type>::value );
};

template<typename Type>
inline constexpr bool has_depth_method_v =
    has_depth_method<Type>::value;

// has_path_method
template<typename Type>
struct has_path_method
{
    using clean_type = clean_t<Type>;

    static constexpr bool value =
        internal::has_path_check<
            clean_type>::value;
};

template<typename Type>
inline constexpr bool has_path_method_v =
    has_path_method<Type>::value;


// ===========================================================================
// V.   Traversal Order Classification
// ===========================================================================

// DTraversalOrder
//   enum: supported tree traversal orders.
enum class DTraversalOrder
{
    pre_order,     // visit node before children
    post_order,    // visit node after children
    in_order,      // visit left, node, right (binary)
    level_order,   // breadth-first
    leaf_only      // visit only leaf nodes
};

// DTraversalCapability
//   enum: bit flags for supported traversal orders.
enum class DTraversalCapability : re_std::uint8_t
{
    none        = 0x00,
    pre_order   = 0x01,
    post_order  = 0x02,
    in_order    = 0x04,
    level_order = 0x08,
    leaf_only   = 0x10
};

inline constexpr DTraversalCapability
operator|(DTraversalCapability _a,
          DTraversalCapability _b) noexcept
{
    return static_cast<DTraversalCapability>(
        static_cast<re_std::uint8_t>(_a) |
        static_cast<re_std::uint8_t>(_b));
}

inline constexpr bool
has_traversal(DTraversalCapability _set,
              DTraversalCapability _flag) noexcept
{
    return (static_cast<re_std::uint8_t>(_set) &
            static_cast<re_std::uint8_t>(_flag)) != 0;
}

NS_INTERNAL

    template<typename Type>
    struct traversal_caps_impl
    {
        using C = clean_t<Type>;

        // pre/post order require children()
        static constexpr bool has_children =
            has_children_accessor_v<C>;

        // in-order requires exactly 2 children (binary tree) - detected via
        // child_count being constexpr 2, or a left()/right() pair.
        // Conservative: require child_at.
        static constexpr bool has_indexed =
            has_random_access_children_v<C>;

        // level-order requires children() (uses a queue internally)
        static constexpr bool has_bfs =
            has_children;

        // leaf-only requires is_leaf or child_count == 0 detection
        static constexpr bool has_leaf_test =
            ( has_is_leaf_v<C> ||
              has_child_count_v<C> );

        static constexpr DTraversalCapability value =
            static_cast<DTraversalCapability>(
                ( has_children
                    ? static_cast<re_std::uint8_t>(
                          DTraversalCapability::
                              pre_order)
                    : 0 ) |
                ( has_children
                    ? static_cast<re_std::uint8_t>(
                          DTraversalCapability::
                              post_order)
                    : 0 ) |
                ( has_indexed
                    ? static_cast<re_std::uint8_t>(
                          DTraversalCapability::
                              in_order)
                    : 0 ) |
                ( has_bfs
                    ? static_cast<re_std::uint8_t>(
                          DTraversalCapability::
                              level_order)
                    : 0 ) |
                ( has_leaf_test
                    ? static_cast<re_std::uint8_t>(
                          DTraversalCapability::
                              leaf_only)
                    : 0 ) );
    };

NS_END  // internal

// container_traversal_capabilities
template<typename Type>
struct container_traversal_capabilities
{
    static constexpr DTraversalCapability value =
        internal::traversal_caps_impl<Type>::value;
};

template<typename Type>
inline constexpr DTraversalCapability
    container_traversal_capabilities_v =
        container_traversal_capabilities<
            Type>::value;


// ===========================================================================
// VI.  Iteration Strategy Classification
// ===========================================================================

// hierarchical_strategy
//   enum: best default iteration strategy for a hierarchical container.
enum class hierarchical_strategy
{
    // not hierarchical - use flat iteration
    flat,

    // children() iterable - stack-based DFS
    stack_dfs,

    // children() iterable + sibling nav - sibling-chain DFS (avoids stack
    // allocation)
    sibling_dfs,

    // child_at(i) + child_count - index-based DFS
    indexed_dfs,

    // not iterable
    unsupported
};

NS_INTERNAL

    template<typename Type>
    struct hier_strategy_impl
    {
        using C = clean_t<Type>;

        static constexpr hierarchical_strategy value
            = !is_hierarchical_container_v<C>
                ? hierarchical_strategy::flat

            : ( has_next_sibling_v<C> &&
                has_first_child_accessor_v<C> )
                ? hierarchical_strategy::sibling_dfs

            : has_random_access_children_v<C>
                ? hierarchical_strategy::indexed_dfs

            : has_children_accessor_v<C>
                ? hierarchical_strategy::stack_dfs

            : hierarchical_strategy::unsupported;
    };

NS_END  // internal

template<typename Type>
struct container_hierarchical_strategy
{
    static constexpr hierarchical_strategy value = internal::hier_strategy_impl<Type>::value;
};

template<typename Type>
inline constexpr hierarchical_strategy
    container_hierarchical_strategy_v =
        container_hierarchical_strategy<
            Type>::value;


// ===========================================================================
// VII. Convenience Predicates
// ===========================================================================

// is_tree_iterable
//   type trait: true if the hierarchical container can be iterated in at least
// one traversal order.
template<typename Type>
struct is_tree_iterable
{
    static constexpr bool value =
        ( container_hierarchical_strategy_v<Type> !=
              hierarchical_strategy::unsupported &&
          container_hierarchical_strategy_v<Type> !=
              hierarchical_strategy::flat );
};

template<typename Type>
inline constexpr bool is_tree_iterable_v =
    is_tree_iterable<Type>::value;

// supports_pre_order
template<typename Type>
struct supports_pre_order
{
    static constexpr bool value =
        has_traversal(
            container_traversal_capabilities_v<
                Type>,
            DTraversalCapability::pre_order);
};

template<typename Type>
inline constexpr bool supports_pre_order_v =
    supports_pre_order<Type>::value;

// supports_level_order
template<typename Type>
struct supports_level_order
{
    static constexpr bool value =
        has_traversal(
            container_traversal_capabilities_v<
                Type>,
            DTraversalCapability::level_order);
};

template<typename Type>
inline constexpr bool supports_level_order_v =
    supports_level_order<Type>::value;


// ===========================================================================
// VIII. Combined Classification
// ===========================================================================

template<typename Type>
struct hierarchical_iterator_class
{
    // topology
    static constexpr bool is_navigable =
        is_navigable_node_v<Type>;
    static constexpr bool has_first_child =
        has_first_child_accessor_v<Type>;
    static constexpr bool has_last_child =
        has_last_child_accessor_v<Type>;
    static constexpr bool has_node_value =
        has_node_value_accessor_v<Type>;
    static constexpr bool has_node_key =
        has_node_key_accessor_v<Type>;

    // siblings
    static constexpr bool has_next_sib =
        has_next_sibling_v<Type>;
    static constexpr bool has_prev_sib =
        has_prev_sibling_v<Type>;
    static constexpr bool bidir_siblings =
        has_bidirectional_siblings_v<Type>;

    // children
    static constexpr bool has_indexed_children =
        has_random_access_children_v<Type>;
    static constexpr bool has_leaf_test =
        has_is_leaf_v<Type>;
    static constexpr bool has_root_test =
        has_is_root_v<Type>;

    // depth / path
    static constexpr bool has_depth =
        has_depth_method_v<Type>;
    static constexpr bool has_path =
        has_path_method_v<Type>;

    // traversal
    static constexpr DTraversalCapability
        traversals =
            container_traversal_capabilities_v<
                Type>;
    static constexpr hierarchical_strategy
        strategy =
            container_hierarchical_strategy_v<
                Type>;

    // aggregate
    static constexpr bool is_tree_iterable =
        is_tree_iterable_v<Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ITERATOR_HIERARCHICAL_ITERATOR_TRAITS_HPP
