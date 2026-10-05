/*******************************************************************************
* djinterp [net]                                                  ftp_endpoint.c
*
* Implementation of the endpoint encodings declared in ftp_endpoint.h.
*   Addresses are validated for their family before they are written, so no
* delimiter can ride inside one, and each encoding is composed in scratch
* storage sized for its longest form.
*
*
* path:      /src/djinterp/net/ftp/ftp_endpoint.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_endpoint.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint8_t, uint16_t, uint64_t
#include <string.h>   // memchr, memcpy
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_span
#include "./ftp_internal.h"                               // shared helpers


//==============================================================================
// 3.  DATA CONNECTIONS
//==============================================================================

/*
d_ftp_internal_parse_ipv4
  File-local: parses exactly a dotted quad -- four fields of one to three
digits, each at most 255 -- with nothing before or after it.
*/
D_STATIC bool
d_ftp_internal_parse_ipv4(
    const char* _text,
    size_t      _length,
    uint8_t     _out_octets[4]
)
{
    size_t position = 0;

    // four fields separated by three dots
    for (size_t field = 0; field < 4u; field++)
    {
        size_t digits = 0;

        // the field is the run of digits here
        while ( ((position + digits) < _length) &&
                (d_ftp_internal_is_digit(_text[position + digits])) )
        {
            digits++;
        }

        uint64_t   value  = 0;
        const bool parsed = d_ftp_internal_parse_uint(_text + position,
                                                      digits,
                                                      255u,
                                                      &value);

        // one to three digits, at most 255
        if ( (!parsed) ||
             (digits > 3u) )
        {
            return false;
        }

        _out_octets[field]  = (uint8_t)value;
        position           += digits;

        // the first three fields end in a dot
        if (field < 3u)
        {
            if ( (position >= _length) ||
                 (_text[position] != '.') )
            {
                return false;
            }

            position++;
        }
    }

    return (position == _length);
}

/*
d_ftp_internal_is_ipv6_text
  File-local: a shape check on a textual IPv6 address: hex digits, colons, and
an embedded IPv4 tail's dots, with at least two colons and short enough for
an endpoint. The backend's inet_pton() is the real validator; this keeps
delimiters and junk out of the fixed-size address field.
*/
D_STATIC bool
d_ftp_internal_is_ipv6_text(
    const char* _text,
    size_t      _length
)
{
    // "::" is the shortest form; the terminator must still fit
    if ( (_length < 2u) ||
         (_length >= D_FTP_ADDRESS_SIZE) )
    {
        return false;
    }

    size_t colons = 0;

    // hex digits, colons, and dots only
    for (size_t index = 0; index < _length; index++)
    {
        const char c = _text[index];

        // colons are counted as well as allowed
        if (c == ':')
        {
            colons++;

            continue;
        }

        if ( (c != '.') &&
             (d_ftp_internal_hex_value(c) < 0) )
        {
            return false;
        }
    }

    return (colons >= 2u);
}

/*
d_ftp_internal_address_length
  File-local: the length of an endpoint's address text. An array with no
terminator holds no valid text and reports its full size, which no address
check accepts.
*/
D_STATIC size_t
d_ftp_internal_address_length(
    const struct d_ftp_endpoint* _endpoint
)
{
    const char* const terminator = memchr(_endpoint->address,
                                          '\0',
                                          sizeof(_endpoint->address));

    // no terminator: no text
    if (!terminator)
    {
        return sizeof(_endpoint->address);
    }

    return (size_t)(terminator - _endpoint->address);
}

/*
d_ftp_internal_set_ipv4
  File-local: fills `_out` with an IPv4 endpoint. A dotted quad always fits
the address field, so the appends below cannot fail.
*/
D_STATIC void
d_ftp_internal_set_ipv4(
    struct d_ftp_endpoint* _out,
    const uint8_t          _octets[4],
    uint16_t               _port
)
{
    struct d_ftp_buffer text = { _out->address, sizeof(_out->address), 0u };

    _out->address[0] = '\0';

    // four decimal fields joined by dots
    for (size_t field = 0; field < 4u; field++)
    {
        // a dot precedes every field but the first
        if (field > 0u)
        {
            d_ftp_internal_append_char(&text,
                                       '.');
        }

        d_ftp_internal_append_uint(&text,
                                   _octets[field],
                                   0u);
    }

    _out->family = D_FTP_FAMILY_IPV4;
    _out->port   = _port;

    return;
}

