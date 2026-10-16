/******************************************************************************
* djinterp [test]                          sort_dispatch_tests_runner.cpp
*
*   main() for the sort_dispatch_tests executable.  Defines DTEST_SPEC_MODE so the suite
* header presents its spec-provider face (no fixtures compiled here), builds
* the option set, and hands the spec to run_module.
*
*
* path:      /build/cmake/config/testing/djinterp/core/util/sort/sort_dispatch_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#define DTEST_SPEC_MODE   // suite exposes its spec provider, not its fixtures

// std
#include <string>
// djinterp [test]
#include "sort_dispatch_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();

    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("sort_dispatch_tests.pdf"));

    return dt::run_module(
        tt::sort_dispatch_spec(),
        "sort_dispatch.hpp unit tests",
        "algorithm-agnostic dispatch: tags, traits, the sorter facade and the sort() entry points",
        opts);
}
