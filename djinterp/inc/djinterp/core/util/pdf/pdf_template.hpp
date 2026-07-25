/******************************************************************************
* djinterp [utility]                                             pdf_template.hpp
*
*   A PDF-specialised document_template.  Everything a template IS -- an element
* sequence, `{token}` interpolation, named styles, `repeat` over a bound
* sequence, a document frame -- lives in document_template and is inherited
* unchanged.  This header adds ONLY what is genuinely PDF-specific and cannot
* live in the renderer-agnostic base:
*
*     1. PAGE GEOMETRY + MARGINS.  A pdf_page_size the output is laid onto, and
*        the four-margin box the flowing cursor writes within.  Margins are a
*        pdf_canvas property in the layout model (the canvas owns the cursor);
*        the template holds the caller's choice and installs it on the canvas at
*        render time.  This is the surface pdf_options re-targets.
*     2. STYLE REALISATION.  document_template names styles ("heading", "footer");
*        it does not know what they look like.  A PDF build binds each name to a
*        canvas_style -- the one place that holds both the abstract name and its
*        concrete PDF form, because canvas_style is a PDF type the base may not
*        mention.
*     3. DOCUMENT METADATA.  Title / author / subject / creator, recorded on the
*        canvas (which mirrors pdf_document::metadata) at render time.
*     4. RENDER ENTRY POINTS.  render_pdf() / save_pdf() / render_into(), which
*        drive the surviving PDF renderer (pdf_canvas_document_renderer, which
*        measures with real font metrics and owns pagination) over the inherited
*        element list.
*
*   WHAT MOVED, AND WHERE IT WENT.  The former pdf_template was a flow-layout
* engine: cursor, line advancement, pagination, a style REGISTRY, an element
* list, and %key% substitution.  Layout, the cursor and the margin box are
* pdf_canvas's; the element list, style NAMING and token substitution are
* document_template's; what is left here is the thin PDF face over both.  There
* is now one layout engine in the PDF layer, not two.
*
*   BUILDING ONE.  Use the inherited document_template surface -- tmpl_heading,
* tmpl_paragraph, tmpl_repeat, add(), frame() -- then bind styles and render:
*
*       pdf_template t;
*       t.add(tmpl_heading("{report.title}", 1, "h1"))
*        .add(tmpl_repeat("cases", "case",
*                { tmpl_key_value("{case.name}", "{case.status}", "kv") }));
*       t.style("h1", canvas_style::heading(18))
*        .style("kv", canvas_style::body(11));
*
*       template_context ctx;                 // values / sequences / slots
*       ctx.values    = ...; ctx.sequences = ...;
*       const std::string bytes = t.render_pdf(ctx);
*
*   PORTABILITY:
*   C++11 baseline -- the floors of document_template and pdf_canvas.  No
* third-party dependency in this header; the PDF backend is chosen inside
* pdf_canvas as before.
*
*
* path:      /inc/djinterp/core/util/pdf/pdf_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.25
******************************************************************************/

#ifndef DJINTERP_UTIL_PDF_TEMPLATE_
#define DJINTERP_UTIL_PDF_TEMPLATE_ 1

// std
#include <map>
#include <string>

// djinterp
#include "../../djinterp.hpp"                        // NS_*, D_NODISCARD, D_NOEXCEPT
#include "../document/document_template.hpp"         // document_template, template_context,
                                                     // template_element, render_template
#include "./pdf_canvas.hpp"                          // pdf_page_size, pdf_unit, canvas_style,
                                                     // pdf_canvas
#include "./pdf_canvas_document_renderer.hpp"        // pdf_canvas_document_renderer


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                     pdf_template                                        ///
///////////////////////////////////////////////////////////////////////////////

