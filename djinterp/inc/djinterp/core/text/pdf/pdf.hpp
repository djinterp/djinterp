/******************************************************************************
* djinterp [pdf]                                                       pdf.hpp
*
* djinterp foundational PDF header:
*   This header provides the library-agnostic core of the djinterp PDF
* subsystem: the value types, the backend protocol, and a zero-dependency
* built-in backend.  It is the PDF analogue of print.hpp - a portable
* foundation that higher layers (pdf_template.hpp) build on without ever
* naming a concrete PDF library.
*
*   GREATEST COMMON SUBSET:
*   Every mainstream PDF engine (libHaru, PDFHummus, PoDoFo, cairo, ...)
* agrees on a small core of operations: open a document, start a page of a
* given size, place text at a point in a standard font and color, stroke
* lines, stroke/fill rectangles, attach document metadata, and serialize.
* That core - and only that core - is the abstract pdf_backend interface.
* Anything richer (image XObjects, TrueType embedding, annotations,
* outlines, encryption) is reported through pdf_capabilities and exposed,
* if at all, by backend-specific extension interfaces - never by this base.
*
*   BACKEND PROTOCOL:
*   pdf_backend is an abstract base class.  Unlike print.hpp, which dispatches
* on a hot per-character path and therefore uses compile-time SFINAE, a PDF
* backend sits on a cold document-assembly path and is genuinely a runtime
* plug-in (chosen by configuration, swapped without recompiling callers), so
* a virtual interface is the right tool.  The protocol is ALSO described
* structurally in pdf_template_traits.hpp, so a duck-typed backend that does
* not derive from pdf_backend is still recognized by is_pdf_backend<> and the
* C++20 concepts; inheritance is the recommended, not the required, route.
*
*   BUILT-IN BACKEND:
*   builtin_pdf_backend implements the common subset with no third-party
* dependency, emitting an uncompressed PDF 1.4 document that uses the
* standard-14 fonts (no font embedding).  It exists so the framework works
* out of the box; a production deployment can supply a libHaru / PDFHummus
* adapter deriving from pdf_backend and pass it to pdf_document.
*
*   COORDINATES:
*   pdf_document and pdf_backend operate in PDF user space: the origin is the
* bottom-left of the page and y increases upward, with all measures in points
* (1/72 inch).  Top-down flow layout is the concern of pdf_template.hpp, not
* of this foundation.
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h / env_*.h for version detection (D_NOEXCEPT,
* D_CONSTEXPR) and operating-system detection (creation-date timestamp).
*
*
* TABLE OF CONTENTS
* =================
* I.    UNITS & GEOMETRY
* II.   PAGE SIZES
* III.  COLOR
* IV.   FONTS
* V.    TEXT & PAINT OPTIONS
* VI.   CAPABILITIES
* VII.  BACKEND PROTOCOL
* VIII. INTERNAL: SERIALIZATION PRIMITIVES
* IX.   BUILT-IN BACKEND
* X.    DOCUMENT FACADE
*
*
* path:      /inc/djinterp/core/pdf/pdf.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_PDF_
#define DJINTERP_PDF_ 1

// std
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <memory>
#include <string>
#include <vector>
// djinterp
#include "../djinterp.hpp"


// D_KEYWORD_PDF
//   constant: keyword used to specify the PDF subsystem (document
// generation, layout, and output).  Defined locally because the
// core keyword set does not yet carry a PDF entry; the #ifndef
// guard lets a project-wide definition take precedence if one is
// later added to djinterp.h.
#ifndef D_KEYWORD_PDF
    #define D_KEYWORD_PDF   pdf
#endif  // D_KEYWORD_PDF

// NS_PDF
//   namespace: the `pdf` namespace for PDF document generation
// and output utilities.
#ifndef NS_PDF
    #define NS_PDF          D_NAMESPACE(D_KEYWORD_PDF)
#endif  // NS_PDF

// D_OVERRIDE
//   macro: portable `override` specifier.  pdf.hpp introduces the
// framework's first runtime-virtual interface; on C++11 and later
// `override` is a contextual keyword, while pre-C++11 toolchains
// do not recognize it, so the macro expands to nothing there.
// Pre-definable to override the detected value.
#ifndef D_OVERRIDE
    #if ( defined(__cplusplus) &&  \
          D_ENV_LANG_IS_CPP11_OR_HIGHER )
        #define D_OVERRIDE  override
    #else
        #define D_OVERRIDE
    #endif
#endif  // D_OVERRIDE

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
NS_PDF


///////////////////////////////////////////////////////////////////////////////
///                I.   UNITS & GEOMETRY                                     ///
///////////////////////////////////////////////////////////////////////////////

// pdf_unit
//   type: a linear measure in PostScript points (1/72 inch).  All
// geometry in the PDF subsystem is expressed in this unit.
typedef double pdf_unit;

// D_PDF_POINTS_PER_INCH
//   constant: points in one inch.
#define D_PDF_POINTS_PER_INCH 72.0

// D_PDF_POINTS_PER_MM
//   constant: points in one millimetre (72 / 25.4).
#define D_PDF_POINTS_PER_MM   2.834645669291339

// inches
//   function: converts a measure in inches to points.
D_CONSTEXPR pdf_unit
inches(
    pdf_unit _in
) D_NOEXCEPT
{
    return (_in * D_PDF_POINTS_PER_INCH);
}

// millimetres
//   function: converts a measure in millimetres to points.
D_CONSTEXPR pdf_unit
millimetres(
    pdf_unit _mm
) D_NOEXCEPT
{
    return (_mm * D_PDF_POINTS_PER_MM);
}


// pdf_point
//   struct: a coordinate in PDF user space (origin bottom-left,
// y increasing upward).
struct pdf_point
{
    pdf_unit x;
    pdf_unit y;

    D_CONSTEXPR pdf_point() D_NOEXCEPT
        : x(0.0), y(0.0)
    {}

    D_CONSTEXPR pdf_point(
        pdf_unit _x,
        pdf_unit _y
    ) D_NOEXCEPT
        : x(_x), y(_y)
    {}
};


// pdf_size
//   struct: a width / height extent in points.
struct pdf_size
{
    pdf_unit width;
    pdf_unit height;

    D_CONSTEXPR pdf_size() D_NOEXCEPT
        : width(0.0), height(0.0)
    {}

    D_CONSTEXPR pdf_size(
        pdf_unit _w,
        pdf_unit _h
    ) D_NOEXCEPT
        : width(_w), height(_h)
    {}
};


// pdf_rect
//   struct: an axis-aligned rectangle anchored at its lower-left
// corner with a width and height in points.
struct pdf_rect
{
    pdf_unit x;
    pdf_unit y;
    pdf_unit width;
    pdf_unit height;

    D_CONSTEXPR pdf_rect() D_NOEXCEPT
        : x(0.0), y(0.0), width(0.0), height(0.0)
    {}

    D_CONSTEXPR pdf_rect(
        pdf_unit _x,
        pdf_unit _y,
        pdf_unit _w,
        pdf_unit _h
    ) D_NOEXCEPT
        : x(_x), y(_y), width(_w), height(_h)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                II.  PAGE SIZES                                           ///
///////////////////////////////////////////////////////////////////////////////

// pdf_page_size
//   struct: a named page extent.  Thin wrapper over pdf_size so
// callers can pass either a preset (pdf_page_size::a4()) or an
// arbitrary custom size.
struct pdf_page_size
{
    pdf_size size;

    D_CONSTEXPR pdf_page_size() D_NOEXCEPT
        : size(612.0, 792.0)
    {}

    D_CONSTEXPR pdf_page_size(
        pdf_unit _w,
        pdf_unit _h
    ) D_NOEXCEPT
        : size(_w, _h)
    {}

    // presets (portrait orientation, points)
    static D_CONSTEXPR pdf_page_size letter()  D_NOEXCEPT { return pdf_page_size(612.0,  792.0);  }
    static D_CONSTEXPR pdf_page_size legal()   D_NOEXCEPT { return pdf_page_size(612.0,  1008.0); }
    static D_CONSTEXPR pdf_page_size tabloid() D_NOEXCEPT { return pdf_page_size(792.0,  1224.0); }
    static D_CONSTEXPR pdf_page_size a3()      D_NOEXCEPT { return pdf_page_size(841.89, 1190.55);}
    static D_CONSTEXPR pdf_page_size a4()      D_NOEXCEPT { return pdf_page_size(595.28, 841.89); }
    static D_CONSTEXPR pdf_page_size a5()      D_NOEXCEPT { return pdf_page_size(419.53, 595.28); }

    // landscape
    //   returns this size with width and height swapped.
    D_CONSTEXPR pdf_page_size
    landscape() const D_NOEXCEPT
    {
        return pdf_page_size(size.height, size.width);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. COLOR                                                ///
///////////////////////////////////////////////////////////////////////////////

// pdf_color
//   struct: an RGB color with components in the normalized range
// [0.0, 1.0], matching the PDF "rg" / "RG" color operators.
struct pdf_color
{
    double r;
    double g;
    double b;

    D_CONSTEXPR pdf_color() D_NOEXCEPT
        : r(0.0), g(0.0), b(0.0)
    {}

    D_CONSTEXPR pdf_color(
        double _r,
        double _g,
        double _b
    ) D_NOEXCEPT
        : r(_r), g(_g), b(_b)
    {}

    // from_rgb255
    //   factory: builds a color from 8-bit channel values.
    static D_CONSTEXPR pdf_color
    from_rgb255(
        int _r,
        int _g,
        int _b
    ) D_NOEXCEPT
    {
        return pdf_color(static_cast<double>(_r) / 255.0,
                         static_cast<double>(_g) / 255.0,
                         static_cast<double>(_b) / 255.0);
    }

    // named constants
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

// pdf_base_font
//   enum: the standard-14 PDF fonts.  Every conforming reader
// provides these without embedding, so they are the portable
// common-subset font set.
enum class pdf_base_font
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
//   function: maps a pdf_base_font to its canonical PostScript
// BaseFont name.
inline const char*
base_font_name(
    pdf_base_font _font
) D_NOEXCEPT
{
    switch (_font)
    {
        case pdf_base_font::courier:                { return "Courier"; }
        case pdf_base_font::courier_bold:           { return "Courier-Bold"; }
        case pdf_base_font::courier_oblique:        { return "Courier-Oblique"; }
        case pdf_base_font::courier_bold_oblique:   { return "Courier-BoldOblique"; }
        case pdf_base_font::helvetica:              { return "Helvetica"; }
        case pdf_base_font::helvetica_bold:         { return "Helvetica-Bold"; }
        case pdf_base_font::helvetica_oblique:      { return "Helvetica-Oblique"; }
        case pdf_base_font::helvetica_bold_oblique: { return "Helvetica-BoldOblique"; }
        case pdf_base_font::times_roman:            { return "Times-Roman"; }
        case pdf_base_font::times_bold:             { return "Times-Bold"; }
        case pdf_base_font::times_italic:           { return "Times-Italic"; }
        case pdf_base_font::times_bold_italic:      { return "Times-BoldItalic"; }
        case pdf_base_font::symbol:                 { return "Symbol"; }
        case pdf_base_font::zapf_dingbats:          { return "ZapfDingbats"; }
        default:                                    { return "Courier"; }
    }
}

// is_monospace_font
//   function: true if the base font is fixed-pitch (the Courier
// family).  Fixed-pitch fonts permit exact column layout.
inline bool
is_monospace_font(
    pdf_base_font _font
) D_NOEXCEPT
{
    return ( (_font == pdf_base_font::courier)              ||
             (_font == pdf_base_font::courier_bold)         ||
             (_font == pdf_base_font::courier_oblique)      ||
             (_font == pdf_base_font::courier_bold_oblique) );
}

// average_advance_factor
//   function: a representative glyph advance as a fraction of the
// font size.  Exact (0.6) for the fixed-pitch Courier family;
// an average approximation for the proportional families, used
// only for layout estimation when exact AFM metrics are absent.
inline double
average_advance_factor(
    pdf_base_font _font
) D_NOEXCEPT
{
    if (is_monospace_font(_font))
    {
        return 0.6;
    }

    switch (_font)
    {
        case pdf_base_font::times_roman:
        case pdf_base_font::times_bold:
        case pdf_base_font::times_italic:
        case pdf_base_font::times_bold_italic:
        {
            return 0.5;
        }

        case pdf_base_font::symbol:
        case pdf_base_font::zapf_dingbats:
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


// pdf_font
//   struct: a resolved font - a standard-14 base face paired with
// a point size.
struct pdf_font
{
    pdf_base_font family;
    pdf_unit      size;

    pdf_font()
        : family(pdf_base_font::courier),
          size(10.0)
    {}

    pdf_font(
        pdf_base_font _family,
        pdf_unit      _size
    ) D_NOEXCEPT
        : family(_family),
          size(_size)
    {}

    // convenience factories
    static pdf_font courier(pdf_unit _sz)   D_NOEXCEPT { return pdf_font(pdf_base_font::courier,   _sz); }
    static pdf_font helvetica(pdf_unit _sz) D_NOEXCEPT { return pdf_font(pdf_base_font::helvetica, _sz); }
    static pdf_font times(pdf_unit _sz)     D_NOEXCEPT { return pdf_font(pdf_base_font::times_roman,_sz); }

    // estimated_width
    //   returns an estimated rendered width for a character count
    // at this font's size.  Exact for Courier; approximate for
    // proportional faces.
    pdf_unit
    estimated_width(
        std::size_t _char_count
    ) const D_NOEXCEPT
    {
        return ( static_cast<pdf_unit>(_char_count) *
                 size *
                 average_advance_factor(family) );
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   TEXT & PAINT OPTIONS                                 ///
///////////////////////////////////////////////////////////////////////////////

// pdf_paint
//   struct: stroke / fill parameters for vector graphics.
struct pdf_paint
{
    pdf_color stroke;
    pdf_color fill;
    pdf_unit  line_width;
    bool      do_stroke;
    bool      do_fill;

    pdf_paint()
        : stroke(pdf_color::black()),
          fill(pdf_color::black()),
          line_width(1.0),
          do_stroke(true),
          do_fill(false)
    {}

    // stroked
    //   factory: a stroke-only paint with the given color and width.
    static pdf_paint
    stroked(
        const pdf_color& _color,
        pdf_unit         _width = 1.0
    )
    {
        pdf_paint p;

        p.stroke     = _color;
        p.line_width = _width;
        p.do_stroke  = true;
        p.do_fill    = false;

        return p;
    }

    // filled
    //   factory: a fill-only paint with the given color.
    static pdf_paint
    filled(
        const pdf_color& _color
    )
    {
        pdf_paint p;

        p.fill      = _color;
        p.do_stroke = false;
        p.do_fill   = true;

        return p;
    }
};


// pdf_text_align
//   enum: horizontal alignment hint for higher layers (the
// foundation places text at an explicit point; alignment is
// resolved by pdf_template).
enum class pdf_text_align
{
    left   = 0,
    center = 1,
    right  = 2
};


// pdf_text_options
//   struct: the full set of text-rendering parameters - font,
// color, alignment, and line leading.
struct pdf_text_options
{
    pdf_font       font;
    pdf_color      color;
    pdf_text_align align;
    pdf_unit       leading;

    pdf_text_options()
        : font(),
          color(pdf_color::black()),
          align(pdf_text_align::left),
          leading(12.0)
    {}

    explicit pdf_text_options(
        const pdf_font& _font
    )
        : font(_font),
          color(pdf_color::black()),
          align(pdf_text_align::left),
          leading(_font.size * 1.2)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  CAPABILITIES                                         ///
///////////////////////////////////////////////////////////////////////////////

// pdf_capabilities
//   struct: a backend's self-reported feature set beyond the
// common subset.  Higher layers query this before attempting an
// optional operation so they can degrade gracefully.
struct pdf_capabilities
{
    bool text;             // positioned text (always true)
    bool vector_graphics;  // lines and rectangles
    bool metadata;         // document information dictionary
    bool images;           // raster image XObjects
    bool custom_fonts;     // TrueType / Type0 embedding
    bool outlines;         // document outline / bookmarks
    bool annotations;      // link / text annotations
    bool encryption;       // document encryption
    bool compression;      // stream compression (deflate)

    pdf_capabilities()
        : text(true),
          vector_graphics(false),
          metadata(false),
          images(false),
          custom_fonts(false),
          outlines(false),
          annotations(false),
          encryption(false),
          compression(false)
    {}
};


///////////////////////////////////////////////////////////////////////////////
///                VII. BACKEND PROTOCOL                                     ///
///////////////////////////////////////////////////////////////////////////////

// pdf_backend
//   class: the abstract common-subset PDF backend.  A concrete
// backend (the built-in writer, or an adapter wrapping libHaru,
// PDFHummus, etc.) implements these operations; pdf_document
// drives them.  The protocol is deliberately minimal - it is the
// intersection of what every PDF engine supports.
//
//   Lifecycle contract:
//     begin_document()                once, first
//     begin_page(size) ... end_page() per page, in order
//     draw_* between begin_page / end_page only
//     end_document()                  once, last
//     serialize() / save()            after end_document()
class pdf_backend
{
public:
    virtual ~pdf_backend()
    {}

    // document lifecycle
    virtual void begin_document() = 0;
    virtual void end_document()   = 0;

    // page lifecycle
    virtual void begin_page(const pdf_size& _size) = 0;
    virtual void end_page()                        = 0;

    // text
    virtual void draw_text(
        const pdf_point&   _at,
        const std::string& _text,
        const pdf_font&    _font,
        const pdf_color&   _color) = 0;

    // vector graphics
    virtual void draw_line(
        const pdf_point& _from,
        const pdf_point& _to,
        const pdf_paint& _paint) = 0;

    virtual void draw_rect(
        const pdf_rect&  _rect,
        const pdf_paint& _paint) = 0;

    // metadata
    virtual void set_metadata(
        const std::string& _key,
        const std::string& _value) = 0;

    // introspection
    virtual pdf_capabilities capabilities() const = 0;

    // output
    virtual std::string serialize()              = 0;
    virtual bool        save(const char* _path)  = 0;
};


// backend_adapter
//   class: adapts any structurally-conforming backend - one that
// satisfies the common-subset protocol (see is_pdf_backend<> in
// pdf_template_traits.hpp) but does NOT derive from pdf_backend -
// to the pdf_backend interface by forwarding each operation.  This
// is what makes the structural detection actionable: a duck-typed
// backend can be driven by pdf_document without inheriting, simply
// by wrapping it.  The adapter borrows its target by reference; the
// target must outlive the adapter.
//
// Usage:
//   third_party_backend tp;            // does not derive pdf_backend
//   backend_adapter<third_party_backend> a(tp);
//   pdf_document doc(a);
template<typename _Backend>
class backend_adapter : public pdf_backend
{
public:
    explicit backend_adapter(
        _Backend& _backend
    ) D_NOEXCEPT
        : m_backend(&_backend)
    {}

    void
    begin_document() D_OVERRIDE
    {
        m_backend->begin_document();

        return;
    }

    void
    end_document() D_OVERRIDE
    {
        m_backend->end_document();

        return;
    }

    void
    begin_page(
        const pdf_size& _size
    ) D_OVERRIDE
    {
        m_backend->begin_page(_size);

        return;
    }

    void
    end_page() D_OVERRIDE
    {
        m_backend->end_page();

        return;
    }

    void
    draw_text(
        const pdf_point&   _at,
        const std::string& _text,
        const pdf_font&    _font,
        const pdf_color&   _color
    ) D_OVERRIDE
    {
        m_backend->draw_text(_at, _text, _font, _color);

        return;
    }

    void
    draw_line(
        const pdf_point& _from,
        const pdf_point& _to,
        const pdf_paint& _paint
    ) D_OVERRIDE
    {
        m_backend->draw_line(_from, _to, _paint);

        return;
    }

    void
    draw_rect(
        const pdf_rect&  _rect,
        const pdf_paint& _paint
    ) D_OVERRIDE
    {
        m_backend->draw_rect(_rect, _paint);

        return;
    }

    void
    set_metadata(
        const std::string& _key,
        const std::string& _value
    ) D_OVERRIDE
    {
        m_backend->set_metadata(_key, _value);

        return;
    }

    pdf_capabilities
    capabilities() const D_OVERRIDE
    {
        return m_backend->capabilities();
    }

    std::string
    serialize() D_OVERRIDE
    {
        return m_backend->serialize();
    }

    bool
    save(
        const char* _path
    ) D_OVERRIDE
    {
        return m_backend->save(_path);
    }

private:
    _Backend* m_backend;
};


///////////////////////////////////////////////////////////////////////////////
///                VIII. INTERNAL: SERIALIZATION PRIMITIVES                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // pdf_escape_text
    //   helper: escapes a run for a PDF literal string - '(', ')',
    // and '\' are backslash-escaped, printable ASCII passes through,
    // and any other byte becomes a three-digit octal escape.
    inline std::string
    pdf_escape_text(
        const std::string& _in
    )
    {
        std::string out;
        out.reserve(_in.size() + 8);

        for (std::size_t i = 0; i < _in.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(_in[i]);

            if ( (c == '(')  ||
                 (c == ')')  ||
                 (c == '\\') )
            {
                out.push_back('\\');
                out.push_back(static_cast<char>(c));

                continue;
            }

            if ( (c >= 0x20) &&
                 (c <= 0x7e) )
            {
                out.push_back(static_cast<char>(c));

                continue;
            }

            char buf[8];

            std::snprintf(buf, sizeof(buf), "\\%03o",
                          static_cast<unsigned int>(c));

            out += buf;
        }

        return out;
    }

    // pdf_num
    //   helper: locale-independent-enough decimal formatting of a
    // coordinate or scalar for content-stream emission.
    inline std::string
    pdf_num(
        double _v
    )
    {
        char buf[64];

        std::snprintf(buf, sizeof(buf), "%g", _v);

        return std::string(buf);
    }

    // pdf_creation_date
    //   helper: current local time as a PDF date string
    // ("D:YYYYMMDDHHmmSS").
    inline std::string
    pdf_creation_date()
    {
        std::time_t now = std::time(nullptr);
        std::tm     tmv;
        char        buf[32];

    #if D_INTERNAL_PDF_OS_WINDOWS
        ::localtime_s(&tmv, &now);
    #else
        tmv = *std::localtime(&now);
    #endif

        std::strftime(buf, sizeof(buf), "D:%Y%m%d%H%M%S", &tmv);

        return std::string(buf);
    }

    // pdf_begin_object
    //   helper: records the byte offset of object _obj_num and
    // writes its "N 0 obj" marker.
    inline void
    pdf_begin_object(
        std::string&              _out,
        std::vector<std::size_t>& _offsets,
        int                       _obj_num
    )
    {
        char buf[32];

        _offsets[static_cast<std::size_t>(_obj_num)] = _out.size();

        std::snprintf(buf, sizeof(buf), "%d 0 obj\n", _obj_num);
        _out += buf;

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                IX.  BUILT-IN BACKEND                                     ///
///////////////////////////////////////////////////////////////////////////////

// builtin_pdf_backend
//   class: zero-dependency pdf_backend producing an uncompressed
// PDF 1.4 document with the standard-14 fonts.  Drawing operations
// are recorded per page and serialized on demand.  This backend
// covers the common subset (text, lines, rectangles, metadata); it
// reports no image, embedding, outline, annotation, encryption, or
// compression support.
class builtin_pdf_backend : public pdf_backend
{
private:
    // op_kind
    //   enum: discriminator for a recorded drawing operation.
    enum class op_kind
    {
        text = 0,
        line = 1,
        rect = 2
    };

    // draw_op
    //   struct: a single recorded drawing operation.  A flat record
    // (rather than a variant) keeps the type C++11-trivial to store.
    struct draw_op
    {
        op_kind kind;

        // text
        pdf_point     text_at;
        std::string   text;
        pdf_base_font font;
        pdf_unit      font_size;
        pdf_color     text_color;

        // line
        pdf_point a;
        pdf_point b;

        // rect
        pdf_rect rect;

        // shared paint (line / rect)
        pdf_paint paint;

        draw_op()
            : kind(op_kind::text),
              text_at(),
              text(),
              font(pdf_base_font::courier),
              font_size(10.0),
              text_color(pdf_color::black()),
              a(),
              b(),
              rect(),
              paint()
        {}
    };

    // page_record
    //   struct: a page's size and ordered drawing operations.
    struct page_record
    {
        pdf_size             size;
        std::vector<draw_op> ops;

        page_record()
            : size(612.0, 792.0),
              ops()
        {}
    };

    // meta_entry
    //   struct: one document information key/value pair.
    struct meta_entry
    {
        std::string key;
        std::string value;
    };

public:
    builtin_pdf_backend()
        : m_pages(),
          m_meta(),
          m_open(false),
          m_page_open(false)
    {}


    // =================================================================
    //  document lifecycle
    // =================================================================

    void
    begin_document() D_OVERRIDE
    {
        m_pages.clear();
        m_meta.clear();
        m_open      = true;
        m_page_open = false;

        return;
    }

    void
    end_document() D_OVERRIDE
    {
        // close a still-open page defensively
        if (m_page_open)
        {
            end_page();
        }

        m_open = false;

        return;
    }


    // =================================================================
    //  page lifecycle
    // =================================================================

    void
    begin_page(
        const pdf_size& _size
    ) D_OVERRIDE
    {
        // implicitly close a previous page if the caller forgot
        if (m_page_open)
        {
            end_page();
        }

        page_record page;
        page.size = _size;

        m_pages.push_back(static_cast<page_record&&>(page));
        m_page_open = true;

        return;
    }

    void
    end_page() D_OVERRIDE
    {
        m_page_open = false;

        return;
    }


    // =================================================================
    //  text
    // =================================================================

    void
    draw_text(
        const pdf_point&   _at,
        const std::string& _text,
        const pdf_font&    _font,
        const pdf_color&   _color
    ) D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        draw_op op;
        op.kind       = op_kind::text;
        op.text_at    = _at;
        op.text       = _text;
        op.font       = _font.family;
        op.font_size  = _font.size;
        op.text_color = _color;

        m_pages.back().ops.push_back(
            static_cast<draw_op&&>(op));

        return;
    }


    // =================================================================
    //  vector graphics
    // =================================================================

    void
    draw_line(
        const pdf_point& _from,
        const pdf_point& _to,
        const pdf_paint& _paint
    ) D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        draw_op op;
        op.kind  = op_kind::line;
        op.a     = _from;
        op.b     = _to;
        op.paint = _paint;

        m_pages.back().ops.push_back(
            static_cast<draw_op&&>(op));

        return;
    }

    void
    draw_rect(
        const pdf_rect&  _rect,
        const pdf_paint& _paint
    ) D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        draw_op op;
        op.kind  = op_kind::rect;
        op.rect  = _rect;
        op.paint = _paint;

        m_pages.back().ops.push_back(
            static_cast<draw_op&&>(op));

        return;
    }


    // =================================================================
    //  metadata
    // =================================================================

    void
    set_metadata(
        const std::string& _key,
        const std::string& _value
    ) D_OVERRIDE
    {
        for (std::size_t i = 0; i < m_meta.size(); ++i)
        {
            if (m_meta[i].key == _key)
            {
                m_meta[i].value = _value;

                return;
            }
        }

        meta_entry e;
        e.key   = _key;
        e.value = _value;

        m_meta.push_back(static_cast<meta_entry&&>(e));

        return;
    }


    // =================================================================
    //  introspection
    // =================================================================

    pdf_capabilities
    capabilities() const D_OVERRIDE
    {
        pdf_capabilities caps;

        caps.text            = true;
        caps.vector_graphics = true;
        caps.metadata        = true;
        caps.images          = false;
        caps.custom_fonts    = false;
        caps.outlines        = false;
        caps.annotations     = false;
        caps.encryption      = false;
        caps.compression     = false;

        return caps;
    }


    // =================================================================
    //  output
    // =================================================================

    std::string
    serialize() D_OVERRIDE
    {
        return build_document();
    }

    bool
    save(
        const char* _path
    ) D_OVERRIDE
    {
        if (!_path)
        {
            return false;
        }

        std::FILE* fp = std::fopen(_path, "wb");

        if (!fp)
        {
            return false;
        }

        std::string pdf     = build_document();
        std::size_t written =
            std::fwrite(pdf.data(), 1, pdf.size(), fp);

        std::fclose(fp);

        return (written == pdf.size());
    }

private:
    // =================================================================
    //  internal: font collection
    // =================================================================

    // collect_fonts
    //   gathers the set of distinct base-font names used by text
    // operations across all pages.  Courier is always present so
    // every page has at least one usable font resource.
    std::vector<std::string>
    collect_fonts() const
    {
        std::vector<std::string> names;

        names.push_back(base_font_name(pdf_base_font::courier));

        for (std::size_t p = 0; p < m_pages.size(); ++p)
        {
            const std::vector<draw_op>& ops = m_pages[p].ops;

            for (std::size_t i = 0; i < ops.size(); ++i)
            {
                if (ops[i].kind != op_kind::text)
                {
                    continue;
                }

                std::string nm = base_font_name(ops[i].font);
                bool        seen = false;

                for (std::size_t k = 0; k < names.size(); ++k)
                {
                    if (names[k] == nm)
                    {
                        seen = true;
                        break;
                    }
                }

                if (!seen)
                {
                    names.push_back(nm);
                }
            }
        }

        return names;
    }

    // font_index
    //   returns the 1-based resource index (/F1, /F2, ...) for a
    // base-font name within the collected font list.
    static std::size_t
    font_index(
        const std::vector<std::string>& _names,
        const std::string&               _name
    )
    {
        for (std::size_t i = 0; i < _names.size(); ++i)
        {
            if (_names[i] == _name)
            {
                return (i + 1);
            }
        }

        return 1;
    }


    // =================================================================
    //  internal: content stream
    // =================================================================

    // build_content_stream
    //   serializes one page's ordered operations into a PDF content
    // stream.  Text uses BT/Tf/Tm/Tj/ET; lines and rectangles use
    // the path-painting operators.
    std::string
    build_content_stream(
        const page_record&              _page,
        const std::vector<std::string>& _fonts
    ) const
    {
        std::string s;

        for (std::size_t i = 0; i < _page.ops.size(); ++i)
        {
            const draw_op& op = _page.ops[i];

            if (op.kind == op_kind::text)
            {
                std::size_t fidx = font_index(
                    _fonts, base_font_name(op.font));

                s += "BT\n";
                s += "/F" + internal::pdf_num(
                    static_cast<double>(fidx)) + " " +
                    internal::pdf_num(op.font_size) + " Tf\n";
                s += internal::pdf_num(op.text_color.r) + " " +
                     internal::pdf_num(op.text_color.g) + " " +
                     internal::pdf_num(op.text_color.b) + " rg\n";
                s += "1 0 0 1 " +
                     internal::pdf_num(op.text_at.x) + " " +
                     internal::pdf_num(op.text_at.y) + " Tm\n";
                s += "(" + internal::pdf_escape_text(op.text) +
                     ") Tj\n";
                s += "ET\n";

                continue;
            }

            if (op.kind == op_kind::line)
            {
                s += internal::pdf_num(op.paint.line_width) + " w\n";
                s += internal::pdf_num(op.paint.stroke.r) + " " +
                     internal::pdf_num(op.paint.stroke.g) + " " +
                     internal::pdf_num(op.paint.stroke.b) + " RG\n";
                s += internal::pdf_num(op.a.x) + " " +
                     internal::pdf_num(op.a.y) + " m " +
                     internal::pdf_num(op.b.x) + " " +
                     internal::pdf_num(op.b.y) + " l S\n";

                continue;
            }

            // rectangle
            if (op.paint.do_fill)
            {
                s += internal::pdf_num(op.paint.fill.r) + " " +
                     internal::pdf_num(op.paint.fill.g) + " " +
                     internal::pdf_num(op.paint.fill.b) + " rg\n";
            }

            if (op.paint.do_stroke)
            {
                s += internal::pdf_num(op.paint.line_width) + " w\n";
                s += internal::pdf_num(op.paint.stroke.r) + " " +
                     internal::pdf_num(op.paint.stroke.g) + " " +
                     internal::pdf_num(op.paint.stroke.b) + " RG\n";
            }

            s += internal::pdf_num(op.rect.x) + " " +
                 internal::pdf_num(op.rect.y) + " " +
                 internal::pdf_num(op.rect.width) + " " +
                 internal::pdf_num(op.rect.height) + " re";

            if ( (op.paint.do_fill) &&
                 (op.paint.do_stroke) )
            {
                s += " B\n";
            }
            else if (op.paint.do_fill)
            {
                s += " f\n";
            }
            else if (op.paint.do_stroke)
            {
                s += " S\n";
            }
            else
            {
                s += " n\n";
            }
        }

        return s;
    }


    // =================================================================
    //  internal: document assembly
    // =================================================================

    // build_document
    //   assembles the full PDF.  Object layout: 1 Catalog, 2 Pages,
    // 3 Info, then F font objects, then a (page-dict, content) pair
    // per page.  Byte offsets feed a cross-reference table.
    std::string
    build_document() const
    {
        std::vector<std::string> fonts = collect_fonts();

        std::size_t font_count = fonts.size();
        std::size_t page_count = m_pages.size();

        // ensure a valid document even with no pages
        bool synth_page = (page_count == 0);

        if (synth_page)
        {
            page_count = 1;
        }

        // object numbering
        int   info_obj       = 3;
        int   first_font_obj = 4;
        int   first_page_obj =
            static_cast<int>(4 + font_count);
        std::size_t total_objs =
            3 + font_count + (2 * page_count);

        std::string              out;
        std::string              kids;
        std::string              res_fonts;
        std::vector<std::size_t> offsets(total_objs + 1, 0);
        char                     buf[256];

        // header with binary marker
        out += "%PDF-1.4\n";
        out += "%\xE2\xE3\xCF\xD3\n";

        // build the shared Font resource sub-dictionary
        for (std::size_t f = 0; f < font_count; ++f)
        {
            int fobj = first_font_obj + static_cast<int>(f);

            std::snprintf(buf, sizeof(buf),
                          "/F%zu %d 0 R ", (f + 1), fobj);
            res_fonts += buf;
        }

        // object 1: catalog
        internal::pdf_begin_object(out, offsets, 1);
        out += "<< /Type /Catalog /Pages 2 0 R >>\n";
        out += "endobj\n";

        // build Kids referencing each page dict
        for (std::size_t p = 0; p < page_count; ++p)
        {
            int page_obj =
                first_page_obj + static_cast<int>(2 * p);

            std::snprintf(buf, sizeof(buf), "%d 0 R ", page_obj);
            kids += buf;
        }

        // object 2: page tree
        internal::pdf_begin_object(out, offsets, 2);
        out += "<< /Type /Pages /Kids [";
        out += kids;
        out += "] /Count ";
        std::snprintf(buf, sizeof(buf), "%zu", page_count);
        out += buf;
        out += " >>\n";
        out += "endobj\n";

        // object 3: document information dictionary
        internal::pdf_begin_object(out, offsets, info_obj);
        out += "<< /Producer (djinterp pdf)";
        for (std::size_t i = 0; i < m_meta.size(); ++i)
        {
            // emit recognized keys with their PDF names; unknown
            // keys are skipped to keep the Info dict well-formed
            std::string pdfkey = info_key_name(m_meta[i].key);

            if (pdfkey.empty())
            {
                continue;
            }

            out += " /" + pdfkey + " (" +
                   internal::pdf_escape_text(m_meta[i].value) + ")";
        }
        out += " /CreationDate (" +
               internal::pdf_creation_date() + ") >>\n";
        out += "endobj\n";

        // font objects
        for (std::size_t f = 0; f < font_count; ++f)
        {
            int fobj = first_font_obj + static_cast<int>(f);

            internal::pdf_begin_object(out, offsets, fobj);
            out += "<< /Type /Font /Subtype /Type1 /BaseFont /";
            out += fonts[f];
            out += " /Encoding /WinAnsiEncoding >>\n";
            out += "endobj\n";
        }

        // page objects
        for (std::size_t p = 0; p < page_count; ++p)
        {
            int page_obj =
                first_page_obj + static_cast<int>(2 * p);
            int content_obj = page_obj + 1;

            pdf_size sz =
                synth_page ? pdf_size(612.0, 792.0) : m_pages[p].size;

            std::string content;

            if (!synth_page)
            {
                content = build_content_stream(m_pages[p], fonts);
            }

            // page dictionary
            internal::pdf_begin_object(out, offsets, page_obj);
            out += "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ";
            out += internal::pdf_num(sz.width) + " " +
                   internal::pdf_num(sz.height);
            out += "] /Resources << /Font << ";
            out += res_fonts;
            out += ">> >> /Contents ";
            std::snprintf(buf, sizeof(buf), "%d 0 R", content_obj);
            out += buf;
            out += " >>\n";
            out += "endobj\n";

            // content stream object
            internal::pdf_begin_object(out, offsets, content_obj);
            out += "<< /Length ";
            std::snprintf(buf, sizeof(buf), "%zu", content.size());
            out += buf;
            out += " >>\n";
            out += "stream\n";
            out += content;
            out += "endstream\n";
            out += "endobj\n";
        }

        // cross-reference table
        std::size_t xref_offset = out.size();

        out += "xref\n";
        std::snprintf(buf, sizeof(buf), "0 %zu\n", (total_objs + 1));
        out += buf;
        out += "0000000000 65535 f \n";

        for (std::size_t n = 1; n <= total_objs; ++n)
        {
            std::snprintf(buf, sizeof(buf), "%010zu 00000 n \n",
                          offsets[n]);
            out += buf;
        }

        // trailer
        out += "trailer\n";
        out += "<< /Size ";
        std::snprintf(buf, sizeof(buf), "%zu", (total_objs + 1));
        out += buf;
        out += " /Root 1 0 R /Info 3 0 R >>\n";
        out += "startxref\n";
        std::snprintf(buf, sizeof(buf), "%zu\n", xref_offset);
        out += buf;
        out += "%%EOF\n";

        return out;
    }

    // info_key_name
    //   maps a friendly metadata key to its PDF Info dictionary
    // name, or "" if the key is not a recognized Info field.
    static std::string
    info_key_name(
        const std::string& _key
    )
    {
        if (_key == "title")    { return "Title"; }
        if (_key == "author")   { return "Author"; }
        if (_key == "subject")  { return "Subject"; }
        if (_key == "keywords") { return "Keywords"; }
        if (_key == "creator")  { return "Creator"; }

        return "";
    }


    // =================================================================
    //  storage
    // =================================================================

    std::vector<page_record> m_pages;
    std::vector<meta_entry>  m_meta;
    bool                     m_open;
    bool                     m_page_open;
};


///////////////////////////////////////////////////////////////////////////////
///                X.   DOCUMENT FACADE                                      ///
///////////////////////////////////////////////////////////////////////////////

// pdf_document
//   class: the agnostic document façade.  Holds a pdf_backend
// (owning a built-in one by default, or borrowing a caller-supplied
// adapter) and exposes the common-subset drawing API in PDF user
// space.  Page state is tracked so add_page() closes the previous
// page automatically.
//
// Usage:
//   pdf_document doc;                       // built-in backend
//   doc.open();
//   doc.add_page(pdf_page_size::letter());
//   doc.text(pdf_point(72, 720), "Hello", pdf_text_options());
//   doc.close();
//   doc.save("out.pdf");
//
//   // with a custom backend:
//   libharu_pdf_backend hb;                 // derives pdf_backend
//   pdf_document doc2(hb);
class pdf_document
{
public:
    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default: owns a built-in backend
    pdf_document()
        : m_owned(new builtin_pdf_backend()),
          m_backend(nullptr),
          m_open(false),
          m_has_page(false)
    {
        m_backend = m_owned.get();
    }

    // borrow: drives a caller-owned backend (e.g. a libHaru adapter)
    explicit pdf_document(
        pdf_backend& _backend
    )
        : m_owned(),
          m_backend(&_backend),
          m_open(false),
          m_has_page(false)
    {}


    // =================================================================
    //  document lifecycle
    // =================================================================

    // open
    //   begins the document.  Idempotent.
    void
    open()
    {
        if (!m_open)
        {
            m_backend->begin_document();
            m_open     = true;
            m_has_page = false;
        }

        return;
    }

    // close
    //   ends the document.  Safe to call once after all pages.
    void
    close()
    {
        if (m_open)
        {
            m_backend->end_document();
            m_open     = false;
            m_has_page = false;
        }

        return;
    }


    // =================================================================
    //  page management
    // =================================================================

    // add_page
    //   starts a new page, opening the document lazily and closing
    // any prior page first.
    void
    add_page(
        const pdf_page_size& _size = pdf_page_size::letter()
    )
    {
        if (!m_open)
        {
            open();
        }

        if (m_has_page)
        {
            m_backend->end_page();
        }

        m_backend->begin_page(_size.size);
        m_has_page = true;

        return;
    }


    // =================================================================
    //  drawing
    // =================================================================

    // text
    //   draws a single text run at _at in the given options' font
    // and color (alignment is the caller's responsibility at this
    // layer - see pdf_template for flow layout).
    void
    text(
        const pdf_point&        _at,
        const std::string&      _text,
        const pdf_text_options& _opts = pdf_text_options()
    )
    {
        m_backend->draw_text(_at, _text, _opts.font, _opts.color);

        return;
    }

    // line
    void
    line(
        const pdf_point& _from,
        const pdf_point& _to,
        const pdf_paint& _paint = pdf_paint()
    )
    {
        m_backend->draw_line(_from, _to, _paint);

        return;
    }

    // rect
    void
    rect(
        const pdf_rect&  _rect,
        const pdf_paint& _paint = pdf_paint()
    )
    {
        m_backend->draw_rect(_rect, _paint);

        return;
    }

    // metadata
    //   records a document information field.  Opens the document
    // lazily (as add_page does) so metadata set before the first
    // page is not discarded by the deferred begin_document().
    void
    metadata(
        const std::string& _key,
        const std::string& _value
    )
    {
        if (!m_open)
        {
            open();
        }

        m_backend->set_metadata(_key, _value);

        return;
    }


    // =================================================================
    //  output
    // =================================================================

    // to_bytes
    //   ends the document if still open, then returns the serialized
    // PDF bytes.
    std::string
    to_bytes()
    {
        if (m_open)
        {
            close();
        }

        return m_backend->serialize();
    }

    // save
    //   ends the document if still open, then writes it to _path.
    bool
    save(
        const char* _path
    )
    {
        if (m_open)
        {
            close();
        }

        return m_backend->save(_path);
    }


    // =================================================================
    //  introspection
    // =================================================================

    pdf_backend&       backend()       D_NOEXCEPT { return *m_backend; }
    const pdf_backend& backend() const D_NOEXCEPT { return *m_backend; }

    pdf_capabilities
    capabilities() const
    {
        return m_backend->capabilities();
    }

private:
    // =================================================================
    //  storage
    // =================================================================

    std::unique_ptr<pdf_backend> m_owned;
    pdf_backend*                 m_backend;
    bool                         m_open;
    bool                         m_has_page;
};


NS_END  // pdf
NS_END  // djinterp


#undef D_INTERNAL_PDF_OS_WINDOWS


#endif  // DJINTERP_PDF_
