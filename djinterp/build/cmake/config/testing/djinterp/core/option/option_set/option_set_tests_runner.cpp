/******************************************************************************
* djinterp [test]                                  option_set_tests_runner.cpp
*
*   Entry point for the option_set.hpp unit suite.  Assembles the six section
* blocks - each contributed by its own translation unit through the block-
* provider declared in option_set_tests.hpp - into a single module_spec and
* hands it to the framework runner.
*
*   Sections:
*     I+II. expansion + flattening    (option_set_tests_expand.cpp)
*     III.  construction checks       (option_set_tests_checks.cpp)
*     IV.   option_set pack surface   (option_set_tests_core.cpp)
*     V.    field marker + value face (option_set_tests_values.cpp)
*     VI.   queries                   (option_set_tests_queries.cpp)
*     VII.  concepts (C++20)          (option_set_tests_concepts.cpp)
*
*   option_set.hpp has a type-level core that compiles down to its base
* standard; its value-carrying face is C++20-only and its concepts (plus the
* one constrained query, option_set_key_type) need C++20 concepts.  Below a
* given tier the corresponding tests are emitted empty, so this runner links
* and reports a clean run at any standard level rather than failing to build.
* run_module returns 0 iff every executed test passed, which becomes the
* process exit status for CI.
*
*   run_module also accepts an optional trailing options argument (the shared
* module-report options surface, e.g. to emit a PDF report); it is omitted here
* so the suite depends only on the default console reporter.
*
*
* path:      /build/cmake/config/testing/djinterp/core/option/option_set_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.07
******************************************************************************/

// djinterp
#include "../../.././../../../../../tests/djinterp/core/option/option_set_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "option_set.hpp";
    mod.descriptor = "expansion, construction checks, pack surface, value face, queries, concepts";

    mod.blocks.push_back(tt::option_set_expand_block());     // I + II.
    mod.blocks.push_back(tt::option_set_checks_block());     // III.
    mod.blocks.push_back(tt::option_set_core_block());       // IV.
    mod.blocks.push_back(tt::option_set_values_block());     // V.
    mod.blocks.push_back(tt::option_set_queries_block());    // VI.
    mod.blocks.push_back(tt::option_set_concepts_block());   // VII.

    // configure the report through the option set: one PDF per suite, all five
    // bundled into a single .zip (stored fallback needs no external library).
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::split>(dt::test_output_split::per_module);
    opts.set<dt::test_option::pack>(dt::test_output_pack::none);

    return dt::run_module(
        mod,
        "djinterp option_set<> unit tests",
        "expansion + checks + pack surface + value face + queries + concepts");

    std::vector<dt::module_spec> mods;
    mods.push_back(mod);        // assembly each runner's main()
    return dt::run_suite("djinterp option layer",
                         "option / override / set / compare unit suites",
        mods, "djinterp option-layer unit tests", opts);
}
