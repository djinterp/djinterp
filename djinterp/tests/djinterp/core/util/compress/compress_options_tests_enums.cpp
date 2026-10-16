/*******************************************************************************
* djinterp [test]                               compress_options_tests_enums.cpp
*
*   Tests for the eight categorical enums (compress_options.hpp section I).
* Each enum's values are pinned in order - the dispatch leaves in compress.cpp
* translate them to backend constants, so a reordering or renumbering is a
* silent behavioral change and must be caught here.  Also confirms the neutral
* first enumerator of each is 0 (the "let the codec / level decide" default) and
* that every enum is a plain, int-convertible enum rather than an enum class
* (the header's version-portability choice).
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests_enums.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_enum_deflate_strategy
  Pins the DEFLATE strategy enumerators (the zlib Z_* strategies).
  Tests the following:
  - the five enumerators hold values 0..4 in the documented order
*/
bool
tests_compress_options_enum_deflate_strategy()
{
    D_CO_CHECK(static_cast<int>(deflate_strategy_default)      == 0);
    D_CO_CHECK(static_cast<int>(deflate_strategy_filtered)     == 1);
    D_CO_CHECK(static_cast<int>(deflate_strategy_huffman_only) == 2);
    D_CO_CHECK(static_cast<int>(deflate_strategy_rle)          == 3);
    D_CO_CHECK(static_cast<int>(deflate_strategy_fixed)        == 4);

    return true;
}

/*
tests_compress_options_enum_lzma_check
  Pins the xz integrity-check enumerators.
  Tests the following:
  - the four enumerators hold values 0..3 in the documented order
*/
bool
tests_compress_options_enum_lzma_check()
{
    D_CO_CHECK(static_cast<int>(lzma_check_none)   == 0);
    D_CO_CHECK(static_cast<int>(lzma_check_crc32)  == 1);
    D_CO_CHECK(static_cast<int>(lzma_check_crc64)  == 2);
    D_CO_CHECK(static_cast<int>(lzma_check_sha256) == 3);

    return true;
}

/*
tests_compress_options_enum_lzma_mode
  Pins the lzma encoder-mode enumerators.
  Tests the following:
  - the three enumerators hold values 0..2 in the documented order
*/
bool
tests_compress_options_enum_lzma_mode()
{
    D_CO_CHECK(static_cast<int>(lzma_mode_default) == 0);
    D_CO_CHECK(static_cast<int>(lzma_mode_fast)    == 1);
    D_CO_CHECK(static_cast<int>(lzma_mode_normal)  == 2);

    return true;
}

/*
tests_compress_options_enum_lzma_match_finder
  Pins the lzma match-finder enumerators.
  Tests the following:
  - the six enumerators hold values 0..5 in the documented order
*/
bool
tests_compress_options_enum_lzma_match_finder()
{
    D_CO_CHECK(static_cast<int>(lzma_mf_default) == 0);
    D_CO_CHECK(static_cast<int>(lzma_mf_hc3)     == 1);
    D_CO_CHECK(static_cast<int>(lzma_mf_hc4)     == 2);
    D_CO_CHECK(static_cast<int>(lzma_mf_bt2)     == 3);
    D_CO_CHECK(static_cast<int>(lzma_mf_bt3)     == 4);
    D_CO_CHECK(static_cast<int>(lzma_mf_bt4)     == 5);

    return true;
}

/*
tests_compress_options_enum_zstd_strategy
  Pins the Zstandard match-strategy enumerators, fastest to highest-ratio.
  Tests the following:
  - the ten enumerators hold values 0..9 in the documented order
*/
bool
tests_compress_options_enum_zstd_strategy()
{
    D_CO_CHECK(static_cast<int>(zstd_strategy_default)  == 0);
    D_CO_CHECK(static_cast<int>(zstd_strategy_fast)     == 1);
    D_CO_CHECK(static_cast<int>(zstd_strategy_dfast)    == 2);
    D_CO_CHECK(static_cast<int>(zstd_strategy_greedy)   == 3);
    D_CO_CHECK(static_cast<int>(zstd_strategy_lazy)     == 4);
    D_CO_CHECK(static_cast<int>(zstd_strategy_lazy2)    == 5);
    D_CO_CHECK(static_cast<int>(zstd_strategy_btlazy2)  == 6);
    D_CO_CHECK(static_cast<int>(zstd_strategy_btopt)    == 7);
    D_CO_CHECK(static_cast<int>(zstd_strategy_btultra)  == 8);
    D_CO_CHECK(static_cast<int>(zstd_strategy_btultra2) == 9);

    return true;
}

