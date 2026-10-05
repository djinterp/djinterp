/*******************************************************************************
* djinterp [net]                                                         http1.c
*
* HTTP/1.1's message heads: http1.h's definitions.
*   Scanning and parsing are separate passes. The scan finds the head's end a
* line at a time, resuming from the parser's state, and enforces the line
* ending and the limits as each line completes, so a slow peer costs no
* rescanning. Once the head has ended, the parse reads it once, whole: the
* start line, each field line, the request-target's form, and the Host rule.
*
*
* path:      /src/djinterp/net/http/http1.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/http/http1.h"  // corresponding header
// std
#include <string.h>  // memchr
// djinterp
#include "../../../../inc/djinterp/config/net/http/cfg_http.h"  // limits
#include "../../../../inc/djinterp/net/net_url.h"               // targets, Host


/*
d_http1_view
  The bytes of a text between two offsets.
*/
static struct d_pack_text
d_http1_view(
    struct d_pack_text _text,
    size_t             _start,
    size_t             _end
)
{
    const struct d_pack_text view = { _text.data + _start,
                                      _end - _start };

    return view;
}

/*
d_http1_find
  The offset of a byte at or after _from, or the text's length.
*/
static size_t
d_http1_find(
    struct d_pack_text _text,
    size_t             _from,
    char               _c
)
{
    const char* const at = (_from < _text.length)
                               ? memchr(_text.data + _from,
                                        _c,
                                        _text.length - _from)
                               : NULL;

    return (at) ? (size_t)(at - _text.data)
                : _text.length;
}

/*
d_http1_too_long
  Which limit a start line past its own limit hit first. Each limit is an
event at a byte offset: the start line's when its bytes, CR included, reach
its limit plus two, at _line + start_line_max + 2; the head's at head_max + 1.
Judging by offset, not by where the bytes happened to be split, is what makes
every way of feeding a head reach the same verdict.
*/
static enum d_http_error
d_http1_too_long(
    const struct d_http1_parser* _parser,
    size_t                       _line
)
{
    return (_line + _parser->start_line_max + 2u <= _parser->head_max + 1u)
               ? D_HTTP_ERROR_START_LINE_TOO_LONG
               : D_HTTP_ERROR_HEAD_TOO_LARGE;
}

/*
d_http1_limits_hold
  With a line still open: INCOMPLETE while the start line, if that is the
open line, and the head so far are within their limits; otherwise the limit
whose offset came first.
*/
static enum d_http_error
d_http1_limits_hold(
    const struct d_http1_parser* _parser,
    size_t                       _open,
    size_t                       _total
)
{
    // the start line, still open, already past its limit
    if ( (!_parser->started) &&
         (_open > _parser->start_line_max + 1u) )
    {
        return d_http1_too_long(_parser,
                                _parser->line);
    }

    return (_total > _parser->head_max) ? D_HTTP_ERROR_HEAD_TOO_LARGE
                                        : D_HTTP_ERROR_INCOMPLETE;
}

/*
d_http1_scan_line
  One complete line, from the parser's line to the LF at _lf. Limits come
first, judged by the offsets at which they were passed, then the CR the LF
needs. Before the start line, an empty line is skipped when _skip allows,
and any other line is the start line; after it, an empty line ends the
head. INCOMPLETE means scanning goes on.
*/
static enum d_http_error
d_http1_scan_line(
    struct d_http1_parser* _parser,
    struct d_pack_text     _data,
    size_t                 _lf,
    bool                   _skip,
    size_t*                _end
)
{
    const size_t raw = _lf - _parser->line;

    // a start line past its limit before its LF arrived
    if ( (!_parser->started) &&
         (raw > _parser->start_line_max + 1u) )
    {
        return d_http1_too_long(_parser,
                                _parser->line);
    }

    // the head's limit passed before this line ended
    if (_lf > _parser->head_max)
    {
        return D_HTTP_ERROR_HEAD_TOO_LARGE;
    }

    // an LF without its CR
    if ( (raw == 0u) ||
         (_data.data[_lf - 1u] != '\r') )
    {
        return D_HTTP_ERROR_LINE_ENDING;
    }

    const size_t length = raw - 1u;

    // the empty line after the start line ends the head
    if ( (_parser->started) &&
         (length == 0u) )
    {
        *_end = _lf + 1u;

        return D_HTTP_OK;
    }

    // an empty line where the start line belongs
    if (length == 0u)
    {
        return (_skip) ? D_HTTP_ERROR_INCOMPLETE
                       : D_HTTP_ERROR_START_LINE;
    }

    // the first line with bytes is the start line, within its limit
    if (!_parser->started)
    {
        _parser->started = true;
        _parser->start   = _parser->line;
    }

    return D_HTTP_ERROR_INCOMPLETE;
}

