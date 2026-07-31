/******************************************************************************
* djinterp [utility]                               markdown_document_renderer.hpp
*
*   The Markdown realisation of document_renderer, completing the string half of
* the dialect set (plain / markdown / xml / html).  A heading becomes a `##`
* run, a list becomes `-` or `1.` at the right indent, and a table becomes a
* GitHub-flavoured pipe table -- including the delimiter row, which is the one
* place Markdown can carry a column's alignment.
*
*   MARKDOWN IS A TEXT FORMAT WITH STRUCTURE, so this renderer sits between the
* plain and markup ones: it has real block syntax (unlike plain, which fakes
* headings with underlines) but no attribute surface (unlike markup, which can
* spell any hint).  The hints it can honour it honours -- `align` through the
* delimiter row, `bold` / `italic` through emphasis runs -- and the rest
* (`color`, `font`, `size`, `background`) it drops, per the subframework's rule.
*
*   ESCAPING IS POSITIONAL, NOT TOTAL.  Escaping every CommonMark punctuation
* character would turn ordinary prose into backslash soup ("a well\-formed
* sentence\."), so the rule here is narrower and stated plainly: escape the
* characters that change meaning MID-LINE -- backslash, backtick, asterisk,
* underscore, brackets, and the less-than sign -- and, inside a table cell, the
* pipe as well.  Characters that are only structural at the START of a line
* (`-`, `#`, `.`, `+`) are left alone, because every string this renderer is
* handed is placed after a block marker, never at column zero.  A newline inside
* a cell becomes `<br>`, the only break a pipe table admits.
*
*   PAGE BREAKS have no Markdown spelling.  The convention every md-to-PDF
* converter honours is an inline HTML div carrying the CSS property, so that is
* what page_break emits; a reader that ignores inline HTML sees nothing, which
* is the correct degradation.
*
*   TABLES STREAM WITH ONE ROW OF LOOKAHEAD.  A pipe table's delimiter row must
* follow the header row, and the header is complete once the first begin_row
* arrives -- so, unlike the plain renderer, no row storage is needed: the
* columns are held only until the header is flushed.
*
*   PORTABILITY:
*   C++11 baseline (matches document_renderer).  No third-party dependency and,
* unlike the markup renderer, no escape-policy dependency either.
*
*
* TABLE OF CONTENTS
* =================
* I.    markdown_document_renderer   (the renderer)
*
*
* path:      /inc/djinterp/core/util/document/templates/markdown_document_renderer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_UTIL_DOCUMENT_MARKDOWN_RENDERER_
#define DJINTERP_UTIL_DOCUMENT_MARKDOWN_RENDERER_ 1

// std
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"      // NS_*, D_NODISCARD, D_NOEXCEPT
#include "./document_attributes.hpp"  // doc_attributes, attr_*, text_alignment
#include "./document_renderer.hpp"    // document_renderer, D_OVERRIDE


NS_DJINTERP


// ===========================================================================
// I.   markdown_document_renderer
// ===========================================================================

// markdown_document_renderer
//   class: the Markdown realisation of document_renderer.  It accumulates a
// byte buffer of GitHub-flavoured Markdown: ATX headings, `-` / `N.` lists at
// nesting indent, `---` rules, and pipe tables whose delimiter row carries each
// column's alignment.
//
// Usage:
//   markdown_document_renderer _r;
//   _table.render(_r);
//   const std::string& _bytes = _r.str();
class markdown_document_renderer
    : public document_renderer
{
public:
    // -- public type aliases -------------------------------------------------

    using size_type = std::size_t;

    // -- construction --------------------------------------------------------

    markdown_document_renderer()
        : m_out(),
          m_ordered(),
          m_counter(),
          m_headers(),
          m_aligns(),
          m_row(),
          m_head_open(false)
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

    // -- blocks --------------------------------------------------------------

    void
    heading(
        size_type             _level,
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        size_type _l = (_level < size_type(1)) ? size_type(1) : _level;

        // ATX headings stop at six
        if (_l > size_type(6))
        {
            _l = size_type(6);
        }

        separate();

        m_out += std::string(_l, '#');
        m_out += ' ';
        m_out += emphasised(_text, _attrs);
        m_out += "\n\n";

        return;
    }

    void
    paragraph(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        m_out += emphasised(_text, _attrs);
        m_out += "\n\n";

        return;
    }

    void
    key_value(
        const std::string&    _key,
        const std::string&    _value,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        m_out += "**";
        m_out += escape_inline(_key);
        m_out += ":** ";
        m_out += emphasised(_value, _attrs);
        m_out += "\n\n";

        return;
    }

    void
    rule(
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        separate();

        // a thematic break needs clear air on both sides to not be read as
        // setext underlining of the line above
        m_out += "---\n\n";

        return;
    }

    void
    vertical_space(
        double                /*_amount*/,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        // Markdown has one unit of vertical space: the blank line
        m_out += "\n";

        return;
    }

    void
    page_break() D_OVERRIDE
    {
        separate();

        // no Markdown spelling exists; the inline-HTML form is what every
        // md-to-PDF converter honours, and a plain reader ignores it
        m_out += "<div style=\"page-break-after: always\"></div>\n\n";

        return;
    }

    // -- lists ---------------------------------------------------------------

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
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        const size_type _depth = m_ordered.size();

        // an item outside any list still renders, at depth 0
        const size_type _indent =
            (_depth > size_type(0)) ? ((_depth - size_type(1)) * k_list_indent)
                                    : size_type(0);

        m_out += std::string(_indent, ' ');

        // ordered lists number their items; unordered take a hyphen bullet
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

        m_out += emphasised(_text, _attrs);
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

        // a closed outermost list is followed by a blank line
        if (m_ordered.empty())
        {
            m_out += '\n';
        }

        return;
    }

    // -- tables --------------------------------------------------------------

    void
    begin_table(
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        clear_table();
        separate();

        return;
    }

    void
    table_column(
        const std::string&    _header,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        m_headers.push_back(escape_cell(_header));
        m_aligns.push_back(attr_align(_attrs, text_alignment::left));

        return;
    }

    void
    begin_row(
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        // the header and its delimiter row are flushed once, ahead of the
        // first data row -- the whole of the lookahead a pipe table needs
        flush_head();

        m_row.clear();

        return;
    }

    void
    cell(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        m_row.push_back(emphasised_cell(_text, _attrs));

        return;
    }

    void
    end_row() D_OVERRIDE
    {
        // pad a short row so the pipe count stays constant down the table
        while (m_row.size() < m_headers.size())
        {
            m_row.push_back(std::string());
        }

        emit_row(m_row);
        m_row.clear();

        return;
    }

    void
    end_table() D_OVERRIDE
    {
        // a table declared but never given a row still owes its header
        flush_head();

        clear_table();

        m_out += '\n';

        return;
    }

