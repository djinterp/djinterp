/*******************************************************************************
* djinterp [cpp][container]                                arena_tests_arena.cpp
*
*   arena.hpp section VI: the arena container's construction, allocation and
* free list, topology (append, prepend, insert, detach with and without a
* parent link, move), collection, depth, traversal, identity, clear, swap and
* equality.
*
*
* path:      /tests/djinterp/cpp/container/arena/arena_tests_arena.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// std
#include <utility>  // std::pair
#include <vector>   // std::vector
// djinterp
#include "arena_tests.hpp"  // D_AN_CHECK, fixtures, the declarations
#include <djinterp/cpp/container/arena/arena.hpp>  // the header under test


NS_DJINTERP
NS_TESTING

// an_children
//   helper: the children of _parent, in order.
template<typename Arena>
static std::vector<node_id> an_children(
    const Arena& _a,
    node_id      _parent
)
{
    std::vector<node_id> out;

    _a.collect_children(_parent, out);

    return out;
}

// an_ids
//   helper: a vector of node ids, for comparisons.
static std::vector<node_id> an_ids(
    std::initializer_list<node_id> _ids
)
{
    return std::vector<node_id>(_ids);
}

// an_fan
//   helper: allocates a root and appends _count children to it (payloads 1
// to _count), returning the root.  The children are slots 1 to _count.
template<typename Arena>
static node_id an_fan(
    Arena&      _a,
    std::size_t _count
)
{
    node_id root = _a.allocate(0);

    for (std::size_t i = 1; i <= _count; ++i)
    {
        _a.append_child(root, _a.allocate(static_cast<int>(i)));
    }

    return root;
}

/*
tests_arena_construction
  a new arena is empty, with stable ids from 1
  Tests the following:
  - size, arena_size and empty agree on an empty arena
  - next_stable_id starts at 1
  - the reserve constructor reserves without allocating nodes
  - nothing is valid in an empty arena
*/
bool tests_arena_construction()
{
    DNodeArena<int> a;
    DNodeArena<int> r(16);

    D_AN_CHECK(a.size()       == 0);
    D_AN_CHECK(a.arena_size() == 0);
    D_AN_CHECK(a.empty());
    D_AN_CHECK(a.next_stable_id() == 1);
    D_AN_CHECK(!a.valid(0));
    D_AN_CHECK(!a.valid(null_node));

    D_AN_CHECK(r.size()     == 0);
    D_AN_CHECK(r.empty());
    D_AN_CHECK(r.capacity() >= 16);

    return true;
}

/*
tests_arena_allocation
  allocation assigns slots, stable ids and version 1
  Tests the following:
  - slots are handed out 0, 1, 2 and stable ids 1, 2, 3
  - each new node is live, version 1, with its payload and no links
  - an explicit stable id is kept, and moves the counter past it
  - an explicit stable id below the counter leaves the counter alone
*/
bool tests_arena_allocation()
{
    DNodeArena<int> a;
    node_id    x = a.allocate(10);
    node_id    y = a.allocate(20);
    node_id    z = a.allocate(30);

    D_AN_CHECK(x == 0);
    D_AN_CHECK(y == 1);
    D_AN_CHECK(z == 2);
    D_AN_CHECK(a.stable_id(x) == 1);
    D_AN_CHECK(a.stable_id(z) == 3);
    D_AN_CHECK(a.version(y)   == 1);
    D_AN_CHECK(a.data(y)      == 20);
    D_AN_CHECK(a.valid(y));
    D_AN_CHECK(a[y].parent()  == null_node);
    D_AN_CHECK(a.size() == 3);
    D_AN_CHECK(!a.empty());

    node_id w = a.allocate(100, 40);
    D_AN_CHECK(a.stable_id(w)     == 100);
    D_AN_CHECK(a.next_stable_id() == 101);

    node_id v = a.allocate(50, 50);
    D_AN_CHECK(a.stable_id(v)     == 50);
    D_AN_CHECK(a.next_stable_id() == 101);
    D_AN_CHECK(a.node_at(v).data  == 50);

    return true;
}

