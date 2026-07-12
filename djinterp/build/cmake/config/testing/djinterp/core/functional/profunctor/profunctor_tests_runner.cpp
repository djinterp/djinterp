// djinterp [test]  profunctor_tests_runner.cpp
//   Entry point for the profunctor.hpp suite: registers every section's tests
//   with report_builder, one module per section.

// djinterp
#include "profunctor_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- profunctor.hpp Test Suite");
    rb.set_subtitle("functional : dimap over a two-parameter arrow -- "
                    "contravariant in the input, covariant in the output");
    rb.set_author("teer");
    rb.set_description(
        "Covers the arrow wrapper profn<F> (the canonical instance, since a bare "
        "lambda does not expose its domain and codomain), the profunctor_traits "
        "protocol and its marker-based detection, and the three generic "
        "operations -- dimap, and the derived lmap (contravariant, input only) "
        "and rmap (covariant, output only). The profunctor laws are checked "
        "behaviourally (arrows agree pointwise), including that pre composes "
        "contravariantly while post composes covariantly. A second, unrelated "
        "instance (pf_arrow) proves the operations dispatch through the trait "
        "rather than being wired to profn. Verified under C++17 and C++20.");

    rb.use_pdf("profunctor_tests_report.pdf");

    // -- I. the arrow wrapper --------------------------------------------
    rb.module("I. arrow wrapper (profn<F>)",
              "construction, call forwarding, make_profn decay, constexpr");
    rb.run("profn wraps a callable",                     &tests_profn_construct);
    rb.run("operator() forwards to the callable",        &tests_profn_call);
    rb.run("make_profn deduces and decays F",            &tests_make_profn_decay);
    rb.run("operator() perfectly forwards its argument", &tests_profn_forwarding);
    rb.run("usable in a constant expression",            &tests_profn_constexpr);
    rb.run("result type is the callable's result",       &tests_profn_return_type);

    // -- II. protocol + detection ----------------------------------------
    rb.module("II. protocol + detection",
              "profunctor_traits, is_profunctor, internal helpers, the concept");
    rb.run("is_profunctor: specialised instances",       &tests_is_profunctor_positive);
    rb.run("is_profunctor: non-profunctors rejected",    &tests_is_profunctor_negative);
    rb.run("is_profunctor: cv-ref stripped",             &tests_is_profunctor_cvref);
    rb.run("detection requires the is_specialized marker",&tests_is_profunctor_requires_marker);
    rb.run("profunctor_traits surface (marker / dimap)", &tests_profunctor_traits_members);
    rb.run("Profunctor concept mirrors the trait",       &tests_profunctor_concept);
    rb.run("internal identity / dimap helpers",          &tests_internal_helpers);

    // -- III. operations -------------------------------------------------
    rb.module("III. operations (dimap / lmap / rmap)",
              "both ends, type changes, the profunctor laws, constexpr");
    rb.run("dimap adapts input and output",              &tests_dimap_both_ends);
    rb.run("dimap changes both parameter types",         &tests_dimap_type_change);
    rb.run("lmap adapts the input only (contravariant)", &tests_lmap_input_only);
    rb.run("rmap adapts the output only (covariant)",    &tests_rmap_output_only);
    rb.run("law: identity",                              &tests_law_identity);
    rb.run("law: dimap composition",                     &tests_law_composition);
    rb.run("law: rmap composes covariantly",             &tests_rmap_covariant_composition);
    rb.run("law: lmap composes contravariantly",         &tests_lmap_contravariant_composition);
    rb.run("dimap factors through lmap and rmap",        &tests_dimap_equals_lmap_rmap);
    rb.run("operations fold in a constant expression",   &tests_operations_constexpr);

    // -- IV. instance ----------------------------------------------------
    rb.module("IV. instance (profn<F>)",
              "post . fn . pre, fresh arrows, no unwrapping, generic dispatch");
    rb.run("dimap builds post . fn . pre",               &tests_instance_composition_order);
    rb.run("dimap returns a fresh profn (no unwrapping)",&tests_instance_returns_profn);
    rb.run("adapted arrows compose further",             &tests_instance_composable);
    rb.run("the source arrow is left untouched",         &tests_instance_preserves_original);
    rb.run("the instance carries is_specialized",        &tests_instance_is_specialized);
    rb.run("operations dispatch through the trait",      &tests_instance_generic_dispatch);

    return rb.finish();
}
