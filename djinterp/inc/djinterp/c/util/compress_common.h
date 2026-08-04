/******************************************************************************
* djinterp [utility]                                        compress_common.h
*
*   The shared compression kernel: tier 0.  One definition of every codec
* identity, every option layout, and every transform, compiled by BOTH C and
* C++.  compress.h (the C face) and compress.hpp (the C++ face) are notation
* over this file and add no semantics of their own.
*
*   THE THREE-FILE SPLIT:
*     compress_common.h / .c   this kernel, compiled by BOTH languages.
*     compress.h               the C face      -- macros, _Generic, builders.
*     compress.hpp / .cpp      the C++ face    -- byte_blob, tags, try_*.
*   A header that must compile in both languages includes THIS file.  Only
* language-specific code reaches for a face, and a face never computes anything
* the other language cannot: if a change to a face would move an output byte,
* the change belongs here instead.
*
*   compress_common.c is compiled once and linked by both, which is what
* D_EXTERN_C_BEGIN below is for -- see cfg_qualifiers.h on
* D_CFG_EXTERN_C_LINKAGE for the one case where that is not what you want.
*
*   WHAT LIVES HERE:
*   The boundary criterion applied to compression puts the codec set, the knob
* set, the status set, and the four transforms (bound, compress, decompress,
* signature) in the core.  Tag types, fluent builders, `byte_blob`, and the
* try_* templates are C++ notation and live in the C++ face; _Generic dispatch
* and the option macros are C notation and live in the C face.
*
*   NO ALLOCATION, EVER:
*   Nothing here allocates.  The C fork has no growable byte buffer yet (no
* d_string, no d_vector), and inventing one inside a codec facade would put an
* allocator in tier 0 where every other module would then inherit it.  Instead
* every transform writes into caller-owned memory under the two-call protocol
* below, or streams through a sink.  When d_string arrives it becomes one more
* sink, and nothing in this header changes.
*
*   THE TWO-CALL BUFFER PROTOCOL:
*   Every buffer-form transform takes (_out, _out_capacity, _out_size).
*     - MEASURE: pass _out == NULL and _out_capacity == 0.  *_out_size receives
*       the exact byte count the transform would write; nothing is written; the
*       status is D_PACK_STATUS_OK.
*     - PRODUCE: pass a buffer of at least that many bytes.
*     - A buffer that is too small yields D_PACK_STATUS_BUFFER_TOO_SMALL and
*       STILL sets *_out_size to the required count, so a caller may grow and
*       retry without a separate measuring pass.
*   The failure carries its own remedy; that is deliberate, and it is the only
* status in this header that a caller is expected to recover from in place.
*
*   THE SINK FORM:
*   A measure pass costs a full encode for most codecs.  A consumer that can
* grow (the C++ face appending to a byte_blob, a FILE*, a socket) therefore
* drives the sink form instead and pays one pass.  The buffer form is defined
* AS the sink form against a bounded-array sink, so there is one algorithm per
* codec and not two.
*
*   WHY NOT fn_write:
*   djinterp.h's fn_write is `size_t (*)(char* const, size_t)` -- no context
* pointer, so it cannot drive a growable buffer or a file handle without
* global mutable state.  struct d_pack_sink carries a context and is otherwise
* the same idea.
*
*   EVERY KNOB IS AN int32_t:
*   A struct member whose type is an enum, or `bool`, has a size the standard
* leaves to the implementation, and C and C++ compilers are free to disagree
* about it for the SAME declaration.  That is a byte-level determinacy break at
* the root of the layout, so no option member is an enum or a bool: every knob
* is an int32_t, and the enums below are named constant sets that populate
* them.  Existing C++ call sites (`opt.zstd.strategy = zstd_strategy_btlazy2`,
* `opt.store_only = true`) continue to compile unchanged through the ordinary
* integral conversion.
*
*   Consequently struct d_compress_options is exactly int32_t[50] with no
* padding, which is asserted below and is what lets the knob table be a dense
* offset table rather than fifty hand-written cases.
*
*   PRISTINE MEANS UNSET, AND THE CORE RESOLVES IT:
*   Every knob defaults to D_COMPRESS_KNOB_UNSET rather than to a number.  A
* backend's own default differs between libraries and between versions of one
* library, so letting the backend choose would break parity for environmental
* reasons -- the same defect already recorded against the BUILTIN backend.
* d_compress_options_resolve substitutes the core's pinned default for every
* UNSET knob, and both faces call it before dispatching, so C and C++ hand the
* backend byte-identical parameters.
*
*   PORTABILITY:
*   C99 / C++11 and upward, per the framework floors.  Presence-only codec
* detection through env_compress.h; this header includes no third-party
* header and adds no link dependency.
*
*
* TABLE OF CONTENTS
* =================
* I.    STATUS                    (formal vs. mechanical failure)
* II.   CODEC IDENTITY            (enum d_codec_id, names, suffixes)
* III.  KNOB CONSTANT SETS        (strategies, checks, modes)
* IV.   OPTION LAYOUT             (the six blocks + d_compress_options)
* V.    LAYOUT ASSERTIONS
* VI.   THE KNOB TABLE            (dense index over the flat option set)
* VII.  OPTION OPERATIONS         (init, resolve, compare, diff)
* VIII. AVAILABILITY
* IX.   STREAM SIGNATURES
* X.    TRANSFORM LEAVES
*
*
* path:      /inc/djinterp/core/util/compress_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.29
******************************************************************************/

