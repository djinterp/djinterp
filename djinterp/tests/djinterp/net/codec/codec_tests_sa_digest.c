/*******************************************************************************
* djinterp [net]                                         codec_tests_sa_digest.c
*
* Standalone tests of codec_digest.h: MD5, SHA-1 and SHA-256 against their
* documents' known answers, HMAC against RFC 2202's and RFC 4231's, and what
* no document states: data in pieces, keys around a block's length, outputs
* that overlap inputs, refusals, comparison, and wiping.
*
*
* path:      /tests/djinterp/net/codec/codec_tests_sa_digest.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "./codec_tests_sa.h"  // the suite
// std
#include <stdio.h>   // printf
#include <string.h>  // memcmp, memcpy, memset, strcmp, strlen, strncmp
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_bytes
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t


// D_TESTS_CODEC_MESSAGES
//   constant: the messages the known answers are for: RFC 1321's suite; FIPS
// 180's two examples of 56 and 112 octets, each of which leaves no room for
// the length in its last block; and 55, 63 and 64 octets, the longest message
// whose padding fits one block, and a block's end either side.
static const char* const D_TESTS_CODEC_MESSAGES[] =
{
    "",
    "a",
    "abc",
    "message digest",
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    "1234567890123456789012345678901234567890"
    "1234567890123456789012345678901234567890",
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
    "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012",
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-",
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
};

// D_TESTS_CODEC_MESSAGE_COUNT
//   constant: how many messages D_TESTS_CODEC_MESSAGES lists.
#define D_TESTS_CODEC_MESSAGE_COUNT                                            \
    (sizeof(D_TESTS_CODEC_MESSAGES) / sizeof(D_TESTS_CODEC_MESSAGES[0]))

// D_TESTS_CODEC_MD5
//   constant: MD5 of each message (RFC 1321 appendix A.5's first seven).
static const char* const D_TESTS_CODEC_MD5[] =
{
    "d41d8cd98f00b204e9800998ecf8427e",
    "0cc175b9c0f1b6a831c399e269772661",
    "900150983cd24fb0d6963f7d28e17f72",
    "f96b697d7cb7938d525a2f31aaf161d0",
    "c3fcd3d76192e4007dfb496cca67e13b",
    "d174ab98d277d9f5a5611c2c9f419d9f",
    "57edf4a22be3c955ac49da2e2107b67a",
    "8215ef0796a20bcaaae116d3876c664a",
    "03dd8807a93175fb062dfb55dc7d359c",
    "3d373b8cd6fd069d313cdc3f38a18963",
    "ce3a13cb6c59e1dad8a170ecd50f0ce8",
    "2a3787f99207e36b2cb2c3406892def0"
};

// D_TESTS_CODEC_SHA1
//   constant: SHA-1 of each message (FIPS 180's examples among them).
static const char* const D_TESTS_CODEC_SHA1[] =
{
    "da39a3ee5e6b4b0d3255bfef95601890afd80709",
    "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8",
    "a9993e364706816aba3e25717850c26c9cd0d89d",
    "c12252ceda8be8994d5fa0290a47231c1d16aae3",
    "32d10c7b8cf96570ca04ce37f2a19d84240d3a89",
    "761c457bf73b14d27e9e9265c46f4b4dda11f940",
    "50abf5706a150990a08b2c5ea40fa0e585554732",
    "84983e441c3bd26ebaae4aa1f95129e5e54670f1",
    "a49b2446a02c645bf419f995b67091253a04a259",
    "ed8b55273b7180b9a64b763ffd802939834d6d6c",
    "63906a17c4247f957df2a7da49615fb7527e6dd6",
    "348ac0938761f10a8980384da206e2516ae3d4b5"
};

// D_TESTS_CODEC_SHA256
//   constant: SHA-256 of each message (FIPS 180's examples among them).
static const char* const D_TESTS_CODEC_SHA256[] =
{
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb",
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
    "f7846f55cf23e14eebeab5b4e1550cad5b509e3348fbc4efa3a1413d393cb650",
    "71c480df93d6ae2f1efad1447c66c9525e316218cf51fc8d9ed832f2daf18b73",
    "db4bfcbd4da0cd85a60c3c37d3fbd8805c77f15fc6b1fdfe614ee0a7c8fdb4c0",
    "f371bc4a311f2b009eef952dd83ca80e2b60026c8e935592d0f9c308453c813e",
    "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
    "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1",
    "d74ba075e4259c6c807c4101e66d281096cf9ff14ba01260dee741b1bdaef326",
    "46372051e032316ff56a181a3b27c36d22fa0f725bf12470878de68f7f2a98cd",
    "dfc806def494bcc996f23e096484f171432a19944968eff9a76c09dab64d0a44"
};

// D_TESTS_CODEC_KINDS
//   constant: the three digest functions, for tests that run on each.
static const enum d_net_digest_kind D_TESTS_CODEC_KINDS[] =
{
    D_NET_DIGEST_MD5,
    D_NET_DIGEST_SHA1,
    D_NET_DIGEST_SHA256
};

// D_TESTS_CODEC_LARGE_KEY_FIRST
//   constant: the message of the first large-key case in RFC 2202 and 4231.
#define D_TESTS_CODEC_LARGE_KEY_FIRST                                          \
    "Test Using Larger Than Block-Size Key - Hash Key First"

// D_TESTS_CODEC_HMAC_MD5
//   constant: RFC 2202 section 2's seven HMAC-MD5 cases.
static const struct d_tests_codec_mac D_TESTS_CODEC_HMAC_MD5[] =
{
    { { .fill = 0x0b, .count = 16 },
      { .text = "Hi There" },
      "9294727a3638bb1c13f48ef8158bfc9d" },
    { { .text = "Jefe" },
      { .text = "what do ya want for nothing?" },
      "750c783e6ab0b503eaa86e310a5db738" },
    { { .fill = 0xaa, .count = 16 },
      { .fill = 0xdd, .count = 50 },
      "56be34521d144c88dbb8c733f0e8b3f6" },
    { { .count = 25 },
      { .fill = 0xcd, .count = 50 },
      "697eaf0aca3a3aea3a75164746ffaa79" },
    { { .fill = 0x0c, .count = 16 },
      { .text = "Test With Truncation" },
      "56461ef2342edc00f9bab995690efd4c" },
    { { .fill = 0xaa, .count = 80 },
      { .text = D_TESTS_CODEC_LARGE_KEY_FIRST },
      "6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd" },
    { { .fill = 0xaa, .count = 80 },
      { .text = "Test Using Larger Than Block-Size Key and "
                "Larger Than One Block-Size Data" },
      "6f630fad67cda0ee1fb1f562db3aa53e" }
};

// D_TESTS_CODEC_HMAC_SHA1
//   constant: RFC 2202 section 3's seven HMAC-SHA1 cases.
static const struct d_tests_codec_mac D_TESTS_CODEC_HMAC_SHA1[] =
{
    { { .fill = 0x0b, .count = 20 },
      { .text = "Hi There" },
      "b617318655057264e28bc0b6fb378c8ef146be00" },
    { { .text = "Jefe" },
      { .text = "what do ya want for nothing?" },
      "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79" },
    { { .fill = 0xaa, .count = 20 },
      { .fill = 0xdd, .count = 50 },
      "125d7342b9ac11cd91a39af48aa17b4f63f175d3" },
    { { .count = 25 },
      { .fill = 0xcd, .count = 50 },
      "4c9007f4026250c6bc8414f9bf50c86c2d7235da" },
    { { .fill = 0x0c, .count = 20 },
      { .text = "Test With Truncation" },
      "4c1a03424b55e07fe7f27be1d58bb9324a9a5a04" },
    { { .fill = 0xaa, .count = 80 },
      { .text = D_TESTS_CODEC_LARGE_KEY_FIRST },
      "aa4ae5e15272d00e95705637ce8a3b55ed402112" },
    { { .fill = 0xaa, .count = 80 },
      { .text = "Test Using Larger Than Block-Size Key and "
                "Larger Than One Block-Size Data" },
      "e8e99d0f45237d786d6bbaa7965c7808bbff1a91" }
};

// D_TESTS_CODEC_HMAC_SHA256
//   constant: RFC 4231 section 4's seven HMAC-SHA-256 cases; the fifth gives
// only the MAC's first 128 bits.
static const struct d_tests_codec_mac D_TESTS_CODEC_HMAC_SHA256[] =
{
    { { .fill = 0x0b, .count = 20 },
      { .text = "Hi There" },
      "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7" },
    { { .text = "Jefe" },
      { .text = "what do ya want for nothing?" },
      "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843" },
    { { .fill = 0xaa, .count = 20 },
      { .fill = 0xdd, .count = 50 },
      "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe" },
    { { .count = 25 },
      { .fill = 0xcd, .count = 50 },
      "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b" },
    { { .fill = 0x0c, .count = 20 },
      { .text = "Test With Truncation" },
      "a3b6167473100ee06e0c796c2955552b" },
    { { .fill = 0xaa, .count = 131 },
      { .text = D_TESTS_CODEC_LARGE_KEY_FIRST },
      "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54" },
    { { .fill = 0xaa, .count = 131 },
      { .text = "This is a test using a larger than block-size "
                "key and a larger than block-size data. The "
                "key needs to be hashed before being used by "
                "the HMAC algorithm." },
      "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2" }
};

/*
d_tests_codec_hex
  A table lookup per nibble, written out here rather than taken from the codec,
so a fault in the codec cannot hide itself.
*/
void
d_tests_codec_hex(
    const uint8_t* _bytes,
    size_t         _size,
    char*          _out
)
{
    static const char DIGITS[] = "0123456789abcdef";

    // two digits per octet, high nibble first
    for (size_t i = 0; i < _size; ++i)
    {
        _out[2 * i]       = DIGITS[_bytes[i] >> 4];
        _out[(2 * i) + 1] = DIGITS[_bytes[i] & 0x0fu];
    }

    _out[2 * _size] = '\0';

    return;
}

