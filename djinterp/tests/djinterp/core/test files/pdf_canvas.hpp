/******************************************************************************
* djinterp [text]                                              pdf_canvas.hpp
*
* djinterp high-level PDF writer:
*   A cursor-based document writer layered over the agnostic pdf_document
* foundation and the accurate pdf_metrics measurement facility.  Where
* pdf_document draws at explicit points in bottom-left user space, pdf_canvas
* flows content top-to-bottom with automatic line advancement, real
* metric-driven word wrapping and justification, alignment, indentation,
* text boxes, rules, headings, spacers, and a simple table - the layer that
* turns the foundation into something you can write simple-to-intermediately
* complex documents with directly.
*
*   LIBRARY AGNOSTIC:
*   pdf_canvas knows nothing about any PDF library.  It drives a
* pdf_document, which drives whatever pdf_backend is installed (the built-in
* writer by default, or a libHaru / PDFHummus adapter).  Swapping the
* rendering library changes nothing above this layer.
*
*   LAYOUT MODEL:
*   A margin box defines the writable region of each page.  A cursor descends
* from the top margin; emitting a paragraph consumes one leading-step per
* wrapped line, and when the cursor would cross the bottom margin a new page
* is started automatically.  Horizontal placement honors per-call alignment
* (left / center / right / justify) and a left indent in points.  Because the
* writer measures with pdf_metrics, alignment and wrapping are accurate for
* proportional fonts, not just fixed-pitch.
*
*   TEXT STYLE:
*   pdf_text_style (from the foundation's pdf_text_options plus an indent)
* is reused; the writer adds paragraph spacing.  Reasonable defaults make
* the no-configuration path produce a clean document.
*
*   CORE INTERFACE:
*     paragraph(text, style)        - flow a wrapped, aligned paragraph
*     heading(text, level)          - a sized, spaced heading
*     line_of(text, style)          - a single (non-wrapped) line
*     bullet_list(items, style)     - a simple bulleted list
*     text_box(rect, text, style)   - wrapped text clipped to a box
*     rule(width, color)            - a horizontal separator
*     vspace(points) / page_break() - spacing and explicit breaks
*     table(columns, rows, opts)    - a basic gridded table
*     finish()                      - flush and return the document
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h / env_*.h via djinterp.hpp.  No third-party
* dependency in this header or its foundation.
*
*
* TABLE OF CONTENTS
* =================
* I.    DOCUMENT STYLE
* II.   TABLE OPTIONS
* III.  PDF CANVAS
*       a. construction / lifecycle
*       b. cursor & page queries
*       c. flow: paragraph / heading / line / list
*       d. text box
*       e. graphics: rule / vspace / page break
*       f. table
*       g. finish
* IV.   CONVENIENCE FACTORIES
*
*
* path:      /inc/djinterp/core/text/pdf/pdf_canvas.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_TEXT_PDF_CANVAS_
#define DJINTERP_TEXT_PDF_CANVAS_ 1

// std
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./pdf.hpp"
#include "./pdf_metrics.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   DOCUMENT STYLE                                       ///
///////////////////////////////////////////////////////////////////////////////

// canvas_style
//   struct: presentation parameters for a flowed text run - font,
// color, alignment, line leading, a left indent in points, and the
// vertical space added after the run.
struct canvas_style
{
    pdf_font       font;
    pdf_color      color;
    pdf_text_align align;
    pdf_unit       leading;
    pdf_unit       indent;
    pdf_unit       space_after;

    canvas_style()
        : font(pdf_base_font::helvetica, 11.0),
          color(pdf_color::black()),
          align(pdf_text_align::left),
          leading(14.0),
          indent(0.0),
          space_after(6.0)
    {}

    explicit canvas_style(
        const pdf_font& _font
    )
        : font(_font),
          color(pdf_color::black()),
          align(pdf_text_align::left),
          leading(_font.size * 1.3),
          indent(0.0),
          space_after(_font.size * 0.5)
    {}

    // to_text_options
    //   projects onto the foundation's pdf_text_options.
    pdf_text_options
    to_text_options() const
    {
        pdf_text_options o(font);

        o.color   = color;
        o.align   = align;
        o.leading = leading;

        return o;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TABLE OPTIONS                                        ///
///////////////////////////////////////////////////////////////////////////////

// table_options
//   struct: rendering parameters for the basic table helper -
// column widths (points), cell padding, the body and header text
// styles, grid line color/width, and whether the first supplied row
// is a header.
struct table_options
{
    std::vector<pdf_unit> column_widths;
    pdf_unit              padding;
    pdf_unit              row_height;
    canvas_style          body_style;
    canvas_style          header_style;
    pdf_color             grid_color;
    pdf_unit              grid_width;
    bool                  has_header;
    pdf_color             header_fill;
    bool                  fill_header;

    table_options()
        : column_widths(),
          padding(4.0),
          row_height(18.0),
          body_style(pdf_font(pdf_base_font::helvetica, 10.0)),
          header_style(pdf_font(pdf_base_font::helvetica_bold, 10.0)),
          grid_color(pdf_color::gray()),
          grid_width(0.5),
          has_header(true),
          header_fill(pdf_color::from_rgb255(235, 235, 235)),
          fill_header(true)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                III. PDF CANVAS                                           ///
///////////////////////////////////////////////////////////////////////////////

// pdf_canvas
//   class: a cursor-based document writer over pdf_document.  Owns
// (or borrows) a document, tracks a flowing cursor within a margin
// box, and emits paragraphs, headings, lists, boxes, rules, and
// tables with automatic pagination and metric-accurate layout.
//
// Usage:
//   pdf_canvas cv(pdf_page_size::letter());
//   cv.heading("Quarterly Report", 1);
//   cv.paragraph("Lorem ipsum dolor sit amet, consectetur ...");
//   cv.rule();
//   cv.bullet_list({"first point", "second point"});
//   cv.save("report.pdf");
class pdf_canvas
{
public:
    using size_type = std::size_t;

    // =================================================================
    //  a. construction / lifecycle
    // =================================================================

    // default: owns a document backed by the built-in backend
    explicit pdf_canvas(
        const pdf_page_size& _page = pdf_page_size::letter()
    )
        : m_owned(new pdf_document()),
          m_doc(nullptr),
          m_page(_page),
          m_margin_left(54.0),
          m_margin_right(54.0),
          m_margin_top(54.0),
          m_margin_bottom(54.0),
          m_cursor_y(0.0),
          m_started(false)
    {
        m_doc = m_owned.get();
    }

    // borrow: writes into a caller-owned document (which may drive a
    // custom backend)
    pdf_canvas(
        pdf_document&        _doc,
        const pdf_page_size& _page = pdf_page_size::letter()
    )
        : m_owned(),
          m_doc(&_doc),
          m_page(_page),
          m_margin_left(54.0),
          m_margin_right(54.0),
          m_margin_top(54.0),
          m_margin_bottom(54.0),
          m_cursor_y(0.0),
          m_started(false)
    {}


    // =================================================================
    //  configuration
    // =================================================================

    void
    set_page_size(
        const pdf_page_size& _page
    )
    {
        m_page = _page;

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
        m_margin_left   = _left;
        m_margin_right  = _right;
        m_margin_top    = _top;
        m_margin_bottom = _bottom;

        return;
    }

    void
    metadata(
        const std::string& _key,
        const std::string& _value
    )
    {
        m_doc->metadata(_key, _value);

        return;
    }


    // =================================================================
    //  b. cursor & page queries
    // =================================================================

    // content_width
    //   horizontal space between the side margins.
    pdf_unit
    content_width() const D_NOEXCEPT
    {
        return (m_page.size.width - m_margin_left - m_margin_right);
    }

    pdf_unit cursor_y()      const D_NOEXCEPT { return m_cursor_y; }
    pdf_unit content_top()   const D_NOEXCEPT { return (m_page.size.height - m_margin_top); }
    pdf_unit content_bottom()const D_NOEXCEPT { return m_margin_bottom; }

    pdf_document&       document()       D_NOEXCEPT { return *m_doc; }
    const pdf_document& document() const D_NOEXCEPT { return *m_doc; }


    // =================================================================
    //  c. flow: paragraph / heading / line / list
    // =================================================================

    // paragraph
    //   flows _text as a wrapped, aligned paragraph in _style.  Lines
    // are wrapped to the content width (less the style's indent) using
    // accurate metrics, justified if requested (except the last line),
    // and the cursor advances past the paragraph plus its space_after.
    void
    paragraph(
        const std::string&  _text,
        const canvas_style& _style
    )
    {
        ensure_started();

        pdf_unit avail = content_width() - _style.indent;

        if (avail <= 0.0)
        {
            avail = content_width();
        }

        std::vector<std::string> lines =
            wrap_to_width(_style.font, _text, avail);

        for (size_type i = 0; i < lines.size(); ++i)
        {
            ensure_room(_style.leading);

            bool last = (i + 1 == lines.size());

            place_line(lines[i], _style, avail, last);

            m_cursor_y -= _style.leading;
        }

        m_cursor_y -= _style.space_after;

        return;
    }

    // paragraph (default style)
    void
    paragraph(
        const std::string& _text
    )
    {
        paragraph(_text, canvas_style());

        return;
    }

    // heading
    //   emits a heading at the given level (1 = largest).  Levels map
    // to bold Helvetica at descending sizes; unknown levels clamp to
    // the smallest.  A little extra space precedes and follows.
    void
    heading(
        const std::string& _text,
        int                _level = 1
    )
    {
        ensure_started();

        pdf_unit size;

        switch (_level)
        {
            case 1:  { size = 20.0; break; }
            case 2:  { size = 16.0; break; }
            case 3:  { size = 13.0; break; }
            default: { size = 11.0; break; }
        }

        canvas_style s(pdf_font(pdf_base_font::helvetica_bold, size));
        s.leading     = size * 1.25;
        s.space_after = size * 0.4;

        // a touch of leading space above a heading (not before the
        // very first thing on a fresh page)
        if (m_cursor_y < content_top())
        {
            m_cursor_y -= (size * 0.3);
        }

        paragraph(_text, s);

        return;
    }

    // line_of
    //   places a single line in _style without wrapping (long lines
    // are truncated with an ellipsis to the content width).  Useful
    // for fixed-pitch / pre-aligned content such as code or test rows.
    void
    line_of(
        const std::string&  _text,
        const canvas_style& _style
    )
    {
        ensure_started();
        ensure_room(_style.leading);

        pdf_unit    avail = content_width() - _style.indent;
        std::string shown = truncate_ellipsis(
            _style.font.family, _style.font.size, _text, avail);

        place_line(shown, _style, avail, true);

        m_cursor_y -= _style.leading;

        return;
    }

    // line_of (default style)
    void
    line_of(
        const std::string& _text
    )
    {
        line_of(_text, canvas_style());

        return;
    }

    // bullet_list
    //   emits each item as a bulleted, wrapped paragraph.  The bullet
    // glyph hangs in the indent and continuation lines align to the
    // text, not the bullet.  The default bullet is the WinAnsi bullet
    // byte (0x95); the foundation emits WinAnsiEncoding, so a UTF-8
    // bullet would mojibake - pass a single-byte WinAnsi glyph.
    void
    bullet_list(
        const std::vector<std::string>& _items,
        const canvas_style&             _style,
        const std::string&              _bullet = "\x95"
    )
    {
        ensure_started();

        // bullet hang width: the bullet plus a space, in the body font
        pdf_unit hang =
            text_width(_style.font, _bullet + " ");

        for (size_type i = 0; i < _items.size(); ++i)
        {
            canvas_style item = _style;
            item.indent      = _style.indent + hang;
            item.space_after = _style.font.size * 0.25;

            pdf_unit avail = content_width() - item.indent;

            if (avail <= 0.0)
            {
                avail = content_width();
            }

            std::vector<std::string> lines =
                wrap_to_width(item.font, _items[i], avail);

            for (size_type k = 0; k < lines.size(); ++k)
            {
                ensure_room(item.leading);

                // first line carries the bullet in the hang column
                if (k == 0)
                {
                    pdf_text_options bopt = item.to_text_options();
                    pdf_unit bx = m_margin_left + _style.indent;
                    pdf_unit by = m_cursor_y - item.font.size;

                    m_doc->text(pdf_point(bx, by), _bullet, bopt);
                }

                place_line(lines[k], item, avail, true);

                m_cursor_y -= item.leading;
            }

            m_cursor_y -= item.space_after;
        }

        return;
    }

    // bullet_list (default style)
    void
    bullet_list(
        const std::vector<std::string>& _items
    )
    {
        bullet_list(_items, canvas_style());

        return;
    }


    // =================================================================
    //  d. text box
    // =================================================================

    // text_box
    //   draws optional box decoration, then flows _text wrapped within
    // _rect (with internal padding), clipped to the box height.  The
    // box does not move the page cursor; it is absolute placement,
    // useful for callouts and sidebars.
    void
    text_box(
        const pdf_rect&     _rect,
        const std::string&  _text,
        const canvas_style& _style,
        pdf_unit            _padding   = 6.0,
        bool                _draw_border = true,
        const pdf_color&    _border    = pdf_color::gray()
    )
    {
        ensure_started();

        // border / background
        if (_draw_border)
        {
            m_doc->rect(_rect, pdf_paint::stroked(_border, 0.75));
        }

        pdf_unit inner_w = _rect.width - (2.0 * _padding);
        pdf_unit top     = _rect.y + _rect.height - _padding;
        pdf_unit bottom  = _rect.y + _padding;

        std::vector<std::string> lines =
            wrap_to_width(_style.font, _text, inner_w);

        pdf_unit y = top;

        for (size_type i = 0; i < lines.size(); ++i)
        {
            pdf_unit baseline = y - _style.font.size;

            // stop if the next line would fall below the box
            if (baseline < bottom)
            {
                break;
            }

            pdf_unit x =
                aligned_x(lines[i], _style,
                          _rect.x + _padding, inner_w, false);

            m_doc->text(pdf_point(x, baseline),
                        lines[i], _style.to_text_options());

            y -= _style.leading;
        }

        return;
    }


    // =================================================================
    //  e. graphics: rule / vspace / page break
    // =================================================================

    // rule
    //   draws a horizontal separator across the content width at the
    // cursor and advances past it.
    void
    rule(
        pdf_unit         _width = 0.75,
        const pdf_color& _color = pdf_color::gray()
    )
    {
        ensure_started();
        ensure_room(8.0);

        pdf_unit y = m_cursor_y - 4.0;

        m_doc->line(
            pdf_point(m_margin_left, y),
            pdf_point(m_page.size.width - m_margin_right, y),
            pdf_paint::stroked(_color, _width));

        m_cursor_y -= 8.0;

        return;
    }

    // vspace
    //   adds blank vertical space, paginating if it would overflow.
    void
    vspace(
        pdf_unit _points
    )
    {
        ensure_started();
        ensure_room(_points);

        m_cursor_y -= _points;

        return;
    }

    // page_break
    //   forces subsequent content onto a new page.
    void
    page_break()
    {
        ensure_started();

        m_doc->add_page(m_page);
        m_cursor_y = content_top();

        return;
    }


    // =================================================================
    //  f. table
    // =================================================================

    // table
    //   renders a gridded table.  _rows is a list of rows, each a list
    // of cell strings; column geometry comes from _opts.column_widths
    // (falling back to equal columns across the content width when
    // empty).  Rows paginate; on a new page a header row repeats if
    // configured.  Cell text is single-line and ellipsized to fit.
    void
    table(
        const std::vector<std::vector<std::string>>& _rows,
        const table_options&                         _opts
    )
    {
        ensure_started();

        if (_rows.empty())
        {
            return;
        }

        std::vector<pdf_unit> widths = resolve_columns(_opts, _rows);

        // optionally treat the first row as a repeating header
        size_type start = 0;
        std::vector<std::string> header;

        if ( (_opts.has_header) &&
             (!_rows.empty()) )
        {
            header = _rows[0];
            start  = 1;

            draw_table_row(header, widths, _opts, true);
        }

        for (size_type r = start; r < _rows.size(); ++r)
        {
            // paginate: if the next row would overflow, break and
            // repeat the header
            if ((m_cursor_y - _opts.row_height) < content_bottom())
            {
                page_break();

                if (_opts.has_header)
                {
                    draw_table_row(header, widths, _opts, true);
                }
            }

            draw_table_row(_rows[r], widths, _opts, false);
        }

        return;
    }


    // =================================================================
    //  g. finish
    // =================================================================

    // save
    //   finalizes and writes the document to _path.
    bool
    save(
        const char* _path
    )
    {
        ensure_started();

        return m_doc->save(_path);
    }

    // to_bytes
    //   finalizes and returns the serialized PDF bytes.
    std::string
    to_bytes()
    {
        ensure_started();

        return m_doc->to_bytes();
    }

private:
    // =================================================================
    //  internal: lifecycle
    // =================================================================

    // ensure_started
    //   lazily begins the first page and seats the cursor.
    void
    ensure_started()
    {
        if (!m_started)
        {
            m_doc->add_page(m_page);
            m_cursor_y = content_top();
            m_started  = true;
        }

        return;
    }

    // ensure_room
    //   starts a new page if _needed points do not remain above the
    // bottom margin.
    void
    ensure_room(
        pdf_unit _needed
    )
    {
        if ((m_cursor_y - _needed) < content_bottom())
        {
            m_doc->add_page(m_page);
            m_cursor_y = content_top();
        }

        return;
    }


    // =================================================================
    //  internal: placement
    // =================================================================

    // aligned_x
    //   computes the x origin for a line of text under an alignment,
    // within a box of _avail width starting at _left.  Justify falls
    // back to left here; justification spacing is applied separately
    // in place_line.
    pdf_unit
    aligned_x(
        const std::string&  _text,
        const canvas_style& _style,
        pdf_unit            _left,
        pdf_unit            _avail,
        bool                /*_last*/
    ) const
    {
        if (_style.align == pdf_text_align::center)
        {
            pdf_unit w = text_width(_style.font, _text);

            return (_left + ((_avail - w) * 0.5));
        }

        if (_style.align == pdf_text_align::right)
        {
            pdf_unit w = text_width(_style.font, _text);

            return (_left + (_avail - w));
        }

        return _left;
    }

    // place_line
    //   draws one line at the current cursor.  For justify alignment
    // on a non-last line with more than one word, inter-word gaps are
    // widened so the line fills _avail; otherwise the line is placed
    // by alignment.
    void
    place_line(
        const std::string&  _text,
        const canvas_style& _style,
        pdf_unit            _avail,
        bool                _last
    )
    {
        pdf_unit         left = m_margin_left + _style.indent;
        pdf_unit         baseline = m_cursor_y - _style.font.size;
        pdf_text_options opt = _style.to_text_options();

        // justified body line: distribute slack across spaces
        if ( (_style.align == pdf_text_align::justify) &&
             (!_last) )
        {
            if (place_justified(_text, _style, left, baseline, _avail))
            {
                return;
            }
        }

        pdf_unit x = aligned_x(_text, _style, left, _avail, _last);

        m_doc->text(pdf_point(x, baseline), _text, opt);

        return;
    }

    // place_justified
    //   draws _text justified to _avail by widening inter-word gaps,
    // placing each word at a computed x.  Returns false (so the caller
    // falls back to left placement) when the line has fewer than two
    // words or already overflows.
    bool
    place_justified(
        const std::string&  _text,
        const canvas_style& _style,
        pdf_unit            _left,
        pdf_unit            _baseline,
        pdf_unit            _avail
    )
    {
        std::vector<std::string> words = split_words(_text);

        if (words.size() < 2)
        {
            return false;
        }

        // total width of the words alone (no spaces)
        pdf_unit words_w = 0.0;

        for (size_type i = 0; i < words.size(); ++i)
        {
            words_w += text_width(_style.font, words[i]);
        }

        pdf_unit slack = _avail - words_w;

        // nothing to distribute (or overflow) - let caller left-place
        if (slack <= 0.0)
        {
            return false;
        }

        pdf_unit         gap = slack / static_cast<pdf_unit>(words.size() - 1);
        pdf_unit         x   = _left;
        pdf_text_options opt = _style.to_text_options();

        // place each word, advancing by its width plus the gap
        for (size_type i = 0; i < words.size(); ++i)
        {
            m_doc->text(pdf_point(x, _baseline), words[i], opt);

            x += text_width(_style.font, words[i]);

            if (i + 1 < words.size())
            {
                x += gap;
            }
        }

        return true;
    }


    // =================================================================
    //  internal: table helpers
    // =================================================================

    // resolve_columns
    //   returns the column widths to use: the configured widths if
    // present, otherwise equal columns sized to the widest row across
    // the content width.
    std::vector<pdf_unit>
    resolve_columns(
        const table_options&                         _opts,
        const std::vector<std::vector<std::string>>& _rows
    ) const
    {
        if (!_opts.column_widths.empty())
        {
            return _opts.column_widths;
        }

        size_type cols = 0;

        for (size_type r = 0; r < _rows.size(); ++r)
        {
            if (_rows[r].size() > cols)
            {
                cols = _rows[r].size();
            }
        }

        std::vector<pdf_unit> widths;

        if (cols == 0)
        {
            return widths;
        }

        pdf_unit each = content_width() / static_cast<pdf_unit>(cols);

        for (size_type c = 0; c < cols; ++c)
        {
            widths.push_back(each);
        }

        return widths;
    }

    // draw_table_row
    //   draws one row's cell backgrounds, grid, and ellipsized text,
    // then advances the cursor by the row height.
    void
    draw_table_row(
        const std::vector<std::string>& _cells,
        const std::vector<pdf_unit>&    _widths,
        const table_options&            _opts,
        bool                            _is_header
    )
    {
        pdf_unit bottom = m_cursor_y - _opts.row_height;
        pdf_unit x      = m_margin_left;

        const canvas_style& style =
            _is_header ? _opts.header_style : _opts.body_style;

        // header background fill
        if ( (_is_header) &&
             (_opts.fill_header) )
        {
            pdf_unit total_w = 0.0;

            for (size_type c = 0; c < _widths.size(); ++c)
            {
                total_w += _widths[c];
            }

            m_doc->rect(
                pdf_rect(m_margin_left, bottom, total_w,
                         _opts.row_height),
                pdf_paint::filled(_opts.header_fill));
        }

        // cells: grid box + clipped text
        for (size_type c = 0; c < _widths.size(); ++c)
        {
            pdf_unit w = _widths[c];

            m_doc->rect(
                pdf_rect(x, bottom, w, _opts.row_height),
                pdf_paint::stroked(_opts.grid_color, _opts.grid_width));

            if (c < _cells.size())
            {
                pdf_unit    inner = w - (2.0 * _opts.padding);
                std::string text  = truncate_ellipsis(
                    style.font.family, style.font.size,
                    _cells[c], inner);

                pdf_unit tx = x + _opts.padding;
                pdf_unit ty = bottom +
                              ((_opts.row_height - style.font.size) * 0.5);

                m_doc->text(pdf_point(tx, ty), text,
                            style.to_text_options());
            }

            x += w;
        }

        m_cursor_y = bottom;

        return;
    }


    // =================================================================
    //  internal: word split
    // =================================================================

    // split_words
    //   splits on ASCII spaces, dropping empty tokens.
    static std::vector<std::string>
    split_words(
        const std::string& _text
    )
    {
        std::vector<std::string> words;
        std::string              cur;

        for (size_type i = 0; i < _text.size(); ++i)
        {
            if (_text[i] == ' ')
            {
                if (!cur.empty())
                {
                    words.push_back(cur);
                    cur.clear();
                }

                continue;
            }

            cur.push_back(_text[i]);
        }

        if (!cur.empty())
        {
            words.push_back(cur);
        }

        return words;
    }


    // =================================================================
    //  storage
    // =================================================================

    std::unique_ptr<pdf_document> m_owned;
    pdf_document*                 m_doc;
    pdf_page_size                 m_page;
    pdf_unit                      m_margin_left;
    pdf_unit                      m_margin_right;
    pdf_unit                      m_margin_top;
    pdf_unit                      m_margin_bottom;
    pdf_unit                      m_cursor_y;
    bool                          m_started;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  CONVENIENCE FACTORIES                                ///
///////////////////////////////////////////////////////////////////////////////

// make_canvas
//   function: creates a pdf_canvas on the built-in backend with the
// given page size.
inline pdf_canvas
make_canvas(
    const pdf_page_size& _page = pdf_page_size::letter()
)
{
    return pdf_canvas(_page);
}


NS_END  // djinterp


#endif  // DJINTERP_TEXT_PDF_CANVAS_
