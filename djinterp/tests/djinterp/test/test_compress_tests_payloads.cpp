#include "test_compress_tests.hpp"

NS_DJINTERP
NS_TESTING


// tc_seed_phrase
//   constant: the prose the module tiles to build a text payload.
static const char tc_seed_phrase[] =
    "the quick brown fox jumps over the lazy dog. ";

// tc_seed_len
//   constant: the seed's length, excluding its terminator.
static const std::size_t tc_seed_len = (sizeof(tc_seed_phrase) - 1u);


/*
tests_compress_make_repeated_size_and_content
  the single-run payload.
  Tests the following:
  - the buffer is exactly the requested length
  - every byte carries the requested value
  - the value is honoured for a printable byte, a high byte and NUL alike
*/
bool
tests_compress_make_repeated_size_and_content()
{
    const dj::byte_buffer z = dt::make_repeated('Z', 4096);

    D_TC_CHECK(z.size() == 4096u);
    D_TC_CHECK(tc_count_byte(z, 'Z') == 4096u);
    D_TC_CHECK(tc_distinct_bytes(z) == 1u);

    // a high byte survives without sign trouble
    const dj::byte_buffer high =
        dt::make_repeated(static_cast<char>(0xFFu), 300);

    D_TC_CHECK(high.size() == 300u);
    D_TC_CHECK(tc_distinct_bytes(high) == 1u);
    D_TC_CHECK(static_cast<unsigned char>(high[0]) == 0xFFu);
    D_TC_CHECK(static_cast<unsigned char>(high[299]) == 0xFFu);

    // and so does a NUL fill, which a C-string-minded path would truncate
    const dj::byte_buffer nul = dt::make_repeated('\0', 64);

    D_TC_CHECK(nul.size() == 64u);
    D_TC_CHECK(tc_count_byte(nul, '\0') == 64u);

    return true;
}

/*
tests_compress_make_repeated_degenerate
  the edges of the single-run payload.
  Tests the following:
  - a zero length yields an empty buffer rather than a one-byte one
  - a length of one yields exactly one byte
  - two calls with the same arguments are byte-identical
*/
bool
tests_compress_make_repeated_degenerate()
{
    D_TC_CHECK(dt::make_repeated('A', 0).empty());
    D_TC_CHECK(dt::make_repeated('A', 0).size() == 0u);

    const dj::byte_buffer one = dt::make_repeated('A', 1);

    D_TC_CHECK(one.size() == 1u);
    D_TC_CHECK(one[0] == 'A');

    // determinism
    D_TC_CHECK(dt::make_repeated('Q', 100) == dt::make_repeated('Q', 100));

    // and the fill byte really is what distinguishes two buffers
    D_TC_CHECK(dt::make_repeated('A', 10) != dt::make_repeated('B', 10));

    return true;
}

/*
tests_compress_make_pattern_cycles_byte_range
  the byte-cycle payload.
  Tests the following:
  - byte i carries the value i modulo 256
  - a buffer of 256 bytes contains every possible byte value exactly once
  - the cycle wraps, so byte 256 repeats byte 0
  - NUL appears, which is what makes this payload length-carrying
*/
bool
tests_compress_make_pattern_cycles_byte_range()
{
    const dj::byte_buffer p = dt::make_pattern(1024);
    std::size_t           i = 0;

    D_TC_CHECK(p.size() == 1024u);

    // byte i is i % 256
    for (i = 0; i < p.size(); ++i)
    {
        D_TC_CHECK(static_cast<unsigned char>(p[i]) ==
                   static_cast<unsigned char>(i & 0xFFu));
    }

    // one full cycle covers the whole byte range
    const dj::byte_buffer full = dt::make_pattern(256);

    D_TC_CHECK(tc_distinct_bytes(full) == 256u);
    D_TC_CHECK(static_cast<unsigned char>(full[0]) == 0x00u);
    D_TC_CHECK(static_cast<unsigned char>(full[255]) == 0xFFu);

    // and it wraps
    D_TC_CHECK(p[256] == p[0]);
    D_TC_CHECK(p[257] == p[1]);
    D_TC_CHECK(p[512] == p[0]);

    // NUL is present, once per cycle
    D_TC_CHECK(tc_count_byte(p, '\0') == 4u);

    return true;
}

