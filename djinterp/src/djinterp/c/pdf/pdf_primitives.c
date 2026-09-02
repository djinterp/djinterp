#include "../../../../../inc/djinterp/c/util/pdf/pdf_primitives.h"


/* =========================================================================
   I.     units
   ========================================================================= */

/*
d_pdf_inches
  Converts a measure in inches to points.

Parameter(s):
  _inches:  the measure.
Return:
  the same measure in points.
*/
double
d_pdf_inches
(
    double _inches
)
{
    return _inches * D_PDF_POINTS_PER_INCH;
}

/*
d_pdf_millimetres
  Converts a measure in millimetres to points.

Parameter(s):
  _mm:  the measure.
Return:
  the same measure in points.
*/
double
d_pdf_millimetres
(
    double _mm
)
{
    return _mm * D_PDF_POINTS_PER_MM;
}


/* =========================================================================
   II.    geometry
   ========================================================================= */

/*
d_pdf_make_point / d_pdf_make_size / d_pdf_make_rect
  Constructors for the three value types.

  BY VALUE, not through an out-parameter.  These are two to four doubles; a
  pointer costs more than the copy and turns a constructor into something that
  can fail.  It also lets them nest -- d_pdf_make_rect over a computed size --
  which is how the C++ constructors read at their call sites.

Parameter(s):
  as named.
Return:
  the value.
*/
struct d_pdf_point
d_pdf_make_point
(
    double _x,
    double _y
)
{
    struct d_pdf_point _p;

    _p.x = _x;
    _p.y = _y;

    return _p;
}

struct d_pdf_size
d_pdf_make_size
(
    double _w,
    double _h
)
{
    struct d_pdf_size _s;

    _s.width  = _w;
    _s.height = _h;

    return _s;
}

struct d_pdf_rect
d_pdf_make_rect
(
    double _x,
    double _y,
    double _w,
    double _h
)
{
    struct d_pdf_rect _r;

    _r.x      = _x;
    _r.y      = _y;
    _r.width  = _w;
    _r.height = _h;

    return _r;
}


/* =========================================================================
   III.   page sizes
   ========================================================================= */

/*
d_pdf_page_size
  The extent of a named preset, in points, portrait.

  THE NUMBERS ARE THE C++ SIDE'S, DIGIT FOR DIGIT.  A4 is 595.28 x 841.89 and
  not 595.276 x 841.89 -- the C++ presets are already rounded to two decimals,
  and "improving" them here would make one language's A4 a hundredth of a point
  wider than the other's.  That difference renders identically and compares
  unequal, which is the worst combination: invisible in output, fatal to a
  differential.

Parameter(s):
  _preset:  enum d_pdf_page_preset.
Return:
  the extent.  An unrecognised preset answers Letter, matching the C++ default
  constructor rather than trapping -- a page size has no error channel.
*/
struct d_pdf_size
d_pdf_page_size
(
    int32_t _preset
)
{
    switch (_preset)
    {
        case D_PDF_PAGE_LETTER:  return d_pdf_make_size(612.0,  792.0);
        case D_PDF_PAGE_LEGAL:   return d_pdf_make_size(612.0,  1008.0);
        case D_PDF_PAGE_TABLOID: return d_pdf_make_size(792.0,  1224.0);
        case D_PDF_PAGE_A3:      return d_pdf_make_size(841.89, 1190.55);
        case D_PDF_PAGE_A4:      return d_pdf_make_size(595.28, 841.89);
        case D_PDF_PAGE_A5:      return d_pdf_make_size(419.53, 595.28);
        default:                 break;
    }

    return d_pdf_make_size(612.0, 792.0);
}

