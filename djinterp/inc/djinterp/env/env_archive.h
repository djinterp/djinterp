/*******************************************************************************
* djinterp [env]                                                   env_archive.h
*
* djinterp archive-library detection.
*   Compile-time detection of third-party archive libraries, mapped onto a
* portable capability matrix for the four formats the archive layer targets:
* zip, tar, gz (gzip), and 7z, with RAR read support as well. It covers
* libarchive, libzip, minizip and minizip-ng, libtar, the 7-Zip / LZMA SDK and
* the bit7z C++ wrapper, and the UnRAR library, plus per-format read / write
* roll-ups, recommended-backend selection, and runtime probe declarations,
* including an external-tool fallback.
*   Like env_compress.h, which it builds on for the underlying codecs and
* includes itself, detection is presence-only via __has_include, and a build
* system may pre-define any D_ENV_ARCHIVE_HAVE_* macro to force a result.
*   Names follow three patterns: D_ENV_ARCHIVE_HAVE_<LIB> is 1 when the
* library is available; D_ENV_ARCHIVE_<LIB>_<FIELD> carries version metadata;
* D_ENV_ARCHIVE_CAN_<READ|WRITE>_<FORMAT> reports format capability.
*
* path:      /inc/djinterp/env/env_archive.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PLATFORM SUPPORT
    ----------------
    1.  PATH-probe helpers
         1.  D_INTERNAL_ARCHIVE_OS_WINDOWS
         2.  D_INTERNAL_ACCESS / D_INTERNAL_PATH_SEP / D_INTERNAL_DIR_SEP
    2.  libarchive header
         1.  <archive.h>
2.  CONFIGURATION AND PROBING
    -------------------------
    1.  Configuration and probing
         1.  D_CFG_ENV_ARCHIVE_ENABLED
         2.  D_INTERNAL_ARCHIVE_PROBE
         3.  D_ENV_ARCHIVE_HAVE_BUILTIN_TAR / _ZIP
3.  ARCHIVE LIBRARIES
    -----------------
    1.  libarchive
         1.  D_ENV_ARCHIVE_HAVE_LIBARCHIVE
         2.  D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY
         3.  libarchive version metadata
         4.  D_ENV_ARCHIVE_LIBARCHIVE_AT_LEAST
    2.  libzip
         1.  D_ENV_ARCHIVE_HAVE_LIBZIP
         2.  libzip version metadata
    3.  minizip and minizip-ng
         1.  D_ENV_ARCHIVE_HAVE_MINIZIP_NG
         2.  D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC
         3.  D_ENV_ARCHIVE_HAVE_MINIZIP
    4.  libtar
         1.  D_ENV_ARCHIVE_HAVE_LIBTAR
    5.  7-Zip / LZMA SDK and bit7z
         1.  D_ENV_ARCHIVE_HAVE_LZMA_SDK
         2.  D_ENV_ARCHIVE_HAVE_BIT7Z
         3.  D_ENV_ARCHIVE_HAVE_7ZIP
    6.  RAR (UnRAR / WinRAR)
         1.  D_ENV_ARCHIVE_HAVE_UNRAR
         2.  D_ENV_ARCHIVE_HAVE_RAR_TOOL
         3.  D_ENV_ARCHIVE_CAN_READ_RAR5
4.  PLATFORM-NATIVE ARCHIVERS
    -------------------------
    1.  Operating-system archivers
         1.  D_ENV_ARCHIVE_LIKELY_APPLE_BSDTAR
         2.  D_ENV_ARCHIVE_LIKELY_WIN_BSDTAR
         3.  D_ENV_ARCHIVE_HAVE_WIN_SHELL_ZIP
5.  FORMAT CAPABILITY MATRIX
    ------------------------
    1.  zip
         1.  D_ENV_ARCHIVE_CAN_READ_ZIP
         2.  D_ENV_ARCHIVE_CAN_WRITE_ZIP
    2.  tar
         1.  D_ENV_ARCHIVE_CAN_READ_TAR
         2.  D_ENV_ARCHIVE_CAN_WRITE_TAR
         3.  D_ENV_ARCHIVE_CAN_WRITE_TGZ
    3.  gz (gzip stream)
         1.  D_ENV_ARCHIVE_CAN_READ_GZ
         2.  D_ENV_ARCHIVE_CAN_WRITE_GZ
    4.  7z
         1.  D_ENV_ARCHIVE_CAN_READ_7Z
         2.  D_ENV_ARCHIVE_CAN_WRITE_7Z
    5.  rar
         1.  D_ENV_ARCHIVE_CAN_READ_RAR
         2.  D_ENV_ARCHIVE_CAN_WRITE_RAR
    6.  Aggregate
         1.  D_ENV_ARCHIVE_CAN_WRITE_ANY
6.  RECOMMENDED-BACKEND SELECTION
    -----------------------------
    1.  Backend identifiers
         1.  D_ENV_ARCHIVE_BACKEND_*
              1.  D_ENV_ARCHIVE_BACKEND_NONE
              2.  D_ENV_ARCHIVE_BACKEND_LIBARCHIVE
              3.  D_ENV_ARCHIVE_BACKEND_LIBZIP
              4.  D_ENV_ARCHIVE_BACKEND_MINIZIP_NG
              5.  D_ENV_ARCHIVE_BACKEND_MINIZIP
              6.  D_ENV_ARCHIVE_BACKEND_MINIZ
              7.  D_ENV_ARCHIVE_BACKEND_LIBTAR
              8.  D_ENV_ARCHIVE_BACKEND_LZMA_SDK
              9.  D_ENV_ARCHIVE_BACKEND_BIT7Z
              10. D_ENV_ARCHIVE_BACKEND_UNRAR
              11. D_ENV_ARCHIVE_BACKEND_RAR_TOOL
              12. D_ENV_ARCHIVE_BACKEND_BUILTIN
    2.  Preferred backends
         1.  D_ENV_ARCHIVE_PREFERRED_ZIP
         2.  D_ENV_ARCHIVE_PREFERRED_TAR
         3.  D_ENV_ARCHIVE_PREFERRED_7Z
         4.  D_ENV_ARCHIVE_PREFERRED_RAR_READ
         5.  D_ENV_ARCHIVE_PREFERRED_RAR_WRITE
7.  RUNTIME PROBES
    --------------
    1.  Runtime probes
*/

