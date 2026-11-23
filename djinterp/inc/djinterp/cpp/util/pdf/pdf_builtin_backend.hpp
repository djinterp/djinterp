/*******************************************************************************
* djinterp [cpp][utility]                                pdf_builtin_backend.hpp
*
*
* path:      /inc/djinterp/cpp/util/pdf/pdf_builtin_backend.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.18
*******************************************************************************/
#ifndef DJINTERP_UTIL_PDF_PDF_BUILTIN_BACKEND_HPP
#define DJINTERP_UTIL_PDF_PDF_BUILTIN_BACKEND_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

///////////////////////////////////////////////////////////////////////////////
// pdf_builtin_backend.hpp
//
// The built-in, dependency-free PDF backend together with the internal
// content-stream serialization primitives it relies on.  Emits the
// color operators (rg/RG, k/K, g/G) and base-14 font names.  Depends on
// the backend protocol in pdf_backend.hpp.
///////////////////////////////////////////////////////////////////////////////

// djinterp
#include "./pdf_backend.hpp"                  // DPdfBackend, DPdfCompression
#include "../../../c/util/compress/compress.h"  // d_compress, D_CODEC_ID_ZLIB
#include "../../../c/util/ostream/ostream.h"  // d_ostream_init_fixed
#include "../../../c/text/ostream_format.h"   // d_ostream_write_double
#include "../../../config/cpp/util/pdf/cfg_pdf.h"  // D_INTERNAL_PDF_COMPRESS

NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                VIII. INTERNAL: SERIALIZATION PRIMITIVES                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // pdf_escape_text
    //   helper: escapes a run for a PDF literal string - '(', ')',
    // and '\' are backslash-escaped, printable ASCII passes through,
    // and any other byte becomes a three-digit octal escape.
    inline
    std::string pdf_escape_text(
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
    //   helper: a coordinate or scalar for content-stream emission, as "%g"
    // writes it in the C locale, whatever the program's locale - the C
    // tier's d_pdf_num, byte for byte, through the same d_ostream_write_double
    // (the decimal-point ruling, stream 1). "%g" under a German locale wrote
    // "1,5", which no PDF reader takes as a number.
    inline
    std::string pdf_num(
        double _v
    )
    {
        char              buf[64];
        dj_ostream        os;
        dj_ostream_fixed  fixed;

        (void)d_ostream_init_fixed(&os, &fixed, buf, sizeof(buf), 1);
        (void)d_ostream_write_double(&os, _v, 'g', -1);
        (void)d_ostream_close(&os);

        return std::string(buf);
    }

    // pdf_set_color
    //   helper: the content-stream operator(s) selecting _color as the
    // fill (lower-case) or stroke (upper-case) color, emitted in the
    // model's native PDF device space (DeviceGray / RGB / CMYK).
    inline
    std::string pdf_set_color(
        const pdf_color& _color,
        bool             _fill
    )
    {
        switch (_color.space)
        {
        case DPdfColorSpace::gray:
            return pdf_num(_color.r) + (_fill ? " g\n" : " G\n");

        case DPdfColorSpace::cmyk:
            return pdf_num(_color.c) + " " + pdf_num(_color.m) + " " +
                   pdf_num(_color.y) + " " + pdf_num(_color.k) +
                   (_fill ? " k\n" : " K\n");

        case DPdfColorSpace::rgb:
        default:
            return pdf_num(_color.r) + " " + pdf_num(_color.g) + " " +
                   pdf_num(_color.b) + (_fill ? " rg\n" : " RG\n");
        }
    }

    // pdf_creation_date
    //   helper: current local time as a PDF date string
    // ("D:YYYYMMDDHHmmSS").
    inline
    std::string pdf_creation_date()
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
    inline
    void pdf_begin_object(
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

    // pdf_deflate
    //   helper: _in in zlib's framing, which FlateDecode reads, at _level,
    // into _out. False, with _out empty, when the build has no zlib or the
    // kernel fails; the caller then writes the stream raw.
    inline
    bool pdf_deflate(
        const std::string& _in,
        int                _level,
        std::string&       _out
    )
    {
        const struct dj_compress_options options =
            d_compress_at_level(static_cast<int32_t>(_level));
        std::size_t bound = 0;
        std::size_t size  = 0;

        _out.clear();

        if ( (d_compress_bound(D_CODEC_ID_ZLIB, _in.size(), &options,
                               &bound) != D_PACK_STATUS_OK) ||
             (bound == 0) )
        {
            return false;
        }

        _out.resize(bound);

        if (d_compress(D_CODEC_ID_ZLIB, _in.data(), _in.size(), &options,
                       &_out[0], _out.size(), &size) != D_PACK_STATUS_OK)
        {
            _out.clear();

            return false;
        }

        _out.resize(size);

        return true;
    }

    // pdf_png_paeth
    //   helper: PNG's Paeth predictor: of the left, upper and upper-left
    // bytes, the one nearest their sum less the upper-left.
    inline
    unsigned pdf_png_paeth(
        unsigned _a,
        unsigned _b,
        unsigned _c
    )
    {
        const int p  = static_cast<int>(_a + _b) - static_cast<int>(_c);
        const int pa = (p > static_cast<int>(_a)) ? (p - static_cast<int>(_a))
                                                  : (static_cast<int>(_a) - p);
        const int pb = (p > static_cast<int>(_b)) ? (p - static_cast<int>(_b))
                                                  : (static_cast<int>(_b) - p);
        const int pc = (p > static_cast<int>(_c)) ? (p - static_cast<int>(_c))
                                                  : (static_cast<int>(_c) - p);

        if ( (pa <= pb) && (pa <= pc) )
        {
            return _a;
        }

        return (pb <= pc) ? _b : _c;
    }

    // pdf_png_filter_row
    //   helper: one row through one of PNG's five filters (0 None, 1 Sub,
    // 2 Up, 3 Average, 4 Paeth), _bpp bytes a pixel, into _out; returns the
    // sum of the filtered bytes' magnitudes as signed bytes, the measure PNG
    // writers choose a filter by.
    inline
    unsigned long pdf_png_filter_row(
        unsigned             _filter,
        const unsigned char* _row,
        const unsigned char* _above,
        std::size_t          _length,
        std::size_t          _bpp,
        unsigned char*       _out
    )
    {
        unsigned long sum = 0;

        for (std::size_t i = 0; i < _length; ++i)
        {
            const unsigned a = (i >= _bpp) ? _row[i - _bpp] : 0u;
            const unsigned b = _above[i];
            const unsigned c = (i >= _bpp) ? _above[i - _bpp] : 0u;
            unsigned       predicted = 0u;
            unsigned char  value;

            switch (_filter)
            {
                case 1:  predicted = a;                           break;
                case 2:  predicted = b;                           break;
                case 3:  predicted = (a + b) / 2u;                break;
                case 4:  predicted = pdf_png_paeth(a, b, c);      break;
                default: predicted = 0u;                          break;
            }

            value   = static_cast<unsigned char>((_row[i] - predicted) & 0xffu);
            _out[i] = value;
            sum    += (value < 128u) ? value : (256u - value);
        }

        return sum;
    }

    // pdf_png_predict
    //   helper: _height rows of _width pixels, _colors bytes each, as PNG
    // predicts them for FlateDecode's /Predictor 15: each row led by the
    // filter that gives it the least sum, then the row through it.
    inline
    std::string pdf_png_predict(
        const std::string& _samples,
        std::size_t        _width,
        std::size_t        _height,
        std::size_t        _colors
    )
    {
        const std::size_t          stride = _width * _colors;
        std::vector<unsigned char> above(stride, 0u);
        std::vector<unsigned char> trial(stride + 1u, 0u);
        std::vector<unsigned char> best(stride + 1u, 0u);
        std::string                out;

        out.reserve(_height * (stride + 1u));

        for (std::size_t y = 0; y < _height; ++y)
        {
            const unsigned char* row = reinterpret_cast<const unsigned char*>(
                                           _samples.data() + (y * stride));
            unsigned long        least = 0;

            for (unsigned filter = 0; filter < 5u; ++filter)
            {
                const unsigned long sum =
                    pdf_png_filter_row(filter, row, above.data(), stride,
                                       _colors, &trial[1]);

                if ( (filter == 0u) || (sum < least) )
                {
                    least    = sum;
                    trial[0] = static_cast<unsigned char>(filter);
                    best.swap(trial);
                }
            }

            out.append(reinterpret_cast<const char*>(best.data()),
                       best.size());
            above.assign(row, row + stride);
        }

        return out;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                IX.  BUILT-IN BACKEND                                     ///
///////////////////////////////////////////////////////////////////////////////

// DBuiltinPdfBackend
//   class: zero-dependency DPdfBackend producing an uncompressed
// PDF 1.4 document with the standard-14 fonts.  Drawing operations
// are recorded per page and serialized on demand.  This backend
// covers the common subset (text, lines, rectangles), vector paths,
// graphics state, raster images (RGB / RGBA / gray), and metadata.
// Its streams are FlateDecode, an image's after PNG's predictors,
// unless the build's knob (D_CFG_PDF_COMPRESS) or set_compression
// says raw, or the build has no zlib; it reports no font embedding,
// outline, annotation, or encryption support.
class DBuiltinPdfBackend : public DPdfBackend
{
private:
    // op_kind
    //   enum: discriminator for a recorded drawing operation.
    enum class op_kind
    {
        text          = 0,
        line          = 1,
        rect          = 2,
        path          = 3,
        save_state    = 4,
        restore_state = 5,
        image         = 6
    };

    // DDrawOp
    //   struct: a single recorded drawing operation.  A flat record
    // (rather than a variant) keeps the type C++11-trivial to store.
    struct DDrawOp
    {
        op_kind kind;

        // text
        DPdfPoint     text_at;
        std::string   text;
        DPdfBaseFont font;
        DPdfUnit      font_size;
        pdf_color     text_color;

        // line
        DPdfPoint a;
        DPdfPoint b;

        // rect
        DPdfRect rect;

        // path
        DPdfPath path;

        // image: index into the backend's image table, and the
        // destination rectangle the image is scaled into
        std::size_t image_index;
        DPdfRect    image_dest;

        // shared paint (line / rect / path)
        DPdfPaint paint;

        DDrawOp()
            : kind(op_kind::text),
              text_at(),
              text(),
              font(DPdfBaseFont::courier),
              font_size(10.0),
              text_color(pdf_color::black()),
              a(),
              b(),
              rect(),
              path(),
              image_index(0),
              image_dest(),
              paint()
        {}
    };

    // DPageRecord
    //   struct: a page's size and ordered drawing operations.
    struct DPageRecord
    {
        DPdfSize             size;
        std::vector<DDrawOp> ops;

        DPageRecord()
            : size(612.0, 792.0),
              ops()
        {}
    };

    // DMetaEntry
    //   struct: one document information key/value pair.
    struct DMetaEntry
    {
        std::string key;
        std::string value;
    };

public:
    DBuiltinPdfBackend()
        : m_pages(),
          m_meta(),
          m_images(),
          m_open(false),
          m_page_open(false),
          m_compression(DPdfCompression::by_default)
    {}


    // =================================================================
    //  document lifecycle
    // =================================================================

    void begin_document() D_OVERRIDE
    {
        m_pages.clear();
        m_meta.clear();
        m_images.clear();
        m_open      = true;
        m_page_open = false;

        return;
    }

    void end_document() D_OVERRIDE
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

    void begin_page(
        const DPdfSize& _size
    ) D_OVERRIDE
    {
        // implicitly close a previous page if the caller forgot
        if (m_page_open)
        {
            end_page();
        }

        DPageRecord page;
        page.size = _size;

        m_pages.push_back(static_cast<DPageRecord&&>(page));
        m_page_open = true;

        return;
    }

    void end_page() D_OVERRIDE
    {
        m_page_open = false;

        return;
    }


    // =================================================================
    //  text
    // =================================================================

    void draw_text(
        const DPdfPoint&   _at,
        const std::string& _text,
        const pdf_font&    _font,
        const pdf_color&   _color
    ) D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        DDrawOp op;
        op.kind       = op_kind::text;
        op.text_at    = _at;
        op.text       = _text;
        op.font       = _font.family;
        op.font_size  = _font.size;
        op.text_color = _color;

        m_pages.back().ops.push_back(
            static_cast<DDrawOp&&>(op));

        return;
    }


    // =================================================================
    //  vector graphics
    // =================================================================

    void draw_line(
        const DPdfPoint& _from,
        const DPdfPoint& _to,
        const DPdfPaint& _paint
    ) D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        DDrawOp op;
        op.kind  = op_kind::line;
        op.a     = _from;
        op.b     = _to;
        op.paint = _paint;

        m_pages.back().ops.push_back(
            static_cast<DDrawOp&&>(op));

        return;
    }

    void draw_rect(
        const DPdfRect&  _rect,
        const DPdfPaint& _paint
    ) D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        DDrawOp op;
        op.kind  = op_kind::rect;
        op.rect  = _rect;
        op.paint = _paint;

        m_pages.back().ops.push_back(
            static_cast<DDrawOp&&>(op));

        return;
    }

    void draw_path(
        const DPdfPath&  _path,
        const DPdfPaint& _paint
    ) D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        DDrawOp op;
        op.kind  = op_kind::path;
        op.path  = _path;
        op.paint = _paint;

        m_pages.back().ops.push_back(
            static_cast<DDrawOp&&>(op));

        return;
    }

    void save_state() D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        DDrawOp op;
        op.kind = op_kind::save_state;

        m_pages.back().ops.push_back(
            static_cast<DDrawOp&&>(op));

        return;
    }

    void restore_state() D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        DDrawOp op;
        op.kind = op_kind::restore_state;

        m_pages.back().ops.push_back(
            static_cast<DDrawOp&&>(op));

        return;
    }

    void draw_image(
        const DPdfImage& _image,
        const DPdfRect&  _dest
    ) D_OVERRIDE
    {
        if ( (m_pages.empty()) ||
             (!_image.valid()) )
        {
            return;
        }

        // store the image and record an op referencing it by index
        m_images.push_back(_image);

        DDrawOp op;
        op.kind        = op_kind::image;
        op.image_index = (m_images.size() - 1);
        op.image_dest  = _dest;

        m_pages.back().ops.push_back(
            static_cast<DDrawOp&&>(op));

        return;
    }


    // =================================================================
    //  metadata
    // =================================================================

    void set_metadata(
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

        DMetaEntry e;
        e.key   = _key;
        e.value = _value;

        m_meta.push_back(static_cast<DMetaEntry&&>(e));

        return;
    }


    // =================================================================
    //  introspection
    // =================================================================

    DPdfCapabilities capabilities() const D_OVERRIDE
    {
        DPdfCapabilities caps;

        caps.text            = true;
        caps.vector_graphics = true;
        caps.metadata        = true;
        caps.images          = true;
        caps.custom_fonts    = false;
        caps.outlines        = false;
        caps.annotations     = false;
        caps.encryption      = false;
        caps.compression     = compressing();

        return caps;
    }

    // set_compression
    //   function: how the next document's streams are written: as the
    // build's knob says (by_default), FlateDecode (flate), or raw (none).
    // FlateDecode needs zlib in the build; without it streams are raw, and
    // capabilities().compression says so.
    void set_compression(
        DPdfCompression _compression
    ) D_OVERRIDE
    {
        m_compression = _compression;

        return;
    }


    // =================================================================
    //  output
    // =================================================================

    std::string serialize() D_OVERRIDE
    {
        return build_document();
    }

    bool save(
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
    std::vector<std::string> collect_fonts() const
    {
        std::vector<std::string> names;

        names.push_back(base_font_name(DPdfBaseFont::courier));

        for (std::size_t p = 0; p < m_pages.size(); ++p)
        {
            const std::vector<DDrawOp>& ops = m_pages[p].ops;

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
    static
    std::size_t font_index(
        const std::vector<std::string>& _names,
        const std::string&              _name
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
    std::string build_content_stream(
        const DPageRecord&              _page,
        const std::vector<std::string>& _fonts
    ) const
    {
        std::string s;

        for (std::size_t i = 0; i < _page.ops.size(); ++i)
        {
            emit_op(s, _page.ops[i], _fonts);
        }

        return s;
    }

    // emit_op
    //   writes one recorded operation's operators.
    static
    void emit_op(
        std::string&                    _s,
        const DDrawOp&                  _op,
        const std::vector<std::string>& _fonts
    )
    {
        switch (_op.kind)
        {
            case op_kind::text:
                emit_text_op(_s, _op, _fonts);
                break;
            case op_kind::line:
                emit_line_op(_s, _op);
                break;
            case op_kind::save_state:
                _s += "q\n";
                break;
            case op_kind::restore_state:
                _s += "Q\n";
                break;
            case op_kind::path:
                emit_paint_setup(_s, _op.paint);
                emit_path_geometry(_s, _op.path);
                _s += paint_suffix(_op.paint);
                break;
            case op_kind::image:
                emit_image_op(_s, _op);
                break;
            default:
                emit_rect_op(_s, _op);
                break;
        }

        return;
    }

    // emit_text_op
    //   writes a text run: BT, its font and size, its color, its
    // position as a text matrix, the escaped string, ET.
    static
    void emit_text_op(
        std::string&                    _s,
        const DDrawOp&                  _op,
        const std::vector<std::string>& _fonts
    )
    {
        std::size_t fidx = font_index(_fonts, base_font_name(_op.font));

        _s += "BT\n";
        _s += "/F" + internal::pdf_num(static_cast<double>(fidx)) + " " +
              internal::pdf_num(_op.font_size) + " Tf\n";
        _s += internal::pdf_set_color(_op.text_color, true);
        _s += "1 0 0 1 " +
              internal::pdf_num(_op.text_at.x) + " " +
              internal::pdf_num(_op.text_at.y) + " Tm\n";
        _s += "(" + internal::pdf_escape_text(_op.text) + ") Tj\n";
        _s += "ET\n";

        return;
    }

    // emit_line_op
    //   writes a stroked segment with its width and color.
    static
    void emit_line_op(
        std::string&   _s,
        const DDrawOp& _op
    )
    {
        _s += internal::pdf_num(_op.paint.line_width) + " w\n";
        _s += internal::pdf_set_color(_op.paint.stroke, false);
        _s += internal::pdf_num(_op.a.x) + " " +
              internal::pdf_num(_op.a.y) + " m " +
              internal::pdf_num(_op.b.x) + " " +
              internal::pdf_num(_op.b.y) + " l S\n";

        return;
    }

    // emit_image_op
    //   places an image by mapping the unit square to the destination
    // rectangle via the cm matrix, then invoking the XObject; q/Q
    // isolates the transform.
    static
    void emit_image_op(
        std::string&   _s,
        const DDrawOp& _op
    )
    {
        const DPdfRect& d = _op.image_dest;

        _s += "q\n";
        _s += internal::pdf_num(d.width) + " 0 0 " +
              internal::pdf_num(d.height) + " " +
              internal::pdf_num(d.x) + " " +
              internal::pdf_num(d.y) + " cm\n";
        _s += "/Im" +
              internal::pdf_num(static_cast<double>(_op.image_index)) +
              " Do\n";
        _s += "Q\n";

        return;
    }

    // emit_rect_op
    //   writes a rectangle with the paint's fill and stroke state.
    static
    void emit_rect_op(
        std::string&   _s,
        const DDrawOp& _op
    )
    {
        emit_paint_setup(_s, _op.paint);
        _s += internal::pdf_num(_op.rect.x) + " " +
              internal::pdf_num(_op.rect.y) + " " +
              internal::pdf_num(_op.rect.width) + " " +
              internal::pdf_num(_op.rect.height) + " re";
        _s += paint_suffix(_op.paint);

        return;
    }

    // emit_paint_setup
    //   writes the fill-color, stroke-color, and line-width state
    // operators implied by a paint, ahead of a path's geometry.
    static
    void emit_paint_setup(
        std::string&     _s,
        const DPdfPaint& _paint
    )
    {
        if (_paint.do_fill)
        {
            _s += internal::pdf_set_color(_paint.fill, true);
        }

        if (_paint.do_stroke)
        {
            _s += internal::pdf_num(_paint.line_width) + " w\n";
            _s += internal::pdf_set_color(_paint.stroke, false);
        }

        return;
    }

    // emit_path_geometry
    //   writes the path-construction operators (m / l / c / h) for a
    // DPdfPath's segments.
    static
    void emit_path_geometry(
        std::string&    _s,
        const DPdfPath& _path
    )
    {
        const std::vector<DPdfPathSegment>& segs = _path.segments();

        for (std::size_t i = 0; i < segs.size(); ++i)
        {
            const DPdfPathSegment& seg = segs[i];

            if (seg.verb == DPdfPathVerb::move_to)
            {
                _s += internal::pdf_num(seg.p0.x) + " " +
                      internal::pdf_num(seg.p0.y) + " m\n";

                continue;
            }

            if (seg.verb == DPdfPathVerb::line_to)
            {
                _s += internal::pdf_num(seg.p0.x) + " " +
                      internal::pdf_num(seg.p0.y) + " l\n";

                continue;
            }

            if (seg.verb == DPdfPathVerb::curve_to)
            {
                _s += internal::pdf_num(seg.p0.x) + " " +
                      internal::pdf_num(seg.p0.y) + " " +
                      internal::pdf_num(seg.p1.x) + " " +
                      internal::pdf_num(seg.p1.y) + " " +
                      internal::pdf_num(seg.p2.x) + " " +
                      internal::pdf_num(seg.p2.y) + " c\n";

                continue;
            }

            // close
            _s += "h\n";
        }

        return;
    }

    // paint_suffix
    //   returns the path-painting operator implied by a paint:
    // B (fill+stroke), f (fill), S (stroke), or n (no-op).
    static
    std::string paint_suffix(
        const DPdfPaint& _paint
    )
    {
        if ( (_paint.do_fill) &&
             (_paint.do_stroke) )
        {
            return " B\n";
        }

        if (_paint.do_fill)
        {
            return " f\n";
        }

        if (_paint.do_stroke)
        {
            return " S\n";
        }

        return " n\n";
    }

    // compressing
    //   function: true when this document's streams are written with
    // FlateDecode: the setting asks for it, or leaves it to the knob
    // and the knob does, and the build has zlib.
    bool compressing() const
    {
        const bool wanted =
            (m_compression == DPdfCompression::flate) ||
            ( (m_compression == DPdfCompression::by_default) &&
              (D_INTERNAL_PDF_COMPRESS != 0) );

        return wanted && (d_codec_can_compress(D_CODEC_ID_ZLIB) != 0);
    }

    // image_samples
    //   returns an image's samples as the stream holds them, and sets
    // _colors and _space to match.  Gray and RGB embed directly; RGBA
    // is composited over white into RGB (the built-in backend does not
    // emit soft masks - a richer backend can override draw_image for
    // true alpha).
    static
    std::string image_samples(
        const DPdfImage& _img,
        std::size_t&     _colors,
        const char*&     _space
    )
    {
        std::string data;
        std::size_t px = _img.width * _img.height;

        _colors = 3;
        _space  = "/DeviceRGB";

        if (_img.format == DPdfImageFormat::gray)
        {
            _colors = 1;
            _space  = "/DeviceGray";
            data.assign(
                reinterpret_cast<const char*>(_img.samples.data()), px);

            return data;
        }

        if (_img.format == DPdfImageFormat::rgb)
        {
            data.assign(
                reinterpret_cast<const char*>(_img.samples.data()),
                px * 3);

            return data;
        }

        data.reserve(px * 3);

        for (std::size_t i = 0; i < px; ++i)
        {
            unsigned r = _img.samples[(i * 4) + 0];
            unsigned g = _img.samples[(i * 4) + 1];
            unsigned b = _img.samples[(i * 4) + 2];
            unsigned a = _img.samples[(i * 4) + 3];

            // out = src*a + white*(1-a), all in 0..255
            unsigned cr = (r * a + 255 * (255 - a)) / 255;
            unsigned cg = (g * a + 255 * (255 - a)) / 255;
            unsigned cb = (b * a + 255 * (255 - a)) / 255;

            data.push_back(static_cast<char>(cr & 0xff));
            data.push_back(static_cast<char>(cg & 0xff));
            data.push_back(static_cast<char>(cb & 0xff));
        }

        return data;
    }

    // emit_image_object
    //   writes one image XObject.  With _flate its samples go through
    // PNG's predictors, a filter chosen per row, then Flate, and the
    // dictionary says how to undo both; without it, or if Flate fails,
    // the samples are written raw.
    static
    void emit_image_object(
        std::string&              _out,
        std::vector<std::size_t>& _offsets,
        int                       _obj_num,
        const DPdfImage&          _img,
        bool                      _flate
    )
    {
        char        buf[160];
        std::size_t colors = 3;
        const char* cs     = "/DeviceRGB";
        std::string data   = image_samples(_img, colors, cs);
        std::string packed;

        _flate = _flate &&
                 internal::pdf_deflate(
                     internal::pdf_png_predict(data, _img.width,
                                               _img.height, colors),
                     D_INTERNAL_PDF_COMPRESS_LEVEL, packed);

        internal::pdf_begin_object(_out, _offsets, _obj_num);
        _out += "<< /Type /XObject /Subtype /Image";
        std::snprintf(buf, sizeof(buf),
                      " /Width %zu /Height %zu",
                      _img.width, _img.height);
        _out += buf;
        _out += " /ColorSpace ";
        _out += cs;
        _out += " /BitsPerComponent 8";

        if (_flate)
        {
            std::snprintf(buf, sizeof(buf),
                          " /Filter /FlateDecode /DecodeParms << /Predictor"
                          " 15 /Colors %zu /BitsPerComponent 8 /Columns %zu"
                          " >>",
                          colors, _img.width);
            _out += buf;
            data.swap(packed);
        }

        _out += " /Length ";
        std::snprintf(buf, sizeof(buf), "%zu", data.size());
        _out += buf;
        _out += " >>\n";
        _out += "stream\n";
        _out += data;
        _out += "\nendstream\n";
        _out += "endobj\n";

        return;
    }

    // emit_content_object
    //   writes a page's content stream object.  With _flate it is
    // compressed, and ends with an end-of-line before endstream, as
    // compressed bytes need; raw, it is written as it always was.
    static
    void emit_content_object(
        std::string&              _out,
        std::vector<std::size_t>& _offsets,
        int                       _obj_num,
        const std::string&        _content,
        bool                      _flate
    )
    {
        char        buf[64];
        std::string packed;

        _flate = _flate &&
                 internal::pdf_deflate(_content,
                                       D_INTERNAL_PDF_COMPRESS_LEVEL, packed);

        internal::pdf_begin_object(_out, _offsets, _obj_num);
        _out += "<< /Length ";
        std::snprintf(buf, sizeof(buf), "%zu",
                      _flate ? packed.size() : _content.size());
        _out += buf;
        _out += _flate ? " /Filter /FlateDecode >>\n" : " >>\n";
        _out += "stream\n";
        _out += _flate ? packed : _content;
        _out += _flate ? "\nendstream\n" : "endstream\n";
        _out += "endobj\n";

        return;
    }


    // =================================================================
    //  internal: document assembly
    // =================================================================

    // DLayout
    //   struct: a document's object numbering and its shared resource
    // dictionaries.  Object layout: 1 Catalog, 2 Pages, 3 Info, then F
    // font objects, then I image XObjects, then a (page-dict, content)
    // pair per page.
    struct DLayout
    {
        std::size_t font_count;
        std::size_t image_count;
        std::size_t page_count;
        bool        synth_page;
        int         first_font_obj;
        int         first_image_obj;
        int         first_page_obj;
        std::size_t total_objs;
        std::string res_fonts;
        std::string res_xobjects;
    };

    // plan_layout
    //   numbers the objects, and builds the Font and XObject resource
    // sub-dictionaries every page shares; image names match the /ImN
    // the content streams use (N is the global image index).  A
    // document with no pages gets one blank letter page.
    DLayout plan_layout(
        const std::vector<std::string>& _fonts
    ) const
    {
        DLayout layout;
        char    buf[64];

        layout.font_count      = _fonts.size();
        layout.image_count     = m_images.size();
        layout.synth_page      = m_pages.empty();
        layout.page_count      = layout.synth_page ? 1 : m_pages.size();
        layout.first_font_obj  = 4;
        layout.first_image_obj = static_cast<int>(4 + layout.font_count);
        layout.first_page_obj  =
            static_cast<int>(4 + layout.font_count + layout.image_count);
        layout.total_objs = 3 + layout.font_count + layout.image_count +
                            (2 * layout.page_count);

        for (std::size_t f = 0; f < layout.font_count; ++f)
        {
            std::snprintf(buf, sizeof(buf), "/F%zu %d 0 R ", (f + 1),
                          layout.first_font_obj + static_cast<int>(f));
            layout.res_fonts += buf;
        }

        for (std::size_t im = 0; im < layout.image_count; ++im)
        {
            std::snprintf(buf, sizeof(buf), "/Im%zu %d 0 R ", im,
                          layout.first_image_obj + static_cast<int>(im));
            layout.res_xobjects += buf;
        }

        return layout;
    }

    // emit_catalog_and_tree
    //   writes object 1, the catalog, and object 2, the page tree with a
    // kid for each page dictionary.
    static
    void emit_catalog_and_tree(
        std::string&              _out,
        std::vector<std::size_t>& _offsets,
        const DLayout&            _layout
    )
    {
        std::string kids;
        char        buf[64];

        internal::pdf_begin_object(_out, _offsets, 1);
        _out += "<< /Type /Catalog /Pages 2 0 R >>\n";
        _out += "endobj\n";

        for (std::size_t p = 0; p < _layout.page_count; ++p)
        {
            std::snprintf(buf, sizeof(buf), "%d 0 R ",
                          _layout.first_page_obj + static_cast<int>(2 * p));
            kids += buf;
        }

        internal::pdf_begin_object(_out, _offsets, 2);
        _out += "<< /Type /Pages /Kids [";
        _out += kids;
        _out += "] /Count ";
        std::snprintf(buf, sizeof(buf), "%zu", _layout.page_count);
        _out += buf;
        _out += " >>\n";
        _out += "endobj\n";

        return;
    }

    // emit_info
    //   writes object 3, the document information dictionary: the
    // producer, each recognized metadata key with its PDF name (an
    // unknown key is skipped, to keep the dictionary well-formed), and
    // the creation date.
    void emit_info(
        std::string&              _out,
        std::vector<std::size_t>& _offsets
    ) const
    {
        internal::pdf_begin_object(_out, _offsets, 3);
        _out += "<< /Producer (djinterp pdf)";

        for (std::size_t i = 0; i < m_meta.size(); ++i)
        {
            std::string pdfkey = info_key_name(m_meta[i].key);

            if (pdfkey.empty())
            {
                continue;
            }

            _out += " /" + pdfkey + " (" +
                    internal::pdf_escape_text(m_meta[i].value) + ")";
        }

        _out += " /CreationDate (" +
                internal::pdf_creation_date() + ") >>\n";
        _out += "endobj\n";

        return;
    }

    // emit_fonts
    //   writes a Type1 font object for each collected base font.
    static
    void emit_fonts(
        std::string&                    _out,
        std::vector<std::size_t>&       _offsets,
        const DLayout&                  _layout,
        const std::vector<std::string>& _fonts
    )
    {
        for (std::size_t f = 0; f < _layout.font_count; ++f)
        {
            internal::pdf_begin_object(
                _out, _offsets, _layout.first_font_obj + static_cast<int>(f));
            _out += "<< /Type /Font /Subtype /Type1 /BaseFont /";
            _out += _fonts[f];
            _out += " /Encoding /WinAnsiEncoding >>\n";
            _out += "endobj\n";
        }

        return;
    }

    // emit_page
    //   writes page _p's dictionary, sharing the layout's resources, and
    // its content stream object after it.
    void emit_page(
        std::string&                    _out,
        std::vector<std::size_t>&       _offsets,
        const DLayout&                  _layout,
        const std::vector<std::string>& _fonts,
        std::size_t                     _p,
        bool                            _flate
    ) const
    {
        const int   page_obj    =
            _layout.first_page_obj + static_cast<int>(2 * _p);
        const int   content_obj = page_obj + 1;
        DPdfSize    sz          = _layout.synth_page
                                      ? DPdfSize(612.0, 792.0)
                                      : m_pages[_p].size;
        std::string content;
        char        buf[64];

        if (!_layout.synth_page)
        {
            content = build_content_stream(m_pages[_p], _fonts);
        }

        internal::pdf_begin_object(_out, _offsets, page_obj);
        _out += "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ";
        _out += internal::pdf_num(sz.width) + " " +
                internal::pdf_num(sz.height);
        _out += "] /Resources << /Font << ";
        _out += _layout.res_fonts;
        _out += ">>";

        if (!_layout.res_xobjects.empty())
        {
            _out += " /XObject << ";
            _out += _layout.res_xobjects;
            _out += ">>";
        }

        _out += " >> /Contents ";
        std::snprintf(buf, sizeof(buf), "%d 0 R", content_obj);
        _out += buf;
        _out += " >>\n";
        _out += "endobj\n";

        emit_content_object(_out, _offsets, content_obj, content, _flate);

        return;
    }

    // emit_xref_and_trailer
    //   writes the cross-reference table from the recorded offsets, the
    // trailer, and startxref.
    static
    void emit_xref_and_trailer(
        std::string&                    _out,
        const std::vector<std::size_t>& _offsets,
        std::size_t                     _total_objs
    )
    {
        const std::size_t xref_offset = _out.size();
        char              buf[64];

        _out += "xref\n";
        std::snprintf(buf, sizeof(buf), "0 %zu\n", (_total_objs + 1));
        _out += buf;
        _out += "0000000000 65535 f \n";

        for (std::size_t n = 1; n <= _total_objs; ++n)
        {
            std::snprintf(buf, sizeof(buf), "%010zu 00000 n \n",
                          _offsets[n]);
            _out += buf;
        }

        _out += "trailer\n";
        _out += "<< /Size ";
        std::snprintf(buf, sizeof(buf), "%zu", (_total_objs + 1));
        _out += buf;
        _out += " /Root 1 0 R /Info 3 0 R >>\n";
        _out += "startxref\n";
        std::snprintf(buf, sizeof(buf), "%zu\n", xref_offset);
        _out += buf;
        _out += "%%EOF\n";

        return;
    }

    // build_document
    //   assembles the full PDF: the header with its binary marker, the
    // catalog and page tree, the information dictionary, the fonts,
    // the images, each page with its content, then the cross-reference
    // table that the recorded byte offsets feed.
    std::string build_document() const
    {
        const std::vector<std::string> fonts  = collect_fonts();
        const DLayout                  layout = plan_layout(fonts);
        const bool                     flate  = compressing();
        std::vector<std::size_t>       offsets(layout.total_objs + 1, 0);
        std::string                    out;

        out += "%PDF-1.4\n";
        out += "%\xE2\xE3\xCF\xD3\n";

        emit_catalog_and_tree(out, offsets, layout);
        emit_info(out, offsets);
        emit_fonts(out, offsets, layout, fonts);

        for (std::size_t im = 0; im < layout.image_count; ++im)
        {
            emit_image_object(out, offsets,
                              layout.first_image_obj + static_cast<int>(im),
                              m_images[im], flate);
        }

        for (std::size_t p = 0; p < layout.page_count; ++p)
        {
            emit_page(out, offsets, layout, fonts, p, flate);
        }

        emit_xref_and_trailer(out, offsets, layout.total_objs);

        return out;
    }

    // info_key_name
    //   maps a friendly metadata key to its PDF Info dictionary
    // name, or "" if the key is not a recognized Info field.
    static
    std::string info_key_name(
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

    std::vector<DPageRecord> m_pages;
    std::vector<DMetaEntry>  m_meta;
    std::vector<DPdfImage>   m_images;
    bool                     m_open;
    bool                     m_page_open;
    DPdfCompression          m_compression;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_PDF_PDF_BUILTIN_BACKEND_HPP
