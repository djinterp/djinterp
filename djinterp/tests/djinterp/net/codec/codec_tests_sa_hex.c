/*******************************************************************************
* djinterp [net]                                            codec_tests_sa_hex.c
*
* Standalone tests of codec_hex.h: every character's value and every value's
* digit in both cases, encoding and decoding of every octet, and the
* refusals, measures and buffers too small.
*
*
* path:      /tests/djinterp/net/codec/codec_tests_sa_hex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "./codec_tests_sa.h"  // the suite
// std
#include <string.h>  // memchr, memcmp, memset, strcmp
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_bytes
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t


/*
d_tests_codec_value_holds
  Whether the value d_net_hex_value gives the character with code `_code`, 0
to 255, is its place in a listing of the digits of either case, or -1, and
d_net_hex_is_digit agrees.
*/
static bool
d_tests_codec_value_holds(
    int _code
)
{
    static const char LOWER[] = "0123456789abcdef";
    static const char UPPER[] = "0123456789ABCDEF";
    const char*       lower   = (_code != 0) ? memchr(LOWER,
                                                      _code,
                                                      16)
                                             : NULL;
    const char*       upper   = (_code != 0) ? memchr(UPPER,
                                                      _code,
                                                      16)
                                             : NULL;
    const int         value   = (lower != NULL) ? (int)(lower - LOWER)
                              : (upper != NULL) ? (int)(upper - UPPER)
                                                : -1;

    return ( (d_net_hex_value((char)(unsigned char)_code) == value) &&
             (d_net_hex_is_digit((char)(unsigned char)_code) == (value >= 0)) );
}

/*
d_tests_sa_codec_hex_digits
  Tests the following:
  - every octet's value as a digit, of either case, or -1, and whether it is
    one
  - every value's digit in each case, and none past 15
*/
bool
d_tests_sa_codec_hex_digits(
    struct d_test_counter* _counter
)
{
    static const char LOWER[] = "0123456789abcdef";
    static const char UPPER[] = "0123456789ABCDEF";
    const char* const TEST    = "hex digits";
    bool              values  = true;
    bool              digits  = true;
    bool              result  = true;

    // every octet's value
    for (int code = 0; code < 256; ++code)
    {
        values = d_tests_codec_value_holds(code) && values;
    }

    // every value's digit, in each case
    for (unsigned int value = 0; value < 16u; ++value)
    {
        digits = ( (d_net_hex_digit(value,
                                    D_NET_HEX_LOWER) == LOWER[value]) &&
                   (d_net_hex_digit(value,
                                    D_NET_HEX_UPPER) == UPPER[value]) ) &&
                 digits;
    }

    result = d_tests_net_assert(values,
                                TEST,
                                "every octet's value",
                                _counter) && result;
    result = d_tests_net_assert( (digits) &&
                                 (d_net_hex_digit(16u,
                                                  D_NET_HEX_UPPER) == '\0'),
                                 TEST,
                                 "every value's digit, and none past 15",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_hex_strings
  Tests the following:
  - every octet encodes, in each case, and decodes back from either
  - an odd count, or a character that is no digit, is refused, writing
    nothing
*/
bool
d_tests_sa_codec_hex_strings(
    struct d_test_counter* _counter
)
{
    const char* const         TEST      = "hex strings";
    uint8_t                   all[256]  = { 0 };
    char                      text[513] = { 0 };
    uint8_t                   back[256] = { 0 };
    size_t                    length    = 0;
    size_t                    size      = 0;
    const struct d_pack_bytes octets    = { all,
                                            sizeof(all) };
    const struct d_pack_text  odd       = { "abc",
                                            3 };
    const struct d_pack_text  stray     = { "0g",
                                            2 };
    bool                      result    = true;

    // every octet, in order
    for (size_t i = 0; i < sizeof(all); ++i)
    {
        all[i] = (uint8_t)i;
    }

    const bool upper = ( (d_net_hex_encode(octets,
                                           D_NET_HEX_UPPER,
                                           text,
                                           sizeof(text),
                                           &length) == D_NET_CODEC_OK) &&
                         (length == 512)                               &&
                         (memcmp(text + 0x5a * 2,
                                 "5A5B",
                                 4) == 0) );
    const struct d_pack_text encoded = { text,
                                         length };
    const bool back_upper = ( (d_net_hex_decode(encoded,
                                                back,
                                                sizeof(back),
                                                &size) == D_NET_CODEC_OK) &&
                              (size == 256)                               &&
                              (memcmp(back,
                                      all,
                                      256) == 0) );

    result = d_tests_net_assert( (upper) &&
                                 (back_upper),
                                 TEST,
                                 "every octet, upper case, there and back",
                                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_net_hex_encode(octets,
                                     D_NET_HEX_LOWER,
                                     text,
                                     sizeof(text),
                                     NULL) == D_NET_CODEC_OK)               &&
                   (memcmp(text + 0xfe * 2,
                           "feff",
                           5) == 0)                                         &&
                   (d_net_hex_decode(odd,
                                     back,
                                     sizeof(back),
                                     &size) == D_NET_CODEC_ERROR_MALFORMED) &&
                   (size == 0)                                              &&
                   (d_net_hex_decode(stray,
                                     back,
                                     sizeof(back),
                                     &size) == D_NET_CODEC_ERROR_MALFORMED) &&
                   (back[0] == 0) ),
                 TEST,
                 "lower case; odd counts and stray characters refused",
                 _counter) && result;

    return result;
}

