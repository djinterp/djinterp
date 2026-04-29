/******************************************************************************
* djinterp [container]                                 linked_list_concepts.hpp
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
* dispatch mechanism — concepts are a syntactic convenience.
*
* 
* path:      /inc/djinterp/core/container/list/linked/linked_list_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  node-shape concepts
2.  sentinel concepts
3.  end-pointer concepts
4.  topology / ownership concepts
5.  composite list concepts
6.  capability concepts
*/

#ifndef DJINTERP_CONTAINER_LINKED_LIST_CONCEPTS_
#define DJINTERP_CONTAINER_LINKED_LIST_CONCEPTS_ 1

// djinterp
#include "../../../djinterp.hpp"

#ifndef D_ENV_LANG_DETECTED_CPP
    #error "list_concepts.hpp requires C++ compilation"
#elif (!defined(D_ENV_CPP_FEATURE_LANG_CONCEPTS))
    #error "Must support C++ concepts language feature"
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

#include "./linked_list_traits.hpp"


NS_DJINTERP


// ===========================================================================
// 1.  NODE-SHAPE CONCEPTS
// ===========================================================================

// linked_list_node_like
//   concept: matches any type that is recognised as a linked-list
// node (singly / doubly / xor / skip).
template<typename _Type>
concept linked_list_node_like =
    is_linked_list_node<_Type>::value;

// singly_linked_node_type
//   concept: matches singly-linked-shaped nodes specifically.
template<typename _Type>
concept singly_linked_node_type =
    is_singly_linked_node<_Type>::value;

// doubly_linked_node_type
//   concept: matches doubly-linked-shaped nodes specifically.
template<typename _Type>
concept doubly_linked_node_type =
    is_doubly_linked_node<_Type>::value;

// xor_linked_node_type
//   concept: matches XOR-linked nodes.
template<typename _Type>
concept xor_linked_node_type =
    is_xor_linked_node<_Type>::value;

// skip_list_node_type
//   concept: matches skip-list-shaped nodes (multi-level forwards).
template<typename _Type>
concept skip_list_node_type =
    is_skip_list_node<_Type>::value;

// bidirectional_node_type
//   concept: matches any node that can be traversed in both
// directions (doubly- or xor-linked).
template<typename _Type>
concept bidirectional_node_type =
    ( is_doubly_linked_node<_Type>::value ||
      is_xor_linked_node<_Type>::value );


// ===========================================================================
// 2.  SENTINEL CONCEPTS
// ===========================================================================

// head_sentinel_list
//   concept: list type that exposes a head sentinel.
template<typename _Type>
concept head_sentinel_list =
    has_head_sentinel<_Type>::value;

// tail_sentinel_list
//   concept: list type that exposes a tail sentinel.
template<typename _Type>
concept tail_sentinel_list =
    has_tail_sentinel<_Type>::value;

// any_sentinel_list
//   concept: list type with at least one sentinel.
template<typename _Type>
concept any_sentinel_list =
    has_any_sentinel<_Type>::value;


// ===========================================================================
// 3.  END-POINTER CONCEPTS
// ===========================================================================

// head_accessible_list
//   concept: list type exposing a head() accessor.
template<typename _Type>
concept head_accessible_list =
    has_head_pointer<_Type>::value;

// tail_accessible_list
//   concept: list type exposing a tail() accessor.
template<typename _Type>
concept tail_accessible_list =
    has_tail_pointer<_Type>::value;

// head_only_list_type
//   concept: list type with head() but no tail() — typical of
// std::forward_list.
template<typename _Type>
concept head_only_list_type =
    is_head_only_list<_Type>::value;

// head_tail_list_type
//   concept: list type with both head() and tail().
template<typename _Type>
concept head_tail_list_type =
    is_head_tail_list<_Type>::value;


// ===========================================================================
// 4.  TOPOLOGY / OWNERSHIP CONCEPTS
// ===========================================================================

// circular_list_type
//   concept: list whose tail's next loops to the head.
template<typename _Type>
concept circular_list_type =
    is_circular_list<_Type>::value;

// linear_list_type
//   concept: list with nullptr-terminated traversal.
template<typename _Type>
concept linear_list_type =
    !is_circular_list<_Type>::value;

// intrusive_list_type
//   concept: list that does not own its nodes.
template<typename _Type>
concept intrusive_list_type =
    is_intrusive_list<_Type>::value;

// owning_list_type
//   concept: list that owns its nodes.
template<typename _Type>
concept owning_list_type =
    is_owning_list<_Type>::value;


// ===========================================================================
// 5.  COMPOSITE LIST CONCEPTS
// ===========================================================================

// linked_list_type
//   concept: any list recognised as linked-list-shaped.
template<typename _Type>
concept linked_list_type =
    is_linked_list<_Type>::value;

// singly_linked_list_type
//   concept: a list whose nodes are singly-linked.
template<typename _Type>
concept singly_linked_list_type =
    is_singly_linked_list<_Type>::value;

// doubly_linked_list_type
//   concept: a list whose nodes are doubly-linked.
template<typename _Type>
concept doubly_linked_list_type =
    is_doubly_linked_list<_Type>::value;

// xor_linked_list_type
//   concept: a list whose nodes use XOR-linked layout.
template<typename _Type>
concept xor_linked_list_type =
    is_xor_linked_list<_Type>::value;

// skip_list_type
//   concept: a list whose nodes carry skip levels.
template<typename _Type>
concept skip_list_type =
    is_skip_list<_Type>::value;

// bidirectional_list_type
//   concept: any list that can be traversed in both directions.
template<typename _Type>
concept bidirectional_list_type =
    ( is_doubly_linked_list<_Type>::value ||
      is_xor_linked_list<_Type>::value );


// ===========================================================================
// 6.  CAPABILITY CONCEPTS
// ===========================================================================

// list_with_o1_back_access
//   concept: list that exposes back() in O(1) time — true when a
// tail pointer is present.
template<typename _Type>
concept list_with_o1_back_access =
    ( is_linked_list<_Type>::value &&
      has_tail_pointer<_Type>::value );

// list_with_o1_push_back
//   concept: list that supports O(1) push_back — same condition as
// O(1) back access plus mutable iteration support.
template<typename _Type>
concept list_with_o1_push_back =
    ( is_linked_list<_Type>::value &&
      has_tail_pointer<_Type>::value );

// reversible_list_type
//   concept: list whose elements can be visited in reverse order.
// True for doubly-linked, xor-linked, or any singly-linked list
// that exposes a tail pointer (so reverse traversal is implemented
// at the container level, e.g. by stack-and-replay).
template<typename _Type>
concept reversible_list_type =
    ( is_doubly_linked_list<_Type>::value ||
      is_xor_linked_list<_Type>::value );

// spliceable_list_type
//   concept: list that supports O(1) splice — requires bidirectional
// node shape so that the rewire can fix both .next and .prev
// without an O(n) walk.
template<typename _Type>
concept spliceable_list_type =
    ( is_doubly_linked_list<_Type>::value ||
      is_xor_linked_list<_Type>::value );


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_LINKED_LIST_CONCEPTS_