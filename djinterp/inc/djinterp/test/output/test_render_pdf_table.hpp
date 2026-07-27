/******************************************************************************
* djinterp [test]                                    test_render_pdf_table.hpp
*
*   The DEFAULT DTest PDF report renderer: draws a test_report as an
* information-dense, assertion-level PDF whose per-test tables are REAL ruled
* and shaded tables -- one row per assertion, columns RESULT / # / ASSERTION /
* EXPECTED / ACTUAL, with pass/fail chips, zebra striping, and fail-tinted
* rows.  This supersedes the monospace-block table arm in test_render_pdf.hpp:
* rather than laying text on a Courier grid, it paints each cell directly on
* pdf_document -- a filled rectangle for the background, a stroked rectangle
* for the rule, and metric-placed text -- so cells can carry per-cell fills and
* colors (the chip greens/reds, the zebra, the fail tint) that the foundation's
* single-fill table() helper cannot express.
*
*   The layout mirrors the design reference (a slate palette, a dark suite
* band, a metadata block, per-test cards).  Because only the base-14 fonts are
* guaranteed, the sans family renders as Helvetica and the code columns as
* Courier (the reference's Aptos / Cascadia Mono).  Geometry is US Letter
* portrait with the reference's margins, giving the same proportional grid.
*
*   MODEL.   report -> document, report_module -> a SUITE section (band +
* metadata), report_unit -> a TEST card (chip + name + assertion table),
* report_check -> one assertion ROW.  The EXPECTED / ACTUAL columns read
* report_check's expected / actual (see the model enrichment in
* test_report.hpp); a check that carries none renders them blank and the
* ASSERTION column falls back to its description -- so the table degrades
* gracefully for bare pass/fail leaves and fills in fully when tests record
* values (via check(expression, expected, actual, ok) / D_CHECK_EQ).
*
*   PAGINATION.  Rows advance a top-down cursor in the foundation's bottom-left
* user space; when a row would cross the bottom margin the painter starts a new
* page and re-draws the column header, so multi-page suites stay legible.
*
*   PORTABILITY:
*   C++11 baseline; requires C++ (PDF text assembly).
*
*
* path:      /inc/djinterp/test/output/test_render_pdf_table.hpp
* link(s):   TBA
* author(s): DTest contributors                            created: 2026.07.12
******************************************************************************/

#ifndef DJINTERP_TEST_RENDER_PDF_TABLE_
#define DJINTERP_TEST_RENDER_PDF_TABLE_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"               // NS_*, D_NODISCARD, D_NOEXCEPT
#include "../../core/util/pdf/pdf.hpp"           // pdf_document + primitives (whole chain)
#include "../../core/util/pdf/pdf_metrics.hpp"   // text_width, wrap_to_width
#include "./test_report.hpp"                    // test_report model (enriched)


NS_DJINTERP
NS_TEST


namespace pdf_table_internal
{
    // ---- palette (slate, matching the design reference) --------------------
    inline pdf_color c_ink()        { return pdf_color::from_rgb255( 31,  41,  55); }
    inline pdf_color c_muted()      { return pdf_color::from_rgb255(100, 116, 139); }
    inline pdf_color c_on_dark()    { return pdf_color::from_rgb255(203, 213, 225); }
    inline pdf_color c_line()       { return pdf_color::from_rgb255(226, 232, 240); }
    inline pdf_color c_band()       { return pdf_color::from_rgb255( 31,  41,  55); }
    inline pdf_color c_white()      { return pdf_color::from_rgb255(255, 255, 255); }
    inline pdf_color c_zebra()      { return pdf_color::from_rgb255(248, 250, 252); }
    inline pdf_color c_label_bg()   { return pdf_color::from_rgb255(241, 245, 249); }
    inline pdf_color c_pass_bg()    { return pdf_color::from_rgb255(220, 252, 231); }
    inline pdf_color c_pass_ink()   { return pdf_color::from_rgb255( 22, 101,  52); }
    inline pdf_color c_pass_light() { return pdf_color::from_rgb255(134, 239, 172); }
    inline pdf_color c_fail_bg()    { return pdf_color::from_rgb255(254, 226, 226); }
    inline pdf_color c_fail_ink()   { return pdf_color::from_rgb255(153,  27,  27); }
    inline pdf_color c_fail_light() { return pdf_color::from_rgb255(252, 165, 165); }
    inline pdf_color c_fail_zebra() { return pdf_color::from_rgb255(254, 242, 242); }
    inline pdf_color c_skip_ink()   { return pdf_color::from_rgb255( 71,  85, 105); }

