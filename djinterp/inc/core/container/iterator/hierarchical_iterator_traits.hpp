/******************************************************************************
* djinterp [container]                      hierarchical_iterator_traits.hpp
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
*   Path:        path() (returns a sequence of keys/indices)
*
*   Based on the detected capabilities, classifies which traversal
* orders are available (pre-order, post-order, in-order, level-
* order, leaf-only) and the best iteration strategy.
*
* DEPENDENCIES:
*   container_traits.hpp   — base hierarchy detection
*
* TABLE OF CONTENTS
* =================
* I.      Node Topology Detection
* II.     Sibling Navigation Detection
* III.    Child Access Detection
* IV.     Depth and Path Detection
* V.      Traversal Order Classification
* VI.     Iteration Strategy Classification
* VII.    Convenience Predicates
* VIII.   Combined Classification
*
*
* path:      /inc/container/meta/hierarchical_iterator_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.24
******************************************************************************/

#ifndef DJINTERP_HIERARCHICAL_ITERATOR_TRAITS_
#define DJINTERP_HIERARCHICAL_ITERATOR_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "./container_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Node Topology Detection
// =============================================================================
// Core tree navigation: parent, children, root.
// These delegate to the existing container_traits
// detectors where available.

// (has_parent_accessor, has_children_accessor,
//  has_root_accessor, has_node_type, has_depth_type
//  are already defined in container_traits.hpp)

// has_first_child_accessor
D_TYPE_TRAIT_TRUE(has_first_child_accessor,
    decltype(std::declval<const _Type&>()
        .first_child()))

// has_last_child_accessor
D_TYPE_TRAIT_TRUE(has_last_child_accessor,
    decltype(std::declval<const _Type&>()
        .last_child()))

// has_value_accessor
//   node has .value() to access the stored datum
// (distinct from the node structure itself).
D_TYPE_TRAIT_TRUE(has_node_value_accessor,
    decltype(std::declval<const _Type&>()
        .value()))

// has_key_accessor
//   node has .key() for keyed trees (e.g. JSON, XML).
D_TYPE_TRAIT_TRUE(has_node_key_accessor,
    decltype(std::declval<const _Type&>()
        .key()))

// is_navigable_node
//   type trait: true if the type supports basic tree
// navigation (has parent + children + root).
template<typename _Type>
struct is_navigable_node
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_parent_accessor_v<clean_type>   &&
          has_children_accessor_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_navigable_node_v =
    is_navigable_node<_Type>::value;


// =============================================================================
// II.  Sibling Navigation Detection
// =============================================================================

// (has_sibling_accessor and has_prev_sibling_accessor
//  are defined in flat_iterator_traits.hpp — re-detect
//  here for independence)

NS_INTERNAL

    template<typename _Type, typename = void>
    struct has_next_sibling_check : std::false_type
    {};

    template<typename _Type>
    struct has_next_sibling_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>()
                .next_sibling())>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_prev_sibling_check : std::false_type
    {};

    template<typename _Type>
    struct has_prev_sibling_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>()
                .prev_sibling())>>
        : std::true_type
    {};

NS_END  // internal

// has_next_sibling
template<typename _Type>
struct has_next_sibling
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_next_sibling_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_next_sibling_v =
    has_next_sibling<_Type>::value;

// has_prev_sibling
template<typename _Type>
struct has_prev_sibling
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_prev_sibling_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_prev_sibling_v =
    has_prev_sibling<_Type>::value;

// has_bidirectional_siblings
//   type trait: true if both next and prev sibling
// navigation are available.
template<typename _Type>
struct has_bidirectional_siblings
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_next_sibling_v<clean_type> &&
          has_prev_sibling_v<clean_type> );
};

template<typename _Type>
inline constexpr bool
    has_bidirectional_siblings_v =
        has_bidirectional_siblings<_Type>::value;


// =============================================================================
// III. Child Access Detection
// =============================================================================

NS_INTERNAL

    // child_at(index) — positional child access
    template<typename _Type, typename = void>
    struct has_child_at_check : std::false_type
    {};

    template<typename _Type>
    struct has_child_at_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>().child_at(
                std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // child_count()
    template<typename _Type, typename = void>
    struct has_child_count_check : std::false_type
    {};

    template<typename _Type>
    struct has_child_count_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>()
                .child_count())>>
        : std::true_type
    {};

    // is_leaf()
    template<typename _Type, typename = void>
    struct has_is_leaf_check : std::false_type
    {};

    template<typename _Type>
    struct has_is_leaf_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>()
                .is_leaf())>>
        : std::true_type
    {};

    // is_root()
    template<typename _Type, typename = void>
    struct has_is_root_check : std::false_type
    {};

    template<typename _Type>
    struct has_is_root_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>()
                .is_root())>>
        : std::true_type
    {};

