/*******************************************************************************
* djinterp [test]                                   compress_tests_roundtrip.cpp
*
*   The reproduction battery.  For every codec, running the shared corpus
* (empty, tiny, text, long run, byte-cycle, medium and large prose, medium and
* large high-entropy, NUL-bearing) through compress then decompress must
* reproduce the input exactly (compress.cpp sections III-VI, reached via the
* dispatch leaves in section VII).  Each codec is gated on runtime
* availability: when present it is round-tripped for real AND its produced
* stream is checked to shrink redundant input and to carry the framing the
* container mandates; when absent the facade is asserted to report the codec
* unavailable in both directions.  So this file exercises both the available
* and the unavailable dispatch arms, whichever apply to the current build.
*
*
* path:      /tests/djinterp/core/util/compress/compress_tests_roundtrip.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <vector>
// djinterp
#include "./compress_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // sweep_codec
    //   helper: the per-codec body shared by every real-codec test below.  It
    // is only ever instantiated for the compressing codecs (never store), so
    // the "must actually shrink a long run" assertion is valid.  When the
    // codec is unavailable it asserts the facade reports so, giving the
    // unavailable dispatch arm coverage on builds lacking the library.
    template<typename _Codec>
    bool
    sweep_codec()
    {
        std::vector<byte_blob> corpus = test::standard_corpus();
        std::size_t              i;

        if (test::is_available<_Codec>())
        {
            // every corpus entry reproduces exactly, both strictly and via the
            // build-agnostic facade check
            for (i = 0; i < corpus.size(); ++i)
            {
                if (!test::roundtrips<_Codec>(corpus[i]))
                {
                    return false;
                }

                if (!test::facade_roundtrip_ok<_Codec>(corpus[i]))
                {
                    return false;
                }
            }

            // a real compressor must shrink a long, highly redundant run
            {
                byte_blob redundant = test::make_repeated('Q', 20000);
                byte_blob packed;

                if (try_compress<_Codec>(redundant, packed) != status_ok)
                {
                    return false;
                }

                if (packed.size() >= redundant.size())
                {
                    return false;
                }

                // the produced stream carries this codec's mandated framing
                // (store / raw-deflate / brotli have none and pass trivially)
                if (!test::has_expected_signature<_Codec>(packed))
                {
                    return false;
                }
            }

            return true;
        }

        // unavailable: compression and decompression both report it, and the
        // facade-correctness check agrees
        {
            byte_blob sample = test::make_text(128);
            byte_blob packed;
            byte_blob restored;

            if (try_compress<_Codec>(sample, packed) != status_unavailable)
            {
                return false;
            }

            if (try_decompress<_Codec>(sample, restored) != status_unavailable)
            {
                return false;
            }

            if (!test::facade_roundtrip_ok<_Codec>(sample))
            {
                return false;
            }
        }

        return true;
    }
}  // namespace


/*
tests_compress_roundtrip_deflate
  Reproduction battery for raw DEFLATE (RFC 1951).
  Tests the following:
  - if available: every corpus input round-trips; a long run shrinks
  - if unavailable: the facade reports status_unavailable both ways
*/
bool
tests_compress_roundtrip_deflate()
{
    return sweep_codec<codecs::deflate>();
}

/*
tests_compress_roundtrip_zlib
  Reproduction battery for zlib-wrapped DEFLATE (RFC 1950).
  Tests the following:
  - if available: every corpus input round-trips; output carries the zlib
    header
  - if unavailable: the facade reports status_unavailable both ways
*/
bool
tests_compress_roundtrip_zlib()
{
    return sweep_codec<codecs::zlib>();
}

/*
tests_compress_roundtrip_gzip
  Reproduction battery for gzip-wrapped DEFLATE (RFC 1952).
  Tests the following:
  - if available: every corpus input round-trips; output carries the gzip
    magic
  - if unavailable: the facade reports status_unavailable both ways
*/
bool
tests_compress_roundtrip_gzip()
{
    return sweep_codec<codecs::gzip>();
}

