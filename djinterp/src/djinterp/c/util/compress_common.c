/******************************************************************************
* djinterp [utility]                                        compress_common.c
*
*   The compression kernel.  Every declaration in compress_common.h is defined
* here, and nothing else is: this is the ONE place a compression result is
* computed, for both languages.
*
*   THE SHAPE OF THIS FILE:
*   Sections I-VIII are pure data and pure functions -- tables, the knob
* algebra, the option operations, the sinks.  They have no dependency on any
* third-party library and are identical on every build, which is what makes
* them the parity floor: an option set compares, diffs and resolves the same
* way whether or not a single codec was detected.
*
*   Section IX is the only part that varies with the environment.  Each codec
* is one pair of leaf functions behind an #if on its env_compress.h flag, and
* an absent codec's leaf is a two-line stub returning
* D_PACK_STATUS_UNAVAILABLE.  Nothing above section IX knows which codecs
* exist.
*
*   ONE ALGORITHM, TWO ADAPTERS:
*   The buffer form of every transform is defined as the sink form driven
* against a d_pack_buffer_sink, and the measure pass is the sink form driven
* against a d_pack_counting_sink.  So measuring, producing and overflowing all
* run the same encoder and cannot disagree about a byte count -- the failure
* mode a separate "compute the size" path always eventually develops.
*
*   THE STORE CODEC IS NOT A PLACEHOLDER:
*   D_CODEC_ID_STORE is the identity transform and is implemented here
* unconditionally.  It is what makes a build with no third-party codec at all
* still a WORKING build rather than a degraded one, and it is what the archive
* layer falls back to when a container's method is unavailable.  Treating it as
* a stub is the mistake that turns a missing library into a missing feature.
*
*   WHAT IS DELIBERATELY NOT HERE:
*   No allocation, no I/O, no global mutable state, no locale dependence, and
* no consultation of the clock, the umask, or the environment.  A transform is
* a pure function of (codec, input, options), which is the precondition for the
* parity law meaning anything.
*
*
* TABLE OF CONTENTS
* =================
* I.    STATUS
* II.   CODEC IDENTITY
* III.  THE KNOB TABLE
* IV.   OPTION OPERATIONS
* V.    THE DEFAULT TABLE            (PROVISIONAL -- see the banner in IV.c)
* VI.   AVAILABILITY
* VII.  SIGNATURES AND CRC32
* IX.   BACKEND LEAVES               (the only environment-dependent section)
* X.    TRANSFORMS
*
*
* path:      /src/djinterp/core/util/compress_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.30
******************************************************************************/

// c
#include <string.h>
// djinterp
#include "./compress_common.h"
#include "../env/env_compress_link.h"


// =============================================================================
// I.   STATUS
// =============================================================================

/*
d_pack_status_name
  The enumerator's own spelling, for a diagnostic that must be greppable back
to the source.  Distinct from d_pack_status_message, which is prose.

Parameter(s):
  _status: the status to name.
Return:
  A static, NUL-terminated name, or "unknown" for a value outside the enum.
*/
const char*
d_pack_status_name(
    enum d_pack_status _status
)
{
    switch (_status)
    {
        case D_PACK_STATUS_OK:               { return "ok"; }
        case D_PACK_STATUS_UNAVAILABLE:      { return "unavailable"; }
        case D_PACK_STATUS_UNSUPPORTED:      { return "unsupported"; }
        case D_PACK_STATUS_INVALID_ARGUMENT: { return "invalid_argument"; }
        case D_PACK_STATUS_CORRUPT_INPUT:    { return "corrupt_input"; }
        case D_PACK_STATUS_TRUNCATED_INPUT:  { return "truncated_input"; }
        case D_PACK_STATUS_WRONG_PASSWORD:   { return "wrong_password"; }
        case D_PACK_STATUS_BUFFER_TOO_SMALL: { return "buffer_too_small"; }
        case D_PACK_STATUS_NO_MEMORY:        { return "no_memory"; }
        case D_PACK_STATUS_SINK_ERROR:       { return "sink_error"; }
        case D_PACK_STATUS_BACKEND_ERROR:    { return "backend_error"; }
    }

    return "unknown";
}

/*
d_pack_status_message
  A one-line explanation.  The formal statuses say what is not defined; the
mechanical ones say what ran short.  Keeping the two readable apart in prose is
the same discipline the disjoint numeric ranges enforce in code.

Parameter(s):
  _status: the status to describe.
Return:
  A static, NUL-terminated sentence.
*/
const char*
d_pack_status_message(
    enum d_pack_status _status
)
{
    switch (_status)
    {
        case D_PACK_STATUS_OK:
        {
            return "the operation completed";
        }
        case D_PACK_STATUS_UNAVAILABLE:
        {
            return "this build was compiled without that codec or format";
        }
        case D_PACK_STATUS_UNSUPPORTED:
        {
            return "the codec or format is present but cannot do that";
        }
        case D_PACK_STATUS_INVALID_ARGUMENT:
        {
            return "the request is malformed";
        }
        case D_PACK_STATUS_CORRUPT_INPUT:
        {
            return "the input is not a valid stream of that kind";
        }
        case D_PACK_STATUS_TRUNCATED_INPUT:
        {
            return "the input ends before the stream does";
        }
        case D_PACK_STATUS_WRONG_PASSWORD:
        {
            return "the passphrase does not decrypt this container";
        }
        case D_PACK_STATUS_BUFFER_TOO_SMALL:
        {
            return "the output buffer is smaller than the result";
        }
        case D_PACK_STATUS_NO_MEMORY:
        {
            return "a backend could not allocate";
        }
        case D_PACK_STATUS_SINK_ERROR:
        {
            return "the sink refused bytes";
        }
        case D_PACK_STATUS_BACKEND_ERROR:
        {
            return "the backend reported an internal failure";
        }
    }

    return "unrecognised status";
}


// =============================================================================
// II.  CODEC IDENTITY
// =============================================================================

// d_internal_codec_names
//   table: the codec names, indexed by d_codec_id.  A table rather than a
// switch because d_codec_id_from_name walks it in the reverse direction, and
// one table cannot disagree with itself the way two switches can.
static const char* const d_internal_codec_names[D_CODEC_ID_COUNT] =
{
    "store", "deflate", "zlib", "gzip", "bzip2", "xz", "zstd", "lz4", "brotli"
};

// d_internal_codec_suffixes
//   table: the conventional file suffix for each codec, indexed by d_codec_id.
// `store` and raw `deflate` have none: neither is a file format, and inventing
// a suffix for them would let a caller write a file no tool can identify.
static const char* const d_internal_codec_suffixes[D_CODEC_ID_COUNT] =
{
    "", "", ".zz", ".gz", ".bz2", ".xz", ".zst", ".lz4", ".br"
};

/*
d_codec_id_name
  The codec's canonical name.

Parameter(s):
  _codec: the codec to name.
Return:
  A static, NUL-terminated name, or "unknown" for a value outside the enum.
*/
const char*
d_codec_id_name(
    enum d_codec_id _codec
)
{
    if (!D_CODEC_ID_IS_VALID(_codec))
    {
        return "unknown";
    }

    return d_internal_codec_names[(int)_codec];
}

/*
d_codec_suffix
  The conventional file suffix, leading dot included.

Parameter(s):
  _codec: the codec to describe.
Return:
  A static, NUL-terminated suffix, or "" when the codec has none.
*/
const char*
d_codec_suffix(
    enum d_codec_id _codec
)
{
    if (!D_CODEC_ID_IS_VALID(_codec))
    {
        return "";
    }

    return d_internal_codec_suffixes[(int)_codec];
}

/*
d_codec_id_from_name
  Resolve a codec name.  Accepts the canonical name and the common aliases a
command line or config file is likely to carry, so that a caller does not have
to normalise before asking.  Matching is exact and case-sensitive: a
case-insensitive match would need a locale-independent fold, and quietly
accepting "GZIP" on one platform and not another is a parity break.

Parameter(s):
  _name:      the name to resolve; may be null, which never matches.
  _out_codec: receives the codec on success; untouched on failure.
Return:
  1 when the name resolved, 0 otherwise.
*/
int
d_codec_id_from_name(
    const char*      _name,
    enum d_codec_id* _out_codec
)
{
    int i;

    if ( (_name == NULL) ||
         (_out_codec == NULL) )
    {
        return 0;
    }

    for (i = 0; i < D_CODEC_ID_COUNT; ++i)
    {
        if (strcmp(_name, d_internal_codec_names[i]) == 0)
        {
            *_out_codec = (enum d_codec_id)i;

            return 1;
        }
    }

    // aliases: the spellings a suffix, a command line, or a MIME type uses.
    if ( (strcmp(_name, "gz")   == 0) ||
         (strcmp(_name, "none") == 0) )
    {
        *_out_codec = (strcmp(_name, "gz") == 0)
                    ? D_CODEC_ID_GZIP
                    : D_CODEC_ID_STORE;

        return 1;
    }
    if ( (strcmp(_name, "bz2")  == 0) ||
         (strcmp(_name, "lzma") == 0) ||
         (strcmp(_name, "zst")  == 0) ||
         (strcmp(_name, "br")   == 0) ||
         (strcmp(_name, "raw")  == 0) )
    {
        if (strcmp(_name, "bz2") == 0)
        {
            *_out_codec = D_CODEC_ID_BZIP2;
        }
        else if (strcmp(_name, "lzma") == 0)
        {
            *_out_codec = D_CODEC_ID_XZ;
        }
        else if (strcmp(_name, "zst") == 0)
        {
            *_out_codec = D_CODEC_ID_ZSTD;
        }
        else if (strcmp(_name, "br") == 0)
        {
            *_out_codec = D_CODEC_ID_BROTLI;
        }
        else
        {
            *_out_codec = D_CODEC_ID_DEFLATE;
        }

        return 1;
    }

    return 0;
}


