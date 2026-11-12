/*******************************************************************************
* djinterp [test]                                     compress_tests_options.cpp
*
*   Tests for how compress.cpp consumes compress_options.  Each backend maps
* the generic `level` onto its own scale and clamps out-of-range values to a
* safe default; these tests drive both the in-range and the clamped branches of
* that mapping (compress.cpp sections III-VI) by round-tripping a payload at a
* spread of levels, including deliberately out-of-range ones that must still
* succeed.  Also pins the generic default (level == -1) and confirms store
* ignores the options entirely.
*
*
* path:      /tests/djinterp/cpp/util/compress/compress_tests_options.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.10
*******************************************************************************/

// std
#include <cstddef>
// djinterp
#include "./compress_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // levels_roundtrip
    //   helper: if _Codec is available, round-tripping a compressible payload
    // must succeed at every level in _levels, each a value on the generic
    // 0..9 scale or UNSET (the core's default), which the kernel maps onto the
    // codec's own scale. Vacuous when the codec is absent.
    template<typename _Codec>
    bool levels_roundtrip(
        const int*  _levels,
        std::size_t _n
    )
    {
        if (!test::is_available<_Codec>())
        {
            return true;
        }

        byte_blob in = test::make_text(3000);
        std::size_t i;

        for (i = 0; i < _n; ++i)
        {
            compress_options opt;

            opt.level = _levels[i];

            if (!test::roundtrips<_Codec>(in, opt))
            {
                return false;
            }
        }

        return true;
    }

    // levels_refused
    //   helper: if _Codec is available, every level in _levels -- each off
    // the generic scale -- is refused with status_invalid_argument before
    // the backend sees it. Vacuous when the codec is absent.
    template<typename _Codec>
    bool levels_refused(
        const int*  _levels,
        std::size_t _n
    )
    {
        if (!test::is_available<_Codec>())
        {
            return true;
        }

        byte_blob in = test::make_text(300);
        std::size_t i;

        for (i = 0; i < _n; ++i)
        {
            compress_options opt;
            byte_blob        out;

            opt.level = _levels[i];

            if (try_compress<_Codec>(in, out, opt) != status_invalid_argument)
            {
                return false;
            }
        }

        return true;
    }

    // the levels off the generic scale every codec refuses
    const int k_off_scale[] = { -1, 10, 15, 99 };
    const std::size_t k_off_scale_n =
        (sizeof(k_off_scale) / sizeof(k_off_scale[0]));
}  // namespace


/*
tests_compress_options_defaults
  Pins the generic option default: every knob of a default-constructed
  compress_options is UNSET (not a number), which the core resolves to its own
  pinned default at dispatch.
  Tests the following:
  - a default-constructed compress_options has level == D_COMPRESS_KNOB_UNSET
  - default options round-trip on the always-present store codec
*/
bool tests_compress_options_defaults()
{
    compress_options opt;

    D_CP_CHECK(opt.level == D_COMPRESS_KNOB_UNSET);

    byte_blob in = test::make_text(300);
    D_CP_CHECK(test::roundtrips<codecs::store>(in, opt));

    return true;
}

/*
tests_compress_options_deflate_levels
  Exercises the zlib family on the generic scale: UNSET selects the default,
  0..9 pass through, and anything else is refused.
  Tests the following:
  - round-trips succeed at UNSET and 0..9 for deflate, zlib, and gzip when the
    backend is available
  - -1, 10, 15 and 99 are refused with status_invalid_argument
*/
bool tests_compress_options_deflate_levels()
{
    const int levels[] = { D_COMPRESS_KNOB_UNSET,
                           0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::deflate>(levels, n));
    D_CP_CHECK(levels_roundtrip<codecs::zlib>(levels, n));
    D_CP_CHECK(levels_roundtrip<codecs::gzip>(levels, n));
    D_CP_CHECK(levels_refused<codecs::deflate>(k_off_scale, k_off_scale_n));
    D_CP_CHECK(levels_refused<codecs::zlib>(k_off_scale, k_off_scale_n));
    D_CP_CHECK(levels_refused<codecs::gzip>(k_off_scale, k_off_scale_n));

    return true;
}

