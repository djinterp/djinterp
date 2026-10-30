// djinterp
#include "timer_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_TM_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_TM_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("timer.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("timer_tests.pdf");

    rb.module("timer",
              "Wall-clock execution timer");

    // constructors
    D_TM_RUN(tests_timer_default_ctor);
    D_TM_RUN(tests_timer_max_ctor);
    D_TM_RUN(tests_timer_zero_max_ctor);
    D_TM_RUN(tests_timer_ctor_custom_duration);
    D_TM_RUN(tests_timer_ns_lifecycle);
    D_TM_RUN(tests_timer_us_lifecycle);

    // operations
    D_TM_RUN(tests_timer_start);
    D_TM_RUN(tests_timer_start_while_running);
    D_TM_RUN(tests_timer_start_when_expired);
    D_TM_RUN(tests_timer_start_when_born_expired);
    D_TM_RUN(tests_timer_stop);
    D_TM_RUN(tests_timer_stop_when_not_running);
    D_TM_RUN(tests_timer_accumulate_cycles);
    D_TM_RUN(tests_timer_exclude_stopped_intervals);
    D_TM_RUN(tests_timer_reset_while_running);
    D_TM_RUN(tests_timer_reset_while_stopped);
    D_TM_RUN(tests_timer_reset_preserves_children);
    D_TM_RUN(tests_timer_reset_all_recurses);
    D_TM_RUN(tests_timer_reset_all_leaf);
    D_TM_RUN(tests_timer_reset_all_preserves_observed);

    // accessors
    D_TM_RUN(tests_timer_elapsed_running);
    D_TM_RUN(tests_timer_elapsed_stopped);
    D_TM_RUN(tests_timer_elapsed_running_with_accumulation);
    D_TM_RUN(tests_timer_max_value);
    D_TM_RUN(tests_timer_has_max_flag);
    D_TM_RUN(tests_timer_running_flag);
    D_TM_RUN(tests_timer_expired_without_max);
    D_TM_RUN(tests_timer_expired_below_limit);
    D_TM_RUN(tests_timer_expired_at_limit);
    D_TM_RUN(tests_timer_expired_above_limit);
    D_TM_RUN(tests_timer_expired_live);
    D_TM_RUN(tests_timer_remaining_without_max);
    D_TM_RUN(tests_timer_remaining_before_start);
    D_TM_RUN(tests_timer_remaining_partial);
    D_TM_RUN(tests_timer_remaining_at_limit);
    D_TM_RUN(tests_timer_remaining_past_limit);
    D_TM_RUN(tests_timer_const_accessors);
    D_TM_RUN(tests_timer_offset_independence);

    // children (owning)
    D_TM_RUN(tests_timer_add_child);
    D_TM_RUN(tests_timer_add_child_with_max);
    D_TM_RUN(tests_timer_multiple_children);
    D_TM_RUN(tests_timer_child_access);
    D_TM_RUN(tests_timer_child_access_const);
    D_TM_RUN(tests_timer_child_count);
    D_TM_RUN(tests_timer_children_independent);
    D_TM_RUN(tests_timer_copy_deep_copies_children);

    // children (non-owning / observed)
    D_TM_RUN(tests_timer_observe);
    D_TM_RUN(tests_timer_observe_multiple);
    D_TM_RUN(tests_timer_observed_out_of_range);
    D_TM_RUN(tests_timer_observed_live_view);
    D_TM_RUN(tests_timer_observed_count);
    D_TM_RUN(tests_timer_observe_non_owning);

    return rb.finish();
}

#undef D_TM_RUN