#ifndef DJINTERP_UTIL_COMPRESS_COMMON_
#define DJINTERP_UTIL_COMPRESS_COMMON_ 1

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../../c/djinterp.h"       // D_STATIC_ASSERT, D_EXTERN_C_*
#include "../env/env_compress.h"    // D_ENV_COMPRESSION_HAVE_*
#include "./sink_common.h"           // d_pack_sink, d_pack_text/bytes


//   cfg_qualifiers.h lets a project take ownership of the D_EXTERN_C family by
// setting D_CFG_DEFINE_EXTERN_C to 0.  This header needs it -- every
// declaration below must carry C linkage so one compiled object serves both
// languages -- so say so in one sentence rather than failing as a cascade of
// syntax errors fifty lines further down.
#if !defined(D_EXTERN_C_BEGIN)
    #error "compress_common.h needs D_EXTERN_C_BEGIN; \
define it or set D_CFG_DEFINE_EXTERN_C to 1"
#endif

D_EXTERN_C_BEGIN


// =============================================================================
// I.   STATUS
// =============================================================================
//   Two kinds of failure, kept apart on purpose.  A FORMAL failure says the
// operation is not defined -- this codec was not compiled in, this stream is
// not what it claims to be.  A MECHANICAL failure says the operation is
// defined and the machinery ran short -- the buffer was small, the backend
// could not allocate.  Reporting them identically reads as correct behaviour
// and is not, so they occupy disjoint numeric ranges: formal below 0x100,
// mechanical at or above it.  A caller that only wants to know "can I retry
// with more room?" tests the range, not the enumerator.

// d_pack_status
//   enum: the result of any compress or archive operation.  Shared with
// archive_common.h, which builds on this header.
enum d_pack_status
{
    // -- success ------------------------------------------------------------
    D_PACK_STATUS_OK                = 0,

    // -- formal: the operation is not defined -------------------------------
    D_PACK_STATUS_UNAVAILABLE       = 0x001,
    D_PACK_STATUS_UNSUPPORTED       = 0x002,
    D_PACK_STATUS_INVALID_ARGUMENT  = 0x003,
    D_PACK_STATUS_CORRUPT_INPUT     = 0x004,
    D_PACK_STATUS_TRUNCATED_INPUT   = 0x005,
    D_PACK_STATUS_WRONG_PASSWORD    = 0x006,

    // -- mechanical: the operation is defined, the machinery ran short ------
    D_PACK_STATUS_BUFFER_TOO_SMALL  = 0x100,
    D_PACK_STATUS_NO_MEMORY         = 0x101,
    D_PACK_STATUS_SINK_ERROR        = 0x102,
    D_PACK_STATUS_BACKEND_ERROR     = 0x103
};

// D_PACK_STATUS_MECHANICAL_FLOOR
//   constant: the first mechanical status.  A status at or above this value
// describes the machinery; below it, the request itself.
#define D_PACK_STATUS_MECHANICAL_FLOOR  0x100

// D_PACK_STATUS_IS_FORMAL
//   macro: 1 when _s reports that the operation is not defined.
#define D_PACK_STATUS_IS_FORMAL(_s)                                            \
    ( ((_s) != D_PACK_STATUS_OK) &&                                            \
      ((int)(_s) < D_PACK_STATUS_MECHANICAL_FLOOR) )

// D_PACK_STATUS_IS_MECHANICAL
//   macro: 1 when _s reports that the machinery ran short.
#define D_PACK_STATUS_IS_MECHANICAL(_s)                                        \
    ( (int)(_s) >= D_PACK_STATUS_MECHANICAL_FLOOR )


const char*        d_pack_status_name(enum d_pack_status _status);
const char*        d_pack_status_message(enum d_pack_status _status);