/*
tests_compress_options_bzip2_block_sizes
  Exercises bzip2 on the generic scale, which the kernel maps onto its 1..9
  block sizes.
  Tests the following:
  - round-trips succeed at UNSET, 0, 1, 5 and 9 when bzip2 is available
  - -1, 10, 15 and 99 are refused with status_invalid_argument
*/
bool tests_compress_options_bzip2_block_sizes()
{
    const int levels[] = { D_COMPRESS_KNOB_UNSET, 0, 1, 5, 9 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::bzip2>(levels, n));
    D_CP_CHECK(levels_refused<codecs::bzip2>(k_off_scale, k_off_scale_n));

    return true;
}

/*
tests_compress_options_xz_levels
  Exercises xz on the generic scale, which is its preset scale.
  Tests the following:
  - round-trips succeed at UNSET, 0, 6 and 9 when xz is available
  - -1, 10, 15 and 99 are refused with status_invalid_argument
*/
bool tests_compress_options_xz_levels()
{
    const int levels[] = { D_COMPRESS_KNOB_UNSET, 0, 6, 9 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::xz>(levels, n));
    D_CP_CHECK(levels_refused<codecs::xz>(k_off_scale, k_off_scale_n));

    return true;
}

/*
tests_compress_options_zstd_levels
  Exercises zstd on the generic scale, which the kernel maps onto zstd's own.
  Tests the following:
  - round-trips succeed at UNSET, 0, 1, 3 and 9 when zstd is available
  - -1, 10, 15 and 99 are refused with status_invalid_argument
*/
bool tests_compress_options_zstd_levels()
{
    const int levels[] = { D_COMPRESS_KNOB_UNSET, 0, 1, 3, 9 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::zstd>(levels, n));
    D_CP_CHECK(levels_refused<codecs::zstd>(k_off_scale, k_off_scale_n));

    return true;
}

/*
tests_compress_options_brotli_levels
  Exercises Brotli on the generic scale, which the kernel maps onto Brotli's
  0..11 quality (9 is Brotli's 11: "best" means best on either scale).
  Tests the following:
  - round-trips succeed at UNSET, 0, 5 and 9 when brotli is available
  - -1, 10, 15 and 99 are refused with status_invalid_argument
*/
bool tests_compress_options_brotli_levels()
{
    const int levels[] = { D_COMPRESS_KNOB_UNSET, 0, 5, 9 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::brotli>(levels, n));
    D_CP_CHECK(levels_refused<codecs::brotli>(k_off_scale, k_off_scale_n));

    return true;
}

/*
tests_compress_options_store_ignores_level
  Confirms the store codec disregards a valid level - its output is the
  verbatim input whatever level is requested - while a level off the generic
  scale is refused for store as for every codec: the kernel validates the
  options before it dispatches, so a bad option set fails the same way
  whichever codec it is handed to.
  Tests the following:
  - store output is byte-identical across UNSET, 0, 5 and 9
  - that output equals the input (verbatim) in every case
  - level 999 is refused with status_invalid_argument
*/
bool tests_compress_options_store_ignores_level()
{
    byte_blob in = test::make_pattern(500);
    byte_blob ref;

    D_CP_CHECK(try_compress<codecs::store>(in, ref) == status_ok);

    const int levels[] = { D_COMPRESS_KNOB_UNSET, 0, 5, 9 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));
    std::size_t       i;

    for (i = 0; i < n; ++i)
    {
        compress_options opt;
        byte_blob      out;

        opt.level = levels[i];

        D_CP_CHECK(try_compress<codecs::store>(in, out, opt) == status_ok);
        D_CP_CHECK(out == ref);   // level made no difference
        D_CP_CHECK(out == in);    // and the copy is verbatim
    }

    {
        compress_options opt;
        byte_blob        out;

        opt.level = 999;

        D_CP_CHECK(try_compress<codecs::store>(in, out, opt) ==
                   status_invalid_argument);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
