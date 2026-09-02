#include "../../../../../inc/djinterp/c/util/pdf/pdf_document.h"


/*
fail_helper
  Records a status on the document and returns 0.

  THE STATUS IS RECORDED EVEN THOUGH THE RETURN ALREADY SAYS "FAILED", because
0 conflates four different situations -- no backend, an unsupported slot, a
backend that ran and refused, and a call made in the wrong state. A caller
that only checks the return still behaves correctly; one that wants to know
why has somewhere to look.

Parameter(s):
  _doc:    the document; may be NULL, in which case nothing is recorded.
  _status: enum d_pdf_document_status.
Return:
  Always 0, so callers may write `return fail_helper(...)`.
*/
static int32_t
fail_helper
(
    struct d_pdf_document* _doc,
    int32_t                _status
)
{
    if (_doc)
    {
        _doc->last_status = _status;
    }

    return 0;
}

/*
ready_helper
  Whether a document has a usable backend.

Parameter(s):
  _doc: the document.
Return:
  1 when the document and its backend are usable, 0 otherwise. The status is
recorded on failure.
*/
static int32_t
ready_helper
(
    struct d_pdf_document* _doc
)
{
    if (!_doc)
    {
        return 0;
    }

    if (!d_pdf_backend_is_valid(_doc->backend))
    {
        return fail_helper(_doc, D_PDF_DOC_NO_BACKEND);
    }

    return 1;
}


/* =========================================================================
   I.     protocol validation
   ========================================================================= */

/*
d_pdf_backend_is_valid
  Whether a backend table can be dispatched through.

  THE SIZE FIELD IS CHECKED, NOT TRUSTED TO EQUAL sizeof. A caller compiled
against an older header passes a smaller table; reading a slot past its end is
undefined and, in practice, reads whatever follows the struct and calls it.
So the test is that the table is at least large enough to contain the slots
this build will read.

  A TABLE LARGER THAN sizeof IS ACCEPTED, deliberately: that is a caller
compiled against a NEWER header, whose extra slots this build simply does not
know about and will not call.

Parameter(s):
  _backend: the table; may be NULL.
Return:
  1 when the table is usable, 0 otherwise.
*/
int32_t
d_pdf_backend_is_valid
(
    const struct d_pdf_backend* _backend
)
{
    if (!_backend)
    {
        return 0;
    }

    if (_backend->size < sizeof(struct d_pdf_backend))
    {
        return 0;
    }

    return 1;
}


/* =========================================================================
   II.    facade lifecycle
   ========================================================================= */

/*
d_pdf_document_init
  Binds a document to a backend.

  DOES NOT OPEN THE DOCUMENT. Binding and opening are separate because opening
writes bytes, and a caller assembling several documents wants to bind them all
before any output begins.

Parameter(s):
  _doc:     the document to initialise.
  _backend: the backend table; borrowed, must outlive the document.
Return:
  1 on success, 0 when either argument is unusable.
*/
int32_t
d_pdf_document_init
(
    struct d_pdf_document* _doc,
    struct d_pdf_backend*  _backend
)
{
    if (!_doc)
    {
        return 0;
    }

    _doc->backend     = _backend;
    _doc->is_open     = 0;
    _doc->has_page    = 0;
    _doc->last_status = D_PDF_DOC_OK;
    _doc->reserved    = 0;

    if (!d_pdf_backend_is_valid(_backend))
    {
        return fail_helper(_doc, D_PDF_DOC_NO_BACKEND);
    }

    return 1;
}

