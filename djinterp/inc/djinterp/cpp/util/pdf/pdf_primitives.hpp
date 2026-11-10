/*******************************************************************************
* djinterp [cpp]                                              pdf_primitives.hpp
*
*
* path:      /inc/djinterp/cpp/util/pdf/pdf_primitives.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.02
*******************************************************************************/
#ifndef DJINTERP_UTIL_PDF_PDF_PRIMITIVES_HPP
#define DJINTERP_UTIL_PDF_PDF_PRIMITIVES_HPP

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

///////////////////////////////////////////////////////////////////////////////
// pdf_primitives.hpp
//
// The PDF drawing vocabulary: units & geometry, page sizes, color,
// fonts, text & paint options, and raster images.  Color accepts any
// native color model (default rgb) and fonts accept any native
// djinterp::font (resolved onto the base-14 faces).  This is the base
// layer of the split pdf module; everything else builds on it.
///////////////////////////////////////////////////////////////////////////////


// std
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../color/color.hpp"  // native color models + color_cast
#include "../../text/font.hpp"                 // native font (resolved onto base-14)
#include "../../text/text_align.hpp"           // native text_alignment (alias target)
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t


// D_KEYWORD_PDF
//   constant: keyword used to specify the PDF subsystem (document
// generation, layout, and output).  Defined locally because the
// core keyword set does not yet carry a PDF entry; the #ifndef
// guard lets a project-wide definition take precedence if one is
// later added to djinterp.h.
#ifndef D_KEYWORD_PDF
    #define D_KEYWORD_PDF   pdf
#endif  // D_KEYWORD_PDF


// D_INTERNAL_PDF_OS_WINDOWS
//   macro: 1 when targeting Windows, 0 otherwise.  Prefers env.h OS
// detection and falls back to raw compiler predefines.
#if ( defined(D_ENV_IS_OS_WINDOWS) &&  \
      defined(D_ENV_OS_ID) )
    #define D_INTERNAL_PDF_OS_WINDOWS  D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
#elif ( defined(_WIN32) ||  \
        defined(_MSC_VER) )
    #define D_INTERNAL_PDF_OS_WINDOWS  1
#else
    #define D_INTERNAL_PDF_OS_WINDOWS  0
#endif


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   UNITS & GEOMETRY                                     ///
///////////////////////////////////////////////////////////////////////////////

// DPdfUnit
//   type: a linear measure in PostScript points (1/72 inch).  All
// geometry in the PDF subsystem is expressed in this unit.
typedef double DPdfUnit;

// D_PDF_POINTS_PER_INCH
//   constant: points in one inch.
#define D_PDF_POINTS_PER_INCH 72.0

// D_PDF_POINTS_PER_MM
//   constant: points in one millimetre (72 / 25.4).
#define D_PDF_POINTS_PER_MM   2.834645669291339

// inches
//   function: converts a measure in inches to points.
D_CONSTEXPR
DPdfUnit inches(
    DPdfUnit _in
) D_NOEXCEPT
{
    return (_in * D_PDF_POINTS_PER_INCH);
}

// millimetres
//   function: converts a measure in millimetres to points.
D_CONSTEXPR
DPdfUnit millimetres(
    DPdfUnit _mm
) D_NOEXCEPT
{
    return (_mm * D_PDF_POINTS_PER_MM);
}


// DPdfPoint
//   struct: a coordinate in PDF user space (origin bottom-left,
// y increasing upward).
struct DPdfPoint
{
    DPdfUnit x;
    DPdfUnit y;

    D_CONSTEXPR DPdfPoint() D_NOEXCEPT
        : x(0.0), y(0.0)
    {}

    D_CONSTEXPR DPdfPoint(
        DPdfUnit _x,
        DPdfUnit _y
    ) D_NOEXCEPT
        : x(_x), y(_y)
    {}
};


// DPdfSize
//   struct: a width / height extent in points.
struct DPdfSize
{
    DPdfUnit width;
    DPdfUnit height;