#ifndef DJINTERP_ENV_ENV_ARCHIVE_H
#define DJINTERP_ENV_ENV_ARCHIVE_H 1

// djinterp
#include "../c/djinterp.h"   // D_EXTERN_C_BEGIN, D_EXTERN_C_END
#include "./env.h"           // D_ENV_OS_ID, D_ENV_IS_OS_WINDOWS
#include "./env_compress.h"  // D_ENV_HAS_INCLUDE, D_ENV_COMPRESSION_HAVE_*


//==============================================================================
// 1.  PLATFORM SUPPORT
//==============================================================================


// 1.1    PATH-probe helpers
//------------------------------------------------------------------------------
// 1.1.1
// D_INTERNAL_ARCHIVE_OS_WINDOWS
//   constant: 1 when the target is Windows, selecting the Windows spellings in
// 1.1.2; 0 otherwise.
#if ( defined(D_ENV_OS_ID) &&                                                 \
      D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
    #define D_INTERNAL_ARCHIVE_OS_WINDOWS 1
#else
    #define D_INTERNAL_ARCHIVE_OS_WINDOWS 0
#endif

// 1.1.2
// D_INTERNAL_ACCESS / D_INTERNAL_PATH_SEP / D_INTERNAL_DIR_SEP
//   macro: the executable-access test, PATH-list separator, and directory
// separator the external-tool probe (d_env_archive_has_tool) uses.
#if D_INTERNAL_ARCHIVE_OS_WINDOWS
    // windows
    #include <io.h>          // _access
    #define D_INTERNAL_ACCESS(p)      _access((p), 0)
    #define D_INTERNAL_PATH_SEP       ';'
    #define D_INTERNAL_DIR_SEP        '\\'
#else
    // posix
    #include <unistd.h>      // access, X_OK
    #define D_INTERNAL_ACCESS(p)      access((p), X_OK)
    #define D_INTERNAL_PATH_SEP       ':'
    #define D_INTERNAL_DIR_SEP        '/'
#endif

// 1.2    libarchive header
//------------------------------------------------------------------------------
// 1.2.1
// <archive.h>
//   included only when the build pre-defines D_ENV_ARCHIVE_HAVE_LIBARCHIVE to
// a nonzero value: the macro is not detected until section 3, below this.
#if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    // libarchive
    #include <archive.h>  // ARCHIVE_VERSION_NUMBER, ARCHIVE_VERSION_STRING
#endif


//==============================================================================
// 2.  CONFIGURATION AND PROBING
//==============================================================================


// 2.1    Configuration and probing
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_ARCHIVE_ENABLED
//   configuration: master toggle for archive-library detection. when 0, every
// D_ENV_ARCHIVE_HAVE_* flag resolves to 0 unless explicitly pre-defined.
#ifndef D_CFG_ENV_ARCHIVE_ENABLED
    #define D_CFG_ENV_ARCHIVE_ENABLED 1
#endif  // D_CFG_ENV_ARCHIVE_ENABLED

// 2.1.2
// D_INTERNAL_ARCHIVE_PROBE
//   macro: internal helper. yields the __has_include result for `header`
// when detection is enabled, and 0 when the master toggle is off.
#if D_CFG_ENV_ARCHIVE_ENABLED
    #define D_INTERNAL_ARCHIVE_PROBE(header) D_ENV_HAS_INCLUDE(header)
#else
    #define D_INTERNAL_ARCHIVE_PROBE(header) 0
#endif

