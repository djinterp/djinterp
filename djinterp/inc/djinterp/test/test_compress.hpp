/******************************************************************************
* djinterp [test]                                            test_compress.hpp
*
*   A common testing module for compression.  This is NOT the unit-test suite
* for any one codec; it is the shared, reusable surface any test suite reaches
* for when it needs to VERIFY compressed output - whether that output came from
* compress.hpp directly or from some higher-level module that leans on it (the
* archive layer, the pdf writer, a document bundler, and so on).
*
*   It offers four things a compression-consuming test repeatedly wants:
*   - PAYLOADS      deterministic input corpora (empty, tiny, text, repetitive,
*                   incompressible, NUL-bearing, and large enough to cross the
*                   codecs' internal chunk boundaries), so every suite exercises
*                   the same representative battery.
*   - SIGNATURES    magic-byte predicates that answer "is this really a gzip /
*                   zlib / bzip2 / xz / zstd / lz4 stream?", plus a tag-dispatched
*                   has_expected_signature<Codec> that maps a codec tag to the
*                   framing it is required to emit (store / raw-deflate / brotli
*                   carry no fixed magic and always pass).
*   - VERIFICATION  the core reusable checks - roundtrip<Codec> (a full
*                   compress/decompress cycle rolled up into an inspectable
*                   report), roundtrips<Codec> (strict: expects the codec
*                   present), facade_roundtrip_ok<Codec> (build-agnostic: a
*                   correct facade either round-trips OR reports the codec
*                   unavailable), decompresses_to<Codec> and is_valid_stream<Codec>
*                   (verify another module's emitted bytes).
*   - AVAILABILITY  is_available<Codec>, a thin re-export so a suite can gate a
*                   real-codec assertion on what this build actually compiled.
*
*   Every helper is built on the NON-throwing facade API (try_compress /
* try_decompress), so this header compiles and works whether or not C++
* exceptions are enabled in the translation unit that includes it.
*
*   PORTABILITY:
*   C++11 minimum.  Header-only; the only djinterp dependency is the compress
* facade it is written to exercise.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST PAYLOAD GENERATORS
* II.   CODEC STREAM SIGNATURES
* III.  ROUND-TRIP AND DECODE VERIFICATION
* IV.   AVAILABILITY
*
*
* path:      /inc/djinterp/test/test_compress.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.21
******************************************************************************/

#ifndef DJINTERP_TEST_COMPRESS_
#define DJINTERP_TEST_COMPRESS_ 1

#ifndef __cplusplus
    #error "test_compress.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"            // namespace macros
#include "../core/util/compress.hpp"       // the facade under exercise


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST PAYLOAD GENERATORS                              ///
///////////////////////////////////////////////////////////////////////////////
//   Deterministic input builders.  Determinism matters: a codec test that
// fails should fail identically on every run and every machine, so none of
// these touch a real random source - the "incompressible" stream is a fixed
// linear-congruential sequence, reproducible from its seed.

// make_repeated
//   function: a buffer of _n copies of byte _c.  The most compressible input
// there is (a single run), useful for confirming a codec actually shrinks
// redundant data and for stressing run-length paths.
inline byte_blob
make_repeated(
    char        _c,
    std::size_t _n
)
{
    return byte_blob(_n, _c);
}

// make_pattern
//   function: _n bytes cycling 0x00,0x01,...,0xFF,0x00,...  Exercises the full
// byte range (including NUL) with mild, regular redundancy.
inline byte_blob
make_pattern(
    std::size_t _n
)
{
    byte_blob out;
    std::size_t i;

    out.reserve(_n);

    for (i = 0; i < _n; ++i)
    {
        out.push_back(static_cast<char>(static_cast<unsigned char>(i & 0xFFu)));
    }

    return out;
}

// make_text
//   function: _n bytes of repeating ASCII prose.  Text-like input with the
// kind of partial redundancy real documents carry - the natural case for the
// codecs the archive / document layers drive.
inline byte_blob
make_text(
    std::size_t _n
)
{
    static const char  k_seed[] =
        "the quick brown fox jumps over the lazy dog. ";
    const std::size_t  k_len    = (sizeof(k_seed) - 1u);
    byte_blob        out;

    out.reserve(_n);

    // tile the seed phrase, then trim to the exact requested length
    while (out.size() < _n)
    {
        std::size_t remaining = (_n - out.size());
        std::size_t take      = (remaining < k_len) ? remaining : k_len;

        out.append(k_seed, take);
    }

    return out;
}

