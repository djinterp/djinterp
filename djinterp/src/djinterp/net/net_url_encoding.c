/*******************************************************************************
* djinterp [net]                                              net_url_encoding.c
*
* Percent-encoding and decoding: net_url.h's section 4.
*   Each component leaves its own class of characters unencoded; a '%' is
* always encoded, and escapes are written in uppercase hex. Decoding refuses a
* malformed escape rather than passing it through.
*
*
* path:      /src/djinterp/net/net_url_encoding.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../inc/djinterp/net/net_url.h"  // corresponding header
// djinterp
#include "./net_url_internal.h"  // shared helpers


/*
d_net_url_part_class
  The characters a component keeps unencoded; '%' is always encoded.
*/
static d_net_url_allows
d_net_url_part_class(
    enum d_net_url_part _part
)
{
    switch (_part)
    {
        case D_NET_URL_PART_USERINFO:
            return d_net_url_allows_userinfo;
        case D_NET_URL_PART_HOST:
            return d_net_url_allows_name;
        case D_NET_URL_PART_PATH:
            return d_net_url_allows_path;
        case D_NET_URL_PART_SEGMENT:
            return d_net_url_is_pchar;
        case D_NET_URL_PART_QUERY:
        case D_NET_URL_PART_FRAGMENT:
            return d_net_url_allows_query;
        case D_NET_URL_PART_COMPONENT:
            return d_net_url_is_unreserved;
        default:
            return NULL;
    }
}

/*
d_net_url_encode
  Every byte outside the component's class, and every '%', becomes an
escape.
*/
enum d_net_url_error
d_net_url_encode(
    struct d_pack_text  _text,
    enum d_net_url_part _part,
    char*               _buffer,
    size_t              _capacity,
    size_t*             _length
)
{
    const d_net_url_allows keeps = d_net_url_part_class(_part);

    // parameter validation
    if ( (!keeps)   ||
         (!_length) ||
         ( (!_text.data) &&
           (_text.length != 0u) ) ||
         ( (!_buffer) &&
           (_capacity != 0u) ) )
    {
        return D_NET_URL_ERROR_ARGUMENT;
    }

    struct d_net_url_writer out = { _buffer,
                                    _capacity,
                                    0u };

    // each byte, kept or escaped
    for (size_t i = 0u; i < _text.length; ++i)
    {
        const char c = _text.data[i];

        // a byte the component takes as it is
        if ( (c != '%') &&
             (keeps(c)) )
        {
            d_net_url_put_char(&out,
                               c);

            continue;
        }

        d_net_url_put_escape(&out,
                             (unsigned char)c);
    }

    return d_net_url_finish(&out,
                            _length);
}

/*
d_net_url_decode
  Each escape becomes its byte; a malformed one refuses the whole text.
*/
enum d_net_url_error
d_net_url_decode(
    struct d_pack_text _text,
    char*              _buffer,
    size_t             _capacity,
    size_t*            _length
)
{
    // parameter validation
    if ( (!_length) ||
         ( (!_text.data) &&
           (_text.length != 0u) ) ||
         ( (!_buffer) &&
           (_capacity != 0u) ) )
    {
        return D_NET_URL_ERROR_ARGUMENT;
    }

    struct d_net_url_writer out = { _buffer,
                                    _capacity,
                                    0u };

    // each character, an escape taken whole
    for (size_t i = 0u; i < _text.length; ++i)
    {
        char c = _text.data[i];

        // an escape needs its two hex digits
        if (c == '%')
        {
            // missing or wrong digits refuse the text
            if ( (i + 2u >= _text.length)              ||
                 (!d_net_url_is_hex(_text.data[i + 1u])) ||
                 (!d_net_url_is_hex(_text.data[i + 2u])) )
            {
                *_length = 0u;

                return D_NET_URL_ERROR_PERCENT;
            }

            c  = (char)( (d_net_url_hex_value(_text.data[i + 1u]) * 16) +
                         d_net_url_hex_value(_text.data[i + 2u]) );
            i += 2u;
        }

        d_net_url_put_char(&out,
                           c);
    }

    return d_net_url_finish(&out,
                            _length);
}