/*
d_tests_codec_span_fill
  Text is copied; a fill repeats its octet; a run counts up from 1.
*/
size_t
d_tests_codec_span_fill(
    const struct d_tests_codec_span* _span,
    uint8_t*                         _out
)
{
    // text is its own octets
    if (_span->text != NULL)
    {
        const size_t length = strlen(_span->text);

        memcpy(_out,
               _span->text,
               length);

        return length;
    }

    // a fill, or a run from 1
    for (size_t i = 0; i < _span->count; ++i)
    {
        _out[i] = (_span->fill != 0) ? _span->fill
                                     : (uint8_t)(i + 1);
    }

    return _span->count;
}

/*
d_tests_codec_digest_matches
  Whether `_kind`'s digest of `_data` renders as `_expected`; prints what it
got when it does not.
*/
static bool
d_tests_codec_digest_matches(
    enum d_net_digest_kind _kind,
    struct d_pack_bytes    _data,
    const char*            _expected
)
{
    uint8_t    digest[D_NET_DIGEST_MAX_SIZE] = { 0 };
    char       hex[D_TESTS_CODEC_HEX_MAX]    = { 0 };
    const bool computed                      = d_net_digest_compute(_kind,
                                                                    _data,
                                                                    digest);

    d_tests_codec_hex(digest,
                      d_net_digest_size(_kind),
                      hex);

    // say what came out, for a failure to be read from the log
    if ( (!computed) ||
         (strcmp(hex,
                 _expected) != 0) )
    {
        printf("    got %s\n    for %s\n",
               hex,
               _expected);

        return false;
    }

    return true;
}

