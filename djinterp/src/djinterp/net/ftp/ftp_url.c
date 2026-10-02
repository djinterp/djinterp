/*******************************************************************************
* djinterp [net]                                                       ftp_url.c
*
* Implementation of the URL handling declared in ftp_url.h.
*
*
* path:      /src/djinterp/net/ftp/ftp_url.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_url.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // uint16_t, uint64_t
#include <string.h>   // memchr, memset
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"            // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"    // d_ftp_error
#include "../../../../inc/djinterp/net/ftp/ftp_security.h"  // d_ftp_security
#include "./ftp_internal.h"                                 // shared helpers


//==============================================================================
// 2.  URLS
//==============================================================================

/*
d_ftp_scheme_default_port
  Only implicit FTPS moves off port 21.
*/
uint16_t
d_ftp_scheme_default_port(
    enum d_ftp_scheme _scheme
)
{
    switch (_scheme)
    {
        case D_FTP_SCHEME_FTP:
        case D_FTP_SCHEME_FTPES:
            return D_FTP_PORT_CONTROL;

        case D_FTP_SCHEME_FTPS:
            return D_FTP_PORT_IMPLICIT_TLS;

        case D_FTP_SCHEME_NONE:
        default:
            break;
    }

    return 0u;
}

/*
d_ftp_scheme_security
  ftps:// is implicit TLS and ftpes:// explicit; ftp:// asks for none, though
options may still request explicit TLS.
*/
enum d_ftp_security
d_ftp_scheme_security(
    enum d_ftp_scheme _scheme
)
{
    switch (_scheme)
    {
        case D_FTP_SCHEME_FTPS:
            return D_FTP_SECURITY_IMPLICIT;

        case D_FTP_SCHEME_FTPES:
            return D_FTP_SECURITY_EXPLICIT;

        case D_FTP_SCHEME_FTP:
        case D_FTP_SCHEME_NONE:
        default:
            break;
    }

    return D_FTP_SECURITY_NONE;
}

/*
d_ftp_internal_scheme
  File-local: the scheme a URL's leading name selects, case-insensitively.
*/
D_STATIC enum d_ftp_scheme
d_ftp_internal_scheme(
    const char* _text,
    size_t      _length
)
{
    // the three FTP schemes
    if (d_ftp_internal_equals_nocase(_text,
                                     _length,
                                     "ftp"))
    {
        return D_FTP_SCHEME_FTP;
    }

    if (d_ftp_internal_equals_nocase(_text,
                                     _length,
                                     "ftps"))
    {
        return D_FTP_SCHEME_FTPS;
    }

    if (d_ftp_internal_equals_nocase(_text,
                                     _length,
                                     "ftpes"))
    {
        return D_FTP_SCHEME_FTPES;
    }

    return D_FTP_SCHEME_NONE;
}

/*
d_ftp_internal_url_userinfo
  File-local: splits "user[:password]" at its first ':'; either part may be
empty.
*/
D_STATIC void
d_ftp_internal_url_userinfo(
    const char*       _text,
    size_t            _start,
    size_t            _end,
    struct d_ftp_url* _out
)
{
    size_t colon = _start;

    // the user name runs to the first ':'
    while ( (colon < _end) &&
            (_text[colon] != ':') )
    {
        colon++;
    }

    _out->has_user    = true;
    _out->user.data   = _text + _start;
    _out->user.length = colon - _start;

    // a ':' introduces the password
    if (colon < _end)
    {
        _out->has_password    = true;
        _out->password.data   = _text + colon + 1u;
        _out->password.length = _end - colon - 1u;
    }

    return;
}

