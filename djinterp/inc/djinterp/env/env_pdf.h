/*******************************************************************************
* djinterp [env]                                                       env_pdf.h
*
* djinterp PDF-library detection.
*   Compile-time detection of the PDF generation, rendering, and parsing
* libraries present in the developer's environment; the PDF analogue of
* env_c_lib.h. It probes for optional third-party headers and, where a library
* exposes one, extracts its version, so portable code can select a backend,
* gate features, and degrade gracefully without the build system telling it
* what is installed.
*   It depends on nothing else in the framework. It pairs with the djinterp
* PDF subsystem (pdf.hpp et al.) without depending on it, so it can drive
* backend selection before any djinterp PDF type is named.
*   Where the compiler supports __has_include (GCC, Clang, and MSVC in their
* relevant versions, plus any C23 / C++17 conformer), each library is detected
* by probing its public header; otherwise the result is "not detected" unless
* the integrator pre-defines the corresponding D_ENV_PDF_HAS_* macro. Every
* macro here is pre-definable, which serves cross builds, vendored copies on
* non-standard include paths, and testing.
*   Versions are reported only when the library exposes compile-time version
* macros and the translation unit has already included its header: this header
* never includes third-party headers, since probing presence must not pull a
* heavy dependency into every compile. Until then the numbers read 0 and the
* string reads "unknown".
*   Generation libraries are libHaru, PDFHummus (PDF-Writer), PoDoFo, and
* Cairo's PDF surface; render / parse libraries are Poppler, MuPDF, and
* PDFium. A library spanning both (PoDoFo creates and parses) is classified by
* its primary role, and the aggregates report whether any library of each kind
* is available.
*   For each library <LIB>: D_ENV_PDF_HAS_<LIB> is 1 when detected;
* D_ENV_PDF_<LIB>_NAME, _VENDOR, _LICENSE, and _HEADER are strings;
* D_ENV_PDF_<LIB>_VERSION_MAJOR, _MINOR, and _PATCH are integers (0 until the
* library's header is included); and D_ENV_PDF_<LIB>_VERSION_STRING is a
* string ("unknown" when unavailable).
*
* path:      /inc/djinterp/env/env_pdf.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PROBE INFRASTRUCTURE
    --------------------
    1.  Probing
         1.  D_ENV_PDF_HAS_INCLUDE_PROBE
         2.  D_INTERNAL_PDF_PROBE
2.  GENERATION LIBRARIES
    --------------------
    1.  libHaru
         1.  D_ENV_PDF_HAS_LIBHARU
         2.  libHaru metadata
         3.  libHaru version
    2.  PDFHummus / PDF-Writer
         1.  D_ENV_PDF_HAS_PDFHUMMUS
         2.  PDFHummus / PDF-Writer metadata
         3.  PDFHummus version
    3.  PoDoFo
         1.  D_ENV_PDF_HAS_PODOFO
         2.  PoDoFo metadata
         3.  PoDoFo version
    4.  Cairo (PDF surface)
         1.  D_ENV_PDF_HAS_CAIRO_PDF
         2.  Cairo (PDF surface) metadata
         3.  D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED
         4.  Cairo version
3.  RENDER / PARSE LIBRARIES
    ------------------------
    1.  Poppler
         1.  D_ENV_PDF_HAS_POPPLER
         2.  Poppler metadata
         3.  Poppler version
    2.  MuPDF
         1.  D_ENV_PDF_HAS_MUPDF
         2.  MuPDF metadata
         3.  MuPDF version
    3.  PDFium
         1.  D_ENV_PDF_HAS_PDFIUM
         2.  PDFium metadata
         3.  PDFium version
4.  AGGREGATE CLASSIFICATION
    ------------------------
    1.  Aggregates
         1.  D_ENV_PDF_HAS_GENERATION_LIB
         2.  D_ENV_PDF_HAS_RENDER_LIB
         3.  D_ENV_PDF_HAS_ANY_LIB
         4.  D_ENV_PDF_GENERATION_LIB_COUNT
5.  PREFERRED-BACKEND HINT
    ----------------------
    1.  Backend identifiers
         1.  D_ENV_PDF_BACKEND_*
              1.  D_ENV_PDF_BACKEND_NONE
              2.  D_ENV_PDF_BACKEND_LIBHARU
              3.  D_ENV_PDF_BACKEND_PDFHUMMUS
              4.  D_ENV_PDF_BACKEND_PODOFO
              5.  D_ENV_PDF_BACKEND_CAIRO
    2.  Preferred backend
         1.  D_ENV_PDF_PREFERRED_BACKEND
*/