/*
d_http1_scan
  Finds the head's end a line at a time, from where the last call stopped;
each complete line advances the parser, so no byte is scanned twice.
*/
static enum d_http_error
d_http1_scan(
    struct d_http1_parser* _parser,
    struct d_pack_text     _data,
    bool                   _skip,
    size_t*                _end
)
{
    // each complete line from where the scan stopped
    while (_parser->line < _data.length)
    {
        const size_t lf = d_http1_find(_data,
                                       _parser->line,
                                       '\n');

        // the line is still open
        if (lf == _data.length)
        {
            break;
        }

        const enum d_http_error verdict = d_http1_scan_line(_parser,
                                                            _data,
                                                            lf,
                                                            _skip,
                                                            _end);

        // the head ended, or a line was refused
        if (verdict != D_HTTP_ERROR_INCOMPLETE)
        {
            return ( (verdict == D_HTTP_OK) &&
                     (*_end > _parser->head_max) )
                       ? D_HTTP_ERROR_HEAD_TOO_LARGE
                       : verdict;
        }

        _parser->line = lf + 1u;
    }

    return d_http1_limits_hold(_parser,
                               _data.length - _parser->line,
                               _data.length);
}

/*
d_http1_next_line
  The line beginning at *_pos, without its CRLF, and *_pos moved past it.
Only lines the scan has seen end are read this way.
*/
static struct d_pack_text
d_http1_next_line(
    struct d_pack_text _data,
    size_t*            _pos
)
{
    const size_t             lf   = d_http1_find(_data,
                                                 *_pos,
                                                 '\n');
    const struct d_pack_text line = d_http1_view(_data,
                                                 *_pos,
                                                 lf - 1u);

    *_pos = lf + 1u;

    return line;
}

/*
d_http1_parse_field
  One field line: a token, a colon, and a value without the whitespace
around it. A line opening with whitespace is an obsolete fold, and a bare CR
anywhere is refused; whitespace before the colon fails the token.
*/
static enum d_http_error
d_http1_parse_field(
    struct d_pack_text    _line,
    struct d_http1_field* _field
)
{
    const size_t colon = d_http1_find(_line,
                                      0u,
                                      ':');

    // a folded line continues the one before it
    if ( (_line.data[0] == ' ') ||
         (_line.data[0] == '\t') )
    {
        return D_HTTP_ERROR_OBS_FOLD;
    }

    // a CR the line's end did not claim
    if (d_http1_find(_line,
                     0u,
                     '\r') < _line.length)
    {
        return D_HTTP_ERROR_LINE_ENDING;
    }

    _field->name  = d_http1_view(_line,
                                 0u,
                                 colon);
    _field->value = d_http_trim_whitespace(
                        d_http1_view(_line,
                                     (colon < _line.length) ? colon + 1u
                                                            : colon,
                                     _line.length));

    // no colon, or no token before it
    if ( (colon == _line.length) ||
         (!d_http_is_token(_field->name)) )
    {
        return D_HTTP_ERROR_FIELD_NAME;
    }

    return (d_http_is_field_value(_field->value)) ? D_HTTP_OK
                                                  : D_HTTP_ERROR_FIELD_VALUE;
}

