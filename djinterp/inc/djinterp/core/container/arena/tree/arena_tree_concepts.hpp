/******************************************************************************
* djinterp [container]                               arena_tree_concepts.hpp
*
*  djinterp arena tree classification concepts
*   C++20 concepts layered on top of arena_tree_traits.hpp.  These
* concepts provide readable `requires` constraints for rooted arenas,
* arena-tree mutation operations, navigation capabilities, and tree
* topology.
*
*   This header is intentionally thin: it does not re-implement
* detection. Instead, each concept forwards to the corresponding public
* trait or variable template from the arena tree trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Root Ownership Concepts
* 3.   Mutation Concepts
* 4.   Navigation Concepts
* 5.   Topology and Complexity Concepts
*
* path:      /inc/container/arena/arena_tree_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                 date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_ARENA_TREE_CONCEPTS_
#define DJINTERP_ARENA_TREE_CONCEPTS_ 1

#include <type_traits>
#include "../arena_concepts.hpp"
#include "./arena_tree_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "arena_tree_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP

// ============================================================================
// I.   Root Ownership Concepts
// ============================================================================

// root_query_arena
//   concept: constrains arenas exposing root().
template<typename _Type>
concept root_query_arena = has_root_method<clean_t<_Type>>::value;

// root_presence_query_arena
//   concept: constrains arenas exposing has_root().
template<typename _Type>
concept root_presence_query_arena = has_has_root_method<clean_t<_Type>>::value;

// root_testable_arena
//   concept: constrains arenas exposing is_root(node_id).
template<typename _Type>
concept root_testable_arena = has_is_root_method<clean_t<_Type>>::value;

// root_assignable_arena
//   concept: constrains arenas exposing set_root(node_id).
template<typename _Type>
concept root_assignable_arena = has_set_root_method<clean_t<_Type>>::value;

// arena_tree
//   concept: constrains types satisfying the arena tree protocol.
template<typename _Type>
concept arena_tree = is_arena_tree<clean_t<_Type>>::value;

// rooted_arena
//   concept: constrains types recognized as rooted arenas.
template<typename _Type>
concept rooted_arena = is_rooted_arena<clean_t<_Type>>::value;

// non_arena_tree
//   concept: constrains types that do not satisfy the arena tree
// protocol.
template<typename _Type>
concept non_arena_tree = !arena_tree<_Type>;


// =============================================================================
// II.  Mutation Concepts
// =============================================================================

// create_root_arena
//   concept: constrains arena trees exposing create_root(payload).
template<typename _Type>
concept create_root_arena =
    has_create_root_method<clean_t<_Type>>::value;

// child_addable_arena
//   concept: constrains arena trees exposing add_child(node_id,
// payload).
template<typename _Type>
concept child_addable_arena =
    has_add_child_method<clean_t<_Type>>::value;

// subtree_removable_arena
//   concept: constrains arena trees exposing remove_subtree(node_id).
template<typename _Type>
concept subtree_removable_arena =
    has_remove_subtree_method<clean_t<_Type>>::value;

// mutable_arena_tree
//   concept: constrains arena trees supporting root creation and child
// insertion.
template<typename _Type>
concept mutable_arena_tree =
    ( arena_tree<_Type>          &&
      create_root_arena<_Type>   &&
      child_addable_arena<_Type> );


// =============================================================================
// III. Navigation Concepts
// =============================================================================

// parent_navigable_arena
//   concept: constrains arena trees supporting child-to-root traversal.
template<typename _Type>
concept parent_navigable_arena =
    is_parent_navigable<clean_t<_Type>>::value;

// sibling_navigable_arena
//   concept: constrains arena trees supporting bidirectional sibling
// traversal.
template<typename _Type>
concept sibling_navigable_arena =
    is_sibling_navigable<clean_t<_Type>>::value;

// fully_navigable_arena
//   concept: constrains arena trees supporting the full n-ary navigation
// set.
template<typename _Type>
concept fully_navigable_arena =
    is_fully_navigable<clean_t<_Type>>::value;


// =============================================================================
// IV.  Topology and Complexity Concepts
// =============================================================================

// binary_arena_tree
//   concept: constrains arena trees using a binary link layout.
template<typename _Type>
concept binary_arena_tree =
    is_binary_arena<clean_t<_Type>>::value;

// nary_arena_tree
//   concept: constrains arena trees using an n-ary link layout.
template<typename _Type>
concept nary_arena_tree =
    is_nary_arena<clean_t<_Type>>::value;

// o1_detachable_arena_tree
//   concept: constrains arena trees supporting O(1) detach.
template<typename _Type>
concept o1_detachable_arena_tree =
    arena_tree_class<clean_t<_Type>>::o1_detach;

// o1_appendable_arena_tree
//   concept: constrains arena trees supporting O(1) append-to-children.
template<typename _Type>
concept o1_appendable_arena_tree =
    arena_tree_class<clean_t<_Type>>::o1_append;

// classified_arena_tree
//   concept: constrains types recognized by the arena tree trait layer.
template<typename _Type>
concept classified_arena_tree =
    ( arena_tree<_Type>              ||
      rooted_arena<_Type>            ||
      parent_navigable_arena<_Type>  ||
      sibling_navigable_arena<_Type> ||
      fully_navigable_arena<_Type>   ||
      binary_arena_tree<_Type>       ||
      nary_arena_tree<_Type> );


NS_END  // djinterp


#endif  // DJINTERP_ARENA_TREE_CONCEPTS_
