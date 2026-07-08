/******************************************************************************
* djinterp [test]                                 alternative_tests_runner.cpp
*
*   Entry point for the alternative.hpp unit suite.  Assembles the four section
* blocks -- each contributed by its own translation unit through the block-
* providers declared in alternative_tests.hpp -- into a single module_spec and
* hands it to the framework runner.
*
*   Sections:
*     0 + I.  protocol & traits ......... alternative_tests_protocol.cpp
*     II.1    aempty ................... alternative_tests_aempty.cpp
*     II.2    alt ...................... alternative_tests_alt.cpp
*     II.3    asum ..................... alternative_tests_asum.cpp
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
* path:      /tests/djinterp/core/functional/alternative_tests_runner.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "alternative_tests.hpp"


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    dt::module_spec mod;
    mod.name       = "alternative.hpp";
    mod.descriptor =
        "Alternative protocol + generic operations (aempty / alt / asum)";

    mod.blocks.push_back(tt::alternative_protocol_block());   // 0 + I.
    mod.blocks.push_back(tt::alternative_aempty_block());     // II.1
    mod.blocks.push_back(tt::alternative_alt_block());        // II.2
    mod.blocks.push_back(tt::alternative_asum_block());       // II.3

    // configure the report through the option set: one PDF for the whole run,
    // written loose (the stored path needs no external library).
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("alternative_tests_report.pdf"));
    opts.set<dt::test_option::split>(dt::test_output_split::whole_run);
    opts.set<dt::test_option::pack>(dt::test_output_pack::none);

    return dt::run_module(
        mod,
        "djinterp alternative<> unit tests",
        "protocol + aempty + alt + asum",
        opts);
}
