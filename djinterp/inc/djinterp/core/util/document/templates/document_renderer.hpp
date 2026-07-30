/******************************************************************************
* djinterp [utility]                                          document_renderer.hpp
*
*   The DIALECT BOUNDARY of the document-template subframework.  A document
* template (table, title page, table of contents, update section) is a pure
* CONTENT model; it names blocks and hints and knows nothing of text vs PDF vs
* HTML.  A document_renderer is the other half: a per-dialect back end that
* turns those semantic calls into bytes.  Templates drive a renderer; a renderer
* realises them.  Swap the renderer, change the dialect -- the template is
* untouched.
*
*   A SEMANTIC SINK, NOT A TREE.  The protocol is imperative (begin_table /
* table_column / begin_row / cell / end_row / end_table, heading, paragraph,
* rule, ...), exactly as pdf_template's flow surface and document_writer's tree
* surface are imperative.  No intermediate document object is materialised: a
* template streams events, the renderer places them.  This is a COLD assembly
* path (a whole document is built once, then serialised), so -- like pdf_backend
* -- the renderer is a genuine runtime plug-in and a virtual base is the right
* tool, not compile-time dispatch.
*
*   HINTS ARE ADVISORY.  Every call carries a doc_attributes bag; a renderer
* reads the keys it understands (`align`, `style`, `font`, `color`, `width`,
* ...) and IGNORES the rest.  A `.txt` renderer never consults `font`; an HTML
* or PDF renderer does.  That is the whole of "dialect-agnostic": the template
* offers semantics plus hints, the renderer decides realisation.
*
*   ONE PRIMITIVE, MANY DEFAULTS.  The single pure-virtual is `write_line`;
* every other method has a default that funnels to it or is a no-op, so a
* minimal back end implements one function and gets headings, paragraphs,
* lists, and key-values as plain lines (silently dropping rules, spacing, and
* tables until it overrides them).  A richer back end overrides only what it
* improves.  plain_document_renderer (Section II) is the reference realisation:
* the plain-text / console dialect, complete with a monospace table engine, and
* the thing that makes a document_table demonstrable out of the box.
*
*   PORTABILITY:
*   C++11 baseline (std::string / std::vector + a virtual base); no third-party
* dependency.
*
*
* TABLE OF CONTENTS
* =================
* I.    document_renderer            (the abstract semantic sink)
* II.   plain_document_renderer      (the plain-text reference realisation)
*
*
* path:      /inc/djinterp/core/util/document/templates/document_renderer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.11
******************************************************************************/

#ifndef DJINTERP_UTIL_DOCUMENT_RENDERER_
#define DJINTERP_UTIL_DOCUMENT_RENDERER_ 1

// std
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"        // NS_*, D_NODISCARD, D_NOEXCEPT
#include "./document_attributes.hpp"    // doc_attributes, attr_*, text_alignment


// D_OVERRIDE
//   macro: portable `override` specifier.  On C++11 and later `override` is a
// contextual keyword; pre-C++11 toolchains do not recognise it, so the macro
// expands to nothing there.  Guarded so it coexists with the identical
// definition pdf_primitives.hpp introduces.  Pre-definable to override.
#ifndef D_OVERRIDE
    #if ( defined(__cplusplus) &&  \
          D_ENV_LANG_IS_CPP11_OR_HIGHER )
        #define D_OVERRIDE  override
    #else
        #define D_OVERRIDE
    #endif
#endif  // D_OVERRIDE


NS_DJINTERP


// ===========================================================================
// I.   document_renderer
// ===========================================================================

// document_renderer
//   class: the abstract per-dialect back end a document template renders into.
// A template calls the semantic methods (heading / paragraph / the table
// group / ...); a concrete renderer turns them into a document in one format.
// Every method but `write_line` has a default (a funnel to write_line, or a
// no-op), so a back end may implement as little as the one primitive.  Each
// call carries a doc_attributes hint bag the renderer honours or ignores.
class document_renderer
{
public:
    // ~document_renderer
    //   destructor: virtual, for deletion through a document_renderer*.
    virtual ~document_renderer() = default;

    // -- document frame ------------------------------------------------------

    // begin_document / end_document
    //   open and close the whole document.  A streaming dialect writes a
    // preamble / epilogue here (an HTML <head>, a PDF first page); a plain one
    // needs neither, so the defaults are no-ops.
    virtual void begin_document(const doc_attributes&) {}
    virtual void end_document() {}

    // -- blocks --------------------------------------------------------------

