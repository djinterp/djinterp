// djinterp
#include "constexpr_container_traits_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"   // report_builder + report model


// D_CCT_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_CCT_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("constexpr_container_traits.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("constexpr_container_traits_tests.pdf");

    rb.module("constexpr_container_traits",
              "The compile-time end of the Lifetime axis: the five "
              "compile-time-size signals, the lattice meet with lifetime_of, "
              "and the classifications built on them");

    // I. compile-time-expressible-size signals
    D_CCT_RUN(tests_tag_basic);
    D_CCT_RUN(tests_tag_access);
    D_CCT_RUN(tests_extent_basic);
    D_CCT_RUN(tests_extent_shapes);
    D_CCT_RUN(tests_size_expression_basic);
    D_CCT_RUN(tests_size_expression_constness);
    D_CCT_RUN(tests_size_expression_return_value);
    D_CCT_RUN(tests_size_expression_constructibility);
    D_CCT_RUN(tests_size_expression_destructor);
    D_CCT_RUN(tests_tuple_size);
    D_CCT_RUN(tests_constexpr_iteration);
    D_CCT_RUN(tests_signal_value_companions);

    // II. lifetime of a container
    D_CCT_RUN(tests_size_lifetime_disjuncts);
    D_CCT_RUN(tests_size_lifetime_negative);
    D_CCT_RUN(tests_size_lifetime_is_binary);
    D_CCT_RUN(tests_size_lifetime_opt_out_ignored);
    D_CCT_RUN(tests_container_lifetime_meet);
    D_CCT_RUN(tests_container_lifetime_lattice);
    D_CCT_RUN(tests_container_lifetime_tag_bypass);
    D_CCT_RUN(tests_container_lifetime_type);
    D_CCT_RUN(tests_container_lifetime_value_companion);

    // III. classification umbrella
    D_CCT_RUN(tests_is_constexpr_positive);
    D_CCT_RUN(tests_is_constexpr_negative);
    D_CCT_RUN(tests_is_constexpr_is_the_compile_time_bit);
    D_CCT_RUN(tests_is_not_constexpr);
    D_CCT_RUN(tests_classification_exhaustive);
    D_CCT_RUN(tests_classification_value_companions);

    // IV. aggregate snapshot
    D_CCT_RUN(tests_class_members);
    D_CCT_RUN(tests_class_agrees_with_traits);
    D_CCT_RUN(tests_class_tuple_size_gap);
    D_CCT_RUN(tests_class_is_constexpr_and_empty);

    // V. robustness  (cv-ref, hostile types, completeness, trait shape)
    D_CCT_RUN(tests_cvref_signals);
    D_CCT_RUN(tests_cvref_verdicts);
    D_CCT_RUN(tests_hostile_types_rejected);
    D_CCT_RUN(tests_hostile_types_survived);
    D_CCT_RUN(tests_incomplete_type);
    D_CCT_RUN(tests_trait_shape);

    // VI. standard library
    D_CCT_RUN(tests_stdlib_array);
    D_CCT_RUN(tests_stdlib_vector_string);
    D_CCT_RUN(tests_stdlib_node_containers);
    D_CCT_RUN(tests_stdlib_tuple_pair);

    // VII. hazards  (the hard-error boundary; the mines are gated behind
    //      -DD_CCT_HAZARD_TESTS=1, which makes the BUILD the finding)
    D_CCT_RUN(tests_hazard_destructor);
    D_CCT_RUN(tests_hazard_tag_base);

    return rb.finish();
}

#undef D_CCT_RUN
