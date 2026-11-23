/*******************************************************************************
* djinterp [cpp][container]                             arena_tests_concepts.cpp
*
*   arena_concepts.hpp: the payload and link-policy concepts, and the node,
* arena and cross-arena concepts.  Each forwards to arena_traits.hpp, so these
* check that each concept and its trait agree, through cv and references.
*
*
* path:      /tests/djinterp/cpp/container/arena/arena_tests_concepts.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// djinterp
#include "arena_tests.hpp"  // D_AN_CHECK, fixtures, the declarations
#include <djinterp/cpp/container/arena/arena_concepts.hpp>  // the header under test
#include <djinterp/cpp/container/arena/arena.hpp>           // its subjects


NS_DJINTERP
NS_TESTING

/*
tests_arena_concepts_payload_and_policy
  the payload and link-policy concepts
  Tests the following:
  - arena_payload and non_arena_payload split the payload fixtures
  - link_policy holds for the presets, not for int
  - each single-link concept follows its flag
  - binary, n-ary, O(1)-detach and O(1)-append policies, by preset
*/
bool tests_arena_concepts_payload_and_policy()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20
    D_AN_CHECK(arena_payload<int>);
    D_AN_CHECK(arena_payload<const an_move_only&>);
    D_AN_CHECK(non_arena_payload<an_immovable>);
    D_AN_CHECK(non_arena_payload<an_indestructible>);

    D_AN_CHECK(link_policy<DFullNaryLinkPolicy>);
    D_AN_CHECK(!link_policy<int>);
    D_AN_CHECK(first_child_link_policy<DLcrsLinkPolicy>);
    D_AN_CHECK(next_sibling_link_policy<DLcrsLinkPolicy>);
    D_AN_CHECK(!parent_link_policy<DLcrsLinkPolicy>);
    D_AN_CHECK(parent_link_policy<DParentedLcrsLinkPolicy>);
    D_AN_CHECK(prev_sibling_link_policy<DFullNaryLinkPolicy>);
    D_AN_CHECK(last_child_link_policy<DFullNaryLinkPolicy>);
    D_AN_CHECK(left_link_policy<DBinaryLinkPolicy>);
    D_AN_CHECK(right_link_policy<DBinaryLinkPolicy>);
    D_AN_CHECK(!left_link_policy<DFullNaryLinkPolicy>);

    D_AN_CHECK(binary_link_policy_type<DParentedBinaryLinkPolicy>);
    D_AN_CHECK(!binary_link_policy_type<DLcrsLinkPolicy>);
    D_AN_CHECK(nary_link_policy<DLcrsLinkPolicy>);
    D_AN_CHECK(!nary_link_policy<DBinaryLinkPolicy>);
    D_AN_CHECK(o1_detach_link_policy<DFullNaryLinkPolicy>);
    D_AN_CHECK(!o1_detach_link_policy<DParentedLcrsLinkPolicy>);
    D_AN_CHECK(o1_append_link_policy<DFullNaryLinkPolicy>);
    D_AN_CHECK(!o1_append_link_policy<DLcrsLinkPolicy>);
#endif

    return true;
}

/*
tests_arena_concepts_node_and_arena
  the node, arena and cross-arena concepts
  Tests the following:
  - arena_node_type, typed, policy-aware and valid-payload nodes; a
    non-node is none of them, and classified_arena_node is their union
  - arena_like and non_arena_like split the fixtures
  - typed, policy-aware and valid-payload arenas
  - parent-traversable, binary, n-ary, O(1)-detachable and appendable arenas
  - cross_referenceable_arenas holds for any two arenas
*/
bool tests_arena_concepts_node_and_arena()
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS_CPP20
    using node = DArenaNode<int, DLcrsLinkPolicy>;
    using full = DNodeArena<int>;
    using bin  = DNodeArena<int, DParentedBinaryLinkPolicy>;

    D_AN_CHECK(arena_node_type<node>);
    D_AN_CHECK(typed_arena_node<node>);
    D_AN_CHECK(policy_aware_arena_node<const node&>);
    D_AN_CHECK(valid_payload_arena_node<node>);
    D_AN_CHECK(classified_arena_node<node>);
    D_AN_CHECK(!arena_node_type<int>);
    D_AN_CHECK(!typed_arena_node<int>);
    D_AN_CHECK(!classified_arena_node<int>);

    D_AN_CHECK(arena_like<full>);
    D_AN_CHECK(arena_like<an_arena_like>);
    D_AN_CHECK(non_arena_like<int>);
    D_AN_CHECK(typed_arena<full>);
    D_AN_CHECK(policy_aware_arena<bin>);
    D_AN_CHECK(valid_payload_arena<full>);
    D_AN_CHECK(!typed_arena<int>);

    D_AN_CHECK(parent_traversable_arena<full>);
    D_AN_CHECK(!parent_traversable_arena<DNodeArena<int, DLcrsLinkPolicy>>);
    D_AN_CHECK(binary_arena<bin>);
    D_AN_CHECK(!binary_arena<full>);
    D_AN_CHECK(nary_arena<full>);
    D_AN_CHECK(o1_detachable_arena<full>);
    D_AN_CHECK(o1_appendable_arena<full>);
    D_AN_CHECK(!o1_appendable_arena<bin>);
    D_AN_CHECK(classified_arena<bin>);
    D_AN_CHECK(!classified_arena<int>);

    D_AN_CHECK((cross_referenceable_arenas<full, bin>));
    D_AN_CHECK(!(cross_referenceable_arenas<full, int>));
#endif

    return true;
}

NS_END  // testing
NS_END  // djinterp
