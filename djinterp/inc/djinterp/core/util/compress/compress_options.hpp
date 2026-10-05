/*******************************************************************************
* djinterp [core]                                           compress_options.hpp
*
*   Codec tuning knobs for the C++ facade -- RE-BASED onto the shared kernel.
*
*   WHAT CHANGED, AND WHY IT HAD TO:
*   This header used to define the fifty-knob option set itself, in parallel
* with the C fork's own copy.  Two definitions of one formal object make parity
* a coincidence: nothing stops one side gaining a knob, reordering a block, or
* choosing a different default.  The struct now DERIVES from
* d_compress_options, so there is exactly one definition and the C++ type is
* notation over it.
*
*   THE DERIVATION COSTS NOTHING, AND THAT IS ASSERTED BELOW.
*   compress_options adds no data member -- only a constructor and comparison
* operators, neither of which occupies storage.  sizeof(compress_options) ==
* sizeof(d_compress_options) is checked at compile time rather than assumed,
* because "the wrapper is free" is exactly the kind of claim that quietly stops
* being true.
*
*   This works only because every knob in the core is an int32_t.  Had the core
* declared its enum-valued members with enum types, or its flags with bool,
* their sizes would be implementation-defined and C and C++ would be free to
* disagree about the layout of one declaration -- so the derivation would have
* been a gamble rather than a guarantee.
*
*   EVERY EXISTING CALL SITE STILL COMPILES.  `opt.level = 9`,
* `opt.zstd.workers = 4`, `opt.deflate.strategy = deflate_strategy_rle` and
* `a.deflate.window_bits == b.deflate.window_bits` all work unchanged: the
* member names and nesting are identical, and assigning an enumerator to an
* int32_t is an ordinary integral conversion.
*
*   ONE BEHAVIOURAL CHANGE, DELIBERATE.  A default-constructed set is PRISTINE
* (every knob D_COMPRESS_KNOB_UNSET), not zero.  It has to be: 0 is a
* meaningful value for most of these knobs -- level 0 means "no compression" --
* so a zero-filled set is a fully-specified request masquerading as an untouched
* one.  The core resolves UNSET knobs to its own pinned defaults at dispatch,
* which is what keeps C and C++ sending a backend identical parameters.
*
*   PORTABILITY:
*   Target floor is C++98.  The EFFECTIVE floor today is C++11, and not because
* of anything in this header -- djinterp.hpp gates below it outright ("djinterp
* requires C++11 or later"), so no header that includes it can compile at C++98
* regardless of what it uses.  Nothing here needs C++11 in its own right, so
* this returns to the target floor the day the gate moves.  Verified building
* at C++11, C++17 and C++20.
*
*   The C++98-era style constraints are kept for now -- D_STATIC_ASSERT with its
* fallback, no constexpr, no nullptr -- so that returning to the target floor
* stays a gate change rather than a rewrite.
*
*
* path:      /inc/djinterp/core/util/compress/compress_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_UTIL_COMPRESS_COMPRESS_OPTIONS_HPP
#define DJINTERP_UTIL_COMPRESS_COMPRESS_OPTIONS_HPP 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../c/util/compress/compress_common.h"      // the one definition of the knob set


NS_DJINTERP

// =============================================================================
// I.   KNOB CONSTANT SETS
// =============================================================================
//   The enums keep their C++ spellings and are typedefs of the core's, not
// second declarations.  Named constants rather than enumerators of a fresh
// enum: one enum, two vocabularies.

typedef enum d_deflate_strategy deflate_strategy;
D_STATIC const deflate_strategy deflate_strategy_default      =
    D_DEFLATE_STRATEGY_DEFAULT;
D_STATIC const deflate_strategy deflate_strategy_filtered     =
    D_DEFLATE_STRATEGY_FILTERED;
D_STATIC const deflate_strategy deflate_strategy_huffman_only =
    D_DEFLATE_STRATEGY_HUFFMAN_ONLY;
D_STATIC const deflate_strategy deflate_strategy_rle          =
    D_DEFLATE_STRATEGY_RLE;
D_STATIC const deflate_strategy deflate_strategy_fixed        =
    D_DEFLATE_STRATEGY_FIXED;

typedef enum d_lzma_check lzma_check;
D_STATIC const lzma_check lzma_check_none   = D_LZMA_CHECK_NONE;
D_STATIC const lzma_check lzma_check_crc32  = D_LZMA_CHECK_CRC32;
D_STATIC const lzma_check lzma_check_crc64  = D_LZMA_CHECK_CRC64;
D_STATIC const lzma_check lzma_check_sha256 = D_LZMA_CHECK_SHA256;

typedef enum d_lzma_mode lzma_mode;
D_STATIC const lzma_mode lzma_mode_fast   = D_LZMA_MODE_FAST;
D_STATIC const lzma_mode lzma_mode_normal = D_LZMA_MODE_NORMAL;

typedef enum d_lzma_mf lzma_mf;
D_STATIC const lzma_mf lzma_mf_hc3 = D_LZMA_MF_HC3;
D_STATIC const lzma_mf lzma_mf_hc4 = D_LZMA_MF_HC4;
D_STATIC const lzma_mf lzma_mf_bt2 = D_LZMA_MF_BT2;
D_STATIC const lzma_mf lzma_mf_bt3 = D_LZMA_MF_BT3;
D_STATIC const lzma_mf lzma_mf_bt4 = D_LZMA_MF_BT4;

typedef enum d_zstd_strategy zstd_strategy;
D_STATIC const zstd_strategy zstd_strategy_fast     = D_ZSTD_STRATEGY_FAST;
D_STATIC const zstd_strategy zstd_strategy_dfast    = D_ZSTD_STRATEGY_DFAST;
D_STATIC const zstd_strategy zstd_strategy_greedy   = D_ZSTD_STRATEGY_GREEDY;
D_STATIC const zstd_strategy zstd_strategy_lazy     = D_ZSTD_STRATEGY_LAZY;
D_STATIC const zstd_strategy zstd_strategy_lazy2    = D_ZSTD_STRATEGY_LAZY2;
D_STATIC const zstd_strategy zstd_strategy_btlazy2  = D_ZSTD_STRATEGY_BTLAZY2;
D_STATIC const zstd_strategy zstd_strategy_btopt    = D_ZSTD_STRATEGY_BTOPT;
D_STATIC const zstd_strategy zstd_strategy_btultra  = D_ZSTD_STRATEGY_BTULTRA;
D_STATIC const zstd_strategy zstd_strategy_btultra2 = D_ZSTD_STRATEGY_BTULTRA2;

typedef enum d_lz4_block_size lz4_block_size;
D_STATIC const lz4_block_size lz4_block_size_default = D_LZ4_BLOCK_SIZE_DEFAULT;
D_STATIC const lz4_block_size lz4_block_size_64kb    = D_LZ4_BLOCK_SIZE_64KB;
D_STATIC const lz4_block_size lz4_block_size_256kb   = D_LZ4_BLOCK_SIZE_256KB;
D_STATIC const lz4_block_size lz4_block_size_1mb     = D_LZ4_BLOCK_SIZE_1MB;
D_STATIC const lz4_block_size lz4_block_size_4mb     = D_LZ4_BLOCK_SIZE_4MB;

typedef enum d_lz4_block_mode lz4_block_mode;
D_STATIC const lz4_block_mode lz4_block_mode_linked      =
    D_LZ4_BLOCK_MODE_LINKED;
D_STATIC const lz4_block_mode lz4_block_mode_independent =
    D_LZ4_BLOCK_MODE_INDEPENDENT;

typedef enum d_brotli_mode brotli_mode;
D_STATIC const brotli_mode brotli_mode_generic = D_BROTLI_MODE_GENERIC;
D_STATIC const brotli_mode brotli_mode_text    = D_BROTLI_MODE_TEXT;
D_STATIC const brotli_mode brotli_mode_font    = D_BROTLI_MODE_FONT;


// =============================================================================
// II.  THE OPTION SET
// =============================================================================

// deflate_options / bzip2_options / ...
//   type: the per-codec knob blocks, aliased from the core.  Aliases and not
// re-declarations, so a knob cannot be added to one language and not the other.
typedef struct d_deflate_options deflate_options;
typedef struct d_bzip2_options   bzip2_options;
typedef struct d_lzma_options    lzma_options;
typedef struct d_zstd_options    zstd_options;
typedef struct d_lz4_options     lz4_options;
typedef struct d_brotli_options  brotli_options;

// compress_options
//   struct: the whole codec-tuning surface.  Derives from the core struct and
// adds no data member, so it IS the core struct with C++ notation attached.
// A default-constructed set is pristine -- see the header banner.
struct compress_options : d_compress_options
{
    compress_options()
    {
        d_compress_options_init(this);
    }

    // is_default -- whether nothing anywhere has been touched.
    bool is_default() const
    {
        return (d_compress_options_are_default(this) != 0);
    }
};

//   The Cost law, checked rather than trusted.  If a data member is ever added
// here this fires immediately, at the point of the mistake, rather than
// surfacing later as a layout mismatch across the language boundary.
D_STATIC_ASSERT(sizeof(compress_options) == sizeof(struct d_compress_options),
                "compress_options must cost nothing over d_compress_options");


// =============================================================================
// III. COMPARISON
// =============================================================================
//   Both operators forward to the core's knob-table walk, so "equal" means the
// same thing in both languages by construction rather than by review.

inline bool
operator==(const compress_options& _a, const compress_options& _b)
{
    return (d_compress_options_equal(&_a, &_b) != 0);
}

inline bool
operator!=(const compress_options& _a, const compress_options& _b)
{
    return (d_compress_options_equal(&_a, &_b) == 0);
}



// =============================================================================
// IV.  PER-CODEC BLOCK COMPARISON
// =============================================================================
//   One comparator per codec block, for a caller that wants to prove a single
// block moved (or did not) rather than asking about the whole set.
//
//   Each is defined by embedding the two blocks in otherwise-pristine option
// sets and asking the kernel.  That costs two stack structs and buys the
// property that matters: there is still exactly ONE knob walk in the codebase,
// so a knob added to d_compress_options is covered here the day it lands,
// without an edit.  A hand-written field chain would silently stop covering it.

// deflate_options_equal / bzip2_options_equal / ...
//   function: whether two per-codec blocks carry the same knob values.
inline bool
deflate_options_equal(
    const deflate_options& _a,
    const deflate_options& _b
)
{
    compress_options ca;
    compress_options cb;

    ca.deflate = _a;
    cb.deflate = _b;

    return (d_compress_options_equal(&ca, &cb) != 0);
}

inline bool
bzip2_options_equal(
    const bzip2_options& _a,
    const bzip2_options& _b
)
{
    compress_options ca;
    compress_options cb;

    ca.bzip2 = _a;
    cb.bzip2 = _b;

    return (d_compress_options_equal(&ca, &cb) != 0);
}

inline bool
lzma_options_equal(
    const lzma_options& _a,
    const lzma_options& _b
)
{
    compress_options ca;
    compress_options cb;

    ca.lzma = _a;
    cb.lzma = _b;

    return (d_compress_options_equal(&ca, &cb) != 0);
}

inline bool
zstd_options_equal(
    const zstd_options& _a,
    const zstd_options& _b
)
{
    compress_options ca;
    compress_options cb;

    ca.zstd = _a;
    cb.zstd = _b;

    return (d_compress_options_equal(&ca, &cb) != 0);
}

inline bool
lz4_options_equal(
    const lz4_options& _a,
    const lz4_options& _b
)
{
    compress_options ca;
    compress_options cb;

    ca.lz4 = _a;
    cb.lz4 = _b;

    return (d_compress_options_equal(&ca, &cb) != 0);
}

inline bool
brotli_options_equal(
    const brotli_options& _a,
    const brotli_options& _b
)
{
    compress_options ca;
    compress_options cb;

    ca.brotli = _a;
    cb.brotli = _b;

    return (d_compress_options_equal(&ca, &cb) != 0);
}

// compress_options_equal
//   function: the named spelling of operator==, kept because a caller reading
// a chain of block comparisons wants the aggregate to read the same way.
inline bool
compress_options_equal(
    const compress_options& _a,
    const compress_options& _b
)
{
    return (d_compress_options_equal(&_a, &_b) != 0);
}

// default_compress_options
//   function: a pristine set.  Named rather than left to the default
// constructor so that "compare against pristine" reads as one expression.
inline compress_options
default_compress_options()
{
    return compress_options();
}

// compress_options_are_default
//   function: whether nothing anywhere has been touched.
inline bool
compress_options_are_default(
    const compress_options& _opt
)
{
    return (d_compress_options_are_default(&_opt) != 0);
}


// =============================================================================
// V.   DIFF DESCRIPTION
// =============================================================================
//   The kernel answers WHICH knobs moved, as a dense index list; naming them
// is d_compress_knob_name's job and formatting the list is the caller's.  This
// is that caller, for the one presentation everyone wanted: a comma-separated
// field list.  Naming is FIELD-level here ("zstd.window_log"), which is the
// granularity a caller tuning a codec is asking about.

// describe_compress_option_diff
//   function: a comma-separated list of every knob that differs between two
// option sets, qualified by its block, or an empty string when the two are
// identical.
inline std::string
describe_compress_option_diff(
    const compress_options& _a,
    const compress_options& _b
)
{
    enum d_compress_knob moved[D_COMPRESS_KNOB_COUNT];
    std::string          out;
    size_t               n;
    size_t               i;

    n = d_compress_options_diff(&_a, &_b, moved, (size_t)D_COMPRESS_KNOB_COUNT);

    for (i = 0; i < n; ++i)
    {
        if (!out.empty())
        {
            out += ", ";
        }
        out += d_compress_knob_name(moved[i]);
    }

    return out;
}

// describe_compress_diff_from_default
//   function: the same field list taken against a freshly constructed set --
// "what has this option set changed from pristine?".
inline std::string
describe_compress_diff_from_default(
    const compress_options& _opt
)
{
    const compress_options fresh;

    return describe_compress_option_diff(_opt, fresh);
}


// =============================================================================
// VI.  OPTION BUILDERS
// =============================================================================
//   Small presets.  Each starts from a pristine set and moves exactly one
// thing, so a caller can assert both the intended change and that nothing else
// drifted.

// compress_level_options
//   function: a pristine set with the generic effort set to _level.
inline compress_options
compress_level_options(
    int _level
)
{
    compress_options opt;

    opt.level = _level;

    return opt;
}

// deflate_tuned_options
//   function: a pristine set with the DEFLATE block tuned.  Named parameters
// rather than a strategy-only shortcut, since window_bits and mem_level are the
// two knobs a DEFLATE-consuming caller most often needs to vary together.
inline compress_options
deflate_tuned_options(
    int              _window_bits,
    int              _mem_level,
    deflate_strategy _strategy
)
{
    compress_options opt;

    opt.deflate.window_bits = _window_bits;
    opt.deflate.mem_level   = _mem_level;
    opt.deflate.strategy    = _strategy;

    return opt;
}

// zstd_level_options
//   function: a pristine set with the zstd block's OWN level set, leaving the
// generic level untouched.  Pairs with compress_level_options for testing which
// of the two a call site actually consults.
inline compress_options
zstd_level_options(
    int _level
)
{
    compress_options opt;

    opt.zstd.level = _level;

    return opt;
}



NS_END  // djinterp


#endif  // DJINTERP_UTIL_COMPRESS_COMPRESS_OPTIONS_HPP
