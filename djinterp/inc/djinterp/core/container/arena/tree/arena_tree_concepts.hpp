/*******************************************************************************
* djinterp [core]                                        arena_tree_concepts.hpp
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
*
* path:      /inc/djinterp/core/container/arena/tree/arena_tree_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                  created: 2026.04.07
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    Feature Gate
      ------------

2.    Root Ownership Concepts
      -----------------------

3.    Mutation Concepts
      -----------------

4.    Navigation Concepts
      -------------------

5.    Topology and Complexity Concepts
      --------------------------------
*/

#ifndef DJINTERP_CONTAINER_ARENA_TREE_ARENA_TREE_CONCEPTS_HPP
#define DJINTERP_CONTAINER_ARENA_TREE_ARENA_TREE_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <type_traits>
// djinterp
#include "../arena_concepts.hpp"
#include "./arena_tree_traits.hpp"

// below C++20 concepts this header is empty: a facility above its tier
// is absent, never an #error
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20


NS_DJINTERP

// ============================================================================
// I.   Root Ownership Concepts
// ============================================================================

// root_query_arena
//   concept: constrains arenas exposing root().
template<typename Type>
concept root_query_arena = has_root_method<clean_t<Type>>::value;

// root_presence_query_arena
//   concept: constrains arenas exposing has_root().
template<typename Type>
concept root_presence_query_arena = has_has_root_method<clean_t<Type>>::value;

// root_testable_arena
//   concept: constrains arenas exposing is_root(node_id).
template<typename Type>
concept root_testable_arena = has_is_root_method<clean_t<Type>>::value;

// root_assignable_arena
//   concept: constrains arenas exposing set_root(node_id).
template<typename Type>
concept root_assignable_arena = has_set_root_method<clean_t<Type>>::value;

// arena_tree
//   concept: constrains types satisfying the arena tree protocol.
template<typename Type>
concept arena_tree = is_arena_tree<clean_t<Type>>::value;

// rooted_arena
//   concept: constrains types recognized as rooted arenas.
template<typename Type>
concept rooted_arena = is_rooted_arena<clean_t<Type>>::value;

// non_arena_tree
//   concept: constrains types that do not satisfy the arena tree protocol.
template<typename Type>
concept non_arena_tree = !arena_tree<Type>;


// ===========================================================================
// II.  Mutation Concepts
// ===========================================================================

// create_root_arena
//   concept: constrains arena trees exposing create_root(payload).
template<typename Type>
concept create_root_arena =
    has_create_root_method<clean_t<Type>>::value;

// child_addable_arena
//   concept: constrains arena trees exposing add_child(node_id, payload).
template<typename Type>
concept child_addable_arena =
    has_add_child_method<clean_t<Type>>::value;

// subtree_removable_arena
//   concept: constrains arena trees exposing remove_subtree(node_id).
template<typename Type>
concept subtree_removable_arena =
    has_remove_subtree_method<clean_t<Type>>::value;

// mutable_arena_tree
//   concept: constrains arena trees supporting root creation and child
// insertion.
template<typename Type>
concept mutable_arena_tree =
    ( arena_tree<Type>          &&
      create_root_arena<Type>   &&
      child_addable_arena<Type> );


// ===========================================================================
// III. Navigation Concepts
// ===========================================================================

// parent_navigable_arena
//   concept: constrains arena trees supporting child-to-root traversal.
template<typename Type>
concept parent_navigable_arena =
    is_parent_navigable<clean_t<Type>>::value;

// sibling_navigable_arena
//   concept: constrains arena trees supporting bidirectional sibling
// traversal.
template<typename Type>
concept sibling_navigable_arena =
    is_sibling_navigable<clean_t<Type>>::value;

// fully_navigable_arena
//   concept: constrains arena trees supporting the full n-ary navigation set.
template<typename Type>
concept fully_navigable_arena =
    is_fully_navigable<clean_t<Type>>::value;


// ===========================================================================
// IV.  Topology and Complexity Concepts
// ===========================================================================

// binary_arena_tree
//   concept: constrains arena trees using a binary link layout.
template<typename Type>
concept binary_arena_tree =
    is_binary_arena<clean_t<Type>>::value;

// nary_arena_tree
//   concept: constrains arena trees using an n-ary link layout.
template<typename Type>
concept nary_arena_tree =
    is_nary_arena<clean_t<Type>>::value;

// o1_detachable_arena_tree
//   concept: constrains arena trees supporting O(1) detach.
template<typename Type>
concept o1_detachable_arena_tree =
    arena_tree_class<clean_t<Type>>::o1_detach;

// o1_appendable_arena_tree
//   concept: constrains arena trees supporting O(1) append-to-children.
template<typename Type>
concept o1_appendable_arena_tree =
    arena_tree_class<clean_t<Type>>::o1_append;

// classified_arena_tree
//   concept: constrains types recognized by the arena tree trait layer.
template<typename Type>
concept classified_arena_tree =
    ( arena_tree<Type>              ||
      rooted_arena<Type>            ||
      parent_navigable_arena<Type>  ||
      sibling_navigable_arena<Type> ||
      fully_navigable_arena<Type>   ||
      binary_arena_tree<Type>       ||
      nary_arena_tree<Type> );


NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARENA_TREE_ARENA_TREE_CONCEPTS_HPP