// 2.1.3
// D_ENV_ARCHIVE_HAVE_BUILTIN_TAR / _ZIP
//   configuration: the djinterp archive facade (archive.hpp / archive.cpp)
// ships dependency-free ustar and ZIP writers/readers. These default to 1 so
// the capability roll-ups below report tar and zip as available even when no
// third-party archive library is detected. The ZIP writer's DEFLATE method
// additionally requires a DEFLATE codec; with none present it falls back to
// the store method, so the built-in remains usable either way. Define either
// macro to 0 to model an environment that excludes the built-in writers.
#ifndef D_ENV_ARCHIVE_HAVE_BUILTIN_TAR
    #define D_ENV_ARCHIVE_HAVE_BUILTIN_TAR  1
#endif  // D_ENV_ARCHIVE_HAVE_BUILTIN_TAR

#ifndef D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP
    #define D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP  1
#endif  // D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP


//==============================================================================
// 3.  ARCHIVE LIBRARIES
//==============================================================================


// 3.1    libarchive
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_ARCHIVE_HAVE_LIBARCHIVE
//   feature: detect if libarchive (<archive.h>) is available. libarchive is
// the broadest backend, reading and writing zip, tar (and tar.* variants),
// gzip, 7z, cpio, iso9660, and others through a unified streaming API.
#ifndef D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    #if D_INTERNAL_ARCHIVE_PROBE(<archive.h>)
        #define D_ENV_ARCHIVE_HAVE_LIBARCHIVE       1
    #else
        #define D_ENV_ARCHIVE_HAVE_LIBARCHIVE       0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_LIBARCHIVE

// 3.1.2
// D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY
//   feature: detect the companion <archive_entry.h> header, required for
// per-entry metadata when writing archives.
#ifndef D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY
    #if D_INTERNAL_ARCHIVE_PROBE(<archive_entry.h>)
        #define D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY 1
    #else
        #define D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY 0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY

// 3.1.3
// libarchive version metadata
//   constant: D_ENV_ARCHIVE_LIBARCHIVE_VERNUM and _VERSION_STR, from
// ARCHIVE_VERSION_NUMBER / ARCHIVE_VERSION_STRING; populated only when
// <archive.h> has been included, and otherwise 0 and "unknown".
#if defined(ARCHIVE_VERSION_NUMBER)
    #define D_ENV_ARCHIVE_LIBARCHIVE_VERNUM     ARCHIVE_VERSION_NUMBER
#else
    #define D_ENV_ARCHIVE_LIBARCHIVE_VERNUM     0
#endif

#if defined(ARCHIVE_VERSION_STRING)
    #define D_ENV_ARCHIVE_LIBARCHIVE_VERSION_STR ARCHIVE_VERSION_STRING
#else
    #define D_ENV_ARCHIVE_LIBARCHIVE_VERSION_STR "unknown"
#endif

// 3.1.4
// D_ENV_ARCHIVE_LIBARCHIVE_AT_LEAST
//   macro: evaluates to 1 if the included libarchive reports at least the
// given version. ARCHIVE_VERSION_NUMBER is MAJOR*1000000 + MINOR*1000 + REV.
#define D_ENV_ARCHIVE_LIBARCHIVE_AT_LEAST(major, minor, rev)                   \
    ( D_ENV_ARCHIVE_HAVE_LIBARCHIVE &&                                         \
      (D_ENV_ARCHIVE_LIBARCHIVE_VERNUM >=                                      \
       (((major) * 1000000) + ((minor) * 1000) + (rev))) )

// 3.2    libzip
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_ARCHIVE_HAVE_LIBZIP
//   feature: detect if libzip (<zip.h>) is available. libzip supports both
// reading and writing of zip archives.
#ifndef D_ENV_ARCHIVE_HAVE_LIBZIP
    #if D_INTERNAL_ARCHIVE_PROBE(<zip.h>)
        #define D_ENV_ARCHIVE_HAVE_LIBZIP           1
    #else
        #define D_ENV_ARCHIVE_HAVE_LIBZIP           0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_LIBZIP

// 3.2.2
// libzip version metadata
//   constant: D_ENV_ARCHIVE_LIBZIP_VERSION_STR, from LIBZIP_VERSION once
// <zip.h> is included; "unknown" otherwise.
#if defined(LIBZIP_VERSION)
    #define D_ENV_ARCHIVE_LIBZIP_VERSION_STR        LIBZIP_VERSION
#else
    #define D_ENV_ARCHIVE_LIBZIP_VERSION_STR        "unknown"
#endif

// 3.3    minizip and minizip-ng
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_ARCHIVE_HAVE_MINIZIP_NG
//   feature: detect minizip-ng (<mz.h>), the modern rewrite supporting zip
// read / write with pluggable codecs and encryption.
#ifndef D_ENV_ARCHIVE_HAVE_MINIZIP_NG
    #if D_INTERNAL_ARCHIVE_PROBE(<mz.h>)
        #define D_ENV_ARCHIVE_HAVE_MINIZIP_NG       1
    #else
        #define D_ENV_ARCHIVE_HAVE_MINIZIP_NG       0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_MINIZIP_NG

