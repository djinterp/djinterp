/*******************************************************************************
* djinterp [net]                                                          http.c
*
* HTTP's semantics: http.h's definitions.
*   Names come from tables in the enumerations' order, reason phrases from a
* table sorted by code and searched by halves, and tokens and field values
* are checked against RFC 9110's grammar byte by byte. Every comparison is
* ASCII, never locale-dependent, so HTTP text reads the same on every host.
*
*
* path:      /src/djinterp/net/http/http.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "../../../../inc/djinterp/net/http/http.h"  // corresponding header
// std
#include <string.h>  // memcmp, strlen


// d_http_reason
//   struct: a registered status code and its reason phrase.
struct d_http_reason
{
    unsigned int code;
    const char*  phrase;
};

// METHOD_NAMES
//   constant: the registered methods' names, in d_http_method's order.
static const char* const METHOD_NAMES[] =
{
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "PATCH",
    "OPTIONS",
    "TRACE",
    "CONNECT"
};

// VERSION_NAMES
//   constant: each version's name, in d_http_version's order.
static const char* const VERSION_NAMES[] =
{
    NULL,
    "HTTP/1.0",
    "HTTP/1.1",
    "HTTP/2",
    "HTTP/3"
};

// REASONS
//   constant: IANA's status code registry, sorted by code.
static const struct d_http_reason REASONS[] =
{
    { 100u, "Continue" },
    { 101u, "Switching Protocols" },
    { 102u, "Processing" },
    { 103u, "Early Hints" },
    { 200u, "OK" },
    { 201u, "Created" },
    { 202u, "Accepted" },
    { 203u, "Non-Authoritative Information" },
    { 204u, "No Content" },
    { 205u, "Reset Content" },
    { 206u, "Partial Content" },
    { 207u, "Multi-Status" },
    { 208u, "Already Reported" },
    { 226u, "IM Used" },
    { 300u, "Multiple Choices" },
    { 301u, "Moved Permanently" },
    { 302u, "Found" },
    { 303u, "See Other" },
    { 304u, "Not Modified" },
    { 305u, "Use Proxy" },
    { 307u, "Temporary Redirect" },
    { 308u, "Permanent Redirect" },
    { 400u, "Bad Request" },
    { 401u, "Unauthorized" },
    { 402u, "Payment Required" },
    { 403u, "Forbidden" },
    { 404u, "Not Found" },
    { 405u, "Method Not Allowed" },
    { 406u, "Not Acceptable" },
    { 407u, "Proxy Authentication Required" },
    { 408u, "Request Timeout" },
    { 409u, "Conflict" },
    { 410u, "Gone" },
    { 411u, "Length Required" },
    { 412u, "Precondition Failed" },
    { 413u, "Content Too Large" },
    { 414u, "URI Too Long" },
    { 415u, "Unsupported Media Type" },
    { 416u, "Range Not Satisfiable" },
    { 417u, "Expectation Failed" },
    { 418u, "I'm a teapot" },
    { 421u, "Misdirected Request" },
    { 422u, "Unprocessable Content" },
    { 423u, "Locked" },
    { 424u, "Failed Dependency" },
    { 425u, "Too Early" },
    { 426u, "Upgrade Required" },
    { 428u, "Precondition Required" },
    { 429u, "Too Many Requests" },
    { 431u, "Request Header Fields Too Large" },
    { 451u, "Unavailable For Legal Reasons" },
    { 500u, "Internal Server Error" },
    { 501u, "Not Implemented" },
    { 502u, "Bad Gateway" },
    { 503u, "Service Unavailable" },
    { 504u, "Gateway Timeout" },
    { 505u, "HTTP Version Not Supported" },
    { 506u, "Variant Also Negotiates" },
    { 507u, "Insufficient Storage" },
    { 508u, "Loop Detected" },
    { 510u, "Not Extended" },
    { 511u, "Network Authentication Required" }
};

/*
d_http_is_digit
  ASCII digits.
*/
static bool
d_http_is_digit(
    char _c
)
{
    return ( (_c >= '0') &&
             (_c <= '9') );
}

