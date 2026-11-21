/*******************************************************************************
* djinterp [config]                                                    cfg_pdf.h
*
*   Build-time defaults for the C++ PDF writer's built-in backend: whether it
* compresses its streams, and how hard. Each is a default only: a document's
* compression setting (DPdfCompression, through DPdfDocument::compression or
* pdf_options::compression) overrides it for that document.
*
*   targets:  cpp/util/pdf/pdf_builtin_backend.hpp
*             -> D_INTERNAL_PDF_COMPRESS, D_INTERNAL_PDF_COMPRESS_LEVEL
*   requires: cfg_common.h
*
* HOW TO USE
* ----------
*   Define a knob before the first djinterp header, or on the compiler's
* command line, for the whole program: -DD_CFG_PDF_COMPRESS=0 writes every
* PDF's streams raw unless a document asks otherwise.
*
* OPTIONS
* -------
*   A document's own setting wins over the knob: DPdfDocument::set_compression,
* pdf_template::compression, and pdf_options::compression, each by_default,
* flate or none.
*
* CONFIGURATION
* -------------
*   D_CFG_PDF_COMPRESS, 0 or 1 (1); D_CFG_PDF_COMPRESS_LEVEL, 1 to 9 (9).
* FlateDecode also needs zlib in the build, as the compress kernel finds it.
*
*
* path:      /inc/djinterp/config/cpp/util/pdf/cfg_pdf.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CPP_UTIL_PDF_CFG_PDF_H
#define DJINTERP_CONFIG_CPP_UTIL_PDF_CFG_PDF_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_* helpers, user overrides


/*
TABLE OF CONTENTS
=================
0.    PDF CONFIGURATION
      -----------------
      1.    Defaults                     (D_CFG_PDF_COMPRESS, _LEVEL)
      2.    Validation of the knobs
      3.    Derived values               (D_INTERNAL_PDF_*)
*/


// ===========================================================================
// 0.   PDF CONFIGURATION
// ===========================================================================

// --- 0.1  Defaults ---

// D_CFG_PDF_COMPRESS
//   brief: 1 (the default) writes every stream with FlateDecode: a page's
// content as it is, an image's samples after PNG's predictors, a filter
// chosen for each row. 0 writes every stream raw, as the backend did before
// it compressed. Where the build has no zlib, streams are raw whatever this
// says, and capabilities().compression says so.
#ifndef D_CFG_PDF_COMPRESS
#   define D_CFG_PDF_COMPRESS D_CFG_ON
#endif

// D_CFG_PDF_COMPRESS_LEVEL
//   brief: Flate's level for the streams, 1 (fastest) to 9 (smallest). 9 is
// the default: a document's streams are small, and written once.
#ifndef D_CFG_PDF_COMPRESS_LEVEL
#   define D_CFG_PDF_COMPRESS_LEVEL 9
#endif


// --- 0.2  Validation of the knobs ---

#if ( (D_CFG_NORM(D_CFG_PDF_COMPRESS) != 0) &&                                \
      (D_CFG_NORM(D_CFG_PDF_COMPRESS) != 1) )
#   error "D_CFG_PDF_COMPRESS must be 0 or 1"
#endif

#if ( (D_CFG_NORM(D_CFG_PDF_COMPRESS_LEVEL) < 1) ||                           \
      (D_CFG_NORM(D_CFG_PDF_COMPRESS_LEVEL) > 9) )
#   error "D_CFG_PDF_COMPRESS_LEVEL must be 1 to 9"
#endif


// --- 0.3  Derived values (D_INTERNAL_*) ---

// D_INTERNAL_PDF_COMPRESS
//   brief: 1 when a document that names no compression of its own asks for
// FlateDecode.
#define D_INTERNAL_PDF_COMPRESS                                               \
    D_CFG_IS_ON(D_CFG_PDF_COMPRESS)

// D_INTERNAL_PDF_COMPRESS_LEVEL
//   brief: the level as the integer the compress kernel takes.
#define D_INTERNAL_PDF_COMPRESS_LEVEL                                         \
    D_CFG_NORM(D_CFG_PDF_COMPRESS_LEVEL)


#endif  // DJINTERP_CONFIG_CPP_UTIL_PDF_CFG_PDF_H
