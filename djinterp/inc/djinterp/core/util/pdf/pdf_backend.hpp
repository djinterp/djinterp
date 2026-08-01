#ifndef DJINTERP_UTIL_PDF_BACKEND_
#define DJINTERP_UTIL_PDF_BACKEND_

///////////////////////////////////////////////////////////////////////////////
// pdf_backend.hpp
//
// Backend capabilities and the abstract pdf_backend protocol - the
// runtime-virtual interface a PDF backend implements.  Depends only on
// the drawing vocabulary in pdf_primitives.hpp.
///////////////////////////////////////////////////////////////////////////////

#include "../../env/env_pdf.h"   // external PDF library detection + preferred-backend hint
#include "./pdf_primitives.hpp"

NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                VII. CAPABILITIES                                         ///
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
///                VII-b. BACKEND TAXONOMY (env_pdf.h reflection)             ///
///////////////////////////////////////////////////////////////////////////////

// pdf_backend_kind
//   enum: identifies a PDF generation backend.  Values mirror the
// D_ENV_PDF_BACKEND_* selector constants from env_pdf.h, so a detected
// or preferred backend round-trips between the macro layer and this
// enum.  `builtin` (== D_ENV_PDF_BACKEND_NONE) is the always-available
// djinterp serializer used when no external generation library exists.
enum class pdf_backend_kind : unsigned
{
    builtin   = D_ENV_PDF_BACKEND_NONE,
    libharu   = D_ENV_PDF_BACKEND_LIBHARU,
    pdfhummus = D_ENV_PDF_BACKEND_PDFHUMMUS,
    podofo    = D_ENV_PDF_BACKEND_PODOFO,
    cairo     = D_ENV_PDF_BACKEND_CAIRO
};


// pdf_env
//   namespace: a C++ surface over env_pdf.h's compile-time detection,
// lifting the D_ENV_PDF_* macros into djinterp::pdf so backend
// selection can be expressed in ordinary constexpr C++ rather than the
// preprocessor.
namespace pdf_env
{
    // generation libraries
    D_STATIC_CONSTEXPR bool has_libharu   = ( D_ENV_PDF_HAS_LIBHARU   != 0 );
    D_STATIC_CONSTEXPR bool has_pdfhummus = ( D_ENV_PDF_HAS_PDFHUMMUS != 0 );
    D_STATIC_CONSTEXPR bool has_podofo    = ( D_ENV_PDF_HAS_PODOFO    != 0 );
    D_STATIC_CONSTEXPR bool has_cairo_pdf = ( D_ENV_PDF_HAS_CAIRO_PDF != 0 );

    // render / parse libraries
    D_STATIC_CONSTEXPR bool has_poppler   = ( D_ENV_PDF_HAS_POPPLER   != 0 );
    D_STATIC_CONSTEXPR bool has_mupdf     = ( D_ENV_PDF_HAS_MUPDF     != 0 );
    D_STATIC_CONSTEXPR bool has_pdfium    = ( D_ENV_PDF_HAS_PDFIUM    != 0 );

    // aggregates
    D_STATIC_CONSTEXPR bool has_generation_lib   = ( D_ENV_PDF_HAS_GENERATION_LIB != 0 );
    D_STATIC_CONSTEXPR bool has_render_lib       = ( D_ENV_PDF_HAS_RENDER_LIB     != 0 );
    D_STATIC_CONSTEXPR bool has_any_lib          = ( D_ENV_PDF_HAS_ANY_LIB        != 0 );
    D_STATIC_CONSTEXPR int  generation_lib_count = D_ENV_PDF_GENERATION_LIB_COUNT;

    // preferred generation backend (a hint); `builtin` when none detected
    D_STATIC_CONSTEXPR pdf_backend_kind preferred =
        static_cast<pdf_backend_kind>(D_ENV_PDF_PREFERRED_BACKEND);

    // preferred_name
    //   function: human-readable name of the preferred backend.
    inline const char* preferred_name()
    {
        return D_ENV_PDF_PREFERRED_BACKEND_NAME;
    }
}   // namespace pdf_env


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

    // -----------------------------------------------------------------
    //  optional richer surface (virtual with fallbacks)
    // -----------------------------------------------------------------
    // These extend the pure-virtual common subset above with vector
    // paths and graphics-state nesting.  They are NOT pure virtual so
    // that a minimal backend still satisfies the protocol; the base
    // provides reasonable fallbacks (path flattening to line strokes,
    // no-op state save/restore).  A capable backend overrides them for
    // native curves and proper graphics-state handling.

    // draw_path
    //   draws a vector path.  The default flattens the path to its
    // straight segments and strokes them via draw_line, dropping
    // fills and approximating curves by their endpoints - enough to
    // remain visible on a backend that does not implement paths
    // natively.  Override for accurate rendering.
    virtual void
    draw_path(
        const pdf_path&  _path,
        const pdf_paint& _paint)
    {
        pdf_point cur;
        pdf_point start;
        bool      have_cur = false;

        const std::vector<pdf_path_segment>& segs = _path.segments();

        // replay each segment as a straight stroke from the current
        // point; curves degrade to a line to their endpoint
        for (std::size_t i = 0; i < segs.size(); ++i)
        {
            const pdf_path_segment& s = segs[i];

            if (s.verb == pdf_path_verb::move_to)
            {
                cur      = s.p0;
                start    = s.p0;
                have_cur = true;

                continue;
            }

            if (s.verb == pdf_path_verb::line_to)
            {
                if (have_cur)
                {
                    draw_line(cur, s.p0, _paint);
                }

                cur = s.p0;

                continue;
            }

            if (s.verb == pdf_path_verb::curve_to)
            {
                if (have_cur)
                {
                    draw_line(cur, s.p2, _paint);
                }

                cur = s.p2;

                continue;
            }

            // close
            if (have_cur)
            {
                draw_line(cur, start, _paint);
                cur = start;
            }
        }

        return;
    }

    // save_state / restore_state
    //   push and pop the graphics state (q / Q).  Default no-ops; a
    // capable backend overrides to nest clipping, transforms, and
    // paint state.
    virtual void save_state()    {}
    virtual void restore_state() {}

    // draw_image
    //   draws a raster image scaled into the destination rectangle
    // _dest (in user space).  Default no-op so a backend without
    // raster support still conforms; query capabilities().images
    // before relying on it.  Override to embed image XObjects.
    virtual void
    draw_image(
        const pdf_image& _image,
        const pdf_rect&  _dest)
    {
        (void)_image;
        (void)_dest;
    }

    // introspection
    virtual pdf_capabilities capabilities() const = 0;

    // kind
    //   function: which backend implementation this is.  Non-pure
    // so existing/adapter backends need not change; defaults to the
    // built-in serializer.  Mirrors env_pdf.h's backend taxonomy.
    virtual pdf_backend_kind kind() const { return pdf_backend_kind::builtin; }

    // output
    virtual std::string serialize()              = 0;
    virtual bool        save(const char* _path)  = 0;
};


