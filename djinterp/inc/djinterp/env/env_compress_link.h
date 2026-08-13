/******************************************************************************
* djinterp [env]                                           env_compress_link.h
*
*   djinterp auto-link directives for detected third-party libraries:
* env_compress.h and env_archive.h detect codec / archive libraries by header
* presence (__has_include), but presence alone does not put a library's import
* lib on the link line. On the MSVC toolchain family (cl and clang-cl, both of
* which define _MSC_VER and honour `#pragma comment(lib, ...)`) this header
* turns each D_ENV_COMPRESSION_HAVE_* / D_ENV_ARCHIVE_HAVE_* flag that is set
* into a link request for the matching library, so a translation unit that
* calls the codec / archive facades links with no build-system edit. On every
* other toolchain it expands to nothing and linking is left to the build system
* (e.g. -lz -llzma), which is what keeps it portable across compilers.
*
*   The header emits nothing on its own: it reacts to whichever of
* env_compress.h / env_archive.h the translation unit has already included
* (detected through their include guards), and only for libraries that were
* actually detected. Include it from an implementation unit AFTER the facade /
* env headers, e.g.:
*     #include "../util/compress.hpp"      // pulls in env_compress.h
*     #include "../env/env_autolink.h"     // requests zlib / lzma / ... links
*
*   Because the requests are confined to the units that include this header
* (the codec / archive .cpp / .c files), a header that merely reads the
* capability macros never gains a link dependency.
*
*   Library file names default to the common vcpkg layout and are debug/release
* aware; override any D_CFG_ENV_AUTOLINK_*_LIB macro from the build system when
* a library is installed under a different name (for instance a static
* "zlibstatic.lib", or a distro-specific debug suffix).
*
*   Static vs. dynamic: liblzma decorates its API __declspec(dllimport) unless
* LZMA_API_STATIC is defined before <lzma.h>. The default here is dynamic (an
* import lib), matching a dynamic vcpkg install; define
* D_CFG_ENV_AUTOLINK_LZMA_STATIC to 1 to link a static liblzma instead (this
* defines LZMA_API_STATIC and selects a static library name). zlib needs no
* such switch: static or import zlib both resolve as long as ZLIB_DLL matches
* the artifact being linked.
*
*
* path:      /inc/djinterp/core/env/env_compress_link.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.07.04
******************************************************************************/

#ifndef DJINTERP_ENV_COMPRESS_AUTOLINK_
#define DJINTERP_ENV_COMPRESS_AUTOLINK_ 1

// djinterp
#include "../../c/djinterp.h"
#include "./env.h"


// =============================================================================
// I.   CONFIGURATION
// =============================================================================

// D_CFG_ENV_AUTOLINK
//   configuration: master toggle for pragma-based auto-linking. defaults to 1;
// define to 0 from the build system to route every third-party link through
// the build system instead.
#ifndef D_CFG_ENV_AUTOLINK
    #define D_CFG_ENV_AUTOLINK 1
#endif


