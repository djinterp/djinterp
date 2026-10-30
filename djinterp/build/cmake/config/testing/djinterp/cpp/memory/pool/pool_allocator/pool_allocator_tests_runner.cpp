/******************************************************************************
* djinterp [test]                              pool_allocator_tests_runner.cpp
*
*   Runner for the pool_allocator suite.
*
*   Options only: select the spec face of the suite header, choose a PDF
* alongside the console output, and hand the spec to run_module.  No test
* logic and no formatting live here - drive_report inside test_defaults.hpp
* owns both.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/memory/pool/pool_allocator/pool_allocator_tests_runner.cpp
*            pool_allocator_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#define DTEST_SPEC_MODE     // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "pool_allocator_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("pool_allocator_tests.pdf"));

    return dt::run_module(
        tt::pool_allocator_spec(),
        "pool_allocator.hpp unit tests",
        "STL-conforming stateful allocator over a pool_resource - the "
        "Allocator type surface, the two allocation paths, object "
        "lifecycle, equality, and container integration.",
        opts);
}