/*
d_ftp_internal_url_host
  File-local: reads the host, bracketed when it is an IPv6 literal, and an
optional port; an empty port means the scheme's default (RFC 3986 3.2.3).
*/
D_STATIC enum d_ftp_error
d_ftp_internal_url_host(
    const char*       _text,
    size_t            _start,
    size_t            _end,
    struct d_ftp_url* _out
)
{
    size_t port_start = _end;

    // an IPv6 literal sits in brackets
    if ( (_start < _end) &&
         (_text[_start] == '[') )
    {
        const char* const close = memchr(_text + _start,
                                         ']',
                                         _end - _start);

        // an unclosed bracket
        if (!close)
        {
            return D_FTP_ERROR_MALFORMED;
        }

        const size_t close_index = (size_t)(close - _text);

        _out->host.data    = _text + _start + 1u;
        _out->host.length  = close_index - _start - 1u;
        _out->ipv6_literal = true;

        // after the bracket: nothing, or ':' and the port
        if ((close_index + 1u) < _end)
        {
            if (_text[close_index + 1u] != ':')
            {
                return D_FTP_ERROR_MALFORMED;
            }

            port_start = close_index + 2u;
        }
    }
    else
    {
        const char* const colon    = memchr(_text + _start,
                                            ':',
                                            _end - _start);
        const size_t      host_end = (colon) ? (size_t)(colon - _text)
                                             : _end;

        _out->host.data   = _text + _start;
        _out->host.length = host_end - _start;

        // a ':' introduces the port
        if (colon)
        {
            port_start = host_end + 1u;
        }
    }

    // a URL names a host
    if (_out->host.length == 0u)
    {
        return D_FTP_ERROR_MALFORMED;
    }

    // an explicit port of 1 to 65535
    if (port_start < _end)
    {
        uint64_t   port   = 0;
        const bool parsed = d_ftp_internal_parse_uint(_text + port_start,
                                                      _end - port_start,
                                                      65535u,
                                                      &port);

        if ( (!parsed) ||
             (port == 0u) )
        {
            return D_FTP_ERROR_MALFORMED;
        }

        _out->port     = (uint16_t)port;
        _out->has_port = true;
    }

    return D_FTP_OK;
}

/*
d_ftp_internal_url_path
  File-local: records the path, first peeling off a trailing ";type=X"
(RFC 1738 3.2.2).
*/
D_STATIC enum d_ftp_error
d_ftp_internal_url_path(
    const char*       _text,
    size_t            _length,
    struct d_ftp_url* _out
)
{
    size_t length = _length;

    // ";type=a", ";type=i", or ";type=d" chooses the transfer
    if ( (length >= 7u) &&
         (d_ftp_internal_equals_nocase(_text + length - 7u,
                                       6u,
                                       ";type=")) )
    {
        const char code = d_ftp_internal_to_lower(_text[length - 1u]);

        if ( (code != 'a') &&
             (code != 'i') &&
             (code != 'd') )
        {
            return D_FTP_ERROR_MALFORMED;
        }

        _out->type_code  = code;
        length          -= 7u;
    }

    _out->path.data   = _text;
    _out->path.length = length;

    return D_FTP_OK;
}

/*
d_ftp_url_parse
  Scheme, authority, path, in that order. The credentials end at the
authority's last '@', so an unencoded '@' in a password still parses the
way browsers and curl read it.
*/
enum d_ftp_error
d_ftp_url_parse(
    const char*       _text,
    size_t            _length,
    struct d_ftp_url* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!_out) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    memset(_out,
           0,
           sizeof(*_out));

    // spaces and control characters must be percent-encoded
    for (size_t index = 0; index < _length; index++)
    {
        const unsigned char byte = (unsigned char)_text[index];

        if ( (byte <= 0x20u) ||
             (byte == 0x7Fu) )
        {
            return D_FTP_ERROR_MALFORMED;
        }
    }

    const char* const colon      = memchr(_text,
                                          ':',
                                          _length);
    const size_t      scheme_end = (colon) ? (size_t)(colon - _text)
                                           : _length;

    // the scheme ends in "://"
    if ( ((scheme_end + 3u) > _length)      ||
         (_text[scheme_end + 1u] != '/')    ||
         (_text[scheme_end + 2u] != '/') )
    {
        return D_FTP_ERROR_MALFORMED;
    }

    const enum d_ftp_scheme scheme = d_ftp_internal_scheme(_text,
                                                           scheme_end);

    // only the FTP family is understood here
    if (scheme == D_FTP_SCHEME_NONE)
    {
        return D_FTP_ERROR_UNSUPPORTED;
    }

    const size_t      authority     = scheme_end + 3u;
    const char* const slash         = memchr(_text + authority,
                                             '/',
                                             _length - authority);
    const size_t      authority_end = (slash) ? (size_t)(slash - _text)
                                              : _length;
    size_t            host_start    = authority;

    // the credentials end at the authority's last '@'
    for (size_t index = authority; index < authority_end; index++)
    {
        if (_text[index] == '@')
        {
            host_start = index + 1u;
        }
    }

    // user[:password]
    if (host_start > authority)
    {
        d_ftp_internal_url_userinfo(_text,
                                    authority,
                                    host_start - 1u,
                                    _out);
    }

    const enum d_ftp_error host = d_ftp_internal_url_host(_text,
                                                          host_start,
                                                          authority_end,
                                                          _out);

    // host and port
    if (host != D_FTP_OK)
    {
        return host;
    }

    // the path follows the '/' that ends the authority
    if (authority_end < _length)
    {
        const enum d_ftp_error path = d_ftp_internal_url_path(
                                          _text + authority_end + 1u,
                                          _length - authority_end - 1u,
                                          _out);

        if (path != D_FTP_OK)
        {
            return path;
        }
    }

    _out->scheme = scheme;

    return D_FTP_OK;
}