#ifndef DJINTERP_ENV_ENV_PDF_H
#define DJINTERP_ENV_ENV_PDF_H 1


//==============================================================================
// 1.  PROBE INFRASTRUCTURE
//==============================================================================


// 1.1    Probing
//------------------------------------------------------------------------------
// 1.1.1
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
#endif  // D_ENV_PDF_HAS_INCLUDE_PROBE

// 1.1.2
// D_INTERNAL_PDF_PROBE
//   helper: (header) expands to 1 if `header` is includable, else 0.  When the
// compiler lacks __has_include the probe is conservatively 0 (callers may
// still pre-define a specific D_ENV_PDF_HAS_* macro to override).
#if D_ENV_PDF_HAS_INCLUDE_PROBE
    #define D_INTERNAL_PDF_PROBE(header) __has_include(header)
#else
    #define D_INTERNAL_PDF_PROBE(header) 0
#endif


//==============================================================================
// 2.  GENERATION LIBRARIES
//==============================================================================


// 2.1    libHaru
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_PDF_HAS_LIBHARU
//   feature: 1 when libHaru, the Haru Free PDF Library, is detected. Pure-C
// generation library. Public header "hpdf.h"; version macros live in
// "hpdf_version.h" (HPDF_MAJOR_VERSION / HPDF_MINOR_VERSION /
// HPDF_BUGFIX_VERSION, plus HPDF_VERSION_TEXT). Historically hpdf.h pulled in
// hpdf_version.h, but some master builds stopped propagating it, so the version
// is probed from hpdf_version.h independently of presence detection.
#ifndef D_ENV_PDF_HAS_LIBHARU
    #if D_INTERNAL_PDF_PROBE(<hpdf.h>)
        #define D_ENV_PDF_HAS_LIBHARU 1
    #else
        #define D_ENV_PDF_HAS_LIBHARU 0
    #endif
#endif  // D_ENV_PDF_HAS_LIBHARU

// 2.1.2
// libHaru metadata
//   constant: D_ENV_PDF_LIBHARU_NAME, _VENDOR, _LICENSE, and _HEADER: the
// library's name, vendor, license, and probed header, as strings.
#define D_ENV_PDF_LIBHARU_NAME    "libHaru"
#define D_ENV_PDF_LIBHARU_VENDOR  "Haru / Takeshi Kanno et al."
#define D_ENV_PDF_LIBHARU_LICENSE "ZLIB/libpng"
#define D_ENV_PDF_LIBHARU_HEADER  "hpdf.h"

// 2.1.3
// libHaru version
//   constant: D_ENV_PDF_LIBHARU_VERSION_MAJOR, _MINOR, _PATCH, and _STRING.
// Populated only if a libHaru header that defines the version macros (hpdf.h
// or hpdf_version.h) was included before this one; otherwise 0 and "unknown".
#ifndef D_ENV_PDF_LIBHARU_VERSION_MAJOR
    #if defined(HPDF_MAJOR_VERSION)
        #define D_ENV_PDF_LIBHARU_VERSION_MAJOR HPDF_MAJOR_VERSION
    #else
        #define D_ENV_PDF_LIBHARU_VERSION_MAJOR 0
    #endif
#endif  // D_ENV_PDF_LIBHARU_VERSION_MAJOR