/*
tests_arena_free_list
  a freed slot is reused first, with a fresh identity
  Tests the following:
  - deallocate makes a node invalid and lowers size, not arena_size
  - the most recently freed slot is reused first (a LIFO free list)
  - the reused node gets the next stable id, version 1 and no links
*/
bool tests_arena_free_list()
{
    DNodeArena<int> a;
    node_id    root = an_fan(a, 2);    // 0, with children 1 and 2

    a.detach(1);
    a.deallocate(1);
    D_AN_CHECK(!a.valid(1));
    D_AN_CHECK(a.size()       == 2);
    D_AN_CHECK(a.arena_size() == 3);

    a.detach(2);
    a.deallocate(2);
    D_AN_CHECK(a.size() == 1);

    node_id p = a.allocate(7);
    node_id q = a.allocate(8);
    D_AN_CHECK(p == 2);                 // freed last, reused first
    D_AN_CHECK(q == 1);
    D_AN_CHECK(a.arena_size() == 3);
    D_AN_CHECK(a.stable_id(p) == 4);
    D_AN_CHECK(a.stable_id(q) == 5);
    D_AN_CHECK(a.version(p)   == 1);
    D_AN_CHECK(a.data(p)      == 7);
    D_AN_CHECK(a[p].parent()  == null_node);
    D_AN_CHECK(a[p].next_sibling() == null_node);
    D_AN_CHECK(a.child_count(root) == 0);

    return true;
}

/*
tests_arena_append_prepend
  append_child and prepend_child keep every link
  Tests the following:
  - appending three children gives them in order, with parent, first_child,
    last_child, next_sibling and prev_sibling all consistent
  - prepending puts a child first and keeps last_child
*/
bool tests_arena_append_prepend()
{
    DNodeArena<int> a;
    node_id    root = an_fan(a, 3);    // children 1, 2, 3

    D_AN_CHECK(an_children(a, root) == an_ids({ 1, 2, 3 }));
    D_AN_CHECK(a[root].first_child() == 1);
    D_AN_CHECK(a[root].last_child()  == 3);
    D_AN_CHECK(a[2].parent()         == root);
    D_AN_CHECK(a[2].prev_sibling()   == 1);
    D_AN_CHECK(a[2].next_sibling()   == 3);
    D_AN_CHECK(a[1].prev_sibling()   == null_node);
    D_AN_CHECK(a[3].next_sibling()   == null_node);

    node_id p = a.allocate(9);
    a.prepend_child(root, p);
    D_AN_CHECK(an_children(a, root) == an_ids({ p, 1, 2, 3 }));
    D_AN_CHECK(a[root].first_child() == p);
    D_AN_CHECK(a[root].last_child()  == 3);
    D_AN_CHECK(a[1].prev_sibling()   == p);
    D_AN_CHECK(a[p].parent()         == root);
    D_AN_CHECK(a.child_count(root)   == 4);

    return true;
}

/*
tests_arena_insert
  insert_after and insert_before keep every link
  Tests the following:
  - inserting after the middle child links both neighbours
  - inserting after the last child makes it the parent's last child
  - inserting before the first child makes it the parent's first child
*/
bool tests_arena_insert()
{
    DNodeArena<int> a;
    node_id    root = an_fan(a, 2);    // children 1, 2

    node_id m = a.allocate(5);
    a.insert_after(1, m);
    D_AN_CHECK(an_children(a, root) == an_ids({ 1, m, 2 }));
    D_AN_CHECK(a[m].parent()       == root);
    D_AN_CHECK(a[m].prev_sibling() == 1);
    D_AN_CHECK(a[2].prev_sibling() == m);

    node_id t = a.allocate(6);
    a.insert_after(2, t);
    D_AN_CHECK(a[root].last_child() == t);
    D_AN_CHECK(an_children(a, root) == an_ids({ 1, m, 2, t }));

    node_id h = a.allocate(7);
    a.insert_before(1, h);
    D_AN_CHECK(a[root].first_child() == h);
    D_AN_CHECK(a[1].prev_sibling()   == h);
    D_AN_CHECK(an_children(a, root) == an_ids({ h, 1, m, 2, t }));

    return true;
}

