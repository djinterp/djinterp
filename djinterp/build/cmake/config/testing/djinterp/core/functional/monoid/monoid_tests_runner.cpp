// djinterp [test]  monoid_tests_runner.cpp
//   Runner for the monoid.hpp unit tests.  Drives every test through the
//   report_builder front end, grouped one module per section of the module
//   under test, prints the live console report, and emits the results as a
//   PDF (report_builder::use_pdf + finish).
//
//   Build: compile alongside the four monoid_tests_*.cpp section files and
//   link.  PDF emission rides the C++17 pdf gate in test_report_runner.hpp;
//   below C++17 the run stays console-only.

// djinterp
#include "monoid_tests.hpp"                 // the tests (djinterp::testing)
#include "../report/test_report_runner.hpp" // report_builder (djinterp::test)


int
main()
{
    using djinterp::test::report_builder;
    namespace tst = djinterp::testing;

    report_builder rb;

    // report metadata.
    rb.set_title("djinterp :: monoid.hpp unit tests");
    rb.set_subtitle("core/functional/monoid.hpp");
    rb.set_author("teer");
    rb.set_description(
        "Semigroup + monoid algebra over the standard instances: the monoid "
        "newtypes, the detection protocol, the per-type combine/identity "
        "instances and their monoid laws, and the generic mempty / mconcat / "
        "fold_monoid operations.");

    // emit one PDF for the whole run, written beside the runner.
    rb.use_pdf("monoid_tests_report.pdf");

    // =====================================================================
    //  I.   MONOID NEWTYPES
    // =====================================================================
    rb.module("I. monoid newtypes",
              "sum / product / all / any / min / max: construction, defaults, "
              "explicitness, constexpr");
    rb.run("sum newtype",                        &tst::tests_sum_newtype);
    rb.run("product newtype",                    &tst::tests_product_newtype);
    rb.run("all newtype",                        &tst::tests_all_newtype);
    rb.run("any newtype",                        &tst::tests_any_newtype);
    rb.run("min newtype",                        &tst::tests_min_newtype);
    rb.run("max newtype",                        &tst::tests_max_newtype);
    rb.run("newtype default-constructibility",   &tst::tests_newtype_default_constructibility);
    rb.run("newtype constexpr construction",     &tst::tests_newtype_constexpr_construction);

    // =====================================================================
    //  II.  MONOID PROTOCOL
    // =====================================================================
    rb.module("II. monoid protocol",
              "is_monoid / is_monoid_v / Monoid detection and the "
              "monoid-implies-semigroup invariant");
    rb.run("is_monoid (positive)",               &tst::tests_is_monoid_positive);
    rb.run("is_monoid (negative)",               &tst::tests_is_monoid_negative);
    rb.run("is_monoid (cv/ref decay)",           &tst::tests_is_monoid_decay);
    rb.run("is_monoid_v shorthand",              &tst::tests_is_monoid_v);
    rb.run("Monoid concept",                     &tst::tests_monoid_concept);
    rb.run("monoid implies semigroup",           &tst::tests_monoid_implies_semigroup);

    // =====================================================================
    //  III. INSTANCES  (semigroup + monoid)
    // =====================================================================
    rb.module("III. instances",
              "per-type combine + identity + the monoid laws "
              "(left/right identity, associativity)");
    rb.run("std::string instance",               &tst::tests_string_instance);
    rb.run("std::vector<T> instance",            &tst::tests_vector_instance);
    rb.run("sum<T> instance",                    &tst::tests_sum_instance);
    rb.run("product<T> instance",                &tst::tests_product_instance);
    rb.run("all instance",                       &tst::tests_all_instance);
    rb.run("any instance",                       &tst::tests_any_instance);
    rb.run("min<T> instance",                    &tst::tests_min_instance);
    rb.run("max<T> instance",                    &tst::tests_max_instance);
    rb.run("is_specialized markers",             &tst::tests_instance_is_specialized);

    // =====================================================================
    //  IV.  GENERIC MONOID OPERATIONS
    // =====================================================================
    rb.module("IV. generic operations",
              "mempty / mconcat / fold_monoid, incl. the empty-foldable "
              "identity path and monoid-type deduction");
    rb.run("mempty",                             &tst::tests_mempty);
    rb.run("mconcat (multi-element)",            &tst::tests_mconcat_multi);
    rb.run("mconcat (single element)",           &tst::tests_mconcat_single);
    rb.run("mconcat (empty -> mempty)",          &tst::tests_mconcat_empty);
    rb.run("mconcat (nested vector)",            &tst::tests_mconcat_nested_vector);
    rb.run("mconcat return type",                &tst::tests_mconcat_return_type);
    rb.run("fold_monoid (multi-element)",        &tst::tests_fold_monoid_multi);
    rb.run("fold_monoid (empty -> mempty)",      &tst::tests_fold_monoid_empty);
    rb.run("fold_monoid type deduction",         &tst::tests_fold_monoid_type_deduction);
    rb.run("fold_monoid string concat",          &tst::tests_fold_monoid_string_concat);

    // print the per-module boxes + comprehensive summary, write the PDF,
    // and return the process exit code (0 iff every test passed).
    return rb.finish();
}
