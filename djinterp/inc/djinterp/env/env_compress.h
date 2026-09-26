/*******************************************************************************
* djinterp [env]                                                  env_compress.h
*
* djinterp compression-codec detection.
*   Compile-time detection of the third-party compression libraries the
* portable archive layer (zip, tar, gz, 7z) builds on: zlib, zlib-ng, miniz,
* libdeflate, bzip2, liblzma, zstd, lz4, and brotli, plus the Apple and
* Windows platform codecs. For each it exposes presence, any version
* information the library publishes, codec-capability roll-ups (DEFLATE, gzip,
* bzip2, xz, zstd, lz4, brotli), recommended-codec selection, and runtime
* query declarations.
*   Detection is presence-only, via __has_include; this header never includes
* the third-party headers, so including it adds no hard dependency. Version
* information is populated only when the translation unit has already included
* the library's own header. A build system may pre-define any
* D_ENV_COMPRESSION_HAVE_* macro to force a result and bypass the probe.
*   Names follow three patterns: D_ENV_COMPRESSION_HAVE_<LIB> is 1 when the
* library is available; D_ENV_COMPRESSION_<LIB>_<FIELD> carries version
* metadata for a detected library; D_ENV_COMPRESSION_HAVE_<CODEC> is 1 when
* some library provides the codec.
*
* path:      /inc/djinterp/env/env_compress.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONFIGURATION AND PROBING
    -------------------------
    1.  Configuration
         1.  D_CFG_ENV_COMPRESSION_ENABLED
    2.  Probing
         1.  D_ENV_HAS_INCLUDE
         2.  D_INTERNAL_COMPRESSION_PROBE
2.  ZLIB FAMILY
    -----------
    1.  zlib
         1.  D_ENV_COMPRESSION_HAVE_ZLIB
         2.  zlib version metadata
         3.  D_ENV_COMPRESSION_ZLIB_AT_LEAST
    2.  zlib-ng
         1.  D_ENV_COMPRESSION_HAVE_ZLIBNG
         2.  zlib-ng version metadata
    3.  miniz
         1.  D_ENV_COMPRESSION_HAVE_MINIZ
         2.  miniz version metadata
    4.  libdeflate
         1.  D_ENV_COMPRESSION_HAVE_LIBDEFLATE
         2.  libdeflate version metadata
3.  OTHER CODEC LIBRARIES
    ---------------------
    1.  bzip2
         1.  D_ENV_COMPRESSION_HAVE_BZIP2
    2.  liblzma (xz / .lzma)
         1.  D_ENV_COMPRESSION_HAVE_LZMA
         2.  liblzma version metadata
    3.  zstd (Zstandard)
         1.  D_ENV_COMPRESSION_HAVE_ZSTD
         2.  zstd version metadata
         3.  D_ENV_COMPRESSION_ZSTD_AT_LEAST
    4.  lz4
         1.  D_ENV_COMPRESSION_HAVE_LZ4
         2.  D_ENV_COMPRESSION_HAVE_LZ4_FRAME
         3.  lz4 version metadata
    5.  brotli
         1.  D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE
         2.  D_ENV_COMPRESSION_HAVE_BROTLI_DECODE
         3.  D_ENV_COMPRESSION_HAVE_BROTLI
4.  PLATFORM-NATIVE CODECS
    ----------------------
    1.  Operating-system codecs
         1.  D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION
         2.  D_ENV_COMPRESSION_HAVE_WIN_COMPRESSION_API
         3.  D_ENV_COMPRESSION_HAVE_PLATFORM_NATIVE
5.  CODEC-CAPABILITY ROLL-UPS
    -------------------------
    1.  Codec availability
         1.  D_ENV_COMPRESSION_HAVE_DEFLATE
         2.  D_ENV_COMPRESSION_HAVE_ZLIB_WRAP
         3.  D_ENV_COMPRESSION_HAVE_GZIP
         4.  D_ENV_COMPRESSION_HAVE_GZIP_WRAP
         5.  D_ENV_COMPRESSION_HAVE_XZ
         6.  D_ENV_COMPRESSION_HAVE_ANY
6.  RECOMMENDED-CODEC SELECTION
    ---------------------------
    1.  Codec identifiers
         1.  D_ENV_COMPRESSION_CODEC_*
              1.  D_ENV_COMPRESSION_CODEC_NONE
              2.  D_ENV_COMPRESSION_CODEC_ZLIB
              3.  D_ENV_COMPRESSION_CODEC_ZLIBNG
              4.  D_ENV_COMPRESSION_CODEC_MINIZ
              5.  D_ENV_COMPRESSION_CODEC_BZIP2
              6.  D_ENV_COMPRESSION_CODEC_LZMA
              7.  D_ENV_COMPRESSION_CODEC_ZSTD
              8.  D_ENV_COMPRESSION_CODEC_LZ4
              9.  D_ENV_COMPRESSION_CODEC_BROTLI
              10. D_ENV_COMPRESSION_CODEC_LIBDEFLATE
    2.  Preferred codecs
         1.  D_ENV_COMPRESSION_PREFERRED_DEFLATE
         2.  D_ENV_COMPRESSION_PREFERRED_GZIP
7.  RUNTIME QUERIES
    ---------------
    1.  Runtime queries
*/

