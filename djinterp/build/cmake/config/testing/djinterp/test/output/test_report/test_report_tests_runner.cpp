/******************************************************************************
* djinterp [test]                                 test_report_tests_runner.cpp
*
*   Entry point for the test_report.hpp suite.  Defining DTEST_SPEC_MODE selects
* the header's spec-provider face (declarations + report_spec(), no fixtures);
* this TU contributes main() and the seven section TUs the bodies.  main() only
* configures options and hands report_spec() to run_module - all templating
* lives in test_defaults.hpp.
*
* path:      /build/cmake/config/testing/djinterp/test/output/test_report_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp   (suite header lives under tests/djinterp/test/output/)
#include "test_report_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_report_tests.pdf"));

    return dt::run_module(
        tt::report_spec(),
        "test_report.hpp unit tests",
        "The report data model: verdict, check, unit, module, report, rates, "
        "and filename expansion.",
        opts);
}
