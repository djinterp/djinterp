#include "test_compress_tests.hpp"

NS_DJINTERP
NS_TESTING


// the documented magics, exactly as the module spells them
static const unsigned char tc_m_gzip[2]  = { 0x1Fu, 0x8Bu };
static const unsigned char tc_m_bzip2[3] = { 'B', 'Z', 'h' };
static const unsigned char tc_m_xz[6]    =
    { 0xFDu, 0x37u, 0x7Au, 0x58u, 0x5Au, 0x00u };
static const unsigned char tc_m_zstd[4]  = { 0x28u, 0xB5u, 0x2Fu, 0xFDu };
static const unsigned char tc_m_lz4[4]   = { 0x04u, 0x22u, 0x4Du, 0x18u };


/*
tests_compress_gzip_magic
  the two gzip identification bytes.
  Tests the following:
  - a buffer opening 1F 8B is accepted
  - trailing content past the header is ignored, as documented
  - a wrong byte at either position is rejected
  - a buffer shorter than two bytes is rejected without reading past its end
*/
bool
tests_compress_gzip_magic()
{
    D_TC_CHECK(dt::has_gzip_magic(tc_bytes(tc_m_gzip, 2)));
    D_TC_CHECK(dt::has_gzip_magic(tc_prefix(tc_m_gzip, 2, 64)));

    // every byte of the magic is checked
    const std::vector<dj::byte_buffer> bad =
        tc_corruptions(tc_prefix(tc_m_gzip, 2, 8), 2);
    std::size_t                        i = 0;

    D_TC_CHECK(bad.size() == 2u);

    for (i = 0; i < bad.size(); ++i)
    {
        D_TC_CHECK(!dt::has_gzip_magic(bad[i]));
    }

    // and the length guard
    const std::vector<dj::byte_buffer> shortb =
        tc_truncations(tc_bytes(tc_m_gzip, 2));

    D_TC_CHECK(shortb.size() == 2u);

    for (i = 0; i < shortb.size(); ++i)
    {
        D_TC_CHECK(!dt::has_gzip_magic(shortb[i]));
    }

    // a plausible near miss: the zlib header is not a gzip header
    D_TC_CHECK(!dt::has_gzip_magic(tc_zlib_header()));

    return true;
}

/*
tests_compress_zlib_magic_accepts_valid_header
  the RFC 1950 header check.
  Tests the following:
  - a header with DEFLATE in the low nibble of CMF and a 31-divisible
    CMF/FLG pair is accepted
  - this holds for several window sizes, i.e. several CMF high nibbles
  - trailing content is ignored
*/
bool
tests_compress_zlib_magic_accepts_valid_header()
{
    // the canonical 32 KiB-window header
    const dj::byte_buffer h = tc_zlib_header(0x78u);

    D_TC_CHECK(h.size() == 2u);
    D_TC_CHECK(dt::has_zlib_magic(h));

    // the pair really does satisfy the FCHECK constraint
    const unsigned int cmf = static_cast<unsigned char>(h[0]);
    const unsigned int flg = static_cast<unsigned char>(h[1]);

    D_TC_CHECK((cmf & 0x0Fu) == 0x08u);
    D_TC_CHECK((((cmf << 8) | flg) % 31u) == 0u);

    // several window sizes, all with method 8 in the low nibble
    D_TC_CHECK(dt::has_zlib_magic(tc_zlib_header(0x08u)));
    D_TC_CHECK(dt::has_zlib_magic(tc_zlib_header(0x18u)));
    D_TC_CHECK(dt::has_zlib_magic(tc_zlib_header(0x28u)));
    D_TC_CHECK(dt::has_zlib_magic(tc_zlib_header(0x48u)));
    D_TC_CHECK(dt::has_zlib_magic(tc_zlib_header(0x68u)));

    // payload past the header is not inspected
    dj::byte_buffer with_tail = h;

    with_tail.append(64, static_cast<char>(0xA5u));
    D_TC_CHECK(dt::has_zlib_magic(with_tail));

    return true;
}

