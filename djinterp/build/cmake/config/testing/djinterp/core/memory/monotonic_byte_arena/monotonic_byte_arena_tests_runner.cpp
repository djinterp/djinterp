/*****************************************************************************
* djinterp [testing]                     monotonic_byte_arena_tests_runner.cpp
*
* Thin runner for the monotonic_byte_arena suite.  Defines DTEST_SPEC_MODE so the suite
* header presents its spec-provider face, seeds a test_option_set for a PDF
* report, and hands the module spec to run_module.  All per-test logic and
* formatting live in test_defaults.hpp.
*
*
* path:      /build/cmake/config/testing/djinterp/core/memory/monotonic_byte_arena/
*                                          monotonic_byte_arena_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#define DTEST_SPEC_MODE            // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "monotonic_byte_arena_tests.hpp"    // resolved via INCLUDES (the tests dir)


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(
        std::string("monotonic_byte_arena_tests.pdf"));

    return dt::run_module(
        tt::monotonic_byte_arena_spec(),
        "monotonic_byte_arena.hpp unit tests",
        "the static and heap-backed byte arenas",
        opts);
}