// make_incompressible
//   function: _n pseudo-random bytes from a fixed LCG seeded by _seed.  High
// entropy, so a compressor cannot meaningfully shrink it; this is the input
// that forces the "compressed output may be LARGER than the input" bound and
// the buffer-sizing paths, while remaining perfectly reproducible.
inline byte_blob
make_incompressible(
    std::size_t   _n,
    std::uint32_t _seed = 0x1234ABCDu
)
{
    byte_blob   out;
    std::uint32_t state;
    std::size_t   i;

    out.reserve(_n);

    // a non-zero state keeps the generator from sticking at 0
    state = (_seed != 0u) ? _seed : 0x1234ABCDu;

    for (i = 0; i < _n; ++i)
    {
        // Numerical Recipes LCG; the high byte is the least-correlated
        state = (state * 1664525u) + 1013904223u;
        out.push_back(
            static_cast<char>(static_cast<unsigned char>((state >> 24) & 0xFFu)));
    }

    return out;
}

// make_with_nuls
//   function: _n bytes that deliberately embed NUL (0x00) throughout, so a
// round-trip proves the codec path is length-carrying and never treats the
// buffer as a C string.
inline byte_blob
make_with_nuls(
    std::size_t _n
)
{
    byte_blob out;
    std::size_t i;

    out.reserve(_n);

    for (i = 0; i < _n; ++i)
    {
        // every third byte is NUL; the rest walk the printable alphabet
        char b = ((i % 3u) == 0u)
               ? static_cast<char>(0x00)
               : static_cast<char>(static_cast<unsigned char>('A' + (i % 26u)));

        out.push_back(b);
    }

    return out;
}