/*
d_tests_sa_codec_hex_buffers
  Tests the following:
  - a call with no buffer measures, encoding and decoding
  - a buffer too small is refused, the length still reported, an encode's
    buffer left empty and a decode's untouched
  - missing arguments and unknown flags are refused
*/
bool
d_tests_sa_codec_hex_buffers(
    struct d_test_counter* _counter
)
{
    const char* const         TEST     = "hex buffers";
    const struct d_pack_bytes octets   = { "\x01\xab",
                                           2 };
    const struct d_pack_text  digits   = { "01ab",
                                           4 };
    const struct d_pack_text  missing  = { NULL,
                                           2 };
    char                      out[8]   = "kept";
    size_t                    length   = 0;
    size_t                    size     = 0;
    bool                      result   = true;

    result = d_tests_net_assert(
                 ( (d_net_hex_encode(octets,
                                     0u,
                                     NULL,
                                     0,
                                     &length) == D_NET_CODEC_OK) &&
                   (length == 4)                                 &&
                   (d_net_hex_decode(digits,
                                     NULL,
                                     0,
                                     &size) == D_NET_CODEC_OK)   &&
                   (size == 2)                                   &&
                   (d_net_hex_encode(octets,
                                     0u,
                                     out,
                                     4,
                                     &length) ==
                    D_NET_CODEC_ERROR_BUFFER_TOO_SMALL)          &&
                   (out[0] == '\0')                              &&
                   (d_net_hex_decode(digits,
                                     out + 1,
                                     1,
                                     &size) ==
                    D_NET_CODEC_ERROR_BUFFER_TOO_SMALL)          &&
                   (size == 2)                                   &&
                   (strcmp(out + 1,
                           "ept") == 0) ),
                 TEST,
                 "measures, and buffers too small",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_net_hex_encode(octets,
                                     0x02u,
                                     out,
                                     sizeof(out),
                                     NULL) ==
                    D_NET_CODEC_ERROR_INVALID_ARGUMENT) &&
                   (d_net_hex_decode(digits,
                                     out,
                                     sizeof(out),
                                     NULL) ==
                    D_NET_CODEC_ERROR_INVALID_ARGUMENT) &&
                   (d_net_hex_decode(missing,
                                     out,
                                     sizeof(out),
                                     &size) ==
                    D_NET_CODEC_ERROR_INVALID_ARGUMENT) ),
                 TEST,
                 "missing arguments and unknown flags are refused",
                 _counter) && result;

    return result;
}

/*
d_tests_codec_hex_limits
  Whether an encoding as long as a size_t allows, its NUL aside, is measured,
and one octet more refused. The octets are never read: a measure stops
before them.
*/
static bool
d_tests_codec_hex_limits(void)
{
    static const uint8_t      one     = 0x5au;
    const size_t              most    = (SIZE_MAX - 1u) / 2u;
    const struct d_pack_bytes longest = { &one,
                                          most };
    const struct d_pack_bytes beyond  = { &one,
                                          most + 1u };
    size_t                    length  = 0;

    return ( (d_net_hex_encode(longest,
                               0u,
                               NULL,
                               0,
                               &length) == D_NET_CODEC_OK) &&
             (length == SIZE_MAX - 1u)                     &&
             (d_net_hex_encode(beyond,
                               0u,
                               NULL,
                               0,
                               &length) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT) );
}