// 3.3.2
// D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC
//   feature: detect the classic zlib-contrib minizip via its split
// <minizip/zip.h> / <minizip/unzip.h> headers.
#ifndef D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC
    #if ( D_INTERNAL_ARCHIVE_PROBE(<minizip/zip.h>) &&                         \
          D_INTERNAL_ARCHIVE_PROBE(<minizip/unzip.h>) )
        #define D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC  1
    #else
        #define D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC  0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC

// 3.3.3
// D_ENV_ARCHIVE_HAVE_MINIZIP
//   feature: 1 if either minizip flavour is available.
#define D_ENV_ARCHIVE_HAVE_MINIZIP                                             \
    ( D_ENV_ARCHIVE_HAVE_MINIZIP_NG ||                                         \
      D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC )

// 3.4    libtar
//------------------------------------------------------------------------------
// 3.4.1
// D_ENV_ARCHIVE_HAVE_LIBTAR
//   feature: detect if libtar (<libtar.h>) is available. libtar reads and
// writes uncompressed tar; compression is layered separately (e.g. gzip).
#ifndef D_ENV_ARCHIVE_HAVE_LIBTAR
    #if D_INTERNAL_ARCHIVE_PROBE(<libtar.h>)
        #define D_ENV_ARCHIVE_HAVE_LIBTAR           1
    #else
        #define D_ENV_ARCHIVE_HAVE_LIBTAR           0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_LIBTAR

// 3.5    7-Zip / LZMA SDK and bit7z
//------------------------------------------------------------------------------
// 3.5.1
// D_ENV_ARCHIVE_HAVE_LZMA_SDK
//   feature: detect the 7-Zip / LZMA SDK C headers. several layouts exist in
// the wild; any of the probed headers implies the SDK is present.
#ifndef D_ENV_ARCHIVE_HAVE_LZMA_SDK
    #if ( D_INTERNAL_ARCHIVE_PROBE(<7z.h>)      ||                             \
          D_INTERNAL_ARCHIVE_PROBE(<7zTypes.h>) ||                             \
          D_INTERNAL_ARCHIVE_PROBE(<LzmaLib.h>) ||                             \
          D_INTERNAL_ARCHIVE_PROBE(<Lzma2Enc.h>) )
        #define D_ENV_ARCHIVE_HAVE_LZMA_SDK         1
    #else
        #define D_ENV_ARCHIVE_HAVE_LZMA_SDK         0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_LZMA_SDK

// 3.5.2
// D_ENV_ARCHIVE_HAVE_BIT7Z
//   feature: detect the bit7z C++ wrapper (<bit7z/bittypes.hpp>), which wraps
// the 7-Zip library for full 7z read / write from C++.
// note: bit7z is C++ only.
#ifndef D_ENV_ARCHIVE_HAVE_BIT7Z
    #if ( (D_ENV_LANG_USING_CPP) &&                                           \
          ( D_INTERNAL_ARCHIVE_PROBE(<bit7z/bittypes.hpp>) ||                  \
            D_INTERNAL_ARCHIVE_PROBE(<bit7z/bit7z.hpp>) ) )
        #define D_ENV_ARCHIVE_HAVE_BIT7Z            1
    #else
        #define D_ENV_ARCHIVE_HAVE_BIT7Z            0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_BIT7Z

// 3.5.3
// D_ENV_ARCHIVE_HAVE_7ZIP
//   feature: 1 if any 7z-capable backend is available.
#define D_ENV_ARCHIVE_HAVE_7ZIP                                               \
    ( D_ENV_ARCHIVE_HAVE_LZMA_SDK ||                                          \
      D_ENV_ARCHIVE_HAVE_BIT7Z    ||                                          \
      D_ENV_ARCHIVE_HAVE_LIBARCHIVE )

// 3.6    RAR (UnRAR / WinRAR)
//------------------------------------------------------------------------------
//   RAR is asymmetric. Extraction is widely available, but creation is
// proprietary: no library can write RAR. Only the RARLAB `rar` / WinRAR
// command-line tool produces .rar archives. libarchive and 7-Zip backends can
// read RAR but never write it.
// 3.6.1
// D_ENV_ARCHIVE_HAVE_UNRAR
//   feature: detect the RARLAB UnRAR library (<unrar.h>, or the C++ SDK header
// <unrar/dll.hpp>). extraction only; UnRAR cannot create archives.
#ifndef D_ENV_ARCHIVE_HAVE_UNRAR
    #if ( D_INTERNAL_ARCHIVE_PROBE(<unrar.h>) ||                              \
          D_INTERNAL_ARCHIVE_PROBE(<unrar/dll.hpp>) )
        #define D_ENV_ARCHIVE_HAVE_UNRAR            1
    #else
        #define D_ENV_ARCHIVE_HAVE_UNRAR            0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_UNRAR

// 3.6.2
// D_ENV_ARCHIVE_HAVE_RAR_TOOL
//   feature: presence of the RARLAB `rar` / WinRAR executable, the only means
// of *creating* RAR archives. this is not compile-time detectable, so it
// defaults to 0; set it from the build system, or confirm at runtime with
// d_env_archive_has_tool("rar"). reading can also use d_env_archive_has_tool
// ("unrar").
#ifndef D_ENV_ARCHIVE_HAVE_RAR_TOOL
    #define D_ENV_ARCHIVE_HAVE_RAR_TOOL             0