    // heading
    //   a section title at _level (1 = outermost).  Default: the text as one
    // line; a dialect decorates it (an underline, an <h1>, a \section).
    virtual void
    heading(
        std::size_t        /*_level*/,
        const std::string& _text,
        const doc_attributes& _attrs
    )
    {
        write_line(_text, _attrs);

        return;
    }

    // paragraph
    //   a block of running text.  Default: one line.
    virtual void
    paragraph(
        const std::string&    _text,
        const doc_attributes& _attrs
    )
    {
        write_line(_text, _attrs);

        return;
    }

    // key_value
    //   a labelled value ("Author: teer").  Default: "key: value" on one line.
    virtual void
    key_value(
        const std::string&    _key,
        const std::string&    _value,
        const doc_attributes& _attrs
    )
    {
        write_line(_key + ": " + _value, _attrs);

        return;
    }

    // rule
    //   a horizontal separator.  Default: nothing (a dialect draws its own).
    virtual void rule(const doc_attributes&) {}

    // vertical_space
    //   blank vertical space of _amount (points, or a dialect's own unit).
    // Default: nothing.
    virtual void vertical_space(double /*_amount*/, const doc_attributes&) {}

    // page_break
    //   force subsequent content onto a new page.  Default: nothing (a
    // continuous dialect has no pages).
    virtual void page_break() {}

    // -- lists ---------------------------------------------------------------

    // begin_list / list_item / end_list
    //   an (optionally ordered) list; nest by opening another list inside an
    // item's scope.  Default item: one line; default open/close: nothing.
    virtual void begin_list(bool /*_ordered*/, const doc_attributes&) {}

    virtual void
    list_item(
        const std::string&    _text,
        const doc_attributes& _attrs
    )
    {
        write_line(_text, _attrs);

        return;
    }

    virtual void end_list() {}

    // -- tables --------------------------------------------------------------
    //   A table is streamed: begin_table, then one table_column per column
    // (its header text and per-column hints such as `align` / `width`), then a
    // begin_row / cell.../ end_row for each data row, then end_table.  Headers
    // are declared once, up front; rows carry data only.  Defaults are no-ops,
    // so a dialect without a table notion drops the table cleanly.

    virtual void begin_table(const doc_attributes&) {}
    virtual void table_column(const std::string& /*_header*/, const doc_attributes&) {}
    virtual void begin_row(const doc_attributes&) {}
    virtual void cell(const std::string& /*_text*/, const doc_attributes&) {}
    virtual void end_row() {}
    virtual void end_table() {}

protected:
    // write_line
    //   the one primitive a concrete renderer must supply: emit _text as a
    // single line in the target format, honouring whichever of _attrs it
    // understands.  The block defaults above funnel here.
    virtual void write_line(const std::string& _text,
                            const doc_attributes& _attrs) = 0;
};


// ===========================================================================
// II.  plain_document_renderer
// ===========================================================================