/*
tests_compress_make_pattern_degenerate
  the edges of the byte-cycle payload.
  Tests the following:
  - a zero length yields an empty buffer
  - a single byte is 0x00, the first value of the cycle
  - the generator is deterministic
*/
bool
tests_compress_make_pattern_degenerate()
{
    D_TC_CHECK(dt::make_pattern(0).empty());

    const dj::byte_buffer one = dt::make_pattern(1);

    D_TC_CHECK(one.size() == 1u);
    D_TC_CHECK(static_cast<unsigned char>(one[0]) == 0x00u);

    // a shorter buffer is a prefix of a longer one
    const dj::byte_buffer shortp = dt::make_pattern(10);
    const dj::byte_buffer longp  = dt::make_pattern(20);

    D_TC_CHECK(longp.substr(0, 10) == shortp);

    // determinism
    D_TC_CHECK(dt::make_pattern(777) == dt::make_pattern(777));

    return true;
}

/*
tests_compress_make_text_length_and_tiling
  the prose payload.
  Tests the following:
  - the buffer is trimmed to exactly the requested length, whether or not
    that falls on a seed boundary
  - it is built by tiling the seed phrase, so byte i equals seed byte
    i modulo the seed length
  - a length that is an exact multiple of the seed tiles cleanly
*/
bool
tests_compress_make_text_length_and_tiling()
{
    const dj::byte_buffer t = dt::make_text(8192);
    std::size_t           i = 0;

    D_TC_CHECK(t.size() == 8192u);

    // the tiling is exact
    for (i = 0; i < t.size(); ++i)
    {
        D_TC_CHECK(t[i] == tc_seed_phrase[i % tc_seed_len]);
    }

    // an exact multiple of the seed length
    const dj::byte_buffer exact = dt::make_text(tc_seed_len * 3u);

    D_TC_CHECK(exact.size() == (tc_seed_len * 3u));
    D_TC_CHECK(exact.substr(0, tc_seed_len) ==
               exact.substr(tc_seed_len, tc_seed_len));

    // a length one past a boundary is trimmed, not rounded up
    const dj::byte_buffer plus1 = dt::make_text(tc_seed_len + 1u);

    D_TC_CHECK(plus1.size() == (tc_seed_len + 1u));
    D_TC_CHECK(plus1[tc_seed_len] == tc_seed_phrase[0]);

    // the payload is text-like: no NUL anywhere
    D_TC_CHECK(tc_count_byte(t, '\0') == 0u);

    return true;
}

/*
tests_compress_make_text_degenerate
  the edges of the prose payload.
  Tests the following:
  - a zero length yields an empty buffer, so the tiling loop is never
    entered
  - a length shorter than the seed is a prefix of it
  - the generator is deterministic
*/
bool
tests_compress_make_text_degenerate()
{
    D_TC_CHECK(dt::make_text(0).empty());

    // shorter than one seed
    const dj::byte_buffer tiny = dt::make_text(5);

    D_TC_CHECK(tiny.size() == 5u);
    D_TC_CHECK(tiny == std::string(tc_seed_phrase, 5));

    const dj::byte_buffer one = dt::make_text(1);

    D_TC_CHECK(one.size() == 1u);
    D_TC_CHECK(one[0] == tc_seed_phrase[0]);

    // exactly one seed
    const dj::byte_buffer seed = dt::make_text(tc_seed_len);

    D_TC_CHECK(seed == std::string(tc_seed_phrase, tc_seed_len));

    // determinism, and prefix consistency
    D_TC_CHECK(dt::make_text(500) == dt::make_text(500));
    D_TC_CHECK(dt::make_text(500).substr(0, 100) == dt::make_text(100));

    return true;
}

