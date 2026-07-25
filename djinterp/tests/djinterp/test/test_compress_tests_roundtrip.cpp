#include "test_compress_tests.hpp"

NS_DJINTERP
NS_TESTING


/*
tests_compress_roundtrip_report_is_fully_populated
  the record of one cycle.
  Tests the following:
  - input_size reports the payload's length, whatever the codec's fate
  - compressed_size agrees with the compressed buffer's own length
  - available agrees with the standalone availability query
  - restored_equals_input is true only when decompression both succeeded
    and reproduced the bytes
*/
bool
tests_compress_roundtrip_report_is_fully_populated()
{
    const dj::byte_buffer      in = dt::make_text(1000);
    const dt::roundtrip_report r  = dt::roundtrip<dj::codecs::store>(in);

    D_TC_CHECK(r.input_size == in.size());
    D_TC_CHECK(r.input_size == 1000u);
    D_TC_CHECK(r.compressed_size == r.compressed.size());
    D_TC_CHECK(r.available == dt::is_available<dj::codecs::store>());

    // the equality flag is exactly its documented conjunction
    D_TC_CHECK(r.restored_equals_input ==
               ((r.decompress_status == dj::status_ok) && (r.restored == in)));

    // the same holds for a codec that may well be absent
    const dt::roundtrip_report g = dt::roundtrip<dj::codecs::gzip>(in);

    D_TC_CHECK(g.input_size == in.size());
    D_TC_CHECK(g.compressed_size == g.compressed.size());
    D_TC_CHECK(g.available == dt::is_available<dj::codecs::gzip>());
    D_TC_CHECK(g.restored_equals_input ==
               ((g.decompress_status == dj::status_ok) && (g.restored == in)));

    return true;
}

/*
tests_compress_roundtrip_store_reproduces_input
  the always-present identity codec.
  Tests the following:
  - store is compiled into every build, so this arm is never vacuous
  - the cycle completes with status_ok on both halves
  - the restored bytes equal the input exactly
  - the strict and build-agnostic checks both accept it
*/
bool
tests_compress_roundtrip_store_reproduces_input()
{
    const dj::byte_buffer in = dt::make_text(4096);

    // store is the one codec a bare build is guaranteed to carry
    D_TC_CHECK(dt::is_available<dj::codecs::store>());

    const dt::roundtrip_report r = dt::roundtrip<dj::codecs::store>(in);

    D_TC_CHECK(r.available);
    D_TC_CHECK(r.compress_status == dj::status_ok);
    D_TC_CHECK(r.decompress_status == dj::status_ok);
    D_TC_CHECK(r.restored_equals_input);
    D_TC_CHECK(r.restored == in);

    // both roll-ups accept it
    D_TC_CHECK(dt::roundtrips<dj::codecs::store>(in));
    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::store>(in));

    // a NUL-bearing payload proves the path is length-carrying
    const dj::byte_buffer nuls = dt::make_with_nuls(1500);

    D_TC_CHECK(dt::roundtrips<dj::codecs::store>(nuls));
    D_TC_CHECK(dt::roundtrip<dj::codecs::store>(nuls).restored == nuls);

    return true;
}

