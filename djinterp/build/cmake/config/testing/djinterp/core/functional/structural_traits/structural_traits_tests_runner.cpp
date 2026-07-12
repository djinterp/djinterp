// djinterp [test]  structural_traits_tests_runner.cpp
//   Entry point for the structural_traits.hpp suite: registers every section's
//   tests with report_builder, one module per section.

// djinterp
#include "structural_traits_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- structural_traits.hpp Test Suite");
    rb.set_subtitle("functional : structural detection for the pattern protocol "
                    "and the dataflow roles (source / step / reducer)");
    rb.set_author("teer");
    rb.set_description(
        "Every trait here is a SFINAE probe of one exact expression, so the suite "
        "pins each probe from every side it can be got at. The arity trilogy "
        "probes a MUTABLE lvalue -- four fixtures (const, non-const, &-qualified, "
        "&&-qualified) determine that expression uniquely -- and the mutable half "
        "is load-bearing: a pull source advances on every call and a tallying "
        "reducer accumulates, so both need a non-const operator(). has_find_method, "
        "by contrast, probes a CONST lvalue and converts its result with an "
        "EXPLICIT static_cast<bool>, so it accepts an explicitly bool-convertible "
        "result that an implicit check would refuse. The optional-like traits "
        "require BOTH halves -- bool-testable AND dereferenceable -- and each is "
        "failed by a fixture missing exactly one. The detected shapes are not "
        "merely recognised but DRIVEN: the scan protocol walks every match, the "
        "pull source exhausts, the unfold threads its state, and the concept-"
        "constrained drivers run real reductions. Verified under C++11, C++14, "
        "C++17, and C++20 -- the header degrades to C++11, as claimed.");

    rb.use_pdf("structural_traits_tests_report.pdf");

    // -- I. member detection ---------------------------------------------
    rb.module("I. member detection",
              "has_match_result_type, has_find_method (the pattern protocol)");
    rb.run("has_match_result_type: the extraction face", &tests_has_match_result_type_positive);
    rb.run("a data member of that name is not a type",   &tests_has_match_result_type_negative);
    rb.run("cv-ref stripped before the lookup",          &tests_has_match_result_type_cvref);
    rb.run("has_find_method: the searchable shape",      &tests_has_find_method_positive);
    rb.run("the detected shape drives a real scan",      &tests_has_find_method_scan_protocol);
    rb.run("wrong shapes rejected (arity / type / data)",&tests_has_find_method_negative);
    rb.run("result taken by EXPLICIT static_cast<bool>", &tests_has_find_method_return_conversion);
    rb.run("find is probed on a CONST lvalue",           &tests_has_find_method_const_lvalue);
    rb.run("parameters need only bind to the probe",     &tests_has_find_method_parameter_binding);
    rb.run("clean_t applied to all three parameters",    &tests_has_find_method_cvref);

    // -- II. arity trilogy -----------------------------------------------
    rb.module("II. arity trilogy",
              "is_nullary_callable / is_unary_callable / is_binary_callable");
    rb.run("nullary: the pull-source step",              &tests_is_nullary_callable_positive);
    rb.run("nullary: non-callables and wrong arity",     &tests_is_nullary_callable_negative);
    rb.run("unary: the transform / predicate step",      &tests_is_unary_callable_positive);
    rb.run("unary: non-callables and wrong arity",       &tests_is_unary_callable_negative);
    rb.run("binary: the reducer step (acc, x) -> acc",   &tests_is_binary_callable_positive);
    rb.run("binary: non-callables and wrong arity",      &tests_is_binary_callable_negative);
    rb.run("the probe is a MUTABLE lvalue",              &tests_arity_mutable_lvalue_contract);
    rb.run("clean_t decays the argument (int& is lost)", &tests_arity_argument_decay);
    rb.run("clean_t decays the type (const is ignored)", &tests_arity_type_decay);
    rb.run("the three partition by arity",               &tests_arity_trilogy_exclusive);

    // -- III. optional-like protocols ------------------------------------
    rb.module("III. optional-like protocols",
              "produces_optional_like (nullary) and is_unfold_step (stateful)");
    rb.run("sources: maybe<>, pointer, std::optional",   &tests_produces_optional_like_positive);
    rb.run("the bool half is required",                  &tests_produces_optional_like_requires_bool);
    rb.run("the deref half is required",                 &tests_produces_optional_like_requires_deref);
    rb.run("the source step must be nullary",            &tests_produces_optional_like_requires_nullary);
    rb.run("the detected shape drives a real pull",      &tests_produces_optional_like_pull_protocol);
    rb.run("unfold step: State -> maybe<next>",          &tests_is_unfold_step_positive);
    rb.run("unfold step: wrong result or wrong state",   &tests_is_unfold_step_negative);
    rb.run("both halves required of the result",         &tests_is_unfold_step_requires_both);
    rb.run("the detected shape drives a real unfold",    &tests_is_unfold_step_unfold_protocol);
    rb.run("nullary and unary halves do not overlap",    &tests_optional_like_nullary_vs_unary);

    // -- IV. convenience aliases -----------------------------------------
    rb.module("IV. convenience aliases (C++14+)",
              "the _v shorthands and their version gate");
    rb.run("has_match_result_type_v agrees",             &tests_has_match_result_type_v_agrees);
    rb.run("has_find_method_v agrees",                   &tests_has_find_method_v_agrees);
    rb.run("the arity shorthands agree",                 &tests_arity_v_agree);
    rb.run("the source-protocol shorthands agree",       &tests_optional_like_v_agree);
    rb.run("the shorthands are constant expressions",    &tests_aliases_are_constant_expressions);
    rb.run("gated on variable templates (C++14+)",       &tests_aliases_gating);

    // -- V. protocol concepts --------------------------------------------
    rb.module("V. protocol concepts (C++20)",
              "BinaryCallable, Reducer, Transducer, UnfoldStep");
    rb.run("BinaryCallable mirrors the trait",           &tests_concept_binary_callable_mirrors);
    rb.run("Reducer: the reduction step",                &tests_concept_reducer);
    rb.run("Reducer constrains arity, not the result",   &tests_concept_reducer_ignores_result);
    rb.run("Transducer: reducer -> reducer",             &tests_concept_transducer);
    rb.run("UnfoldStep mirrors the trait",               &tests_concept_unfold_step);
    rb.run("the faces drive real reductions / unfolds",  &tests_concept_drives_the_protocols);
    rb.run("the concepts gate overload resolution",      &tests_concept_overload_gating);
    rb.run("gated on concepts (C++20)",                  &tests_concepts_gating);

    return rb.finish();
}
