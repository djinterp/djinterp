/******************************************************************************
* djinterp [utility]                                             document_table.hpp
*
*   A dialect-agnostic TABLE for documents.  document_table is a pure content
* model -- columns (each a header plus alignment / width / style hints) and rows
* of string cells -- with a single render() that streams the table through a
* document_renderer.  It computes no widths, escapes nothing, and knows no
* format: it declares the columns, opens a row per record, emits a cell per
* value, and lets the renderer decide realisation.  The plain renderer lays it
* out as aligned monospace columns; an HTML renderer would emit <table>; a
* Markdown one a pipe table; a PDF one a ruled grid -- all from this one model.
*
*   HINTS, NOT STYLING.  A column carries typed conveniences (a text_alignment,
* a width, a style name) that render() folds into the per-column doc_attributes
* the renderer reads.  A dialect that cannot honour a hint ignores it: a `.txt`
* table drops the style name and keeps the alignment; an HTML table keeps both.
* Extra, dialect-specific hints (a colour, say) go in a column's `extra` bag and
* ride along untouched.
*
*   WHERE LAYOUT LIVES.  Column widths, padding, and separators are the
* RENDERER's business, not the table's -- so the same document_table is exact
* in monospace and idiomatic in HTML without the model knowing either.  This is
* the division the subframework rests on: the template supplies structure and
* hints; the renderer supplies the dialect.
*
*   PORTABILITY:
*   C++11 baseline (matches document_renderer).
*
*
* TABLE OF CONTENTS
* =================
* I.    document_column              (a column: header + alignment / width / style)
* II.   document_table (class)
*       a. construction / configuration
*       b. columns
*       c. rows
*       d. queries
*       e. render / to_string
* III.  make_document_table          (factory)
*
*
* path:      /inc/djinterp/core/util/document/templates/document_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.11
******************************************************************************/

#ifndef DJINTERP_UTIL_DOCUMENT_TABLE_
#define DJINTERP_UTIL_DOCUMENT_TABLE_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"        // NS_*, D_NODISCARD, D_NOEXCEPT
#include "./document_attributes.hpp"    // doc_attributes, doc_attr_*, text_alignment
#include "./document_renderer.hpp"      // document_renderer, plain_document_renderer


NS_DJINTERP


// ===========================================================================
// I.   document_column
// ===========================================================================

// document_column
//   struct: one table column -- a header label and the hints that govern its
// cells.  `align` and `width` (0 = size to content) and `style` (a renderer
// style name, "" = none) are the common ones, folded into doc_attributes at
// render time; `extra` carries any further dialect-specific hints verbatim.
struct document_column
{
    std::string    header;
    text_alignment align;
    std::size_t    width;    // 0 = auto (size to content)
    std::string    style;    // renderer style name ("" = none)
    doc_attributes extra;    // further hints, passed through untouched

    // document_column
    //   constructor: an empty, left-aligned, auto-width column.
    document_column()
        : header(),
          align(text_alignment::left),
          width(0),
          style(),
          extra()
    {}

    // document_column
    //   constructor: a column with a header and (optionally) an alignment,
    // width, and style name.
    explicit document_column(
        std::string    _header,
        text_alignment _align = text_alignment::left,
        std::size_t    _width = 0,
        std::string    _style = std::string()
    )
        : header(static_cast<std::string&&>(_header)),
          align(_align),
          width(_width),
          style(static_cast<std::string&&>(_style)),
          extra()
    {}
};


// ===========================================================================
// II.  document_table (class)
// ===========================================================================

// document_table
//   class: a table as a content model -- an ordered list of document_columns
// and an ordered list of string-cell rows, plus an optional caption and a
// table-level hint bag.  render() streams it through a document_renderer;
// to_string() renders it through the plain renderer for text / debugging.
//
// Usage:
//   document_table _t;
//   _t.add_column("Module")
//     .add_column("Units", text_alignment::right)
//     .add_column("Pass",  text_alignment::right);
//   _t.add_row({ "color", "12", "12" });
//   _t.add_row({ "table", "40", "39" });
//   plain_document_renderer _r;
//   _t.render(_r);                       // or: std::string s = _t.to_string();
class document_table
{
public:
    // -- public type aliases -------------------------------------------------

