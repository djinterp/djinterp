/******************************************************************************
* djinterp [test]                            test_emit_tests_runner.cpp
*
*   Standalone runner for the test_emit suite.  It includes the single suite
* header with DTEST_SPEC_MODE defined - which drops the header's fixtures and
* check helper and exposes its module_spec provider instead - collects that one
* spec, and hands it to run_module.  run_module lowers the spec into the test
* tree (structure + per-node metadata) and projects it onto a report driving
* the console report and, here, one PDF.  The four section translation units
* (tunes / targets / sinks / builder), compiled WITHOUT DTEST_SPEC_MODE, supply
* the bool() definitions the spec's function pointers resolve to; this file is
* only the spec collection and the entry point.
*
*   OUTPUT VIA TEST_OPTIONS - ONE PDF:
*   The document side rides one option set (test_options.hpp):
*     document    -> pdf                    emit PDF (not the txt default)
*     output_file -> "test_emit_tests.pdf"  the document's path
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
* path:      /build/cmake/config/testing/djinterp/test/output/test_emit_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "../../../../../../../tests/djinterp/test/output/test_emit_tests.hpp"  // resolved via the test include path

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    // configure the report through the option set: console plus one PDF.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_emit_tests.pdf"));

    return dt::run_module(
        tt::emit_spec(),                                  // the suite, as data
        "djinterp test_emit unit tests",                  // report title
        "The output surface over a finished run: option tunes, the target "
        "algebra, the terminal sinks, and the builder that lowers both faces "
        "into the one bundle/packaging engine.",
        opts);                                            // options: one PDF
}
