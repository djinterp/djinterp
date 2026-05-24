/*******************************************************************************
* djinterp [env]                                                 env_compress.h
*
*   djinterp compression-codec environment detection header:
* This header provides compile-time detection of third-party data-compression
* codec libraries that the portable archive layer (zip/tar/gz/7z) builds upon.
* It detects the presence of each library via __has_include, exposes any
* version information the library publishes (only when its own header has
* already been included by the translation unit), and rolls the results up
* into codec-capability flags (DEFLATE, gzip-stream, bzip2, xz/lzma, zstd,
* lz4, brotli).
*
* scope:
*   - zlib / zlib-ng / miniz (DEFLATE, gzip wrapper, zlib wrapper)
*   - bzip2 (bzlib)
*   - liblzma (xz / .lzma)
*   - zstd (Zstandard)
*   - lz4 (LZ4 / LZ4HC frame)
*   - brotli (encoder / decoder)
*   - aggregate codec-capability roll-ups
*   - recommended-codec selection helpers
*   - runtime version-query declarations
*
* usage:
*   Include env.h first (directly or transitively), then this header:
*     #include "./env.h"
*     #include "./env_compression.h"
*
*   Detection is presence-only and never #includes the third-party headers
*   themselves, so merely including this file introduces no hard dependency.
*   A build system may pre-define any D_ENV_COMPRESSION_HAVE_* macro to force
*   a result and bypass the __has_include probe.
*
* NAMING CONVENTION:
*   D_ENV_COMPRESSION_HAVE_[LIB]     - 1 if the library is available, 0 if not
*   D_ENV_COMPRESSION_[LIB]_[FIELD]  - version / metadata for a detected library
*   D_ENV_COMPRESSION_HAVE_[CODEC]   - 1 if some library provides the codec
*

* path:      /inc/djinterp/core/env/env_compress.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.05.23
*******************************************************************************/

#ifndef DJINTERP_ENV_COMPRESSION_
#define DJINTERP_ENV_COMPRESSION_ 1

#include "./env.h"


// =============================================================================
// I.   CONFIGURATION AND PORTABLE __has_include
// =============================================================================

// D_CFG_ENV_COMPRESSION_ENABLED
//   configuration: master toggle for codec-library detection. when 0, every
// D_ENV_COMPRESSION_HAVE_* flag resolves to 0 unless explicitly pre-defined.
#ifndef D_CFG_ENV_COMPRESSION_ENABLED
    #define D_CFG_ENV_COMPRESSION_ENABLED 1
#endif

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
#endif

// D_INTERNAL_COMPRESSION_PROBE
//   macro: internal helper. yields the __has_include result for `header`
// when detection is enabled, and 0 when the master toggle is off.
#if D_CFG_ENV_COMPRESSION_ENABLED
    #define D_INTERNAL_COMPRESSION_PROBE(header) D_ENV_HAS_INCLUDE(header)
#else
    #define D_INTERNAL_COMPRESSION_PROBE(header) 0
#endif


// =============================================================================
// II.  ZLIB FAMILY (DEFLATE / gzip / zlib wrapper)
// =============================================================================

// -----------------------------------------------------------------------------
// A.  zlib
// -----------------------------------------------------------------------------

// D_ENV_COMPRESSION_HAVE_ZLIB
//   feature: detect if zlib (<zlib.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_ZLIB
    #if D_INTERNAL_COMPRESSION_PROBE(<zlib.h>)
        #define D_ENV_COMPRESSION_HAVE_ZLIB     1
    #else
        #define D_ENV_COMPRESSION_HAVE_ZLIB     0
    #endif
#endif

// zlib version metadata (only populated when <zlib.h> has been included)
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

// D_ENV_COMPRESSION_ZLIB_AT_LEAST
//   macro: evaluates to 1 if the included zlib reports at least the given
// version. encoding mirrors ZLIB_VERNUM: 0xMNR0 (major, minor, revision).
#define D_ENV_COMPRESSION_ZLIB_AT_LEAST(major, minor, rev)                     \
    ( D_ENV_COMPRESSION_HAVE_ZLIB &&                                           \
      (D_ENV_COMPRESSION_ZLIB_VERNUM >=                                        \
       (((major) << 12) | ((minor) << 8) | ((rev) << 4))) )


