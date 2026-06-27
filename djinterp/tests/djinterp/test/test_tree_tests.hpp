/******************************************************************************
* djinterp [test]                                          test_tree_tests.hpp
*
*   Unit-test declarations and shared helpers for test_tree.hpp.
*
*   Every test is a nullary `bool tests_*()` predicate (true == pass) living
* flat in djinterp::testing.  The declarations are grouped to mirror the
* sections of test_tree.hpp; each group is defined in its own .cpp section
* TU, and the runner (test_tree_tests_runner.cpp) links them together and
* calls each in turn.
*
*   CONJUNCTIVE-ROOT ACCOUNTING (read this once):
*   test_tree backs its forest with a single tree whose ROOT NODE is the
* implied conjunctive root.  That root is created on the FIRST add_root (as a
* default-constructed element, hence status `pending`, type id 0) and is an
* ORDINARY node in the walk - it is visited by begin()/end() and counted by
* size() and the run surface.  Consequently, for a forest grown with
* add_root:
*     - size()          == (number of roots) + 1
*     - count_pending() includes the (pending) conjunctive root
*     - all_passed()    is false while that root remains pending
* The counting tests bake this in deliberately; the pre-built-forest
* constructor is used where a non-pending root is required (e.g. to drive
* all_passed() true on a non-empty forest).
*
*
* path:      /tests/djinterp/test/test_tree_tests.hpp
* link(s):   TBA
* author(s): djinterp test-suite
******************************************************************************/

#ifndef DJINTERP_TEST_TREE_TESTS_
#define DJINTERP_TEST_TREE_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "test_tree.hpp"
#include "test_common.hpp"
#include "test_kind.hpp"
#include "test_object.hpp"


NS_DJINTERP
NS_TESTING


// ---------------------------------------------------------------------------
//  names under test (brought in from djinterp::test for brevity)
// ---------------------------------------------------------------------------

using ::djinterp::test::test_tree;
using ::djinterp::test::is_test_tree;
using ::djinterp::test::basic_test;
using ::djinterp::test::test_kind;
using ::djinterp::test::test_status;
using ::djinterp::test::test_type_id;
using ::djinterp::test::make_test;
using ::djinterp::test::make_test_kind;
using ::djinterp::nary_tree;


// ---------------------------------------------------------------------------
//  shared helpers
// ---------------------------------------------------------------------------

// make_status_test
//   helper: builds a basic_test of the given type id whose status is forced
// to _status.  For passed / failed the result flag is set consistently (via
// evaluate); the other states set only the status, matching how those nodes
// are produced in practice.
inline basic_test
make_status_test(
    test_type_id _id,
    test_status  _status
)
{
    basic_test t(_id);

    // drive the node into the requested status
    if (_status == test_status::passed)
    {
        t.evaluate(true);
    }
    else if (_status == test_status::failed)
    {
        t.evaluate(false);
    }
    else if (_status == test_status::skipped)
    {
        t.skip();
    }
    else if (_status == test_status::error)
    {
        t.set_status(basic_test::status_error);
    }
    else
    {
        // test_status::pending - already the default-constructed state, set
        // explicitly for clarity
        t.set_status(basic_test::status_pending);
    }

    return t;
}

// status_is
//   helper: true iff _t's status matches _expected, compared numerically -
// exactly the comparison test_tree's count_by_status performs internally.
inline bool
status_is(
    const basic_test& _t,
    test_status       _expected
)
{
    return ( static_cast<int>(_t.status()) ==
             static_cast<int>(_expected) );
}

// make_interior_kind
//   helper: a test_kind record describing an INTERIOR node of the given id
// and rank (is_leaf == false, no default options).
inline test_kind
make_interior_kind(
    test_type_id  _id,
    std::uint16_t _rank
)
{
    return make_test_kind(_id, "interior", _rank, false, nullptr);
}

// make_leaf_kind
//   helper: a test_kind record describing a LEAF node of the given id and
// rank (is_leaf == true, no default options).
inline test_kind
make_leaf_kind(
    test_type_id  _id,
    std::uint16_t _rank
)
{
    return make_test_kind(_id, "leaf", _rank, true, nullptr);
}


