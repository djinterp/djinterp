#include "test_compress_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_compress_decompresses_to_matching_payload
  the decode-and-compare helper's accepting case.
  Tests the following:
  - a stream the facade produced decodes to the payload it was given
  - this holds across the whole standard corpus for the identity codec
  - the helper agrees with the round-trip report on the same bytes
*/
bool
tests_compress_decompresses_to_matching_payload()
{
    const std::vector<dj::byte_buffer> c = dt::standard_corpus();
    std::size_t                        i = 0;

    for (i = 0; i < c.size(); ++i)
    {
        const dt::roundtrip_report r = dt::roundtrip<dj::codecs::store>(c[i]);

        D_TC_CHECK(r.compress_status == dj::status_ok);

        // the emitted bytes decode back to the payload
        if (!dt::decompresses_to<dj::codecs::store>(r.compressed, c[i]))
        {
            std::printf("    [FAIL] corpus entry %u did not decode back\n",
                        static_cast<unsigned int>(i));

            return false;
        }

        // and the helper agrees with the report
        D_TC_CHECK(dt::decompresses_to<dj::codecs::store>(r.compressed,
                                                          c[i]) ==
                   r.restored_equals_input);
    }

    return true;
}

/*
tests_compress_decompresses_to_rejects_mismatch
  the decode-and-compare helper's rejecting case.
  Tests the following:
  - a stream that decodes to different bytes is rejected
  - a single differing byte is enough, so the comparison is not a length
    check or a prefix match
  - a length difference is caught in both directions
*/
bool
tests_compress_decompresses_to_rejects_mismatch()
{
    const dj::byte_buffer      payload = dt::make_text(500);
    const dt::roundtrip_report r =
        dt::roundtrip<dj::codecs::store>(payload);

    D_TC_CHECK(r.compress_status == dj::status_ok);
    D_TC_CHECK(dt::decompresses_to<dj::codecs::store>(r.compressed, payload));

    // one byte different
    dj::byte_buffer off_by_one = payload;

    off_by_one[250] = static_cast<char>(off_by_one[250] ^ 0x01);
    D_TC_CHECK(off_by_one != payload);
    D_TC_CHECK(!dt::decompresses_to<dj::codecs::store>(r.compressed,
                                                       off_by_one));

    // a shorter expectation: a prefix must not match
    const dj::byte_buffer prefix = payload.substr(0, 499);

    D_TC_CHECK(!dt::decompresses_to<dj::codecs::store>(r.compressed, prefix));

    // a longer expectation
    dj::byte_buffer longer = payload;

    longer.push_back('!');
    D_TC_CHECK(!dt::decompresses_to<dj::codecs::store>(r.compressed, longer));

    // an entirely different payload
    D_TC_CHECK(!dt::decompresses_to<dj::codecs::store>(
                   r.compressed, dt::make_repeated('Q', 500)));

    // and a differing first byte, not just a middle one
    dj::byte_buffer first = payload;

    first[0] = static_cast<char>(first[0] ^ 0xFF);
    D_TC_CHECK(!dt::decompresses_to<dj::codecs::store>(r.compressed, first));

    return true;
}

/*
tests_compress_decompresses_to_empty_cases
  the zero-length edges of decode-and-compare.
  Tests the following:
  - an empty stream that decodes to an empty payload is accepted
  - an empty expectation against a non-empty stream is rejected
  - a non-empty expectation against an empty stream is rejected
*/
bool
tests_compress_decompresses_to_empty_cases()
{
    const dj::byte_buffer      empty;
    const dt::roundtrip_report e = dt::roundtrip<dj::codecs::store>(empty);

    D_TC_CHECK(e.compress_status == dj::status_ok);
    D_TC_CHECK(dt::decompresses_to<dj::codecs::store>(e.compressed, empty));

    // a non-empty payload's stream does not decode to nothing
    const dt::roundtrip_report r =
        dt::roundtrip<dj::codecs::store>(dt::make_text(100));

    D_TC_CHECK(!dt::decompresses_to<dj::codecs::store>(r.compressed, empty));

    // nor does an empty stream decode to something
    D_TC_CHECK(!dt::decompresses_to<dj::codecs::store>(e.compressed,
                                                       dt::make_text(100)));

    // an unavailable codec rejects both, since the decode never succeeds
    if (!dt::is_available<dj::codecs::gzip>())
    {
        D_TC_CHECK(!dt::decompresses_to<dj::codecs::gzip>(empty, empty));
    }

    return true;
}

