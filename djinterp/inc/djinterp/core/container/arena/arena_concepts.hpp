/******************************************************************************
* djinterp [container]                                    arena_concepts.hpp
*
*  djinterp arena classification concepts
*   C++20 concepts layered on top of arena_traits.hpp.  These concepts
* provide readable `requires` constraints for arena payloads, link
* policies, arena nodes, arena-like containers, and cross-arena
* relationships.
*
*   This header is intentionally thin: it does not re-implement
* detection. Instead, each concept forwards to the corresponding public
* trait or variable template from the arena trait layer.
*
* TABLE OF CONTENTS
* =================
* 1.   Feature Gate
* 2.   Payload Concepts
* 3.   Link Policy Concepts
* 4.   Arena Node Concepts
* 5.   Arena Concepts
* 6.   Cross-Arena Concepts
*
* path:      /inc/container/arena/arena_concepts.hpp
* link(s):   TBA
* author(s): OpenAI ChatGPT                                 date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_ARENA_CONCEPTS_
#define DJINTERP_ARENA_CONCEPTS_ 1

#include <type_traits>
#include "arena_traits.hpp"

#if !defined(__cpp_concepts) || (__cpp_concepts < 201907L)
    #error "arena_concepts.hpp requires C++20 concepts support."
#endif


NS_DJINTERP

// =============================================================================
// I.   Payload Concepts
// =============================================================================

// arena_payload
//   concept: constrains types satisfying the minimum arena payload
// requirements.
template<typename _Type>
concept arena_payload =
    is_arena_payload<clean_t<_Type>>::value;

// non_arena_payload
//   concept: constrains types that do not satisfy arena payload
// requirements.
template<typename _Type>
concept non_arena_payload =
    !arena_payload<_Type>;


// =============================================================================
// II.  Link Policy Concepts
// =============================================================================

// link_policy
//   concept: constrains types exposing the arena link policy interface.
template<typename _Type>
concept link_policy =
    is_link_policy<clean_t<_Type>>::value;

// first_child_link_policy
//   concept: constrains policies providing a first-child link.
template<typename _Type>
concept first_child_link_policy =
    has_link_flag<clean_t<_Type>, tree_link::first_child>::value;

// next_sibling_link_policy
//   concept: constrains policies providing a next-sibling link.
template<typename _Type>
concept next_sibling_link_policy =
    has_link_flag<clean_t<_Type>, tree_link::next_sibling>::value;

// parent_link_policy
//   concept: constrains policies providing a parent link.
template<typename _Type>
concept parent_link_policy =
    has_link_flag<clean_t<_Type>, tree_link::parent>::value;

// prev_sibling_link_policy
//   concept: constrains policies providing a previous-sibling link.
template<typename _Type>
concept prev_sibling_link_policy =
    has_link_flag<clean_t<_Type>, tree_link::prev_sibling>::value;

// last_child_link_policy
//   concept: constrains policies providing a last-child link.
template<typename _Type>
concept last_child_link_policy =
    has_link_flag<clean_t<_Type>, tree_link::last_child>::value;

// left_link_policy
//   concept: constrains policies providing a left-child link.
template<typename _Type>
concept left_link_policy =
    has_link_flag<clean_t<_Type>, tree_link::left>::value;

// right_link_policy
//   concept: constrains policies providing a right-child link.
template<typename _Type>
concept right_link_policy =
    has_link_flag<clean_t<_Type>, tree_link::right>::value;

// binary_link_policy
//   concept: constrains policies using a binary left/right layout.
template<typename _Type>
concept binary_link_policy =
    ( left_link_policy<_Type> &&
      right_link_policy<_Type> );

// nary_link_policy
//   concept: constrains policies using an n-ary first-child / next-
// sibling layout.
template<typename _Type>
concept nary_link_policy =
    ( first_child_link_policy<_Type> &&
      next_sibling_link_policy<_Type> );

// o1_detach_link_policy
//   concept: constrains policies supporting O(1) detach.
template<typename _Type>
concept o1_detach_link_policy =
    ( prev_sibling_link_policy<_Type> &&
      next_sibling_link_policy<_Type> );

// o1_append_link_policy
//   concept: constrains policies supporting O(1) append-to-children.
template<typename _Type>
concept o1_append_link_policy =
    ( first_child_link_policy<_Type> &&
      last_child_link_policy<_Type> );


// =============================================================================
// III. Arena Node Concepts
// =============================================================================

// arena_node
//   concept: constrains types satisfying the arena node protocol.
template<typename _Type>
concept arena_node =
    is_arena_node<clean_t<_Type>>::value;

// typed_arena_node
//   concept: constrains arena nodes exposing a payload_type.
template<typename _Type>
concept typed_arena_node =
    !std::is_void_v<arena_node_payload_type_t<clean_t<_Type>>>;

// policy_aware_arena_node
//   concept: constrains arena nodes exposing a link_policy.
template<typename _Type>
concept policy_aware_arena_node =
    !std::is_void_v<arena_node_link_policy_t<clean_t<_Type>>>;

// valid_payload_arena_node
//   concept: constrains arena nodes whose payload_type satisfies the
// arena payload rules.
template<typename _Type>
concept valid_payload_arena_node =
    ( typed_arena_node<_Type> &&
      is_arena_payload<
          arena_node_payload_type_t<clean_t<_Type>>>::value );

// classified_arena_node
//   concept: constrains types recognized by the arena node trait layer.
template<typename _Type>
concept classified_arena_node =
    ( arena_node<_Type> ||
      typed_arena_node<_Type> ||
      policy_aware_arena_node<_Type> );


// =============================================================================
// IV.  Arena Concepts
// =============================================================================

// arena_like
//   concept: constrains types satisfying the arena container protocol.
template<typename _Type>
concept arena_like =
    is_arena<clean_t<_Type>>::value;

// non_arena_like
//   concept: constrains types that do not satisfy the arena protocol.
template<typename _Type>
concept non_arena_like =
    !arena_like<_Type>;

// typed_arena
//   concept: constrains arena-like types exposing a payload_type.
template<typename _Type>
concept typed_arena =
    !std::is_void_v<arena_payload_type_t<clean_t<_Type>>>;

// policy_aware_arena
//   concept: constrains arena-like types exposing a link_policy.
template<typename _Type>
concept policy_aware_arena =
    !std::is_void_v<arena_link_policy_type_t<clean_t<_Type>>>;

// valid_payload_arena
//   concept: constrains arenas whose payload_type satisfies the arena
// payload requirements.
template<typename _Type>
concept valid_payload_arena =
    ( typed_arena<_Type> &&
      is_arena_payload<
          arena_payload_type_t<clean_t<_Type>>>::value );

// parent_traversable_arena
//   concept: constrains arenas supporting parent traversal.
template<typename _Type>
concept parent_traversable_arena =
    arena_class<clean_t<_Type>>::supports_parent_traversal;

// binary_arena
//   concept: constrains arenas using a binary left/right layout.
template<typename _Type>
concept binary_arena =
    arena_class<clean_t<_Type>>::is_binary;

// nary_arena
//   concept: constrains arenas using an n-ary first-child / next-
// sibling layout.
template<typename _Type>
concept nary_arena =
    arena_class<clean_t<_Type>>::is_nary;

// o1_detachable_arena
//   concept: constrains arenas supporting O(1) detach.
template<typename _Type>
concept o1_detachable_arena =
    arena_class<clean_t<_Type>>::o1_detach;

// o1_appendable_arena
//   concept: constrains arenas supporting O(1) append-to-children.
template<typename _Type>
concept o1_appendable_arena =
    arena_class<clean_t<_Type>>::o1_append;

// classified_arena
//   concept: constrains types recognized by the arena trait layer.
template<typename _Type>
concept classified_arena =
    ( arena_like<_Type>              ||
      parent_traversable_arena<_Type> ||
      binary_arena<_Type>            ||
      nary_arena<_Type>              ||
      o1_detachable_arena<_Type>     ||
      o1_appendable_arena<_Type> );


// =============================================================================
// V.   Cross-Arena Concepts
// =============================================================================

// cross_referenceable_arenas
//   concept: constrains pairs of arenas that may share stable_id cross-
// references.
template<typename _ArenaA,
         typename _ArenaB>
concept cross_referenceable_arenas =
    arenas_cross_referenceable<
        clean_t<_ArenaA>,
        clean_t<_ArenaB>>::value;


NS_END  // djinterp


#endif  // DJINTERP_ARENA_CONCEPTS_
