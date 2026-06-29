// djinterp
#include "../../../../../../../../../tests/djinterp/core/event/event_handler/event_handler_tests.hpp"
#include "djinterp/test/test_report_runner.hpp"    // report_builder + report model


// D_EH_RUN
//   macro: runs ::djinterp::testing::<fn> as a unit test named for the
// function - recorded in the report and echoed to the live console.
#define D_EH_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("event_handler.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("event_handler_tests.pdf");

    rb.module("event_handler",
              "The handler primitive: handler_id, verdict normalization and "
              "tuple-apply, handler compatibility traits, the seq/skip monoid, "
              "and the C++20 concept layer");

    // handler identification
    D_EH_RUN(tests_handler_id_relational);
    D_EH_RUN(tests_handler_id_validity);
    D_EH_RUN(tests_handler_id_null);
    D_EH_RUN(tests_handler_id_value_semantics);

    // verdict normalization + tuple-apply
    D_EH_RUN(tests_invoke_normalized_void);
    D_EH_RUN(tests_invoke_normalized_verdict);
    D_EH_RUN(tests_apply_handler_arities);
    D_EH_RUN(tests_apply_handler_void_normalization);
    D_EH_RUN(tests_apply_handler_forwards_values);

    // handler compatibility detection + traits
    D_EH_RUN(tests_handler_traits_is_invocable);
    D_EH_RUN(tests_handler_traits_return_type);
    D_EH_RUN(tests_handler_traits_returns_void_verdict);
    D_EH_RUN(tests_handler_traits_is_compatible);
    D_EH_RUN(tests_handler_traits_is_nothrow);
    D_EH_RUN(tests_handler_traits_expected_arity);
    D_EH_RUN(tests_handler_traits_cvref);

    // the handler monoid (seq, skip)
    D_EH_RUN(tests_skip_always_pass);
    D_EH_RUN(tests_seq_pass_pass);
    D_EH_RUN(tests_seq_left_zero);
    D_EH_RUN(tests_seq_void_normalization);
    D_EH_RUN(tests_seq_associativity);
    D_EH_RUN(tests_seq_lvalue_passthrough);
    D_EH_RUN(tests_seq_clean_type);

    // concept constraints (C++20+; vacuous pass where concepts are absent)
    D_EH_RUN(tests_concept_is_handler);
    D_EH_RUN(tests_concept_handler_for);
    D_EH_RUN(tests_concept_void_verdict_handler_for);
    D_EH_RUN(tests_concept_nothrow_handlers);
    D_EH_RUN(tests_concept_handler_for_event_of_arity);
    D_EH_RUN(tests_concept_arity_aliases);
    D_EH_RUN(tests_concept_non_event_safety);

    return rb.finish();
}

#undef D_EH_RUN