/*
d_pdf_landscape
  The same extent with width and height swapped.

  TAKES A SIZE, NOT A PRESET, because a caller may have a custom page -- and a
  preset-only version would send that caller off to write the swap themselves,
  which is one more place to get it backwards.

Parameter(s):
  _size:  the portrait extent.
Return:
  the landscape extent.
*/
struct d_pdf_size
d_pdf_landscape
(
    struct d_pdf_size _size
)
{
    return d_pdf_make_size(_size.height, _size.width);
}


/* =========================================================================
   IV.    colour
   ========================================================================= */

/*
clamp01_helper
  Clamps a channel to 0..1.

  CLAMPED RATHER THAN TRUSTED, because a colour channel outside 0..1 is not an
  error a caller can be told about -- there is no return path -- and emitting
  it produces a content stream a reader may reject outright.

Parameter(s):
  _v:  the channel.
Return:
  the clamped channel.  A NaN falls through both comparisons and is returned
  unchanged, which is deliberate: silently turning NaN into 0 would hide the
  arithmetic mistake that produced it.
*/
static double
clamp01_helper
(
    double _v
)
{
    if (_v < 0.0)
    {
        return 0.0;
    }

    if (_v > 1.0)
    {
        return 1.0;
    }

    return _v;
}

/*
d_pdf_color_rgb
  An RGB device colour.

Parameter(s):
  _r, _g, _b:  channels, stored verbatim -- see the note in the body.
Return:
  the colour, with the cmyk channels zeroed.
*/
struct d_pdf_color
d_pdf_color_rgb
(
    double _r,
    double _g,
    double _b
)
{
    struct d_pdf_color _out;

    _out.space    = D_PDF_COLOR_RGB;
    _out.reserved = 0;
    /*   NOT CLAMPED, AND THAT IS THE C++ SIDE'S BEHAVIOUR RATHER THAN AN
       OVERSIGHT HERE.  The first draft clamped to 0..1, which is defensible in
       isolation -- an out-of-range channel produces a content stream a reader
       may reject.  But the C++ rgb constructor stores its arguments verbatim:
       measured, `pdf_color(1.5, -0.2, 0.5)` keeps 1.5 and -0.2.
         Clamping here would mean the two tiers disagree on every out-of-range
       colour, silently, with the C side "more correct" -- which is still a
       divergence and still the thing this port exists not to introduce.  If
       clamping is wanted it belongs in BOTH, as a ruling, not in the port. */
    _out.r        = _r;
    _out.g        = _g;
    _out.b        = _b;
    _out.c        = 0.0;
    _out.m        = 0.0;
    _out.y        = 0.0;
    _out.k        = 0.0;

    return _out;
}

/*
d_pdf_color_gray
  A grayscale device colour.

  THE rgb VIEW IS POPULATED WITH THE LEVEL IN ALL THREE CHANNELS, because the
  header promises r/g/b is always valid.  A gray colour whose rgb view was left
  at zero would render black through any rgb-only backend -- which is a silent
  wrong colour rather than a missing one, and would be blamed on the backend.

Parameter(s):
  _level:  0 (black) to 1 (white), clamped.
Return:
  the colour.
*/
struct d_pdf_color
d_pdf_color_gray
(
    double _level
)
{
    struct d_pdf_color _out;
    const double       _v = clamp01_helper(_level);

    _out.space    = D_PDF_COLOR_GRAY;
    _out.reserved = 0;
    _out.r        = _v;
    _out.g        = _v;
    _out.b        = _v;
    _out.c        = 0.0;
    _out.m        = 0.0;
    _out.y        = 0.0;
    _out.k        = 0.0;

    return _out;
}