// =============================================================================
// II.  CODEC IDENTITY
// =============================================================================
//   Values are pinned explicitly and MUST NOT be reordered: document_bundle,
// output_packaging and the DTest bridge all pass a d_codec_id by value, and
// test_packaging.hpp records that the neighbouring format enum already differs
// in order from DTest's own.  An implicit ordering is one insertion away from
// silently selecting the wrong codec.

// d_codec_id
//   enum: the stream codecs this framework names.  `store` is the identity
// transform and is always available, which is what keeps a store-only path
// working on a build with no third-party codec at all.
enum d_codec_id
{
    D_CODEC_ID_STORE    = 0,
    D_CODEC_ID_DEFLATE  = 1,
    D_CODEC_ID_ZLIB     = 2,
    D_CODEC_ID_GZIP     = 3,
    D_CODEC_ID_BZIP2    = 4,
    D_CODEC_ID_XZ       = 5,
    D_CODEC_ID_ZSTD     = 6,
    D_CODEC_ID_LZ4      = 7,
    D_CODEC_ID_BROTLI   = 8
};

// D_CODEC_ID_COUNT
//   constant: the number of enumerators in d_codec_id.
#define D_CODEC_ID_COUNT    9

// D_CODEC_ID_IS_VALID
//   macro: 1 when _c names a codec this framework knows.  Says nothing about
// whether the build can perform it -- that is d_codec_is_available.
#define D_CODEC_ID_IS_VALID(_c)                                                \
    ( ((int)(_c) >= 0) && ((int)(_c) < D_CODEC_ID_COUNT) )


const char*        d_codec_id_name(enum d_codec_id _codec);
const char*        d_codec_suffix(enum d_codec_id _codec);
int                d_codec_id_from_name(const char*      _name,
                                        enum d_codec_id* _out_codec);


// =============================================================================
// III. KNOB CONSTANT SETS
// =============================================================================
//   Named values for the knobs whose domain is an enumeration.  These are
// constant sets, not member types: see EVERY KNOB IS AN int32_t in the header
// banner for why no option member is declared with one of these types.

// D_COMPRESS_KNOB_UNSET
//   constant: the pristine value of every knob.  Chosen as INT32_MIN because
// no codec parameter can legitimately take it, so it can never collide with a
// caller's intent -- unlike 0 or -1, both of which are meaningful to several
// codecs.  d_compress_options_resolve replaces it with the pinned default.
#define D_COMPRESS_KNOB_UNSET       INT32_MIN

// D_COMPRESS_LEVEL_*
//   constant: the generic effort scale, normalised across codecs.  A codec
// whose native scale differs (brotli 0-11, zstd -7-22) is mapped onto it by
// d_compress_resolve_level rather than by each call site.
#define D_COMPRESS_LEVEL_NONE       0
#define D_COMPRESS_LEVEL_FASTEST    1
#define D_COMPRESS_LEVEL_FAST       3
#define D_COMPRESS_LEVEL_DEFAULT    6
#define D_COMPRESS_LEVEL_BEST       9

// d_deflate_strategy
//   enum: DEFLATE strategy constants; values match zlib's Z_* so the mapping
// to a zlib-family backend is the identity.
enum d_deflate_strategy
{
    D_DEFLATE_STRATEGY_DEFAULT      = 0,
    D_DEFLATE_STRATEGY_FILTERED     = 1,
    D_DEFLATE_STRATEGY_HUFFMAN_ONLY = 2,
    D_DEFLATE_STRATEGY_RLE          = 3,
    D_DEFLATE_STRATEGY_FIXED        = 4
};

// d_lzma_check
//   enum: the integrity check an .xz container carries; values match liblzma's
// LZMA_CHECK_*.
enum d_lzma_check
{
    D_LZMA_CHECK_NONE   = 0,
    D_LZMA_CHECK_CRC32  = 1,
    D_LZMA_CHECK_CRC64  = 4,
    D_LZMA_CHECK_SHA256 = 10
};

// d_lzma_mode
//   enum: LZMA match-finder mode; values match liblzma's LZMA_MODE_*.
enum d_lzma_mode
{
    D_LZMA_MODE_FAST   = 1,
    D_LZMA_MODE_NORMAL = 2
};

// d_lzma_mf
//   enum: LZMA match finder; values match liblzma's LZMA_MF_*.
enum d_lzma_mf
{
    D_LZMA_MF_HC3 = 0x03,
    D_LZMA_MF_HC4 = 0x04,
    D_LZMA_MF_BT2 = 0x12,
    D_LZMA_MF_BT3 = 0x13,
    D_LZMA_MF_BT4 = 0x14
};

