/*******************************************************************************
* djinterp [net]                                         codec_tests_sa_base64.c
*
* Standalone tests of codec_base64.h: RFC 4648's vectors in every variant,
* the refusals of strict, unpadded and canonical decoding, PEM's whitespace,
* IMAP's alphabet one character at a time, lengths, measuring, buffers too
* small, and a round trip of every length.
*
*
* path:      /tests/djinterp/net/codec/codec_tests_sa_base64.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "./codec_tests_sa.h"  // the suite
// std
#include <string.h>  // memcmp, memset, strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_bytes
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, SIZE_MAX


// D_TESTS_CODEC_BASE64_PLAIN
//   constant: RFC 4648 section 10's inputs.
static const char* const D_TESTS_CODEC_BASE64_PLAIN[] =
{
    "", "f", "fo", "foo", "foob", "fooba", "foobar"
};

// D_TESTS_CODEC_BASE64_PADDED
//   constant: RFC 4648 section 10's encodings of them.
static const char* const D_TESTS_CODEC_BASE64_PADDED[] =
{
    "", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy"
};

// D_TESTS_CODEC_BASE64_UNPADDED
//   constant: the same, unpadded.
static const char* const D_TESTS_CODEC_BASE64_UNPADDED[] =
{
    "", "Zg", "Zm8", "Zm9v", "Zm9vYg", "Zm9vYmE", "Zm9vYmFy"
};

// D_TESTS_CODEC_BASE64_COUNT
//   constant: how many vectors the three tables hold.
#define D_TESTS_CODEC_BASE64_COUNT                                             \
    (sizeof(D_TESTS_CODEC_BASE64_PLAIN) /                                      \
     sizeof(D_TESTS_CODEC_BASE64_PLAIN[0]))

/*
d_tests_codec_decodes
  Whether `_text` decodes under `_flags` with `_expected` as the status and,
when that is success, `_plain` as the octets; on a refusal, whether nothing
was written.
*/
static bool
d_tests_codec_decodes(
    const char*            _text,
    unsigned int           _flags,
    enum d_net_codec_error _expected,
    const char*            _plain
)
{
    const struct d_pack_text text       = { _text,
                                            strlen(_text) };
    uint8_t                  out[64]    = { 0 };
    uint8_t                  before[64] = { 0 };
    size_t                   size       = 99;

    memset(out,
           0xa5,
           sizeof(out));
    memcpy(before,
           out,
           sizeof(out));

    const enum d_net_codec_error status = d_net_base64_decode(text,
                                                              _flags,
                                                              out,
                                                              sizeof(out),
                                                              &size);

    // a success gives the octets, a refusal nothing at all
    if (status != _expected)
    {
        return false;
    }

    return (status == D_NET_CODEC_OK)
           ? ( (size == strlen(_plain)) &&
               (memcmp(out,
                       _plain,
                       size) == 0) )
           : ( (size == 0) &&
               (memcmp(out,
                       before,
                       sizeof(out)) == 0) );
}

/*
d_tests_codec_encodes
  Whether `_plain` encodes under `_flags` as exactly `_expected`, the length
reported to match.
*/
static bool
d_tests_codec_encodes(
    const char*  _plain,
    unsigned int _flags,
    const char*  _expected
)
{
    const struct d_pack_bytes plain    = { _plain,
                                           strlen(_plain) };
    char                      text[64] = { 0 };
    size_t                    length   = 0;

    return ( (d_net_base64_encode(plain,
                                  _flags,
                                  text,
                                  sizeof(text),
                                  &length) == D_NET_CODEC_OK) &&
             (strcmp(text,
                     _expected) == 0)                         &&
             (length == strlen(_expected)) );
}

