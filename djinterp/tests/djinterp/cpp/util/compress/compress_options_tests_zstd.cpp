/*******************************************************************************
* djinterp [test]                                compress_options_tests_zstd.cpp
*
*   Tests for the Zstandard block (DZstdOptions, compress_options.hpp
* section II): nineteen knobs, pristine at UNSET, and the flags the core pins
* when it resolves for zstd.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_options_tests_zstd.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_zstd_defaults
  Confirms the zstd block of a default-constructed compress_options is
  pristine: every knob UNSET, which the core resolves at dispatch.
  (DZstdOptions is the core's plain struct and has no constructor of its own;
  a block is initialised by the option set it lives in.)
  Tests the following:
  - each of the 19 knobs is D_COMPRESS_KNOB_UNSET
*/
bool tests_compress_options_zstd_defaults()
{
    compress_options o;
    D_CO_CHECK(o.zstd.level == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.window_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.hash_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.chain_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.search_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.min_match == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.target_length == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.strategy == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.long_distance_matching == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_hash_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_min_match == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_bucket_size_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_hash_rate_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.content_size_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.checksum_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.dict_id_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.workers == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.job_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.overlap_log == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_zstd_flag_defaults
  Confirms the zstd flags the core pins when it resolves for zstd, each for a
  reason rather than by convention: the frame records content size, checksum
  and dictionary id; long-distance matching is off; and there are no worker
  threads, which is what makes the output independent of how many cores the
  machine has.
  Tests the following:
  - content_size_flag, checksum_flag and dict_id_flag resolve to 1
  - long_distance_matching and workers resolve to 0
  - the encoder's own parameters (window_log .. target_length) stay UNSET
*/
bool tests_compress_options_zstd_flag_defaults()
{
    compress_options in;
    compress_options out;

    D_CO_CHECK(d_compress_options_resolve(D_CODEC_ID_ZSTD, &in, &out) ==
               D_PACK_STATUS_OK);

    D_CO_CHECK(out.zstd.content_size_flag      == 1);
    D_CO_CHECK(out.zstd.checksum_flag          == 1);
    D_CO_CHECK(out.zstd.dict_id_flag           == 1);
    D_CO_CHECK(out.zstd.long_distance_matching == 0);
    D_CO_CHECK(out.zstd.workers                == 0);

    D_CO_CHECK(out.zstd.window_log    == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.zstd.hash_log      == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.zstd.chain_log     == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.zstd.search_log    == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.zstd.min_match     == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.zstd.target_length == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_zstd_fields
  Confirms each knob is a distinct, writable member - assigning one leaves the
  others untouched (no aliasing or packing surprise).
  Tests the following:
  - each written knob reads back its value
  - every knob not yet written is still UNSET
*/
bool tests_compress_options_zstd_fields()
{
    compress_options o;

    o.zstd.window_log = 27;
    D_CO_CHECK(o.zstd.window_log == 27);
    D_CO_CHECK(o.zstd.level == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.hash_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.chain_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.search_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.min_match == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.target_length == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.strategy == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.long_distance_matching == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_hash_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_min_match == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_bucket_size_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_hash_rate_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.content_size_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.checksum_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.dict_id_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.workers == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.job_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.overlap_log == D_COMPRESS_KNOB_UNSET);

    o.zstd.strategy = zstd_strategy_btultra2;
    D_CO_CHECK(o.zstd.window_log == 27);
    D_CO_CHECK(o.zstd.strategy == zstd_strategy_btultra2);
    D_CO_CHECK(o.zstd.level == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.hash_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.chain_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.search_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.min_match == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.target_length == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.long_distance_matching == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_hash_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_min_match == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_bucket_size_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_hash_rate_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.content_size_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.checksum_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.dict_id_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.workers == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.job_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.overlap_log == D_COMPRESS_KNOB_UNSET);

    o.zstd.checksum_flag = 0;
    D_CO_CHECK(o.zstd.window_log == 27);
    D_CO_CHECK(o.zstd.strategy == zstd_strategy_btultra2);
    D_CO_CHECK(o.zstd.checksum_flag == 0);
    D_CO_CHECK(o.zstd.level == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.hash_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.chain_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.search_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.min_match == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.target_length == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.long_distance_matching == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_hash_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_min_match == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_bucket_size_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.ldm_hash_rate_log == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.content_size_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.dict_id_flag == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.workers == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.job_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.overlap_log == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_zstd_layout
  Confirms DZstdOptions is portable plain data of exactly its knobs: 19
  int32_t fields, no padding, so the C and C++ faces share one layout.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible
    and non-polymorphic
  - its size is 19 int32_t
*/
bool tests_compress_options_zstd_layout()
{
    D_CO_CHECK(pod_ish_layout<DZstdOptions>());
    D_CO_CHECK(sizeof(DZstdOptions) == (19 * sizeof(int32_t)));

    return true;
}


NS_END  // testing
NS_END  // djinterp