/*
d_http_is_tchar
  tchar (RFC 9110, 5.6.2): letters, digits, and fifteen punctuation marks.
*/
static bool
d_http_is_tchar(
    char _c
)
{
    // letters and digits, ASCII only
    if ( ( (_c >= 'a') &&
           (_c <= 'z') )     ||
         ( (_c >= 'A') &&
           (_c <= 'Z') )     ||
         (d_http_is_digit(_c)) )
    {
        return true;
    }

    switch (_c)
    {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '*':
        case '+':
        case '-':
        case '.':
        case '^':
        case '_':
        case '`':
        case '|':
        case '~':
            return true;
        default:
            return false;
    }
}

/*
d_http_lower
  ASCII lowercase, never locale case.
*/
static char
d_http_lower(
    char _c
)
{
    return ( (_c >= 'A') &&
             (_c <= 'Z') ) ? (char)(_c + ('a' - 'A'))
                           : _c;
}

/*
d_http_is_blank
  The whitespace HTTP allows inside a field value: SP and HTAB.
*/
static bool
d_http_is_blank(
    char _c
)
{
    return ( (_c == ' ') ||
             (_c == '\t') );
}

/*
d_http_method_name
  An index into METHOD_NAMES, checked as unsigned so no value escapes it.
*/
const char*
d_http_method_name(
    enum d_http_method _method
)
{
    return ( (unsigned int)_method < (unsigned int)D_HTTP_METHOD_OTHER )
               ? METHOD_NAMES[_method]
               : NULL;
}

/*
d_http_method_parse
  A token first, then an exact, case-sensitive match against the table; a
token matching nothing is an extension method.
*/
enum d_http_error
d_http_method_parse(
    struct d_pack_text  _text,
    enum d_http_method* _out
)
{
    // parameter validation
    if ( (!_out) ||
         ( (!_text.data) &&
           (_text.length != 0u) ) )
    {
        return D_HTTP_ERROR_ARGUMENT;
    }

    // a method is a token
    if (!d_http_is_token(_text))
    {
        return D_HTTP_ERROR_METHOD;
    }

    // each registered name, compared exactly
    for (unsigned int m = 0u; m < (unsigned int)D_HTTP_METHOD_OTHER; ++m)
    {
        const size_t length = strlen(METHOD_NAMES[m]);

        // the same length and the same bytes
        if ( (length == _text.length) &&
             (memcmp(_text.data,
                     METHOD_NAMES[m],
                     length) == 0) )
        {
            *_out = (enum d_http_method)m;

            return D_HTTP_OK;
        }
    }

    *_out = D_HTTP_METHOD_OTHER;

    return D_HTTP_OK;
}

/*
d_http_method_is_safe
  GET, HEAD, OPTIONS, and TRACE (RFC 9110, 9.2.1).
*/
bool
d_http_method_is_safe(
    enum d_http_method _method
)
{
    return ( (_method == D_HTTP_METHOD_GET)     ||
             (_method == D_HTTP_METHOD_HEAD)    ||
             (_method == D_HTTP_METHOD_OPTIONS) ||
             (_method == D_HTTP_METHOD_TRACE) );
}

/*
d_http_method_is_idempotent
  The safe methods, and PUT and DELETE (RFC 9110, 9.2.2).
*/
bool
d_http_method_is_idempotent(
    enum d_http_method _method
)
{
    return ( (d_http_method_is_safe(_method)) ||
             (_method == D_HTTP_METHOD_PUT)   ||
             (_method == D_HTTP_METHOD_DELETE) );
}

/*
d_http_version_name
  An index into VERSION_NAMES, whose first entry is NULL for unknown.
*/
const char*
d_http_version_name(
    enum d_http_version _version
)
{
    return ( (unsigned int)_version <= (unsigned int)D_HTTP_VERSION_3 )
               ? VERSION_NAMES[_version]
               : NULL;
}

