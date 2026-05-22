/******************************************************************************
* djinterp [pdf]                                              pdf_template.hpp
*
*   Flow-layout PDF template engine.  Where pdf.hpp draws at explicit points
* in bottom-left user space, pdf_template lays content out top-to-bottom with
* automatic line advancement and pagination, and resolves named specifiers
* through the existing text_template engine.  It is the PDF analogue of
* text_template.hpp: a feature-rich layer over a minimal foundation.
*
*   COMPOSITION OVER REINVENTION:
*   Token substitution (%key%, sections, lists, conditionals, transforms) is
* NOT reimplemented here.  Each text element carries a format string that is
* run through a borrowed text_template before being placed, so the full
* binding surface of text_template is available for PDF content with zero
* duplication.  The template owns no PDF library knowledge - it drives the
* agnostic pdf_document façade, which in turn drives whatever pdf_backend is
* installed (built-in, libHaru, PDFHummus, ...).
*
*   LAYOUT MODEL:
*   A cursor descends from the top margin.  Each emitted line consumes one
* leading-step of vertical space; when the cursor would cross the bottom
* margin a new page is begun and the cursor resets to the top.  Horizontal
* placement honors the element's alignment (left / center / right) within the
* content width, and left indentation is expressed in fixed character-cells
* of the active font so test-tree depth maps cleanly to indials.
*
*   ELEMENT MODEL:
*   The template is a sequence of elements, each one of:
*     text       - a format string rendered and placed as one or more lines
*     rule        - a horizontal separator line
*     vspace      - blank vertical space
*     page_break  - forces a new page
*   Elements are appended in document order and emitted by render_to_pdf().
*
*   STYLES:
*   A pdf_text_style bundles font, color, alignment, indentation, and leading.
* Named styles can be registered and referenced by element, so a caller can
* theme headings, body text, and footers once and reuse them.
*
*   RENDERABLE PROTOCOL (see pdf_template_traits.hpp):
*     render_to_pdf(pdf_document&) - draw into a caller-supplied document
*     render_pdf()                 - serialize a fresh document to bytes
*     save_pdf(path)               - serialize and write to disk
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h / env_*.h for version detection and djinterp.hpp
* for namespace and constexpr support.  No third-party dependency in this
* header or its foundation.
*
*
* TABLE OF CONTENTS
* =================
* I.    DEFAULTS
* II.   TEXT STYLE
* III.  PAGE LAYOUT
* IV.   PDF TEMPLATE
*       a. construction
*       b. page / margin configuration
*       c. style registration
*       d. binding passthrough (text_template surface)
*       e. element appension
*       f. render_to_pdf
*       g. render_pdf / save_pdf
*       h. clear / query
* V.    CONVENIENCE FACTORIES
*
*
* path:      /inc/djinterp/core/pdf/pdf_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_PDF_TEMPLATE_
#define DJINTERP_PDF_TEMPLATE_ 1

// std
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../text/text_template.hpp"
#include "./pdf.hpp"
#include "./pdf_template_traits.hpp"


NS_DJINTERP
NS_PDF


///////////////////////////////////////////////////////////////////////////////
///                I.   DEFAULTS                                            ///
///////////////////////////////////////////////////////////////////////////////

#ifndef D_PDF_TPL_DEFAULT_MARGIN
    #define D_PDF_TPL_DEFAULT_MARGIN     54.0
#endif

#ifndef D_PDF_TPL_DEFAULT_FONT_SIZE
    #define D_PDF_TPL_DEFAULT_FONT_SIZE  10.0
#endif

#ifndef D_PDF_TPL_DEFAULT_LEADING
    #define D_PDF_TPL_DEFAULT_LEADING    12.0
#endif

#ifndef D_PDF_TPL_DEFAULT_PREFIX
    #define D_PDF_TPL_DEFAULT_PREFIX     "{"
#endif

#ifndef D_PDF_TPL_DEFAULT_SUFFIX
    #define D_PDF_TPL_DEFAULT_SUFFIX     "}"
#endif


///////////////////////////////////////////////////////////////////////////////
///                II.  TEXT STYLE                                          ///
///////////////////////////////////////////////////////////////////////////////

// pdf_text_style
//   struct: a reusable bundle of text-presentation parameters -
// font, color, alignment, leading, and a left indentation given in
// whole character-cells of the font (so depth maps to columns).
struct pdf_text_style
{
    pdf_font       font;
    pdf_color      color;
    pdf_text_align align;
    pdf_unit       leading;
    std::size_t    indent_cells;

    pdf_text_style()
        : font(pdf_base_font::courier, D_PDF_TPL_DEFAULT_FONT_SIZE),
          color(pdf_color::black()),
          align(pdf_text_align::left),
          leading(D_PDF_TPL_DEFAULT_LEADING),
          indent_cells(0)
    {}

    explicit pdf_text_style(
        const pdf_font& _font
    )
        : font(_font),
          color(pdf_color::black()),
          align(pdf_text_align::left),
          leading(_font.size * 1.2),
          indent_cells(0)
    {}

    // to_text_options
    //   projects this style onto the foundation's pdf_text_options
    // (indentation is consumed by the layout engine, not the
    // foundation, so it is not carried across).
    pdf_text_options
    to_text_options() const
    {
        pdf_text_options opts(font);

        opts.color   = color;
        opts.align   = align;
        opts.leading = leading;

        return opts;
    }

    // cell_width
    //   the horizontal advance of one character-cell at this
    // style's font and size (exact for Courier).
    pdf_unit
    cell_width() const D_NOEXCEPT
    {
        return ( font.size * average_advance_factor(font.family) );
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. PAGE LAYOUT                                         ///
///////////////////////////////////////////////////////////////////////////////

// pdf_layout
//   struct: page geometry for the flow engine - page size and the
// four margins, all in points.  Provides derived content-box
// helpers used during pagination and alignment.
struct pdf_layout
{
    pdf_page_size page;
    pdf_unit      margin_left;
    pdf_unit      margin_right;
    pdf_unit      margin_top;
    pdf_unit      margin_bottom;

    pdf_layout()
        : page(pdf_page_size::letter()),
          margin_left(D_PDF_TPL_DEFAULT_MARGIN),
          margin_right(D_PDF_TPL_DEFAULT_MARGIN),
          margin_top(D_PDF_TPL_DEFAULT_MARGIN),
          margin_bottom(D_PDF_TPL_DEFAULT_MARGIN)
    {}

    explicit pdf_layout(
        const pdf_page_size& _page
    )
        : page(_page),
          margin_left(D_PDF_TPL_DEFAULT_MARGIN),
          margin_right(D_PDF_TPL_DEFAULT_MARGIN),
          margin_top(D_PDF_TPL_DEFAULT_MARGIN),
          margin_bottom(D_PDF_TPL_DEFAULT_MARGIN)
    {}

    // content_width
    //   horizontal space between the side margins.
    pdf_unit
    content_width() const D_NOEXCEPT
    {
        return (page.size.width - margin_left - margin_right);
    }

    // content_top
    //   y of the first baseline region (top margin, from the top
    // of the page in bottom-left user space).
    pdf_unit
    content_top() const D_NOEXCEPT
    {
        return (page.size.height - margin_top);
    }

    // content_bottom
    //   y below which content must not be placed.
    pdf_unit
    content_bottom() const D_NOEXCEPT
    {
        return margin_bottom;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  PDF TEMPLATE                                        ///
///////////////////////////////////////////////////////////////////////////////

// pdf_template
//   class: flow-layout PDF document template.  Holds an ordered
// list of elements (text, rules, spacing, page breaks), a borrowed
// text_template for token resolution, a style registry, and page
// geometry.  render_to_pdf() walks the elements, resolving and
// placing each one with automatic line wrapping and pagination.
//
// Usage:
//   pdf_template tpl;
//   tpl.bind("suite", "container");
//   tpl.add_text("DTest report: {suite}", tpl.heading_style());
//   tpl.add_rule();
//   tpl.add_text("  [PASS] vector basics");
//   tpl.save_pdf("report.pdf");
class pdf_template
{
public:
    using size_type = std::size_t;

private:
    // element_kind
    //   enum: discriminator for a flow element.
    enum class element_kind
    {
        text       = 0,
        rule       = 1,
        vspace     = 2,
        page_break = 3
    };

    // element
    //   struct: one flow element.  A flat record keeps storage
    // C++11-trivial; only the fields relevant to `kind` are read.
    struct element
    {
        element_kind   kind;
        std::string    format;      // text: format string
        pdf_text_style style;       // text / rule styling
        std::string    style_name;  // optional named-style reference
        pdf_unit       amount;      // vspace: points; rule: line width

        element()
            : kind(element_kind::text),
              format(),
              style(),
              style_name(),
              amount(0.0)
        {}
    };

    // named_style
    //   struct: a registered style addressable by name.
    struct named_style
    {
        std::string    name;
        pdf_text_style style;
    };

public:
    // =================================================================
    //  a. construction
    // =================================================================

    pdf_template()
        : m_layout(),
          m_tmpl(D_PDF_TPL_DEFAULT_PREFIX, D_PDF_TPL_DEFAULT_SUFFIX),
          m_elements(),
          m_styles(),
          m_body_style()
    {}

    explicit pdf_template(
        const pdf_layout& _layout
    )
        : m_layout(_layout),
          m_tmpl(D_PDF_TPL_DEFAULT_PREFIX, D_PDF_TPL_DEFAULT_SUFFIX),
          m_elements(),
          m_styles(),
          m_body_style()
    {}


    // =================================================================
    //  b. page / margin configuration
    // =================================================================

    pdf_layout&       layout()       D_NOEXCEPT { return m_layout; }
    const pdf_layout& layout() const D_NOEXCEPT { return m_layout; }

    void
    set_layout(
        const pdf_layout& _layout
    )
    {
        m_layout = _layout;

        return;
    }

    void
    set_page_size(
        const pdf_page_size& _page
    )
    {
        m_layout.page = _page;

        return;
    }

    void
    set_margins(
        pdf_unit _left,
        pdf_unit _right,
        pdf_unit _top,
        pdf_unit _bottom
    )
    {
        m_layout.margin_left   = _left;
        m_layout.margin_right  = _right;
        m_layout.margin_top    = _top;
        m_layout.margin_bottom = _bottom;

        return;
    }


    // =================================================================
    //  c. style registration
    // =================================================================

    // body_style
    //   the default style applied to text elements that do not name
    // a style of their own.
    pdf_text_style&       body_style()       D_NOEXCEPT { return m_body_style; }
    const pdf_text_style& body_style() const D_NOEXCEPT { return m_body_style; }

    void
    set_body_style(
        const pdf_text_style& _style
    )
    {
        m_body_style = _style;

        return;
    }

    // register_style
    //   records a named style for later reference by add_text.
    void
    register_style(
        const std::string&    _name,
        const pdf_text_style& _style
    )
    {
        for (size_type i = 0; i < m_styles.size(); ++i)
        {
            if (m_styles[i].name == _name)
            {
                m_styles[i].style = _style;

                return;
            }
        }

        named_style ns;
        ns.name  = _name;
        ns.style = _style;

        m_styles.push_back(static_cast<named_style&&>(ns));

        return;
    }

    // heading_style
    //   a convenience preset: bold Helvetica, slightly larger.
    static pdf_text_style
    heading_style()
    {
        pdf_text_style s(pdf_font(pdf_base_font::helvetica_bold, 14.0));
        s.leading = 18.0;

        return s;
    }

    // monospace_style
    //   a convenience preset: Courier, suited to aligned test output.
    static pdf_text_style
    monospace_style(
        pdf_unit _size = D_PDF_TPL_DEFAULT_FONT_SIZE
    )
    {
        return pdf_text_style(pdf_font(pdf_base_font::courier, _size));
    }


    // =================================================================
    //  d. binding passthrough (text_template surface)
    // =================================================================

    // The template forwards the common binding methods to the
    // borrowed text_template so callers theme content with the same
    // %key% surface they already use elsewhere.

    text_template&       binder()       D_NOEXCEPT { return m_tmpl; }
    const text_template& binder() const D_NOEXCEPT { return m_tmpl; }

    void
    bind(
        const std::string& _key,
        const std::string& _value
    )
    {
        m_tmpl.bind(_key, _value);

        return;
    }

    template<typename _Fn>
    void
    bind_function(
        const std::string& _key,
        _Fn&&              _resolver
    )
    {
        m_tmpl.bind_function(
            _key,
            text_template::resolver_type(
                static_cast<_Fn&&>(_resolver)));

        return;
    }

    void
    bind_section(
        const std::string& _key,
        bool               _value
    )
    {
        m_tmpl.bind_section(_key, _value);

        return;
    }

    void
    clear_bindings()
    {
        m_tmpl.clear_bindings();

        return;
    }


    // =================================================================
    //  e. element appension
    // =================================================================

    // add_text
    //   appends a text element with an explicit style.  The format
    // string is resolved through the text_template at render time.
    void
    add_text(
        const std::string&    _format,
        const pdf_text_style& _style
    )
    {
        element e;
        e.kind   = element_kind::text;
        e.format = _format;
        e.style  = _style;

        m_elements.push_back(static_cast<element&&>(e));

        return;
    }

    // add_text (body style)
    //   appends a text element rendered in the body style.
    void
    add_text(
        const std::string& _format
    )
    {
        add_text(_format, m_body_style);

        return;
    }

    // add_styled_text
    //   appends a text element referencing a registered style by
    // name; falls back to the body style if the name is unknown at
    // render time.
    void
    add_styled_text(
        const std::string& _format,
        const std::string& _style_name
    )
    {
        element e;
        e.kind       = element_kind::text;
        e.format     = _format;
        e.style      = m_body_style;
        e.style_name = _style_name;

        m_elements.push_back(static_cast<element&&>(e));

        return;
    }

    // add_rule
    //   appends a horizontal separator line spanning the content
    // width at the current cursor.
    void
    add_rule(
        pdf_unit         _line_width = 0.75,
        const pdf_color& _color      = pdf_color::gray()
    )
    {
        element e;
        e.kind              = element_kind::rule;
        e.amount            = _line_width;
        e.style.color       = _color;

        m_elements.push_back(static_cast<element&&>(e));

        return;
    }

    // add_vspace
    //   appends blank vertical space of _points.
    void
    add_vspace(
        pdf_unit _points
    )
    {
        element e;
        e.kind   = element_kind::vspace;
        e.amount = _points;

        m_elements.push_back(static_cast<element&&>(e));

        return;
    }

    // add_page_break
    //   forces subsequent content onto a new page.
    void
    add_page_break()
    {
        element e;
        e.kind = element_kind::page_break;

        m_elements.push_back(static_cast<element&&>(e));

        return;
    }


    // =================================================================
    //  f. render_to_pdf
    // =================================================================

    // render_to_pdf
    //   draws every element into the supplied document, resolving
    // format strings, wrapping long lines to the content width,
    // honoring alignment and indentation, and starting new pages
    // when the cursor crosses the bottom margin.  The document is
    // left open for the caller to finalize.
    void
    render_to_pdf(
        pdf_document& _doc
    ) const
    {
        layout_state st;

        // begin the first page and seat the cursor at the top
        _doc.add_page(m_layout.page);
        st.cursor_y = m_layout.content_top();
        st.has_page = true;

        for (size_type i = 0; i < m_elements.size(); ++i)
        {
            emit_element(_doc, m_elements[i], st);
        }

        return;
    }


    // =================================================================
    //  g. render_pdf / save_pdf
    // =================================================================

    // render_pdf
    //   renders into a fresh document backed by the built-in
    // backend and returns the serialized PDF bytes.
    std::string
    render_pdf() const
    {
        pdf_document doc;

        render_to_pdf(doc);

        return doc.to_bytes();
    }

    // render_pdf (custom backend)
    //   renders into a fresh document driving a caller-supplied
    // backend, then returns the serialized bytes.
    std::string
    render_pdf(
        pdf_backend& _backend
    ) const
    {
        pdf_document doc(_backend);

        render_to_pdf(doc);

        return doc.to_bytes();
    }

    // save_pdf
    //   renders with the built-in backend and writes to _path.
    bool
    save_pdf(
        const char* _path
    ) const
    {
        pdf_document doc;

        render_to_pdf(doc);

        return doc.save(_path);
    }

    // save_pdf (custom backend)
    bool
    save_pdf(
        const char*  _path,
        pdf_backend& _backend
    ) const
    {
        pdf_document doc(_backend);

        render_to_pdf(doc);

        return doc.save(_path);
    }


    // =================================================================
    //  h. clear / query
    // =================================================================

    void
    clear_elements()
    {
        m_elements.clear();

        return;
    }

    size_type
    element_count() const D_NOEXCEPT
    {
        return m_elements.size();
    }

    bool
    empty() const D_NOEXCEPT
    {
        return m_elements.empty();
    }

private:
    // =================================================================
    //  internal: layout state
    // =================================================================

    // layout_state
    //   struct: the mutable cursor carried through a render pass.
    struct layout_state
    {
        pdf_unit cursor_y;
        bool     has_page;

        layout_state()
            : cursor_y(0.0),
              has_page(false)
        {}
    };


    // =================================================================
    //  internal: style resolution
    // =================================================================

    // resolve_style
    //   returns the effective style for an element: a referenced
    // named style if present and known, otherwise the element's own
    // style.
    pdf_text_style
    resolve_style(
        const element& _e
    ) const
    {
        if (!_e.style_name.empty())
        {
            for (size_type i = 0; i < m_styles.size(); ++i)
            {
                if (m_styles[i].name == _e.style_name)
                {
                    return m_styles[i].style;
                }
            }
        }

        return _e.style;
    }


    // =================================================================
    //  internal: pagination
    // =================================================================

    // ensure_room
    //   guarantees at least _needed points remain above the bottom
    // margin, starting a new page and reseating the cursor if not.
    void
    ensure_room(
        pdf_document& _doc,
        layout_state& _st,
        pdf_unit      _needed
    ) const
    {
        if ( (_st.cursor_y - _needed) < m_layout.content_bottom() )
        {
            _doc.add_page(m_layout.page);
            _st.cursor_y = m_layout.content_top();
        }

        return;
    }


    // =================================================================
    //  internal: line wrapping
    // =================================================================

    // wrap_line
    //   hard-wraps _text to at most _max_cols characters per slice,
    // preserving a blank line as a single empty slice.
    static std::vector<std::string>
    wrap_line(
        const std::string& _text,
        size_type          _max_cols
    )
    {
        std::vector<std::string> out;

        if ( (_max_cols == 0) ||
             (_text.size() <= _max_cols) )
        {
            out.push_back(_text);

            return out;
        }

        size_type pos = 0;

        while (pos < _text.size())
        {
            out.push_back(_text.substr(pos, _max_cols));
            pos += _max_cols;
        }

        return out;
    }

    // split_newlines
    //   splits a resolved format result on '\n' (folding '\r') so
    // each source line is laid out independently.
    static std::vector<std::string>
    split_newlines(
        const std::string& _text
    )
    {
        std::vector<std::string> lines;
        std::string              cur;

        for (size_type i = 0; i < _text.size(); ++i)
        {
            char c = _text[i];

            if (c == '\n')
            {
                if ( (!cur.empty()) &&
                     (cur[cur.size() - 1] == '\r') )
                {
                    cur.erase(cur.size() - 1);
                }

                lines.push_back(cur);
                cur.clear();

                continue;
            }

            cur.push_back(c);
        }

        // a trailing line without a newline still counts
        if (!cur.empty())
        {
            lines.push_back(cur);
        }

        // a completely empty result is one empty line
        if (lines.empty())
        {
            lines.push_back("");
        }

        return lines;
    }


    // =================================================================
    //  internal: alignment
    // =================================================================

    // line_x
    //   computes the x origin for a line of _char_count characters
    // under the given style, accounting for alignment and the
    // style's character-cell indentation.
    pdf_unit
    line_x(
        const pdf_text_style& _style,
        size_type             _char_count
    ) const
    {
        pdf_unit left  = m_layout.margin_left +
                         ( static_cast<pdf_unit>(_style.indent_cells) *
                           _style.cell_width() );
        pdf_unit width = _style.font.estimated_width(_char_count);

        if (_style.align == pdf_text_align::center)
        {
            pdf_unit avail = m_layout.content_width();

            return ( m_layout.margin_left +
                     ((avail - width) * 0.5) );
        }

        if (_style.align == pdf_text_align::right)
        {
            return ( m_layout.page.size.width -
                     m_layout.margin_right - width );
        }

        return left;
    }


    // =================================================================
    //  internal: element emission
    // =================================================================

    // emit_element
    //   dispatches on element kind, advancing the cursor and adding
    // pages as needed.
    void
    emit_element(
        pdf_document&  _doc,
        const element& _e,
        layout_state&  _st
    ) const
    {
        if (_e.kind == element_kind::page_break)
        {
            _doc.add_page(m_layout.page);
            _st.cursor_y = m_layout.content_top();

            return;
        }

        if (_e.kind == element_kind::vspace)
        {
            ensure_room(_doc, _st, _e.amount);
            _st.cursor_y -= _e.amount;

            return;
        }

        if (_e.kind == element_kind::rule)
        {
            ensure_room(_doc, _st, _e.style.leading);

            pdf_unit y = _st.cursor_y - (_e.style.leading * 0.5);

            _doc.line(
                pdf_point(m_layout.margin_left, y),
                pdf_point(m_layout.page.size.width -
                              m_layout.margin_right, y),
                pdf_paint::stroked(_e.style.color, _e.amount));

            _st.cursor_y -= _e.style.leading;

            return;
        }

        // text element
        emit_text(_doc, _e, _st);

        return;
    }

    // emit_text
    //   resolves the element's format string, splits and wraps it,
    // and places each resulting line with pagination.
    void
    emit_text(
        pdf_document&  _doc,
        const element& _e,
        layout_state&  _st
    ) const
    {
        pdf_text_style   style = resolve_style(_e);
        pdf_text_options opts  = style.to_text_options();

        // resolve %key% tokens through the borrowed text_template
        std::string resolved = m_tmpl.render(_e.format);

        // compute how many character cells fit in the content width
        pdf_unit cell = style.cell_width();
        size_type max_cols = 0;

        if (cell > 0.0)
        {
            pdf_unit avail = m_layout.content_width() -
                ( static_cast<pdf_unit>(style.indent_cells) * cell );

            if (avail > 0.0)
            {
                max_cols = static_cast<size_type>(avail / cell);
            }
        }

        std::vector<std::string> source = split_newlines(resolved);

        // lay out each source line, wrapping to the content width
        for (size_type i = 0; i < source.size(); ++i)
        {
            std::vector<std::string> pieces =
                wrap_line(source[i], max_cols);

            for (size_type k = 0; k < pieces.size(); ++k)
            {
                ensure_room(_doc, _st, style.leading);

                pdf_unit x = line_x(style, pieces[k].size());
                pdf_unit y = _st.cursor_y - style.font.size;

                _doc.text(pdf_point(x, y), pieces[k], opts);

                _st.cursor_y -= style.leading;
            }
        }

        return;
    }


    // =================================================================
    //  storage
    // =================================================================

    pdf_layout               m_layout;
    mutable text_template    m_tmpl;
    std::vector<element>     m_elements;
    std::vector<named_style> m_styles;
    pdf_text_style           m_body_style;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   CONVENIENCE FACTORIES                                ///
///////////////////////////////////////////////////////////////////////////////

// make_pdf_template
//   function: creates a pdf_template with the given page size and
// default margins.
inline pdf_template
make_pdf_template(
    const pdf_page_size& _page = pdf_page_size::letter()
)
{
    return pdf_template(pdf_layout(_page));
}

// make_report_template
//   function: creates a monospace-bodied pdf_template suited to
// fixed-column test output, with a registered "heading" style.
inline pdf_template
make_report_template(
    const pdf_page_size& _page = pdf_page_size::letter()
)
{
    pdf_layout   lay(_page);
    pdf_template tpl(lay);

    tpl.set_body_style(pdf_template::monospace_style());
    tpl.register_style("heading", pdf_template::heading_style());

    return tpl;
}


NS_END  // pdf
NS_END  // djinterp


#endif  // DJINTERP_PDF_TEMPLATE_