/*
d_http1_parse_fields
  Every field line from _pos to the empty line ending the head, in order;
each is parsed before room is sought for it, so faults are reported in the
order the lines came.
*/
static enum d_http_error
d_http1_parse_fields(
    struct d_pack_text     _data,
    size_t                 _pos,
    struct d_http1_fields* _fields
)
{
    size_t             pos  = _pos;
    struct d_pack_text line = d_http1_next_line(_data,
                                                &pos);

    _fields->count = 0u;

    // each line until the empty one
    while (line.length > 0u)
    {
        struct d_http1_field    field  = { .name = { NULL, 0u } };
        const enum d_http_error parsed = d_http1_parse_field(line,
                                                             &field);

        // a refused line, or no room for a good one
        if ( (parsed != D_HTTP_OK) ||
             (_fields->count >= _fields->capacity) )
        {
            return (parsed != D_HTTP_OK) ? parsed
                                         : D_HTTP_ERROR_TOO_MANY_FIELDS;
        }

        _fields->items[_fields->count] = field;
        _fields->count                += 1u;
        line                           = d_http1_next_line(_data,
                                                           &pos);
    }

    return D_HTTP_OK;
}

/*
d_http1_classify_target
  RFC 9112, 3.2: CONNECT takes authority-form, a host and a port, and nothing
else does; "*" is OPTIONS' alone; any other target is origin-form, an
absolute path and query with no authority, or absolute-form, an absolute
URI. A fragment never belongs.
*/
static enum d_http_error
d_http1_classify_target(
    struct d_http1_request* _out
)
{
    const struct d_pack_text target = _out->target;
    struct d_net_url         url    = { .port = 0u };

    // CONNECT's authority-form: a host and a port, and no userinfo
    if (_out->method == D_HTTP_METHOD_CONNECT)
    {
        _out->target_form = D_HTTP1_TARGET_AUTHORITY;

        return ( (d_net_url_parse_authority(target,
                                            &url,
                                            NULL) == D_NET_URL_OK) &&
                 (!url.has_userinfo)                               &&
                 (url.host.length > 0u)                            &&
                 (url.port_text.length > 0u) ) ? D_HTTP_OK
                                                : D_HTTP_ERROR_TARGET;
    }

    // the asterisk-form, OPTIONS' alone
    if ( (target.length == 1u) &&
         (target.data[0] == '*') )
    {
        _out->target_form = D_HTTP1_TARGET_ASTERISK;

        return (_out->method == D_HTTP_METHOD_OPTIONS) ? D_HTTP_OK
                                                       : D_HTTP_ERROR_TARGET;
    }

    // a URI reference, and no fragment
    if ( (d_net_url_parse(target,
                          &url,
                          NULL) != D_NET_URL_OK) ||
         (url.has_fragment) )
    {
        return D_HTTP_ERROR_TARGET;
    }

    _out->target_form = (target.data[0] == '/') ? D_HTTP1_TARGET_ORIGIN
                                                : D_HTTP1_TARGET_ABSOLUTE;

    return ( ( (_out->target_form == D_HTTP1_TARGET_ORIGIN) &&
               (!url.has_authority) )                            ||
             ( (_out->target_form == D_HTTP1_TARGET_ABSOLUTE) &&
               (d_net_url_is_absolute(&url)) ) ) ? D_HTTP_OK
                                                 : D_HTTP_ERROR_TARGET;
}

/*
d_http1_parse_request_line
  method SP request-target SP HTTP-version, each separator one space (RFC
9112, 3): an empty part, or a third space, is malformed.
*/
static enum d_http_error
d_http1_parse_request_line(
    struct d_pack_text      _line,
    struct d_http1_request* _out
)
{
    const size_t first  = d_http1_find(_line,
                                       0u,
                                       ' ');
    const size_t second = d_http1_find(_line,
                                       first + 1u,
                                       ' ');

    // a CR the line's end did not claim
    if (d_http1_find(_line,
                     0u,
                     '\r') < _line.length)
    {
        return D_HTTP_ERROR_LINE_ENDING;
    }

    // a method, a target, and a version, each separated by one space
    if ( (first == 0u)                  ||
         (second >= _line.length)       ||
         (second == first + 1u)         ||
         (second + 1u == _line.length)  ||
         (d_http1_find(_line,
                       second + 1u,
                       ' ') < _line.length) )
    {
        return D_HTTP_ERROR_START_LINE;
    }

    _out->method_name = d_http1_view(_line,
                                     0u,
                                     first);
    _out->target      = d_http1_view(_line,
                                     first + 1u,
                                     second);

    // the method, then the version, then the target's form
    if (d_http_method_parse(_out->method_name,
                            &_out->method) != D_HTTP_OK)
    {
        return D_HTTP_ERROR_METHOD;
    }

    // HTTP/1.x
    if (d_http_version_parse(d_http1_view(_line,
                                          second + 1u,
                                          _line.length),
                             &_out->version) != D_HTTP_OK)
    {
        return D_HTTP_ERROR_VERSION;
    }

    return d_http1_classify_target(_out);
}

