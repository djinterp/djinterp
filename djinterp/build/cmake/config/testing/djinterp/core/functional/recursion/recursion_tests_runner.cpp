// djinterp [test]  recursion_tests_runner.cpp
//   Entry point for the recursion.hpp suite: registers every section's tests
//   with report_builder, one module per section, and renders a PDF at finish().

// djinterp
#include "recursion_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- recursion.hpp Test Suite");
    rb.set_subtitle("functional : recursion schemes over a Functor "
                    "(mu, cata, ana, hylo)");
    rb.set_author("teer");
    rb.set_description(
        "The least fixed point mu<F> and the universal fold / unfold / refold "
        "it supports. Fixtures supply three base functors -- nat_f (one hole), "
        "list_f (hole plus payload), tree_f (two holes) -- plus std::vector as "
        "a native recursive carrier. Verified under C++17 and C++20.");

    rb.use_pdf("recursion_tests_report.pdf");

    // -- I. mu<F> --------------------------------------------------------
    rb.module("I. mu (least fixed point)",
              "In / out / empty / layer_type / shared-layer semantics");
    rb.run("default-constructed mu is empty",            &tests_mu_default_empty);
    rb.run("In yields a non-empty value",                &tests_mu_in_nonempty);
    rb.run("out returns the boxed layer",                &tests_mu_out_returns_layer);
    rb.run("layer_type is F<mu<F>>",                     &tests_mu_layer_type);
    rb.run("shared layer: copy / assign / In copies",    &tests_mu_sharing);
    rb.run("nested layers navigate via out",             &tests_mu_nested_navigation);

    // -- II-IV. traits + detection ---------------------------------------
    rb.module("II-IV. traits + detection",
              "recursive_traits / corecursive_traits, is_recursive, mu instances");
    rb.run("recursive_traits<mu>: base / project",       &tests_recursive_traits_mu);
    rb.run("project peels one layer",                    &tests_recursive_traits_project);
    rb.run("corecursive_traits<mu>: base / embed",       &tests_corecursive_traits_mu);
    rb.run("embed builds one layer",                     &tests_corecursive_traits_embed);
    rb.run("project / embed round-trip",                 &tests_traits_roundtrip);
    rb.run("is_recursive detection + concept",           &tests_is_recursive);
    rb.run("is_corecursive detection + concept",         &tests_is_corecursive);
    rb.run("native carrier (std::vector) registration",  &tests_custom_carrier_registration);

    // -- V. cata ---------------------------------------------------------
    rb.module("V. cata (universal fold)",
              "phi-algebras over nat / list / tree / vector, laws, callable forms");
    rb.run("nat depth",                                  &tests_cata_nat_depth);
    rb.run("list sum",                                   &tests_cata_list_sum);
    rb.run("list length (payload ignored)",              &tests_cata_list_length);
    rb.run("list to string (foreign result type)",       &tests_cata_list_to_string);
    rb.run("tree leaf-sum (two holes)",                  &tests_cata_tree_leaf_sum);
    rb.run("tree height (two holes, max)",               &tests_cata_tree_height);
    rb.run("native vector fold",                         &tests_cata_native_vector);
    rb.run("base case only",                             &tests_cata_base_case);
    rb.run("reflection law: cata[In] == id",             &tests_cata_reflection_law);
    rb.run("algebra as lambda / fn / functor",           &tests_cata_algebra_forms);

    // -- VI. ana ---------------------------------------------------------
    rb.module("VI. ana (universal unfold)",
              "psi-coalgebras building nat / list / tree / vector");
    rb.run("nat from int",                               &tests_ana_nat);
    rb.run("list range",                                 &tests_ana_list_range);
    rb.run("balanced tree (two holes)",                  &tests_ana_tree_balanced);
    rb.run("native vector build",                        &tests_ana_native_vector);
    rb.run("base case only",                             &tests_ana_base_case);
    rb.run("produces exact structure",                   &tests_ana_produces_structure);
    rb.run("coalgebra as lambda / fn / functor",         &tests_ana_coalgebra_forms);

    // -- VII. hylo -------------------------------------------------------
    rb.module("VII. hylo (deforested refold)",
              "refold without a mu; factorial; hylo == cata . ana");
    rb.run("nat depth refold",                           &tests_hylo_nat_depth);
    rb.run("list sum refold",                            &tests_hylo_list_sum);
    rb.run("factorial (build [n..1], fold product)",     &tests_hylo_factorial);
    rb.run("tree leaf-count refold",                     &tests_hylo_tree);
    rb.run("hylo == cata . ana",                         &tests_hylo_equals_cata_ana);
    rb.run("base case only",                             &tests_hylo_base_case);
    rb.run("algebra / coalgebra forms",                  &tests_hylo_forms);

    return rb.finish();
}