/*
tests_arena_detach_linked
  detach with a parent link unlinks the node and keeps its subtree
  Tests the following:
  - full n-ary (O(1)): the middle, the first and the last child, each
    leaving the parent's first_child, last_child and siblings consistent
  - parented LCRS (no prev_sibling: the parent's chain is walked): the
    middle and the first child
  - the detached node's own links outward are cleared, and its children stay
*/
bool tests_arena_detach_linked()
{
    DNodeArena<int> a;
    node_id    root = an_fan(a, 3);    // children 1, 2, 3
    node_id    g    = a.allocate(9);

    a.append_child(2, g);
    a.detach(2);
    D_AN_CHECK(an_children(a, root) == an_ids({ 1, 3 }));
    D_AN_CHECK(a[1].next_sibling() == 3);
    D_AN_CHECK(a[3].prev_sibling() == 1);
    D_AN_CHECK(a[2].parent()       == null_node);
    D_AN_CHECK(a[2].next_sibling() == null_node);
    D_AN_CHECK(a[2].prev_sibling() == null_node);
    D_AN_CHECK(an_children(a, 2)   == an_ids({ g }));

    a.detach(1);
    D_AN_CHECK(a[root].first_child() == 3);
    a.detach(3);
    D_AN_CHECK(a[root].first_child() == null_node);
    D_AN_CHECK(a[root].last_child()  == null_node);

    DNodeArena<int, DParentedLcrsLinkPolicy> p;
    node_id pr = an_fan(p, 3);
    p.detach(2);
    D_AN_CHECK(an_children(p, pr) == an_ids({ 1, 3 }));
    p.detach(1);
    D_AN_CHECK(an_children(p, pr) == an_ids({ 3 }));
    D_AN_CHECK(p[3].parent() == pr);

    return true;
}

/*
tests_arena_detach_unparented
  detach without a parent link unlinks correctly
  Tests the following:
  - LCRS (first_child, next_sibling): the middle child, then the first; the
    others keep their order (it once cut the sibling chain at the node)
  - first_child, next_sibling and prev_sibling, no parent: the first child
    (whose parent nothing on the node names)
  - first_child, next_sibling and last_child, no parent: the last child,
    whose parent's last_child moves back
  - detaching a node nothing links to changes nothing
*/
bool tests_arena_detach_unparented()
{
    using fnp = DTreeLinkPolicy<DTreeLink::first_child  |
                                 DTreeLink::next_sibling |
                                 DTreeLink::prev_sibling>;
    using fnl = DTreeLinkPolicy<DTreeLink::first_child  |
                                 DTreeLink::next_sibling |
                                 DTreeLink::last_child>;

    DNodeArena<int, DLcrsLinkPolicy> l;
    node_id lr = an_fan(l, 3);
    l.detach(2);
    D_AN_CHECK(an_children(l, lr) == an_ids({ 1, 3 }));
    D_AN_CHECK(l[2].next_sibling() == null_node);
    l.detach(1);
    D_AN_CHECK(an_children(l, lr) == an_ids({ 3 }));
    l.detach(lr);                       // a root: nothing links to it
    D_AN_CHECK(an_children(l, lr) == an_ids({ 3 }));

    DNodeArena<int, fnp> p;
    node_id pr = an_fan(p, 3);
    p.detach(1);
    D_AN_CHECK(an_children(p, pr) == an_ids({ 2, 3 }));
    D_AN_CHECK(p[2].prev_sibling() == null_node);

    DNodeArena<int, fnl> t;
    node_id tr = an_fan(t, 3);
    t.detach(3);
    D_AN_CHECK(an_children(t, tr) == an_ids({ 1, 2 }));
    D_AN_CHECK(t[tr].last_child() == 2);
    t.detach(1);
    t.detach(2);
    D_AN_CHECK(t[tr].first_child() == null_node);
    D_AN_CHECK(t[tr].last_child()  == null_node);

    return true;
}

