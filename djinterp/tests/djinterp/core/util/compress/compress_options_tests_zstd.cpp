/*******************************************************************************
* djinterp [test]                                compress_options_tests_zstd.cpp
*
*   Tests for zstd_options (compress_options.hpp section II.iv) - the widest
* tuning block, mirroring Zstandard's advanced-parameter set.  Every numeric
* knob defaults to 0 ("backend default"), but the frame flags are the subtle
* part and get their own test: content_size_flag and dict_id_flag default to
* true while checksum_flag defaults to false.
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests_zstd.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_zstd_defaults
  Confirms every numeric and categorical field of a default-constructed
  zstd_options sits on its sentinel.
  Tests the following:
  - level and all *_log / match / length knobs default to 0
  - the long-distance-matcher knobs default to 0 and the matcher is off
  - strategy defaults to the neutral enumerator; workers, job_size, overlap_log
    default to 0
  (the boolean frame flags are checked by the flag-defaults test)
*/
bool
tests_compress_options_zstd_defaults()
{
    zstd_options z;

    D_CO_CHECK(z.level         == 0);
    D_CO_CHECK(z.window_log    == 0);
    D_CO_CHECK(z.hash_log      == 0);
    D_CO_CHECK(z.chain_log     == 0);
    D_CO_CHECK(z.search_log    == 0);
    D_CO_CHECK(z.min_match     == 0);
    D_CO_CHECK(z.target_length == 0);
    D_CO_CHECK(z.strategy      == zstd_strategy_default);

    D_CO_CHECK(z.long_distance_matching == false);
    D_CO_CHECK(z.ldm_hash_log        == 0);
    D_CO_CHECK(z.ldm_min_match       == 0);
    D_CO_CHECK(z.ldm_bucket_size_log == 0);
    D_CO_CHECK(z.ldm_hash_rate_log   == 0);

    D_CO_CHECK(z.workers     == 0);
    D_CO_CHECK(z.job_size    == static_cast<std::size_t>(0));
    D_CO_CHECK(z.overlap_log == 0);

    return true;
}

/*
tests_compress_options_zstd_flag_defaults
  Locks the frame-flag defaults - the one part of zstd_options a "everything
  starts at zero/false" assumption would get wrong.
  Tests the following:
  - content_size_flag defaults to true (the decompressed size is written)
  - dict_id_flag defaults to true
  - checksum_flag defaults to false
*/
bool
tests_compress_options_zstd_flag_defaults()
{
    zstd_options z;

    D_CO_CHECK(z.content_size_flag == true);
    D_CO_CHECK(z.dict_id_flag      == true);
    D_CO_CHECK(z.checksum_flag     == false);

    return true;
}

/*
tests_compress_options_zstd_fields
  Confirms the fields are independent writable members across the numeric,
  boolean, enum, and size_t kinds.
  Tests the following:
  - representative fields of each kind round-trip written values
  - writing one field does not disturb the others
*/
bool
tests_compress_options_zstd_fields()
{
    zstd_options z;

    z.level = 19;
    D_CO_CHECK(z.level      == 19);
    D_CO_CHECK(z.window_log == 0);        // untouched

    z.window_log = 27;
    D_CO_CHECK(z.window_log == 27);
    D_CO_CHECK(z.level      == 19);       // preserved

    z.strategy = zstd_strategy_btultra2;
    D_CO_CHECK(z.strategy   == zstd_strategy_btultra2);

    z.long_distance_matching = true;
    z.ldm_hash_log           = 24;
    D_CO_CHECK(z.long_distance_matching == true);
    D_CO_CHECK(z.ldm_hash_log           == 24);

    // flip each flag off/on relative to its default
    z.content_size_flag = false;
    z.checksum_flag     = true;
    z.dict_id_flag      = false;
    D_CO_CHECK(z.content_size_flag == false);
    D_CO_CHECK(z.checksum_flag     == true);
    D_CO_CHECK(z.dict_id_flag      == false);

    z.workers  = 4;
    z.job_size = static_cast<std::size_t>(1) << 20;   // 1 MiB
    D_CO_CHECK(z.workers  == 4);
    D_CO_CHECK(z.job_size == (static_cast<std::size_t>(1) << 20));

    // spot-check that the earliest writes survived the later ones
    D_CO_CHECK(z.level      == 19);
    D_CO_CHECK(z.window_log == 27);
    D_CO_CHECK(z.strategy   == zstd_strategy_btultra2);

    return true;
}

/*
tests_compress_options_zstd_layout
  Confirms zstd_options is portable, ABI-stable plain data despite its size.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible,
    and non-polymorphic
*/
bool
tests_compress_options_zstd_layout()
{
    D_CO_CHECK(pod_ish_layout<zstd_options>());

    return true;
}


NS_END  // testing
NS_END  // djinterp
