/******************************************************************************
* djinterp [test]                                    test_compress_options.hpp
*
*   A common testing module for the compression OPTION vocabulary.  This is the
* data-layer companion to test_compress.hpp: where that header verifies what a
* codec DOES (payloads, stream framing, round-trips), this one answers questions
* about the option values themselves - are these two option sets the same, is
* this one still pristine, and exactly which knobs moved?
*
*   compress_options is plain data with no operator==, and it nests six
* per-codec blocks totalling forty-nine advanced knobs on top of the generic
* `level`.  Any suite that hands an option set across a boundary and wants to
* prove it arrived intact otherwise re-derives that fifty-field walk by hand.
* It is collected once here.
*
*   It offers three things an option-passing test repeatedly wants:
*   - COMPARISON    deep equality at every level - one comparator per codec
*                   block, plus compress_options_equal over the whole surface -
*                   so a caller can check a single block or the entire set.
*   - PRISTINE      compress_options_are_default, the "nothing has been touched"
*                   predicate, for asserting that a call left the tuning alone.
*   - DIFF          describe_compress_option_diff, a readable list naming the
*                   fields that differ, for a failing check's message.  Naming
*                   is FIELD-level here ("zstd.window_log"), which is the point
*                   of a dedicated compress-options header; the archive-level
*                   view in test_archive_options.hpp deliberately reports these
*                   same blocks coarsely, since a call site tuning a container
*                   rarely cares which of nineteen zstd knobs moved.
*
*   LAYERING:
*   This header does NOT include test_compress.hpp, and does not need to.  Its
* only dependency is the option vocabulary it compares, so a module that merely
* FORWARDS options - a bundler, a settings round-trip, a config differ - can
* include it without dragging in the compress facade or linking compress.cpp.
* test_archive_options.hpp layers on this header for exactly that reason.  A
* suite that wants both concerns includes both headers.
*
*   PORTABILITY:
*   C++98 - C++23, matching the vocabulary it compares.  Header-only, no
* third-party include, and no link dependency of any kind.
*
*
* TABLE OF CONTENTS
* =================
* I.    PER-CODEC BLOCK COMPARISON
* II.   AGGREGATE COMPARISON
* III.  PRISTINE-DEFAULT PREDICATES
* IV.   DIFF DESCRIPTION
* V.    OPTION BUILDERS
*
*
* path:      /inc/djinterp/test/test_compress_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.22
******************************************************************************/

#ifndef DJINTERP_TEST_COMPRESS_OPTIONS_
#define DJINTERP_TEST_COMPRESS_OPTIONS_ 1

#ifndef __cplusplus
    #error "test_compress_options.hpp requires C++ compilation"
#endif

// std
#include <string>
// djinterp
#include "../core/djinterp.hpp"                 // namespace macros
#include "../core/util/compress_options.hpp"    // the vocabulary under comparison


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   PER-CODEC BLOCK COMPARISON                           ///
///////////////////////////////////////////////////////////////////////////////
//   One comparator per block.  Keeping them separate keeps each function short
// enough to read at a glance, and lets a caller assert on a single codec's
// tuning without pulling in the other five.

// deflate_options_equal
//   function: true iff every field of the DEFLATE tuning block matches.
inline bool
deflate_options_equal(
    const deflate_options& _a,
    const deflate_options& _b
)
{
    return ( (_a.window_bits == _b.window_bits) &&
             (_a.mem_level   == _b.mem_level)   &&
             (_a.strategy    == _b.strategy) );
}

// bzip2_options_equal
//   function: true iff every field of the bzip2 tuning block matches.
inline bool
bzip2_options_equal(
    const bzip2_options& _a,
    const bzip2_options& _b
)
{
    return ( (_a.block_size_100k  == _b.block_size_100k)  &&
             (_a.work_factor      == _b.work_factor)      &&
             (_a.verbosity        == _b.verbosity)        &&
             (_a.small_decompress == _b.small_decompress) );
}