/*
tests_arena_move_subtree
  move_subtree re-parents a whole subtree
  Tests the following:
  - the moved node becomes the new parent's last child
  - it leaves its old parent's children
  - its own children come with it, one level deeper
*/
bool tests_arena_move_subtree()
{
    DNodeArena<int> a;
    node_id    root = an_fan(a, 2);    // children 1, 2
    node_id    g    = a.allocate(9);

    a.append_child(1, g);
    D_AN_CHECK(a.depth(g) == 2);

    a.move_subtree(1, 2);
    D_AN_CHECK(an_children(a, root) == an_ids({ 2 }));
    D_AN_CHECK(an_children(a, 2)    == an_ids({ 1 }));
    D_AN_CHECK(an_children(a, 1)    == an_ids({ g }));
    D_AN_CHECK(a[1].parent()  == 2);
    D_AN_CHECK(a.depth(g)     == 3);

    return true;
}

/*
tests_arena_collection_and_depth
  subtree, children, ancestors, depth and child count
  Tests the following:
  - collect_subtree gathers the root and its descendants breadth-first
  - collect_ancestors gathers the parents up to the root, nearest first,
    without the node itself
  - depth counts the links to the root (the root is 0)
  - child_count counts immediate children only
*/
bool tests_arena_collection_and_depth()
{
    DNodeArena<int> a;
    node_id    root = an_fan(a, 2);    // children 1, 2
    node_id    g    = a.allocate(9);   // slot 3, under 1
    std::vector<node_id> sub;
    std::vector<node_id> up;

    a.append_child(1, g);
    a.collect_subtree(root, sub);
    D_AN_CHECK(sub == an_ids({ root, 1, 2, g }));

    a.collect_ancestors(g, up);
    D_AN_CHECK(up == an_ids({ 1, root }));

    D_AN_CHECK(a.depth(root) == 0);
    D_AN_CHECK(a.depth(2)    == 1);
    D_AN_CHECK(a.depth(g)    == 2);
    D_AN_CHECK(a.child_count(root) == 2);
    D_AN_CHECK(a.child_count(1)    == 1);
    D_AN_CHECK(a.child_count(g)    == 0);

    return true;
}

/*
tests_arena_traversal
  depth-first visits in pre-order, breadth-first by level, each with depth
  Tests the following:
  - visit_depth_first: root, then each child's subtree in order
  - visit_breadth_first: the root, then level 1, then level 2
  - the depth handed to the visitor matches depth()
*/
bool tests_arena_traversal()
{
    DNodeArena<int> a;
    node_id    root = an_fan(a, 2);    // children 1, 2
    node_id    g    = a.allocate(9);   // slot 3, under 1
    std::vector<std::pair<node_id, std::size_t>> dfs;
    std::vector<std::pair<node_id, std::size_t>> bfs;

    a.append_child(1, g);
    a.visit_depth_first(root, [&dfs](node_id _id, std::size_t _d)
    {
        dfs.push_back(std::make_pair(_id, _d));
    });
    a.visit_breadth_first(root, [&bfs](node_id _id, std::size_t _d)
    {
        bfs.push_back(std::make_pair(_id, _d));
    });

    D_AN_CHECK(dfs.size() == 4);
    D_AN_CHECK(dfs[0] == std::make_pair(root, std::size_t(0)));
    D_AN_CHECK(dfs[1] == std::make_pair(node_id(1), std::size_t(1)));
    D_AN_CHECK(dfs[2] == std::make_pair(g, std::size_t(2)));
    D_AN_CHECK(dfs[3] == std::make_pair(node_id(2), std::size_t(1)));

    D_AN_CHECK(bfs.size() == 4);
    D_AN_CHECK(bfs[0] == std::make_pair(root, std::size_t(0)));
    D_AN_CHECK(bfs[1] == std::make_pair(node_id(1), std::size_t(1)));
    D_AN_CHECK(bfs[2] == std::make_pair(node_id(2), std::size_t(1)));
    D_AN_CHECK(bfs[3] == std::make_pair(g, std::size_t(2)));

    return true;
}