#endif  // D_ENV_ARCHIVE_HAVE_RAR_TOOL

// 3.6.3
// D_ENV_ARCHIVE_CAN_READ_RAR5
//   feature: 1 if a backend that understands the RAR5 format is available.
// UnRAR and 7-Zip handle RAR5; libarchive has supported RAR5 read since 3.4.0.
// As with 7z write, we key on libarchive presence rather than
// D_ENV_ARCHIVE_LIBARCHIVE_AT_LEAST (which requires including <archive.h>);
// any libarchive recent enough to be installed satisfies the 3.4 floor.
#define D_ENV_ARCHIVE_CAN_READ_RAR5                                          \
    ( D_ENV_ARCHIVE_HAVE_UNRAR    ||                                        \
      D_ENV_ARCHIVE_HAVE_BIT7Z    ||                                        \
      D_ENV_ARCHIVE_HAVE_RAR_TOOL ||                                        \
      D_ENV_ARCHIVE_HAVE_LIBARCHIVE )


//==============================================================================
// 4.  PLATFORM-NATIVE ARCHIVERS
//==============================================================================
// These flag archiver facilities shipped with the operating system itself,
// using the OS classification from env.h. Tool-bundling flags are prefixed
// LIKELY_ because they are inferred from the OS version and must be confirmed
// at runtime via d_env_archive_has_tool(). All probes degrade to 0 when OS
// detection is unavailable (D_ENV_OS_ID undefined).


// 4.1    Operating-system archivers
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_ARCHIVE_LIKELY_APPLE_BSDTAR
//   feature: macOS bundles bsdtar (libarchive) as /usr/bin/tar, giving tar,
// tar.gz, and zip handling without any linked library.
#ifndef D_ENV_ARCHIVE_LIKELY_APPLE_BSDTAR
    #if ( defined(D_ENV_OS_ID) &&                                             \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) )
        #define D_ENV_ARCHIVE_LIKELY_APPLE_BSDTAR   1
    #else
        #define D_ENV_ARCHIVE_LIKELY_APPLE_BSDTAR   0
    #endif
#endif  // D_ENV_ARCHIVE_LIKELY_APPLE_BSDTAR

// 4.1.2
// D_ENV_ARCHIVE_LIKELY_WIN_BSDTAR
//   feature: Windows 10 (build 17063 / version 1803) and Windows 11 bundle
// bsdtar as tar.exe, which also reads and writes zip.
#ifndef D_ENV_ARCHIVE_LIKELY_WIN_BSDTAR
    #if ( defined(D_ENV_OS_ID) &&                                             \
          ( (D_ENV_OS_ID == D_ENV_OS_FLAG_WIN_PC_10) ||                       \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_WIN_PC_11) ) )
        #define D_ENV_ARCHIVE_LIKELY_WIN_BSDTAR     1
    #else
        #define D_ENV_ARCHIVE_LIKELY_WIN_BSDTAR     0
    #endif
#endif  // D_ENV_ARCHIVE_LIKELY_WIN_BSDTAR

// 4.1.3
// D_ENV_ARCHIVE_HAVE_WIN_SHELL_ZIP
//   feature: detect the Windows Shell automation interface (<shldisp.h>,
// IShellDispatch / "compressed folders"), usable to create and extract zip
// archives on desktop Windows without a third-party library.
#ifndef D_ENV_ARCHIVE_HAVE_WIN_SHELL_ZIP
    #if ( defined(D_ENV_OS_ID)                       &&                       \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)           &&                       \
          D_INTERNAL_ARCHIVE_PROBE(<shldisp.h>) )
        #define D_ENV_ARCHIVE_HAVE_WIN_SHELL_ZIP    1
    #else
        #define D_ENV_ARCHIVE_HAVE_WIN_SHELL_ZIP    0
    #endif
#endif  // D_ENV_ARCHIVE_HAVE_WIN_SHELL_ZIP


//==============================================================================
// 5.  FORMAT CAPABILITY MATRIX
//==============================================================================
// The roll-ups below answer "can this build read / write <format>?" without
// the caller needing to know which backend is present. They combine library
// availability with the codec roll-ups from env_compress.h where a format
// requires a codec (e.g. zip "deflate", or tar.gz).


// 5.1    zip
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_ARCHIVE_CAN_READ_ZIP
//   feature: 1 if some backend can read zip archives.
#define D_ENV_ARCHIVE_CAN_READ_ZIP                                            \
    ( D_ENV_ARCHIVE_HAVE_LIBARCHIVE      ||                                   \
      D_ENV_ARCHIVE_HAVE_LIBZIP          ||                                   \
      D_ENV_ARCHIVE_HAVE_MINIZIP         ||                                   \
      D_ENV_COMPRESSION_HAVE_MINIZ       ||                                   \
      D_ENV_ARCHIVE_HAVE_WIN_SHELL_ZIP   ||                                   \
      D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP )

// 5.1.2
// D_ENV_ARCHIVE_CAN_WRITE_ZIP
//   feature: 1 if some backend can write zip archives.
#define D_ENV_ARCHIVE_CAN_WRITE_ZIP         D_ENV_ARCHIVE_CAN_READ_ZIP

// 5.2    tar
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_ARCHIVE_CAN_READ_TAR
//   feature: 1 if some backend can read tar archives (uncompressed).
#define D_ENV_ARCHIVE_CAN_READ_TAR                                            \
    ( D_ENV_ARCHIVE_HAVE_LIBARCHIVE ||                                        \
      D_ENV_ARCHIVE_HAVE_LIBTAR     ||                                        \
      D_ENV_ARCHIVE_HAVE_BUILTIN_TAR )

// 5.2.2
// D_ENV_ARCHIVE_CAN_WRITE_TAR
//   feature: 1 if some backend can write tar archives (uncompressed).
#define D_ENV_ARCHIVE_CAN_WRITE_TAR         D_ENV_ARCHIVE_CAN_READ_TAR

// 5.2.3
// D_ENV_ARCHIVE_CAN_WRITE_TGZ
//   feature: 1 if a compressed tar.gz can be produced (a tar writer plus a
// gzip codec, or libarchive which bundles the gzip filter).
#define D_ENV_ARCHIVE_CAN_WRITE_TGZ                                           \
    ( D_ENV_ARCHIVE_HAVE_LIBARCHIVE ||                                        \
      ( D_ENV_ARCHIVE_HAVE_LIBTAR      && D_ENV_COMPRESSION_HAVE_GZIP ) ||    \
      ( D_ENV_ARCHIVE_HAVE_BUILTIN_TAR && D_ENV_COMPRESSION_HAVE_GZIP_WRAP ) )

// 5.3    gz (gzip stream)
//------------------------------------------------------------------------------
// 5.3.1
// D_ENV_ARCHIVE_CAN_READ_GZ
//   feature: 1 if a gzip stream can be read (any gzip-container codec or the
// libarchive gzip filter).
#define D_ENV_ARCHIVE_CAN_READ_GZ                                             \
    ( D_ENV_COMPRESSION_HAVE_GZIP_WRAP ||                                     \
      D_ENV_ARCHIVE_HAVE_LIBARCHIVE )

// 5.3.2
// D_ENV_ARCHIVE_CAN_WRITE_GZ
//   feature: 1 if a gzip stream can be written.
#define D_ENV_ARCHIVE_CAN_WRITE_GZ          D_ENV_ARCHIVE_CAN_READ_GZ

// 5.4    7z
//------------------------------------------------------------------------------
// 5.4.1
// D_ENV_ARCHIVE_CAN_READ_7Z
//   feature: 1 if some backend can read 7z archives.
#define D_ENV_ARCHIVE_CAN_READ_7Z           D_ENV_ARCHIVE_HAVE_7ZIP

// 5.4.2
// D_ENV_ARCHIVE_CAN_WRITE_7Z
//   feature: 1 if some backend can write 7z archives. libarchive has shipped
// 7z write support since 3.0; we key on presence rather than
// D_ENV_ARCHIVE_LIBARCHIVE_AT_LEAST (which needs <archive.h> to be included to
// read ARCHIVE_VERSION_NUMBER, and this is a presence-only detection layer).
// bit7z and the LZMA SDK can also create 7z.
#define D_ENV_ARCHIVE_CAN_WRITE_7Z                                            \
    ( D_ENV_ARCHIVE_HAVE_BIT7Z      ||                                        \
      D_ENV_ARCHIVE_HAVE_LZMA_SDK   ||                                        \
      D_ENV_ARCHIVE_HAVE_LIBARCHIVE )

// 5.5    rar
//------------------------------------------------------------------------------
// 5.5.1
// D_ENV_ARCHIVE_CAN_READ_RAR
//   feature: 1 if some backend can extract RAR (UnRAR, libarchive's RAR
// reader, a 7-Zip-based backend, or the rar/unrar tool).
#define D_ENV_ARCHIVE_CAN_READ_RAR                                            \
    ( D_ENV_ARCHIVE_HAVE_UNRAR      ||                                        \
      D_ENV_ARCHIVE_HAVE_LIBARCHIVE ||                                        \
      D_ENV_ARCHIVE_HAVE_BIT7Z      ||                                        \
      D_ENV_ARCHIVE_HAVE_RAR_TOOL )

// 5.5.2
// D_ENV_ARCHIVE_CAN_WRITE_RAR
//   feature: 1 only if the proprietary rar/WinRAR tool is present. no library
// can create RAR, so this never lights up from a linked dependency alone.
#define D_ENV_ARCHIVE_CAN_WRITE_RAR         D_ENV_ARCHIVE_HAVE_RAR_TOOL