    D_CONSTEXPR DPdfSize() D_NOEXCEPT
        : width(0.0), height(0.0)
    {}

    D_CONSTEXPR DPdfSize(
        DPdfUnit _w,
        DPdfUnit _h
    ) D_NOEXCEPT
        : width(_w), height(_h)
    {}
};


// DPdfRect
//   struct: an axis-aligned rectangle anchored at its lower-left
// corner with a width and height in points.
struct DPdfRect
{
    DPdfUnit x;
    DPdfUnit y;
    DPdfUnit width;
    DPdfUnit height;

    D_CONSTEXPR DPdfRect() D_NOEXCEPT
        : x(0.0), y(0.0), width(0.0), height(0.0)
    {}

    D_CONSTEXPR DPdfRect(
        DPdfUnit _x,
        DPdfUnit _y,
        DPdfUnit _w,
        DPdfUnit _h
    ) D_NOEXCEPT
        : x(_x), y(_y), width(_w), height(_h)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                II.  PAGE SIZES                                           ///
///////////////////////////////////////////////////////////////////////////////

// DPdfPageSize
//   struct: a named page extent.  Thin wrapper over DPdfSize so
// callers can pass either a preset (DPdfPageSize::a4()) or an
// arbitrary custom size.
struct DPdfPageSize
{
    DPdfSize size;

    D_CONSTEXPR DPdfPageSize() D_NOEXCEPT
        : size(612.0, 792.0)
    {}

    D_CONSTEXPR DPdfPageSize(
        DPdfUnit _w,
        DPdfUnit _h
    ) D_NOEXCEPT
        : size(_w, _h)
    {}

    // presets (portrait orientation, points)
    static D_CONSTEXPR DPdfPageSize letter()  D_NOEXCEPT { return DPdfPageSize(612.0,  792.0);  }
    static D_CONSTEXPR DPdfPageSize legal()   D_NOEXCEPT { return DPdfPageSize(612.0,  1008.0); }
    static D_CONSTEXPR DPdfPageSize tabloid() D_NOEXCEPT { return DPdfPageSize(792.0,  1224.0); }
    static D_CONSTEXPR DPdfPageSize a3()      D_NOEXCEPT { return DPdfPageSize(841.89, 1190.55);}
    static D_CONSTEXPR DPdfPageSize a4()      D_NOEXCEPT { return DPdfPageSize(595.28, 841.89); }
    static D_CONSTEXPR DPdfPageSize a5()      D_NOEXCEPT { return DPdfPageSize(419.53, 595.28); }