/*
d_pdf_color_cmyk
  A CMYK device colour.

  THE rgb VIEW IS DERIVED, by the standard naive conversion
  r = (1-c)(1-k), and likewise for g and b.  This is not colour-managed and
  does not claim to be: it exists so an rgb-only backend handed a cmyk colour
  emits something close rather than something black.  A caller needing accurate
  separation wants a profile, which is out of this tier's scope.

Parameter(s):
  _c, _m, _y, _k:  channels, clamped to 0..1.
Return:
  the colour, with both views populated.
*/
struct d_pdf_color
d_pdf_color_cmyk
(
    double _c,
    double _m,
    double _y,
    double _k
)
{
    struct d_pdf_color _out;

    _out.space    = D_PDF_COLOR_CMYK;
    _out.reserved = 0;
    _out.c        = clamp01_helper(_c);
    _out.m        = clamp01_helper(_m);
    _out.y        = clamp01_helper(_y);
    _out.k        = clamp01_helper(_k);

    _out.r        = (1.0 - _out.c) * (1.0 - _out.k);
    _out.g        = (1.0 - _out.m) * (1.0 - _out.k);
    _out.b        = (1.0 - _out.y) * (1.0 - _out.k);

    return _out;
}

/*
d_pdf_color_channel_count
  How many channels a space emits.

Parameter(s):
  _space:  enum d_pdf_color_space.
Return:
  1, 3 or 4; 0 for an unrecognised space, which no valid space returns.
*/
int32_t
d_pdf_color_channel_count
(
    int32_t _space
)
{
    switch (_space)
    {
        case D_PDF_COLOR_GRAY: return 1;
        case D_PDF_COLOR_RGB:  return 3;
        case D_PDF_COLOR_CMYK: return 4;
        default:               break;
    }

    return 0;
}


/* =========================================================================
   V.     faces
   ========================================================================= */

/*
d_pdf_base_font_name
  The canonical PostScript BaseFont name.

  THE HYPHENATION IS PART OF THE NAME.  "Times-Roman" and "Helvetica-BoldOblique"
  are the exact strings a PDF /BaseFont entry must carry; "Times Roman" or
  "Helvetica-Bold-Oblique" name no font and a reader falls back silently.

Parameter(s):
  _font:  enum d_pdf_base_font.
Return:
  a static literal, borrowed, never freed.  An unrecognised face answers
  "Courier", matching the C++ default.
*/
const char*
d_pdf_base_font_name
(
    int32_t _font
)
{
    switch (_font)
    {
        case D_PDF_FONT_COURIER:                return "Courier";
        case D_PDF_FONT_COURIER_BOLD:           return "Courier-Bold";
        case D_PDF_FONT_COURIER_OBLIQUE:        return "Courier-Oblique";
        case D_PDF_FONT_COURIER_BOLD_OBLIQUE:   return "Courier-BoldOblique";
        case D_PDF_FONT_HELVETICA:              return "Helvetica";
        case D_PDF_FONT_HELVETICA_BOLD:         return "Helvetica-Bold";
        case D_PDF_FONT_HELVETICA_OBLIQUE:      return "Helvetica-Oblique";
        case D_PDF_FONT_HELVETICA_BOLD_OBLIQUE: return "Helvetica-BoldOblique";
        case D_PDF_FONT_TIMES_ROMAN:            return "Times-Roman";
        case D_PDF_FONT_TIMES_BOLD:             return "Times-Bold";
        case D_PDF_FONT_TIMES_ITALIC:           return "Times-Italic";
        case D_PDF_FONT_TIMES_BOLD_ITALIC:      return "Times-BoldItalic";
        case D_PDF_FONT_SYMBOL:                 return "Symbol";
        case D_PDF_FONT_ZAPF_DINGBATS:          return "ZapfDingbats";
        default:                                break;
    }

    return "Courier";
}

