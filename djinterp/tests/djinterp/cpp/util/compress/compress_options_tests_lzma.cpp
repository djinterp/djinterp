/*******************************************************************************
* djinterp [test]                                compress_options_tests_lzma.cpp
*
*   Tests for the lzma / xz block (DLzmaOptions, compress_options.hpp
* section II): eleven knobs, pristine at UNSET, and what the core's resolver
* does with them for xz.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_options_tests_lzma.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_lzma_defaults
  Confirms the lzma block of a default-constructed compress_options is
  pristine: every knob UNSET, which the core resolves at dispatch.
  (DLzmaOptions is the core's plain struct and has no constructor of its own;
  a block is initialised by the option set it lives in.)
  Tests the following:
  - each of the 11 knobs is D_COMPRESS_KNOB_UNSET
*/
bool tests_compress_options_lzma_defaults()
{
    compress_options o;
    D_CO_CHECK(o.lzma.extreme == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.check == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.dict_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lc == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lp == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.pb == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.mode == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.nice_len == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.mf == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.depth == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.threads == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_lzma_sentinels
  Confirms how the core resolves the lzma block for xz: the knobs with a
  cross-implementation default (extreme, check, threads) get one, the eight
  whose default is the encoder's own stay UNSET (the backend then omits the
  parameter rather than pass a number), a knob the caller set is never
  replaced, and other codecs' blocks are left as they were.
  Tests the following:
  - extreme, check and threads are no longer UNSET after resolution
  - lc, lp, pb, mode, nice_len, mf and depth are still UNSET
  - a dict_size the caller set survives resolution
  - the zstd block is still pristine
*/
bool tests_compress_options_lzma_sentinels()
{
    compress_options in;
    compress_options out;

    in.lzma.dict_size = (1 << 20);

    D_CO_CHECK(d_compress_options_resolve(D_CODEC_ID_XZ, &in, &out) ==
               D_PACK_STATUS_OK);

    D_CO_CHECK(out.lzma.extreme  != D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.lzma.check    != D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.lzma.threads  != D_COMPRESS_KNOB_UNSET);

    D_CO_CHECK(out.lzma.lc       == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.lzma.lp       == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.lzma.pb       == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.lzma.mode     == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.lzma.nice_len == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.lzma.mf       == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(out.lzma.depth    == D_COMPRESS_KNOB_UNSET);

    D_CO_CHECK(out.lzma.dict_size == (1 << 20));
    D_CO_CHECK(zstd_options_equal(out.zstd, in.zstd));

    return true;
}

/*
tests_compress_options_lzma_fields
  Confirms each knob is a distinct, writable member - assigning one leaves the
  others untouched (no aliasing or packing surprise).
  Tests the following:
  - each written knob reads back its value
  - every knob not yet written is still UNSET
*/
bool tests_compress_options_lzma_fields()
{
    compress_options o;

    o.lzma.check = lzma_check_sha256;
    D_CO_CHECK(o.lzma.check == lzma_check_sha256);
    D_CO_CHECK(o.lzma.extreme == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.dict_size == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lc == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lp == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.pb == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.mode == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.nice_len == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.mf == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.depth == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.threads == D_COMPRESS_KNOB_UNSET);

    o.lzma.dict_size = (1 << 20);
    D_CO_CHECK(o.lzma.check == lzma_check_sha256);
    D_CO_CHECK(o.lzma.dict_size == (1 << 20));
    D_CO_CHECK(o.lzma.extreme == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lc == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lp == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.pb == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.mode == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.nice_len == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.mf == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.depth == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.threads == D_COMPRESS_KNOB_UNSET);

    o.lzma.mode = lzma_mode_normal;
    D_CO_CHECK(o.lzma.check == lzma_check_sha256);
    D_CO_CHECK(o.lzma.dict_size == (1 << 20));
    D_CO_CHECK(o.lzma.mode == lzma_mode_normal);
    D_CO_CHECK(o.lzma.extreme == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lc == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lp == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.pb == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.nice_len == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.mf == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.depth == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.threads == D_COMPRESS_KNOB_UNSET);

    o.lzma.mf = lzma_mf_bt4;
    D_CO_CHECK(o.lzma.check == lzma_check_sha256);
    D_CO_CHECK(o.lzma.dict_size == (1 << 20));
    D_CO_CHECK(o.lzma.mode == lzma_mode_normal);
    D_CO_CHECK(o.lzma.mf == lzma_mf_bt4);
    D_CO_CHECK(o.lzma.extreme == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lc == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.lp == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.pb == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.nice_len == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.depth == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.lzma.threads == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_lzma_layout
  Confirms DLzmaOptions is portable plain data of exactly its knobs: 11
  int32_t fields, no padding, so the C and C++ faces share one layout.
  Tests the following:
  - the struct is trivially copyable, standard-layout, trivially destructible
    and non-polymorphic
  - its size is 11 int32_t
*/
bool tests_compress_options_lzma_layout()
{
    D_CO_CHECK(pod_ish_layout<DLzmaOptions>());
    D_CO_CHECK(sizeof(DLzmaOptions) == (11 * sizeof(int32_t)));

    return true;
}


NS_END  // testing
NS_END  // djinterp
