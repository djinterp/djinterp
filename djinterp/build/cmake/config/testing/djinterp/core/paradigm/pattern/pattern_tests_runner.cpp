/******************************************************************************
* djinterp [test]                                      pattern_tests_runner.cpp
*
*   Entry point for the pattern primitive unit-test module. Defines
* DTEST_SPEC_MODE so the suite header compiles its pattern_spec() provider,
* builds the default option set, requests a PDF report, and hands the module
* spec to run_module. The section translation units supply the predicate bodies
* the spec's entries reference.
*
* path:  /build/cmake/config/testing/djinterp/core/paradigm/pattern/pattern_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.10
******************************************************************************/

#define DTEST_SPEC_MODE

#include <string>

#include "pattern_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("pattern_tests.pdf"));

    return dt::run_module(
        tt::pattern_spec(),
        "djinterp::core::paradigm::pattern",
        "Pattern primitive module unit tests",
        opts);
}
