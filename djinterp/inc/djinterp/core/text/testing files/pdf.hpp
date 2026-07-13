/******************************************************************************
* djinterp [text]                                                      pdf.hpp
*
* djinterp foundational PDF header:
*   This header provides the library-agnostic core of the djinterp PDF
* subsystem: the value types, the backend protocol, and a zero-dependency
* built-in backend.  It is the PDF analogue of print.hpp - a portable
* foundation that higher layers (pdf_template.hpp) build on without ever
* naming a concrete PDF library.
*
*   GREATEST COMMON SUBSET:
*   Every mainstream PDF engine (libHaru, PDFHummus, PoDoFo, cairo, ...)
* agrees on a small core of operations: open a document, start a page of a
* given size, place text at a point in a standard font and color, stroke
* lines, stroke/fill rectangles, attach document metadata, and serialize.
* That core - and only that core - is the abstract pdf_backend interface.
* Anything richer (image XObjects, TrueType embedding, annotations,
* outlines, encryption) is reported through pdf_capabilities and exposed,
* if at all, by backend-specific extension interfaces - never by this base.
*
*   BACKEND PROTOCOL:
*   pdf_backend is an abstract base class.  Unlike print.hpp, which dispatches
* on a hot per-character path and therefore uses compile-time SFINAE, a PDF
* backend sits on a cold document-assembly path and is genuinely a runtime
* plug-in (chosen by configuration, swapped without recompiling callers), so
* a virtual interface is the right tool.  The protocol is ALSO described
* structurally in pdf_template_traits.hpp, so a duck-typed backend that does
* not derive from pdf_backend is still recognized by is_pdf_backend<> and the
* C++20 concepts; inheritance is the recommended, not the required, route.
*
*   BUILT-IN BACKEND:
*   builtin_pdf_backend implements the common subset with no third-party
* dependency, emitting an uncompressed PDF 1.4 document that uses the
* standard-14 fonts (no font embedding).  It exists so the framework works
* out of the box; a production deployment can supply a libHaru / PDFHummus
* adapter deriving from pdf_backend and pass it to pdf_document.
*
*   COORDINATES:
*   pdf_document and pdf_backend operate in PDF user space: the origin is the
* bottom-left of the page and y increases upward, with all measures in points
* (1/72 inch).  Top-down flow layout is the concern of pdf_template.hpp, not
* of this foundation.
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h / env_*.h for version detection (D_NOEXCEPT,
* D_CONSTEXPR) and operating-system detection (creation-date timestamp).
*
*
* TABLE OF CONTENTS
* =================
* I.    UNITS & GEOMETRY
* II.   PAGE SIZES
* III.  COLOR
* IV.   FONTS
* V.    TEXT & PAINT OPTIONS
* VI.   CAPABILITIES
* VII.  BACKEND PROTOCOL
* VIII. INTERNAL: SERIALIZATION PRIMITIVES
* IX.   BUILT-IN BACKEND
* X.    DOCUMENT FACADE
*
*
* path:      /inc/djinterp/core/text/pdf/pdf.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_TEXT_PDF_
#define DJINTERP_TEXT_PDF_

// The PDF module is split into four layered submodules:
//   pdf_primitives.hpp       - geometry, page sizes, color, fonts, options, raster
//   pdf_backend.hpp          - capabilities + the abstract backend protocol
//   pdf_builtin_backend.hpp  - serialization primitives + the built-in backend
//   pdf_document.hpp         - the high-level document facade
// Include this umbrella for the whole module (the include surface is
// unchanged); include a submodule directly to pull in only what you need.
#include "./pdf_document.hpp"  // transitively includes the entire chain


#endif  // DJINTERP_TEXT_PDF_