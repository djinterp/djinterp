// djinterp
#include "test_tree_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_TT_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_TT_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("test_tree.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("test_tree_tests.pdf");

    rb.module("test_tree",
              "Default test container: kinds paired with a node forest");

    // I.  construction, type aliases, and access
    D_TT_RUN(tests_tree_default_construct_is_empty);
    D_TT_RUN(tests_tree_construct_from_kinds);
    D_TT_RUN(tests_tree_construct_from_kinds_and_forest);
    D_TT_RUN(tests_tree_type_aliases);
    D_TT_RUN(tests_tree_kinds_accessor);
    D_TT_RUN(tests_tree_underlying_accessor);

    // II.  forwarded capacity / iteration / root / clear
    D_TT_RUN(tests_tree_size_and_empty);
    D_TT_RUN(tests_tree_root_accessor);
    D_TT_RUN(tests_tree_begin_end_iteration);
    D_TT_RUN(tests_tree_const_begin_end_iteration);
    D_TT_RUN(tests_tree_clear);

    // III.  add_root
    D_TT_RUN(tests_tree_add_root_creates_conjunctive_root);
    D_TT_RUN(tests_tree_add_root_returns_child_node);
    D_TT_RUN(tests_tree_add_root_multiple_roots);
    D_TT_RUN(tests_tree_add_root_not_rank_checked);
    D_TT_RUN(tests_tree_add_root_conjunctive_root_is_pending);

    // IV.  append_child (rank-checked, _ValidateRank == true)
    D_TT_RUN(tests_tree_append_child_null_parent_returns_null);
    D_TT_RUN(tests_tree_append_child_no_kinds_lower_rank_accepted);
    D_TT_RUN(tests_tree_append_child_no_kinds_equal_rank_accepted);
    D_TT_RUN(tests_tree_append_child_no_kinds_higher_rank_rejected);
    D_TT_RUN(tests_tree_append_child_registered_interior_accepts_within_rank);
    D_TT_RUN(tests_tree_append_child_registered_higher_rank_rejected);
    D_TT_RUN(tests_tree_append_child_registered_leaf_parent_rejects);
    D_TT_RUN(tests_tree_append_child_unregistered_parent_not_treated_as_leaf);
    D_TT_RUN(tests_tree_append_child_mixed_resolution);
    D_TT_RUN(tests_tree_append_child_returns_node_and_stores_value);
    D_TT_RUN(tests_tree_append_child_success_increments_size);

    // V.  rank-validation flag and the _ValidateRank == false path
    D_TT_RUN(tests_tree_validate_rank_flag_true);
    D_TT_RUN(tests_tree_validate_rank_flag_false);
    D_TT_RUN(tests_tree_rank_disabled_accepts_any_child);
    D_TT_RUN(tests_tree_rank_disabled_leaf_parent_accepts);
    D_TT_RUN(tests_tree_rank_disabled_null_parent_still_null);

    // VI.  run / counting surface
    D_TT_RUN(tests_tree_count_by_status_empty_all_zero);
    D_TT_RUN(tests_tree_count_by_status_int_and_enum_agree);
    D_TT_RUN(tests_tree_count_passed_failed_skipped_pending);
    D_TT_RUN(tests_tree_count_includes_conjunctive_root);
    D_TT_RUN(tests_tree_all_passed_empty_true);
    D_TT_RUN(tests_tree_all_passed_false_pending_root);
    D_TT_RUN(tests_tree_all_passed_true_skipped_allowed);
    D_TT_RUN(tests_tree_failed_tree_all_passed_false_any_failed_true);
    D_TT_RUN(tests_tree_error_tree_all_passed_false_any_failed_true);
    D_TT_RUN(tests_tree_any_failed_false_when_none);

    // VII.  detection: is_test_tree trait, _v companion, concept
    D_TT_RUN(tests_tree_is_test_tree_true_for_instantiation);
    D_TT_RUN(tests_tree_is_test_tree_false_for_non_tree);
    D_TT_RUN(tests_tree_is_test_tree_strips_cv_ref);
    D_TT_RUN(tests_tree_is_test_tree_value_alias);
    D_TT_RUN(tests_tree_test_tree_concept);

    return rb.finish();
}

#undef D_TT_RUN
