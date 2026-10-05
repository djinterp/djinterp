/*******************************************************************************
* djinterp [c]                                                  pdf_primitives.c
*
* Units, geometry, page sizes, device colour and the standard-14 faces.
*   Defines what pdf_primitives.h declares, value for value with the C++
* tier's pdf_primitives.hpp: the page presets, face names, advance factors
* and family matching are transcribed from it. Colour channels are stored
* verbatim, never clamped, as the header requires.
*
*
* path:      /src/djinterp/c/util/pdf/pdf_primitives.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/pdf/pdf_primitives.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strstr
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int32_t


double
d_pdf_inches(
    double _inches
)
{
    return _inches * D_PDF_POINTS_PER_INCH;
}

double
d_pdf_millimetres(
    double _mm
)
{
    return _mm * D_PDF_POINTS_PER_MM;
}

struct d_pdf_point
d_pdf_make_point(
    double _x,
    double _y
)
{
    struct d_pdf_point point;

    point.x = _x;
    point.y = _y;

    return point;
}

struct d_pdf_size
d_pdf_make_size(
    double _w,
    double _h
)
{
    struct d_pdf_size size;

    size.width  = _w;
    size.height = _h;

    return size;
}

struct d_pdf_rect
d_pdf_make_rect(
    double _x,
    double _y,
    double _w,
    double _h
)
{
    struct d_pdf_rect rect;

    rect.x      = _x;
    rect.y      = _y;
    rect.width  = _w;
    rect.height = _h;

    return rect;
}

/*
d_pdf_page_size
  The C++ presets, in enum order. An unknown preset gives Letter, the C++
page size's default.
*/
struct d_pdf_size
d_pdf_page_size(
    int32_t _preset
)
{
    static const double sizes[D_PDF_PAGE_COUNT][2] =
    {
        { 612.0,  792.0   },    // Letter
        { 612.0,  1008.0  },    // Legal
        { 792.0,  1224.0  },    // Tabloid
        { 841.89, 1190.55 },    // A3
        { 595.28, 841.89  },    // A4
        { 419.53, 595.28  }     // A5
    };
    const int32_t preset = ( (_preset >= 0) &&
                             (_preset < (int32_t)D_PDF_PAGE_COUNT) )
                               ? _preset
                               : (int32_t)D_PDF_PAGE_LETTER;

    return d_pdf_make_size(sizes[preset][0], sizes[preset][1]);
}

/*
d_pdf_landscape
  Swaps the extents, exactly as the C++ does: applied twice it gives the
portrait size back.
*/
struct d_pdf_size
d_pdf_landscape(
    struct d_pdf_size _size
)
{
    return d_pdf_make_size(_size.height, _size.width);
}

/*
d_internal_pdf_color
  A colour of the given space with every channel zero; the constructors set
the channels that space carries.
*/
static struct d_pdf_color
d_internal_pdf_color(
    int32_t _space
)
{
    struct d_pdf_color color;

    color.space    = _space;
    color.reserved = 0;
    color.r        = 0.0;
    color.g        = 0.0;
    color.b        = 0.0;
    color.c        = 0.0;
    color.m        = 0.0;
    color.y        = 0.0;
    color.k        = 0.0;

    return color;
}

struct d_pdf_color
d_pdf_color_rgb(
    double _r,
    double _g,
    double _b
)
{
    struct d_pdf_color color = d_internal_pdf_color((int32_t)D_PDF_COLOR_RGB);

    color.r = _r;
    color.g = _g;
    color.b = _b;

    return color;
}

/*
d_pdf_color_gray
  DeviceGray, with the rgb view set to the level on all three channels, as
the C++ device_gray does.
*/
struct d_pdf_color
d_pdf_color_gray(
    double _level
)
{
    struct d_pdf_color color = d_internal_pdf_color((int32_t)D_PDF_COLOR_GRAY);

    color.r = _level;
    color.g = _level;
    color.b = _level;

    return color;
}

