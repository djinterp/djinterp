/******************************************************************************
* djinterp [utility]                            pdf_canvas_document_renderer.hpp
*
*   The SECOND PDF realisation of document_renderer -- the one backed by
* pdf_canvas rather than pdf_template.  It exists because the two PDF flow
* engines are not interchangeable in the one place it matters most:
*
*     pdf_template   flow layout, a named-style registry, and text_template
*                    token substitution -- but NO grid primitive, and its
*                    horizontal metrics are fixed character cells.
*     pdf_canvas     flow layout measured with pdf_metrics (true Adobe Core-14
*                    advances, so wrapping / centring / justification are exact
*                    for proportional faces) and a real gridded, paginating
*                    TABLE -- but no token layer.
*
*   pdf_document_renderer drives the first, so a document template's table falls
* back to a Courier monospace block laid on a flow.  This renderer drives the
* second, so the same template's table becomes an actual ruled grid with
* repeating headers across page breaks, and its paragraphs wrap on measured
* widths.  Neither renderer is a superset: pick by what the document needs.
*
*   THIS IS A BRIDGE, NOT A MERGE.  The two engines duplicate a cursor, page
* advancement, pagination and alignment between them, and that duplication is
* real and worth removing.  Removing it is a change to the PDF layer; putting a
* second document_renderer on the better-measured engine is not, and it makes
* the redundancy concrete: after this header, both engines have exactly one
* consumer apiece and the same interface in front of them, which is the position
* from which a merge is a refactor rather than a rewrite.
*
*   TABLES BUFFER, DELIBERATELY.  pdf_canvas::table takes the whole grid at once
* because it resolves column widths and paginates over it; the renderer protocol
* streams.  So the table calls accumulate rows and the grid is handed over at
* end_table.  That is the one place this renderer holds state a streaming
* dialect would not -- the cost of getting a real grid rather than a text block.
*
*   HINTS -> canvas_style.  A `style` hint names a registered canvas_style
* (register_style); `align` / `color` / `size` / `font` / `bold` / `italic`
* refine it on top.  A hint the canvas cannot honour is never read.  Colours are
* "#RRGGBB"; anything else leaves the base colour alone.
*
*   OPT-IN DEPENDENCY, as pdf_document_renderer: this header pulls in the whole
* pdf.hpp tree, so a text / markup build never includes it and never pays for
* the PDF engine.
*
*   PORTABILITY:
*   C++11 baseline (matches document_renderer and pdf_canvas).
*
*
* TABLE OF CONTENTS
* =================
* I.    pdf_canvas_document_renderer (class)
*       a. construction / style registry
*       b. block overrides
*       c. list overrides
*       d. table overrides (buffered)
*       e. output
*
*
* path:      /inc/djinterp/core/util/pdf/pdf_canvas_document_renderer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.23
******************************************************************************/

#ifndef DJINTERP_UTIL_PDF_CANVAS_DOCUMENT_RENDERER_
#define DJINTERP_UTIL_PDF_CANVAS_DOCUMENT_RENDERER_ 1

// std
#include <cstddef>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
// djinterp
#include "../../djinterp.hpp"                       // NS_*, D_NODISCARD
#include "../document/templates/document_renderer.hpp"
                                                    // document_renderer,
                                                    // doc_attributes, D_OVERRIDE
#include "./pdf_canvas.hpp"                         // pdf_canvas, canvas_style,
                                                    // table_options


NS_DJINTERP


// ===========================================================================
// I.   pdf_canvas_document_renderer
// ===========================================================================

