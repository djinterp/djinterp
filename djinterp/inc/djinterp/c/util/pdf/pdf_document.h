/******************************************************************************
* djinterp [utility]                                              pdf_document.h
*
* The backend protocol and the document facade.
*   A backend is whatever writes bytes; the facade is the agnostic drawing API
* callers use, tracking page state so that starting a page closes the previous
* one.
*
*   THE PROTOCOL IS A TABLE OF FUNCTION POINTERS PLUS A CONTEXT, which is the
* decision three earlier modules deferred to this one. C++ spells it as an
* abstract class with nine pure virtuals; C spells it as `d_pdf_backend`. The
* answers to the questions that made it a decision rather than a translation:
*
*     WHO OWNS THE CONTEXT -- the caller, always. The facade never frees it and
*   never copies it. A backend that allocates cleans up in its own `destroy`
*   slot, which the facade calls only if the caller asked it to by setting
*   `owns_context`. Ownership is therefore stated at the call site that knows
*   the answer, rather than inferred by a facade that does not.
*
*     WHAT A NULL SLOT MEANS -- unsupported, not a crash. Every dispatch checks
*   before calling and reports failure through the return, so a minimal backend
*   supplies four slots and leaves five null rather than writing five stubs
*   that do nothing. This mirrors the C++ side's split between pure virtuals
*   and the ones with base-class fallbacks; the difference is that C cannot
*   inherit a fallback, so the check moves to the caller and is written once
*   here.
*
*     WHETHER THE TABLE IS VERSIONED -- yes, by a leading `size` field. A
*   caller compiled against an older header passes a smaller table, and the
*   facade tests `size` before reading any slot beyond the original set. The
*   alternative is that adding a tenth operation silently reads past the end
*   of every existing caller's struct. `size` is checked, never trusted to
*   match sizeof.
*
*   THE FACADE IS A VALUE, NOT A HANDLE. `d_pdf_document` is a struct the
* caller owns and may put on the stack; there is no create/destroy pair,
* because the facade holds no memory of its own -- only a borrowed backend and
* four bits of page state.
*
*   NO UMBRELLA HEADER. The C++ module has `pdf.hpp`, which contains one
* include and nothing else. A C `pdf.h` mirroring it would be a pure
* forwarder, and those are being removed from this tree rather than added to
* it. A caller includes the submodule it needs.
*
*
* path:      \inc\djinterp\c\util\pdf\pdf_document.h
* link(s):   ch-pdf.tex
* author(s): TBA                                            created: 2026.08.09
******************************************************************************/

#ifndef DJINTERP_C_UTIL_PDF_DOCUMENT_
#define DJINTERP_C_UTIL_PDF_DOCUMENT_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "./pdf_primitives.h"
#include "./pdf_backend.h"


D_EXTERN_C_BEGIN

// d_pdf_backend
//   type: the backend protocol. Every slot takes the context as its first
// argument and returns int32_t -- 1 on success, 0 on failure -- including the
// slots whose C++ counterparts return void, because a C caller has no
// exception to catch and a write that failed must be reportable.
//
//   `size` MUST be set to sizeof(struct d_pdf_backend) by the caller. It is
// the version field; the facade refuses a table whose size is smaller than the
// slots it needs to read.
struct d_pdf_backend
{
    size_t  size;                   // sizeof(struct d_pdf_backend); required
    void*   context;                // borrowed by the facade, never freed
    int32_t owns_context;           // non-zero: facade may call destroy
    int32_t reserved;               // pad; must be 0

    // document lifecycle
    int32_t (*begin_document)(void* _context);
    int32_t (*end_document)(void* _context);

    // page lifecycle
    int32_t (*begin_page)(void* _context, struct d_pdf_size _size);
    int32_t (*end_page)(void* _context);

    // text
    int32_t (*draw_text)(void*                     _context,
                         struct d_pdf_point        _at,
                         const char*               _text,
                         size_t                    _length,
                         const struct d_pdf_font*  _font,
                         const struct d_pdf_color* _color);