/*
d_pdf_color_cmyk
  DeviceCMYK, keeping the four channels verbatim and filling the rgb view by
the naive conversion (1 - c)(1 - k), (1 - m)(1 - k), (1 - y)(1 - k), so an
rgb-only reader still has a colour to use. The formula is the C++ colour
module's; it computes in that module's float channels and this in double, so
the two views agree to float precision rather than bit for bit.
*/
struct d_pdf_color
d_pdf_color_cmyk(
    double _c,
    double _m,
    double _y,
    double _k
)
{
    struct d_pdf_color color = d_internal_pdf_color((int32_t)D_PDF_COLOR_CMYK);

    color.c = _c;
    color.m = _m;
    color.y = _y;
    color.k = _k;
    color.r = (1.0 - _c) * (1.0 - _k);
    color.g = (1.0 - _m) * (1.0 - _k);
    color.b = (1.0 - _y) * (1.0 - _k);

    return color;
}

/*
d_pdf_color_channel_count
  0 for a space the header does not define.
*/
int32_t
d_pdf_color_channel_count(
    int32_t _space
)
{
    switch (_space)
    {
        case D_PDF_COLOR_GRAY: return 1;
        case D_PDF_COLOR_RGB:  return 3;
        case D_PDF_COLOR_CMYK: return 4;
        default:               return 0;
    }
}

/*
d_pdf_base_font_name
  The standard-14 PostScript names; an unknown face is named Courier, as the
C++ default case does.
*/
const char*
d_pdf_base_font_name(
    int32_t _font
)
{
    static const char* const names[D_PDF_FONT_COUNT] =
    {
        "Courier",           "Courier-Bold",
        "Courier-Oblique",   "Courier-BoldOblique",
        "Helvetica",         "Helvetica-Bold",
        "Helvetica-Oblique", "Helvetica-BoldOblique",
        "Times-Roman",       "Times-Bold",
        "Times-Italic",      "Times-BoldItalic",
        "Symbol",            "ZapfDingbats"
    };

    return ( (_font >= 0) &&
             (_font < (int32_t)D_PDF_FONT_COUNT) )
        ? names[_font]
        : names[D_PDF_FONT_COURIER];
}

/*
d_pdf_font_is_monospaced
  The four Courier faces, named one by one as the C++ tests them rather than
as a range that happens to be contiguous.
*/
int32_t
d_pdf_font_is_monospaced(
    int32_t _font
)
{
    return ( (_font == (int32_t)D_PDF_FONT_COURIER)              ||
             (_font == (int32_t)D_PDF_FONT_COURIER_BOLD)         ||
             (_font == (int32_t)D_PDF_FONT_COURIER_OBLIQUE)      ||
             (_font == (int32_t)D_PDF_FONT_COURIER_BOLD_OBLIQUE) ) ? 1 : 0;
}

/*
d_pdf_average_advance_factor
  Per em: 0.6 for Courier, Symbol and ZapfDingbats, 0.5 for Times, and 0.52
for everything else, which is Helvetica and any unknown face.
*/
double
d_pdf_average_advance_factor(
    int32_t _font
)
{
    // Courier is monospaced at 600/1000 em
    if (d_pdf_font_is_monospaced(_font))
    {
        return 0.6;
    }

    switch (_font)
    {
        case D_PDF_FONT_TIMES_ROMAN:
        case D_PDF_FONT_TIMES_BOLD:
        case D_PDF_FONT_TIMES_ITALIC:
        case D_PDF_FONT_TIMES_BOLD_ITALIC:
            return 0.5;

        case D_PDF_FONT_SYMBOL:
        case D_PDF_FONT_ZAPF_DINGBATS:
            return 0.6;

        default:
            return 0.52;
    }
}

