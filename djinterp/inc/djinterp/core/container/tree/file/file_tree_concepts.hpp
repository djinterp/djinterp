/******************************************************************************
* djinterp [core]                                        file_tree_concepts.hpp
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
* path:      /inc/cpp/fs/file_tree_concepts.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_CONCEPTS_
#define DJINTERP_FS_FILE_TREE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "file_tree_concepts.hpp requires C++ compilation"
#endif

#include "./file_tree_traits.hpp"


NS_DJINTERP
NS_FS

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.   CORE STRUCTURAL CONCEPTS
// ===========================================================================

// node_typed_file_tree
//   concept: the type exposes a nested node_type alias.
template<typename _Type>
concept node_typed_file_tree =
    has_node_type_v<_Type>;

// scannable_file_tree
//   concept: the type exposes a scan(const char*) populator.
template<typename _Type>
concept scannable_file_tree =
    has_scan_method_v<_Type>;

// resolvable_file_tree
//   concept: the type exposes a resolve(const char*) path lookup.
template<typename _Type>
concept resolvable_file_tree =
    has_resolve_method_v<_Type>;


// ===========================================================================
// II.  FILE TREE IDENTITY CONCEPTS
// ===========================================================================

// file_tree_type
//   concept: the type satisfies the minimum structural requirements of a
// file tree - a node_type alias plus the scan and resolve surface.
template<typename _Type>
concept file_tree_type =
    node_typed_file_tree<_Type> &&
    scannable_file_tree<_Type>  &&
    resolvable_file_tree<_Type>;

// named_file_tree
//   concept: the file tree exposes name access (raw pointer or
// std::string form).
template<typename _Type>
concept named_file_tree =
    file_tree_type<_Type> &&
    ( has_name_method_v<_Type> || has_name_str_method_v<_Type> );

// path_addressable_file_tree
//   concept: the file tree can reconstruct a full path from a node.
template<typename _Type>
concept path_addressable_file_tree =
    file_tree_type<_Type> &&
    has_full_path_method_v<_Type>;


// ===========================================================================
// III. TRAVERSAL CONCEPTS
// ===========================================================================

// dfs_traversable_file_tree
//   concept: the file tree supports visit_depth_first(root, fn).
template<typename _Type>
concept dfs_traversable_file_tree =
    file_tree_type<_Type> &&
    has_depth_first_method_v<_Type>;

// bfs_traversable_file_tree
//   concept: the file tree supports visit_breadth_first(root, fn).
template<typename _Type>
concept bfs_traversable_file_tree =
    file_tree_type<_Type> &&
    has_breadth_first_method_v<_Type>;

// traversable_file_tree
//   concept: the file tree supports at least one traversal order.
template<typename _Type>
concept traversable_file_tree =
    file_tree_type<_Type> &&
    ( has_depth_first_method_v<_Type> ||
      has_breadth_first_method_v<_Type> );


// ===========================================================================
// IV.  MUTATION CONCEPTS
// ===========================================================================

// child_insertable_file_tree
//   concept: the file tree supports add_child(parent, name, type).
template<typename _Type>
concept child_insertable_file_tree =
    file_tree_type<_Type> &&
    has_add_child_method_v<_Type>;

// clearable_file_tree
//   concept: the file tree supports clear().
template<typename _Type>
concept clearable_file_tree =
    has_clear_method_v<_Type>;

// mutable_file_tree
//   concept: the file tree supports both child insertion and clearing.
template<typename _Type>
concept mutable_file_tree =
    file_tree_type<_Type> &&
    has_add_child_method_v<_Type> &&
    has_clear_method_v<_Type>;


// ===========================================================================
// V.   PAYLOAD SHAPE CONCEPTS
// ===========================================================================

// file_entry_payload
//   concept: the payload type carries the full file_entry shape
// (name_offset / name_length / type / size).
template<typename _Payload>
concept file_entry_payload =
    is_file_entry_payload_v<_Payload>;

// file_entry_carrying_tree
//   concept: the file tree's node_type carries a file_entry-shaped
// payload.
template<typename _Type>
concept file_entry_carrying_tree =
    file_tree_type<_Type> &&
    is_file_entry_payload_v<file_payload_type_of_t<_Type>>;


// ===========================================================================
// VI.  AGGREGATE CLASSIFICATION CONCEPTS
// ===========================================================================

// classified_file_tree
//   concept: shorthand for any type recognized as a file tree by the
// aggregate classification struct.
template<typename _Type>
concept classified_file_tree =
    file_tree_class<_Type>::is_file_tree;

// classified_named_file_tree
//   concept: shorthand for any type recognized as named by the aggregate
// classification struct.
template<typename _Type>
concept classified_named_file_tree =
    file_tree_class<_Type>::is_named;

// classified_path_addressable_file_tree
//   concept: shorthand for any type recognized as path-addressable by
// the aggregate classification struct.
template<typename _Type>
concept classified_path_addressable_file_tree =
    file_tree_class<_Type>::is_path_addressable;

// classified_traversable_file_tree
//   concept: shorthand for any type recognized as traversable by the
// aggregate classification struct.
template<typename _Type>
concept classified_traversable_file_tree =
    file_tree_class<_Type>::is_traversable;

// classified_mutable_file_tree
//   concept: shorthand for any type recognized as mutable by the
// aggregate classification struct.
template<typename _Type>
concept classified_mutable_file_tree =
    file_tree_class<_Type>::is_mutable;

// classified_file_entry_carrying_tree
//   concept: shorthand for any type whose node payload is recognized as
// file_entry-shaped by the aggregate classification struct.
template<typename _Type>
concept classified_file_entry_carrying_tree =
    file_tree_class<_Type>::carries_file_entry;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_CONCEPTS_
