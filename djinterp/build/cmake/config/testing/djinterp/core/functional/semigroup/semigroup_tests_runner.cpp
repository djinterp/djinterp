// djinterp [test]  semigroup_tests_runner.cpp
//   Entry point for the semigroup.hpp suite: registers every section's tests
//   with report_builder, one module per section, and renders a PDF at finish().

// djinterp
#include "semigroup_tests.hpp"
#include "../report/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- semigroup.hpp Test Suite");
    rb.set_subtitle("functional : the Semigroup protocol and its associative "
                    "combine (mappend)");
    rb.set_author("teer");
    rb.set_description(
        "The protocol and the operation alone -- concrete instances live in "
        "monoid.hpp -- so the suite supplies its own semigroups: string concat, "
        "int sum, int max, left projection, and a modular family registered "
        "through the trait's _Enable SFINAE hook. Associativity is checked on "
        "every instance (including non-commutative ones), and mappend's "
        "conditional-constexpr folding under static_assert. Verified under C++17 "
        "and C++20.");

    rb.use_pdf("semigroup_tests_report.pdf");

    // -- I. protocol + detection -----------------------------------------
    rb.module("I. protocol + detection",
              "semigroup_traits, is_semigroup, the _Enable hook, the concept");
    rb.run("is_semigroup: specialised instances",        &tests_is_semigroup_positive);
    rb.run("is_semigroup: non-semigroups rejected",      &tests_is_semigroup_negative);
    rb.run("is_semigroup: cv-ref stripped",              &tests_is_semigroup_cvref);
    rb.run("family detected via _Enable SFINAE hook",    &tests_is_semigroup_family_sfinae);
    rb.run("detection requires the is_specialized marker",&tests_is_semigroup_requires_marker);
    rb.run("semigroup_traits surface (marker / combine)", &tests_semigroup_traits_members);
    rb.run("Semigroup concept mirrors the trait",        &tests_semigroup_concept);

    // -- II. mappend -----------------------------------------------------
    rb.module("II. mappend (associative combine)",
              "dispatch, associativity, order, idempotence, constexpr, generic");
    rb.run("dispatch to each instance's combine",        &tests_mappend_dispatch);
    rb.run("associativity law (all instances)",          &tests_mappend_associativity);
    rb.run("operand order preserved (non-commutative)",  &tests_mappend_order_preserved);
    rb.run("max is idempotent",                          &tests_mappend_idempotent_max);
    rb.run("folds inside static_assert (constexpr)",     &tests_mappend_constexpr);
    rb.run("modular family wraps at its modulus",        &tests_mappend_family_modular);
    rb.run("generic over the protocol (thrice)",         &tests_mappend_generic_thrice);
    rb.run("accepts const / lvalue operands",            &tests_mappend_cvref_args);
    rb.run("returns the semigroup type",                 &tests_mappend_return_type);
    rb.run("requires both operands same type",           &tests_mappend_requires_same_type);
    rb.run("runs at runtime (dual domain)",              &tests_mappend_runtime_domain);

    return rb.finish();
}
