/*******************************************************************************
* djinterp [c]                                                    pdf_document.c
*
* The backend protocol's validation and the document facade.
*   Defines what pdf_document.h declares. The facade follows the C++
* pdf_document: opening is idempotent, adding a page opens the document and
* ends the previous page, metadata opens the document, and saving closes it
* first. Closing calls only end_document, as the C++ does: a page still open
* is the backend's to finish. Unlike the C++, drawing with no page open is
* refused (D_PDF_DOC_BAD_STATE), as the header specifies, and every call
* records why it failed in `last_status`. State changes only on success.
*
*
* path:      /src/djinterp/c/util/pdf/pdf_document.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/pdf/pdf_document.h"  // corresponding header
// std
#include <stddef.h>  // size_t, offsetof, NULL
#include <string.h>  // memset
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int32_t


// D_INTERNAL_PDF_HAS_SLOT
//   macro: whether a table of `_backend->size` bytes reaches the whole of
// `_slot`. A slot past the end of an older caller's table is absent, not read.
#define D_INTERNAL_PDF_HAS_SLOT(_backend, _slot)                             \
    ( (_backend)->size >= ( offsetof(struct d_pdf_backend, _slot) +           \
                            sizeof((_backend)->_slot) ) )

/*
d_pdf_backend_is_valid
  A table is valid when it reaches every slot of the original protocol,
through `save`; the optional slots after it are checked one by one when
used. Null slots are not invalid -- they are reported as unsupported when
called.
*/
int32_t
d_pdf_backend_is_valid(
    const struct d_pdf_backend* _backend
)
{
    return ( (_backend != NULL) &&
             (D_INTERNAL_PDF_HAS_SLOT(_backend, save)) ) ? 1 : 0;
}

/*
d_internal_pdf_result
  Records a status, and turns it into the facade's 1 or 0.
*/
static int32_t
d_internal_pdf_result(
    struct d_pdf_document* _doc,
    int32_t                _status
)
{
    _doc->last_status = _status;

    return (_status == (int32_t)D_PDF_DOC_OK) ? 1 : 0;
}

/*
d_internal_pdf_ran
  The status of a slot that was called.
*/
static int32_t
d_internal_pdf_ran(
    int32_t _ok
)
{
    return _ok ? (int32_t)D_PDF_DOC_OK : (int32_t)D_PDF_DOC_BACKEND_FAILED;
}

int32_t
d_pdf_document_init(
    struct d_pdf_document* _doc,
    struct d_pdf_backend*  _backend
)
{
    // nowhere to initialise
    if (!_doc)
    {
        return 0;
    }

    memset(_doc, 0, sizeof(*_doc));

    // a table too small to read is no backend
    if (!d_pdf_backend_is_valid(_backend))
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_NO_BACKEND);
    }

    _doc->backend = _backend;

    return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_OK);
}

int32_t
d_pdf_document_open(
    struct d_pdf_document* _doc
)
{
    // no document
    if (!_doc)
    {
        return 0;
    }

    // no backend to open
    if (!d_pdf_backend_is_valid(_doc->backend))
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_NO_BACKEND);
    }

    // already open: nothing to do
    if (_doc->is_open)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_OK);
    }

    if (!_doc->backend->begin_document)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_UNSUPPORTED);
    }

    if (!_doc->backend->begin_document(_doc->backend->context))
    {
        return d_internal_pdf_result(_doc,
                                     (int32_t)D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->is_open  = 1;
    _doc->has_page = 0;

    return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_OK);
}

int32_t
d_pdf_document_close(
    struct d_pdf_document* _doc
)
{
    // no document
    if (!_doc)
    {
        return 0;
    }

    // not open: nothing to close
    if (!_doc->is_open)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_OK);
    }

    if (!d_pdf_backend_is_valid(_doc->backend))
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_NO_BACKEND);
    }

    if (!_doc->backend->end_document)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_UNSUPPORTED);
    }

    if (!_doc->backend->end_document(_doc->backend->context))
    {
        return d_internal_pdf_result(_doc,
                                     (int32_t)D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->is_open  = 0;
    _doc->has_page = 0;

    return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_OK);
}

