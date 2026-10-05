/*******************************************************************************
* djinterp [config]                                               cfg_compress.h
*
*   Build-time configuration for the compression facade: which codecs are
* compiled in, how much determinism is bought at what cost, and the sizes the
* kernel would otherwise hard-code.
*
*   targets:  core/util/compress_common.h -> D_INTERNAL_COMPRESS_*
*             core/util/compress.h        -> D_INTERNAL_COMPRESS_*
*             core/util/compress/compress.hpp      -> D_INTERNAL_COMPRESS_EXCEPTIONS
*   requires: cfg_common.h (helpers, D_CFG_TESTING); env/compress/env_compress.h
*             for the detection this cascade's environment layer reads
*
*   TWO LAYERS, AND THE DIFFERENCE MATTERS.  env_compress.h answers "is zlib
* installed?"; this file answers "do we want it?".  Detection is a fact and
* configuration is a choice, and conflating them is how a build ends up unable
* to exclude a library that happens to be on the include path.  Every codec
* knob here therefore defaults FROM the detection and can be turned off
* independently of it -- never the reverse, because configuration cannot
* conjure a library that is not there.
*
*
* path:      /inc/djinterp/config/c/util/compress/cfg_compress.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_CONFIG_C_UTIL_COMPRESS_CFG_COMPRESS_H
#define DJINTERP_CONFIG_C_UTIL_COMPRESS_CFG_COMPRESS_H 1

// djinterp
// (0) root first: helpers, user overrides, testing flag and preset.
#include "../../../cfg_common.h"
// (0b) the detection this cascade defaults from.
#include "../../../../env/util/compress/env_compress.h"

/*
TABLE OF CONTENTS
=================
0.    COMPRESSION CONFIGURATION
      -------------------------
      1.    Aggregate                    (D_CFG_COMPRESS_ALL)
      2.    Per-codec enables            (D_CFG_COMPRESS_CODEC_*)
      3.    Determinism                  (D_CFG_COMPRESS_PIN_*)
      4.    Sizes and defaults           (D_CFG_COMPRESS_CHUNK, _DEFAULT_LEVEL)
      5.    Validation                   (D_CFG_COMPRESS_VALIDATE)
      6.    C++ surface                  (D_CFG_COMPRESS_EXCEPTIONS)
      7.    Validation of the knobs
      8.    Derived values               (D_INTERNAL_COMPRESS_*)
*/


// ===========================================================================
// 0.   COMPRESSION CONFIGURATION
// ===========================================================================

// --- 0.1  Aggregate ---

// D_CFG_COMPRESS_ALL
//   brief: aggregate fallback for every per-codec enable in 0.2. Set 0 to
// compile out every third-party codec at once, leaving `store` -- which is
// implemented in the kernel and is not gated, because a build with no codec
// must still be a WORKING build rather than a broken one. Individual knobs
// override this; the cascade is individual > aggregate > detection.
#ifndef D_CFG_COMPRESS_ALL
#   define D_CFG_COMPRESS_ALL 1
#endif


// --- 0.2  Per-codec enables ---
//   Each defaults to (aggregate AND detected). A codec that is not installed
// stays off however these are set: configuration selects among what exists, it
// does not conjure. Turning one OFF while it is installed is the supported
// direction, and is how a build drops a dependency it does not want linked.

// D_CFG_COMPRESS_CODEC_DEFLATE
//   brief: raw DEFLATE, and with it the zlib and gzip wrappers -- one provider
// serves all three framings, so they cannot be enabled separately.
#ifndef D_CFG_COMPRESS_CODEC_DEFLATE
#   define D_CFG_COMPRESS_CODEC_DEFLATE D_CFG_COMPRESS_ALL
#endif

// D_CFG_COMPRESS_CODEC_BZIP2
//   brief: bzip2 (.bz2).
#ifndef D_CFG_COMPRESS_CODEC_BZIP2
#   define D_CFG_COMPRESS_CODEC_BZIP2 D_CFG_COMPRESS_ALL
#endif

// D_CFG_COMPRESS_CODEC_LZMA
//   brief: xz / lzma (liblzma).
#ifndef D_CFG_COMPRESS_CODEC_LZMA
#   define D_CFG_COMPRESS_CODEC_LZMA D_CFG_COMPRESS_ALL
#endif

