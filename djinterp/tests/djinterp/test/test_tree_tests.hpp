/*******************************************************************************
* djinterp [test]                                            test_tree_tests.hpp
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
* author(s): djinterp test-suite                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TEST_TREE_TESTS_
#define DJINTERP_TEST_TREE_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
// djinterp  -- framework header first, then mode-gated.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../inc/djinterp/test/test_tree.hpp"
#include "../../../inc/djinterp/test/test_common.hpp"
#include "../../../inc/djinterp/test/test_kind.hpp"
#include "../../../inc/djinterp/test/test_object.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // spec mode: module_spec + run_module
#endif


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test); the tests live here in
// djinterp::testing.  (Spec mode also uses it for dt::module_spec / run_module.)
namespace dt = ::djinterp::test;


#ifndef DTEST_SPEC_MODE  // fixtures + brought-in names: normal (section-file) mode only


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


#endif  // !DTEST_SPEC_MODE  (fixtures)


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


#ifdef DTEST_SPEC_MODE

// =========================================================================
//  suite spec provider (spec mode)
//    Descriptors are drawn from each test's own /* Verifies ... */ block.
// =========================================================================

inline dt::module_spec
tree_spec()
{
    return dt::module_spec{
        "test_tree",
        "test_tree.hpp - the default test container: a six-kind set paired with "
        "a node forest whose implied conjunctive root anchors every top-level "
        "root.  Construction, the forwarded capacity / iteration surface, "
        "add_root, rank-checked append_child, the rank-validation opt-out, the "
        "counting surface, and the is_test_tree detection trait.",
        {
            dt::block_spec{
                "construction",
                "Construction, type aliases, and access to the kind set / backing.",
                {
                    { "tests_tree_default_construct_is_empty",      "the default constructor yields an empty tree - no nodes, empty kind set, null conjunctive root", &tests_tree_default_construct_is_empty },
                    { "tests_tree_construct_from_kinds",            "the kind-container constructor stores the kinds and they govern rank-checked insertion", &tests_tree_construct_from_kinds },
                    { "tests_tree_construct_from_kinds_and_forest", "the (kinds, forest) constructor adopts a pre-built backing forest wholesale", &tests_tree_construct_from_kinds_and_forest },
                    { "tests_tree_type_aliases",                    "the public type aliases resolve to the expected types for the default backing / kind container", &tests_tree_type_aliases },
                    { "tests_tree_kinds_accessor",                  "both overloads of kinds() refer to the one stored container", &tests_tree_kinds_accessor },
                    { "tests_tree_underlying_accessor",             "both overloads of underlying() expose the backing forest", &tests_tree_underlying_accessor },
                }
            },
            dt::block_spec{
                "forwarding",
                "The forwarded capacity / iteration / root / clear surface.",
                {
                    { "tests_tree_size_and_empty",             "the forwarded capacity surface tracks the backing forest, counting the conjunctive root once it exists", &tests_tree_size_and_empty },
                    { "tests_tree_root_accessor",              "root() returns the implied conjunctive root once created, through both overloads", &tests_tree_root_accessor },
                    { "tests_tree_begin_end_iteration",        "the non-const walk visits exactly the backing nodes - the conjunctive root and every root beneath it", &tests_tree_begin_end_iteration },
                    { "tests_tree_const_begin_end_iteration",  "the const walk visits the same nodes via the const overloads", &tests_tree_const_begin_end_iteration },
                    { "tests_tree_clear",                      "clear() empties the forest (including the conjunctive root) and the tree is reusable afterwards", &tests_tree_clear },
                }
            },
            dt::block_spec{
                "add_root",
                "add_root: the lazily-created conjunctive root and unconstrained "
                "top-level insertion.",
                {
                    { "tests_tree_add_root_creates_conjunctive_root",  "the first add_root lazily creates the implied conjunctive root and attaches the new root beneath it", &tests_tree_add_root_creates_conjunctive_root },
                    { "tests_tree_add_root_returns_child_node",        "add_root returns the newly inserted top-level root carrying the element, not the conjunctive root", &tests_tree_add_root_returns_child_node },
                    { "tests_tree_add_root_multiple_roots",            "many roots all attach beneath the single conjunctive root", &tests_tree_add_root_multiple_roots },
                    { "tests_tree_add_root_not_rank_checked",          "add_root is unconstrained - the conjunctive root admits any sequence, including leaf-kind and non-monotonic ranks", &tests_tree_add_root_not_rank_checked },
                    { "tests_tree_add_root_conjunctive_root_is_pending","the conjunctive root is a default-constructed element - type id 0 and status pending", &tests_tree_add_root_conjunctive_root_is_pending },
                }
            },
            dt::block_spec{
                "append_child",
                "The rank-checked build surface (validate_rank == true): the "
                "null-parent guard, the rank rule, the leaf rule, and their "
                "interaction with the kind set.",
                {
                    { "tests_tree_append_child_null_parent_returns_null",             "append_child rejects a null parent and does not fabricate a conjunctive root", &tests_tree_append_child_null_parent_returns_null },
                    { "tests_tree_append_child_no_kinds_lower_rank_accepted",         "with no kinds registered, a lower-ranked child (by id-as-rank) is admitted", &tests_tree_append_child_no_kinds_lower_rank_accepted },
                    { "tests_tree_append_child_no_kinds_equal_rank_accepted",         "the rank rule is '<=', so an equal-ranked child is admitted", &tests_tree_append_child_no_kinds_equal_rank_accepted },
                    { "tests_tree_append_child_no_kinds_higher_rank_rejected",        "a strictly higher-ranked child is rejected under the id-as-rank fallback", &tests_tree_append_child_no_kinds_higher_rank_rejected },
                    { "tests_tree_append_child_registered_interior_accepts_within_rank","a registered interior parent admits a child within its rank", &tests_tree_append_child_registered_interior_accepts_within_rank },
                    { "tests_tree_append_child_registered_higher_rank_rejected",      "rank monotonicity through the kind set - a higher-ranked child is rejected even under an interior parent", &tests_tree_append_child_registered_higher_rank_rejected },
                    { "tests_tree_append_child_registered_leaf_parent_rejects",       "the leaf rule takes precedence over rank - a registered leaf parent admits no children", &tests_tree_append_child_registered_leaf_parent_rejects },
                    { "tests_tree_append_child_unregistered_parent_not_treated_as_leaf","the leaf check uses find_kind, so an unregistered parent is not treated as a leaf - admission falls through to rank", &tests_tree_append_child_unregistered_parent_not_treated_as_leaf },
                    { "tests_tree_append_child_mixed_resolution",                     "rank resolution mixes registered and unregistered ids in both directions (registered rank vs id-as-rank)", &tests_tree_append_child_mixed_resolution },
                    { "tests_tree_append_child_returns_node_and_stores_value",        "a successful append returns the new node carrying the supplied element", &tests_tree_append_child_returns_node_and_stores_value },
                    { "tests_tree_append_child_success_increments_size",             "an accepted insert grows the forest by one; a rejected insert leaves it unchanged", &tests_tree_append_child_success_increments_size },
                }
            },
            dt::block_spec{
                "rank_validation",
                "The validate_rank flag and the validation-off (_ValidateRank == "
                "false) path.",
                {
                    { "tests_tree_validate_rank_flag_true",          "the default instantiation exposes validate_rank == true", &tests_tree_validate_rank_flag_true },
                    { "tests_tree_validate_rank_flag_false",         "the opt-out instantiation exposes validate_rank == false", &tests_tree_validate_rank_flag_false },
                    { "tests_tree_rank_disabled_accepts_any_child",  "with validation off, append_child accepts a child regardless of rank", &tests_tree_rank_disabled_accepts_any_child },
                    { "tests_tree_rank_disabled_leaf_parent_accepts","with validation off, even a registered leaf parent admits a child (the leaf rule is compiled out)", &tests_tree_rank_disabled_leaf_parent_accepts },
                    { "tests_tree_rank_disabled_null_parent_still_null","the null-parent guard runs before the rank dispatch, so a null parent is rejected even with validation off", &tests_tree_rank_disabled_null_parent_still_null },
                }
            },
            dt::block_spec{
                "counting",
                "The run / counting surface: count_by_status, all_passed, and "
                "any_failed, remembering the conjunctive root's pending "
                "contribution.",
                {
                    { "tests_tree_count_by_status_empty_all_zero",                 "every count is zero on an empty forest (the walk visits nothing)", &tests_tree_count_by_status_empty_all_zero },
                    { "tests_tree_count_by_status_int_and_enum_agree",             "count_by_status compares numerically, so the enum, a plain int, and the element's status_type agree", &tests_tree_count_by_status_int_and_enum_agree },
                    { "tests_tree_count_passed_failed_skipped_pending",            "each per-status count over a forest with one node of every status (plus the pending conjunctive root)", &tests_tree_count_passed_failed_skipped_pending },
                    { "tests_tree_count_includes_conjunctive_root",                "the counts include the implied conjunctive root", &tests_tree_count_includes_conjunctive_root },
                    { "tests_tree_all_passed_empty_true",                          "all_passed is vacuously true (and any_failed false) on an empty forest", &tests_tree_all_passed_empty_true },
                    { "tests_tree_all_passed_false_pending_root",                  "an add_root-grown forest of only-passing roots is not all_passed, because the conjunctive root remains pending", &tests_tree_all_passed_false_pending_root },
                    { "tests_tree_all_passed_true_skipped_allowed",                "all_passed is true with no failed/pending/error nodes, even when skipped nodes are present", &tests_tree_all_passed_true_skipped_allowed },
                    { "tests_tree_failed_tree_all_passed_false_any_failed_true",   "a forest with a failed node fails all_passed and trips any_failed", &tests_tree_failed_tree_all_passed_false_any_failed_true },
                    { "tests_tree_error_tree_all_passed_false_any_failed_true",    "an error node fails all_passed via its error condition and trips any_failed (with a pre-built non-pending root)", &tests_tree_error_tree_all_passed_false_any_failed_true },
                    { "tests_tree_any_failed_false_when_none",                     "any_failed is false when neither failed nor error nodes exist, for both a passed-only forest and the empty forest", &tests_tree_any_failed_false_when_none },
                }
            },
            dt::block_spec{
                "detection",
                "The is_test_tree trait, its _v companion, and the concept.",
                {
                    { "tests_tree_is_test_tree_true_for_instantiation", "is_test_tree recognises instantiations of the class template, including a rank-disabled one", &tests_tree_is_test_tree_true_for_instantiation },
                    { "tests_tree_is_test_tree_false_for_non_tree",     "is_test_tree rejects unrelated types - scalars, the element type, the kind container, and the bare backing", &tests_tree_is_test_tree_false_for_non_tree },
                    { "tests_tree_is_test_tree_strips_cv_ref",          "is_test_tree applies clean_t, so cv- and reference-qualified forms agree with the bare type", &tests_tree_is_test_tree_strips_cv_ref },
                    { "tests_tree_is_test_tree_value_alias",            "the is_test_tree_v variable-template companion mirrors the trait", &tests_tree_is_test_tree_value_alias },
                    { "tests_tree_test_tree_concept",                  "the test_tree_type concept mirrors is_test_tree, where concepts are available", &tests_tree_test_tree_concept },
                }
            },
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_TREE_TESTS_