/*
d_ftp_internal_endpoint_octets
  File-local: reads an endpoint back into address bytes, failing for anything
PORT and PASV cannot carry: another family, a malformed address, port 0.
*/
D_STATIC bool
d_ftp_internal_endpoint_octets(
    const struct d_ftp_endpoint* _endpoint,
    uint8_t                      _out_octets[4]
)
{
    // only an IPv4 endpoint with a port
    if ( (_endpoint->family != D_FTP_FAMILY_IPV4) ||
         (_endpoint->port == 0u) )
    {
        return false;
    }

    return d_ftp_internal_parse_ipv4(_endpoint->address,
                                     d_ftp_internal_address_length(_endpoint),
                                     _out_octets);
}

/*
d_ftp_internal_write_tuple
  File-local: writes the six comma-separated bytes PORT and PASV share -- the
address bytes, then the port's high and low bytes -- into scratch storage
sized for them.
*/
D_STATIC void
d_ftp_internal_write_tuple(
    struct d_ftp_buffer* _local,
    const uint8_t        _octets[4],
    uint16_t             _port
)
{
    const unsigned values[6] =
    {
        _octets[0],
        _octets[1],
        _octets[2],
        _octets[3],
        (unsigned)(_port >> 8),
        (unsigned)(_port & 0xFFu)
    };

    // six fields with commas between them
    for (size_t field = 0; field < 6u; field++)
    {
        // a comma precedes every field but the first
        if (field > 0u)
        {
            d_ftp_internal_append_char(_local,
                                       ',');
        }

        d_ftp_internal_append_uint(_local,
                                   values[field],
                                   0u);
    }

    return;
}

/*
d_ftp_internal_parse_tuple
  File-local: reads "h1,h2,h3,h4,p1,p2" from the start of the text: six
fields of one to three digits, each at most 255, and reports how many bytes
it spanned. A fourth digit on any field fails it.
*/
D_STATIC bool
d_ftp_internal_parse_tuple(
    const char* _text,
    size_t      _length,
    uint8_t     _out_values[6],
    size_t*     _out_end
)
{
    size_t position = 0;

    // six fields separated by five commas
    for (size_t field = 0; field < 6u; field++)
    {
        size_t digits = 0;

        // the field is the run of digits here
        while ( ((position + digits) < _length) &&
                (d_ftp_internal_is_digit(_text[position + digits])) )
        {
            digits++;
        }

        uint64_t   value  = 0;
        const bool parsed = d_ftp_internal_parse_uint(_text + position,
                                                      digits,
                                                      255u,
                                                      &value);

        // one to three digits, at most 255
        if ( (!parsed) ||
             (digits > 3u) )
        {
            return false;
        }

        _out_values[field]  = (uint8_t)value;
        position           += digits;

        // the first five fields end in a comma
        if (field < 5u)
        {
            if ( (position >= _length) ||
                 (_text[position] != ',') )
            {
                return false;
            }

            position++;
        }
    }

    *_out_end = position;

    return true;
}

/*
d_ftp_internal_tuple_port
  File-local: the port a tuple's last two bytes encode, high byte first.
*/
D_STATIC uint16_t
d_ftp_internal_tuple_port(
    const uint8_t _values[6]
)
{
    return (uint16_t)(((unsigned)_values[4] << 8u) | (unsigned)_values[5]);
}

/*
d_ftp_internal_is_delimiter
  File-local: reports whether `_c` may delimit EPSV and EPRT fields: printable
ASCII from 33 to 126 (RFC 2428 2), excluding digits, which would make the
fields ambiguous.
*/
D_STATIC bool
d_ftp_internal_is_delimiter(
    char _c
)
{
    return ( (_c >= '!')                    &&
             (_c <= '~')                    &&
             (!d_ftp_internal_is_digit(_c)) );
}