#ifndef D_ENV_PDF_LIBHARU_VERSION_MINOR
    #if defined(HPDF_MINOR_VERSION)
        #define D_ENV_PDF_LIBHARU_VERSION_MINOR HPDF_MINOR_VERSION
    #else
        #define D_ENV_PDF_LIBHARU_VERSION_MINOR 0
    #endif
#endif  // D_ENV_PDF_LIBHARU_VERSION_MINOR

#ifndef D_ENV_PDF_LIBHARU_VERSION_PATCH
    #if defined(HPDF_BUGFIX_VERSION)
        #define D_ENV_PDF_LIBHARU_VERSION_PATCH HPDF_BUGFIX_VERSION
    #else
        #define D_ENV_PDF_LIBHARU_VERSION_PATCH 0
    #endif
#endif  // D_ENV_PDF_LIBHARU_VERSION_PATCH

#ifndef D_ENV_PDF_LIBHARU_VERSION_STRING
    #if defined(HPDF_VERSION_TEXT)
        #define D_ENV_PDF_LIBHARU_VERSION_STRING HPDF_VERSION_TEXT
    #else
        #define D_ENV_PDF_LIBHARU_VERSION_STRING "unknown"
    #endif
#endif  // D_ENV_PDF_LIBHARU_VERSION_STRING

// 2.2    PDFHummus / PDF-Writer
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_PDF_HAS_PDFHUMMUS
//   feature: 1 when PDFHummus (PDF-Writer) is detected. Native-C++ high-
// performance write + parse library. Public header "PDFWriter.h". Upstream does
// not expose a stable compile-time version macro, so version is reported as
// unknown under header-only detection; integrators can pre-define the version
// macros if they track it.
#ifndef D_ENV_PDF_HAS_PDFHUMMUS
    #if ( D_INTERNAL_PDF_PROBE(<PDFWriter.h>) ||  \
          D_INTERNAL_PDF_PROBE(<PDFWriter/PDFWriter.h>) )
        #define D_ENV_PDF_HAS_PDFHUMMUS 1
    #else
        #define D_ENV_PDF_HAS_PDFHUMMUS 0
    #endif
#endif  // D_ENV_PDF_HAS_PDFHUMMUS

// 2.2.2
// PDFHummus / PDF-Writer metadata
//   constant: D_ENV_PDF_PDFHUMMUS_NAME, _VENDOR, _LICENSE, and _HEADER: the
// library's name, vendor, license, and probed header, as strings.
#define D_ENV_PDF_PDFHUMMUS_NAME    "PDFHummus (PDF-Writer)"
#define D_ENV_PDF_PDFHUMMUS_VENDOR  "Gal Kahana"
#define D_ENV_PDF_PDFHUMMUS_LICENSE "Apache-2.0"
#define D_ENV_PDF_PDFHUMMUS_HEADER  "PDFWriter.h"

// 2.2.3
// PDFHummus version
//   constant: D_ENV_PDF_PDFHUMMUS_VERSION_MAJOR, _MINOR, _PATCH, and _STRING.
// Always 0 and "unknown" unless pre-defined; see 2.2.1.
#ifndef D_ENV_PDF_PDFHUMMUS_VERSION_MAJOR
    #define D_ENV_PDF_PDFHUMMUS_VERSION_MAJOR 0
#endif  // D_ENV_PDF_PDFHUMMUS_VERSION_MAJOR
#ifndef D_ENV_PDF_PDFHUMMUS_VERSION_MINOR
    #define D_ENV_PDF_PDFHUMMUS_VERSION_MINOR 0
#endif  // D_ENV_PDF_PDFHUMMUS_VERSION_MINOR
#ifndef D_ENV_PDF_PDFHUMMUS_VERSION_PATCH
    #define D_ENV_PDF_PDFHUMMUS_VERSION_PATCH 0
#endif  // D_ENV_PDF_PDFHUMMUS_VERSION_PATCH
#ifndef D_ENV_PDF_PDFHUMMUS_VERSION_STRING
    #define D_ENV_PDF_PDFHUMMUS_VERSION_STRING "unknown"