// -----------------------------------------------------------------------------
// B.  zlib-ng (drop-in zlib replacement; compat or native API)
// -----------------------------------------------------------------------------

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
#endif

#if defined(ZLIBNG_VER_STRING)
    #define D_ENV_COMPRESSION_ZLIBNG_VERSION_STR    ZLIBNG_VER_STRING
#elif defined(ZLIBNG_VERSION)
    #define D_ENV_COMPRESSION_ZLIBNG_VERSION_STR    ZLIBNG_VERSION
#else
    #define D_ENV_COMPRESSION_ZLIBNG_VERSION_STR    "unknown"
#endif


// -----------------------------------------------------------------------------
// C.  miniz (single-file zlib / DEFLATE / zip implementation)
// -----------------------------------------------------------------------------

// D_ENV_COMPRESSION_HAVE_MINIZ
//   feature: detect if miniz (<miniz.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_MINIZ
    #if D_INTERNAL_COMPRESSION_PROBE(<miniz.h>)
        #define D_ENV_COMPRESSION_HAVE_MINIZ        1
    #else
        #define D_ENV_COMPRESSION_HAVE_MINIZ        0
    #endif
#endif

#if defined(MZ_VERSION)
    #define D_ENV_COMPRESSION_MINIZ_VERSION_STR     MZ_VERSION
#else
    #define D_ENV_COMPRESSION_MINIZ_VERSION_STR     "unknown"
#endif


// -----------------------------------------------------------------------------
// D.  libdeflate (high-performance whole-buffer DEFLATE / zlib / gzip)
// -----------------------------------------------------------------------------

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
#endif

#if defined(LIBDEFLATE_VERSION_STRING)
    #define D_ENV_COMPRESSION_LIBDEFLATE_VERSION_STR LIBDEFLATE_VERSION_STRING
#else
    #define D_ENV_COMPRESSION_LIBDEFLATE_VERSION_STR "unknown"
#endif


// =============================================================================
// III. BZIP2
// =============================================================================

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
#endif


// =============================================================================
// IV.  LIBLZMA (xz / .lzma)
// =============================================================================

// D_ENV_COMPRESSION_HAVE_LZMA
//   feature: detect if liblzma (<lzma.h>) is available. liblzma provides the
// xz container, the legacy .lzma format, and raw LZMA1/LZMA2 streams.
#ifndef D_ENV_COMPRESSION_HAVE_LZMA
    #if D_INTERNAL_COMPRESSION_PROBE(<lzma.h>)
        #define D_ENV_COMPRESSION_HAVE_LZMA         1
    #else
        #define D_ENV_COMPRESSION_HAVE_LZMA         0
    #endif
#endif

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


// =============================================================================
// V.   ZSTD (Zstandard)
// =============================================================================

// D_ENV_COMPRESSION_HAVE_ZSTD
//   feature: detect if zstd (<zstd.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_ZSTD
    #if D_INTERNAL_COMPRESSION_PROBE(<zstd.h>)
        #define D_ENV_COMPRESSION_HAVE_ZSTD         1
    #else
        #define D_ENV_COMPRESSION_HAVE_ZSTD         0
    #endif
#endif

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

// D_ENV_COMPRESSION_ZSTD_AT_LEAST
//   macro: evaluates to 1 if the included zstd reports at least the given
// version. ZSTD_VERSION_NUMBER is encoded as MAJOR*10000 + MINOR*100 + RELEASE.
#define D_ENV_COMPRESSION_ZSTD_AT_LEAST(major, minor, rel)                     \
    ( D_ENV_COMPRESSION_HAVE_ZSTD &&                                           \
      (D_ENV_COMPRESSION_ZSTD_VERNUM >=                                        \
       (((major) * 10000) + ((minor) * 100) + (rel))) )


// =============================================================================
// VI.  LZ4
// =============================================================================

// D_ENV_COMPRESSION_HAVE_LZ4
//   feature: detect if lz4 (<lz4.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_LZ4
    #if D_INTERNAL_COMPRESSION_PROBE(<lz4.h>)
        #define D_ENV_COMPRESSION_HAVE_LZ4          1
    #else
        #define D_ENV_COMPRESSION_HAVE_LZ4          0
    #endif