/*
tests_compress_zlib_magic_rejects_bad_header
  the two ways a zlib header can be wrong.
  Tests the following:
  - a compression method other than 8 is rejected, whatever the checksum
  - a correct method with a pair that is not a multiple of 31 is rejected
  - a buffer shorter than two bytes is rejected
  - unlike the fixed-magic codecs, the check is arithmetic rather than a
    byte comparison, so a wrong FLG is caught even with a right CMF
*/
bool
tests_compress_zlib_magic_rejects_bad_header()
{
    unsigned int flg = 0;

    // a non-DEFLATE method, even where the pair divides by 31
    for (flg = 0; flg < 256u; ++flg)
    {
        if ((((0x77u << 8) | flg) % 31u) == 0u)
        {
            dj::byte_buffer b;

            b.push_back(static_cast<char>(0x77u));   // low nibble 7, not 8
            b.push_back(static_cast<char>(flg));

            D_TC_CHECK((((0x77u << 8) | flg) % 31u) == 0u);
            D_TC_CHECK(!dt::has_zlib_magic(b));
            break;
        }
    }

    // the right method with a failing FCHECK
    dj::byte_buffer bad = tc_zlib_header(0x78u);

    bad[1] = static_cast<char>(
        static_cast<unsigned char>(
            (static_cast<unsigned char>(bad[1]) + 1u) & 0xFFu));

    D_TC_CHECK((static_cast<unsigned int>(
                    static_cast<unsigned char>(bad[0])) & 0x0Fu) == 0x08u);
    D_TC_CHECK(!dt::has_zlib_magic(bad));

    // the length guard
    D_TC_CHECK(!dt::has_zlib_magic(dj::byte_buffer()));
    D_TC_CHECK(!dt::has_zlib_magic(dj::byte_buffer("\x78")));

    // and the other codecs' magics are not zlib headers
    D_TC_CHECK(!dt::has_zlib_magic(tc_bytes(tc_m_bzip2, 3)));
    D_TC_CHECK(!dt::has_zlib_magic(tc_bytes(tc_m_lz4, 4)));

    return true;
}

/*
tests_compress_bzip2_magic
  the three-byte BZh signature.
  Tests the following:
  - a buffer opening "BZh" is accepted, with trailing content ignored
  - a wrong byte at any of the three positions is rejected
  - a buffer shorter than three bytes is rejected, including the two-byte
    prefix that would pass a laxer check
*/
bool
tests_compress_bzip2_magic()
{
    D_TC_CHECK(dt::has_bzip2_magic(tc_bytes(tc_m_bzip2, 3)));
    D_TC_CHECK(dt::has_bzip2_magic(tc_prefix(tc_m_bzip2, 3, 32)));

    // a real bzip2 stream carries the block-size digit next; still accepted
    D_TC_CHECK(dt::has_bzip2_magic(dj::byte_buffer("BZh9\x31\x41\x59")));

    const std::vector<dj::byte_buffer> bad =
        tc_corruptions(tc_prefix(tc_m_bzip2, 3, 8), 3);
    std::size_t                        i = 0;

    D_TC_CHECK(bad.size() == 3u);

    for (i = 0; i < bad.size(); ++i)
    {
        D_TC_CHECK(!dt::has_bzip2_magic(bad[i]));
    }

    // the length guard, including the near-miss two-byte prefix
    const std::vector<dj::byte_buffer> shortb =
        tc_truncations(tc_bytes(tc_m_bzip2, 3));

    D_TC_CHECK(shortb.size() == 3u);

    for (i = 0; i < shortb.size(); ++i)
    {
        D_TC_CHECK(!dt::has_bzip2_magic(shortb[i]));
    }

    D_TC_CHECK(!dt::has_bzip2_magic(dj::byte_buffer("BZ")));

    return true;
}

/*
tests_compress_xz_magic
  the six-byte xz signature.
  Tests the following:
  - the full six-byte header is accepted, with trailing content ignored
  - a wrong byte at any of the six positions is rejected, so the whole
    signature is compared rather than a prefix of it
  - every buffer shorter than six bytes is rejected
*/
bool
tests_compress_xz_magic()
{
    D_TC_CHECK(dt::has_xz_magic(tc_bytes(tc_m_xz, 6)));
    D_TC_CHECK(dt::has_xz_magic(tc_prefix(tc_m_xz, 6, 100)));

    const std::vector<dj::byte_buffer> bad =
        tc_corruptions(tc_prefix(tc_m_xz, 6, 8), 6);
    std::size_t                        i = 0;

    D_TC_CHECK(bad.size() == 6u);

    // every position matters, including the trailing NUL
    for (i = 0; i < bad.size(); ++i)
    {
        D_TC_CHECK(!dt::has_xz_magic(bad[i]));
    }

    const std::vector<dj::byte_buffer> shortb =
        tc_truncations(tc_bytes(tc_m_xz, 6));

    D_TC_CHECK(shortb.size() == 6u);

    for (i = 0; i < shortb.size(); ++i)
    {
        D_TC_CHECK(!dt::has_xz_magic(shortb[i]));
    }

    // the ASCII middle of the signature alone is not enough
    D_TC_CHECK(!dt::has_xz_magic(dj::byte_buffer("7zXZ\0\0", 6)));

    return true;
}

