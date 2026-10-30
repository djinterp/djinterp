/*****************************************************************************
* djinterp [testing]                     pool_memory_strategy_tests_runner.cpp
*
* Thin runner for the DPoolMemoryStrategy suite.  Defines DTEST_SPEC_MODE so the suite
* header presents its spec-provider face, seeds a test_option_set for a PDF
* report, and hands the module spec to run_module.  All per-test logic and
* formatting live in test_defaults.hpp.
*
*   This is the opt-in pool suite: it exercises DPoolMemoryStrategy against a
* real pool_resource.  Its CMakeLists entry defines D_MS_TEST_POOL_LAYER so
* the section TU and the provider's pool block compile in; without that macro
* the module reports zero tests.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/memory/pool_memory_strategy/pool_memory_strategy_tests_runner.cpp
*                                          pool_memory_strategy_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "pool_memory_strategy_tests.hpp"    // resolved via INCLUDES (the tests dir)


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("pool_memory_strategy_tests.pdf"));

    return dt::run_module(
        tt::pool_memory_strategy_spec(),
        "pool_memory_strategy.hpp unit tests",
        "the pool-backed element strategy (opt-in)",
        opts);
}