/*
tests_compress_make_incompressible_is_deterministic
  reproducibility of the high-entropy payload.
  Tests the following:
  - the same seed reproduces the same bytes on every call, which is what
    makes a codec failure reproducible across runs and machines
  - the default seed is stable across calls
  - a shorter request is a prefix of a longer one from the same seed
*/
bool
tests_compress_make_incompressible_is_deterministic()
{
    D_TC_CHECK(dt::make_incompressible(1000) ==
               dt::make_incompressible(1000));

    // an explicit seed is equally stable
    D_TC_CHECK(dt::make_incompressible(500, 42u) ==
               dt::make_incompressible(500, 42u));

    // the sequence is a stream, so a short draw prefixes a long one
    const dj::byte_buffer longer  = dt::make_incompressible(500, 7u);
    const dj::byte_buffer shorter = dt::make_incompressible(100, 7u);

    D_TC_CHECK(longer.substr(0, 100) == shorter);

    // and the default seed agrees with naming it explicitly
    D_TC_CHECK(dt::make_incompressible(64) ==
               dt::make_incompressible(64, 0x1234ABCDu));

    return true;
}

/*
tests_compress_make_incompressible_seed_behaviour
  the seed parameter.
  Tests the following:
  - different seeds produce different streams
  - a zero seed falls back to the documented non-zero state instead of
    sticking at zero, so the payload is still high-entropy
  - the length is honoured exactly, and zero length is empty
*/
bool
tests_compress_make_incompressible_seed_behaviour()
{
    D_TC_CHECK(dt::make_incompressible(256, 1u) !=
               dt::make_incompressible(256, 2u));
    D_TC_CHECK(dt::make_incompressible(256, 99u) !=
               dt::make_incompressible(256, 100u));

    // a zero seed is replaced by the default, not left to stick at zero
    const dj::byte_buffer zero_seed = dt::make_incompressible(256, 0u);

    D_TC_CHECK(zero_seed.size() == 256u);
    D_TC_CHECK(zero_seed == dt::make_incompressible(256, 0x1234ABCDu));

    // which means it is NOT a run of identical bytes
    D_TC_CHECK(tc_distinct_bytes(zero_seed) > 1u);

    // lengths
    D_TC_CHECK(dt::make_incompressible(0).empty());
    D_TC_CHECK(dt::make_incompressible(0, 5u).empty());
    D_TC_CHECK(dt::make_incompressible(1).size() == 1u);
    D_TC_CHECK(dt::make_incompressible(65537).size() == 65537u);

    return true;
}

/*
tests_compress_make_incompressible_spread
  the entropy the payload is named for.
  Tests the following:
  - a long draw touches most of the byte range, unlike the patterned
    corpora
  - it is not a single run, and not the byte cycle
  - the spread holds for a non-default seed too
*/
bool
tests_compress_make_incompressible_spread()
{
    const dj::byte_buffer r = dt::make_incompressible(8192);

    D_TC_CHECK(r.size() == 8192u);

    // a high-entropy draw of 8 KiB should touch nearly every byte value
    D_TC_CHECK(tc_distinct_bytes(r) > 200u);

    // it is neither a run nor the cycle
    D_TC_CHECK(r != dt::make_repeated(r[0], 8192));
    D_TC_CHECK(r != dt::make_pattern(8192));
    D_TC_CHECK(r != dt::make_text(8192));

    // and the spread does not depend on the default seed
    D_TC_CHECK(tc_distinct_bytes(dt::make_incompressible(8192, 999u)) > 200u);

    return true;
}

/*
tests_compress_make_with_nuls_layout
  the NUL-bearing payload.
  Tests the following:
  - every third byte is NUL and the others walk the printable alphabet
  - the exact length is honoured, including lengths that do not land on a
    group boundary
  - the buffer carries more NULs than a C-string path could survive
*/
bool
tests_compress_make_with_nuls_layout()
{
    const dj::byte_buffer n = dt::make_with_nuls(1500);
    std::size_t           i = 0;

    D_TC_CHECK(n.size() == 1500u);

    // the documented layout, byte by byte
    for (i = 0; i < n.size(); ++i)
    {
        if ((i % 3u) == 0u)
        {
            D_TC_CHECK(n[i] == static_cast<char>(0x00));
        }
        else
        {
            D_TC_CHECK(n[i] ==
                       static_cast<char>(
                           static_cast<unsigned char>('A' + (i % 26u))));
            D_TC_CHECK(n[i] != static_cast<char>(0x00));
        }
    }

    // one NUL per three bytes
    D_TC_CHECK(tc_count_byte(n, '\0') == 500u);

    // a length off the group boundary is still exact.  Note index 3 is a
    // multiple of three, so a four-byte buffer both opens and ends on a NUL
    D_TC_CHECK(dt::make_with_nuls(4).size() == 4u);
    D_TC_CHECK(dt::make_with_nuls(4)[3] == '\0');
    D_TC_CHECK(dt::make_with_nuls(4)[2] != '\0');
    D_TC_CHECK(dt::make_with_nuls(5).size() == 5u);
    D_TC_CHECK(dt::make_with_nuls(5)[4] != '\0');
    D_TC_CHECK(dt::make_with_nuls(0).empty());

    // the first byte is always NUL
    D_TC_CHECK(dt::make_with_nuls(1)[0] == '\0');

    // determinism
    D_TC_CHECK(dt::make_with_nuls(300) == dt::make_with_nuls(300));

    return true;
}

