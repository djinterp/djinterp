/*******************************************************************************
* djinterp [test]                               compress_options_tests_bzip2.cpp
*
*   Tests for bzip2_options (compress_options.hpp section II.ii).  Confirms the
* sentinel defaults (block_size_100k 0, work_factor 0, verbosity 0,
* small_decompress false), independent fields, and portable plain-data layout.
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests_bzip2.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_bzip2_defaults
  Confirms a default-constructed bzip2_options sits on its sentinels.
  Tests the following:
  - block_size_100k, work_factor, and verbosity default to 0
  - small_decompress defaults to false
*/
bool
tests_compress_options_bzip2_defaults()
{
    bzip2_options b;

    D_CO_CHECK(b.block_size_100k  == 0);
    D_CO_CHECK(b.work_factor      == 0);
    D_CO_CHECK(b.verbosity        == 0);
    D_CO_CHECK(b.small_decompress == false);

    return true;
}

/*
tests_compress_options_bzip2_fields
  Confirms each field is a distinct, writable member.
  Tests the following:
  - each field round-trips a written value
  - writing one field does not disturb the others
*/
bool
tests_compress_options_bzip2_fields()
{
    bzip2_options b;

    b.block_size_100k = 9;
    D_CO_CHECK(b.block_size_100k == 9);
    D_CO_CHECK(b.work_factor     == 0);      // untouched
    D_CO_CHECK(b.verbosity       == 0);      // untouched

    b.work_factor = 250;
    D_CO_CHECK(b.work_factor     == 250);
    D_CO_CHECK(b.block_size_100k == 9);      // preserved

    b.verbosity = 4;
    D_CO_CHECK(b.verbosity       == 4);

    b.small_decompress = true;
    D_CO_CHECK(b.small_decompress == true);
    D_CO_CHECK(b.block_size_100k  == 9);     // preserved
    D_CO_CHECK(b.work_factor      == 250);   // preserved

    return true;
}

/*
tests_compress_options_bzip2_layout
  Confirms bzip2_options is portable, ABI-stable plain data.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible,
    and non-polymorphic
*/
bool
tests_compress_options_bzip2_layout()
{
    D_CO_CHECK(pod_ish_layout<bzip2_options>());

    return true;
}


NS_END  // testing
NS_END  // djinterp
