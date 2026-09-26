/*******************************************************************************
* djinterp [env]                                             env_compress_link.h
*
* djinterp auto-link directives for detected third-party libraries.
*   env_compress.h and env_archive.h detect codec and archive libraries by
* header presence (__has_include), but presence alone does not put a library's
* import library on the link line. On the MSVC toolchain family (cl and
* clang-cl, both of which define _MSC_VER and honour #pragma comment(lib,
* ...)) this header turns each D_ENV_COMPRESSION_HAVE_* and
* D_ENV_ARCHIVE_HAVE_* flag that is set into a link request for the matching
* library, so a translation unit that calls the codec and archive facades
* links with no build-system edit. On every other toolchain it expands to
* nothing, and linking is left to the build system (e.g. -lz -llzma).
*   The header emits nothing on its own: it reacts to whichever of
* env_compress.h and env_archive.h the translation unit has already included,
* detected through their include guards, and only for libraries that were
* actually detected. Include it from an implementation unit after the facade
* and env headers, for example:
*     #include "../util/compress.hpp"         // pulls in env_compress.h
*     #include "../env/env_compress_link.h"  // requests zlib / lzma / ... links
*   Because the requests are confined to the units that include this header
* (the codec and archive .c / .cpp files), a header that merely reads the
* capability macros never gains a link dependency.
*   Library file names default to the common vcpkg layout and are debug /
* release aware. Override any D_CFG_ENV_AUTOLINK_*_LIB macro from the build
* system when a library is installed under a different name, for instance a
* static "zlibstatic.lib" or a distribution-specific debug suffix.
*   liblzma decorates its API __declspec(dllimport) unless LZMA_API_STATIC is
* defined before <lzma.h>. The default here is dynamic (an import library),
* matching a dynamic vcpkg install; define D_CFG_ENV_AUTOLINK_LZMA_STATIC to 1
* to link a static liblzma instead, which also defines LZMA_API_STATIC. zlib
* needs no such switch: static and import zlib both resolve as long as
* ZLIB_DLL matches the artifact being linked.
*
* path:      /inc/djinterp/env/env_compress_link.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.04
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONFIGURATION
    -------------
    1.  Master switch
         1.  D_CFG_ENV_AUTOLINK
    2.  MSVC gate and link-request helper
         1.  D_INTERNAL_AUTOLINK_LIB
2.  LIBRARY NAME DEFAULTS
    ---------------------
    1.  Debug builds
         1.  D_CFG_ENV_AUTOLINK_*_LIB (debug)
    2.  Release builds
         1.  D_CFG_ENV_AUTOLINK_*_LIB (release)
3.  CODEC LINK REQUESTS
    -------------------
    1.  Codec libraries
         1.  D_CFG_ENV_AUTOLINK_LZMA_STATIC
         2.  Codec link requests
4.  ARCHIVE LINK REQUESTS
    ---------------------
    1.  Archive libraries
         1.  Archive link requests
*/

#ifndef DJINTERP_ENV_ENV_COMPRESS_LINK_H
#define DJINTERP_ENV_ENV_COMPRESS_LINK_H 1

// djinterp
#include "./env.h"  // D_ENV_BUILD_DEBUG


//==============================================================================
// 1.  CONFIGURATION
//==============================================================================


// 1.1    Master switch
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_AUTOLINK
//   configuration: master toggle for pragma-based auto-linking. defaults to 1;
// define to 0 from the build system to route every third-party link through
// the build system instead.
#ifndef D_CFG_ENV_AUTOLINK
    #define D_CFG_ENV_AUTOLINK 1
#endif  // D_CFG_ENV_AUTOLINK

// 1.2    MSVC gate and link-request helper
//------------------------------------------------------------------------------
//   the rest of this header is a no-op unless the compiler is MSVC-family (cl
// or clang-cl, which define _MSC_VER and accept `#pragma comment(lib, ...)`)
// and the master toggle is on. Every other toolchain skips the block and
// relies on the build system for linking. The test reads _MSC_VER, not
// D_ENV_COMPILER_MSVC_FAMILY: a link pragma needs the real toolchain, and the
// env macro also follows a simulated one.
#if ( defined(_MSC_VER) &&                                                    \
      (D_CFG_ENV_AUTOLINK) )

    // 1.2.1
    // D_INTERNAL_AUTOLINK_LIB
    //   macro: emit a link request for the library named by `name`, a string
    // literal (or a macro expanding to one). __pragma is used, not #pragma, so
    // that `name` undergoes macro replacement.
    #define D_INTERNAL_AUTOLINK_LIB(name) __pragma(comment(lib, name))


