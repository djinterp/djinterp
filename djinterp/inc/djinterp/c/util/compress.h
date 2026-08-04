/******************************************************************************
* djinterp [utility]                                                compress.h
*
*   The C face of the compression kernel: tier 1a.  Ergonomics ONLY.  Every
* line here expands to a call into compress_common.h; nothing in this file
* computes a result, decides a default, or knows what a codec is.  If a change
* to this header would change an output byte, the change belongs in the core.
*
*   THE THREE-FILE SPLIT:
*     compress_common.h / .c   the kernel, compiled by BOTH languages.  The only
*                              place a compression result is computed.
*     compress.h       (this)  the C face.
*     compress.hpp / .cpp      the C++ face.
*   A shared header that must compile in both languages includes
* compress_common.h; C code includes this file; C++ code includes compress.hpp.
*
*   C++ DOES NOT INCLUDE THIS FILE.  It is not merely unnecessary there, it is
* not valid there: the option builders below are C99 compound literals, which
* C++ has no equivalent for.  The C++ face wraps compress_common.h directly.
* That asymmetry is the point of having two faces over one core.
*
*   NO compress.c.  Everything here is a macro or a D_INLINE function, and
* D_INLINE is `static inline` in C, so this header exports no external symbol
* and needs no translation unit.  A compress.c appears the day a C-only
* function wants external linkage; there is none today, and an empty TU would
* be worse than an absent one.
*
*   WHAT A C FACE IS FOR:
*   The core's calling convention is honest and verbose -- a pristine option
* struct declared, mutated, and passed by address; a measure call; a produce
* call; a status compared against an enumerator.  Correct, and tedious at every
* call site.  This header supplies four things and stops:
*
*     1. SHORTHAND CALLS      the options argument omitted (NULL = pristine).
*     2. OPTION BUILDERS      a tuned option set as a single expression.
*     3. CODEC NAMING         _Generic, so a call site may name a codec with an
*                             enumerator OR a string, whichever reads better.
*     4. STATUS CONTROL FLOW  the early-return and retry shapes, written once.
*
*   NO MACRO TOWER.  Goals section 11 is explicit that C does not get
* templates, and a macro tower that simulates them would be exactly the
* "notation mistaken for semantics" error the framework warns about.  There is
* no dispatch engine here, no type-list, no recursion.  The _Generic in section
* III chooses between two spellings of one argument; it does not overload
* behaviour.
*
*   TIERING:
*   _Generic is C11.  Below it the codec-naming macro degrades to requiring an
* enumerator, which every call site can already supply -- so a C99 build loses
* a spelling, not a capability, and nothing fails to compile.  Every other
* macro here is C99 and needs no tier.
*
*   CONFIGURATION:
*   Module knobs belong in a cfg_pack.h alongside the other *_cfg.h files, not
* here; this header reads D_CFG_* and defines none.
*
*
* TABLE OF CONTENTS
* =================
* I.    STATUS CONTROL FLOW       (D_PACK_OK / D_PACK_TRY / retryability)
* II.   OPTION BUILDERS           (compound-literal option sets)
* III.  CODEC NAMING              (_Generic over enumerator or string)
* IV.   SHORTHAND CALLS
* V.    FIXED-BUFFER PATTERN
* VI.   AVAILABLE-CODEC ITERATION
*
*
* path:      /inc/djinterp/core/util/compress.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.29
******************************************************************************/

#ifndef DJINTERP_UTIL_COMPRESS_
#define DJINTERP_UTIL_COMPRESS_ 1

#ifdef __cplusplus
    #error "compress.h is the C face; C++ includes compress.hpp"
#endif

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "./compress_common.h"      // the kernel this is notation over


// =============================================================================
// I.   STATUS CONTROL FLOW
// =============================================================================
//   The three shapes a caller writes over and over.  D_PACK_TRY is the early
// return; D_PACK_RETRY is the grow-and-repeat that the two-call protocol was
// designed to make possible in one step.
//
//   Note what D_PACK_RETRY does NOT do: it does not allocate.  It re-invokes
// against a buffer the caller has meanwhile made larger, and the required size
// is already in hand because a BUFFER_TOO_SMALL failure carries it.  Putting an
// allocator in this macro would put one in tier 1a, which is exactly where it
// must not be.

// D_PACK_OK
//   macro: 1 when _s is success.  Reads better than the comparison at a branch
// and keeps the enumerator name out of call-site code.
#define D_PACK_OK(_s)               ((_s) == D_PACK_STATUS_OK)

// D_PACK_FAILED
//   macro: 1 when _s is any failure, formal or mechanical.
#define D_PACK_FAILED(_s)           ((_s) != D_PACK_STATUS_OK)

