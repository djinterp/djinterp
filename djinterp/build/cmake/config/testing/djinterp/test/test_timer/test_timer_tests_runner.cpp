// djinterp
#include "test_timer_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_TT_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_TT_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("test_timer.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("test_timer_tests.pdf");

    rb.module("test_timer",
              "Event-emitting test timer");

    // construction
    D_TT_RUN(tests_test_timer_default_ctor);
    D_TT_RUN(tests_test_timer_max_ctor);
    D_TT_RUN(tests_test_timer_max_handler_ctor);
    D_TT_RUN(tests_test_timer_handler_ctor);

    // operations
    D_TT_RUN(tests_test_timer_start_fires_on_start);
    D_TT_RUN(tests_test_timer_start_twice_one_event);
    D_TT_RUN(tests_test_timer_start_when_expired_noop);
    D_TT_RUN(tests_test_timer_stop_fires_on_stop);
    D_TT_RUN(tests_test_timer_stop_fires_on_expire);
    D_TT_RUN(tests_test_timer_stop_not_running_noop);
    D_TT_RUN(tests_test_timer_reset_fires_on_reset);
    D_TT_RUN(tests_test_timer_reset_all_leaf);
    D_TT_RUN(tests_test_timer_reset_all_recurses);
    D_TT_RUN(tests_test_timer_multiple_cycles_accumulate);

    // accessors
    D_TT_RUN(tests_test_timer_elapsed);
    D_TT_RUN(tests_test_timer_max);
    D_TT_RUN(tests_test_timer_has_max);
    D_TT_RUN(tests_test_timer_running);
    D_TT_RUN(tests_test_timer_expired);
    D_TT_RUN(tests_test_timer_remaining);

    // children (owning)
    D_TT_RUN(tests_test_timer_add_child);
    D_TT_RUN(tests_test_timer_add_child_with_max);
    D_TT_RUN(tests_test_timer_multiple_children);
    D_TT_RUN(tests_test_timer_child_access);
    D_TT_RUN(tests_test_timer_child_access_const);
    D_TT_RUN(tests_test_timer_child_count);
    D_TT_RUN(tests_test_timer_children_independent);
    D_TT_RUN(tests_test_timer_copy_deep_copies_children);

    // children (non-owning / observed)
    D_TT_RUN(tests_test_timer_observe);
    D_TT_RUN(tests_test_timer_observe_multiple);
    D_TT_RUN(tests_test_timer_observed_out_of_range);
    D_TT_RUN(tests_test_timer_observed_live_view);
    D_TT_RUN(tests_test_timer_observed_count);
    D_TT_RUN(tests_test_timer_observe_non_owning);

    // events
    D_TT_RUN(tests_test_timer_event_tag_names);
    D_TT_RUN(tests_test_timer_event_tag_args);
    D_TT_RUN(tests_test_timer_no_handler_noop);
    D_TT_RUN(tests_test_timer_handler_accessor);
    D_TT_RUN(tests_test_timer_set_handler_attach_detach);
    D_TT_RUN(tests_test_timer_set_handler_swaps);

    return rb.finish();
}

#undef D_TT_RUN