NS_END  // internal

// has_child_at
template<typename _Type>
struct has_child_at
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_child_at_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_child_at_v =
    has_child_at<_Type>::value;

// has_child_count
template<typename _Type>
struct has_child_count
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_child_count_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_child_count_v =
    has_child_count<_Type>::value;

// has_is_leaf
template<typename _Type>
struct has_is_leaf
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_is_leaf_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_is_leaf_v =
    has_is_leaf<_Type>::value;

// has_is_root
template<typename _Type>
struct has_is_root
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_is_root_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_is_root_v =
    has_is_root<_Type>::value;

// has_random_access_children
//   type trait: true if children can be accessed by
// index (child_at + child_count).
template<typename _Type>
struct has_random_access_children
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_child_at_v<clean_type> &&
          has_child_count_v<clean_type> );
};

template<typename _Type>
inline constexpr bool
    has_random_access_children_v =
        has_random_access_children<_Type>::value;


// =============================================================================
// IV.  Depth and Path Detection
// =============================================================================

NS_INTERNAL

    // depth()
    template<typename _Type, typename = void>
    struct has_depth_method_check : std::false_type
    {};

    template<typename _Type>
    struct has_depth_method_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>().depth())>>
        : std::true_type
    {};

    // path()
    template<typename _Type, typename = void>
    struct has_path_check : std::false_type
    {};

    template<typename _Type>
    struct has_path_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>().path())>>
        : std::true_type
    {};

    // level() (synonym for depth in some APIs)
    template<typename _Type, typename = void>
    struct has_level_check : std::false_type
    {};

    template<typename _Type>
    struct has_level_check<_Type,
        std::void_t<decltype(
            std::declval<const _Type&>().level())>>
        : std::true_type
    {};

NS_END  // internal

// has_depth_method
template<typename _Type>
struct has_depth_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( internal::has_depth_method_check<
              clean_type>::value ||
          internal::has_level_check<
              clean_type>::value );
};

template<typename _Type>
inline constexpr bool has_depth_method_v =
    has_depth_method<_Type>::value;

// has_path_method
template<typename _Type>
struct has_path_method
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_path_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_path_method_v =
    has_path_method<_Type>::value;


// =============================================================================
// V.   Traversal Order Classification
// =============================================================================

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
enum class DTraversalCapability : std::uint8_t
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
        static_cast<std::uint8_t>(_a) |
        static_cast<std::uint8_t>(_b));
}

inline constexpr bool
has_traversal(DTraversalCapability _set,
              DTraversalCapability _flag) noexcept
{
    return (static_cast<std::uint8_t>(_set) &
            static_cast<std::uint8_t>(_flag)) != 0;
}

NS_INTERNAL

    template<typename _Type>
    struct traversal_caps_impl
    {
        using C = clean_t<_Type>;

        // pre/post order require children()
        static constexpr bool has_children =
            has_children_accessor_v<C>;

        // in-order requires exactly 2 children
        // (binary tree) — detected via child_count
        // being constexpr 2, or a left()/right()
        // pair.  Conservative: require child_at.
        static constexpr bool has_indexed =
            has_random_access_children_v<C>;

        // level-order requires children()
        // (uses a queue internally)
        static constexpr bool has_bfs =
            has_children;

        // leaf-only requires is_leaf or
        // child_count == 0 detection
        static constexpr bool has_leaf_test =
            ( has_is_leaf_v<C> ||
              has_child_count_v<C> );

        static constexpr DTraversalCapability value =
            static_cast<DTraversalCapability>(
                ( has_children
                    ? static_cast<std::uint8_t>(
                          DTraversalCapability::
                              pre_order)
                    : 0 ) |
                ( has_children
                    ? static_cast<std::uint8_t>(
                          DTraversalCapability::
                              post_order)
                    : 0 ) |
                ( has_indexed
                    ? static_cast<std::uint8_t>(
                          DTraversalCapability::
                              in_order)
                    : 0 ) |
                ( has_bfs
                    ? static_cast<std::uint8_t>(
                          DTraversalCapability::
                              level_order)
                    : 0 ) |
                ( has_leaf_test
                    ? static_cast<std::uint8_t>(
                          DTraversalCapability::
                              leaf_only)
                    : 0 ) );
    };

NS_END  // internal

