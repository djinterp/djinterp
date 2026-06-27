// djinterp
#include "test_tree_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_tree_append_child_null_parent_returns_null
  Verifies the null-parent guard: append_child rejects a null parent and does
  NOT fabricate a conjunctive root (only add_root does that).
  Tests the following:
  - append_child(nullptr, ...) returns nullptr
  - the forest stays empty (size 0, no conjunctive root)
*/
bool
tests_tree_append_child_null_parent_returns_null()
{
    test_tree<basic_test> t;

    test_tree<basic_test>::node_type* c =
        t.append_child(nullptr, make_test(1, true));

    return ( (c == nullptr) &&
             (t.size() == 0) );
}

/*
tests_tree_append_child_no_kinds_lower_rank_accepted
  With no kind set, the raw type id acts as the rank.  A strictly lower-ranked
  child is admitted.
  Tests the following:
  - rank fallback (id-as-rank) via can_be_child_of
  - child rank < parent rank is accepted
*/
bool
tests_tree_append_child_no_kinds_lower_rank_accepted()
{
    test_tree<basic_test> t;

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(5, true));
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(3, true));   // 3 <= 5 -> accept

    return (c != nullptr);
}

/*
tests_tree_append_child_no_kinds_equal_rank_accepted
  Verifies the rank rule is "<=", so an equal-ranked child is admitted
  (boundary).
  Tests the following:
  - child rank == parent rank is accepted
*/
bool
tests_tree_append_child_no_kinds_equal_rank_accepted()
{
    test_tree<basic_test> t;

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(5, true));
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(5, true));   // 5 <= 5 -> accept

    return (c != nullptr);
}

/*
tests_tree_append_child_no_kinds_higher_rank_rejected
  Verifies a strictly higher-ranked child is rejected under the id-as-rank
  fallback.
  Tests the following:
  - child rank > parent rank returns nullptr
*/
bool
tests_tree_append_child_no_kinds_higher_rank_rejected()
{
    test_tree<basic_test> t;

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(3, true));
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(5, true));   // 5 <= 3 is false -> reject

    return (c == nullptr);
}

/*
tests_tree_append_child_registered_interior_accepts_within_rank
  With a kind set, ranks resolve through the matched kinds.  An interior
  parent admits a child of equal-or-lower rank.
  Tests the following:
  - registered interior parent (not a leaf) does not block on the leaf rule
  - resolved child rank <= resolved parent rank is accepted
*/
bool
tests_tree_append_child_registered_interior_accepts_within_rank()
{
    std::vector<test_kind> ks;
    ks.push_back(make_interior_kind(10, 5));   // parent: interior, rank 5
    ks.push_back(make_interior_kind(20, 3));   // child:  rank 3

    test_tree<basic_test> t(std::move(ks));

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(10, true));
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(20, true));   // 3 <= 5 -> accept

    return (c != nullptr);
}

/*
tests_tree_append_child_registered_higher_rank_rejected
  Verifies rank monotonicity through the kind set: a higher-ranked child is
  rejected even under an interior parent.
  Tests the following:
  - resolved child rank > resolved parent rank returns nullptr
*/
bool
tests_tree_append_child_registered_higher_rank_rejected()
{
    std::vector<test_kind> ks;
    ks.push_back(make_interior_kind(10, 5));   // parent: rank 5
    ks.push_back(make_interior_kind(30, 7));   // child:  rank 7

    test_tree<basic_test> t(std::move(ks));

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(10, true));
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(30, true));   // 7 <= 5 is false -> reject

    return (c == nullptr);
}

/*
tests_tree_append_child_registered_leaf_parent_rejects
  Verifies the leaf rule takes precedence over rank: a registered leaf parent
  admits no children, even a strictly lower-ranked one.
  Tests the following:
  - leaf parent rejection short-circuits before the rank check
*/
bool
tests_tree_append_child_registered_leaf_parent_rejects()
{
    std::vector<test_kind> ks;
    ks.push_back(make_leaf_kind(40, 5));        // parent: LEAF, rank 5
    ks.push_back(make_interior_kind(50, 1));    // child:  rank 1 (lower!)

    test_tree<basic_test> t(std::move(ks));

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(40, true));         // leaf-kind parent
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(50, true));  // rank ok, but parent is leaf

    return (c == nullptr);
}

