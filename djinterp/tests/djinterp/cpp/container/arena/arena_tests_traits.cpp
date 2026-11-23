/*******************************************************************************
* djinterp [cpp][container]                               arena_tests_traits.cpp
*
*   arena_traits.hpp: payload validation, link-policy detection and its two
* extractors, node detection and extraction, arena detection and extraction,
* cross-arena referenceability, and arena_class.
*
*
* path:      /tests/djinterp/cpp/container/arena/arena_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "arena_tests.hpp"  // D_AN_CHECK, fixtures, the declarations
#include <djinterp/cpp/container/arena/arena_traits.hpp>  // the header under test
#include <djinterp/cpp/container/arena/arena.hpp>         // its subjects


NS_DJINTERP
NS_TESTING

/*
tests_arena_traits_payload
  a payload is destructible and movable or copyable
  Tests the following:
  - int, an aggregate and a move-only type are payloads
  - a type that can be neither moved nor copied is not
  - a type whose destructor is deleted is not
  - the _v form agrees
*/
bool tests_arena_traits_payload()
{
    D_AN_CHECK(is_arena_payload<int>::value);
    D_AN_CHECK(is_arena_payload<an_payload>::value);
    D_AN_CHECK(is_arena_payload<an_move_only>::value);
    D_AN_CHECK(!is_arena_payload<an_immovable>::value);
    D_AN_CHECK(!is_arena_payload<an_indestructible>::value);
    D_AN_CHECK(is_arena_payload_v<int>);
    D_AN_CHECK(!is_arena_payload_v<an_immovable>);

    return true;
}

/*
tests_arena_traits_link_policy
  link policies, their flags and their link counts
  Tests the following:
  - every preset, and a generated policy, is a link policy; int and a node
    type are not (a node has num_links but no flags)
  - has_link_flag reads one flag, and is false for a non-policy
  - link_policy_flags and link_policy_num_links read the policy, and are 0
    for a non-policy
*/
bool tests_arena_traits_link_policy()
{
    D_AN_CHECK(is_link_policy<DLcrsLinkPolicy>::value);
    D_AN_CHECK(is_link_policy<DFullNaryLinkPolicy>::value);
    D_AN_CHECK(is_link_policy<DParentedBinaryLinkPolicy>::value);
    D_AN_CHECK(is_link_policy<DTreeLinkPolicy<DTreeLink::right>>::value);
    D_AN_CHECK(is_link_policy<const DLcrsLinkPolicy&>::value);
    D_AN_CHECK(!is_link_policy<int>::value);
    D_AN_CHECK(!is_link_policy<DArenaNode<int>>::value);
    D_AN_CHECK(is_link_policy_v<DBinaryLinkPolicy>);

    D_AN_CHECK((has_link_flag<DFullNaryLinkPolicy,
               DTreeLink::parent>::value));
    D_AN_CHECK(!(has_link_flag<DLcrsLinkPolicy, DTreeLink::parent>::value));
    D_AN_CHECK((has_link_flag<DBinaryLinkPolicy, DTreeLink::left>::value));
    D_AN_CHECK(!(has_link_flag<int, DTreeLink::left>::value));
    D_AN_CHECK((has_link_flag_v<DBinaryLinkPolicy, DTreeLink::right>));

    D_AN_CHECK(link_policy_flags<DFullNaryLinkPolicy>::value == 31u);
    D_AN_CHECK(link_policy_flags<int>::value == 0u);
    D_AN_CHECK(link_policy_flags_v<DBinaryLinkPolicy> == 96u);
    D_AN_CHECK(link_policy_num_links<DParentedLcrsLinkPolicy>::value == 3);
    D_AN_CHECK(link_policy_num_links<int>::value == 0);
    D_AN_CHECK(link_policy_num_links_v<DFullNaryLinkPolicy> == 5);

    return true;
}

