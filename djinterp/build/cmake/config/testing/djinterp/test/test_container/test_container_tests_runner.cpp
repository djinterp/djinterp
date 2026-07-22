/******************************************************************************
* djinterp [test]                             test_container_tests_runner.cpp
*
*   Standalone runner for the test_container suite.  It includes the single
* suite header with DTEST_SPEC_MODE defined - which drops the header's
* fixtures and check helper and exposes its module_spec provider instead -
* collects that one spec, and hands it to run_module.  run_module lowers the
* spec into the six-kind test_tree (view 1: structure + per-node metadata) and
* projects the same spec onto a report_builder driving the console report and,
* here, one PDF (view 2: the roll-up the document renders).  The four section
* translation units (probes / element / contract / concepts), compiled WITHOUT
* DTEST_SPEC_MODE, supply the bool() definitions the spec's function pointers
* resolve to; this file is only the spec collection and the entry point.
*
*   OUTPUT VIA TEST_OPTIONS - ONE PDF:
*   The document side rides one option set (test_options.hpp):
*     document    -> pdf                       emit PDF (not the txt default)
*     output_file -> "test_container_tests.pdf" the document's path
*   Leave `document` at its txt default (or drop these two lines) for a
*   console-only run.  Because this is a SINGLE module there is nothing to
*   split or bundle, so unlike the event aggregate runner it defines no
*   D_TEST_REPORT_ENABLE_ARCHIVE and sets no split / pack / archive_format.
*
*   FINDING THE FILE:
*   output_file is a RELATIVE name, so the .pdf lands in the process's current
* working directory - under Visual Studio the Debugging "Working Directory",
* NOT the folder holding the .exe.  Set output_file to an absolute path to pin
* it, or watch the console for the "wrote report: ..." line.
*
*   The PDF stack is C++17-gated: below C++17 the document is skipped and the
* run stays console-only (the suite itself compiles to C++11).  To run against
* another emit target, change document / output_file the documented way; no
* other change is needed.
*
* path:      /build/cmake/config/testing/djinterp/test/test_container/test_container_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "../../../../../../../tests/djinterp/test/test_container/test_container_tests.hpp"  // resolved via the test include path

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    // configure the report through the option set: console plus one PDF.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_container_tests.pdf"));

    return dt::run_module(
        tt::container_spec(),                             // the suite, as data
        "djinterp test_container unit tests",             // report title
        "The test_container contract: structural probes, the guarded element "
        "protocol, the read/run/build ladder, and the C++20 concept mirror.",
        opts);                                            // options: one PDF
}