// d_zstd_strategy
//   enum: Zstandard strategy; values match ZSTD_strategy.
enum d_zstd_strategy
{
    D_ZSTD_STRATEGY_FAST     = 1,
    D_ZSTD_STRATEGY_DFAST    = 2,
    D_ZSTD_STRATEGY_GREEDY   = 3,
    D_ZSTD_STRATEGY_LAZY     = 4,
    D_ZSTD_STRATEGY_LAZY2    = 5,
    D_ZSTD_STRATEGY_BTLAZY2  = 6,
    D_ZSTD_STRATEGY_BTOPT    = 7,
    D_ZSTD_STRATEGY_BTULTRA  = 8,
    D_ZSTD_STRATEGY_BTULTRA2 = 9
};

// d_lz4_block_size
//   enum: LZ4 frame block-size selector; values match LZ4F_blockSizeID_t.
enum d_lz4_block_size
{
    D_LZ4_BLOCK_SIZE_DEFAULT = 0,
    D_LZ4_BLOCK_SIZE_64KB    = 4,
    D_LZ4_BLOCK_SIZE_256KB   = 5,
    D_LZ4_BLOCK_SIZE_1MB     = 6,
    D_LZ4_BLOCK_SIZE_4MB     = 7
};

// d_lz4_block_mode
//   enum: whether LZ4 frame blocks may reference their predecessors; values
// match LZ4F_blockMode_t.
enum d_lz4_block_mode
{
    D_LZ4_BLOCK_MODE_LINKED      = 0,
    D_LZ4_BLOCK_MODE_INDEPENDENT = 1
};

// d_brotli_mode
//   enum: Brotli's input hint; values match BROTLI_MODE_*.
enum d_brotli_mode
{
    D_BROTLI_MODE_GENERIC = 0,
    D_BROTLI_MODE_TEXT    = 1,
    D_BROTLI_MODE_FONT    = 2
};


// =============================================================================
// IV.  OPTION LAYOUT
// =============================================================================
//   Fifty knobs: one generic effort plus six per-codec blocks of 3, 4, 11, 19,
// 7 and 5.  Declaration order is normative -- it is the knob-table order, the
// diff-report order, and the serialisation order, and the C++ face's field
// list must match it member for member.

// d_deflate_options
//   struct: the DEFLATE tuning block (3 knobs).  `strategy` takes a
// d_deflate_strategy value.
struct d_deflate_options
{
    int32_t window_bits;
    int32_t mem_level;
    int32_t strategy;
};

// d_bzip2_options
//   struct: the bzip2 tuning block (4 knobs).  `small_decompress` is a flag.
struct d_bzip2_options
{
    int32_t block_size_100k;
    int32_t work_factor;
    int32_t verbosity;
    int32_t small_decompress;
};

// d_lzma_options
//   struct: the lzma / xz tuning block (11 knobs).  `check` takes a
// d_lzma_check value, `mode` a d_lzma_mode, `mf` a d_lzma_mf; `extreme` is a
// flag.
struct d_lzma_options
{
    int32_t extreme;
    int32_t check;
    int32_t dict_size;
    int32_t lc;
    int32_t lp;
    int32_t pb;
    int32_t mode;
    int32_t nice_len;
    int32_t mf;
    int32_t depth;
    int32_t threads;
};

// d_zstd_options
//   struct: the Zstandard tuning block (19 knobs).  `level` is zstd's OWN
// level and is distinct from the generic effort in d_compress_options;
// `strategy` takes a d_zstd_strategy value; the three *_flag members and
// `long_distance_matching` are flags.
struct d_zstd_options
{
    int32_t level;
    int32_t window_log;
    int32_t hash_log;
    int32_t chain_log;
    int32_t search_log;
    int32_t min_match;
    int32_t target_length;
    int32_t strategy;
    int32_t long_distance_matching;
    int32_t ldm_hash_log;
    int32_t ldm_min_match;
    int32_t ldm_bucket_size_log;
    int32_t ldm_hash_rate_log;
    int32_t content_size_flag;
    int32_t checksum_flag;
    int32_t dict_id_flag;
    int32_t workers;
    int32_t job_size;
    int32_t overlap_log;
};

// d_lz4_options
//   struct: the LZ4 frame tuning block (7 knobs).  `block_size` takes a
// d_lz4_block_size value and `block_mode` a d_lz4_block_mode; the remaining
// four are flags.
struct d_lz4_options
{
    int32_t level;
    int32_t block_size;
    int32_t block_mode;
    int32_t content_checksum;
    int32_t block_checksum;
    int32_t store_content_size;
    int32_t favor_dec_speed;
};