/*
d_http1_check_host
  HTTP/1.1 needs exactly one Host, HTTP/1.0 at most one (RFC 9112, 3.2); its
value is an authority without userinfo, and may be empty.
*/
static enum d_http_error
d_http1_check_host(
    const struct d_http1_request* _out
)
{
    const struct d_pack_text name   = { D_HTTP_FIELD_HOST,
                                        sizeof(D_HTTP_FIELD_HOST) - 1u };
    const size_t             count  = _out->fields.count;
    const size_t             first  = d_http1_fields_index(&_out->fields,
                                                           name,
                                                           0u);
    const size_t             second = (first < count)
                                          ? d_http1_fields_index(
                                                &_out->fields,
                                                name,
                                                first + 1u)
                                          : count;
    struct d_net_url         url    = { .port = 0u };

    // two in any version, or none in 1.1
    if ( (second < count) ||
         ( (first == count) &&
           (_out->version == D_HTTP_VERSION_1_1) ) )
    {
        return D_HTTP_ERROR_HOST;
    }

    return ( (first == count) ||
             ( (d_net_url_parse_authority(_out->fields.items[first].value,
                                          &url,
                                          NULL) == D_NET_URL_OK) &&
               (!url.has_userinfo) ) ) ? D_HTTP_OK
                                       : D_HTTP_ERROR_HOST;
}

/*
d_http1_parse_status_line
  HTTP-version SP status-code SP reason-phrase (RFC 9112, 4). The version and
code are fixed-width; an empty reason may lack the space before it; a
reason's bytes are text, spaces, and tabs.
*/
static enum d_http_error
d_http1_parse_status_line(
    struct d_pack_text       _line,
    struct d_http1_response* _out
)
{
    // a CR the line's end did not claim
    if (d_http1_find(_line,
                     0u,
                     '\r') < _line.length)
    {
        return D_HTTP_ERROR_LINE_ENDING;
    }

    // eight bytes of version, a space, three of code, a space before more
    if ( (_line.length < 12u)       ||
         (_line.data[8] != ' ')     ||
         ( (_line.length > 12u) &&
           (_line.data[12] != ' ') ) )
    {
        return D_HTTP_ERROR_START_LINE;
    }

    // HTTP/1.x
    if (d_http_version_parse(d_http1_view(_line,
                                          0u,
                                          8u),
                             &_out->version) != D_HTTP_OK)
    {
        return D_HTTP_ERROR_VERSION;
    }

    // three digits, 100 to 599
    if (d_http_status_parse(d_http1_view(_line,
                                         9u,
                                         12u),
                            &_out->status) != D_HTTP_OK)
    {
        return D_HTTP_ERROR_STATUS;
    }

    _out->reason = d_http1_view(_line,
                                (_line.length > 12u) ? 13u
                                                     : 12u,
                                _line.length);

    // each byte of the reason: text, a space, or a tab
    for (size_t i = 0u; i < _out->reason.length; ++i)
    {
        const unsigned char byte = (unsigned char)_out->reason.data[i];

        // a control other than HTAB, or DEL
        if ( ( (byte < 0x20u) &&
               (byte != 0x09u) ) ||
             (byte == 0x7Fu) )
        {
            return D_HTTP_ERROR_START_LINE;
        }
    }

    return D_HTTP_OK;
}

/*
d_http1_parser_init
  cfg_http.h's limits, then an empty scan.
*/
void
d_http1_parser_init(
    struct d_http1_parser* _parser
)
{
    // nothing to set up
    if (!_parser)
    {
        return;
    }

    _parser->start_line_max = (size_t)D_INTERNAL_HTTP_START_LINE_MAX;
    _parser->head_max       = (size_t)D_INTERNAL_HTTP_HEAD_MAX;
    d_http1_parser_reset(_parser);

    return;
}