// D_PACK_TRY
//   macro: evaluate _expr and return its status from the enclosing function
// unless it succeeded.  The enclosing function must return enum d_pack_status.
#define D_PACK_TRY(_expr)                                                      \
    do                                                                         \
    {                                                                          \
        enum d_pack_status d_internal_s = (_expr);                             \
                                                                               \
        if (d_internal_s != D_PACK_STATUS_OK)                                  \
        {                                                                      \
            return d_internal_s;                                               \
        }                                                                      \
    }                                                                          \
    while (0)

// D_PACK_TRY_GOTO
//   macro: as D_PACK_TRY, but jumps to _label after assigning _status_var, for
// a function with cleanup to do.
#define D_PACK_TRY_GOTO(_expr, _status_var, _label)                            \
    do                                                                         \
    {                                                                          \
        (_status_var) = (_expr);                                               \
                                                                               \
        if ((_status_var) != D_PACK_STATUS_OK)                                 \
        {                                                                      \
            goto _label;                                                       \
        }                                                                      \
    }                                                                          \
    while (0)

// D_PACK_IS_RETRYABLE
//   macro: 1 when _s says the request was fine and only the buffer was short.
// A formal failure is never retryable, which is the whole reason the two kinds
// occupy disjoint ranges.
#define D_PACK_IS_RETRYABLE(_s)                                                \
    ((_s) == D_PACK_STATUS_BUFFER_TOO_SMALL)


// =============================================================================
// II.  OPTION BUILDERS
// =============================================================================
//   A tuned option set as a single expression, so a call site does not need a
// named local and three statements to raise one knob.  Each builder starts from
// D_COMPRESS_OPTIONS_INIT -- every knob UNSET -- and moves exactly what it
// names.  That is the same discipline test_compress_options.hpp's builders
// follow on the C++ side, and for the same reason: a test can then assert both
// the intended change and that nothing else drifted.  These are the C
// counterparts of compress_level_options / zstd_level_options and should stay
// in step with them.
//
//   WHY THESE ARE FUNCTIONS AND NOT MACROS.  The obvious spelling is a compound
// literal with designated initialisers -- roughly
// `#define D_COMPRESS_TUNED(...)
//      ((struct d_compress_options){ __VA_ARGS__ })`.
//   It is wrong, and silently.  C zero-initialises every member a designated
// initialiser does not name, so D_COMPRESS_TUNED(.zstd.workers = 4) would set
// the other forty-nine knobs to 0 rather than leaving them UNSET -- and 0 is a
// MEANINGFUL value for many of them (level 0 is "no compression", not "unset").
// The result would be a fully-specified option set masquerading as a pristine
// one, which no round-trip test catches because the wrong values survive the
// round trip faithfully.  C has no "start from this initialiser and override"
// form for a compound literal, so the builder has to be a function that
// declares a pristine set and then assigns.
//
//   For tuning a knob with no named builder, declare and assign -- two lines,
// and unambiguous:
//       struct d_compress_options opt = D_COMPRESS_OPTIONS_INIT;
//       opt.zstd.workers = 4;

// d_compress_options_of
//   function: a pristine option set as an expression.  The base every builder
// below starts from, and useful alone for a call site that wants to be explicit
// that it is requesting the core's defaults rather than passing NULL.
D_INLINE struct d_compress_options
d_compress_options_of(void)
{
    struct d_compress_options opt = D_COMPRESS_OPTIONS_INIT;

    return opt;
}

// d_compress_at_level
//   function: a pristine set with the generic effort at _level.
D_INLINE struct d_compress_options
d_compress_at_level(
    int32_t _level
)
{
    struct d_compress_options opt = D_COMPRESS_OPTIONS_INIT;

    opt.level = _level;

    return opt;
}

// d_compress_at_codec_level
//   function: a pristine set with a codec's OWN level set and the generic
// effort left UNSET.  Pairs with d_compress_at_level for testing which of the
// two a call site actually consults -- the same pairing zstd_level_options
// provides on the C++ side.  A codec with no private level leaves the set
// pristine.
D_INLINE struct d_compress_options
d_compress_at_codec_level(
    enum d_codec_id _codec,
    int32_t         _level
)
{
    struct d_compress_options opt = D_COMPRESS_OPTIONS_INIT;

    switch (_codec)
    {
        case D_CODEC_ID_ZSTD:
        {
            opt.zstd.level = _level;
            break;
        }
        case D_CODEC_ID_LZ4:
        {
            opt.lz4.level = _level;
            break;
        }
        case D_CODEC_ID_BROTLI:
        {
            opt.brotli.quality = _level;
            break;
        }
        case D_CODEC_ID_BZIP2:
        {
            opt.bzip2.block_size_100k = _level;
            break;
        }
        default:
        {
            break;
        }
    }

    return opt;
}

