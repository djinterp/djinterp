/*******************************************************************************
* djinterp [test]                               compress_options_tests_enums.cpp
*
*   Tests for the eight categorical enums (compress_options.hpp section I).
* Each holds the value its library uses for that setting (zlib, liblzma,
* Zstandard, LZ4, Brotli), so a backend leaf hands the value through
* unchanged; a renumbering would be a silent behavioral change and must be
* caught here. Also confirms the "library default" enumerators are zero and
* that every enum is a plain, int-convertible C enum.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_options_tests_enums.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_enum_deflate_strategy
  Pins DEFLATE's strategies to zlib's Z_* values.
  Tests the following:
  - default 0, filtered 1, huffman_only 2, rle 3, fixed 4
*/
bool tests_compress_options_enum_deflate_strategy()
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
  Pins the xz integrity checks to liblzma's LZMA_CHECK_* values.
  Tests the following:
  - none 0, crc32 1, crc64 4, sha256 10
*/
bool tests_compress_options_enum_lzma_check()
{
    D_CO_CHECK(static_cast<int>(lzma_check_none)   == 0);
    D_CO_CHECK(static_cast<int>(lzma_check_crc32)  == 1);
    D_CO_CHECK(static_cast<int>(lzma_check_crc64)  == 4);
    D_CO_CHECK(static_cast<int>(lzma_check_sha256) == 10);

    return true;
}

/*
tests_compress_options_enum_lzma_mode
  Pins the lzma encoder modes to liblzma's LZMA_MODE_* values (the
  encoder's own default is UNSET, not an enumerator).
  Tests the following:
  - fast 1, normal 2
*/
bool tests_compress_options_enum_lzma_mode()
{
    D_CO_CHECK(static_cast<int>(lzma_mode_fast)   == 1);
    D_CO_CHECK(static_cast<int>(lzma_mode_normal) == 2);

    return true;
}

/*
tests_compress_options_enum_lzma_match_finder
  Pins the lzma match finders to liblzma's LZMA_MF_* values.
  Tests the following:
  - hc3 0x03, hc4 0x04, bt2 0x12, bt3 0x13, bt4 0x14
*/
bool tests_compress_options_enum_lzma_match_finder()
{
    D_CO_CHECK(static_cast<int>(lzma_mf_hc3) == 0x03);
    D_CO_CHECK(static_cast<int>(lzma_mf_hc4) == 0x04);
    D_CO_CHECK(static_cast<int>(lzma_mf_bt2) == 0x12);
    D_CO_CHECK(static_cast<int>(lzma_mf_bt3) == 0x13);
    D_CO_CHECK(static_cast<int>(lzma_mf_bt4) == 0x14);

    return true;
}

/*
tests_compress_options_enum_zstd_strategy
  Pins the Zstandard strategies to ZSTD_strategy, fastest to
  highest-ratio.
  Tests the following:
  - fast 1 through btultra2 9, in order
*/
bool tests_compress_options_enum_zstd_strategy()
{
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
  Pins the LZ4 frame block sizes to LZ4F_blockSizeID_t.
  Tests the following:
  - default 0, 64kb 4, 256kb 5, 1mb 6, 4mb 7
*/
bool tests_compress_options_enum_lz4_block_size()
{
    D_CO_CHECK(static_cast<int>(lz4_block_size_default) == 0);
    D_CO_CHECK(static_cast<int>(lz4_block_size_64kb)    == 4);
    D_CO_CHECK(static_cast<int>(lz4_block_size_256kb)   == 5);
    D_CO_CHECK(static_cast<int>(lz4_block_size_1mb)     == 6);
    D_CO_CHECK(static_cast<int>(lz4_block_size_4mb)     == 7);

    return true;
}

/*
tests_compress_options_enum_lz4_block_mode
  Pins the LZ4 frame block modes to LZ4F_blockMode_t.
  Tests the following:
  - linked 0, independent 1
*/
bool tests_compress_options_enum_lz4_block_mode()
{
    D_CO_CHECK(static_cast<int>(lz4_block_mode_linked)      == 0);
    D_CO_CHECK(static_cast<int>(lz4_block_mode_independent) == 1);

    return true;
}

/*
tests_compress_options_enum_brotli_mode
  Pins Brotli's input hints to BROTLI_MODE_*.
  Tests the following:
  - generic 0, text 1, font 2
*/
bool tests_compress_options_enum_brotli_mode()
{
    D_CO_CHECK(static_cast<int>(brotli_mode_generic) == 0);
    D_CO_CHECK(static_cast<int>(brotli_mode_text)    == 1);
    D_CO_CHECK(static_cast<int>(brotli_mode_font)    == 2);

    return true;
}

/*
tests_compress_options_enum_neutral_defaults_are_zero
  Confirms that where a library's own enumeration has a zero meaning
  "the library's default", the enumerator holding it is zero; the
  knobs whose library has no such zero (lzma mode and match finder,
  zstd strategy) take UNSET for their default instead.
  Tests the following:
  - deflate_strategy_default, lzma_check_none, lz4_block_size_default,
  -   lz4_block_mode_linked and brotli_mode_generic are all 0
*/
bool tests_compress_options_enum_neutral_defaults_are_zero()
{
    D_CO_CHECK(static_cast<int>(deflate_strategy_default) == 0);
    D_CO_CHECK(static_cast<int>(lzma_check_none)          == 0);
    D_CO_CHECK(static_cast<int>(lz4_block_size_default)   == 0);
    D_CO_CHECK(static_cast<int>(lz4_block_mode_linked)    == 0);
    D_CO_CHECK(static_cast<int>(brotli_mode_generic)      == 0);

    return true;
}

/*
tests_compress_options_enum_are_plain
  Confirms every categorical enum is a plain (unscoped) enum, implicitly
  convertible to int, rather than an enum class: they are the core's C enums,
  and a knob holds their value in an int32_t.
  Tests the following:
  - all eight enum types satisfy the plain-enum property
*/
bool tests_compress_options_enum_are_plain()
{
    D_CO_CHECK(plain_enum<DDeflateStrategy>());
    D_CO_CHECK(plain_enum<DLzmaCheck>());
    D_CO_CHECK(plain_enum<DLzmaMode>());
    D_CO_CHECK(plain_enum<DLzmaMf>());
    D_CO_CHECK(plain_enum<DZstdStrategy>());
    D_CO_CHECK(plain_enum<DLz4BlockSize>());
    D_CO_CHECK(plain_enum<DLz4BlockMode>());
    D_CO_CHECK(plain_enum<DBrotliMode>());

    return true;
}


NS_END  // testing
NS_END  // djinterp