/*
d_tests_sa_codec_base64_known
  Tests the following:
  - RFC 4648 section 10's vectors encode, padded and unpadded
  - they decode padded strictly, and unpadded where that is allowed, which
    also accepts them padded
*/
bool
d_tests_sa_codec_base64_known(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "base64 known answers";
    bool              result = true;

    // each vector, each way
    for (size_t i = 0; i < D_TESTS_CODEC_BASE64_COUNT; ++i)
    {
        const char* const plain    = D_TESTS_CODEC_BASE64_PLAIN[i];
        const char* const padded   = D_TESTS_CODEC_BASE64_PADDED[i];
        const char* const unpadded = D_TESTS_CODEC_BASE64_UNPADDED[i];

        result = d_tests_net_assert(
                     ( (d_tests_codec_encodes(plain,
                                              D_NET_BASE64_STANDARD,
                                              padded)) &&
                       (d_tests_codec_encodes(plain,
                                              D_NET_BASE64_UNPADDED,
                                              unpadded)) ),
                     TEST,
                     "a vector encodes, padded and not",
                     _counter) && result;
        result = d_tests_net_assert(
                     ( (d_tests_codec_decodes(padded,
                                              D_NET_BASE64_STANDARD,
                                              D_NET_CODEC_OK,
                                              plain)) &&
                       (d_tests_codec_decodes(unpadded,
                                              D_NET_BASE64_UNPADDED,
                                              D_NET_CODEC_OK,
                                              plain)) &&
                       (d_tests_codec_decodes(padded,
                                              D_NET_BASE64_UNPADDED,
                                              D_NET_CODEC_OK,
                                              plain)) ),
                     TEST,
                     "a vector decodes, padded and not",
                     _counter) && result;
    }

    return result;
}

/*
d_tests_sa_codec_base64_strict
  Tests the following:
  - strict decoding refuses missing padding, padding before data, too much
    padding, characters outside the alphabet, and whitespace, writing nothing
  - unpadded decoding still refuses a lone final character, and padding that
    does not complete its group
*/
bool
d_tests_sa_codec_base64_strict(
    struct d_test_counter* _counter
)
{
    static const char* const STRICT[] =
    {
        "Zm9", "Zm=v", "Z===", "====", "Zm9v!A==", "Zm 9v", "Zg==Zg==", "Zg"
    };
    static const char* const UNPADDED[] =
    {
        "Z", "Zm9vY", "Zg=", "Zm9==", "Zg===", "Zm9v=", "Z=g"
    };
    const char* const TEST   = "base64 refusals";
    bool              result = true;

    // each strict refusal
    for (size_t i = 0; i < sizeof(STRICT) / sizeof(STRICT[0]); ++i)
    {
        const bool refused =
            d_tests_codec_decodes(STRICT[i],
                                  D_NET_BASE64_STANDARD,
                                  D_NET_CODEC_ERROR_MALFORMED,
                                  "");

        result = d_tests_net_assert(refused,
                                    TEST,
                                    STRICT[i],
                                    _counter) && result;
    }

    // each refusal unpadded decoding keeps
    for (size_t i = 0; i < sizeof(UNPADDED) / sizeof(UNPADDED[0]); ++i)
    {
        const bool refused =
            d_tests_codec_decodes(UNPADDED[i],
                                  D_NET_BASE64_UNPADDED,
                                  D_NET_CODEC_ERROR_MALFORMED,
                                  "");

        result = d_tests_net_assert(refused,
                                    TEST,
                                    UNPADDED[i],
                                    _counter) && result;
    }

    return result;
}

/*
d_tests_sa_codec_base64_canonical
  Tests the following:
  - bits past the last octet are accepted by default, and refused when the
    text must be canonical: four of them after two digits, two after three
  - text the encoder wrote is always canonical
*/
bool
d_tests_sa_codec_base64_canonical(
    struct d_test_counter* _counter
)
{
    const char* const  TEST      = "base64, canonical";
    const unsigned int CANONICAL = D_NET_BASE64_CANONICAL;
    bool               result    = true;

    result = d_tests_net_assert(
                 ( (d_tests_codec_decodes("Zh==",
                                          D_NET_BASE64_STANDARD,
                                          D_NET_CODEC_OK,
                                          "f"))  &&
                   (d_tests_codec_decodes("Zm9=",
                                          D_NET_BASE64_STANDARD,
                                          D_NET_CODEC_OK,
                                          "fo")) &&
                   (d_tests_codec_decodes("Zm+=",
                                          D_NET_BASE64_STANDARD,
                                          D_NET_CODEC_OK,
                                          "fo")) ),
                 TEST,
                 "unused bits are ignored by default",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_codec_decodes("Zh==",
                                          CANONICAL,
                                          D_NET_CODEC_ERROR_MALFORMED,
                                          ""))   &&
                   (d_tests_codec_decodes("Zm9=",
                                          CANONICAL,
                                          D_NET_CODEC_ERROR_MALFORMED,
                                          ""))   &&
                   (d_tests_codec_decodes("Zm+=",
                                          CANONICAL,
                                          D_NET_CODEC_ERROR_MALFORMED,
                                          ""))   &&
                   (d_tests_codec_decodes("Zm8=",
                                          CANONICAL,
                                          D_NET_CODEC_OK,
                                          "fo")) &&
                   (d_tests_codec_decodes("Zg",
                                          CANONICAL | D_NET_BASE64_UNPADDED,
                                          D_NET_CODEC_OK,
                                          "f")) ),
                 TEST,
                 "unused bits must be zero when canonical",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_base64_blanks
  Tests the following:
  - with blanks allowed, space, tab, CR and LF are skipped anywhere, padding
    included, as PEM's lines put them
  - no other whitespace is a blank, and without the flag none is allowed
