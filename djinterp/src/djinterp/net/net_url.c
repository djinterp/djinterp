/*******************************************************************************
* djinterp [net]                                                       net_url.c
*
* URLs parsed and recomposed: net_url.h's section 2.
*   Parsing splits a reference at its delimiters, as RFC 3986's Appendix B
* does, then checks each component against its own grammar, so an error names
* the first character no grammar allows: every host form (registered names,
* strict IPv4, IPv6 with "::" and an IPv4 tail, RFC 6874 zones, IPvFuture),
* ports, and the rule that a relative reference's first segment holds no
* colon. Recomposition (section 5.3) writes every present component back,
* empty ones included.
*
*
* path:      /src/djinterp/net/net_url.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../inc/djinterp/net/net_url.h"  // corresponding header
// std
#include <string.h>  // memcmp, memset, strchr
// djinterp
#include "./net_url_internal.h"  // shared helpers


/*
d_net_url_find
  The offset of the first character of a set at or after _start, or the
text's length. A NUL in the text is never taken for the set's terminator.
*/
static size_t
d_net_url_find(
    struct d_pack_text _text,
    size_t             _start,
    const char*        _set
)
{
    // each character, until one is in the set
    for (size_t i = _start; i < _text.length; ++i)
    {
        // the set's own terminator is not one of its characters
        if ( (_text.data[i] != '\0') &&
             (strchr(_set,
                     _text.data[i]) != NULL) )
        {
            return i;
        }
    }

    return _text.length;
}

/*
d_net_url_scan
  Checks a component against its character class, escapes as units. On
failure the offending offset, counted from the whole text, goes to
_error_at, and the result is _refusal, or D_NET_URL_ERROR_PERCENT for a
malformed escape.
*/
static enum d_net_url_error
d_net_url_scan(
    struct d_pack_text   _part,
    size_t               _offset,
    d_net_url_allows     _allows,
    enum d_net_url_error _refusal,
    size_t*              _error_at
)
{
    // each character, an escape taken whole
    for (size_t i = 0u; i < _part.length; ++i)
    {
        const char c = _part.data[i];

        // a character the component may not hold
        if (!_allows(c))
        {
            *_error_at = _offset + i;

            return _refusal;
        }

        // an escape needs its two hex digits
        if (c == '%')
        {
            // the digits are missing or wrong
            if ( (i + 2u >= _part.length)              ||
                 (!d_net_url_is_hex(_part.data[i + 1u])) ||
                 (!d_net_url_is_hex(_part.data[i + 2u])) )
            {
                *_error_at = _offset + i;

                return D_NET_URL_ERROR_PERCENT;
            }

            i += 2u;
        }
    }

    return D_NET_URL_OK;
}

/*
d_net_url_take_octet
  One dec-octet of an IPv4 address: 0 to 255, without a leading zero.
*/
static bool
d_net_url_take_octet(
    struct d_pack_text _text,
    size_t*            _pos
)
{
    const size_t start = *_pos;
    unsigned int value = 0u;

    // at most three digits
    while ( (*_pos < _text.length)              &&
            (*_pos - start < 3u)                &&
            (d_net_url_is_digit(_text.data[*_pos])) )
    {
        value = (value * 10u) + (unsigned int)(_text.data[*_pos] - '0');
        *_pos += 1u;
    }

    const size_t digits = *_pos - start;

    return ( (digits > 0u)     &&
             (value <= 255u)   &&
             ( (digits == 1u) ||
               (_text.data[start] != '0') ) );
}

/*
d_net_url_is_ipv4
  IPv4address: four dec-octets separated by dots. A host that fails this
is a registered name, which digits and dots can also form.
*/
static bool
d_net_url_is_ipv4(
    struct d_pack_text _text
)
{
    size_t pos = 0u;

    // four octets, a dot between each two
    for (unsigned int octet = 0u; octet < 4u; ++octet)
    {
        // a malformed octet ends it
        if (!d_net_url_take_octet(_text,
                                  &pos))
        {
            return false;
        }

        // the dot, except after the last octet
        if (octet < 3u)
        {
            // no dot where one belongs
            if ( (pos >= _text.length) ||
                 (_text.data[pos] != '.') )
            {
                return false;
            }

            pos += 1u;
        }
    }

    return (pos == _text.length);
}