// d_brotli_options
//   struct: the Brotli tuning block (5 knobs).  `mode` takes a d_brotli_mode
// value; `large_window` is a flag.
struct d_brotli_options
{
    int32_t quality;
    int32_t window_bits;
    int32_t block_bits;
    int32_t mode;
    int32_t large_window;
};

// d_compress_options
//   struct: the whole codec-tuning surface -- the generic effort plus every
// per-codec block.  Plain data, no allocation, trivially copyable, and
// byte-identical under C and C++.  A pristine set has every knob at
// D_COMPRESS_KNOB_UNSET; see D_COMPRESS_OPTIONS_INIT.
struct d_compress_options
{
    int32_t                     level;
    struct d_deflate_options    deflate;
    struct d_bzip2_options      bzip2;
    struct d_lzma_options       lzma;
    struct d_zstd_options       zstd;
    struct d_lz4_options        lz4;
    struct d_brotli_options     brotli;
};

// D_COMPRESS_OPTIONS_INIT
//   macro: the pristine initialiser -- every knob UNSET.  Written as a nested
// brace initialiser so a caller may declare a const option set at file scope
// with no runtime step:
//     static const struct d_compress_options k_opt = D_COMPRESS_OPTIONS_INIT;
#define D_COMPRESS_OPTIONS_INIT                                                \
{                                                                              \
    D_COMPRESS_KNOB_UNSET,                                                     \
    { D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET },   \
    { D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET },                                                 \
    { D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET },                          \
    { D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET },                                                 \
    { D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET },                                                 \
    { D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,     \
      D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET }                           \
}


// =============================================================================
// V.   LAYOUT ASSERTIONS
// =============================================================================
//   The Layout law: one declaration, size and offset asserted in both
// dialects.  Because every knob is an int32_t, these are exact and portable --
// they hold on every data model rather than only on LP64, so there is no tier
// on which they have to be relaxed.
//
//   The density assertion on d_compress_options is load-bearing beyond drift
// detection: it is the proof that the knob table in section VI can be a dense
// offset table.  If a member of another type is ever added, the assertion
// fires here rather than the table silently going stale.

D_STATIC_ASSERT(sizeof(struct d_deflate_options) == (3 * sizeof(int32_t)),
                "d_deflate_options: layout drift");
D_STATIC_ASSERT(sizeof(struct d_bzip2_options) == (4 * sizeof(int32_t)),
                "d_bzip2_options: layout drift");
D_STATIC_ASSERT(sizeof(struct d_lzma_options) == (11 * sizeof(int32_t)),
                "d_lzma_options: layout drift");
D_STATIC_ASSERT(sizeof(struct d_zstd_options) == (19 * sizeof(int32_t)),
                "d_zstd_options: layout drift");
D_STATIC_ASSERT(sizeof(struct d_lz4_options) == (7 * sizeof(int32_t)),
                "d_lz4_options: layout drift");
D_STATIC_ASSERT(sizeof(struct d_brotli_options) == (5 * sizeof(int32_t)),
                "d_brotli_options: layout drift");
D_STATIC_ASSERT(sizeof(struct d_compress_options) == (50 * sizeof(int32_t)),
                "d_compress_options: not densely packed; knob table is stale");

D_STATIC_ASSERT(offsetof(struct d_compress_options, level) == 0,
                "d_compress_options: field drift at level");
D_STATIC_ASSERT(offsetof(struct d_compress_options, deflate) ==
                (1 * sizeof(int32_t)),
                "d_compress_options: field drift at deflate");
D_STATIC_ASSERT(offsetof(struct d_compress_options, bzip2) ==
                (4 * sizeof(int32_t)),
                "d_compress_options: field drift at bzip2");
D_STATIC_ASSERT(offsetof(struct d_compress_options, lzma) ==
                (8 * sizeof(int32_t)),
                "d_compress_options: field drift at lzma");
D_STATIC_ASSERT(offsetof(struct d_compress_options, zstd) ==
                (19 * sizeof(int32_t)),
                "d_compress_options: field drift at zstd");
D_STATIC_ASSERT(offsetof(struct d_compress_options, lz4) ==
                (38 * sizeof(int32_t)),
                "d_compress_options: field drift at lz4");
D_STATIC_ASSERT(offsetof(struct d_compress_options, brotli) ==
                (45 * sizeof(int32_t)),
                "d_compress_options: field drift at brotli");