/*
d_pdf_document_open
  Begins the document.

  IDEMPOTENT, matching the C++ facade: opening an already-open document is a
no-op that succeeds, not an error. A caller that opens defensively before
drawing should not have to track whether it already did.

Parameter(s):
  _doc: the document.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_pdf_document_open
(
    struct d_pdf_document* _doc
)
{
    if (!ready_helper(_doc))
    {
        return 0;
    }

    if (_doc->is_open)
    {
        return 1;
    }

    if (!_doc->backend->begin_document)
    {
        return fail_helper(_doc, D_PDF_DOC_UNSUPPORTED);
    }

    if (!_doc->backend->begin_document(_doc->backend->context))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->is_open     = 1;
    _doc->has_page    = 0;
    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}

/*
d_pdf_document_close
  Ends the document, closing any open page first.

  CLOSES THE PAGE BEFORE THE DOCUMENT. The C++ facade does the same, and the
order is not cosmetic: a backend that ends a document with a page still open
writes an unterminated page object, and the resulting file opens to an error
in some readers and a blank page in others.

Parameter(s):
  _doc: the document.
Return:
  1 on success, 0 otherwise. Closing an already-closed document succeeds.
*/
int32_t
d_pdf_document_close
(
    struct d_pdf_document* _doc
)
{
    if (!ready_helper(_doc))
    {
        return 0;
    }

    if (!_doc->is_open)
    {
        return 1;
    }

    if (_doc->has_page)
    {
        if (_doc->backend->end_page)
        {
            if (!_doc->backend->end_page(_doc->backend->context))
            {
                return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
            }
        }

        _doc->has_page = 0;
    }

    if (!_doc->backend->end_document)
    {
        return fail_helper(_doc, D_PDF_DOC_UNSUPPORTED);
    }

    if (!_doc->backend->end_document(_doc->backend->context))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->is_open     = 0;
    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}

/*
d_pdf_document_dispose
  Releases the backend's context, if and only if the caller said to.

  THE FACADE NEVER DECIDES OWNERSHIP. It calls `destroy` when `owns_context` is
set and does nothing otherwise, so a borrowed backend passed to two documents
is not freed twice and an owned one is not leaked. The flag is set at the call
site that knows the answer.

Parameter(s):
  _doc: the document.
Return:
  none.
*/
void
d_pdf_document_dispose
(
    struct d_pdf_document* _doc
)
{
    if ( (!_doc) || (!d_pdf_backend_is_valid(_doc->backend)) )
    {
        return;
    }

    if (_doc->backend->owns_context && _doc->backend->destroy)
    {
        _doc->backend->destroy(_doc->backend->context);
        _doc->backend->context = 0;
    }

    _doc->backend  = 0;
    _doc->is_open  = 0;
    _doc->has_page = 0;

    return;
}


/* =========================================================================
   III.   pages
   ========================================================================= */

/*
d_pdf_document_add_page
  Starts a page, opening the document lazily and closing any prior page.

  LAZY OPEN, matching the C++ facade: a caller may add a page without having
opened the document, which is the shape most callers actually write.

Parameter(s):
  _doc:  the document.
  _size: the page extent, in points.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_pdf_document_add_page
(
    struct d_pdf_document* _doc,
    struct d_pdf_size      _size
)
{
    if (!ready_helper(_doc))
    {
        return 0;
    }

    if (!_doc->is_open)
    {
        if (!d_pdf_document_open(_doc))
        {
            return 0;   /* status already recorded */
        }
    }

    if (_doc->has_page)
    {
        if (_doc->backend->end_page)
        {
            if (!_doc->backend->end_page(_doc->backend->context))
            {
                return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
            }
        }

        _doc->has_page = 0;
    }

    if (!_doc->backend->begin_page)
    {
        return fail_helper(_doc, D_PDF_DOC_UNSUPPORTED);
    }

    if (!_doc->backend->begin_page(_doc->backend->context, _size))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->has_page    = 1;
    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}


/* =========================================================================
   IV.    drawing
   ========================================================================= */

/*
drawable_helper
  Whether the document is in a state where drawing is meaningful.

  DRAWING WITH NO PAGE OPEN IS AN ERROR, NOT A LAZY OPEN. Adding a page
implicitly here would silently produce a document whose content landed on a
page the caller never asked for, at whatever size the default happened to be --
which renders, and is therefore not caught by anything checking only that the
file parses.

Parameter(s):
  _doc: the document.
Return:
  1 when drawing may proceed, 0 otherwise; the status is recorded on failure.
*/
static int32_t
drawable_helper
(
    struct d_pdf_document* _doc
)
{
    if (!ready_helper(_doc))
    {
        return 0;
    }

    if ( (!_doc->is_open) || (!_doc->has_page) )
    {
        return fail_helper(_doc, D_PDF_DOC_BAD_STATE);
    }

    return 1;
}

