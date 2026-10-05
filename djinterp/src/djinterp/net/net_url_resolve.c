/*******************************************************************************
* djinterp [net]                                               net_url_resolve.c
*
* URLs resolved and normalized: net_url.h's section 3.
*   Resolution is RFC 3986's strict algorithm (5.2): the target's components
* come from the reference or the base, the paths merge, and dot segments go.
* Normalization (6.2) fixes case, escapes, dot segments, and the scheme-based
* rules for ports and empty paths. Both remove dot segments in place, in the
* caller's buffer, by the table of the RFC's steps below.
*
*
* path:      /src/djinterp/net/net_url_resolve.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../inc/djinterp/net/net_url.h"  // corresponding header
// std
#include <string.h>  // memcmp, memmove
// djinterp
#include "../../../inc/djinterp/net/net.h"  // d_net_port_format, D_NET_PORT_TEXT_MAX
#include "./net_url_internal.h"             // shared helpers


// d_net_url_dot_rule
//   struct: one of remove_dot_segments' input patterns (RFC 3986, 5.2.4):
// its text, whether it must be all the input left, how many of its trailing
// bytes remain as a '/', and whether it drops the last output segment.
struct d_net_url_dot_rule
{
    const char* text;
    size_t      length;
    bool        whole;
    size_t      keep;
    bool        pop;
};

// DOT_RULES
//   constant: steps A to D of remove_dot_segments, in the RFC's order;
// anything else is step E, which moves a segment to the output.
static const struct d_net_url_dot_rule DOT_RULES[] =
{
    { "../",  3u, false, 0u, false },
    { "./",   2u, false, 0u, false },
    { "/./",  3u, false, 1u, false },
    { "/.",   2u, true,  1u, false },
    { "/../", 4u, false, 1u, true  },
    { "/..",  3u, true,  1u, true  },
    { ".",    1u, true,  0u, false },
    { "..",   2u, true,  0u, false }
};

/*
d_net_url_last_slash
  Where the output shrinks to when step C drops its last segment: the
position of the output's last '/', or its start.
*/
static size_t
d_net_url_last_slash(
    const char* _path,
    size_t      _out
)
{
    size_t i = _out;

    // back to just after the last '/'
    while ( (i > 0u) &&
            (_path[i - 1u] != '/') )
    {
        i -= 1u;
    }

    return (i > 0u) ? i - 1u
                    : 0u;
}

/*
d_net_url_dot_rule_at
  The first of DOT_RULES matching the input at _in, or NULL for step E.
*/
static const struct d_net_url_dot_rule*
d_net_url_dot_rule_at(
    const char* _path,
    size_t      _in,
    size_t      _end
)
{
    const size_t left = _end - _in;

    // each rule in the RFC's order
    for (size_t r = 0u; r < sizeof(DOT_RULES) / sizeof(DOT_RULES[0]); ++r)
    {
        const struct d_net_url_dot_rule* rule = &DOT_RULES[r];

        // a whole-input rule needs exactly its text; the others, a prefix
        if ( ( (rule->whole) ? (left == rule->length)
                             : (left >= rule->length) ) &&
             (memcmp(_path + _in,
                     rule->text,
                     rule->length) == 0) )
        {
            return rule;
        }
    }

    return NULL;
}

/*
d_net_url_remove_dots
  RFC 3986's remove_dot_segments (5.2.4), in place: input is read at _in and
output written at out, and out never passes _in, since every step either
consumes input or moves a segment forward. A rule keeping a '/' rewrites
the byte it leaves as the next input so. Returns the new length.
*/
static size_t
d_net_url_remove_dots(
    char*  _path,
    size_t _length
)
{
    size_t in  = 0u;
    size_t out = 0u;

    // until the input is used up
    while (in < _length)
    {
        const struct d_net_url_dot_rule* rule = d_net_url_dot_rule_at(_path,
                                                                      in,
                                                                      _length);

        // steps A to D: consume the pattern, perhaps leaving a '/'
        if (rule)
        {
            in += rule->length - rule->keep;

            // the '/' left is the next input
            if (rule->keep > 0u)
            {
                _path[in] = '/';
            }

            out = (rule->pop) ? d_net_url_last_slash(_path,
                                                     out)
                              : out;

            continue;
        }

        size_t end = in + ( (_path[in] == '/') ? 1u
                                               : 0u );

        // step E: the first segment, with its leading '/', moves to output
        while ( (end < _length) &&
                (_path[end] != '/') )
        {
            end += 1u;
        }

        memmove(_path + out,
                _path + in,
                end - in);
        out += end - in;
        in   = end;
    }

    return out;
}

/*
d_net_url_drop_dots
  Removes dot segments from the path written since _start, when everything
written so far fit; otherwise the length stays an upper bound.
*/
static void
d_net_url_drop_dots(
    struct d_net_url_writer* _writer,
    size_t                   _start
)
{
    // only a path that is really in the buffer can be rewritten
    if ( (_writer->buffer) &&
         (_writer->length < _writer->capacity) )
    {
        _writer->length = _start + d_net_url_remove_dots(
                                       _writer->buffer + _start,
                                       _writer->length - _start);
    }

    return;
}

