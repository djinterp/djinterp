/*******************************************************************************
* djinterp [net]                                                  codec_base64.c
*
* Definitions for codec_base64.h: base64 in the variants net's protocols use.
*   Decoding takes two passes. The first reads the whole text -- digits,
* padding, whitespace where the variant allows it -- and decides whether the
* text is acceptable and how many octets it holds, so that nothing is written
* for a refused text or a buffer too small; the second decodes. Encoding knows
* its length from the input's alone, and checks the buffer before writing.
*
*
* path:      /src/djinterp/net/codec/codec_base64.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/codec/codec_base64.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"             // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"     // d_pack_bytes
#include "../../../../inc/djinterp/net/net_common.h"   // d_net_ascii_is_space
#include "../../../../inc/djinterp/net/codec/codec_common.h"  // error codes
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, uint32_t, SIZE_MAX


// D_INTERNAL_NET_BASE64_FLAGS
//   constant: every flag codec_base64.h defines; any other bit is refused.
#define D_INTERNAL_NET_BASE64_FLAGS                                            \
    ((unsigned int)(D_NET_BASE64_UNPADDED  |                                   \
                    D_NET_BASE64_BLANKS    |                                   \
                    D_NET_BASE64_CANONICAL |                                   \
                    D_NET_BASE64_MODIFIED))

// d_internal_net_base64_alphabet
//   the standard alphabet (RFC 4648 section 4); the modified one differs only
// in its last character.
D_STATIC const char d_internal_net_base64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// d_internal_net_base64_scan
//   what the first pass of a decode learns of a text: how many digits and how
// much padding it holds, and the value of its last digit.
struct d_internal_net_base64_scan
{
    size_t digits;   // characters of the alphabet
    size_t padding;  // '=' characters, all after the digits
    int    last;     // the last digit's value, or -1 when there is none
};

/*
d_net_base64_digit
  The two alphabets differ only at 63, so one table serves both.
*/
char
d_net_base64_digit(
    unsigned int _sextet,
    unsigned int _flags
)
{
    // six bits give 64 values, and no more
    if (_sextet > 63u)
    {
        return '\0';
    }

    // the modified alphabet's one difference
    if ( (_sextet == 63u) &&
         ((_flags & D_NET_BASE64_MODIFIED) != 0u) )
    {
        return ',';
    }

    return d_internal_net_base64_alphabet[_sextet];
}

/*
d_net_base64_value
  Ranges rather than a search of the alphabet, so the cost does not grow with
a character's place in it, and NUL needs no special case.
*/
int
d_net_base64_value(
    char         _c,
    unsigned int _flags
)
{
    const char last = ((_flags & D_NET_BASE64_MODIFIED) != 0u) ? ','
                                                                : '/';

    // the letters, then the digits, then the two that differ by alphabet
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return _c - 'A';
    }

    if ( (_c >= 'a') &&
         (_c <= 'z') )
    {
        return _c - 'a' + 26;
    }

    if ( (_c >= '0') &&
         (_c <= '9') )
    {
        return _c - '0' + 52;
    }

    if (_c == '+')
    {
        return 62;
    }

    return (_c == last) ? 63 : -1;
}

/*
d_net_base64_encoded_length
  Every three octets take four characters; one or two left over take four
with padding, or one more than their count without. The bound leaves room for
the NUL the encoder writes.
*/
size_t
d_net_base64_encoded_length(
    size_t       _size,
    unsigned int _flags
)
{
    const size_t groups    = _size / 3u;
    const size_t remainder = _size % 3u;

    // the encoding and its NUL must be representable
    if (groups > ((SIZE_MAX - 5u) / 4u))
    {
        return 0;
    }

    // a partial group, padded or not
    if (remainder == 0u)
    {
        return groups * 4u;
    }

    return (groups * 4u) +
           (((_flags & D_NET_BASE64_UNPADDED) != 0u) ? (remainder + 1u)
                                                     : 4u);
}

