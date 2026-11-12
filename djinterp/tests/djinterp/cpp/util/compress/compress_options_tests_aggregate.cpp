/*******************************************************************************
* djinterp [test]                           compress_options_tests_aggregate.cpp
*
*   Tests for compress_options itself (compress_options.hpp section II): the
* generic effort, the six blocks composed together, value semantics and ==,
* the layout it shares with the core, and the UNSET sentinel.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_options_tests_aggregate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
*******************************************************************************/

// djinterp
#include "./compress_options_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_compress_options_aggregate_level_default
  Confirms the generic effort of a default-constructed compress_options is
  UNSET, which the core resolves to its pinned default level at dispatch.
  Tests the following:
  - a default-constructed compress_options has level == D_COMPRESS_KNOB_UNSET
*/
bool tests_compress_options_aggregate_level_default()
{
    compress_options o;

    D_CO_CHECK(o.level == D_COMPRESS_KNOB_UNSET);

    return true;
}

/*
tests_compress_options_aggregate_subobjects_defaulted
  Confirms the constructor leaves no knob uninitialised: every one of the
  fifty, generic effort and all six blocks, is UNSET, walked through the
  core's own knob table so a knob added later is covered without an edit
  here.
  Tests the following:
  - each of the D_COMPRESS_KNOB_COUNT knobs reads back UNSET
  - is_default() and compress_options_are_default() both say so
*/
bool tests_compress_options_aggregate_subobjects_defaulted()
{
    compress_options o;
    int              k;

    for (k = 0; k < D_COMPRESS_KNOB_COUNT; ++k)
    {
        const enum dj_compress_knob knob =
            static_cast<enum dj_compress_knob>(k);

        D_CO_CHECK(d_compress_options_knob_get(&o, knob) ==
                   D_COMPRESS_KNOB_UNSET);
    }

    D_CO_CHECK(o.is_default());
    D_CO_CHECK(compress_options_are_default(o));

    return true;
}

/*
tests_compress_options_aggregate_independence
  Confirms the per-codec blocks within one compress_options are independent -
  tuning one codec never perturbs another's block.
  Tests the following:
  - writing into one block leaves the others UNSET
  - a second write into a different block preserves the first
  - the set is no longer pristine once anything is written
*/
bool tests_compress_options_aggregate_independence()
{
    compress_options o;

    o.deflate.window_bits = 15;
    D_CO_CHECK(o.deflate.window_bits   == 15);
    D_CO_CHECK(o.bzip2.block_size_100k == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.zstd.window_log       == D_COMPRESS_KNOB_UNSET);
    D_CO_CHECK(o.brotli.quality        == D_COMPRESS_KNOB_UNSET);

    o.zstd.window_log = 27;
    D_CO_CHECK(o.zstd.window_log     == 27);
    D_CO_CHECK(o.deflate.window_bits == 15);
    D_CO_CHECK(o.lzma.lc             == D_COMPRESS_KNOB_UNSET);

    D_CO_CHECK(!o.is_default());

    return true;
}

/*
tests_compress_options_aggregate_value_semantics
  Confirms compress_options is a value type - copy construction and copy
  assignment produce independent copies - and that its == and != are the
  core's knob-table comparison.
  Tests the following:
  - a copy carries the source's modified fields
  - mutating the copy leaves the source unchanged
  - copy assignment likewise produces an independent copy
  - equal sets compare ==, and a single differing knob makes them !=
*/
bool tests_compress_options_aggregate_value_semantics()
{
    compress_options a;

    a.level           = 5;
    a.zstd.window_log = 27;
    a.brotli.quality  = 11;
    a.lzma.check      = lzma_check_sha256;

    // copy construction carries every modified field
    compress_options b(a);
    D_CO_CHECK(b.level           == 5);
    D_CO_CHECK(b.zstd.window_log == 27);
    D_CO_CHECK(b.brotli.quality  == 11);
    D_CO_CHECK(b.lzma.check      == lzma_check_sha256);
    D_CO_CHECK(b == a);

    // mutating the copy does not disturb the source
    b.level           = 9;
    b.zstd.window_log = 20;
    D_CO_CHECK(a.level           == 5);
    D_CO_CHECK(a.zstd.window_log == 27);
    D_CO_CHECK(b != a);

    // copy assignment behaves the same
    compress_options c;
    c = a;
    D_CO_CHECK(c.level           == 5);
    D_CO_CHECK(c.zstd.window_log == 27);
    D_CO_CHECK(c.brotli.quality  == 11);
    c.brotli.quality = 0;
    D_CO_CHECK(a.brotli.quality  == 11);

    return true;
}

/*
tests_compress_options_aggregate_layout
  Confirms the whole set is portable plain data that costs nothing over the
  core's struct: it derives from dj_compress_options and adds no member.
  Tests the following:
  - compress_options is trivially copyable, standard-layout, trivially
    destructible and non-polymorphic
  - its size is the core struct's, which is D_COMPRESS_KNOB_COUNT int32_t
*/
bool tests_compress_options_aggregate_layout()
{
    D_CO_CHECK(pod_ish_layout<compress_options>());
    D_CO_CHECK(sizeof(compress_options) == sizeof(struct dj_compress_options));
    D_CO_CHECK(sizeof(compress_options) ==
               (D_COMPRESS_KNOB_COUNT * sizeof(int32_t)));

    return true;
}

/*
tests_compress_options_aggregate_unset_sentinel
  Confirms the sentinel that marks a knob as not set: INT32_MIN, which lies
  outside every knob's domain (no level, size or flag is that negative), so
  "unset" can never be mistaken for a value; and a knob set and then put back
  to UNSET makes the set pristine again.
  Tests the following:
  - D_COMPRESS_KNOB_UNSET is INT32_MIN
  - writing a knob makes the set non-default, restoring UNSET makes it
    default again
*/
bool tests_compress_options_aggregate_unset_sentinel()
{
    compress_options o;

    D_CO_CHECK(D_COMPRESS_KNOB_UNSET == INT32_MIN);

    o.bzip2.work_factor = 30;
    D_CO_CHECK(!o.is_default());

    o.bzip2.work_factor = D_COMPRESS_KNOB_UNSET;
    D_CO_CHECK(o.is_default());

    return true;
}


NS_END  // testing
NS_END  // djinterp