// The entire facility is a no-op unless the compiler is MSVC-family (cl or
// clang-cl, which define _MSC_VER and accept `#pragma comment(lib, ...)`) and
// the master toggle is on. Every other toolchain skips the block and relies on
// the build system for linking.
#if ( defined(_MSC_VER) && D_CFG_ENV_AUTOLINK )

    // D_INTERNAL_AUTOLINK_LIB
    //   macro: emit a link request for the library named by `name`, a string
    // literal (or a macro expanding to one). __pragma is used, not #pragma, so
    // that `name` undergoes macro replacement.
    #define D_INTERNAL_AUTOLINK_LIB(name) __pragma(comment(lib, name))


    // =========================================================================
    // II.  LIBRARY NAME DEFAULTS (debug / release aware, override-able)
    // =========================================================================
    //   Each name is a single string literal so it can be handed straight to
    // the comment pragma. A build-system override (an -D on any of these
    // D_CFG_ENV_AUTOLINK_*_LIB macros) wins, because each default is guarded
    // with #ifndef. Debug names follow vcpkg's "d"-suffixed layout; adjust via
    // the override macros where a library deviates.

    #if defined(D_ENV_BUILD_DEBUG)

        #ifndef D_CFG_ENV_AUTOLINK_ZLIB_LIB
            #define D_CFG_ENV_AUTOLINK_ZLIB_LIB          "zlibd.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_LZMA_LIB
            #define D_CFG_ENV_AUTOLINK_LZMA_LIB          "lzmad.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_BZIP2_LIB
            #define D_CFG_ENV_AUTOLINK_BZIP2_LIB         "bz2d.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_ZSTD_LIB
            #define D_CFG_ENV_AUTOLINK_ZSTD_LIB          "zstd.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_LZ4_LIB
            #define D_CFG_ENV_AUTOLINK_LZ4_LIB           "lz4d.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLIENC_LIB     "brotliencd.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB     "brotlidecd.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB  "brotlicommond.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
            #define D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB    "archive.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_LIBZIP_LIB
            #define D_CFG_ENV_AUTOLINK_LIBZIP_LIB        "zip.lib"
        #endif

    #else

        #ifndef D_CFG_ENV_AUTOLINK_ZLIB_LIB
            #define D_CFG_ENV_AUTOLINK_ZLIB_LIB          "zlib.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_LZMA_LIB
            #define D_CFG_ENV_AUTOLINK_LZMA_LIB          "lzma.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_BZIP2_LIB
            #define D_CFG_ENV_AUTOLINK_BZIP2_LIB         "bz2.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_ZSTD_LIB
            #define D_CFG_ENV_AUTOLINK_ZSTD_LIB          "zstd.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_LZ4_LIB
            #define D_CFG_ENV_AUTOLINK_LZ4_LIB           "lz4.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_BROTLIENC_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLIENC_LIB     "brotlienc.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLIDEC_LIB     "brotlidec.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB
            #define D_CFG_ENV_AUTOLINK_BROTLICOMMON_LIB  "brotlicommon.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB
            #define D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB    "archive.lib"
        #endif
        #ifndef D_CFG_ENV_AUTOLINK_LIBZIP_LIB
            #define D_CFG_ENV_AUTOLINK_LIBZIP_LIB        "zip.lib"
        #endif

    #endif  // D_ENV_BUILD_DEBUG


    // =========================================================================
    // III. COMPRESSION CODEC LINK REQUESTS
    // =========================================================================
    //   Emitted only when env_compress.h has been included by this translation
    // unit, and then only for codecs that were detected.
    #ifdef DJINTERP_ENV_COMPRESSION_

        // liblzma static switch. when static linking is requested,
        // LZMA_API_STATIC must be defined before <lzma.h> is included so the
        // API is not decorated __declspec(dllimport); include this header ahead
        // of <lzma.h> for the definition to take effect.
        #ifndef D_CFG_ENV_AUTOLINK_LZMA_STATIC
            #define D_CFG_ENV_AUTOLINK_LZMA_STATIC 0
        #endif
        #if ( D_CFG_ENV_AUTOLINK_LZMA_STATIC && !defined(LZMA_API_STATIC) )
            #define LZMA_API_STATIC 1
        #endif

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

    #endif  // DJINTERP_ENV_COMPRESSION_


    // =========================================================================
    // IV.  ARCHIVE LIBRARY LINK REQUESTS
    // =========================================================================
    //   Emitted only when env_archive.h has been included by this translation
    // unit, and then only for backends that were detected. minizip / libtar /
    // the LZMA SDK are not auto-linked here (their installed library names vary
    // widely); add them with the same D_CFG_ENV_AUTOLINK_*_LIB override
    // pattern if a build needs them.
    #ifdef DJINTERP_ENV_ARCHIVE_

        #if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LIBARCHIVE_LIB)
        #endif

        #if D_ENV_ARCHIVE_HAVE_LIBZIP
            D_INTERNAL_AUTOLINK_LIB(D_CFG_ENV_AUTOLINK_LIBZIP_LIB)
        #endif

    #endif  // DJINTERP_ENV_ARCHIVE_

#endif  // _MSC_VER && D_CFG_ENV_AUTOLINK


#endif  // DJINTERP_ENV_COMPRESS_AUTOLINK_