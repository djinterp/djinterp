/*******************************************************************************
* djinterp [cpp]                                                pdf_document.hpp
*
*
* path:      /inc/djinterp/cpp/util/pdf/pdf_document.hpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.05
*******************************************************************************/
#ifndef DJINTERP_UTIL_PDF_PDF_DOCUMENT_HPP
#define DJINTERP_UTIL_PDF_PDF_DOCUMENT_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

///////////////////////////////////////////////////////////////////////////////
// pdf_document.hpp
//
// The high-level DPdfDocument facade: open/close, add_page, text and
// drawing calls, and save.  Defaults to the built-in backend, so this
// header pulls in the whole module via pdf_builtin_backend.hpp.
///////////////////////////////////////////////////////////////////////////////

// djinterp
#include "./pdf_builtin_backend.hpp"


NS_DJINTERP

///////////////////////////////////////////////////////////////////////////////
///                X.   DOCUMENT FACADE                                      ///
///////////////////////////////////////////////////////////////////////////////

// DPdfDocument
//   class: the agnostic document façade.  Holds a DPdfBackend
// (owning a built-in one by default, or borrowing a caller-supplied
// adapter) and exposes the common-subset drawing API in PDF user
// space.  Page state is tracked so add_page() closes the previous
// page automatically.
//
// Usage:
//   DPdfDocument doc;                       // built-in backend
//   doc.open();
//   doc.add_page(DPdfPageSize::letter());
//   doc.text(DPdfPoint(72, 720), "Hello", DPdfTextOptions());
//   doc.close();
//   doc.save("out.pdf");
//
//   // with a custom backend:
//   libharu_pdf_backend hb;                 // derives DPdfBackend
//   DPdfDocument doc2(hb);
class DPdfDocument
{
public:
    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // default: owns a built-in backend
    DPdfDocument()
        : m_owned(new DBuiltinPdfBackend()),
          m_backend(nullptr),
          m_open(false),
          m_has_page(false)
    {
        m_backend = m_owned.get();
    }

    // borrow: drives a caller-owned backend (e.g. a libHaru adapter)
    explicit DPdfDocument(
        DPdfBackend& _backend
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
    void open()
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
    void close()
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
    void add_page(
        const DPdfPageSize& _size = DPdfPageSize::letter()
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
    void text(
        const DPdfPoint&       _at,
        const std::string&     _text,
        const DPdfTextOptions& _opts = DPdfTextOptions()
    )
    {
        m_backend->draw_text(_at, _text, _opts.font, _opts.color);

        return;
    }

    // line
    void line(
        const DPdfPoint& _from,
        const DPdfPoint& _to,
        const DPdfPaint& _paint = DPdfPaint()
    )
    {
        m_backend->draw_line(_from, _to, _paint);

        return;
    }

    // rect
    void rect(
        const DPdfRect&  _rect,
        const DPdfPaint& _paint = DPdfPaint()
    )
    {
        m_backend->draw_rect(_rect, _paint);

        return;
    }

    // path
    //   draws an arbitrary vector path under the given paint.
    void path(
        const DPdfPath&  _path,
        const DPdfPaint& _paint = DPdfPaint()
    )
    {
        m_backend->draw_path(_path, _paint);

        return;
    }

    // circle
    void circle(
        const DPdfPoint& _center,
        DPdfUnit         _radius,
        const DPdfPaint& _paint = DPdfPaint()
    )
    {
        m_backend->draw_path(
            DPdfPath::circle(_center, _radius), _paint);

        return;
    }

    // ellipse
    void ellipse(
        const DPdfPoint& _center,
        DPdfUnit         _rx,
        DPdfUnit         _ry,
        const DPdfPaint& _paint = DPdfPaint()
    )
    {
        m_backend->draw_path(
            DPdfPath::ellipse(_center, _rx, _ry), _paint);

        return;
    }

    // rounded_rect
    void rounded_rect(
        const DPdfRect&  _rect,
        DPdfUnit         _radius,
        const DPdfPaint& _paint = DPdfPaint()
    )
    {
        m_backend->draw_path(
            DPdfPath::rounded_rect(_rect, _radius), _paint);

        return;
    }

    // polygon
    void polygon(
        const std::vector<DPdfPoint>& _points,
        const DPdfPaint&              _paint = DPdfPaint()
    )
    {
        m_backend->draw_path(DPdfPath::polygon(_points), _paint);

        return;
    }

    // polyline
    void polyline(
        const std::vector<DPdfPoint>& _points,
        const DPdfPaint&              _paint = DPdfPaint()
    )
    {
        m_backend->draw_path(DPdfPath::polyline(_points), _paint);

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
    void image(
        const DPdfImage& _image,
        const DPdfRect&  _dest
    )
    {
        m_backend->draw_image(_image, _dest);

        return;
    }

    // metadata
    //   records a document information field.  Opens the document
    // lazily (as add_page does) so metadata set before the first
    // page is not discarded by the deferred begin_document().
    void metadata(
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
    std::string to_bytes()
    {
        if (m_open)
        {
            close();
        }

        return m_backend->serialize();
    }

    // save
    //   ends the document if still open, then writes it to _path.
    bool save(
        const char* _path
    )
    {
        if (m_open)
        {
            close();
        }

        return m_backend->save(_path);
    }

    // save_to
    //   ends the document if still open, then writes it to a stream: a
    // string, the console, a caller's array, several at once. True only when
    // the stream took every byte; the stream keeps why it did not.
    bool save_to(
        DOutputStream& _os
    )
    {
        if (m_open)
        {
            close();
        }

        return m_backend->save_to(_os);
    }


    // =================================================================
    //  introspection
    // =================================================================

    DPdfBackend&       backend()       D_NOEXCEPT { return *m_backend; }
    const DPdfBackend& backend() const D_NOEXCEPT { return *m_backend; }

    DPdfCapabilities capabilities() const
    {
        return m_backend->capabilities();
    }

private:
    // =================================================================
    //  storage
    // =================================================================

    std::unique_ptr<DPdfBackend> m_owned;
    DPdfBackend*                 m_backend;
    bool                         m_open;
    bool                         m_has_page;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_PDF_PDF_DOCUMENT_HPP
