/******************************************************************************
* djinterp [test]                  test_runner_tests_runner.cpp
*
*   The entry point for the test_runner unit-test suite.  Defining
* DTEST_SPEC_MODE selects the suite header's spec-provider face (declarations +
* runner_spec, no fixture bodies), so this translation unit contributes main()
* and the section TUs contribute the test bodies.  main() renders the module to
* a PDF via run_module.
*
* path:      /tests/djinterp/test/test_runner_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#define DTEST_SPEC_MODE

#include <string>

#include "../../../../../../../tests/djinterp/test/test_runner_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_runner_tests.pdf"));

    return dt::run_module(
        tt::runner_spec(),
        "djinterp::test::test_runner",
        "Unit tests for the top-level RUN facade (reporter helpers, the facade, "
        "rendering, and show/routing + free functions).",
        opts);
}
