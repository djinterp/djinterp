/******************************************************************************
* djinterp [test]                                       test_report_document.hpp
*
*   The report's PAGE DESIGN, expressed as a document template rather than a
* painter -- the piece that lets test_render_pdf_table.hpp's bespoke PDF
* composition retire.
*
*   WHAT WAS LEFT.  That header held two different things: a table engine and a
* page design.  The table half (per-cell fills and colours, ruled cells, header
* re-draw across page breaks) moved into pdf_canvas::table + cell_format, which
* is where a table engine belongs.  What remained was composition -- a title
* block, a dark suite band per module, a metadata block, a card per test -- and
* composition is a TEMPLATE: a sequence of semantic calls, not a sequence of
* rectangles.
*
*   THE PAYOFF OF SAYING IT SEMANTICALLY.  The painter could only ever produce
* PDF, because it spoke in fills and glyph placements.  The same design written
* as document_renderer calls renders as PDF (canvas-backed: real bands, ruled
* tables), as HTML (bands become styled divs, tables become <table>), as
* Markdown, and as plain monospace -- from one description.  A band is
* `heading` carrying a `background` hint; a card is a heading plus a table.
* Every dialect honours what it can and drops the rest, which is the whole of
* the hint contract.
*
*   THE PALETTE IS DATA (report_palette), not constants baked into draw calls,
* so a caller restyles without touching the composition -- and a dialect that
* cannot colour simply never reads it.  The default is the slate palette the
* painter used, so the PDF output stays recognisably the same document.
*
*   WHAT IT IS NOT.  This does not decide page geometry, margins or fonts: those
* belong to the renderer a caller hands in (pdf_canvas_document_renderer's page
* size and body style, the markup profile, and so on).  The template describes
* the DOCUMENT; the renderer decides the medium.
*
*   PORTABILITY:
*   C++11 baseline (document_renderer's floor).  No PDF dependency -- this
* header is renderer-agnostic and a text-only build includes it happily.
*
*
* TABLE OF CONTENTS
* =================
* I.    report_palette              (the colour vocabulary, as data)
* II.   report_document_options     (what to include)
* III.  render_report_document      (the composition)
*
*
* path:      /inc/djinterp/test/output/test_report_document.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.24
******************************************************************************/

#ifndef DJINTERP_TEST_REPORT_DOCUMENT_
#define DJINTERP_TEST_REPORT_DOCUMENT_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../../core/djinterp.hpp"                  // NS_*, D_NODISCARD
#include "../../core/util/document/templates/document_renderer.hpp"
                                                    // document_renderer
#include "../../core/util/document/templates/document_attributes.hpp"
                                                    // doc_attributes, doc_attr_*
#include "./test_report_content.hpp"                // the emitters + key scheme
#include "./test_report.hpp"                        // test_report + the model


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   report_palette                                      ///
///////////////////////////////////////////////////////////////////////////////

// report_palette
//   struct: the document's colour vocabulary as "#RRGGBB" tokens.  Held as
// data so a caller restyles without touching the composition below, and so a
// dialect that cannot colour simply never reads it.  Empty means "no ground" --
// a band with an empty fill renders as an ordinary heading.
struct report_palette
{
    // suite_band
    //   the ground behind a module's heading -- the painter's dark slate.
    std::string suite_band;

    // suite_text
    //   the type colour on that ground.
    std::string suite_text;

    // card_band
    //   the lighter ground behind a unit's heading.
    std::string card_band;

    // card_text
    std::string card_text;

    // zebra
    //   the alternate-row stripe, for a renderer that can stripe (the canvas
    // renderer's set_zebra; HTML via a stylesheet).  Not consumed here -- it is
    // carried so one palette describes the whole document.
    std::string zebra;

    // report_palette (default)
    //   the slate palette the PDF painter used, so the composed document stays
    // recognisably the same one.
    report_palette()
        : suite_band("#2F3A4A"),
          suite_text("#FFFFFF"),
          card_band ("#EEF1F5"),
          card_text ("#1B2430"),
          zebra     ("#F4F5F8")
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                II.  report_document_options                             ///
///////////////////////////////////////////////////////////////////////////////

// report_document_options
//   struct: which blocks the document carries.  Every one defaults on; a
// caller assembling its own frame (a layout term supplying the cover and the
// table of contents, say) turns off what it already provides.
struct report_document_options
{
    report_palette palette;