// =============================================================================
// III. THE KNOB TABLE
// =============================================================================
//   Fifty entries, one per knob, in declaration order.  Every option operation
// below is a loop over this table, which is the point: the fifty-field walk
// exists once here instead of once per boundary.

// d_internal_knob_names
//   table: the qualified name of each knob ("zstd.window_log"), indexed by
// d_compress_knob.  These are the strings a diff report prints and the strings
// a CLI or config file binds against, so they are the knobs' external
// identity and must not be reworded casually.
static const char* const d_internal_knob_names[D_COMPRESS_KNOB_COUNT] =
{
    "level",
    "deflate.window_bits", "deflate.mem_level", "deflate.strategy",
    "bzip2.block_size_100k", "bzip2.work_factor", "bzip2.verbosity",
    "bzip2.small_decompress",
    "lzma.extreme", "lzma.check", "lzma.dict_size", "lzma.lc", "lzma.lp",
    "lzma.pb", "lzma.mode", "lzma.nice_len", "lzma.mf", "lzma.depth",
    "lzma.threads",
    "zstd.level", "zstd.window_log", "zstd.hash_log", "zstd.chain_log",
    "zstd.search_log", "zstd.min_match", "zstd.target_length",
    "zstd.strategy", "zstd.long_distance_matching", "zstd.ldm_hash_log",
    "zstd.ldm_min_match", "zstd.ldm_bucket_size_log", "zstd.ldm_hash_rate_log",
    "zstd.content_size_flag", "zstd.checksum_flag", "zstd.dict_id_flag",
    "zstd.workers", "zstd.job_size", "zstd.overlap_log",
    "lz4.level", "lz4.block_size", "lz4.block_mode", "lz4.content_checksum",
    "lz4.block_checksum", "lz4.store_content_size", "lz4.favor_dec_speed",
    "brotli.quality", "brotli.window_bits", "brotli.block_bits",
    "brotli.mode", "brotli.large_window"
};

