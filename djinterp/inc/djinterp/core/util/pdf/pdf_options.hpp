/*******************************************************************************
* djinterp [core]                                                pdf_options.hpp
*
*   The configuration vocabulary for the PDF subsystem: one description of "how
* a PDF should look" - page geometry, default text presentation, and document
* metadata - that configures EITHER authoring surface (a pdf_template's flow
* layout or a pdf_document's drawing defaults) at EITHER binding time.
*
*   A SINGLE RUNTIME FACE:
*     a concrete `pdf_options` aggregate of plain fields; build it, set what you
*   care about, and apply_to(...) a target.  Every field is an ordinary value, so
*   a parser or a config file can populate it at the boundary.  Everything
*   downstream takes only a `pdf_options`.
*
*   WHAT IT CONFIGURES:
*     pdf_template   layout (page size + four margins) and the body text style
*                    (font, color, alignment, leading, cell indent) - the full
*                    presentation of the flow engine.
*     pdf_document   document metadata (title / author / subject / creator).
*                    A pdf_document draws at explicit points with per-call page
*                    sizes and text options rather than storing layout, so the
*                    page size and a ready-made pdf_text_options are EXPOSED
*                    (page(), to_text_options()) for the caller's add_page /
*                    text calls, while apply_to(pdf_document&) sets the one
*                    thing the façade does store - the Info metadata.
*
*   PRESETS vs VALUES:
*   Page size and text color are open value types (any custom extent / device
* color) at runtime, but the schema selects them from small preset enums
* (pdf_page_preset / pdf_color_preset) so they are clean non-type template
* arguments - exactly as a value enum stands in for a richer runtime type in
* test_options.  The presets lower through page_of_preset / color_of_preset,
* which the runtime face also exposes as convenience setters.
*
*   PORTABILITY:
*   The runtime core (PART A) and the application glue follow the PDF module's
* floor.  The schema and its lowering (PARTS B/C) ride the same C++20 /
* class-type-NTTP gate as the test_options vocabulary; below it the runtime
* `pdf_options` remains the portable path.
*
*
* CONTENTS
* ========
*   I.    presets                  (pdf_page_preset / pdf_color_preset + maps)
*   II.   pdf_options              (the aggregate: fields, setters, views)
*   III.  application              (apply_to / make_template)
*
*
* path:      /inc/djinterp/core/util/pdf/pdf_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.25
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_UTIL_PDF_PDF_OPTIONS_HPP
#define DJINTERP_UTIL_PDF_PDF_OPTIONS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <string>
// djinterp
#include "../../../djinterp.hpp"
#include "./pdf.hpp"                        // pdf_primitives + pdf_document façade
#include "./pdf_template.hpp"               // pdf_template


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                                                                         ///
///                        PART A - RUNTIME CORE                            ///
///                                                                         ///
///////////////////////////////////////////////////////////////////////////////


// ===========================================================================
// A.I.  PRESETS
// ===========================================================================

// pdf_page_preset
//   enum: a named page extent for the schema and the convenience setters.  The
// runtime field is an open pdf_page_size (any custom extent); this names the
// standard presets so a page choice can be a non-type template argument.
enum class pdf_page_preset
{
    letter,
    legal,
    tabloid,
    a3,
    a4,
    a5
};

// pdf_color_preset
//   enum: a named device color, the schema/​setter counterpart of the open
// pdf_color runtime field.
enum class pdf_color_preset
{
    black,
    white,
    red,
    green,
    blue,
    gray
};

// page_of_preset
//   function: the pdf_page_size a pdf_page_preset names.
D_INLINE pdf_page_size
page_of_preset(
    pdf_page_preset _preset
)
{
    switch (_preset)
    {
        case pdf_page_preset::letter:  return pdf_page_size::letter();
        case pdf_page_preset::legal:   return pdf_page_size::legal();
        case pdf_page_preset::tabloid: return pdf_page_size::tabloid();
        case pdf_page_preset::a3:      return pdf_page_size::a3();
        case pdf_page_preset::a4:      return pdf_page_size::a4();
        case pdf_page_preset::a5:      return pdf_page_size::a5();
    }

    return pdf_page_size::letter();
}

// color_of_preset
//   function: the pdf_color a pdf_color_preset names.
D_INLINE pdf_color
color_of_preset(
    pdf_color_preset _preset
)
{
    switch (_preset)
    {
        case pdf_color_preset::black: return pdf_color::black();
        case pdf_color_preset::white: return pdf_color::white();
        case pdf_color_preset::red:   return pdf_color::red();
        case pdf_color_preset::green: return pdf_color::green();
        case pdf_color_preset::blue:  return pdf_color::blue();
        case pdf_color_preset::gray:  return pdf_color::gray();
    }

    return pdf_color::black();
}


// ===========================================================================
// A.II. pdf_options
// ===========================================================================

// pdf_options
//   struct: the whole PDF configuration as plain values - page geometry, the
// default text presentation (the template body style / the document text
// default), and document metadata.  Defaults match the pdf_template defaults
// exactly, so a default-constructed pdf_options applied to a fresh template is
// a no-op.  Fields are public (set them directly); the fluent setters and the
// `to_*` views are conveniences.  Strings left empty are "unset".
struct pdf_options
{
    // -- page geometry -------------------------------------------------------
    pdf_page_size  page;            // page extent (default letter)
    pdf_unit       margin_left;     // points (default 54; the canvas margin)
    pdf_unit       margin_right;    // points
    pdf_unit       margin_top;      // points
    pdf_unit       margin_bottom;   // points

    // -- default text presentation -------------------------------------------
    pdf_base_font  font_family;     // base-14 face (default courier)
    pdf_unit       font_size;       // points (default 11; the canvas body size)
    pdf_color      text_color;      // device color (default black)
    pdf_text_align align;           // horizontal alignment (default left)
    pdf_unit       leading;         // line leading, points (default LEADING)
    std::size_t    indent_cells;    // left indent in character-cells (default 0)

    // -- document metadata (Info dictionary; empty = unset) ------------------
    std::string    title;
    std::string    author;
    std::string    subject;
    std::string    creator;

    pdf_options()
        : page(pdf_page_size::letter()),
          margin_left(54.0),
          margin_right(54.0),
          margin_top(54.0),
          margin_bottom(54.0),
          font_family(pdf_base_font::courier),
          font_size(11.0),
          text_color(pdf_color::black()),
          align(pdf_text_align::left),
          leading(14.0),
          indent_cells(0)
    {}

    // -- fluent setters (return *this so calls chain) ------------------------

    // set_page
    //   choose the page extent from a preset.
    pdf_options&
    set_page(
        pdf_page_preset _preset
    )
    {
        page = page_of_preset(_preset);

        return *this;
    }

    // set_margins
    //   set all four margins to one value.
    pdf_options&
    set_margins(
        pdf_unit _all
    )
    {
        margin_left   = _all;
        margin_right  = _all;
        margin_top    = _all;
        margin_bottom = _all;

        return *this;
    }

    // set_margins
    //   set the four margins individually.
    pdf_options&
    set_margins(
        pdf_unit _left,
        pdf_unit _right,
        pdf_unit _top,
        pdf_unit _bottom
    )
    {
        margin_left   = _left;
        margin_right  = _right;
        margin_top    = _top;
        margin_bottom = _bottom;

        return *this;
    }

    // set_font
    //   set the default face and size.
    pdf_options&
    set_font(
        pdf_base_font _family,
        pdf_unit      _size
    )
    {
        font_family = _family;
        font_size   = _size;

        return *this;
    }

    // set_color
    //   set the default text color from a preset.
    pdf_options&
    set_color(
        pdf_color_preset _preset
    )
    {
        text_color = color_of_preset(_preset);

        return *this;
    }

    // -- projections (the views the targets consume) -------------------------

    // font
    //   the default text run's pdf_font (family + size).
    D_NODISCARD pdf_font
    font() const D_NOEXCEPT
    {
        return pdf_font(font_family, font_size);
    }

    // to_canvas_style
    //   the default presentation as a canvas_style (font, color, alignment,
    // leading) - the body style the PDF renderer starts each block from.  Cell
    // indent is a per-element / structural concern the canvas owns, not a
    // body-style default, so indent_cells is not folded in here.
    D_NODISCARD canvas_style
    to_canvas_style() const
    {
        canvas_style _style(font());

        _style.color   = text_color;
        _style.align   = align;
        _style.leading = leading;

        return _style;
    }

    // to_text_options
    //   the default presentation as a foundation pdf_text_options (font, color,
    // alignment, leading) - the per-call text default for a pdf_document.  Cell
    // indent is a layout concept and is not carried here, matching
    // canvas_style::to_text_options.
    D_NODISCARD pdf_text_options
    to_text_options() const
    {
        pdf_text_options _opts(font());

        _opts.color   = text_color;
        _opts.align   = align;
        _opts.leading = leading;

        return _opts;
    }


    // ===================================================================
    // A.III. APPLICATION
    // ===================================================================

    // apply_to (pdf_template)
    //   configure a pdf_template: install the page geometry, the four margins,
    // the body style, and the document metadata.  Element-level styling and
    // bindings remain the caller's; this sets the document-wide presentation a
    // fresh template would otherwise leave at its defaults.
    void
    apply_to(
        pdf_template& _tpl
    ) const
    {
        _tpl.page(page);
        _tpl.margins(margin_left, margin_right, margin_top, margin_bottom);
        _tpl.body_style(to_canvas_style());

        if (!title.empty())   { _tpl.metadata("Title",   title);   }
        if (!author.empty())  { _tpl.metadata("Author",  author);  }
        if (!subject.empty()) { _tpl.metadata("Subject", subject); }
        if (!creator.empty()) { _tpl.metadata("Creator", creator); }

        return;
    }

    // apply_to (pdf_document)
    //   configure a document façade: record the metadata fields the caller has
    // set (a pdf_document stores no layout, so page size and text options are
    // taken per call - see page() / to_text_options()).  Setting metadata
    // opens the document lazily, exactly as pdf_document::metadata does.
    void
    apply_to(
        pdf_document& _doc
    ) const
    {
        if (!title.empty())   { _doc.metadata("Title",   title);   }
        if (!author.empty())  { _doc.metadata("Author",  author);  }
        if (!subject.empty()) { _doc.metadata("Subject", subject); }
        if (!creator.empty()) { _doc.metadata("Creator", creator); }

        return;
    }
};


// make_template
//   function: a pdf_template built and configured from a pdf_options in one
// call - constructed, then given the option set's page, margins, body style,
// and metadata via apply_to.
D_NODISCARD D_INLINE pdf_template
make_template(
    const pdf_options& _opts
)
{
    pdf_template _tpl;

    _opts.apply_to(_tpl);

    return _tpl;
}




NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_PDF_PDF_OPTIONS_HPP
