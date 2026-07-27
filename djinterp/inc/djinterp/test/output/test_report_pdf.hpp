/******************************************************************************
* djinterp [test]                                            test_report_pdf.hpp
*
*   The PDF entry points for a finished report -- the drop-in replacement for
* test_render_pdf_table.hpp's `render_report_pdf_bytes_table`, and the reason
* that file could be deleted.
*
*   WHY IT IS ITS OWN HEADER.  test_report_document.hpp describes the document
* and deliberately carries NO PDF dependency, so a text-only build includes it
* freely.  This header is the pairing of that description with the canvas-backed
* renderer, and it is the only place in the DTest output layer that pulls in
* pdf.hpp.  Same opt-in split the document layer keeps between
* document_renderer.hpp and pdf_document_renderer.hpp.
*
*   WHAT REPLACED WHAT.  The old entry point painted every cell directly on
* pdf_document: filled rectangles for backgrounds, stroked rectangles for rules,
* metric-placed glyphs, 882 lines of it, because the foundation's table helper
* could not express per-cell fills.  It can now (pdf_canvas's cell_format), and
* the page design is a template (test_report_document.hpp), so this is the whole
* of what remains: register the verdict palette, set the stripe, compose, and
* ask for the bytes.
*
*   THE SAME DOCUMENT, NOT THE SAME MILLIMETRES.  The painter hard-coded the
* design reference's margins and card metrics; the template delegates geometry
* to the renderer.  Output is recognisably the same report -- banded suites,
* ruled and shaded assertion tables, coloured verdicts, repeating headers across
* page breaks -- but it will not diff byte-for-byte against the old painter, and
* the status chips fill their cell rather than sitting inset within it.
*
*   PORTABILITY:
*   C++11 baseline (the renderer's floor).
*
*
* TABLE OF CONTENTS
* =================
* I.    report_pdf_palette          (the verdict styles + stripe, as defaults)
* II.   render_report_pdf_bytes_table (the entry point the runner calls)
*
*
* path:      /inc/djinterp/test/output/test_report_pdf.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.24
******************************************************************************/

#ifndef DJINTERP_TEST_REPORT_PDF_
#define DJINTERP_TEST_REPORT_PDF_ 1

// std
#include <string>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/util/pdf/pdf_canvas_document_renderer.hpp"
                                            // pdf_canvas_document_renderer
#include "./test_report_document.hpp"       // render_report_document
#include "./test_report.hpp"                // test_report


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   report_pdf_palette                                  ///
///////////////////////////////////////////////////////////////////////////////

// apply_report_pdf_palette
//   function: register the verdict styles the content emitters name
// (`verdict.pass` and friends, set as a `style` hint on each RESULT cell) and
// the row stripe.  Split out so a caller may restyle by calling
// register_style itself afterwards rather than forking the entry point.
inline void
apply_report_pdf_palette(
    pdf_canvas_document_renderer& _renderer
)
{
    canvas_style _pass;
    canvas_style _fail;
    canvas_style _skip;
    canvas_style _pend;
    canvas_style _error;

    _pass.color  = pdf_color(0.00, 0.50, 0.15);
    _fail.color  = pdf_color(0.80, 0.05, 0.05);
    _skip.color  = pdf_color(0.45, 0.45, 0.45);
    _pend.color  = pdf_color(0.35, 0.35, 0.55);
    _error.color = pdf_color(0.60, 0.30, 0.00);

    // a verdict reads as a chip: centred in its cell
    _pass.align = _fail.align = _skip.align = pdf_text_align::center;
    _pend.align = _error.align                = pdf_text_align::center;

    _renderer.register_style(std::string("verdict.pass"),  _pass);
    _renderer.register_style(std::string("verdict.fail"),  _fail);
    _renderer.register_style(std::string("verdict.skip"),  _skip);
    _renderer.register_style(std::string("verdict.pend"),  _pend);
    _renderer.register_style(std::string("verdict.error"), _error);

    _renderer.set_zebra(pdf_color::from_rgb255(244, 245, 248));

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  render_report_pdf_bytes_table                       ///
///////////////////////////////////////////////////////////////////////////////

// render_report_pdf_bytes_table
//   function: the serialized PDF for _report -- banded suites, a card per
// test, and a ruled, shaded, paginating assertion table per card.  Signature
// preserved from the header it replaces, so the runner's call sites are
// unchanged.
D_NODISCARD inline std::string
render_report_pdf_bytes_table(
    const test_report&             _report,
    const report_document_options& _opts = report_document_options()
)
{
    pdf_canvas_document_renderer _renderer;

    apply_report_pdf_palette(_renderer);

    _renderer.begin_document(doc_attributes());

    render_report_document(_report, _renderer, _opts);

    _renderer.end_document();

    return _renderer.to_bytes();
}


// render_report_pdf_bytes
//   function: the same document with page-per-suite pagination -- what the old
// test_render_pdf.hpp entry point produced, expressed as an option rather than
// as a second renderer and a second traversal.
D_NODISCARD inline std::string
render_report_pdf_bytes(
    const test_report& _report
)
{
    report_document_options _opts;

    _opts.page_break_per_module = true;

    return render_report_pdf_bytes_table(_report, _opts);
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_REPORT_PDF_