/*
tests_tree_append_child_unregistered_parent_not_treated_as_leaf
  Verifies a subtle point: the leaf check uses find_kind directly, so an
  UNREGISTERED parent is NOT treated as a leaf (unlike the is_leaf free
  function's default).  Admission falls through to the rank check.
  Tests the following:
  - unregistered parent (find_kind -> nullptr) skips the leaf rejection
  - the child is then admitted purely on rank (id-as-rank fallback)
*/
bool
tests_tree_append_child_unregistered_parent_not_treated_as_leaf()
{
    test_tree<basic_test> t;   // empty kind set: id 5 and id 3 unregistered

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(5, true));          // unregistered parent
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(3, true));   // 3 <= 5 -> accept

    return (c != nullptr);
}

/*
tests_tree_append_child_mixed_resolution
  Verifies rank resolution mixes registered and unregistered ids correctly in
  both directions (registered rank vs id-as-rank fallback).
  Tests the following:
  - registered interior parent + unregistered child: accept and reject by rank
  - unregistered parent (id-as-rank) + registered child: accept by rank
*/
bool
tests_tree_append_child_mixed_resolution()
{
    // direction 1: parent registered interior (rank 2), child unregistered
    std::vector<test_kind> ks1;
    ks1.push_back(make_interior_kind(100, 2));

    test_tree<basic_test> t1(std::move(ks1));

    test_tree<basic_test>::node_type* p1 =
        t1.add_root(make_test(100, true));
    test_tree<basic_test>::node_type* c1_ok =
        t1.append_child(p1, make_test(1, true));   // rank 1 <= 2 -> accept
    test_tree<basic_test>::node_type* c1_no =
        t1.append_child(p1, make_test(5, true));   // rank 5 >  2 -> reject

    // direction 2: parent unregistered (rank = id 10), child registered rank 4
    std::vector<test_kind> ks2;
    ks2.push_back(make_interior_kind(3, 4));

    test_tree<basic_test> t2(std::move(ks2));

    test_tree<basic_test>::node_type* p2 =
        t2.add_root(make_test(10, true));
    test_tree<basic_test>::node_type* c2_ok =
        t2.append_child(p2, make_test(3, true));   // rank 4 <= 10 -> accept

    return ( (c1_ok != nullptr) &&
             (c1_no == nullptr) &&
             (c2_ok != nullptr) );
}

/*
tests_tree_append_child_returns_node_and_stores_value
  Verifies a successful append returns the new node carrying the supplied
  element.
  Tests the following:
  - the returned node stores the child's type id
  - the returned node stores the child's status (failed result -> failed)
*/
bool
tests_tree_append_child_returns_node_and_stores_value()
{
    test_tree<basic_test> t;

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(9, true));
    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(4, false));   // 4 <= 9 -> accept; failed

    return ( (c != nullptr)                       &&
             (c->data().type_id() == 4)           &&
             status_is(c->data(), test_status::failed) );
}

/*
tests_tree_append_child_success_increments_size
  Verifies an accepted insert grows the forest by one and a rejected insert
  leaves it unchanged.
  Tests the following:
  - size() increases by exactly one on acceptance
  - a rejected (over-rank) insert returns nullptr and does not change size()
*/
bool
tests_tree_append_child_success_increments_size()
{
    test_tree<basic_test> t;

    test_tree<basic_test>::node_type* p =
        t.add_root(make_test(9, true));

    const std::size_t before = t.size();

    test_tree<basic_test>::node_type* c =
        t.append_child(p, make_test(4, true));    // accept

    const bool grew =
        ( (c != nullptr) &&
          (t.size() == before + 1) );

    test_tree<basic_test>::node_type* r =
        t.append_child(p, make_test(50, true));   // 50 > 9 -> reject

    const bool unchanged =
        ( (r == nullptr) &&
          (t.size() == before + 1) );

    return ( grew &&
             unchanged );
}


NS_END  // testing
NS_END  // djinterp
