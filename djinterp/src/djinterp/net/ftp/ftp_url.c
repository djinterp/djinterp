/*******************************************************************************
* djinterp [net]                                                       ftp_url.c
*
* Implementation of the URL handling declared in ftp_url.h.
*   Percent-encoding is net_url.h's: this file adds FTP's rules to it, the
* bytes a decoded path may not hold and the slashes a path keeps.
*
*
* path:      /src/djinterp/net/ftp/ftp_url.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_url.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memchr, memset
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"             // framework root
#include "../../../../inc/djinterp/c/util/sink_common.h"     // d_pack_text
#include "../../../../inc/djinterp/net/net_common.h"  // d_net_ascii_*
#include "../../../../inc/djinterp/net/codec/codec_line.h"   // d_net_line_is_clean
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"     // d_ftp_error
#include "../../../../inc/djinterp/net/ftp/ftp_security.h"   // d_ftp_security
#include "../../../../inc/djinterp/net/net_url.h"            // d_net_url_decode, d_net_url_encode
#include "./ftp_internal.h"                                  // shared helpers
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint16_t


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
    const struct d_pack_text name = { _text,
                                      _length };

    // the three FTP schemes
    if (d_net_ascii_ieq_cstr(name,
                             "ftp"))
    {
        return D_FTP_SCHEME_FTP;
    }

    if (d_net_ascii_ieq_cstr(name,
                             "ftps"))
    {
        return D_FTP_SCHEME_FTPS;
    }

    if (d_net_ascii_ieq_cstr(name,
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
  File-local: reads the host and an optional port with net_url's authority
parser, so the host is a registered name, an IPv4 address, or a bracketed
IPv6 or IPvFuture literal as RFC 3986 writes them. A URL names a host; an
explicit port is 1 to 65535, and an empty one means the scheme's default
(RFC 3986 3.2.3).
*/
D_STATIC enum d_ftp_error
d_ftp_internal_url_host(
    const char*       _text,
    size_t            _start,
    size_t            _end,
    struct d_ftp_url* _out
)
{
    const struct d_pack_text authority = { _text + _start,
                                           _end - _start };
    struct d_net_url         parsed    = { .port = 0u };

    // RFC 3986's host and port; a host there is, and no port 0
    if ( (d_net_url_parse_authority(authority,
                                    &parsed,
                                    NULL) != D_NET_URL_OK) ||
         (parsed.host.length == 0u)                       ||
         ( (parsed.port_text.length > 0u) &&
           (parsed.port == 0u) ) )
    {
        return D_FTP_ERROR_MALFORMED;
    }

    _out->host         = parsed.host;
    _out->port         = parsed.port;
    _out->has_port     = (parsed.port_text.length > 0u);
    _out->ipv6_literal = ( (parsed.host_kind == D_NET_URL_HOST_IPV6) ||
                           (parsed.host_kind == D_NET_URL_HOST_IPVFUTURE) );

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
    if (length >= 7u)
    {
        const struct d_pack_text tail = { _text + length - 7u,
                                          7u };
        const char               code = d_net_ascii_lower(tail.data[6]);

        // the suffix, where there is one, names one of three types
        if (d_net_ascii_istarts(tail,
                                ";type="))
        {
            // any other type refuses the URL
            if ( (code != 'a') &&
                 (code != 'i') &&
                 (code != 'd') )
            {
                return D_FTP_ERROR_MALFORMED;
            }

            _out->type_code  = code;
            length          -= 7u;
        }
    }

    _out->path.data   = _text;
    _out->path.length = length;

    return D_FTP_OK;
}

/*
d_ftp_internal_url_clean
  File-local: reports whether a URL is free of spaces and control
characters, which must be percent-encoded.
*/
D_STATIC bool
d_ftp_internal_url_clean(
    const char* _text,
    size_t      _length
)
{
    // every byte above space, and not DEL
    for (size_t index = 0u; index < _length; index++)
    {
        const unsigned char byte = (unsigned char)_text[index];

        if ( (byte <= 0x20u) ||
             (byte == 0x7Fu) )
        {
            return false;
        }
    }

    return true;
}