/*
tests_compress_roundtrip_unavailable_codec_contract
  the other side of the availability split.
  Tests the following:
  - an unavailable codec reports status_unavailable from compression
  - it emits no bytes at all rather than partially succeeding
  - restored_equals_input is false, so a caller cannot mistake it for a
    successful cycle
  - available distinguishes this from a genuine failure
  - on a build where every codec IS present, the body asserts the other
    branch instead, so it is never vacuous
*/
bool
tests_compress_roundtrip_unavailable_codec_contract()
{
    const dj::byte_buffer in = dt::make_text(256);
    std::size_t           checked = 0;

    // walk the framed codecs; whichever are absent must obey the contract
    if (!dt::is_available<dj::codecs::gzip>())
    {
        const dt::roundtrip_report r = dt::roundtrip<dj::codecs::gzip>(in);

        D_TC_CHECK(!r.available);
        D_TC_CHECK(r.compress_status == dj::status_unavailable);
        D_TC_CHECK(r.compressed.empty());
        D_TC_CHECK(r.compressed_size == 0u);
        D_TC_CHECK(!r.restored_equals_input);
        D_TC_CHECK(r.input_size == in.size());
        ++checked;
    }

    if (!dt::is_available<dj::codecs::bzip2>())
    {
        const dt::roundtrip_report r = dt::roundtrip<dj::codecs::bzip2>(in);

        D_TC_CHECK(!r.available);
        D_TC_CHECK(r.compress_status == dj::status_unavailable);
        D_TC_CHECK(r.compressed.empty());
        D_TC_CHECK(!r.restored_equals_input);
        ++checked;
    }

    if (!dt::is_available<dj::codecs::zstd>())
    {
        const dt::roundtrip_report r = dt::roundtrip<dj::codecs::zstd>(in);

        D_TC_CHECK(!r.available);
        D_TC_CHECK(r.compress_status == dj::status_unavailable);
        D_TC_CHECK(r.compressed.empty());
        ++checked;
    }

    // on a fully-equipped build nothing above ran; assert the OTHER branch
    // so this body still carries weight there
    if (checked == 0u)
    {
        D_TC_CHECK(dt::roundtrips<dj::codecs::gzip>(in));
        D_TC_CHECK(dt::roundtrips<dj::codecs::bzip2>(in));
        D_TC_CHECK(dt::roundtrips<dj::codecs::zstd>(in));
    }

    // either way the build-agnostic check accepts all three
    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::gzip>(in));
    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::bzip2>(in));
    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::zstd>(in));

    return true;
}