    // landscape
    //   returns this size with width and height swapped.
    D_CONSTEXPR
    DPdfPageSize landscape() const D_NOEXCEPT
    {
        return DPdfPageSize(size.height, size.width);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. COLOR                                                ///
///////////////////////////////////////////////////////////////////////////////

// DPdfColorSpace
//   enum: the PDF device color spaces a pdf_color may occupy.  The
// scoped enumerators do NOT collide with the rgb / cmyk color *types*.
enum class DPdfColorSpace : re_std::uint8_t
{
    gray = 0,   // DeviceGray  (1 channel,  "g" / "G")
    rgb  = 1,   // DeviceRGB   (3 channels, "rg" / "RG")
    cmyk = 2    // DeviceCMYK  (4 channels, "k" / "K")
};

// pdf_color
//   struct: a device color for content-stream emission.  It accepts
// ANY native color model (the default is djinterp::rgb): rgb and cmyk
// keep their own device space, while every other model folds to rgb
// through the conversion hub.  The r/g/b view is ALWAYS populated, so
// rgb-only backends and existing call sites keep working unchanged.
struct pdf_color
{
    DPdfColorSpace space = DPdfColorSpace::rgb;
    double          r = 0.0, g = 0.0, b = 0.0;          // rgb view (always valid)
    double          c = 0.0, m = 0.0, y = 0.0, k = 0.0; // cmyk channels (space == cmyk)

    D_CONSTEXPR
    pdf_color() = default;                  // rgb black

    // legacy rgb-channel constructor (kept for existing call sites).
    D_CONSTEXPR pdf_color(
        double _r,
        double _g,
        double _b
    ) D_NOEXCEPT
        : space(DPdfColorSpace::rgb), r(_r), g(_g), b(_b)
    {}

    // from a native rgb.
    pdf_color(
        const rgb& _c
    )
        : space(DPdfColorSpace::rgb), r(_c.r), g(_c.g), b(_c.b)
    {}

    // from a native cmyk: kept as DeviceCMYK, with the rgb view filled
    // in via the conversion hub as a fallback for rgb-only consumers.
    pdf_color(
        const cmyk& _c
    )
        : space(DPdfColorSpace::cmyk),
          c(_c.c), m(_c.m), y(_c.y), k(_c.k)
    {
        const rgb _fallback = color_cast<rgb>(_c);

        r = _fallback.r;
        g = _fallback.g;
        b = _fallback.b;
    }

    // from any other native color model: routed to rgb via the hub.
    template <typename Model,
              typename = typename std::enable_if<
                  is_color_model<Model>::value
                  && !std::is_same<clean_t<Model>, rgb>::value
                  && !std::is_same<clean_t<Model>, cmyk>::value>::type>
    pdf_color(
        const Model& _c
    )
    {
        const rgb _converted = color_cast<rgb>(_c);

        space = DPdfColorSpace::rgb;
        r     = _converted.r;
        g     = _converted.g;
        b     = _converted.b;
    }

    // device_gray
    //   factory: a single-channel DeviceGray color.
    static
    pdf_color device_gray(
        double _v
    )
    {
        pdf_color out;

        out.space = DPdfColorSpace::gray;
        out.r     = out.g = out.b = _v;

        return out;
    }

    // from_rgb255
    //   factory: builds an rgb-space color from 8-bit channel values.
    static D_CONSTEXPR
    pdf_color from_rgb255(
        int _r,
        int _g,
        int _b
    ) D_NOEXCEPT
    {
        return pdf_color(static_cast<double>(_r) / 255.0,
                         static_cast<double>(_g) / 255.0,
                         static_cast<double>(_b) / 255.0);
    }

    // as_rgb
    //   accessor: the rgb view of this color (always valid).
    rgb as_rgb() const
    {
        return rgb(static_cast<channel_t>(r),
                   static_cast<channel_t>(g),
                   static_cast<channel_t>(b));
    }

    // named constants (rgb device space)
    static D_CONSTEXPR pdf_color black() D_NOEXCEPT { return pdf_color(0.0, 0.0, 0.0); }
    static D_CONSTEXPR pdf_color white() D_NOEXCEPT { return pdf_color(1.0, 1.0, 1.0); }
    static D_CONSTEXPR pdf_color red()   D_NOEXCEPT { return pdf_color(0.8, 0.0, 0.0); }
    static D_CONSTEXPR pdf_color green() D_NOEXCEPT { return pdf_color(0.0, 0.6, 0.0); }
    static D_CONSTEXPR pdf_color blue()  D_NOEXCEPT { return pdf_color(0.0, 0.0, 0.8); }
    static D_CONSTEXPR pdf_color gray()  D_NOEXCEPT { return pdf_color(0.5, 0.5, 0.5); }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  FONTS                                                ///
///////////////////////////////////////////////////////////////////////////////

// DPdfBaseFont
//   enum: the standard-14 PDF fonts.  Every conforming reader
// provides these without embedding, so they are the portable
// common-subset font set.
enum class DPdfBaseFont
{
    courier                  = 0,
    courier_bold             = 1,
    courier_oblique          = 2,
    courier_bold_oblique     = 3,
    helvetica                = 4,
    helvetica_bold           = 5,
    helvetica_oblique        = 6,
    helvetica_bold_oblique   = 7,
    times_roman              = 8,
    times_bold               = 9,
    times_italic             = 10,
    times_bold_italic        = 11,
    symbol                   = 12,
    zapf_dingbats            = 13
};

// base_font_name
//   function: maps a DPdfBaseFont to its canonical PostScript
// BaseFont name.
inline
const char* base_font_name(
    DPdfBaseFont _font
) D_NOEXCEPT
{
    switch (_font)
    {
        case DPdfBaseFont::courier:                { return "Courier"; }
        case DPdfBaseFont::courier_bold:           { return "Courier-Bold"; }
        case DPdfBaseFont::courier_oblique:        { return "Courier-Oblique"; }
        case DPdfBaseFont::courier_bold_oblique:   { return "Courier-BoldOblique"; }
        case DPdfBaseFont::helvetica:              { return "Helvetica"; }
        case DPdfBaseFont::helvetica_bold:         { return "Helvetica-Bold"; }
        case DPdfBaseFont::helvetica_oblique:      { return "Helvetica-Oblique"; }
        case DPdfBaseFont::helvetica_bold_oblique: { return "Helvetica-BoldOblique"; }
        case DPdfBaseFont::times_roman:            { return "Times-Roman"; }
        case DPdfBaseFont::times_bold:             { return "Times-Bold"; }
        case DPdfBaseFont::times_italic:           { return "Times-Italic"; }
        case DPdfBaseFont::times_bold_italic:      { return "Times-BoldItalic"; }
        case DPdfBaseFont::symbol:                 { return "Symbol"; }
        case DPdfBaseFont::zapf_dingbats:          { return "ZapfDingbats"; }
        default:                                    { return "Courier"; }
    }
}

// is_monospace_font
//   function: true if the base font is fixed-pitch (the Courier
// family).  Fixed-pitch fonts permit exact column layout.
inline
bool is_monospace_font(
    DPdfBaseFont _font
) D_NOEXCEPT
{
    return ( (_font == DPdfBaseFont::courier)              ||
             (_font == DPdfBaseFont::courier_bold)         ||
             (_font == DPdfBaseFont::courier_oblique)      ||
             (_font == DPdfBaseFont::courier_bold_oblique) );
}

// average_advance_factor
//   function: a representative glyph advance as a fraction of the
// font size.  Exact (0.6) for the fixed-pitch Courier family;
// an average approximation for the proportional families, used
// only for layout estimation when exact AFM metrics are absent.
inline
double average_advance_factor(
    DPdfBaseFont _font
) D_NOEXCEPT
{
    if (is_monospace_font(_font))
    {
        return 0.6;
    }

    switch (_font)
    {
        case DPdfBaseFont::times_roman:
        case DPdfBaseFont::times_bold:
        case DPdfBaseFont::times_italic:
        case DPdfBaseFont::times_bold_italic:
        {
            return 0.5;
        }

        case DPdfBaseFont::symbol:
        case DPdfBaseFont::zapf_dingbats:
        {
            return 0.6;
        }

        default:
        {
            // Helvetica family and fallback
            return 0.52;
        }
    }
}


// pdf_base_font_from
//   function: resolves an arbitrary face - given as a family name
// plus bold / italic flags - to the nearest standard-14 base font.
// The family name is matched case-insensitively: "courier" or "mono"
// selects the fixed-pitch Courier family, "sans" selects Helvetica,
// "times" or "serif" selects Times, and anything else (including the
// empty name) falls back to Helvetica.  This is how the framework's
// native fonts are abstracted onto the base-14 faces the built-in
// backend can emit without font embedding.
inline
DPdfBaseFont pdf_base_font_from(
    const std::string& _family,
    bool               _bold,
    bool               _italic
)
{
    std::string lower;

    lower.reserve(_family.size());

    for (std::size_t i = 0; i < _family.size(); ++i)
    {
        const char ch = _family[i];

        lower.push_back(static_cast<char>(
            (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch));
    }

    enum family_group { group_helvetica, group_times, group_courier };

    family_group group = group_helvetica;

    if (lower.find("courier") != std::string::npos ||
        lower.find("mono")    != std::string::npos)
    {
        group = group_courier;
    }
    else if (lower.find("sans") != std::string::npos)
    {
        group = group_helvetica;        // "sans-serif" must beat "serif"
    }
    else if (lower.find("times") != std::string::npos ||
             lower.find("serif") != std::string::npos)
    {
        group = group_times;
    }

    switch (group)
    {
        case group_courier:
            if (_bold && _italic) { return DPdfBaseFont::courier_bold_oblique; }
            if (_bold)            { return DPdfBaseFont::courier_bold; }
            if (_italic)          { return DPdfBaseFont::courier_oblique; }
            return DPdfBaseFont::courier;

        case group_times:
            if (_bold && _italic) { return DPdfBaseFont::times_bold_italic; }
            if (_bold)            { return DPdfBaseFont::times_bold; }
            if (_italic)          { return DPdfBaseFont::times_italic; }
            return DPdfBaseFont::times_roman;

        case group_helvetica:
        default:
            if (_bold && _italic) { return DPdfBaseFont::helvetica_bold_oblique; }
            if (_bold)            { return DPdfBaseFont::helvetica_bold; }
            if (_italic)          { return DPdfBaseFont::helvetica_oblique; }
            return DPdfBaseFont::helvetica;
    }
}


// pdf_font
//   struct: a resolved font - a standard-14 base face paired with
// a point size.
struct pdf_font
{
    DPdfBaseFont family;
    DPdfUnit      size;

    pdf_font()
        : family(DPdfBaseFont::courier),
          size(10.0)
    {}

    pdf_font(
        DPdfBaseFont _family,
        DPdfUnit      _size
    ) D_NOEXCEPT
        : family(_family),
          size(_size)
    {}

    // from a native djinterp::font: the face is resolved onto the
    // nearest base-14 font and the point size is carried across, so any
    // framework font can drive the built-in backend.
    template <unsigned Feat, typename Color>
    pdf_font(
        const djinterp::font<Feat, Color>& _f
    )
        : family(pdf_base_font_from(_f.family,
                                    djinterp::fn_is_bold(_f),
                                    djinterp::fn_is_italic(_f))),
          size(static_cast<DPdfUnit>(_f.size))
    {}

    // convenience factories
    static pdf_font courier(DPdfUnit _sz)   D_NOEXCEPT { return pdf_font(DPdfBaseFont::courier,   _sz); }
    static pdf_font helvetica(DPdfUnit _sz) D_NOEXCEPT { return pdf_font(DPdfBaseFont::helvetica, _sz); }
    static pdf_font times(DPdfUnit _sz)     D_NOEXCEPT { return pdf_font(DPdfBaseFont::times_roman,_sz); }

    // estimated_width
    //   returns an estimated rendered width for a character count
    // at this font's size.  Exact for Courier; approximate for
    // proportional faces.
    DPdfUnit estimated_width(
        std::size_t _char_count
    ) const D_NOEXCEPT
    {
        return ( static_cast<DPdfUnit>(_char_count) *
                 size *
                 average_advance_factor(family) );
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   TEXT & PAINT OPTIONS                                 ///
///////////////////////////////////////////////////////////////////////////////

// DPdfPaint
//   struct: stroke / fill parameters for vector graphics.
struct DPdfPaint
{
    pdf_color stroke;
    pdf_color fill;
    DPdfUnit  line_width;
    bool      do_stroke;
    bool      do_fill;

    DPdfPaint()
        : stroke(pdf_color::black()),
          fill(pdf_color::black()),
          line_width(1.0),
          do_stroke(true),
          do_fill(false)
    {}

    // stroked
    //   factory: a stroke-only paint with the given color and width.
    static
    DPdfPaint stroked(
        const pdf_color& _color,
        DPdfUnit         _width = 1.0
    )
    {
        DPdfPaint p;

        p.stroke     = _color;
        p.line_width = _width;
        p.do_stroke  = true;
        p.do_fill    = false;

        return p;
    }

    // filled
    //   factory: a fill-only paint with the given color.
    static
    DPdfPaint filled(
        const pdf_color& _color
    )
    {
        DPdfPaint p;

        p.fill      = _color;
        p.do_stroke = false;
        p.do_fill   = true;

        return p;
    }
};


// DPdfPathVerb
//   enum: the operation a path segment performs.  A path is a
// sequence of (verb, points) records replayed by the backend.
enum class DPdfPathVerb
{
    move_to  = 0,  // start a new subpath at p0
    line_to  = 1,  // straight segment to p0
    curve_to = 2,  // cubic Bezier with controls p0, p1 to endpoint p2
    close    = 3   // close the current subpath
};


// DPdfPathSegment
//   struct: one path operation and its (up to three) control points.
// Unused points are left default-constructed.
struct DPdfPathSegment
{
    DPdfPathVerb verb;
    DPdfPoint     p0;
    DPdfPoint     p1;
    DPdfPoint     p2;

    DPdfPathSegment()
        : verb(DPdfPathVerb::move_to),
          p0(),
          p1(),
          p2()
    {}
};


// DPdfPath
//   class: a library-agnostic vector path.  Built from move / line /
// curve / close operations and replayed by a backend's draw_path in
// a single call, so any engine (built-in, libHaru, ...) can render
// it natively.  Factory helpers construct common shapes from cubic
// Bezier approximations.
class DPdfPath
{
public:
    using size_type = std::size_t;

    DPdfPath()
        : m_segments()
    {}

    // move_to
    //   begins a new subpath at _p.
    DPdfPath& move_to(
        const DPdfPoint& _p
    )
    {
        DPdfPathSegment s;
        s.verb = DPdfPathVerb::move_to;
        s.p0   = _p;

        m_segments.push_back(static_cast<DPdfPathSegment&&>(s));

        return *this;
    }

    // line_to
    //   adds a straight segment to _p.
    DPdfPath& line_to(
        const DPdfPoint& _p
    )
    {
        DPdfPathSegment s;
        s.verb = DPdfPathVerb::line_to;
        s.p0   = _p;

        m_segments.push_back(static_cast<DPdfPathSegment&&>(s));

        return *this;
    }

    // curve_to
    //   adds a cubic Bezier with control points _c1, _c2 ending at
    // _end.
    DPdfPath& curve_to(
        const DPdfPoint& _c1,
        const DPdfPoint& _c2,
        const DPdfPoint& _end
    )
    {
        DPdfPathSegment s;
        s.verb = DPdfPathVerb::curve_to;
        s.p0   = _c1;
        s.p1   = _c2;
        s.p2   = _end;

        m_segments.push_back(static_cast<DPdfPathSegment&&>(s));

        return *this;
    }

    // close
    //   closes the current subpath back to its start.
    DPdfPath& close()
    {
        DPdfPathSegment s;
        s.verb = DPdfPathVerb::close;

        m_segments.push_back(static_cast<DPdfPathSegment&&>(s));

        return *this;
    }

    const std::vector<DPdfPathSegment>& segments() const D_NOEXCEPT
    {
        return m_segments;
    }

    bool  empty() const D_NOEXCEPT { return m_segments.empty(); }
    void  clear()       { m_segments.clear(); return; }

    // -----------------------------------------------------------------
    //  shape factories
    // -----------------------------------------------------------------

    // rectangle
    //   factory: a closed rectangular path.
    static
    DPdfPath rectangle(
        const DPdfRect& _r
    )
    {
        DPdfPath p;

        p.move_to(DPdfPoint(_r.x, _r.y));
        p.line_to(DPdfPoint(_r.x + _r.width, _r.y));
        p.line_to(DPdfPoint(_r.x + _r.width, _r.y + _r.height));
        p.line_to(DPdfPoint(_r.x, _r.y + _r.height));
        p.close();

        return p;
    }

    // polyline
    //   factory: an open path through the given points.
    static
    DPdfPath polyline(
        const std::vector<DPdfPoint>& _pts
    )
    {
        DPdfPath p;

        for (size_type i = 0; i < _pts.size(); ++i)
        {
            if (i == 0)
            {
                p.move_to(_pts[i]);
            }
            else
            {
                p.line_to(_pts[i]);
            }
        }

        return p;
    }

    // polygon
    //   factory: a closed path through the given points.
    static
    DPdfPath polygon(
        const std::vector<DPdfPoint>& _pts
    )
    {
        DPdfPath p = polyline(_pts);

        if (!p.empty())
        {
            p.close();
        }

        return p;
    }

    // ellipse
    //   factory: a closed ellipse centered at _center with the given
    // radii, approximated by four cubic Bezier arcs.  The control
    // offset 0.5522847498 (4/3 * (sqrt(2) - 1)) is the standard
    // circle-to-Bezier constant.
    static
    DPdfPath ellipse(
        const DPdfPoint& _center,
        DPdfUnit         _rx,
        DPdfUnit         _ry
    )
    {
        const DPdfUnit k = 0.5522847498307936;
        DPdfUnit       cx = _center.x;
        DPdfUnit       cy = _center.y;
        DPdfUnit       ox = _rx * k;
        DPdfUnit       oy = _ry * k;

        DPdfPath p;

        p.move_to(DPdfPoint(cx + _rx, cy));
        p.curve_to(DPdfPoint(cx + _rx, cy + oy),
                   DPdfPoint(cx + ox, cy + _ry),
                   DPdfPoint(cx,      cy + _ry));
        p.curve_to(DPdfPoint(cx - ox, cy + _ry),
                   DPdfPoint(cx - _rx, cy + oy),
                   DPdfPoint(cx - _rx, cy));
        p.curve_to(DPdfPoint(cx - _rx, cy - oy),
                   DPdfPoint(cx - ox, cy - _ry),
                   DPdfPoint(cx,      cy - _ry));
        p.curve_to(DPdfPoint(cx + ox, cy - _ry),
                   DPdfPoint(cx + _rx, cy - oy),
                   DPdfPoint(cx + _rx, cy));
        p.close();

        return p;
    }

    // circle
    //   factory: a closed circle (an ellipse with equal radii).
    static
    DPdfPath circle(
        const DPdfPoint& _center,
        DPdfUnit         _radius
    )
    {
        return ellipse(_center, _radius, _radius);
    }

    // rounded_rect
    //   factory: a rectangle with quarter-Bezier rounded corners of
    // radius _radius (clamped to half the smaller side).
    static
    DPdfPath rounded_rect(
        const DPdfRect& _r,
        DPdfUnit        _radius
    )
    {
        DPdfUnit rr = _radius;

        // clamp the corner radius to fit the rectangle
        if (rr > (_r.width  * 0.5)) { rr = _r.width  * 0.5; }
        if (rr > (_r.height * 0.5)) { rr = _r.height * 0.5; }

        const DPdfUnit k  = 0.5522847498307936;
        DPdfUnit       o  = rr * k;
        DPdfUnit       x0 = _r.x;
        DPdfUnit       y0 = _r.y;
        DPdfUnit       x1 = _r.x + _r.width;
        DPdfUnit       y1 = _r.y + _r.height;

        DPdfPath p;

        p.move_to(DPdfPoint(x0 + rr, y0));
        p.line_to(DPdfPoint(x1 - rr, y0));
        p.curve_to(DPdfPoint(x1 - rr + o, y0),
                   DPdfPoint(x1, y0 + rr - o),
                   DPdfPoint(x1, y0 + rr));
        p.line_to(DPdfPoint(x1, y1 - rr));
        p.curve_to(DPdfPoint(x1, y1 - rr + o),
                   DPdfPoint(x1 - rr + o, y1),
                   DPdfPoint(x1 - rr, y1));
        p.line_to(DPdfPoint(x0 + rr, y1));
        p.curve_to(DPdfPoint(x0 + rr - o, y1),
                   DPdfPoint(x0, y1 - rr + o),
                   DPdfPoint(x0, y1 - rr));
        p.line_to(DPdfPoint(x0, y0 + rr));
        p.curve_to(DPdfPoint(x0, y0 + rr - o),
                   DPdfPoint(x0 + rr - o, y0),
                   DPdfPoint(x0 + rr, y0));
        p.close();

        return p;
    }

private:
    std::vector<DPdfPathSegment> m_segments;
};


// DPdfTextAlign
//   alias: horizontal alignment for higher layers (the foundation
// places text at an explicit point; alignment is resolved by
// pdf_template).  This is the framework's native
// djinterp::text_alignment - the same {left, center, right,
// justify} enumerators with identical values - so the PDF layer
// shares one alignment vocabulary with the rest of the framework
// and the two names are fully interchangeable.
using DPdfTextAlign = djinterp::text_alignment;


// DPdfTextOptions
//   struct: the full set of text-rendering parameters - font,
// color, alignment, and line leading.
struct DPdfTextOptions
{
    pdf_font       font;
    pdf_color      color;
    DPdfTextAlign align;
    DPdfUnit       leading;

    DPdfTextOptions()
        : font(),
          color(pdf_color::black()),
          align(DPdfTextAlign::left),
          leading(12.0)
    {}

    explicit DPdfTextOptions(
        const pdf_font& _font
    )
        : font(_font),
          color(pdf_color::black()),
          align(DPdfTextAlign::left),
          leading(_font.size * 1.2)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  RASTER IMAGES                                        ///
///////////////////////////////////////////////////////////////////////////////

// DPdfImageFormat
//   enum: the pixel layout of a raw image buffer.  Encoded formats
// (PNG / JPEG) are intentionally excluded from the common subset:
// the built-in backend embeds only raw samples, while a richer
// backend may accept encoded data through a backend-specific path.
enum class DPdfImageFormat
{
    gray = 0,  // 1 component  per pixel (8-bit)
    rgb  = 1,  // 3 components per pixel (8-bit, R G B)
    rgba = 2   // 4 components per pixel (8-bit, R G B A)
};

// image_component_count
//   function: the number of 8-bit samples per pixel for a format.
inline
int image_component_count(
    DPdfImageFormat _fmt
) D_NOEXCEPT
{
    switch (_fmt)
    {
        case DPdfImageFormat::gray: { return 1; }
        case DPdfImageFormat::rgb:  { return 3; }
        case DPdfImageFormat::rgba: { return 4; }
        default:                     { return 3; }
    }
}


// DPdfImage
//   struct: a raw raster image - pixel dimensions, format, and an
// owned buffer of 8-bit samples in row-major top-to-bottom order
// (the natural order for most decoders; the backend flips to PDF's
// bottom-up image space).  The buffer length must be at least
// width * height * component_count(format).
struct DPdfImage
{
    std::size_t                width;
    std::size_t                height;
    DPdfImageFormat           format;
    std::vector<unsigned char> samples;

    DPdfImage()
        : width(0),
          height(0),
          format(DPdfImageFormat::rgb),
          samples()
    {}

    DPdfImage(
        std::size_t      _w,
        std::size_t      _h,
        DPdfImageFormat _fmt
    )
        : width(_w),
          height(_h),
          format(_fmt),
          samples()
    {
        samples.resize(expected_size());
    }

    // expected_size
    //   the required sample-buffer length for this image.
    std::size_t expected_size() const D_NOEXCEPT
    {
        return ( width * height *
                 static_cast<std::size_t>(
                     image_component_count(format)) );
    }

    // valid
    //   true if the buffer holds at least the expected sample count
    // and the dimensions are non-zero.
    bool valid() const D_NOEXCEPT
    {
        return ( (width > 0)  &&
                 (height > 0) &&
                 (samples.size() >= expected_size()) );
    }
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_PDF_PDF_PRIMITIVES_HPP