/*
d_pdf_document_dispose
  Calls the backend's destroy slot only when the caller set owns_context and
the table reaches the slot; then forgets the backend. It does not close the
document: closing writes output, and disposing is not a request to.
*/
void
d_pdf_document_dispose(
    struct d_pdf_document* _doc
)
{
    // no document
    if (!_doc)
    {
        return;
    }

    struct d_pdf_backend* const backend = _doc->backend;

    // destroy the context only if the facade was given it to own
    if ( (d_pdf_backend_is_valid(backend))             &&
         (backend->owns_context != 0)                  &&
         (D_INTERNAL_PDF_HAS_SLOT(backend, destroy))  &&
         (backend->destroy != NULL) )
    {
        backend->destroy(backend->context);
    }

    memset(_doc, 0, sizeof(*_doc));

    return;
}

int32_t
d_pdf_document_add_page(
    struct d_pdf_document* _doc,
    struct d_pdf_size      _size
)
{
    // no document, or one that cannot be opened
    if ( (!_doc) ||
         (!d_pdf_document_open(_doc)) )
    {
        return 0;
    }

    struct d_pdf_backend* const backend = _doc->backend;

    // both page slots are needed before anything is ended
    if ( ( (_doc->has_page) && (!backend->end_page) ) ||
         (!backend->begin_page) )
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_UNSUPPORTED);
    }

    // starting a page ends the previous one
    if (_doc->has_page)
    {
        if (!backend->end_page(backend->context))
        {
            return d_internal_pdf_result(_doc,
                                         (int32_t)D_PDF_DOC_BACKEND_FAILED);
        }

        _doc->has_page = 0;
    }

    if (!backend->begin_page(backend->context, _size))
    {
        return d_internal_pdf_result(_doc,
                                     (int32_t)D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->has_page = 1;

    return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_OK);
}

/*
d_internal_pdf_drawable
  The status a drawing call starts from: no backend, or no page open, or OK.
*/
static int32_t
d_internal_pdf_drawable(
    const struct d_pdf_document* _doc
)
{
    if (!d_pdf_backend_is_valid(_doc->backend))
    {
        return (int32_t)D_PDF_DOC_NO_BACKEND;
    }

    return ( (_doc->is_open) && (_doc->has_page) )
        ? (int32_t)D_PDF_DOC_OK
        : (int32_t)D_PDF_DOC_BAD_STATE;
}

int32_t
d_pdf_document_text(
    struct d_pdf_document*    _doc,
    struct d_pdf_point        _at,
    const char*               _text,
    size_t                    _length,
    const struct d_pdf_font*  _font,
    const struct d_pdf_color* _color
)
{
    // no document
    if (!_doc)
    {
        return 0;
    }

    const int32_t status = d_internal_pdf_drawable(_doc);

    if (status != (int32_t)D_PDF_DOC_OK)
    {
        return d_internal_pdf_result(_doc, status);
    }

    if (!_doc->backend->draw_text)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_UNSUPPORTED);
    }

    return d_internal_pdf_result(
        _doc,
        d_internal_pdf_ran(_doc->backend->draw_text(_doc->backend->context,
                                                    _at,
                                                    _text,
                                                    _length,
                                                    _font,
                                                    _color)));
}

int32_t
d_pdf_document_line(
    struct d_pdf_document*    _doc,
    struct d_pdf_point        _from,
    struct d_pdf_point        _to,
    const struct d_pdf_paint* _paint
)
{
    // no document
    if (!_doc)
    {
        return 0;
    }

    const int32_t status = d_internal_pdf_drawable(_doc);

    if (status != (int32_t)D_PDF_DOC_OK)
    {
        return d_internal_pdf_result(_doc, status);
    }

    if (!_doc->backend->draw_line)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_UNSUPPORTED);
    }

    return d_internal_pdf_result(
        _doc,
        d_internal_pdf_ran(_doc->backend->draw_line(_doc->backend->context,
                                                    _from,
                                                    _to,
                                                    _paint)));
}

