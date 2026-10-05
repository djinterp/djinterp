/*******************************************************************************
* djinterp [net]                                              net_url_internal.h
*
* Private helpers shared by the net_url sources.
*   RFC 3986's character classes, text views, and a writer that counts every
* byte, written or not. Static inline, since the parser calls the classes per
* character; not installed, and not part of the public interface.
*
*
* path:      /src/djinterp/net/net_url_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CHARACTER CLASSES
    -----------------
    1.  Letters, digits, and case
    2.  RFC 3986 classes
    3.  Component classes
2.  TEXT
    ----
    1.  Views
3.  OUTPUT
    ------
    1.  The writer
    2.  Component writers
4.  SCHEMES
    -------
    1.  Web schemes
*/

#ifndef DJINTERP_NET_NET_URL_INTERNAL_H
#define DJINTERP_NET_NET_URL_INTERNAL_H 1

// std
#include <stddef.h>  // size_t
#include <string.h>  // memcpy
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text
#include "../../../inc/djinterp/net/net_url.h"         // d_net_url


//==============================================================================
// 1.  CHARACTER CLASSES
//==============================================================================
// ASCII only, never locale-dependent, so a URL parses the same everywhere.


// 1.1    Letters, digits, and case
//------------------------------------------------------------------------------
// d_net_url_is_alpha
//   function: ASCII letters, never locale letters.
static inline bool
d_net_url_is_alpha(
    char _c
)
{
    return ( ( (_c >= 'a') && (_c <= 'z') ) ||
             ( (_c >= 'A') && (_c <= 'Z') ) );
}

// d_net_url_lower
//   function: ASCII lowercase, never locale case.
static inline char
d_net_url_lower(
    char _c
)
{
    return ( (_c >= 'A') &&
             (_c <= 'Z') ) ? (char)(_c + ('a' - 'A'))
                           : _c;
}

