/******************************************************************************
* djinterp [container]                                  nary_tree_concepts.hpp
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
* 
* path:      /inc/djinterp/core/container/tree/nary/nary_tree_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_NARY_TREE_CONCEPTS_
#define DJINTERP_NARY_TREE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "nary_tree_concepts.hpp requires C++ compilation"
#endif

// djinterp
#include "../../../djinterp.hpp"
#include "./nary_tree_traits.hpp"


NS_DJINTERP

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===========================================================================
// I.   Child Access Model Concepts
// ===========================================================================

// lcrs_child_access_type
//   concept: constrains types exposing left-child/right-sibling child access.
template<typename _Type>
concept lcrs_child_access_type =
    ( has_first_child_access<_Type>::value &&
      has_next_sibling_access<_Type>::value );

// container_child_access_type
//   concept: constrains types whose children() result is iterable.
template<typename _Type>
concept container_child_access_type =
    has_iterable_children<_Type>::value;

// edge_child_access_type
//   concept: constrains types exposing edge-based child access.
template<typename _Type>
concept edge_child_access_type =
    has_edges_method<_Type>::value;

// hybrid_child_access_type
//   concept: constrains types that satisfy more than one child-access model.
template<typename _Type>
concept hybrid_child_access_type =
    ( child_access_strategy<_Type>::value == nary_child_access::hybrid );

// classified_lcrs_child_access_type
//   concept: constrains types classified specifically as LCRS.
template<typename _Type>
concept classified_lcrs_child_access_type =
    ( child_access_strategy<_Type>::value == nary_child_access::lcrs );

// classified_container_child_access_type
//   concept: constrains types classified specifically as container-children.
template<typename _Type>
concept classified_container_child_access_type =
    ( child_access_strategy<_Type>::value == nary_child_access::container );

// classified_edge_child_access_type
//   concept: constrains types classified specifically as edge-based.
template<typename _Type>
concept classified_edge_child_access_type =
    ( child_access_strategy<_Type>::value == nary_child_access::edges );


// ===========================================================================
// II.  Core Tree Identity Concepts
// ===========================================================================

// nary_tree_container_type
//   concept: constrains types satisfying the n-ary tree protocol.
template<typename _Type>
concept nary_tree_container_type =
    is_nary_tree<_Type>::value;

// nary_tree_node_like
//   concept: constrains types that structurally look like n-ary tree nodes.
template<typename _Type>
concept nary_tree_node_like =
    is_nary_tree_node<_Type>::value;

// rooted_nary_tree_type
//   concept: constrains n-ary trees with root ownership/query support.
template<typename _Type>
concept rooted_nary_tree_type =
    ( nary_tree_container_type<_Type> &&
      has_has_root_method<_Type>::value );


// ===========================================================================
// III. Navigation Concepts
// ===========================================================================

// parent_navigable_nary_tree
//   concept: constrains n-ary trees with parent traversal.
template<typename _Type>
concept parent_navigable_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_parent_access<_Type>::value );

// sibling_navigable_nary_tree
//   concept: constrains n-ary trees with bidirectional sibling traversal.
template<typename _Type>
concept sibling_navigable_nary_tree =
    ( nary_tree_container_type<_Type> &&
      nary_tree_class<_Type>::bidirectional_siblings );

// fully_navigable_nary_tree
//   concept: constrains fully navigable n-ary trees.
template<typename _Type>
concept fully_navigable_nary_tree =
    ( nary_tree_container_type<_Type> &&
      nary_tree_class<_Type>::fully_navigable );

// depth_aware_nary_tree
//   concept: constrains n-ary trees exposing depth().
template<typename _Type>
concept depth_aware_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_depth_method<_Type>::value );

// child_counting_nary_tree
//   concept: constrains n-ary trees exposing child_count().
template<typename _Type>
concept child_counting_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_child_count_method<_Type>::value );

// leaf_query_nary_tree
//   concept: constrains n-ary trees exposing is_leaf().
template<typename _Type>
concept leaf_query_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_is_leaf_method<_Type>::value );

// root_query_nary_tree
//   concept: constrains n-ary trees exposing is_root().
template<typename _Type>
concept root_query_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_is_root_method<_Type>::value );


// ===========================================================================
// IV.  Mutation Concepts
// ===========================================================================

// mutable_nary_tree_type
//   concept: constrains n-ary trees with basic mutation support.
template<typename _Type>
concept mutable_nary_tree_type =
    ( nary_tree_container_type<_Type> &&
      has_append_child_method<_Type>::value &&
      has_detach_method<_Type>::value );

// appendable_nary_tree
//   concept: constrains n-ary trees supporting append_child().
template<typename _Type>
concept appendable_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_append_child_method<_Type>::value );

// prependable_nary_tree
//   concept: constrains n-ary trees supporting prepend_child().
template<typename _Type>
concept prependable_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_prepend_child_method<_Type>::value );

// sibling_insertable_nary_tree
//   concept: constrains n-ary trees supporting sibling insertion.
template<typename _Type>
concept sibling_insertable_nary_tree =
    ( nary_tree_container_type<_Type> &&
      ( has_insert_after_method<_Type>::value ||
        has_insert_before_method<_Type>::value ) );

// detachable_nary_tree
//   concept: constrains n-ary trees supporting detach().
template<typename _Type>
concept detachable_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_detach_method<_Type>::value );

// movable_subtree_nary_tree
//   concept: constrains n-ary trees supporting move_subtree().
template<typename _Type>
concept movable_subtree_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_move_subtree_method<_Type>::value );

// removable_subtree_nary_tree
//   concept: constrains n-ary trees supporting remove_subtree().
template<typename _Type>
concept removable_subtree_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_remove_subtree_method<_Type>::value );


// ===========================================================================
// V.   Handle Form Concepts
// ===========================================================================

// index_handle_nary_tree
//   concept: constrains n-ary trees using integral index handles.
template<typename _Type>
concept index_handle_nary_tree =
    ( nary_tree_container_type<_Type> &&
      uses_index_handles<_Type>::value );

// raw_pointer_handle_nary_tree
//   concept: constrains n-ary trees using raw-pointer handles.
template<typename _Type>
concept raw_pointer_handle_nary_tree =
    ( nary_tree_container_type<_Type> &&
      uses_pointer_handles<_Type>::value );

// smart_pointer_handle_nary_tree
//   concept: constrains n-ary trees using smart-pointer handles.
template<typename _Type>
concept smart_pointer_handle_nary_tree =
    ( nary_tree_container_type<_Type> &&
      uses_smart_pointer_handles<_Type>::value );


// ===========================================================================
// VI.  Memory Model Concepts
// ===========================================================================

// arena_backed_nary_tree
//   concept: constrains arena-backed n-ary trees.
template<typename _Type>
concept arena_backed_nary_tree =
    ( nary_tree_container_type<_Type> &&
      is_arena_backed<_Type>::value );

// pool_backed_nary_tree
//   concept: constrains pool-backed n-ary trees.
template<typename _Type>
concept pool_backed_nary_tree =
    ( nary_tree_container_type<_Type> &&
      is_pool_backed<_Type>::value );

// allocator_backed_nary_tree
//   concept: constrains allocator-aware n-ary trees.
template<typename _Type>
concept allocator_backed_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_allocator_type<_Type>::value );

// standard_memory_nary_tree
//   concept: constrains n-ary trees classified with standard allocator storage.
template<typename _Type>
concept standard_memory_nary_tree =
    ( nary_tree_container_type<_Type> &&
      memory_model_strategy<_Type>::value == nary_memory_model::standard );


// ===========================================================================
// VII.  Identity and Versioning Concepts
// ===========================================================================

// stable_identity_nary_tree
//   concept: constrains n-ary trees exposing stable identity.
template<typename _Type>
concept stable_identity_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_stable_id<_Type>::value );

// versioned_nary_tree_type
//   concept: constrains n-ary trees exposing version information.
template<typename _Type>
concept versioned_nary_tree_type =
    ( nary_tree_container_type<_Type> &&
      has_version<_Type>::value );

// change_tracked_nary_tree
//   concept: constrains n-ary trees with stable identity plus versioning.
template<typename _Type>
concept change_tracked_nary_tree =
    ( nary_tree_container_type<_Type> &&
      nary_tree_class<_Type>::has_change_tracking );


// ===========================================================================
// VIII. Complexity-Oriented Concepts
// ===========================================================================

// o1_append_nary_tree
//   concept: constrains n-ary trees with O(1) append capability.
template<typename _Type>
concept o1_append_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_o1_append<_Type>::value );

// o1_detach_nary_tree
//   concept: constrains n-ary trees with O(1) detach capability.
template<typename _Type>
concept o1_detach_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_o1_detach<_Type>::value );

// o1_sibling_insert_nary_tree
//   concept: constrains n-ary trees with O(1) sibling insertion capability.
template<typename _Type>
concept o1_sibling_insert_nary_tree =
    ( nary_tree_container_type<_Type> &&
      has_o1_sibling_insert<_Type>::value );


// ===========================================================================
// IX.  Aggregate Classification Concepts
// ===========================================================================

// classified_nary_tree
//   concept: shorthand for any type recognized by nary_tree_class.
template<typename _Type>
concept classified_nary_tree =
    nary_tree_class<_Type>::is_nary;

// policy_driven_nary_tree
//   concept: constrains n-ary trees exposing link-policy awareness.
template<typename _Type>
concept policy_driven_nary_tree =
    ( nary_tree_container_type<_Type> &&
      nary_tree_class<_Type>::policy_driven );

// container_children_nary_tree
//   concept: constrains n-ary trees classified as using children() containers.
template<typename _Type>
concept container_children_nary_tree =
    ( nary_tree_container_type<_Type> &&
      nary_tree_class<_Type>::has_children_container );

// edge_children_nary_tree
//   concept: constrains n-ary trees classified as using edge collections.
template<typename _Type>
concept edge_children_nary_tree =
    ( nary_tree_container_type<_Type> &&
      nary_tree_class<_Type>::has_edges );

#endif  // __cpp_concepts >= 201907L


NS_END  // djinterp


#endif  // DJINTERP_NARY_TREE_CONCEPTS_