/*
tests_arena_identity
  versions, stable ids and the next id
  Tests the following:
  - bump_version raises a node's version by one, and only its own
  - set_next_stable_id sets the id the next allocation takes
*/
bool tests_arena_identity()
{
    DNodeArena<int> a;
    node_id    x = a.allocate(1);
    node_id    y = a.allocate(2);

    a.bump_version(x);
    a.bump_version(x);
    D_AN_CHECK(a.version(x) == 3);
    D_AN_CHECK(a.version(y) == 1);

    a.set_next_stable_id(500);
    D_AN_CHECK(a.next_stable_id() == 500);
    D_AN_CHECK(a.stable_id(a.allocate(3)) == 500);
    D_AN_CHECK(a.next_stable_id() == 501);

    return true;
}

/*
tests_arena_clear_and_swap
  clear resets everything; swap exchanges everything
  Tests the following:
  - clear empties the arena, its slots and its free list, and restarts the
    stable ids at 1
  - swap exchanges the nodes, the live counts, the free lists and the
    next stable ids
*/
bool tests_arena_clear_and_swap()
{
    DNodeArena<int> a;
    DNodeArena<int> b;

    an_fan(a, 3);
    a.detach(3);
    a.deallocate(3);
    b.allocate(7);

    a.swap(b);
    D_AN_CHECK(a.size() == 1);
    D_AN_CHECK(b.size() == 3);
    D_AN_CHECK(a.data(0) == 7);
    D_AN_CHECK(a.next_stable_id() == 2);
    D_AN_CHECK(b.next_stable_id() == 5);
    D_AN_CHECK(b.allocate(8) == 3);     // b kept a's free list

    b.clear();
    D_AN_CHECK(b.empty());
    D_AN_CHECK(b.arena_size() == 0);
    D_AN_CHECK(b.next_stable_id() == 1);
    D_AN_CHECK(b.allocate(9) == 0);
    D_AN_CHECK(b.stable_id(0) == 1);

    return true;
}

/*
tests_arena_equality
  arenas compare everything a caller can observe
  Tests the following:
  - two arenas built the same way are equal, and so are two empty ones
  - a different payload, link or live count makes them unequal
  - the same live nodes with a different free-list head are unequal (the
    next allocate would differ)
*/
bool tests_arena_equality()
{
    DNodeArena<an_payload> a;
    DNodeArena<an_payload> b;

    D_AN_CHECK(a == b);

    a.allocate(an_payload{ 1 });
    b.allocate(an_payload{ 1 });
    D_AN_CHECK(a == b);
    D_AN_CHECK(!(a != b));

    a.data(0).value = 2;
    D_AN_CHECK(a != b);
    a.data(0).value = 1;

    a.allocate(an_payload{ 3 });
    D_AN_CHECK(a != b);
    b.allocate(an_payload{ 3 });
    a.append_child(0, 1);
    D_AN_CHECK(a != b);
    b.append_child(0, 1);
    D_AN_CHECK(a == b);

    DNodeArena<an_payload> c;
    DNodeArena<an_payload> d;
    c.allocate(an_payload{ 1 });
    c.allocate(an_payload{ 2 });
    d.allocate(an_payload{ 1 });
    d.allocate(an_payload{ 2 });
    c.deallocate(0);
    c.deallocate(1);                    // c's free list: 1, then 0
    d.deallocate(1);
    d.deallocate(0);                    // d's free list: 0, then 1
    D_AN_CHECK(c.size() == d.size());
    D_AN_CHECK(c != d);

    return true;
}