/*
d_http1_parser_reset
  The scan forgotten; the limits kept.
*/
void
d_http1_parser_reset(
    struct d_http1_parser* _parser
)
{
    // nothing to reset
    if (!_parser)
    {
        return;
    }

    _parser->line    = 0u;
    _parser->start   = 0u;
    _parser->started = false;

    return;
}

/*
d_http1_parse_request
  The scan, then, once the head has ended, one pass over it: the request
line, the fields, and the Host rule. The parser is reset as soon as the scan
answers anything but INCOMPLETE.
*/
enum d_http_error
d_http1_parse_request(
    struct d_http1_parser*  _parser,
    struct d_pack_text      _data,
    struct d_http1_request* _out,
    size_t*                 _consumed
)
{
    // parameter validation
    if ( (!_parser)                         ||
         (!_out)                            ||
         (!_consumed)                       ||
         ( (!_data.data) &&
           (_data.length != 0u) )           ||
         ( (!_out->fields.items) &&
           (_out->fields.capacity != 0u) ) )
    {
        return D_HTTP_ERROR_ARGUMENT;
    }

    size_t            end    = 0u;
    enum d_http_error result = d_http1_scan(_parser,
                                            _data,
                                            true,
                                            &end);
    size_t            pos    = _parser->start;

    // not ended yet: the scan keeps its place
    if (result == D_HTTP_ERROR_INCOMPLETE)
    {
        return result;
    }

    d_http1_parser_reset(_parser);

    // the request line
    if (result == D_HTTP_OK)
    {
        result = d_http1_parse_request_line(d_http1_next_line(_data,
                                                              &pos),
                                            _out);
    }

    // the fields
    if (result == D_HTTP_OK)
    {
        result = d_http1_parse_fields(_data,
                                      pos,
                                      &_out->fields);
    }

    // the Host rule
    if (result == D_HTTP_OK)
    {
        result = d_http1_check_host(_out);
    }

    // the head's length, for the caller to find what follows
    if (result == D_HTTP_OK)
    {
        *_consumed = end;
    }

    return result;
}

/*
d_http1_parse_response
  As for requests, with the status line, no empty lines skipped before it,
and no Host rule.
*/
enum d_http_error
d_http1_parse_response(
    struct d_http1_parser*   _parser,
    struct d_pack_text       _data,
    struct d_http1_response* _out,
    size_t*                  _consumed
)
{
    // parameter validation
    if ( (!_parser)                         ||
         (!_out)                            ||
         (!_consumed)                       ||
         ( (!_data.data) &&
           (_data.length != 0u) )           ||
         ( (!_out->fields.items) &&
           (_out->fields.capacity != 0u) ) )
    {
        return D_HTTP_ERROR_ARGUMENT;
    }

    size_t            end    = 0u;
    enum d_http_error result = d_http1_scan(_parser,
                                            _data,
                                            false,
                                            &end);
    size_t            pos    = _parser->start;

    // not ended yet: the scan keeps its place
    if (result == D_HTTP_ERROR_INCOMPLETE)
    {
        return result;
    }

    d_http1_parser_reset(_parser);

    // the status line
    if (result == D_HTTP_OK)
    {
        result = d_http1_parse_status_line(d_http1_next_line(_data,
                                                             &pos),
                                           _out);
    }

    // the fields
    if (result == D_HTTP_OK)
    {
        result = d_http1_parse_fields(_data,
                                      pos,
                                      &_out->fields);
    }

    // the head's length, for the caller to find what follows
    if (result == D_HTTP_OK)
    {
        *_consumed = end;
    }

    return result;
}

/*
d_http1_fields_index
  A linear search from _from; heads hold few fields, and order matters.
*/
size_t
d_http1_fields_index(
    const struct d_http1_fields* _fields,
    struct d_pack_text           _name,
    size_t                       _from
)
{
    // no fields to search
    if (!_fields)
    {
        return 0u;
    }

    // each field from _from on
    for (size_t i = _from; i < _fields->count; ++i)
    {
        // the first whose name matches
        if (d_http_field_name_equal(_fields->items[i].name,
                                    _name))
        {
            return i;
        }
    }

    return _fields->count;
}