/*
d_http_version_parse
  Exactly eight bytes, "HTTP/" and a digit, '.', and a digit; the major
digit must be 1, since only HTTP/1.x has this syntax on the wire.
*/
enum d_http_error
d_http_version_parse(
    struct d_pack_text   _text,
    enum d_http_version* _out
)
{
    // parameter validation
    if ( (!_out) ||
         ( (!_text.data) &&
           (_text.length != 0u) ) )
    {
        return D_HTTP_ERROR_ARGUMENT;
    }

    // HTTP-version = HTTP-name "/" DIGIT "." DIGIT, and major version 1
    if ( (_text.length != 8u)               ||
         (memcmp(_text.data,
                 "HTTP/1.",
                 7u) != 0)                  ||
         (!d_http_is_digit(_text.data[7])) )
    {
        return D_HTTP_ERROR_VERSION;
    }

    *_out = (_text.data[7] == '0') ? D_HTTP_VERSION_1_0
                                   : D_HTTP_VERSION_1_1;

    return D_HTTP_OK;
}

/*
d_http_status_is_valid
  Three digits whose first is 1 to 5 (RFC 9110, section 15).
*/
bool
d_http_status_is_valid(
    unsigned int _status
)
{
    return ( (_status >= 100u) &&
             (_status <= 599u) );
}

/*
d_http_status_parse
  Exactly three digits, and then a valid code.
*/
enum d_http_error
d_http_status_parse(
    struct d_pack_text _text,
    unsigned int*      _out
)
{
    // parameter validation
    if ( (!_out) ||
         ( (!_text.data) &&
           (_text.length != 0u) ) )
    {
        return D_HTTP_ERROR_ARGUMENT;
    }

    // status-code = 3DIGIT
    if ( (_text.length != 3u)              ||
         (!d_http_is_digit(_text.data[0])) ||
         (!d_http_is_digit(_text.data[1])) ||
         (!d_http_is_digit(_text.data[2])) )
    {
        return D_HTTP_ERROR_STATUS;
    }

    const unsigned int status = ( (unsigned int)(_text.data[0] - '0') * 100u ) +
                                ( (unsigned int)(_text.data[1] - '0') * 10u ) +
                                (unsigned int)(_text.data[2] - '0');

    // three digits outside 100 to 599 are not a status code
    if (!d_http_status_is_valid(status))
    {
        return D_HTTP_ERROR_STATUS;
    }

    *_out = status;

    return D_HTTP_OK;
}

/*
d_http_status_class_of
  The first digit, 1 to 5, is the class, in d_http_status_class's order.
*/
enum d_http_status_class
d_http_status_class_of(
    unsigned int _status
)
{
    return (d_http_status_is_valid(_status))
               ? (enum d_http_status_class)(_status / 100u)
               : D_HTTP_STATUS_CLASS_NONE;
}