/*
d_tests_codec_hex_edges_encode
  Whether no octets encode to an empty string; octets missing, or a buffer
missing that is said to have room, are refused; and a buffer of one byte, too
small, is left an empty string, while one with room to spare keeps every byte
past the NUL.
*/
static bool
d_tests_codec_hex_edges_encode(void)
{
    const struct d_pack_bytes none    = { NULL,
                                          0 };
    const struct d_pack_bytes missing = { NULL,
                                          1 };
    const struct d_pack_bytes octets  = { "\x01\xab",
                                          2 };
    char                      out[8]  = "XXXXXXX";
    char                      tiny[1] = { 'X' };
    size_t                    length  = 7;

    return ( (d_net_hex_encode(none,
                               0u,
                               out,
                               sizeof(out),
                               &length) == D_NET_CODEC_OK) &&
             (length == 0)                                 &&
             (out[0] == '\0')                              &&
             (d_net_hex_encode(missing,
                               0u,
                               out,
                               sizeof(out),
                               &length) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT)          &&
             (d_net_hex_encode(octets,
                               0u,
                               NULL,
                               1,
                               &length) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT)          &&
             (d_net_hex_encode(octets,
                               0u,
                               tiny,
                               sizeof(tiny),
                               &length) ==
              D_NET_CODEC_ERROR_BUFFER_TOO_SMALL)          &&
             (tiny[0] == '\0')                             &&
             (d_net_hex_encode(octets,
                               0u,
                               out,
                               sizeof(out),
                               &length) == D_NET_CODEC_OK) &&
             (memcmp(out,
                     "01ab\0XX",
                     7) == 0) );
}

/*
d_tests_codec_hex_edges_decode
  Whether no text decodes to no octets; text missing, or a buffer missing that
is said to have room, is refused; a stray first character is caught; and the
first octet is written, and nothing past the last.
*/
static bool
d_tests_codec_hex_edges_decode(void)
{
    const struct d_pack_text none     = { NULL,
                                          0 };
    const struct d_pack_text missing  = { NULL,
                                          1 };
    const struct d_pack_text first    = { "zf",
                                          2 };
    const struct d_pack_text digits   = { "ff00",
                                          4 };
    uint8_t                  back[3]  = { 0x11u,
                                          0x22u,
                                          0x33u };
    size_t                   size     = 9;

    return ( (d_net_hex_decode(none,
                               back,
                               sizeof(back),
                               &size) == D_NET_CODEC_OK)              &&
             (size == 0)                                              &&
             (d_net_hex_decode(missing,
                               back,
                               sizeof(back),
                               &size) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT)                     &&
             (d_net_hex_decode(digits,
                               NULL,
                               1,
                               &size) ==
              D_NET_CODEC_ERROR_INVALID_ARGUMENT)                     &&
             (d_net_hex_decode(first,
                               back,
                               sizeof(back),
                               &size) == D_NET_CODEC_ERROR_MALFORMED) &&
             (d_net_hex_decode(digits,
                               back,
                               sizeof(back),
                               &size) == D_NET_CODEC_OK)              &&
             (size == 2)                                              &&
             (back[0] == 0xffu)                                       &&
             (back[1] == 0x00u)                                       &&
             (back[2] == 0x33u) );
}

/*
d_tests_sa_codec_hex_edges
  Tests the following:
  - no octets, and no text, are no error: an empty string, no octets
  - missing octets, text, or buffers said to have room are refused
  - an encoding as long as a size_t allows is measured, one octet more refused
  - a buffer of one byte is left an empty string; bytes past the NUL, or past
    the last octet, are never written; the first octet always is
*/
bool
d_tests_sa_codec_hex_edges(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "hex edges";
    bool              result = true;

    result = d_tests_net_assert(d_tests_codec_hex_edges_encode(),
                                TEST,
                                "encoding at its edges",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_codec_hex_edges_decode(),
                                TEST,
                                "decoding at its edges",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_codec_hex_limits(),
                                TEST,
                                "the longest encoding a size_t allows",
                                _counter) && result;

    return result;
}
