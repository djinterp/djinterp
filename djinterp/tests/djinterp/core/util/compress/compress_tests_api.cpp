/*******************************************************************************
* djinterp [test]                                         compress_tests_api.cpp
*
*   Tests for the non-throwing template API and the dispatch leaves
* (compress.hpp section VII; compress.cpp section VII).  Covers the argument
* guard, the unavailable / out-of-range arms, the decode error branches of the
* real backends (garbage and truncation), and a deterministically forced
* backend error (an out-of-range compression level rejected by the zlib
* encoder).
*
*   COVERAGE NOTE - defensively unreachable lines.
*   A handful of statements in compress.cpp guard against conditions that the
* public surface cannot produce, and so are not (and cannot be) exercised from
* here:
*   - store_copy's own null/size guard and its `_n > max_size()` overflow guard
*     (the dispatch leaf validates the pointer first, and no allocation can
*     exceed std::string::max_size()).
*   - the "encoder init failed" / "did not reach stream end" arms for VALID
*     inputs, since deflateBound sizes the output to guarantee completion (the
*     init-failure arm IS reached here by forcing a bad level, below).
*   - the grow-and-retry EXHAUSTION returns (bzip2 / xz / brotli / lz4) and the
*     zlib_window_bits default arm, reachable only with inputs or ids the facade
*     never forms.
*   - zstd's CONTENTSIZE_UNKNOWN -> status_unsupported arm, which needs a
*     streaming-produced frame with no content-size header (the one-shot encoder
*     always writes one).
*   Everything else in the file is covered by this suite.
*
*
* path:      /tests/djinterp/core/util/compress/compress_tests_api.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstddef>
#include <string>
// djinterp
#include "./compress_tests.hpp"


NS_DJINTERP
NS_TESTING


namespace
{
    // garbage_rejected
    //   helper: if _Codec is available, decoding a clearly-invalid buffer must
    // fail (any non-ok status), never falsely report success.  The buffer opens
    // with 0xFF bytes, which are invalid as a leading block / header for every
    // container the facade emits (including raw DEFLATE), so the decoder errors
    // promptly.  When the codec is unavailable the check is vacuous.
    template<typename _Codec>
    bool
    garbage_rejected()
    {
        if (!test::is_available<_Codec>())
        {
            return true;
        }

        byte_blob garbage(64u, static_cast<char>(0xFF));
        byte_blob out;
        status      s = try_decompress<_Codec>(garbage, out);

        return (s != status_ok);
    }
}  // namespace


/*
tests_compress_api_template_dispatch
  Confirms the non-throwing template entry points forward to the dispatch leaf
  with the correct codec id - try_compress<Codec> is equivalent to calling
  compress_buffer with codec_traits<Codec>::id().
  Tests the following:
  - try_compress<store> matches compress_buffer(codec_id_store, ...) in status
    and output
  - try_decompress<store> matches decompress_buffer(codec_id_store, ...)
*/
bool
tests_compress_api_template_dispatch()
{
    byte_blob      in = test::make_text(400);
    byte_blob      via_template;
    byte_blob      via_leaf;
    compress_options opt;
    status           ts;
    status           ls;

    // compress: template vs. explicit leaf with the mapped id
    ts = try_compress<codecs::store>(in, via_template, opt);
    ls = ::djinterp::internal::compress_buffer(
             codec_traits<codecs::store>::id(),
             in.data(), in.size(), opt, via_leaf);

    D_CP_CHECK(ts == ls);
    D_CP_CHECK(via_template == via_leaf);

    // decompress: same equivalence
    byte_blob dt;
    byte_blob dl;

    ts = try_decompress<codecs::store>(via_template, dt);
    ls = ::djinterp::internal::decompress_buffer(
             codec_traits<codecs::store>::id(),
             via_leaf.data(), via_leaf.size(), dl);

    D_CP_CHECK(ts == ls);
    D_CP_CHECK(dt == dl);
    D_CP_CHECK(dt == in);

    return true;
}