#endif

// D_ENV_COMPRESSION_HAVE_LZ4_FRAME
//   feature: detect if the lz4 frame API (<lz4frame.h>) is available. the
// frame format is required for interoperable .lz4 files.
#ifndef D_ENV_COMPRESSION_HAVE_LZ4_FRAME
    #if D_INTERNAL_COMPRESSION_PROBE(<lz4frame.h>)
        #define D_ENV_COMPRESSION_HAVE_LZ4_FRAME    1
    #else
        #define D_ENV_COMPRESSION_HAVE_LZ4_FRAME    0
    #endif
#endif

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


// =============================================================================
// VII. BROTLI
// =============================================================================

// D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE
//   feature: detect if the brotli encoder (<brotli/encode.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE
    #if D_INTERNAL_COMPRESSION_PROBE(<brotli/encode.h>)
        #define D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE 1
    #else
        #define D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE 0
    #endif
#endif

// D_ENV_COMPRESSION_HAVE_BROTLI_DECODE
//   feature: detect if the brotli decoder (<brotli/decode.h>) is available.
#ifndef D_ENV_COMPRESSION_HAVE_BROTLI_DECODE
    #if D_INTERNAL_COMPRESSION_PROBE(<brotli/decode.h>)
        #define D_ENV_COMPRESSION_HAVE_BROTLI_DECODE 1
    #else
        #define D_ENV_COMPRESSION_HAVE_BROTLI_DECODE 0
    #endif
#endif

// D_ENV_COMPRESSION_HAVE_BROTLI
//   feature: 1 if both brotli encoder and decoder are available.
#define D_ENV_COMPRESSION_HAVE_BROTLI                                          \
    ( D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE &&                                  \
      D_ENV_COMPRESSION_HAVE_BROTLI_DECODE )


// =============================================================================
// VIII. PLATFORM-NATIVE CODECS
// =============================================================================
// These detect codec facilities shipped with the operating system itself,
// using the OS classification from env.h. They are kept distinct from the
// portable libraries above because their APIs differ; the archive layer may
// consult them explicitly as a no-dependency fallback. All probes degrade to
// 0 when OS detection is unavailable (D_ENV_OS_ID undefined).

// D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION
//   feature: detect Apple's libcompression (<compression.h>), which provides
// LZFSE, LZ4, raw DEFLATE (ZLIB), and LZMA. ships on macOS 10.11+ and iOS 9+.
#ifndef D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION
    #if ( defined(D_ENV_OS_ID)                          &&                    \
          ( (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) ||                           \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_IOS)   ||                           \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_APPLE) )      &&                    \
          D_INTERNAL_COMPRESSION_PROBE(<compression.h>) )
        #define D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION 1
    #else
        #define D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION 0
    #endif
#endif

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
#endif

// D_ENV_COMPRESSION_HAVE_PLATFORM_NATIVE
//   feature: 1 if any OS-native codec facility was detected.
#define D_ENV_COMPRESSION_HAVE_PLATFORM_NATIVE                                \
    ( D_ENV_COMPRESSION_HAVE_APPLE_LIBCOMPRESSION ||                          \
      D_ENV_COMPRESSION_HAVE_WIN_COMPRESSION_API )


// =============================================================================
// IX.  CODEC-CAPABILITY ROLL-UPS
// =============================================================================

// D_ENV_COMPRESSION_HAVE_DEFLATE
//   feature: 1 if a raw-DEFLATE provider is available (zlib, zlib-ng, miniz,
// or libdeflate). DEFLATE underpins gzip, zlib streams, and zip "deflate".
#define D_ENV_COMPRESSION_HAVE_DEFLATE                                         \
    ( D_ENV_COMPRESSION_HAVE_ZLIB       ||                                     \
      D_ENV_COMPRESSION_HAVE_ZLIBNG     ||                                     \
      D_ENV_COMPRESSION_HAVE_MINIZ      ||                                     \
      D_ENV_COMPRESSION_HAVE_LIBDEFLATE )

