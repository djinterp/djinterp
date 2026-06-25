#ifndef DJINTERP_PDF_BUILTIN_BACKEND_
#define DJINTERP_PDF_BUILTIN_BACKEND_

///////////////////////////////////////////////////////////////////////////////
// pdf_builtin_backend.hpp
//
// The built-in, dependency-free PDF backend together with the internal
// content-stream serialization primitives it relies on.  Emits the
// color operators (rg/RG, k/K, g/G) and base-14 font names.  Depends on
// the backend protocol in pdf_backend.hpp.
///////////////////////////////////////////////////////////////////////////////

#include "pdf_backend.hpp"

NS_DJINTERP
NS_PDF
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

    // pdf_set_color
    //   helper: the content-stream operator(s) selecting _color as the
    // fill (lower-case) or stroke (upper-case) color, emitted in the
    // model's native PDF device space (DeviceGray / RGB / CMYK).
    inline std::string
    pdf_set_color(
        const pdf_color& _color,
        bool             _fill
    )
    {
        switch (_color.space)
        {
        case pdf_color_space::gray:
            return pdf_num(_color.r) + (_fill ? " g\n" : " G\n");

        case pdf_color_space::cmyk:
            return pdf_num(_color.c) + " " + pdf_num(_color.m) + " " +
                   pdf_num(_color.y) + " " + pdf_num(_color.k) +
                   (_fill ? " k\n" : " K\n");

        case pdf_color_space::rgb:
        default:
            return pdf_num(_color.r) + " " + pdf_num(_color.g) + " " +
                   pdf_num(_color.b) + (_fill ? " rg\n" : " RG\n");
        }
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
// covers the common subset (text, lines, rectangles), vector paths,
// graphics state, raster images (raw RGB / RGBA / gray, embedded
// uncompressed), and metadata; it reports no font embedding,
// outline, annotation, encryption, or compression support.
class builtin_pdf_backend : public pdf_backend
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

        // path
        pdf_path path;

        // image: index into the backend's image table, and the
        // destination rectangle the image is scaled into
        std::size_t image_index;
        pdf_rect    image_dest;

        // shared paint (line / rect / path)
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
              path(),
              image_index(0),
              image_dest(),
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
          m_images(),
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
        m_images.clear();
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

    void
    draw_path(
        const pdf_path&  _path,
        const pdf_paint& _paint
    ) D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        draw_op op;
        op.kind  = op_kind::path;
        op.path  = _path;
        op.paint = _paint;

        m_pages.back().ops.push_back(
            static_cast<draw_op&&>(op));

        return;
    }

    void
    save_state() D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        draw_op op;
        op.kind = op_kind::save_state;

        m_pages.back().ops.push_back(
            static_cast<draw_op&&>(op));

        return;
    }

    void
    restore_state() D_OVERRIDE
    {
        if (m_pages.empty())
        {
            return;
        }

        draw_op op;
        op.kind = op_kind::restore_state;

        m_pages.back().ops.push_back(
            static_cast<draw_op&&>(op));

        return;
    }

    void
    draw_image(
        const pdf_image& _image,
        const pdf_rect&  _dest
    ) D_OVERRIDE
    {
        if ( (m_pages.empty()) ||
             (!_image.valid()) )
        {
            return;
        }

        // store the image and record an op referencing it by index
        m_images.push_back(_image);

        draw_op op;
        op.kind        = op_kind::image;
        op.image_index = (m_images.size() - 1);
        op.image_dest  = _dest;

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
        caps.images          = true;
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
                s += internal::pdf_set_color(op.text_color, true);
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
                s += internal::pdf_set_color(op.paint.stroke, false);
                s += internal::pdf_num(op.a.x) + " " +
                     internal::pdf_num(op.a.y) + " m " +
                     internal::pdf_num(op.b.x) + " " +
                     internal::pdf_num(op.b.y) + " l S\n";

                continue;
            }

            if (op.kind == op_kind::save_state)
            {
                s += "q\n";

                continue;
            }

            if (op.kind == op_kind::restore_state)
            {
                s += "Q\n";

                continue;
            }

            if (op.kind == op_kind::path)
            {
                emit_paint_setup(s, op.paint);
                emit_path_geometry(s, op.path);
                s += paint_suffix(op.paint);

                continue;
            }

            if (op.kind == op_kind::image)
            {
                // place the image by mapping the unit square to the
                // destination rectangle via the cm matrix, then
                // invoking the XObject; q/Q isolates the transform
                const pdf_rect& d = op.image_dest;

                s += "q\n";
                s += internal::pdf_num(d.width) + " 0 0 " +
                     internal::pdf_num(d.height) + " " +
                     internal::pdf_num(d.x) + " " +
                     internal::pdf_num(d.y) + " cm\n";
                s += "/Im" +
                     internal::pdf_num(
                         static_cast<double>(op.image_index)) +
                     " Do\n";
                s += "Q\n";

                continue;
            }

            // rectangle
            if (op.paint.do_fill)
            {
                s += internal::pdf_set_color(op.paint.fill, true);
            }

            if (op.paint.do_stroke)
            {
                s += internal::pdf_num(op.paint.line_width) + " w\n";
                s += internal::pdf_set_color(op.paint.stroke, false);
            }

            s += internal::pdf_num(op.rect.x) + " " +
                 internal::pdf_num(op.rect.y) + " " +
                 internal::pdf_num(op.rect.width) + " " +
                 internal::pdf_num(op.rect.height) + " re";

            s += paint_suffix(op.paint);
        }

        return s;
    }

    // emit_paint_setup
    //   writes the fill-color, stroke-color, and line-width state
    // operators implied by a paint, ahead of a path's geometry.
    static void
    emit_paint_setup(
        std::string&     _s,
        const pdf_paint& _paint
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
    // pdf_path's segments.
    static void
    emit_path_geometry(
        std::string&    _s,
        const pdf_path& _path
    )
    {
        const std::vector<pdf_path_segment>& segs = _path.segments();

        for (std::size_t i = 0; i < segs.size(); ++i)
        {
            const pdf_path_segment& seg = segs[i];

            if (seg.verb == pdf_path_verb::move_to)
            {
                _s += internal::pdf_num(seg.p0.x) + " " +
                      internal::pdf_num(seg.p0.y) + " m\n";

                continue;
            }

            if (seg.verb == pdf_path_verb::line_to)
            {
                _s += internal::pdf_num(seg.p0.x) + " " +
                      internal::pdf_num(seg.p0.y) + " l\n";

                continue;
            }

            if (seg.verb == pdf_path_verb::curve_to)
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
    static std::string
    paint_suffix(
        const pdf_paint& _paint
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

    // emit_image_object
    //   writes one image XObject as an uncompressed raw sample
    // stream.  Gray and RGB images embed directly; RGBA is composited
    // over white into RGB (the built-in backend does not emit soft
    // masks - a richer backend can override draw_image for true
    // alpha).  The stream is raw binary, written verbatim.
    static void
    emit_image_object(
        std::string&              _out,
        std::vector<std::size_t>& _offsets,
        int                       _obj_num,
        const pdf_image&          _img
    )
    {
        char buf[128];

        // resolve color space and assemble the raw sample payload
        std::string         data;
        const char*         cs = "/DeviceRGB";
        std::size_t         px = _img.width * _img.height;

        if (_img.format == pdf_image_format::gray)
        {
            cs = "/DeviceGray";
            data.assign(
                reinterpret_cast<const char*>(_img.samples.data()),
                px);
        }
        else if (_img.format == pdf_image_format::rgb)
        {
            data.assign(
                reinterpret_cast<const char*>(_img.samples.data()),
                px * 3);
        }
        else
        {
            // RGBA: composite each pixel over a white background and
            // store as RGB, since this backend emits no soft mask
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
        }

        internal::pdf_begin_object(_out, _offsets, _obj_num);
        _out += "<< /Type /XObject /Subtype /Image";
        std::snprintf(buf, sizeof(buf),
                      " /Width %zu /Height %zu",
                      _img.width, _img.height);
        _out += buf;
        _out += " /ColorSpace ";
        _out += cs;
        _out += " /BitsPerComponent 8 /Length ";
        std::snprintf(buf, sizeof(buf), "%zu", data.size());
        _out += buf;
        _out += " >>\n";
        _out += "stream\n";
        _out += data;
        _out += "\nendstream\n";
        _out += "endobj\n";

        return;
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

        // object numbering: 1 Catalog, 2 Pages, 3 Info, then F font
        // objects, then I image XObjects, then a (page-dict, content)
        // pair per page
        std::size_t image_count   = m_images.size();
        int   info_obj            = 3;
        int   first_font_obj      = 4;
        int   first_image_obj     =
            static_cast<int>(4 + font_count);
        int   first_page_obj      =
            static_cast<int>(4 + font_count + image_count);
        std::size_t total_objs =
            3 + font_count + image_count + (2 * page_count);

        std::string              out;
        std::string              kids;
        std::string              res_fonts;
        std::string              res_xobjects;
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

        // build the shared XObject resource sub-dictionary; image
        // names match the /ImN emitted in the content stream (N is
        // the global image index)
        for (std::size_t im = 0; im < image_count; ++im)
        {
            int iobj = first_image_obj + static_cast<int>(im);

            std::snprintf(buf, sizeof(buf),
                          "/Im%zu %d 0 R ", im, iobj);
            res_xobjects += buf;
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

        // image XObject objects (uncompressed raw samples)
        for (std::size_t im = 0; im < image_count; ++im)
        {
            int iobj = first_image_obj + static_cast<int>(im);

            emit_image_object(out, offsets, iobj, m_images[im]);
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
            out += ">>";
            if (!res_xobjects.empty())
            {
                out += " /XObject << ";
                out += res_xobjects;
                out += ">>";
            }
            out += " >> /Contents ";
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
    std::vector<pdf_image>   m_images;
    bool                     m_open;
    bool                     m_page_open;
};


NS_END  // pdf
NS_END  // djinterp

#endif  // DJINTERP_PDF_BUILTIN_BACKEND_