#endif  // D_ENV_PDF_PDFHUMMUS_VERSION_STRING

// 2.3    PoDoFo
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_PDF_HAS_PODOFO
//   feature: 1 when PoDoFo is detected. C++ create / parse / modify library.
// The modern API header is "podofo/podofo.h"; older trees used
// "podofo/base/PdfDefines.h". Version macros: PODOFO_VERSION_MAJOR / MINOR /
// PATCH, with PODOFO_VERSION packed as 0xMMmmpp and PODOFO_MAKE_VERSION(M,m,p).
// Modern PoDoFo (0.10+) requires C++17. License: LGPL-2.0+/MPL-2.0 (library);
// GPL (tools).
#ifndef D_ENV_PDF_HAS_PODOFO
    #if ( D_INTERNAL_PDF_PROBE(<podofo/podofo.h>) ||  \
          D_INTERNAL_PDF_PROBE(<podofo/base/PdfDefines.h>) )
        #define D_ENV_PDF_HAS_PODOFO 1
    #else
        #define D_ENV_PDF_HAS_PODOFO 0
    #endif
#endif  // D_ENV_PDF_HAS_PODOFO

// 2.3.2
// PoDoFo metadata
//   constant: D_ENV_PDF_PODOFO_NAME, _VENDOR, _LICENSE, and _HEADER: the
// library's name, vendor, license, and probed header, as strings.
#define D_ENV_PDF_PODOFO_NAME    "PoDoFo"
#define D_ENV_PDF_PODOFO_VENDOR  "PoDoFo project"
#define D_ENV_PDF_PODOFO_LICENSE "LGPL-2.0+/MPL-2.0"
#define D_ENV_PDF_PODOFO_HEADER  "podofo/podofo.h"

// 2.3.3
// PoDoFo version
//   constant: D_ENV_PDF_PODOFO_VERSION_MAJOR, _MINOR, _PATCH, and _STRING. The
// numbers come from PODOFO_VERSION_* once PoDoFo's header is included; the
// string is always "unknown" unless pre-defined.
#ifndef D_ENV_PDF_PODOFO_VERSION_MAJOR
    #if defined(PODOFO_VERSION_MAJOR)
        #define D_ENV_PDF_PODOFO_VERSION_MAJOR PODOFO_VERSION_MAJOR
    #else
        #define D_ENV_PDF_PODOFO_VERSION_MAJOR 0
    #endif
#endif  // D_ENV_PDF_PODOFO_VERSION_MAJOR

#ifndef D_ENV_PDF_PODOFO_VERSION_MINOR
    #if defined(PODOFO_VERSION_MINOR)
        #define D_ENV_PDF_PODOFO_VERSION_MINOR PODOFO_VERSION_MINOR
    #else
        #define D_ENV_PDF_PODOFO_VERSION_MINOR 0
    #endif
#endif  // D_ENV_PDF_PODOFO_VERSION_MINOR

#ifndef D_ENV_PDF_PODOFO_VERSION_PATCH
    #if defined(PODOFO_VERSION_PATCH)
        #define D_ENV_PDF_PODOFO_VERSION_PATCH PODOFO_VERSION_PATCH
    #else
        #define D_ENV_PDF_PODOFO_VERSION_PATCH 0
    #endif
#endif  // D_ENV_PDF_PODOFO_VERSION_PATCH

#ifndef D_ENV_PDF_PODOFO_VERSION_STRING
    #define D_ENV_PDF_PODOFO_VERSION_STRING "unknown"
#endif  // D_ENV_PDF_PODOFO_VERSION_STRING