*/
bool
d_tests_sa_codec_base64_blanks(
    struct d_test_counter* _counter
)
{
    const char* const  TEST   = "base64, blanks";
    const unsigned int BLANKS = D_NET_BASE64_BLANKS;
    bool               result = true;

    result = d_tests_net_assert(
                 ( (d_tests_codec_decodes(" Zm9v\r\n\tYmFy\n",
                                          BLANKS,
                                          D_NET_CODEC_OK,
                                          "foobar")) &&
                   (d_tests_codec_decodes("Zm9vYg=\r\n=",
                                          BLANKS,
                                          D_NET_CODEC_OK,
                                          "foob"))   &&
                   (d_tests_codec_decodes("\r\n",
                                          BLANKS,
                                          D_NET_CODEC_OK,
                                          "")) ),
                 TEST,
                 "blanks are skipped",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_codec_decodes("Zm9v\vYmFy",
                                          BLANKS,
                                          D_NET_CODEC_ERROR_MALFORMED,
                                          "")) &&
                   (d_tests_codec_decodes("Zm9v\nYmFy",
                                          D_NET_BASE64_STANDARD,
                                          D_NET_CODEC_ERROR_MALFORMED,
                                          "")) &&
                   (d_tests_codec_decodes("Zm9v Zg=",
                                          BLANKS,
                                          D_NET_CODEC_ERROR_MALFORMED,
                                          "")) ),
                 TEST,
                 "only blanks, only where allowed, and padding still counts",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_alphabets_agree
  Whether every six-bit value maps to a character and back under `_flags`,
and `_last` is the character for 63.
*/
static bool
d_tests_codec_alphabets_agree(
    unsigned int _flags,
    char         _last
)
{
    bool all = true;

    // every value, there and back
    for (unsigned int sextet = 0; sextet < 64u; ++sextet)
    {
        const char c = d_net_base64_digit(sextet,
                                          _flags);

        all = (d_net_base64_value(c,
                                  _flags) == (int)sextet) && all;
    }

    return ( (all)                                 &&
             (d_net_base64_digit(63u,
                                 _flags) == _last) &&
             (d_net_base64_digit(64u,
                                 _flags) == '\0')  &&
             (d_net_base64_value('=',
                                 _flags) == -1)    &&
             (d_net_base64_value('\0',
                                 _flags) == -1) );
}

