/*******************************************************************************
* djinterp [cpp][container]                               arena_tests_runner.cpp
*
*   Runner for the arena suite (arena_tests.hpp): defines DTEST_SPEC_MODE,
* takes the suite's spec and hands it to DTest.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/arena/arena_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.16
*                                                            revised: 2026.11.16
*******************************************************************************/
// suite exposes its spec provider, not fixtures
#define DTEST_SPEC_MODE

// std
#include <string>  // std::string
// djinterp
#include "arena_tests.hpp"  // relative to the tests dir via INCLUDES

int main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("arena_tests.pdf"));

    return dt::run_module(
        tt::arena_spec(),
        "container/arena unit tests",
        "The link flags and policies, arena_node, the arena container, its "
        "traits and concepts, and the arena tree with its traits and "
        "concepts.",
        opts);
}
