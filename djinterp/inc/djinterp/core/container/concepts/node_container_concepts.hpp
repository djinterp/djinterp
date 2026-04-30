/******************************************************************************
* djinterp [container]                             node_container_concepts.hpp
*
* Node container concepts:
*   C++20 concepts layered over node_container_traits.hpp. These concepts
* provide readable constraints for node-based containers without replacing the
* existing SFINAE trait surface.
*
*   This header complements the small built-in concept block already present
* in node_container_traits.hpp by exposing a broader standalone concepts
* surface for:
*   - ownership-policy and ownership-model detection
*   - entry-point and node-type detection
*   - node-container identity and topology
*   - shorthand concepts over node_container_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/node_container_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_NODE_CONTAINER_CONCEPTS_
#define DJINTERP_NODE_CONTAINER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "node_container_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/node_container_traits.hpp"


NS_DJINTERP


#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===========================================================================
// I.   Ownership-policy concepts
// ===========================================================================

// ownership_policy_typed_node_container
//   concept: constrains types exposing an ownership_policy alias.
template<typename _Type>
concept ownership_policy_typed_node_container =
    has_ownership_policy<_Type>::value;

// entry_storage_typed_node_container
//   concept: constrains types exposing an entry_storage alias.
template<typename _Type>
concept entry_storage_typed_node_container =
    has_entry_storage_type<_Type>::value;

// entry_owns_flagged_node_container
//   concept: constrains types exposing the static entry_owns boolean.
template<typename _Type>
concept entry_owns_flagged_node_container =
    has_entry_owns_constant<_Type>::value;


// ===========================================================================
// II.  Entry-point concepts
// ===========================================================================

// generic_entry_point_node_container
//   concept: constrains types exposing entry_point().
template<typename _Type>
concept generic_entry_point_node_container =
    has_entry_point_method<_Type>::value;

// root_entry_node_container
//   concept: constrains types exposing root().
template<typename _Type>
concept root_entry_node_container =
    has_root_method<_Type>::value;

// root_presence_node_container
//   concept: constrains types exposing has_root().
template<typename _Type>
concept root_presence_node_container =
    has_has_root_method<_Type>::value;

// head_entry_node_container
//   concept: constrains types exposing head().
template<typename _Type>
concept head_entry_node_container =
    has_head_method<_Type>::value;

// tail_entry_node_container
//   concept: constrains types exposing tail().
template<typename _Type>
concept tail_entry_node_container =
    has_tail_method<_Type>::value;

// has_entry_query_node_container
//   concept: constrains types exposing has_entry().
template<typename _Type>
concept has_entry_query_node_container =
    has_has_entry_method<_Type>::value;

// releasable_entry_node_container
//   concept: constrains types exposing release_entry().
template<typename _Type>
concept releasable_entry_node_container =
    has_release_entry_method<_Type>::value;

// any_entry_point_node_container
//   concept: constrains types exposing some valid node-container entry point.
template<typename _Type>
concept any_entry_point_node_container =
    has_any_entry_point<_Type>::value;


// ===========================================================================
// III. Node-type and ownership-model concepts
// ===========================================================================

// node_typed_node_container
//   concept: constrains types exposing node_type.
template<typename _Type>
concept node_typed_node_container =
    has_node_type<_Type>::value;

// non_owning_node_container_type
//   concept: constrains node containers classified as non-owning.
template<typename _Type>
concept non_owning_node_container_type =
    ( ownership_model_of_v<_Type> == DOwnershipModel::non_owning );

// unique_ownership_node_container_type
//   concept: constrains node containers classified as uniquely owning.
template<typename _Type>
concept unique_ownership_node_container_type =
    ( ownership_model_of_v<_Type> == DOwnershipModel::unique );

// shared_ownership_node_container_type
//   concept: constrains node containers classified as shared-owning.
template<typename _Type>
concept shared_ownership_node_container_type =
    ( ownership_model_of_v<_Type> == DOwnershipModel::shared );

// known_ownership_node_container_type
//   concept: constrains node containers with a recognized ownership model.
template<typename _Type>
concept known_ownership_node_container_type =
    ( ownership_model_of_v<_Type> != DOwnershipModel::unknown );


// ===========================================================================
// IV.  Identity and topology concepts
// ===========================================================================

// structural_node_container_type
//   concept: constrains types structurally recognized as node containers.
template<typename _Type>
concept structural_node_container_type =
    is_node_container<_Type>::value;

// structural_tree_shaped_node_container
//   concept: constrains node containers with a root-shaped topology.
template<typename _Type>
concept structural_tree_shaped_node_container =
    is_tree_shaped_container<_Type>::value;

// structural_list_shaped_node_container
//   concept: constrains node containers with a head-shaped topology.
template<typename _Type>
concept structural_list_shaped_node_container =
    is_list_shaped_container<_Type>::value;

// structural_doubly_linked_node_container
//   concept: constrains list-shaped node containers with both head and tail.
template<typename _Type>
concept structural_doubly_linked_node_container =
    is_doubly_linked_container<_Type>::value;

// generic_shaped_node_container
//   concept: constrains node containers classified as generic entry-point
// containers.
template<typename _Type>
concept generic_shaped_node_container =
    ( node_container_shape_of_v<_Type> == DNodeContainerShape::generic );

// tree_shaped_node_container_type
//   concept: constrains node containers classified as tree-shaped.
template<typename _Type>
concept tree_shaped_node_container_type =
    ( node_container_shape_of_v<_Type> == DNodeContainerShape::tree );

// list_shaped_node_container_type
//   concept: constrains node containers classified as list-shaped.
template<typename _Type>
concept list_shaped_node_container_type =
    ( node_container_shape_of_v<_Type> == DNodeContainerShape::list );


// ===========================================================================
// V.   Classification-based shorthand concepts
// ===========================================================================

// classified_node_container
//   concept: shorthand for any type recognized by node_container_class<T>.
template<typename _Type>
concept classified_node_container =
    node_container_class<_Type>::is_node_container;

// classified_owning_node_container
//   concept: shorthand for node_container_class<T>::is_owning.
template<typename _Type>
concept classified_owning_node_container =
    node_container_class<_Type>::is_owning;

// classified_tree_node_container
//   concept: shorthand for tree-shaped node containers via node_container_class<T>.
template<typename _Type>
concept classified_tree_node_container =
    ( node_container_class<_Type>::shape == DNodeContainerShape::tree );

// classified_list_node_container
//   concept: shorthand for list-shaped node containers via node_container_class<T>.
template<typename _Type>
concept classified_list_node_container =
    ( node_container_class<_Type>::shape == DNodeContainerShape::list );

// classified_unique_ownership_node_container
//   concept: shorthand for uniquely-owned node containers via node_container_class<T>.
template<typename _Type>
concept classified_unique_ownership_node_container =
    ( node_container_class<_Type>::ownership == DOwnershipModel::unique );

// classified_shared_ownership_node_container
//   concept: shorthand for shared-owning node containers via node_container_class<T>.
template<typename _Type>
concept classified_shared_ownership_node_container =
    ( node_container_class<_Type>::ownership == DOwnershipModel::shared );

#endif  // __cpp_concepts >= 201907L


NS_END  // djinterp


#endif  // DJINTERP_NODE_CONTAINER_CONCEPTS_