/*
d_ftp_parse_pasv
  Tries each run of digits in turn, because servers put the tuple in
parentheses, after '=', or bare, and some put other digits before it.
*/
enum d_ftp_error
d_ftp_parse_pasv(
    const char*            _text,
    size_t                 _length,
    struct d_ftp_endpoint* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // each run of digits is a candidate
    for (size_t start = 0; start < _length; start++)
    {
        // only the first digit of a run starts a candidate
        if ( (!d_ftp_internal_is_digit(_text[start])) ||
             ( (start > 0u) &&
               (d_ftp_internal_is_digit(_text[start - 1u])) ) )
        {
            continue;
        }

        uint8_t    values[6] = { 0 };
        size_t     end       = 0;
        const bool parsed    = d_ftp_internal_parse_tuple(_text + start,
                                                          _length - start,
                                                          values,
                                                          &end);

        // a valid tuple with a usable port ends the search
        if ( (parsed) &&
             (d_ftp_internal_tuple_port(values) != 0u) )
        {
            d_ftp_internal_set_ipv4(_out,
                                    values,
                                    d_ftp_internal_tuple_port(values));

            return D_FTP_OK;
        }
    }

    return D_FTP_ERROR_MALFORMED;
}

/*
d_ftp_parse_epsv
  Scans for '(' followed by three identical delimiters, a port, and the
delimiter again; the closing ')' is not required, since some servers omit it.
*/
enum d_ftp_error
d_ftp_parse_epsv(
    const char*            _text,
    size_t                 _length,
    struct d_ftp_endpoint* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    // each '(' with room for "ddd" and a port after it is a candidate
    for (size_t open = 0; (open + 5u) < _length; open++)
    {
        const char delimiter = _text[open + 1u];

        // "(" and three equal delimiters
        if ( (_text[open] != '(')                           ||
             (!d_ftp_internal_is_delimiter(delimiter))      ||
             (_text[open + 2u] != delimiter)                ||
             (_text[open + 3u] != delimiter) )
        {
            continue;
        }

        const size_t digits_start = open + 4u;
        size_t       digits_end   = digits_start;

        // the port runs to the next non-digit
        while ( (digits_end < _length) &&
                (d_ftp_internal_is_digit(_text[digits_end])) )
        {
            digits_end++;
        }

        uint64_t   port   = 0;
        const bool parsed = d_ftp_internal_parse_uint(_text + digits_start,
                                                      digits_end -
                                                      digits_start,
                                                      65535u,
                                                      &port);

        // a port of 1 to 65535, closed by the same delimiter
        if ( (parsed)                        &&
             (port != 0u)                    &&
             (digits_end < _length)          &&
             (_text[digits_end] == delimiter) )
        {
            _out->family     = D_FTP_FAMILY_NONE;
            _out->port       = (uint16_t)port;
            _out->address[0] = '\0';

            return D_FTP_OK;
        }
    }

    return D_FTP_ERROR_MALFORMED;
}

/*
d_ftp_parse_port
  The tuple must be the whole argument, blanks aside.
*/
enum d_ftp_error
d_ftp_parse_port(
    const char*            _text,
    size_t                 _length,
    struct d_ftp_endpoint* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const struct d_ftp_span text      = d_ftp_internal_trim(_text,
                                                            _length);
    uint8_t                 values[6] = { 0 };
    size_t                  end       = 0;
    const bool              parsed    = d_ftp_internal_parse_tuple(text.data,
                                                                   text.length,
                                                                   values,
                                                                   &end);

    // exactly one tuple, with a usable port
    if ( (!parsed)                                  ||
         (end != text.length)                       ||
         (d_ftp_internal_tuple_port(values) == 0u) )
    {
        return D_FTP_ERROR_MALFORMED;
    }

    d_ftp_internal_set_ipv4(_out,
                            values,
                            d_ftp_internal_tuple_port(values));

    return D_FTP_OK;
}