// pdf_canvas_document_renderer
//   class: the pdf_canvas realisation of document_renderer.  A document
// template (title_page, document_table, a layout term's lowering) renders into
// it exactly as it renders into the plain or markup ones; the caller then asks
// for the serialized bytes.
//
// Usage:
//   pdf_canvas_document_renderer _r;
//   _r.register_style("verdict.pass", green_style());
//   _table.render(_r);
//   const std::string _bytes = _r.to_bytes();
class pdf_canvas_document_renderer
    : public document_renderer
{
public:
    // -- public type aliases -------------------------------------------------

    using size_type  = std::size_t;
    using style_map  = std::map<std::string, canvas_style>;
    using row_type   = std::vector<std::string>;
    using grid_type  = std::vector<row_type>;
    using attr_row   = std::vector<doc_attributes>;
    using attr_grid  = std::vector<attr_row>;

    // ------------------------------------------------------------------
    //  a. construction / style registry
    // ------------------------------------------------------------------

    // pdf_canvas_document_renderer
    //   owns a canvas on a page of _page.
    explicit pdf_canvas_document_renderer(
        const pdf_page_size& _page = pdf_page_size::letter()
    )
        : m_owned(new pdf_canvas(_page)),
          m_canvas(nullptr),
          m_styles(),
          m_body(),
          m_grid(),
          m_attrs(),
          m_row(),
          m_row_attrs(),
          m_have_header(false),
          m_zebra(false),
          m_zebra_fill(pdf_color::from_rgb255(246, 246, 248)),
          m_list_depth(0),
          m_ordered(),
          m_counter()
    {
        m_canvas = m_owned.get();
    }

    // pdf_canvas_document_renderer (borrowing)
    //   writes into a caller-owned canvas -- so a caller may lay out its own
    // preamble, hand the canvas over for the document body, and take it back.
    explicit pdf_canvas_document_renderer(
        pdf_canvas& _canvas
    )
        : m_owned(),
          m_canvas(&_canvas),
          m_styles(),
          m_body(),
          m_grid(),
          m_attrs(),
          m_row(),
          m_row_attrs(),
          m_have_header(false),
          m_zebra(false),
          m_zebra_fill(pdf_color::from_rgb255(246, 246, 248)),
          m_list_depth(0),
          m_ordered(),
          m_counter()
    {}

    // register_style
    //   bind a style NAME (the `style` hint's value) to a canvas_style.  An
    // unregistered name simply falls back to the body style, so a producer may
    // emit names a given build does not define.
    void
    register_style(
        const std::string&  _name,
        const canvas_style& _style
    )
    {
        m_styles[_name] = _style;

        return;
    }

    // set_body_style / body_style
    //   the style every block starts from before its hints refine it.
    void
    set_body_style(
        const canvas_style& _style
    )
    {
        m_body = _style;

        return;
    }

    D_NODISCARD const canvas_style&
    body_style() const D_NOEXCEPT
    {
        return m_body;
    }

    // canvas
    //   the driven canvas, for a caller that wants to interleave raw drawing.
    D_NODISCARD pdf_canvas&
    canvas() D_NOEXCEPT
    {
        return *m_canvas;
    }

    // ------------------------------------------------------------------
    //  b. block overrides
    // ------------------------------------------------------------------

    void
    heading(
        size_type             _level,
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // the canvas sizes and spaces a heading by level itself; a style hint
        // overrides that wholesale
        if ( attr_has(_attrs, doc_attr_style) ||
             attr_has(_attrs, doc_attr_background) )
        {
            flow_block(_text, _attrs);

            return;
        }

        m_canvas->heading(_text, static_cast<int>(_level));

        return;
    }

    void
    paragraph(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        flow_block(_text, _attrs);

        return;
    }

    void
    key_value(
        const std::string&    _key,
        const std::string&    _value,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        flow_block(_key + ": " + _value, _attrs);

        return;
    }

    void
    rule(
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        const canvas_style _style = resolve(_attrs);

        m_canvas->rule(1.0, _style.color);

        return;
    }

    void
    vertical_space(
        double                _amount,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        // a zero or negative request still yields a readable gap
        m_canvas->vspace(
            (_amount > 0.0) ? static_cast<pdf_unit>(_amount)
                            : static_cast<pdf_unit>(6.0));

        return;
    }

    void
    page_break() D_OVERRIDE
    {
        m_canvas->page_break();

        return;
    }

    // ------------------------------------------------------------------
    //  c. list overrides
    // ------------------------------------------------------------------

    void
    begin_list(
        bool                  _ordered,
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        ++m_list_depth;

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
        canvas_style _style = resolve(_attrs);
        std::string  _line;

        // nesting indents; the canvas takes indent in points
        _style.indent += static_cast<pdf_unit>(
            (m_list_depth > size_type(0))
                ? ((m_list_depth - size_type(1)) * size_type(18))
                : size_type(0));

        // ordered lists number their items; unordered take a bullet
        if ( (!m_ordered.empty()) &&
             m_ordered.back() )
        {
            ++m_counter.back();
            _line = std::to_string(m_counter.back()) + ". " + _text;
        }
        else
        {
            _line = "- " + _text;
        }

        m_canvas->paragraph(_line, _style);

        return;
    }

    void
    end_list() D_OVERRIDE
    {
        // close the innermost list, if any
        if (m_list_depth > size_type(0))
        {
            --m_list_depth;
            m_ordered.pop_back();
            m_counter.pop_back();
        }

        return;
    }

    // ------------------------------------------------------------------
    //  d. table overrides (buffered)
    // ------------------------------------------------------------------
    //   pdf_canvas::table resolves column widths across the WHOLE grid and
    // paginates over it, so it takes every row at once.  The protocol streams,
    // so the rows accumulate here and the grid is handed over at end_table.

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
        // the header row is the grid's first row, per the canvas's convention
        if (!m_have_header)
        {
            m_grid.push_back(row_type());
            m_attrs.push_back(attr_row());
            m_have_header = true;
        }

        m_grid[0].push_back(_header);
        m_attrs[0].push_back(_attrs);

        return;
    }

    void
    begin_row(
        const doc_attributes& /*_attrs*/
    ) D_OVERRIDE
    {
        m_row.clear();
        m_row_attrs.clear();

        return;
    }

    void
    cell(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        // the hint bag rides alongside the text and is turned into a
        // cell_format at end_table, where the canvas asks for it
        m_row.push_back(_text);
        m_row_attrs.push_back(_attrs);

        return;
    }

    void
    end_row() D_OVERRIDE
    {
        m_grid.push_back(m_row);
        m_attrs.push_back(m_row_attrs);

        m_row.clear();
        m_row_attrs.clear();

        return;
    }

    void
    end_table() D_OVERRIDE
    {
        // an empty table draws nothing
        if (m_grid.empty())
        {
            clear_table();

            return;
        }

        table_options _opts;

        _opts.has_header  = m_have_header;
        _opts.fill_header = m_have_header;

        // the per-cell overrides: the canvas asks (row, col) as it paints, and
        // this answers from the hint bags buffered alongside the text.  The
        // call happens inside table() below, so capturing `this` is safe --
        // clear_table only runs after it returns.
        _opts.cells =
            [this](std::size_t _r, std::size_t _c) -> cell_format
            {
                return cell_format_for(_r, _c);
            };

        m_canvas->table(m_grid, _opts);

        clear_table();

        return;
    }

    // set_zebra
    //   stripe alternate DATA rows with _fill.  A cell carrying its own
    // `background` hint still wins, so a fail-tinted row overrides the stripe
    // rather than fighting it.
    void
    set_zebra(
        const pdf_color& _fill
    )
    {
        m_zebra      = true;
        m_zebra_fill = _fill;

        return;
    }

    // clear_zebra
    //   drop the stripe.
    void
    clear_zebra()
    {
        m_zebra = false;

        return;
    }

    // ------------------------------------------------------------------
    //  e. output
    // ------------------------------------------------------------------

    // to_bytes
    //   finalize and return the serialized PDF.
    D_NODISCARD std::string
    to_bytes()
    {
        return m_canvas->to_bytes();
    }

    // save
    //   finalize and write the document to _path.
    bool
    save(
        const char* _path
    )
    {
        return m_canvas->save(_path);
    }