// ---------------------------------------------------------------------------
//  I.  construction, type aliases, and metadata access
//      (test_tree_tests_construction.cpp)
// ---------------------------------------------------------------------------

bool tests_tree_default_construct_is_empty();
bool tests_tree_construct_from_kinds();
bool tests_tree_construct_from_kinds_and_forest();
bool tests_tree_type_aliases();
bool tests_tree_kinds_accessor();
bool tests_tree_underlying_accessor();


// ---------------------------------------------------------------------------
//  II.  forwarded capacity / iteration / root / clear
//       (test_tree_tests_forwarding.cpp)
// ---------------------------------------------------------------------------

bool tests_tree_size_and_empty();
bool tests_tree_root_accessor();
bool tests_tree_begin_end_iteration();
bool tests_tree_const_begin_end_iteration();
bool tests_tree_clear();


// ---------------------------------------------------------------------------
//  III.  add_root (conjunctive root + unconstrained top-level insertion)
//        (test_tree_tests_add_root.cpp)
// ---------------------------------------------------------------------------

bool tests_tree_add_root_creates_conjunctive_root();
bool tests_tree_add_root_returns_child_node();
bool tests_tree_add_root_multiple_roots();
bool tests_tree_add_root_not_rank_checked();
bool tests_tree_add_root_conjunctive_root_is_pending();


// ---------------------------------------------------------------------------
//  IV.  append_child (rank-checked build surface, _ValidateRank == true)
//       (test_tree_tests_append_child.cpp)
// ---------------------------------------------------------------------------

bool tests_tree_append_child_null_parent_returns_null();
bool tests_tree_append_child_no_kinds_lower_rank_accepted();
bool tests_tree_append_child_no_kinds_equal_rank_accepted();
bool tests_tree_append_child_no_kinds_higher_rank_rejected();
bool tests_tree_append_child_registered_interior_accepts_within_rank();
bool tests_tree_append_child_registered_higher_rank_rejected();
bool tests_tree_append_child_registered_leaf_parent_rejects();
bool tests_tree_append_child_unregistered_parent_not_treated_as_leaf();
bool tests_tree_append_child_mixed_resolution();
bool tests_tree_append_child_returns_node_and_stores_value();
bool tests_tree_append_child_success_increments_size();


// ---------------------------------------------------------------------------
//  V.  rank-validation flag and the _ValidateRank == false path
//      (test_tree_tests_rank_validation.cpp)
// ---------------------------------------------------------------------------

bool tests_tree_validate_rank_flag_true();
bool tests_tree_validate_rank_flag_false();
bool tests_tree_rank_disabled_accepts_any_child();
bool tests_tree_rank_disabled_leaf_parent_accepts();
bool tests_tree_rank_disabled_null_parent_still_null();


// ---------------------------------------------------------------------------
//  VI.  run / counting surface
//       (test_tree_tests_counting.cpp)
// ---------------------------------------------------------------------------

bool tests_tree_count_by_status_empty_all_zero();
bool tests_tree_count_by_status_int_and_enum_agree();
bool tests_tree_count_passed_failed_skipped_pending();
bool tests_tree_count_includes_conjunctive_root();
bool tests_tree_all_passed_empty_true();
bool tests_tree_all_passed_false_pending_root();
bool tests_tree_all_passed_true_skipped_allowed();
bool tests_tree_failed_tree_all_passed_false_any_failed_true();
bool tests_tree_error_tree_all_passed_false_any_failed_true();
bool tests_tree_any_failed_false_when_none();


// ---------------------------------------------------------------------------
//  VII.  detection: is_test_tree trait, _v companion, and the concept
//        (test_tree_tests_detection.cpp)
// ---------------------------------------------------------------------------

bool tests_tree_is_test_tree_true_for_instantiation();
bool tests_tree_is_test_tree_false_for_non_tree();
bool tests_tree_is_test_tree_strips_cv_ref();
bool tests_tree_is_test_tree_value_alias();
bool tests_tree_test_tree_concept();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_TREE_TESTS_