// d_compress_reproducible
//   function: a pristine set tuned so that two runs over identical input
// produce identical bytes -- no thread-count dependence, no timestamp.  Worth
// a named builder because getting it wrong is silent: the output is valid
// either way, and only a byte comparison across runs reveals the difference.
//   The gzip header's own timestamp is a CONTAINER knob and is cleared through
// archive.h's d_archive_reproducible, not here.
D_INLINE struct d_compress_options
d_compress_reproducible(void)
{
    struct d_compress_options opt = D_COMPRESS_OPTIONS_INIT;

    opt.zstd.workers = 0;
    opt.lzma.threads = 1;

    return opt;
}

// D_COMPRESS_FASTEST
//   macro: a pristine set asking for the least effort that still compresses.
#define D_COMPRESS_FASTEST                                                     \
    d_compress_at_level(D_COMPRESS_LEVEL_FASTEST)

// D_COMPRESS_BEST
//   macro: a pristine set asking for the most effort.
#define D_COMPRESS_BEST                                                        \
    d_compress_at_level(D_COMPRESS_LEVEL_BEST)


// =============================================================================
// III. CODEC NAMING
// =============================================================================
//   One argument, two spellings.  D_CODEC(gzip) is the enumerator;
// D_CODEC("gz") is the string.  Both resolve to an enum d_codec_id, so the
// call sites below take either.
//
//   The string form matters more than it looks: a codec choice that arrives
// from a config file, an environment variable, or `djinterp foo bar 42` is
// text, and the framework's sixth commitment is that everything is
// addressable from outside.  Without the string form every such call site
// writes its own name-to-enum switch, which is five copies of a table the
// core already has.
//
//   TIERING: _Generic is C11.  Below C11 the macro forwards its argument
// unchanged, so the enumerator spelling still works and the string spelling
// becomes a compile error at the call site rather than a silent wrong answer.
// A C99 caller that needs text calls d_codec_id_from_name directly.

#if D_ENV_LANG_IS_C11_OR_HIGHER

// d_internal_codec_from_any
//   function: the string arm of D_CODEC -- resolves a codec name, falling back
// to store when the name is unknown.  A fallback rather than a diagnostic
// because this is an expression: a caller needing to DETECT a bad name calls
// d_codec_id_from_name, which reports it.
D_INLINE enum d_codec_id
d_internal_codec_from_any(
    const char* _name
)
{
    enum d_codec_id codec = D_CODEC_ID_STORE;

    (void)d_codec_id_from_name(_name, &codec);

    return codec;
}

// d_internal_codec_identity
//   function: the enumerator arm of D_CODEC -- the identity, present so both
// arms of the _Generic are function calls and the macro has one shape.
D_INLINE enum d_codec_id
d_internal_codec_identity(
    enum d_codec_id _codec
)
{
    return _codec;
}

// D_CODEC
//   macro: an enum d_codec_id from either an enumerator or a codec name.
#define D_CODEC(_x)                                                            \
    _Generic((_x),                                                             \
             char*:       d_internal_codec_from_any,                           \
             const char*: d_internal_codec_from_any,                           \
             default:     d_internal_codec_identity)(_x)

#else   // C99: no _Generic

// D_CODEC
//   macro: degraded form -- forwards an enumerator unchanged.  The string
// spelling is unavailable below C11; call d_codec_id_from_name for it.
#define D_CODEC(_x)     (_x)

#endif  // D_ENV_LANG_IS_C11_OR_HIGHER


// =============================================================================
// IV.  SHORTHAND CALLS
// =============================================================================
//   The core's transforms with the options argument omitted.  A NULL options
// pointer means "the core's pinned defaults", which is the same set
// D_COMPRESS_OPTIONS produces -- so these are shorthands and not a second
// default policy.  There is exactly one place defaults are decided, and it is
// d_compress_options_resolve.

// D_COMPRESS_MEASURE
//   macro: the measure call spelled so the NULL / 0 pair cannot be mistyped.
// Sets *_out_size to the exact byte count _in would produce under _codec.
#define D_COMPRESS_MEASURE(_codec, _in, _in_size, _out_size)                   \
    d_compress(D_CODEC(_codec), (_in), (_in_size), NULL,                       \
               NULL, (size_t)0, (_out_size))

