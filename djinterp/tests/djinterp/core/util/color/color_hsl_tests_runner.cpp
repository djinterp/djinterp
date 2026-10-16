/*******************************************************************************
* djinterp [tests]                                    color_hsl_tests_runner.cpp
*
*   Test runner for the color_hsl suite.  Compiled with DTEST_SPEC_MODE
* defined, so the suite header presents its spec face: the section translation
* units supply the test bodies, and this file supplies main().
*
*   Build via the sibling CMakeLists.txt, which globs the section TUs and
* links them against this runner:
*
*       cmake --build . --target color_hsl_tests
*       ./color_hsl_tests
*
*
* path:      /tests/djinterp/core/util/color/color_hsl_tests_runner.cpp
*            color_hsl_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#define DTEST_SPEC_MODE 1

// std
#include <string>
// djinterp
// djinterp [tests]
#include "color_hsl_tests.hpp"


// main
//   entry point: hands the module spec to the framework runner, configured to
// emit a PDF report alongside the console summary.
int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("color_hsl_tests.pdf"));

    return dt::run_module(
        tt::color_hsl_spec(),
        "color_hsl",
        "The HSL cylindrical model",
        opts);
}
