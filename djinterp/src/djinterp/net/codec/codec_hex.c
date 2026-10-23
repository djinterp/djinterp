/*******************************************************************************
* djinterp [net]                                                     codec_hex.c
*
* Definitions for codec_hex.h's encoder and decoder. The single-digit
* functions are the header's, inline.
*   Both know their output's length from their input's alone, so each checks
* the buffer before writing; the decoder reads the whole text first, so a
* refused text writes nothing.
*
*
* path:      /src/djinterp/net/codec/codec_hex.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/codec/codec_hex.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"             // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"     // d_pack_bytes
#include "../../../../inc/djinterp/net/codec/codec_common.h"  // error codes
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, SIZE_MAX


/*
d_net_hex_encode
  Two digits per octet: the length is known before anything is written, so a
buffer too small is refused untouched but for the NUL that leaves it empty.
*/
enum d_net_codec_error
d_net_hex_encode(
    struct d_pack_bytes _data,
    unsigned int        _flags,
    char*               _text,
    size_t              _capacity,
    size_t*             _length
)
{
    // parameter validation
    if ( ( (_data.data == NULL) &&
           (_data.size > 0) )                              ||
         ( (_text == NULL) &&
           (_capacity > 0) )                               ||
         ((_flags & ~(unsigned int)D_NET_HEX_UPPER) != 0u) ||
         (_data.size > ((SIZE_MAX - 1u) / 2u)) )
    {
        return D_NET_CODEC_ERROR_INVALID_ARGUMENT;
    }

    const size_t needed = 2u * _data.size;

    // the length is reported whatever follows
    if (_length != NULL)
    {
        *_length = needed;
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

    // the high nibble, then the low
    for (size_t i = 0; i < _data.size; i++)
    {
        _text[2u * i]        = d_net_hex_digit((unsigned int)(bytes[i] >> 4),
                                               _flags);
        _text[(2u * i) + 1u] = d_net_hex_digit((unsigned int)(bytes[i] & 0x0fu),
                                               _flags);
    }

    _text[needed] = '\0';

    return D_NET_CODEC_OK;
}

/*
d_net_hex_decode
  Every character is checked before any octet is written, so a malformed text
or a buffer too small leaves the buffer as it was.
*/
enum d_net_codec_error
d_net_hex_decode(
    struct d_pack_text _text,
    void*              _data,
    size_t             _capacity,
    size_t*            _size
)
{
    // parameter validation
    if ( (_size == NULL)          ||
         ( (_text.data == NULL) &&
           (_text.length > 0) )   ||
         ( (_data == NULL) &&
           (_capacity > 0) ) )
    {
        return D_NET_CODEC_ERROR_INVALID_ARGUMENT;
    }

    *_size = 0;

    // an even count of digits, and nothing else
    if ((_text.length % 2u) != 0u)
    {
        return D_NET_CODEC_ERROR_MALFORMED;
    }

    for (size_t i = 0; i < _text.length; i++)
    {
        if (d_net_hex_value(_text.data[i]) < 0)
        {
            return D_NET_CODEC_ERROR_MALFORMED;
        }
    }

    *_size = _text.length / 2u;

    // a measure stops here, and so does a buffer too small
    if (_capacity < *_size)
    {
        return (_data == NULL) ? D_NET_CODEC_OK
                               : D_NET_CODEC_ERROR_BUFFER_TOO_SMALL;
    }

    uint8_t* const out = (uint8_t*)_data;

    // each pair of digits, high nibble first
    for (size_t i = 0; i < *_size; i++)
    {
        const int high = d_net_hex_value(_text.data[2u * i]);
        const int low  = d_net_hex_value(_text.data[(2u * i) + 1u]);

        out[i] = (uint8_t)((high << 4) | low);
    }

    return D_NET_CODEC_OK;
}
