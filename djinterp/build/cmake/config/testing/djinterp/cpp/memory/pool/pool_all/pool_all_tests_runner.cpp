/******************************************************************************
* djinterp [testing]                              pool_all_tests_runner.cpp
*
* Aggregate runner for the whole pool module.  Unlike the per-module runners,
* this one includes BOTH pool suite headers in their spec face and hands the
* union of their providers to run_suite, which emits ONE combined report (a
* band per module) over the module.  Co-compiling the two headers in a single
* TU is well-formed because each suite gives its check macro a distinct suffix
* (D_PR_ / D_PA_) - so there is no macro collision - and every provider,
* declaration, and test name is unique across the pair.
*
*   THE PAIR IS ONE CONTRACT: pool_allocator.hpp includes pool.hpp and is
* meaningless without it, so a combined report is the honest unit of review -
* an allocator failure and the pool failure underneath it land side by side
* rather than in two documents nobody reads together.
*
*   NOT TO BE CONFUSED WITH pool_memory_strategy_tests, which lives on the
* memory-strategy axis and exercises the strategy ADAPTER built over a pool
* rather than the pool itself.  That suite keeps the D_PL_ suffix; these two
* use D_PR_ and D_PA_.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/memory/pool/pool_all/pool_all_tests_runner.cpp
*                                                  pool_all_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#define DTEST_SPEC_MODE            // every suite exposes its spec provider

// std
#include <string>
#include <vector>
// djinterp - both pool suites, in spec face
#include "pool_tests.hpp"
#include "pool_allocator_tests.hpp"


int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("pool_all_tests.pdf"));
    // opts.set<dt::test_option::split>(dt::test_split::per_module);

    std::vector<dt::module_spec> modules = {
        tt::pool_spec(),
        tt::pool_allocator_spec(),
    };

    return dt::run_suite(
        "pool module",
        "the whole module: the policy-composed pool_resource - release "
        "strategies, block layouts, slot geometry, growth, accounting and "
        "the classification layer - and the STL-conforming allocator that "
        "hands single slots out of it",
        modules,
        "pool module - full suite",
        opts);
}
