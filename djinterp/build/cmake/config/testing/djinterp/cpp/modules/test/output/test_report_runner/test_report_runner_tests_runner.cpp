/******************************************************************************
* djinterp [tests]                          test_report_runner_tests_runner.cpp
*
*   The runner for the test_report_runner.hpp suite: a thin main() that selects
* the suite header's provider face, builds a test_option_set, and hands the
* spec to run_module.  It formats nothing itself - run_module drives the
* console report and (per the options below) renders the run's own PDF.
*
*   This target is compiled with D_TEST_REPORT_ENABLE_ARCHIVE defined (see the
* CMakeLists and the BUILD PREREQUISITE note in the suite header): the module
* under test does not compile at C++17 or higher without it, and the archive
* section exercises the packaging arm that macro gates.
*
* path:      /build/cmake/config/testing/djinterp/cpp/modules/test/output/test_report_runner/test_report_runner_tests_runner.cpp
*                test_report_runner/test_report_runner_tests_runner.cpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.22
******************************************************************************/

#define DTEST_SPEC_MODE   // the suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_report_runner_tests.hpp"   // resolved via the INCLUDES the
                                          //   CMakeLists adds (the tests dir)

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("test_report_runner_tests.pdf"));

    return dt::run_module(
        tt::report_runner_spec(),
        "test_report_runner.hpp unit tests",
        "the report_builder front end, its recording and console faces, and "
        "the document and archive emission at finish()",
        opts);
}
