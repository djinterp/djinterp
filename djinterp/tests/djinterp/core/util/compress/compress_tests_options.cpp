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
* path:      /tests/djinterp/core/util/compress/compress_tests_options.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
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
    // must succeed at every level in _levels - the in-range values pass
    // through, and any out-of-range values must be clamped by the backend to a
    // working default (never rejected).  Vacuous when the codec is absent.
    template<typename _Codec>
    bool
    levels_roundtrip(
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
}  // namespace


/*
tests_compress_options_defaults
  Pins the generic option default that every backend keys its "use the backend
  default effort" branch on.
  Tests the following:
  - a default-constructed compress_options has level == -1
  - default options round-trip on the always-present store codec
*/
bool
tests_compress_options_defaults()
{
    compress_options opt;

    D_CP_CHECK(opt.level == -1);

    byte_blob in = test::make_text(300);
    D_CP_CHECK(test::roundtrips<codecs::store>(in, opt));

    return true;
}

/*
tests_compress_options_deflate_levels
  Exercises the zlib-family level mapping: the -1 sentinel selects the backend
  default, and 0..9 pass through.  (Values above 9 are rejected - covered in
  the api suite - so they are not included here.)
  Tests the following:
  - round-trips succeed at levels -1 and 0..9 for deflate, zlib, and gzip when
    the backend is available
*/
bool
tests_compress_options_deflate_levels()
{
    const int levels[] = { -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::deflate>(levels, n));
    D_CP_CHECK(levels_roundtrip<codecs::zlib>(levels, n));
    D_CP_CHECK(levels_roundtrip<codecs::gzip>(levels, n));

    return true;
}

/*
tests_compress_options_bzip2_block_sizes
  Exercises bzip2's block-size mapping, which clamps anything outside 1..9 to
  the maximum.
  Tests the following:
  - round-trips succeed at in-range levels 1, 5, 9
  - round-trips succeed at out-of-range levels -1, 0, 15 (clamped to 9) when
    bzip2 is available
*/
bool
tests_compress_options_bzip2_block_sizes()
{
    const int levels[] = { -1, 0, 1, 5, 9, 15 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::bzip2>(levels, n));

    return true;
}

/*
tests_compress_options_xz_levels
  Exercises xz's preset mapping, which clamps anything outside 0..9 to preset 6.
  Tests the following:
  - round-trips succeed at in-range levels 0, 6, 9
  - round-trips succeed at out-of-range levels -1, 15 (clamped to 6) when xz is
    available
*/
bool
tests_compress_options_xz_levels()
{
    const int levels[] = { -1, 0, 6, 9, 15 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::xz>(levels, n));

    return true;
}

/*
tests_compress_options_zstd_levels
  Exercises zstd's level mapping, where the -1 sentinel selects the backend
  default and other values pass through zstd's own (internally clamped) range.
  Tests the following:
  - round-trips succeed at levels -1, 0, 1, 3, 9, 19 when zstd is available
*/
bool
tests_compress_options_zstd_levels()
{
    const int levels[] = { -1, 0, 1, 3, 9, 19 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::zstd>(levels, n));

    return true;
}

/*
tests_compress_options_brotli_levels
  Exercises Brotli's quality mapping, which clamps anything outside 0..11 to the
  default quality.
  Tests the following:
  - round-trips succeed at in-range qualities 0, 5, 11
  - round-trips succeed at out-of-range values -1, 99 (clamped) when brotli is
    available
*/
bool
tests_compress_options_brotli_levels()
{
    const int levels[] = { -1, 0, 5, 11, 99 };
    const std::size_t n = (sizeof(levels) / sizeof(levels[0]));

    D_CP_CHECK(levels_roundtrip<codecs::brotli>(levels, n));

    return true;
}

/*
tests_compress_options_store_ignores_level
  Confirms the store codec disregards the options - its output is the verbatim
  input no matter what level is requested, including an absurd one.
  Tests the following:
  - store output is byte-identical across a spread of levels
  - that output equals the input (verbatim) in every case
*/
bool
tests_compress_options_store_ignores_level()
{
    byte_blob in = test::make_pattern(500);
    byte_blob ref;

    D_CP_CHECK(try_compress<codecs::store>(in, ref) == status_ok);

    const int levels[] = { -1, 0, 5, 9, 999 };
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

    return true;
}


NS_END  // testing
NS_END  // djinterp
