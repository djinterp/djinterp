/*******************************************************************************
* djinterp [test]                              compress_options_tests_brotli.cpp
*
*   Tests for brotli_options (compress_options.hpp section II.vi).  Confirms the
* sentinel defaults - notably quality, whose "derive from the generic level"
* sentinel is -1 (not 0, which is a valid Brotli quality) - along with
* independent fields and portable plain-data layout.
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests_brotli.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_brotli_defaults
  Confirms a default-constructed brotli_options sits on its sentinels, with
  particular attention to the -1 quality sentinel.
  Tests the following:
  - quality defaults to -1 (derive from the generic level), a value distinct
    from the valid quality 0
  - window_bits and block_bits default to 0 (backend default / automatic)
  - mode defaults to brotli_mode_generic and large_window to false
*/
bool
tests_compress_options_brotli_defaults()
{
    brotli_options b;

    D_CO_CHECK(b.quality      == -1);
    D_CO_CHECK(b.quality      != 0);      // 0 is a real quality, not the sentinel
    D_CO_CHECK(b.window_bits  == 0);
    D_CO_CHECK(b.block_bits   == 0);
    D_CO_CHECK(b.mode         == brotli_mode_generic);
    D_CO_CHECK(b.large_window == false);

    return true;
}

/*
tests_compress_options_brotli_fields
  Confirms the fields are independent writable members.
  Tests the following:
  - representative numeric, enum, and boolean fields round-trip written values
  - writing one field does not disturb the others
*/
bool
tests_compress_options_brotli_fields()
{
    brotli_options b;

    b.quality = 11;                       // maximum ratio
    D_CO_CHECK(b.quality     == 11);
    D_CO_CHECK(b.window_bits == 0);       // untouched

    b.window_bits = 24;
    b.block_bits  = 22;
    D_CO_CHECK(b.window_bits == 24);
    D_CO_CHECK(b.block_bits  == 22);
    D_CO_CHECK(b.quality     == 11);      // preserved

    b.mode = brotli_mode_text;
    D_CO_CHECK(b.mode == brotli_mode_text);

    b.large_window = true;
    D_CO_CHECK(b.large_window == true);

    // earlier writes survived
    D_CO_CHECK(b.quality     == 11);
    D_CO_CHECK(b.window_bits == 24);
    D_CO_CHECK(b.block_bits  == 22);

    return true;
}

/*
tests_compress_options_brotli_layout
  Confirms brotli_options is portable, ABI-stable plain data.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible,
    and non-polymorphic
*/
bool
tests_compress_options_brotli_layout()
{
    D_CO_CHECK(pod_ish_layout<brotli_options>());

    return true;
}


NS_END  // testing
NS_END  // djinterp