// 2.4    Cairo (PDF surface)
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_PDF_HAS_CAIRO_PDF
//   feature: 1 when Cairo's PDF surface header is detected. Cairo is a 2D
// graphics library with an optional PDF backend selected at its own build time.
// Presence of the PDF surface header does not guarantee the PDF backend was
// compiled in; CAIRO_HAS_PDF_SURFACE (from cairo's cairo-features.h, available
// once <cairo.h> is included) is the authoritative runtime-capable gate.
// Version macros: CAIRO_VERSION_MAJOR / MINOR / MICRO and CAIRO_VERSION_STRING.
// License: LGPL-2.1 / MPL-1.1.
#ifndef D_ENV_PDF_HAS_CAIRO_PDF
    #if D_INTERNAL_PDF_PROBE(<cairo-pdf.h>)
        #define D_ENV_PDF_HAS_CAIRO_PDF 1
    #else
        #define D_ENV_PDF_HAS_CAIRO_PDF 0
    #endif
#endif  // D_ENV_PDF_HAS_CAIRO_PDF

// 2.4.2
// Cairo (PDF surface) metadata
//   constant: D_ENV_PDF_CAIRO_PDF_NAME, _VENDOR, _LICENSE, and _HEADER: the
// library's name, vendor, license, and probed header, as strings.
#define D_ENV_PDF_CAIRO_PDF_NAME    "Cairo (PDF surface)"
#define D_ENV_PDF_CAIRO_PDF_VENDOR  "cairographics.org"
#define D_ENV_PDF_CAIRO_PDF_LICENSE "LGPL-2.1/MPL-1.1"
#define D_ENV_PDF_CAIRO_PDF_HEADER  "cairo-pdf.h"

// 2.4.3
// D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED
//   capability: 1 only when cairo's headers report the PDF surface backend
// was actually compiled in (requires <cairo.h> included beforehand).
#ifndef D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED
    #if ( defined(CAIRO_HAS_PDF_SURFACE) &&                                   \
          (CAIRO_HAS_PDF_SURFACE) )
        #define D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED 1
    #else
        #define D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED 0
    #endif
#endif  // D_ENV_PDF_CAIRO_PDF_BACKEND_ENABLED

// 2.4.4
// Cairo version
//   constant: D_ENV_PDF_CAIRO_PDF_VERSION_MAJOR, _MINOR, _PATCH, and _STRING.
// From CAIRO_VERSION_* once <cairo.h> is included; otherwise 0 and "unknown".
#ifndef D_ENV_PDF_CAIRO_PDF_VERSION_MAJOR
    #if defined(CAIRO_VERSION_MAJOR)
        #define D_ENV_PDF_CAIRO_PDF_VERSION_MAJOR CAIRO_VERSION_MAJOR
    #else
        #define D_ENV_PDF_CAIRO_PDF_VERSION_MAJOR 0
    #endif
#endif  // D_ENV_PDF_CAIRO_PDF_VERSION_MAJOR

#ifndef D_ENV_PDF_CAIRO_PDF_VERSION_MINOR
    #if defined(CAIRO_VERSION_MINOR)
        #define D_ENV_PDF_CAIRO_PDF_VERSION_MINOR CAIRO_VERSION_MINOR
    #else
        #define D_ENV_PDF_CAIRO_PDF_VERSION_MINOR 0
    #endif
#endif  // D_ENV_PDF_CAIRO_PDF_VERSION_MINOR

#ifndef D_ENV_PDF_CAIRO_PDF_VERSION_PATCH
    #if defined(CAIRO_VERSION_MICRO)
        #define D_ENV_PDF_CAIRO_PDF_VERSION_PATCH CAIRO_VERSION_MICRO
    #else
        #define D_ENV_PDF_CAIRO_PDF_VERSION_PATCH 0
    #endif
#endif  // D_ENV_PDF_CAIRO_PDF_VERSION_PATCH

#ifndef D_ENV_PDF_CAIRO_PDF_VERSION_STRING
    #if defined(CAIRO_VERSION_STRING)
        #define D_ENV_PDF_CAIRO_PDF_VERSION_STRING CAIRO_VERSION_STRING
    #else
        #define D_ENV_PDF_CAIRO_PDF_VERSION_STRING "unknown"
    #endif
#endif  // D_ENV_PDF_CAIRO_PDF_VERSION_STRING


//==============================================================================
// 3.  RENDER / PARSE LIBRARIES
//==============================================================================


