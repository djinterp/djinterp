/******************************************************************************
* djinterp [utility]                                            pdf_primitives.h
*
* The leaf of the PDF kernel: geometry, page sizes, device colour and the
* standard-14 face enumeration.
*   These are the value types every other pdf module is expressed in terms of.
* In C++ the include graph runs pdf_metrics -> pdf -> pdf_document ->
* pdf_builtin_backend -> pdf_backend -> pdf_primitives, so measuring a string
* drags in the document model and the backend writer. The C tier splits where
* the dependencies actually are rather than where the C++ headers happen to
* sit, and this file is what pdf_metrics.h depends on.
*
*   THE FACE ENUM LIVES HERE AND NOWHERE ELSE. It was first declared in
* pdf_metrics.h -- fourteen enumerators, correct, and a second declaration of a
* type belonging to this module. Two enums describing one set of faces agree
* until somebody adds a face to one, and the drift is silent because both
* compile. `d_pdf_font_is_monospaced` moved for the same reason: it was written
* there as a range test over the Courier block, which agrees with the C++ four
* comparisons only because those faces happen to be contiguous -- agreement by
* coincidence of ordering rather than by shared derivation.
*
*   UNITS ARE POINTS AND THE TYPE IS `double`. PDF user space is defined in
* points and the page presets are fractional (A4 is 595.28 x 841.89). This is
* the one place the framework's millipoint discipline does not apply: ruling D4
* moved document spacing to millipoints so two languages could byte-compare it,
* but a page size that is 595.28 by definition cannot be expressed exactly in
* any integer unit.
*
*   COLOUR CHANNELS ARE STORED VERBATIM, NOT CLAMPED. The C++ rgb constructor
* stores its arguments as given -- measured, pdf_color(1.5, -0.2, 0.5) keeps
* 1.5 and -0.2. Clamping here would mean the two tiers disagree on every
* out-of-range colour, silently, with the C side "more correct" -- which is
* still a divergence. If clamping is wanted it belongs in both, as a ruling.
*
*
* path:      \inc\djinterp\c\util\pdf\pdf_primitives.h
* link(s):   ch-pdf.tex
* author(s): TBA                                            created: 2026.08.09
******************************************************************************/

#ifndef DJINTERP_C_UTIL_PDF_PRIMITIVES_
#define DJINTERP_C_UTIL_PDF_PRIMITIVES_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"


D_EXTERN_C_BEGIN

// D_PDF_POINTS_PER_INCH / D_PDF_POINTS_PER_MM
//   constant: conversions into PDF points.
#define D_PDF_POINTS_PER_INCH  72.0
#define D_PDF_POINTS_PER_MM    2.834645669291339

// d_pdf_point
//   type: a coordinate in PDF user space. Origin is BOTTOM-LEFT with y
// increasing upward -- the opposite of every screen coordinate system, so
// layout code ported from a raster target has the sign of every vertical
// offset to invert.
struct d_pdf_point
{
    double x;
    double y;
};

// d_pdf_size
//   type: a width / height extent in points.
struct d_pdf_size
{
    double width;
    double height;
};

// d_pdf_rect
//   type: an axis-aligned rectangle anchored at its LOWER-LEFT corner. With y
// increasing upward, "the corner nearest the origin" and "the corner drawn
// first" are the same point, which is why PDF anchors there.
struct d_pdf_rect
{
    double x;
    double y;
    double width;
    double height;
};

// d_pdf_page_preset
//   type: the named page extents. An enum plus a lookup, where C++ has six
// static factories: the factories cannot be spelled in C, and fourteen callers
// each writing 595.28 by hand is fourteen places for A4 to be typed wrong with
// no detector, because a page 0.01pt off still renders.
enum d_pdf_page_preset
{
    D_PDF_PAGE_LETTER  = 0,
    D_PDF_PAGE_LEGAL   = 1,
    D_PDF_PAGE_TABLOID = 2,
    D_PDF_PAGE_A3      = 3,
    D_PDF_PAGE_A4      = 4,
    D_PDF_PAGE_A5      = 5,

    D_PDF_PAGE_COUNT   = 6
};

// d_pdf_color_space
//   type: the PDF device colour spaces. Values are the C++ enum's values.
enum d_pdf_color_space
{
    D_PDF_COLOR_GRAY = 0,           // DeviceGray  (1 channel,  "g" / "G")
    D_PDF_COLOR_RGB  = 1,           // DeviceRGB   (3 channels, "rg" / "RG")
    D_PDF_COLOR_CMYK = 2,           // DeviceCMYK  (4 channels, "k" / "K")

    D_PDF_COLOR_SPACE_COUNT = 3
};

// d_pdf_color
//   type: a device colour for content-stream emission. The r/g/b view is
// ALWAYS populated whatever the space, so an rgb-only backend keeps working
// when handed a cmyk colour; a caller EMITTING must still branch on `space`,
// because emitting rgb for a cmyk colour changes the output's colour model.
struct d_pdf_color
{
    int32_t space;                  // enum d_pdf_color_space
    int32_t reserved;               // pad; must be 0
    double  r;                      // rgb view -- always valid
    double  g;
    double  b;
    double  c;                      // cmyk channels; meaningful when CMYK
    double  m;
    double  y;
    double  k;
};