/*
d_ftp_internal_url_authority
  File-local: reads the authority between `_start` and `_end`: credentials,
which end at its last '@' so an unencoded '@' in a password still parses
the way browsers and curl read it, then the host and port.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_url_authority(
    const char*       _text,
    size_t            _start,
    size_t            _end,
    struct d_ftp_url* _out
)
{
    size_t host_start = _start;

    // the credentials end at the authority's last '@'
    for (size_t index = _start; index < _end; index++)
    {
        if (_text[index] == '@')
        {
            host_start = index + 1u;
        }
    }

    // user[:password]
    if (host_start > _start)
    {
        d_ftp_internal_url_userinfo(_text,
                                    _start,
                                    host_start - 1u,
                                    _out);
    }

    return d_ftp_internal_url_host(_text,
                                   host_start,
                                   _end,
                                   _out);
}

/*
d_ftp_internal_url_rest
  File-local: reads everything after "scheme://": the authority, which runs
to the first '/', and the path that follows that '/'.
*/
D_STATIC enum d_ftp_error
d_ftp_internal_url_rest(
    const char*       _text,
    size_t            _length,
    size_t            _authority,
    struct d_ftp_url* _out
)
{
    const char* const slash         = memchr(_text + _authority,
                                             '/',
                                             _length - _authority);
    const size_t      authority_end = (slash) ? (size_t)(slash - _text)
                                              : _length;
    enum d_ftp_error  error         =
        d_ftp_internal_url_authority(_text,
                                     _authority,
                                     authority_end,
                                     _out);

    // the path follows the '/' that ends the authority
    if ( (error == D_FTP_OK) &&
         (authority_end < _length) )
    {
        error = d_ftp_internal_url_path(_text + authority_end + 1u,
                                        _length - authority_end - 1u,
                                        _out);
    }

    return error;
}

/*
d_ftp_url_parse
  Scheme, authority, path, in that order; the scheme is recorded last, once
everything else has parsed.
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

    const char* const colon      = memchr(_text,
                                          ':',
                                          _length);
    const size_t      scheme_end = (colon) ? (size_t)(colon - _text)
                                           : _length;

    // encoded throughout, and a scheme that ends in "://"
    if ( (!d_ftp_internal_url_clean(_text,
                                    _length))      ||
         ((scheme_end + 3u) > _length)             ||
         (_text[scheme_end + 1u] != '/')           ||
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

    const enum d_ftp_error error = d_ftp_internal_url_rest(_text,
                                                           _length,
                                                           scheme_end + 3u,
                                                           _out);

    // the scheme, once the rest has parsed
    if (error == D_FTP_OK)
    {
        _out->scheme = scheme;
    }

    return error;
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
d_ftp_percent_decode
  net_url's decoder writes into the buffer's free room; a malformed escape
refuses the text, as does a NUL, CR or LF in what it decodes to, and any
refusal leaves the buffer as it was.
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

    const struct d_pack_text   text    = { _text,
                                           _length };
    const size_t               mark    = _out->length;
    size_t                     decoded = 0u;
    const enum d_net_url_error error   = d_net_url_decode(text,
                                                          _out->data + mark,
                                                          _out->capacity -
                                                              mark,
                                                          &decoded);
    const struct d_pack_text   result  = { _out->data + mark,
                                           decoded };

    // decoded whole, and no NUL, CR or LF in it, which no path may hold
    if ( (error == D_NET_URL_OK) &&
         (d_net_line_is_clean(result)) )
    {
        _out->length = mark + decoded;

        return D_FTP_OK;
    }

    d_ftp_internal_rollback(_out,
                            mark);

    return (error == D_NET_URL_ERROR_BUFFER_TOO_SMALL)
               ? D_FTP_ERROR_BUFFER_TOO_SMALL
               : D_FTP_ERROR_MALFORMED;
}

/*
d_ftp_internal_encode_segment
  File-local: appends one segment, every byte but RFC 3986's unreserved ones
escaped by net_url's encoder; false when it does not fit, leaving the caller
to roll the buffer back.
*/
D_STATIC bool
d_ftp_internal_encode_segment(
    struct d_pack_text   _segment,
    struct d_ftp_buffer* _out
)
{
    size_t written = 0u;

    // the encoding and its terminator, in the room that is left
    if (d_net_url_encode(_segment,
                         D_NET_URL_PART_COMPONENT,
                         _out->data + _out->length,
                         _out->capacity - _out->length,
                         &written) != D_NET_URL_OK)
    {
        return false;
    }

    _out->length += written;

    return true;
}

/*
d_ftp_percent_encode
  Segment by segment, with the slashes between them kept when asked to be;
the text lands whole or not at all.
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

    const size_t mark  = _out->length;
    size_t       start = 0u;

    // one segment per pass, and the slash after it
    for (;;)
    {
        const char* const        slash   =
            (_keep_slashes) ? memchr(_text + start,
                                     '/',
                                     _length - start)
                            : NULL;
        const size_t             end     = (slash) ? (size_t)(slash - _text)
                                                   : _length;
        const struct d_pack_text segment = { _text + start,
                                             end - start };

        // the segment, then its slash, or nothing at all
        if ( (!d_ftp_internal_encode_segment(segment,
                                             _out)) ||
             ( (slash) &&
               (!d_ftp_internal_append_char(_out,
                                            '/')) ) )
        {
            d_ftp_internal_rollback(_out,
                                    mark);

            return D_FTP_ERROR_BUFFER_TOO_SMALL;
        }

        // the last segment ends the text
        if (!slash)
        {
            return D_FTP_OK;
        }

        start = end + 1u;
    }
}
