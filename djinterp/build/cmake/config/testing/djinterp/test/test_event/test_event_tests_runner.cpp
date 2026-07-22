/******************************************************************************
* djinterp [test]                                 test_event_tests_runner.cpp
*
*   Entry point for the test_event.hpp suite.  Defining DTEST_SPEC_MODE selects
* the header's spec-provider face (declarations + event_spec(), no fixtures);
* this TU contributes main() and the four section TUs the bodies.  main() only
* configures options and hands event_spec() to run_module - all templating
* lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/test_event_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_event_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_event_tests.pdf"));

    return dt::run_module(
        tt::event_spec(),
        "test_event.hpp unit tests",
        "The DTest event alphabet: node, tags, custom-event macros, and dispatch.",
        opts);
}