/*
d_net_url_ipv6_step
  After a group of an IPv6 address: consumes its ':' or "::". Reports
false for a malformed separator, sets *_done at the end of the text.
*/
static bool
d_net_url_ipv6_step(
    struct d_pack_text _text,
    size_t*            _pos,
    bool*              _compressed,
    bool*              _done
)
{
    // the address ends after this group
    if (*_pos == _text.length)
    {
        *_done = true;

        return true;
    }

    // groups are separated by ':'
    if (_text.data[*_pos] != ':')
    {
        return false;
    }

    *_pos += 1u;

    // a second ':' makes "::", of which there may be one
    if ( (*_pos < _text.length) &&
         (_text.data[*_pos] == ':') )
    {
        // a second "::"
        if (*_compressed)
        {
            return false;
        }

        *_compressed = true;
        *_pos       += 1u;
        *_done       = (*_pos == _text.length);

        return true;
    }

    // a single ':' must lead to another group
    const bool another = (*_pos < _text.length);

    return another;
}

/*
d_net_url_is_ipv6
  IPv6address (RFC 3986, 3.2.2): up to eight groups of one to four hex
digits, one "::" standing for one or more zero groups, and an IPv4 address
allowed in place of the last two groups.
*/
static bool
d_net_url_is_ipv6(
    struct d_pack_text _text
)
{
    size_t       pos        = 0u;
    unsigned int groups     = 0u;
    bool         compressed = false;
    bool         done       = false;

    // a leading "::"
    if ( (_text.length >= 2u) &&
         (memcmp(_text.data,
                 "::",
                 2u) == 0) )
    {
        compressed = true;
        pos        = 2u;
        done       = (pos == _text.length);
    }

    // groups and their separators, in turn
    while (!done)
    {
        const size_t start = pos;

        // one to four hex digits
        while ( (pos < _text.length)            &&
                (pos - start < 4u)              &&
                (d_net_url_is_hex(_text.data[pos])) )
        {
            pos += 1u;
        }

        // an IPv4 address takes the last two groups
        if ( (pos < _text.length) &&
             (_text.data[pos] == '.') )
        {
            return ( ( (compressed) ? (groups <= 5u)
                                    : (groups == 6u) ) &&
                     (d_net_url_is_ipv4(d_net_url_view(_text,
                                                       start,
                                                       _text.length))) );
        }

        groups += 1u;

        // an empty group, or a malformed separator
        if ( (pos == start) ||
             (!d_net_url_ipv6_step(_text,
                                   &pos,
                                   &compressed,
                                   &done)) )
        {
            return false;
        }
    }

    return (compressed) ? (groups <= 7u)
                        : (groups == 8u);
}

/*
d_net_url_is_future
  IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
*/
static bool
d_net_url_is_future(
    struct d_pack_text _inner
)
{
    size_t pos    = 1u;
    size_t unused = 0u;

    // the version: one or more hex digits
    while ( (pos < _inner.length) &&
            (d_net_url_is_hex(_inner.data[pos])) )
    {
        pos += 1u;
    }

    // a version, a '.', and at least one character after it
    if ( (pos == 1u)                 ||
         (pos + 1u >= _inner.length) ||
         (_inner.data[pos] != '.') )
    {
        return false;
    }

    return (d_net_url_scan(d_net_url_view(_inner,
                                          pos + 1u,
                                          _inner.length),
                           0u,
                           d_net_url_allows_future,
                           D_NET_URL_ERROR_IP_LITERAL,
                           &unused) == D_NET_URL_OK);
}

/*
d_net_url_literal_kind
  What a bracketed host holds: IPvFuture, IPv6 with an optional RFC 6874
zone ("%25" and one or more characters), or nothing valid.
*/
static enum d_net_url_host
d_net_url_literal_kind(
    struct d_pack_text _inner
)
{
    // IPvFuture begins with its version marker
    if ( (_inner.length > 0u) &&
         ( (_inner.data[0] == 'v') ||
           (_inner.data[0] == 'V') ) )
    {
        return (d_net_url_is_future(_inner)) ? D_NET_URL_HOST_IPVFUTURE
                                              : D_NET_URL_HOST_NONE;
    }

    const size_t zone   = d_net_url_find_char(_inner,
                                              0u,
                                              _inner.length,
                                              '%');
    size_t       unused = 0u;

    // no zone: the whole literal is the address
    if (zone == _inner.length)
    {
        return (d_net_url_is_ipv6(_inner)) ? D_NET_URL_HOST_IPV6
                                           : D_NET_URL_HOST_NONE;
    }

    const bool zoned = ( (zone + 3u < _inner.length) &&
                         (_inner.data[zone + 1u] == '2') &&
                         (_inner.data[zone + 2u] == '5') &&
                         (d_net_url_is_ipv6(d_net_url_view(_inner,
                                                           0u,
                                                           zone))) &&
                         (d_net_url_scan(d_net_url_view(_inner,
                                                        zone + 3u,
                                                        _inner.length),
                                         0u,
                                         d_net_url_allows_zone,
                                         D_NET_URL_ERROR_IP_LITERAL,
                                         &unused) == D_NET_URL_OK) );

    return (zoned) ? D_NET_URL_HOST_IPV6
                   : D_NET_URL_HOST_NONE;
}