// D_CFG_COMPRESS_CODEC_ZSTD
//   brief: Zstandard.
#ifndef D_CFG_COMPRESS_CODEC_ZSTD
#   define D_CFG_COMPRESS_CODEC_ZSTD D_CFG_COMPRESS_ALL
#endif

// D_CFG_COMPRESS_CODEC_LZ4
//   brief: LZ4 frame.
#ifndef D_CFG_COMPRESS_CODEC_LZ4
#   define D_CFG_COMPRESS_CODEC_LZ4 D_CFG_COMPRESS_ALL
#endif

// D_CFG_COMPRESS_CODEC_BROTLI
//   brief: Brotli. Needs both the encoder and the decoder library; a build
// carrying only one reports the codec unavailable rather than half-present.
#ifndef D_CFG_COMPRESS_CODEC_BROTLI
#   define D_CFG_COMPRESS_CODEC_BROTLI D_CFG_COMPRESS_ALL
#endif


// --- 0.3  Determinism ---
//   These are the knobs that trade throughput for reproducibility. They are
// separated from everything else because they are the ones a user might
// legitimately want OFF, and because turning them off is a decision with a
// consequence rather than a preference.

// D_CFG_COMPRESS_PIN_THREADS
//   brief: pin every codec's worker count to a single thread (1, the default),
// or let the backend choose (0).
//   ON is required for goals §2. A multi-threaded encoder partitions its input
// by worker count, so the same bytes compressed on a 4-core and a 64-core
// machine produce DIFFERENT output -- a parity divergence with no bug behind
// it, and one that only appears when the two forks run on different hardware.
//   Turn it OFF only for a build where throughput matters and byte-identical
// output does not. A parity suite run against such a build is meaningless, so
// cfg_testing.h forces this back ON in a test build.
#ifndef D_CFG_COMPRESS_PIN_THREADS
#   define D_CFG_COMPRESS_PIN_THREADS 1
#endif

// D_CFG_COMPRESS_PIN_DEFAULTS
//   brief: resolve unspecified knobs from the core's pinned table (1, the
// default) rather than leaving them for the backend (0).
//   ON is what makes two builds linking different library VERSIONS produce the
// same bytes. OFF restores the historical behaviour documented as "-1 selects
// the backend default", which is faster to reason about and gives up parity
// across library upgrades. There is no third option: a default comes from
// somewhere, and the only question is whether that somewhere is versioned.
#ifndef D_CFG_COMPRESS_PIN_DEFAULTS
#   define D_CFG_COMPRESS_PIN_DEFAULTS 1
#endif


// --- 0.4  Sizes and defaults ---

// D_CFG_COMPRESS_CHUNK
//   brief: the staging-buffer size, in bytes, a streaming leaf reads and writes
// in. Default 65536.
//   It is a STACK buffer, so raising it raises every codec leaf's frame size;
// lower it on a constrained target rather than raising it for throughput, which
// it does not meaningfully affect. It must exceed a single codec block for the
// multi-chunk path to be exercised at all -- the standard test corpus is sized
// against the default, so a build that lowers this should re-check that its
// large-payload tests still cross the boundary.
#ifndef D_CFG_COMPRESS_CHUNK
#   define D_CFG_COMPRESS_CHUNK 65536
#endif

// D_CFG_COMPRESS_DEFAULT_LEVEL
//   brief: the generic effort a pristine option set resolves to, on the 0-9
// scale. Default 6.
//   This is the single number that moves when a project wants "smaller by
// default" or "faster by default" across every codec at once, and it is pinned
// here rather than per-codec because the per-codec scales are derived from it.
#ifndef D_CFG_COMPRESS_DEFAULT_LEVEL
#   define D_CFG_COMPRESS_DEFAULT_LEVEL 6
#endif


// --- 0.5  Validation ---

