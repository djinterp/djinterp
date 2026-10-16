/*******************************************************************************
* djinterp [test]                                    compress_tests_throwing.cpp
*
*   Tests for the throwing convenience API - compress<Codec> / decompress<Codec>
* and the compression_error type (compress.hpp section, gated on
* D_ENV_COMPRESSION_HAS_EXCEPTIONS).  When exceptions are disabled in the build
* the throwing API is not compiled, so every test here degrades to a trivial
* pass; the declarations remain so the runner links unchanged.
*
*   The failure cases are made deterministic across codec configurations: an
* out-of-range level makes compress<deflate> fail on every build (a backend
* error where the zlib backend is present, an unavailable error where it is
* not), and invalid input makes decompress<deflate> fail likewise - so both
* always throw, and both messages always name the operation and codec.  Each
* test that expects a throw catches it locally, so nothing escapes to the
* runner.
*
*
* path:      /tests/djinterp/core/util/compress/compress_tests_throwing.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <cstring>
#include <string>
// djinterp
#include "./compress_tests.hpp"
// note: <stdexcept> is included by compress.hpp when exceptions are enabled


NS_DJINTERP
NS_TESTING


#if D_ENV_COMPRESSION_HAS_EXCEPTIONS

namespace
{
    // throwing_roundtrip_if_available
    //   helper: if _Codec is available, a compress/decompress cycle through the
    // THROWING API must reproduce the input and raise nothing; any exception is
    // a failure.  Vacuous when the codec is absent.
    template<typename _Codec>
    bool
    throwing_roundtrip_if_available(
        const byte_blob& _in
    )
    {
        if (!test::is_available<_Codec>())
        {
            return true;
        }

        try
        {
            byte_blob c = compress<_Codec>(_in);
            byte_blob d = decompress<_Codec>(c);

            return (d == _in);
        }
        catch (...)
        {
            return false;
        }
    }
}  // namespace

#endif  // D_ENV_COMPRESSION_HAS_EXCEPTIONS


/*
tests_compress_throwing_success_matches_try
  Confirms the throwing API returns exactly what the non-throwing API produces
  on success - it is a thin wrapper, not a distinct implementation.
  Tests the following:
  - compress<store> returns the same bytes try_compress<store> writes
  - decompress<store> returns the same bytes try_decompress<store> writes, which
    equal the original input
  (trivial pass when exceptions are disabled)
*/
bool
tests_compress_throwing_success_matches_try()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    byte_blob in = test::make_text(400);
    byte_blob via_try;
    byte_blob dec_try;

    D_CP_CHECK(try_compress<codecs::store>(in, via_try) == status_ok);

    try
    {
        byte_blob via_throw = compress<codecs::store>(in);
        D_CP_CHECK(via_throw == via_try);

        D_CP_CHECK(try_decompress<codecs::store>(via_try, dec_try) == status_ok);

        byte_blob dec_throw = decompress<codecs::store>(via_throw);
        D_CP_CHECK(dec_throw == dec_try);
        D_CP_CHECK(dec_throw == in);
    }
    catch (...)
    {
        return false;   // the success path must not throw
    }

    return true;
#else
    return true;
#endif
}

/*
tests_compress_throwing_roundtrip
  Confirms the throwing API round-trips every available codec.
  Tests the following:
  - compress then decompress via the throwing API reproduces the input for each
    available codec, raising nothing
  (trivial pass when exceptions are disabled)
*/
bool
tests_compress_throwing_roundtrip()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    byte_blob in = test::make_text(2000);

    D_CP_CHECK(throwing_roundtrip_if_available<codecs::store>(in));
    D_CP_CHECK(throwing_roundtrip_if_available<codecs::deflate>(in));
    D_CP_CHECK(throwing_roundtrip_if_available<codecs::zlib>(in));
    D_CP_CHECK(throwing_roundtrip_if_available<codecs::gzip>(in));
    D_CP_CHECK(throwing_roundtrip_if_available<codecs::bzip2>(in));
    D_CP_CHECK(throwing_roundtrip_if_available<codecs::xz>(in));
    D_CP_CHECK(throwing_roundtrip_if_available<codecs::zstd>(in));
    D_CP_CHECK(throwing_roundtrip_if_available<codecs::lz4>(in));
    D_CP_CHECK(throwing_roundtrip_if_available<codecs::brotli>(in));

    return true;