    // fonts (base-14): sans -> Helvetica, code -> Courier
    inline pdf_font f_sans(pdf_unit _sz)      { return pdf_font(pdf_base_font::helvetica,      _sz); }
    inline pdf_font f_sans_bold(pdf_unit _sz) { return pdf_font(pdf_base_font::helvetica_bold, _sz); }
    inline pdf_font f_mono(pdf_unit _sz)      { return pdf_font(pdf_base_font::courier,        _sz); }
    inline pdf_font f_mono_bold(pdf_unit _sz) { return pdf_font(pdf_base_font::courier_bold,   _sz); }

    // horizontal text placement within a cell span
    enum text_align_x { align_left = 0, align_center = 1, align_right = 2 };

    // ---- painter -----------------------------------------------------------
    //   a top-down layout cursor over pdf_document's bottom-left user space:
    // fills, strokes, and metric-placed text, with page-break bookkeeping.
    struct painter
    {
        pdf_document& doc;

        pdf_unit page_w;
        pdf_unit page_h;
        pdf_unit m_left;
        pdf_unit m_right;
        pdf_unit m_top;
        pdf_unit m_bottom;

        pdf_unit y;        // current top edge (descends down the page)

        explicit painter(
            pdf_document& _doc
        )
            : doc(_doc),
              page_w(612.0), page_h(792.0),
              m_left(43.2), m_right(43.2), m_top(36.0), m_bottom(36.0),
              y(0.0)
        {}

        pdf_unit content_w() const
        {
            return page_w - m_left - m_right;
        }

        // begin
        //   opens the first page and seats the cursor at the top margin.
        void
        begin()
        {
            doc.add_page(pdf_page_size::letter());
            y = page_h - m_top;

            return;
        }

        // new_page
        //   starts a fresh page and reseats the cursor.
        void
        new_page()
        {
            doc.add_page(pdf_page_size::letter());
            y = page_h - m_top;

            return;
        }

        // room
        //   true iff a block of height _h fits before the bottom margin.
        bool
        room(
            pdf_unit _h
        ) const
        {
            return ( (y - _h) >= m_bottom );
        }

        // fill
        //   fills a rectangle whose TOP-left corner is (_x, _top).
        void
        fill(
            pdf_unit         _x,
            pdf_unit         _top,
            pdf_unit         _w,
            pdf_unit         _h,
            const pdf_color& _c
        )
        {
            doc.rect(pdf_rect(_x, _top - _h, _w, _h), pdf_paint::filled(_c));

            return;
        }

        // stroke
        //   strokes the outline of a top-anchored rectangle.
        void
        stroke(
            pdf_unit         _x,
            pdf_unit         _top,
            pdf_unit         _w,
            pdf_unit         _h,
            const pdf_color& _c,
            pdf_unit         _lw = 0.5
        )
        {
            doc.rect(pdf_rect(_x, _top - _h, _w, _h), pdf_paint::stroked(_c, _lw));

            return;
        }

        // glyphs
        //   draws a single line of text with its baseline at (_x, _baseline).
        void
        glyphs(
            pdf_unit           _x,
            pdf_unit           _baseline,
            const std::string& _s,
            const pdf_font&    _font,
            const pdf_color&   _color
        )
        {
            if (_s.empty())
            {
                return;
            }

            pdf_text_options _o(_font);
            _o.color = _color;

            doc.text(pdf_point(_x, _baseline), _s, _o);

            return;
        }

