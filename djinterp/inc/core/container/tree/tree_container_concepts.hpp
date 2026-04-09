/******************************************************************************
* djinterp [container]                             tree_container_concepts.hpp
*
* Tree container concepts:
*   C++20 concepts layered over tree_container_traits.hpp. These concepts
* provide readable constraints for tree-like containers without replacing the
* existing SFINAE trait surface.
*
*   The concepts mirror the structural detection axes from
* tree_container_traits.hpp:
*   - core alias and root detection
*   - topology (binary / n-ary / parented)
*   - operational capabilities (rotation, rebalancing, merge, split)
*   - aggregate tree classification
*
* path:      /inc/container/tree_container_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                   date: 2026.04.08
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_CONTAINER_CONCEPTS_
#define DJINTERP_CONTAINER_TREE_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "tree_container_concepts.hpp requires C++ compilation"
#endif

#include "tree_container_traits.hpp"


NS_DJINTERP
NS_TRAITS

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// =============================================================================
// I.   CORE STRUCTURAL CONCEPTS
// =============================================================================

// node_typed_tree_container
//   concept: the type exposes a nested node_type alias.
template<typename _Type>
concept node_typed_tree_container =
    has_node_type_v<_Type>;

// depth_typed_tree_container
//   concept: the type exposes a nested depth_type alias.
template<typename _Type>
concept depth_typed_tree_container =
    has_depth_type_v<_Type>;

// key_comparable_tree_container
//   concept: the type exposes a nested key_compare alias.
template<typename _Type>
concept key_comparable_tree_container =
    has_key_compare_v<_Type>;

// root_accessible_tree_container
//   concept: the type exposes a root() accessor.
template<typename _Type>
concept root_accessible_tree_container =
    has_root_method_v<_Type>;

// root_settable_tree_container
//   concept: the type exposes a set_root(...) mutator.
template<typename _Type>
concept root_settable_tree_container =
    has_set_root_method_v<_Type>;


// =============================================================================
// II.  TREE IDENTITY CONCEPTS
// =============================================================================

// tree_container_type
//   concept: the type satisfies the minimum structural requirements of a
// tree container.
template<typename _Type>
concept tree_container_type =
    node_typed_tree_container<_Type> &&
    root_accessible_tree_container<_Type>;

// searchable_tree_container
//   concept: the tree container exposes key-based ordering metadata.
template<typename _Type>
concept searchable_tree_container =
    tree_container_type<_Type> &&
    key_comparable_tree_container<_Type>;


// =============================================================================
// III. TOPOLOGY CONCEPTS
// =============================================================================

// binary_tree_container
//   concept: the tree's node_type satisfies the binary-node protocol.
template<typename _Type>
concept binary_tree_container =
    tree_container_type<_Type> &&
    is_binary_tree_v<_Type>;

// nary_tree_container
//   concept: the tree's node_type satisfies the n-ary-node protocol.
template<typename _Type>
concept nary_tree_container =
    tree_container_type<_Type> &&
    is_nary_tree_v<_Type>;

// parented_tree_container
//   concept: the tree's node_type supports parent navigation.
template<typename _Type>
concept parented_tree_container =
    tree_container_type<_Type> &&
    is_parented_tree_v<_Type>;


// =============================================================================
// IV.  OPERATIONAL CAPABILITY CONCEPTS
// =============================================================================

// left_rotatable_tree_container
//   concept: the tree structurally supports rotate_left(node).
template<typename _Type>
concept left_rotatable_tree_container =
    tree_container_type<_Type> &&
    has_rotate_left_method_v<_Type>;

// right_rotatable_tree_container
//   concept: the tree structurally supports rotate_right(node).
template<typename _Type>
concept right_rotatable_tree_container =
    tree_container_type<_Type> &&
    has_rotate_right_method_v<_Type>;

// rotatable_tree_container
//   concept: the tree supports both left and right rotations.
template<typename _Type>
concept rotatable_tree_container =
    tree_container_type<_Type> &&
    has_rotate_left_method_v<_Type> &&
    has_rotate_right_method_v<_Type>;

// self_balancing_tree_container
//   concept: the tree exposes rebalance(node).
template<typename _Type>
concept self_balancing_tree_container =
    tree_container_type<_Type> &&
    has_rebalance_method_v<_Type>;

// mergeable_tree_container
//   concept: the tree supports merge(tree).
template<typename _Type>
concept mergeable_tree_container =
    tree_container_type<_Type> &&
    has_merge_method_v<_Type>;

// splittable_tree_container
//   concept: the tree supports split(key).
template<typename _Type>
concept splittable_tree_container =
    tree_container_type<_Type> &&
    has_split_method_v<_Type>;


// =============================================================================
// V.   AGGREGATE CLASSIFICATION CONCEPTS
// =============================================================================

// classified_tree_container
//   concept: shorthand for any type recognized as a tree by the aggregate
// classification struct.
template<typename _Type>
concept classified_tree_container =
    tree_container_class<_Type>::is_tree;

// classified_search_tree_container
//   concept: shorthand for any type recognized as a search tree by the
// aggregate classification struct.
template<typename _Type>
concept classified_search_tree_container =
    tree_container_class<_Type>::is_search_tree;

// classified_binary_tree_container
//   concept: shorthand for any type recognized as binary by the aggregate
// classification struct.
template<typename _Type>
concept classified_binary_tree_container =
    tree_container_class<_Type>::is_binary;

// classified_nary_tree_container
//   concept: shorthand for any type recognized as n-ary by the aggregate
// classification struct.
template<typename _Type>
concept classified_nary_tree_container =
    tree_container_class<_Type>::is_nary;

// classified_parented_tree_container
//   concept: shorthand for any type recognized as parented by the aggregate
// classification struct.
template<typename _Type>
concept classified_parented_tree_container =
    tree_container_class<_Type>::is_parented;

// classified_rotatable_tree_container
//   concept: shorthand for any type recognized as rotatable by the aggregate
// classification struct.
template<typename _Type>
concept classified_rotatable_tree_container =
    tree_container_class<_Type>::is_rotatable;

// classified_self_balancing_tree_container
//   concept: shorthand for any type recognized as self-balancing by the
// aggregate classification struct.
template<typename _Type>
concept classified_self_balancing_tree_container =
    tree_container_class<_Type>::is_self_balancing;

// classified_mergeable_tree_container
//   concept: shorthand for any type recognized as mergeable by the aggregate
// classification struct.
template<typename _Type>
concept classified_mergeable_tree_container =
    tree_container_class<_Type>::is_mergeable;

// classified_splittable_tree_container
//   concept: shorthand for any type recognized as splittable by the aggregate
// classification struct.
template<typename _Type>
concept classified_splittable_tree_container =
    tree_container_class<_Type>::is_splittable;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_END  // traits
NS_END  // djinterp

#endif  // DJINTERP_CONTAINER_TREE_CONTAINER_CONCEPTS_