/*
d_http_status_reason
  A binary search of REASONS, which is sorted by code.
*/
const char*
d_http_status_reason(
    unsigned int _status
)
{
    const size_t count = sizeof(REASONS) / sizeof(REASONS[0]);
    size_t       low   = 0u;
    size_t       high  = count;

    // narrow to the first entry not below the code
    while (low < high)
    {
        const size_t middle = low + ( (high - low) / 2u );

        // the code lies above the middle entry, or at or below it
        if (REASONS[middle].code < _status)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return ( (low < count) &&
             (REASONS[low].code == _status) ) ? REASONS[low].phrase
                                              : "";
}

/*
d_http_status_allows_content
  Only a valid code outside 1xx, 204, and 304 can have content.
*/
bool
d_http_status_allows_content(
    unsigned int _status
)
{
    return ( (d_http_status_is_valid(_status)) &&
             (_status >= 200u)                 &&
             (_status != 204u)                 &&
             (_status != 304u) );
}

/*
d_http_status_is_redirect
  The codes whose Location a client may follow on its own.
*/
bool
d_http_status_is_redirect(
    unsigned int _status
)
{
    return ( (_status == 301u) ||
             (_status == 302u) ||
             (_status == 303u) ||
             (_status == 307u) ||
             (_status == 308u) );
}

/*
d_http_is_token
  One or more tchar; a NULL text with a length is no token.
*/
bool
d_http_is_token(
    struct d_pack_text _text
)
{
    // an empty or unreadable text is no token
    if ( (_text.length == 0u) ||
         (!_text.data) )
    {
        return false;
    }

    // each byte a tchar
    for (size_t i = 0u; i < _text.length; ++i)
    {
        // a byte no token may hold
        if (!d_http_is_tchar(_text.data[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_http_is_field_value
  Every byte visible ASCII, obs-text, or whitespace, and no whitespace at
either end. That leaves out every control but HTAB, DEL, and so CR, LF, and
NUL, which RFC 9110 calls invalid and dangerous.
*/
bool
d_http_is_field_value(
    struct d_pack_text _text
)
{
    // an unreadable text is no value; an empty one is
    if ( (!_text.data) &&
         (_text.length != 0u) )
    {
        return false;
    }

    // each byte, and whitespace only inside
    for (size_t i = 0u; i < _text.length; ++i)
    {
        const unsigned char byte  = (unsigned char)_text.data[i];
        const bool          blank = d_http_is_blank(_text.data[i]);

        // a control, DEL, or whitespace at either end
        if ( ( (!blank)           &&
               ( (byte < 0x21u) ||
                 (byte == 0x7Fu) ) )        ||
             ( (blank) &&
               ( (i == 0u) ||
                 (i + 1u == _text.length) ) ) )
        {
            return false;
        }
    }

    return true;
}

/*
d_http_trim_whitespace
  Narrows the view from both ends; the bytes are not touched.
*/
struct d_pack_text
d_http_trim_whitespace(
    struct d_pack_text _text
)
{
    struct d_pack_text trimmed = _text;

    // an unreadable text has nothing to trim
    if (!trimmed.data)
    {
        return trimmed;
    }

    // leading whitespace
    while ( (trimmed.length > 0u) &&
            (d_http_is_blank(trimmed.data[0])) )
    {
        trimmed.data   += 1;
        trimmed.length -= 1u;
    }

    // trailing whitespace
    while ( (trimmed.length > 0u) &&
            (d_http_is_blank(trimmed.data[trimmed.length - 1u])) )
    {
        trimmed.length -= 1u;
    }

    return trimmed;
}

/*
d_http_field_name_equal
  The same length, and the same bytes once ASCII case is folded.
*/
bool
d_http_field_name_equal(
    struct d_pack_text _name,
    struct d_pack_text _other
)
{
    // different lengths, or an unreadable name
    if ( (_name.length != _other.length) ||
         ( (_name.length != 0u) &&
           ( (!_name.data) ||
             (!_other.data) ) ) )
    {
        return false;
    }

    // each byte, case folded
    for (size_t i = 0u; i < _name.length; ++i)
    {
        // the first difference
        if (d_http_lower(_name.data[i]) != d_http_lower(_other.data[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_http_error_name
  A short phrase for each error, for messages.
*/
const char*
d_http_error_name(
    enum d_http_error _error
)
{
    switch (_error)
    {
        case D_HTTP_OK:
            return "no error";
        case D_HTTP_ERROR_ARGUMENT:
            return "a NULL argument";
        case D_HTTP_ERROR_METHOD:
            return "method is not a token";
        case D_HTTP_ERROR_VERSION:
            return "version is not HTTP/1.x";
        case D_HTTP_ERROR_STATUS:
            return "status code is not three digits from 100 to 599";
        case D_HTTP_ERROR_FIELD_NAME:
            return "field name is not a token";
        case D_HTTP_ERROR_FIELD_VALUE:
            return "field value holds a forbidden byte";
        case D_HTTP_ERROR_INCOMPLETE:
            return "more bytes needed";
        case D_HTTP_ERROR_START_LINE:
            return "malformed start line";
        case D_HTTP_ERROR_START_LINE_TOO_LONG:
            return "start line too long";
        case D_HTTP_ERROR_HEAD_TOO_LARGE:
            return "head too large";
        case D_HTTP_ERROR_LINE_ENDING:
            return "bare CR or LF";
        case D_HTTP_ERROR_OBS_FOLD:
            return "obsolete line folding";
        case D_HTTP_ERROR_TOO_MANY_FIELDS:
            return "too many field lines";
        case D_HTTP_ERROR_TARGET:
            return "malformed request-target";
        case D_HTTP_ERROR_HOST:
            return "missing, repeated, or malformed Host";
        default:
            return "unknown error";
    }
}
