/******************************************************************************
* djinterp [test]                             option_override_tests_runner.cpp
*
*   Entry point for the option_override.hpp unit suite.  Assembles the five
* section blocks - each contributed by its own translation unit through the
* block-provider declared in option_override_tests.hpp - into a single
* module_spec and hands it to the framework runner.
*
*   Sections:
*     I.   (re)construction helpers   (option_override_helpers_tests.cpp)
*     II.  merge_args_union           (option_override_merge_tests.cpp)
*     III. lazy on_delta_only + append (option_override_lazy_tests.cpp)
*     IV.  option_set_override engine (option_override_engine_tests.cpp)
*     V.   ready-made policies        (option_override_policies_tests.cpp)
*
*   option_override.hpp is C++20-only (its engine constrains the policy
* parameter with the OverridePolicy concept).  Below C++20 every block above is
* emitted empty, so this runner still links and reports a clean (0 tests) run
* rather than failing to build.  run_module returns 0 iff every executed test
* passed, which becomes the process exit status for CI.
*
*   run_module also accepts an optional trailing options argument (the same
* module-report options surface used elsewhere in the framework, e.g. to emit a
* PDF report); it is omitted here so the suite depends only on the default
* console reporter.
*
*
* path:      /build/cmake/config/testing/djinterp/core/option/option_override_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// djinterp
#include "../../../../../../../../tests/djinterp/core/option/option_override_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "option_override.hpp";
    mod.descriptor = "option-aware override engine, merge_args_union, and ready-made policies";

    mod.blocks.push_back(tt::option_override_helpers_block());   // I.
    mod.blocks.push_back(tt::option_override_merge_block());     // II.
    mod.blocks.push_back(tt::option_override_lazy_block());      // III.
    mod.blocks.push_back(tt::option_override_engine_block());    // IV.
    mod.blocks.push_back(tt::option_override_policies_block());  // V.

    return dt::run_module(
        mod,
        "djinterp option_override<> unit tests",
        "engine + merge + lazy SFINAE + policies (C++20)");
}