/*
d_tests_sa_codec_base64_modified
  Tests the following:
  - both alphabets map every value to a character and back, and differ only
    at 63: '/' or ','
  - octets that encode to 63 do so in each alphabet, and each decoder refuses
    the other's character
*/
bool
d_tests_sa_codec_base64_modified(
    struct d_test_counter* _counter
)
{
    const char* const  TEST     = "base64, IMAP's alphabet";
    const unsigned int MODIFIED = D_NET_BASE64_MODIFIED;
    bool               result   = true;

    result = d_tests_net_assert(
                 ( (d_tests_codec_alphabets_agree(D_NET_BASE64_STANDARD,
                                                  '/')) &&
                   (d_tests_codec_alphabets_agree(MODIFIED,
                                                  ',')) ),
                 TEST,
                 "each alphabet maps every value and back",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_codec_encodes("\xff\xff",
                                          D_NET_BASE64_STANDARD,
                                          "//8="))     &&
                   (d_tests_codec_encodes("\xff\xff",
                                          MODIFIED,
                                          ",,8="))     &&
                   (d_tests_codec_decodes(",,8=",
                                          MODIFIED,
                                          D_NET_CODEC_OK,
                                          "\xff\xff")) &&
                   (d_tests_codec_decodes("//8=",
                                          MODIFIED,
                                          D_NET_CODEC_ERROR_MALFORMED,
                                          ""))         &&
                   (d_tests_codec_decodes(",,8=",
                                          D_NET_BASE64_STANDARD,
                                          D_NET_CODEC_ERROR_MALFORMED,
                                          "")) ),
                 TEST,
                 "63 is '/' in one alphabet and ',' in the other",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_base64_lengths
  Tests the following:
  - encoded lengths, padded and unpadded, for 0 to 6 octets
  - a length whose encoding a size_t cannot hold, with its NUL, is 0
