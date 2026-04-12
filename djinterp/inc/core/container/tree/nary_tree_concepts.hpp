/******************************************************************************
* djinterp [container]                                nary_tree_concepts.hpp
*
* N-ary tree concepts:
*   C++20 concepts layered over nary_tree_traits.hpp. These concepts provide
* readable constraints for n-ary tree implementations without replacing the
* existing SFINAE trait surface.
*
*   This header complements the small built-in concept block already present
* in nary_tree_traits.hpp by exposing a fuller standalone concepts surface for:
*   - child-access models
*   - navigation capabilities
*   - mutation capabilities
*   - handle form and memory model
*   - change tracking and aggregate classification
*
* path:      /inc/container/meta/nary_tree_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                   date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_NARY_TREE_CONCEPTS_
#define DJINTERP_NARY_TREE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "nary_tree_concepts.hpp requires C++ compilation"
#endif

#include "nary_tree_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// =============================================================================
// I.   Child Access Model Concepts
// =============================================================================

// lcrs_child_access_type
//   concept: constrains types exposing left-child/right-sibling child access.
template<typename _T>
concept lcrs_child_access_type =
    ( has_first_child_access<_T>::value &&
      has_next_sibling_access<_T>::value );

// container_child_access_type
//   concept: constrains types whose children() result is iterable.
template<typename _T>
concept container_child_access_type =
    has_iterable_children<_T>::value;

// edge_child_access_type
//   concept: constrains types exposing edge-based child access.
template<typename _T>
concept edge_child_access_type =
    has_edges_method<_T>::value;

// hybrid_child_access_type
//   concept: constrains types that satisfy more than one child-access model.
template<typename _T>
concept hybrid_child_access_type =
    ( child_access_strategy<_T>::value == nary_child_access::hybrid );

// classified_lcrs_child_access_type
//   concept: constrains types classified specifically as LCRS.
template<typename _T>
concept classified_lcrs_child_access_type =
    ( child_access_strategy<_T>::value == nary_child_access::lcrs );

// classified_container_child_access_type
//   concept: constrains types classified specifically as container-children.
template<typename _T>
concept classified_container_child_access_type =
    ( child_access_strategy<_T>::value == nary_child_access::container );

// classified_edge_child_access_type
//   concept: constrains types classified specifically as edge-based.
template<typename _T>
concept classified_edge_child_access_type =
    ( child_access_strategy<_T>::value == nary_child_access::edges );


// =============================================================================
// II.  Core Tree Identity Concepts
// =============================================================================

// nary_tree_container_type
//   concept: constrains types satisfying the n-ary tree protocol.
template<typename _T>
concept nary_tree_container_type =
    is_nary_tree<_T>::value;

// nary_tree_node_like
//   concept: constrains types that structurally look like n-ary tree nodes.
template<typename _T>
concept nary_tree_node_like =
    is_nary_tree_node<_T>::value;

// rooted_nary_tree_type
//   concept: constrains n-ary trees with root ownership/query support.
template<typename _T>
concept rooted_nary_tree_type =
    ( nary_tree_container_type<_T> &&
      has_has_root_method<_T>::value );


// =============================================================================
// III. Navigation Concepts
// =============================================================================

// parent_navigable_nary_tree
//   concept: constrains n-ary trees with parent traversal.
template<typename _T>
concept parent_navigable_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_parent_access<_T>::value );

// sibling_navigable_nary_tree
//   concept: constrains n-ary trees with bidirectional sibling traversal.
template<typename _T>
concept sibling_navigable_nary_tree =
    ( nary_tree_container_type<_T> &&
      nary_tree_class<_T>::bidirectional_siblings );

// fully_navigable_nary_tree
//   concept: constrains fully navigable n-ary trees.
template<typename _T>
concept fully_navigable_nary_tree =
    ( nary_tree_container_type<_T> &&
      nary_tree_class<_T>::fully_navigable );

// depth_aware_nary_tree
//   concept: constrains n-ary trees exposing depth().
template<typename _T>
concept depth_aware_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_depth_method<_T>::value );

// child_counting_nary_tree
//   concept: constrains n-ary trees exposing child_count().
template<typename _T>
concept child_counting_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_child_count_method<_T>::value );

// leaf_query_nary_tree
//   concept: constrains n-ary trees exposing is_leaf().
template<typename _T>
concept leaf_query_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_is_leaf_method<_T>::value );

// root_query_nary_tree
//   concept: constrains n-ary trees exposing is_root().
template<typename _T>
concept root_query_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_is_root_method<_T>::value );


// =============================================================================
// IV.  Mutation Concepts
// =============================================================================

// mutable_nary_tree_type
//   concept: constrains n-ary trees with basic mutation support.
template<typename _T>
concept mutable_nary_tree_type =
    ( nary_tree_container_type<_T> &&
      has_append_child_method<_T>::value &&
      has_detach_method<_T>::value );

// appendable_nary_tree
//   concept: constrains n-ary trees supporting append_child().
template<typename _T>
concept appendable_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_append_child_method<_T>::value );

// prependable_nary_tree
//   concept: constrains n-ary trees supporting prepend_child().
template<typename _T>
concept prependable_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_prepend_child_method<_T>::value );

// sibling_insertable_nary_tree
//   concept: constrains n-ary trees supporting sibling insertion.
template<typename _T>
concept sibling_insertable_nary_tree =
    ( nary_tree_container_type<_T> &&
      ( has_insert_after_method<_T>::value ||
        has_insert_before_method<_T>::value ) );

// detachable_nary_tree
//   concept: constrains n-ary trees supporting detach().
template<typename _T>
concept detachable_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_detach_method<_T>::value );

// movable_subtree_nary_tree
//   concept: constrains n-ary trees supporting move_subtree().
template<typename _T>
concept movable_subtree_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_move_subtree_method<_T>::value );

// removable_subtree_nary_tree
//   concept: constrains n-ary trees supporting remove_subtree().
template<typename _T>
concept removable_subtree_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_remove_subtree_method<_T>::value );


// =============================================================================
// V.   Handle Form Concepts
// =============================================================================

// index_handle_nary_tree
//   concept: constrains n-ary trees using integral index handles.
template<typename _T>
concept index_handle_nary_tree =
    ( nary_tree_container_type<_T> &&
      uses_index_handles<_T>::value );

// raw_pointer_handle_nary_tree
//   concept: constrains n-ary trees using raw-pointer handles.
template<typename _T>
concept raw_pointer_handle_nary_tree =
    ( nary_tree_container_type<_T> &&
      uses_pointer_handles<_T>::value );

// smart_pointer_handle_nary_tree
//   concept: constrains n-ary trees using smart-pointer handles.
template<typename _T>
concept smart_pointer_handle_nary_tree =
    ( nary_tree_container_type<_T> &&
      uses_smart_pointer_handles<_T>::value );


// =============================================================================
// VI.  Memory Model Concepts
// =============================================================================

// arena_backed_nary_tree
//   concept: constrains arena-backed n-ary trees.
template<typename _T>
concept arena_backed_nary_tree =
    ( nary_tree_container_type<_T> &&
      is_arena_backed<_T>::value );

// pool_backed_nary_tree
//   concept: constrains pool-backed n-ary trees.
template<typename _T>
concept pool_backed_nary_tree =
    ( nary_tree_container_type<_T> &&
      is_pool_backed<_T>::value );

// allocator_backed_nary_tree
//   concept: constrains allocator-aware n-ary trees.
template<typename _T>
concept allocator_backed_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_allocator_type<_T>::value );

// standard_memory_nary_tree
//   concept: constrains n-ary trees classified with standard allocator storage.
template<typename _T>
concept standard_memory_nary_tree =
    ( nary_tree_container_type<_T> &&
      memory_model_strategy<_T>::value == nary_memory_model::standard );


// =============================================================================
// VII.  Identity and Versioning Concepts
// =============================================================================

// stable_identity_nary_tree
//   concept: constrains n-ary trees exposing stable identity.
template<typename _T>
concept stable_identity_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_stable_id<_T>::value );

// versioned_nary_tree_type
//   concept: constrains n-ary trees exposing version information.
template<typename _T>
concept versioned_nary_tree_type =
    ( nary_tree_container_type<_T> &&
      has_version<_T>::value );

// change_tracked_nary_tree
//   concept: constrains n-ary trees with stable identity plus versioning.
template<typename _T>
concept change_tracked_nary_tree =
    ( nary_tree_container_type<_T> &&
      nary_tree_class<_T>::has_change_tracking );


// =============================================================================
// VIII. Complexity-Oriented Concepts
// =============================================================================

// o1_append_nary_tree
//   concept: constrains n-ary trees with O(1) append capability.
template<typename _T>
concept o1_append_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_o1_append<_T>::value );

// o1_detach_nary_tree
//   concept: constrains n-ary trees with O(1) detach capability.
template<typename _T>
concept o1_detach_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_o1_detach<_T>::value );

// o1_sibling_insert_nary_tree
//   concept: constrains n-ary trees with O(1) sibling insertion capability.
template<typename _T>
concept o1_sibling_insert_nary_tree =
    ( nary_tree_container_type<_T> &&
      has_o1_sibling_insert<_T>::value );


// =============================================================================
// IX.  Aggregate Classification Concepts
// =============================================================================

// classified_nary_tree
//   concept: shorthand for any type recognized by nary_tree_class.
template<typename _T>
concept classified_nary_tree =
    nary_tree_class<_T>::is_nary;

// policy_driven_nary_tree
//   concept: constrains n-ary trees exposing link-policy awareness.
template<typename _T>
concept policy_driven_nary_tree =
    ( nary_tree_container_type<_T> &&
      nary_tree_class<_T>::policy_driven );

// container_children_nary_tree
//   concept: constrains n-ary trees classified as using children() containers.
template<typename _T>
concept container_children_nary_tree =
    ( nary_tree_container_type<_T> &&
      nary_tree_class<_T>::has_children_container );

// edge_children_nary_tree
//   concept: constrains n-ary trees classified as using edge collections.
template<typename _T>
concept edge_children_nary_tree =
    ( nary_tree_container_type<_T> &&
      nary_tree_class<_T>::has_edges );

#endif  // __cpp_concepts >= 201907L

NS_END  // traits
NS_END  // container
NS_END  // djinterp

#endif  // DJINTERP_NARY_TREE_CONCEPTS_
