/*******************************************************************************
* djinterp [cpp][container]                                 arena_tests_node.cpp
*
*   arena.hpp section V: arena_node's default and constructed state, its links
* by index and by name, has_children and is_leaf, reset_links, and its
* equality.
*
*
* path:      /tests/djinterp/cpp/container/arena/arena_tests_node.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// djinterp
#include "arena_tests.hpp"  // D_AN_CHECK, fixtures, the declarations
#include <djinterp/cpp/container/arena/arena.hpp>  // the header under test


NS_DJINTERP
NS_TESTING

/*
tests_arena_node_default_state
  a default node is dead, unlinked and anonymous
  Tests the following:
  - stable_id 0, version 0, not alive, free_next null_node
  - every link is null_node, so it has no children and is a leaf
  - num_links and the two member types follow the policy
*/
bool tests_arena_node_default_state()
{
    using node = DArenaNode<int, DFullNaryLinkPolicy>;
    node n;

    D_AN_CHECK(node::num_links == 5);
    D_AN_CHECK(std::is_same<node::payload_type, int>::value);
    D_AN_CHECK(std::is_same<node::link_policy, DFullNaryLinkPolicy>::value);
    D_AN_CHECK(std::is_same<DArenaNode<int>::link_policy,
                            DFullNaryLinkPolicy>::value);

    D_AN_CHECK(n.stable_id == 0);
    D_AN_CHECK(n.version   == 0);
    D_AN_CHECK(!n.alive);
    D_AN_CHECK(n.free_next == null_node);

    for (std::size_t i = 0; i < node::num_links; ++i)
    {
        D_AN_CHECK(n.links[i] == null_node);
    }

    D_AN_CHECK(!n.has_children());
    D_AN_CHECK(n.is_leaf());

    return true;
}

/*
tests_arena_node_constructed_state
  a node built from a stable id and a payload is live, version 1, unlinked
  Tests the following:
  - the copy and the move constructors both set stable_id, version 1, alive
    and the payload
  - its links are all null_node and free_next is null_node
*/
bool tests_arena_node_constructed_state()
{
    using node = DArenaNode<int, DLcrsLinkPolicy>;
    const int seven = 7;
    node a(42, seven);
    node b(43, 8);

    D_AN_CHECK(a.stable_id == 42);
    D_AN_CHECK(a.version   == 1);
    D_AN_CHECK(a.alive);
    D_AN_CHECK(a.data      == 7);
    D_AN_CHECK(a.free_next == null_node);
    D_AN_CHECK(a.links[0] == null_node);
    D_AN_CHECK(a.links[1] == null_node);

    D_AN_CHECK(b.stable_id == 43);
    D_AN_CHECK(b.version   == 1);
    D_AN_CHECK(b.alive);
    D_AN_CHECK(b.data      == 8);

    return true;
}

/*
tests_arena_node_links
  links by index and by name, children, leaves, and reset
  Tests the following:
  - set_link<I> and get_link<I> address the slot the policy assigns
  - each named accessor reads its own slot
  - an n-ary node has children exactly when first_child is set
  - a binary node has children when either left or right is set
  - reset_links sets every link back to null_node
*/
bool tests_arena_node_links()
{
    DArenaNode<int, DFullNaryLinkPolicy>       n;
    DArenaNode<int, DParentedBinaryLinkPolicy> b;

    n.set_link<DFullNaryLinkPolicy::first_child_idx>(1);
    n.set_link<DFullNaryLinkPolicy::next_sibling_idx>(2);
    n.set_link<DFullNaryLinkPolicy::parent_idx>(3);
    n.set_link<DFullNaryLinkPolicy::prev_sibling_idx>(4);
    n.set_link<DFullNaryLinkPolicy::last_child_idx>(5);

    D_AN_CHECK(n.first_child()  == 1);
    D_AN_CHECK(n.next_sibling() == 2);
    D_AN_CHECK(n.parent()       == 3);
    D_AN_CHECK(n.prev_sibling() == 4);
    D_AN_CHECK(n.last_child()   == 5);
    D_AN_CHECK(n.get_link<0>()  == 1);
    D_AN_CHECK(n.get_link<4>()  == 5);
    D_AN_CHECK(n.has_children());
    D_AN_CHECK(!n.is_leaf());

    n.reset_links();
    D_AN_CHECK(n.first_child() == null_node);
    D_AN_CHECK(n.last_child()  == null_node);
    D_AN_CHECK(n.parent()      == null_node);
    D_AN_CHECK(n.is_leaf());

    D_AN_CHECK(b.is_leaf());
    b.set_link<DParentedBinaryLinkPolicy::right_idx>(9);
    D_AN_CHECK(b.right() == 9);
    D_AN_CHECK(b.left()  == null_node);
    D_AN_CHECK(b.has_children());
    b.reset_links();
    b.set_link<DParentedBinaryLinkPolicy::left_idx>(8);
    D_AN_CHECK(b.left() == 8);
    D_AN_CHECK(b.has_children());
    b.set_link<DParentedBinaryLinkPolicy::parent_idx>(7);
    D_AN_CHECK(b.parent() == 7);

    return true;
}

/*
tests_arena_node_equality
  nodes compare their observable state
  Tests the following:
  - two live nodes with the same links, identity and payload are equal
  - a different link, stable_id, version or payload makes them unequal
  - a live node never equals a dead one
  - two dead slots are equal when their free-list links are, whatever
    payload each still holds
*/
bool tests_arena_node_equality()
{
    using node = DArenaNode<an_payload, DLcrsLinkPolicy>;
    node a(1, an_payload{ 5 });
    node b(1, an_payload{ 5 });

    D_AN_CHECK(a == b);
    D_AN_CHECK(!(a != b));

    b.set_link<0>(3);
    D_AN_CHECK(a != b);
    b.reset_links();
    D_AN_CHECK(a == b);

    b.stable_id = 2;
    D_AN_CHECK(a != b);
    b.stable_id = 1;
    b.version   = 2;
    D_AN_CHECK(a != b);
    b.version   = 1;
    b.data      = an_payload{ 6 };
    D_AN_CHECK(a != b);

    b.alive = false;
    D_AN_CHECK(a != b);

    a.alive     = false;
    a.free_next = 4;
    b.free_next = 4;
    D_AN_CHECK(a == b);     // payloads 5 and 6, but both slots are dead
    b.free_next = 9;
    D_AN_CHECK(a != b);

    return true;
}

NS_END  // testing
NS_END  // djinterp