/*
d_internal_pdf_contains
  Case-insensitive substring test over ASCII, for family matching; the
needle is given in lower case.
*/
static int
d_internal_pdf_contains(
    const char* _haystack,
    const char* _needle
)
{
    for (size_t i = 0u; _haystack[i] != '\0'; ++i)
    {
        size_t j = 0u;

        // compare from here, folding the haystack's upper case
        while (_needle[j] != '\0')
        {
            char c = _haystack[i + j];

            if ( (c >= 'A') &&
                 (c <= 'Z') )
            {
                c = (char)(c - 'A' + 'a');
            }

            if (c != _needle[j])
            {
                break;
            }

            ++j;
        }

        if (_needle[j] == '\0')
        {
            return 1;
        }
    }

    return 0;
}

/*
d_pdf_base_font_from
  The C++ matching: a family naming "courier" or "mono" is Courier; one
naming "sans" is Helvetica, which is checked before "serif" so that
"sans-serif" is not read as Times; one naming "times" or "serif" is Times;
anything else, and a NULL family, is Helvetica. Bold and italic then pick
the face.
*/
int32_t
d_pdf_base_font_from(
    const char* _family,
    int32_t     _bold,
    int32_t     _italic
)
{
    const char* const family = (_family != NULL) ? _family : "";
    const int         bold   = (_bold != 0);
    const int         italic = (_italic != 0);

    // Courier
    if ( (d_internal_pdf_contains(family, "courier")) ||
         (d_internal_pdf_contains(family, "mono")) )
    {
        return (bold && italic) ? (int32_t)D_PDF_FONT_COURIER_BOLD_OBLIQUE
             : bold             ? (int32_t)D_PDF_FONT_COURIER_BOLD
             : italic           ? (int32_t)D_PDF_FONT_COURIER_OBLIQUE
             :                    (int32_t)D_PDF_FONT_COURIER;
    }

    // Times, unless the family is sans
    if ( (!d_internal_pdf_contains(family, "sans")) &&
         ( (d_internal_pdf_contains(family, "times")) ||
           (d_internal_pdf_contains(family, "serif")) ) )
    {
        return (bold && italic) ? (int32_t)D_PDF_FONT_TIMES_BOLD_ITALIC
             : bold             ? (int32_t)D_PDF_FONT_TIMES_BOLD
             : italic           ? (int32_t)D_PDF_FONT_TIMES_ITALIC
             :                    (int32_t)D_PDF_FONT_TIMES_ROMAN;
    }

    return (bold && italic) ? (int32_t)D_PDF_FONT_HELVETICA_BOLD_OBLIQUE
         : bold             ? (int32_t)D_PDF_FONT_HELVETICA_BOLD
         : italic           ? (int32_t)D_PDF_FONT_HELVETICA_OBLIQUE
         :                    (int32_t)D_PDF_FONT_HELVETICA;
}

struct d_pdf_font
d_pdf_font_init(void)
{
    return d_pdf_make_font((int32_t)D_PDF_FONT_COURIER, 10.0);
}

struct d_pdf_font
d_pdf_make_font(
    int32_t _family,
    double  _size
)
{
    struct d_pdf_font font;

    font.family   = _family;
    font.reserved = 0;
    font.size     = _size;

    return font;
}

/*
d_pdf_font_estimated_width
  count x size x the face's average advance: exact for Courier, an estimate
for the proportional faces, whose true widths are pdf_metrics.h's.
*/
double
d_pdf_font_estimated_width(
    const struct d_pdf_font* _font,
    size_t                   _char_count
)
{
    // no font, no width
    if (!_font)
    {
        return 0.0;
    }

    return (double)_char_count *
           _font->size *
           d_pdf_average_advance_factor(_font->family);
}

/*
d_pdf_paint_init
  Black stroke and fill, width 1, stroking on and filling off -- the C++
defaults.
*/
struct d_pdf_paint
d_pdf_paint_init(void)
{
    struct d_pdf_paint paint;

    paint.stroke     = d_pdf_color_rgb(0.0, 0.0, 0.0);
    paint.fill       = d_pdf_color_rgb(0.0, 0.0, 0.0);
    paint.line_width = 1.0;
    paint.do_stroke  = 1;
    paint.do_fill    = 0;

    return paint;
}
