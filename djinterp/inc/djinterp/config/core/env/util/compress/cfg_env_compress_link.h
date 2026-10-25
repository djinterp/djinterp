/*******************************************************************************
* djinterp [config]                                      cfg_env_compress_link.h
*
* Configuration for env_compress_link.h, the auto-link directives under
* env/util/compress/.
*   Resolves its master switch, the library file each link request names,
* debug and release, and liblzma's static switch (decision 105 of the
* register, as decision 60 did for TLS). Each name is a single string
* literal, guarded with #ifndef, so a build-system -D wins; debug names follow
* vcpkg's "d"-suffixed layout.
*   targets:  env/util/compress/env_compress_link.h -> D_CFG_ENV_AUTOLINK,
*             D_CFG_ENV_AUTOLINK_*_LIB, D_CFG_ENV_AUTOLINK_LZMA_STATIC
*   requires: cfg_common.h (D_CFG_IS_BOOL); env.h (D_ENV_BUILD_DEBUG)
*
*
* path:      /inc/djinterp/config/core/env/util/compress/cfg_env_compress_link.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  AUTO-LINKING
    ------------
    1.  Master switch
         1.  D_CFG_ENV_AUTOLINK
    2.  Library names
         1.  D_CFG_ENV_AUTOLINK_*_LIB
    3.  liblzma
         1.  D_CFG_ENV_AUTOLINK_LZMA_STATIC
2.  VALIDATION
    ----------
    1.  Knob validation
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_UTIL_COMPRESS_CFG_ENV_COMPRESS_LINK_H
#define DJINTERP_CONFIG_CORE_ENV_UTIL_COMPRESS_CFG_ENV_COMPRESS_LINK_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset
#include "../../../../../env/env.h"  // D_ENV_BUILD_DEBUG


//==============================================================================
// 1.  AUTO-LINKING
//==============================================================================


// 1.1    Master switch
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_AUTOLINK
//   configuration: master toggle for pragma-based auto-linking, on the MSVC
// toolchain family. Defaults to 1; 0 routes every third-party link through
// the build system instead.
#ifndef D_CFG_ENV_AUTOLINK
    #define D_CFG_ENV_AUTOLINK 1
#endif  // D_CFG_ENV_AUTOLINK

// 1.2    Library names
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_ENV_AUTOLINK_*_LIB
//   configuration: the library file each link request names: ZLIB, LZMA,
// BZIP2, ZSTD, LZ4, the three BROTLI libraries, LIBARCHIVE, and LIBZIP; the
// debug names where D_ENV_BUILD_DEBUG is defined, the release names, without
// the "d" suffix, elsewhere.
#if defined(D_ENV_BUILD_DEBUG)
    #ifndef D_CFG_ENV_AUTOLINK_ZLIB_LIB
        #define D_CFG_ENV_AUTOLINK_ZLIB_LIB         "zlibd.lib"
    #endif  // D_CFG_ENV_AUTOLINK_ZLIB_LIB
    #ifndef D_CFG_ENV_AUTOLINK_LZMA_LIB
        #define D_CFG_ENV_AUTOLINK_LZMA_LIB         "lzmad.lib"
    #endif  // D_CFG_ENV_AUTOLINK_LZMA_LIB
    #ifndef D_CFG_ENV_AUTOLINK_BZIP2_LIB
        #define D_CFG_ENV_AUTOLINK_BZIP2_LIB        "bz2d.lib"
    #endif  // D_CFG_ENV_AUTOLINK_BZIP2_LIB
    #ifndef D_CFG_ENV_AUTOLINK_ZSTD_LIB
        #define D_CFG_ENV_AUTOLINK_ZSTD_LIB         "zstd.lib"
    #endif  // D_CFG_ENV_AUTOLINK_ZSTD_LIB
    #ifndef D_CFG_ENV_AUTOLINK_LZ4_LIB
        #define D_CFG_ENV_AUTOLINK_LZ4_LIB          "lz4d.lib"
    #endif  // D_CFG_ENV_AUTOLINK_LZ4_LIB
    #ifndef D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
        #define D_CFG_ENV_AUTOLINK_BROTLIENC_LIB    "brotliencd.lib"
    #endif  // D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
    #ifndef D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
        #define D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB    "brotlidecd.lib"
    #endif  // D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
    #ifndef D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
        #define D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB "brotlicommond.lib"
    #endif  // D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
    #ifndef D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
        #define D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB   "archive.lib"
    #endif  // D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
    #ifndef D_CFG_ENV_AUTOLINK_LIBZIP_LIB
        #define D_CFG_ENV_AUTOLINK_LIBZIP_LIB       "zip.lib"
    #endif  // D_CFG_ENV_AUTOLINK_LIBZIP_LIB
#else
    #ifndef D_CFG_ENV_AUTOLINK_ZLIB_LIB
        #define D_CFG_ENV_AUTOLINK_ZLIB_LIB         "zlib.lib"
    #endif  // D_CFG_ENV_AUTOLINK_ZLIB_LIB
    #ifndef D_CFG_ENV_AUTOLINK_LZMA_LIB
        #define D_CFG_ENV_AUTOLINK_LZMA_LIB         "lzma.lib"
    #endif  // D_CFG_ENV_AUTOLINK_LZMA_LIB
    #ifndef D_CFG_ENV_AUTOLINK_BZIP2_LIB
        #define D_CFG_ENV_AUTOLINK_BZIP2_LIB        "bz2.lib"
    #endif  // D_CFG_ENV_AUTOLINK_BZIP2_LIB
    #ifndef D_CFG_ENV_AUTOLINK_ZSTD_LIB
        #define D_CFG_ENV_AUTOLINK_ZSTD_LIB         "zstd.lib"
    #endif  // D_CFG_ENV_AUTOLINK_ZSTD_LIB
    #ifndef D_CFG_ENV_AUTOLINK_LZ4_LIB
        #define D_CFG_ENV_AUTOLINK_LZ4_LIB          "lz4.lib"
    #endif  // D_CFG_ENV_AUTOLINK_LZ4_LIB
    #ifndef D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
        #define D_CFG_ENV_AUTOLINK_BROTLIENC_LIB    "brotlienc.lib"
    #endif  // D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
    #ifndef D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
        #define D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB    "brotlidec.lib"
    #endif  // D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
    #ifndef D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
        #define D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB "brotlicommon.lib"
    #endif  // D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
    #ifndef D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
        #define D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB   "archive.lib"
    #endif  // D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
    #ifndef D_CFG_ENV_AUTOLINK_LIBZIP_LIB
        #define D_CFG_ENV_AUTOLINK_LIBZIP_LIB       "zip.lib"
    #endif  // D_CFG_ENV_AUTOLINK_LIBZIP_LIB
#endif  // D_ENV_BUILD_DEBUG

// 1.3    liblzma
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_ENV_AUTOLINK_LZMA_STATIC
//   configuration: 1 links liblzma statically: env_compress_link.h then
// defines LZMA_API_STATIC, which must precede <lzma.h> so the API is not
// decorated __declspec(dllimport). 0, the default, matches a dynamic install.
#ifndef D_CFG_ENV_AUTOLINK_LZMA_STATIC
    #define D_CFG_ENV_AUTOLINK_LZMA_STATIC 0
#endif  // D_CFG_ENV_AUTOLINK_LZMA_STATIC


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_ENV_AUTOLINK)
    #error "D_CFG_ENV_AUTOLINK must be 0 or 1"
#endif

#if !D_CFG_IS_BOOL(D_CFG_ENV_AUTOLINK_LZMA_STATIC)
    #error "D_CFG_ENV_AUTOLINK_LZMA_STATIC must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_UTIL_COMPRESS_CFG_ENV_COMPRESS_LINK_H