/*
tests_compress_zstd_magic
  the four-byte Zstandard frame magic.
  Tests the following:
  - the little-endian 28 B5 2F FD header is accepted, trailing content
    ignored
  - a wrong byte at any of the four positions is rejected
  - every buffer shorter than four bytes is rejected
*/
bool
tests_compress_zstd_magic()
{
    D_TC_CHECK(dt::has_zstd_magic(tc_bytes(tc_m_zstd, 4)));
    D_TC_CHECK(dt::has_zstd_magic(tc_prefix(tc_m_zstd, 4, 40)));

    const std::vector<dj::byte_buffer> bad =
        tc_corruptions(tc_prefix(tc_m_zstd, 4, 8), 4);
    std::size_t                        i = 0;

    D_TC_CHECK(bad.size() == 4u);

    for (i = 0; i < bad.size(); ++i)
    {
        D_TC_CHECK(!dt::has_zstd_magic(bad[i]));
    }

    const std::vector<dj::byte_buffer> shortb =
        tc_truncations(tc_bytes(tc_m_zstd, 4));

    D_TC_CHECK(shortb.size() == 4u);

    for (i = 0; i < shortb.size(); ++i)
    {
        D_TC_CHECK(!dt::has_zstd_magic(shortb[i]));
    }

    // the byte order is little-endian, so the reversed magic is rejected
    unsigned char rev[4];

    rev[0] = tc_m_zstd[3];
    rev[1] = tc_m_zstd[2];
    rev[2] = tc_m_zstd[1];
    rev[3] = tc_m_zstd[0];

    D_TC_CHECK(!dt::has_zstd_magic(tc_bytes(rev, 4)));

    return true;
}

/*
tests_compress_lz4_frame_magic
  the four-byte LZ4 frame magic.
  Tests the following:
  - the little-endian 04 22 4D 18 header is accepted, trailing content
    ignored
  - a wrong byte at any of the four positions is rejected
  - every buffer shorter than four bytes is rejected
  - it is distinct from the Zstandard magic of the same width
*/
bool
tests_compress_lz4_frame_magic()
{
    D_TC_CHECK(dt::has_lz4_frame_magic(tc_bytes(tc_m_lz4, 4)));
    D_TC_CHECK(dt::has_lz4_frame_magic(tc_prefix(tc_m_lz4, 4, 40)));

    const std::vector<dj::byte_buffer> bad =
        tc_corruptions(tc_prefix(tc_m_lz4, 4, 8), 4);
    std::size_t                        i = 0;

    D_TC_CHECK(bad.size() == 4u);

    for (i = 0; i < bad.size(); ++i)
    {
        D_TC_CHECK(!dt::has_lz4_frame_magic(bad[i]));
    }

    const std::vector<dj::byte_buffer> shortb =
        tc_truncations(tc_bytes(tc_m_lz4, 4));

    D_TC_CHECK(shortb.size() == 4u);

    for (i = 0; i < shortb.size(); ++i)
    {
        D_TC_CHECK(!dt::has_lz4_frame_magic(shortb[i]));
    }

    // the two four-byte magics do not collide
    D_TC_CHECK(!dt::has_lz4_frame_magic(tc_bytes(tc_m_zstd, 4)));
    D_TC_CHECK(!dt::has_zstd_magic(tc_bytes(tc_m_lz4, 4)));

    return true;
}