/*
d_net_url_merge
  RFC 3986's merge (5.2.3): the base path up to and including its last
'/', or "/" when the base has an authority and no path, then the
reference's path.
*/
static void
d_net_url_merge(
    struct d_net_url_writer* _writer,
    const struct d_net_url*  _base,
    const struct d_net_url*  _reference
)
{
    size_t keep = _base->path.length;

    // back to just after the base path's last '/'
    while ( (keep > 0u) &&
            (_base->path.data[keep - 1u] != '/') )
    {
        keep -= 1u;
    }

    // an authority with no path merges as "/"
    if ( (_base->has_authority) &&
         (_base->path.length == 0u) )
    {
        d_net_url_put_char(_writer,
                           '/');
    }
    else
    {
        d_net_url_put(_writer,
                      d_net_url_view(_base->path,
                                     0u,
                                     keep));
    }

    d_net_url_put(_writer,
                  _reference->path);

    return;
}

/*
d_net_url_resolve_path
  The target's path (5.2.2): the reference's own, the base's untouched when
the reference has none, or the two merged; all but the base's untouched
path lose their dot segments.
*/
static void
d_net_url_resolve_path(
    struct d_net_url_writer* _writer,
    const struct d_net_url*  _base,
    const struct d_net_url*  _reference,
    bool                     _own_authority
)
{
    const size_t start    = _writer->length;
    const bool   absolute = ( (_reference->path.length > 0u) &&
                              (_reference->path.data[0] == '/') );

    // no path and no authority of its own: the base's path, as it is
    if ( (!_own_authority) &&
         (_reference->path.length == 0u) )
    {
        d_net_url_put(_writer,
                      _base->path);

        return;
    }

    // the reference's path stands alone, or merges with the base's
    if ( (_own_authority) ||
         (absolute) )
    {
        d_net_url_put(_writer,
                      _reference->path);
    }
    else
    {
        d_net_url_merge(_writer,
                        _base,
                        _reference);
    }

    d_net_url_drop_dots(_writer,
                        start);

    return;
}

/*
d_net_url_resolve
  RFC 3986's strict resolution (5.2.2): the reference's scheme and authority
when it has them, the base's otherwise; the query from the reference,
unless it has neither path nor query nor authority of its own; the fragment
from the reference, always.
*/
enum d_net_url_error
d_net_url_resolve(
    const struct d_net_url* _base,
    const struct d_net_url* _reference,
    char*                   _buffer,
    size_t                  _capacity,
    size_t*                 _length
)
{
    // parameter validation
    if ( (!_base)      ||
         (!_reference) ||
         (!_length)    ||
         ( (!_buffer) &&
           (_capacity != 0u) ) )
    {
        return D_NET_URL_ERROR_ARGUMENT;
    }

    // only an absolute base can be resolved against
    if (!d_net_url_is_absolute(_base))
    {
        *_length = 0u;

        return D_NET_URL_ERROR_BASE;
    }

    struct d_net_url_writer out           = { _buffer,
                                              _capacity,
                                              0u };
    const bool              own_scheme    = (_reference->scheme.length > 0u);
    const bool              own_authority = ( (own_scheme) ||
                                              (_reference->has_authority) );
    const struct d_net_url* authority     = (own_authority) ? _reference
                                                            : _base;
    const bool              own_query     = ( (own_authority)                 ||
                                              (_reference->path.length > 0u) ||
                                              (_reference->has_query) );

    d_net_url_put(&out,
                  (own_scheme) ? _reference->scheme
                               : _base->scheme);
    d_net_url_put_char(&out,
                       ':');

    // the chosen authority, when it exists
    if (authority->has_authority)
    {
        d_net_url_put_authority(&out,
                                authority);
    }

    d_net_url_resolve_path(&out,
                           _base,
                           _reference,
                           own_authority);
    d_net_url_put_tail(&out,
                       (own_query) ? _reference
                                   : _base,
                       _reference);

    return d_net_url_finish(&out,
                            _length);
}

/*
d_net_url_put_normal
  Appends a component with its escapes normalized (6.2.2.1 and 6.2.2.2):
an escaped unreserved character decoded, every other escape's hex
uppercased; letters are lowercased when _lower is set, decoded ones too.
*/
static void
d_net_url_put_normal(
    struct d_net_url_writer* _writer,
    struct d_pack_text       _text,
    bool                     _lower
)
{
    // each character, an escape taken whole
    for (size_t i = 0u; i < _text.length; ++i)
    {
        char c = _text.data[i];

        // a well-formed escape: decoded if unreserved, else re-cased
        if ( (c == '%')                            &&
             (i + 2u < _text.length)               &&
             (d_net_url_is_hex(_text.data[i + 1u])) &&
             (d_net_url_is_hex(_text.data[i + 2u])) )
        {
            const int value = (d_net_url_hex_value(_text.data[i + 1u]) * 16) +
                              d_net_url_hex_value(_text.data[i + 2u]);

            i += 2u;
            c  = (char)value;

            // an escaped reserved or other byte stays escaped
            if (!d_net_url_is_unreserved(c))
            {
                d_net_url_put_escape(_writer,
                                     (unsigned char)value);

                continue;
            }
        }

        d_net_url_put_char(_writer,
                           (_lower) ? d_net_url_lower(c)
                                    : c);
    }

    return;
}