// lzma_options_equal
//   function: true iff every field of the lzma / xz tuning block matches.
inline bool
lzma_options_equal(
    const lzma_options& _a,
    const lzma_options& _b
)
{
    return ( (_a.extreme   == _b.extreme)   &&
             (_a.check     == _b.check)     &&
             (_a.dict_size == _b.dict_size) &&
             (_a.lc        == _b.lc)        &&
             (_a.lp        == _b.lp)        &&
             (_a.pb        == _b.pb)        &&
             (_a.mode      == _b.mode)      &&
             (_a.nice_len  == _b.nice_len)  &&
             (_a.mf        == _b.mf)        &&
             (_a.depth     == _b.depth)     &&
             (_a.threads   == _b.threads) );
}

// zstd_options_equal
//   function: true iff every field of the Zstandard tuning block matches.
inline bool
zstd_options_equal(
    const zstd_options& _a,
    const zstd_options& _b
)
{
    return ( (_a.level                  == _b.level)                  &&
             (_a.window_log             == _b.window_log)             &&
             (_a.hash_log               == _b.hash_log)               &&
             (_a.chain_log              == _b.chain_log)              &&
             (_a.search_log             == _b.search_log)             &&
             (_a.min_match              == _b.min_match)              &&
             (_a.target_length          == _b.target_length)          &&
             (_a.strategy               == _b.strategy)               &&
             (_a.long_distance_matching == _b.long_distance_matching) &&
             (_a.ldm_hash_log           == _b.ldm_hash_log)           &&
             (_a.ldm_min_match          == _b.ldm_min_match)          &&
             (_a.ldm_bucket_size_log    == _b.ldm_bucket_size_log)    &&
             (_a.ldm_hash_rate_log      == _b.ldm_hash_rate_log)      &&
             (_a.content_size_flag      == _b.content_size_flag)      &&
             (_a.checksum_flag          == _b.checksum_flag)          &&
             (_a.dict_id_flag           == _b.dict_id_flag)           &&
             (_a.workers                == _b.workers)                &&
             (_a.job_size               == _b.job_size)               &&
             (_a.overlap_log            == _b.overlap_log) );
}

// lz4_options_equal
//   function: true iff every field of the LZ4 frame tuning block matches.
inline bool
lz4_options_equal(
    const lz4_options& _a,
    const lz4_options& _b
)
{
    return ( (_a.level              == _b.level)              &&
             (_a.block_size         == _b.block_size)         &&
             (_a.block_mode         == _b.block_mode)         &&
             (_a.content_checksum   == _b.content_checksum)   &&
             (_a.block_checksum     == _b.block_checksum)     &&
             (_a.store_content_size == _b.store_content_size) &&
             (_a.favor_dec_speed    == _b.favor_dec_speed) );
}

// brotli_options_equal
//   function: true iff every field of the Brotli tuning block matches.
inline bool
brotli_options_equal(
    const brotli_options& _a,
    const brotli_options& _b
)
{
    return ( (_a.quality      == _b.quality)      &&
             (_a.window_bits  == _b.window_bits)  &&
             (_a.block_bits   == _b.block_bits)   &&
             (_a.mode         == _b.mode)         &&
             (_a.large_window == _b.large_window) );
}


///////////////////////////////////////////////////////////////////////////////
///                II.  AGGREGATE COMPARISON                                 ///
///////////////////////////////////////////////////////////////////////////////

// compress_options_equal
//   function: true iff two option sets are field-for-field identical - the
// generic level plus all six per-codec blocks.
inline bool
compress_options_equal(
    const compress_options& _a,
    const compress_options& _b
)
{
    return ( (_a.level == _b.level)                        &&
             deflate_options_equal(_a.deflate, _b.deflate) &&
             bzip2_options_equal(_a.bzip2, _b.bzip2)       &&
             lzma_options_equal(_a.lzma, _b.lzma)          &&
             zstd_options_equal(_a.zstd, _b.zstd)          &&
             lz4_options_equal(_a.lz4, _b.lz4)             &&
             brotli_options_equal(_a.brotli, _b.brotli) );
}


///////////////////////////////////////////////////////////////////////////////
///                III. PRISTINE-DEFAULT PREDICATES                          ///
///////////////////////////////////////////////////////////////////////////////

// default_compress_options
//   function: a freshly default-constructed option set - the baseline every
// "did anything move?" check compares against.
inline compress_options
default_compress_options()
{
    return compress_options();
}