/*
tests_compress_is_valid_stream_accepts_own_output
  the well-formedness check's accepting case.
  Tests the following:
  - a stream the facade itself produced is reported well-formed
  - the check disregards the decoded value, so it accepts a stream whose
    payload the caller does not have on hand
  - it holds across the standard corpus
*/
bool
tests_compress_is_valid_stream_accepts_own_output()
{
    const std::vector<dj::byte_buffer> c = dt::standard_corpus();
    std::size_t                        i = 0;

    for (i = 0; i < c.size(); ++i)
    {
        const dt::roundtrip_report r = dt::roundtrip<dj::codecs::store>(c[i]);

        D_TC_CHECK(r.compress_status == dj::status_ok);

        if (!dt::is_valid_stream<dj::codecs::store>(r.compressed))
        {
            std::printf("    [FAIL] corpus entry %u produced an invalid "
                        "stream\n", static_cast<unsigned int>(i));

            return false;
        }
    }

    // the check does not consult the payload: it accepts the stream whether
    // or not the caller knows what it decodes to
    const dt::roundtrip_report r =
        dt::roundtrip<dj::codecs::store>(dt::make_text(300));

    D_TC_CHECK(dt::is_valid_stream<dj::codecs::store>(r.compressed));
    D_TC_CHECK(dt::decompresses_to<dj::codecs::store>(r.compressed,
                                                      dt::make_text(300)));
    D_TC_CHECK(!dt::decompresses_to<dj::codecs::store>(r.compressed,
                                                       dt::make_text(301)));

    return true;
}

/*
tests_compress_is_valid_stream_rejects_undecodable
  the well-formedness check's rejecting case.
  Tests the following:
  - bytes that do not decode under a codec are reported malformed
  - an unavailable codec reports every buffer malformed, since nothing
    decodes
  - the identity codec is the documented exception: it decodes any buffer,
    so it accepts arbitrary bytes by construction
*/
bool
tests_compress_is_valid_stream_rejects_undecodable()
{
    const dj::byte_buffer junk("certainly not a compressed stream");
    const dj::byte_buffer empty;
    std::size_t           checked = 0;

    // a codec this build lacks reports everything malformed
    if (!dt::is_available<dj::codecs::gzip>())
    {
        D_TC_CHECK(!dt::is_valid_stream<dj::codecs::gzip>(junk));
        D_TC_CHECK(!dt::is_valid_stream<dj::codecs::gzip>(empty));
        ++checked;
    }

    if (!dt::is_available<dj::codecs::xz>())
    {
        D_TC_CHECK(!dt::is_valid_stream<dj::codecs::xz>(junk));
        ++checked;
    }

    if (!dt::is_available<dj::codecs::bzip2>())
    {
        D_TC_CHECK(!dt::is_valid_stream<dj::codecs::bzip2>(junk));
        ++checked;
    }

    // on a fully-equipped build, a real decoder must reject garbage instead
    if (checked == 0u)
    {
        D_TC_CHECK(!dt::is_valid_stream<dj::codecs::gzip>(junk));
        D_TC_CHECK(!dt::is_valid_stream<dj::codecs::xz>(junk));
        D_TC_CHECK(!dt::is_valid_stream<dj::codecs::bzip2>(junk));
    }

    // store is the documented exception: decoding is the identity, so ANY
    // buffer is a well-formed store stream.  Pinned here so a later change
    // to that behaviour is caught rather than silently altering what a
    // caller's is_valid_stream<store> assertion means.
    D_TC_CHECK(dt::is_valid_stream<dj::codecs::store>(junk));
    D_TC_CHECK(dt::is_valid_stream<dj::codecs::store>(empty));
    D_TC_CHECK(dt::decompresses_to<dj::codecs::store>(junk, junk));

    return true;
}

/*
tests_compress_is_available_matches_facade
  the availability re-export.
  Tests the following:
  - the re-export returns exactly what the facade's own query returns, for
    every one of the nine codec tags
  - the identity codec is present on every build
  - repeated calls agree, so the answer is stable within a run
*/
bool
tests_compress_is_available_matches_facade()
{
    D_TC_CHECK(dt::is_available<dj::codecs::store>() ==
               dj::codec_is_available<dj::codecs::store>());
    D_TC_CHECK(dt::is_available<dj::codecs::deflate>() ==
               dj::codec_is_available<dj::codecs::deflate>());
    D_TC_CHECK(dt::is_available<dj::codecs::zlib>() ==
               dj::codec_is_available<dj::codecs::zlib>());
    D_TC_CHECK(dt::is_available<dj::codecs::gzip>() ==
               dj::codec_is_available<dj::codecs::gzip>());
    D_TC_CHECK(dt::is_available<dj::codecs::bzip2>() ==
               dj::codec_is_available<dj::codecs::bzip2>());
    D_TC_CHECK(dt::is_available<dj::codecs::xz>() ==
               dj::codec_is_available<dj::codecs::xz>());
    D_TC_CHECK(dt::is_available<dj::codecs::zstd>() ==
               dj::codec_is_available<dj::codecs::zstd>());
    D_TC_CHECK(dt::is_available<dj::codecs::lz4>() ==
               dj::codec_is_available<dj::codecs::lz4>());
    D_TC_CHECK(dt::is_available<dj::codecs::brotli>() ==
               dj::codec_is_available<dj::codecs::brotli>());

    // the identity codec is always compiled in
    D_TC_CHECK(dt::is_available<dj::codecs::store>());

    // and the answer is stable within a run
    D_TC_CHECK(dt::is_available<dj::codecs::gzip>() ==
               dt::is_available<dj::codecs::gzip>());
    D_TC_CHECK(dt::is_available<dj::codecs::zstd>() ==
               dt::is_available<dj::codecs::zstd>());

    return true;
}

