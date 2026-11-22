/*******************************************************************************
* djinterp [cpp][meta]                        memory_discipline_tests_runner.cpp
*
*   Runner for the memory_discipline suite (memory_discipline_tests.hpp):
* defines DTEST_SPEC_MODE, takes the suite's spec and hands it to DTest.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/meta/memory_discipline/memory_discipline_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>  // std::string
// djinterp
#include "memory_discipline_tests.hpp"  // relative to the tests dir via INCLUDES

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("memory_discipline_tests.pdf"));

    return dt::run_module(
        tt::memory_discipline_spec(),
        "memory_discipline.hpp unit tests",
        "The discipline taxonomy, its predicates and carrier, the bridge to "
        "storage siting, the strategy-contract signals, and discipline_of.",
        opts);
}
