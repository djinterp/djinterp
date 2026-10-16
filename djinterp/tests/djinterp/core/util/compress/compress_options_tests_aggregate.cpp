/*******************************************************************************
* djinterp [test]                           compress_options_tests_aggregate.cpp
*
*   Tests for the compress_options aggregate (compress_options.hpp section III)
* - the complete knob set passed to a compression call.  Confirms the generic
* `level` sentinel (the stub-compatibility invariant), that the constructor
* default-constructs each per-codec sub-block, that the sub-blocks are mutually
* independent, that compress_options has value semantics (independent deep
* copies via the implicit copy and assignment), and that the whole aggregate is
* portable plain data.  A final test maps the exact set of -1 sentinels across
* the vocabulary, guarding the deliberate -1-vs-0 split.
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests_aggregate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_aggregate_level_default
  Confirms the generic effort sentinel - the field the minimal stub in
  compress.hpp defines, which this expanded form must preserve.
  Tests the following:
  - a default-constructed compress_options has level == -1 (the stub-compatible
    "backend default" sentinel)
*/
bool
tests_compress_options_aggregate_level_default()
{
    compress_options o;

    D_CO_CHECK(o.level == -1);

    return true;
}

/*
tests_compress_options_aggregate_subobjects_defaulted
  Confirms the aggregate constructor default-constructs every per-codec block
  (the members are not left uninitialized) - spot-checked on each block,
  including the non-zero defaults that a memberwise-zero assumption would miss.
  Tests the following:
  - each of the six sub-blocks reports its own documented defaults inside a
    default compress_options
*/
bool
tests_compress_options_aggregate_subobjects_defaulted()
{
    compress_options o;

    D_CO_CHECK(o.deflate.window_bits == 0);
    D_CO_CHECK(o.deflate.strategy    == deflate_strategy_default);

    D_CO_CHECK(o.bzip2.block_size_100k  == 0);
    D_CO_CHECK(o.bzip2.small_decompress == false);

    D_CO_CHECK(o.lzma.check == lzma_check_crc64);   // non-zero default
    D_CO_CHECK(o.lzma.lc    == -1);                 // -1 sentinel
    D_CO_CHECK(o.lzma.pb    == -1);

    D_CO_CHECK(o.zstd.level             == 0);
    D_CO_CHECK(o.zstd.content_size_flag == true);   // non-false default
    D_CO_CHECK(o.zstd.dict_id_flag      == true);   // non-false default

    D_CO_CHECK(o.lz4.level      == 0);
    D_CO_CHECK(o.lz4.block_mode == lz4_block_mode_linked);

    D_CO_CHECK(o.brotli.quality == -1);             // -1 sentinel
    D_CO_CHECK(o.brotli.mode    == brotli_mode_generic);

    return true;
}

/*
tests_compress_options_aggregate_independence
  Confirms the per-codec blocks within one compress_options are independent -
  tuning one codec never perturbs another's block.
  Tests the following:
  - writing into one sub-block leaves the others at their defaults
  - a second write into a different block preserves the first
*/
bool
tests_compress_options_aggregate_independence()
{
    compress_options o;

    o.deflate.window_bits = 15;
    D_CO_CHECK(o.deflate.window_bits   == 15);
    D_CO_CHECK(o.bzip2.block_size_100k == 0);       // untouched
    D_CO_CHECK(o.zstd.window_log       == 0);       // untouched
    D_CO_CHECK(o.brotli.quality        == -1);      // untouched sentinel

    o.zstd.window_log = 27;
    D_CO_CHECK(o.zstd.window_log     == 27);
    D_CO_CHECK(o.deflate.window_bits == 15);        // preserved
    D_CO_CHECK(o.lzma.lc             == -1);        // untouched sentinel

    return true;
}

/*
tests_compress_options_aggregate_value_semantics
  Confirms compress_options is a value type - the implicit copy constructor and
  copy assignment produce independent, deep copies (each carries its own
  sub-blocks), so mutating a copy never reaches back to the source.
  Tests the following:
  - a copy-constructed instance carries the source's modified fields
  - mutating the copy leaves the source unchanged
  - copy assignment likewise produces an independent deep copy
*/
bool
tests_compress_options_aggregate_value_semantics()
{
    compress_options a;

    a.level            = 5;
    a.zstd.window_log  = 27;
    a.brotli.quality   = 11;
    a.lzma.check       = lzma_check_sha256;

    // copy construction carries every modified field
    compress_options b(a);
    D_CO_CHECK(b.level           == 5);
    D_CO_CHECK(b.zstd.window_log == 27);
    D_CO_CHECK(b.brotli.quality  == 11);
    D_CO_CHECK(b.lzma.check      == lzma_check_sha256);

    // mutating the copy does not disturb the source (deep, independent copy)
    b.level           = 9;
    b.zstd.window_log = 20;
    D_CO_CHECK(a.level           == 5);
    D_CO_CHECK(a.zstd.window_log == 27);

    // copy assignment behaves the same
    compress_options c;
    c = a;
    D_CO_CHECK(c.level           == 5);
    D_CO_CHECK(c.zstd.window_log == 27);
    D_CO_CHECK(c.brotli.quality  == 11);

    c.brotli.quality = 0;
    D_CO_CHECK(a.brotli.quality  == 11);   // source still intact

    return true;
}

/*
tests_compress_options_aggregate_layout
  Confirms the whole aggregate is portable, ABI-stable plain data - it composes
  only plain-data sub-blocks, so the aggregate inherits the property.
  Tests the following:
  - compress_options is trivially copyable, standard-layout, trivially
    destructible, and non-polymorphic
*/
bool
tests_compress_options_aggregate_layout()
{
    D_CO_CHECK(pod_ish_layout<compress_options>());

    return true;
}

/*
tests_compress_options_aggregate_minus_one_sentinels
  Maps the complete set of fields whose "leave it to the backend/preset"
  sentinel is -1 rather than 0 - the deliberate exceptions to the 0-is-unset
  rule - and confirms the 0-sentinel knobs are indeed 0 (not -1).  A guard on
  the vocabulary's sentinel discipline as a whole.
  Tests the following:
  - exactly compress_options.level, lzma.lc / lp / pb, and brotli.quality
    default to -1
  - representative 0-sentinel integer knobs default to 0
*/
bool
tests_compress_options_aggregate_minus_one_sentinels()
{
    compress_options o;

    // the full -1 set
    D_CO_CHECK(o.level          == -1);
    D_CO_CHECK(o.lzma.lc        == -1);
    D_CO_CHECK(o.lzma.lp        == -1);
    D_CO_CHECK(o.lzma.pb        == -1);
    D_CO_CHECK(o.brotli.quality == -1);

    // and the 0-sentinel knobs are 0, not -1
    D_CO_CHECK(o.deflate.window_bits   == 0);
    D_CO_CHECK(o.deflate.mem_level     == 0);
    D_CO_CHECK(o.bzip2.block_size_100k == 0);
    D_CO_CHECK(o.lzma.nice_len         == 0);
    D_CO_CHECK(o.zstd.level            == 0);
    D_CO_CHECK(o.zstd.window_log       == 0);
    D_CO_CHECK(o.lz4.level             == 0);
    D_CO_CHECK(o.brotli.window_bits    == 0);

    return true;
}


NS_END  // testing
NS_END  // djinterp