// 3.1    Poppler
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_PDF_HAS_POPPLER
//   feature: 1 when Poppler is detected. Rendering / parsing library (the cpp
// wrapper header is "poppler/cpp/poppler-document.h"; the lower C++ API lives
// under "poppler/PDFDoc.h"). Version: POPPLER_VERSION / _MAJOR / _MINOR /
// _MICRO from poppler/cpp/poppler-version.h. License: GPL-2.0+.
#ifndef D_ENV_PDF_HAS_POPPLER
    #if ( D_INTERNAL_PDF_PROBE(<poppler/cpp/poppler-document.h>) ||  \
          D_INTERNAL_PDF_PROBE(<poppler/PDFDoc.h>) )
        #define D_ENV_PDF_HAS_POPPLER 1
    #else
        #define D_ENV_PDF_HAS_POPPLER 0
    #endif
#endif  // D_ENV_PDF_HAS_POPPLER

// 3.1.2
// Poppler metadata
//   constant: D_ENV_PDF_POPPLER_NAME, _VENDOR, _LICENSE, and _HEADER: the
// library's name, vendor, license, and probed header, as strings.
#define D_ENV_PDF_POPPLER_NAME    "Poppler"
#define D_ENV_PDF_POPPLER_VENDOR  "freedesktop.org"
#define D_ENV_PDF_POPPLER_LICENSE "GPL-2.0+"
#define D_ENV_PDF_POPPLER_HEADER  "poppler/cpp/poppler-document.h"

// 3.1.3
// Poppler version
//   constant: D_ENV_PDF_POPPLER_VERSION_MAJOR, _MINOR, _PATCH, and _STRING.
// From POPPLER_VERSION_* once Poppler's version header is included; otherwise
// 0 and "unknown".
#ifndef D_ENV_PDF_POPPLER_VERSION_MAJOR
    #if defined(POPPLER_VERSION_MAJOR)
        #define D_ENV_PDF_POPPLER_VERSION_MAJOR POPPLER_VERSION_MAJOR
    #else
        #define D_ENV_PDF_POPPLER_VERSION_MAJOR 0
    #endif
#endif  // D_ENV_PDF_POPPLER_VERSION_MAJOR

#ifndef D_ENV_PDF_POPPLER_VERSION_MINOR
    #if defined(POPPLER_VERSION_MINOR)
        #define D_ENV_PDF_POPPLER_VERSION_MINOR POPPLER_VERSION_MINOR
    #else
        #define D_ENV_PDF_POPPLER_VERSION_MINOR 0
    #endif
#endif  // D_ENV_PDF_POPPLER_VERSION_MINOR

#ifndef D_ENV_PDF_POPPLER_VERSION_PATCH
    #if defined(POPPLER_VERSION_MICRO)
        #define D_ENV_PDF_POPPLER_VERSION_PATCH POPPLER_VERSION_MICRO
    #else
        #define D_ENV_PDF_POPPLER_VERSION_PATCH 0
    #endif
#endif  // D_ENV_PDF_POPPLER_VERSION_PATCH

#ifndef D_ENV_PDF_POPPLER_VERSION_STRING
    #if defined(POPPLER_VERSION)
        #define D_ENV_PDF_POPPLER_VERSION_STRING POPPLER_VERSION
    #else
        #define D_ENV_PDF_POPPLER_VERSION_STRING "unknown"
    #endif
#endif  // D_ENV_PDF_POPPLER_VERSION_STRING

// 3.2    MuPDF
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_PDF_HAS_MUPDF
//   feature: 1 when MuPDF is detected. Lightweight render / parse (and some
// write) library. Public header "mupdf/fitz.h". MuPDF does not provide a stable
// compile-time version macro (version is a build-time string), so the version
// is reported unknown under header-only detection. License: AGPL-3.0 (or
// commercial).
#ifndef D_ENV_PDF_HAS_MUPDF
    #if D_INTERNAL_PDF_PROBE(<mupdf/fitz.h>)
        #define D_ENV_PDF_HAS_MUPDF 1
    #else
        #define D_ENV_PDF_HAS_MUPDF 0
    #endif
