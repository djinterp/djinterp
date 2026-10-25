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
*     #include "../util/compress.hpp"                      // env_compress.h
*     #include "../env/util/compress/env_compress_link.h"  // link requests
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
*
* path:      /inc/djinterp/env/util/compress/env_compress_link.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.04
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONFIGURATION
    -------------
    1.  MSVC gate and link-request helper
         1.  D_INTERNAL_ENV_AUTOLINK_LIB
2.  CODEC LINK REQUESTS
    -------------------
    1.  Codec libraries
         1.  LZMA_API_STATIC
         2.  Codec link requests
3.  ARCHIVE LINK REQUESTS
    ---------------------
    1.  Archive libraries
         1.  Archive link requests
*/

#ifndef DJINTERP_ENV_UTIL_COMPRESS_ENV_COMPRESS_LINK_H
#define DJINTERP_ENV_UTIL_COMPRESS_ENV_COMPRESS_LINK_H 1

// djinterp
#include "../../../config/core/env/util/compress/cfg_env_compress_link.h"
                // D_CFG_ENV_AUTOLINK, its library names, its liblzma switch


//==============================================================================
// 1.  CONFIGURATION
//==============================================================================


// The knobs -- the master switch, each library's name and liblzma's static
// switch -- are cfg_env_compress_link.h's (decision 105 of the register).


// 1.1    MSVC gate and link-request helper
//------------------------------------------------------------------------------
//   the rest of this header is a no-op unless the compiler is MSVC-family (cl
// or clang-cl, which define _MSC_VER and accept `#pragma comment(lib, ...)`)
// and the master toggle is on. Every other toolchain skips the block and
// relies on the build system for linking. The test reads _MSC_VER, not
// D_ENV_COMPILER_MSVC_FAMILY: a link pragma needs the real toolchain, and the
// env macro also follows a simulated one.
#if ( (defined(_MSC_VER)) &&                                                   \
      (D_CFG_ENV_AUTOLINK) )

    // 1.1.1
    // D_INTERNAL_ENV_AUTOLINK_LIB
    //   macro: emit a link request for the library named by `name`, a string
    // literal (or a macro expanding to one). __pragma is used, not #pragma, so
    // that `name` undergoes macro replacement.
    #define D_INTERNAL_ENV_AUTOLINK_LIB(name) __pragma(comment(lib, name))


//==============================================================================
// 2.  CODEC LINK REQUESTS
//==============================================================================
// Emitted only when env_compress.h has been included by this translation
// unit, and then only for codecs that were detected.


// 2.1    Codec libraries
//------------------------------------------------------------------------------
    #ifdef DJINTERP_ENV_UTIL_COMPRESS_ENV_COMPRESS_H

        // 2.1.1
        // LZMA_API_STATIC
        //   macro: liblzma's static switch, defined when
        // D_CFG_ENV_AUTOLINK_LZMA_STATIC (cfg_env_compress_link.h) asks for
        // static linking. It must precede <lzma.h>: include this header
        // ahead of it.
        #if ( (D_CFG_ENV_AUTOLINK_LZMA_STATIC) &&                              \
              (!defined(LZMA_API_STATIC)) )
            #define LZMA_API_STATIC 1
        #endif

        // 2.1.2
        // Codec link requests
        //   one D_INTERNAL_ENV_AUTOLINK_LIB per detected codec.
        #if D_ENV_COMPRESSION_HAVE_ZLIB
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_ZLIB_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_LZMA
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LZMA_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_BZIP2
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_BZIP2_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_ZSTD
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_ZSTD_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_LZ4
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LZ4_LIB)
        #endif

        #if D_ENV_COMPRESSION_HAVE_BROTLI
            // brotli ships as three libraries: encoder, decoder, and a shared
            // common core that the other two depend on.
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_BROTLIENC_LIB)
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB)
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB)
        #endif

    #endif  // DJINTERP_ENV_UTIL_COMPRESS_ENV_COMPRESS_H


//==============================================================================
// 3.  ARCHIVE LINK REQUESTS
//==============================================================================
// Emitted only when env_archive.h has been included by this translation unit,
// and then only for backends that were detected. minizip, libtar, and the
// LZMA SDK are not auto-linked here, since their installed library names vary
// widely; add them with the same D_CFG_ENV_AUTOLINK_*_LIB override pattern if
// a build needs them.


// 3.1    Archive libraries
//------------------------------------------------------------------------------
    #ifdef DJINTERP_ENV_UTIL_ARCHIVE_ENV_ARCHIVE_H

        // 3.1.1
        // Archive link requests
        //   one D_INTERNAL_ENV_AUTOLINK_LIB per detected archive backend.
        #if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB)
        #endif

        #if D_ENV_ARCHIVE_HAVE_LIBZIP
            D_INTERNAL_ENV_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LIBZIP_LIB)
        #endif

    #endif  // DJINTERP_ENV_UTIL_ARCHIVE_ENV_ARCHIVE_H

#endif  // _MSC_VER && D_CFG_ENV_AUTOLINK


#endif  // DJINTERP_ENV_UTIL_COMPRESS_ENV_COMPRESS_LINK_H
