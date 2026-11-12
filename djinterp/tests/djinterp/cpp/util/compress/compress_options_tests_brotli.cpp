/*******************************************************************************
* djinterp [test]                              compress_options_tests_brotli.cpp
*
*   Tests for the Brotli block (DBrotliOptions): quality, window and block bits,
* mode and the large-window flag (compress_options.hpp section II).
* The block is the core's struct; a pristine one has every knob UNSET.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_options_tests_brotli.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_brotli_defaults
  Confirms the brotli block of a default-constructed compress_options is
  pristine: every knob UNSET, which the core resolves at dispatch.
  (DBrotliOptions is the core's plain struct and has no constructor of its
  own; a block is initialised by the option set it lives in.)
  Tests the following:
  - each of the 5 knobs is D_COMPRESS_KNOB_UNSET
*/
bool tests_compress_options_brotli_defaults()
{
    compress_options o;
    D_CO_CHECK(o.brotli.quality == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.window_bits == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.block_bits == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.mode == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.large_window == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_brotli_fields
  Confirms each knob is a distinct, writable member - assigning one leaves the
  others untouched (no aliasing or packing surprise).
  Tests the following:
  - each written knob reads back its value
  - every knob not yet written is still UNSET
*/
bool tests_compress_options_brotli_fields()
{
    compress_options o;

    o.brotli.quality = 11;
    D_CO_CHECK(o.brotli.quality == 11);
    D_CO_CHECK(o.brotli.window_bits == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.block_bits == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.mode == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.large_window == D_COMPRESS_KNOB_UNSET);

    o.brotli.mode = brotli_mode_text;
    D_CO_CHECK(o.brotli.quality == 11);
    D_CO_CHECK(o.brotli.mode == brotli_mode_text);
    D_CO_CHECK(o.brotli.window_bits == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.block_bits == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.large_window == D_COMPRESS_KNOB_UNSET);

    o.brotli.large_window = 1;
    D_CO_CHECK(o.brotli.quality == 11);
    D_CO_CHECK(o.brotli.mode == brotli_mode_text);
    D_CO_CHECK(o.brotli.large_window == 1);
    D_CO_CHECK(o.brotli.window_bits == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.block_bits == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_brotli_layout
  Confirms DBrotliOptions is portable plain data of exactly its knobs: 5
  int32_t fields, no padding, so the C and C++ faces share one layout.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible
    and non-polymorphic
  - its size is 5 int32_t
*/
bool tests_compress_options_brotli_layout()
{
    D_CO_CHECK(pod_ish_layout<DBrotliOptions>());
    D_CO_CHECK(sizeof(DBrotliOptions) == (5 * sizeof(int32_t)));

    return true;
}


NS_END  // testing
NS_END  // djinterp