        // cell_text
        //   lays _s (wrapped to fit) into a cell [_x.._x+_w] x [top .._top-_h],
        // aligned horizontally per _align, and vertically centred as a block.
        void
        cell_text(
            pdf_unit           _x,
            pdf_unit           _top,
            pdf_unit           _w,
            pdf_unit           _h,
            const std::string& _s,
            const pdf_font&    _font,
            const pdf_color&   _color,
            text_align_x       _align,
            pdf_unit           _pad = 3.0
        )
        {
            if (_s.empty())
            {
                return;
            }

            const pdf_unit _inner = _w - (2.0 * _pad);
            const pdf_unit _lh    = _font.size * 1.22;

            std::vector<std::string> _lines = wrap_to_width(_font, _s, _inner);

            const pdf_unit _block = static_cast<pdf_unit>(_lines.size()) * _lh;
            const pdf_unit _start = _top - ((_h - _block) / 2.0);   // top of the text block

            std::size_t _i = 0;

            for (_i = 0; _i < _lines.size(); ++_i)
            {
                const std::string& _ln = _lines[_i];
                const pdf_unit _tw = text_width(_font, _ln);

                pdf_unit _tx = _x + _pad;

                if (_align == align_center)
                {
                    _tx = _x + ((_w - _tw) / 2.0);
                }
                else if (_align == align_right)
                {
                    _tx = _x + _w - _pad - _tw;
                }

                // baseline sits ~one cap-height below the line's top
                const pdf_unit _baseline =
                    _start - (static_cast<pdf_unit>(_i) * _lh) - (_font.size * 0.80);

                glyphs(_tx, _baseline, _ln, _font, _color);
            }

            return;
        }

        // measure_h
        //   the height a cell of width _w needs to hold _s at _font.
        pdf_unit
        measure_h(
            pdf_unit           _w,
            const std::string& _s,
            const pdf_font&    _font,
            pdf_unit           _pad = 3.0,
            pdf_unit           _vpad = 3.0
        )
        {
            const pdf_unit _inner = _w - (2.0 * _pad);
            const pdf_unit _lh    = _font.size * 1.22;

            std::size_t _n = 1;

            if (!_s.empty())
            {
                _n = wrap_to_width(_font, _s, _inner).size();
            }

            pdf_unit _h = (static_cast<pdf_unit>(_n) * _lh) + (2.0 * _vpad);

            if (_h < 15.5)
            {
                _h = 15.5;
            }

            return _h;
        }
    };

    // ---- status vocabulary -------------------------------------------------
    inline void
    check_face(
        test_status  _status,
        const char*& _word,
        pdf_color&   _bg,
        pdf_color&   _ink
    )
    {
        switch (_status)
        {
            case test_status::passed:  { _word = "PASS";  _bg = c_pass_bg();  _ink = c_pass_ink(); break; }
            case test_status::failed:  { _word = "FAIL";  _bg = c_fail_bg();  _ink = c_fail_ink(); break; }
            case test_status::error:   { _word = "ERROR"; _bg = c_fail_bg();  _ink = c_fail_ink(); break; }
            case test_status::skipped: { _word = "SKIP";  _bg = c_label_bg(); _ink = c_skip_ink(); break; }
            default:                   { _word = "PEND";  _bg = c_label_bg(); _ink = c_skip_ink(); break; }
        }

        return;
    }

    // ratio / percent over the model's roll-up counts
    inline std::string ratio(std::size_t _p, std::size_t _t)
    {
        return std::to_string(_p) + " / " + std::to_string(_t);
    }
    inline std::string percent(std::size_t _p, std::size_t _t)
    {
        if (_t == 0) { return "0%"; }
        const std::size_t _r = (_p * 100 + _t / 2) / _t;
        return std::to_string(_r) + "%";
    }

