// djinterp [test]  functional_common_tests_runner.cpp
//   Entry point for the functional_common.hpp suite: registers every section's
//   tests with report_builder, one module per section.

// djinterp
#include "functional_common_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- functional_common.hpp Test Suite");
    rb.set_subtitle("functional : the shared callable vocabulary -- is_callable, "
                    "callable_result_t, is_predicate, and their concept faces");
    rb.set_author("teer");
    rb.set_description(
        "Covers the merged module: the two call traits, the predicate trait, the "
        "C++14 _v shorthands, and the C++20 concept faces absorbed from "
        "functional_concepts.hpp. These are EXPRESSION-probing traits, so the "
        "suite pins the contract from both sides: a const-lvalue call (a "
        "non-const or &&-qualified operator() is not callable), templated "
        "operator()s (the generic-lambda shape a declared-signature probe would "
        "miss), call_nonesuch for ill-formed calls, and -- on the predicate side "
        "-- IMPLICIT convertibility to bool, so a result with an explicit "
        "operator bool is callable but not a predicate. The concept faces are "
        "exercised in all three forms: mirroring the trait, as a constrained "
        "template parameter, and as a requires-clause that gates overload "
        "resolution. Verified under C++11, C++14, C++17, and C++20 -- the three "
        "floors the module's own gating targets.");

    rb.use_pdf("functional_common_tests_report.pdf");

    // -- I. call traits --------------------------------------------------
    rb.module("I. call traits",
              "is_callable, callable_result_t, and the const-lvalue contract");
    rb.run("is_callable: the callable shapes",           &tests_is_callable_positive);
    rb.run("is_callable: non-callables rejected",        &tests_is_callable_negative);
    rb.run("the probe is a const-lvalue call",           &tests_is_callable_const_lvalue);
    rb.run("succeeds on a templated operator()",         &tests_is_callable_templated_operator);
    rb.run("resolves against an overload set",           &tests_is_callable_overload_set);
    rb.run("argument category and constness are exact",  &tests_is_callable_arg_categories);
    rb.run("works at every arity",                       &tests_is_callable_arity_spread);
    rb.run("carries the integral_constant surface",      &tests_is_callable_integral_constant);
    rb.run("reuses is_invocable_with",                   &tests_is_callable_reuses_invocable_with);
    rb.run("callable_result_t: the call's result type",  &tests_callable_result_basic);
    rb.run("ill-formed call yields call_nonesuch",       &tests_callable_result_nonesuch);
    rb.run("the result's reference is preserved",        &tests_callable_result_preserves_reference);
    rb.run("callable_result_t aliases call_result_t",    &tests_callable_result_is_alias);
    rb.run("re-exports function_traits.hpp",             &tests_reexports_function_traits);

    // -- II. predicate trait ---------------------------------------------
    rb.module("II. predicate trait",
              "is_predicate: callable on one Arg with a bool-convertible result");
    rb.run("is_predicate: the predicate shapes",         &tests_is_predicate_positive);
    rb.run("is_predicate: non-predicates rejected",      &tests_is_predicate_negative);
    rb.run("any bool-convertible result qualifies",      &tests_is_predicate_convertible_results);
    rb.run("an EXPLICIT operator bool does not",         &tests_is_predicate_explicit_operator_bool);
    rb.run("inherits the const-lvalue contract",         &tests_is_predicate_const_lvalue);
    rb.run("a predicate is a predicate OVER an Arg",     &tests_is_predicate_arg_type);
    rb.run("succeeds on a templated operator()",         &tests_is_predicate_templated_operator);
    rb.run("takes exactly one argument",                 &tests_is_predicate_single_argument);
    rb.run("reuses is_invocable_r_with<bool, ...>",      &tests_is_predicate_reuses_invocable_r_with);
    rb.run("carries the integral_constant surface",      &tests_is_predicate_integral_constant);

    // -- III. convenience aliases ----------------------------------------
    rb.module("III. convenience aliases (C++14+)",
              "is_callable_v / is_predicate_v and their version gate");
    rb.run("is_callable_v agrees with the trait",        &tests_is_callable_v_agrees);
    rb.run("is_predicate_v agrees with the trait",       &tests_is_predicate_v_agrees);
    rb.run("the shorthands are constant expressions",    &tests_aliases_are_constant_expressions);
    rb.run("is_callable_v carries the argument pack",    &tests_is_callable_v_variadic);
    rb.run("gated on variable templates (C++14+)",       &tests_aliases_gating);

    // -- IV. concept faces -----------------------------------------------
    rb.module("IV. concept faces (C++20)",
              "Callable / Predicate: mirrors, constraints, overload gating");
    rb.run("Callable mirrors is_callable",               &tests_concept_callable_mirrors_trait);
    rb.run("Predicate mirrors is_predicate",             &tests_concept_predicate_mirrors_trait);
    rb.run("usable as a constrained template parameter", &tests_concept_constrained_parameter);
    rb.run("usable in a requires-clause",                &tests_concept_requires_clause);
    rb.run("the concepts gate overload resolution",      &tests_concept_overload_gating);
    rb.run("Predicate is strictly stronger than Callable",&tests_concept_callable_but_not_predicate);
    rb.run("gated on concepts (C++20)",                  &tests_concepts_gating);

    return rb.finish();
}
