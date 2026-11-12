/*******************************************************************************
* djinterp [test]                               compress_options_tests_bzip2.cpp
*
*   Tests for the bzip2 block (DBzip2Options): block size, work factor,
* verbosity and the small-decompress flag (compress_options.hpp section II).
* The block is the core's struct; a pristine one has every knob UNSET.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_options_tests_bzip2.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_bzip2_defaults
  Confirms the bzip2 block of a default-constructed compress_options is
  pristine: every knob UNSET, which the core resolves at dispatch.
  (DBzip2Options is the core's plain struct and has no constructor of its own;
  a block is initialised by the option set it lives in.)
  Tests the following:
  - each of the 4 knobs is D_COMPRESS_KNOB_UNSET
*/
bool tests_compress_options_bzip2_defaults()
{
    compress_options o;
    D_CO_CHECK(o.bzip2.block_size_100k == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.bzip2.work_factor == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.bzip2.verbosity == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.bzip2.small_decompress == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_bzip2_fields
  Confirms each knob is a distinct, writable member - assigning one leaves the
  others untouched (no aliasing or packing surprise).
  Tests the following:
  - each written knob reads back its value
  - every knob not yet written is still UNSET
*/
bool tests_compress_options_bzip2_fields()
{
    compress_options o;

    o.bzip2.block_size_100k = 9;
    D_CO_CHECK(o.bzip2.block_size_100k == 9);
    D_CO_CHECK(o.bzip2.work_factor == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.bzip2.verbosity == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.bzip2.small_decompress == D_COMPRESS_KNOB_UNSET);

    o.bzip2.work_factor = 30;
    D_CO_CHECK(o.bzip2.block_size_100k == 9);
    D_CO_CHECK(o.bzip2.work_factor == 30);
    D_CO_CHECK(o.bzip2.verbosity == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.bzip2.small_decompress == D_COMPRESS_KNOB_UNSET);

    o.bzip2.small_decompress = 1;
    D_CO_CHECK(o.bzip2.block_size_100k == 9);
    D_CO_CHECK(o.bzip2.work_factor == 30);
    D_CO_CHECK(o.bzip2.small_decompress == 1);
    D_CO_CHECK(o.bzip2.verbosity == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_bzip2_layout
  Confirms DBzip2Options is portable plain data of exactly its knobs: 4
  int32_t fields, no padding, so the C and C++ faces share one layout.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible
    and non-polymorphic
  - its size is 4 int32_t
*/
bool tests_compress_options_bzip2_layout()
{
    D_CO_CHECK(pod_ish_layout<DBzip2Options>());
    D_CO_CHECK(sizeof(DBzip2Options) == (4 * sizeof(int32_t)));

    return true;
}


NS_END  // testing
NS_END  // djinterp
