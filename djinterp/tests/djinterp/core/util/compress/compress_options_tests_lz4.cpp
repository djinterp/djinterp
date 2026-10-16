/*******************************************************************************
* djinterp [test]                                 compress_options_tests_lz4.cpp
*
*   Tests for lz4_options (compress_options.hpp section II.v) - the LZ4 frame
* descriptor plus encoder level.  Confirms the sentinel defaults (level 0,
* neutral block size / mode, all frame checksums and flags off), independent
* fields, and portable plain-data layout.
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests_lz4.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_lz4_defaults
  Confirms a default-constructed lz4_options sits on its sentinels.
  Tests the following:
  - level defaults to 0 (unset; the generic level is consulted first)
  - block_size and block_mode default to their neutral enumerators
  - content_checksum, block_checksum, store_content_size, and favor_dec_speed
    all default to false
*/
bool
tests_compress_options_lz4_defaults()
{
    lz4_options l;

    D_CO_CHECK(l.level              == 0);
    D_CO_CHECK(l.block_size         == lz4_block_size_default);
    D_CO_CHECK(l.block_mode         == lz4_block_mode_linked);
    D_CO_CHECK(l.content_checksum   == false);
    D_CO_CHECK(l.block_checksum     == false);
    D_CO_CHECK(l.store_content_size == false);
    D_CO_CHECK(l.favor_dec_speed    == false);

    return true;
}

/*
tests_compress_options_lz4_fields
  Confirms the fields are independent writable members.
  Tests the following:
  - representative numeric, enum, and boolean fields round-trip written values
  - writing one field does not disturb the others
*/
bool
tests_compress_options_lz4_fields()
{
    lz4_options l;

    l.level = 12;                        // LZ4HC maximum
    D_CO_CHECK(l.level      == 12);
    D_CO_CHECK(l.block_size == lz4_block_size_default);   // untouched

    l.block_size = lz4_block_size_4mb;
    l.block_mode = lz4_block_mode_independent;
    D_CO_CHECK(l.block_size == lz4_block_size_4mb);
    D_CO_CHECK(l.block_mode == lz4_block_mode_independent);
    D_CO_CHECK(l.level      == 12);      // preserved

    l.content_checksum   = true;
    l.block_checksum     = true;
    l.store_content_size = true;
    l.favor_dec_speed    = true;
    D_CO_CHECK(l.content_checksum   == true);
    D_CO_CHECK(l.block_checksum     == true);
    D_CO_CHECK(l.store_content_size == true);
    D_CO_CHECK(l.favor_dec_speed    == true);

    // earlier writes survived
    D_CO_CHECK(l.level      == 12);
    D_CO_CHECK(l.block_size == lz4_block_size_4mb);
    D_CO_CHECK(l.block_mode == lz4_block_mode_independent);

    return true;
}

/*
tests_compress_options_lz4_layout
  Confirms lz4_options is portable, ABI-stable plain data.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible,
    and non-polymorphic
*/
bool
tests_compress_options_lz4_layout()
{
    D_CO_CHECK(pod_ish_layout<lz4_options>());

    return true;
}


NS_END  // testing
NS_END  // djinterp
