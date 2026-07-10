/******************************************************************************
* djinterp [test]                                     mediator_tests_runner.cpp
*
*   Entry point for the mediator pattern unit-test module.  Defines
* DTEST_SPEC_MODE so the suite header compiles its mediator_spec() provider,
* builds the default option set, requests a PDF report, and hands the module
* spec to run_module.  The section translation units supply the predicate
* bodies that the spec's entries reference.
*
* path:  /build/cmake/config/testing/djinterp/patterns/mediator/mediator_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.09
******************************************************************************/

#define DTEST_SPEC_MODE

#include <string>

#include "mediator_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("mediator_tests.pdf"));

    return dt::run_module(
        tt::mediator_spec(),
        "djinterp::patterns::mediator",
        "Mediator pattern module unit tests",
        opts);
}