//==============================================================================
// 2.  LIBRARY NAME DEFAULTS
//==============================================================================
// Each name is a single string literal, so it can be handed straight to the
// comment pragma. Each default is guarded with #ifndef, so a build-system
// override (a -D of any D_CFG_ENV_AUTOLINK_*_LIB macro) wins. Debug names
// follow vcpkg's "d"-suffixed layout; adjust through the override macros
// where a library deviates.


    #if defined(D_ENV_BUILD_DEBUG)

// 2.1    Debug builds
//------------------------------------------------------------------------------
        // 2.1.1
        // D_CFG_ENV_AUTOLINK_*_LIB (debug)
        //   configuration: the library file each link request names: ZLIB,
        // LZMA, BZIP2, ZSTD, LZ4, the three BROTLI libraries, LIBARCHIVE, and
        // LIBZIP.
        #ifndef D_CFG_ENV_AUTOLINK_ZLIB_LIB
            #define D_CFG_ENV_AUTOLINK_ZLIB_LIB          "zlibd.lib"
        #endif  // D_CFG_ENV_AUTOLINK_ZLIB_LIB
        #ifndef D_CFG_ENV_AUTOLINK_LZMA_LIB
            #define D_CFG_ENV_AUTOLINK_LZMA_LIB          "lzmad.lib"
        #endif  // D_CFG_ENV_AUTOLINK_LZMA_LIB
        #ifndef D_CFG_ENV_AUTOLINK_BZIP2_LIB
            #define D_CFG_ENV_AUTOLINK_BZIP2_LIB         "bz2d.lib"
        #endif  // D_CFG_ENV_AUTOLINK_BZIP2_LIB
        #ifndef D_CFG_ENV_AUTOLINK_ZSTD_LIB
            #define D_CFG_ENV_AUTOLINK_ZSTD_LIB          "zstd.lib"
        #endif  // D_CFG_ENV_AUTOLINK_ZSTD_LIB
        #ifndef D_CFG_ENV_AUTOLINK_LZ4_LIB
            #define D_CFG_ENV_AUTOLINK_LZ4_LIB           "lz4d.lib"
        #endif  // D_CFG_ENV_AUTOLINK_LZ4_LIB
        #ifndef D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLIENC_LIB     "brotliencd.lib"
        #endif  // D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
        #ifndef D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB     "brotlidecd.lib"
        #endif  // D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
        #ifndef D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB  "brotlicommond.lib"
        #endif  // D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
        #ifndef D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
            #define D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB    "archive.lib"
        #endif  // D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
        #ifndef D_CFG_ENV_AUTOLINK_LIBZIP_LIB
            #define D_CFG_ENV_AUTOLINK_LIBZIP_LIB        "zip.lib"
        #endif  // D_CFG_ENV_AUTOLINK_LIBZIP_LIB

    #else