/*
d_ftp_url_port
  The explicit port wins over the scheme's.
*/
uint16_t
d_ftp_url_port(
    const struct d_ftp_url* _url
)
{
    // parameter validation
    if (!_url)
    {
        return 0u;
    }

    return (_url->has_port) ? _url->port
                            : d_ftp_scheme_default_port(_url->scheme);
}

/*
d_ftp_internal_is_unreserved
  File-local: RFC 3986's unreserved characters, which never need escaping.
*/
D_STATIC bool
d_ftp_internal_is_unreserved(
    char _c
)
{
    return ( (d_ftp_internal_is_alpha(_c)) ||
             (d_ftp_internal_is_digit(_c)) ||
             (_c == '-')                   ||
             (_c == '.')                   ||
             (_c == '_')                   ||
             (_c == '~') );
}

/*
d_ftp_percent_decode
  Byte by byte, rolling back on any failure so the buffer is left as it was.
*/
enum d_ftp_error
d_ftp_percent_decode(
    const char*          _text,
    size_t               _length,
    struct d_ftp_buffer* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t mark  = _out->length;
    size_t       index = 0;

    // one byte or one escape per pass
    while (index < _length)
    {
        char   c     = _text[index];
        size_t width = 1u;

        // "%XX" is one byte
        if (c == '%')
        {
            const int high = ((index + 2u) < _length)
                             ? d_ftp_internal_hex_value(_text[index + 1u])
                             : -1;
            const int low  = ((index + 2u) < _length)
                             ? d_ftp_internal_hex_value(_text[index + 2u])
                             : -1;

            // a truncated or non-hex escape, or a forbidden byte
            if ( (high < 0) ||
                 (low < 0) )
            {
                d_ftp_internal_rollback(_out,
                                        mark);

                return D_FTP_ERROR_MALFORMED;
            }

            c     = (char)(unsigned char)((high * 16) + low);
            width = 3u;

            // NUL truncates a name; CR and LF would inject a command
            if ( (c == '\0') ||
                 (c == '\r') ||
                 (c == '\n') )
            {
                d_ftp_internal_rollback(_out,
                                        mark);

                return D_FTP_ERROR_MALFORMED;
            }
        }

        // the decoded byte
        if (!d_ftp_internal_append_char(_out,
                                        c))
        {
            d_ftp_internal_rollback(_out,
                                    mark);

            return D_FTP_ERROR_BUFFER_TOO_SMALL;
        }

        index += width;
    }

    return D_FTP_OK;
}

/*
d_ftp_percent_encode
  The exact length is counted first, so the text lands whole or not at all.
*/
enum d_ftp_error
d_ftp_percent_encode(
    const char*          _text,
    size_t               _length,
    bool                 _keep_slashes,
    struct d_ftp_buffer* _out
)
{
    // parameter validation
    if ( (!_text) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    static const char HEX[] = "0123456789ABCDEF";
    size_t            needed = 0;

    // measure: one byte or three
    for (size_t index = 0; index < _length; index++)
    {
        const bool plain = ( (d_ftp_internal_is_unreserved(_text[index])) ||
                             ( (_keep_slashes) &&
                               (_text[index] == '/') ) );

        needed += (plain) ? 1u : 3u;
    }

    // all of it, or nothing
    if (needed > d_ftp_internal_room(_out))
    {
        return D_FTP_ERROR_BUFFER_TOO_SMALL;
    }

    // write
    for (size_t index = 0; index < _length; index++)
    {
        const unsigned char byte  = (unsigned char)_text[index];
        const bool          plain =
            ( (d_ftp_internal_is_unreserved(_text[index])) ||
              ( (_keep_slashes) &&
                (_text[index] == '/') ) );

        // unreserved bytes pass; the rest become "%XX"
        if (plain)
        {
            d_ftp_internal_append_char(_out,
                                       _text[index]);
        }
        else
        {
            d_ftp_internal_append_char(_out,
                                       '%');
            d_ftp_internal_append_char(_out,
                                       HEX[byte >> 4]);
            d_ftp_internal_append_char(_out,
                                       HEX[byte & 0x0Fu]);
        }
    }

    return D_FTP_OK;
}