/*
d_ftp_internal_eprt_fields
  File-local: cuts an EPRT argument, wrapped in its delimiter, into exactly
three fields: protocol, address, and port.
*/
D_STATIC bool
d_ftp_internal_eprt_fields(
    struct d_ftp_span  _text,
    struct d_ftp_span* _fields
)
{
    size_t count = 0u;
    size_t start = 1u;

    // cut a field at each further delimiter
    for (size_t index = 1u; index < _text.length; index++)
    {
        // only delimiters end fields
        if (_text.data[index] != _text.data[0])
        {
            continue;
        }

        // a fourth field is a delimiter too many
        if (count == 3u)
        {
            return false;
        }

        _fields[count].data   = _text.data + start;
        _fields[count].length = index - start;
        count++;
        start = index + 1u;
    }

    return (count == 3u);
}

/*
d_ftp_internal_eprt_store
  File-local: checks an EPRT argument's address, of the protocol's family,
and its port of 1 to 65535, then stores them.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_eprt_store(
    const struct d_ftp_span* _fields,
    uint64_t                 _protocol,
    struct d_ftp_endpoint*   _out
)
{
    uint8_t    octets[4]  = { 0 };
    uint64_t   port       = 0u;
    const bool address_ok = (_protocol == 1u)
                            ? d_ftp_internal_parse_ipv4(_fields[1].data,
                                                        _fields[1].length,
                                                        octets)
                            : d_ftp_internal_is_ipv6_text(_fields[1].data,
                                                          _fields[1].length);

    // a well-formed address and a port of 1 to 65535
    if ( (!address_ok)                                 ||
         (!d_ftp_internal_parse_uint(_fields[2].data,
                                     _fields[2].length,
                                     65535u,
                                     &port))           ||
         (port == 0u) )
    {
        return D_FTP_ERROR_MALFORMED;
    }

    memcpy(_out->address,
           _fields[1].data,
           _fields[1].length);

    _out->address[_fields[1].length] = '\0';
    _out->family = (_protocol == 1u) ? D_FTP_FAMILY_IPV4 : D_FTP_FAMILY_IPV6;
    _out->port   = (uint16_t)port;

    return D_FTP_OK;
}

/*
d_ftp_parse_eprt
  Splits at the delimiter the argument opens with, which must also close it,
into exactly three fields: protocol, address, and port.
*/
enum d_ftp_error
d_ftp_parse_eprt(
    const char*            _text,
    size_t                 _length,
    struct d_ftp_endpoint* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const struct d_ftp_span text      = d_ftp_internal_trim(_text,
                                                            _length);
    struct d_ftp_span       fields[3] = { { NULL, 0u } };
    uint64_t                protocol  = 0u;

    // one delimiter, no digit, around three fields, the first a number
    if ( (text.length < 2u)                                   ||
         (!d_ftp_internal_is_delimiter(text.data[0]))         ||
         (text.data[text.length - 1u] != text.data[0])        ||
         (!d_ftp_internal_eprt_fields(text,
                                      fields))                ||
         (!d_ftp_internal_parse_uint(fields[0].data,
                                     fields[0].length,
                                     255u,
                                     &protocol)) )
    {
        return D_FTP_ERROR_MALFORMED;
    }

    // RFC 2428 defines only IPv4 (1) and IPv6 (2)
    if ( (protocol != 1u) &&
         (protocol != 2u) )
    {
        return D_FTP_ERROR_PROTOCOL_UNSUPPORTED;
    }

    return d_ftp_internal_eprt_store(fields,
                                     protocol,
                                     _out);
}