/*
tests_arena_traits_node
  node detection and the node's two type extractors
  Tests the following:
  - arena_node<...> is a node, through cv and references
  - a type with the five members is_arena_node reads is a node too (the
    detection is structural)
  - int and an arena are not nodes
  - the payload and policy extractors read a node, and give void otherwise
*/
bool tests_arena_traits_node()
{
    using node = DArenaNode<an_payload, DLcrsLinkPolicy>;

    D_AN_CHECK(is_arena_node<node>::value);
    D_AN_CHECK(is_arena_node<const node&>::value);
    D_AN_CHECK(is_arena_node<an_node_like>::value);
    D_AN_CHECK(!is_arena_node<int>::value);
    D_AN_CHECK(!is_arena_node<DNodeArena<int>>::value);
    D_AN_CHECK(is_arena_node_v<node>);

    D_AN_CHECK(std::is_same<arena_node_payload_type_t<node>,
               an_payload>::value);
    D_AN_CHECK(std::is_same<arena_node_link_policy_t<node>,
                            DLcrsLinkPolicy>::value);
    D_AN_CHECK(std::is_same<arena_node_payload_type_t<int>, void>::value);
    D_AN_CHECK(std::is_same<arena_node_link_policy_t<int>, void>::value);

    return true;
}

/*
tests_arena_traits_arena
  arena detection, extraction, and cross-arena referenceability
  Tests the following:
  - arena<...> is an arena, through cv and references, whatever its policy
  - a type with payload_type, link_policy and operator[](node_id) is one too
  - int, std::vector<int> and a node are not
  - the payload and policy extractors read an arena, and give void otherwise
  - two arenas are cross-referenceable whatever their payloads; an arena and
    a non-arena are not
*/
bool tests_arena_traits_arena()
{
    using nary = DNodeArena<an_payload>;
    using bin  = DNodeArena<int, DBinaryLinkPolicy>;

    D_AN_CHECK(is_arena<nary>::value);
    D_AN_CHECK(is_arena<const bin&>::value);
    D_AN_CHECK(is_arena<an_arena_like>::value);
    D_AN_CHECK(!is_arena<int>::value);
    D_AN_CHECK(!is_arena<std::vector<int>>::value);
    D_AN_CHECK(!is_arena<DArenaNode<int>>::value);
    D_AN_CHECK(is_arena_v<bin>);

    D_AN_CHECK(std::is_same<arena_payload_type_t<nary>, an_payload>::value);
    D_AN_CHECK(std::is_same<arena_link_policy_type_t<bin>,
                            DBinaryLinkPolicy>::value);
    D_AN_CHECK(std::is_same<arena_payload_type_t<int>, void>::value);
    D_AN_CHECK(std::is_same<arena_link_policy_type_t<int>, void>::value);

    D_AN_CHECK((arenas_cross_referenceable<nary, bin>::value));
    D_AN_CHECK(!(arenas_cross_referenceable<nary, int>::value));
    D_AN_CHECK((arenas_cross_referenceable_v<bin, bin>));

    return true;
}

/*
tests_arena_traits_class
  arena_class reports each preset's capabilities
  Tests the following:
  - full n-ary: n-ary, parent traversal, O(1) detach and append; not binary
  - LCRS: n-ary only
  - parented binary: binary with parent traversal; not n-ary
  - a non-arena: not an arena, no policy, no valid payload, no capability
*/
bool tests_arena_traits_class()
{
    using full = DArenaClass<DNodeArena<int>>;
    using lcrs = DArenaClass<DNodeArena<int, DLcrsLinkPolicy>>;
    using pbin = DArenaClass<DNodeArena<int, DParentedBinaryLinkPolicy>>;
    using none = DArenaClass<int>;

    D_AN_CHECK(full::is_arena_type);
    D_AN_CHECK(full::valid_payload);
    D_AN_CHECK(full::has_policy);
    D_AN_CHECK(full::is_nary);
    D_AN_CHECK(!full::is_binary);
    D_AN_CHECK(full::supports_parent_traversal);
    D_AN_CHECK(full::o1_detach);
    D_AN_CHECK(full::o1_append);
    D_AN_CHECK(std::is_same<full::payload_type, int>::value);

    D_AN_CHECK(lcrs::is_nary);
    D_AN_CHECK(!lcrs::supports_parent_traversal);
    D_AN_CHECK(!lcrs::o1_detach);
    D_AN_CHECK(!lcrs::o1_append);

    D_AN_CHECK(pbin::is_binary);
    D_AN_CHECK(!pbin::is_nary);
    D_AN_CHECK(pbin::supports_parent_traversal);
    D_AN_CHECK(pbin::has_left);
    D_AN_CHECK(pbin::has_right);

    D_AN_CHECK(!none::is_arena_type);
    D_AN_CHECK(!none::has_policy);
    D_AN_CHECK(!none::valid_payload);
    D_AN_CHECK(!none::is_nary);
    D_AN_CHECK(!none::is_binary);
    D_AN_CHECK(!none::o1_detach);

    return true;
}

NS_END  // testing
NS_END  // djinterp