/*
tests_compress_magic_predicates_reject_empty
  the shared length guard.
  Tests the following:
  - every framed predicate rejects an empty buffer, so none of them reads
    past the end of a zero-length payload
  - each also rejects a one-byte buffer
  - none of them accepts a buffer of the wrong codec's magic
*/
bool
tests_compress_magic_predicates_reject_empty()
{
    const dj::byte_buffer empty;
    const dj::byte_buffer one(1, static_cast<char>(0x1Fu));

    D_TC_CHECK(!dt::has_gzip_magic(empty));
    D_TC_CHECK(!dt::has_zlib_magic(empty));
    D_TC_CHECK(!dt::has_bzip2_magic(empty));
    D_TC_CHECK(!dt::has_xz_magic(empty));
    D_TC_CHECK(!dt::has_zstd_magic(empty));
    D_TC_CHECK(!dt::has_lz4_frame_magic(empty));

    D_TC_CHECK(!dt::has_gzip_magic(one));
    D_TC_CHECK(!dt::has_zlib_magic(one));
    D_TC_CHECK(!dt::has_bzip2_magic(one));
    D_TC_CHECK(!dt::has_xz_magic(one));
    D_TC_CHECK(!dt::has_zstd_magic(one));
    D_TC_CHECK(!dt::has_lz4_frame_magic(one));

    // no predicate accepts another codec's header
    const dj::byte_buffer gz = tc_prefix(tc_m_gzip, 2, 8);

    D_TC_CHECK(dt::has_gzip_magic(gz));
    D_TC_CHECK(!dt::has_bzip2_magic(gz));
    D_TC_CHECK(!dt::has_xz_magic(gz));
    D_TC_CHECK(!dt::has_zstd_magic(gz));
    D_TC_CHECK(!dt::has_lz4_frame_magic(gz));

    const dj::byte_buffer xz = tc_prefix(tc_m_xz, 6, 8);

    D_TC_CHECK(dt::has_xz_magic(xz));
    D_TC_CHECK(!dt::has_gzip_magic(xz));
    D_TC_CHECK(!dt::has_zstd_magic(xz));

    return true;
}

/*
tests_compress_expected_signature_dispatch
  the tag-to-predicate mapping.
  Tests the following:
  - each framed codec tag routes to the predicate it is required to satisfy,
    accepting its own magic
  - each rejects every other codec's magic, so no two tags share a route
  - the dispatch agrees with calling the predicate directly
*/
bool
tests_compress_expected_signature_dispatch()
{
    const dj::byte_buffer gz   = tc_prefix(tc_m_gzip, 2, 16);
    const dj::byte_buffer zl   = tc_zlib_header();
    const dj::byte_buffer bz   = tc_prefix(tc_m_bzip2, 3, 16);
    const dj::byte_buffer xz   = tc_prefix(tc_m_xz, 6, 16);
    const dj::byte_buffer zs   = tc_prefix(tc_m_zstd, 4, 16);
    const dj::byte_buffer l4   = tc_prefix(tc_m_lz4, 4, 16);

    // each tag accepts its own framing
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::gzip>(gz));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::zlib>(zl));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::bzip2>(bz));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::xz>(xz));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::zstd>(zs));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::lz4>(l4));

    // and the dispatch agrees with the predicate it routes to
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::gzip>(gz) ==
               dt::has_gzip_magic(gz));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::zlib>(zl) ==
               dt::has_zlib_magic(zl));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::bzip2>(bz) ==
               dt::has_bzip2_magic(bz));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::xz>(xz) ==
               dt::has_xz_magic(xz));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::zstd>(zs) ==
               dt::has_zstd_magic(zs));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::lz4>(l4) ==
               dt::has_lz4_frame_magic(l4));

    // no tag accepts a foreign framing
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::gzip>(bz));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::bzip2>(gz));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::xz>(zs));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::zstd>(l4));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::lz4>(zs));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::zlib>(bz));

    // and each rejects an empty buffer
    const dj::byte_buffer empty;

    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::gzip>(empty));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::zlib>(empty));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::bzip2>(empty));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::xz>(empty));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::zstd>(empty));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::lz4>(empty));

    return true;
}

/*
tests_compress_expected_signature_unframed_codecs
  the three codecs with no constant prefix.
  Tests the following:
  - store, raw DEFLATE and brotli pass unconditionally, since none of them
    carries a fixed leading magic
  - they pass on an empty buffer and on arbitrary bytes alike
  - a caller therefore learns nothing from framing for these, which is
    exactly what the module documents
*/
bool
tests_compress_expected_signature_unframed_codecs()
{
    const dj::byte_buffer empty;
    const dj::byte_buffer junk("not a stream of any kind at all");
    const dj::byte_buffer other = tc_prefix(tc_m_xz, 6, 16);

    // unconditionally true, whatever they are handed
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::store>(empty));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::store>(junk));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::store>(other));

    D_TC_CHECK(dt::has_expected_signature<dj::codecs::deflate>(empty));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::deflate>(junk));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::deflate>(other));

    D_TC_CHECK(dt::has_expected_signature<dj::codecs::brotli>(empty));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::brotli>(junk));
    D_TC_CHECK(dt::has_expected_signature<dj::codecs::brotli>(other));

    // the framed codecs do NOT behave that way on the same inputs, which is
    // what makes the distinction meaningful
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::gzip>(junk));
    D_TC_CHECK(!dt::has_expected_signature<dj::codecs::xz>(junk));

    return true;
}

NS_END  // testing
NS_END  // djinterp