protected:
    // write_line
    //   the base primitive: a bare line is one paragraph, so a template that
    // only calls the defaults still lays out.
    void
    write_line(
        const std::string&    _text,
        const doc_attributes& _attrs
    ) D_OVERRIDE
    {
        flow_block(_text, _attrs);

        return;
    }

private:
    // flow_block
    //   emit one block of text: a banded strip when it carries a `background`
    // hint, an ordinary paragraph otherwise.  Every block override funnels
    // through here so the band is available uniformly -- a section header, a
    // suite band and a callout are all "a paragraph with a ground".
    void
    flow_block(
        const std::string&    _text,
        const doc_attributes& _attrs
    )
    {
        const canvas_style _style = resolve(_attrs);
        const std::string  _bg    =
            attr_or(_attrs, doc_attr_background, std::string());

        // no ground: the ordinary flow path
        if (_bg.empty())
        {
            m_canvas->paragraph(_text, _style);

            return;
        }

        pdf_color _fill = pdf_color::white();

        parse_hex_color(_bg, _fill);

        m_canvas->band(_text, _fill, _style);

        return;
    }

    // -- hint resolution -----------------------------------------------------

    // resolve
    //   the body style, or a registered style the `style` hint names, refined
    // by whichever of align / color / size / font / bold / italic are present.
    D_NODISCARD canvas_style
    resolve(
        const doc_attributes& _attrs
    ) const
    {
        canvas_style _style = m_body;

        // a registered style name replaces the base wholesale
        const std::string _name = attr_or(_attrs, doc_attr_style, std::string());

        if (!_name.empty())
        {
            style_map::const_iterator _it = m_styles.find(_name);

            // an unregistered name falls back to the body style, by design
            if (_it != m_styles.end())
            {
                _style = _it->second;
            }
        }

        _style.align = attr_align(_attrs, _style.align);

        const std::string _size = attr_or(_attrs, doc_attr_size, std::string());

        if (!_size.empty())
        {
            const double _points = std::atof(_size.c_str());

            if (_points > 0.0)
            {
                _style.font.size = static_cast<pdf_unit>(_points);
            }
        }

        const std::string _color =
            attr_or(_attrs, doc_attr_color, std::string());

        if (!_color.empty())
        {
            parse_hex_color(_color, _style.color);
        }

        _style.font.family = resolve_family(_attrs, _style.font.family);

        return _style;
    }

    // resolve_family
    //   the base-14 face the font / bold / italic hints name, starting from
    // _base.  A `font` hint naming a family switches family; the flags then
    // pick the weight / slope variant within it.
    D_NODISCARD static pdf_base_font
    resolve_family(
        const doc_attributes& _attrs,
        pdf_base_font         _base
    )
    {
        const std::string _font = attr_or(_attrs, doc_attr_font, std::string());
        const bool        _bold = attr_flag(_attrs, doc_attr_bold,   false);
        const bool        _ital = attr_flag(_attrs, doc_attr_italic, false);

        bool _mono  = is_courier(_base);
        bool _serif = is_times(_base);

        // a font hint names a FAMILY; the exact face comes from the flags
        if (!_font.empty())
        {
            _mono  = ( (_font == "mono")    || (_font == "monospace") ||
                       (_font == "Courier") || (_font == "courier") );
            _serif = ( (_font == "serif")   || (_font == "Times")     ||
                       (_font == "times") );
        }

        if (_mono)
        {
            if (_bold && _ital) { return pdf_base_font::courier_bold_oblique; }
            if (_bold)          { return pdf_base_font::courier_bold;         }
            if (_ital)          { return pdf_base_font::courier_oblique;      }

            return pdf_base_font::courier;
        }

        if (_serif)
        {
            if (_bold && _ital) { return pdf_base_font::times_bold_italic; }
            if (_bold)          { return pdf_base_font::times_bold;        }
            if (_ital)          { return pdf_base_font::times_italic;      }

            return pdf_base_font::times_roman;
        }

        if (_bold && _ital) { return pdf_base_font::helvetica_bold_oblique; }
        if (_bold)          { return pdf_base_font::helvetica_bold;         }
        if (_ital)          { return pdf_base_font::helvetica_oblique;      }

        // no hint at all keeps whatever the base style carried
        if ( _font.empty() &&
             (!_bold) &&
             (!_ital) )
        {
            return _base;
        }

        return pdf_base_font::helvetica;
    }

    D_NODISCARD static bool
    is_courier(
        pdf_base_font _f
    ) D_NOEXCEPT
    {
        return ( (_f == pdf_base_font::courier)              ||
                 (_f == pdf_base_font::courier_bold)         ||
                 (_f == pdf_base_font::courier_oblique)      ||
                 (_f == pdf_base_font::courier_bold_oblique) );
    }

    D_NODISCARD static bool
    is_times(
        pdf_base_font _f
    ) D_NOEXCEPT
    {
        return ( (_f == pdf_base_font::times_roman)  ||
                 (_f == pdf_base_font::times_bold)   ||
                 (_f == pdf_base_font::times_italic) ||
                 (_f == pdf_base_font::times_bold_italic) );
    }

    // parse_hex_color
    //   "#RRGGBB" into _out, leaving it untouched on any other spelling -- a
    // colour name this layer does not know is simply not honoured.
    static void
    parse_hex_color(
        const std::string& _text,
        pdf_color&         _out
    )
    {
        // only the six-digit hash form is recognised
        if ( (_text.size() != std::size_t(7)) ||
             (_text[0] != '#') )
        {
            return;
        }

        const int _r = hex_byte(_text[1], _text[2]);
        const int _g = hex_byte(_text[3], _text[4]);
        const int _b = hex_byte(_text[5], _text[6]);

        // a malformed digit pair leaves the base colour alone
        if ( (_r < 0) || (_g < 0) || (_b < 0) )
        {
            return;
        }

        _out = pdf_color(static_cast<double>(_r) / 255.0,
                         static_cast<double>(_g) / 255.0,
                         static_cast<double>(_b) / 255.0);

        return;
    }

    D_NODISCARD static int
    hex_byte(
        char _hi,
        char _lo
    ) D_NOEXCEPT
    {
        const int _h = hex_digit(_hi);
        const int _l = hex_digit(_lo);

        if ( (_h < 0) || (_l < 0) )
        {
            return -1;
        }

        return ((_h * 16) + _l);
    }

    D_NODISCARD static int
    hex_digit(
        char _c
    ) D_NOEXCEPT
    {
        if ((_c >= '0') && (_c <= '9')) { return (_c - '0');      }
        if ((_c >= 'a') && (_c <= 'f')) { return (_c - 'a' + 10); }
        if ((_c >= 'A') && (_c <= 'F')) { return (_c - 'A' + 10); }

        return -1;
    }

    // cell_format_for
    //   the overrides cell (_r, _c) carries: the zebra stripe first (so it is
    // the weakest), then whatever the cell's own hint bag asks for.  A
    // registered `style` supplies colour / font / alignment; `background`,
    // `color` and `align` refine on top.  A header cell is never striped.
    D_NODISCARD cell_format
    cell_format_for(
        std::size_t _r,
        std::size_t _c
    ) const
    {
        cell_format _fmt;

        const bool _is_header = ( m_have_header &&
                                  (_r == std::size_t(0)) );

        // the stripe is the weakest layer: data rows only, alternating
        if ( m_zebra &&
             (!_is_header) &&
             ((_r % std::size_t(2)) == std::size_t(0)) )
        {
            _fmt.has_fill = true;
            _fmt.fill     = m_zebra_fill;
        }

        // a row or column the bags do not reach keeps just the stripe
        if ( (_r >= m_attrs.size()) ||
             (_c >= m_attrs[_r].size()) )
        {
            return _fmt;
        }

        const doc_attributes& _bag = m_attrs[_r][_c];

        // a registered style name seeds colour / font / alignment
        const std::string _name = attr_or(_bag, doc_attr_style, std::string());

        if (!_name.empty())
        {
            style_map::const_iterator _it = m_styles.find(_name);

            if (_it != m_styles.end())
            {
                _fmt.has_text_color = true;
                _fmt.text_color     = _it->second.color;
                _fmt.has_font       = true;
                _fmt.font           = _it->second.font;
                _fmt.has_align      = true;
                _fmt.align          = _it->second.align;
            }
        }

        // an explicit background beats the stripe
        const std::string _bg =
            attr_or(_bag, doc_attr_background, std::string());

        if (!_bg.empty())
        {
            pdf_color _fill = _fmt.has_fill ? _fmt.fill : pdf_color::white();

            parse_hex_color(_bg, _fill);

            _fmt.has_fill = true;
            _fmt.fill     = _fill;
        }

        const std::string _fg = attr_or(_bag, doc_attr_color, std::string());

        if (!_fg.empty())
        {
            pdf_color _text =
                _fmt.has_text_color ? _fmt.text_color : pdf_color::black();

            parse_hex_color(_fg, _text);

            _fmt.has_text_color = true;
            _fmt.text_color     = _text;
        }

        // an alignment hint always wins over a style's default
        if (attr_has(_bag, doc_attr_align))
        {
            _fmt.has_align = true;
            _fmt.align     = attr_align(_bag, _fmt.align);
        }

        return _fmt;
    }

    // clear_table
    //   drop all buffered grid state.
    void
    clear_table()
    {
        m_grid.clear();
        m_attrs.clear();
        m_row.clear();
        m_row_attrs.clear();
        m_have_header = false;

        return;
    }

    // -- state ---------------------------------------------------------------
    //   Declared in initialiser-list order.

    std::unique_ptr<pdf_canvas> m_owned;        // when this renderer owns one
    pdf_canvas*                 m_canvas;       // owned or borrowed
    style_map                   m_styles;       // name -> canvas_style
    canvas_style                m_body;         // the base every block starts from

    grid_type                   m_grid;         // buffered table, header first
    attr_grid                   m_attrs;        // per-cell hint bags, parallel
    row_type                    m_row;          // the row being assembled
    attr_row                    m_row_attrs;    // its hint bags
    bool                        m_have_header;

    bool                        m_zebra;        // stripe alternate data rows?
    pdf_color                   m_zebra_fill;

    size_type                   m_list_depth;
    std::vector<bool>           m_ordered;
    std::vector<size_type>      m_counter;
};


NS_END  // djinterp


#endif  // DJINTERP_UTIL_PDF_CANVAS_DOCUMENT_RENDERER_
