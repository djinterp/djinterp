/*******************************************************************************
* djinterp [net]                                                    ssh_base64.c
*
*   Definitions for ssh_base64.h.
*
*
* path:      /src/djinterp/net/ssh/ssh_base64.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ssh/ssh_base64.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // SIZE_MAX, uint32_t
#include <string.h>   // strchr
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ssh/ssh_common.h"  // d_ssh_status


// d_ssh_internal_base64
//   the standard Base64 alphabet (RFC 4648 section 4).
D_STATIC const char d_ssh_internal_base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
d_ssh_internal_base64_value
  File-local: a character's six-bit value, or -1 if it is outside the
alphabet (the terminator included).
*/
D_STATIC int
d_ssh_internal_base64_value(
    char _c
)
{
    const char* const found = (_c != '\0') ? strchr(d_ssh_internal_base64, _c)
                                           : NULL;

    return (found) ? (int)(found - d_ssh_internal_base64) : -1;
}

/*
d_ssh_base64_encoded_size
  Four characters per three bytes; a final partial group takes four with
padding, or one more than its byte count without.
*/
size_t
d_ssh_base64_encoded_size(
    size_t _size,
    bool   _padding
)
{
    const size_t groups    = _size / 3;
    const size_t remainder = _size % 3;

    // the encoding and a terminator must be representable
    if (groups > ((SIZE_MAX - 5) / 4))
    {
        return 0;
    }

    if (remainder == 0)
    {
        return groups * 4;
    }

    return (groups * 4) + ((_padding) ? 4 : (remainder + 1));
}

/*
d_ssh_internal_base64_group
  File-local: encodes one to three bytes as one more character than there
are bytes, the missing low bits encoded as zeros; returns the characters
written.
*/
D_STATIC size_t
d_ssh_internal_base64_group(
    const unsigned char* _bytes,
    size_t               _count,
    char*                _text
)
{
    const uint32_t second = (_count > 1) ? ((uint32_t)_bytes[1] << 8) : 0u;
    const uint32_t third  = (_count > 2) ? (uint32_t)_bytes[2] : 0u;
    const uint32_t group  = ((uint32_t)_bytes[0] << 16) | second | third;

    // six bits per character, from the top
    for (size_t i = 0; i <= _count; i++)
    {
        _text[i] = d_ssh_internal_base64[(group >> (18 - (6 * i))) & 0x3Fu];
    }

    return _count + 1;
}

/*
d_ssh_base64_encode
  Three bytes at a time; the one or two left over encode their missing bits
as zeros, and padding, when asked for, completes the final group.
*/
enum d_ssh_status
d_ssh_base64_encode(
    const void* _data,
    size_t      _size,
    bool        _padding,
    char*       _text,
    size_t      _capacity
)
{
    if ( (!_text) ||
         ( (!_data) &&
           (_size > 0) ) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    const size_t needed = d_ssh_base64_encoded_size(_size, _padding);

    // the encoding and its terminator must both fit
    if ( ( (needed == 0) &&
           (_size > 0) ) ||
         (_capacity <= needed) )
    {
        if (_capacity > 0)
        {
            _text[0] = '\0';
        }

        return D_SSH_ERR_ARGUMENT;
    }

    const unsigned char* const bytes = (const unsigned char*)_data;
    size_t                     out   = 0;

    // each three bytes become four characters, a shorter tail fewer
    for (size_t in = 0; in < _size; in += 3)
    {
        const size_t count = ((_size - in) < 3) ? (_size - in) : 3;

        out += d_ssh_internal_base64_group(bytes + in, count, _text + out);
    }

    // padding completes the final group of four
    while ( (_padding) &&
            ((out % 4) != 0) )
    {
        _text[out++] = '=';
    }

    _text[out] = '\0';

    return D_SSH_OK;
}

/*
d_ssh_internal_base64_payload
  File-local: strips at most two padding characters and returns the length
left, or SIZE_MAX if a lone final character would hold no whole byte, or the
padding does not complete exactly the group it pads.
*/
D_STATIC size_t
d_ssh_internal_base64_payload(
    const char* _text,
    size_t      _length
)
{
    size_t padding = 0;

    // at most two trailing padding characters
    while ( (padding < 2)       &&
            (padding < _length) &&
            (_text[_length - 1 - padding] == '=') )
    {
        padding++;
    }

    const size_t length    = _length - padding;
    const size_t remainder = length % 4;

    if ( (remainder == 1) ||
         ( (padding > 0) &&
           ( ((_length % 4) != 0) ||
             (padding != (4 - remainder)) ) ) )
    {
        return SIZE_MAX;
    }

    return length;
}

/*
d_ssh_internal_base64_bits
  File-local: decodes alphabet characters six bits at a time, writing a byte
whenever eight have gathered. Fails on a character outside the alphabet, or
on nonzero bits left past the last whole byte, which would give one byte
string a second encoding.
*/
D_STATIC bool
d_ssh_internal_base64_bits(
    const char*    _text,
    size_t         _length,
    unsigned char* _out,
    size_t*        _written
)
{
    uint32_t     accumulator = 0;
    unsigned int bits        = 0;
    size_t       written     = 0;

    // six bits in per character; a byte out whenever eight have gathered
    for (size_t i = 0; i < _length; i++)
    {
        const int value = d_ssh_internal_base64_value(_text[i]);

        if (value < 0)
        {
            return false;
        }

        accumulator = (accumulator << 6) | (uint32_t)value;
        bits       += 6;

        if (bits >= 8)
        {
            bits           -= 8;
            _out[written++] = (unsigned char)((accumulator >> bits) & 0xFFu);
        }
    }

    *_written = written;

    return ((accumulator & ((1u << bits) - 1u)) == 0);
}

/*
d_ssh_base64_decode
  The padding is checked before anything is decoded, and the output size is
known from the length alone, so a short buffer is refused before any byte is
written.
*/
enum d_ssh_status
d_ssh_base64_decode(
    const char* _text,
    size_t      _length,
    void*       _data,
    size_t      _capacity,
    size_t*     _size
)
{
    if (_size)
    {
        *_size = 0;
    }

    if ( (!_size) ||
         (!_data) ||
         ( (!_text) &&
           (_length > 0) ) )
    {
        return D_SSH_ERR_ARGUMENT;
    }

    const size_t length = d_ssh_internal_base64_payload(_text, _length);

    if (length == SIZE_MAX)
    {
        return D_SSH_ERR_FORMAT;
    }

    const size_t remainder = length % 4;
    const size_t decoded   = ((length / 4) * 3) +
                             ((remainder > 0) ? (remainder - 1) : 0);

    if (decoded > _capacity)
    {
        return D_SSH_ERR_ARGUMENT;
    }

    size_t     written = 0;
    const bool valid   = d_ssh_internal_base64_bits(_text,
                                                    length,
                                                    (unsigned char*)_data,
                                                    &written);

    if (!valid)
    {
        return D_SSH_ERR_FORMAT;
    }

    *_size = written;

    return D_SSH_OK;
}
