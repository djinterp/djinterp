/*******************************************************************************
* djinterp [core]                                                env_archive.h
*
*   djinterp archive-library environment detection header:
* This header provides compile-time detection of third-party archive libraries
* and maps them onto a portable capability matrix for the four target formats
* requested by the archive layer: zip, tar, gz (gzip), and 7z. It builds on
* env_compression.h for the underlying codecs and, like that header, performs
* presence-only detection (via __has_include) without #including any of the
* third-party headers, so it adds no hard dependency.
*
* scope:
*   - libarchive (multi-format: zip, tar, gz, 7z, cpio, iso, and more)
*   - libzip (zip read / write)
*   - minizip and minizip-ng (zip read / write)
*   - libtar (tar read / write)
*   - 7-Zip / LZMA SDK (7z) and the bit7z C++ wrapper
*   - per-format read / write capability roll-ups
*   - recommended-backend selection per format
*   - runtime probe and info declarations (incl. external-tool fallback)
*
* usage:
*   Include env.h first (directly or transitively), then this header:
*     #include "./env.h"
*     #include "./env_archive.h"
*   (env_compression.h is pulled in automatically.)
*
*   A build system may pre-define any D_ENV_ARCHIVE_HAVE_* macro to force a
*   result and bypass the __has_include probe.
*
* NAMING CONVENTION:
*   D_ENV_ARCHIVE_HAVE_[LIB]         - 1 if the library is available, 0 if not
*   D_ENV_ARCHIVE_[LIB]_[FIELD]      - version / metadata for a detected library
*   D_ENV_ARCHIVE_CAN_[READ|WRITE]_[FORMAT] - format capability roll-ups
*
* path:      \inc\core\env\env_archive.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.05.23
*******************************************************************************/

#ifndef DJINTERP_ENV_ARCHIVE_
#define DJINTERP_ENV_ARCHIVE_ 1

#include "./env.h"
#include "./env_compression.h"


// =============================================================================
// I.   CONFIGURATION
// =============================================================================

// D_CFG_ENV_ARCHIVE_ENABLED
//   configuration: master toggle for archive-library detection. when 0, every
// D_ENV_ARCHIVE_HAVE_* flag resolves to 0 unless explicitly pre-defined.
#ifndef D_CFG_ENV_ARCHIVE_ENABLED
    #define D_CFG_ENV_ARCHIVE_ENABLED 1
#endif

// D_INTERNAL_ARCHIVE_PROBE
//   macro: internal helper. yields the __has_include result for `header`
// when detection is enabled, and 0 when the master toggle is off.
#if D_CFG_ENV_ARCHIVE_ENABLED
    #define D_INTERNAL_ARCHIVE_PROBE(header) D_ENV_HAS_INCLUDE(header)
#else
    #define D_INTERNAL_ARCHIVE_PROBE(header) 0
#endif

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
#endif

#ifndef D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP
    #define D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP  1
#endif


// =============================================================================
// II.  LIBARCHIVE (multi-format)
// =============================================================================

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
#endif

// D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY
//   feature: detect the companion <archive_entry.h> header, required for
// per-entry metadata when writing archives.
#ifndef D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY
    #if D_INTERNAL_ARCHIVE_PROBE(<archive_entry.h>)
        #define D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY 1
    #else
        #define D_ENV_ARCHIVE_HAVE_LIBARCHIVE_ENTRY 0
    #endif
#endif

// libarchive version metadata (only populated when <archive.h> is included)
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

// D_ENV_ARCHIVE_LIBARCHIVE_AT_LEAST
//   macro: evaluates to 1 if the included libarchive reports at least the
// given version. ARCHIVE_VERSION_NUMBER is MAJOR*1000000 + MINOR*1000 + REV.
#define D_ENV_ARCHIVE_LIBARCHIVE_AT_LEAST(major, minor, rev)                   \
    ( D_ENV_ARCHIVE_HAVE_LIBARCHIVE &&                                         \
      (D_ENV_ARCHIVE_LIBARCHIVE_VERNUM >=                                      \
       (((major) * 1000000) + ((minor) * 1000) + (rev))) )


// =============================================================================
// III. LIBZIP
// =============================================================================

// D_ENV_ARCHIVE_HAVE_LIBZIP
//   feature: detect if libzip (<zip.h>) is available. libzip supports both
// reading and writing of zip archives.
#ifndef D_ENV_ARCHIVE_HAVE_LIBZIP
    #if D_INTERNAL_ARCHIVE_PROBE(<zip.h>)
        #define D_ENV_ARCHIVE_HAVE_LIBZIP           1
    #else
        #define D_ENV_ARCHIVE_HAVE_LIBZIP           0
    #endif