#endif  // D_ENV_PDF_HAS_MUPDF

// 3.2.2
// MuPDF metadata
//   constant: D_ENV_PDF_MUPDF_NAME, _VENDOR, _LICENSE, and _HEADER: the
// library's name, vendor, license, and probed header, as strings.
#define D_ENV_PDF_MUPDF_NAME    "MuPDF"
#define D_ENV_PDF_MUPDF_VENDOR  "Artifex Software"
#define D_ENV_PDF_MUPDF_LICENSE "AGPL-3.0 / commercial"
#define D_ENV_PDF_MUPDF_HEADER  "mupdf/fitz.h"

// 3.2.3
// MuPDF version
//   constant: D_ENV_PDF_MUPDF_VERSION_MAJOR, _MINOR, _PATCH, and _STRING. The
// numbers are always 0 unless pre-defined; the string comes from FZ_VERSION
// once MuPDF's header is included.
#ifndef D_ENV_PDF_MUPDF_VERSION_MAJOR
    #define D_ENV_PDF_MUPDF_VERSION_MAJOR 0
#endif  // D_ENV_PDF_MUPDF_VERSION_MAJOR
#ifndef D_ENV_PDF_MUPDF_VERSION_MINOR
    #define D_ENV_PDF_MUPDF_VERSION_MINOR 0
#endif  // D_ENV_PDF_MUPDF_VERSION_MINOR
#ifndef D_ENV_PDF_MUPDF_VERSION_PATCH
    #define D_ENV_PDF_MUPDF_VERSION_PATCH 0
#endif  // D_ENV_PDF_MUPDF_VERSION_PATCH
#ifndef D_ENV_PDF_MUPDF_VERSION_STRING
    #if defined(FZ_VERSION)
        #define D_ENV_PDF_MUPDF_VERSION_STRING FZ_VERSION
    #else
        #define D_ENV_PDF_MUPDF_VERSION_STRING "unknown"
    #endif
#endif  // D_ENV_PDF_MUPDF_VERSION_STRING

// 3.3    PDFium
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_PDF_HAS_PDFIUM
//   feature: 1 when PDFium is detected. Google's render / parse library (also
// limited write). Public header "fpdfview.h" (plus the fpdf_*.h family). No
// stable public compile-time version macro; version is reported unknown under
// header-only detection. License: BSD-3-Clause / Apache-2.0.
#ifndef D_ENV_PDF_HAS_PDFIUM
    #if ( D_INTERNAL_PDF_PROBE(<fpdfview.h>) ||  \
          D_INTERNAL_PDF_PROBE(<public/fpdfview.h>) )
        #define D_ENV_PDF_HAS_PDFIUM 1
    #else
        #define D_ENV_PDF_HAS_PDFIUM 0
    #endif
#endif  // D_ENV_PDF_HAS_PDFIUM

// 3.3.2
// PDFium metadata
//   constant: D_ENV_PDF_PDFIUM_NAME, _VENDOR, _LICENSE, and _HEADER: the
// library's name, vendor, license, and probed header, as strings.
#define D_ENV_PDF_PDFIUM_NAME    "PDFium"
#define D_ENV_PDF_PDFIUM_VENDOR  "Google / Foxit"
#define D_ENV_PDF_PDFIUM_LICENSE "BSD-3-Clause / Apache-2.0"
#define D_ENV_PDF_PDFIUM_HEADER  "fpdfview.h"

// 3.3.3
// PDFium version
//   constant: D_ENV_PDF_PDFIUM_VERSION_MAJOR, _MINOR, _PATCH, and _STRING.
// Always 0 and "unknown" unless pre-defined; see 3.3.1.
#ifndef D_ENV_PDF_PDFIUM_VERSION_MAJOR
    #define D_ENV_PDF_PDFIUM_VERSION_MAJOR 0