// D_COMPRESS_INTO
//   macro: compress into a caller-owned buffer with default tuning.
#define D_COMPRESS_INTO(_codec, _in, _in_size, _out, _cap, _out_size)          \
    d_compress(D_CODEC(_codec), (_in), (_in_size), NULL,                       \
               (_out), (_cap), (_out_size))

// D_DECOMPRESS_MEASURE
//   macro: the measure call for decompression.  Decompression has no bound, so
// this is the only way to size a buffer without reading a length the container
// recorded -- see the asymmetry note in compress_common.h section XI.
#define D_DECOMPRESS_MEASURE(_codec, _in, _in_size, _out_size)                 \
    d_decompress(D_CODEC(_codec), (_in), (_in_size),                           \
                 NULL, (size_t)0, (_out_size))

// D_DECOMPRESS_INTO
//   macro: decompress into a caller-owned buffer.
#define D_DECOMPRESS_INTO(_codec, _in, _in_size, _out, _cap, _out_size)        \
    d_decompress(D_CODEC(_codec), (_in), (_in_size),                           \
                 (_out), (_cap), (_out_size))


// =============================================================================
// V.   FIXED-BUFFER PATTERN
// =============================================================================
//   The common C shape: a buffer of known size on the stack or in static
// storage, and a transform that must not overrun it.  These macros take the
// ARRAY, not a pointer and a length, so the capacity comes from the array's own
// type and cannot drift out of step with the declaration.
//
//   D_ARRAY_STATIC_SIZE is djinterp.h's, and it is used rather than a local
// sizeof division so that passing a pointer where an array was meant is caught
// by the existing macro instead of silently computing a capacity of 1.

// D_COMPRESS_TO_ARRAY
//   macro: compress into the array _arr, whose capacity is taken from its own
// declaration.  Yields D_PACK_STATUS_BUFFER_TOO_SMALL with *_out_size set to
// the requirement when the array is too small.
#define D_COMPRESS_TO_ARRAY(_codec, _in, _in_size, _arr, _out_size)            \
    d_compress(D_CODEC(_codec), (_in), (_in_size), NULL,                       \
               (_arr), D_ARRAY_STATIC_SIZE(_arr), (_out_size))

// D_DECOMPRESS_TO_ARRAY
//   macro: decompress into the array _arr, capacity taken from its declaration.
#define D_DECOMPRESS_TO_ARRAY(_codec, _in, _in_size, _arr, _out_size)          \
    d_decompress(D_CODEC(_codec), (_in), (_in_size),                           \
                 (_arr), D_ARRAY_STATIC_SIZE(_arr), (_out_size))

// D_COMPRESS_TO_ARRAY_TUNED
//   macro: as D_COMPRESS_TO_ARRAY with an explicit option set, which may be a
// builder expression from section II.
#define D_COMPRESS_TO_ARRAY_TUNED(_codec, _in, _in_size, _opt, _arr, _out_size)\
    d_compress(D_CODEC(_codec), (_in), (_in_size), &(_opt),                    \
               (_arr), D_ARRAY_STATIC_SIZE(_arr), (_out_size))


// =============================================================================
// VI.  AVAILABLE-CODEC ITERATION
// =============================================================================
//   Walking the codecs a build actually has.  This is the loop a diagnostic
// banner, a capability report, and a differential test sweep all write, and it
// is easy to write subtly wrong -- by iterating the enumerators and forgetting
// that a named codec need not be present, or by hardcoding nine cases and
// missing one when a tenth codec is added.
//
//   D_FOR_EACH_CODEC visits every codec the framework NAMES;
// D_FOR_EACH_AVAILABLE_CODEC visits only those this build can perform.  The
// distinction is the same one d_codec_id_name and d_codec_is_available draw,
// and a test suite wants both: the first to assert that every name resolves,
// the second to gate an assertion that a codec really round-trips.

// D_FOR_EACH_CODEC
//   macro: iterate _var over every codec the framework names, present or not.
#define D_FOR_EACH_CODEC(_var)                                                 \
    for (enum d_codec_id _var = D_CODEC_ID_STORE;                              \
         (int)_var < D_CODEC_ID_COUNT;                                         \
         _var = (enum d_codec_id)((int)_var + 1))

// D_FOR_EACH_AVAILABLE_CODEC
//   macro: iterate _var over only the codecs this build can perform.  Skips
// the rest with `continue`, so the body runs exactly for the available ones.
#define D_FOR_EACH_AVAILABLE_CODEC(_var)                                       \
    D_FOR_EACH_CODEC(_var)                                                     \
        if (!d_codec_is_available(_var)) { continue; } else


#endif  // DJINTERP_UTIL_COMPRESS_