#ifndef DJINTERP_ENV_ENV_COMPRESS_H
#define DJINTERP_ENV_ENV_COMPRESS_H 1


// djinterp
#include "../c/djinterp.h"  // D_EXTERN_C_BEGIN, D_EXTERN_C_END
#include "./env.h"          // D_ENV_OS_ID, D_ENV_IS_OS_WINDOWS


//==============================================================================
// 1.  CONFIGURATION AND PROBING
//==============================================================================


// 1.1    Configuration
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_COMPRESSION_ENABLED
//   configuration: master toggle for codec-library detection. when 0, every
// D_ENV_COMPRESSION_HAVE_* flag resolves to 0 unless explicitly pre-defined.
#ifndef D_CFG_ENV_COMPRESSION_ENABLED
    #define D_CFG_ENV_COMPRESSION_ENABLED 1
#endif  // D_CFG_ENV_COMPRESSION_ENABLED

// 1.2    Probing
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_HAS_INCLUDE
//   macro: portable wrapper around __has_include. evaluates to 1 when the
// named header is locatable by the preprocessor, 0 otherwise (or when the
// compiler lacks __has_include, in which case detection falls back to 0 and
// build-system overrides are required).
// note: guarded with #ifndef so it may be promoted to env.h without clashing.
#ifndef D_ENV_HAS_INCLUDE
    #if defined(__has_include)
        #define D_ENV_HAS_INCLUDE(header) __has_include(header)
    #else
        #define D_ENV_HAS_INCLUDE(header) 0
    #endif
#endif  // D_ENV_HAS_INCLUDE

// 1.2.2
// D_INTERNAL_COMPRESSION_PROBE
//   macro: internal helper. yields the __has_include result for `header`
// when detection is enabled, and 0 when the master toggle is off.
#if D_CFG_ENV_COMPRESSION_ENABLED
    #define D_INTERNAL_COMPRESSION_PROBE(header) D_ENV_HAS_INCLUDE(header)
#else
    #define D_INTERNAL_COMPRESSION_PROBE(header) 0
#endif


//==============================================================================
// 2.  ZLIB FAMILY
//==============================================================================
// DEFLATE providers and the gzip and zlib wrappers built on it.


// 2.1    zlib
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_COMPRESSION_HAVE_ZLIB
//   feature: detect if zlib (<zlib.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_ZLIB
    #if D_INTERNAL_COMPRESSION_PROBE(<zlib.h>)
        #define D_ENV_COMPRESSION_HAVE_ZLIB     1
    #else
        #define D_ENV_COMPRESSION_HAVE_ZLIB     0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_ZLIB

