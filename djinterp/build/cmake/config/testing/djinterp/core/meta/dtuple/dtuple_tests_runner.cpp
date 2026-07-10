/******************************************************************************
* djinterp [test]                                        dtuple_tests_runner.cpp
*
*   Standalone runner for the dtuple suite.  It includes the single suite
* header with DTEST_SPEC_MODE defined - which drops the header's shared
* fixtures and leaves dtuple_spec() as the entry point - collects that one
* spec, and hands it to run_module.  run_module lowers the spec into the
* six-kind test tree (structure + per-node metadata) and projects it onto a
* report driving the console report and, here, one PDF.  The twelve section
* translation units (pack / construction / modifiers / transformation /
* access / counting / splitting / utilities / selection / homogeneity / 2d /
* relations), compiled WITHOUT DTEST_SPEC_MODE, supply the
* void(test_handler&) worker definitions the spec's function pointers resolve
* to; this file is only the spec collection and the entry point.
*
*   NAMESPACES:
*   Like the test_handler suite (and unlike type_traits), the dtuple suite
* lives in ::djinterp::testing, so its provider is tt::dtuple_spec() while
* the option set and run_module come from ::djinterp::test (dt::).  Both
* aliases are therefore in play, exactly as in test_handler_tests_runner.cpp.
*
*   OUTPUT VIA TEST_OPTIONS - ONE PDF:
*   The document side rides one option set (test_options.hpp):
*     document    -> pdf                  emit PDF (not the txt default)
*     output_file -> "dtuple_tests.pdf"   the document's path
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
*   VERDICT / EXIT CODE:
*   run_module returns the report's process exit code (0 = every leaf passed),
* handed straight back from main().  A section whose worker records a failure
* or error turns its unit test red and the exit code non-zero.
*
* path:      /build/cmake/config/testing/djinterp/core/meta/dtuple_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not the fixtures

// std
#include <string>
// djinterp
#include "../../../../../../../tests/djinterp/core/meta/dtuple_tests.hpp"  // resolved via the test include path

int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    // configure the report through the option set: console plus one PDF.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("dtuple_tests.pdf"));

    return dt::run_module(
        tt::dtuple_spec(),                                // the suite, as data
        "djinterp dtuple unit tests",                     // report title
        "The compile-time tuple metafunction library: pack utilities, "
        "construction / transformation / access, counting, splitting, "
        "selection, homogeneity, and 2D / jagged tuples.",
        opts);                                            // options: one PDF
}