/*
tests_arena_equivalence
  equivalent(): the same forest of payloads, wherever it sits (both kinds of equality)
  Tests the following:
  - the same tree built in another slot order is unequal but equivalent
  - stable ids, versions and freed slots don't count; a payload does
  - children in another order make another tree
  - a forest's trees match in any order; an extra node breaks the match
  - an LCRS arena, without parent links, finds its trees the same way
*/
bool tests_arena_equivalence()
{
    // a: 1 -> {2, 3}, the root allocated first
    DNodeArena<an_payload> a;
    a.allocate(an_payload{ 1 });
    a.allocate(an_payload{ 2 });
    a.allocate(an_payload{ 3 });
    a.append_child(0, 1);
    a.append_child(0, 2);

    // b: the same tree, the root allocated last
    DNodeArena<an_payload> b;
    b.allocate(an_payload{ 2 });
    b.allocate(an_payload{ 3 });
    b.allocate(an_payload{ 1 });
    b.append_child(2, 0);
    b.append_child(2, 1);

    D_AN_CHECK(a != b);
    D_AN_CHECK(equivalent(a, b));
    D_AN_CHECK(equivalent(b, a));
    D_AN_CHECK(equivalent(a, a));

    // other stable ids: unequal, still equivalent; a payload matters
    DNodeArena<an_payload> c;
    c.allocate(100, an_payload{ 1 });
    c.allocate(200, an_payload{ 2 });
    c.allocate(300, an_payload{ 3 });
    c.append_child(0, 1);
    c.append_child(0, 2);
    D_AN_CHECK(c != a);
    D_AN_CHECK(equivalent(a, c));
    c.data(2).value = 4;
    D_AN_CHECK(!equivalent(a, c));

    // the children in the other order
    DNodeArena<an_payload> d;
    d.allocate(an_payload{ 1 });
    d.allocate(an_payload{ 3 });
    d.allocate(an_payload{ 2 });
    d.append_child(0, 1);
    d.append_child(0, 2);
    D_AN_CHECK(!equivalent(a, d));

    // a forest of two single-node trees, in either slot order
    DNodeArena<an_payload> e;
    DNodeArena<an_payload> f;
    e.allocate(an_payload{ 7 });
    e.allocate(an_payload{ 8 });
    f.allocate(an_payload{ 8 });
    f.allocate(an_payload{ 7 });
    D_AN_CHECK(equivalent(e, f));
    f.allocate(an_payload{ 9 });
    D_AN_CHECK(!equivalent(e, f));
    e.allocate(an_payload{ 9 });
    D_AN_CHECK(equivalent(e, f));

    // a slot freed and reused bumps its version: unequal, equivalent
    e.deallocate(2);
    e.allocate(an_payload{ 9 });
    D_AN_CHECK(e != f);
    D_AN_CHECK(equivalent(e, f));

    // LCRS, no parent links: the same tree in two slot orders
    DNodeArena<an_payload, DLcrsLinkPolicy> g;
    DNodeArena<an_payload, DLcrsLinkPolicy> h;
    g.allocate(an_payload{ 1 });
    g.allocate(an_payload{ 2 });
    g.append_child(0, 1);
    h.allocate(an_payload{ 2 });
    h.allocate(an_payload{ 1 });
    h.append_child(1, 0);
    D_AN_CHECK(g != h);
    D_AN_CHECK(equivalent(g, h));
    h.data(0).value = 5;
    D_AN_CHECK(!equivalent(g, h));

    return true;
}

NS_END  // testing
NS_END  // djinterp
