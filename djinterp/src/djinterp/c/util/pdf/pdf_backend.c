/*******************************************************************************
* djinterp [c]                                                     pdf_backend.c
*
* Backend capabilities and the backend taxonomy.
*   Defines what pdf_backend.h declares. Availability and the preferred
* backend are read from env_pdf.h's D_ENV_PDF_* detection; the builtin
* serializer is always available.
*
*
* path:      /src/djinterp/c/util/pdf/pdf_backend.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/pdf/pdf_backend.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int32_t


/*
d_pdf_capabilities_init
  Text only, the C++ constructor's defaults.
*/
struct d_pdf_capabilities
d_pdf_capabilities_init(void)
{
    struct d_pdf_capabilities caps;

    caps.text            = 1;
    caps.vector_graphics = 0;
    caps.metadata        = 0;
    caps.images          = 0;
    caps.custom_fonts    = 0;
    caps.outlines        = 0;
    caps.annotations     = 0;
    caps.encryption      = 0;
    caps.compression     = 0;

    return caps;
}

/*
d_pdf_capabilities_builtin
  What the C++ builtin backend reports: text, vector graphics, metadata and
images.
*/
struct d_pdf_capabilities
d_pdf_capabilities_builtin(void)
{
    struct d_pdf_capabilities caps = d_pdf_capabilities_init();

    caps.vector_graphics = 1;
    caps.metadata        = 1;
    caps.images          = 1;

    return caps;
}

/*
d_pdf_capabilities_supports_all
  1 when every capability `_need` asks for is one `_have` offers. Nothing
needed is always supported; a missing `_have` supports nothing.
*/
int32_t
d_pdf_capabilities_supports_all(
    const struct d_pdf_capabilities* _have,
    const struct d_pdf_capabilities* _need
)
{
    // no requirement: trivially met
    if (!_need)
    {
        return 1;
    }

    const int32_t need[D_PDF_CAPABILITY_COUNT] =
    {
        _need->text,        _need->vector_graphics, _need->metadata,
        _need->images,      _need->custom_fonts,    _need->outlines,
        _need->annotations, _need->encryption,      _need->compression
    };
    int32_t       have[D_PDF_CAPABILITY_COUNT] = { 0 };

    if (_have)
    {
        have[0] = _have->text;
        have[1] = _have->vector_graphics;
        have[2] = _have->metadata;
        have[3] = _have->images;
        have[4] = _have->custom_fonts;
        have[5] = _have->outlines;
        have[6] = _have->annotations;
        have[7] = _have->encryption;
        have[8] = _have->compression;
    }

    // every capability needed must be had
    for (int i = 0; i < D_PDF_CAPABILITY_COUNT; ++i)
    {
        if ( (need[i] != 0) &&
             (have[i] == 0) )
        {
            return 0;
        }
    }

    return 1;
}

/*
d_pdf_backend_name
  The display names env_pdf.h defines, and the builtin's own. An unknown kind
has no name and gets "unknown" rather than NULL.
*/
const char*
d_pdf_backend_name(
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
        default:                      return "unknown";
    }
}

int32_t
d_pdf_backend_is_available(
    int32_t _kind
)
{
    switch (_kind)
    {
        case D_PDF_BACKEND_BUILTIN:   return 1;
        case D_PDF_BACKEND_LIBHARU:   return D_ENV_PDF_HAS_LIBHARU ? 1 : 0;
        case D_PDF_BACKEND_PDFHUMMUS: return D_ENV_PDF_HAS_PDFHUMMUS ? 1 : 0;
        case D_PDF_BACKEND_PODOFO:    return D_ENV_PDF_HAS_PODOFO ? 1 : 0;
        case D_PDF_BACKEND_CAIRO:     return D_ENV_PDF_HAS_CAIRO_PDF ? 1 : 0;
        default:                      return 0;
    }
}

int32_t
d_pdf_backend_preferred(void)
{
    return (int32_t)D_ENV_PDF_PREFERRED_BACKEND;
}
