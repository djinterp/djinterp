// djinterp
#include "test_tree_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_tree_count_by_status_empty_all_zero
  Verifies every count is zero on an empty forest (the walk visits nothing).
  Tests the following:
  - count_passed / count_failed / count_skipped / count_pending == 0
  - count_by_status for an arbitrary status == 0
*/
bool
tests_tree_count_by_status_empty_all_zero()
{
    test_tree<basic_test> t;

    return ( (t.count_passed()  == 0) &&
             (t.count_failed()  == 0) &&
             (t.count_skipped() == 0) &&
             (t.count_pending() == 0) &&
             (t.count_by_status(test_status::error) == 0) );
}

/*
tests_tree_count_by_status_int_and_enum_agree
  Verifies count_by_status compares numerically, so the enum, a plain int, and
  the element's own status_type all yield the same count.
  Tests the following:
  - count_by_status<test_status>, <int>, and <status_type> agree for passed
  - the pending conjunctive root is counted for pending (enum and int)
*/
bool
tests_tree_count_by_status_int_and_enum_agree()
{
    test_tree<basic_test> t;
    t.add_root(make_status_test(1, test_status::passed));

    // passed: one root child; the conjunctive root is pending, not passed
    const bool passed_ok =
        ( (t.count_by_status(test_status::passed)         == 1) &&
          (t.count_by_status(0)                           == 1) &&
          (t.count_by_status(basic_test::status_passed)   == 1) );

    // pending: only the conjunctive root
    const bool pending_ok =
        ( (t.count_by_status(test_status::pending) == 1) &&
          (t.count_by_status(3)                    == 1) );

    return ( passed_ok &&
             pending_ok );
}

/*
tests_tree_count_passed_failed_skipped_pending
  Verifies each per-status count over a forest holding one node of every
  status, remembering the conjunctive root contributes one extra pending.
  Tests the following:
  - count_passed / failed / skipped == 1 each
  - count_pending == 2 (one explicit pending child + the conjunctive root)
  - count_by_status(error) == 1
  - size() == 6 (five roots + conjunctive root)
*/
bool
tests_tree_count_passed_failed_skipped_pending()
{
    test_tree<basic_test> t;
    t.add_root(make_status_test(1, test_status::passed));
    t.add_root(make_status_test(2, test_status::failed));
    t.add_root(make_status_test(3, test_status::skipped));
    t.add_root(make_status_test(4, test_status::pending));
    t.add_root(make_status_test(5, test_status::error));

    return ( (t.size()          == 6) &&
             (t.count_passed()  == 1) &&
             (t.count_failed()  == 1) &&
             (t.count_skipped() == 1) &&
             (t.count_pending() == 2) &&
             (t.count_by_status(test_status::error) == 1) );
}

/*
tests_tree_count_includes_conjunctive_root
  Isolates the conjunctive root's contribution to the counting surface.
  Tests the following:
  - before any add_root: count_pending == 0
  - after one passed root: count_pending == 1 (only the conjunctive root),
    count_passed == 1, and root() is itself pending
*/
bool
tests_tree_count_includes_conjunctive_root()
{
    test_tree<basic_test> t;

    const bool empty_ok = (t.count_pending() == 0);

    t.add_root(make_status_test(1, test_status::passed));

    const bool after_ok =
        ( (t.count_pending() == 1) &&
          (t.count_passed()  == 1) &&
          (t.root() != nullptr)    &&
          status_is(t.root()->data(), test_status::pending) );

    return ( empty_ok &&
             after_ok );
}

/*
tests_tree_all_passed_empty_true
  Verifies all_passed is vacuously true (and any_failed false) on an empty
  forest - no failed, pending, or error nodes exist.
  Tests the following:
  - all_passed() == true on the empty tree
  - any_failed() == false on the empty tree
*/
bool
tests_tree_all_passed_empty_true()
{
    test_tree<basic_test> t;

    return ( (t.all_passed() == true)  &&
             (t.any_failed() == false) &&
             (t.empty()      == true) );
}