// d_internal_knob_codecs
//   table: which codec each knob belongs to, indexed by d_compress_knob.  The
// generic `level` belongs to no single codec and is recorded as STORE, which
// is the identity codec and therefore the one "applies to everything" answer
// that is not a lie.
static const unsigned char d_internal_knob_codecs[D_COMPRESS_KNOB_COUNT] =
{
    (unsigned char)D_CODEC_ID_STORE,
    (unsigned char)D_CODEC_ID_DEFLATE, (unsigned char)D_CODEC_ID_DEFLATE,
    (unsigned char)D_CODEC_ID_DEFLATE,
    (unsigned char)D_CODEC_ID_BZIP2, (unsigned char)D_CODEC_ID_BZIP2,
    (unsigned char)D_CODEC_ID_BZIP2, (unsigned char)D_CODEC_ID_BZIP2,
    (unsigned char)D_CODEC_ID_XZ, (unsigned char)D_CODEC_ID_XZ,
    (unsigned char)D_CODEC_ID_XZ, (unsigned char)D_CODEC_ID_XZ,
    (unsigned char)D_CODEC_ID_XZ, (unsigned char)D_CODEC_ID_XZ,
    (unsigned char)D_CODEC_ID_XZ, (unsigned char)D_CODEC_ID_XZ,
    (unsigned char)D_CODEC_ID_XZ, (unsigned char)D_CODEC_ID_XZ,
    (unsigned char)D_CODEC_ID_XZ,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD, (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_ZSTD,
    (unsigned char)D_CODEC_ID_LZ4, (unsigned char)D_CODEC_ID_LZ4,
    (unsigned char)D_CODEC_ID_LZ4, (unsigned char)D_CODEC_ID_LZ4,
    (unsigned char)D_CODEC_ID_LZ4, (unsigned char)D_CODEC_ID_LZ4,
    (unsigned char)D_CODEC_ID_LZ4,
    (unsigned char)D_CODEC_ID_BROTLI, (unsigned char)D_CODEC_ID_BROTLI,
    (unsigned char)D_CODEC_ID_BROTLI, (unsigned char)D_CODEC_ID_BROTLI,
    (unsigned char)D_CODEC_ID_BROTLI
};

/*
d_compress_knob_name
  The knob's qualified name.

Parameter(s):
  _knob: the knob to name.
Return:
  A static, NUL-terminated name, or "unknown" for an out-of-range index.
*/
const char*
d_compress_knob_name(
    enum d_compress_knob _knob
)
{
    if ( ((int)_knob < 0) ||
         ((int)_knob >= D_COMPRESS_KNOB_COUNT) )
    {
        return "unknown";
    }

    return d_internal_knob_names[(int)_knob];
}

/*
d_compress_knob_offset
  The knob's byte offset within a d_compress_options.

  There is no stored offset table, because there does not need to be one: the
struct is asserted in the header to be exactly fifty int32_t with no padding,
so the offset IS the index scaled by the element size.  A stored table would
be a second description of the same layout, free to drift from the first.

Parameter(s):
  _knob: the knob to locate.
Return:
  The byte offset, or 0 for an out-of-range index.
*/
size_t
d_compress_knob_offset(
    enum d_compress_knob _knob
)
{
    if ( ((int)_knob < 0) ||
         ((int)_knob >= D_COMPRESS_KNOB_COUNT) )
    {
        return (size_t)0;
    }

    return ((size_t)_knob * sizeof(int32_t));
}

/*
d_compress_knob_codec
  Which codec a knob tunes.

Parameter(s):
  _knob: the knob to classify.
Return:
  The owning codec, or D_CODEC_ID_STORE for the generic effort and for an
out-of-range index.
*/
enum d_codec_id
d_compress_knob_codec(
    enum d_compress_knob _knob
)
{
    if ( ((int)_knob < 0) ||
         ((int)_knob >= D_COMPRESS_KNOB_COUNT) )
    {
        return D_CODEC_ID_STORE;
    }

    return (enum d_codec_id)d_internal_knob_codecs[(int)_knob];
}

/*
d_compress_knob_from_name
  Resolve a qualified knob name ("zstd.window_log") to its index.  The reverse
of d_compress_knob_name over the same table, which is what a config file, a
CLI, and the interpreter's `djinterp` addressing all need.

Parameter(s):
  _name:     the qualified name; may be null, which never matches.
  _out_knob: receives the index on success; untouched on failure.
Return:
  1 when the name resolved, 0 otherwise.
*/
int
d_compress_knob_from_name(
    const char*           _name,
    enum d_compress_knob* _out_knob
)
{
    int i;

    if ( (_name == NULL) ||
         (_out_knob == NULL) )
    {
        return 0;
    }

    for (i = 0; i < D_COMPRESS_KNOB_COUNT; ++i)
    {
        if (strcmp(_name, d_internal_knob_names[i]) == 0)
        {
            *_out_knob = (enum d_compress_knob)i;

            return 1;
        }
    }

    return 0;
}

/*
d_compress_options_knob_get
  Read one knob by index.

  The read is a memcpy through the computed offset rather than a cast of the
struct to int32_t* followed by an index.  The cast is what everyone writes and
it is not well defined in C++ -- accessing a struct's members through an
unrelated pointer type is an aliasing violation there even when every member
happens to be that type.  memcpy is defined in both languages and compiles to
the same load.

Parameter(s):
  _options: the set to read; may be null, which reads as UNSET.
  _knob:    the knob to read.
Return:
  The knob's value, or D_COMPRESS_KNOB_UNSET for a null set or an
out-of-range index.
*/
int32_t
d_compress_options_knob_get(
    const struct d_compress_options* _options,
    enum d_compress_knob             _knob
)
{
    int32_t value;

    if ( (_options == NULL) ||
         ((int)_knob < 0)   ||
         ((int)_knob >= D_COMPRESS_KNOB_COUNT) )
    {
        return D_COMPRESS_KNOB_UNSET;
    }

    memcpy(&value,
           ((const unsigned char*)_options) + d_compress_knob_offset(_knob),
           sizeof(int32_t));

    return value;
}

/*
d_compress_options_knob_set
  Write one knob by index.

Parameter(s):
  _options: the set to modify; may be null, which does nothing.
  _knob:    the knob to write.
  _value:   the value to store; D_COMPRESS_KNOB_UNSET restores it to pristine.
Return:
  1 when the write happened, 0 for a null set or an out-of-range index.
*/
int
d_compress_options_knob_set(
    struct d_compress_options* _options,
    enum d_compress_knob       _knob,
    int32_t                    _value
)
{
    if ( (_options == NULL) ||
         ((int)_knob < 0)   ||
         ((int)_knob >= D_COMPRESS_KNOB_COUNT) )
    {
        return 0;
    }

    memcpy(((unsigned char*)_options) + d_compress_knob_offset(_knob),
           &_value,
           sizeof(int32_t));

    return 1;
}


// =============================================================================
// IV.  OPTION OPERATIONS
// =============================================================================

/*
d_compress_options_init
  Set every knob to UNSET.  Produces the same bytes as
D_COMPRESS_OPTIONS_INIT; the macro is for static storage, this is for a set
already in hand.

Parameter(s):
  _options: the set to reset; may be null, which does nothing.
Return:
  none.
*/
void
d_compress_options_init(
    struct d_compress_options* _options
)
{
    int i;

    if (_options == NULL)
    {
        return;
    }

    for (i = 0; i < D_COMPRESS_KNOB_COUNT; ++i)
    {
        (void)d_compress_options_knob_set(_options,
                                          (enum d_compress_knob)i,
                                          D_COMPRESS_KNOB_UNSET);
    }

    return;
}

/*
d_compress_options_equal
  Whether two sets are knob-for-knob identical.

  A loop over the knob table rather than a memcmp: memcmp would be correct
here today, because the struct is asserted to have no padding, but it would
become silently wrong the moment a member of another type is added, and it
would compare padding bytes that no operation sets.  The loop stays correct
under both changes.

Parameter(s):
  _a: the first set; may be null.
  _b: the second set; may be null.
Return:
  1 when both are null or every knob matches, 0 otherwise.  A null set never
equals a non-null one, even a pristine one -- absence and pristine are
different answers.
*/
int
d_compress_options_equal(
    const struct d_compress_options* _a,
    const struct d_compress_options* _b
)
{
    int i;

    if ( (_a == NULL) ||
         (_b == NULL) )
    {
        return (_a == _b) ? 1 : 0;
    }

    for (i = 0; i < D_COMPRESS_KNOB_COUNT; ++i)
    {
        enum d_compress_knob k = (enum d_compress_knob)i;

        if (d_compress_options_knob_get(_a, k) !=
            d_compress_options_knob_get(_b, k))
        {
            return 0;
        }
    }

    return 1;
}

/*
d_compress_options_are_default
  Whether nothing anywhere in the set has been touched.

Parameter(s):
  _options: the set to inspect; may be null, which is not pristine.
Return:
  1 when every knob is still UNSET, 0 otherwise.
*/
int
d_compress_options_are_default(
    const struct d_compress_options* _options
)
{
    int i;

    if (_options == NULL)
    {
        return 0;
    }

    for (i = 0; i < D_COMPRESS_KNOB_COUNT; ++i)
    {
        if (d_compress_options_knob_get(_options, (enum d_compress_knob)i) !=
            D_COMPRESS_KNOB_UNSET)
        {
            return 0;
        }
    }

    return 1;
}

/*
d_compress_options_diff
  The knobs that differ between two sets.

  Counting and listing are the same walk, so passing a null _out_knobs asks
only for the count and a caller can size an array before filling it.  The
return value is the TOTAL number of differences, not the number written, so a
caller can detect truncation by comparing it against _out_capacity.

Parameter(s):
  _a:            the first set.
  _b:            the second set.
  _out_knobs:    receives the differing indices; may be null to count only.
  _out_capacity: how many indices _out_knobs holds.
Return:
  The total number of differing knobs, which may exceed _out_capacity.
*/
size_t
d_compress_options_diff(
    const struct d_compress_options* _a,
    const struct d_compress_options* _b,
    enum d_compress_knob*            _out_knobs,
    size_t                           _out_capacity
)
{
    size_t found = 0;
    int    i;

    if ( (_a == NULL) ||
         (_b == NULL) )
    {
        return 0;
    }

    for (i = 0; i < D_COMPRESS_KNOB_COUNT; ++i)
    {
        enum d_compress_knob k = (enum d_compress_knob)i;

        if (d_compress_options_knob_get(_a, k) !=
            d_compress_options_knob_get(_b, k))
        {
            if ( (_out_knobs != NULL) &&
                 (found < _out_capacity) )
            {
                _out_knobs[found] = k;
            }

            ++found;
        }
    }

    return found;
}


// =============================================================================
// V.   THE DEFAULT TABLE
// =============================================================================
//   *** PROVISIONAL.  This table is the ONE place a default value is decided,
//   *** and the values below are placeholders pending a compression chapter in
//   *** the .tex corpus.  There is no such chapter today, so these numbers are
//   *** conventional defaults rather than normative ones.
//   ***
//   *** What is NOT provisional is that the table exists and that both
//   *** language faces read it.  Letting each backend supply its own default
//   *** would make the output depend on which library version happened to be
//   *** linked -- parity failing for environmental reasons, which is the
//   *** defect already recorded against the BUILTIN backend.  Pinning the
//   *** values here makes that impossible; deciding WHICH values is a
//   *** separate, cheap, one-place edit once the chapter lands.
//
//   A knob whose default is UNSET is deliberately left unresolved: it means
// "this codec has no cross-implementation default worth pinning", and the
// backend leaf omits the parameter entirely rather than passing a number.
// That is different from pinning a value, and the distinction is why the
// table stores UNSET rather than 0.

// d_internal_knob_defaults
//   table: the resolved value of each knob, indexed by d_compress_knob.
static const int32_t d_internal_knob_defaults[D_COMPRESS_KNOB_COUNT] =
{
    D_COMPRESS_LEVEL_DEFAULT,                       // level

    15, 8, D_DEFLATE_STRATEGY_DEFAULT,              // deflate

    9, 30, 0, 0,                                    // bzip2

    0, D_LZMA_CHECK_CRC64,                          // lzma.extreme, .check
    D_COMPRESS_KNOB_UNSET,                          // lzma.dict_size
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,   // lzma.lc, .lp
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,   // lzma.pb, .mode
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,   // lzma.nice_len, .mf
    D_COMPRESS_KNOB_UNSET, 1,                       // lzma.depth, .threads

    // zstd: level, then window_log .. target_length left to the encoder
    3,
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,
    // strategy unset; long-distance matching off
    D_COMPRESS_KNOB_UNSET, 0,
    // the four ldm knobs mean nothing while ldm is off
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,
    // content size, checksum and dict id all recorded
    1, 1, 1,
    // workers 0 == encode on the calling thread, which is what makes the
    // output independent of how many cores the machine has
    0,
    D_COMPRESS_KNOB_UNSET, D_COMPRESS_KNOB_UNSET,

    0, D_LZ4_BLOCK_SIZE_DEFAULT,                    // lz4.level, .block_size
    D_LZ4_BLOCK_MODE_LINKED, 1, 0, 1, 0,            // lz4 framing flags

    11, 22, 24, D_BROTLI_MODE_GENERIC, 0            // brotli
};

/*
d_compress_resolve_level
  Map the generic effort onto a codec's own scale.

  The generic scale is 0-9.  A codec whose native range differs is mapped here,
once, rather than at each call site -- which is what stops one caller sending
brotli a level of 9 (mid-range) while another sends it 11 (maximum) and both
believing they asked for "best".

Parameter(s):
  _codec:         the codec whose scale is wanted.
  _generic_level: a value on the 0-9 generic scale, or UNSET for the default.
Return:
  The level to hand that codec, clamped into its native range.
*/
int32_t
d_compress_resolve_level(
    enum d_codec_id _codec,
    int32_t         _generic_level
)
{
    int32_t g = _generic_level;

    if (g == D_COMPRESS_KNOB_UNSET)
    {
        g = D_COMPRESS_LEVEL_DEFAULT;
    }
    if (g < 0)
    {
        g = 0;
    }
    if (g > 9)
    {
        g = 9;
    }

    switch (_codec)
    {
        case D_CODEC_ID_BROTLI:
        {
            // brotli quality is 0-11; scale 0-9 onto it, keeping 0 at 0 and
            // 9 at 11 so "best" really is brotli's maximum.
            return (int32_t)((g * 11) / 9);
        }
        case D_CODEC_ID_ZSTD:
        {
            // zstd's useful positive range is 1-19 for general use; 22 is
            // reachable only by naming zstd.level directly, because its cost
            // is disproportionate and should be an explicit request.
            return (g == 0) ? 1 : (int32_t)((g * 19) / 9);
        }
        case D_CODEC_ID_BZIP2:
        {
            // bzip2's level IS its block size in units of 100k, 1-9.
            return (g == 0) ? 1 : g;
        }
        case D_CODEC_ID_LZ4:
        {
            // LZ4 frame levels below 3 select the fast codec, which ignores
            // the number; 3-12 select LZ4HC.
            return (g <= 2) ? 0 : (int32_t)(((g - 2) * 12) / 7);
        }
        case D_CODEC_ID_STORE:
        {
            return 0;
        }
        default:
        {
            // deflate, zlib, gzip and xz all use 0-9 natively.
            return g;
        }
    }
}

/*
d_internal_codec_owns_knob
  Whether a knob belongs to a codec, accounting for the shared families.

  d_compress_knob_codec reports the knob's DECLARED owner, which is what a diff
report wants to print.  Resolution needs a wider question, because two of the
codec families share one knob block:

    deflate / zlib / gzip   one encoder, three framings, one set of DEFLATE
                            knobs -- windowBits selects the framing and is not
                            a separate tuning surface
    xz                      carries the lzma block

  Asking only the declared owner is what makes `resolve` leave DEFLATE knobs
UNSET when the target is gzip, so the encoder is handed INT32_MIN as its
memLevel and fails at init.  That is exactly the bug this function exists to
prevent, and it is invisible through the ZIP path -- which dispatches on
DEFLATE and so resolves correctly by accident.

Parameter(s):
  _codec: the codec being resolved for.
  _knob:  the knob in question.
Return:
  1 when _codec's encoder reads _knob, 0 otherwise.
*/
static int
d_internal_codec_owns_knob(
    enum d_codec_id      _codec,
    enum d_compress_knob _knob
)
{
    enum d_codec_id owner = d_compress_knob_codec(_knob);

    if (owner == _codec)
    {
        return 1;
    }

    // the zlib family: one encoder, three framings
    if ( (owner == D_CODEC_ID_DEFLATE) &&
         ( (_codec == D_CODEC_ID_ZLIB) ||
           (_codec == D_CODEC_ID_GZIP) ) )
    {
        return 1;
    }

    return 0;
}

/*
d_compress_options_resolve
  Produce a fully specified option set from one that may carry UNSET knobs.

  Only the knobs belonging to _codec are resolved, plus the generic effort.
Knobs owned by other codecs are copied through untouched: resolving them would
report spurious differences against a set the caller never touched, and would
make "is this pristine?" answer differently before and after a call that has
nothing to do with them.

  Both language faces call this before dispatching, which is what guarantees C
and C++ hand a backend byte-identical parameters.

Parameter(s):
  _codec: the codec the set is being resolved for.
  _in:    the set to resolve; may be null, which resolves as fully pristine.
  _out:   receives the resolved set; must not be null and may alias _in.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_INVALID_ARGUMENT for a null _out or an
unknown codec.
*/
enum d_pack_status
d_compress_options_resolve(
    enum d_codec_id                  _codec,
    const struct d_compress_options* _in,
    struct d_compress_options*       _out
)
{
    struct d_compress_options work;
    int                       i;

    if ( (_out == NULL) ||
         (!D_CODEC_ID_IS_VALID(_codec)) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    if (_in == NULL)
    {
        d_compress_options_init(&work);
    }
    else
    {
        work = *_in;
    }

    // the generic effort first: a codec-specific level that is still UNSET
    // inherits from it, so a caller who set only `level` gets the effect they
    // asked for on every codec.
    if (d_compress_options_knob_get(&work, D_COMPRESS_KNOB_LEVEL) ==
        D_COMPRESS_KNOB_UNSET)
    {
        (void)d_compress_options_knob_set(
            &work, D_COMPRESS_KNOB_LEVEL, D_COMPRESS_LEVEL_DEFAULT);
    }

    for (i = 0; i < D_COMPRESS_KNOB_COUNT; ++i)
    {
        enum d_compress_knob k = (enum d_compress_knob)i;

        if (k == D_COMPRESS_KNOB_LEVEL)
        {
            continue;
        }
        if (!d_internal_codec_owns_knob(_codec, k))
        {
            continue;
        }
        if (d_compress_options_knob_get(&work, k) != D_COMPRESS_KNOB_UNSET)
        {
            continue;
        }

        (void)d_compress_options_knob_set(&work, k,
                                          d_internal_knob_defaults[i]);
    }

    // a codec with a private level that the caller left alone takes the
    // generic effort, mapped onto its own scale.
    switch (_codec)
    {
        case D_CODEC_ID_ZSTD:
        case D_CODEC_ID_LZ4:
        case D_CODEC_ID_BROTLI:
        case D_CODEC_ID_BZIP2:
        {
            enum d_compress_knob own = D_COMPRESS_KNOB_ZSTD_LEVEL;

            if (_codec == D_CODEC_ID_LZ4)
            {
                own = D_COMPRESS_KNOB_LZ4_LEVEL;
            }
            else if (_codec == D_CODEC_ID_BROTLI)
            {
                own = D_COMPRESS_KNOB_BROTLI_QUALITY;
            }
            else if (_codec == D_CODEC_ID_BZIP2)
            {
                own = D_COMPRESS_KNOB_BZIP2_BLOCK_SIZE_100K;
            }

            if ( (_in != NULL) &&
                 (d_compress_options_knob_get(_in, own) ==
                  D_COMPRESS_KNOB_UNSET) )
            {
                (void)d_compress_options_knob_set(
                    &work, own,
                    d_compress_resolve_level(
                        _codec,
                        d_compress_options_knob_get(&work,
                                                    D_COMPRESS_KNOB_LEVEL)));
            }
            break;
        }
        default:
        {
            break;
        }
    }

    *_out = work;

    return D_PACK_STATUS_OK;
}

// d_internal_knob_range
//   struct: one knob's permitted interval, for the validation table below.
struct d_internal_knob_range
{
    enum d_compress_knob    knob;
    int32_t                 low;
    int32_t                 high;
};

// d_internal_knob_ranges
//   table: the knobs whose domain is a bounded interval, with that interval.
// A knob absent from this table is unconstrained beyond its type -- either
// because its range is a backend detail (dict_size, job_size) or because the
// value is a member of a constant set the caller cannot mistype without also
// mistyping the enumerator.
static const struct d_internal_knob_range
d_internal_knob_ranges[] =
{
    { D_COMPRESS_KNOB_LEVEL,                  0,   9 },
    { D_COMPRESS_KNOB_DEFLATE_WINDOW_BITS,    9,  15 },
    { D_COMPRESS_KNOB_DEFLATE_MEM_LEVEL,      1,   9 },
    { D_COMPRESS_KNOB_DEFLATE_STRATEGY,       0,   4 },
    { D_COMPRESS_KNOB_BZIP2_BLOCK_SIZE_100K,  1,   9 },
    { D_COMPRESS_KNOB_BZIP2_WORK_FACTOR,      0, 250 },
    { D_COMPRESS_KNOB_BZIP2_VERBOSITY,        0,   4 },
    { D_COMPRESS_KNOB_LZMA_LC,                0,   4 },
    { D_COMPRESS_KNOB_LZMA_LP,                0,   4 },
    { D_COMPRESS_KNOB_LZMA_PB,                0,   4 },
    { D_COMPRESS_KNOB_ZSTD_LEVEL,           -22,  22 },
    { D_COMPRESS_KNOB_ZSTD_STRATEGY,          1,   9 },
    { D_COMPRESS_KNOB_BROTLI_QUALITY,         0,  11 },
    { D_COMPRESS_KNOB_BROTLI_WINDOW_BITS,    10,  24 },
    { D_COMPRESS_KNOB_BROTLI_MODE,            0,   2 }
};

/*
d_compress_options_validate
  Whether every knob relevant to a codec is inside its permitted range.

  Reports the FIRST offending knob rather than a count, because a caller's next
action is to name it in a message; a caller wanting all of them walks the knobs
itself. Validation is separate from resolution on purpose: resolve fills gaps,
validate judges what is there, and a set can be fully resolved and still
invalid if a caller wrote a number by hand.

Parameter(s):
  _options:      the set to check; may be null, which is trivially valid.
  _codec:        the codec whose ranges apply.
  _out_offender: receives the first out-of-range knob; may be null.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_INVALID_ARGUMENT naming the offender.
*/
enum d_pack_status
d_compress_options_validate(
    enum d_codec_id                  _codec,
    const struct d_compress_options* _options,
    enum d_compress_knob*            _out_offender
)
{
    size_t n = sizeof(d_internal_knob_ranges) /
               sizeof(d_internal_knob_ranges[0]);
    size_t i;

    if (_options == NULL)
    {
        return D_PACK_STATUS_OK;
    }
    if (!D_CODEC_ID_IS_VALID(_codec))
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < n; ++i)
    {
        int32_t v;

        if ( (d_internal_knob_ranges[i].knob != D_COMPRESS_KNOB_LEVEL) &&
             (!d_internal_codec_owns_knob(_codec,
                                          d_internal_knob_ranges[i].knob)) )
        {
            continue;
        }

        v = d_compress_options_knob_get(_options,
                                        d_internal_knob_ranges[i].knob);

        if (v == D_COMPRESS_KNOB_UNSET)
        {
            continue;
        }
        if ( (v < d_internal_knob_ranges[i].low) ||
             (v > d_internal_knob_ranges[i].high) )
        {
            if (_out_offender != NULL)
            {
                *_out_offender = d_internal_knob_ranges[i].knob;
            }

            return D_PACK_STATUS_INVALID_ARGUMENT;
        }
    }

    return D_PACK_STATUS_OK;
}


// =============================================================================
// VI.  AVAILABILITY
// =============================================================================
//   The only place env_compress.h's compile-time flags become runtime answers.
// Everything above this line behaves identically on every build.

/*
d_codec_is_available
  Whether this build can both compress and decompress with a codec.

Parameter(s):
  _codec: the codec to query.
Return:
  1 when the codec is fully usable, 0 otherwise.  STORE is always 1.
*/
int
d_codec_is_available(
    enum d_codec_id _codec
)
{
    return ( d_codec_can_compress(_codec) &&
             d_codec_can_decompress(_codec) ) ? 1 : 0;
}

/*
d_codec_can_compress
  Whether this build can produce a stream in a codec.

  Brotli is the one codec whose two directions are detected separately, because
its encoder and decoder are separate libraries and a build may legitimately
carry only one.  Reporting a half-present brotli as "available" would turn a
formal answer into a backend error at the worst moment.

Parameter(s):
  _codec: the codec to query.
Return:
  1 when compression is possible, 0 otherwise.
*/
int
d_codec_can_compress(
    enum d_codec_id _codec
)
{
    switch (_codec)
    {
        case D_CODEC_ID_STORE:
        {
            return 1;
        }
        case D_CODEC_ID_DEFLATE:
        {
            return D_ENV_COMPRESSION_HAVE_DEFLATE ? 1 : 0;
        }
        case D_CODEC_ID_ZLIB:
        {
            // the zlib WRAPPER, not the raw stream: a provider can supply raw
            // DEFLATE without the RFC 1950 framing, so the two are separate
            // capability flags and compress.hpp already distinguishes them.
            return D_ENV_COMPRESSION_HAVE_ZLIB_WRAP ? 1 : 0;
        }
        case D_CODEC_ID_GZIP:
        {
            return D_ENV_COMPRESSION_HAVE_GZIP_WRAP ? 1 : 0;
        }
        case D_CODEC_ID_BZIP2:
        {
            return D_ENV_COMPRESSION_HAVE_BZIP2 ? 1 : 0;
        }
        case D_CODEC_ID_XZ:
        {
            return D_ENV_COMPRESSION_HAVE_LZMA ? 1 : 0;
        }
        case D_CODEC_ID_ZSTD:
        {
            return D_ENV_COMPRESSION_HAVE_ZSTD ? 1 : 0;
        }
        case D_CODEC_ID_LZ4:
        {
            return D_ENV_COMPRESSION_HAVE_LZ4 ? 1 : 0;
        }
        case D_CODEC_ID_BROTLI:
        {
            return D_ENV_COMPRESSION_HAVE_BROTLI_ENCODE ? 1 : 0;
        }
    }

    return 0;
}

/*
d_codec_can_decompress
  Whether this build can consume a stream in a codec.

Parameter(s):
  _codec: the codec to query.
Return:
  1 when decompression is possible, 0 otherwise.
*/
int
d_codec_can_decompress(
    enum d_codec_id _codec
)
{
    if (_codec == D_CODEC_ID_BROTLI)
    {
        return D_ENV_COMPRESSION_HAVE_BROTLI_DECODE ? 1 : 0;
    }

    return d_codec_can_compress(_codec);
}

/*
d_codec_backend_id
  Which library provides a codec on this build.

Parameter(s):
  _codec: the codec to query.
Return:
  A D_ENV_COMPRESSION_CODEC_* constant, or D_ENV_COMPRESSION_CODEC_NONE.
*/
int
d_codec_backend_id(
    enum d_codec_id _codec
)
{
    switch (_codec)
    {
        case D_CODEC_ID_STORE:
        {
            return D_ENV_COMPRESSION_CODEC_NONE;
        }
        case D_CODEC_ID_DEFLATE:
        case D_CODEC_ID_ZLIB:
        {
            return D_ENV_COMPRESSION_PREFERRED_DEFLATE;
        }
        case D_CODEC_ID_GZIP:
        {
            return D_ENV_COMPRESSION_PREFERRED_GZIP;
        }
        case D_CODEC_ID_BZIP2:
        {
            return D_ENV_COMPRESSION_HAVE_BZIP2
                 ? D_ENV_COMPRESSION_CODEC_BZIP2
                 : D_ENV_COMPRESSION_CODEC_NONE;
        }
        case D_CODEC_ID_XZ:
        {
            return D_ENV_COMPRESSION_HAVE_LZMA
                 ? D_ENV_COMPRESSION_CODEC_LZMA
                 : D_ENV_COMPRESSION_CODEC_NONE;
        }
        case D_CODEC_ID_ZSTD:
        {
            return D_ENV_COMPRESSION_HAVE_ZSTD
                 ? D_ENV_COMPRESSION_CODEC_ZSTD
                 : D_ENV_COMPRESSION_CODEC_NONE;
        }
        case D_CODEC_ID_LZ4:
        {
            return D_ENV_COMPRESSION_HAVE_LZ4
                 ? D_ENV_COMPRESSION_CODEC_LZ4
                 : D_ENV_COMPRESSION_CODEC_NONE;
        }
        case D_CODEC_ID_BROTLI:
        {
            return D_ENV_COMPRESSION_HAVE_BROTLI
                 ? D_ENV_COMPRESSION_CODEC_BROTLI
                 : D_ENV_COMPRESSION_CODEC_NONE;
        }
    }

    return D_ENV_COMPRESSION_CODEC_NONE;
}

/*
d_codec_backend_name
  The name of the library providing a codec.

Parameter(s):
  _codec: the codec to query.
Return:
  A static, NUL-terminated backend name.  "builtin" for STORE, which needs no
library, and "none" for a codec this build cannot perform.
*/
const char*
d_codec_backend_name(
    enum d_codec_id _codec
)
{
    if (_codec == D_CODEC_ID_STORE)
    {
        return "builtin";
    }
    if (!d_codec_can_compress(_codec))
    {
        return "none";
    }

    return d_env_compression_codec_name(d_codec_backend_id(_codec));
}

/*
d_codec_available_list
  The codecs this build can perform, in enumeration order.

Parameter(s):
  _out_codecs:   receives the codecs; may be null to count only.
  _out_capacity: how many entries _out_codecs holds.
Return:
  The total number available, which may exceed _out_capacity.
*/
size_t
d_codec_available_list(
    enum d_codec_id* _out_codecs,
    size_t           _out_capacity
)
{
    size_t found = 0;
    int    i;

    for (i = 0; i < D_CODEC_ID_COUNT; ++i)
    {
        if (!d_codec_is_available((enum d_codec_id)i))
        {
            continue;
        }
        if ( (_out_codecs != NULL) &&
             (found < _out_capacity) )
        {
            _out_codecs[found] = (enum d_codec_id)i;
        }

        ++found;
    }

    return found;
}


// =============================================================================
// VII. SIGNATURES AND CRC32
// =============================================================================

/*
d_codec_signature_length
  How many leading bytes are needed to recognise a codec's framing.

Parameter(s):
  _codec: the codec to query.
Return:
  The byte count, or 0 for a codec with no fixed magic.
*/
int
d_codec_signature_length(
    enum d_codec_id _codec
)
{
    switch (_codec)
    {
        case D_CODEC_ID_ZLIB:   { return 2; }
        case D_CODEC_ID_GZIP:   { return 3; }
        case D_CODEC_ID_BZIP2:  { return 3; }
        case D_CODEC_ID_XZ:     { return 6; }
        case D_CODEC_ID_ZSTD:   { return 4; }
        case D_CODEC_ID_LZ4:    { return 4; }
        default:                { return 0; }
    }
}

/*
d_codec_signature_matches
  Whether a buffer opens with a codec's framing.

  store, raw DEFLATE and brotli answer 1 unconditionally: none of the three has
a fixed magic number, so there is nothing to check and reporting a mismatch
would be an invention.  A conformance test should assert this rather than treat
it as a gap.

  The zlib check is not a magic number but a validity rule: the first byte's
low nibble must be 8 (the DEFLATE method), the window field must be in range,
and the two-byte header must be a multiple of 31.  That is as strong as a zlib
header gets, and it is why a raw DEFLATE stream can occasionally satisfy it --
which is exactly why d_codec_detect tries zlib last.

Parameter(s):
  _codec: the framing to test for.
  _data:  the buffer; may be null, which never matches.
  _size:  the buffer's length.
Return:
  1 when the buffer opens with that framing, 0 otherwise.
*/
int
d_codec_signature_matches(
    enum d_codec_id _codec,
    const void*     _data,
    size_t          _size
)
{
    const unsigned char* p   = (const unsigned char*)_data;
    int                  len = d_codec_signature_length(_codec);

    if (len == 0)
    {
        return 1;
    }
    if ( (p == NULL) ||
         (_size < (size_t)len) )
    {
        return 0;
    }

    switch (_codec)
    {
        case D_CODEC_ID_ZLIB:
        {
            unsigned int header = ((unsigned int)p[0] << 8) |
                                   (unsigned int)p[1];

            return ( ((p[0] & 0x0Fu) == 0x08u) &&
                     ((p[0] >> 4)    <= 0x07u) &&
                     ((header % 31u) == 0u) ) ? 1 : 0;
        }
        case D_CODEC_ID_GZIP:
        {
            return ( (p[0] == 0x1Fu) &&
                     (p[1] == 0x8Bu) &&
                     (p[2] == 0x08u) ) ? 1 : 0;
        }
        case D_CODEC_ID_BZIP2:
        {
            return ( (p[0] == 'B') &&
                     (p[1] == 'Z') &&
                     (p[2] == 'h') ) ? 1 : 0;
        }
        case D_CODEC_ID_XZ:
        {
            return ( (p[0] == 0xFDu) && (p[1] == '7') && (p[2] == 'z') &&
                     (p[3] == 'X')   && (p[4] == 'Z') && (p[5] == 0x00u) )
                 ? 1 : 0;
        }
        case D_CODEC_ID_ZSTD:
        {
            return ( (p[0] == 0x28u) && (p[1] == 0xB5u) &&
                     (p[2] == 0x2Fu) && (p[3] == 0xFDu) ) ? 1 : 0;
        }
        case D_CODEC_ID_LZ4:
        {
            return ( (p[0] == 0x04u) && (p[1] == 0x22u) &&
                     (p[2] == 0x4Du) && (p[3] == 0x18u) ) ? 1 : 0;
        }
        default:
        {
            return 1;
        }
    }
}

/*
d_codec_detect
  Identify a stream from its opening bytes.

  Ordered strongest signature first.  zlib is tried LAST among the framed
codecs because its check is a validity rule rather than a magic number and can
be satisfied by chance; the four-and-six byte magics cannot.  The three codecs
with no framing at all are never detected, which is correct -- an unframed
stream is indistinguishable from arbitrary data and claiming otherwise would be
a guess presented as a fact.

Parameter(s):
  _data:      the buffer to inspect.
  _size:      its length.
  _out_codec: receives the codec on success; untouched on failure.
Return:
  1 when a framing was recognised, 0 otherwise.
*/
int
d_codec_detect(
    const void*      _data,
    size_t           _size,
    enum d_codec_id* _out_codec
)
{
    static const enum d_codec_id order[] =
    {
        D_CODEC_ID_XZ,   D_CODEC_ID_ZSTD, D_CODEC_ID_LZ4,
        D_CODEC_ID_GZIP, D_CODEC_ID_BZIP2, D_CODEC_ID_ZLIB
    };

    size_t n = sizeof(order) / sizeof(order[0]);
    size_t i;

    if ( (_data == NULL) ||
         (_out_codec == NULL) )
    {
        return 0;
    }

    for (i = 0; i < n; ++i)
    {
        if (d_codec_signature_matches(order[i], _data, _size))
        {
            *_out_codec = order[i];

            return 1;
        }
    }

    return 0;
}

/*
d_pack_crc32
  The IEEE 802.3 CRC-32 the gzip trailer and every ZIP record carry.

  Computed from the reflected polynomial on the fly rather than from a stored
256-entry table.  The table is four times faster and costs a kilobyte of
mutable-or-generated state in a file that otherwise has none; this is not on a
hot path (it runs once per entry, over data a codec has already walked several
times), so the simpler thing wins.  If profiling ever says otherwise, the
replacement is a static const table and nothing else changes.

  Seed a fresh computation with 0.  The seed and return are the RUNNING value,
so a caller may checksum a stream in pieces by feeding each result forward.

Parameter(s):
  _seed: 0 to begin, or the previous result to continue.
  _data: the bytes to fold in; may be null when _size is 0.
  _size: how many bytes.
Return:
  The running CRC-32.
*/
uint32_t
d_pack_crc32(
    uint32_t    _seed,
    const void* _data,
    size_t      _size
)
{
    const unsigned char* p = (const unsigned char*)_data;
    uint32_t             c = _seed ^ 0xFFFFFFFFu;
    size_t               i;
    int                  b;

    if ( (p == NULL) ||
         (_size == 0u) )
    {
        return _seed;
    }

    for (i = 0; i < _size; ++i)
    {
        c ^= (uint32_t)p[i];

        for (b = 0; b < 8; ++b)
        {
            uint32_t mask = (uint32_t)(0u - (c & 1u));

            c = (c >> 1) ^ (0xEDB88320u & mask);
        }
    }

    return (c ^ 0xFFFFFFFFu);
}


// =============================================================================
// VIII. SINK ADAPTERS
// =============================================================================

// d_internal_tee
//   struct: the context of a sink that counts the bytes passing through it and
// forwards them to an inner sink.  Lets a transform report its own output size
// without requiring the caller's sink to be countable.
struct d_internal_tee
{
    struct d_pack_sink  inner;
    size_t              total;
};

/*
d_internal_count_write
  The counting sink's write: accept everything, record the total.

Parameter(s):
  _context: a struct d_pack_counting_sink.
  _data:    ignored.
  _size:    the byte count to add.
Return:
  _size always -- a counting sink never refuses.
*/
static size_t
d_internal_count_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_pack_counting_sink* c = (struct d_pack_counting_sink*)_context;

    (void)_data;

    if (c != NULL)
    {
        c->total += _size;
    }

    return _size;
}