/*
d_pdf_document_text
  Draws text at a point.

Parameter(s):
  _doc:    the document.
  _at:     the baseline origin, in PDF user space.
  _text:   the bytes; borrowed for the duration of the call only.
  _length: how many.
  _font:   the face and size; NULL uses the default font.
  _color:  the fill colour; NULL uses black.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_pdf_document_text
(
    struct d_pdf_document*    _doc,
    struct d_pdf_point        _at,
    const char*               _text,
    size_t                    _length,
    const struct d_pdf_font*  _font,
    const struct d_pdf_color* _color
)
{
    struct d_pdf_font  _default_font;
    struct d_pdf_color _default_color;

    if (!drawable_helper(_doc))
    {
        return 0;
    }

    if (!_doc->backend->draw_text)
    {
        return fail_helper(_doc, D_PDF_DOC_UNSUPPORTED);
    }

    /*   Defaults are substituted HERE rather than in the backend, so every
       backend sees a fully specified call and none of them has to carry its
       own idea of what a null font means. */
    if (!_font)
    {
        _default_font = d_pdf_font_init();
        _font         = &_default_font;
    }

    if (!_color)
    {
        _default_color = d_pdf_color_gray(0.0);
        _color         = &_default_color;
    }

    if (!_doc->backend->draw_text(_doc->backend->context, _at, _text,
                                  _length, _font, _color))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}

/*
d_pdf_document_line
  Draws a straight line.

Parameter(s):
  _doc:   the document.
  _from:  the start point.
  _to:    the end point.
  _paint: stroke parameters; NULL uses the default paint.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_pdf_document_line
(
    struct d_pdf_document*    _doc,
    struct d_pdf_point        _from,
    struct d_pdf_point        _to,
    const struct d_pdf_paint* _paint
)
{
    struct d_pdf_paint _default_paint;

    if (!drawable_helper(_doc))
    {
        return 0;
    }

    if (!_doc->backend->draw_line)
    {
        return fail_helper(_doc, D_PDF_DOC_UNSUPPORTED);
    }

    if (!_paint)
    {
        _default_paint = d_pdf_paint_init();
        _paint         = &_default_paint;
    }

    if (!_doc->backend->draw_line(_doc->backend->context, _from, _to, _paint))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}

/*
d_pdf_document_rect
  Draws a rectangle.

Parameter(s):
  _doc:   the document.
  _rect:  the rectangle, anchored at its lower-left corner.
  _paint: stroke and fill parameters; NULL uses the default paint.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_pdf_document_rect
(
    struct d_pdf_document*    _doc,
    struct d_pdf_rect         _rect,
    const struct d_pdf_paint* _paint
)
{
    struct d_pdf_paint _default_paint;

    if (!drawable_helper(_doc))
    {
        return 0;
    }

    if (!_doc->backend->draw_rect)
    {
        return fail_helper(_doc, D_PDF_DOC_UNSUPPORTED);
    }

    if (!_paint)
    {
        _default_paint = d_pdf_paint_init();
        _paint         = &_default_paint;
    }

    if (!_doc->backend->draw_rect(_doc->backend->context, _rect, _paint))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}


/* =========================================================================
   V.     metadata, capabilities, output
   ========================================================================= */