int32_t
d_pdf_document_rect(
    struct d_pdf_document*    _doc,
    struct d_pdf_rect         _rect,
    const struct d_pdf_paint* _paint
)
{
    // no document
    if (!_doc)
    {
        return 0;
    }

    const int32_t status = d_internal_pdf_drawable(_doc);

    if (status != (int32_t)D_PDF_DOC_OK)
    {
        return d_internal_pdf_result(_doc, status);
    }

    if (!_doc->backend->draw_rect)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_UNSUPPORTED);
    }

    return d_internal_pdf_result(
        _doc,
        d_internal_pdf_ran(_doc->backend->draw_rect(_doc->backend->context,
                                                    _rect,
                                                    _paint)));
}

int32_t
d_pdf_document_set_metadata(
    struct d_pdf_document* _doc,
    const char*            _key,
    const char*            _value
)
{
    // no document, or one that cannot be opened
    if ( (!_doc) ||
         (!d_pdf_document_open(_doc)) )
    {
        return 0;
    }

    if (!_doc->backend->set_metadata)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_UNSUPPORTED);
    }

    return d_internal_pdf_result(
        _doc,
        d_internal_pdf_ran(_doc->backend->set_metadata(_doc->backend->context,
                                                       _key,
                                                       _value)));
}

/*
d_pdf_document_capabilities
  The one slot with a fallback written here, standing in for the C++ base
class's: a backend that leaves `capabilities` null reports text only, the
default d_pdf_capabilities_init produces. A missing `_out` is a caller
error, reported as a bad state.
*/
int32_t
d_pdf_document_capabilities(
    struct d_pdf_document*     _doc,
    struct d_pdf_capabilities* _out
)
{
    // no document
    if (!_doc)
    {
        return 0;
    }

    if (!d_pdf_backend_is_valid(_doc->backend))
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_NO_BACKEND);
    }

    if (!_out)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_BAD_STATE);
    }

    // the base-class default: text only
    if (!_doc->backend->capabilities)
    {
        *_out = d_pdf_capabilities_init();

        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_OK);
    }

    return d_internal_pdf_result(
        _doc,
        d_internal_pdf_ran(_doc->backend->capabilities(_doc->backend->context,
                                                       _out)));
}

int32_t
d_pdf_document_save(
    struct d_pdf_document* _doc,
    const char*            _path
)
{
    // no document, or one that cannot be closed first
    if ( (!_doc) ||
         (!d_pdf_document_close(_doc)) )
    {
        return 0;
    }

    if (!d_pdf_backend_is_valid(_doc->backend))
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_NO_BACKEND);
    }

    if (!_doc->backend->save)
    {
        return d_internal_pdf_result(_doc, (int32_t)D_PDF_DOC_UNSUPPORTED);
    }

    return d_internal_pdf_result(
        _doc,
        d_internal_pdf_ran(_doc->backend->save(_doc->backend->context,
                                               _path)));
}

const char*
d_pdf_document_status_name(
    int32_t _status
)
{
    switch (_status)
    {
        case D_PDF_DOC_OK:             return "ok";
        case D_PDF_DOC_NO_BACKEND:     return "no backend";
        case D_PDF_DOC_UNSUPPORTED:    return "unsupported";
        case D_PDF_DOC_BACKEND_FAILED: return "backend failed";
        case D_PDF_DOC_BAD_STATE:      return "bad state";
        default:                       return "unknown";
    }
}

int32_t
d_pdf_document_last_status(
    const struct d_pdf_document* _doc
)
{
    return (_doc != NULL) ? _doc->last_status
                          : (int32_t)D_PDF_DOC_NO_BACKEND;
}
