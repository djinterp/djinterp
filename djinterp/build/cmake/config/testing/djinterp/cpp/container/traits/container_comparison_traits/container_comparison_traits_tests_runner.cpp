// djinterp
#include "container_comparison_traits_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"   // report_builder + report model


// D_CCM_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_CCM_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("container_comparison_traits.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("container_comparison_traits_tests.pdf");

    rb.module("container_comparison_traits",
              "The comparison of two container TYPES: the content hierarchy, "
              "the discipline strength order, the three realization preorders, "
              "the product refinement they fold into - and the order-theoretic "
              "laws that fold has to obey");

    // I. comparison relations
    D_CCM_RUN(tests_order_relation_enum);
    D_CCM_RUN(tests_order_relation_name);
    D_CCM_RUN(tests_order_relation_meet_table);
    D_CCM_RUN(tests_order_relation_meet_laws);

    // II. content: the native level lambda
    D_CCM_RUN(tests_content_level_enum);
    D_CCM_RUN(tests_content_level_name);
    D_CCM_RUN(tests_content_level_rank);
    D_CCM_RUN(tests_content_level_coarser);
    D_CCM_RUN(tests_native_content_level_rungs);
    D_CCM_RUN(tests_native_content_level_precedence);
    D_CCM_RUN(tests_native_content_level_value_companion);

    // III. discipline: the overlay strength order
    D_CCM_RUN(tests_discipline_equivalent);
    D_CCM_RUN(tests_discipline_ordered);
    D_CCM_RUN(tests_discipline_incomparable);
    D_CCM_RUN(tests_discipline_non_container);

    // IV. realization preorders
    D_CCM_RUN(tests_lifetime_relation);
    D_CCM_RUN(tests_lifetime_is_total);
    D_CCM_RUN(tests_iterability_bottom);
    D_CCM_RUN(tests_iterability_lattice);
    D_CCM_RUN(tests_iterability_mixed_kinds);
    D_CCM_RUN(tests_storage_relation);
    D_CCM_RUN(tests_storage_inert);

    // V. the comparison profile
    D_CCM_RUN(tests_profile_members);
    D_CCM_RUN(tests_profile_agrees_with_axes);
    D_CCM_RUN(tests_refinement_composition);
    D_CCM_RUN(tests_refinement_non_container);
    D_CCM_RUN(tests_realization_equal);
    D_CCM_RUN(tests_realization_comparable);
    D_CCM_RUN(tests_profile_value_companions);

    // VI. the order-theoretic laws
    D_CCM_RUN(tests_law_reflexivity);
    D_CCM_RUN(tests_law_direction);
    D_CCM_RUN(tests_law_transitivity);
    D_CCM_RUN(tests_law_content_excluded);
    D_CCM_RUN(tests_axis_coupling);

    // VII. robustness  (cv-ref, hostile types, completeness, trait shape)
    D_CCM_RUN(tests_cvref_axes);
    D_CCM_RUN(tests_cvref_profile);
    D_CCM_RUN(tests_hostile_types);
    D_CCM_RUN(tests_incomplete_type);
    D_CCM_RUN(tests_trait_shape);

    // VIII. standard library
    D_CCM_RUN(tests_stdlib_content_rungs);
    D_CCM_RUN(tests_stdlib_sequences);
    D_CCM_RUN(tests_stdlib_associative);
    D_CCM_RUN(tests_stdlib_refinement);

    return rb.finish();
}

#undef D_CCM_RUN
