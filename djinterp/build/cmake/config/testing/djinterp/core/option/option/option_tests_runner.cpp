/******************************************************************************
* djinterp [test]                                      option_tests_runner.cpp
*
*   Entry point for the option.hpp unit suite.  option.hpp is ONE header, so
* it lowers to ONE module_spec whose blocks are its four sections; each
* section's block-provider (declared in option_tests.hpp, defined in that
* section's .cpp) is folded in here and the whole is handed to run_module.
*
*   run_module drives BOTH framework views from the one spec: the six-kind
* tree (test_suite > test_module > test_block > unit_test > test_fn, with
* per-node name + descriptor) and the console report (one unit per test, its
* descriptor the assertion line).  It returns a process exit code - 0 iff
* every leaf verdict passed - which main hands straight back.
*
*   CONSOLE ONLY BY DESIGN:
*   This runner emits the console report and no document (the _pdf argument is
* left null).  To also render a PDF, switch to the options-driven run_module
* overload and configure a test_option_set exactly as the aggregate event
* runner does -
*     dt::test_option_set opts = dt::default_test_options();
*     opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
*     opts.set<dt::test_option::output_file>(std::string("option_tests.pdf"));
*     return dt::run_module(mod, "...", "...", opts);
*   - no other change is needed.
*
*   To run a single section in isolation, build a module_spec from just that
* section's block-provider and pass it here the same way.
*
* path:      /build/cmake/config/testing/djinterp/core/option/option_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// djinterp
#include "option_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    // option.hpp -> one module; its four sections -> the module's blocks.
    dt::module_spec mod;
    mod.name       = "option.hpp";
    mod.descriptor = "Core option<> type, is_option detection trait, and the "
                     "Option/UnaryOption/ArgsOption concept analogs.";
    mod.blocks.push_back(tt::option_sentinels_block());   // I.   sentinels
    mod.blocks.push_back(tt::option_core_block());        // II.  option<> core
    mod.blocks.push_back(tt::option_is_option_block());   // III. is_option
    mod.blocks.push_back(tt::option_concepts_block());    // IV.  concepts (C++20)

    return dt::run_module(
        mod,
        "djinterp option<> unit tests",
        "Coverage of option.hpp: sentinels, core type, detection trait, concepts.");
}