// =============================================================================
// VI.  THE KNOB TABLE
// =============================================================================
//   A dense index over the flat option set.  It exists so that comparison,
// "is this pristine?", diff reporting, serialisation and CLI binding are each
// ONE loop over fifty entries instead of fifty hand-written cases -- the
// fifty-field walk test_compress_options.hpp records as re-derived by hand at
// every boundary.  A diff is a fold over this table.
//
//   Enumerator order MUST match declaration order in section IV; the layout
// assertions above are what guarantee the correspondence is dense.
//
//   Access goes through d_compress_options_knob_get / _set, which resolve an
// index to a byte offset and memcpy.  That is deliberate: it is well defined in
// both languages, whereas casting the struct to int32_t* and indexing relies on
// an aliasing guarantee C++ does not give.
//
//   Because the struct is asserted to be exactly fifty int32_t with no padding,
// the offset table is the IDENTITY -- knob k lives at byte offset
// k * sizeof(int32_t), verified: D_COMPRESS_KNOB_ZSTD_WINDOW_LOG is 20 and
// offsetof(struct d_compress_options, zstd.window_log) is 80.  So
// d_compress_knob_offset needs no stored table and cannot fall out of step with
// the struct; the density assertion in section V is what makes that safe to
// rely on, and if a member of another type is ever added the assertion fires
// before the arithmetic goes wrong.

// d_compress_knob
//   enum: a dense index over every knob of a d_compress_options, in
// declaration order.
enum d_compress_knob
{
    D_COMPRESS_KNOB_LEVEL                       = 0,

    D_COMPRESS_KNOB_DEFLATE_WINDOW_BITS         = 1,
    D_COMPRESS_KNOB_DEFLATE_MEM_LEVEL           = 2,
    D_COMPRESS_KNOB_DEFLATE_STRATEGY            = 3,

    D_COMPRESS_KNOB_BZIP2_BLOCK_SIZE_100K       = 4,
    D_COMPRESS_KNOB_BZIP2_WORK_FACTOR           = 5,
    D_COMPRESS_KNOB_BZIP2_VERBOSITY             = 6,
    D_COMPRESS_KNOB_BZIP2_SMALL_DECOMPRESS      = 7,

    D_COMPRESS_KNOB_LZMA_EXTREME                = 8,
    D_COMPRESS_KNOB_LZMA_CHECK                  = 9,
    D_COMPRESS_KNOB_LZMA_DICT_SIZE              = 10,
    D_COMPRESS_KNOB_LZMA_LC                     = 11,
    D_COMPRESS_KNOB_LZMA_LP                     = 12,
    D_COMPRESS_KNOB_LZMA_PB                     = 13,
    D_COMPRESS_KNOB_LZMA_MODE                   = 14,
    D_COMPRESS_KNOB_LZMA_NICE_LEN               = 15,
    D_COMPRESS_KNOB_LZMA_MF                     = 16,
    D_COMPRESS_KNOB_LZMA_DEPTH                  = 17,
    D_COMPRESS_KNOB_LZMA_THREADS                = 18,

    D_COMPRESS_KNOB_ZSTD_LEVEL                  = 19,
    D_COMPRESS_KNOB_ZSTD_WINDOW_LOG             = 20,
    D_COMPRESS_KNOB_ZSTD_HASH_LOG               = 21,
    D_COMPRESS_KNOB_ZSTD_CHAIN_LOG              = 22,
    D_COMPRESS_KNOB_ZSTD_SEARCH_LOG             = 23,
    D_COMPRESS_KNOB_ZSTD_MIN_MATCH              = 24,
    D_COMPRESS_KNOB_ZSTD_TARGET_LENGTH          = 25,
    D_COMPRESS_KNOB_ZSTD_STRATEGY               = 26,
    D_COMPRESS_KNOB_ZSTD_LONG_DISTANCE_MATCHING = 27,
    D_COMPRESS_KNOB_ZSTD_LDM_HASH_LOG           = 28,
    D_COMPRESS_KNOB_ZSTD_LDM_MIN_MATCH          = 29,
    D_COMPRESS_KNOB_ZSTD_LDM_BUCKET_SIZE_LOG    = 30,
    D_COMPRESS_KNOB_ZSTD_LDM_HASH_RATE_LOG      = 31,
    D_COMPRESS_KNOB_ZSTD_CONTENT_SIZE_FLAG      = 32,
    D_COMPRESS_KNOB_ZSTD_CHECKSUM_FLAG          = 33,
    D_COMPRESS_KNOB_ZSTD_DICT_ID_FLAG           = 34,
    D_COMPRESS_KNOB_ZSTD_WORKERS                = 35,
    D_COMPRESS_KNOB_ZSTD_JOB_SIZE               = 36,
    D_COMPRESS_KNOB_ZSTD_OVERLAP_LOG            = 37,

