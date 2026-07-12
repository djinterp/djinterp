// djinterp [test]  value_list_tests_runner.cpp
//   Entry point for the value_list.hpp suite: registers every section's tests
//   with report_builder, one module per section, and renders the results to PDF.

// djinterp
#include "value_list_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- value_list.hpp Test Suite");
    rb.set_subtitle("meta : the NTTP sequence -- the value-domain counterpart of "
                    "the dtuple type sequence");
    rb.set_author("teer");
    rb.set_description(
        "Covers the sequence itself, its detection face, the size and element "
        "traits, and the three carrier-driven ops (growth, transform, fold). Two "
        "fixtures carry most of the weight: decl_only_op is DECLARED and never "
        "DEFINED -- transform maps a list with it perfectly well, because "
        "transform only probes the leaf in an unevaluated context and uses its "
        "result type; and digits_op is non-commutative, so folding 1, 2, 3 from a "
        "seed of 0 yields 123 only under a LEFT fold. That traversal order is "
        "pinned a second, independent way by folding with prepend, which reverses "
        "the list. concat is checked against its monoid laws (associativity, and "
        "the empty list as a two-sided identity), and transform against the "
        "functor laws (identity and composition). The whole module is gated to "
        "C++17, so every test is gated to match -- and that this suite still "
        "COMPILES under C++11 and C++14, having included the header, is precisely "
        "the check that the header is inert below its tier. Verified under C++11, "
        "C++14, C++17, and C++20.");

    // render the final results to PDF.
    rb.use_pdf("value_list_tests_report.pdf");

    // -- I. the sequence -------------------------------------------------
    rb.module("I. the sequence (value_list<auto...>)",
              "size(), the empty-object layout, heterogeneity, the C++17 tier");
    rb.run("size() counts the pack",                     &tests_value_list_size_member);
    rb.run("the list is an empty object (no storage)",   &tests_value_list_is_empty_object);
    rb.run("the pack is heterogeneous (auto...)",        &tests_value_list_heterogeneous);
    rb.run("size() is static constexpr noexcept",        &tests_value_list_size_noexcept);
    rb.run("duplicates are distinct elements",           &tests_value_list_duplicates);
    rb.run("order and the NTTP's type fix identity",     &tests_value_list_type_identity);
    rb.run("the module is gated to its C++17 tier",      &tests_value_list_tier_gate);

    // -- II. detection ---------------------------------------------------
    rb.module("II. detection (is_value_list / ValueList)",
              "specialization-based detection and its C++20 concept face");
    rb.run("is_value_list: any specialization",          &tests_is_value_list_positive);
    rb.run("is_value_list: the type sequence is not one",&tests_is_value_list_negative);
    rb.run("detection is exact (look-alike, derived)",   &tests_is_value_list_exact);
    rb.run("clean_t strips cv-ref first",                &tests_is_value_list_cvref);
    rb.run("is_value_list_v agrees",                     &tests_is_value_list_v_agrees);
    rb.run("ValueList mirrors and gates resolution",     &tests_concept_value_list);
    rb.run("the concept face is gated to C++20",         &tests_concept_value_list_gating);

    // -- III. size + access ----------------------------------------------
    rb.module("III. size + element access",
              "value_list_size, value_list_at, and the carrier-returning at()");
    rb.run("value_list_size: the tuple_size analog",     &tests_value_list_size_trait);
    rb.run("it is an integral_constant",                 &tests_value_list_size_integral_constant);
    rb.run("value_list_size_v agrees",                   &tests_value_list_size_v_agrees);
    rb.run("value_list_at: first, middle, last",         &tests_value_list_at_positions);
    rb.run("the element keeps its own type",             &tests_value_list_at_preserves_value_type);
    rb.run("a deep index peels to the end",              &tests_value_list_at_deep);
    rb.run("value_list_at_v agrees",                     &tests_value_list_at_v_agrees);
    rb.run("at<I>() hands back a val_t carrier",         &tests_at_returns_carrier);
    rb.run("at() feeds the carrier pipeline",            &tests_at_feeds_the_carrier_pipeline);
    rb.run("the traits need an UNQUALIFIED list",        &tests_traits_require_an_unqualified_list);

    // -- IV. growth ------------------------------------------------------
    rb.module("IV. growth (append / prepend / concat)",
              "the four concat arities, and concat's monoid laws");
    rb.run("append puts the element at the end",         &tests_append);
    rb.run("append to the empty list",                   &tests_append_to_empty);
    rb.run("prepend puts the element at the front",      &tests_prepend);
    rb.run("prepend to the empty list",                  &tests_prepend_to_empty);
    rb.run("growth does not homogenise the values",      &tests_growth_heterogeneous);
    rb.run("concat() and concat(a): the degenerate arities", &tests_concat_nullary_and_unary);
    rb.run("concat(a, b): the base case, order kept",    &tests_concat_binary);
    rb.run("concat of three or more folds pairwise",     &tests_concat_variadic);
    rb.run("empty lists contribute nothing",             &tests_concat_empty_lists);
    rb.run("concat's monoid laws (identity, assoc)",     &tests_concat_monoid_laws);

    // -- V. transformation -----------------------------------------------
    rb.module("V. transformation (transform)",
              "the functor laws, value-type changes, the unevaluated probe");
    rb.run("every element is mapped, in order",          &tests_transform_maps_every_element);
    rb.run("the empty list maps to the empty list",      &tests_transform_empty);
    rb.run("law: identity",                              &tests_transform_law_identity);
    rb.run("law: composition",                           &tests_transform_law_composition);
    rb.run("the leaf may change the value's TYPE",       &tests_transform_changes_value_type);
    rb.run("a DECLARED-only leaf works (unevaluated)",   &tests_transform_unevaluated_probe);
    rb.run("a generic leaf maps a mixed list",           &tests_transform_heterogeneous);
    rb.run("length is invariant (map, not filter)",      &tests_transform_preserves_length);

    // -- VI. reduction ---------------------------------------------------
    rb.module("VI. reduction (fold)",
              "the left fold, and the accumulator shapes it admits");
    rb.run("fold threads a reducer over the list",       &tests_fold_reduces);
    rb.run("the empty list returns the seed",            &tests_fold_empty_returns_seed);
    rb.run("the fold is LEFT-associative",               &tests_fold_is_left_associative);
    rb.run("a single element: one turn of the recursion",&tests_fold_single_element);
    rb.run("the accumulator may be a value_list",        &tests_fold_accumulator_may_be_a_list);
    rb.run("fold + prepend REVERSES the list",           &tests_fold_reverses_with_prepend);
    rb.run("a deep list folds to depth",                 &tests_fold_deep);
    rb.run("the seed may be a plain value",              &tests_fold_seed_may_be_a_plain_value);
    rb.run("transform then fold: map-reduce",            &tests_fold_and_transform_compose);

    return rb.finish();
}
