// djinterp
#include "counter_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_CTR_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_CTR_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("counter.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("counter_tests.pdf");

    rb.module("counter",
              "Bounded counter with owned and observed children");

    // constructors
    D_CTR_RUN(tests_counter_default_ctor);
    D_CTR_RUN(tests_counter_value_ctor);
    D_CTR_RUN(tests_counter_value_ctor_default_bounds);
    D_CTR_RUN(tests_counter_value_ctor_min_only);
    D_CTR_RUN(tests_counter_negative_bounds);

    // operations
    D_CTR_RUN(tests_counter_increment);
    D_CTR_RUN(tests_counter_increment_amount);
    D_CTR_RUN(tests_counter_increment_to_max);
    D_CTR_RUN(tests_counter_increment_past_max);
    D_CTR_RUN(tests_counter_decrement);
    D_CTR_RUN(tests_counter_decrement_amount);
    D_CTR_RUN(tests_counter_decrement_to_min);
    D_CTR_RUN(tests_counter_decrement_past_min);
    D_CTR_RUN(tests_counter_increment_decrement_sequence);
    D_CTR_RUN(tests_counter_reset);
    D_CTR_RUN(tests_counter_reset_nonzero_initial);
    D_CTR_RUN(tests_counter_reset_preserves_children);
    D_CTR_RUN(tests_counter_reset_all_recurses);
    D_CTR_RUN(tests_counter_reset_all_leaf);
    D_CTR_RUN(tests_counter_reset_all_preserves_observed);

    // accessors
    D_CTR_RUN(tests_counter_value);
    D_CTR_RUN(tests_counter_initial);
    D_CTR_RUN(tests_counter_min);
    D_CTR_RUN(tests_counter_max);
    D_CTR_RUN(tests_counter_at_min_equal);
    D_CTR_RUN(tests_counter_at_min_below);
    D_CTR_RUN(tests_counter_at_min_above);
    D_CTR_RUN(tests_counter_at_max_equal);
    D_CTR_RUN(tests_counter_at_max_above);
    D_CTR_RUN(tests_counter_at_max_below);
    D_CTR_RUN(tests_counter_const_accessors);
    D_CTR_RUN(tests_counter_default_bounds_extremes);

    // children (owning)
    D_CTR_RUN(tests_counter_add_child);
    D_CTR_RUN(tests_counter_add_child_with_bounds);
    D_CTR_RUN(tests_counter_multiple_children);
    D_CTR_RUN(tests_counter_child_access);
    D_CTR_RUN(tests_counter_child_access_const);
    D_CTR_RUN(tests_counter_child_count);
    D_CTR_RUN(tests_counter_children_independent);
    D_CTR_RUN(tests_counter_copy_deep_copies_children);

    // children (non-owning / observed)
    D_CTR_RUN(tests_counter_observe);
    D_CTR_RUN(tests_counter_observe_multiple);
    D_CTR_RUN(tests_counter_observed_out_of_range);
    D_CTR_RUN(tests_counter_observed_live_view);
    D_CTR_RUN(tests_counter_observed_count);
    D_CTR_RUN(tests_counter_observe_non_owning);

    return rb.finish();
}

#undef D_CTR_RUN