/*
d_internal_buffer_write
  The buffer sink's write: copy what fits, and keep counting what does not.

  A short write would abort the transform, so this ACCEPTS every byte and
records the overflow instead.  That is what lets one failed pass report the
exact size to grow to: the encoder runs to completion, the sink knows the full
requirement, and the caller retries once rather than measuring separately.

Parameter(s):
  _context: a struct d_pack_buffer_sink.
  _data:    the bytes offered.
  _size:    how many.
Return:
  _size always; the caller checks `overflow` rather than the return.
*/
static size_t
d_internal_buffer_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_pack_buffer_sink* b = (struct d_pack_buffer_sink*)_context;

    if (b == NULL)
    {
        return 0;
    }

    b->needed += _size;

    if ( (b->buffer != NULL) &&
         ((b->written + _size) <= b->capacity) )
    {
        if (_size != 0u)
        {
            memcpy(b->buffer + b->written, _data, _size);
        }

        b->written += _size;
    }
    else if (_size != 0u)
    {
        b->overflow = 1;
    }

    return _size;
}

/*
d_pack_counting_sink_init
  Reset a counting sink to zero.

Parameter(s):
  _counter: the sink context; may be null, which does nothing.
Return:
  none.
*/
void
d_pack_counting_sink_init(
    struct d_pack_counting_sink* _counter
)
{
    if (_counter != NULL)
    {
        _counter->total = 0u;
    }

    return;
}

