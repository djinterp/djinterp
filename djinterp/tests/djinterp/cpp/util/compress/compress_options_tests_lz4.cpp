/*******************************************************************************
* djinterp [test]                                 compress_options_tests_lz4.cpp
*
*   Tests for the LZ4 frame block (DLz4Options): level, block size and mode, and
* the four framing flags (compress_options.hpp section II).
* The block is the core's struct; a pristine one has every knob UNSET.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_options_tests_lz4.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_lz4_defaults
  Confirms the lz4 block of a default-constructed compress_options is
  pristine: every knob UNSET, which the core resolves at dispatch.
  (DLz4Options is the core's plain struct and has no constructor of its own; a
  block is initialised by the option set it lives in.)
  Tests the following:
  - each of the 7 knobs is D_COMPRESS_KNOB_UNSET
*/
bool tests_compress_options_lz4_defaults()
{
    compress_options o;
    D_CO_CHECK(o.lz4.level == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.block_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.block_mode == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.content_checksum == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.block_checksum == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.store_content_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.favor_dec_speed == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_lz4_fields
  Confirms each knob is a distinct, writable member - assigning one leaves the
  others untouched (no aliasing or packing surprise).
  Tests the following:
  - each written knob reads back its value
  - every knob not yet written is still UNSET
*/
bool tests_compress_options_lz4_fields()
{
    compress_options o;

    o.lz4.block_size = lz4_block_size_4mb;
    D_CO_CHECK(o.lz4.block_size == lz4_block_size_4mb);
    D_CO_CHECK(o.lz4.level == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.block_mode == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.content_checksum == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.block_checksum == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.store_content_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.favor_dec_speed == D_COMPRESS_KNOB_UNSET);

    o.lz4.block_mode = lz4_block_mode_independent;
    D_CO_CHECK(o.lz4.block_size == lz4_block_size_4mb);
    D_CO_CHECK(o.lz4.block_mode == lz4_block_mode_independent);
    D_CO_CHECK(o.lz4.level == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.content_checksum == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.block_checksum == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.store_content_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.favor_dec_speed == D_COMPRESS_KNOB_UNSET);

    o.lz4.content_checksum = 1;
    D_CO_CHECK(o.lz4.block_size == lz4_block_size_4mb);
    D_CO_CHECK(o.lz4.block_mode == lz4_block_mode_independent);
    D_CO_CHECK(o.lz4.content_checksum == 1);
    D_CO_CHECK(o.lz4.level == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.block_checksum == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.store_content_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lz4.favor_dec_speed == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_lz4_layout
  Confirms DLz4Options is portable plain data of exactly its knobs: 7 int32_t
  fields, no padding, so the C and C++ faces share one layout.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible
    and non-polymorphic
  - its size is 7 int32_t
*/
bool tests_compress_options_lz4_layout()
{
    D_CO_CHECK(pod_ish_layout<DLz4Options>());
    D_CO_CHECK(sizeof(DLz4Options) == (7 * sizeof(int32_t)));

    return true;
}


NS_END  // testing
NS_END  // djinterp
