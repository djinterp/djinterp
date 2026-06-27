// djinterp
#include "test_tree_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_tree_add_root_creates_conjunctive_root
  Verifies the first add_root lazily creates the implied conjunctive root and
  attaches the new top-level root beneath it.
  Tests the following:
  - root() is null before, non-null after the first add_root
  - size() == 2 (conjunctive root + one root child)
  - the returned node is non-null
*/
bool
tests_tree_add_root_creates_conjunctive_root()
{
    test_tree<basic_test> t;

    const bool before = (t.root() == nullptr);

    test_tree<basic_test>::node_type* child =
        t.add_root(make_test(1, true));

    const bool after =
        ( (t.root() != nullptr) &&
          (t.size() == 2)       &&
          (child    != nullptr) );

    return ( before &&
             after );
}

/*
tests_tree_add_root_returns_child_node
  Verifies add_root returns the newly inserted TOP-LEVEL root (a child of the
  conjunctive root), carrying the supplied element - not the conjunctive root
  itself.
  Tests the following:
  - the returned node stores the inserted element (type id and status)
  - the returned node is distinct from root() (the conjunctive root)
*/
bool
tests_tree_add_root_returns_child_node()
{
    test_tree<basic_test> t;

    test_tree<basic_test>::node_type* child =
        t.add_root(make_test(5, true));

    return ( (child != nullptr)                            &&
             (child->data().type_id() == 5)               &&
             status_is(child->data(), test_status::passed) &&
             (t.root() != child)                          &&
             (t.root()->data().type_id() == 0) );
}

/*
tests_tree_add_root_multiple_roots
  Verifies many roots all attach beneath the single conjunctive root.
  Tests the following:
  - ensure_conjunctive_root is created once, reused thereafter
  - size() == roots + 1
  - the run surface sees every root child plus the pending conjunctive root
*/
bool
tests_tree_add_root_multiple_roots()
{
    test_tree<basic_test> t;
    t.add_root(make_test(1, true));
    t.add_root(make_test(2, true));
    t.add_root(make_test(3, true));

    return ( (t.size()          == 4) &&
             (t.count_passed()  == 3) &&
             (t.count_pending() == 1) );
}

/*
tests_tree_add_root_not_rank_checked
  Verifies add_root is UNCONSTRAINED: the conjunctive root admits any
  sequence, including leaf-kind roots and non-monotonic ranks.
  Tests the following:
  - a root whose kind is registered as a leaf is still accepted
  - roots inserted in arbitrary (non-decreasing or decreasing) rank order all
    succeed
*/
bool
tests_tree_add_root_not_rank_checked()
{
    std::vector<test_kind> ks;
    ks.push_back(make_leaf_kind(5, 1));   // id 5 is a leaf kind

    test_tree<basic_test> t(std::move(ks));

    test_tree<basic_test>::node_type* a =
        t.add_root(make_test(5, true));    // leaf-kind root: fine at top level
    test_tree<basic_test>::node_type* b =
        t.add_root(make_test(1, true));    // lower id after: not rank-checked
    test_tree<basic_test>::node_type* c =
        t.add_root(make_test(99, true));   // higher id: not rank-checked

    return ( (a != nullptr) &&
             (b != nullptr) &&
             (c != nullptr) &&
             (t.size() == 4) );
}

/*
tests_tree_add_root_conjunctive_root_is_pending
  Verifies the conjunctive root is a default-constructed element: type id 0
  and status pending.
  Tests the following:
  - root()->data() has type id 0
  - root()->data() has status pending
*/
bool
tests_tree_add_root_conjunctive_root_is_pending()
{
    test_tree<basic_test> t;
    t.add_root(make_test(5, true));

    test_tree<basic_test>::node_type* r = t.root();

    return ( (r != nullptr)             &&
             (r->data().type_id() == 0) &&
             status_is(r->data(), test_status::pending) );
}


NS_END  // testing
NS_END  // djinterp
