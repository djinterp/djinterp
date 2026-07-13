/******************************************************************************
* djinterp [core]                                                  env_pdf.h
*
* djinterp PDF library environment detection:
*   Compile-time detection of PDF generation, rendering, and parsing libraries
* present in the developer's environment.  This is the PDF analogue of
* env_c_lib.h: it probes for optional third-party headers and, where each
* library exposes one, extracts the version - so portable code can select a
* backend, gate features, and degrade gracefully without a build system
* telling it what is installed.
*
*   Requires:  env.h  (for D_ENV_COMPILER_*, D_ENV_LANG_*, D_ENV_OS_*).
*   Optionally pairs with the djinterp pdf subsystem (pdf.hpp et al.), but
* depends on none of it - this header is pure environment detection and can
* be used to drive a backend selection before any djinterp PDF type is named.
*
*   DETECTION STRATEGY:
*   Where the compiler supports __has_include (all of GCC, Clang, MSVC in
* their relevant versions, plus any C23 / C++17 conformer), each library is
* detected by probing its public header.  When __has_include is unavailable
* the module falls back to "not detected" unless the integrator pre-defines
* the corresponding D_ENV_PDF_HAS_* macro.  Every macro here is pre-definable:
* #define it before including this header to force a value (useful for cross
* builds, vendored copies on a non-standard include path, or testing).
*
*   VERSION EXTRACTION:
*   A detected library's version is reported only when the library itself
* exposes compile-time version macros AND the integrator has actually
* included that library's header in the translation unit (this module does
* NOT #include third-party headers - probing presence must not pull a heavy
* dependency into every compile).  Until the vendor header is included, the
* version components read 0 and the version string reads "unknown".  Include
* the vendor header before this one to populate them.
*
*   CATEGORIES:
*     Generation (write):  libHaru, PDFHummus/PDF-Writer, PoDoFo, Cairo-PDF
*     Render / parse:      Poppler, MuPDF, PDFium
*   A library may span categories (PoDoFo both creates and parses); it is
* classified by its primary role.  Aggregate flags summarize whether ANY
* generation or rendering library is available.
*
*   NAMING CONVENTION:
*     D_ENV_PDF_HAS_[LIB]              1 if detected, 0 otherwise
*     D_ENV_PDF_[LIB]_NAME            human-readable library name (string)
*     D_ENV_PDF_[LIB]_VENDOR          vendor / origin (string)
*     D_ENV_PDF_[LIB]_LICENSE         license short name (string)
*     D_ENV_PDF_[LIB]_HEADER          probed header path (string)
*     D_ENV_PDF_[LIB]_VERSION_MAJOR   integer (0 if header not included)
*     D_ENV_PDF_[LIB]_VERSION_MINOR   integer
*     D_ENV_PDF_[LIB]_VERSION_PATCH   integer
*     D_ENV_PDF_[LIB]_VERSION_STRING  string ("unknown" if unavailable)
*
*
* TABLE OF CONTENTS
* =================
* I.    PROBE INFRASTRUCTURE
* II.   GENERATION LIBRARIES
*       a. libHaru
*       b. PDFHummus / PDF-Writer
*       c. PoDoFo
*       d. Cairo (PDF surface)
* III.  RENDER / PARSE LIBRARIES
*       a. Poppler
*       b. MuPDF
*       c. PDFium
* IV.   AGGREGATE CLASSIFICATION
* V.    PREFERRED-BACKEND HINT
*
*
* path:      /inc/core/env/env_pdf.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_PDF_
#define DJINTERP_ENVIRONMENT_PDF_ 1


// =============================================================================
// I.   PROBE INFRASTRUCTURE
// =============================================================================

// D_ENV_PDF_HAS_INCLUDE_PROBE
//   capability: 1 when the compiler implements __has_include, which this
// module uses to detect optional library headers.  Guarded so the module
// degrades cleanly (to "nothing detected" unless pre-defined) on toolchains
// without it.  __has_include is available as __has_include since GCC 5,
// Clang 3.x, and MSVC 19.1x (VS2017 15.3), and is standard in C++17 / C23.
#ifndef D_ENV_PDF_HAS_INCLUDE_PROBE
    #if defined(__has_include)
        #define D_ENV_PDF_HAS_INCLUDE_PROBE 1
    #else
        #define D_ENV_PDF_HAS_INCLUDE_PROBE 0
    #endif
#endif

// D_INTERNAL_PDF_PROBE(header)
//   helper: expands to 1 if `header` is includable, else 0.  When the
// compiler lacks __has_include the probe is conservatively 0 (callers may
// still pre-define a specific D_ENV_PDF_HAS_* macro to override).
#if D_ENV_PDF_HAS_INCLUDE_PROBE
    #define D_INTERNAL_PDF_PROBE(header) __has_include(header)
#else
    #define D_INTERNAL_PDF_PROBE(header) 0
#endif


// =============================================================================
// II.  GENERATION LIBRARIES
// =============================================================================

// -----------------------------------------------------------------------------
// a.  libHaru  (Haru Free PDF Library)
//
//   Pure-C generation library.  Public header "hpdf.h"; version macros live
// in "hpdf_version.h" (HPDF_MAJOR_VERSION / HPDF_MINOR_VERSION /
// HPDF_BUGFIX_VERSION, plus HPDF_VERSION_TEXT).  Historically hpdf.h pulled
// in hpdf_version.h, but some master builds stopped propagating it, so the
// version is probed from hpdf_version.h independently of presence detection.
// -----------------------------------------------------------------------------

#ifndef D_ENV_PDF_HAS_LIBHARU
    #if D_INTERNAL_PDF_PROBE(<hpdf.h>)
        #define D_ENV_PDF_HAS_LIBHARU 1
    #else
        #define D_ENV_PDF_HAS_LIBHARU 0
    #endif
#endif

#define D_ENV_PDF_LIBHARU_NAME    "libHaru"
#define D_ENV_PDF_LIBHARU_VENDOR  "Haru / Takeshi Kanno et al."
#define D_ENV_PDF_LIBHARU_LICENSE "ZLIB/libpng"
#define D_ENV_PDF_LIBHARU_HEADER  "hpdf.h"

// version: populated only if the integrator has included a libHaru header
// that defines the version macros (hpdf.h or hpdf_version.h) before this one
#ifndef D_ENV_PDF_LIBHARU_VERSION_MAJOR
    #if defined(HPDF_MAJOR_VERSION)
        #define D_ENV_PDF_LIBHARU_VERSION_MAJOR HPDF_MAJOR_VERSION
    #else
        #define D_ENV_PDF_LIBHARU_VERSION_MAJOR 0
    #endif
#endif

#ifndef D_ENV_PDF_LIBHARU_VERSION_MINOR
    #if defined(HPDF_MINOR_VERSION)
        #define D_ENV_PDF_LIBHARU_VERSION_MINOR HPDF_MINOR_VERSION
    #else
        #define D_ENV_PDF_LIBHARU_VERSION_MINOR 0
    #endif
#endif

#ifndef D_ENV_PDF_LIBHARU_VERSION_PATCH
    #if defined(HPDF_BUGFIX_VERSION)
        #define D_ENV_PDF_LIBHARU_VERSION_PATCH HPDF_BUGFIX_VERSION
    #else
        #define D_ENV_PDF_LIBHARU_VERSION_PATCH 0
    #endif
#endif

#ifndef D_ENV_PDF_LIBHARU_VERSION_STRING
    #if defined(HPDF_VERSION_TEXT)
        #define D_ENV_PDF_LIBHARU_VERSION_STRING HPDF_VERSION_TEXT
    #else
        #define D_ENV_PDF_LIBHARU_VERSION_STRING "unknown"
    #endif
#endif


// -----------------------------------------------------------------------------
// b.  PDFHummus / PDF-Writer
//
//   Native-C++ high-performance write + parse library.  Public header
// "PDFWriter.h".  Upstream does not expose a stable compile-time version
// macro, so version is reported as unknown under header-only detection;
// integrators can pre-define the version macros if they track it.
// -----------------------------------------------------------------------------

#ifndef D_ENV_PDF_HAS_PDFHUMMUS
    #if ( D_INTERNAL_PDF_PROBE(<PDFWriter.h>) ||  \
          D_INTERNAL_PDF_PROBE(<PDFWriter/PDFWriter.h>) )
        #define D_ENV_PDF_HAS_PDFHUMMUS 1
    #else
        #define D_ENV_PDF_HAS_PDFHUMMUS 0
    #endif
#endif

#define D_ENV_PDF_PDFHUMMUS_NAME    "PDFHummus (PDF-Writer)"
#define D_ENV_PDF_PDFHUMMUS_VENDOR  "Gal Kahana"
#define D_ENV_PDF_PDFHUMMUS_LICENSE "Apache-2.0"
#define D_ENV_PDF_PDFHUMMUS_HEADER  "PDFWriter.h"

#ifndef D_ENV_PDF_PDFHUMMUS_VERSION_MAJOR
    #define D_ENV_PDF_PDFHUMMUS_VERSION_MAJOR 0
#endif
#ifndef D_ENV_PDF_PDFHUMMUS_VERSION_MINOR
    #define D_ENV_PDF_PDFHUMMUS_VERSION_MINOR 0
#endif
#ifndef D_ENV_PDF_PDFHUMMUS_VERSION_PATCH
    #define D_ENV_PDF_PDFHUMMUS_VERSION_PATCH 0
#endif
#ifndef D_ENV_PDF_PDFHUMMUS_VERSION_STRING
    #define D_ENV_PDF_PDFHUMMUS_VERSION_STRING "unknown"
#endif


// -----------------------------------------------------------------------------
// c.  PoDoFo
//
//   C++ create / parse / modify library.  The modern API header is
// "podofo/podofo.h"; older trees used "podofo/base/PdfDefines.h".  Version
// macros: PODOFO_VERSION_MAJOR / MINOR / PATCH, with PODOFO_VERSION packed as
// 0xMMmmpp and PODOFO_MAKE_VERSION(M,m,p).  Modern PoDoFo (0.10+) requires
// C++17.  License: LGPL-2.0+/MPL-2.0 (library); GPL (tools).
// -----------------------------------------------------------------------------

#ifndef D_ENV_PDF_HAS_PODOFO
    #if ( D_INTERNAL_PDF_PROBE(<podofo/podofo.h>) ||  \
          D_INTERNAL_PDF_PROBE(<podofo/base/PdfDefines.h>) )
        #define D_ENV_PDF_HAS_PODOFO 1
    #else
        #define D_ENV_PDF_HAS_PODOFO 0
    #endif
#endif

#define D_ENV_PDF_PODOFO_NAME    "PoDoFo"
#define D_ENV_PDF_PODOFO_VENDOR  "PoDoFo project"
#define D_ENV_PDF_PODOFO_LICENSE "LGPL-2.0+/MPL-2.0"
#define D_ENV_PDF_PODOFO_HEADER  "podofo/podofo.h"

#ifndef D_ENV_PDF_PODOFO_VERSION_MAJOR
    #if defined(PODOFO_VERSION_MAJOR)
        #define D_ENV_PDF_PODOFO_VERSION_MAJOR PODOFO_VERSION_MAJOR
    #else
        #define D_ENV_PDF_PODOFO_VERSION_MAJOR 0
    #endif
#endif

#ifndef D_ENV_PDF_PODOFO_VERSION_MINOR
    #if defined(PODOFO_VERSION_MINOR)
        #define D_ENV_PDF_PODOFO_VERSION_MINOR PODOFO_VERSION_MINOR
    #else
        #define D_ENV_PDF_PODOFO_VERSION_MINOR 0
    #endif
#endif

#ifndef D_ENV_PDF_PODOFO_VERSION_PATCH
    #if defined(PODOFO_VERSION_PATCH)
        #define D_ENV_PDF_PODOFO_VERSION_PATCH PODOFO_VERSION_PATCH
    #else
        #define D_ENV_PDF_PODOFO_VERSION_PATCH 0
    #endif
#endif

#ifndef D_ENV_PDF_PODOFO_VERSION_STRING
    #define D_ENV_PDF_PODOFO_VERSION_STRING "unknown"
#endif


// -----------------------------------------------------------------------------
// d.  Cairo (PDF surface)
//
//   Cairo is a 2D graphics library with an optional PDF backend selected at
// its own build time.  Presence of the PDF surface header does not guarantee
// the PDF backend was compiled in; CAIRO_HAS_PDF_SURFACE (from cairo's
// cairo-features.h, available once <cairo.h> is included) is the authoritative
// runtime-capable gate.  Version macros: CAIRO_VERSION_MAJOR / MINOR / MICRO
// and CAIRO_VERSION_STRING.  License: LGPL-2.1 / MPL-1.1.
// -----------------------------------------------------------------------------

#ifndef D_ENV_PDF_HAS_CAIRO_PDF
    #if D_INTERNAL_PDF_PROBE(<cairo-pdf.h>)
        #define D_ENV_PDF_HAS_CAIRO_PDF 1
    #else
        #define D_ENV_PDF_HAS_CAIRO_PDF 0
    #endif
#endif

#define D_ENV_PDF_CAIRO_PDF_NAME    "Cairo (PDF surface)"
#define D_ENV_PDF_CAIRO_PDF_VENDOR  "cairographics.org"
#define D_ENV_PDF_CAIRO_PDF_LICENSE "LGPL-2.1/MPL-1.1"
#define D_ENV_PDF_CAIRO_PDF_HEADER  "cairo-pdf.h"

// D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED
//   capability: 1 only when cairo's headers report the PDF surface backend
// was actually compiled in (requires <cairo.h> included beforehand).
#ifndef D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED
    #if defined(CAIRO_HAS_PDF_SURFACE) && (CAIRO_HAS_PDF_SURFACE)
        #define D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED 1
    #else
        #define D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED 0
    #endif
#endif

#ifndef D_ENV_PDF_CAIRO_PDF_VERSION_MAJOR
    #if defined(CAIRO_VERSION_MAJOR)
        #define D_ENV_PDF_CAIRO_PDF_VERSION_MAJOR CAIRO_VERSION_MAJOR
    #else
        #define D_ENV_PDF_CAIRO_PDF_VERSION_MAJOR 0
    #endif
#endif

#ifndef D_ENV_PDF_CAIRO_PDF_VERSION_MINOR
    #if defined(CAIRO_VERSION_MINOR)
        #define D_ENV_PDF_CAIRO_PDF_VERSION_MINOR CAIRO_VERSION_MINOR
    #else
        #define D_ENV_PDF_CAIRO_PDF_VERSION_MINOR 0
    #endif
#endif

#ifndef D_ENV_PDF_CAIRO_PDF_VERSION_PATCH
    #if defined(CAIRO_VERSION_MICRO)
        #define D_ENV_PDF_CAIRO_PDF_VERSION_PATCH CAIRO_VERSION_MICRO
    #else
        #define D_ENV_PDF_CAIRO_PDF_VERSION_PATCH 0
    #endif
#endif

#ifndef D_ENV_PDF_CAIRO_PDF_VERSION_STRING
    #if defined(CAIRO_VERSION_STRING)
        #define D_ENV_PDF_CAIRO_PDF_VERSION_STRING CAIRO_VERSION_STRING
    #else
        #define D_ENV_PDF_CAIRO_PDF_VERSION_STRING "unknown"
    #endif
#endif


// =============================================================================
// III. RENDER / PARSE LIBRARIES
// =============================================================================

// -----------------------------------------------------------------------------
// a.  Poppler
//
//   Rendering / parsing library (the cpp wrapper header is
// "poppler/cpp/poppler-document.h"; the lower C++ API lives under
// "poppler/PDFDoc.h").  Version: POPPLER_VERSION / _MAJOR / _MINOR / _MICRO
// from poppler/cpp/poppler-version.h.  License: GPL-2.0+.
// -----------------------------------------------------------------------------

#ifndef D_ENV_PDF_HAS_POPPLER
    #if ( D_INTERNAL_PDF_PROBE(<poppler/cpp/poppler-document.h>) ||  \
          D_INTERNAL_PDF_PROBE(<poppler/PDFDoc.h>) )
        #define D_ENV_PDF_HAS_POPPLER 1
    #else
        #define D_ENV_PDF_HAS_POPPLER 0
    #endif
#endif

#define D_ENV_PDF_POPPLER_NAME    "Poppler"
#define D_ENV_PDF_POPPLER_VENDOR  "freedesktop.org"
#define D_ENV_PDF_POPPLER_LICENSE "GPL-2.0+"
#define D_ENV_PDF_POPPLER_HEADER  "poppler/cpp/poppler-document.h"

#ifndef D_ENV_PDF_POPPLER_VERSION_MAJOR
    #if defined(POPPLER_VERSION_MAJOR)
        #define D_ENV_PDF_POPPLER_VERSION_MAJOR POPPLER_VERSION_MAJOR
    #else
        #define D_ENV_PDF_POPPLER_VERSION_MAJOR 0
    #endif
#endif

#ifndef D_ENV_PDF_POPPLER_VERSION_MINOR
    #if defined(POPPLER_VERSION_MINOR)
        #define D_ENV_PDF_POPPLER_VERSION_MINOR POPPLER_VERSION_MINOR
    #else
        #define D_ENV_PDF_POPPLER_VERSION_MINOR 0
    #endif
#endif

#ifndef D_ENV_PDF_POPPLER_VERSION_PATCH
    #if defined(POPPLER_VERSION_MICRO)
        #define D_ENV_PDF_POPPLER_VERSION_PATCH POPPLER_VERSION_MICRO
    #else
        #define D_ENV_PDF_POPPLER_VERSION_PATCH 0
    #endif
#endif

#ifndef D_ENV_PDF_POPPLER_VERSION_STRING
    #if defined(POPPLER_VERSION)
        #define D_ENV_PDF_POPPLER_VERSION_STRING POPPLER_VERSION
    #else
        #define D_ENV_PDF_POPPLER_VERSION_STRING "unknown"
    #endif
#endif


// -----------------------------------------------------------------------------
// b.  MuPDF
//
//   Lightweight render / parse (and some write) library.  Public header
// "mupdf/fitz.h".  MuPDF does not provide a stable compile-time version
// macro (version is a build-time string), so the version is reported unknown
// under header-only detection.  License: AGPL-3.0 (or commercial).
// -----------------------------------------------------------------------------

#ifndef D_ENV_PDF_HAS_MUPDF
    #if D_INTERNAL_PDF_PROBE(<mupdf/fitz.h>)
        #define D_ENV_PDF_HAS_MUPDF 1
    #else
        #define D_ENV_PDF_HAS_MUPDF 0
    #endif
#endif

#define D_ENV_PDF_MUPDF_NAME    "MuPDF"
#define D_ENV_PDF_MUPDF_VENDOR  "Artifex Software"
#define D_ENV_PDF_MUPDF_LICENSE "AGPL-3.0 / commercial"
#define D_ENV_PDF_MUPDF_HEADER  "mupdf/fitz.h"

#ifndef D_ENV_PDF_MUPDF_VERSION_MAJOR
    #define D_ENV_PDF_MUPDF_VERSION_MAJOR 0
#endif
#ifndef D_ENV_PDF_MUPDF_VERSION_MINOR
    #define D_ENV_PDF_MUPDF_VERSION_MINOR 0
#endif
#ifndef D_ENV_PDF_MUPDF_VERSION_PATCH
    #define D_ENV_PDF_MUPDF_VERSION_PATCH 0
#endif
#ifndef D_ENV_PDF_MUPDF_VERSION_STRING
    #if defined(FZ_VERSION)
        #define D_ENV_PDF_MUPDF_VERSION_STRING FZ_VERSION
    #else
        #define D_ENV_PDF_MUPDF_VERSION_STRING "unknown"
    #endif
#endif


// -----------------------------------------------------------------------------
// c.  PDFium
//
//   Google's render / parse library (also limited write).  Public header
// "fpdfview.h" (plus the fpdf_*.h family).  No stable public compile-time
// version macro; version is reported unknown under header-only detection.
// License: BSD-3-Clause / Apache-2.0.
// -----------------------------------------------------------------------------

#ifndef D_ENV_PDF_HAS_PDFIUM
    #if ( D_INTERNAL_PDF_PROBE(<fpdfview.h>) ||  \
          D_INTERNAL_PDF_PROBE(<public/fpdfview.h>) )
        #define D_ENV_PDF_HAS_PDFIUM 1
    #else
        #define D_ENV_PDF_HAS_PDFIUM 0
    #endif
#endif

#define D_ENV_PDF_PDFIUM_NAME    "PDFium"
#define D_ENV_PDF_PDFIUM_VENDOR  "Google / Foxit"
#define D_ENV_PDF_PDFIUM_LICENSE "BSD-3-Clause / Apache-2.0"
#define D_ENV_PDF_PDFIUM_HEADER  "fpdfview.h"

#ifndef D_ENV_PDF_PDFIUM_VERSION_MAJOR
    #define D_ENV_PDF_PDFIUM_VERSION_MAJOR 0
#endif
#ifndef D_ENV_PDF_PDFIUM_VERSION_MINOR
    #define D_ENV_PDF_PDFIUM_VERSION_MINOR 0
#endif
#ifndef D_ENV_PDF_PDFIUM_VERSION_PATCH
    #define D_ENV_PDF_PDFIUM_VERSION_PATCH 0
#endif
#ifndef D_ENV_PDF_PDFIUM_VERSION_STRING
    #define D_ENV_PDF_PDFIUM_VERSION_STRING "unknown"
#endif


// =============================================================================
// IV.  AGGREGATE CLASSIFICATION
// =============================================================================

// D_ENV_PDF_HAS_GENERATION_LIB
//   aggregate: 1 if ANY PDF generation (write) library is detected.
#ifndef D_ENV_PDF_HAS_GENERATION_LIB
    #if ( D_ENV_PDF_HAS_LIBHARU   ||  \
          D_ENV_PDF_HAS_PDFHUMMUS ||  \
          D_ENV_PDF_HAS_PODOFO    ||  \
          D_ENV_PDF_HAS_CAIRO_PDF )
        #define D_ENV_PDF_HAS_GENERATION_LIB 1
    #else
        #define D_ENV_PDF_HAS_GENERATION_LIB 0
    #endif
#endif

// D_ENV_PDF_HAS_RENDER_LIB
//   aggregate: 1 if ANY PDF render / parse library is detected.
#ifndef D_ENV_PDF_HAS_RENDER_LIB
    #if ( D_ENV_PDF_HAS_POPPLER ||  \
          D_ENV_PDF_HAS_MUPDF   ||  \
          D_ENV_PDF_HAS_PDFIUM  ||  \
          D_ENV_PDF_HAS_PODOFO )
        #define D_ENV_PDF_HAS_RENDER_LIB 1
    #else
        #define D_ENV_PDF_HAS_RENDER_LIB 0
    #endif
#endif

// D_ENV_PDF_HAS_ANY_LIB
//   aggregate: 1 if ANY PDF library at all is detected.
#ifndef D_ENV_PDF_HAS_ANY_LIB
    #if ( D_ENV_PDF_HAS_GENERATION_LIB ||  \
          D_ENV_PDF_HAS_RENDER_LIB )
        #define D_ENV_PDF_HAS_ANY_LIB 1
    #else
        #define D_ENV_PDF_HAS_ANY_LIB 0
    #endif
#endif

// D_ENV_PDF_GENERATION_LIB_COUNT
//   aggregate: number of distinct generation libraries detected.
#ifndef D_ENV_PDF_GENERATION_LIB_COUNT
    #define D_ENV_PDF_GENERATION_LIB_COUNT  \
        ( D_ENV_PDF_HAS_LIBHARU   +  \
          D_ENV_PDF_HAS_PDFHUMMUS +  \
          D_ENV_PDF_HAS_PODOFO    +  \
          D_ENV_PDF_HAS_CAIRO_PDF )
#endif


// =============================================================================
// V.   PREFERRED-BACKEND HINT
// =============================================================================

// PDF backend identifier constants.  D_ENV_PDF_BACKEND_NONE means no third
// party generation library was detected and callers should fall back to the
// djinterp built-in backend (builtin_pdf_backend).
#define D_ENV_PDF_BACKEND_NONE      0
#define D_ENV_PDF_BACKEND_LIBHARU   1
#define D_ENV_PDF_BACKEND_PDFHUMMUS 2
#define D_ENV_PDF_BACKEND_PODOFO    3
#define D_ENV_PDF_BACKEND_CAIRO     4

// D_ENV_PDF_PREFERRED_BACKEND
//   hint: a suggested generation backend given what is installed, ordered by
// suitability for djinterp's write-only document model and license
// friendliness: libHaru (ZLIB, pure C, maps cleanly to the common subset)
// first, then PDFHummus (Apache-2.0, native C++), then PoDoFo, then Cairo's
// PDF surface (only when its backend is actually compiled in).  This is only
// a hint; the integrator selects the real backend.  Pre-definable to force.
#ifndef D_ENV_PDF_PREFERRED_BACKEND
    #if D_ENV_PDF_HAS_LIBHARU
        #define D_ENV_PDF_PREFERRED_BACKEND      D_ENV_PDF_BACKEND_LIBHARU
        #define D_ENV_PDF_PREFERRED_BACKEND_NAME D_ENV_PDF_LIBHARU_NAME
    #elif D_ENV_PDF_HAS_PDFHUMMUS
        #define D_ENV_PDF_PREFERRED_BACKEND      D_ENV_PDF_BACKEND_PDFHUMMUS
        #define D_ENV_PDF_PREFERRED_BACKEND_NAME D_ENV_PDF_PDFHUMMUS_NAME
    #elif D_ENV_PDF_HAS_PODOFO
        #define D_ENV_PDF_PREFERRED_BACKEND      D_ENV_PDF_BACKEND_PODOFO
        #define D_ENV_PDF_PREFERRED_BACKEND_NAME D_ENV_PDF_PODOFO_NAME
    #elif ( D_ENV_PDF_HAS_CAIRO_PDF &&  \
            D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED )
        #define D_ENV_PDF_PREFERRED_BACKEND      D_ENV_PDF_BACKEND_CAIRO
        #define D_ENV_PDF_PREFERRED_BACKEND_NAME D_ENV_PDF_CAIRO_PDF_NAME
    #else
        #define D_ENV_PDF_PREFERRED_BACKEND      D_ENV_PDF_BACKEND_NONE
        #define D_ENV_PDF_PREFERRED_BACKEND_NAME "djinterp builtin"
    #endif
#endif


#endif  // DJINTERP_ENVIRONMENT_PDF_
