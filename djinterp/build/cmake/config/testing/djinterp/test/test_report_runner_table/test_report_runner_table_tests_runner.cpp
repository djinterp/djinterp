/******************************************************************************
* djinterp [tests]              test_report_runner_table_tests_runner.cpp
*
*   The main() for the test_report_runner_table.hpp suite.  Defines DTEST_SPEC_MODE so
* the suite header presents its provider face (report_runner_table_spec()) rather than
* its fixtures, builds a default option set, selects the PDF document face - the
* very PDF path this suite exercises - and hands the spec to run_module, whose
* non-zero return on any failure becomes the process exit code.
*
*   The section TUs (compiled WITHOUT DTEST_SPEC_MODE) define the tests_*
* predicates the spec's function pointers name; linking them with this runner
* forms the executable.
*
* path:      /build/cmake/config/testing/djinterp/test/output/test_report_runner_table_tests_runner.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.19
******************************************************************************/

#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_report_runner_table_tests.hpp"   // bare: resolved via the tests-dir INCLUDE


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_report_runner_table_tests.pdf"));

    return dt::run_module(
        tt::report_runner_table_spec(),
        "test_report_runner_table.hpp unit tests",                          // title
        "report_builder: live console accumulation + document emission",  // subtitle
        opts);
}
