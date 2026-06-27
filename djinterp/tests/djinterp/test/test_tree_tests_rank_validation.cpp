// djinterp
#include "test_tree_tests.hpp"


NS_DJINTERP
NS_TESTING


// tree_no_rank
//   alias: a test_tree with rank validation compiled out (_ValidateRank ==
// false), otherwise identical to the default instantiation.
using tree_no_rank =
    test_tree<basic_test,
              nary_tree<basic_test>,
              std::vector<test_kind>,
              false>;


/*
tests_tree_validate_rank_flag_true
  Verifies the default instantiation exposes validate_rank == true.
  Tests the following:
  - the static constexpr validate_rank flag (default)
*/
bool
tests_tree_validate_rank_flag_true()
{
    static_assert(test_tree<basic_test>::validate_rank == true,
                  "default _ValidateRank should be true");

    return (test_tree<basic_test>::validate_rank == true);
}

/*
tests_tree_validate_rank_flag_false
  Verifies the opt-out instantiation exposes validate_rank == false.
  Tests the following:
  - the static constexpr validate_rank flag (false variant)
*/
bool
tests_tree_validate_rank_flag_false()
{
    static_assert(tree_no_rank::validate_rank == false,
                  "_ValidateRank should be false for the opt-out variant");

    return (tree_no_rank::validate_rank == false);
}

/*
tests_tree_rank_disabled_accepts_any_child
  Verifies that with validation off, append_child accepts a child regardless
  of rank (the rank dispatch returns true unconditionally).
  Tests the following:
  - rank_admits_dispatch(..., std::false_type) path
  - a far higher-ranked child is admitted
*/
bool
tests_tree_rank_disabled_accepts_any_child()
{
    tree_no_rank t;

    tree_no_rank::node_type* p =
        t.add_root(make_test(1, true));            // parent rank 1
    tree_no_rank::node_type* c =
        t.append_child(p, make_test(999, true));   // rank 999 >> 1, accepted

    return ( (c != nullptr) &&
             (t.size() == 3) );                    // root + parent + child
}

/*
tests_tree_rank_disabled_leaf_parent_accepts
  Verifies that with validation off, even a registered leaf parent admits a
  child (the leaf rule is compiled out along with the rank rule).
  Tests the following:
  - leaf-kind parent admits a child when _ValidateRank is false
*/
bool
tests_tree_rank_disabled_leaf_parent_accepts()
{
    std::vector<test_kind> ks;
    ks.push_back(make_leaf_kind(5, 1));   // id 5 registered as a leaf

    tree_no_rank t(std::move(ks));

    tree_no_rank::node_type* p =
        t.add_root(make_test(5, true));           // leaf-kind parent
    tree_no_rank::node_type* c =
        t.append_child(p, make_test(1, true));    // accepted: checks disabled

    return (c != nullptr);
}

/*
tests_tree_rank_disabled_null_parent_still_null
  Verifies the null-parent guard runs BEFORE the rank dispatch, so a null
  parent is rejected even with validation disabled.
  Tests the following:
  - append_child(nullptr, ...) returns nullptr regardless of _ValidateRank
*/
bool
tests_tree_rank_disabled_null_parent_still_null()
{
    tree_no_rank t;

    tree_no_rank::node_type* c =
        t.append_child(nullptr, make_test(1, true));

    return ( (c == nullptr) &&
             (t.size() == 0) );
}


NS_END  // testing
NS_END  // djinterp