#endif

#if defined(LIBZIP_VERSION)
    #define D_ENV_ARCHIVE_LIBZIP_VERSION_STR        LIBZIP_VERSION
#else
    #define D_ENV_ARCHIVE_LIBZIP_VERSION_STR        "unknown"
#endif


// =============================================================================
// IV.  MINIZIP / MINIZIP-NG
// =============================================================================

// D_ENV_ARCHIVE_HAVE_MINIZIP_NG
//   feature: detect minizip-ng (<mz.h>), the modern rewrite supporting zip
// read / write with pluggable codecs and encryption.
#ifndef D_ENV_ARCHIVE_HAVE_MINIZIP_NG
    #if D_INTERNAL_ARCHIVE_PROBE(<mz.h>)
        #define D_ENV_ARCHIVE_HAVE_MINIZIP_NG       1
    #else
        #define D_ENV_ARCHIVE_HAVE_MINIZIP_NG       0
    #endif
#endif

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
#endif

// D_ENV_ARCHIVE_HAVE_MINIZIP
//   feature: 1 if either minizip flavour is available.
#define D_ENV_ARCHIVE_HAVE_MINIZIP                                             \
    ( D_ENV_ARCHIVE_HAVE_MINIZIP_NG ||                                         \
      D_ENV_ARCHIVE_HAVE_MINIZIP_CLASSIC )


// =============================================================================
// V.   LIBTAR
// =============================================================================

// D_ENV_ARCHIVE_HAVE_LIBTAR
//   feature: detect if libtar (<libtar.h>) is available. libtar reads and
// writes uncompressed tar; compression is layered separately (e.g. gzip).
#ifndef D_ENV_ARCHIVE_HAVE_LIBTAR
    #if D_INTERNAL_ARCHIVE_PROBE(<libtar.h>)
        #define D_ENV_ARCHIVE_HAVE_LIBTAR           1
    #else
        #define D_ENV_ARCHIVE_HAVE_LIBTAR           0
    #endif
#endif


// =============================================================================
// VI.  7-ZIP / LZMA SDK AND bit7z
// =============================================================================

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
#endif

// D_ENV_ARCHIVE_HAVE_BIT7Z
//   feature: detect the bit7z C++ wrapper (<bit7z/bittypes.hpp>), which wraps
// the 7-Zip library for full 7z read / write from C++.
// note: bit7z is C++ only.
#ifndef D_ENV_ARCHIVE_HAVE_BIT7Z
    #if ( defined(__cplusplus) &&                                             \
          ( D_INTERNAL_ARCHIVE_PROBE(<bit7z/bittypes.hpp>) ||                  \
            D_INTERNAL_ARCHIVE_PROBE(<bit7z/bit7z.hpp>) ) )
        #define D_ENV_ARCHIVE_HAVE_BIT7Z            1
    #else
        #define D_ENV_ARCHIVE_HAVE_BIT7Z            0
    #endif
#endif

// D_ENV_ARCHIVE_HAVE_7ZIP
//   feature: 1 if any 7z-capable backend is available.
#define D_ENV_ARCHIVE_HAVE_7ZIP                                               \
    ( D_ENV_ARCHIVE_HAVE_LZMA_SDK ||                                          \
      D_ENV_ARCHIVE_HAVE_BIT7Z    ||                                          \
      D_ENV_ARCHIVE_HAVE_LIBARCHIVE )


// =============================================================================
// VII. RAR (UNRAR / WINRAR)
// =============================================================================
// RAR is asymmetric. Extraction is widely available, but creation is
// proprietary: no library can write RAR. Only the RARLAB `rar` / WinRAR
// command-line tool produces .rar archives. libarchive and 7-Zip backends can
// read RAR but never write it.

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
#endif

// D_ENV_ARCHIVE_HAVE_RAR_TOOL
//   feature: presence of the RARLAB `rar` / WinRAR executable, the only means
// of *creating* RAR archives. this is not compile-time detectable, so it
// defaults to 0; set it from the build system, or confirm at runtime with
// d_env_archive_has_tool("rar"). reading can also use d_env_archive_has_tool
// ("unrar").
#ifndef D_ENV_ARCHIVE_HAVE_RAR_TOOL
    #define D_ENV_ARCHIVE_HAVE_RAR_TOOL             0
#endif

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