// 2.1.2
// zlib version metadata
//   constant: D_ENV_COMPRESSION_ZLIB_VERNUM and _VERSION_STR, from the
// ZLIB_VERNUM / ZLIB_VERSION macros; populated only when <zlib.h> has been
// included, and otherwise 0 and "unknown".
#if defined(ZLIB_VERNUM)
    #define D_ENV_COMPRESSION_ZLIB_VERNUM       ZLIB_VERNUM
#else
    #define D_ENV_COMPRESSION_ZLIB_VERNUM       0
#endif

#if defined(ZLIB_VERSION)
    #define D_ENV_COMPRESSION_ZLIB_VERSION_STR  ZLIB_VERSION
#else
    #define D_ENV_COMPRESSION_ZLIB_VERSION_STR  "unknown"
#endif

// 2.1.3
// D_ENV_COMPRESSION_ZLIB_AT_LEAST
//   macro: evaluates to 1 if the included zlib reports at least the given
// version. encoding mirrors ZLIB_VERNUM: 0xMNR0 (major, minor, revision).
#define D_ENV_COMPRESSION_ZLIB_AT_LEAST(major, minor, rev)                     \
    ( D_ENV_COMPRESSION_HAVE_ZLIB &&                                           \
      (D_ENV_COMPRESSION_ZLIB_VERNUM >=                                        \
       (((major) << 12) | ((minor) << 8) | ((rev) << 4))) )

// 2.2    zlib-ng
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_COMPRESSION_HAVE_ZLIBNG
//   feature: detect if zlib-ng is available, in either its native (<zlib-ng.h>,
// zng_ prefixed) or compat (masquerades as zlib) configuration.
#ifndef D_ENV_COMPRESSION_HAVE_ZLIBNG
    #if ( D_INTERNAL_COMPRESSION_PROBE(<zlib-ng.h>) ||                         \
          defined(ZLIBNG_VERSION)                   ||                         \
          defined(ZLIBNG_VER_STRING) )
        #define D_ENV_COMPRESSION_HAVE_ZLIBNG       1
    #else
        #define D_ENV_COMPRESSION_HAVE_ZLIBNG       0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_ZLIBNG

// 2.2.2
// zlib-ng version metadata
//   constant: D_ENV_COMPRESSION_ZLIBNG_VERSION_STR, from ZLIBNG_VER_STRING or
// ZLIBNG_VERSION once zlib-ng's header is included; "unknown" otherwise.
#if defined(ZLIBNG_VER_STRING)
    #define D_ENV_COMPRESSION_ZLIBNG_VERSION_STR    ZLIBNG_VER_STRING
#elif defined(ZLIBNG_VERSION)
    #define D_ENV_COMPRESSION_ZLIBNG_VERSION_STR    ZLIBNG_VERSION
#else
    #define D_ENV_COMPRESSION_ZLIBNG_VERSION_STR    "unknown"
#endif

// 2.3    miniz
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_COMPRESSION_HAVE_MINIZ
//   feature: detect if miniz (<miniz.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_MINIZ
    #if D_INTERNAL_COMPRESSION_PROBE(<miniz.h>)
        #define D_ENV_COMPRESSION_HAVE_MINIZ        1
    #else
        #define D_ENV_COMPRESSION_HAVE_MINIZ        0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_MINIZ

// 2.3.2
// miniz version metadata
//   constant: D_ENV_COMPRESSION_MINIZ_VERSION_STR, from MZ_VERSION once
// <miniz.h> is included; "unknown" otherwise.
#if defined(MZ_VERSION)
    #define D_ENV_COMPRESSION_MINIZ_VERSION_STR     MZ_VERSION
#else
    #define D_ENV_COMPRESSION_MINIZ_VERSION_STR     "unknown"