*/
bool
d_tests_sa_codec_base64_lengths(
    struct d_test_counter* _counter
)
{
    static const size_t PADDED[]   = { 0, 4, 4, 4, 8, 8, 8 };
    static const size_t UNPADDED[] = { 0, 2, 3, 4, 6, 7, 8 };
    const char* const   TEST       = "base64 lengths";
    bool                all        = true;
    bool                result     = true;

    // each short length, both ways
    for (size_t size = 0; size < 7; ++size)
    {
        all = ( (d_net_base64_encoded_length(size,
                                             D_NET_BASE64_STANDARD) ==
                 PADDED[size]) &&
                (d_net_base64_encoded_length(size,
                                             D_NET_BASE64_UNPADDED) ==
                 UNPADDED[size]) ) && all;
    }

    result = d_tests_net_assert(all,
                                TEST,
                                "four characters for every three octets",
                                _counter) && result;
    result = d_tests_net_assert(
                 ( (d_net_base64_encoded_length(SIZE_MAX,
                                                D_NET_BASE64_STANDARD) ==
                    0) &&
                   (d_net_base64_encoded_length((SIZE_MAX / 4u) * 3u,
                                                D_NET_BASE64_UNPADDED) ==
                    0) ),
                 TEST,
                 "a length past a size_t is 0",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_too_small
  Whether "foobar" is refused by an encode into 8 characters, its length still
reported and `_out` left empty, and its encoding by a decode into 5 octets,
its size still reported and `_out` otherwise as it was.
*/
static bool
d_tests_codec_too_small(
    char* _out
)
{
    const struct d_pack_bytes plain  = { "foobar",
                                         6 };
    const struct d_pack_text  text   = { "Zm9vYmFy",
                                         8 };
    size_t                    length = 0;
    size_t                    size   = 0;

    return ( (d_net_base64_encode(plain,
                                  0u,
                                  _out,
                                  8,
                                  &length) ==
              D_NET_CODEC_ERROR_BUFFER_TOO_SMALL) &&
             (length == 8)                        &&
             (_out[0] == '\0')                    &&
             (d_net_base64_decode(text,
                                  0u,
                                  _out,
                                  5,
                                  &size) ==
              D_NET_CODEC_ERROR_BUFFER_TOO_SMALL) &&
             (size == 6)                          &&
             (strcmp(_out + 1,
                     "ntouched") == 0) );
}

/*
d_tests_codec_refusals
  Whether an unknown flag, a NULL buffer with room, a NULL size, and text
missing its characters are each refused.
*/
static bool
d_tests_codec_refusals(
    void
)
{
    const struct d_pack_bytes plain   = { "foobar",
                                          6 };
    const struct d_pack_text  text    = { "Zm9vYmFy",
                                          8 };
    const struct d_pack_text  missing = { NULL,
                                          4 };
    char                      out[16] = { 0 };
    size_t                    size    = 0;

    return ( (d_net_base64_encode(plain,
                                  0x80u,
                                  out,
                                  sizeof(out),
                                  NULL) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT) &&
             (d_net_base64_decode(text,
                                  0u,
                                  NULL,
                                  sizeof(out),
                                  &size) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT) &&
             (d_net_base64_decode(text,
                                  0u,
                                  out,
                                  sizeof(out),
                                  NULL) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT) &&
             (d_net_base64_decode(missing,
                                  0u,
                                  out,
                                  sizeof(out),
                                  &size) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT) );
}

/*
d_tests_sa_codec_base64_buffers
  Tests the following:
  - a call with no buffer measures, encoding and decoding
  - a buffer too small is refused, the length still reported, an encode's
    buffer left empty and a decode's untouched
  - missing arguments and unknown flags are refused
*/
bool
d_tests_sa_codec_base64_buffers(
    struct d_test_counter* _counter
)
{
    const char* const         TEST    = "base64 buffers";
    const struct d_pack_bytes plain   = { "foobar",
                                          6 };
    const struct d_pack_text  text    = { "Zm9vYmFy",
                                          8 };
    char                      out[16] = "untouched";
    size_t                    length  = 0;
    size_t                    size    = 0;
    bool                      result  = true;

    result = d_tests_net_assert(
                 ( (d_net_base64_encode(plain,
                                        0u,
                                        NULL,
                                        0,
                                        &length) == D_NET_CODEC_OK) &&
                   (length == 8)                                    &&
                   (d_net_base64_decode(text,
                                        0u,
                                        NULL,
                                        0,
                                        &size) == D_NET_CODEC_OK)   &&
                   (size == 6) ),
                 TEST,
                 "no buffer measures",
                 _counter) && result;
    result = d_tests_net_assert(d_tests_codec_too_small(out),
                                TEST,
                                "too small: the length reported, nothing "
                                "decoded",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_codec_refusals(),
                                TEST,
                                "missing arguments and unknown flags are "
                                "refused",
                                _counter) && result;

    return result;
}

/*
d_tests_codec_round_trips
  Whether `_size` octets of `_data` encode under `_flags` and decode back,
canonically, to themselves.
*/
static bool
d_tests_codec_round_trips(
    const uint8_t* _data,
    size_t         _size,
    unsigned int   _flags
)
{
    const struct d_pack_bytes data      = { _data,
                                            _size };
    char                      text[512] = { 0 };
    uint8_t                   back[384] = { 0 };
    size_t                    length    = 0;
    size_t                    size      = 0;

    // the encoding, then the decoding of it
    if (d_net_base64_encode(data,
                            _flags,
                            text,
                            sizeof(text),
                            &length) != D_NET_CODEC_OK)
    {
        return false;
    }

    const struct d_pack_text encoded = { text,
                                         length };

    return ( (d_net_base64_decode(encoded,
                                  _flags | D_NET_BASE64_CANONICAL,
                                  back,
                                  sizeof(back),
                                  &size) == D_NET_CODEC_OK) &&
             (size == _size)                                &&
             (memcmp(back,
                     _data,
                     _size) == 0) );
}

/*
d_tests_sa_codec_base64_round_trip
  Tests the following:
  - every length from 0 to 300 octets, of every octet value, encodes and
    decodes back canonically: padded, unpadded, and in IMAP's alphabet
*/
bool
d_tests_sa_codec_base64_round_trip(
    struct d_test_counter* _counter
)
{
    static const unsigned int FLAGS[] =
    {
        D_NET_BASE64_STANDARD,
        D_NET_BASE64_UNPADDED,
        D_NET_BASE64_MODIFIED | D_NET_BASE64_UNPADDED
    };
    const char* const TEST      = "base64 round trips";
    uint8_t           data[300] = { 0 };
    bool              result    = true;

    // every octet value appears, in no simple order
    for (size_t i = 0; i < sizeof(data); ++i)
    {
        data[i] = (uint8_t)((i * 167u) + 13u);
    }

    // each variant, every length
    for (size_t f = 0; f < sizeof(FLAGS) / sizeof(FLAGS[0]); ++f)
    {
        bool all = true;

        for (size_t size = 0; size <= sizeof(data); ++size)
        {
            all = d_tests_codec_round_trips(data,
                                            size,
                                            FLAGS[f]) && all;
        }

        result = d_tests_net_assert(all,
                                    TEST,
                                    "every length comes back as it went",
                                    _counter) && result;
    }

    return result;
}
