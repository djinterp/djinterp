/******************************************************************************
* djinterp [test]                          test_handler_tests_runner.cpp
*
*   Standalone runner for the test_handler suite.  It includes the single suite
* header with DTEST_SPEC_MODE defined - which drops the header's fixtures and
* check helper and exposes its module_spec provider instead - collects that one
* spec, and hands it to run_module.  run_module lowers the spec into the test
* tree (structure + per-node metadata) and projects it onto a report driving
* the console report and, here, one PDF.  The five section translation units
* (result / counters / dispatch / walk / session), compiled WITHOUT
* DTEST_SPEC_MODE, supply the bool() definitions the spec's function pointers
* resolve to; this file is only the spec collection and the entry point.
*
*   OUTPUT VIA TEST_OPTIONS - ONE PDF:
*   The document side rides one option set (test_options.hpp):
*     document    -> pdf                       emit PDF (not the txt default)
*     output_file -> "test_handler_tests.pdf"  the document's path
*   Leave `document` at its txt default (or drop these two lines) for a
*   console-only run.  Being a SINGLE module, there is nothing to split or
*   bundle, so no split / pack / archive_format is set.
*
*   FINDING THE FILE:
*   output_file is a RELATIVE name, so the .pdf lands in the process's current
* working directory - under Visual Studio the Debugging "Working Directory",
* NOT the folder holding the .exe.  Set output_file to an absolute path to pin
* it, or watch the console for the "wrote report: ..." line.
*
* path:      /build/cmake/config/testing/djinterp/test/test_handler_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "../../../../../../../tests/djinterp/test/test_handler_tests.hpp"  // resolved via the test include path

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    // configure the report through the option set: console plus one PDF.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_handler_tests.pdf"));

    return dt::run_module(
        tt::handler_spec(),                               // the suite, as data
        "djinterp test_handler unit tests",               // report title
        "The DTest session root: a typed event_dispatcher composed with "
        "per-session counters and a tree walker.",
        opts);                                            // options: one PDF
}