// container_traversal_capabilities
template<typename _Type>
struct container_traversal_capabilities
{
    static constexpr DTraversalCapability value =
        internal::traversal_caps_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DTraversalCapability
    container_traversal_capabilities_v =
        container_traversal_capabilities<
            _Type>::value;


// =============================================================================
// VI.  Iteration Strategy Classification
// =============================================================================

// DHierarchicalStrategy
//   enum: best default iteration strategy for a
// hierarchical container.
enum class DHierarchicalStrategy
{
    // not hierarchical — use flat iteration
    flat,

    // children() iterable — stack-based DFS
    stack_dfs,

    // children() iterable + sibling nav —
    // sibling-chain DFS (avoids stack allocation)
    sibling_dfs,

    // child_at(i) + child_count — index-based DFS
    indexed_dfs,

    // not iterable
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct hier_strategy_impl
    {
        using C = clean_t<_Type>;

        static constexpr DHierarchicalStrategy value
            = !is_hierarchical_container_v<C>
                ? DHierarchicalStrategy::flat

            : ( has_next_sibling_v<C> &&
                has_first_child_accessor_v<C> )
                ? DHierarchicalStrategy::sibling_dfs

            : has_random_access_children_v<C>
                ? DHierarchicalStrategy::indexed_dfs

            : has_children_accessor_v<C>
                ? DHierarchicalStrategy::stack_dfs

            : DHierarchicalStrategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_hierarchical_strategy
{
    static constexpr DHierarchicalStrategy value =
        internal::hier_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DHierarchicalStrategy
    container_hierarchical_strategy_v =
        container_hierarchical_strategy<
            _Type>::value;


// =============================================================================
// VII. Convenience Predicates
// =============================================================================

// is_tree_iterable
//   type trait: true if the hierarchical container can be
// iterated in at least one traversal order.
template<typename _Type>
struct is_tree_iterable
{
    static constexpr bool value =
        ( container_hierarchical_strategy_v<_Type> !=
              DHierarchicalStrategy::unsupported &&
          container_hierarchical_strategy_v<_Type> !=
              DHierarchicalStrategy::flat );
};

template<typename _Type>
inline constexpr bool is_tree_iterable_v =
    is_tree_iterable<_Type>::value;

// supports_pre_order
template<typename _Type>
struct supports_pre_order
{
    static constexpr bool value =
        has_traversal(
            container_traversal_capabilities_v<
                _Type>,
            DTraversalCapability::pre_order);
};

template<typename _Type>
inline constexpr bool supports_pre_order_v =
    supports_pre_order<_Type>::value;

// supports_level_order
template<typename _Type>
struct supports_level_order
{
    static constexpr bool value =
        has_traversal(
            container_traversal_capabilities_v<
                _Type>,
            DTraversalCapability::level_order);
};

template<typename _Type>
inline constexpr bool supports_level_order_v =
    supports_level_order<_Type>::value;


// =============================================================================
// VIII. Combined Classification
// =============================================================================

template<typename _Type>
struct hierarchical_iterator_class
{
    // topology
    static constexpr bool is_navigable =
        is_navigable_node_v<_Type>;
    static constexpr bool has_first_child =
        has_first_child_accessor_v<_Type>;
    static constexpr bool has_last_child =
        has_last_child_accessor_v<_Type>;
    static constexpr bool has_node_value =
        has_node_value_accessor_v<_Type>;
    static constexpr bool has_node_key =
        has_node_key_accessor_v<_Type>;

    // siblings
    static constexpr bool has_next_sib =
        has_next_sibling_v<_Type>;
    static constexpr bool has_prev_sib =
        has_prev_sibling_v<_Type>;
    static constexpr bool bidir_siblings =
        has_bidirectional_siblings_v<_Type>;

    // children
    static constexpr bool has_indexed_children =
        has_random_access_children_v<_Type>;
    static constexpr bool has_leaf_test =
        has_is_leaf_v<_Type>;
    static constexpr bool has_root_test =
        has_is_root_v<_Type>;

    // depth / path
    static constexpr bool has_depth =
        has_depth_method_v<_Type>;
    static constexpr bool has_path =
        has_path_method_v<_Type>;

    // traversal
    static constexpr DTraversalCapability
        traversals =
            container_traversal_capabilities_v<
                _Type>;
    static constexpr DHierarchicalStrategy
        strategy =
            container_hierarchical_strategy_v<
                _Type>;

    // aggregate
    static constexpr bool is_tree_iterable =
        is_tree_iterable_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_HIERARCHICAL_ITERATOR_TRAITS_
