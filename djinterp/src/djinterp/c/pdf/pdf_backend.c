#include "../../../../../inc/djinterp/c/util/pdf/pdf_backend.h"


/*
d_pdf_capabilities_init
  The default capability set: text only.

  NOT A ZEROED STRUCT, and that is the whole reason this function exists. Every
PDF engine can place text, so `text` defaults true while everything else
defaults false -- which is what the C++ constructor does. A caller reaching for
memset would get a backend that claims it cannot place text, and would then
find pdf_document silently refusing to emit anything.

Parameter(s):
  none.
Return:
  A d_pdf_capabilities with text set and every other capability clear.
*/
struct d_pdf_capabilities
d_pdf_capabilities_init
(
    void
)
{
    struct d_pdf_capabilities _caps;

    _caps.text            = 1;
    _caps.vector_graphics = 0;
    _caps.metadata        = 0;
    _caps.images          = 0;
    _caps.custom_fonts    = 0;
    _caps.outlines        = 0;
    _caps.annotations     = 0;
    _caps.encryption      = 0;
    _caps.compression     = 0;

    return _caps;
}

/*
d_pdf_capabilities_builtin
  The capability set of the built-in serializer.

  TEXT AND VECTOR GRAPHICS AND METADATA, nothing else. The built-in writer is
the always-available fallback and deliberately implements the intersection of
what every engine supports -- so a document that renders through it renders
through all of them. Claiming a capability here that the serializer does not
implement would make that guarantee false in the one backend that cannot be
swapped out.

Parameter(s):
  none.
Return:
  A d_pdf_capabilities describing the built-in backend.
*/
struct d_pdf_capabilities
d_pdf_capabilities_builtin
(
    void
)
{
    struct d_pdf_capabilities _caps = d_pdf_capabilities_init();

    _caps.vector_graphics = 1;
    _caps.metadata        = 1;

    return _caps;
}

/*
d_pdf_capabilities_supports_all
  Whether _have covers every capability _need asks for.

  IMPLICATION, NOT EQUALITY. A backend offering more than is needed still
satisfies the need; comparing the structs for equality would reject exactly the
case a capability check exists to accept. Only the members _need sets are
examined.

Parameter(s):
  _have: the backend's capabilities.
  _need: the capabilities a document requires.
Return:
  1 when every capability set in _need is also set in _have, 0 otherwise. A
NULL _need is satisfied by anything and answers 1; a NULL _have satisfies
nothing and answers 0, because "no backend" cannot support a requirement.
*/
int32_t
d_pdf_capabilities_supports_all
(
    const struct d_pdf_capabilities* _have,
    const struct d_pdf_capabilities* _need
)
{
    if (!_need)
    {
        return 1;
    }

    if (!_have)
    {
        return 0;
    }

    /*   MEMBER BY MEMBER, NOT A LOOP OVER A CAST POINTER. Treating the struct
       as an int32_t array would be shorter and would work today, but it reads
       padding as data the moment a member of another type is added -- and the
       assertion in the header is what catches that addition, not this loop. */
    if (_need->text            && !_have->text)            { return 0; }
    if (_need->vector_graphics && !_have->vector_graphics) { return 0; }
    if (_need->metadata        && !_have->metadata)        { return 0; }
    if (_need->images          && !_have->images)          { return 0; }
    if (_need->custom_fonts    && !_have->custom_fonts)    { return 0; }
    if (_need->outlines        && !_have->outlines)        { return 0; }
    if (_need->annotations     && !_have->annotations)     { return 0; }
    if (_need->encryption      && !_have->encryption)      { return 0; }
    if (_need->compression     && !_have->compression)     { return 0; }

    return 1;
}

/*
d_pdf_backend_name
  The human-readable name of a backend.

Parameter(s):
  _kind: a d_pdf_backend_kind value.
Return:
  A static literal, borrowed, never freed. An unrecognised kind answers the
builtin name, matching the enum's zero value being the always-available
backend.

  THE STRINGS COME FROM env_pdf.h, NOT FROM HERE. The first version spelled
them itself -- "libharu", "podofo", "builtin" -- which read fine and disagreed
with the C++ side on every one: its preferred_name() forwards
D_ENV_PDF_PREFERRED_BACKEND_NAME, so the real names are "libHaru", "PoDoFo",
"djinterp builtin". Inventing display strings for a mapping a shared C header
already owns is the same error as restating the selector values, and the
differential caught it on the name the build actually uses.
*/
const char*
d_pdf_backend_name
(
    int32_t _kind
)
{
    switch (_kind)
    {
        case D_PDF_BACKEND_BUILTIN:   return D_PDF_BACKEND_BUILTIN_NAME;
        case D_PDF_BACKEND_LIBHARU:   return D_ENV_PDF_LIBHARU_NAME;
        case D_PDF_BACKEND_PDFHUMMUS: return D_ENV_PDF_PDFHUMMUS_NAME;
        case D_PDF_BACKEND_PODOFO:    return D_ENV_PDF_PODOFO_NAME;
        case D_PDF_BACKEND_CAIRO:     return D_ENV_PDF_CAIRO_PDF_NAME;
        default:                      break;
    }

    return D_PDF_BACKEND_BUILTIN_NAME;
}

/*
d_pdf_backend_is_available
  Whether a backend is present in this build.

  ANSWERED FROM env_pdf.h's DETECTION MACROS, not from a runtime probe. The
macros are resolved at configure time and the answer cannot change while the
program runs, so a caller can branch on this once at startup.

  THE BUILTIN IS ALWAYS AVAILABLE. It is the djinterp serializer and has no
external dependency, which is what makes it the fallback rather than one option
among five.

Parameter(s):
  _kind: a d_pdf_backend_kind value.
Return:
  1 when the backend can be used in this build, 0 otherwise. An unrecognised
kind answers 0 -- it is not the builtin, whatever d_pdf_backend_name says about
its display string.
*/
int32_t
d_pdf_backend_is_available
(
    int32_t _kind
)
{
    switch (_kind)
    {
        case D_PDF_BACKEND_BUILTIN:
        {
            return 1;
        }

        case D_PDF_BACKEND_LIBHARU:
        {
            return (D_ENV_PDF_HAS_LIBHARU != 0) ? 1 : 0;
        }

        case D_PDF_BACKEND_PDFHUMMUS:
        {
            return (D_ENV_PDF_HAS_PDFHUMMUS != 0) ? 1 : 0;
        }

        case D_PDF_BACKEND_PODOFO:
        {
            return (D_ENV_PDF_HAS_PODOFO != 0) ? 1 : 0;
        }

        case D_PDF_BACKEND_CAIRO:
        {
            return (D_ENV_PDF_HAS_CAIRO_PDF != 0) ? 1 : 0;
        }

        default:
        {
            break;
        }
    }

    return 0;
}

/*
d_pdf_backend_preferred
  The preferred generation backend for this build.

  A HINT, NOT A COMMITMENT -- the C++ side documents it as such and this side
keeps that reading. It reports what env_pdf.h detected; a caller may still ask
for another backend and get it, provided d_pdf_backend_is_available agrees.

Parameter(s):
  none.
Return:
  A d_pdf_backend_kind value; D_PDF_BACKEND_BUILTIN when no external generation
library was detected.
*/
int32_t
d_pdf_backend_preferred
(
    void
)
{
    return (int32_t)D_ENV_PDF_PREFERRED_BACKEND;
}