// =============================================================================
// VIII. PLATFORM-NATIVE ARCHIVERS
// =============================================================================
// These flag archiver facilities shipped with the operating system itself,
// using the OS classification from env.h. Tool-bundling flags are prefixed
// LIKELY_ because they are inferred from the OS version and must be confirmed
// at runtime via d_env_archive_has_tool(). All probes degrade to 0 when OS
// detection is unavailable (D_ENV_OS_ID undefined).

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
#endif

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
#endif

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
#endif


// =============================================================================
// IX.  FORMAT CAPABILITY MATRIX
// =============================================================================
// The roll-ups below answer "can this build read / write <format>?" without
// the caller needing to know which backend is present. They combine library
// availability with the codec roll-ups from env_compression.h where a format
// requires a codec (e.g. zip "deflate", or tar.gz).

// -----------------------------------------------------------------------------
// A.  zip
// -----------------------------------------------------------------------------

// D_ENV_ARCHIVE_CAN_READ_ZIP
//   feature: 1 if some backend can read zip archives.
#define D_ENV_ARCHIVE_CAN_READ_ZIP                                            \
    ( D_ENV_ARCHIVE_HAVE_LIBARCHIVE      ||                                   \
      D_ENV_ARCHIVE_HAVE_LIBZIP          ||                                   \
      D_ENV_ARCHIVE_HAVE_MINIZIP         ||                                   \
      D_ENV_COMPRESSION_HAVE_MINIZ       ||                                   \
      D_ENV_ARCHIVE_HAVE_WIN_SHELL_ZIP   ||                                   \
      D_ENV_ARCHIVE_HAVE_BUILTIN_ZIP )

// D_ENV_ARCHIVE_CAN_WRITE_ZIP
//   feature: 1 if some backend can write zip archives.
#define D_ENV_ARCHIVE_CAN_WRITE_ZIP         D_ENV_ARCHIVE_CAN_READ_ZIP

// -----------------------------------------------------------------------------
// B.  tar
// -----------------------------------------------------------------------------

// D_ENV_ARCHIVE_CAN_READ_TAR
//   feature: 1 if some backend can read tar archives (uncompressed).
#define D_ENV_ARCHIVE_CAN_READ_TAR                                            \
    ( D_ENV_ARCHIVE_HAVE_LIBARCHIVE ||                                        \
      D_ENV_ARCHIVE_HAVE_LIBTAR     ||                                        \
      D_ENV_ARCHIVE_HAVE_BUILTIN_TAR )

// D_ENV_ARCHIVE_CAN_WRITE_TAR
//   feature: 1 if some backend can write tar archives (uncompressed).
#define D_ENV_ARCHIVE_CAN_WRITE_TAR         D_ENV_ARCHIVE_CAN_READ_TAR

// D_ENV_ARCHIVE_CAN_WRITE_TGZ
//   feature: 1 if a compressed tar.gz can be produced (a tar writer plus a
// gzip codec, or libarchive which bundles the gzip filter).
#define D_ENV_ARCHIVE_CAN_WRITE_TGZ                                           \
    ( D_ENV_ARCHIVE_HAVE_LIBARCHIVE ||                                        \
      ( D_ENV_ARCHIVE_HAVE_LIBTAR      && D_ENV_COMPRESSION_HAVE_GZIP ) ||    \
      ( D_ENV_ARCHIVE_HAVE_BUILTIN_TAR && D_ENV_COMPRESSION_HAVE_GZIP_WRAP ) )

// -----------------------------------------------------------------------------
// C.  gz (gzip stream)
// -----------------------------------------------------------------------------

// D_ENV_ARCHIVE_CAN_READ_GZ
//   feature: 1 if a gzip stream can be read (any gzip-container codec or the
// libarchive gzip filter).
#define D_ENV_ARCHIVE_CAN_READ_GZ                                             \
    ( D_ENV_COMPRESSION_HAVE_GZIP_WRAP ||                                     \
      D_ENV_ARCHIVE_HAVE_LIBARCHIVE )

// D_ENV_ARCHIVE_CAN_WRITE_GZ
//   feature: 1 if a gzip stream can be written.
#define D_ENV_ARCHIVE_CAN_WRITE_GZ          D_ENV_ARCHIVE_CAN_READ_GZ

// -----------------------------------------------------------------------------
// D.  7z
// -----------------------------------------------------------------------------

// D_ENV_ARCHIVE_CAN_READ_7Z
//   feature: 1 if some backend can read 7z archives.
#define D_ENV_ARCHIVE_CAN_READ_7Z           D_ENV_ARCHIVE_HAVE_7ZIP

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

// -----------------------------------------------------------------------------
// E.  rar
// -----------------------------------------------------------------------------