// D_ENV_COMPRESSION_HAVE_ZLIB_WRAP
//   feature: 1 if a zlib-wrapper (RFC 1950) provider is available.
#define D_ENV_COMPRESSION_HAVE_ZLIB_WRAP    D_ENV_COMPRESSION_HAVE_DEFLATE

// D_ENV_COMPRESSION_HAVE_GZIP
//   feature: 1 if a gzip-stream (RFC 1952) provider with the streaming gz*
// file API (gzopen, etc.) is available. zlib and zlib-ng provide it; miniz
// and libdeflate do not, so they are intentionally excluded here.
#define D_ENV_COMPRESSION_HAVE_GZIP                                            \
    ( D_ENV_COMPRESSION_HAVE_ZLIB ||                                           \
      D_ENV_COMPRESSION_HAVE_ZLIBNG )

// D_ENV_COMPRESSION_HAVE_GZIP_WRAP
//   feature: 1 if the gzip container (RFC 1952) can be produced or parsed by
// any means, including whole-buffer codecs. broader than HAVE_GZIP: it also
// accepts libdeflate, which writes valid .gz members without the gz* API.
#define D_ENV_COMPRESSION_HAVE_GZIP_WRAP                                       \
    ( D_ENV_COMPRESSION_HAVE_GZIP ||                                           \
      D_ENV_COMPRESSION_HAVE_LIBDEFLATE )

// D_ENV_COMPRESSION_HAVE_XZ
//   feature: 1 if an xz / lzma codec is available (alias for liblzma).
#define D_ENV_COMPRESSION_HAVE_XZ           D_ENV_COMPRESSION_HAVE_LZMA

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


// =============================================================================
// X.   RECOMMENDED-CODEC SELECTION
// =============================================================================

// codec identifier constants (stable small integers for runtime dispatch)
#define D_ENV_COMPRESSION_CODEC_NONE        0
#define D_ENV_COMPRESSION_CODEC_ZLIB        1
#define D_ENV_COMPRESSION_CODEC_ZLIBNG      2
#define D_ENV_COMPRESSION_CODEC_MINIZ       3
#define D_ENV_COMPRESSION_CODEC_BZIP2       4
#define D_ENV_COMPRESSION_CODEC_LZMA        5
#define D_ENV_COMPRESSION_CODEC_ZSTD        6
#define D_ENV_COMPRESSION_CODEC_LZ4         7
#define D_ENV_COMPRESSION_CODEC_BROTLI      8
#define D_ENV_COMPRESSION_CODEC_LIBDEFLATE  9

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

// D_ENV_COMPRESSION_PREFERRED_GZIP
//   constant: preferred gzip-stream backend (zlib-ng, then zlib).
#if D_ENV_COMPRESSION_HAVE_ZLIBNG
    #define D_ENV_COMPRESSION_PREFERRED_GZIP    D_ENV_COMPRESSION_CODEC_ZLIBNG
#elif D_ENV_COMPRESSION_HAVE_ZLIB
    #define D_ENV_COMPRESSION_PREFERRED_GZIP    D_ENV_COMPRESSION_CODEC_ZLIB
#else
    #define D_ENV_COMPRESSION_PREFERRED_GZIP    D_ENV_COMPRESSION_CODEC_NONE
#endif


// =============================================================================
// XI.  RUNTIME QUERY DECLARATIONS
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// d_env_compression_codec_name
//   function: returns a human-readable name for a codec identifier.
//   params:
//     codec - one of the D_ENV_COMPRESSION_CODEC_* constants.
//   returns: a static, NUL-terminated codec name, or "none".
const char* d_env_compression_codec_name(int codec);

// d_env_compression_zlib_runtime_version
//   function: returns the zlib runtime version string via zlibVersion(),
// or "unavailable" when zlib was not linked.
const char* d_env_compression_zlib_runtime_version(void);

// d_env_compression_print_info
//   function: prints the set of detected codec libraries and their versions
// to stdout. intended for diagnostics and build verification.
//   returns: none.
void d_env_compression_print_info(void);

#ifdef __cplusplus
}
#endif


#endif  // DJINTERP_ENV_COMPRESSION_