/*
tests_compress_is_available_agrees_with_report
  the two ways of asking the same question.
  Tests the following:
  - the standalone query and the report's available flag never disagree
  - this holds for every codec tag and for several payloads, so the flag is
    not accidentally derived from the payload
*/
bool
tests_compress_is_available_agrees_with_report()
{
    const dj::byte_buffer empty;
    const dj::byte_buffer small = dt::make_text(10);
    const dj::byte_buffer large = dt::make_text(100000);

    D_TC_CHECK(dt::roundtrip<dj::codecs::store>(empty).available ==
               dt::is_available<dj::codecs::store>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::store>(large).available ==
               dt::is_available<dj::codecs::store>());

    D_TC_CHECK(dt::roundtrip<dj::codecs::gzip>(empty).available ==
               dt::is_available<dj::codecs::gzip>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::gzip>(small).available ==
               dt::is_available<dj::codecs::gzip>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::gzip>(large).available ==
               dt::is_available<dj::codecs::gzip>());

    D_TC_CHECK(dt::roundtrip<dj::codecs::zstd>(small).available ==
               dt::is_available<dj::codecs::zstd>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::xz>(small).available ==
               dt::is_available<dj::codecs::xz>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::lz4>(small).available ==
               dt::is_available<dj::codecs::lz4>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::brotli>(small).available ==
               dt::is_available<dj::codecs::brotli>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::bzip2>(small).available ==
               dt::is_available<dj::codecs::bzip2>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::zlib>(small).available ==
               dt::is_available<dj::codecs::zlib>());
    D_TC_CHECK(dt::roundtrip<dj::codecs::deflate>(small).available ==
               dt::is_available<dj::codecs::deflate>());

    return true;
}

/*
tests_compress_verification_helpers_agree
  the relationships between the checks.
  Tests the following:
  - decode-and-compare implies well-formed: nothing can match a payload
    without first decoding cleanly
  - a successful strict round-trip implies both helpers accept the emitted
    stream
  - an unavailable codec fails every helper together, never partially
*/
bool
tests_compress_verification_helpers_agree()
{
    const std::vector<dj::byte_buffer> c = dt::standard_corpus();
    std::size_t                        i = 0;

    for (i = 0; i < c.size(); ++i)
    {
        const dt::roundtrip_report r = dt::roundtrip<dj::codecs::store>(c[i]);

        // decode-and-compare implies well-formed
        if (dt::decompresses_to<dj::codecs::store>(r.compressed, c[i]))
        {
            D_TC_CHECK(dt::is_valid_stream<dj::codecs::store>(r.compressed));
        }

        // a strict round-trip implies both helpers accept the stream
        if (dt::roundtrips<dj::codecs::store>(c[i]))
        {
            D_TC_CHECK(dt::is_valid_stream<dj::codecs::store>(r.compressed));
            D_TC_CHECK(dt::decompresses_to<dj::codecs::store>(r.compressed,
                                                              c[i]));
        }
    }

    // an unavailable codec fails everything together
    if (!dt::is_available<dj::codecs::gzip>())
    {
        const dj::byte_buffer      in = dt::make_text(100);
        const dt::roundtrip_report r  = dt::roundtrip<dj::codecs::gzip>(in);

        D_TC_CHECK(!dt::roundtrips<dj::codecs::gzip>(in));
        D_TC_CHECK(!dt::is_valid_stream<dj::codecs::gzip>(r.compressed));
        D_TC_CHECK(!dt::decompresses_to<dj::codecs::gzip>(r.compressed, in));

        // but the build-agnostic check still passes, which is the whole
        // point of having it
        D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::gzip>(in));
    }

    return true;
}

NS_END  // testing
NS_END  // djinterp