// plain_document_renderer
//   class: the plain-text / console realisation of document_renderer -- the
// reference dialect, and the one that makes a document_table renderable with
// no other back end present.  It accumulates a byte buffer: headings gain an
// underline, lists indent by depth, a rule is a run of dashes, a page break is
// a form feed, and tables are laid out as aligned monospace columns (widths
// computed from headers, cells, and any `width` hint; each column padded to
// its `align`).  Presentation hints with no plain-text meaning -- `font`,
// `color`, `style` -- are ignored, as the design intends.
class plain_document_renderer
    : public document_renderer
{
public:
    // -- public type aliases -------------------------------------------------

    using size_type = std::size_t;

    // -- construction --------------------------------------------------------

    plain_document_renderer()
        : m_out(),
          m_ordered(),
          m_counter(),
          m_headers(),
          m_aligns(),
          m_hints(),
          m_rows(),
          m_row()
    {}

    // -- result --------------------------------------------------------------

    // str
    //   the accumulated document.
    D_NODISCARD const std::string&
    str() const D_NOEXCEPT
    {
        return m_out;
    }

    // clear
    //   discard the accumulated document (and any in-flight table / list).
    void
    clear()
    {
        m_out.clear();
        m_ordered.clear();
        m_counter.clear();
        clear_table();

        return;
    }

    // -- overrides -----------------------------------------------------------

    void
    heading(
        std::size_t           _level,
        const std::string&    _text,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        // separate a heading from preceding content
        if (!m_out.empty())
        {
            m_out += '\n';
        }

        m_out += _text;
        m_out += '\n';

        // levels 1 and 2 gain a full-width underline ('=' then '-')
        if (_level <= size_type(2))
        {
            const char _u = (_level <= size_type(1)) ? '=' : '-';

            m_out += std::string(display_width(_text), _u);
            m_out += '\n';
        }

        return;
    }

    void
    rule(
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        size_type _w = parse_size(attr_or(_attrs, doc_attr_width, std::string()));

        // no width hint: a sensible default page-width rule
        if (_w == size_type(0))
        {
            _w = k_default_rule_cells;
        }

        m_out += std::string(_w, '-');
        m_out += '\n';

        return;
    }

    void
    vertical_space(
        double                /*_amount*/,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        // a plain blank line: the magnitude is meaningless in monospace
        m_out += '\n';

        return;
    }

    void
    page_break() D_OVERRIDE
    {
        // the conventional plain-text page break
        m_out += '\f';

        return;
    }

    void
    begin_list(
        bool                  _ordered,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        m_ordered.push_back(_ordered);
        m_counter.push_back(size_type(0));

        return;
    }

    void
    list_item(
        const std::string&    _text,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        const size_type _depth = m_ordered.size();

        // an item outside any list still renders, at depth 0
        const size_type _indent =
            (_depth > size_type(0)) ? ((_depth - size_type(1)) * k_list_indent)
                                    : size_type(0);

        m_out += std::string(_indent, ' ');

        // ordered lists number their items; unordered use a bullet
        if ( (_depth > size_type(0)) &&
             m_ordered.back() )
        {
            ++m_counter.back();
            m_out += std::to_string(m_counter.back());
            m_out += ". ";
        }
        else
        {
            m_out += "- ";
        }

        m_out += _text;
        m_out += '\n';

        return;
    }

    void
    end_list() D_OVERRIDE
    {
        // close the innermost list, if any
        if (!m_ordered.empty())
        {
            m_ordered.pop_back();
            m_counter.pop_back();
        }

        return;
    }

    void
    begin_table(
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        clear_table();

        return;
    }

    void
    table_column(
        const std::string&    _header,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        m_headers.push_back(_header);
        m_aligns.push_back(attr_align(_attrs, text_alignment::left));
        m_hints.push_back(
            parse_size(attr_or(_attrs, doc_attr_width, std::string())));

        return;
    }

    void
    begin_row(
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        m_row.clear();

        return;
    }

    void
    cell(
        const std::string&    _text,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        m_row.push_back(_text);

        return;
    }

    void
    end_row() D_OVERRIDE
    {
        m_rows.push_back(m_row);
        m_row.clear();

        return;
    }

    void
    end_table() D_OVERRIDE
    {
        const size_type _cols = column_count();

        // an empty table emits nothing
        if (_cols == size_type(0))
        {
            clear_table();

            return;
        }

        std::vector<size_type> _widths = computed_widths(_cols);

        // header row
        emit_row(m_headers, _widths, _cols);

        // header underline: dashes per column, joined at the separators
        std::string _rule;
        size_type   _c = 0;

        for (_c = 0; _c < _cols; ++_c)
        {
            if (_c > size_type(0))
            {
                _rule += k_rule_join;
            }

            _rule += std::string(_widths[_c], '-');
        }

        m_out += _rule;
        m_out += '\n';

        // data rows
        size_type _r = 0;

        for (_r = 0; _r < m_rows.size(); ++_r)
        {
            emit_row(m_rows[_r], _widths, _cols);
        }

        clear_table();

        return;
    }

protected:
    void
    write_line(
        const std::string&    _text,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        m_out += _text;
        m_out += '\n';

        return;
    }

private:
    // -- table layout helpers ------------------------------------------------

    // column_count
    //   the table width: the number of declared columns, widened to the widest
    // data row so a ragged row is never truncated.
    D_NODISCARD size_type
    column_count() const
    {
        size_type _cols = m_headers.size();
        size_type _r    = 0;

        for (_r = 0; _r < m_rows.size(); ++_r)
        {
            if (m_rows[_r].size() > _cols)
            {
                _cols = m_rows[_r].size();
            }
        }

        return _cols;
    }

    // computed_widths
    //   the print width of each of _cols columns: the max of the header width,
    // every cell width in that column, and the column's `width` hint.
    D_NODISCARD std::vector<size_type>
    computed_widths(
        size_type _cols
    ) const
    {
        std::vector<size_type> _w(_cols, size_type(0));
        size_type              _c = 0;

        // seed from headers and per-column hints
        for (_c = 0; _c < _cols; ++_c)
        {
            if (_c < m_headers.size())
            {
                _w[_c] = display_width(m_headers[_c]);
            }

            if ( (_c < m_hints.size()) &&
                 (m_hints[_c] > _w[_c]) )
            {
                _w[_c] = m_hints[_c];
            }
        }

        // widen to the data
        size_type _r = 0;

        for (_r = 0; _r < m_rows.size(); ++_r)
        {
            for (_c = 0; _c < m_rows[_r].size(); ++_c)
            {
                const size_type _cw = display_width(m_rows[_r][_c]);

                if (_cw > _w[_c])
                {
                    _w[_c] = _cw;
                }
            }
        }

        return _w;
    }

    // emit_row
    //   append one row of _cells to the buffer, each padded to its column
    // width and alignment and joined by the separator.  A short row is padded
    // with empty cells; the alignment falls back to left past the declared
    // columns.
    void
    emit_row(
        const std::vector<std::string>& _cells,
        const std::vector<size_type>&   _widths,
        size_type                       _cols
    )
    {
        std::string _line;
        size_type   _c = 0;

        for (_c = 0; _c < _cols; ++_c)
        {
            if (_c > size_type(0))
            {
                _line += k_col_sep;
            }

            const std::string    _text  =
                (_c < _cells.size()) ? _cells[_c] : std::string();
            const text_alignment _align =
                (_c < m_aligns.size()) ? m_aligns[_c] : text_alignment::left;

            _line += pad_cell(_text, _widths[_c], _align);
        }

        m_out += _line;
        m_out += '\n';

        return;
    }

    // clear_table
    //   drop all in-flight table state.
    void
    clear_table()
    {
        m_headers.clear();
        m_aligns.clear();
        m_hints.clear();
        m_rows.clear();
        m_row.clear();

        return;
    }

    // -- small static helpers ------------------------------------------------

    // display_width
    //   the print width of a string.  Bytes, not glyphs: exact for ASCII
    // (which test output is); wide-character width is a future refinement.
    D_NODISCARD static size_type
    display_width(
        const std::string& _s
    )
    {
        return _s.size();
    }

    // pad_cell
    //   _text padded to _width under _align.  Over-wide text is returned as-is
    // (never truncated); `justify` pads like `left` for a single cell.
    D_NODISCARD static std::string
    pad_cell(
        const std::string& _text,
        size_type          _width,
        text_alignment     _align
    )
    {
        const size_type _len = display_width(_text);

        // nothing to pad when the cell already fills (or overflows) the column
        if (_len >= _width)
        {
            return _text;
        }

        const size_type _pad = _width - _len;

        if (_align == text_alignment::right)
        {
            return std::string(_pad, ' ') + _text;
        }

        if (_align == text_alignment::center)
        {
            const size_type _left = _pad / size_type(2);

            return std::string(_left, ' ') + _text +
                   std::string(_pad - _left, ' ');
        }

        // left / justify
        return _text + std::string(_pad, ' ');
    }

    // parse_size
    //   the leading unsigned decimal of _s as a size_type, or 0 when _s holds
    // no leading digit (an absent / non-numeric hint).
    D_NODISCARD static size_type
    parse_size(
        const std::string& _s
    )
    {
        size_type _v = 0;
        size_type _i = 0;

        while ( (_i < _s.size())    &&
                (_s[_i] >= '0')     &&
                (_s[_i] <= '9') )
        {
            _v = (_v * size_type(10)) +
                 static_cast<size_type>(_s[_i] - '0');
            ++_i;
        }

        return _v;
    }

    // -- layout constants ----------------------------------------------------

    // k_col_sep / k_rule_join
    //   the inter-column separator and its matching run in the header rule.
    // constexpr pointers to string literals -- initialised in class, so no
    // out-of-class definition and no inline-variable dependency.
    static constexpr const char* k_col_sep   = " | ";
    static constexpr const char* k_rule_join = "-+-";

    // k_list_indent
    //   spaces of indent per list nesting level.
    static constexpr size_type k_list_indent = size_type(2);

    // k_default_rule_cells
    //   the width of a rule that carries no `width` hint.
    static constexpr size_type k_default_rule_cells = size_type(64);

    // -- storage -------------------------------------------------------------

    std::string                           m_out;       // the document so far

    std::vector<bool>                     m_ordered;   // list-nesting: ordered?
    std::vector<size_type>                m_counter;   // list-nesting: item no.

    std::vector<std::string>              m_headers;   // table: column headers
    std::vector<text_alignment>           m_aligns;    // table: column aligns
    std::vector<size_type>                m_hints;     // table: column width hints
    std::vector<std::vector<std::string>> m_rows;      // table: buffered rows
    std::vector<std::string>              m_row;       // table: row in progress
};


NS_END  // djinterp


#endif  // DJINTERP_UTIL_DOCUMENT_RENDERER_