    D_COMPRESS_KNOB_LZ4_LEVEL                   = 38,
    D_COMPRESS_KNOB_LZ4_BLOCK_SIZE              = 39,
    D_COMPRESS_KNOB_LZ4_BLOCK_MODE              = 40,
    D_COMPRESS_KNOB_LZ4_CONTENT_CHECKSUM        = 41,
    D_COMPRESS_KNOB_LZ4_BLOCK_CHECKSUM          = 42,
    D_COMPRESS_KNOB_LZ4_STORE_CONTENT_SIZE      = 43,
    D_COMPRESS_KNOB_LZ4_FAVOR_DEC_SPEED         = 44,

    D_COMPRESS_KNOB_BROTLI_QUALITY              = 45,
    D_COMPRESS_KNOB_BROTLI_WINDOW_BITS          = 46,
    D_COMPRESS_KNOB_BROTLI_BLOCK_BITS           = 47,
    D_COMPRESS_KNOB_BROTLI_MODE                 = 48,
    D_COMPRESS_KNOB_BROTLI_LARGE_WINDOW         = 49
};

// D_COMPRESS_KNOB_COUNT
//   constant: the number of knobs in a d_compress_options.  Asserted against
// the struct's size in section V.
#define D_COMPRESS_KNOB_COUNT   50

D_STATIC_ASSERT((D_COMPRESS_KNOB_BROTLI_LARGE_WINDOW + 1) ==
                D_COMPRESS_KNOB_COUNT,
                "d_compress_knob: index set is not dense");


const char*        d_compress_knob_name(enum d_compress_knob _knob);
size_t             d_compress_knob_offset(enum d_compress_knob _knob);
enum d_codec_id    d_compress_knob_codec(enum d_compress_knob _knob);
int                d_compress_knob_from_name(const char*           _name,
                                             enum d_compress_knob* _out_knob);
int32_t            d_compress_options_knob_get(
                    const struct d_compress_options* _options,
                    enum d_compress_knob             _knob);
int                d_compress_options_knob_set(
                    struct d_compress_options* _options,
                    enum d_compress_knob       _knob,
                    int32_t                    _value);


// =============================================================================
// VII. OPTION OPERATIONS
// =============================================================================
//   d_compress_options_init and D_COMPRESS_OPTIONS_INIT produce the same
// bytes; the macro is for static storage, the function for a set already in
// hand.
//
//   d_compress_options_resolve is the parity-critical one.  It takes a set
// that may carry UNSET knobs and writes one in which every knob relevant to
// _codec holds a concrete value from the core's pinned default table.  Both
// language faces call it before dispatching to a backend, so the backend's own
// defaults are never consulted and cannot introduce an environmental
// divergence.  Knobs belonging to other codecs are left as they are: resolving
// them would report spurious differences from a set the caller never touched.
//
//   d_compress_options_diff walks the knob table and writes the index of each
// differing knob into _out_knobs, returning the count.  Pass _out_knobs ==
// NULL to count only.  Rendering those indices as text ("zstd.window_log") is
// d_compress_knob_name's job, and formatting the list is the caller's -- the
// core reports the difference, it does not choose a presentation.

void               d_compress_options_init(struct d_compress_options* _options);
int                d_compress_options_equal(
                    const struct d_compress_options* _a,
                    const struct d_compress_options* _b);
int                d_compress_options_are_default(
                    const struct d_compress_options* _options);
size_t             d_compress_options_diff(
                    const struct d_compress_options* _a,
                    const struct d_compress_options* _b,
                    enum d_compress_knob*            _out_knobs,
                    size_t                           _out_capacity);
enum d_pack_status d_compress_options_resolve(
                    enum d_codec_id                  _codec,
                    const struct d_compress_options* _in,
                    struct d_compress_options*       _out);
enum d_pack_status d_compress_options_validate(
                    enum d_codec_id                  _codec,
                    const struct d_compress_options* _options,
                    enum d_compress_knob*            _out_offender);
int32_t            d_compress_resolve_level(enum d_codec_id _codec,
                                            int32_t         _generic_level);


// =============================================================================
// VIII. AVAILABILITY
// =============================================================================
//   Availability is a build fact, and a codec that is merely absent is a
// FORMAL condition (D_PACK_STATUS_UNAVAILABLE), not an error: a correct caller
// either round-trips or is told the codec is not there.  test_compress.hpp's
// facade_roundtrip_ok already encodes exactly that contract.
//
//   d_codec_is_available is a function and not a macro because a build may
// gain a codec through a runtime-loaded backend, and because a caller
// switching over a runtime d_codec_id cannot use a preprocessor answer.  The
// compile-time roll-ups in env_compress.h remain available for a caller that
// wants to omit code entirely rather than branch.

