#ifndef DJINTERP_TEXT_PDF_DOCUMENT_
#define DJINTERP_TEXT_PDF_DOCUMENT_

///////////////////////////////////////////////////////////////////////////////
// pdf_document.hpp
//
// The high-level pdf_document facade: open/close, add_page, text and
// drawing calls, and save.  Defaults to the built-in backend, so this
// header pulls in the whole module via pdf_builtin_backend.hpp.
///////////////////////////////////////////////////////////////////////////////

#include "./pdf_builtin_backend.hpp"


NS_DJINTERP

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

    // path
    //   draws an arbitrary vector path under the given paint.
    void
    path(
        const pdf_path&  _path,
        const pdf_paint& _paint = pdf_paint()
    )
    {
        m_backend->draw_path(_path, _paint);

        return;
    }

    // circle
    void
    circle(
        const pdf_point& _center,
        pdf_unit         _radius,
        const pdf_paint& _paint = pdf_paint()
    )
    {
        m_backend->draw_path(
            pdf_path::circle(_center, _radius), _paint);

        return;
    }

    // ellipse
    void
    ellipse(
        const pdf_point& _center,
        pdf_unit         _rx,
        pdf_unit         _ry,
        const pdf_paint& _paint = pdf_paint()
    )
    {
        m_backend->draw_path(
            pdf_path::ellipse(_center, _rx, _ry), _paint);

        return;
    }

    // rounded_rect
    void
    rounded_rect(
        const pdf_rect&  _rect,
        pdf_unit         _radius,
        const pdf_paint& _paint = pdf_paint()
    )
    {
        m_backend->draw_path(
            pdf_path::rounded_rect(_rect, _radius), _paint);

        return;
    }

    // polygon
    void
    polygon(
        const std::vector<pdf_point>& _points,
        const pdf_paint&              _paint = pdf_paint()
    )
    {
        m_backend->draw_path(pdf_path::polygon(_points), _paint);

        return;
    }

    // polyline
    void
    polyline(
        const std::vector<pdf_point>& _points,
        const pdf_paint&              _paint = pdf_paint()
    )
    {
        m_backend->draw_path(pdf_path::polyline(_points), _paint);

        return;
    }

    // save_state / restore_state
    //   push and pop graphics state on the backend.
    void save_state()    { m_backend->save_state();    return; }
    void restore_state() { m_backend->restore_state(); return; }

    // image
    //   draws a raster image scaled into the destination rectangle.
    // Has no effect on a backend whose capabilities().images is
    // false.
    void
    image(
        const pdf_image& _image,
        const pdf_rect&  _dest
    )
    {
        m_backend->draw_image(_image, _dest);

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


NS_END  // djinterp


#endif  // DJINTERP_TEXT_PDF_DOCUMENT_