    // ---- assertion-grid geometry -------------------------------------------
    //   the reference's 979 / 547 / 5544 / 1720 / 1720 twip columns, taken as
    // fractions of the content width so they scale with the margins.
    struct grid
    {
        pdf_unit x[5];   // left edge of each column
        pdf_unit w[5];   // width of each column

        explicit grid(const painter& _p)
        {
            const pdf_unit _cw = _p.content_w();
            const pdf_unit _total = 10510.0;
            const pdf_unit _tw[5] = { 979.0, 547.0, 5544.0, 1720.0, 1720.0 };

            pdf_unit _cursor = _p.m_left;
            std::size_t _i = 0;

            for (_i = 0; _i < 5; ++_i)
            {
                w[_i] = _cw * (_tw[_i] / _total);
                x[_i] = _cursor;
                _cursor += w[_i];
            }
        }
    };

    // module_is_test_level
    //   true when no check anywhere in the module carries an expected/actual
    // value - i.e. the module reports test-level results (each check is a named
    // test), not assertion-level ones.  Drives the RESULT/#/TEST/DESCRIPTION
    // vs RESULT/#/ASSERTION/EXPECTED/ACTUAL column choice and the roll-up nouns.
    inline bool
    module_is_test_level(
        const report_module& _m
    )
    {
        std::size_t _ui = 0;

        for (_ui = 0; _ui < _m.units.size(); ++_ui)
        {
            const report_unit& _u = _m.units[_ui];

            std::size_t _ci = 0;
            for (_ci = 0; _ci < _u.checks.size(); ++_ci)
            {
                if (!_u.checks[_ci].expected.empty() ||
                    !_u.checks[_ci].actual.empty())
                {
                    return false;
                }
            }
        }

        return true;
    }

    // test_level_cols
    //   the TEST / DESCRIPTION split for a test-level table: reuses the grid's
    // RESULT and # columns and repartitions the remaining width into a narrow
    // identifier column and a wide prose column.
    inline void
    test_level_cols(
        const grid& _g,
        pdf_unit&   _test_w,
        pdf_unit&   _desc_x,
        pdf_unit&   _desc_w
    )
    {
        const pdf_unit _text_total = _g.w[2] + _g.w[3] + _g.w[4];

        _test_w = _text_total * 0.42;
        _desc_x = _g.x[2] + _test_w;
        _desc_w = (_g.x[4] + _g.w[4]) - _desc_x;

        return;
    }

    // ---- suite band --------------------------------------------------------

    // band
    //   the dark module header: "UNIT TEST SUITE / name" left, verdict + roll-up
    // right.  Full content width.
    inline void
    band(
        painter&             _p,
        const report_module& _m,
        bool                 _test_level
    )
    {
        const pdf_unit _h    = 42.0;
        const pdf_unit _x    = _p.m_left;
        const pdf_unit _cw   = _p.content_w();

        if (!_p.room(_h))
        {
            _p.new_page();
        }

        const pdf_unit _top = _p.y;

        _p.fill(_x, _top, _cw, _h, c_band());

        // left: label + name
        _p.glyphs(_x + 12.0, _top - 15.0, "UNIT TEST SUITE",
                  f_sans(7.0), c_on_dark());
        _p.glyphs(_x + 12.0, _top - 30.0, _m.name,
                  f_sans_bold(14.0), c_white());

        // right: verdict word + stat line (right-aligned within the band)
        const bool  _ok    = (_m.verdict() == report_verdict::passed);
        const char* _vword = _ok ? "PASSED" : "FAILED";
        const pdf_color _vc = _ok ? c_pass_light() : c_fail_light();

        const pdf_font _vf = f_sans_bold(13.0);
        const pdf_unit _vw = text_width(_vf, _vword);
        _p.glyphs(_x + _cw - 12.0 - _vw, _top - 16.0, _vword, _vf, _vc);

        const std::string _stat = _test_level
            ? ( ratio(_m.passed_units(),  _m.total_units())  + " sections    " +
                ratio(_m.passed_checks(), _m.total_checks()) + " tests" )
            : ( ratio(_m.passed_units(),  _m.total_units())  + " tests    " +
                ratio(_m.passed_checks(), _m.total_checks()) + " assertions" );
        const pdf_font _sf = f_sans(7.5);
        const pdf_unit _sw = text_width(_sf, _stat);
        _p.glyphs(_x + _cw - 12.0 - _sw, _top - 30.0, _stat, _sf, c_on_dark());

        _p.y = _top - _h;

        return;
    }