/*
tests_compress_api_invalid_argument_compress
  Confirms compress_buffer rejects a null pointer with a non-zero length before
  dispatching to any backend, for several codec ids (the guard precedes the
  switch, so the id is immaterial).
  Tests the following:
  - compress_buffer(id, null, n>0) returns status_invalid_argument for store,
    a real codec, and an out-of-range id alike
*/
bool
tests_compress_api_invalid_argument_compress()
{
    byte_blob      out;
    compress_options opt;

    D_CP_CHECK(::djinterp::internal::compress_buffer(
                   codec_id_store, static_cast<const char*>(0), 7u, opt, out)
               == status_invalid_argument);
    D_CP_CHECK(::djinterp::internal::compress_buffer(
                   codec_id_gzip, static_cast<const char*>(0), 7u, opt, out)
               == status_invalid_argument);
    D_CP_CHECK(::djinterp::internal::compress_buffer(
                   static_cast<codec_id>(9999),
                   static_cast<const char*>(0), 7u, opt, out)
               == status_invalid_argument);

    return true;
}

/*
tests_compress_api_invalid_argument_decompress
  Confirms decompress_buffer applies the same null/size guard as compress.
  Tests the following:
  - decompress_buffer(id, null, n>0) returns status_invalid_argument for store,
    a real codec, and an out-of-range id alike
*/
bool
tests_compress_api_invalid_argument_decompress()
{
    byte_blob out;

    D_CP_CHECK(::djinterp::internal::decompress_buffer(
                   codec_id_store, static_cast<const char*>(0), 7u, out)
               == status_invalid_argument);
    D_CP_CHECK(::djinterp::internal::decompress_buffer(
                   codec_id_gzip, static_cast<const char*>(0), 7u, out)
               == status_invalid_argument);
    D_CP_CHECK(::djinterp::internal::decompress_buffer(
                   static_cast<codec_id>(9999),
                   static_cast<const char*>(0), 7u, out)
               == status_invalid_argument);

    return true;
}

/*
tests_compress_api_out_of_range_unavailable
  Confirms an unknown codec id lands on the dispatch switch default and reports
  the codec unavailable rather than misbehaving - in both directions, with
  valid (non-null) input so the argument guard is passed first.
  Tests the following:
  - compress_buffer(out-of-range id, valid input) returns status_unavailable
  - decompress_buffer(out-of-range id, valid input) returns status_unavailable
*/
bool
tests_compress_api_out_of_range_unavailable()
{
    byte_blob      in = test::make_text(64);
    byte_blob      out;
    compress_options opt;
    codec_id         bogus = static_cast<codec_id>(9999);

    D_CP_CHECK(::djinterp::internal::compress_buffer(
                   bogus, in.data(), in.size(), opt, out)
               == status_unavailable);
    D_CP_CHECK(::djinterp::internal::decompress_buffer(
                   bogus, in.data(), in.size(), out)
               == status_unavailable);

    return true;
}

/*
tests_compress_api_decompress_garbage
  Confirms every available real codec (except brotli, isolated below) rejects a
  clearly-invalid input with an error status rather than a crash or a false
  success - exercising the backends' decode error branches.
  Tests the following:
  - decoding invalid bytes under deflate / zlib / gzip / bzip2 / xz / zstd / lz4
    yields a non-ok status when the codec is available
*/
bool
tests_compress_api_decompress_garbage()
{
    D_CP_CHECK(garbage_rejected<codecs::deflate>());
    D_CP_CHECK(garbage_rejected<codecs::zlib>());
    D_CP_CHECK(garbage_rejected<codecs::gzip>());
    D_CP_CHECK(garbage_rejected<codecs::bzip2>());
    D_CP_CHECK(garbage_rejected<codecs::xz>());
    D_CP_CHECK(garbage_rejected<codecs::zstd>());
    D_CP_CHECK(garbage_rejected<codecs::lz4>());

    return true;
}