#endif

// 2.4    libdeflate
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_COMPRESSION_HAVE_LIBDEFLATE
//   feature: detect libdeflate (<libdeflate.h>), a high-throughput codec for
// the DEFLATE, zlib, and gzip container formats. it operates on whole buffers
// and does not provide a streaming gz* file API, so it is folded into the
// DEFLATE roll-up but not into the streaming-gzip roll-up below.
#ifndef D_ENV_COMPRESSION_HAVE_LIBDEFLATE
    #if D_INTERNAL_COMPRESSION_PROBE(<libdeflate.h>)
        #define D_ENV_COMPRESSION_HAVE_LIBDEFLATE   1
    #else
        #define D_ENV_COMPRESSION_HAVE_LIBDEFLATE   0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_LIBDEFLATE

// 2.4.2
// libdeflate version metadata
//   constant: D_ENV_COMPRESSION_LIBDEFLATE_VERSION_STR, from
// LIBDEFLATE_VERSION_STRING once <libdeflate.h> is included; "unknown"
// otherwise.
#if defined(LIBDEFLATE_VERSION_STRING)
    #define D_ENV_COMPRESSION_LIBDEFLATE_VERSION_STR LIBDEFLATE_VERSION_STRING
#else
    #define D_ENV_COMPRESSION_LIBDEFLATE_VERSION_STR "unknown"
#endif


//==============================================================================
// 3.  OTHER CODEC LIBRARIES
//==============================================================================


// 3.1    bzip2
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_COMPRESSION_HAVE_BZIP2
//   feature: detect if bzip2 (<bzlib.h>) is available.
// note: bzip2 exposes its version only at runtime via BZ2_bzlibVersion();
// there is no compile-time version macro.
#ifndef D_ENV_COMPRESSION_HAVE_BZIP2
    #if D_INTERNAL_COMPRESSION_PROBE(<bzlib.h>)
        #define D_ENV_COMPRESSION_HAVE_BZIP2        1
    #else
        #define D_ENV_COMPRESSION_HAVE_BZIP2        0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_BZIP2

// 3.2    liblzma (xz / .lzma)
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_COMPRESSION_HAVE_LZMA
//   feature: detect if liblzma (<lzma.h>) is available. liblzma provides the
// xz container, the legacy .lzma format, and raw LZMA1/LZMA2 streams.
#ifndef D_ENV_COMPRESSION_HAVE_LZMA
    #if D_INTERNAL_COMPRESSION_PROBE(<lzma.h>)
        #define D_ENV_COMPRESSION_HAVE_LZMA         1
    #else
        #define D_ENV_COMPRESSION_HAVE_LZMA         0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_LZMA

// 3.2.2
// liblzma version metadata
//   constant: D_ENV_COMPRESSION_LZMA_VERNUM and _VERSION_STR, from
// LZMA_VERSION / LZMA_VERSION_STRING once <lzma.h> is included; otherwise 0
// and "unknown".
#if defined(LZMA_VERSION)
    #define D_ENV_COMPRESSION_LZMA_VERNUM           LZMA_VERSION
#else
    #define D_ENV_COMPRESSION_LZMA_VERNUM           0
#endif

#if defined(LZMA_VERSION_STRING)
    #define D_ENV_COMPRESSION_LZMA_VERSION_STR      LZMA_VERSION_STRING
#else
    #define D_ENV_COMPRESSION_LZMA_VERSION_STR      "unknown"
#endif

// 3.3    zstd (Zstandard)
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_COMPRESSION_HAVE_ZSTD
//   feature: detect if zstd (<zstd.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_ZSTD
    #if D_INTERNAL_COMPRESSION_PROBE(<zstd.h>)
        #define D_ENV_COMPRESSION_HAVE_ZSTD         1
    #else
        #define D_ENV_COMPRESSION_HAVE_ZSTD         0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_ZSTD