    // meta_row
    //   one label / value row of the metadata block.
    inline void
    meta_row(
        painter&           _p,
        const std::string& _label,
        const std::string& _value,
        const pdf_font&    _value_font,
        const pdf_color&   _value_color
    )
    {
        const pdf_unit _x  = _p.m_left;
        const pdf_unit _cw = _p.content_w();
        const pdf_unit _lw = _cw * (3000.0 / 10510.0);
        const pdf_unit _vw = _cw - _lw;

        const pdf_unit _h = _p.measure_h(_vw, _value, _value_font);

        if (!_p.room(_h))
        {
            _p.new_page();
        }

        const pdf_unit _top = _p.y;

        _p.fill(_x,       _top, _lw, _h, c_label_bg());
        _p.fill(_x + _lw, _top, _vw, _h, c_white());
        _p.stroke(_x,       _top, _lw, _h, c_line());
        _p.stroke(_x + _lw, _top, _vw, _h, c_line());

        _p.cell_text(_x, _top, _lw, _h, _label, f_sans_bold(7.0), c_muted(), align_left);
        _p.cell_text(_x + _lw, _top, _vw, _h, _value, _value_font, _value_color, align_left);

        _p.y = _top - _h;

        return;
    }

    // meta_block
    //   the label / value block beneath a suite band.
    inline void
    meta_block(
        painter&             _p,
        const report_module& _m,
        bool                 _test_level
    )
    {
        const bool _ok = (_m.verdict() == report_verdict::passed);

        meta_row(_p, "MODULE", _m.name, f_mono(8.0), c_ink());

        if (!_m.description.empty())
        {
            meta_row(_p, "DESCRIPTION", _m.description, f_sans(8.5), c_ink());
        }

        if (_test_level)
        {
            meta_row(_p, "SECTIONS", ratio(_m.passed_units(),  _m.total_units()),  f_sans(8.5), c_ink());
            meta_row(_p, "TESTS",    ratio(_m.passed_checks(), _m.total_checks()), f_sans(8.5), c_ink());
            meta_row(_p, "RESULT",
                     percent(_m.passed_checks(), _m.total_checks()) +
                         "    (" + ratio(_m.passed_checks(), _m.total_checks()) + " tests)",
                     f_sans_bold(8.5),
                     _ok ? c_pass_ink() : c_fail_ink());
        }
        else
        {
            meta_row(_p, "TESTS",      ratio(_m.passed_units(),  _m.total_units()),  f_sans(8.5), c_ink());
            meta_row(_p, "ASSERTIONS", ratio(_m.passed_checks(), _m.total_checks()), f_sans(8.5), c_ink());
            meta_row(_p, "PASS RATE",
                     percent(_m.passed_checks(), _m.total_checks()) +
                         "    (" + ratio(_m.passed_checks(), _m.total_checks()) + " assertions)",
                     f_sans_bold(8.5),
                     _ok ? c_pass_ink() : c_fail_ink());
        }

        return;
    }

    // section_label
    //   a small muted caps label ("TEST DETAIL").
    inline void
    section_label(
        painter&           _p,
        const std::string& _text
    )
    {
        const pdf_unit _h = 18.0;

        if (!_p.room(_h))
        {
            _p.new_page();
        }

        _p.y -= 8.0;
        _p.glyphs(_p.m_left, _p.y - 8.0, _text, f_sans_bold(7.0), c_muted());
        _p.y -= 12.0;

        return;
    }