/*
tests_compress_api_decompress_garbage_brotli
  Confirms Brotli rejects invalid input.  Isolated from the sweep above because
  Brotli's one-shot decode cannot distinguish "error" from "needs more output",
  so the facade grows and retries its output buffer on failure (up to a bounded
  ceiling) before returning an error - a transient allocation the other codecs
  do not make.
  Tests the following:
  - decoding invalid bytes under brotli yields a non-ok status when available
*/
bool
tests_compress_api_decompress_garbage_brotli()
{
    // a short invalid input keeps the retry buffer's starting size small
    if (!test::is_available<codecs::brotli>())
    {
        return true;
    }

    byte_blob garbage(16u, static_cast<char>(0xFF));
    byte_blob out;

    D_CP_CHECK(try_decompress<codecs::brotli>(garbage, out) != status_ok);

    return true;
}

/*
tests_compress_api_deflate_bad_level
  Confirms an out-of-range compression level is surfaced as a backend error -
  the zlib encoder rejects a level above 9, exercising the "encoder init
  failed" arm.  All three zlib-family codecs share the availability of the zlib
  backend, so all three are checked together.
  Tests the following:
  - compressing with an out-of-range level under deflate / zlib / gzip yields
    status_backend_error when the zlib backend is available
*/
bool
tests_compress_api_deflate_bad_level()
{
    if (!test::is_available<codecs::deflate>())
    {
        return true;
    }

    byte_blob      in = test::make_text(200);
    byte_blob      out;
    compress_options opt;

    // above zlib's valid 0..9 (and not the -1 "default" sentinel)
    opt.level = 999;

    D_CP_CHECK(try_compress<codecs::deflate>(in, out, opt) == status_backend_error);
    D_CP_CHECK(try_compress<codecs::zlib>(in, out, opt)    == status_backend_error);
    D_CP_CHECK(try_compress<codecs::gzip>(in, out, opt)    == status_backend_error);

    return true;
}

/*
tests_compress_api_zlib_truncated
  Confirms a truncated stream is reported as an error, not a partial success -
  exercising the decoder's "ran out of input before the stream ended" path for
  all three zlib-family framings.
  Tests the following:
  - decoding a half-length prefix of a valid stream yields a non-ok status
  - decoding a 3-byte prefix likewise fails
  - this holds for zlib, gzip, and raw deflate when the backend is available
*/
bool
tests_compress_api_zlib_truncated()
{
    if (!test::is_available<codecs::zlib>())
    {
        return true;
    }

    // a payload large enough that its compressed form is safely multi-byte
    byte_blob in = test::make_text(5000);

    // zlib framing
    {
        byte_blob packed;
        byte_blob out;

        D_CP_CHECK(try_compress<codecs::zlib>(in, packed) == status_ok);
        D_CP_CHECK(packed.size() > 6u);
        D_CP_CHECK(try_decompress<codecs::zlib>(
                       packed.substr(0, packed.size() / 2u), out) != status_ok);
        D_CP_CHECK(try_decompress<codecs::zlib>(
                       packed.substr(0, 3u), out) != status_ok);
    }

    // gzip framing (has a trailer; truncation drops it)
    {
        byte_blob packed;
        byte_blob out;

        D_CP_CHECK(try_compress<codecs::gzip>(in, packed) == status_ok);
        D_CP_CHECK(try_decompress<codecs::gzip>(
                       packed.substr(0, packed.size() / 2u), out) != status_ok);
    }

    // raw deflate framing
    {
        byte_blob packed;
        byte_blob out;

        D_CP_CHECK(try_compress<codecs::deflate>(in, packed) == status_ok);
        D_CP_CHECK(try_decompress<codecs::deflate>(
                       packed.substr(0, packed.size() / 2u), out) != status_ok);
    }

    return true;
}


NS_END  // testing
NS_END  // djinterp