/*
d_net_url_take_scheme
  A scheme is everything before the first ':' that comes before any '/',
'?', or '#', when that is a letter followed by letters, digits, '+', '-',
and '.'. A colon there ending no valid scheme is an error: a relative
reference's first segment may not hold one (RFC 3986, 4.2).
*/
static enum d_net_url_error
d_net_url_take_scheme(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t*            _pos,
    size_t*            _error_at
)
{
    const size_t colon = d_net_url_find(_text,
                                        0u,
                                        ":/?#");

    // no colon before the first delimiter: a relative reference
    if ( (colon == _text.length) ||
         (_text.data[colon] != ':') )
    {
        return D_NET_URL_OK;
    }

    // each character of the candidate scheme; an empty one fails at 0
    for (size_t i = 0u; i < colon; ++i)
    {
        const char c = _text.data[i];

        // a letter first, then letters, digits, '+', '-', '.'
        if ( ( (i == 0u) &&
               (!d_net_url_is_alpha(c)) ) ||
             ( (!d_net_url_is_alpha(c)) &&
               (!d_net_url_is_digit(c)) &&
               (c != '+')               &&
               (c != '-')               &&
               (c != '.') ) )
        {
            *_error_at = i;

            return D_NET_URL_ERROR_SCHEME;
        }
    }

    _out->scheme = d_net_url_view(_text,
                                  0u,
                                  colon);
    *_pos = colon + 1u;

    return (colon == 0u) ? D_NET_URL_ERROR_SCHEME
                         : D_NET_URL_OK;
}

/*
d_net_url_take_port
  port = *DIGIT, here also bounded by 65535. Leading zeros are allowed.
*/
static enum d_net_url_error
d_net_url_take_port(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t             _colon,
    size_t             _end,
    size_t*            _error_at
)
{
    unsigned long value = 0u;

    // no ':' after the host, no port
    if (_colon >= _end)
    {
        return D_NET_URL_OK;
    }

    // each digit, the value kept within a port's range
    for (size_t i = _colon + 1u; i < _end; ++i)
    {
        // a port is digits only
        if (!d_net_url_is_digit(_text.data[i]))
        {
            *_error_at = i;

            return D_NET_URL_ERROR_PORT;
        }

        value = (value * 10u) + (unsigned long)(_text.data[i] - '0');

        // and at most 65535, reported at its first digit
        if (value > 65535u)
        {
            *_error_at = _colon + 1u;

            return D_NET_URL_ERROR_PORT;
        }
    }

    _out->has_port  = true;
    _out->port_text = d_net_url_view(_text,
                                     _colon + 1u,
                                     _end);
    _out->port      = (d_net_port)value;

    return D_NET_URL_OK;
}

/*
d_net_url_take_literal
  An IP literal: a bracketed IPv6 or IPvFuture address, which only a port
may follow.
*/
static enum d_net_url_error
d_net_url_take_literal(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t             _start,
    size_t             _end,
    size_t*            _error_at
)
{
    const size_t close = d_net_url_find_char(_text,
                                             _start,
                                             _end,
                                             ']');

    // an unclosed bracket
    if (close == _end)
    {
        *_error_at = _start;

        return D_NET_URL_ERROR_IP_LITERAL;
    }

    const struct d_pack_text  inner = d_net_url_view(_text,
                                                     _start + 1u,
                                                     close);
    const enum d_net_url_host kind  = d_net_url_literal_kind(inner);

    // not an address, or something other than a port after the bracket
    if ( (kind == D_NET_URL_HOST_NONE) ||
         ( (close + 1u < _end) &&
           (_text.data[close + 1u] != ':') ) )
    {
        *_error_at = (kind == D_NET_URL_HOST_NONE) ? _start + 1u
                                                   : close + 1u;

        return D_NET_URL_ERROR_IP_LITERAL;
    }

    _out->host      = inner;
    _out->host_kind = kind;

    return d_net_url_take_port(_text,
                               _out,
                               close + 1u,
                               _end,
                               _error_at);
}