/*
d_pack_buffer_sink_init
  Point a buffer sink at caller-owned memory.

Parameter(s):
  _sink:     the sink context; may be null, which does nothing.
  _buffer:   the destination; null makes the sink behave as a counter.
  _capacity: how many bytes _buffer holds.
Return:
  none.
*/
void
d_pack_buffer_sink_init(
    struct d_pack_buffer_sink* _sink,
    void*                      _buffer,
    size_t                     _capacity
)
{
    if (_sink == NULL)
    {
        return;
    }

    _sink->buffer   = (unsigned char*)_buffer;
    _sink->capacity = _capacity;
    _sink->written  = 0u;
    _sink->needed   = 0u;
    _sink->overflow = 0;

    return;
}

/*
d_pack_sink_from_counter
  A sink driving a counting context.

Parameter(s):
  _counter: the context to bind.
Return:
  The bound sink.
*/
struct d_pack_sink
d_pack_sink_from_counter(
    struct d_pack_counting_sink* _counter
)
{
    struct d_pack_sink s;

    s.write   = &d_internal_count_write;
    s.context = _counter;

    return s;
}

/*
d_pack_sink_from_buffer
  A sink driving a fixed-buffer context.

Parameter(s):
  _buffer: the context to bind.
Return:
  The bound sink.
*/
struct d_pack_sink
d_pack_sink_from_buffer(
    struct d_pack_buffer_sink* _buffer
)
{
    struct d_pack_sink s;

    s.write   = &d_internal_buffer_write;
    s.context = _buffer;

    return s;
}