/*
d_net_url_put_normal_host
  A host lowercased and its escapes normalized; an IPv6 zone keeps its
case, since interface names can depend on it, and IPvFuture text is left
as written.
*/
static void
d_net_url_put_normal_host(
    struct d_net_url_writer* _writer,
    const struct d_net_url*  _url
)
{
    const size_t zone = (_url->host_kind == D_NET_URL_HOST_IPV6)
                            ? d_net_url_find_char(_url->host,
                                                  0u,
                                                  _url->host.length,
                                                  '%')
                            : _url->host.length;

    // IPvFuture's text has no defined case
    if (_url->host_kind == D_NET_URL_HOST_IPVFUTURE)
    {
        d_net_url_put(_writer,
                      _url->host);

        return;
    }

    d_net_url_put_normal(_writer,
                         d_net_url_view(_url->host,
                                        0u,
                                        zone),
                         true);
    d_net_url_put(_writer,
                  d_net_url_view(_url->host,
                                 zone,
                                 _url->host.length));

    return;
}

/*
d_net_url_put_normal_authority
  The authority normalized: userinfo's escapes, the host lowercased, and a
port that is empty or the scheme's default dropped (6.2.3), leading zeros
with it.
*/
static void
d_net_url_put_normal_authority(
    struct d_net_url_writer* _writer,
    const struct d_net_url*  _url
)
{
    const d_net_port implied = d_net_url_default_port(_url->scheme);

    // the port text, in canonical decimal
    char port[D_NET_PORT_TEXT_MAX + 1] = { 0 };

    d_net_url_put_char(_writer,
                       '/');
    d_net_url_put_char(_writer,
                       '/');

    // userinfo keeps its case
    if (_url->has_userinfo)
    {
        d_net_url_put_normal(_writer,
                             _url->userinfo,
                             false);
        d_net_url_put_char(_writer,
                           '@');
    }

    d_net_url_put_bracket(_writer,
                          _url,
                          '[');
    d_net_url_put_normal_host(_writer,
                              _url);
    d_net_url_put_bracket(_writer,
                          _url,
                          ']');

    // a port that says something
    if ( (_url->has_port)              &&
         (_url->port_text.length > 0u) &&
         ( (implied == 0u) ||
           (_url->port != implied) ) )
    {
        const struct d_pack_text text = { port,
                                          d_net_port_format(_url->port,
                                                            port,
                                                            sizeof(port)) };

        d_net_url_put_char(_writer,
                           ':');
        d_net_url_put(_writer,
                      text);
    }

    return;
}

/*
d_net_url_put_normal_tail
  '?' and the query, '#' and the fragment, each when present, their
escapes normalized.
*/
static void
d_net_url_put_normal_tail(
    struct d_net_url_writer* _writer,
    const struct d_net_url*  _url
)
{
    // the query
    if (_url->has_query)
    {
        d_net_url_put_char(_writer,
                           '?');
        d_net_url_put_normal(_writer,
                             _url->query,
                             false);
    }

    // the fragment
    if (_url->has_fragment)
    {
        d_net_url_put_char(_writer,
                           '#');
        d_net_url_put_normal(_writer,
                             _url->fragment,
                             false);
    }

    return;
}

/*
d_net_url_normalize
  Case, escapes, dot segments, and the scheme-based rules, component by
component; the path's dot segments go only from an absolute reference,
where they cannot climb above a root.
*/
enum d_net_url_error
d_net_url_normalize(
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

    // the scheme, lowercased, and its ':'
    if (_url->scheme.length > 0u)
    {
        d_net_url_put_normal(&out,
                             _url->scheme,
                             true);
        d_net_url_put_char(&out,
                           ':');
    }

    // the authority
    if (_url->has_authority)
    {
        d_net_url_put_normal_authority(&out,
                                       _url);
    }

    const size_t path = out.length;

    d_net_url_put_normal(&out,
                         _url->path,
                         false);

    // an empty web path is "/"
    if ( (_url->has_authority)       &&
         (_url->path.length == 0u)   &&
         (d_net_url_internal_is_web_scheme(_url->scheme)) )
    {
        d_net_url_put_char(&out,
                           '/');
    }

    // dot segments go only from an absolute reference
    if (d_net_url_is_absolute(_url))
    {
        d_net_url_drop_dots(&out,
                            path);
    }

    d_net_url_put_normal_tail(&out,
                              _url);

    return d_net_url_finish(&out,
                            _length);
}