// D_CFG_COMPRESS_VALIDATE
//   brief: range-check option knobs before dispatching (1, the default), or
// trust the caller (0).
//   The check is a walk over a fifteen-entry table, so it costs nothing
// measurable next to an encode. OFF exists for a build that has already
// validated upstream and wants the leaf to be a pure dispatch -- but note that
// an out-of-range knob then reaches the backend, where the failure is a
// backend error rather than a named offending knob.
#ifndef D_CFG_COMPRESS_VALIDATE
#   define D_CFG_COMPRESS_VALIDATE 1
#endif


// --- 0.6  C++ surface ---

// D_CFG_COMPRESS_EXCEPTIONS
//   brief: compile the throwing convenience API (`compress<gzip>(data)`).
// Defaults from the compiler's exception support; set 0 to omit it even where
// exceptions are enabled. The non-throwing try_* API is always present and is
// unaffected, so turning this off narrows the surface rather than degrading it.
#ifndef D_CFG_COMPRESS_EXCEPTIONS
#   if ( defined(__cpp_exceptions) ||                                          \
         defined(__EXCEPTIONS)     ||                                          \
         defined(_CPPUNWIND) )
#       define D_CFG_COMPRESS_EXCEPTIONS 1
#   else
#       define D_CFG_COMPRESS_EXCEPTIONS 0
#   endif
#endif


// --- 0.7  Validation of the knobs ---
//   Boolean knobs are checked as LITERALS, not arithmetically: `-DKNOB=yes`
// normalizes to 0 and would silently disable something the user asked to
// enable. The two numeric knobs are range-checked instead.

#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_ALL)
#   error "D_CFG_COMPRESS_ALL must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_CODEC_DEFLATE)
#   error "D_CFG_COMPRESS_CODEC_DEFLATE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_CODEC_BZIP2)
#   error "D_CFG_COMPRESS_CODEC_BZIP2 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_CODEC_LZMA)
#   error "D_CFG_COMPRESS_CODEC_LZMA must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_CODEC_ZSTD)
#   error "D_CFG_COMPRESS_CODEC_ZSTD must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_CODEC_LZ4)
#   error "D_CFG_COMPRESS_CODEC_LZ4 must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_CODEC_BROTLI)
#   error "D_CFG_COMPRESS_CODEC_BROTLI must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_PIN_THREADS)
#   error "D_CFG_COMPRESS_PIN_THREADS must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_PIN_DEFAULTS)
#   error "D_CFG_COMPRESS_PIN_DEFAULTS must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_VALIDATE)
#   error "D_CFG_COMPRESS_VALIDATE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_COMPRESS_EXCEPTIONS)
#   error "D_CFG_COMPRESS_EXCEPTIONS must be 0 or 1"
#endif

#if ( D_CFG_NORM(D_CFG_COMPRESS_CHUNK) < 1024 )
#   error "D_CFG_COMPRESS_CHUNK must be at least 1024 bytes"
#endif
#if ( (D_CFG_NORM(D_CFG_COMPRESS_DEFAULT_LEVEL) < 0) ||                        \
      (D_CFG_NORM(D_CFG_COMPRESS_DEFAULT_LEVEL) > 9) )
#   error "D_CFG_COMPRESS_DEFAULT_LEVEL must be on the 0-9 generic scale"
#endif


// --- 0.8  Derived values (D_INTERNAL_*) ---
//   What compress_common.h and the two faces actually read. Each folds the
// user's choice together with the environment's answer, so the module branches
// on ONE symbol and contains no config logic of its own.

// D_INTERNAL_COMPRESS_DEFLATE / _BZIP2 / _LZMA / _ZSTD / _LZ4 / _BROTLI
//   brief: 1 when the codec is both detected AND wanted. The AND is the whole
// point: a module reading only the detection would compile in a library the
// user asked to exclude, and a module reading only the config would try to call
// one that is not installed.
#define D_INTERNAL_COMPRESS_DEFLATE                                            \
    ( D_ENV_COMPRESSION_HAVE_DEFLATE &&                                        \
      D_CFG_IS_ON(D_CFG_COMPRESS_CODEC_DEFLATE) )
#define D_INTERNAL_COMPRESS_BZIP2                                              \
    ( D_ENV_COMPRESSION_HAVE_BZIP2 &&                                          \
      D_CFG_IS_ON(D_CFG_COMPRESS_CODEC_BZIP2) )