/*
d_tests_codec_known
  Every message of D_TESTS_CODEC_MESSAGES against `_expected`, its digests
under `_kind`.
*/
static bool
d_tests_codec_known(
    enum d_net_digest_kind _kind,
    const char* const*     _expected,
    const char*            _test,
    struct d_test_counter* _counter
)
{
    bool result = true;

    // each message against its known answer
    for (size_t i = 0; i < D_TESTS_CODEC_MESSAGE_COUNT; ++i)
    {
        const struct d_pack_bytes message =
        {
            D_TESTS_CODEC_MESSAGES[i],
            strlen(D_TESTS_CODEC_MESSAGES[i])
        };

        result = d_tests_net_assert(d_tests_codec_digest_matches(_kind,
                                                                 message,
                                                                 _expected[i]),
                                    _test,
                                    "a message digests to its known answer",
                                    _counter) && result;
    }

    return result;
}

/*
d_tests_sa_codec_digest_sizes
  Tests the following:
  - d_net_digest_size gives 16, 20 and 32 octets for MD5, SHA-1 and SHA-256
  - it gives 0 for D_NET_DIGEST_NONE and for a value that names no function
  - D_NET_DIGEST_MAX_SIZE is the largest of the three
*/
bool
d_tests_sa_codec_digest_sizes(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "digest sizes";
    bool              result = true;

    result = d_tests_net_assert(
                 ( (d_net_digest_size(D_NET_DIGEST_MD5) == 16)  &&
                   (d_net_digest_size(D_NET_DIGEST_SHA1) == 20) &&
                   (d_net_digest_size(D_NET_DIGEST_SHA256) == 32) ),
                 TEST,
                 "each function's digest has its documented size",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_net_digest_size(D_NET_DIGEST_NONE) == 0) &&
                   (d_net_digest_size((enum d_net_digest_kind)99) == 0) ),
                 TEST,
                 "no function, no size",
                 _counter) && result;
    result = d_tests_net_assert(
                 (D_NET_DIGEST_MAX_SIZE == D_NET_DIGEST_SHA256_SIZE),
                 TEST,
                 "the largest digest is SHA-256's",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_md5_known
  Tests the following:
  - MD5 of RFC 1321's seven messages, of FIPS 180's two, and of messages
    either side of a block's end, is as known
*/
bool
d_tests_sa_codec_md5_known(
    struct d_test_counter* _counter
)
{
    return d_tests_codec_known(D_NET_DIGEST_MD5,
                               D_TESTS_CODEC_MD5,
                               "MD5 known answers",
                               _counter);
}

/*
d_tests_sa_codec_sha1_known
  Tests the following:
  - SHA-1 of the twelve messages, FIPS 180's among them, is as known
*/
bool
d_tests_sa_codec_sha1_known(
    struct d_test_counter* _counter
)
{
    return d_tests_codec_known(D_NET_DIGEST_SHA1,
                               D_TESTS_CODEC_SHA1,
                               "SHA-1 known answers",
                               _counter);
}

/*
d_tests_sa_codec_sha256_known
  Tests the following:
  - SHA-256 of the twelve messages, FIPS 180's among them, is as known
*/
bool
d_tests_sa_codec_sha256_known(
    struct d_test_counter* _counter
)
{
    return d_tests_codec_known(D_NET_DIGEST_SHA256,
                               D_TESTS_CODEC_SHA256,
                               "SHA-256 known answers",
                               _counter);
}

/*
d_tests_sa_codec_digest_million
  Tests the following:
  - a million 'a's, given a thousand at a time, digest under each function to
    the answer FIPS 180 gives for SHA-1 and SHA-256, and to MD5's known one:
    many blocks, and a length past 2^20 octets
*/
bool
d_tests_sa_codec_digest_million(
    struct d_test_counter* _counter
)
{
    static const char* const EXPECTED[] =
    {
        "7707d6ae4e027c70eea2a935c2296f21",
        "34aa973cd4c4daa4f61eeb2bdbad27316534016f",
        "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"
    };
    const char* const TEST    = "a million octets";
    uint8_t           a[1000] = { 0 };
    bool              result  = true;

    memset(a,
           'a',
           sizeof(a));

    // each function over a thousand pieces
    for (size_t k = 0; k < 3; ++k)
    {
        struct d_net_digest digest                     = { 0 };
        uint8_t             out[D_NET_DIGEST_MAX_SIZE] = { 0 };
        char                hex[D_TESTS_CODEC_HEX_MAX] = { 0 };

        d_net_digest_init(&digest,
                          D_TESTS_CODEC_KINDS[k]);

        // a thousand pieces of a thousand
        for (size_t i = 0; i < 1000; ++i)
        {
            d_net_digest_update(&digest,
                                a,
                                sizeof(a));
        }

        const bool done = d_net_digest_final(&digest,
                                             out);

        d_tests_codec_hex(out,
                          d_net_digest_size(D_TESTS_CODEC_KINDS[k]),
                          hex);
        result = d_tests_net_assert( (done) &&
                                     (strcmp(hex,
                                             EXPECTED[k]) == 0),
                                     TEST,
                                     "a million 'a's digest as known",
                                     _counter) && result;
    }

    return result;
}

/*
d_tests_codec_pieces_match
  Whether `_length` octets of `_data`, fed to `_kind` in pieces whose sizes
cycle through SIZES, digest as they do in one piece.
*/
static bool
d_tests_codec_pieces_match(
    enum d_net_digest_kind _kind,
    const uint8_t*         _data,
    size_t                 _length
)
{
    static const size_t       SIZES[] = { 1, 0, 7, 64, 13, 63, 65, 3, 128, 56 };
    const struct d_pack_bytes whole   = { _data,
                                          _length };
    struct d_net_digest       digest                        = { 0 };
    uint8_t                   once[D_NET_DIGEST_MAX_SIZE]   = { 0 };
    uint8_t                   pieces[D_NET_DIGEST_MAX_SIZE] = { 0 };
    size_t                    at                            = 0;

    d_net_digest_init(&digest,
                      _kind);

    // the cycle of piece sizes, the last piece cut to fit
    for (size_t i = 0; at < _length; ++i)
    {
        const size_t want = SIZES[i % (sizeof(SIZES) / sizeof(SIZES[0]))];
        const size_t take = (want < _length - at) ? want
                                                  : _length - at;

        d_net_digest_update(&digest,
                            _data + at,
                            take);
        at += take;
    }

    return ( (d_net_digest_compute(_kind,
                                   whole,
                                   once)) &&
             (d_net_digest_final(&digest,
                                 pieces)) &&
             (memcmp(once,
                     pieces,
                     d_net_digest_size(_kind)) == 0) );
}

/*
d_tests_sa_codec_digest_pieces
  Tests the following:
  - for every length from 0 to 300 octets, under each function, data given in
    pieces of 0, 1, 3, 7, 13, 56, 63, 64, 65 and 128 octets digests as it does
    whole: every way the buffering can meet a block's end
*/
bool
d_tests_sa_codec_digest_pieces(
    struct d_test_counter* _counter
)
{
    const char* const TEST      = "digests in pieces";
    uint8_t           data[300] = { 0 };
    uint32_t          seed      = 12345u;
    bool              result    = true;

    // pseudo-random octets, the same every run
    for (size_t i = 0; i < sizeof(data); ++i)
    {
        seed    = (seed * 1103515245u) + 12345u;
        data[i] = (uint8_t)(seed >> 16);
    }

    // each function, at every length
    for (size_t k = 0; k < 3; ++k)
    {
        bool all = true;

        for (size_t length = 0; length <= sizeof(data); ++length)
        {
            all = d_tests_codec_pieces_match(D_TESTS_CODEC_KINDS[k],
                                             data,
                                             length) && all;
        }

        result = d_tests_net_assert(all,
                                    TEST,
                                    "pieces digest as the whole does",
                                    _counter) && result;
    }

    return result;
}

/*
d_tests_codec_refused
  Whether d_net_digest_final refuses `_digest` and leaves `_out` as it was.
*/
static bool
d_tests_codec_refused(
    struct d_net_digest* _digest
)
{
    uint8_t out[D_NET_DIGEST_MAX_SIZE]    = { 0 };
    uint8_t before[D_NET_DIGEST_MAX_SIZE] = { 0 };

    memset(out,
           0xa5,
           sizeof(out));
    memcpy(before,
           out,
           sizeof(out));

    return ( (!d_net_digest_final(_digest,
                                  out)) &&
             (memcmp(out,
                     before,
                     sizeof(out)) == 0) );
}

/*
d_tests_sa_codec_digest_refusals
  Tests the following:
  - a digest started with no function, or with a value that names none, is
    refused at final, which writes nothing
  - data that is missing spoils a digest; an empty piece does not
  - a finished digest, and one finished without an output, is refused
  - an update after final is ignored, and final still refuses
  - NULL states are ignored by init and update and refused by final
*/
bool
d_tests_sa_codec_digest_refusals(
    struct d_test_counter* _counter
)
{
    const char* const   TEST                       = "digest refusals";
    struct d_net_digest digest                     = { 0 };
    uint8_t             out[D_NET_DIGEST_MAX_SIZE] = { 0 };
    bool                result                     = true;

    d_net_digest_init(&digest,
                      D_NET_DIGEST_NONE);
    result = d_tests_net_assert(d_tests_codec_refused(&digest),
                                TEST,
                                "no function is refused",
                                _counter) && result;
    d_net_digest_init(&digest,
                      (enum d_net_digest_kind)99);
    result = d_tests_net_assert(d_tests_codec_refused(&digest),
                                TEST,
                                "an unknown function is refused",
                                _counter) && result;
    d_net_digest_init(&digest,
                      D_NET_DIGEST_SHA1);
    d_net_digest_update(&digest,
                        NULL,
                        3);
    result = d_tests_net_assert(d_tests_codec_refused(&digest),
                                TEST,
                                "missing data spoils the digest",
                                _counter) && result;
    d_net_digest_init(&digest,
                      D_NET_DIGEST_SHA1);
    d_net_digest_update(&digest,
                        NULL,
                        0);
    result = d_tests_net_assert(d_net_digest_final(&digest,
                                                   out),
                                TEST,
                                "an empty piece is no fault",
                                _counter) && result;
    d_net_digest_update(&digest,
                        "abc",
                        3);
    result = d_tests_net_assert(d_tests_codec_refused(&digest),
                                TEST,
                                "a finished digest takes nothing more",
                                _counter) && result;
    d_net_digest_init(&digest,
                      D_NET_DIGEST_MD5);
    result = d_tests_net_assert( (!d_net_digest_final(&digest,
                                                      NULL)) &&
                                 (d_tests_codec_refused(&digest)),
                                 TEST,
                                 "final without an output ends the digest",
                                 _counter) && result;
    d_net_digest_init(NULL,
                      D_NET_DIGEST_MD5);
    d_net_digest_update(NULL,
                        "abc",
                        3);
    result = d_tests_net_assert(d_tests_codec_refused(NULL),
                                TEST,
                                "no state, no digest",
                                _counter) && result;

    return result;
}

/*
d_tests_codec_macs
  Every case of `_cases` under `_kind`, comparing as much of the MAC as the
case's expected hex gives.
*/
static bool
d_tests_codec_macs(
    enum d_net_digest_kind          _kind,
    const struct d_tests_codec_mac* _cases,
    size_t                          _count,
    const char*                     _test,
    struct d_test_counter*          _counter
)
{
    bool result = true;

    // each case: key and message built, MAC rendered and compared
    for (size_t i = 0; i < _count; ++i)
    {
        uint8_t                   key[D_TESTS_CODEC_SPAN_MAX]  = { 0 };
        uint8_t                   data[D_TESTS_CODEC_SPAN_MAX] = { 0 };
        uint8_t                   mac[D_NET_DIGEST_MAX_SIZE]   = { 0 };
        char                      hex[D_TESTS_CODEC_HEX_MAX]   = { 0 };
        const struct d_pack_bytes key_bytes  =
        {
            key,
            d_tests_codec_span_fill(&_cases[i].key,
                                    key)
        };
        const struct d_pack_bytes data_bytes =
        {
            data,
            d_tests_codec_span_fill(&_cases[i].data,
                                    data)
        };
        const bool                done       = d_net_hmac(_kind,
                                                          key_bytes,
                                                          data_bytes,
                                                          mac);

        d_tests_codec_hex(mac,
                          d_net_digest_size(_kind),
                          hex);
        result = d_tests_net_assert( (done) &&
                                     (strncmp(hex,
                                              _cases[i].expected,
                                              strlen(_cases[i].expected)) ==
                                      0),
                                     _test,
                                     "a case's MAC is as its RFC gives it",
                                     _counter) && result;
    }

    return result;
}

/*
d_tests_sa_codec_hmac_md5_known
  Tests the following:
  - HMAC-MD5 of RFC 2202's seven cases, keys longer than a block among them
*/
bool
d_tests_sa_codec_hmac_md5_known(
    struct d_test_counter* _counter
)
{
    return d_tests_codec_macs(D_NET_DIGEST_MD5,
                              D_TESTS_CODEC_HMAC_MD5,
                              sizeof(D_TESTS_CODEC_HMAC_MD5) /
                              sizeof(D_TESTS_CODEC_HMAC_MD5[0]),
                              "HMAC-MD5 known answers",
                              _counter);
}

/*
d_tests_sa_codec_hmac_sha1_known
  Tests the following:
  - HMAC-SHA1 of RFC 2202's seven cases, keys longer than a block among them
*/
bool
d_tests_sa_codec_hmac_sha1_known(
    struct d_test_counter* _counter
)
{
    return d_tests_codec_macs(D_NET_DIGEST_SHA1,
                              D_TESTS_CODEC_HMAC_SHA1,
                              sizeof(D_TESTS_CODEC_HMAC_SHA1) /
                              sizeof(D_TESTS_CODEC_HMAC_SHA1[0]),
                              "HMAC-SHA1 known answers",
                              _counter);
}

/*
d_tests_sa_codec_hmac_sha256_known
  Tests the following:
  - HMAC-SHA-256 of RFC 4231's seven cases, the truncated fifth by its prefix
*/
bool
d_tests_sa_codec_hmac_sha256_known(
    struct d_test_counter* _counter
)
{
    return d_tests_codec_macs(D_NET_DIGEST_SHA256,
                              D_TESTS_CODEC_HMAC_SHA256,
                              sizeof(D_TESTS_CODEC_HMAC_SHA256) /
                              sizeof(D_TESTS_CODEC_HMAC_SHA256[0]),
                              "HMAC-SHA-256 known answers",
                              _counter);
}

/*
d_tests_codec_mac_of
  The MAC of a fixed message under `_kind`, keyed with `_size` octets of
`_key`, into `_mac`; false if refused.
*/
static bool
d_tests_codec_mac_of(
    enum d_net_digest_kind _kind,
    const uint8_t*         _key,
    size_t                 _size,
    uint8_t*               _mac
)
{
    const struct d_pack_bytes key     = { _key,
                                          _size };
    const struct d_pack_bytes message = { "the quick brown fox",
                                          19 };

    return d_net_hmac(_kind,
                      key,
                      message,
                      _mac);
}

/*
d_tests_codec_key_digested
  Whether `_kind` authenticates with the first `_length` octets of `_key` as it
does with their digest: what RFC 2104 requires of a key longer than a block,
and what cannot happen with one that fits.
*/
static bool
d_tests_codec_key_digested(
    enum d_net_digest_kind _kind,
    const uint8_t*         _key,
    size_t                 _length
)
{
    const size_t              size                          =
        d_net_digest_size(_kind);
    const struct d_pack_bytes key                           = { _key,
                                                                _length };
    uint8_t                   digest[D_NET_DIGEST_MAX_SIZE] = { 0 };
    uint8_t                   a[D_NET_DIGEST_MAX_SIZE]      = { 0 };
    uint8_t                   b[D_NET_DIGEST_MAX_SIZE]      = { 0 };

    return ( (d_net_digest_compute(_kind,
                                   key,
                                   digest)) &&
             (d_tests_codec_mac_of(_kind,
                                   _key,
                                   _length,
                                   a))      &&
             (d_tests_codec_mac_of(_kind,
                                   digest,
                                   size,
                                   b))      &&
             (memcmp(a,
                     b,
                     size) == 0) );
}

/*
d_tests_codec_key_padded
  Whether `_kind` authenticates with 20 octets of `_key` as it does with the
same octets followed by zeros to a block's length, as RFC 2104's padding of
the key makes it.
*/
static bool
d_tests_codec_key_padded(
    enum d_net_digest_kind _kind,
    const uint8_t*         _key
)
{
    const size_t size                            = d_net_digest_size(_kind);
    uint8_t      padded[D_NET_DIGEST_BLOCK_SIZE] = { 0 };
    uint8_t      a[D_NET_DIGEST_MAX_SIZE]        = { 0 };
    uint8_t      b[D_NET_DIGEST_MAX_SIZE]        = { 0 };

    memcpy(padded,
           _key,
           20);

    return ( (d_tests_codec_mac_of(_kind,
                                   _key,
                                   20,
                                   a)) &&
             (d_tests_codec_mac_of(_kind,
                                   padded,
                                   sizeof(padded),
                                   b)) &&
             (memcmp(a,
                     b,
                     size) == 0) );
}

/*
d_tests_sa_codec_hmac_keys
  Tests the following:
  - under each function, keys one octet and 37 octets longer than a block
    authenticate as their digests do, and a key of exactly a block does not
  - zeros after a key, up to a block, change nothing
  - the empty key authenticates as a block of zeros does
*/
bool
d_tests_sa_codec_hmac_keys(
    struct d_test_counter* _counter
)
{
    const char* const TEST                              = "HMAC keys";
    uint8_t           key[D_NET_DIGEST_BLOCK_SIZE + 37] = { 0 };
    uint8_t           zeros[D_NET_DIGEST_BLOCK_SIZE]    = { 0 };
    bool              result                            = true;

    // a key with no two octets alike nearby
    for (size_t i = 0; i < sizeof(key); ++i)
    {
        key[i] = (uint8_t)((i * 37u) + 11u);
    }

    // each function
    for (size_t k = 0; k < 3; ++k)
    {
        const enum d_net_digest_kind kind                     =
            D_TESTS_CODEC_KINDS[k];
        uint8_t                      a[D_NET_DIGEST_MAX_SIZE] = { 0 };
        uint8_t                      b[D_NET_DIGEST_MAX_SIZE] = { 0 };

        result = d_tests_net_assert(
                     ( (d_tests_codec_key_digested(kind,
                                                   key,
                                                   sizeof(key)))       &&
                       (d_tests_codec_key_digested(kind,
                                                   key,
                                                   sizeof(zeros) + 1)) &&
                       (!d_tests_codec_key_digested(kind,
                                                    key,
                                                    sizeof(zeros))) ),
                     TEST,
                     "only a key longer than a block is digested",
                     _counter) && result;
        result = d_tests_net_assert(d_tests_codec_key_padded(kind,
                                                             key),
                                    TEST,
                                    "zeros after a key change nothing",
                                    _counter) && result;
        result = d_tests_net_assert( (d_tests_codec_mac_of(kind,
                                                           NULL,
                                                           0,
                                                           a)) &&
                                     (d_tests_codec_mac_of(kind,
                                                           zeros,
                                                           sizeof(zeros),
                                                           b)) &&
                                     (memcmp(a,
                                             b,
                                             d_net_digest_size(kind)) == 0),
                                     TEST,
                                     "the empty key is a block of zeros",
                                     _counter) && result;
    }

    return result;
}

/*
d_tests_sa_codec_hmac_refusals
  Tests the following:
  - d_net_hmac refuses no function, a NULL output, and a key or message
    missing its octets, writing nothing
  - d_net_digest_compute refuses the same, and an empty message is no fault
*/
bool
d_tests_sa_codec_hmac_refusals(
    struct d_test_counter* _counter
)
{
    const char* const         TEST                         = "HMAC refusals";
    const struct d_pack_bytes none                         = { NULL,
                                                               0 };
    const struct d_pack_bytes missing                      = { NULL,
                                                               2 };
    const struct d_pack_bytes text                         = { "text",
                                                               4 };
    uint8_t                   out[D_NET_DIGEST_MAX_SIZE]   = { 0 };
    uint8_t                   zero[D_NET_DIGEST_MAX_SIZE]  = { 0 };
    bool                      result                       = true;

    const bool hmac = ( (!d_net_hmac(D_NET_DIGEST_NONE,
                                     text,
                                     text,
                                     out))  &&
                        (!d_net_hmac(D_NET_DIGEST_MD5,
                                     text,
                                     text,
                                     NULL)) &&
                        (!d_net_hmac(D_NET_DIGEST_MD5,
                                     missing,
                                     text,
                                     out))  &&
                        (!d_net_hmac(D_NET_DIGEST_MD5,
                                     text,
                                     missing,
                                     out))  &&
                        (memcmp(out,
                                zero,
                                sizeof(out)) == 0) );
    const bool compute = ( (!d_net_digest_compute(D_NET_DIGEST_NONE,
                                                  text,
                                                  out))  &&
                           (!d_net_digest_compute(D_NET_DIGEST_SHA1,
                                                  text,
                                                  NULL)) &&
                           (!d_net_digest_compute(D_NET_DIGEST_SHA1,
                                                  missing,
                                                  out))  &&
                           (memcmp(out,
                                   zero,
                                   sizeof(out)) == 0)    &&
                           (d_net_digest_compute(D_NET_DIGEST_SHA1,
                                                 none,
                                                 out)) );

    result = d_tests_net_assert(hmac,
                                TEST,
                                "d_net_hmac refuses, and writes nothing",
                                _counter) && result;
    result = d_tests_net_assert(compute,
                                TEST,
                                "d_net_digest_compute refuses likewise",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_codec_digest_overlap
  Tests the following:
  - d_net_digest_compute may write its digest over its own input
  - d_net_hmac may write its MAC over its key, or over its message
*/
bool
d_tests_sa_codec_digest_overlap(
    struct d_test_counter* _counter
)
{
    const char* const TEST                         = "outputs over inputs";
    const char        TEXT[]                       =
        "an input at least as long as the longest digest";
    uint8_t           buffer[sizeof(TEXT)]         = { 0 };
    uint8_t           apart[D_NET_DIGEST_MAX_SIZE] = { 0 };
    bool              result                       = true;
    const struct d_pack_bytes text                 = { TEXT,
                                                       sizeof(TEXT) - 1 };
    const struct d_pack_bytes in_buffer            = { buffer,
                                                       sizeof(TEXT) - 1 };

    // each function, each overlap
    for (size_t k = 0; k < 3; ++k)
    {
        const enum d_net_digest_kind kind = D_TESTS_CODEC_KINDS[k];
        const size_t                 size = d_net_digest_size(kind);

        memcpy(buffer,
               TEXT,
               sizeof(TEXT));
        result = d_tests_net_assert( (d_net_digest_compute(kind,
                                                           text,
                                                           apart))  &&
                                     (d_net_digest_compute(kind,
                                                           in_buffer,
                                                           buffer)) &&
                                     (memcmp(apart,
                                             buffer,
                                             size) == 0),
                                     TEST,
                                     "a digest over its own input",
                                     _counter) && result;
        memcpy(buffer,
               TEXT,
               sizeof(TEXT));
        result = d_tests_net_assert( (d_net_hmac(kind,
                                                 text,
                                                 text,
                                                 apart))  &&
                                     (d_net_hmac(kind,
                                                 in_buffer,
                                                 text,
                                                 buffer)) &&
                                     (memcmp(apart,
                                             buffer,
                                             size) == 0),
                                     TEST,
                                     "a MAC over its key",
                                     _counter) && result;
        memcpy(buffer,
               TEXT,
               sizeof(TEXT));
        result = d_tests_net_assert( (d_net_hmac(kind,
                                                 text,
                                                 in_buffer,
                                                 buffer)) &&
                                     (memcmp(apart,
                                             buffer,
                                             size) == 0),
                                     TEST,
                                     "a MAC over its message",
                                     _counter) && result;
    }

    return result;
}

/*
d_tests_sa_codec_digest_equal
  Tests the following:
  - equal strings compare equal, and so do two empty ones
  - a difference in the first, a middle, or the last octet is found
  - a NULL string is never equal to anything
*/
bool
d_tests_sa_codec_digest_equal(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "constant-time comparison";
    const uint8_t     a[8]   = { 1, 2, 3, 4, 5, 6, 7, 8 };
    uint8_t           b[8]   = { 1, 2, 3, 4, 5, 6, 7, 8 };
    bool              result = true;

    result = d_tests_net_assert( (d_net_digest_equal(a,
                                                     b,
                                                     sizeof(a))) &&
                                 (d_net_digest_equal(a,
                                                     b,
                                                     0)),
                                 TEST,
                                 "equal strings are equal",
                                 _counter) && result;

    // one octet changed at a time, at each end and between
    for (size_t i = 0; i < sizeof(b); i += 3)
    {
        b[i] ^= 0x40u;
        result = d_tests_net_assert(!d_net_digest_equal(a,
                                                        b,
                                                        sizeof(a)),
                                    TEST,
                                    "one differing octet is found",
                                    _counter) && result;
        b[i] ^= 0x40u;
    }

    result = d_tests_net_assert( (!d_net_digest_equal(NULL,
                                                      b,
                                                      0)) &&
                                 (!d_net_digest_equal(a,
                                                      NULL,
                                                      sizeof(a))),
                                 TEST,
                                 "NULL is equal to nothing",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_wipe
  Tests the following:
  - d_net_wipe zeroes every octet it is given, and nothing past them
  - NULL, and a size of 0, are ignored
*/
bool
d_tests_sa_codec_wipe(
    struct d_test_counter* _counter
)
{
    const char* const TEST       = "wiping";
    uint8_t           buffer[16] = { 0 };
    bool              zeroed     = true;
    bool              result     = true;

    memset(buffer,
           0xaa,
           sizeof(buffer));
    d_net_wipe(buffer,
               sizeof(buffer) - 1);
    d_net_wipe(buffer,
               0);
    d_net_wipe(NULL,
               sizeof(buffer));

    // all but the last octet wiped
    for (size_t i = 0; i + 1 < sizeof(buffer); ++i)
    {
        zeroed = (buffer[i] == 0) && zeroed;
    }

    result = d_tests_net_assert( (zeroed) &&
                                 (buffer[sizeof(buffer) - 1] == 0xaa),
                                 TEST,
                                 "the octets given are zero, the next is not",
                                 _counter) && result;

    return result;
}