// D_ENV_ARCHIVE_CAN_READ_RAR
//   feature: 1 if some backend can extract RAR (UnRAR, libarchive's RAR
// reader, a 7-Zip-based backend, or the rar/unrar tool).
#define D_ENV_ARCHIVE_CAN_READ_RAR                                            \
    ( D_ENV_ARCHIVE_HAVE_UNRAR      ||                                        \
      D_ENV_ARCHIVE_HAVE_LIBARCHIVE ||                                        \
      D_ENV_ARCHIVE_HAVE_BIT7Z      ||                                        \
      D_ENV_ARCHIVE_HAVE_RAR_TOOL )

// D_ENV_ARCHIVE_CAN_WRITE_RAR
//   feature: 1 only if the proprietary rar/WinRAR tool is present. no library
// can create RAR, so this never lights up from a linked dependency alone.
#define D_ENV_ARCHIVE_CAN_WRITE_RAR         D_ENV_ARCHIVE_HAVE_RAR_TOOL

// -----------------------------------------------------------------------------
// F.  aggregate
// -----------------------------------------------------------------------------

// D_ENV_ARCHIVE_CAN_WRITE_ANY
//   feature: 1 if any supported format can be written.
#define D_ENV_ARCHIVE_CAN_WRITE_ANY                                           \
    ( D_ENV_ARCHIVE_CAN_WRITE_ZIP ||                                          \
      D_ENV_ARCHIVE_CAN_WRITE_TAR ||                                          \
      D_ENV_ARCHIVE_CAN_WRITE_GZ  ||                                          \
      D_ENV_ARCHIVE_CAN_WRITE_7Z  ||                                          \
      D_ENV_ARCHIVE_CAN_WRITE_RAR )


// =============================================================================
// X.   RECOMMENDED-BACKEND SELECTION
// =============================================================================

// backend identifier constants (stable small integers for runtime dispatch)
#define D_ENV_ARCHIVE_BACKEND_NONE          0
#define D_ENV_ARCHIVE_BACKEND_LIBARCHIVE    1
#define D_ENV_ARCHIVE_BACKEND_LIBZIP        2
#define D_ENV_ARCHIVE_BACKEND_MINIZIP_NG    3
#define D_ENV_ARCHIVE_BACKEND_MINIZIP       4
#define D_ENV_ARCHIVE_BACKEND_MINIZ         5
#define D_ENV_ARCHIVE_BACKEND_LIBTAR        6
#define D_ENV_ARCHIVE_BACKEND_LZMA_SDK      7
#define D_ENV_ARCHIVE_BACKEND_BIT7Z         8
#define D_ENV_ARCHIVE_BACKEND_UNRAR         9
#define D_ENV_ARCHIVE_BACKEND_RAR_TOOL      10
#define D_ENV_ARCHIVE_BACKEND_BUILTIN       11

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

// D_ENV_ARCHIVE_PREFERRED_RAR_WRITE
//   constant: preferred RAR creation backend. only the proprietary tool can
// write RAR, so this is the tool or nothing.
#if D_ENV_ARCHIVE_HAVE_RAR_TOOL
    #define D_ENV_ARCHIVE_PREFERRED_RAR_WRITE D_ENV_ARCHIVE_BACKEND_RAR_TOOL
#else
    #define D_ENV_ARCHIVE_PREFERRED_RAR_WRITE D_ENV_ARCHIVE_BACKEND_NONE
#endif


// =============================================================================
// XI.  RUNTIME PROBE AND INFO DECLARATIONS
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// d_env_archive_backend_name
//   function: returns a human-readable name for a backend identifier.
//   params:
//     backend - one of the D_ENV_ARCHIVE_BACKEND_* constants.
//   returns: a static, NUL-terminated backend name, or "none".
const char* d_env_archive_backend_name(int backend);

// d_env_archive_libarchive_runtime_version
//   function: returns the libarchive runtime version string via
// archive_version_string(), or "unavailable" when libarchive was not linked.
const char* d_env_archive_libarchive_runtime_version(void);

// d_env_archive_has_tool
//   function: probes for an external archiver executable on PATH, enabling a
// shell-out fallback when no library backend is linked.
//   params:
//     tool_name - command name to look for (e.g. "tar", "gzip", "zip", "7z").
//   returns: 1 if the tool is found and executable, 0 otherwise.
int d_env_archive_has_tool(const char* tool_name);

// d_env_archive_print_info
//   function: prints the detected archive backends, their versions, and the
// resulting per-format capability matrix to stdout.
//   returns: none.
void d_env_archive_print_info(void);

#ifdef __cplusplus
}
#endif


#endif  // DJINTERP_ENV_ARCHIVE_