/*
d_net_url_take_host
  host = IP-literal / IPv4address / reg-name, and then the port. A name
ends at the first ':', which no name may hold.
*/
static enum d_net_url_error
d_net_url_take_host(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t             _start,
    size_t             _end,
    size_t*            _error_at
)
{
    // a bracketed literal
    if ( (_start < _end) &&
         (_text.data[_start] == '[') )
    {
        return d_net_url_take_literal(_text,
                                      _out,
                                      _start,
                                      _end,
                                      _error_at);
    }

    const size_t               colon = d_net_url_find_char(_text,
                                                           _start,
                                                           _end,
                                                           ':');
    const struct d_pack_text   name  = d_net_url_view(_text,
                                                      _start,
                                                      colon);
    const enum d_net_url_error found = d_net_url_scan(name,
                                                      _start,
                                                      d_net_url_allows_name,
                                                      D_NET_URL_ERROR_HOST,
                                                      _error_at);

    // a character no name may hold
    if (found != D_NET_URL_OK)
    {
        return found;
    }

    _out->host      = name;
    _out->host_kind = (d_net_url_is_ipv4(name)) ? D_NET_URL_HOST_IPV4
                                                : D_NET_URL_HOST_NAME;

    return d_net_url_take_port(_text,
                               _out,
                               colon,
                               _end,
                               _error_at);
}

/*
d_net_url_take_authority_body
  authority = [ userinfo "@" ] host [ ":" port ], over [_first, _end): the
part of a URL after "//", or an authority standing alone. No part of it
may hold a second '@'.
*/
static enum d_net_url_error
d_net_url_take_authority_body(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t             _first,
    size_t             _end,
    size_t*            _error_at
)
{
    const size_t first = _first;
    const size_t end   = _end;
    const size_t at    = d_net_url_find_char(_text,
                                             first,
                                             end,
                                             '@');

    _out->has_authority = true;
    _out->host_kind     = D_NET_URL_HOST_NAME;

    // userinfo, before the '@'
    if (at < end)
    {
        const enum d_net_url_error user = d_net_url_scan(
                                              d_net_url_view(_text,
                                                             first,
                                                             at),
                                              first,
                                              d_net_url_allows_userinfo,
                                              D_NET_URL_ERROR_USERINFO,
                                              _error_at);

        // a character userinfo may not hold
        if (user != D_NET_URL_OK)
        {
            return user;
        }

        _out->has_userinfo = true;
        _out->userinfo     = d_net_url_view(_text,
                                            first,
                                            at);
    }

    return d_net_url_take_host(_text,
                               _out,
                               (at < end) ? at + 1u
                                          : first,
                               end,
                               _error_at);
}

/*
d_net_url_take_authority
  An authority follows "//", up to the next '/', '?', or '#'.
*/
static enum d_net_url_error
d_net_url_take_authority(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t*            _pos,
    size_t*            _error_at
)
{
    const size_t start = *_pos;

    // no "//", no authority
    if ( (_text.length - start < 2u)      ||
         (_text.data[start] != '/')       ||
         (_text.data[start + 1u] != '/') )
    {
        return D_NET_URL_OK;
    }

    const size_t end = d_net_url_find(_text,
                                      start + 2u,
                                      "/?#");

    *_pos = end;

    return d_net_url_take_authority_body(_text,
                                         _out,
                                         start + 2u,
                                         end,
                                         _error_at);
}

/*
d_net_url_take_rest
  The path, up to a '?' or '#'; the query, up to a '#'; the fragment, to
the end. With an authority the path is empty or begins with '/', which the
authority's own end guarantees.
*/
static enum d_net_url_error
d_net_url_take_rest(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t             _pos,
    size_t*            _error_at
)
{
    const size_t         path_end = d_net_url_find(_text,
                                                   _pos,
                                                   "?#");
    enum d_net_url_error result   = d_net_url_scan(
                                        d_net_url_view(_text,
                                                       _pos,
                                                       path_end),
                                        _pos,
                                        d_net_url_allows_path,
                                        D_NET_URL_ERROR_PATH,
                                        _error_at);
    size_t               next     = path_end;

    _out->path = d_net_url_view(_text,
                                _pos,
                                path_end);

    // a query, up to any '#'
    if ( (result == D_NET_URL_OK) &&
         (next < _text.length)    &&
         (_text.data[next] == '?') )
    {
        next            = d_net_url_find(_text,
                                         next + 1u,
                                         "#");
        _out->has_query = true;
        _out->query     = d_net_url_view(_text,
                                         path_end + 1u,
                                         next);
        result          = d_net_url_scan(_out->query,
                                         path_end + 1u,
                                         d_net_url_allows_query,
                                         D_NET_URL_ERROR_QUERY,
                                         _error_at);
    }

    // a fragment, to the end
    if ( (result == D_NET_URL_OK) &&
         (next < _text.length) )
    {
        _out->has_fragment = true;
        _out->fragment     = d_net_url_view(_text,
                                            next + 1u,
                                            _text.length);
        result             = d_net_url_scan(_out->fragment,
                                            next + 1u,
                                            d_net_url_allows_query,
                                            D_NET_URL_ERROR_FRAGMENT,
                                            _error_at);
    }

    return result;
}

