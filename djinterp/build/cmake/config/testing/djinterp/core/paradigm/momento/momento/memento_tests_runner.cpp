/******************************************************************************
* djinterp [test]                                      memento_tests_runner.cpp
*
*   Entry point for the memento pattern unit-test module. Defines
* DTEST_SPEC_MODE so the suite header compiles its memento_spec() provider,
* builds the default option set, requests a PDF report, and hands the module
* spec to run_module. The section translation units supply the predicate bodies
* the spec's entries reference.
*
* path:  /build/cmake/config/testing/djinterp/paradigm/momento/memento_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.10
******************************************************************************/

#define DTEST_SPEC_MODE

#include <string>

#include "memento_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("memento_tests.pdf"));

    return dt::run_module(
        tt::memento_spec(),
        "djinterp::paradigm::memento",
        "Memento pattern module unit tests",
        opts);
}
