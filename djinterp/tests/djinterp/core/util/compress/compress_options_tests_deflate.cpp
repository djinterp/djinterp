/*******************************************************************************
* djinterp [test]                             compress_options_tests_deflate.cpp
*
*   Tests for deflate_options (compress_options.hpp section II.i) - advanced
* tuning for the zlib-family codecs.  Confirms the sentinel defaults
* (window_bits 0, mem_level 0, strategy neutral), that each field is an
* independent lvalue, and that the struct is portable plain data.
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests_deflate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_deflate_defaults
  Confirms a default-constructed deflate_options sits entirely on its "leave it
  to the backend" sentinels.
  Tests the following:
  - window_bits and mem_level default to 0 (backend maximum / default)
  - strategy defaults to the neutral deflate_strategy_default
*/
bool
tests_compress_options_deflate_defaults()
{
    deflate_options d;

    D_CO_CHECK(d.window_bits == 0);
    D_CO_CHECK(d.mem_level   == 0);
    D_CO_CHECK(d.strategy    == deflate_strategy_default);

    return true;
}

/*
tests_compress_options_deflate_fields
  Confirms each field is a distinct, writable member - assigning one leaves the
  others untouched (no unintended aliasing or packing surprise).
  Tests the following:
  - each field round-trips a written value
  - writing one field does not disturb the others
*/
bool
tests_compress_options_deflate_fields()
{
    deflate_options d;

    d.window_bits = 15;
    D_CO_CHECK(d.window_bits == 15);
    D_CO_CHECK(d.mem_level   == 0);                       // untouched
    D_CO_CHECK(d.strategy    == deflate_strategy_default); // untouched

    d.mem_level = 9;
    D_CO_CHECK(d.mem_level   == 9);
    D_CO_CHECK(d.window_bits == 15);                      // preserved

    d.strategy = deflate_strategy_rle;
    D_CO_CHECK(d.strategy    == deflate_strategy_rle);
    D_CO_CHECK(d.window_bits == 15);                      // preserved
    D_CO_CHECK(d.mem_level   == 9);                       // preserved

    return true;
}

/*
tests_compress_options_deflate_layout
  Confirms deflate_options is portable, ABI-stable plain data.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible,
    and non-polymorphic
*/
bool
tests_compress_options_deflate_layout()
{
    D_CO_CHECK(pod_ish_layout<deflate_options>());

    return true;
}


NS_END  // testing
NS_END  // djinterp
