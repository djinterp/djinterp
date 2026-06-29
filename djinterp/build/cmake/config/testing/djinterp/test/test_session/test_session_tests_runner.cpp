// djinterp
#include "test_session_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_TS_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_TS_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("test_session.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("test_session_tests.pdf");

    rb.module("test_session",
              "Top-level run context: tree, counters, timer, and lifecycle");

    // I.  construction, type aliases, initial state
    D_TS_RUN(tests_session_default_state);
    D_TS_RUN(tests_session_default_counters_zero);
    D_TS_RUN(tests_session_default_tree_and_timer);
    D_TS_RUN(tests_session_type_aliases);

    // II.  accessors
    D_TS_RUN(tests_session_tree_accessor_mutable_and_const);
    D_TS_RUN(tests_session_counter_accessors_mutable_and_const);
    D_TS_RUN(tests_session_total_sums_counters);
    D_TS_RUN(tests_session_timer_accessor_mutable_and_const);
    D_TS_RUN(tests_session_elapsed_initial_zero);
    D_TS_RUN(tests_session_save_load_are_noops);

    // III.  state machine
    D_TS_RUN(tests_session_run_transitions_and_noops);
    D_TS_RUN(tests_session_pause_transitions_and_noops);
    D_TS_RUN(tests_session_resume_transitions_and_noops);
    D_TS_RUN(tests_session_finish_transitions_and_noops);
    D_TS_RUN(tests_session_reset_clears_state);
    D_TS_RUN(tests_session_lifecycle_sequence);
    D_TS_RUN(tests_session_timer_tracks_state);

    // IV.  handler-driven run
    D_TS_RUN(tests_session_run_handler_empty_tree);
    D_TS_RUN(tests_session_run_handler_all_statuses);
    D_TS_RUN(tests_session_run_handler_all_pass_pending_root);
    D_TS_RUN(tests_session_run_handler_and_tree_prebuilt_passed);
    D_TS_RUN(tests_session_run_handler_not_idle_skips_walk);
    D_TS_RUN(tests_session_run_handler_and_tree_not_idle_no_move);

    // V.  verdict decision logic
    D_TS_RUN(tests_session_current_verdict_empty);
    D_TS_RUN(tests_session_current_verdict_failed_on_failed);
    D_TS_RUN(tests_session_current_verdict_failed_on_error);
    D_TS_RUN(tests_session_current_verdict_pending);
    D_TS_RUN(tests_session_current_verdict_passed);

    return rb.finish();
}

#undef D_TS_RUN