/*
d_internal_net_base64_group
  Encodes one to three octets as one character more than there are octets,
the missing low bits encoded as zeros, and returns the characters written.
*/
D_STATIC size_t
d_internal_net_base64_group(
    const uint8_t* _bytes,
    size_t         _count,
    unsigned int   _flags,
    char*          _text
)
{
    const uint32_t second = (_count > 1u) ? ((uint32_t)_bytes[1] << 8) : 0u;
    const uint32_t third  = (_count > 2u) ? (uint32_t)_bytes[2] : 0u;
    const uint32_t group  = ((uint32_t)_bytes[0] << 16) | second | third;

    // six bits per character, from the top
    for (size_t i = 0; i <= _count; i++)
    {
        const unsigned int shift = (unsigned int)(18u - (6u * i));

        _text[i] = d_net_base64_digit((group >> shift) & 0x3fu,
                                      _flags);
    }

    return _count + 1u;
}

/*
d_net_base64_encode
  The length is known before anything is written, so a buffer too small is
refused untouched but for the NUL that leaves it an empty string.
*/
enum d_net_codec_error
d_net_base64_encode(
    struct d_pack_bytes _data,
    unsigned int        _flags,
    char*               _text,
    size_t              _capacity,
    size_t*             _length
)
{
    // parameter validation
    if ( ( (_data.data == NULL) &&
           (_data.size > 0) )                            ||
         ( (_text == NULL) &&
           (_capacity > 0) )                             ||
         ((_flags & ~D_INTERNAL_NET_BASE64_FLAGS) != 0u) )
    {
        return D_NET_CODEC_ERROR_INVALID_ARGUMENT;
    }

    const size_t needed = d_net_base64_encoded_length(_data.size,
                                                      _flags);

    // the length is reported whatever follows
    if (_length != NULL)
    {
        *_length = needed;
    }

    // an encoding whose length a size_t cannot hold
    if ( (needed == 0) &&
         (_data.size > 0) )
    {
        return D_NET_CODEC_ERROR_INVALID_ARGUMENT;
    }

    // a measure stops here; a short buffer is left an empty string
    if (_capacity <= needed)
    {
        if (_capacity > 0)
        {
            _text[0] = '\0';
        }

        return (_text == NULL) ? D_NET_CODEC_OK
                               : D_NET_CODEC_ERROR_BUFFER_TOO_SMALL;
    }

    const uint8_t* bytes = (const uint8_t*)_data.data;
    size_t         out   = 0;

    // each three octets become four characters, a shorter tail fewer
    for (size_t in = 0; in < _data.size; in += 3u)
    {
        const size_t count = ((_data.size - in) < 3u) ? (_data.size - in)
                                                      : 3u;

        out += d_internal_net_base64_group(bytes + in,
                                           count,
                                           _flags,
                                           _text + out);
    }

    // padding completes the final group of four
    while ( ((_flags & D_NET_BASE64_UNPADDED) == 0u) &&
            ((out % 4u) != 0u) )
    {
        _text[out++] = '=';
    }

    _text[out] = '\0';

    return D_NET_CODEC_OK;
}

/*
d_internal_net_base64_scan
  The first pass: counts the digits and the padding, skipping whitespace
where the variant allows it, and refuses a character outside the alphabet or a
digit after padding has begun.
*/
D_STATIC bool
d_internal_net_base64_scan(
    struct d_pack_text                 _text,
    unsigned int                       _flags,
    struct d_internal_net_base64_scan* _scan
)
{
    const bool blanks = ((_flags & D_NET_BASE64_BLANKS) != 0u);

    _scan->digits  = 0;
    _scan->padding = 0;
    _scan->last    = -1;

    // every character is whitespace, padding, or a digit before any padding
    for (size_t i = 0; i < _text.length; i++)
    {
        const char c = _text.data[i];

        if ( (blanks) &&
             (d_net_ascii_is_space(c)) )
        {
            continue;
        }

        if (c == '=')
        {
            _scan->padding++;

            continue;
        }

        const int value = d_net_base64_value(c,
                                             _flags);

        if ( (value < 0) ||
             (_scan->padding > 0) )
        {
            return false;
        }

        _scan->digits++;
        _scan->last = value;
    }

    return true;
}

