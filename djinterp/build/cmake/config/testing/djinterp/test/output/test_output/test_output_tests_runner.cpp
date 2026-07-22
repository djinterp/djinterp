/******************************************************************************
* djinterp [test]                                 test_output_tests_runner.cpp
*
*   Entry point for the test_output.hpp suite.  Defining DTEST_SPEC_MODE selects
* the header's spec-provider face (declarations + output_spec(), no fixtures);
* this TU contributes main() and the four section TUs the bodies.  main() only
* configures options and hands output_spec() to run_module - all templating
* lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/output/test_output_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp   (suite header lives under tests/djinterp/test/output/)
#include "test_output_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_output_tests.pdf"));

    return dt::run_module(
        tt::output_spec(),
        "test_output.hpp unit tests",
        "The report output pipeline: formats, rendering, bundle assembly, "
        "and emit to buffer / disk.",
        opts);
}