    // vector graphics
    int32_t (*draw_line)(void*                     _context,
                         struct d_pdf_point        _from,
                         struct d_pdf_point        _to,
                         const struct d_pdf_paint* _paint);
    int32_t (*draw_rect)(void*                     _context,
                         struct d_pdf_rect         _rect,
                         const struct d_pdf_paint* _paint);

    // metadata
    int32_t (*set_metadata)(void* _context, const char* _key,
                            const char* _value);

    // capabilities and output
    int32_t (*capabilities)(void* _context,
                            struct d_pdf_capabilities* _out);
    int32_t (*save)(void* _context, const char* _path);

    // optional: graphics state nesting. NULL means unsupported, and the facade
    // reports that rather than pretending the state was saved.
    int32_t (*save_state)(void* _context);
    int32_t (*restore_state)(void* _context);

    // optional: called by the facade only when owns_context is non-zero
    void    (*destroy)(void* _context);
};

// d_pdf_document
//   type: the document facade. A value the caller owns; no create/destroy.
struct d_pdf_document
{
    struct d_pdf_backend* backend;  // borrowed
    int32_t               is_open;
    int32_t               has_page;
    int32_t               last_status;   // enum d_pdf_document_status
    int32_t               reserved;      // pad; must be 0
};

// D_PDF_DOCUMENT_STATUS
//   type: why a facade call failed. Distinguishing "no backend" from "the
// backend refused" matters: one is a caller error and one is an output error,
// and a single 0 return conflates them.
enum d_pdf_document_status
{
    D_PDF_DOC_OK              = 0,
    D_PDF_DOC_NO_BACKEND      = 1,  // null table, or size too small
    D_PDF_DOC_UNSUPPORTED     = 2,  // the slot is null
    D_PDF_DOC_BACKEND_FAILED  = 3,  // the slot ran and returned 0
    D_PDF_DOC_BAD_STATE       = 4,  // e.g. drawing with no page open

    D_PDF_DOC_STATUS_COUNT    = 5
};

// I.   protocol validation
int32_t d_pdf_backend_is_valid(const struct d_pdf_backend* _backend);

// II.  facade lifecycle
int32_t d_pdf_document_init(struct d_pdf_document* _doc,
                            struct d_pdf_backend*  _backend);
int32_t d_pdf_document_open(struct d_pdf_document* _doc);
int32_t d_pdf_document_close(struct d_pdf_document* _doc);
void    d_pdf_document_dispose(struct d_pdf_document* _doc);

// III. pages
int32_t d_pdf_document_add_page(struct d_pdf_document* _doc,
                                struct d_pdf_size      _size);

// IV.  drawing
int32_t d_pdf_document_text(struct d_pdf_document*    _doc,
                            struct d_pdf_point        _at,
                            const char*               _text,
                            size_t                    _length,
                            const struct d_pdf_font*  _font,
                            const struct d_pdf_color* _color);
int32_t d_pdf_document_line(struct d_pdf_document*    _doc,
                            struct d_pdf_point        _from,
                            struct d_pdf_point        _to,
                            const struct d_pdf_paint* _paint);
int32_t d_pdf_document_rect(struct d_pdf_document*    _doc,
                            struct d_pdf_rect         _rect,
                            const struct d_pdf_paint* _paint);

// V.   metadata, capabilities, output
int32_t d_pdf_document_set_metadata(struct d_pdf_document* _doc,
                                    const char*            _key,
                                    const char*            _value);
int32_t d_pdf_document_capabilities(struct d_pdf_document*     _doc,
                                    struct d_pdf_capabilities* _out);
int32_t d_pdf_document_save(struct d_pdf_document* _doc,
                            const char*            _path);

// VI.  status
const char* d_pdf_document_status_name(int32_t _status);
int32_t     d_pdf_document_last_status(const struct d_pdf_document* _doc);

// VII. layout assertions
D_STATIC_ASSERT(D_PDF_DOC_OK == 0,
                "success must be the zero status -- a zeroed status field "
                "means nothing has gone wrong yet");
D_STATIC_ASSERT(offsetof(struct d_pdf_backend, size) == 0,
                "size must be the FIRST member -- it is the version field, and "
                "reading it must not depend on the rest of the layout");

D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_PDF_DOCUMENT_