/*
d_pdf_font_is_monospaced
  Whether a face is monospaced.

  FOUR COMPARISONS, MIRRORING THE C++ SIDE, not the range test this was first
  written as.  Both spellings answer identically for all fourteen faces today,
  because the Courier block happens to be contiguous at 0..3.  They agree by
  coincidence of ORDERING rather than by shared derivation -- so a reordering
  of the enum breaks the range test and leaves the comparisons correct, and
  the two tiers would then disagree with nothing to report it.

Parameter(s):
  _font:  enum d_pdf_base_font.
Return:
  1 for the four Courier faces, 0 otherwise.
*/
int32_t
d_pdf_font_is_monospaced
(
    int32_t _font
)
{
    return ( (_font == D_PDF_FONT_COURIER)              ||
             (_font == D_PDF_FONT_COURIER_BOLD)         ||
             (_font == D_PDF_FONT_COURIER_OBLIQUE)      ||
             (_font == D_PDF_FONT_COURIER_BOLD_OBLIQUE) ) ? 1 : 0;
}

/*
d_pdf_average_advance_factor
  A coarse average advance, as a fraction of the point size.

Parameter(s):
  _font:  enum d_pdf_base_font.
Return:
  the factor.  The Helvetica family and any unrecognised face answer 0.52,
  which is the C++ default arm.
*/
double
d_pdf_average_advance_factor
(
    int32_t _font
)
{
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
        {
            return 0.5;
        }

        case D_PDF_FONT_SYMBOL:
        case D_PDF_FONT_ZAPF_DINGBATS:
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

/*
contains_helper
  Whether _haystack contains _needle, comparing ASCII case-insensitively.

  HAND-FOLDED 'A'..'Z', NOT tolower().  tolower() is locale-dependent: in a
  Turkish locale it folds 'I' to a dotless 'i', so "TIMES" stops matching
  "times" and every uppercase family name silently resolves to Helvetica.  The
  C++ side folds by hand for this reason and this side does the same.

  strstr() IS UNAVAILABLE for the same reason -- it is case-sensitive, and
  lowercasing the input first would need an allocation this tier does not have.

Parameter(s):
  _haystack:  the family name.
  _needle:    a lowercase literal to look for.
Return:
  1 when found, 0 otherwise; 0 for any NULL argument.
*/
static int32_t
contains_helper
(
    const char* _haystack,
    const char* _needle
)
{
    size_t _i = 0;

    if ( (!_haystack) || (!_needle) )
    {
        return 0;
    }

    for (_i = 0; _haystack[_i] != '\0'; ++_i)
    {
        size_t _j = 0;

        while (_needle[_j] != '\0')
        {
            char _h = _haystack[_i + _j];
            char _n = _needle[_j];

            if (_h == '\0')
            {
                return 0;       // haystack ran out mid-match
            }

            if ((_h >= 'A') && (_h <= 'Z'))
            {
                _h = (char)(_h - 'A' + 'a');
            }

            if (_h != _n)
            {
                break;
            }

            ++_j;
        }

        if (_needle[_j] == '\0')
        {
            return 1;
        }
    }

    return 0;
}

/*
d_pdf_base_font_from
  Resolves a family name plus bold/italic flags to a face.

  THE ORDER OF THE THREE TESTS IS LOAD-BEARING.  "sans" must be tested before
  "serif", because "sans-serif" contains both and the serif test would claim
  it -- resolving the most common family name on the web to Times.  The C++
  side carries that comment; sorting these into a table is exactly the tidy-up
  that breaks it.

Parameter(s):
  _family:  the requested family; NULL resolves to the Helvetica group.
  _bold:    non-zero for bold.
  _italic:  non-zero for italic / oblique.
Return:
  enum d_pdf_base_font.
*/
int32_t
d_pdf_base_font_from
(
    const char* _family,
    int32_t     _bold,
    int32_t     _italic
)
{
    enum { GROUP_HELVETICA = 0, GROUP_TIMES = 1, GROUP_COURIER = 2 };

    int32_t _group = GROUP_HELVETICA;

    if ( contains_helper(_family, "courier") ||
         contains_helper(_family, "mono") )
    {
        _group = GROUP_COURIER;
    }
    else if (contains_helper(_family, "sans"))
    {
        _group = GROUP_HELVETICA;       // "sans-serif" must beat "serif"
    }
    else if ( contains_helper(_family, "times") ||
              contains_helper(_family, "serif") )
    {
        _group = GROUP_TIMES;
    }

    switch (_group)
    {
        case GROUP_COURIER:
        {
            if (_bold && _italic) { return D_PDF_FONT_COURIER_BOLD_OBLIQUE; }
            if (_bold)            { return D_PDF_FONT_COURIER_BOLD; }
            if (_italic)          { return D_PDF_FONT_COURIER_OBLIQUE; }
            return D_PDF_FONT_COURIER;
        }

        case GROUP_TIMES:
        {
            if (_bold && _italic) { return D_PDF_FONT_TIMES_BOLD_ITALIC; }
            if (_bold)            { return D_PDF_FONT_TIMES_BOLD; }
            if (_italic)          { return D_PDF_FONT_TIMES_ITALIC; }
            return D_PDF_FONT_TIMES_ROMAN;
        }

        case GROUP_HELVETICA:
        default:
        {
            if (_bold && _italic) { return D_PDF_FONT_HELVETICA_BOLD_OBLIQUE; }
            if (_bold)            { return D_PDF_FONT_HELVETICA_BOLD; }
            if (_italic)          { return D_PDF_FONT_HELVETICA_OBLIQUE; }
            return D_PDF_FONT_HELVETICA;
        }
    }
}

/*
d_pdf_font_init
  The default font: Courier at 10 points.

  NOT A ZEROED STRUCT. Zero is Courier at 0.0 -- a font with no size, which
renders nothing and reads as a broken backend rather than as an uninitialised
value. The pair here is the C++ default constructor's.

Parameter(s):
  none.
Return:
  The default font.
*/
struct d_pdf_font
d_pdf_font_init
(
    void
)
{
    struct d_pdf_font _font;

    _font.family   = D_PDF_FONT_COURIER;
    _font.reserved = 0;
    _font.size     = 10.0;

    return _font;
}

/*
d_pdf_make_font
  A face at a point size.

Parameter(s):
  _family: enum d_pdf_base_font.
  _size:   points.
Return:
  The font.
*/
struct d_pdf_font
d_pdf_make_font
(
    int32_t _family,
    double  _size
)
{
    struct d_pdf_font _font;

    _font.family   = _family;
    _font.reserved = 0;
    _font.size     = _size;

    return _font;
}

/*
d_pdf_font_estimated_width
  An estimated rendered width for a character count at this font's size.

  AN ESTIMATE, AND CALLERS SHOULD PREFER d_pdf_text_width, WHICH MEASURES. It
is exact for Courier, where every advance is the same, and approximate for
every proportional face -- so a column sized with this and filled with "WWWW"
overflows. It exists for the case with no string to measure yet.

Parameter(s):
  _font:       the font; NULL estimates 0.
  _char_count: how many characters.
Return:
  The estimated width in points.
*/
double
d_pdf_font_estimated_width
(
    const struct d_pdf_font* _font,
    size_t                   _char_count
)
{
    if (!_font)
    {
        return 0.0;
    }

    return ( (double)_char_count *
             _font->size *
             d_pdf_average_advance_factor(_font->family) );
}

/*
d_pdf_paint_init
  The default paint: black, width 1, stroke on and fill off.

  A ZEROED STRUCT DRAWS NOTHING. do_stroke and do_fill would both be false and
line_width zero, so every vector call would succeed and produce an empty page --
a failure with no error and no output to inspect. These are the C++ default
constructor's values.

Parameter(s):
  none.
Return:
  The default paint.
*/
struct d_pdf_paint
d_pdf_paint_init
(
    void
)
{
    struct d_pdf_paint _paint;

    _paint.stroke     = d_pdf_color_gray(0.0);
    _paint.fill       = d_pdf_color_gray(0.0);
    _paint.line_width = 1.0;
    _paint.do_stroke  = 1;
    _paint.do_fill    = 0;

    return _paint;
}