    // header_row
    //   the dark column header, re-drawn on each page a table spills onto.
    // Assertion-level: RESULT / # / ASSERTION / EXPECTED / ACTUAL.
    // Test-level:      RESULT / # / TEST / DESCRIPTION.
    inline void
    header_row(
        painter&    _p,
        const grid& _g,
        bool        _test_level
    )
    {
        const pdf_unit _h   = 16.0;
        const pdf_unit _top = _p.y;

        if (_test_level)
        {
            pdf_unit _tw = 0.0, _dx = 0.0, _dw = 0.0;
            test_level_cols(_g, _tw, _dx, _dw);

            _p.fill(_g.x[0], _top, _g.w[0], _h, c_band());
            _p.fill(_g.x[1], _top, _g.w[1], _h, c_band());
            _p.fill(_g.x[2], _top, _tw,     _h, c_band());
            _p.fill(_dx,     _top, _dw,     _h, c_band());

            _p.cell_text(_g.x[0], _top, _g.w[0], _h, "RESULT",      f_sans_bold(7.0), c_white(), align_center);
            _p.cell_text(_g.x[1], _top, _g.w[1], _h, "#",           f_sans_bold(7.0), c_white(), align_right);
            _p.cell_text(_g.x[2], _top, _tw,     _h, "TEST",        f_sans_bold(7.0), c_white(), align_left);
            _p.cell_text(_dx,     _top, _dw,     _h, "DESCRIPTION", f_sans_bold(7.0), c_white(), align_left);

            _p.y = _top - _h;

            return;
        }

        static const char* _labels[5] = { "RESULT", "#", "ASSERTION", "EXPECTED", "ACTUAL" };
        static const text_align_x _al[5] =
            { align_center, align_right, align_left, align_left, align_left };

        std::size_t _i = 0;

        for (_i = 0; _i < 5; ++_i)
        {
            _p.fill(_g.x[_i], _top, _g.w[_i], _h, c_band());
            _p.cell_text(_g.x[_i], _top, _g.w[_i], _h, _labels[_i],
                         f_sans_bold(7.0), c_white(), _al[_i]);
        }

        _p.y = _top - _h;

        return;
    }