// pdf_template
//   class: a document_template whose output is a PDF.  Adds page geometry and
// margins, the name -> canvas_style bindings, document metadata, and the
// render-to-PDF entry points; inherits the whole templating surface (elements,
// tokens, repeat, frame) from the base.
class pdf_template
    : public document_template
{
public:

    using style_map = std::map<std::string, canvas_style>;
    using meta_map  = std::map<std::string, std::string>;

    // ------------------------------------------------------------------
    //  construction
    // ------------------------------------------------------------------

    pdf_template()
        : document_template(),
          m_page(pdf_page_size::letter()),
          m_margin_left(54.0),
          m_margin_right(54.0),
          m_margin_top(54.0),
          m_margin_bottom(54.0),
          m_have_margins(false),
          m_body(),
          m_have_body(false),
          m_styles(),
          m_meta()
    {}

    explicit pdf_template(
        const pdf_page_size& _page
    )
        : document_template(),
          m_page(_page),
          m_margin_left(54.0),
          m_margin_right(54.0),
          m_margin_top(54.0),
          m_margin_bottom(54.0),
          m_have_margins(false),
          m_body(),
          m_have_body(false),
          m_styles(),
          m_meta()
    {}

    // ------------------------------------------------------------------
    //  PDF-specific configuration
    // ------------------------------------------------------------------

    // page
    //   the page geometry the document is laid onto.  This, with margins(), is
    // what a pdf_options / pdf_config re-targets.  Chainable, to match the
    // document_template builders.
    pdf_template&
    page(
        const pdf_page_size& _page
    )
    {
        m_page = _page;

        return (*this);
    }

    D_NODISCARD const pdf_page_size&
    page() const D_NOEXCEPT
    {
        return m_page;
    }

    // margins
    //   the writable box the flowing cursor stays within, in points.  Installed
    // on the canvas at render time (see pdf_canvas::set_margins).  Left unset,
    // the canvas keeps its own default -- so a caller that does not care pays
    // nothing and inherits sensible margins.
    pdf_template&
    margins(
        pdf_unit _left,
        pdf_unit _right,
        pdf_unit _top,
        pdf_unit _bottom
    )
    {
        m_margin_left   = _left;
        m_margin_right  = _right;
        m_margin_top    = _top;
        m_margin_bottom = _bottom;
        m_have_margins  = true;

        return (*this);
    }

    // style
    //   bind a style NAME (as emitted by the inherited elements' `style` hint)
    // to its concrete canvas_style.  A name left unbound falls back to the body
    // style in the renderer, so a producer may emit names a given build does
    // not define -- the same tolerance document_template assumes.
    pdf_template&
    style(
        const std::string&  _name,
        const canvas_style& _style
    )
    {
        m_styles[_name] = _style;

        return (*this);
    }

    // body_style
    //   the style every block starts from before its own hints refine it.
    // Optional: unset leaves the renderer's own default in place.
    pdf_template&
    body_style(
        const canvas_style& _style
    )
    {
        m_body      = _style;
        m_have_body = true;

        return (*this);
    }

    // metadata
    //   a document-info entry (e.g. "Title", "Author", "Subject", "Creator"),
    // recorded on the canvas at render time (pdf_canvas::metadata mirrors
    // pdf_document::metadata).  Additive; last write per key wins.
    pdf_template&
    metadata(
        const std::string& _key,
        const std::string& _value
    )
    {
        m_meta[_key] = _value;

        return (*this);
    }

    D_NODISCARD const style_map&
    styles() const D_NOEXCEPT
    {
        return m_styles;
    }

    // ------------------------------------------------------------------
    //  render entry points
    // ------------------------------------------------------------------

    // render_pdf
    //   render the inherited skeleton against _context and return the
    // serialized PDF bytes.  Drives a freshly-owned canvas on this template's
    // page and margins; the base's render_template does begin_document(frame())
    // -> elements -> end_document.
    D_NODISCARD std::string
    render_pdf(
        const template_context& _context
    ) const
    {
        pdf_canvas_document_renderer _renderer(m_page);
        configure_(_renderer, /*owned_canvas=*/true);
        render_template(*this, _context, _renderer);

        return _renderer.to_bytes();
    }

    // save_pdf
    //   render as render_pdf, but write the bytes to _path.  Returns the
    // renderer's own success result.
    D_NODISCARD bool
    save_pdf(
        const std::string&      _path,
        const template_context& _context
    ) const
    {
        pdf_canvas_document_renderer _renderer(m_page);
        configure_(_renderer, /*owned_canvas=*/true);
        render_template(*this, _context, _renderer);

        return _renderer.save(_path);
    }

    // render_into
    //   render into a caller-owned canvas, so a caller may lay out its own
    // preamble, hand the canvas over for the document body, and take it back.
    // The page geometry and margins are the canvas's, not this template's, so
    // they are left untouched; styles and metadata are still applied.
    void
    render_into(
        pdf_canvas&             _canvas,
        const template_context& _context
    ) const
    {
        pdf_canvas_document_renderer _renderer(_canvas);
        configure_(_renderer, /*owned_canvas=*/false);
        render_template(*this, _context, _renderer);

        return;
    }

private:

    // configure_
    //   install this template's PDF-specific bindings onto a renderer before a
    // render: margins and metadata onto the driven canvas (margins only when
    // the renderer owns its canvas -- a borrowed canvas keeps the caller's box),
    // then the body style and every named style.
    void
    configure_(
        pdf_canvas_document_renderer& _renderer,
        bool                          _owned_canvas
    ) const
    {
        if (_owned_canvas && m_have_margins)
        {
            _renderer.canvas().set_margins(
                m_margin_left,
                m_margin_right,
                m_margin_top,
                m_margin_bottom);
        }

        for (meta_map::const_iterator _it = m_meta.begin();
             _it != m_meta.end();
             ++_it)
        {
            _renderer.canvas().metadata(_it->first, _it->second);
        }

        if (m_have_body)
        {
            _renderer.set_body_style(m_body);
        }

        for (style_map::const_iterator _it = m_styles.begin();
             _it != m_styles.end();
             ++_it)
        {
            _renderer.register_style(_it->first, _it->second);
        }

        return;
    }

    pdf_page_size m_page;
    pdf_unit      m_margin_left;
    pdf_unit      m_margin_right;
    pdf_unit      m_margin_top;
    pdf_unit      m_margin_bottom;
    bool          m_have_margins;
    canvas_style  m_body;
    bool          m_have_body;
    style_map     m_styles;
    meta_map      m_meta;
};


NS_END  // djinterp


#endif  // DJINTERP_UTIL_PDF_TEMPLATE_