    using size_type   = std::size_t;
    using column_type = document_column;
    using row_type    = std::vector<std::string>;

    // -- a. construction / configuration -------------------------------------

    // document_table
    //   constructor: an empty table (no columns, no rows, no caption).
    document_table()
        : m_columns(),
          m_rows(),
          m_caption(),
          m_caption_level(k_default_caption_level),
          m_attrs()
    {}

    // set_caption
    //   set a caption emitted (as a heading at _level) above the table; an
    // empty caption emits nothing.  Returns *this for fluent assembly.
    document_table&
    set_caption(
        std::string _caption,
        size_type   _level = k_default_caption_level
    )
    {
        m_caption       = static_cast<std::string&&>(_caption);
        m_caption_level = _level;

        return *this;
    }

    // set_attr
    //   bind a table-level hint (e.g. a `style` for the whole table), passed to
    // the renderer's begin_table.  Returns *this.
    document_table&
    set_attr(
        const std::string& _key,
        const std::string& _value
    )
    {
        m_attrs.set(_key, _value);

        return *this;
    }

    // attributes
    //   the table-level hint bag (mutable), for callers wanting direct access.
    D_NODISCARD doc_attributes&
    attributes() D_NOEXCEPT
    {
        return m_attrs;
    }

    D_NODISCARD const doc_attributes&
    attributes() const D_NOEXCEPT
    {
        return m_attrs;
    }

    // -- b. columns ----------------------------------------------------------

    // add_column
    //   append a column with a header and optional alignment / width / style.
    // Returns *this for fluent assembly.
    document_table&
    add_column(
        std::string    _header,
        text_alignment _align = text_alignment::left,
        size_type      _width = 0,
        std::string    _style = std::string()
    )
    {
        m_columns.push_back(
            document_column(static_cast<std::string&&>(_header),
                            _align,
                            _width,
                            static_cast<std::string&&>(_style)));

        return *this;
    }

    // add_column
    //   append an already-built column.  Returns *this.
    document_table&
    add_column(
        document_column _column
    )
    {
        m_columns.push_back(static_cast<document_column&&>(_column));

        return *this;
    }

    // columns
    //   the column list (read-only).
    D_NODISCARD const std::vector<document_column>&
    columns() const D_NOEXCEPT
    {
        return m_columns;
    }

    // -- c. rows -------------------------------------------------------------

    // add_row
    //   append a row of cells.  A row shorter than the column list is padded
    // with empty cells by the renderer; a longer one widens the table.  Returns
    // *this for fluent assembly.
    document_table&
    add_row(
        row_type _cells
    )
    {
        m_rows.push_back(static_cast<row_type&&>(_cells));

        return *this;
    }

    // add_row
    //   inline-cells convenience over the vector form.
    document_table&
    add_row(
        std::initializer_list<std::string> _cells
    )
    {
        m_rows.push_back(row_type(_cells.begin(), _cells.end()));

        return *this;
    }

    // rows
    //   the row list (read-only).
    D_NODISCARD const std::vector<row_type>&
    rows() const D_NOEXCEPT
    {
        return m_rows;
    }

    // -- d. queries ----------------------------------------------------------

    D_NODISCARD size_type
    column_count() const D_NOEXCEPT
    {
        return m_columns.size();
    }

    D_NODISCARD size_type
    row_count() const D_NOEXCEPT
    {
        return m_rows.size();
    }

    D_NODISCARD bool
    empty() const D_NOEXCEPT
    {
        return ( m_columns.empty() &&
                 m_rows.empty() );
    }

    // -- e. render / to_string -----------------------------------------------

