/******************************************************************************
* djinterp [utility]                                               pdf_backend.h
*
* Backend capabilities and the backend taxonomy.
*   A PDF backend is whatever actually writes bytes: the built-in serializer,
* or an adapter over libHaru, PDFHummus, PoDoFo or Cairo. This header carries
* the two VALUE types that describe one -- what it can do, and which one it is.
*
*   THE SELECTOR VALUES ARE NOT RESTATED HERE. `d_pdf_backend_kind` is defined
* in terms of the D_ENV_PDF_BACKEND_* macros from env_pdf.h, exactly as the C++
* enum is. Writing 0..4 again would be a second copy of a mapping that already
* exists in a C header both tiers include -- and a detected backend would then
* round-trip correctly through one language and not the other.
*
*   THERE IS NO d_pdf_env HERE, AND THAT IS THE POINT. The C++ side lifts the
* D_ENV_PDF_* detection macros into a `pdf_env` namespace of constexpr bools so
* backend selection can be written in ordinary C++ instead of the preprocessor.
* C has no such problem: the macros ARE the C surface, and a caller tests
* D_ENV_PDF_HAS_LIBHARU directly. Mirroring them into constants here would add
* a second spelling of each with no reader who wanted it.
*
*   THE ABSTRACT PROTOCOL IS DELIBERATELY ABSENT. `pdf_backend` is a C++ class
* with nine pure virtuals, and its C form is a struct of function pointers plus
* a context -- a real design decision (who owns the context, what a null slot
* means, whether the table is versioned) rather than a mechanical translation.
* It belongs with the module that first needs to dispatch through it, and
* inventing it here to look complete would fix those answers before anything
* had asked the questions.
*
*   CAPABILITIES DEFAULT TO TEXT-ONLY. `text` is the one capability every PDF
* engine has, so a zeroed struct would describe a backend that cannot do
* anything -- including the thing all of them can. `d_pdf_capabilities_init`
* produces the same defaults as the C++ constructor: text true, the rest false.
* A caller memsetting the struct to zero gets a DIFFERENT default, which is why
* the initialiser exists rather than a comment saying "zero is fine".
*
*
* path:      \inc\djinterp\c\util\pdf\pdf_backend.h
* link(s):   ch-pdf.tex
* author(s): TBA                                            created: 2026.08.09
******************************************************************************/

#ifndef DJINTERP_C_UTIL_PDF_BACKEND_
#define DJINTERP_C_UTIL_PDF_BACKEND_ 1

#include <stddef.h>
#include <stdint.h>
#include "../../djinterp.h"
#include "../../../core/env/env_pdf.h"
#include "./pdf_primitives.h"


D_EXTERN_C_BEGIN

// d_pdf_capabilities
//   type: what a backend can do. Every member is 0 or 1; int32_t rather than a
// bitfield or a bool array so the struct has one obvious layout across
// compilers and can be compared member-for-member by a differential.
struct d_pdf_capabilities
{
    int32_t text;                   // positioned text (always 1)
    int32_t vector_graphics;        // lines and rectangles
    int32_t metadata;               // document information dictionary
    int32_t images;                 // raster image XObjects
    int32_t custom_fonts;           // TrueType / Type0 embedding
    int32_t outlines;               // document outline / bookmarks
    int32_t annotations;            // link / text annotations
    int32_t encryption;             // document encryption
    int32_t compression;            // stream compression (deflate)
};

// D_PDF_CAPABILITY_COUNT
//   constant: how many capability members the struct carries. A caller
// iterating them needs the count, and deriving it from sizeof would be wrong
// the moment a non-int32_t member is added.
#define D_PDF_CAPABILITY_COUNT  9

// D_PDF_BACKEND_BUILTIN_NAME
//   constant: the display name of the built-in serializer.
//
//   THE ONE NAME THIS TIER HAS TO CARRY. env_pdf.h defines
// D_ENV_PDF_LIBHARU_NAME, _PODOFO_NAME, _PDFHUMMUS_NAME and _CAIRO_PDF_NAME,
// but spells the builtin's name inline inside the #else that sets
// D_ENV_PDF_PREFERRED_BACKEND_NAME -- so there is no macro to forward. The
// string is duplicated here rather than invented: it must match that branch
// exactly or the two tiers report different names for the same backend, which
// is how the first version of this file was wrong.
//   env_pdf.h arguably should own a D_ENV_PDF_BUILTIN_NAME alongside the other
// four; until it does, this is the single place the C tier states it.
#define D_PDF_BACKEND_BUILTIN_NAME  "djinterp builtin"

// d_pdf_backend_kind
//   type: which backend. Values are the D_ENV_PDF_BACKEND_* selectors from
// env_pdf.h, so a detected or preferred backend round-trips between the macro
// layer and this enum. `builtin` is the always-available djinterp serializer,
// used when no external generation library is present.
enum d_pdf_backend_kind
{
    D_PDF_BACKEND_BUILTIN   = D_ENV_PDF_BACKEND_NONE,
    D_PDF_BACKEND_LIBHARU   = D_ENV_PDF_BACKEND_LIBHARU,
    D_PDF_BACKEND_PDFHUMMUS = D_ENV_PDF_BACKEND_PDFHUMMUS,
    D_PDF_BACKEND_PODOFO    = D_ENV_PDF_BACKEND_PODOFO,
    D_PDF_BACKEND_CAIRO     = D_ENV_PDF_BACKEND_CAIRO,

    D_PDF_BACKEND_COUNT     = 5
};

// I.   capabilities
struct d_pdf_capabilities d_pdf_capabilities_init(void);
struct d_pdf_capabilities d_pdf_capabilities_builtin(void);
int32_t                   d_pdf_capabilities_supports_all(const struct d_pdf_capabilities* _have,
                                                          const struct d_pdf_capabilities* _need);

// II.  backend taxonomy
const char* d_pdf_backend_name(int32_t _kind);
int32_t     d_pdf_backend_is_available(int32_t _kind);
int32_t     d_pdf_backend_preferred(void);

// III. layout assertions
D_STATIC_ASSERT(D_PDF_BACKEND_BUILTIN == 0,
                "the builtin backend must be the zero selector -- a zeroed "
                "kind is the always-available one");
D_STATIC_ASSERT(D_PDF_BACKEND_COUNT == 5,
                "a backend was added to env_pdf.h -- extend d_pdf_backend_name "
                "and d_pdf_backend_is_available");
D_STATIC_ASSERT(sizeof(struct d_pdf_capabilities) ==
                    (D_PDF_CAPABILITY_COUNT * sizeof(int32_t)),
                "d_pdf_capabilities gained a member of another type -- "
                "D_PDF_CAPABILITY_COUNT no longer describes it");

D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_PDF_BACKEND_