// 3.3.2
// zstd version metadata
//   constant: D_ENV_COMPRESSION_ZSTD_VERNUM and _VERSION_STR, from
// ZSTD_VERSION_NUMBER / ZSTD_VERSION_STRING once <zstd.h> is included;
// otherwise 0 and "unknown".
#if defined(ZSTD_VERSION_NUMBER)
    #define D_ENV_COMPRESSION_ZSTD_VERNUM           ZSTD_VERSION_NUMBER
#else
    #define D_ENV_COMPRESSION_ZSTD_VERNUM           0
#endif

#if defined(ZSTD_VERSION_STRING)
    #define D_ENV_COMPRESSION_ZSTD_VERSION_STR      ZSTD_VERSION_STRING
#else
    #define D_ENV_COMPRESSION_ZSTD_VERSION_STR      "unknown"
#endif

// 3.3.3
// D_ENV_COMPRESSION_ZSTD_AT_LEAST
//   macro: evaluates to 1 if the included zstd reports at least the given
// version. ZSTD_VERSION_NUMBER is encoded as MAJOR*10000 + MINOR*100 + RELEASE.
#define D_ENV_COMPRESSION_ZSTD_AT_LEAST(major, minor, rel)                     \
    ( D_ENV_COMPRESSION_HAVE_ZSTD &&                                           \
      (D_ENV_COMPRESSION_ZSTD_VERNUM >=                                        \
       (((major) * 10000) + ((minor) * 100) + (rel))) )

// 3.4    lz4
//------------------------------------------------------------------------------
// 3.4.1
// D_ENV_COMPRESSION_HAVE_LZ4
//   feature: detect if lz4 (<lz4.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_LZ4
    #if D_INTERNAL_COMPRESSION_PROBE(<lz4.h>)
        #define D_ENV_COMPRESSION_HAVE_LZ4          1
    #else
        #define D_ENV_COMPRESSION_HAVE_LZ4          0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_LZ4

// 3.4.2
// D_ENV_COMPRESSION_HAVE_LZ4_FRAME
//   feature: detect if the lz4 frame API (<lz4frame.h>) is available. the
// frame format is required for interoperable .lz4 files.
#ifndef D_ENV_COMPRESSION_HAVE_LZ4_FRAME
    #if D_INTERNAL_COMPRESSION_PROBE(<lz4frame.h>)
        #define D_ENV_COMPRESSION_HAVE_LZ4_FRAME    1
    #else
        #define D_ENV_COMPRESSION_HAVE_LZ4_FRAME    0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_LZ4_FRAME

// 3.4.3
// lz4 version metadata
//   constant: D_ENV_COMPRESSION_LZ4_VERNUM and _VERSION_STR, from
// LZ4_VERSION_NUMBER / LZ4_VERSION_STRING once <lz4.h> is included;
// otherwise 0 and "unknown".
#if defined(LZ4_VERSION_NUMBER)
    #define D_ENV_COMPRESSION_LZ4_VERNUM            LZ4_VERSION_NUMBER
#else
    #define D_ENV_COMPRESSION_LZ4_VERNUM            0
#endif

#if defined(LZ4_VERSION_STRING)
    #define D_ENV_COMPRESSION_LZ4_VERSION_STR       LZ4_VERSION_STRING
#else
    #define D_ENV_COMPRESSION_LZ4_VERSION_STR       "unknown"
#endif

// 3.5    brotli
//------------------------------------------------------------------------------
// 3.5.1
// D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE
//   feature: detect if the brotli encoder (<brotli/encode.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE
    #if D_INTERNAL_COMPRESSION_PROBE(<brotli/encode.h>)
        #define D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE 1
    #else
        #define D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE 0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE

// 3.5.2
// D_ENV_COMPRESSION_HAVE_BROTLI_DECODE
//   feature: detect if the brotli decoder (<brotli/decode.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_BROTLI_DECODE
    #if D_INTERNAL_COMPRESSION_PROBE(<brotli/decode.h>)
        #define D_ENV_COMPRESSION_HAVE_BROTLI_DECODE 1
    #else
        #define D_ENV_COMPRESSION_HAVE_BROTLI_DECODE 0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_BROTLI_DECODE

// 3.5.3
// D_ENV_COMPRESSION_HAVE_BROTLI
//   feature: 1 if both brotli encoder and decoder are available.
#define D_ENV_COMPRESSION_HAVE_BROTLI                                          \
    ( D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE &&                                  \
      D_ENV_COMPRESSION_HAVE_BROTLI_DECODE )


//==============================================================================
// 4.  PLATFORM-NATIVE CODECS
//==============================================================================
// These detect codec facilities shipped with the operating system itself,
// using the OS classification from env.h. They are kept distinct from the
// portable libraries above because their APIs differ; the archive layer may
// consult them explicitly as a no-dependency fallback. All probes degrade to
// 0 when OS detection is unavailable (D_ENV_OS_ID undefined).


// 4.1    Operating-system codecs
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION
//   feature: detect Apple's libcompression (<compress.h>), which provides
// LZFSE, LZ4, raw DEFLATE (ZLIB), and LZMA. ships on macOS 10.11+ and iOS 9+.
#ifndef D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION
    #if ( defined(D_ENV_OS_ID)                          &&                    \
          ( (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) ||                           \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_IOS)   ||                           \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_APPLE) )      &&                    \
          D_INTERNAL_COMPRESSION_PROBE(<compress.h>) )
        #define D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION 1
    #else
        #define D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION 0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION

// 4.1.2
// D_ENV_COMPRESSION_HAVE_WIN_COMPRESSION_API
//   feature: detect the Windows Compression API (<compressapi.h>, Cabinet.dll):
// MSZIP, XPRESS, XPRESS_HUFF, and LZMS. desktop Windows 8 and later.
// note: MSZIP is a DEFLATE variant but is not interchangeable with RFC 1952
// gzip, so this flag is not folded into the gzip roll-up below.
#ifndef D_ENV_COMPRESSION_HAVE_WIN_COMPRESSION_API
    #if ( defined(D_ENV_OS_ID)                          &&                    \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)              &&                    \
          D_INTERNAL_COMPRESSION_PROBE(<compressapi.h>) )
        #define D_ENV_COMPRESSION_HAVE_WIN_COMPRESSION_API 1
    #else
        #define D_ENV_COMPRESSION_HAVE_WIN_COMPRESSION_API 0
    #endif
#endif  // D_ENV_COMPRESSION_HAVE_WIN_COMPRESSION_API

// 4.1.3
// D_ENV_COMPRESSION_HAVE_PLATFORM_NATIVE
//   feature: 1 if any OS-native codec facility was detected.
#define D_ENV_COMPRESSION_HAVE_PLATFORM_NATIVE                                \
    ( D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION ||                          \
      D_ENV_COMPRESSION_HAVE_WIN_COMPRESSION_API )


//==============================================================================
// 5.  CODEC-CAPABILITY ROLL-UPS
//==============================================================================


// 5.1    Codec availability
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_COMPRESSION_HAVE_DEFLATE
//   feature: 1 if a raw-DEFLATE provider is available (zlib, zlib-ng, miniz,
// or libdeflate). DEFLATE underpins gzip, zlib streams, and zip "deflate".
#define D_ENV_COMPRESSION_HAVE_DEFLATE                                         \
    ( D_ENV_COMPRESSION_HAVE_ZLIB       ||                                     \
      D_ENV_COMPRESSION_HAVE_ZLIBNG     ||                                     \
      D_ENV_COMPRESSION_HAVE_MINIZ      ||                                     \
      D_ENV_COMPRESSION_HAVE_LIBDEFLATE )