/*
tests_compress_options_enum_lz4_block_size
  Pins the LZ4 frame maximum-block-size enumerators.
  Tests the following:
  - the five enumerators hold values 0..4 in the documented order
*/
bool
tests_compress_options_enum_lz4_block_size()
{
    D_CO_CHECK(static_cast<int>(lz4_block_size_default) == 0);
    D_CO_CHECK(static_cast<int>(lz4_block_size_64kb)    == 1);
    D_CO_CHECK(static_cast<int>(lz4_block_size_256kb)   == 2);
    D_CO_CHECK(static_cast<int>(lz4_block_size_1mb)     == 3);
    D_CO_CHECK(static_cast<int>(lz4_block_size_4mb)     == 4);

    return true;
}

/*
tests_compress_options_enum_lz4_block_mode
  Pins the LZ4 frame block-mode enumerators.
  Tests the following:
  - linked is 0 (the default) and independent is 1
*/
bool
tests_compress_options_enum_lz4_block_mode()
{
    D_CO_CHECK(static_cast<int>(lz4_block_mode_linked)      == 0);
    D_CO_CHECK(static_cast<int>(lz4_block_mode_independent) == 1);

    return true;
}

/*
tests_compress_options_enum_brotli_mode
  Pins the Brotli content-mode enumerators.
  Tests the following:
  - the three enumerators hold values 0..2 in the documented order
*/
bool
tests_compress_options_enum_brotli_mode()
{
    D_CO_CHECK(static_cast<int>(brotli_mode_generic) == 0);
    D_CO_CHECK(static_cast<int>(brotli_mode_text)    == 1);
    D_CO_CHECK(static_cast<int>(brotli_mode_font)    == 2);

    return true;
}

/*
tests_compress_options_enum_neutral_defaults_are_zero
  Confirms the neutral "let the codec / level decide" enumerator of every enum
  is 0 - the invariant the per-codec structs lean on when they default an
  enum-typed field to its neutral value.
  Tests the following:
  - the first (neutral) enumerator of all eight enums equals 0
*/
bool
tests_compress_options_enum_neutral_defaults_are_zero()
{
    D_CO_CHECK(static_cast<int>(deflate_strategy_default)  == 0);
    D_CO_CHECK(static_cast<int>(lzma_check_none)           == 0);
    D_CO_CHECK(static_cast<int>(lzma_mode_default)         == 0);
    D_CO_CHECK(static_cast<int>(lzma_mf_default)           == 0);
    D_CO_CHECK(static_cast<int>(zstd_strategy_default)     == 0);
    D_CO_CHECK(static_cast<int>(lz4_block_size_default)    == 0);
    D_CO_CHECK(static_cast<int>(lz4_block_mode_linked)     == 0);
    D_CO_CHECK(static_cast<int>(brotli_mode_generic)       == 0);

    return true;
}

/*
tests_compress_options_enum_are_plain
  Confirms every categorical enum is a plain (unscoped) enum, implicitly
  convertible to int, rather than an enum class - the header's deliberate
  version-portability choice (C++98..C++23, no enum class).
  Tests the following:
  - all eight enum types satisfy the plain-enum property
*/
bool
tests_compress_options_enum_are_plain()
{
    D_CO_CHECK(plain_enum<deflate_strategy>());
    D_CO_CHECK(plain_enum<lzma_check>());
    D_CO_CHECK(plain_enum<lzma_mode>());
    D_CO_CHECK(plain_enum<lzma_match_finder>());
    D_CO_CHECK(plain_enum<zstd_strategy>());
    D_CO_CHECK(plain_enum<lz4_block_size>());
    D_CO_CHECK(plain_enum<lz4_block_mode>());
    D_CO_CHECK(plain_enum<brotli_mode>());

    return true;
}


NS_END  // testing
NS_END  // djinterp