/*
tests_compress_roundtrip_bzip2
  Reproduction battery for bzip2.
  Tests the following:
  - if available: every corpus input round-trips; output carries the "BZh"
    magic
  - if unavailable: the facade reports status_unavailable both ways
*/
bool
tests_compress_roundtrip_bzip2()
{
    return sweep_codec<codecs::bzip2>();
}

/*
tests_compress_roundtrip_xz
  Reproduction battery for xz / lzma.
  Tests the following:
  - if available: every corpus input round-trips; output carries the xz magic
  - if unavailable: the facade reports status_unavailable both ways
*/
bool
tests_compress_roundtrip_xz()
{
    return sweep_codec<codecs::xz>();
}

/*
tests_compress_roundtrip_zstd
  Reproduction battery for Zstandard.
  Tests the following:
  - if available: every corpus input round-trips; output carries the zstd
    frame magic
  - if unavailable: the facade reports status_unavailable both ways
*/
bool
tests_compress_roundtrip_zstd()
{
    return sweep_codec<codecs::zstd>();
}

/*
tests_compress_roundtrip_lz4
  Reproduction battery for the LZ4 frame format.
  Tests the following:
  - if available: every corpus input round-trips; output carries the LZ4 frame
    magic
  - if unavailable: the facade reports status_unavailable both ways
*/
bool
tests_compress_roundtrip_lz4()
{
    return sweep_codec<codecs::lz4>();
}

/*
tests_compress_roundtrip_brotli
  Reproduction battery for Brotli.
  Tests the following:
  - if available: every corpus input round-trips; a long run shrinks
  - if unavailable: the facade reports status_unavailable both ways
*/
bool
tests_compress_roundtrip_brotli()
{
    return sweep_codec<codecs::brotli>();
}

/*
tests_compress_roundtrip_signatures
  Cross-checks the shared stream-signature helpers against real output and
  confirms the framings are distinct.  Each block runs only when its codec is
  available.
  Tests the following:
  - a produced stream matches its own codec's signature
  - it does NOT match a different codec's signature (framings are distinct)
  - the framing-free codecs (store / deflate / brotli) pass has_expected_signature
    unconditionally
*/
bool
tests_compress_roundtrip_signatures()
{
    byte_blob sample = test::make_text(1000);
    byte_blob packed;

    // framing-free codecs always satisfy has_expected_signature
    D_CP_CHECK(test::has_expected_signature<codecs::store>(sample));
    D_CP_CHECK(test::has_expected_signature<codecs::deflate>(sample));
    D_CP_CHECK(test::has_expected_signature<codecs::brotli>(sample));

    if (test::is_available<codecs::gzip>())
    {
        D_CP_CHECK(try_compress<codecs::gzip>(sample, packed) == status_ok);
        D_CP_CHECK(test::has_gzip_magic(packed));
        D_CP_CHECK(!test::has_zlib_magic(packed));   // gzip is not zlib
        D_CP_CHECK(!test::has_bzip2_magic(packed));
    }

    if (test::is_available<codecs::zlib>())
    {
        D_CP_CHECK(try_compress<codecs::zlib>(sample, packed) == status_ok);
        D_CP_CHECK(test::has_zlib_magic(packed));
        D_CP_CHECK(!test::has_gzip_magic(packed));   // zlib is not gzip
    }

    if (test::is_available<codecs::bzip2>())
    {
        D_CP_CHECK(try_compress<codecs::bzip2>(sample, packed) == status_ok);
        D_CP_CHECK(test::has_bzip2_magic(packed));
        D_CP_CHECK(!test::has_gzip_magic(packed));
    }

    if (test::is_available<codecs::xz>())
    {
        D_CP_CHECK(try_compress<codecs::xz>(sample, packed) == status_ok);
        D_CP_CHECK(test::has_xz_magic(packed));
    }

    if (test::is_available<codecs::zstd>())
    {
        D_CP_CHECK(try_compress<codecs::zstd>(sample, packed) == status_ok);
        D_CP_CHECK(test::has_zstd_magic(packed));
    }

    if (test::is_available<codecs::lz4>())
    {
        D_CP_CHECK(try_compress<codecs::lz4>(sample, packed) == status_ok);
        D_CP_CHECK(test::has_lz4_frame_magic(packed));
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