int                d_codec_is_available(enum d_codec_id _codec);
int                d_codec_can_compress(enum d_codec_id _codec);
int                d_codec_can_decompress(enum d_codec_id _codec);
int                d_codec_backend_id(enum d_codec_id _codec);
const char*        d_codec_backend_name(enum d_codec_id _codec);
size_t             d_codec_available_list(enum d_codec_id* _out_codecs,
                                          size_t           _out_capacity);


// =============================================================================
// IX.  STREAM SIGNATURES
// =============================================================================
//   Magic-byte recognition, lifted out of test_compress.hpp so that the
// framing rules live in the core with the codecs rather than in the test
// layer.  The C++ predicates (has_gzip_magic, has_zstd_magic, ...) and the
// tag-dispatched has_expected_signature<Codec> become wrappers over these.
//
//   store, raw DEFLATE and brotli carry no fixed magic, so
// d_codec_signature_matches answers 1 for them unconditionally.  That is a
// property of the formats, not a shortcut, and a conformance test should
// assert it rather than treat it as a gap.
//
//   d_pack_crc32 is here rather than in archive_common.h because BOTH layers
// need it -- the gzip trailer carries a CRC32 of the uncompressed data, and
// every ZIP local header and central-directory record carries one per entry.
// Defining it once in the lower layer is what stops the two from becoming two
// implementations of one formal object.  It is the standard IEEE 802.3
// polynomial (reflected 0xEDB88320); seed a fresh run with 0.

int                d_codec_signature_matches(enum d_codec_id _codec,
                                             const void*     _data,
                                             size_t          _size);
int                d_codec_signature_length(enum d_codec_id _codec);
int                d_codec_detect(const void*      _data,
                                  size_t           _size,
                                  enum d_codec_id* _out_codec);
uint32_t           d_pack_crc32(uint32_t    _seed,
                                const void* _data,
                                size_t      _size);


// =============================================================================
// X.   TRANSFORM LEAVES
// =============================================================================
//   The four transforms, each in a buffer form and a sink form.  The buffer
// form is defined as the sink form against a d_pack_buffer_sink, so there is
// one algorithm per codec.
//
//   BUFFER FORM contract, restated because it is the one thing a caller gets
// wrong: _out == NULL with _out_capacity == 0 MEASURES; a too-small buffer
// returns D_PACK_STATUS_BUFFER_TOO_SMALL with *_out_size set to the required
// count.  _out_size is never NULL.
//
//   d_compress_bound is an upper bound computed from _in_size alone and is
// cheap; it always exceeds the true output for an incompressible input, which
// is the case that makes compressed output LARGER than its input.  A caller
// sizing a buffer once should use it.  A caller that needs the exact size
// should MEASURE instead, and pay the extra pass knowingly.
//
//   Decompression has no bound: the expansion ratio is a property of the
// stream, not of its length.  d_decompress therefore has no _bound companion,
// and a caller either measures, reads a size the container recorded, or drives
// the sink form.  This asymmetry is real and is not an omission.

enum d_pack_status d_compress_bound(
                    enum d_codec_id                  _codec,
                    size_t                           _in_size,
                    const struct d_compress_options* _options,
                    size_t*                          _out_bound);
enum d_pack_status d_compress(enum d_codec_id                  _codec,
                              const void*                      _in,
                              size_t                           _in_size,
                              const struct d_compress_options* _options,
                              void*                            _out,
                              size_t                           _out_capacity,
                              size_t*                          _out_size);
enum d_pack_status d_decompress(enum d_codec_id _codec,
                                const void*     _in,
                                size_t          _in_size,
                                void*           _out,
                                size_t          _out_capacity,
                                size_t*         _out_size);
enum d_pack_status d_compress_to_sink(
                    enum d_codec_id                  _codec,
                    const void*                      _in,
                    size_t                           _in_size,
                    const struct d_compress_options* _options,
                    struct d_pack_sink               _sink,
                    size_t*                          _out_size);
enum d_pack_status d_decompress_to_sink(enum d_codec_id    _codec,
                                        const void*        _in,
                                        size_t             _in_size,
                                        struct d_pack_sink _sink,
                                        size_t*            _out_size);


D_EXTERN_C_END


#endif  // DJINTERP_UTIL_COMPRESS_COMMON_