// 5.1.2
// D_ENV_COMPRESSION_HAVE_ZLIB_WRAP
//   feature: 1 if a zlib-wrapper (RFC 1950) provider is available.
#define D_ENV_COMPRESSION_HAVE_ZLIB_WRAP    D_ENV_COMPRESSION_HAVE_DEFLATE

// 5.1.3
// D_ENV_COMPRESSION_HAVE_GZIP
//   feature: 1 if a gzip-stream (RFC 1952) provider with the streaming gz*
// file API (gzopen, etc.) is available. zlib and zlib-ng provide it; miniz
// and libdeflate do not, so they are intentionally excluded here.
#define D_ENV_COMPRESSION_HAVE_GZIP                                            \
    ( D_ENV_COMPRESSION_HAVE_ZLIB ||                                           \
      D_ENV_COMPRESSION_HAVE_ZLIBNG )

// 5.1.4
// D_ENV_COMPRESSION_HAVE_GZIP_WRAP
//   feature: 1 if the gzip container (RFC 1952) can be produced or parsed by
// any means, including whole-buffer codecs. broader than HAVE_GZIP: it also
// accepts libdeflate, which writes valid .gz members without the gz* API.
#define D_ENV_COMPRESSION_HAVE_GZIP_WRAP                                       \
    ( D_ENV_COMPRESSION_HAVE_GZIP ||                                           \
      D_ENV_COMPRESSION_HAVE_LIBDEFLATE )

// 5.1.5
// D_ENV_COMPRESSION_HAVE_XZ
//   feature: 1 if an xz / lzma codec is available (alias for liblzma).
#define D_ENV_COMPRESSION_HAVE_XZ           D_ENV_COMPRESSION_HAVE_LZMA

// 5.1.6
// D_ENV_COMPRESSION_HAVE_ANY
//   feature: 1 if at least one compression codec library was detected.
#define D_ENV_COMPRESSION_HAVE_ANY                                             \
    ( D_ENV_COMPRESSION_HAVE_DEFLATE       ||                                  \
      D_ENV_COMPRESSION_HAVE_BZIP2         ||                                  \
      D_ENV_COMPRESSION_HAVE_LZMA          ||                                  \
      D_ENV_COMPRESSION_HAVE_ZSTD          ||                                  \
      D_ENV_COMPRESSION_HAVE_LZ4           ||                                  \
      D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE ||                                  \
      D_ENV_COMPRESSION_HAVE_BROTLI_DECODE )


//==============================================================================
// 6.  RECOMMENDED-CODEC SELECTION
//==============================================================================


// 6.1    Codec identifiers
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_COMPRESSION_CODEC_*
//   constant: stable small integers naming a codec, for runtime dispatch and
// for the 6.2 preferences.

// 6.1.1.1
// D_ENV_COMPRESSION_CODEC_NONE
//   constant: identifies no codec.
#define D_ENV_COMPRESSION_CODEC_NONE        0

// 6.1.1.2
// D_ENV_COMPRESSION_CODEC_ZLIB
//   constant: identifies zlib.
#define D_ENV_COMPRESSION_CODEC_ZLIB        1

// 6.1.1.3
// D_ENV_COMPRESSION_CODEC_ZLIBNG
//   constant: identifies zlib-ng.
#define D_ENV_COMPRESSION_CODEC_ZLIBNG      2

// 6.1.1.4
// D_ENV_COMPRESSION_CODEC_MINIZ
//   constant: identifies miniz.
#define D_ENV_COMPRESSION_CODEC_MINIZ       3

// 6.1.1.5
// D_ENV_COMPRESSION_CODEC_BZIP2
//   constant: identifies bzip2.
#define D_ENV_COMPRESSION_CODEC_BZIP2       4

// 6.1.1.6
// D_ENV_COMPRESSION_CODEC_LZMA
//   constant: identifies liblzma.
#define D_ENV_COMPRESSION_CODEC_LZMA        5

