/*******************************************************************************
* djinterp [core]                                         file_tree_concepts.hpp
*
* File tree concepts:
*   C++20 concepts layered over file_tree_traits.hpp.  These concepts provide
* readable constraints for file-tree-like containers without replacing the
* underlying SFINAE trait surface, which remains available for C++11/14/17.
*
*   The concepts mirror the structural detection axes from
* file_tree_traits.hpp:
*   - core alias and scanning surface
*   - resolution and naming surface
*   - traversal capability (dfs / bfs)
*   - mutation capability
*   - payload shape (file_entry-like)
*   - aggregate file-tree classification
*
*
* path:      /inc/djinterp/core/container/tree/file/file_tree_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_CONCEPTS_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#ifndef __cplusplus
    #error "file_tree_concepts.hpp requires C++ compilation"
#endif

// djinterp
#include "./file_tree_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   CORE STRUCTURAL CONCEPTS
// ===========================================================================

// node_typed_file_tree
//   concept: the type exposes a nested node_type alias.
template<typename Type>
concept node_typed_file_tree =
    has_node_type_v<Type>;

// scannable_file_tree
//   concept: the type exposes a scan(const char*) populator.
template<typename Type>
concept scannable_file_tree =
    has_scan_method_v<Type>;

// resolvable_file_tree
//   concept: the type exposes a resolve(const char*) path lookup.
template<typename Type>
concept resolvable_file_tree =
    has_resolve_method_v<Type>;


// ===========================================================================
// II.  FILE TREE IDENTITY CONCEPTS
// ===========================================================================

// file_tree_type
//   concept: the type satisfies the minimum structural requirements of a file
// tree - a node_type alias plus the scan and resolve surface.
template<typename Type>
concept file_tree_type =
    node_typed_file_tree<Type> &&
    scannable_file_tree<Type>  &&
    resolvable_file_tree<Type>;

// named_file_tree
//   concept: the file tree exposes name access (raw pointer or std::string
// form).
template<typename Type>
concept named_file_tree =
    file_tree_type<Type> &&
    ( has_name_method_v<Type> || has_name_str_method_v<Type> );

// path_addressable_file_tree
//   concept: the file tree can reconstruct a full path from a node.
template<typename Type>
concept path_addressable_file_tree =
    file_tree_type<Type> &&
    has_full_path_method_v<Type>;


// ===========================================================================
// III. TRAVERSAL CONCEPTS
// ===========================================================================

// dfs_traversable_file_tree
//   concept: the file tree supports visit_depth_first(root, fn).
template<typename Type>
concept dfs_traversable_file_tree =
    file_tree_type<Type> &&
    has_depth_first_method_v<Type>;

// bfs_traversable_file_tree
//   concept: the file tree supports visit_breadth_first(root, fn).
template<typename Type>
concept bfs_traversable_file_tree =
    file_tree_type<Type> &&
    has_breadth_first_method_v<Type>;

// traversable_file_tree
//   concept: the file tree supports at least one traversal order.
template<typename Type>
concept traversable_file_tree =
    file_tree_type<Type> &&
    ( has_depth_first_method_v<Type> ||
      has_breadth_first_method_v<Type> );


// ===========================================================================
// IV.  MUTATION CONCEPTS
// ===========================================================================

// child_insertable_file_tree
//   concept: the file tree supports add_child(parent, name, type).
template<typename Type>
concept child_insertable_file_tree =
    file_tree_type<Type> &&
    has_add_child_method_v<Type>;

// clearable_file_tree
//   concept: the file tree supports clear().
template<typename Type>
concept clearable_file_tree =
    has_clear_method_v<Type>;

// mutable_file_tree
//   concept: the file tree supports both child insertion and clearing.
template<typename Type>
concept mutable_file_tree =
    file_tree_type<Type> &&
    has_add_child_method_v<Type> &&
    has_clear_method_v<Type>;


// ===========================================================================
// V.   PAYLOAD SHAPE CONCEPTS
// ===========================================================================

// file_entry_payload
//   concept: the payload type carries the full file_entry shape (name_offset /
// name_length / type / size).
template<typename Payload>
concept file_entry_payload =
    is_file_entry_payload_v<Payload>;

// file_entry_carrying_tree
//   concept: the file tree's node_type carries a file_entry-shaped payload.
template<typename Type>
concept file_entry_carrying_tree =
    file_tree_type<Type> &&
    is_file_entry_payload_v<file_payload_type_of_t<Type>>;


// ===========================================================================
// VI.  AGGREGATE CLASSIFICATION CONCEPTS
// ===========================================================================

// classified_file_tree
//   concept: shorthand for any type recognized as a file tree by the aggregate
// classification struct.
template<typename Type>
concept classified_file_tree =
    file_tree_class<Type>::is_file_tree;

// classified_named_file_tree
//   concept: shorthand for any type recognized as named by the aggregate
// classification struct.
template<typename Type>
concept classified_named_file_tree =
    file_tree_class<Type>::is_named;

// classified_path_addressable_file_tree
//   concept: shorthand for any type recognized as path-addressable by the
// aggregate classification struct.
template<typename Type>
concept classified_path_addressable_file_tree =
    file_tree_class<Type>::is_path_addressable;

// classified_traversable_file_tree
//   concept: shorthand for any type recognized as traversable by the aggregate
// classification struct.
template<typename Type>
concept classified_traversable_file_tree =
    file_tree_class<Type>::is_traversable;

// classified_mutable_file_tree
//   concept: shorthand for any type recognized as mutable by the aggregate
// classification struct.
template<typename Type>
concept classified_mutable_file_tree =
    file_tree_class<Type>::is_mutable;

// classified_file_entry_carrying_tree
//   concept: shorthand for any type whose node payload is recognized as
// file_entry-shaped by the aggregate classification struct.
template<typename Type>
concept classified_file_entry_carrying_tree =
    file_tree_class<Type>::carries_file_entry;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_CONCEPTS_HPP