    // render
    //   stream the table through _renderer: the optional caption, then
    // begin_table, one table_column per column (carrying its folded hints),
    // one begin_row / cell.../ end_row per row (each cell tagged with its
    // column's style), then end_table.  The renderer decides layout.
    void
    render(
        document_renderer& _renderer
    ) const
    {
        // the caption rides above the table, as a heading
        if (!m_caption.empty())
        {
            _renderer.heading(m_caption_level, m_caption, doc_attributes());
        }

        // per-column hint bags, built once and reused across every row
        std::vector<doc_attributes> _col_attrs;
        std::vector<doc_attributes> _cell_attrs;

        build_column_attrs(_col_attrs, _cell_attrs);

        _renderer.begin_table(m_attrs);

        // declare the columns (header + alignment / width / style hints)
        size_type _c = 0;

        for (_c = 0; _c < m_columns.size(); ++_c)
        {
            _renderer.table_column(m_columns[_c].header, _col_attrs[_c]);
        }

        // emit the data rows
        size_type _r = 0;

        for (_r = 0; _r < m_rows.size(); ++_r)
        {
            _renderer.begin_row(doc_attributes());

            const row_type& _row = m_rows[_r];
            size_type       _k   = 0;

            for (_k = 0; _k < _row.size(); ++_k)
            {
                // a cell inherits its column's style when one exists; cells
                // past the declared columns carry no style
                _renderer.cell(
                    _row[_k],
                    (_k < _cell_attrs.size()) ? _cell_attrs[_k]
                                              : doc_attributes());
            }

            _renderer.end_row();
        }

        _renderer.end_table();

        return;
    }

    // to_string
    //   the table rendered through the plain (monospace) renderer -- the
    // convenient text face, and the one used in tests.
    D_NODISCARD std::string
    to_string() const
    {
        plain_document_renderer _r;

        render(_r);

        return _r.str();
    }

private:
    // build_column_attrs
    //   fold each column's typed hints into two doc_attributes vectors: the
    // per-column bag handed to table_column (alignment + width + style + the
    // column's `extra`), and the per-column cell bag handed to each cell (its
    // style alone).  Built once per render, not per cell.
    void
    build_column_attrs(
        std::vector<doc_attributes>& _col_attrs,
        std::vector<doc_attributes>& _cell_attrs
    ) const
    {
        _col_attrs.reserve(m_columns.size());
        _cell_attrs.reserve(m_columns.size());

        size_type _c = 0;

        for (_c = 0; _c < m_columns.size(); ++_c)
        {
            const document_column& _col = m_columns[_c];

            // start the column bag from any dialect-specific extras, then layer
            // the typed hints on top
            doc_attributes _ca = _col.extra;

            _ca.set(doc_attr_align, std::string(align_to_string(_col.align)));

            // a width of 0 means "size to content" -- no hint emitted
            if (_col.width > size_type(0))
            {
                _ca.set(doc_attr_width, std::to_string(_col.width));
            }

            // a style name is optional
            if (!_col.style.empty())
            {
                _ca.set(doc_attr_style, _col.style);
            }

            _col_attrs.push_back(_ca);

            // the cell bag carries only the column's style (if any)
            doc_attributes _da;

            if (!_col.style.empty())
            {
                _da.set(doc_attr_style, _col.style);
            }

            _cell_attrs.push_back(_da);
        }

        return;
    }

    // k_default_caption_level
    //   the heading level a caption is emitted at when none is given.
    static constexpr size_type k_default_caption_level = size_type(3);

    // -- storage -------------------------------------------------------------

    std::vector<document_column> m_columns;
    std::vector<row_type>        m_rows;
    std::string                  m_caption;
    size_type                    m_caption_level;
    doc_attributes               m_attrs;
};


// ===========================================================================
// III. make_document_table
// ===========================================================================

// make_document_table
//   function: a document_table whose columns are the given headers, all left-
// aligned and auto-width -- the common quick start before rows are added.
D_NODISCARD inline document_table
make_document_table(
    std::initializer_list<std::string> _headers
)
{
    document_table _t;

    for (const std::string& _h : _headers)
    {
        _t.add_column(_h);
    }

    return _t;
}


NS_END  // djinterp


#endif  // DJINTERP_UTIL_DOCUMENT_TABLE_
