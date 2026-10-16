/******************************************************************************
* djinterp [test]                                     template_tests_runner.cpp
*
*   Entry point for the template-source-sink algebra unit-test module.  Defines
* DTEST_SPEC_MODE so the suite header compiles its template_spec() provider,
* builds the default option set, requests a PDF report, and hands the module
* spec to run_module.  The section translation units supply the predicate bodies
* the spec's entries reference.
*
* path:  /build/cmake/config/testing/djinterp/core/paradigm/template/template_tests_runner.cpp
* author(s): Sam 'teer' Neal-Blim                          created: 2026.07.11
******************************************************************************/

#define DTEST_SPEC_MODE

#include <string>

#include "template_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("template_tests.pdf"));

    return dt::run_module(
        tt::template_spec(),
        "djinterp::core::paradigm::template",
        "Template-source-sink algebra unit tests",
        opts);
}
