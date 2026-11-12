/*******************************************************************************
* djinterp [test]                            document_templates_tests_runner.cpp
*
*   The runner for the document-template suite: one module per header under
* test, each predicate registered with report_builder, and the report written
* to the console and to document_templates_tests_report.pdf.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/text/document_templates/document_templates_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.10
*                                                            revised: 2026.11.10
*******************************************************************************/

// djinterp
#include "document_templates_tests.hpp"  // through the leaf's INCLUDES
#include "djinterp/cpp/modules/test/output/test_report_runner.hpp"


int main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- document templates");
    rb.set_subtitle("the dialect-agnostic document-template subframework");
    rb.set_author("teer");
    rb.set_description(
        "Attributes, the renderer protocol in its plain realization, the "
        "table content model and the title page, observed through the "
        "plain renderer's text; then the PDF face over DPdfCanvas, driven "
        "end to end and checked to emit a well-formed PDF.");

    rb.use_pdf("document_templates_tests_report.pdf");

    // -- I. document_attributes -------------------------------------------
    rb.module("I. document_attributes",
              "the open kv hint bag: keys, flags, tokens");
    rb.run("set and read",              &tests_attr_set_and_read);
    rb.run("flag parsing",              &tests_attr_flag_parsing);
    rb.run("align token round trip",    &tests_align_token_roundtrip);
    rb.run("attr_align reads its key",  &tests_attr_align_reads_key);

    // -- II. DDocumentRenderer (plain) ------------------------------------
    rb.module("II. document_renderer",
              "the abstract protocol, in its plain realization");
    rb.run("heading and paragraph",     &tests_plain_heading_and_paragraph);
    rb.run("ordered list",              &tests_plain_ordered_list);
    rb.run("key-value and rule",        &tests_plain_key_value_and_rule);
    rb.run("page break",                &tests_plain_page_break);

    // -- III. DDocumentTable ----------------------------------------------
    rb.module("III. document_table",
              "content model: build API, queries, render");
    rb.run("build and query",           &tests_table_build_queries);
    rb.run("monospace render",          &tests_table_render_monospace);
    rb.run("ragged row",                &tests_table_ragged_row);
    rb.run("make_document_table",       &tests_make_document_table_factory);

    // -- IV. DTitlePage ---------------------------------------------------
    rb.module("IV. title_page", "cover composition and knobs");
    rb.run("composition",               &tests_title_page_composition);
    rb.run("page-break toggle",         &tests_title_page_page_break_toggle);
    rb.run("style-name constants",      &tests_title_page_style_name_constants);

    // -- V. DPdfCanvasDocumentRenderer ------------------------------------
    rb.module("V. DPdfCanvasDocumentRenderer",
              "the PDF face over DPdfCanvas, end to end");
    rb.run("title page to PDF",         &tests_pdf_renders_title_page);
    rb.run("table to PDF",              &tests_pdf_renders_table);
    rb.run("brace text escaped",        &tests_pdf_handles_brace_text);
    rb.run("registered style resolves", &tests_pdf_resolves_registered_style);

    return rb.finish();
}