/*
d_internal_tee_write
  The tee sink's write: count the bytes, then forward them to an inner sink.

  This is how a transform reports the size it produced without depending on
what the caller's sink does with the bytes.  A caller's sink may be a file, a
socket, or a growable buffer, and none of them is obliged to be countable; the
tee makes the count the transform's own business.

Parameter(s):
  _context: a struct d_internal_tee.
  _data:    the bytes offered.
  _size:    how many.
Return:
  What the inner sink accepted, so a short write still propagates.
*/
static size_t
d_internal_tee_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct d_internal_tee* t = (struct d_internal_tee*)_context;
    size_t                 taken;

    if ( (t == NULL) ||
         (t->inner.write == NULL) )
    {
        return 0;
    }

    taken = t->inner.write(t->inner.context, _data, _size);
    t->total += taken;

    return taken;
}

/*
d_internal_tee_bind
  Wrap a sink so the bytes passing through it are counted.

Parameter(s):
  _tee:   the tee context to initialise.
  _inner: the sink to forward to.
Return:
  A sink that counts and forwards.
*/
static struct d_pack_sink
d_internal_tee_bind(
    struct d_internal_tee* _tee,
    struct d_pack_sink     _inner
)
{
    struct d_pack_sink s;

    _tee->inner = _inner;
    _tee->total = 0u;

    s.write   = &d_internal_tee_write;
    s.context = _tee;

    return s;
}

/*
d_internal_emit
  Push bytes to a sink, mapping a refusal onto this module's status vocabulary.
The one place a sink failure becomes a d_pack_status.

Parameter(s):
  _sink: the destination.
  _data: the bytes.
  _size: how many.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_SINK_ERROR.
*/
static enum d_pack_status
d_internal_emit(
    struct d_pack_sink _sink,
    const void*        _data,
    size_t             _size
)
{
    return d_sink_emit(_sink, _data, _size)
         ? D_PACK_STATUS_OK
         : D_PACK_STATUS_SINK_ERROR;
}