/*
tests_tree_all_passed_false_pending_root
  Verifies that an add_root-grown forest of only-passing roots is NOT
  all_passed, because the conjunctive root remains pending.  This exercises
  the second condition of all_passed (pending count drives the result).
  Tests the following:
  - all_passed() == false while the conjunctive root is pending
  - any_failed() == false (no failed/error nodes)
*/
bool
tests_tree_all_passed_false_pending_root()
{
    test_tree<basic_test> t;
    t.add_root(make_status_test(1, test_status::passed));
    t.add_root(make_status_test(2, test_status::passed));

    return ( (t.all_passed() == false) &&
             (t.any_failed() == false) );
}

/*
tests_tree_all_passed_true_skipped_allowed
  Verifies all_passed is true on a forest with no failed/pending/error nodes -
  even when skipped nodes are present (skipped does not break all_passed).
  A pre-built forest with an EVALUATED (passed, non-pending) root is used so no
  pending conjunctive root interferes.
  Tests the following:
  - all_passed() == true with passed + skipped nodes and a non-pending root
  - any_failed() == false
  - count_skipped() == 1
*/
bool
tests_tree_all_passed_true_skipped_allowed()
{
    nary_tree<basic_test> f;
    f.emplace_root(make_status_test(0, test_status::passed));
    f.append_child(f.root(), make_status_test(1, test_status::passed));
    f.append_child(f.root(), make_status_test(2, test_status::skipped));

    test_tree<basic_test> t(std::vector<test_kind>(), std::move(f));

    return ( (t.all_passed()    == true)  &&
             (t.any_failed()    == false) &&
             (t.count_skipped() == 1) );
}

/*
tests_tree_failed_tree_all_passed_false_any_failed_true
  Verifies a forest containing a failed node fails all_passed (first
  condition) and trips any_failed.
  Tests the following:
  - all_passed() == false when a failed node is present
  - any_failed() == true when a failed node is present
*/
bool
tests_tree_failed_tree_all_passed_false_any_failed_true()
{
    test_tree<basic_test> t;
    t.add_root(make_status_test(1, test_status::failed));

    return ( (t.all_passed() == false) &&
             (t.any_failed() == true) );
}

/*
tests_tree_error_tree_all_passed_false_any_failed_true
  Verifies an error node fails all_passed via its THIRD condition (failed and
  pending both zero, error non-zero) and trips any_failed via its SECOND
  operand.  A pre-built forest with an evaluated root removes the pending
  conjunctive root so the error node is the deciding factor.
  Tests the following:
  - all_passed() == false driven solely by the error count
  - any_failed() == true driven by the error count
  - count_by_status(error) == 1
*/
bool
tests_tree_error_tree_all_passed_false_any_failed_true()
{
    nary_tree<basic_test> f;
    f.emplace_root(make_status_test(0, test_status::passed));   // non-pending
    f.append_child(f.root(), make_status_test(1, test_status::error));

    test_tree<basic_test> t(std::vector<test_kind>(), std::move(f));

    return ( (t.all_passed() == false) &&
             (t.any_failed() == true)  &&
             (t.count_by_status(test_status::error) == 1) );
}

/*
tests_tree_any_failed_false_when_none
  Verifies any_failed is false when neither failed nor error nodes exist, for
  both a populated (passed-only) forest and the empty forest.
  Tests the following:
  - any_failed() == false on a passed-only pre-built forest
  - any_failed() == false on the empty tree
*/
bool
tests_tree_any_failed_false_when_none()
{
    nary_tree<basic_test> f;
    f.emplace_root(make_status_test(0, test_status::passed));
    f.append_child(f.root(), make_status_test(1, test_status::passed));

    test_tree<basic_test> populated(std::vector<test_kind>(), std::move(f));

    test_tree<basic_test> empty;

    return ( (populated.any_failed() == false) &&
             (empty.any_failed()     == false) );
}


NS_END  // testing
NS_END  // djinterp
