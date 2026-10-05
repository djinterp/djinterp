/*******************************************************************************
* djinterp [core]                                       linked_list_concepts.hpp
*
* C++20 concepts for the linked-list module:
*   This header layers concept syntax over the SFINAE traits in
* linked_list_traits.hpp.  Every trait there has a corresponding
* concept here, plus a small set of composite concepts for common
* constraint patterns.  The concepts allow `requires` clauses and
* shorthand template constraints in code that only compiles on C++20.
*
*   When concepts are unavailable (pre-C++20 or compilers without
* __cpp_concepts >= 201907L), the entire concept block is elided.
* The trait surface in linked_list_traits.hpp remains the canonical
* dispatch mechanism - concepts are a syntactic convenience.
*
*
* path:      /inc/djinterp/core/container/list/linked/linked_list_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    node-shape concepts
      -------------------

2.    sentinel concepts
      -----------------

3.    end-pointer concepts
      --------------------

4.    topology / ownership concepts
      -----------------------------

5.    composite list concepts
      -----------------------

6.    capability concepts
      -------------------
*/

#ifndef DJINTERP_CONTAINER_LIST_LINKED_LINKED_LIST_CONCEPTS_HPP
#define DJINTERP_CONTAINER_LIST_LINKED_LINKED_LIST_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../../djinterp.hpp"

#ifndef D_ENV_LANG_DETECTED_CPP
    #error "list_concepts.hpp requires C++ compilation"
#endif  // D_ENV_LANG_DETECTED_CPP

#include "./linked_list_traits.hpp"

// below C++20 concepts this header is empty: a facility above its tier
// is absent, never an #error
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20


NS_DJINTERP


// ===========================================================================
// 1.  NODE-SHAPE CONCEPTS
// ===========================================================================

// linked_list_node_like
//   concept: matches any type that is recognised as a linked-list node (singly
// / doubly / xor / skip).
template<typename Type>
concept linked_list_node_like =
    is_linked_list_node<Type>::value;

// singly_linked_node_type
//   concept: matches singly-linked-shaped nodes specifically.
template<typename Type>
concept singly_linked_node_type =
    is_singly_linked_node<Type>::value;

// doubly_linked_node_type
//   concept: matches doubly-linked-shaped nodes specifically.
template<typename Type>
concept doubly_linked_node_type =
    is_doubly_linked_node<Type>::value;

// xor_linked_node_type
//   concept: matches XOR-linked nodes.
template<typename Type>
concept xor_linked_node_type =
    is_xor_linked_node<Type>::value;

// skip_list_node_type
//   concept: matches skip-list-shaped nodes (multi-level forwards).
template<typename Type>
concept skip_list_node_type =
    is_skip_list_node<Type>::value;

// bidirectional_node_type
//   concept: matches any node that can be traversed in both directions
// (doubly- or xor-linked).
template<typename Type>
concept bidirectional_node_type =
    ( is_doubly_linked_node<Type>::value ||
      is_xor_linked_node<Type>::value );


// ===========================================================================
// 2.  SENTINEL CONCEPTS
// ===========================================================================

// head_sentinel_list
//   concept: list type that exposes a head sentinel.
template<typename Type>
concept head_sentinel_list =
    has_head_sentinel<Type>::value;

// tail_sentinel_list
//   concept: list type that exposes a tail sentinel.
template<typename Type>
concept tail_sentinel_list =
    has_tail_sentinel<Type>::value;

// any_sentinel_list
//   concept: list type with at least one sentinel.
template<typename Type>
concept any_sentinel_list =
    has_any_sentinel<Type>::value;


// ===========================================================================
// 3.  END-POINTER CONCEPTS
// ===========================================================================

// head_accessible_list
//   concept: list type exposing a head() accessor.
template<typename Type>
concept head_accessible_list =
    has_head_pointer<Type>::value;

// tail_accessible_list
//   concept: list type exposing a tail() accessor.
template<typename Type>
concept tail_accessible_list =
    has_tail_pointer<Type>::value;

// head_only_list_type
//   concept: list type with head() but no tail() - typical of
// std::forward_list.
template<typename Type>
concept head_only_list_type =
    is_head_only_list<Type>::value;

// head_tail_list_type
//   concept: list type with both head() and tail().
template<typename Type>
concept head_tail_list_type =
    is_head_tail_list<Type>::value;


// ===========================================================================
// 4.  TOPOLOGY / OWNERSHIP CONCEPTS
// ===========================================================================

// circular_list_type
//   concept: list whose tail's next loops to the head.
template<typename Type>
concept circular_list_type =
    is_circular_list<Type>::value;

// linear_list_type
//   concept: list with nullptr-terminated traversal.
template<typename Type>
concept linear_list_type =
    !is_circular_list<Type>::value;

// intrusive_list_type
//   concept: list that does not own its nodes.
template<typename Type>
concept intrusive_list_type =
    is_intrusive_list<Type>::value;

// owning_list_type
//   concept: list that owns its nodes.
template<typename Type>
concept owning_list_type =
    is_owning_list<Type>::value;


// ===========================================================================
// 5.  COMPOSITE LIST CONCEPTS
// ===========================================================================

// linked_list_type
//   concept: any list recognised as linked-list-shaped.
template<typename Type>
concept linked_list_type =
    is_linked_list<Type>::value;

// singly_linked_list_type
//   concept: a list whose nodes are singly-linked.
template<typename Type>
concept singly_linked_list_type =
    is_singly_linked_list<Type>::value;

// doubly_linked_list_type
//   concept: a list whose nodes are doubly-linked.
template<typename Type>
concept doubly_linked_list_type =
    is_doubly_linked_list<Type>::value;

// xor_linked_list_type
//   concept: a list whose nodes use XOR-linked layout.
template<typename Type>
concept xor_linked_list_type =
    is_xor_linked_list<Type>::value;

// skip_list_type
//   concept: a list whose nodes carry skip levels.
template<typename Type>
concept skip_list_type =
    is_skip_list<Type>::value;

// bidirectional_list_type
//   concept: any list that can be traversed in both directions.
template<typename Type>
concept bidirectional_list_type =
    ( is_doubly_linked_list<Type>::value ||
      is_xor_linked_list<Type>::value );


// ===========================================================================
// 6.  CAPABILITY CONCEPTS
// ===========================================================================

// list_with_o1_back_access
//   concept: list that exposes back() in O(1) time - true when a tail pointer
// is present.
template<typename Type>
concept list_with_o1_back_access =
    ( is_linked_list<Type>::value &&
      has_tail_pointer<Type>::value );

// list_with_o1_push_back
//   concept: list that supports O(1) push_back - same condition as O(1) back
// access plus mutable iteration support.
template<typename Type>
concept list_with_o1_push_back =
    ( is_linked_list<Type>::value &&
      has_tail_pointer<Type>::value );

// reversible_list_type
//   concept: list whose elements can be visited in reverse order. True for
// doubly-linked, xor-linked, or any singly-linked list that exposes a tail
// pointer (so reverse traversal is implemented at the container level, e.g. by
// stack-and-replay).
template<typename Type>
concept reversible_list_type =
    ( is_doubly_linked_list<Type>::value ||
      is_xor_linked_list<Type>::value );

// spliceable_list_type
//   concept: list that supports O(1) splice - requires bidirectional node
// shape so that the rewire can fix both .next and .prev without an O(n) walk.
template<typename Type>
concept spliceable_list_type =
    ( is_doubly_linked_list<Type>::value ||
      is_xor_linked_list<Type>::value );


NS_END  // djinterp


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_LIST_LINKED_LINKED_LIST_CONCEPTS_HPP