/*
d_pdf_document_set_metadata
  Sets a document information entry.

  NOT GATED ON A PAGE. Metadata belongs to the document, so it may be set
before the first page and after the last -- unlike drawing, which needs a page
to land on.

Parameter(s):
  _doc:   the document.
  _key:   the entry name, NUL-terminated.
  _value: the entry value, NUL-terminated.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_pdf_document_set_metadata
(
    struct d_pdf_document* _doc,
    const char*            _key,
    const char*            _value
)
{
    if (!ready_helper(_doc))
    {
        return 0;
    }

    if ( (!_key) || (!_value) )
    {
        return fail_helper(_doc, D_PDF_DOC_BAD_STATE);
    }

    if (!_doc->backend->set_metadata)
    {
        return fail_helper(_doc, D_PDF_DOC_UNSUPPORTED);
    }

    if (!_doc->backend->set_metadata(_doc->backend->context, _key, _value))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}

/*
d_pdf_document_capabilities
  Reports what the bound backend can do.

  A BACKEND WITH NO capabilities SLOT REPORTS THE TEXT-ONLY DEFAULT rather than
failing. Every backend can place text, so the honest answer to "what can this
one do" when it declines to say is the minimum every backend has -- not
nothing, which would stop a caller drawing text that would in fact have worked.

Parameter(s):
  _doc: the document.
  _out: filled on success.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_pdf_document_capabilities
(
    struct d_pdf_document*     _doc,
    struct d_pdf_capabilities* _out
)
{
    if (!_out)
    {
        return fail_helper(_doc, D_PDF_DOC_BAD_STATE);
    }

    if (!ready_helper(_doc))
    {
        return 0;
    }

    if (!_doc->backend->capabilities)
    {
        *_out             = d_pdf_capabilities_init();
        _doc->last_status = D_PDF_DOC_OK;

        return 1;
    }

    if (!_doc->backend->capabilities(_doc->backend->context, _out))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}

/*
d_pdf_document_save
  Writes the assembled document to a path.

  DOES NOT CLOSE THE DOCUMENT FIRST, and that is deliberate rather than an
omission. Closing is the caller's decision because a backend may legitimately
support saving a snapshot mid-assembly; a facade that closed implicitly would
make that impossible and would surprise a caller who meant to continue. Saving
an open document with an open page is a caller error the backend reports.

Parameter(s):
  _doc:  the document.
  _path: the output path, NUL-terminated.
Return:
  1 on success, 0 otherwise.
*/
int32_t
d_pdf_document_save
(
    struct d_pdf_document* _doc,
    const char*            _path
)
{
    if (!ready_helper(_doc))
    {
        return 0;
    }

    if (!_path)
    {
        return fail_helper(_doc, D_PDF_DOC_BAD_STATE);
    }

    if (!_doc->backend->save)
    {
        return fail_helper(_doc, D_PDF_DOC_UNSUPPORTED);
    }

    if (!_doc->backend->save(_doc->backend->context, _path))
    {
        return fail_helper(_doc, D_PDF_DOC_BACKEND_FAILED);
    }

    _doc->last_status = D_PDF_DOC_OK;

    return 1;
}


/* =========================================================================
   VI.    status
   ========================================================================= */

/*
d_pdf_document_status_name
  The spelling of a status code, for a diagnostic.

Parameter(s):
  _status: enum d_pdf_document_status.
Return:
  A static literal; "unknown" for anything out of range, never NULL.
*/
const char*
d_pdf_document_status_name
(
    int32_t _status
)
{
    switch (_status)
    {
        case D_PDF_DOC_OK:             return "ok";
        case D_PDF_DOC_NO_BACKEND:     return "no-backend";
        case D_PDF_DOC_UNSUPPORTED:    return "unsupported";
        case D_PDF_DOC_BACKEND_FAILED: return "backend-failed";
        case D_PDF_DOC_BAD_STATE:      return "bad-state";
        default:                       break;
    }

    return "unknown";
}

/*
d_pdf_document_last_status
  Why the most recent call failed.

Parameter(s):
  _doc: the document; NULL answers no-backend.
Return:
  enum d_pdf_document_status.
*/
int32_t
d_pdf_document_last_status
(
    const struct d_pdf_document* _doc
)
{
    if (!_doc)
    {
        return D_PDF_DOC_NO_BACKEND;
    }

    return _doc->last_status;
}