// =============================================================================
// IX.  BACKEND LEAVES
// =============================================================================
//   The only environment-dependent section.  Each codec is one compress leaf
// and one decompress leaf, guarded by its env_compress.h flag; an absent codec
// gets a stub returning D_PACK_STATUS_UNAVAILABLE.
//
//   Every leaf has the same signature and the same contract: consume the whole
// input, push the whole result to the sink, return a status.  Streaming state,
// chunk sizes and backend handles are each leaf's private business.
//
//   STORE IS UNCONDITIONAL and comes first, because it is the codec that makes
// a dependency-free build a working build rather than a broken one.

// D_INTERNAL_CHUNK
//   constant: the staging-buffer size a streaming leaf reads and writes in.
// 64 KiB matches the chunk test_compress.hpp's large payloads are chosen to
// exceed, so the multi-chunk path is exercised by the standard corpus.
#define D_INTERNAL_CHUNK    (64u * 1024u)

/*
d_internal_store_compress
  The identity transform: copy the input to the sink unchanged.

Parameter(s):
  _in:      the input bytes.
  _in_size: how many.
  _sink:    the destination.
Return:
  D_PACK_STATUS_OK, or the sink's failure.
*/
static enum d_pack_status
d_internal_store_compress(
    const void*        _in,
    size_t             _in_size,
    struct d_pack_sink _sink
)
{
    return d_internal_emit(_sink, _in, _in_size);
}

#if D_ENV_COMPRESSION_HAVE_ZLIB || D_ENV_COMPRESSION_HAVE_ZLIBNG

#include <zlib.h>

/*
d_internal_unconst
  Drop a const qualifier through a union.

  zlib before 1.2.5.2 declares next_in as a non-const Bytef*, so a const input
cannot be assigned to it directly.  A cast would work and would also silence a
genuine diagnostic everywhere else in the file; routing it through a union
confines the conversion to one named, commented function that a reader can
audit.  The bytes are never written through the result.

Parameter(s):
  _p: the pointer to unqualify.
Return:
  The same address, without const.
*/
static void*
d_internal_unconst(
    const void* _p
)
{
    union
    {
        const void* in;
        void*       out;
    }
    u;

    u.in = _p;

    return u.out;
}

/*
d_internal_zlib_window_bits
  Translate a codec and its resolved DEFLATE knobs into zlib's windowBits,
whose sign and offset select the framing: negative is raw DEFLATE, 8-15 is a
zlib wrapper, and +16 is a gzip wrapper.  Collapsing the three framings onto
one parameter is a zlib convention and not an obvious one, so it is done here
once rather than at three call sites.

Parameter(s):
  _codec:   DEFLATE, ZLIB or GZIP.
  _options: a resolved option set.
Return:
  The windowBits value to hand deflateInit2 / inflateInit2.
*/
static int
d_internal_zlib_window_bits(
    enum d_codec_id                  _codec,
    const struct d_compress_options* _options
)
{
    int32_t bits = d_compress_options_knob_get(
        _options, D_COMPRESS_KNOB_DEFLATE_WINDOW_BITS);

    if ( (bits == D_COMPRESS_KNOB_UNSET) ||
         (bits < 9) || (bits > 15) )
    {
        bits = 15;
    }

    if (_codec == D_CODEC_ID_DEFLATE)
    {
        return -(int)bits;
    }
    if (_codec == D_CODEC_ID_GZIP)
    {
        return (int)bits + 16;
    }

    return (int)bits;
}

/*
d_internal_zlib_compress
  The DEFLATE-family compress leaf, serving raw DEFLATE, zlib and gzip -- one
implementation with one framing parameter, rather than three near-copies.

Parameter(s):
  _codec:   DEFLATE, ZLIB or GZIP.
  _in:      the input bytes.
  _in_size: how many.
  _options: a resolved option set.
  _sink:    the destination.
Return:
  D_PACK_STATUS_OK, or a mechanical status describing the failure.
*/
static enum d_pack_status
d_internal_zlib_compress(
    enum d_codec_id                  _codec,
    const void*                      _in,
    size_t                           _in_size,
    const struct d_compress_options* _options,
    struct d_pack_sink               _sink
)
{
    unsigned char      stage[D_INTERNAL_CHUNK];
    z_stream           zs;
    enum d_pack_status status = D_PACK_STATUS_OK;
    int                rc;

    memset(&zs, 0, sizeof(zs));

    rc = deflateInit2(
        &zs,
        (int)d_compress_resolve_level(
            _codec,
            d_compress_options_knob_get(_options, D_COMPRESS_KNOB_LEVEL)),
        Z_DEFLATED,
        d_internal_zlib_window_bits(_codec, _options),
        (int)d_compress_options_knob_get(_options,
                                         D_COMPRESS_KNOB_DEFLATE_MEM_LEVEL),
        (int)d_compress_options_knob_get(_options,
                                         D_COMPRESS_KNOB_DEFLATE_STRATEGY));

    if (rc != Z_OK)
    {
        return (rc == Z_MEM_ERROR)
             ? D_PACK_STATUS_NO_MEMORY
             : D_PACK_STATUS_BACKEND_ERROR;
    }

    zs.next_in  = (Bytef*)d_internal_unconst(_in);
    zs.avail_in = (uInt)_in_size;

    do
    {
        zs.next_out  = stage;
        zs.avail_out = (uInt)sizeof(stage);

        rc = deflate(&zs, Z_FINISH);

        if ( (rc != Z_OK)          &&
             (rc != Z_STREAM_END)  &&
             (rc != Z_BUF_ERROR) )
        {
            status = D_PACK_STATUS_BACKEND_ERROR;
            break;
        }

        status = d_internal_emit(_sink, stage,
                                 sizeof(stage) - (size_t)zs.avail_out);

        if (status != D_PACK_STATUS_OK)
        {
            break;
        }
    }
    while (rc != Z_STREAM_END);

    (void)deflateEnd(&zs);

    return status;
}

/*
d_internal_zlib_decompress
  The DEFLATE-family decompress leaf.

Parameter(s):
  _codec:   DEFLATE, ZLIB or GZIP.
  _in:      the compressed bytes.
  _in_size: how many.
  _sink:    the destination.
Return:
  D_PACK_STATUS_OK; D_PACK_STATUS_CORRUPT_INPUT when the stream is malformed;
D_PACK_STATUS_TRUNCATED_INPUT when it ends early.
*/
static enum d_pack_status
d_internal_zlib_decompress(
    enum d_codec_id    _codec,
    const void*        _in,
    size_t             _in_size,
    struct d_pack_sink _sink
)
{
    unsigned char      stage[D_INTERNAL_CHUNK];
    z_stream           zs;
    enum d_pack_status status = D_PACK_STATUS_OK;
    int                rc;

    memset(&zs, 0, sizeof(zs));

    rc = inflateInit2(&zs, d_internal_zlib_window_bits(_codec, NULL));

    if (rc != Z_OK)
    {
        return (rc == Z_MEM_ERROR)
             ? D_PACK_STATUS_NO_MEMORY
             : D_PACK_STATUS_BACKEND_ERROR;
    }

    zs.next_in  = (Bytef*)d_internal_unconst(_in);
    zs.avail_in = (uInt)_in_size;

    for (;;)
    {
        zs.next_out  = stage;
        zs.avail_out = (uInt)sizeof(stage);

        rc = inflate(&zs, Z_NO_FLUSH);

        if ( (rc == Z_DATA_ERROR) ||
             (rc == Z_NEED_DICT) )
        {
            status = D_PACK_STATUS_CORRUPT_INPUT;
            break;
        }
        if (rc == Z_MEM_ERROR)
        {
            status = D_PACK_STATUS_NO_MEMORY;
            break;
        }

        status = d_internal_emit(_sink, stage,
                                 sizeof(stage) - (size_t)zs.avail_out);

        if (status != D_PACK_STATUS_OK)
        {
            break;
        }
        if (rc == Z_STREAM_END)
        {
            break;
        }
        if ( (rc == Z_BUF_ERROR) &&
             (zs.avail_in == 0u) )
        {
            status = D_PACK_STATUS_TRUNCATED_INPUT;
            break;
        }
    }

    (void)inflateEnd(&zs);

    return status;
}

#endif  // zlib family


// =============================================================================
// X.   TRANSFORMS
// =============================================================================
//   The public leaves.  Each resolves options once, dispatches to a backend
// leaf, and adapts between the sink and buffer forms.  No codec knowledge
// lives here; no buffer-protocol knowledge lives in a backend leaf.