NS_INTERNAL

    // The adapter forwards the OPTIONAL backend surface (paths,
    // graphics state, images) only when the wrapped type actually
    // provides each method; otherwise it falls back to the
    // pdf_backend base default.  These detectors enable that
    // tag-dispatch without requiring the wrapped type to implement
    // the whole optional surface (the common subset is enough to be
    // a backend).  Kept here, in the foundation, so pdf.hpp stays
    // self-sufficient and does not depend on the traits header.

    // adapter_has_draw_path
    template<typename _Type, typename = void>
    struct adapter_has_draw_path : std::false_type {};

    template<typename _Type>
    struct adapter_has_draw_path<_Type, void_t<
        decltype(std::declval<_Type&>().draw_path(
            std::declval<const pdf_path&>(),
            std::declval<const pdf_paint&>()))
    >> : std::true_type {};

    // adapter_has_save_state
    template<typename _Type, typename = void>
    struct adapter_has_save_state : std::false_type {};

    template<typename _Type>
    struct adapter_has_save_state<_Type, void_t<
        decltype(std::declval<_Type&>().save_state())
    >> : std::true_type {};

    // adapter_has_restore_state
    template<typename _Type, typename = void>
    struct adapter_has_restore_state : std::false_type {};

    template<typename _Type>
    struct adapter_has_restore_state<_Type, void_t<
        decltype(std::declval<_Type&>().restore_state())
    >> : std::true_type {};

    // adapter_has_draw_image
    template<typename _Type, typename = void>
    struct adapter_has_draw_image : std::false_type {};

    template<typename _Type>
    struct adapter_has_draw_image<_Type, void_t<
        decltype(std::declval<_Type&>().draw_image(
            std::declval<const pdf_image&>(),
            std::declval<const pdf_rect&>()))
    >> : std::true_type {};

NS_END  // internal


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

    // The optional surface (paths, graphics state, images) is
    // forwarded only when the wrapped backend actually provides the
    // method; otherwise the pdf_backend base default is used.  This
    // lets a common-subset-only backend be adapted without having to
    // implement the whole optional surface.  Dispatch is by tag on
    // the internal::adapter_has_* detectors.

    void
    draw_path(
        const pdf_path&  _path,
        const pdf_paint& _paint
    ) D_OVERRIDE
    {
        forward_draw_path(
            _path, _paint,
            internal::adapter_has_draw_path<_Backend>());

        return;
    }

    void
    save_state() D_OVERRIDE
    {
        forward_save_state(
            internal::adapter_has_save_state<_Backend>());

        return;
    }

    void
    restore_state() D_OVERRIDE
    {
        forward_restore_state(
            internal::adapter_has_restore_state<_Backend>());

        return;
    }

    void
    draw_image(
        const pdf_image& _image,
        const pdf_rect&  _dest
    ) D_OVERRIDE
    {
        forward_draw_image(
            _image, _dest,
            internal::adapter_has_draw_image<_Backend>());

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
    // ---- optional-surface tag dispatch ----
    // present: forward to the wrapped backend.
    // absent:  defer to the pdf_backend base implementation (path
    //          flattening to line strokes, no-op state, no-op image).

    void
    forward_draw_path(
        const pdf_path&  _path,
        const pdf_paint& _paint,
        std::true_type
    )
    {
        m_backend->draw_path(_path, _paint);

        return;
    }

    void
    forward_draw_path(
        const pdf_path&  _path,
        const pdf_paint& _paint,
        std::false_type
    )
    {
        pdf_backend::draw_path(_path, _paint);

        return;
    }

    void
    forward_save_state(
        std::true_type
    )
    {
        m_backend->save_state();

        return;
    }

    void
    forward_save_state(
        std::false_type
    )
    {
        pdf_backend::save_state();

        return;
    }

    void
    forward_restore_state(
        std::true_type
    )
    {
        m_backend->restore_state();

        return;
    }

    void
    forward_restore_state(
        std::false_type
    )
    {
        pdf_backend::restore_state();

        return;
    }

    void
    forward_draw_image(
        const pdf_image& _image,
        const pdf_rect&  _dest,
        std::true_type
    )
    {
        m_backend->draw_image(_image, _dest);

        return;
    }

    void
    forward_draw_image(
        const pdf_image& _image,
        const pdf_rect&  _dest,
        std::false_type
    )
    {
        pdf_backend::draw_image(_image, _dest);

        return;
    }

    _Backend* m_backend;
};


NS_END  // djinterp

#endif  // DJINTERP_UTIL_PDF_BACKEND_