/*
tests_compress_standard_corpus_shape
  the representative battery.
  Tests the following:
  - the corpus carries the ten documented payloads in the documented order
  - each has its documented length, including the empty first entry
  - two entries exceed the 64 KiB streaming chunk, so the multi-chunk path
    is covered by any sweep over the corpus
*/
bool
tests_compress_standard_corpus_shape()
{
    const std::vector<dj::byte_buffer> c = dt::standard_corpus();

    D_TC_CHECK(c.size() == 10u);

    // the documented lengths, in order
    D_TC_CHECK(c[0].empty());
    D_TC_CHECK(c[1].size() == 1u);
    D_TC_CHECK(c[2].size() == 12u);
    D_TC_CHECK(c[3].size() == 4096u);
    D_TC_CHECK(c[4].size() == 1024u);
    D_TC_CHECK(c[5].size() == 8192u);
    D_TC_CHECK(c[6].size() == 8192u);
    D_TC_CHECK(c[7].size() == 1500u);
    D_TC_CHECK(c[8].size() == 200000u);
    D_TC_CHECK(c[9].size() == 160000u);

    // the documented content
    D_TC_CHECK(c[1] == dj::byte_buffer("A"));
    D_TC_CHECK(c[2] == dj::byte_buffer("hello, world"));
    D_TC_CHECK(c[3] == dt::make_repeated('Z', 4096));
    D_TC_CHECK(c[4] == dt::make_pattern(1024));
    D_TC_CHECK(c[5] == dt::make_text(8192));
    D_TC_CHECK(c[6] == dt::make_incompressible(8192));
    D_TC_CHECK(c[7] == dt::make_with_nuls(1500));

    // two payloads cross the 64 KiB chunk boundary
    D_TC_CHECK(c[8].size() > 65536u);
    D_TC_CHECK(c[9].size() > 65536u);

    // and the battery really is representative: a run, a cycle, prose, a
    // high-entropy draw and a NUL-bearing buffer
    D_TC_CHECK(tc_distinct_bytes(c[3]) == 1u);
    D_TC_CHECK(tc_distinct_bytes(c[4]) == 256u);
    D_TC_CHECK(tc_distinct_bytes(c[6]) > 200u);
    D_TC_CHECK(tc_count_byte(c[7], '\0') > 0u);

    return true;
}

/*
tests_compress_standard_corpus_is_deterministic
  reproducibility of the battery.
  Tests the following:
  - two calls produce byte-identical corpora, so a sweep run twice
    compares like with like
  - the high-entropy members are stable, which is the only part that could
    have drifted
*/
bool
tests_compress_standard_corpus_is_deterministic()
{
    const std::vector<dj::byte_buffer> a = dt::standard_corpus();
    const std::vector<dj::byte_buffer> b = dt::standard_corpus();
    std::size_t                        i = 0;

    D_TC_CHECK(a.size() == b.size());

    for (i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i])
        {
            std::printf("    [FAIL] corpus entry %u differs between calls\n",
                        static_cast<unsigned int>(i));

            return false;
        }
    }

    // no member touches a real random source, so nothing drifts
    D_TC_CHECK(a[6] == dt::make_incompressible(8192));
    D_TC_CHECK(a[9] == dt::make_incompressible(160000));

    return true;
}

NS_END  // testing
NS_END  // djinterp