/*
d_ftp_format_port
  Scratch storage sized by D_FTP_PORT_ARGUMENT_SIZE holds any tuple.
*/
enum d_ftp_error
d_ftp_format_port(
    const struct d_ftp_endpoint* _endpoint,
    struct d_ftp_buffer*         _out
)
{
    // parameter validation
    if ( (!_endpoint) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t    octets[4] = { 0 };
    const bool usable    = d_ftp_internal_endpoint_octets(_endpoint,
                                                          octets);

    // PORT carries IPv4 only
    if (!usable)
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    char                text[D_FTP_PORT_ARGUMENT_SIZE] = { 0 };
    struct d_ftp_buffer local = { text, sizeof(text), 0u };

    d_ftp_internal_write_tuple(&local,
                               octets,
                               _endpoint->port);

    return d_ftp_internal_commit(_out,
                                 &local);
}

/*
d_ftp_format_eprt
  The address is validated for its family before it is written, so no
delimiter can ride along inside it.
*/
enum d_ftp_error
d_ftp_format_eprt(
    const struct d_ftp_endpoint* _endpoint,
    struct d_ftp_buffer*         _out
)
{
    // parameter validation
    if ( (!_endpoint) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t length    = d_ftp_internal_address_length(_endpoint);
    uint8_t      octets[4] = { 0 };
    bool         valid     = false;

    // the address must suit its family
    if (_endpoint->family == D_FTP_FAMILY_IPV4)
    {
        valid = d_ftp_internal_parse_ipv4(_endpoint->address,
                                          length,
                                          octets);
    }
    else if (_endpoint->family == D_FTP_FAMILY_IPV6)
    {
        valid = d_ftp_internal_is_ipv6_text(_endpoint->address,
                                            length);
    }

    // and a zero port names no connection
    if ( (!valid) ||
         (_endpoint->port == 0u) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    char                text[D_FTP_EPRT_ARGUMENT_SIZE] = { 0 };
    struct d_ftp_buffer local = { text, sizeof(text), 0u };
    const char          protocol =
        (_endpoint->family == D_FTP_FAMILY_IPV4) ? '1' : '2';

    d_ftp_internal_append_char(&local,
                               '|');
    d_ftp_internal_append_char(&local,
                               protocol);
    d_ftp_internal_append_char(&local,
                               '|');
    d_ftp_internal_append(&local,
                          _endpoint->address,
                          length);
    d_ftp_internal_append_char(&local,
                               '|');
    d_ftp_internal_append_uint(&local,
                               _endpoint->port,
                               0u);
    d_ftp_internal_append_char(&local,
                               '|');

    return d_ftp_internal_commit(_out,
                                 &local);
}

/*
d_ftp_format_pasv
  The conventional 227 text, whose parenthesized tuple every client finds.
*/
enum d_ftp_error
d_ftp_format_pasv(
    const struct d_ftp_endpoint* _endpoint,
    struct d_ftp_buffer*         _out
)
{
    // parameter validation
    if ( (!_endpoint) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t    octets[4] = { 0 };
    const bool usable    = d_ftp_internal_endpoint_octets(_endpoint,
                                                          octets);

    // PASV carries IPv4 only
    if (!usable)
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    char                text[64] = { 0 };
    struct d_ftp_buffer local    = { text, sizeof(text), 0u };

    d_ftp_internal_append_text(&local,
                               "Entering Passive Mode (");
    d_ftp_internal_write_tuple(&local,
                               octets,
                               _endpoint->port);
    d_ftp_internal_append_text(&local,
                               ").");

    return d_ftp_internal_commit(_out,
                                 &local);
}

/*
d_ftp_format_epsv
  RFC 2428's own example form, with '|' as the delimiter.
*/
enum d_ftp_error
d_ftp_format_epsv(
    const struct d_ftp_endpoint* _endpoint,
    struct d_ftp_buffer*         _out
)
{
    // parameter validation; a zero port names no connection
    if ( (!_endpoint)                      ||
         (!d_ftp_internal_buffer_ok(_out)) ||
         (_endpoint->port == 0u) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    char                text[64] = { 0 };
    struct d_ftp_buffer local    = { text, sizeof(text), 0u };

    d_ftp_internal_append_text(&local,
                               "Entering Extended Passive Mode (|||");
    d_ftp_internal_append_uint(&local,
                               _endpoint->port,
                               0u);
    d_ftp_internal_append_text(&local,
                               "|)");

    return d_ftp_internal_commit(_out,
                                 &local);
}