// 2.2    Release builds
//------------------------------------------------------------------------------
        // 2.2.1
        // D_CFG_ENV_AUTOLINK_*_LIB (release)
        //   configuration: as 2.1.1's debug names, without the "d" suffix.
        #ifndef D_CFG_ENV_AUTOLINK_ZLIB_LIB
            #define D_CFG_ENV_AUTOLINK_ZLIB_LIB          "zlib.lib"
        #endif  // D_CFG_ENV_AUTOLINK_ZLIB_LIB
        #ifndef D_CFG_ENV_AUTOLINK_LZMA_LIB
            #define D_CFG_ENV_AUTOLINK_LZMA_LIB          "lzma.lib"
        #endif  // D_CFG_ENV_AUTOLINK_LZMA_LIB
        #ifndef D_CFG_ENV_AUTOLINK_BZIP2_LIB
            #define D_CFG_ENV_AUTOLINK_BZIP2_LIB         "bz2.lib"
        #endif  // D_CFG_ENV_AUTOLINK_BZIP2_LIB
        #ifndef D_CFG_ENV_AUTOLINK_ZSTD_LIB
            #define D_CFG_ENV_AUTOLINK_ZSTD_LIB          "zstd.lib"
        #endif  // D_CFG_ENV_AUTOLINK_ZSTD_LIB
        #ifndef D_CFG_ENV_AUTOLINK_LZ4_LIB
            #define D_CFG_ENV_AUTOLINK_LZ4_LIB           "lz4.lib"
        #endif  // D_CFG_ENV_AUTOLINK_LZ4_LIB
        #ifndef D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLIENC_LIB     "brotlienc.lib"
        #endif  // D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
        #ifndef D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB     "brotlidec.lib"
        #endif  // D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
        #ifndef D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB  "brotlicommon.lib"
        #endif  // D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
        #ifndef D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
            #define D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB    "archive.lib"
        #endif  // D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
        #ifndef D_CFG_ENV_AUTOLINK_LIBZIP_LIB
            #define D_CFG_ENV_AUTOLINK_LIBZIP_LIB        "zip.lib"
        #endif  // D_CFG_ENV_AUTOLINK_LIBZIP_LIB

    #endif  // D_ENV_BUILD_DEBUG


//==============================================================================
// 3.  CODEC LINK REQUESTS
//==============================================================================
// Emitted only when env_compress.h has been included by this translation
// unit, and then only for codecs that were detected.


// 3.1    Codec libraries
//------------------------------------------------------------------------------
    #ifdef DJINTERP_ENV_ENV_COMPRESS_H

        // 3.1.1
        // D_CFG_ENV_AUTOLINK_LZMA_STATIC
        //   configuration: liblzma's static switch. When static linking is
        // requested, LZMA_API_STATIC must be defined before <lzma.h> is
        // included, so the API is not decorated __declspec(dllimport); include
        // this header ahead of <lzma.h> for the definition to take effect.
        #ifndef D_CFG_ENV_AUTOLINK_LZMA_STATIC
            #define D_CFG_ENV_AUTOLINK_LZMA_STATIC 0
        #endif  // D_CFG_ENV_AUTOLINK_LZMA_STATIC
        #if ( (D_CFG_ENV_AUTOLINK_LZMA_STATIC) &&                              \
              (!defined(LZMA_API_STATIC)) )
            #define LZMA_API_STATIC 1
        #endif

        // 3.1.2
        // Codec link requests
        //   one D_INTERNAL_AUTOLINK_LIB per detected codec.
        #if D_ENV_COMPRESSION_HAVE_ZLIB
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_ZLIB_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_LZMA
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LZMA_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_BZIP2
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_BZIP2_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_ZSTD
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_ZSTD_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_LZ4
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LZ4_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_BROTLI
            // brotli ships as three libraries: encoder, decoder, and a shared
            // common core that the other two depend on.
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_BROTLIENC_LIB)
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB)
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB)
        #endif

    #endif  // DJINTERP_ENV_ENV_COMPRESS_H


//==============================================================================
// 4.  ARCHIVE LINK REQUESTS
//==============================================================================
// Emitted only when env_archive.h has been included by this translation unit,
// and then only for backends that were detected. minizip, libtar, and the
// LZMA SDK are not auto-linked here, since their installed library names vary
// widely; add them with the same D_CFG_ENV_AUTOLINK_*_LIB override pattern if
// a build needs them.


// 4.1    Archive libraries
//------------------------------------------------------------------------------
    #ifdef DJINTERP_ENV_ENV_ARCHIVE_H

        // 4.1.1
        // Archive link requests
        //   one D_INTERNAL_AUTOLINK_LIB per detected archive backend.
        #if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB)
        #endif

        #if D_ENV_ARCHIVE_HAVE_LIBZIP
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LIBZIP_LIB)
        #endif

    #endif  // DJINTERP_ENV_ENV_ARCHIVE_H

#endif  // _MSC_VER && D_CFG_ENV_AUTOLINK


#endif  // DJINTERP_ENV_ENV_COMPRESS_LINK_H