#endif  // D_ENV_PDF_PDFIUM_VERSION_MAJOR
#ifndef D_ENV_PDF_PDFIUM_VERSION_MINOR
    #define D_ENV_PDF_PDFIUM_VERSION_MINOR 0
#endif  // D_ENV_PDF_PDFIUM_VERSION_MINOR
#ifndef D_ENV_PDF_PDFIUM_VERSION_PATCH
    #define D_ENV_PDF_PDFIUM_VERSION_PATCH 0
#endif  // D_ENV_PDF_PDFIUM_VERSION_PATCH
#ifndef D_ENV_PDF_PDFIUM_VERSION_STRING
    #define D_ENV_PDF_PDFIUM_VERSION_STRING "unknown"
#endif  // D_ENV_PDF_PDFIUM_VERSION_STRING


//==============================================================================
// 4.  AGGREGATE CLASSIFICATION
//==============================================================================


// 4.1    Aggregates
//------------------------------------------------------------------------------
// 4.1.1
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
#endif  // D_ENV_PDF_HAS_GENERATION_LIB

// 4.1.2
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
#endif  // D_ENV_PDF_HAS_RENDER_LIB

// 4.1.3
// D_ENV_PDF_HAS_ANY_LIB
//   aggregate: 1 if ANY PDF library at all is detected.
#ifndef D_ENV_PDF_HAS_ANY_LIB
    #if ( D_ENV_PDF_HAS_GENERATION_LIB ||  \
          D_ENV_PDF_HAS_RENDER_LIB )
        #define D_ENV_PDF_HAS_ANY_LIB 1
    #else
        #define D_ENV_PDF_HAS_ANY_LIB 0
    #endif
#endif  // D_ENV_PDF_HAS_ANY_LIB

// 4.1.4
// D_ENV_PDF_GENERATION_LIB_COUNT
//   aggregate: number of distinct generation libraries detected.
#ifndef D_ENV_PDF_GENERATION_LIB_COUNT
    #define D_ENV_PDF_GENERATION_LIB_COUNT  \
        ( D_ENV_PDF_HAS_LIBHARU   +  \
          D_ENV_PDF_HAS_PDFHUMMUS +  \
          D_ENV_PDF_HAS_PODOFO    +  \
          D_ENV_PDF_HAS_CAIRO_PDF )
#endif  // D_ENV_PDF_GENERATION_LIB_COUNT


//==============================================================================
// 5.  PREFERRED-BACKEND HINT
//==============================================================================


// 5.1    Backend identifiers
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_PDF_BACKEND_*
//   constant: stable small integers naming a generation backend.

// 5.1.1.1
// D_ENV_PDF_BACKEND_NONE
//   constant: identifies no third-party generation library, meaning the
// framework's built-in backend (builtin_pdf_backend).
#define D_ENV_PDF_BACKEND_NONE      0

// 5.1.1.2
// D_ENV_PDF_BACKEND_LIBHARU
//   constant: identifies libHaru.
#define D_ENV_PDF_BACKEND_LIBHARU   1

// 5.1.1.3
// D_ENV_PDF_BACKEND_PDFHUMMUS
//   constant: identifies PDFHummus.
#define D_ENV_PDF_BACKEND_PDFHUMMUS 2

// 5.1.1.4
// D_ENV_PDF_BACKEND_PODOFO
//   constant: identifies PoDoFo.
#define D_ENV_PDF_BACKEND_PODOFO    3

// 5.1.1.5
// D_ENV_PDF_BACKEND_CAIRO
//   constant: identifies Cairo's PDF surface.
#define D_ENV_PDF_BACKEND_CAIRO     4

// 5.2    Preferred backend
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_PDF_PREFERRED_BACKEND
//   hint: a suggested generation backend, with D_ENV_PDF_PREFERRED_BACKEND_NAME
// naming it, given what is installed. Ordered by
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
#endif  // D_ENV_PDF_PREFERRED_BACKEND


#endif  // DJINTERP_ENV_ENV_PDF_H
