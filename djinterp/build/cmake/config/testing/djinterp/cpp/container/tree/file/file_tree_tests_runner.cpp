/*******************************************************************************
* djinterp [cpp][container]                           file_tree_tests_runner.cpp
*
*   The runner for file_tree_tests.hpp: its spec, run as one module.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/tree/file/file_tree_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
#define DTEST_SPEC_MODE  // the suite's spec provider, not its fixtures

// std
#include <string>  // std::string
// djinterp
#include "file_tree_tests.hpp"  // the suite, through INCLUDES


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("file_tree_tests.pdf"));

    return dt::run_module(
        tt::file_tree_spec(),
        "container/tree/file unit tests",
        "The file tree's core, its scanners run against a directory the "
        "suite builds, its attributes, paths, filters, traits and concepts.",
        opts);
}