// 5.6    Aggregate
//------------------------------------------------------------------------------
// 5.6.1
// D_ENV_ARCHIVE_CAN_WRITE_ANY
//   feature: 1 if any supported format can be written.
#define D_ENV_ARCHIVE_CAN_WRITE_ANY                                           \
    ( D_ENV_ARCHIVE_CAN_WRITE_ZIP ||                                          \
      D_ENV_ARCHIVE_CAN_WRITE_TAR ||                                          \
      D_ENV_ARCHIVE_CAN_WRITE_GZ  ||                                          \
      D_ENV_ARCHIVE_CAN_WRITE_7Z  ||                                          \
      D_ENV_ARCHIVE_CAN_WRITE_RAR )


//==============================================================================
// 6.  RECOMMENDED-BACKEND SELECTION
//==============================================================================


// 6.1    Backend identifiers
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_ARCHIVE_BACKEND_*
//   constant: stable small integers naming a backend, for runtime dispatch and
// for the 6.2 preferences.

// 6.1.1.1
// D_ENV_ARCHIVE_BACKEND_NONE
//   constant: identifies no backend.
#define D_ENV_ARCHIVE_BACKEND_NONE          0

// 6.1.1.2
// D_ENV_ARCHIVE_BACKEND_LIBARCHIVE
//   constant: identifies libarchive.
#define D_ENV_ARCHIVE_BACKEND_LIBARCHIVE    1

// 6.1.1.3
// D_ENV_ARCHIVE_BACKEND_LIBZIP
//   constant: identifies libzip.
#define D_ENV_ARCHIVE_BACKEND_LIBZIP        2

// 6.1.1.4
// D_ENV_ARCHIVE_BACKEND_MINIZIP_NG
//   constant: identifies minizip-ng.
#define D_ENV_ARCHIVE_BACKEND_MINIZIP_NG    3

// 6.1.1.5
// D_ENV_ARCHIVE_BACKEND_MINIZIP
//   constant: identifies classic minizip.
#define D_ENV_ARCHIVE_BACKEND_MINIZIP       4

// 6.1.1.6
// D_ENV_ARCHIVE_BACKEND_MINIZ
//   constant: identifies miniz.
#define D_ENV_ARCHIVE_BACKEND_MINIZ         5

// 6.1.1.7
// D_ENV_ARCHIVE_BACKEND_LIBTAR
//   constant: identifies libtar.
#define D_ENV_ARCHIVE_BACKEND_LIBTAR        6

// 6.1.1.8
// D_ENV_ARCHIVE_BACKEND_LZMA_SDK
//   constant: identifies the 7-Zip / LZMA SDK.
#define D_ENV_ARCHIVE_BACKEND_LZMA_SDK      7

// 6.1.1.9
// D_ENV_ARCHIVE_BACKEND_BIT7Z
//   constant: identifies bit7z.
#define D_ENV_ARCHIVE_BACKEND_BIT7Z         8

// 6.1.1.10
// D_ENV_ARCHIVE_BACKEND_UNRAR
//   constant: identifies the UnRAR library.
#define D_ENV_ARCHIVE_BACKEND_UNRAR         9

// 6.1.1.11
// D_ENV_ARCHIVE_BACKEND_RAR_TOOL
//   constant: identifies the rar / WinRAR tool.
#define D_ENV_ARCHIVE_BACKEND_RAR_TOOL      10

// 6.1.1.12
// D_ENV_ARCHIVE_BACKEND_BUILTIN
//   constant: identifies the framework's built-in tar and zip writers.
#define D_ENV_ARCHIVE_BACKEND_BUILTIN       11

// 6.2    Preferred backends
//------------------------------------------------------------------------------
// 6.2.1
// D_ENV_ARCHIVE_PREFERRED_ZIP
//   constant: preferred zip backend. a dedicated zip library is favoured for
// fine-grained control, then libarchive, then miniz as a self-contained
// fallback.
#if D_ENV_ARCHIVE_HAVE_LIBZIP
    #define D_ENV_ARCHIVE_PREFERRED_ZIP     D_ENV_ARCHIVE_BACKEND_LIBZIP
#elif D_ENV_ARCHIVE_HAVE_MINIZIP_NG
    #define D_ENV_ARCHIVE_PREFERRED_ZIP     D_ENV_ARCHIVE_BACKEND_MINIZIP_NG
#elif D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC
    #define D_ENV_ARCHIVE_PREFERRED_ZIP     D_ENV_ARCHIVE_BACKEND_MINIZIP
#elif D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    #define D_ENV_ARCHIVE_PREFERRED_ZIP     D_ENV_ARCHIVE_BACKEND_LIBARCHIVE
#elif D_ENV_COMPRESSION_HAVE_MINIZ
    #define D_ENV_ARCHIVE_PREFERRED_ZIP     D_ENV_ARCHIVE_BACKEND_MINIZ
#elif D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP
    #define D_ENV_ARCHIVE_PREFERRED_ZIP     D_ENV_ARCHIVE_BACKEND_BUILTIN
#else
    #define D_ENV_ARCHIVE_PREFERRED_ZIP     D_ENV_ARCHIVE_BACKEND_NONE
#endif