// d_net_url_is_digit
//   function: ASCII digits.
static inline bool
d_net_url_is_digit(
    char _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

// d_net_url_hex_value
//   function: a hex digit's value, or -1.
static inline int
d_net_url_hex_value(
    char _c
)
{
    // each range of digits in turn
    if (d_net_url_is_digit(_c))
    {
        return _c - '0';
    }

    // lowercase letters
    if ( (_c >= 'a') && (_c <= 'f') )
    {
        return _c - 'a' + 10;
    }

    return ( (_c >= 'A') && (_c <= 'F') ) ? (_c - 'A' + 10)
                                          : -1;
}

// d_net_url_is_hex
//   function: whether a character is a hex digit.
static inline bool
d_net_url_is_hex(
    char _c
)
{
    return (d_net_url_hex_value(_c) >= 0);
}

// 1.2    RFC 3986 classes
//------------------------------------------------------------------------------
// d_net_url_is_unreserved
//   function: unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
static inline bool
d_net_url_is_unreserved(
    char _c
)
{
    return ( (d_net_url_is_alpha(_c)) ||
             (d_net_url_is_digit(_c)) ||
             (_c == '-')              ||
             (_c == '.')              ||
             (_c == '_')              ||
             (_c == '~') );
}

// d_net_url_is_sub_delim
//   function: sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / ","
// / ";" / "="
static inline bool
d_net_url_is_sub_delim(
    char _c
)
{
    switch (_c)
    {
        case '!':
        case '$':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case '+':
        case ',':
        case ';':
        case '=':
            return true;
        default:
            return false;
    }
}

// d_net_url_is_pchar
//   function: pchar, without its pct-encoded alternative: unreserved /
// sub-delims / ":" / "@"
static inline bool
d_net_url_is_pchar(
    char _c
)
{
    return ( (d_net_url_is_unreserved(_c)) ||
             (d_net_url_is_sub_delim(_c))  ||
             (_c == ':')                   ||
             (_c == '@') );
}

// 1.3    Component classes
//------------------------------------------------------------------------------
// d_net_url_allows
//   type: a character class. Answering true for '%' permits percent
// escapes in the component, whose two hex digits the scanner checks.
typedef bool (*d_net_url_allows)(char _c);

// d_net_url_allows_userinfo
//   function: userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
static inline bool
d_net_url_allows_userinfo(
    char _c
)
{
    return ( (d_net_url_is_unreserved(_c)) ||
             (d_net_url_is_sub_delim(_c))  ||
             (_c == ':')                   ||
             (_c == '%') );
}

// d_net_url_allows_name
//   function: reg-name = *( unreserved / pct-encoded / sub-delims )
static inline bool
d_net_url_allows_name(
    char _c
)
{
    return ( (d_net_url_is_unreserved(_c)) ||
             (d_net_url_is_sub_delim(_c))  ||
             (_c == '%') );
}

// d_net_url_allows_path
//   function: a path's characters: pchar and "/".
static inline bool
d_net_url_allows_path(
    char _c
)
{
    return ( (d_net_url_is_pchar(_c)) ||
             (_c == '/')              ||
             (_c == '%') );
}

// d_net_url_allows_query
//   function: query = fragment = *( pchar / "/" / "?" )
static inline bool
d_net_url_allows_query(
    char _c
)
{
    return ( (d_net_url_allows_path(_c)) ||
             (_c == '?') );
}

// d_net_url_allows_zone
//   function: ZoneID = 1*( unreserved / pct-encoded ), RFC 6874.
static inline bool
d_net_url_allows_zone(
    char _c
)
{
    return ( (d_net_url_is_unreserved(_c)) ||
             (_c == '%') );
}

// d_net_url_allows_future
//   function: IPvFuture's tail: unreserved / sub-delims / ":", with no escapes.
static inline bool
d_net_url_allows_future(
    char _c
)
{
    return ( (d_net_url_is_unreserved(_c)) ||
             (d_net_url_is_sub_delim(_c))  ||
             (_c == ':') );
}


//==============================================================================
// 2.  TEXT
//==============================================================================
// Views into a parsed text, which nothing here copies.


// 2.1    Views
//------------------------------------------------------------------------------
// d_net_url_view
//   function: the part of a text between two offsets.
static inline struct d_pack_text
d_net_url_view(
    struct d_pack_text _text,
    size_t             _start,
    size_t             _end
)
{
    const struct d_pack_text part = { _text.data + _start,
                                      _end - _start };

    return part;
}

// d_net_url_find_char
//   function: the offset of a character in [_start, _end), or _end.
static inline size_t
d_net_url_find_char(
    struct d_pack_text _text,
    size_t             _start,
    size_t             _end,
    char               _c
)
{
    // each character in the range
    for (size_t i = _start; i < _end; ++i)
    {
        // the first match ends the search
        if (_text.data[i] == _c)
        {
            return i;
        }
    }

    return _end;
}


//==============================================================================
// 3.  OUTPUT
//==============================================================================
// Text produced into a caller's buffer.


// 3.1    The writer
//------------------------------------------------------------------------------
// d_net_url_writer
//   struct: output into a caller's buffer that counts every byte, written or
// not. A piece is written only if it and everything before it fit with room
// for the terminator, so the buffer never holds a gap.
struct d_net_url_writer
{
    char*  buffer;
    size_t capacity;
    size_t length;
};

// d_net_url_put
//   function: appends a piece, if it and everything before it fit with the
// terminator.
static inline void
d_net_url_put(
    struct d_net_url_writer* _writer,
    struct d_pack_text       _piece
)
{
    // write only while everything so far has fit
    if ( (_writer->buffer)                                  &&
         (_piece.length > 0u)                               &&
         (_writer->length + _piece.length < _writer->capacity) )
    {
        memcpy(_writer->buffer + _writer->length,
               _piece.data,
               _piece.length);
    }

    _writer->length += _piece.length;

    return;
}

// d_net_url_put_char
//   function: appends one character.
static inline void
d_net_url_put_char(
    struct d_net_url_writer* _writer,
    char                     _c
)
{
    const struct d_pack_text piece = { &_c,
                                       1u };

    d_net_url_put(_writer,
                  piece);

    return;
}

// d_net_url_put_escape
//   function: appends a byte as "%XX", uppercase.
static inline void
d_net_url_put_escape(
    struct d_net_url_writer* _writer,
    unsigned char            _byte
)
{
    const char* const DIGITS = "0123456789ABCDEF";

    d_net_url_put_char(_writer,
                       '%');
    d_net_url_put_char(_writer,
                       DIGITS[_byte >> 4u]);
    d_net_url_put_char(_writer,
                       DIGITS[_byte & 0x0Fu]);

    return;
}

// d_net_url_finish
//   function: reports the length, and terminates the text when all of it fit.
static inline enum d_net_url_error
d_net_url_finish(
    struct d_net_url_writer* _writer,
    size_t*                  _length
)
{
    *_length = _writer->length;

    // the text and its terminator fit
    if ( (_writer->buffer) &&
         (_writer->length < _writer->capacity) )
    {
        _writer->buffer[_writer->length] = '\0';

        return D_NET_URL_OK;
    }

    return D_NET_URL_ERROR_BUFFER;
}

// 3.2    Component writers
//------------------------------------------------------------------------------
// d_net_url_put_bracket
//   function: appends a bracket when the host is an IP literal, which RFC
// 3986 writes bracketed and d_net_url holds without.
static inline void
d_net_url_put_bracket(
    struct d_net_url_writer* _writer,
    const struct d_net_url*  _url,
    char                     _bracket
)
{
    // only IP literals are bracketed
    if ( (_url->host_kind == D_NET_URL_HOST_IPV6) ||
         (_url->host_kind == D_NET_URL_HOST_IPVFUTURE) )
    {
        d_net_url_put_char(_writer,
                           _bracket);
    }

    return;
}

// d_net_url_put_authority
//   function: "//", userinfo and '@', the host, and ':' and the port text, each
// as written.
static inline void
d_net_url_put_authority(
    struct d_net_url_writer* _writer,
    const struct d_net_url*  _url
)
{
    d_net_url_put_char(_writer,
                       '/');
    d_net_url_put_char(_writer,
                       '/');

    // userinfo, when present, even empty
    if (_url->has_userinfo)
    {
        d_net_url_put(_writer,
                      _url->userinfo);
        d_net_url_put_char(_writer,
                           '@');
    }

    d_net_url_put_bracket(_writer,
                          _url,
                          '[');
    d_net_url_put(_writer,
                  _url->host);
    d_net_url_put_bracket(_writer,
                          _url,
                          ']');

    // a port, when present, even empty
    if (_url->has_port)
    {
        d_net_url_put_char(_writer,
                           ':');
        d_net_url_put(_writer,
                      _url->port_text);
    }

    return;
}

// d_net_url_put_tail
//   function: '?' and the query, '#' and the fragment, each when present.
static inline void
d_net_url_put_tail(
    struct d_net_url_writer* _writer,
    const struct d_net_url*  _query,
    const struct d_net_url*  _fragment
)
{
    // the query's source may differ from the fragment's in resolution
    if (_query->has_query)
    {
        d_net_url_put_char(_writer,
                           '?');
        d_net_url_put(_writer,
                      _query->query);
    }

    // the fragment
    if (_fragment->has_fragment)
    {
        d_net_url_put_char(_writer,
                           '#');
        d_net_url_put(_writer,
                      _fragment->fragment);
    }

    return;
}


//==============================================================================
// 4.  SCHEMES
//==============================================================================
// Defined in net_url_scheme.c, beside the table it reads.


// 4.1    Web schemes
//------------------------------------------------------------------------------
// d_net_url_internal_is_web_scheme -- whether a scheme's empty path is "/"
// (RFC 3986, 6.2.3; RFC 9110, 4.2): http, https, ws, and wss
bool d_net_url_internal_is_web_scheme(struct d_pack_text _scheme);


#endif  // DJINTERP_NET_NET_URL_INTERNAL_H