// d_pdf_base_font
//   type: the standard-14 faces. Values are the C++ enum's, 0..13 in the same
// order: a caller bridging the tiers casts between them, and a renumbering
// here would measure every string in the wrong face -- producing plausible
// widths, which is the worst kind of wrong.
enum d_pdf_base_font
{
    D_PDF_FONT_COURIER                = 0,
    D_PDF_FONT_COURIER_BOLD           = 1,
    D_PDF_FONT_COURIER_OBLIQUE        = 2,
    D_PDF_FONT_COURIER_BOLD_OBLIQUE   = 3,
    D_PDF_FONT_HELVETICA              = 4,
    D_PDF_FONT_HELVETICA_BOLD         = 5,
    D_PDF_FONT_HELVETICA_OBLIQUE      = 6,
    D_PDF_FONT_HELVETICA_BOLD_OBLIQUE = 7,
    D_PDF_FONT_TIMES_ROMAN            = 8,
    D_PDF_FONT_TIMES_BOLD             = 9,
    D_PDF_FONT_TIMES_ITALIC           = 10,
    D_PDF_FONT_TIMES_BOLD_ITALIC      = 11,
    D_PDF_FONT_SYMBOL                 = 12,
    D_PDF_FONT_ZAPF_DINGBATS          = 13,

    D_PDF_FONT_COUNT                  = 14
};

// d_pdf_font
//   type: a base-14 face at a point size. Defaults are Courier at 10.0, which
// is the C++ default constructor's pair -- a zeroed struct gives Courier at
// 0.0, an invisible font, so d_pdf_font_init exists rather than a comment
// saying zero is fine.
struct d_pdf_font
{
    int32_t family;                 // enum d_pdf_base_font
    int32_t reserved;               // pad; must be 0
    double  size;                   // points
};

// d_pdf_paint
//   type: stroke and fill parameters for vector graphics. Defaults are black
// stroke, black fill, width 1.0, STROKE ON and FILL OFF -- a zeroed struct
// draws nothing at all, which reads as a broken backend rather than as an
// uninitialised paint.
struct d_pdf_paint
{
    struct d_pdf_color stroke;
    struct d_pdf_color fill;
    double             line_width;
    int32_t            do_stroke;
    int32_t            do_fill;
};

// I.   units
double d_pdf_inches(double _inches);
double d_pdf_millimetres(double _mm);

// II.  geometry
struct d_pdf_point d_pdf_make_point(double _x,
                                    double _y);
struct d_pdf_size  d_pdf_make_size(double _w,
                                   double _h);
struct d_pdf_rect  d_pdf_make_rect(double _x,
                                   double _y,
                                   double _w,
                                   double _h);

// III. page sizes
struct d_pdf_size d_pdf_page_size(int32_t _preset);
struct d_pdf_size d_pdf_landscape(struct d_pdf_size _size);

// IV.  colour
struct d_pdf_color d_pdf_color_rgb(double _r,
                                   double _g,
                                   double _b);
struct d_pdf_color d_pdf_color_gray(double _level);
struct d_pdf_color d_pdf_color_cmyk(double _c,
                                    double _m,
                                    double _y,
                                    double _k);
int32_t            d_pdf_color_channel_count(int32_t _space);

// V.   faces and paint
const char* d_pdf_base_font_name(int32_t _font);
int32_t     d_pdf_font_is_monospaced(int32_t _font);
double      d_pdf_average_advance_factor(int32_t _font);
int32_t     d_pdf_base_font_from(const char* _family,
                                 int32_t     _bold,
                                 int32_t     _italic);

struct d_pdf_font  d_pdf_font_init(void);
struct d_pdf_font  d_pdf_make_font(int32_t _family,
                                   double  _size);
double             d_pdf_font_estimated_width(const struct d_pdf_font* _font,
                                              size_t                   _char_count);
struct d_pdf_paint d_pdf_paint_init(void);

// VI.  layout assertions
D_STATIC_ASSERT(D_PDF_FONT_COUNT == 14,
                "a standard-14 face was added -- extend the name table, the "
                "advance factors, and pdf_metrics's width dispatch");
D_STATIC_ASSERT(D_PDF_PAGE_COUNT == 6,
                "a page preset was added -- extend d_pdf_page_size");
D_STATIC_ASSERT(D_PDF_COLOR_SPACE_COUNT == 3,
                "a colour space was added -- extend d_pdf_color_channel_count");
D_STATIC_ASSERT(offsetof(struct d_pdf_color, g) ==
                    offsetof(struct d_pdf_color, r) + sizeof(double),
                "d_pdf_color layout drift -- g must follow r");
D_STATIC_ASSERT(offsetof(struct d_pdf_color, b) ==
                    offsetof(struct d_pdf_color, g) + sizeof(double),
                "d_pdf_color layout drift -- b must follow g");

D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_PDF_PRIMITIVES_