// 6.1.1.7
// D_ENV_COMPRESSION_CODEC_ZSTD
//   constant: identifies zstd.
#define D_ENV_COMPRESSION_CODEC_ZSTD        6

// 6.1.1.8
// D_ENV_COMPRESSION_CODEC_LZ4
//   constant: identifies lz4.
#define D_ENV_COMPRESSION_CODEC_LZ4         7

// 6.1.1.9
// D_ENV_COMPRESSION_CODEC_BROTLI
//   constant: identifies brotli.
#define D_ENV_COMPRESSION_CODEC_BROTLI      8

// 6.1.1.10
// D_ENV_COMPRESSION_CODEC_LIBDEFLATE
//   constant: identifies libdeflate.
#define D_ENV_COMPRESSION_CODEC_LIBDEFLATE  9

// 6.2    Preferred codecs
//------------------------------------------------------------------------------
// 6.2.1
// D_ENV_COMPRESSION_PREFERRED_DEFLATE
//   constant: preferred *streaming* DEFLATE backend, favouring zlib-ng for
// throughput, then stock zlib, then miniz, else none. libdeflate is faster
// still but whole-buffer only, so it is omitted here; consult
// D_ENV_COMPRESSION_HAVE_LIBDEFLATE directly when the whole input is in hand.
#if D_ENV_COMPRESSION_HAVE_ZLIBNG
    #define D_ENV_COMPRESSION_PREFERRED_DEFLATE D_ENV_COMPRESSION_CODEC_ZLIBNG
#elif D_ENV_COMPRESSION_HAVE_ZLIB
    #define D_ENV_COMPRESSION_PREFERRED_DEFLATE D_ENV_COMPRESSION_CODEC_ZLIB
#elif D_ENV_COMPRESSION_HAVE_MINIZ
    #define D_ENV_COMPRESSION_PREFERRED_DEFLATE D_ENV_COMPRESSION_CODEC_MINIZ
#else
    #define D_ENV_COMPRESSION_PREFERRED_DEFLATE D_ENV_COMPRESSION_CODEC_NONE
#endif

// 6.2.2
// D_ENV_COMPRESSION_PREFERRED_GZIP
//   constant: preferred gzip-stream backend (zlib-ng, then zlib).
#if D_ENV_COMPRESSION_HAVE_ZLIBNG
    #define D_ENV_COMPRESSION_PREFERRED_GZIP    D_ENV_COMPRESSION_CODEC_ZLIBNG
#elif D_ENV_COMPRESSION_HAVE_ZLIB
    #define D_ENV_COMPRESSION_PREFERRED_GZIP    D_ENV_COMPRESSION_CODEC_ZLIB
#else
    #define D_ENV_COMPRESSION_PREFERRED_GZIP    D_ENV_COMPRESSION_CODEC_NONE
#endif


//==============================================================================
// 7.  RUNTIME QUERIES
//==============================================================================
// Declared here and defined in the compression implementation. Unlike the
// macros above, they can report on what the build actually linked.


// 7.1    Runtime queries
//------------------------------------------------------------------------------
D_EXTERN_C_BEGIN

/**
 * @brief Returns a human-readable name for a codec identifier.
 *
 * @param[in] _codec  one of the D_ENV_COMPRESSION_CODEC_* constants.
 * @return a static, null-terminated codec name, or "none".
 */
const char* d_env_compression_codec_name(int _codec);
/**
 * @brief Returns the version of the zlib linked at runtime.
 *
 * @return zlibVersion()'s static string, or "unavailable" when zlib was not
 *         linked.
 */
const char* d_env_compression_zlib_runtime_version(void);
/**
 * @brief Prints the detected codec libraries and their versions to stdout.
 *
 * @note Intended for diagnostics and build verification.
 */
void        d_env_compression_print_info(void);

D_EXTERN_C_END


#endif  // DJINTERP_ENV_ENV_COMPRESS_H