#define D_INTERNAL_COMPRESS_LZMA                                               \
    ( D_ENV_COMPRESSION_HAVE_LZMA &&                                           \
      D_CFG_IS_ON(D_CFG_COMPRESS_CODEC_LZMA) )
#define D_INTERNAL_COMPRESS_ZSTD                                               \
    ( D_ENV_COMPRESSION_HAVE_ZSTD &&                                           \
      D_CFG_IS_ON(D_CFG_COMPRESS_CODEC_ZSTD) )
#define D_INTERNAL_COMPRESS_LZ4                                                \
    ( D_ENV_COMPRESSION_HAVE_LZ4 &&                                            \
      D_CFG_IS_ON(D_CFG_COMPRESS_CODEC_LZ4) )
#define D_INTERNAL_COMPRESS_BROTLI                                             \
    ( D_ENV_COMPRESSION_HAVE_BROTLI &&                                         \
      D_CFG_IS_ON(D_CFG_COMPRESS_CODEC_BROTLI) )

// D_INTERNAL_COMPRESS_ZLIB_WRAP / _GZIP_WRAP
//   brief: the two DEFLATE framings, which ride on the same provider and the
// same enable -- there is no configuration in which a build has raw DEFLATE and
// wants the zlib wrapper excluded.
#define D_INTERNAL_COMPRESS_ZLIB_WRAP                                          \
    ( D_ENV_COMPRESSION_HAVE_ZLIB_WRAP &&                                      \
      D_CFG_IS_ON(D_CFG_COMPRESS_CODEC_DEFLATE) )
#define D_INTERNAL_COMPRESS_GZIP_WRAP                                          \
    ( D_ENV_COMPRESSION_HAVE_GZIP_WRAP &&                                      \
      D_CFG_IS_ON(D_CFG_COMPRESS_CODEC_DEFLATE) )

// D_INTERNAL_COMPRESS_PIN_THREADS / _PIN_DEFAULTS / _VALIDATE / _EXCEPTIONS
//   brief: the determinism, validation and surface gates, normalised to a
// strict 0/1 so a module may use them in `#if` without D_CFG_IS_ON.
//   In a TEST build the two determinism gates are forced ON regardless of the
// user's setting: a parity suite run against a build that lets the backend pick
// its own thread count is not measuring parity, and silently producing a green
// diff from such a build would be worse than refusing to run.
#if ( D_CFG_IS_ON(D_CFG_TESTING) || D_CFG_IS_ON(D_CFG_COMPRESS_PIN_THREADS) )
#   define D_INTERNAL_COMPRESS_PIN_THREADS 1
#else
#   define D_INTERNAL_COMPRESS_PIN_THREADS 0
#endif

#if ( D_CFG_IS_ON(D_CFG_TESTING) || D_CFG_IS_ON(D_CFG_COMPRESS_PIN_DEFAULTS) )
#   define D_INTERNAL_COMPRESS_PIN_DEFAULTS 1
#else
#   define D_INTERNAL_COMPRESS_PIN_DEFAULTS 0
#endif

#if D_CFG_IS_ON(D_CFG_COMPRESS_VALIDATE)
#   define D_INTERNAL_COMPRESS_VALIDATE 1
#else
#   define D_INTERNAL_COMPRESS_VALIDATE 0
#endif

#if D_CFG_IS_ON(D_CFG_COMPRESS_EXCEPTIONS)
#   define D_INTERNAL_COMPRESS_EXCEPTIONS 1
#else
#   define D_INTERNAL_COMPRESS_EXCEPTIONS 0
#endif

// D_INTERNAL_COMPRESS_CHUNK / _DEFAULT_LEVEL
//   brief: the numeric knobs, passed through after range validation so the
// module reads one name rather than a knob plus a bounds check.
#define D_INTERNAL_COMPRESS_CHUNK           D_CFG_COMPRESS_CHUNK
#define D_INTERNAL_COMPRESS_DEFAULT_LEVEL   D_CFG_COMPRESS_DEFAULT_LEVEL


#endif  // DJINTERP_CONFIG_C_UTIL_COMPRESS_CFG_COMPRESS_H