/*
d_internal_net_base64_acceptable
  Whether what the scan found makes a text of the variant. A last group of one
digit holds no whole octet; padding, when there is any, must complete the last
group exactly; and a canonical text leaves the bits past its last octet zero:
the low four bits of the last digit of a two-digit group, the low two of a
three-digit one.
*/
D_STATIC bool
d_internal_net_base64_acceptable(
    const struct d_internal_net_base64_scan* _scan,
    unsigned int                             _flags
)
{
    const size_t remainder = _scan->digits % 4u;
    const int    unused    = (remainder == 2u) ? 0x0f
                           : (remainder == 3u) ? 0x03
                                               : 0x00;

    // no whole octet in a group of one, and at most two padding characters
    if ( (remainder == 1u) ||
         (_scan->padding > 2u) )
    {
        return false;
    }

    // padding completes the last group, or is absent where that is allowed
    if ( ( (_scan->padding > 0u) &&
           (remainder + _scan->padding != 4u) ) ||
         ( (_scan->padding == 0u) &&
           (remainder != 0u) &&
           ((_flags & D_NET_BASE64_UNPADDED) == 0u) ) )
    {
        return false;
    }

    return ( ((_flags & D_NET_BASE64_CANONICAL) == 0u) ||
             ((_scan->last & unused) == 0) );
}

/*
d_internal_net_base64_bits
  The second pass: six bits in per digit, an octet out whenever eight have
gathered, until `_size` octets are out. Whitespace and padding are skipped as
characters outside the alphabet; the scan has refused every other.
*/
D_STATIC void
d_internal_net_base64_bits(
    struct d_pack_text _text,
    unsigned int       _flags,
    uint8_t*           _out,
    size_t             _size
)
{
    uint32_t     accumulator = 0;
    unsigned int bits        = 0;
    size_t       written     = 0;

    // until every octet the scan counted is out
    for (size_t i = 0; written < _size; i++)
    {
        const int value = d_net_base64_value(_text.data[i],
                                             _flags);

        if (value < 0)
        {
            continue;
        }

        accumulator = (accumulator << 6) | (uint32_t)value;
        bits       += 6u;

        // a whole octet, from the top of what has gathered
        if (bits >= 8u)
        {
            bits             -= 8u;
            _out[written++]   = (uint8_t)(accumulator >> bits);
            accumulator      &= (1u << bits) - 1u;
        }
    }

    return;
}

/*
d_net_base64_decode
  The scan decides everything before the decode writes anything: whether the
text is acceptable, how many octets it holds, and so whether they fit.
*/
enum d_net_codec_error
d_net_base64_decode(
    struct d_pack_text _text,
    unsigned int       _flags,
    void*              _data,
    size_t             _capacity,
    size_t*            _size
)
{
    // parameter validation
    if ( (_size == NULL)                                 ||
         ( (_text.data == NULL) &&
           (_text.length > 0) )                          ||
         ( (_data == NULL) &&
           (_capacity > 0) )                             ||
         ((_flags & ~D_INTERNAL_NET_BASE64_FLAGS) != 0u) )
    {
        return D_NET_CODEC_ERROR_INVALID_ARGUMENT;
    }

    struct d_internal_net_base64_scan scan = { 0, 0, -1 };

    *_size = 0;

    // the whole text is judged first
    if ( (!d_internal_net_base64_scan(_text,
                                      _flags,
                                      &scan)) ||
         (!d_internal_net_base64_acceptable(&scan,
                                            _flags)) )
    {
        return D_NET_CODEC_ERROR_MALFORMED;
    }

    const size_t remainder = scan.digits % 4u;

    *_size = ((scan.digits / 4u) * 3u) +
             ((remainder > 0u) ? (remainder - 1u) : 0u);

    // a measure stops here, and so does a buffer too small
    if (_capacity < *_size)
    {
        return (_data == NULL) ? D_NET_CODEC_OK
                               : D_NET_CODEC_ERROR_BUFFER_TOO_SMALL;
    }

    d_internal_net_base64_bits(_text,
                               _flags,
                               (uint8_t*)_data,
                               *_size);

    return D_NET_CODEC_OK;
}
