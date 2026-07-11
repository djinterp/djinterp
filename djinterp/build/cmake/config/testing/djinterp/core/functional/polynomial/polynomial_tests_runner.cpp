// djinterp [test]  polynomial_tests_runner.cpp
//   Entry point for the polynomial.hpp suite: registers every section's tests
//   with report_builder, one module per section, and renders a PDF at finish().

// djinterp
#include "polynomial_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- polynomial.hpp Test Suite");
    rb.set_subtitle("parse/grammar : the polynomial functor F as a foldable "
                    "protocol citizen");
    rb.set_author("teer");
    rb.set_description(
        "Value-level vocabulary, type-level shapes, and the Functor / "
        "Traversable instances of F. Functor and cata / ana integration use "
        "plain functions; the Traversable checks thread a minimal test_maybe "
        "effect. Verified under C++17 and C++20.");

    rb.use_pdf("polynomial_tests_report.pdf");

    // -- I. value-level polynomial-functor vocabulary --------------------
    rb.module("I. value-level vocabulary",
              "poly_var / poly_unit / poly_const / poly_sum / poly_product");
    rb.run("poly_var: recursive position, F(X)=X",       &tests_poly_var);
    rb.run("poly_unit: unit variant, F(X)=1",            &tests_poly_unit);
    rb.run("poly_const: constant variant, F(X)=K_C",     &tests_poly_const);
    rb.run("poly_sum: binary sum and injections",        &tests_poly_sum);
    rb.run("poly_product: binary product",               &tests_poly_product);
    rb.run("value_type / constant_type contracts",
           &tests_value_level_value_types);

    // -- II. type-level shapes -------------------------------------------
    rb.module("II. type-level shapes",
              "poly_*_t documentation-only structural builders");
    rb.run("poly_constant_t: constant_type",             &tests_poly_constant_t);
    rb.run("poly_recursion_t: empty marker",             &tests_poly_recursion_t);
    rb.run("poly_sum_t: left / right",                   &tests_poly_sum_t);
    rb.run("poly_product_t: children / arity",           &tests_poly_product_t);
    rb.run("poly_compose_t: outer / inner",              &tests_poly_compose_t);
    rb.run("poly_mu_t: functor",                         &tests_poly_mu_t);

    // -- III. functor_traits instances -----------------------------------
    rb.module("III. functor instances",
              "functor_traits, functor_map, laws, cata / ana integration");
    rb.run("is_functor: every poly_* carrier",           &tests_is_functor_poly);
    rb.run("is_functor: negative cases",                 &tests_is_functor_negative);
    rb.run("functor_value_type: inner X",                &tests_functor_value_type);
    rb.run("rebind: re-parameterise over U",             &tests_functor_rebind);
    rb.run("map: poly_var transforms value",             &tests_functor_map_poly_var);
    rb.run("map: poly_unit never calls f",               &tests_functor_map_poly_unit);
    rb.run("map: poly_const preserves payload",          &tests_functor_map_poly_const);
    rb.run("map: poly_sum maps active arm",              &tests_functor_map_poly_sum);
    rb.run("map: poly_product maps both",                &tests_functor_map_poly_product);
    rb.run("functor law: map(id) == id",                 &tests_functor_law_identity);
    rb.run("functor law: map(g.f) == map(g).map(f)",     &tests_functor_law_composition);
    rb.run("cata / ana / hylo over mu<nat_f>",           &tests_functor_cata_integration);

    // -- IV. traversable_traits instances --------------------------------
    rb.module("IV. traversable instances",
              "traversable_traits, traverse, sequence (var / unit / const only)");
    rb.run("is_traversable: var / unit / const",         &tests_is_traversable_poly);
    rb.run("is_traversable: sum / product are NOT",       &tests_is_traversable_negative);
    rb.run("traversable_value_type: inner X",            &tests_traversable_value_type);
    rb.run("traverse: poly_var threads the effect",      &tests_traverse_poly_var);
    rb.run("traverse: poly_unit is pure (f uncalled)",   &tests_traverse_poly_unit);
    rb.run("traverse: poly_const is pure, payload kept", &tests_traverse_poly_const);
    rb.run("sequence: poly_var inverts the effect",      &tests_sequence_poly_var);
    rb.run("sequence: poly_unit / poly_const",           &tests_sequence_poly_unit_const);

    return rb.finish();
}