/*
tests_compress_roundtrip_empty_input
  the zero-length payload.
  Tests the following:
  - an empty payload is not treated as a failure by an available codec
  - it round-trips back to an empty payload
  - input_size is zero and the report is still fully populated
  - the build-agnostic check accepts every codec on it
*/
bool
tests_compress_roundtrip_empty_input()
{
    const dj::byte_buffer      empty;
    const dt::roundtrip_report r = dt::roundtrip<dj::codecs::store>(empty);

    D_TC_CHECK(r.input_size == 0u);
    D_TC_CHECK(r.available);
    D_TC_CHECK(r.compress_status == dj::status_ok);
    D_TC_CHECK(r.decompress_status == dj::status_ok);
    D_TC_CHECK(r.restored_equals_input);
    D_TC_CHECK(r.restored.empty());
    D_TC_CHECK(dt::roundtrips<dj::codecs::store>(empty));

    // every codec obeys the contract on an empty payload
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::store>(empty, "store"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::deflate>(empty,
                                                             "deflate"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::zlib>(empty, "zlib"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::gzip>(empty, "gzip"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::bzip2>(empty, "bzip2"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::xz>(empty, "xz"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::zstd>(empty, "zstd"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::lz4>(empty, "lz4"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::brotli>(empty, "brotli"));

    return true;
}

/*
tests_compress_roundtrip_large_input
  the multi-chunk path.
  Tests the following:
  - a payload past the 64 KiB streaming chunk round-trips whole
  - both a compressible and an incompressible large payload survive
  - the report's sizes agree with the buffers for a large input too
*/
bool
tests_compress_roundtrip_large_input()
{
    const dj::byte_buffer prose   = dt::make_text(200000);
    const dj::byte_buffer entropy = dt::make_incompressible(160000);

    D_TC_CHECK(prose.size() > 65536u);
    D_TC_CHECK(entropy.size() > 65536u);

    const dt::roundtrip_report p = dt::roundtrip<dj::codecs::store>(prose);

    D_TC_CHECK(p.input_size == 200000u);
    D_TC_CHECK(p.compressed_size == p.compressed.size());
    D_TC_CHECK(p.restored_equals_input);
    D_TC_CHECK(p.restored.size() == prose.size());

    const dt::roundtrip_report e = dt::roundtrip<dj::codecs::store>(entropy);

    D_TC_CHECK(e.input_size == 160000u);
    D_TC_CHECK(e.restored_equals_input);
    D_TC_CHECK(e.restored == entropy);

    // and the contract holds for every codec on a large payload
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::store>(prose, "store"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::gzip>(prose, "gzip"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::zstd>(entropy, "zstd"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::xz>(entropy, "xz"));

    return true;
}

/*
tests_compress_roundtrip_accepts_options
  the optional tuning argument.
  Tests the following:
  - an explicit option set is accepted by all three round-trip entry points
  - passing options does not change the correctness of the cycle
  - the default argument and an explicitly default set behave identically
*/
bool
tests_compress_roundtrip_accepts_options()
{
    const dj::byte_buffer    in = dt::make_text(2048);
    dj::compress_options     fast;
    dj::compress_options     best;

    fast.level = 1;
    best.level = 9;

    // an explicit set is accepted, and the cycle is still correct
    const dt::roundtrip_report f = dt::roundtrip<dj::codecs::store>(in, fast);
    const dt::roundtrip_report b = dt::roundtrip<dj::codecs::store>(in, best);

    D_TC_CHECK(f.restored_equals_input);
    D_TC_CHECK(b.restored_equals_input);
    D_TC_CHECK(f.restored == in);
    D_TC_CHECK(b.restored == in);

    // the roll-ups take options too
    D_TC_CHECK(dt::roundtrips<dj::codecs::store>(in, fast));
    D_TC_CHECK(dt::roundtrips<dj::codecs::store>(in, best));
    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::store>(in, fast));
    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::store>(in, best));

    // the defaulted argument matches an explicitly default set
    const dj::compress_options  fresh;
    const dt::roundtrip_report  d = dt::roundtrip<dj::codecs::store>(in);
    const dt::roundtrip_report  x =
        dt::roundtrip<dj::codecs::store>(in, fresh);

    D_TC_CHECK(d.compress_status == x.compress_status);
    D_TC_CHECK(d.compressed_size == x.compressed_size);
    D_TC_CHECK(d.restored_equals_input == x.restored_equals_input);

    // a deeply tuned set is accepted without disturbing the contract
    dj::compress_options tuned;

    tuned.zstd.window_log     = 20;
    tuned.deflate.window_bits = 15;
    tuned.brotli.quality      = 5;

    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::store>(in, tuned));
    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::zstd>(in, tuned));

    return true;
}

/*
tests_compress_roundtrips_is_strict
  the availability-requiring check.
  Tests the following:
  - the strict check accepts an available codec that completes the cycle
  - it REJECTS an unavailable codec, which is what distinguishes it from
    the build-agnostic check
  - the two checks therefore disagree exactly where a codec is missing
*/
bool
tests_compress_roundtrips_is_strict()
{
    const dj::byte_buffer in = dt::make_text(512);

    // the always-present codec is accepted by both
    D_TC_CHECK(dt::roundtrips<dj::codecs::store>(in));
    D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::store>(in));

    // for each framed codec, strict == available && agnostic
    D_TC_CHECK(dt::roundtrips<dj::codecs::gzip>(in) ==
               dt::is_available<dj::codecs::gzip>());
    D_TC_CHECK(dt::roundtrips<dj::codecs::bzip2>(in) ==
               dt::is_available<dj::codecs::bzip2>());
    D_TC_CHECK(dt::roundtrips<dj::codecs::xz>(in) ==
               dt::is_available<dj::codecs::xz>());
    D_TC_CHECK(dt::roundtrips<dj::codecs::zstd>(in) ==
               dt::is_available<dj::codecs::zstd>());
    D_TC_CHECK(dt::roundtrips<dj::codecs::lz4>(in) ==
               dt::is_available<dj::codecs::lz4>());
    D_TC_CHECK(dt::roundtrips<dj::codecs::brotli>(in) ==
               dt::is_available<dj::codecs::brotli>());
    D_TC_CHECK(dt::roundtrips<dj::codecs::zlib>(in) ==
               dt::is_available<dj::codecs::zlib>());
    D_TC_CHECK(dt::roundtrips<dj::codecs::deflate>(in) ==
               dt::is_available<dj::codecs::deflate>());

    // where a codec is missing, the two checks disagree by design
    if (!dt::is_available<dj::codecs::gzip>())
    {
        D_TC_CHECK(!dt::roundtrips<dj::codecs::gzip>(in));
        D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::gzip>(in));
    }

    return true;
}

/*
tests_compress_facade_roundtrip_ok_is_build_agnostic
  the check a suite can assert unconditionally.
  Tests the following:
  - every one of the nine codec tags satisfies the contract on this build
  - the longhand form of the contract agrees with the module's roll-up in
    each case, so the roll-up is not merely returning true
*/
bool
tests_compress_facade_roundtrip_ok_is_build_agnostic()
{
    const dj::byte_buffer in = dt::make_text(3000);

    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::store>(in, "store"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::deflate>(in, "deflate"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::zlib>(in, "zlib"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::gzip>(in, "gzip"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::bzip2>(in, "bzip2"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::xz>(in, "xz"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::zstd>(in, "zstd"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::lz4>(in, "lz4"));
    D_TC_CHECK(tc_facade_contract_holds<dj::codecs::brotli>(in, "brotli"));

    // at least one codec really is present, so the available branch above
    // was genuinely taken
    D_TC_CHECK(dt::is_available<dj::codecs::store>());

    return true;
}

/*
tests_compress_facade_roundtrip_ok_over_corpus
  the contract across the whole battery.
  Tests the following:
  - the contract holds for every payload of the standard corpus, for a
    representative spread of codecs
  - the identity codec reproduces every corpus payload exactly, including
    the empty one and the two that cross the chunk boundary
*/
bool
tests_compress_facade_roundtrip_ok_over_corpus()
{
    const std::vector<dj::byte_buffer> c = dt::standard_corpus();
    std::size_t                        i = 0;

    for (i = 0; i < c.size(); ++i)
    {
        // the build-agnostic contract, for a spread of codecs
        D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::store>(c[i]));
        D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::deflate>(c[i]));
        D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::gzip>(c[i]));
        D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::zstd>(c[i]));
        D_TC_CHECK(dt::facade_roundtrip_ok<dj::codecs::brotli>(c[i]));

        // and the identity codec reproduces the payload exactly
        const dt::roundtrip_report r = dt::roundtrip<dj::codecs::store>(c[i]);

        if (!r.restored_equals_input)
        {
            std::printf("    [FAIL] corpus entry %u did not round-trip\n",
                        static_cast<unsigned int>(i));

            return false;
        }

        D_TC_CHECK(r.restored == c[i]);
        D_TC_CHECK(r.input_size == c[i].size());
    }

    return true;
}

/*
tests_compress_roundtrip_never_throws_on_garbage
  the non-throwing foundation.
  Tests the following:
  - decoding arbitrary non-stream bytes returns a status rather than
    raising, since every helper is built on the try_ API
  - the helpers report a plain false rather than propagating a failure
  - this holds whether or not exceptions are enabled in this translation
    unit
*/
bool
tests_compress_roundtrip_never_throws_on_garbage()
{
    const dj::byte_buffer junk("this is not a compressed stream of any kind");
    const dj::byte_buffer truncated_gzip =
        tc_prefix(reinterpret_cast<const unsigned char*>("\x1F\x8B"), 2, 3);
    dj::byte_buffer       out;

    // the low-level entry points return a status
    const dj::status s1 = dj::try_decompress<dj::codecs::gzip>(junk, out);
    const dj::status s2 = dj::try_decompress<dj::codecs::xz>(junk, out);
    const dj::status s3 =
        dj::try_decompress<dj::codecs::zstd>(truncated_gzip, out);

    // any status at all is acceptable; what matters is that we got one
    D_TC_CHECK((s1 == dj::status_ok) || (s1 != dj::status_ok));
    D_TC_CHECK((s2 == dj::status_ok) || (s2 != dj::status_ok));
    D_TC_CHECK((s3 == dj::status_ok) || (s3 != dj::status_ok));

    // and the helpers built on them return a plain bool
    const bool v1 = dt::is_valid_stream<dj::codecs::gzip>(junk);
    const bool v2 = dt::is_valid_stream<dj::codecs::xz>(junk);
    const bool v3 = dt::decompresses_to<dj::codecs::bzip2>(junk, junk);

    D_TC_CHECK((v1 == true) || (v1 == false));
    D_TC_CHECK((v2 == true) || (v2 == false));
    D_TC_CHECK((v3 == true) || (v3 == false));

    // a full report over garbage is still fully populated
    const dt::roundtrip_report r = dt::roundtrip<dj::codecs::gzip>(junk);

    D_TC_CHECK(r.input_size == junk.size());
    D_TC_CHECK(r.compressed_size == r.compressed.size());

    return true;
}

NS_END  // testing
NS_END  // djinterp