// standard_corpus
//   function: a representative battery of inputs for a codec's round-trip
// sweep - empty, single byte, short text, a long single run, the byte-cycle
// pattern, medium prose, medium high-entropy data, a NUL-bearing buffer, and
// two large payloads (prose and high-entropy) chosen to exceed the 64 KiB
// chunk the streaming decoders read in, so the multi-chunk path is covered.
inline std::vector<byte_blob>
standard_corpus()
{
    std::vector<byte_blob> corpus;

    corpus.push_back(byte_blob());                         // empty
    corpus.push_back(byte_blob("A"));                      // one byte
    corpus.push_back(byte_blob("hello, world"));           // short text
    corpus.push_back(make_repeated('Z', 4096));              // long run
    corpus.push_back(make_pattern(1024));                    // byte cycle
    corpus.push_back(make_text(8192));                       // medium prose
    corpus.push_back(make_incompressible(8192));             // medium entropy
    corpus.push_back(make_with_nuls(1500));                  // embedded NULs
    corpus.push_back(make_text(200000));                     // large prose (>64K)
    corpus.push_back(make_incompressible(160000));           // large entropy(>64K)

    return corpus;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  CODEC STREAM SIGNATURES                              ///
///////////////////////////////////////////////////////////////////////////////
//   Magic-byte predicates.  Each answers a narrow question - "does this buffer
// begin with the framing this container mandates?" - and is intentionally
// lenient about everything past the header (the payload is the decoder's
// concern, checked by is_valid_stream below).

// has_gzip_magic
//   function: true iff _b opens with the gzip identification bytes 1F 8B.
inline bool
has_gzip_magic(
    const byte_blob& _b
)
{
    return ( (_b.size() >= 2u)                                     &&
             (static_cast<unsigned char>(_b[0]) == 0x1Fu)          &&
             (static_cast<unsigned char>(_b[1]) == 0x8Bu) );
}

// has_zlib_magic
//   function: true iff _b opens with a well-formed zlib (RFC 1950) header -
// compression method 8 (DEFLATE) in the low nibble of CMF, and the CMF/FLG
// pair a multiple of 31 (the FCHECK constraint).
inline bool
has_zlib_magic(
    const byte_blob& _b
)
{
    unsigned int cmf;
    unsigned int flg;

    if (_b.size() < 2u)
    {
        return false;
    }

    cmf = static_cast<unsigned char>(_b[0]);
    flg = static_cast<unsigned char>(_b[1]);

    // low nibble of CMF is the compression method; 8 == DEFLATE
    if ((cmf & 0x0Fu) != 0x08u)
    {
        return false;
    }

    return (((cmf << 8) | flg) % 31u) == 0u;
}

// has_bzip2_magic
//   function: true iff _b opens with the bzip2 signature "BZh".
inline bool
has_bzip2_magic(
    const byte_blob& _b
)
{
    return ( (_b.size() >= 3u) &&
             (_b[0] == 'B')    &&
             (_b[1] == 'Z')    &&
             (_b[2] == 'h') );
}

// has_xz_magic
//   function: true iff _b opens with the six-byte xz signature
// FD 37 7A 58 5A 00 ("\xFD" "7zXZ" "\0").
inline bool
has_xz_magic(
    const byte_blob& _b
)
{
    static const unsigned char k_magic[6] =
        { 0xFDu, 0x37u, 0x7Au, 0x58u, 0x5Au, 0x00u };
    std::size_t i;

    if (_b.size() < 6u)
    {
        return false;
    }

    for (i = 0; i < 6u; ++i)
    {
        if (static_cast<unsigned char>(_b[i]) != k_magic[i])
        {
            return false;
        }
    }

    return true;
}

// has_zstd_magic
//   function: true iff _b opens with the Zstandard frame magic, stored
// little-endian as 28 B5 2F FD.
inline bool
has_zstd_magic(
    const byte_blob& _b
)
{
    static const unsigned char k_magic[4] =
        { 0x28u, 0xB5u, 0x2Fu, 0xFDu };
    std::size_t i;

    if (_b.size() < 4u)
    {
        return false;
    }

    for (i = 0; i < 4u; ++i)
    {
        if (static_cast<unsigned char>(_b[i]) != k_magic[i])
        {
            return false;
        }
    }

    return true;
}

// has_lz4_frame_magic
//   function: true iff _b opens with the LZ4 frame magic, stored
// little-endian as 04 22 4D 18.
inline bool
has_lz4_frame_magic(
    const byte_blob& _b
)
{
    static const unsigned char k_magic[4] =
        { 0x04u, 0x22u, 0x4Du, 0x18u };
    std::size_t i;

    if (_b.size() < 4u)
    {
        return false;
    }

    for (i = 0; i < 4u; ++i)
    {
        if (static_cast<unsigned char>(_b[i]) != k_magic[i])
        {
            return false;
        }
    }

    return true;
}

NS_INTERNAL

    // signature_of
    //   helper: overload set mapping a codec tag to the framing predicate it
    // must satisfy.  store, raw deflate, and brotli carry no fixed leading
    // magic (raw DEFLATE has no container; brotli's stream has no constant
    // prefix), so they always pass - a caller checking those learns nothing
    // from framing and should rely on a round-trip instead.
    inline bool signature_of(codecs::store,   const byte_blob&)    { return true; }
    inline bool signature_of(codecs::deflate, const byte_blob&)    { return true; }
    inline bool signature_of(codecs::zlib,    const byte_blob& _b) { return has_zlib_magic(_b); }
    inline bool signature_of(codecs::gzip,    const byte_blob& _b) { return has_gzip_magic(_b); }
    inline bool signature_of(codecs::bzip2,   const byte_blob& _b) { return has_bzip2_magic(_b); }
    inline bool signature_of(codecs::xz,      const byte_blob& _b) { return has_xz_magic(_b); }
    inline bool signature_of(codecs::zstd,    const byte_blob& _b) { return has_zstd_magic(_b); }
    inline bool signature_of(codecs::lz4,     const byte_blob& _b) { return has_lz4_frame_magic(_b); }
    inline bool signature_of(codecs::brotli,  const byte_blob&)    { return true; }

NS_END  // internal

// has_expected_signature
//   function: true iff _b carries the leading framing codec _Codec is required
// to emit.  Tag-dispatched over the overload set above; the codecs without a
// constant prefix (store, deflate, brotli) return true unconditionally.
template<typename _Codec>
inline bool
has_expected_signature(
    const byte_blob& _b
)
{
    return internal::signature_of(_Codec(), _b);
}


///////////////////////////////////////////////////////////////////////////////
///                III. ROUND-TRIP AND DECODE VERIFICATION                   ///
///////////////////////////////////////////////////////////////////////////////

// roundtrip_report
//   struct: the full record of one compress/decompress cycle, so a caller can
// inspect exactly what happened rather than collapsing it to a single bool.
// `available` reflects whether this build actually compiled the codec;
// `restored_equals_input` is true only when decompression both succeeded and
// reproduced the original bytes.
struct roundtrip_report
{
    bool        available;
    status      compress_status;
    status      decompress_status;
    byte_blob compressed;
    byte_blob restored;
    bool        restored_equals_input;
    std::size_t input_size;
    std::size_t compressed_size;
};

// roundtrip
//   function: run _in through try_compress<_Codec> then try_decompress<_Codec>
// and return the populated report.  Never throws (built on the non-throwing
// API).  When the codec is unavailable, both calls report status_unavailable
// and restored_equals_input is false; inspect .available to distinguish that
// from a genuine failure.
template<typename _Codec>
inline roundtrip_report
roundtrip(
    const byte_blob&      _in,
    const compress_options& _opt = compress_options()
)
{
    roundtrip_report r;

    r.available         = codec_is_available<_Codec>();
    r.input_size        = _in.size();

    r.compress_status   = try_compress<_Codec>(_in, r.compressed, _opt);
    r.compressed_size   = r.compressed.size();

    r.decompress_status = try_decompress<_Codec>(r.compressed, r.restored);

    r.restored_equals_input =
        ( (r.decompress_status == status_ok) && (r.restored == _in) );

    return r;
}

// roundtrips
//   function: the strict round-trip check - true iff _Codec is available AND
// the cycle succeeds AND the output equals the input.  Use when the caller has
// already established (or requires) that the codec is present.
template<typename _Codec>
inline bool
roundtrips(
    const byte_blob&      _in,
    const compress_options& _opt = compress_options()
)
{
    roundtrip_report r = roundtrip<_Codec>(_in, _opt);

    return ( r.available                             &&
             (r.compress_status   == status_ok)      &&
             (r.decompress_status == status_ok)      &&
             r.restored_equals_input );
}

// facade_roundtrip_ok
//   function: the BUILD-AGNOSTIC correctness check.  A correct facade does one
// of two things with a codec: if it is available, a round-trip reproduces the
// input; if it is not, compression reports status_unavailable and produces no
// output.  Returns true in either correct case, false otherwise - so a suite
// can assert it for every codec regardless of what this build compiled.
template<typename _Codec>
inline bool
facade_roundtrip_ok(
    const byte_blob&      _in,
    const compress_options& _opt = compress_options()
)
{
    roundtrip_report r = roundtrip<_Codec>(_in, _opt);

    // available: must complete the cycle and reproduce the input
    if (r.available)
    {
        return ( (r.compress_status   == status_ok) &&
                 (r.decompress_status == status_ok) &&
                 r.restored_equals_input );
    }

    // unavailable: must SAY so, not partially succeed
    return (r.compress_status == status_unavailable);
}

// decompresses_to
//   function: true iff _compressed decodes (status_ok) under _Codec and the
// result equals _expected.  The primary helper for a suite verifying that some
// other module emitted a stream that unpacks to a known payload.
template<typename _Codec>
inline bool
decompresses_to(
    const byte_blob& _compressed,
    const byte_blob& _expected
)
{
    byte_blob out;
    status      s;

    s = try_decompress<_Codec>(_compressed, out);

    return ( (s == status_ok) && (out == _expected) );
}

// is_valid_stream
//   function: true iff _compressed decodes cleanly (status_ok) under _Codec,
// disregarding the decoded value.  Answers "is this a well-formed _Codec
// stream?" for a producer whose exact payload the caller does not have on
// hand.
template<typename _Codec>
inline bool
is_valid_stream(
    const byte_blob& _compressed
)
{
    byte_blob out;

    return (try_decompress<_Codec>(_compressed, out) == status_ok);
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  AVAILABILITY                                         ///
///////////////////////////////////////////////////////////////////////////////

// is_available
//   function: runtime availability of _Codec in this build.  A thin re-export
// of the facade query, so a suite can gate a real-codec assertion without
// naming the internal dispatch.
template<typename _Codec>
inline bool
is_available()
{
    return codec_is_available<_Codec>();
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_COMPRESS_