    // test_table
    //   a card per unit: a band row (chip | name | count), an optional
    // description line (the unit's descriptor), the column header, then one row
    // per check, paginating with a repeated header.  Test-level checks render
    // RESULT / # / TEST / DESCRIPTION; assertion-level checks render
    // RESULT / # / ASSERTION / EXPECTED / ACTUAL.
    inline void
    test_table(
        painter&           _p,
        const report_unit& _u,
        bool               _test_level
    )
    {
        const grid _g(_p);

        const bool _pass  = _u.passed();
        pdf_color  _bg    = _pass ? c_pass_bg()  : c_fail_bg();
        pdf_color  _ink   = _pass ? c_pass_ink() : c_fail_ink();

        // -- band: chip | test name | assertion count --
        {
            const pdf_unit _h = 20.0;

            if (!_p.room(_h + 16.0))   // keep the band with at least its header
            {
                _p.new_page();
            }

            const pdf_unit _top = _p.y;

            const pdf_unit _chip_w = _g.w[0];
            const pdf_unit _name_x = _g.x[1];
            const pdf_unit _name_w = _g.w[1] + _g.w[2];
            const pdf_unit _cnt_x  = _g.x[3];
            const pdf_unit _cnt_w  = _g.w[3] + _g.w[4];

            _p.fill(_g.x[0], _top, _chip_w, _h, _bg);
            _p.fill(_name_x, _top, _name_w, _h, c_zebra());
            _p.fill(_cnt_x,  _top, _cnt_w,  _h, c_zebra());

            _p.cell_text(_g.x[0], _top, _chip_w, _h, _pass ? "PASS" : "FAIL",
                         f_sans_bold(7.5), _ink, align_center);

            // "<lead>  name" as two runs on one baseline
            const char* _lead = _test_level ? "SECTION" : "TEST";
            const pdf_unit _base = _top - (_h / 2.0) - (10.0 * 0.30);
            _p.glyphs(_name_x + 4.0, _base, _lead, f_sans_bold(7.0), c_muted());
            _p.glyphs(_name_x + 4.0 + text_width(f_sans_bold(7.0), _lead) + 6.0,
                      _base, _u.name, f_sans_bold(10.0), c_ink());

            const std::string _cnt = _test_level
                ? (ratio(_u.passed_checks(), _u.total_checks()) + " tests")
                : (ratio(_u.passed_checks(), _u.total_checks()) + " assertions");
            _p.cell_text(_cnt_x, _top, _cnt_w, _h, _cnt,
                         f_sans(8.0), _pass ? c_muted() : c_fail_ink(), align_right);

            _p.y = _top - _h;
        }

        // -- optional description line (the unit's descriptor) --
        if (!_u.description.empty())
        {
            const pdf_unit _cw = _p.content_w();
            const pdf_font _df = f_sans(8.5);
            const pdf_unit _h  = _p.measure_h(_cw, _u.description, _df);

            if (!_p.room(_h))
            {
                _p.new_page();
            }

            const pdf_unit _top = _p.y;

            _p.fill(_p.m_left, _top, _cw, _h, c_white());
            _p.cell_text(_p.m_left, _top, _cw, _h, _u.description, _df, c_muted(), align_left, 4.0);

            _p.y = _top - _h;
        }

        // -- column header --
        header_row(_p, _g, _test_level);

        // -- rows --
        {
            const pdf_font _mono  = f_mono(8.0);
            const pdf_font _num   = f_sans(8.0);
            const pdf_font _prose = f_sans(8.0);

            pdf_unit _tw = 0.0, _dx = 0.0, _dw = 0.0;
            test_level_cols(_g, _tw, _dx, _dw);

            std::size_t _i = 0;

            for (_i = 0; _i < _u.checks.size(); ++_i)
            {
                const report_check& _c = _u.checks[_i];

                const char* _word = "";
                pdf_color   _cbg   = c_pass_bg();
                pdf_color   _cink  = c_pass_ink();
                check_face(_c.status, _word, _cbg, _cink);

                const bool _cpass = (_c.status == test_status::passed);
                const pdf_color _zeb =
                    _cpass ? ((_i % 2) ? c_zebra() : c_white()) : c_fail_zebra();

                // ---- test-level row: RESULT / # / TEST / DESCRIPTION ----
                if (_test_level)
                {
                    const std::string& _name = _c.expression;    // identifier
                    const std::string& _desc = _c.description;   // prose

                    pdf_unit _h = _p.measure_h(_tw, _name, _mono);
                    const pdf_unit _hd = _p.measure_h(_dw, _desc, _prose);
                    if (_hd > _h) { _h = _hd; }

                    if (!_p.room(_h))
                    {
                        _p.new_page();
                        header_row(_p, _g, true);
                    }

                    const pdf_unit _top = _p.y;

                    _p.fill(_g.x[0], _top, _g.w[0], _h, _cbg);
                    _p.fill(_g.x[1], _top, _g.w[1], _h, _zeb);
                    _p.fill(_g.x[2], _top, _tw,     _h, _zeb);
                    _p.fill(_dx,     _top, _dw,     _h, _zeb);

                    _p.stroke(_g.x[0], _top, _g.w[0], _h, c_line());
                    _p.stroke(_g.x[1], _top, _g.w[1], _h, c_line());
                    _p.stroke(_g.x[2], _top, _tw,     _h, c_line());
                    _p.stroke(_dx,     _top, _dw,     _h, c_line());

                    _p.cell_text(_g.x[0], _top, _g.w[0], _h, _word, f_sans_bold(7.5), _cink, align_center);
                    _p.cell_text(_g.x[1], _top, _g.w[1], _h, std::to_string(_i + 1), _num, c_muted(), align_right);
                    _p.cell_text(_g.x[2], _top, _tw,     _h, _name, _mono, c_ink(), align_left);
                    _p.cell_text(_dx,     _top, _dw,     _h, _desc, _prose, c_ink(), align_left);

                    _p.y = _top - _h;
                    continue;
                }

                // ---- assertion-level row: RESULT / # / ASSERTION / EXPECTED / ACTUAL ----
                const std::string _assertion =
                    _c.expression.empty() ? _c.description : _c.expression;

                pdf_unit _h = _p.measure_h(_g.w[2], _assertion, _mono);
                {
                    const pdf_unit _he = _p.measure_h(_g.w[3], _c.expected, _mono);
                    const pdf_unit _ha = _p.measure_h(_g.w[4], _c.actual,   _mono);
                    if (_he > _h) { _h = _he; }
                    if (_ha > _h) { _h = _ha; }
                }

                if (!_p.room(_h))
                {
                    _p.new_page();
                    header_row(_p, _g, false);
                }

                const pdf_unit _top = _p.y;

                _p.fill(_g.x[0], _top, _g.w[0], _h, _cbg);
                _p.fill(_g.x[1], _top, _g.w[1], _h, _zeb);
                _p.fill(_g.x[2], _top, _g.w[2], _h, _zeb);
                _p.fill(_g.x[3], _top, _g.w[3], _h, _zeb);
                _p.fill(_g.x[4], _top, _g.w[4], _h, _zeb);

                std::size_t _k = 0;
                for (_k = 0; _k < 5; ++_k)
                {
                    _p.stroke(_g.x[_k], _top, _g.w[_k], _h, c_line());
                }

                _p.cell_text(_g.x[0], _top, _g.w[0], _h, _word, f_sans_bold(7.5), _cink, align_center);
                _p.cell_text(_g.x[1], _top, _g.w[1], _h, std::to_string(_i + 1), _num, c_muted(), align_right);
                _p.cell_text(_g.x[2], _top, _g.w[2], _h, _assertion, _mono, c_ink(), align_left);
                _p.cell_text(_g.x[3], _top, _g.w[3], _h, _c.expected, _mono, c_muted(), align_left);
                _p.cell_text(_g.x[4], _top, _g.w[4], _h, _c.actual, _mono,
                             _cpass ? c_muted() : c_fail_ink(), align_left);

                _p.y = _top - _h;
            }
        }

        // gap after the card
        _p.y -= 10.0;

        return;
    }

