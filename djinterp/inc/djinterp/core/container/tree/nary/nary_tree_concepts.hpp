/*******************************************************************************
* djinterp [core]                                         nary_tree_concepts.hpp
*
* N-ary tree concepts:
*   C++20 concepts layered over nary_tree_traits.hpp. These concepts provide
* readable constraints for n-ary tree implementations without replacing the
* existing SFINAE trait surface.
*
*   This header complements the small built-in concept block already present
* in nary_tree_traits.hpp by exposing a fuller standalone concepts surface
* for:
*   - child-access models
*   - navigation capabilities
*   - mutation capabilities
*   - handle form and memory model
*   - change tracking and aggregate classification
*
*
* path:      /inc/djinterp/core/container/tree/nary/nary_tree_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.11
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_CONCEPTS_HPP
#define DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#ifndef __cplusplus
    #error "nary_tree_concepts.hpp requires C++ compilation"
#endif

// djinterp
#include "../../../../djinterp.hpp"
#include "./nary_tree_traits.hpp"


NS_DJINTERP

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===========================================================================
// I.   Child Access Model Concepts
// ===========================================================================

// lcrs_child_access_type
//   concept: constrains types exposing left-child/right-sibling child access.
template<typename Type>
concept lcrs_child_access_type =
    ( has_first_child_access<Type>::value &&
      has_next_sibling_access<Type>::value );

// container_child_access_type
//   concept: constrains types whose children() result is iterable.
template<typename Type>
concept container_child_access_type =
    has_iterable_children<Type>::value;

// edge_child_access_type
//   concept: constrains types exposing edge-based child access.
template<typename Type>
concept edge_child_access_type =
    has_edges_method<Type>::value;

// hybrid_child_access_type
//   concept: constrains types that satisfy more than one child-access model.
template<typename Type>
concept hybrid_child_access_type =
    ( child_access_strategy<Type>::value == nary_child_access::hybrid );

// classified_lcrs_child_access_type
//   concept: constrains types classified specifically as LCRS.
template<typename Type>
concept classified_lcrs_child_access_type =
    ( child_access_strategy<Type>::value == nary_child_access::lcrs );

// classified_container_child_access_type
//   concept: constrains types classified specifically as container-children.
template<typename Type>
concept classified_container_child_access_type =
    ( child_access_strategy<Type>::value == nary_child_access::container );

// classified_edge_child_access_type
//   concept: constrains types classified specifically as edge-based.
template<typename Type>
concept classified_edge_child_access_type =
    ( child_access_strategy<Type>::value == nary_child_access::edges );


// ===========================================================================
// II.  Core Tree Identity Concepts
// ===========================================================================

// nary_tree_container_type
//   concept: constrains types satisfying the n-ary tree protocol.
template<typename Type>
concept nary_tree_container_type =
    is_nary_tree<Type>::value;

// nary_tree_node_like
//   concept: constrains types that structurally look like n-ary tree nodes.
template<typename Type>
concept nary_tree_node_like =
    is_nary_tree_node<Type>::value;

// rooted_nary_tree_type
//   concept: constrains n-ary trees with root ownership/query support.
template<typename Type>
concept rooted_nary_tree_type =
    ( nary_tree_container_type<Type> &&
      has_root_member_method<Type>::value );


// ===========================================================================
// III. Navigation Concepts
// ===========================================================================

// parent_navigable_nary_tree
//   concept: constrains n-ary trees with parent traversal.
template<typename Type>
concept parent_navigable_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_parent_access<Type>::value );

// sibling_navigable_nary_tree
//   concept: constrains n-ary trees with bidirectional sibling traversal.
template<typename Type>
concept sibling_navigable_nary_tree =
    ( nary_tree_container_type<Type> &&
      nary_tree_class<Type>::bidirectional_siblings );

// fully_navigable_nary_tree
//   concept: constrains fully navigable n-ary trees.
template<typename Type>
concept fully_navigable_nary_tree =
    ( nary_tree_container_type<Type> &&
      nary_tree_class<Type>::fully_navigable );

// depth_aware_nary_tree
//   concept: constrains n-ary trees exposing depth().
template<typename Type>
concept depth_aware_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_depth_method<Type>::value );

// child_counting_nary_tree
//   concept: constrains n-ary trees exposing child_count().
template<typename Type>
concept child_counting_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_child_count_method<Type>::value );

// leaf_query_nary_tree
//   concept: constrains n-ary trees exposing is_leaf().
template<typename Type>
concept leaf_query_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_is_leaf_method<Type>::value );

// root_query_nary_tree
//   concept: constrains n-ary trees exposing is_root().
template<typename Type>
concept root_query_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_is_root_method<Type>::value );


// ===========================================================================
// IV.  Mutation Concepts
// ===========================================================================

// mutable_nary_tree_type
//   concept: constrains n-ary trees with basic mutation support.
template<typename Type>
concept mutable_nary_tree_type =
    ( nary_tree_container_type<Type> &&
      has_append_child_method<Type>::value &&
      has_detach_method<Type>::value );

// appendable_nary_tree
//   concept: constrains n-ary trees supporting append_child().
template<typename Type>
concept appendable_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_append_child_method<Type>::value );

// prependable_nary_tree
//   concept: constrains n-ary trees supporting prepend_child().
template<typename Type>
concept prependable_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_prepend_child_method<Type>::value );

// sibling_insertable_nary_tree
//   concept: constrains n-ary trees supporting sibling insertion.
template<typename Type>
concept sibling_insertable_nary_tree =
    ( nary_tree_container_type<Type> &&
      ( has_insert_after_method<Type>::value ||
        has_insert_before_method<Type>::value ) );

// detachable_nary_tree
//   concept: constrains n-ary trees supporting detach().
template<typename Type>
concept detachable_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_detach_method<Type>::value );

// movable_subtree_nary_tree
//   concept: constrains n-ary trees supporting move_subtree().
template<typename Type>
concept movable_subtree_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_move_subtree_method<Type>::value );

// removable_subtree_nary_tree
//   concept: constrains n-ary trees supporting remove_subtree().
template<typename Type>
concept removable_subtree_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_remove_subtree_method<Type>::value );


// ===========================================================================
// V.   Handle Form Concepts
// ===========================================================================

// index_handle_nary_tree
//   concept: constrains n-ary trees using integral index handles.
template<typename Type>
concept index_handle_nary_tree =
    ( nary_tree_container_type<Type> &&
      uses_index_handles<Type>::value );

// raw_pointer_handle_nary_tree
//   concept: constrains n-ary trees using raw-pointer handles.
template<typename Type>
concept raw_pointer_handle_nary_tree =
    ( nary_tree_container_type<Type> &&
      uses_pointer_handles<Type>::value );

// smart_pointer_handle_nary_tree
//   concept: constrains n-ary trees using smart-pointer handles.
template<typename Type>
concept smart_pointer_handle_nary_tree =
    ( nary_tree_container_type<Type> &&
      uses_smart_pointer_handles<Type>::value );


// ===========================================================================
// VI.  Memory Model Concepts
// ===========================================================================

// arena_backed_nary_tree
//   concept: constrains arena-backed n-ary trees.
template<typename Type>
concept arena_backed_nary_tree =
    ( nary_tree_container_type<Type> &&
      is_arena_backed<Type>::value );

// pool_backed_nary_tree
//   concept: constrains pool-backed n-ary trees.
template<typename Type>
concept pool_backed_nary_tree =
    ( nary_tree_container_type<Type> &&
      is_pool_backed<Type>::value );

// allocator_backed_nary_tree
//   concept: constrains allocator-aware n-ary trees.
template<typename Type>
concept allocator_backed_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_allocator_type<Type>::value );

// standard_memory_nary_tree
//   concept: constrains n-ary trees classified with standard allocator
// storage.
template<typename Type>
concept standard_memory_nary_tree =
    ( nary_tree_container_type<Type> &&
      memory_model_strategy<Type>::value == nary_memory_model::standard );


// ===========================================================================
// VII.  Identity and Versioning Concepts
// ===========================================================================

// stable_identity_nary_tree
//   concept: constrains n-ary trees exposing stable identity.
template<typename Type>
concept stable_identity_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_stable_id<Type>::value );

// versioned_nary_tree_type
//   concept: constrains n-ary trees exposing version information.
template<typename Type>
concept versioned_nary_tree_type =
    ( nary_tree_container_type<Type> &&
      has_version<Type>::value );

// change_tracked_nary_tree
//   concept: constrains n-ary trees with stable identity plus versioning.
template<typename Type>
concept change_tracked_nary_tree =
    ( nary_tree_container_type<Type> &&
      nary_tree_class<Type>::has_change_tracking );


// ===========================================================================
// VIII. Complexity-Oriented Concepts
// ===========================================================================

// o1_append_nary_tree
//   concept: constrains n-ary trees with O(1) append capability.
template<typename Type>
concept o1_append_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_o1_append<Type>::value );

// o1_detach_nary_tree
//   concept: constrains n-ary trees with O(1) detach capability.
template<typename Type>
concept o1_detach_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_o1_detach<Type>::value );

// o1_sibling_insert_nary_tree
//   concept: constrains n-ary trees with O(1) sibling insertion capability.
template<typename Type>
concept o1_sibling_insert_nary_tree =
    ( nary_tree_container_type<Type> &&
      has_o1_sibling_insert<Type>::value );


// ===========================================================================
// IX.  Aggregate Classification Concepts
// ===========================================================================

// classified_nary_tree
//   concept: shorthand for any type recognized by nary_tree_class.
template<typename Type>
concept classified_nary_tree =
    nary_tree_class<Type>::is_nary;

// policy_driven_nary_tree
//   concept: constrains n-ary trees exposing link-policy awareness.
template<typename Type>
concept policy_driven_nary_tree =
    ( nary_tree_container_type<Type> &&
      nary_tree_class<Type>::policy_driven );

// container_children_nary_tree
//   concept: constrains n-ary trees classified as using children() containers.
template<typename Type>
concept container_children_nary_tree =
    ( nary_tree_container_type<Type> &&
      nary_tree_class<Type>::has_children_container );

// edge_children_nary_tree
//   concept: constrains n-ary trees classified as using edge collections.
template<typename Type>
concept edge_children_nary_tree =
    ( nary_tree_container_type<Type> &&
      nary_tree_class<Type>::has_edges );

#endif  // __cpp_concepts >= 201907L


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_CONCEPTS_HPP
