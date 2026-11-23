/*******************************************************************************
* djinterp [cpp][container]                                 arena_tests_tree.cpp
*
*   tree/arena_tree.hpp: root tracking, the allocate-and-link helpers, single
* and subtree removal (on a parented and an unparented policy), clear, swap and
* equality, and the four aliases.
*
*
* path:      /tests/djinterp/cpp/container/arena/arena_tests_tree.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "arena_tests.hpp"  // D_AN_CHECK, fixtures, the declarations
#include <djinterp/cpp/container/arena/tree/arena_tree.hpp>  // the header under test


NS_DJINTERP
NS_TESTING

// an_kids
//   helper: the children of _parent, in order.
template<typename Tree>
static std::vector<node_id> an_kids(
    const Tree& _t,
    node_id     _parent
)
{
    std::vector<node_id> out;

    _t.collect_children(_parent, out);

    return out;
}

/*
tests_arena_tree_root
  root tracking: create, query, set
  Tests the following:
  - a new tree has no root, and root() is null_node
  - create_root allocates the root and records it; is_root names it only
  - create_root with a stable id keeps that id
  - set_root moves the root to another node
*/
bool tests_arena_tree_root()
{
    DArenaNaryTree<int> t;
    DArenaNaryTree<int> u;

    D_AN_CHECK(!t.has_root());
    D_AN_CHECK(t.root() == null_node);

    node_id r = t.create_root(5);
    D_AN_CHECK(t.has_root());
    D_AN_CHECK(t.root() == r);
    D_AN_CHECK(t.is_root(r));
    D_AN_CHECK(t.data(r) == 5);
    D_AN_CHECK(t.size() == 1);

    node_id c = t.add_child(r, 6);
    D_AN_CHECK(!t.is_root(c));
    t.set_root(c);
    D_AN_CHECK(t.root() == c);
    D_AN_CHECK(t.is_root(c));
    D_AN_CHECK(!t.is_root(r));

    node_id s = u.create_root(77, 3);
    D_AN_CHECK(u.stable_id(s) == 77);
    D_AN_CHECK(u.next_stable_id() == 78);

    return true;
}

/*
tests_arena_tree_add
  add_child, add_child_first and add_sibling_after
  Tests the following:
  - add_child appends, add_child_first prepends, add_sibling_after inserts
    after the given sibling, each allocating the node and returning it
  - each new node's parent is the right one
  - add_child with a stable id keeps the id
*/
bool tests_arena_tree_add()
{
    DArenaNaryTree<int> t;
    node_id r = t.create_root(0);
    node_id a = t.add_child(r, 1);
    node_id b = t.add_child(r, 2);
    node_id f = t.add_child_first(r, 3);
    node_id s = t.add_sibling_after(a, 4);
    node_id k = t.add_child(r, 50, 5);

    D_AN_CHECK(an_kids(t, r) == std::vector<node_id>({ f, a, s, b, k }));
    D_AN_CHECK(t[s].parent() == r);
    D_AN_CHECK(t[f].parent() == r);
    D_AN_CHECK(t.data(s) == 4);
    D_AN_CHECK(t.stable_id(k) == 50);
    D_AN_CHECK(t.size() == 6);

    return true;
}

/*
tests_arena_tree_remove
  remove takes one node and orphans its children
  Tests the following:
  - removing a leaf frees it and leaves its siblings in order
  - removing a node with children frees only it; each child stays live, as a
    root of its own: no parent, no siblings, depth 0
  - when the freed slot is reused, the orphans are still roots
  - removing the root clears it
*/
bool tests_arena_tree_remove()
{
    DArenaNaryTree<int> t;
    node_id r  = t.create_root(0);
    node_id a  = t.add_child(r, 1);
    node_id b  = t.add_child(r, 2);
    node_id c1 = t.add_child(a, 3);
    node_id c2 = t.add_child(a, 4);

    t.remove(b);
    D_AN_CHECK(!t.valid(b));
    D_AN_CHECK(an_kids(t, r) == std::vector<node_id>({ a }));

    t.remove(a);
    D_AN_CHECK(!t.valid(a));
    D_AN_CHECK(t.valid(c1));
    D_AN_CHECK(t.valid(c2));
    D_AN_CHECK(t[c1].parent()       == null_node);
    D_AN_CHECK(t[c1].next_sibling() == null_node);
    D_AN_CHECK(t[c2].prev_sibling() == null_node);
    D_AN_CHECK(t.depth(c2) == 0);
    D_AN_CHECK(t.child_count(r) == 0);
    D_AN_CHECK(t.size() == 3);

    t.add_child(r, 9);                  // reuses a freed slot
    D_AN_CHECK(t.depth(c1) == 0);
    D_AN_CHECK(t.depth(c2) == 0);

    t.remove(r);
    D_AN_CHECK(!t.has_root());

    return true;
}

