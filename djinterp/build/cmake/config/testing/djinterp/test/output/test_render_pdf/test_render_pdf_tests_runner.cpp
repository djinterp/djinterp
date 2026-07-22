/******************************************************************************
* djinterp [test]                               test_render_pdf_tests_runner.cpp
*
*   The entry point for the test_render_pdf.hpp unit suite.  Defining
* DTEST_SPEC_MODE selects the suite header's spec-provider face (declarations +
* render_pdf_spec(), no fixtures), so this translation unit contributes main()
* and the section TUs contribute the test bodies.  main() hands the spec to
* run_module, which lowers it into the six-kind tree (structural console dump)
* and projects it onto the report driving the live console and one PDF.
*
*   OUTPUT VIA TEST_OPTIONS - ONE PDF:
*   The document side rides one option set (test_options.hpp):
*     document    -> pdf                          emit PDF (not the txt default)
*     output_file -> "test_render_pdf_tests.pdf"  the document's path
*   Leave `document` at its txt default (or drop those two lines) for a
*   console-only run.  A single module: nothing to split or bundle.
*
* path:      /config/testing/djinterp/test/output/test_render_pdf_tests_runner.cpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.16
******************************************************************************/

#define DTEST_SPEC_MODE                    // suite exposes its spec provider, not fixtures

// std
#include <string>
// djinterp
#include "test_render_pdf_tests.hpp"       // render_pdf_spec() (resolved via the test include path)


int
main()
{
    namespace dt = ::djinterp::test;
    namespace tt = ::djinterp::testing;

    // configure the report through the option set: console plus one PDF.
    dt::test_option_set opts = dt::default_test_options();
    opts.set<dt::test_option::document>(dt::test_doc_type::pdf);
    opts.set<dt::test_option::output_file>(std::string("test_render_pdf_tests.pdf"));

    return dt::run_module(
        tt::render_pdf_spec(),                            // the suite, as data
        "djinterp test_render_pdf unit tests",            // report title
        "The PDF arm of the render-collapse: the styled-op layout and its "
        "default, the emission layer (brace_escape / resolve_text / emit_ops), "
        "the report/module walk and its byte entry points, and the "
        "document-renderer path (verdict cover, module-summary table, coloring).",
        opts);                                            // options: one PDF
}
