// djinterp
#include "bounded_container_traits_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"   // report_builder + report model


// D_BCT_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_BCT_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("bounded_container_traits.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("bounded_container_traits_tests.pdf");

    rb.module("bounded_container_traits",
              "The boundedness axis: capacity signals, domain signals, and "
              "the classifications built on them");

    // I. structural signals
    D_BCT_RUN(tests_signal_fixed_extent);
    D_BCT_RUN(tests_signal_fixed_extent_shapes);
    D_BCT_RUN(tests_signal_tuple_size);
    D_BCT_RUN(tests_signal_static_bounds);
    D_BCT_RUN(tests_signal_capacity);
    D_BCT_RUN(tests_signal_reserve);
    D_BCT_RUN(tests_signal_max_size);
    D_BCT_RUN(tests_signal_size);
    D_BCT_RUN(tests_signal_value_companions);

    // II. capacity classification
    D_BCT_RUN(tests_bounded_extent_disjunct);
    D_BCT_RUN(tests_bounded_tuple_size_disjunct);
    D_BCT_RUN(tests_bounded_static_bounds_disjunct);
    D_BCT_RUN(tests_bounded_capacity_disjunct);
    D_BCT_RUN(tests_bounded_negative);
    D_BCT_RUN(tests_bounded_ignores_size);
    D_BCT_RUN(tests_bounded_first_signal_wins);
    D_BCT_RUN(tests_unbounded_truth_table);
    D_BCT_RUN(tests_bounded_unbounded_exclusive);
    D_BCT_RUN(tests_capacity_value_companions);

    // III. domain classification
    D_BCT_RUN(tests_domain_bounded);
    D_BCT_RUN(tests_domain_implies_bounded);
    D_BCT_RUN(tests_domain_orthogonal);
    D_BCT_RUN(tests_domain_value_companion);

    // IV. capacity-bound summary
    D_BCT_RUN(tests_capacity_bound_enumerators);
    D_BCT_RUN(tests_capacity_bound_name);
    D_BCT_RUN(tests_capacity_bound_name_out_of_range);
    D_BCT_RUN(tests_capacity_bound_of);
    D_BCT_RUN(tests_capacity_bound_of_type);
    D_BCT_RUN(tests_capacity_bound_of_agrees);
    D_BCT_RUN(tests_capacity_bound_of_value_companion);

    // V. aggregate snapshot
    D_BCT_RUN(tests_class_signals);
    D_BCT_RUN(tests_class_verdicts);
    D_BCT_RUN(tests_class_summary);
    D_BCT_RUN(tests_class_agrees_with_traits);
    D_BCT_RUN(tests_class_is_constexpr);

    // VI. robustness  (cv-ref, hostile types, incomplete types, trait shape)
    D_BCT_RUN(tests_cvref_signals);
    D_BCT_RUN(tests_cvref_verdicts);
    D_BCT_RUN(tests_hostile_types_rejected);
    D_BCT_RUN(tests_hostile_types_survived);
    D_BCT_RUN(tests_incomplete_type);
    D_BCT_RUN(tests_trait_shape);

    // VII. standard library
    D_BCT_RUN(tests_stdlib_array);
    D_BCT_RUN(tests_stdlib_vector);
    D_BCT_RUN(tests_stdlib_node_containers);
    D_BCT_RUN(tests_stdlib_string);
    D_BCT_RUN(tests_stdlib_tuple_pair);
    D_BCT_RUN(tests_stdlib_associative);

    return rb.finish();
}

#undef D_BCT_RUN