// 6.2.2
// D_ENV_ARCHIVE_PREFERRED_TAR
//   constant: preferred tar backend (libarchive, then libtar, then built-in).
#if D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    #define D_ENV_ARCHIVE_PREFERRED_TAR     D_ENV_ARCHIVE_BACKEND_LIBARCHIVE
#elif D_ENV_ARCHIVE_HAVE_LIBTAR
    #define D_ENV_ARCHIVE_PREFERRED_TAR     D_ENV_ARCHIVE_BACKEND_LIBTAR
#elif D_ENV_ARCHIVE_HAVE_BUILTIN_TAR
    #define D_ENV_ARCHIVE_PREFERRED_TAR     D_ENV_ARCHIVE_BACKEND_BUILTIN
#else
    #define D_ENV_ARCHIVE_PREFERRED_TAR     D_ENV_ARCHIVE_BACKEND_NONE
#endif

// 6.2.3
// D_ENV_ARCHIVE_PREFERRED_7Z
//   constant: preferred 7z backend (bit7z for C++, then the LZMA SDK, then
// libarchive).
#if D_ENV_ARCHIVE_HAVE_BIT7Z
    #define D_ENV_ARCHIVE_PREFERRED_7Z      D_ENV_ARCHIVE_BACKEND_BIT7Z
#elif D_ENV_ARCHIVE_HAVE_LZMA_SDK
    #define D_ENV_ARCHIVE_PREFERRED_7Z      D_ENV_ARCHIVE_BACKEND_LZMA_SDK
#elif D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    #define D_ENV_ARCHIVE_PREFERRED_7Z      D_ENV_ARCHIVE_BACKEND_LIBARCHIVE
#else
    #define D_ENV_ARCHIVE_PREFERRED_7Z      D_ENV_ARCHIVE_BACKEND_NONE
#endif

// 6.2.4
// D_ENV_ARCHIVE_PREFERRED_RAR_READ
//   constant: preferred RAR extraction backend (UnRAR, then a 7-Zip backend,
// then libarchive, then the tool).
#if D_ENV_ARCHIVE_HAVE_UNRAR
    #define D_ENV_ARCHIVE_PREFERRED_RAR_READ D_ENV_ARCHIVE_BACKEND_UNRAR
#elif D_ENV_ARCHIVE_HAVE_BIT7Z
    #define D_ENV_ARCHIVE_PREFERRED_RAR_READ D_ENV_ARCHIVE_BACKEND_BIT7Z
#elif D_ENV_ARCHIVE_HAVE_LIBARCHIVE
    #define D_ENV_ARCHIVE_PREFERRED_RAR_READ D_ENV_ARCHIVE_BACKEND_LIBARCHIVE
#elif D_ENV_ARCHIVE_HAVE_RAR_TOOL
    #define D_ENV_ARCHIVE_PREFERRED_RAR_READ D_ENV_ARCHIVE_BACKEND_RAR_TOOL
#else
    #define D_ENV_ARCHIVE_PREFERRED_RAR_READ D_ENV_ARCHIVE_BACKEND_NONE
#endif

// 6.2.5
// D_ENV_ARCHIVE_PREFERRED_RAR_WRITE
//   constant: preferred RAR creation backend. only the proprietary tool can
// write RAR, so this is the tool or nothing.
#if D_ENV_ARCHIVE_HAVE_RAR_TOOL
    #define D_ENV_ARCHIVE_PREFERRED_RAR_WRITE D_ENV_ARCHIVE_BACKEND_RAR_TOOL
#else
    #define D_ENV_ARCHIVE_PREFERRED_RAR_WRITE D_ENV_ARCHIVE_BACKEND_NONE
#endif


//==============================================================================
// 7.  RUNTIME PROBES
//==============================================================================
// Declared here and defined in the archive implementation. Unlike the macros
// above, they can report on what the build actually linked and on tools
// installed on the machine.


// 7.1    Runtime probes
//------------------------------------------------------------------------------
D_EXTERN_C_BEGIN

/**
 * @brief Returns a human-readable name for a backend identifier.
 *
 * @param[in] _backend  one of the D_ENV_ARCHIVE_BACKEND_* constants.
 * @return a static, null-terminated backend name, or "none".
 */
const char* d_env_archive_backend_name(int _backend);
/**
 * @brief Returns the version of the libarchive linked at runtime.
 *
 * @return archive_version_string()'s static string, or "unavailable" when
 *         libarchive was not linked.
 */
const char* d_env_archive_libarchive_runtime_version(void);
/**
 * @brief Probes PATH for an external archiver executable.
 *
 * @note Enables a shell-out fallback when no library backend is linked.
 *
 * @param[in] _tool_name  the command to look for (e.g. "tar", "gzip", "zip",
 *                        "7z").
 * @return `1` if the tool is found and executable, `0` otherwise.
 */
int         d_env_archive_has_tool(const char* _tool_name);
/**
 * @brief Prints the detected archive backends, their versions, and the
 *        resulting per-format capability matrix to stdout.
 */
void        d_env_archive_print_info(void);

D_EXTERN_C_END


#endif  // DJINTERP_ENV_ENV_ARCHIVE_H