/*
d_net_url_parse
  Scheme, authority, then path, query, and fragment, each validated as it
is taken. A refusal zeroes the whole result.
*/
enum d_net_url_error
d_net_url_parse(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t*            _error_at
)
{
    // parameter validation
    if ( (!_out) ||
         ( (!_text.data) &&
           (_text.length != 0u) ) )
    {
        return D_NET_URL_ERROR_ARGUMENT;
    }

    size_t                   ignored = 0u;
    size_t*                  at      = (_error_at) ? _error_at
                                                   : &ignored;
    const struct d_pack_text text    = { (_text.data) ? _text.data
                                                      : "",
                                         _text.length };
    size_t                   pos     = 0u;

    memset(_out,
           0,
           sizeof(*_out));
    _out->text = text;
    *at        = 0u;

    enum d_net_url_error result = d_net_url_take_scheme(text,
                                                        _out,
                                                        &pos,
                                                        at);

    // the authority, then everything after it
    if (result == D_NET_URL_OK)
    {
        result = d_net_url_take_authority(text,
                                          _out,
                                          &pos,
                                          at);
    }

    // the path, query, and fragment
    if (result == D_NET_URL_OK)
    {
        result = d_net_url_take_rest(text,
                                     _out,
                                     pos,
                                     at);
    }

    // a refused reference leaves nothing half-parsed
    if (result != D_NET_URL_OK)
    {
        memset(_out,
               0,
               sizeof(*_out));
    }

    return result;
}

/*
d_net_url_parse_authority
  The whole text as an authority's body; a '/', '?', or '#' in it is refused
by the host's grammar, as it would be after "//".
*/
enum d_net_url_error
d_net_url_parse_authority(
    struct d_pack_text _text,
    struct d_net_url*  _out,
    size_t*            _error_at
)
{
    // parameter validation
    if ( (!_out) ||
         ( (!_text.data) &&
           (_text.length != 0u) ) )
    {
        return D_NET_URL_ERROR_ARGUMENT;
    }

    size_t                   ignored = 0u;
    size_t*                  at      = (_error_at) ? _error_at
                                                   : &ignored;
    const struct d_pack_text text    = { (_text.data) ? _text.data
                                                      : "",
                                         _text.length };

    memset(_out,
           0,
           sizeof(*_out));
    _out->text = text;
    *at        = 0u;

    const enum d_net_url_error result = d_net_url_take_authority_body(
                                            text,
                                            _out,
                                            0u,
                                            text.length,
                                            at);

    // a refused authority leaves nothing half-parsed
    if (result != D_NET_URL_OK)
    {
        memset(_out,
               0,
               sizeof(*_out));
    }

    return result;
}

/*
d_net_url_is_absolute
  A scheme is what makes a reference absolute.
*/
bool
d_net_url_is_absolute(
    const struct d_net_url* _url
)
{
    return ( (_url) &&
             (_url->scheme.length > 0u) );
}

/*
d_net_url_format
  RFC 3986's recomposition (5.3), keeping every present component, even an
empty one.
*/
enum d_net_url_error
d_net_url_format(
    const struct d_net_url* _url,
    char*                   _buffer,
    size_t                  _capacity,
    size_t*                 _length
)
{
    // parameter validation
    if ( (!_url)    ||
         (!_length) ||
         ( (!_buffer) &&
           (_capacity != 0u) ) )
    {
        return D_NET_URL_ERROR_ARGUMENT;
    }

    struct d_net_url_writer out = { _buffer,
                                    _capacity,
                                    0u };

    // the scheme and its ':'
    if (_url->scheme.length > 0u)
    {
        d_net_url_put(&out,
                      _url->scheme);
        d_net_url_put_char(&out,
                           ':');
    }

    // the authority
    if (_url->has_authority)
    {
        d_net_url_put_authority(&out,
                                _url);
    }

    d_net_url_put(&out,
                  _url->path);
    d_net_url_put_tail(&out,
                       _url,
                       _url);

    return d_net_url_finish(&out,
                            _length);
}