    // title_block
    //   the leading title / subtitle / author block.
    bool           title_block;

    // run_summary
    //   the whole-run figure block beneath the title.
    bool           run_summary;

    // suite_bands
    //   whether a module heading carries a coloured ground.
    bool           suite_bands;

    // card_bands
    //   whether a unit heading carries one.
    bool           card_bands;

    // module_summaries
    //   the per-module figure block under each suite band.
    bool           module_summaries;

    // page_break_per_module
    //   start each module on a fresh page.  Meaningful for paginated dialects;
    // a continuous one renders the break as nothing.
    bool           page_break_per_module;

    report_document_options()
        : palette             (),
          title_block         (true),
          run_summary         (true),
          suite_bands         (true),
          card_bands          (true),
          module_summaries    (true),
          page_break_per_module(false)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                III. render_report_document                              ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // band_attrs_helper
    //   helper: the hint bag for a banded heading -- a ground and a type
    // colour, both dropped when the palette leaves them empty.
    D_NODISCARD inline doc_attributes
    band_attrs_helper(
        const std::string& _fill,
        const std::string& _text,
        bool               _banded
    )
    {
        doc_attributes _attrs;

        // an unbanded (or uncoloured) heading carries no ground at all
        if ( (!_banded) ||
             _fill.empty() )
        {
            return _attrs;
        }

        _attrs.set(doc_attr_background, _fill);

        if (!_text.empty())
        {
            _attrs.set(doc_attr_color, _text);
        }

        return _attrs;
    }

NS_END  // internal


// render_report_document
//   function: compose the whole report into _renderer -- title block, run
// summary, then a banded section per module holding a card per unit, each card
// carrying its assertion table.
//
//   Renderer-agnostic by construction: hand it the canvas-backed PDF renderer
// and the bands are filled strips and the tables are ruled grids; hand it the
// HTML one and the same call sequence becomes styled divs and real <table>s.
// The document frame (begin_document / end_document) is the CALLER's, so several
// documents may be composed into one.
inline void
render_report_document(
    const test_report&             _report,
    document_renderer&             _renderer,
    const report_document_options& _opts = report_document_options()
)
{
    // --- title block ---------------------------------------------------------

    if (_opts.title_block)
    {
        _renderer.heading(std::size_t(1),
                          _report.title.empty() ? std::string("Test Report")
                                                : _report.title,
                          doc_attributes());

        if (!_report.subtitle.empty())
        {
            _renderer.paragraph(_report.subtitle, doc_attributes());
        }

        if (!_report.author.empty())
        {
            _renderer.key_value(std::string("Author"),
                                _report.author,
                                doc_attributes());
        }

        _renderer.rule(doc_attributes());
    }

    // --- run summary ---------------------------------------------------------

    if (_opts.run_summary)
    {
        emit_run_summary(_report, _renderer);
    }

    // --- one banded section per module ---------------------------------------

    for (std::size_t _m = 0; _m < _report.modules.size(); ++_m)
    {
        const report_module& _module = _report.modules[_m];

        // a fresh page per suite, when asked and when the dialect paginates.
        // Never before the first -- that would open on a blank page.
        if ( _opts.page_break_per_module &&
             (_m > std::size_t(0)) )
        {
            _renderer.page_break();
        }

        _renderer.heading(
            std::size_t(2),
            _module.name,
            internal::band_attrs_helper(_opts.palette.suite_band,
                                        _opts.palette.suite_text,
                                        _opts.suite_bands));

        if (_opts.module_summaries)
        {
            emit_module_summary(_module, _renderer);
        }

        // --- a card per unit -------------------------------------------------

        for (std::size_t _u = 0; _u < _module.units.size(); ++_u)
        {
            _renderer.heading(
                std::size_t(3),
                _module.units[_u].name,
                internal::band_attrs_helper(_opts.palette.card_band,
                                            _opts.palette.card_text,
                                            _opts.card_bands));

            emit_unit_table(_module.units[_u], _renderer);
        }
    }

    return;
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_REPORT_DOCUMENT_
