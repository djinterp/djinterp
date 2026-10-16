/******************************************************************************
* djinterp [test]                                     visitor_tests_runner.cpp
*
*   Entry point for the visitor pattern unit-test module.  Defines DTEST_SPEC_MODE
* so the suite header compiles its visitor_spec() provider, builds the default
* option set, requests a PDF report, and hands the module spec to run_module.  The
* section translation units supply the predicate bodies the spec's entries
* reference.
*
*   The spec is dialect-INDEPENDENT: the variant and concept blocks are always
* present, and their predicates stand down and report success where the feature is
* absent.  So this runner needs no conditional wiring, and the test count is the
* same at every language level the module supports.
*
* path:  /build/cmake/config/testing/djinterp/patterns/visitor/visitor_tests_runner.cpp
* author(s): Sam 'teer' Neal-Blim                          created: 2026.07.11
******************************************************************************/

#define DTEST_SPEC_MODE

#include <string>

#include "visitor_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("visitor_tests.pdf"));

    return dt::run_module(
        tt::visitor_spec(),
        "djinterp::patterns::visitor",
        "Visitor pattern unit tests: classic, acyclic, static and variant dispatch",
        opts);
}
