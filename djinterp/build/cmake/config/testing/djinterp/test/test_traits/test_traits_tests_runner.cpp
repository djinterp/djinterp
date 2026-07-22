// djinterp
#include "test_traits_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"   // report_builder + report model


// D_TT_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_TT_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("test_traits.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("test_traits_tests.pdf");

    rb.module("test_traits",
              "The trait-testing toolkit, turned on itself: the probe "
              "declarators, the readings, the non-short-circuiting "
              "quantifiers, the bool-trait shape check, the cv-ref matrix, "
              "the fixture zoo, the build-time pins, and the C++20 concept "
              "layer");

    // I. probe declarators
    D_TT_RUN(tests_type_probe);
    D_TT_RUN(tests_type_probe_2);
    D_TT_RUN(tests_expr_probe);
    D_TT_RUN(tests_expr_probe_2);
    D_TT_RUN(tests_noexcept_probe);
    D_TT_RUN(tests_constexpr_probe);
    D_TT_RUN(tests_probe_scope);

    // II. reading a probe
    D_TT_RUN(tests_is_nothrow_probe);
    D_TT_RUN(tests_yields_lvalue);
    D_TT_RUN(tests_yields_xvalue);
    D_TT_RUN(tests_yields_prvalue);
    D_TT_RUN(tests_is_valid);
    D_TT_RUN(tests_is_valid_evil);
    D_TT_RUN(tests_reading_value_companions);

    // III. type-set quantifiers
    D_TT_RUN(tests_count_holds);
    D_TT_RUN(tests_count_holds_empty_pack);
    D_TT_RUN(tests_holds_for_all);
    D_TT_RUN(tests_holds_for_any);
    D_TT_RUN(tests_holds_for_none);
    D_TT_RUN(tests_quantifier_binding);
    D_TT_RUN(tests_no_short_circuit);
    D_TT_RUN(tests_quantifier_value_companions);

    // IV. trait shape
    D_TT_RUN(tests_is_bool_trait_positive);
    D_TT_RUN(tests_is_bool_trait_sink);
    D_TT_RUN(tests_is_bool_trait_predicates);
    D_TT_RUN(tests_is_bool_trait_non_traits);
    D_TT_RUN(tests_trait_is_well_formed);
    D_TT_RUN(tests_trait_v_agrees);
    D_TT_RUN(tests_shape_self_application);
    D_TT_RUN(tests_shape_value_companions);

    // V. cv-ref agreement
    D_TT_RUN(tests_cvref_cells);
    D_TT_RUN(tests_cvref_report_accessors);
    D_TT_RUN(tests_cvref_report_first_disagreement);
    D_TT_RUN(tests_trait_across_cvref);
    D_TT_RUN(tests_trait_ignores_cvref);
    D_TT_RUN(tests_cvref_value_companion);

    // VI. fixtures  (the type zoo)
    D_TT_RUN(tests_fixture_shapes);
    D_TT_RUN(tests_fixture_private_members);
    D_TT_RUN(tests_fixture_ambiguous_members);
    D_TT_RUN(tests_fixture_greedy);
    D_TT_RUN(tests_fixture_evil);
    D_TT_RUN(tests_fixture_throwing_nothrowing);
    D_TT_RUN(tests_fixture_literal_nonliteral);
    D_TT_RUN(tests_fixture_enums);
    D_TT_RUN(tests_fixture_nonclass_types);
    D_TT_RUN(tests_hostile_list_cardinalities);
    D_TT_RUN(tests_hostile_list_complete);

    // VII. build-time pins  (the mines are gated behind -DD_TT_HAZARD_TESTS=1,
    //      which makes the BUILD the finding)
    D_TT_RUN(tests_static_pin);
    D_TT_RUN(tests_build_time_hazards);

    // VIII. concept layer  (C++20)
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    D_TT_RUN(tests_trait_concept_agree);
    D_TT_RUN(tests_declare_subsumes_refines);
    D_TT_RUN(tests_declare_subsumes_implies_only);
    D_TT_RUN(tests_declare_subsumes_partial);
    D_TT_RUN(tests_declare_subsumes_shape);

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

    return rb.finish();
}

#undef D_TT_RUN