/*
tests_arena_tree_remove_subtree
  remove_subtree frees the whole subtree, on any n-ary policy
  Tests the following:
  - removing an inner node frees it and every descendant, and its siblings
    stay in order
  - removing the root empties the tree
  - on an LCRS tree (no parent link), removing a middle subtree keeps both
    neighbours
  - removing null_node does nothing
*/
bool tests_arena_tree_remove_subtree()
{
    DArenaNaryTree<int> t;
    node_id r = t.create_root(0);
    node_id a = t.add_child(r, 1);
    node_id b = t.add_child(r, 2);
    node_id c = t.add_child(r, 3);

    t.add_child(t.add_child(b, 4), 5);
    t.remove_subtree(b);
    D_AN_CHECK(t.size() == 3);
    D_AN_CHECK(an_kids(t, r) == std::vector<node_id>({ a, c }));

    t.remove_subtree(null_node);
    D_AN_CHECK(t.size() == 3);

    t.remove_subtree(r);
    D_AN_CHECK(t.empty());
    D_AN_CHECK(!t.has_root());

    DArenaLcrsTree<int> l;
    node_id lr = l.create_root(0);
    node_id la = l.add_child(lr, 1);
    node_id lb = l.add_child(lr, 2);
    node_id lc = l.add_child(lr, 3);
    l.add_child(lb, 4);
    l.remove_subtree(lb);
    D_AN_CHECK(l.size() == 3);
    D_AN_CHECK(an_kids(l, lr) == std::vector<node_id>({ la, lc }));

    return true;
}

/*
tests_arena_tree_clear_swap_equality
  clear, swap and equality include the root
  Tests the following:
  - clear empties the tree and forgets the root
  - swap exchanges the roots with the arenas
  - two trees built the same way are equal; moving one's root makes them
    unequal
*/
bool tests_arena_tree_clear_swap_equality()
{
    DArenaNaryTree<int> t;
    DArenaNaryTree<int> u;

    node_id r = t.create_root(1);
    t.add_child(r, 2);
    u.create_root(1);
    u.add_child(u.root(), 2);
    D_AN_CHECK(t == u);
    D_AN_CHECK(!(t != u));

    t.set_root(1);
    D_AN_CHECK(t != u);
    t.set_root(r);

    DArenaNaryTree<int> v;
    v.swap(t);
    D_AN_CHECK(!t.has_root());
    D_AN_CHECK(t.empty());
    D_AN_CHECK(v.root() == r);
    D_AN_CHECK(v.size() == 2);

    v.clear();
    D_AN_CHECK(!v.has_root());
    D_AN_CHECK(v.empty());

    return true;
}

/*
tests_arena_tree_aliases
  the four aliases name the four policies
  Tests the following:
  - arena_nary_tree is full n-ary, arena_lcrs_tree LCRS, arena_parented_tree
    parented LCRS, and arena_binary_tree parented binary
  - each is an arena_tree of its payload, and a binary tree can hold a root
*/
bool tests_arena_tree_aliases()
{
    DArenaBinaryTree<int> b;

    D_AN_CHECK(std::is_same<DArenaNaryTree<int>::link_policy,
                            DFullNaryLinkPolicy>::value);
    D_AN_CHECK(std::is_same<DArenaLcrsTree<int>::link_policy,
                            DLcrsLinkPolicy>::value);
    D_AN_CHECK(std::is_same<DArenaParentedTree<int>::link_policy,
                            DParentedLcrsLinkPolicy>::value);
    D_AN_CHECK(std::is_same<DArenaBinaryTree<int>::link_policy,
                            DParentedBinaryLinkPolicy>::value);
    D_AN_CHECK(std::is_same<DArenaLcrsTree<char>,
                            DArenaTree<char, DLcrsLinkPolicy>>::value);
    D_AN_CHECK(std::is_same<DArenaNaryTree<int>::payload_type, int>::value);

    node_id r = b.create_root(7);
    D_AN_CHECK(b.is_root(r));
    D_AN_CHECK(b.data(r) == 7);

    return true;
}

/*
tests_arena_tree_equivalence
  equivalent() on an arena tree: the arenas' forests, and the root's tree
  Tests the following:
  - two trees with no root, or the same rooted tree in other slots, are
    equivalent
  - the same forest rooted at another of its trees is not, though the two
    arenas are
*/
bool tests_arena_tree_equivalence()
{
    using tree_type  = DArenaTree<an_payload>;
    using arena_type = DNodeArena<an_payload, tree_type::link_policy>;

    tree_type none_a;
    tree_type none_b;
    D_AN_CHECK(equivalent(none_a, none_b));

    // t: root 1 -> {2}, and a second tree 5
    tree_type t;
    const node_id t_root = t.create_root(an_payload{ 1 });
    t.add_child(t_root, an_payload{ 2 });
    t.allocate(an_payload{ 5 });

    // u: the same, allocated in another order
    tree_type u;
    u.allocate(an_payload{ 5 });
    const node_id u_root = u.create_root(an_payload{ 1 });
    u.add_child(u_root, an_payload{ 2 });

    D_AN_CHECK(t != u);
    D_AN_CHECK(equivalent(t, u));
    D_AN_CHECK(!equivalent(t, none_a));

    // v: the same forest, rooted at 5
    tree_type v;
    v.create_root(an_payload{ 5 });
    const node_id other = v.allocate(an_payload{ 1 });
    v.append_child(other, v.allocate(an_payload{ 2 }));

    D_AN_CHECK(!equivalent(t, v));
    D_AN_CHECK(equivalent(static_cast<const arena_type&>(t),
                          static_cast<const arena_type&>(v)));

    return true;
}

NS_END  // testing
NS_END  // djinterp