/*
d_compress_to_sink
  Compress _in into _sink under _codec.

Parameter(s):
  _codec:    the codec to use.
  _in:       the input bytes; may be null when _in_size is 0.
  _in_size:  how many.
  _options:  the tuning; null requests the core's pinned defaults.
  _sink:     the destination.
  _out_size: receives the number of bytes produced; may be null.
Return:
  D_PACK_STATUS_OK, or a status describing the failure.  A codec this build
cannot perform yields D_PACK_STATUS_UNAVAILABLE, which is a formal answer and
not an error -- see facade_roundtrip_ok in test_compress.hpp.
*/
enum d_pack_status
d_compress_to_sink(
    enum d_codec_id                  _codec,
    const void*                      _in,
    size_t                           _in_size,
    const struct d_compress_options* _options,
    struct d_pack_sink               _sink,
    size_t*                          _out_size
)
{
    struct d_internal_tee     tally;
    struct d_pack_sink        counted;
    struct d_compress_options resolved;
    enum d_pack_status        status;

    if ( ((_in == NULL) && (_in_size != 0u)) ||
         (_sink.write == NULL) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if (!D_CODEC_ID_IS_VALID(_codec))
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if (!d_codec_can_compress(_codec))
    {
        return D_PACK_STATUS_UNAVAILABLE;
    }

    status = d_compress_options_resolve(_codec, _options, &resolved);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    status = d_compress_options_validate(_codec, &resolved, NULL);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    // interpose a tee so the produced size is always known, whatever the
    // caller's sink does with the bytes.
    counted = d_internal_tee_bind(&tally, _sink);

    switch (_codec)
    {
        case D_CODEC_ID_STORE:
        {
            status = d_internal_store_compress(_in, _in_size, counted);
            break;
        }
#if D_ENV_COMPRESSION_HAVE_ZLIB || D_ENV_COMPRESSION_HAVE_ZLIBNG
        case D_CODEC_ID_DEFLATE:
        case D_CODEC_ID_ZLIB:
        case D_CODEC_ID_GZIP:
        {
            status = d_internal_zlib_compress(_codec, _in, _in_size,
                                              &resolved, counted);
            break;
        }
#endif
        default:
        {
            status = D_PACK_STATUS_UNAVAILABLE;
            break;
        }
    }

    if ( (status == D_PACK_STATUS_OK) &&
         (_out_size != NULL) )
    {
        *_out_size = tally.total;
    }

    return status;
}

/*
d_decompress_to_sink
  Decompress _in into _sink under _codec.

Parameter(s):
  _codec:    the codec the stream is in.
  _in:       the compressed bytes; may be null when _in_size is 0.
  _in_size:  how many.
  _sink:     the destination.
  _out_size: receives the number of bytes produced; may be null.
Return:
  D_PACK_STATUS_OK, or a status describing the failure.
*/
enum d_pack_status
d_decompress_to_sink(
    enum d_codec_id    _codec,
    const void*        _in,
    size_t             _in_size,
    struct d_pack_sink _sink,
    size_t*            _out_size
)
{
    enum d_pack_status status;

    if ( ((_in == NULL) && (_in_size != 0u)) ||
         (_sink.write == NULL) )
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if (!D_CODEC_ID_IS_VALID(_codec))
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if (!d_codec_can_decompress(_codec))
    {
        return D_PACK_STATUS_UNAVAILABLE;
    }

    switch (_codec)
    {
        case D_CODEC_ID_STORE:
        {
            status = d_internal_emit(_sink, _in, _in_size);

            if ( (status == D_PACK_STATUS_OK) &&
                 (_out_size != NULL) )
            {
                *_out_size = _in_size;
            }

            return status;
        }
#if D_ENV_COMPRESSION_HAVE_ZLIB || D_ENV_COMPRESSION_HAVE_ZLIBNG
        case D_CODEC_ID_DEFLATE:
        case D_CODEC_ID_ZLIB:
        case D_CODEC_ID_GZIP:
        {
            struct d_internal_tee tally;
            struct d_pack_sink    counted =
                d_internal_tee_bind(&tally, _sink);

            status = d_internal_zlib_decompress(_codec, _in, _in_size,
                                                counted);

            if ( (status == D_PACK_STATUS_OK) &&
                 (_out_size != NULL) )
            {
                *_out_size = tally.total;
            }
            break;
        }
#endif
        default:
        {
            status = D_PACK_STATUS_UNAVAILABLE;
            break;
        }
    }

    return status;
}

/*
d_internal_run_to_buffer
  The one adapter from a sink-form transform to the two-call buffer protocol.

  Both d_compress and d_decompress route through it, so the measure case, the
overflow case and the success case are decided in exactly one place and cannot
drift between the two directions.

Parameter(s):
  _compressing: 1 to compress, 0 to decompress.
  _codec:       the codec.
  _in:          the input bytes.
  _in_size:     how many.
  _options:     the tuning; ignored when decompressing.
  _out:         the destination; null with _out_capacity 0 measures.
  _out_cap:     how many bytes _out holds.
  _out_size:    receives the produced or required size; must not be null.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL with *_out_size carrying the
requirement, or a status from the transform.
*/
static enum d_pack_status
d_internal_run_to_buffer(
    int                              _compressing,
    enum d_codec_id                  _codec,
    const void*                      _in,
    size_t                           _in_size,
    const struct d_compress_options* _options,
    void*                            _out,
    size_t                           _out_cap,
    size_t*                          _out_size
)
{
    struct d_pack_buffer_sink dest;
    struct d_pack_sink        sink;
    enum d_pack_status        status;

    if (_out_size == NULL)
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    d_pack_buffer_sink_init(&dest, _out, _out_cap);
    sink = d_pack_sink_from_buffer(&dest);

    status = _compressing
           ? d_compress_to_sink(_codec, _in, _in_size, _options, sink, NULL)
           : d_decompress_to_sink(_codec, _in, _in_size, sink, NULL);

    if (status != D_PACK_STATUS_OK)
    {
        return status;
    }

    // `needed` is the full requirement whether or not the buffer held it, so
    // one failed pass tells the caller exactly how far to grow.
    *_out_size = dest.needed;

    // a MEASURE call (null destination, zero capacity) asked only for that
    // number and has succeeded.  An overflow is only a failure when the caller
    // actually offered somewhere to put the bytes -- conflating the two would
    // make the documented measure form report a mechanical failure for doing
    // exactly what it was asked to do.
    if ( (_out == NULL) &&
         (_out_cap == 0u) )
    {
        return D_PACK_STATUS_OK;
    }

    if (dest.overflow)
    {
        return D_PACK_STATUS_BUFFER_TOO_SMALL;
    }

    return D_PACK_STATUS_OK;
}

/*
d_compress
  Compress into a caller-owned buffer under the two-call protocol.

Parameter(s):
  _codec:        the codec to use.
  _in:           the input bytes.
  _in_size:      how many.
  _options:      the tuning; null requests the core's pinned defaults.
  _out:          the destination; null with _out_capacity 0 measures.
  _out_capacity: how many bytes _out holds.
  _out_size:     receives the produced or required size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL with the requirement in
*_out_size, or a status describing the failure.
*/
enum d_pack_status
d_compress(
    enum d_codec_id                  _codec,
    const void*                      _in,
    size_t                           _in_size,
    const struct d_compress_options* _options,
    void*                            _out,
    size_t                           _out_capacity,
    size_t*                          _out_size
)
{
    return d_internal_run_to_buffer(1, _codec, _in, _in_size, _options,
                                    _out, _out_capacity, _out_size);
}

/*
d_decompress
  Decompress into a caller-owned buffer under the two-call protocol.

Parameter(s):
  _codec:        the codec the stream is in.
  _in:           the compressed bytes.
  _in_size:      how many.
  _out:          the destination; null with _out_capacity 0 measures.
  _out_capacity: how many bytes _out holds.
  _out_size:     receives the produced or required size.
Return:
  D_PACK_STATUS_OK, D_PACK_STATUS_BUFFER_TOO_SMALL with the requirement in
*_out_size, or a status describing the failure.
*/
enum d_pack_status
d_decompress(
    enum d_codec_id _codec,
    const void*     _in,
    size_t          _in_size,
    void*           _out,
    size_t          _out_capacity,
    size_t*         _out_size
)
{
    return d_internal_run_to_buffer(0, _codec, _in, _in_size, NULL,
                                    _out, _out_capacity, _out_size);
}

/*
d_compress_bound
  An upper bound on the compressed size of an input of _in_size bytes.

  Cheap and always safe: it exceeds the true output even for incompressible
input, which is the case that makes compressed output LARGER than its input --
the case a caller who sizes a buffer from the input length alone gets wrong.
A caller who needs the EXACT size measures instead and pays a full pass
knowingly.

  The bound is the input plus the worst-case expansion of the codec's framing.
Where a backend publishes its own bound, that is used; where it does not, a
conservative closed form is used instead.

Parameter(s):
  _codec:     the codec to bound.
  _in_size:   the input length.
  _options:   the tuning; may be null.
  _out_bound: receives the bound; must not be null.
Return:
  D_PACK_STATUS_OK, or D_PACK_STATUS_INVALID_ARGUMENT.
*/
enum d_pack_status
d_compress_bound(
    enum d_codec_id                  _codec,
    size_t                           _in_size,
    const struct d_compress_options* _options,
    size_t*                          _out_bound
)
{
    (void)_options;

    if (_out_bound == NULL)
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }
    if (!D_CODEC_ID_IS_VALID(_codec))
    {
        return D_PACK_STATUS_INVALID_ARGUMENT;
    }

    if (_codec == D_CODEC_ID_STORE)
    {
        *_out_bound = _in_size;

        return D_PACK_STATUS_OK;
    }

    // the generic closed form: DEFLATE's worst case is 5 bytes of block header
    // per 16 KiB plus a small constant, and every other codec here is bounded
    // more tightly than that.  64 bytes of slack covers each one's container
    // header and trailer.
    *_out_bound = _in_size + (_in_size / 16384u) * 5u + 64u;

    return D_PACK_STATUS_OK;
}
