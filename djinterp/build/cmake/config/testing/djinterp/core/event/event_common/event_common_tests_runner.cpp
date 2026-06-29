// djinterp
#include "../../../../../../../../../tests/djinterp/core/event/event_common/event_common_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_EC_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_EC_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("event_common.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("event_common_tests.pdf");

    rb.module("event_common",
              "Event foundations: verdict set, tag detection, traits, "
              "declaration macros, and the C++20 concept layer");

    // verdict (the set P)
    D_EC_RUN(tests_verdict_enumerators);
    D_EC_RUN(tests_verdict_type_properties);
    D_EC_RUN(tests_consumed_values);
    D_EC_RUN(tests_consumed_consistency);

    // event tag detection
    D_EC_RUN(tests_has_payload_type);
    D_EC_RUN(tests_has_args_type);
    D_EC_RUN(tests_has_event_payload);
    D_EC_RUN(tests_event_payload_select);
    D_EC_RUN(tests_has_event_name);
    D_EC_RUN(tests_is_tuple);

    // index sequence polyfill + tuple-apply
    D_EC_RUN(tests_index_sequence);
    D_EC_RUN(tests_make_index_sequence);
    D_EC_RUN(tests_apply_impl_direct);
    D_EC_RUN(tests_apply_tuple_arities);
    D_EC_RUN(tests_apply_tuple_values);

    // event traits
    D_EC_RUN(tests_event_traits_payload_type);
    D_EC_RUN(tests_event_traits_args_alias);
    D_EC_RUN(tests_event_traits_arity);
    D_EC_RUN(tests_event_traits_has_name);
    D_EC_RUN(tests_event_traits_has_args);
    D_EC_RUN(tests_event_traits_legacy_and_both);

    // declaration macros
    D_EC_RUN(tests_d_event_payload);
    D_EC_RUN(tests_d_event_name);
    D_EC_RUN(tests_d_event_empty);
    D_EC_RUN(tests_d_event_arity_range);
    D_EC_RUN(tests_d_event_is_event);

    // concept constraints (C++20+; vacuous pass where concepts are absent)
    D_EC_RUN(tests_concept_is_event);
    D_EC_RUN(tests_concept_event_type);
    D_EC_RUN(tests_concept_non_event_type);
    D_EC_RUN(tests_concept_empty_event_type);
    D_EC_RUN(tests_concept_argument_event_type);
    D_EC_RUN(tests_concept_named_event_type);
    D_EC_RUN(tests_concept_unnamed_event_type);
    D_EC_RUN(tests_concept_event_of_arity);
    D_EC_RUN(tests_concept_nullary_unary);
    D_EC_RUN(tests_concept_binary_ternary);
    D_EC_RUN(tests_concept_variadic_event_type);

    return rb.finish();
}

#undef D_EC_RUN
