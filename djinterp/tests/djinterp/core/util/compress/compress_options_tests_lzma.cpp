/*******************************************************************************
* djinterp [test]                                compress_options_tests_lzma.cpp
*
*   Tests for lzma_options (compress_options.hpp section II.iii) - the xz /
* liblzma tuning block.  This struct carries the vocabulary's least obvious
* defaults, so it gets a dedicated sentinel test in addition to the usual
* default / field / layout coverage: the literal-context knobs lc / lp / pb use
* -1 (not 0) as their "keep the preset default" sentinel because 0 is itself a
* meaningful value for them, and `check` defaults to crc64 (the xz default),
* which is NOT the enum's zero enumerator.
*
*
* path:      /tests/djinterp/core/util/compress/compress_options_tests_lzma.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_lzma_defaults
  Confirms every field of a default-constructed lzma_options.
  Tests the following:
  - extreme false; dict_size, nice_len, depth, and threads 0
  - mode and mf default to their neutral enumerators
  - check, lc, lp, and pb hold their (non-zero) sentinels, checked in detail by
    the sentinel test below
*/
bool
tests_compress_options_lzma_defaults()
{
    lzma_options l;

    D_CO_CHECK(l.extreme   == false);
    D_CO_CHECK(l.check     == lzma_check_crc64);
    D_CO_CHECK(l.dict_size == static_cast<std::size_t>(0));
    D_CO_CHECK(l.lc        == -1);
    D_CO_CHECK(l.lp        == -1);
    D_CO_CHECK(l.pb        == -1);
    D_CO_CHECK(l.mode      == lzma_mode_default);
    D_CO_CHECK(l.nice_len  == 0);
    D_CO_CHECK(l.mf        == lzma_mf_default);
    D_CO_CHECK(l.depth     == 0);
    D_CO_CHECK(l.threads   == 0u);

    return true;
}

/*
tests_compress_options_lzma_sentinels
  Locks the two design decisions a naive default would get wrong: the lc / lp /
  pb sentinel is -1 rather than 0 (0 is a legal value for these knobs, so it
  cannot double as "unset"), and the container check defaults to crc64 rather
  than the enum's zero value (none).
  Tests the following:
  - lc, lp, pb default to -1, and 0 is a distinct, non-sentinel value for them
  - check defaults to crc64, which differs from lzma_check_none (the zero
    enumerator)
*/
bool
tests_compress_options_lzma_sentinels()
{
    lzma_options l;

    // the -1 sentinel is deliberately NOT 0
    D_CO_CHECK(l.lc == -1);
    D_CO_CHECK(l.lp == -1);
    D_CO_CHECK(l.pb == -1);
    D_CO_CHECK(l.lc != 0);
    D_CO_CHECK(l.lp != 0);
    D_CO_CHECK(l.pb != 0);

    // the check default is crc64, not the zero enumerator
    D_CO_CHECK(l.check == lzma_check_crc64);
    D_CO_CHECK(l.check != lzma_check_none);
    D_CO_CHECK(static_cast<int>(lzma_check_none) == 0);   // and none really is 0

    return true;
}

/*
tests_compress_options_lzma_fields
  Confirms the fields are independent writable members, including the categorical
  ones.
  Tests the following:
  - representative numeric, boolean, and enum fields round-trip written values
  - writing one field does not disturb the others
*/
bool
tests_compress_options_lzma_fields()
{
    lzma_options l;

    l.extreme = true;
    D_CO_CHECK(l.extreme == true);
    D_CO_CHECK(l.check   == lzma_check_crc64);   // untouched

    l.check = lzma_check_sha256;
    D_CO_CHECK(l.check   == lzma_check_sha256);

    l.dict_size = static_cast<std::size_t>(1) << 26;   // 64 MiB
    D_CO_CHECK(l.dict_size == (static_cast<std::size_t>(1) << 26));

    l.lc = 0; l.lp = 2; l.pb = 1;
    D_CO_CHECK(l.lc == 0);
    D_CO_CHECK(l.lp == 2);
    D_CO_CHECK(l.pb == 1);

    l.mode = lzma_mode_normal;
    l.mf   = lzma_mf_bt4;
    D_CO_CHECK(l.mode == lzma_mode_normal);
    D_CO_CHECK(l.mf   == lzma_mf_bt4);

    l.threads = 4u;
    D_CO_CHECK(l.threads == 4u);

    // the earlier writes all survived
    D_CO_CHECK(l.extreme == true);
    D_CO_CHECK(l.check   == lzma_check_sha256);

    return true;
}

/*
tests_compress_options_lzma_layout
  Confirms lzma_options is portable, ABI-stable plain data.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible,
    and non-polymorphic
*/
bool
tests_compress_options_lzma_layout()
{
    D_CO_CHECK(pod_ish_layout<lzma_options>());

    return true;
}


NS_END  // testing
NS_END  // djinterp