protected:
    // write_line
    //   the base primitive: a bare line becomes a paragraph, so a template
    // that only calls the defaults still produces valid Markdown.
    void
    write_line(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        paragraph(_text, _attrs);

        return;
    }

private:
    // -- constants -----------------------------------------------------------

    // k_list_indent
    //   the spaces one list level indents by -- two, which every Markdown
    // implementation reads as nesting for both bullet and ordered lists.
    static constexpr size_type k_list_indent = size_type(2);

    // -- escaping ------------------------------------------------------------

    // escape_inline
    //   backslash-escape the characters that change meaning MID-LINE.  See the
    // header note: the line-start-only structural characters are deliberately
    // left alone, because every string handed to this renderer is placed after
    // a block marker.
    D_NODISCARD static std::string
    escape_inline(
        const std::string& _text
    )
    {
        std::string _out;
        size_type   _i = size_type(0);

        _out.reserve(_text.size());

        for (_i = size_type(0); _i < _text.size(); ++_i)
        {
            const char _c = _text[_i];

            // the mid-line significant set; anything else passes through
            if ( (_c == '\\') || (_c == '`') ||
                 (_c == '*')  || (_c == '_') ||
                 (_c == '[')  || (_c == ']') ||
                 (_c == '<') )
            {
                _out += '\\';
            }

            _out += _c;
        }

        return _out;
    }

    // escape_cell
    //   escape_inline plus the two things a pipe table cannot carry raw: the
    // pipe (which would open a column) and a line break (which would end the
    // row).  A break becomes the <br> every GFM implementation accepts.
    D_NODISCARD static std::string
    escape_cell(
        const std::string& _text
    )
    {
        const std::string _escaped = escape_inline(_text);

        std::string _out;
        size_type   _i = size_type(0);

        _out.reserve(_escaped.size());

        for (_i = size_type(0); _i < _escaped.size(); ++_i)
        {
            const char _c = _escaped[_i];

            // a hard break inside a cell is the only break a row admits
            if (_c == '\n')
            {
                _out += "<br>";

                continue;
            }

            // a raw pipe would be read as a column separator
            if (_c == '|')
            {
                _out += "\\|";

                continue;
            }

            _out += _c;
        }

        return _out;
    }

    // emphasised
    //   _text escaped and wrapped in the emphasis runs its hints ask for --
    // the two presentation hints Markdown can actually spell.
    D_NODISCARD static std::string
    emphasised(
        const std::string&    _text,
        const doc_attributes& _attrs
    )
    {
        return wrap_emphasis(escape_inline(_text), _attrs);
    }

    // emphasised_cell
    //   the cell-safe counterpart of emphasised.
    D_NODISCARD static std::string
    emphasised_cell(
        const std::string&    _text,
        const doc_attributes& _attrs
    )
    {
        return wrap_emphasis(escape_cell(_text), _attrs);
    }

    // wrap_emphasis
    //   apply the bold / italic hints to already-escaped text.  Both may
    // apply; an empty string is left bare so a run of `****` never appears.
    D_NODISCARD static std::string
    wrap_emphasis(
        const std::string&    _escaped,
        const doc_attributes& _attrs
    )
    {
        // emphasising nothing produces stray delimiters
        if (_escaped.empty())
        {
            return _escaped;
        }

        std::string _out   = _escaped;
        const bool  _bold  = attr_flag(_attrs, doc_attr_bold,   false);
        const bool  _ital  = attr_flag(_attrs, doc_attr_italic, false);

        if (_ital)
        {
            _out = "*" + _out + "*";
        }

        if (_bold)
        {
            _out = "**" + _out + "**";
        }

        return _out;
    }

    // -- table helpers -------------------------------------------------------

    // delimiter_for
    //   the delimiter-row cell that spells an alignment -- the one hint a pipe
    // table carries structurally.  `justify` has no Markdown spelling and
    // degrades to left.
    D_NODISCARD static const char*
    delimiter_for(
        text_alignment _align
    ) D_NOEXCEPT
    {
        switch (_align)
        {
            case text_alignment::center:  { return ":---:"; }
            case text_alignment::right:   { return "---:";  }
            case text_alignment::left:    { return ":---";  }
            case text_alignment::justify: { return ":---";  }
        }

        return ":---";
    }

    // emit_row
    //   one pipe-delimited row, leading and trailing bars included so the
    // table reads correctly even when a cell is empty.
    void
    emit_row(
        const std::vector<std::string>& _cells
    )
    {
        size_type _i = size_type(0);

        m_out += '|';

        for (_i = size_type(0); _i < _cells.size(); ++_i)
        {
            m_out += ' ';
            m_out += _cells[_i];
            m_out += " |";
        }

        m_out += '\n';

        return;
    }

    // flush_head
    //   emit the header row and its delimiter row, once.  Idempotent, so both
    // begin_row and end_table may call it.
    void
    flush_head()
    {
        size_type _i = size_type(0);

        // already flushed, or a table with no columns at all
        if ( m_head_open ||
             m_headers.empty() )
        {
            return;
        }

        emit_row(m_headers);

        m_out += '|';

        for (_i = size_type(0); _i < m_headers.size(); ++_i)
        {
            const text_alignment _a = (_i < m_aligns.size())
                                          ? m_aligns[_i]
                                          : text_alignment::left;

            m_out += ' ';
            m_out += delimiter_for(_a);
            m_out += " |";
        }

        m_out += '\n';

        m_head_open = true;

        return;
    }

    // clear_table
    //   drop all in-flight table state.
    void
    clear_table()
    {
        m_headers.clear();
        m_aligns.clear();
        m_row.clear();
        m_head_open = false;

        return;
    }

    // separate
    //   ensure the buffer ends in a blank line, so the next block starts in
    // clear air.  A block that is not separated is absorbed into the previous
    // paragraph by every Markdown parser.
    void
    separate()
    {
        // nothing written yet: already in clear air
        if (m_out.empty())
        {
            return;
        }

        if (m_out[m_out.size() - size_type(1)] != '\n')
        {
            m_out += '\n';
        }

        if ( (m_out.size() < size_type(2)) ||
             (m_out[m_out.size() - size_type(2)] != '\n') )
        {
            m_out += '\n';
        }

        return;
    }

    // -- state ---------------------------------------------------------------

    std::string                 m_out;
    std::vector<bool>           m_ordered;    // open lists: ordered?
    std::vector<size_type>      m_counter;    // open lists: item counter
    std::vector<std::string>    m_headers;    // escaped column headers
    std::vector<text_alignment> m_aligns;     // per-column alignment
    std::vector<std::string>    m_row;        // the row being assembled
    bool                        m_head_open;  // header + delimiter emitted?
};


NS_END  // djinterp


#endif  // DJINTERP_UTIL_DOCUMENT_MARKDOWN_RENDERER_