#else
    return true;
#endif
}

/*
tests_compress_throwing_throws_on_failure
  Confirms compress<Codec> raises compression_error on a non-success status.
  Uses an out-of-range level on deflate, which fails on every build (a backend
  error with the zlib backend present, an unavailable error without it).
  Tests the following:
  - compress<deflate> with an out-of-range level throws compression_error
  (trivial pass when exceptions are disabled)
*/
bool
tests_compress_throwing_throws_on_failure()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    byte_blob      in = test::make_text(200);
    compress_options opt;
    bool             threw = false;

    opt.level = 999;

    try
    {
        byte_blob out = compress<codecs::deflate>(in, opt);
        (void)out;   // unreachable on a correct facade
    }
    catch (const compression_error&)
    {
        threw = true;
    }

    D_CP_CHECK(threw);

    return true;
#else
    return true;
#endif
}

/*
tests_compress_throwing_what_message
  Confirms the exception message identifies the failing operation and codec, in
  the documented "compress<name>: <status>" shape.
  Tests the following:
  - the thrown what() is non-null and contains "compress<deflate>"
  - it is longer than the prefix alone (a status description follows)
  (trivial pass when exceptions are disabled)
*/
bool
tests_compress_throwing_what_message()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    byte_blob      in = test::make_text(200);
    compress_options opt;
    bool             checked = false;

    opt.level = 999;

    try
    {
        byte_blob out = compress<codecs::deflate>(in, opt);
        (void)out;
    }
    catch (const compression_error& e)
    {
        const char* w = e.what();

        D_CP_CHECK(w != 0);
        D_CP_CHECK(std::strstr(w, "compress<deflate>") != 0);
        D_CP_CHECK(std::strlen(w) > std::strlen("compress<deflate>: "));

        checked = true;
    }

    D_CP_CHECK(checked);

    return true;
#else
    return true;
#endif
}

/*
tests_compress_throwing_error_type
  Confirms compression_error sits in the standard exception hierarchy and
  preserves the message it is constructed with.
  Tests the following:
  - the thrown error is catchable as std::runtime_error and as std::exception
  - compression_error(msg).what() returns msg verbatim
  (trivial pass when exceptions are disabled)
*/
bool
tests_compress_throwing_error_type()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    byte_blob      in = test::make_text(50);
    compress_options opt;
    bool             as_runtime   = false;
    bool             as_exception = false;

    opt.level = 999;

    // catchable as the runtime_error base
    try
    {
        byte_blob o = compress<codecs::deflate>(in, opt);
        (void)o;
    }
    catch (const std::runtime_error&)
    {
        as_runtime = true;
    }
    D_CP_CHECK(as_runtime);

    // catchable as the exception base
    try
    {
        byte_blob o = compress<codecs::deflate>(in, opt);
        (void)o;
    }
    catch (const std::exception&)
    {
        as_exception = true;
    }
    D_CP_CHECK(as_exception);

    // the message is preserved verbatim through construction
    {
        compression_error probe("sentinel-message");
        D_CP_CHECK(std::strcmp(probe.what(), "sentinel-message") == 0);
    }

    return true;
#else
    return true;
#endif
}

/*
tests_compress_throwing_decompress_throws
  Confirms decompress<Codec> raises compression_error on invalid input, with a
  message naming the decompress operation.  Invalid bytes make decompress<deflate>
  fail on every build (a backend error with the backend present, an unavailable
  error without it).
  Tests the following:
  - decompress<deflate> on invalid input throws compression_error
  - the message contains "decompress<deflate>"
  (trivial pass when exceptions are disabled)
*/
bool
tests_compress_throwing_decompress_throws()
{
#if D_ENV_COMPRESSION_HAS_EXCEPTIONS
    byte_blob garbage(64u, static_cast<char>(0xFF));
    bool        threw = false;

    try
    {
        byte_blob out = decompress<codecs::deflate>(garbage);
        (void)out;
    }
    catch (const compression_error& e)
    {
        threw = true;
        D_CP_CHECK(std::strstr(e.what(), "decompress<deflate>") != 0);
    }

    D_CP_CHECK(threw);

    return true;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