    // title_block
    //   the document title + subtitle.
    inline void
    title_block(
        painter&           _p,
        const test_report& _r
    )
    {
        const std::string _title =
            _r.title.empty() ? std::string("C++ Unit Test Report") : _r.title;

        _p.glyphs(_p.m_left, _p.y - 18.0, _title, f_sans_bold(20.0), c_ink());
        _p.y -= 26.0;

        if (!_r.subtitle.empty())
        {
            _p.glyphs(_p.m_left, _p.y - 10.0, _r.subtitle, f_sans(10.0), c_muted());
            _p.y -= 22.0;
        }
        else
        {
            _p.y -= 6.0;
        }

        return;
    }

}   // namespace pdf_table_internal


// render_report_pdf_bytes_table
//   render _report to a complete PDF byte stream whose per-test tables are real
// ruled/shaded assertion grids.  This is the DTest PDF default.
D_NODISCARD inline std::string
render_report_pdf_bytes_table(
    const test_report& _report
)
{
    using namespace pdf_table_internal;

    pdf_document _doc;
    painter      _p(_doc);

    _p.begin();
    title_block(_p, _report);

    std::size_t _mi = 0;

    for (_mi = 0; _mi < _report.modules.size(); ++_mi)
    {
        const report_module& _m = _report.modules[_mi];

        const bool _tl = module_is_test_level(_m);

        band(_p, _m, _tl);
        meta_block(_p, _m, _tl);
        section_label(_p, "TEST DETAIL");

        std::size_t _ui = 0;

        for (_ui = 0; _ui < _m.units.size(); ++_ui)
        {
            test_table(_p, _m.units[_ui], _tl);
        }
    }

    return _doc.to_bytes();
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_RENDER_PDF_TABLE_