// compress_options_are_default
//   function: true iff _opt still matches a freshly constructed set, i.e.
// nothing anywhere in the tree has been touched.
inline bool
compress_options_are_default(
    const compress_options& _opt
)
{
    const compress_options fresh;

    return compress_options_equal(_opt, fresh);
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  DIFF DESCRIPTION                                     ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // diff_accumulator
    //   helper: appends comma-separated field names to a growing string.  A
    // plain struct with a static member rather than a lambda, so the header
    // stays usable under C++98.
    struct diff_accumulator
    {
        static void
        add(
            std::string& _out,
            const char*  _name
        )
        {
            if (!_out.empty())
            {
                _out += ", ";
            }
            _out += _name;

            return;
        }
    };

NS_END  // internal

// describe_compress_option_diff
//   function: a comma-separated list of every field that differs between two
// option sets, qualified by its block ("zstd.window_log"), or an empty string
// when the two are identical.  Field-level throughout - this is the header
// whose whole subject is the codec tuning, so burying a moved knob under a
// block name would defeat the purpose.
inline std::string
describe_compress_option_diff(
    const compress_options& _a,
    const compress_options& _b
)
{
    typedef internal::diff_accumulator acc;

    std::string out;

    // generic effort
    if (_a.level != _b.level) { acc::add(out, "level"); }

    // deflate (3)
    if (_a.deflate.window_bits != _b.deflate.window_bits) { acc::add(out, "deflate.window_bits"); }
    if (_a.deflate.mem_level   != _b.deflate.mem_level)   { acc::add(out, "deflate.mem_level"); }
    if (_a.deflate.strategy    != _b.deflate.strategy)    { acc::add(out, "deflate.strategy"); }

    // bzip2 (4)
    if (_a.bzip2.block_size_100k  != _b.bzip2.block_size_100k)  { acc::add(out, "bzip2.block_size_100k"); }
    if (_a.bzip2.work_factor      != _b.bzip2.work_factor)      { acc::add(out, "bzip2.work_factor"); }
    if (_a.bzip2.verbosity        != _b.bzip2.verbosity)        { acc::add(out, "bzip2.verbosity"); }
    if (_a.bzip2.small_decompress != _b.bzip2.small_decompress) { acc::add(out, "bzip2.small_decompress"); }

    // lzma (11)
    if (_a.lzma.extreme   != _b.lzma.extreme)   { acc::add(out, "lzma.extreme"); }
    if (_a.lzma.check     != _b.lzma.check)     { acc::add(out, "lzma.check"); }
    if (_a.lzma.dict_size != _b.lzma.dict_size) { acc::add(out, "lzma.dict_size"); }
    if (_a.lzma.lc        != _b.lzma.lc)        { acc::add(out, "lzma.lc"); }
    if (_a.lzma.lp        != _b.lzma.lp)        { acc::add(out, "lzma.lp"); }
    if (_a.lzma.pb        != _b.lzma.pb)        { acc::add(out, "lzma.pb"); }
    if (_a.lzma.mode      != _b.lzma.mode)      { acc::add(out, "lzma.mode"); }
    if (_a.lzma.nice_len  != _b.lzma.nice_len)  { acc::add(out, "lzma.nice_len"); }
    if (_a.lzma.mf        != _b.lzma.mf)        { acc::add(out, "lzma.mf"); }
    if (_a.lzma.depth     != _b.lzma.depth)     { acc::add(out, "lzma.depth"); }
    if (_a.lzma.threads   != _b.lzma.threads)   { acc::add(out, "lzma.threads"); }

    // zstd (19)
    if (_a.zstd.level                  != _b.zstd.level)                  { acc::add(out, "zstd.level"); }
    if (_a.zstd.window_log             != _b.zstd.window_log)             { acc::add(out, "zstd.window_log"); }
    if (_a.zstd.hash_log               != _b.zstd.hash_log)               { acc::add(out, "zstd.hash_log"); }
    if (_a.zstd.chain_log              != _b.zstd.chain_log)              { acc::add(out, "zstd.chain_log"); }
    if (_a.zstd.search_log             != _b.zstd.search_log)             { acc::add(out, "zstd.search_log"); }
    if (_a.zstd.min_match              != _b.zstd.min_match)              { acc::add(out, "zstd.min_match"); }
    if (_a.zstd.target_length          != _b.zstd.target_length)          { acc::add(out, "zstd.target_length"); }
    if (_a.zstd.strategy               != _b.zstd.strategy)               { acc::add(out, "zstd.strategy"); }
    if (_a.zstd.long_distance_matching != _b.zstd.long_distance_matching) { acc::add(out, "zstd.long_distance_matching"); }
    if (_a.zstd.ldm_hash_log           != _b.zstd.ldm_hash_log)           { acc::add(out, "zstd.ldm_hash_log"); }
    if (_a.zstd.ldm_min_match          != _b.zstd.ldm_min_match)          { acc::add(out, "zstd.ldm_min_match"); }
    if (_a.zstd.ldm_bucket_size_log    != _b.zstd.ldm_bucket_size_log)    { acc::add(out, "zstd.ldm_bucket_size_log"); }
    if (_a.zstd.ldm_hash_rate_log      != _b.zstd.ldm_hash_rate_log)      { acc::add(out, "zstd.ldm_hash_rate_log"); }
    if (_a.zstd.content_size_flag      != _b.zstd.content_size_flag)      { acc::add(out, "zstd.content_size_flag"); }
    if (_a.zstd.checksum_flag          != _b.zstd.checksum_flag)          { acc::add(out, "zstd.checksum_flag"); }
    if (_a.zstd.dict_id_flag           != _b.zstd.dict_id_flag)           { acc::add(out, "zstd.dict_id_flag"); }
    if (_a.zstd.workers                != _b.zstd.workers)                { acc::add(out, "zstd.workers"); }
    if (_a.zstd.job_size               != _b.zstd.job_size)               { acc::add(out, "zstd.job_size"); }
    if (_a.zstd.overlap_log            != _b.zstd.overlap_log)            { acc::add(out, "zstd.overlap_log"); }

    // lz4 (7)
    if (_a.lz4.level              != _b.lz4.level)              { acc::add(out, "lz4.level"); }
    if (_a.lz4.block_size         != _b.lz4.block_size)         { acc::add(out, "lz4.block_size"); }
    if (_a.lz4.block_mode         != _b.lz4.block_mode)         { acc::add(out, "lz4.block_mode"); }
    if (_a.lz4.content_checksum   != _b.lz4.content_checksum)   { acc::add(out, "lz4.content_checksum"); }
    if (_a.lz4.block_checksum     != _b.lz4.block_checksum)     { acc::add(out, "lz4.block_checksum"); }
    if (_a.lz4.store_content_size != _b.lz4.store_content_size) { acc::add(out, "lz4.store_content_size"); }
    if (_a.lz4.favor_dec_speed    != _b.lz4.favor_dec_speed)    { acc::add(out, "lz4.favor_dec_speed"); }

    // brotli (5)
    if (_a.brotli.quality      != _b.brotli.quality)      { acc::add(out, "brotli.quality"); }
    if (_a.brotli.window_bits  != _b.brotli.window_bits)  { acc::add(out, "brotli.window_bits"); }
    if (_a.brotli.block_bits   != _b.brotli.block_bits)   { acc::add(out, "brotli.block_bits"); }
    if (_a.brotli.mode         != _b.brotli.mode)         { acc::add(out, "brotli.mode"); }
    if (_a.brotli.large_window != _b.brotli.large_window) { acc::add(out, "brotli.large_window"); }

    return out;
}

// describe_compress_diff_from_default
//   function: the same field list taken against a freshly constructed set -
// "what has this option set changed from pristine?".
inline std::string
describe_compress_diff_from_default(
    const compress_options& _opt
)
{
    const compress_options fresh;

    return describe_compress_option_diff(_opt, fresh);
}


///////////////////////////////////////////////////////////////////////////////
///                V.   OPTION BUILDERS                                      ///
///////////////////////////////////////////////////////////////////////////////
//   Small presets for the option sets suites reach for most.  Each starts from
// a pristine set and moves exactly one thing, so a caller can assert both the
// intended change and that nothing else drifted.

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
// two knobs a DEFLATE-consuming suite most often needs to vary together.
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


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_COMPRESS_OPTIONS_
