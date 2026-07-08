/******************************************************************************
* djinterp [test]                                applicative_tests_runner.cpp
*
*   Entry point for the applicative.hpp unit suite.  Assembles the five section
* blocks -- each contributed by its own translation unit through the block-
* providers declared in applicative_tests.hpp -- into a single module_spec and
* hands it to the framework runner.
*
*   Sections:
*     0 + I.  protocol & traits ......... applicative_tests_protocol.cpp
*     0.      is_applicable ............. applicative_tests_applicable.cpp
*     II.1    pure ..................... applicative_tests_pure.cpp
*     II.2    ap ....................... applicative_tests_ap.cpp
*     II.3    lift_a2 .................. applicative_tests_lift_a2.cpp
*
*   The results are emitted as a single PDF alongside the live console report.
* Configuration rides the shared module-report option set: document = pdf,
* output_file = the report path, split = whole_run (one document for the run),
* pack = none (written loose, no container).  The option-set value face (set<>)
* is C++20 and PDF rendering rides the C++17 pdf gate; below either tier the run
* degrades to a clean console-only report rather than failing to build.
* run_module returns 0 iff every executed test passed, which becomes the process
* exit status for CI.
*
*
* path:      /tests/djinterp/core/functional/applicative_tests_runner.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "applicative_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "applicative.hpp";
    mod.descriptor =
        "Applicative protocol + generic operations (pure / ap / lift_a2)";

    mod.blocks.push_back(tt::applicative_protocol_block());     // 0 + I.
    mod.blocks.push_back(tt::applicative_applicable_block());   // 0. (applicability)
    mod.blocks.push_back(tt::applicative_pure_block());         // II.1
    mod.blocks.push_back(tt::applicative_ap_block());           // II.2
    mod.blocks.push_back(tt::applicative_lift_a2_block());      // II.3

    // configure the report through the option set: one PDF for the whole run,
    // written loose (the stored path needs no external library).
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("applicative_tests_report.pdf"));
    opts.set<dt::test_option::split>(dt::test_output_split::whole_run);
    opts.set<dt::test_option::pack>(dt::test_output_pack::none);

    return dt::run_module(
        mod,
        "djinterp applicative<> unit tests",
        "protocol + is_applicable + pure + ap + lift_a2",
        opts);
}
