/*******************************************************************************
* djinterp [net]                                                   smtp_common.c
*
* djinterp SMTP protocol vocabulary -- implementation.
*   Defines everything smtp_common.h declares. Every routine is pure: it reads
* its arguments and writes caller-owned memory, never allocating and never
* touching a transport, which keeps the protocol rules testable without a
* network.
*
*
* path:      /src/djinterp/net/smtp/smtp_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../inc/djinterp/net/smtp/smtp_common.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf
#include <string.h>   // memchr, memcpy, strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"      // d_pack_bytes, d_pack_text
#include "../../../../inc/djinterp/net/net_common.h"   // d_net_ascii_*
#include "../../../../inc/djinterp/net/codec/codec_base64.h"  // d_net_base64_*
#include "../../../../inc/djinterp/net/codec/codec_hex.h"     // d_net_hex_is_digit
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // SIZE_MAX


// d_internal_smtp_name
//   struct: one entry of a keyword table.
struct d_internal_smtp_name
{
    const char*  name;
    unsigned int value;
};

// EXTENSION_NAMES
//   constant: EHLO keywords and the extension bits they set.
static const struct d_internal_smtp_name EXTENSION_NAMES[] =
{
    { "PIPELINING",          D_SMTP_EXTENSION_PIPELINING },
    { "SIZE",                D_SMTP_EXTENSION_SIZE },
    { "8BITMIME",            D_SMTP_EXTENSION_8BITMIME },
    { "STARTTLS",            D_SMTP_EXTENSION_STARTTLS },
    { "AUTH",                D_SMTP_EXTENSION_AUTH },
    { "ENHANCEDSTATUSCODES", D_SMTP_EXTENSION_ENHANCEDSTATUSCODES },
    { "SMTPUTF8",            D_SMTP_EXTENSION_SMTPUTF8 },
    { "CHUNKING",            D_SMTP_EXTENSION_CHUNKING },
    { "DSN",                 D_SMTP_EXTENSION_DSN }
};

// AUTH_NAMES
//   constant: SASL mechanism names and their bits.
static const struct d_internal_smtp_name AUTH_NAMES[] =
{
    { "PLAIN",    D_SMTP_AUTH_PLAIN },
    { "LOGIN",    D_SMTP_AUTH_LOGIN },
    { "CRAM-MD5", D_SMTP_AUTH_CRAM_MD5 },
    { "XOAUTH2",  D_SMTP_AUTH_XOAUTH2 }
};

// VERB_NAMES
//   constant: command verbs and their d_smtp_verb values.
static const struct d_internal_smtp_name VERB_NAMES[] =
{
    { "HELO",     D_SMTP_VERB_HELO },
    { "EHLO",     D_SMTP_VERB_EHLO },
    { "MAIL",     D_SMTP_VERB_MAIL },
    { "RCPT",     D_SMTP_VERB_RCPT },
    { "DATA",     D_SMTP_VERB_DATA },
    { "RSET",     D_SMTP_VERB_RSET },
    { "NOOP",     D_SMTP_VERB_NOOP },
    { "QUIT",     D_SMTP_VERB_QUIT },
    { "VRFY",     D_SMTP_VERB_VRFY },
    { "HELP",     D_SMTP_VERB_HELP },
    { "STARTTLS", D_SMTP_VERB_STARTTLS },
    { "AUTH",     D_SMTP_VERB_AUTH }
};

// ATEXT_SPECIALS
//   constant: the non-alphanumeric characters of RFC 5322 atext.
static const char ATEXT_SPECIALS[] = "!#$%&'*+-/=?^_`{|}~";

/*
d_internal_smtp_lookup
  Linear search of a keyword table; the tables are a dozen entries at most.
Returns the matched entry's value, or `_fallback`.
*/
static unsigned int
d_internal_smtp_lookup(
    const struct d_internal_smtp_name* _table,
    size_t                             _count,
    const char*                        _text,
    size_t                             _length,
    unsigned int                       _fallback
)
{
    const struct d_pack_text text = { _text,
                                      _length };

    // each name in turn, ignoring case
    for (size_t i = 0; i < _count; ++i)
    {
        if (d_net_ascii_ieq_cstr(text,
                                 _table[i].name))
        {
            return _table[i].value;
        }
    }

    return _fallback;
}

/*
d_smtp_error_string
  A switch with one case per error; a value outside the enum falls
through to "unknown error", so the result is never NULL.
*/
const char*
d_smtp_error_string(
    enum d_smtp_error _error
)
{
    switch (_error)
    {
        case D_SMTP_OK:                     return "success";
        case D_SMTP_ERROR_INVALID_ARGUMENT: return "invalid argument";
        case D_SMTP_ERROR_STATE:            return "operation out of sequence";
        case D_SMTP_ERROR_OUT_OF_MEMORY:    return "out of memory";
        case D_SMTP_ERROR_RESOLVE:          return "host not found";
        case D_SMTP_ERROR_CONNECT:          return "connection failed";
        case D_SMTP_ERROR_TIMED_OUT:        return "timed out";
        case D_SMTP_ERROR_TRANSPORT:        return "transport error";
        case D_SMTP_ERROR_CLOSED:           return "connection closed by peer";
        case D_SMTP_ERROR_TLS:              return "TLS failure";
        case D_SMTP_ERROR_PROTOCOL:         return "protocol violation";
        case D_SMTP_ERROR_LINE_TOO_LONG:    return "limit exceeded";
        case D_SMTP_ERROR_REJECTED:         return "rejected by peer";
        case D_SMTP_ERROR_UNSUPPORTED:      return "not supported";
        case D_SMTP_ERROR_AUTH:             return "authentication failed";
        case D_SMTP_ERROR_INSECURE:         return "refused to authenticate "
                                                   "in the clear";
    }

    return "unknown error";
}

/*
d_smtp_error_to_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules. A value outside the enum is UNKNOWN.
*/
enum d_net_error
d_smtp_error_to_net(
    enum d_smtp_error _error
)
{
    switch (_error)
    {
        case D_SMTP_OK:
            return D_NET_OK;
        case D_SMTP_ERROR_INVALID_ARGUMENT:
        case D_SMTP_ERROR_STATE:
            return D_NET_ERROR_INVALID_ARGUMENT;
        case D_SMTP_ERROR_TLS:
        case D_SMTP_ERROR_PROTOCOL:
            return D_NET_ERROR_PROTOCOL;
        case D_SMTP_ERROR_LINE_TOO_LONG:
            return D_NET_ERROR_MESSAGE_TOO_LARGE;
        case D_SMTP_ERROR_RESOLVE:
            return D_NET_ERROR_HOST_UNREACHABLE;
        case D_SMTP_ERROR_CONNECT:
            return D_NET_ERROR_CONNECTION_REFUSED;
        case D_SMTP_ERROR_CLOSED:
            return D_NET_ERROR_CLOSED;
        case D_SMTP_ERROR_TIMED_OUT:
            return D_NET_ERROR_TIMED_OUT;
        case D_SMTP_ERROR_OUT_OF_MEMORY:
            return D_NET_ERROR_OUT_OF_MEMORY;
        case D_SMTP_ERROR_AUTH:
        case D_SMTP_ERROR_INSECURE:
            return D_NET_ERROR_ACCESS_DENIED;
        case D_SMTP_ERROR_TRANSPORT:
        case D_SMTP_ERROR_REJECTED:
        case D_SMTP_ERROR_UNSUPPORTED:
            return D_NET_ERROR_UNKNOWN;
    }

    return D_NET_ERROR_UNKNOWN;
}

/*
d_smtp_error_from_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules the other way. A value outside the
enum is TRANSPORT.
*/
enum d_smtp_error
d_smtp_error_from_net(
    enum d_net_error _error
)
{
    switch (_error)
    {
        case D_NET_OK:
            return D_SMTP_OK;
        case D_NET_ERROR_INVALID_ARGUMENT:
            return D_SMTP_ERROR_INVALID_ARGUMENT;
        case D_NET_ERROR_OUT_OF_MEMORY:
            return D_SMTP_ERROR_OUT_OF_MEMORY;
        case D_NET_ERROR_TIMED_OUT:
            return D_SMTP_ERROR_TIMED_OUT;
        case D_NET_ERROR_PROTOCOL:
            return D_SMTP_ERROR_PROTOCOL;
        case D_NET_ERROR_CLOSED:
        case D_NET_ERROR_CONNECTION_RESET:
        case D_NET_ERROR_CONNECTION_ABORTED:
            return D_SMTP_ERROR_CLOSED;
        case D_NET_ERROR_WOULD_BLOCK:
        case D_NET_ERROR_INTERRUPTED:
        case D_NET_ERROR_CONNECTION_REFUSED:
        case D_NET_ERROR_ADDRESS_IN_USE:
        case D_NET_ERROR_ADDRESS_INVALID:
        case D_NET_ERROR_HOST_UNREACHABLE:
        case D_NET_ERROR_NETWORK_DOWN:
        case D_NET_ERROR_ACCESS_DENIED:
        case D_NET_ERROR_MESSAGE_TOO_LARGE:
        case D_NET_ERROR_TOO_MANY_OPEN_FILES:
        case D_NET_ERROR_UNKNOWN:
            return D_SMTP_ERROR_TRANSPORT;
    }

    return D_SMTP_ERROR_TRANSPORT;
}

/*
d_smtp_error_from_ssl
  One case per enumerator and no default, so the compiler reports a missing
case. A transport failure that reported nothing -- a write that took
nothing -- has no error of its own to give, and is TRANSPORT.
*/
enum d_smtp_error
d_smtp_error_from_ssl(
    enum d_ssl_error _status,
    enum d_net_error _cause
)
{
    switch (_status)
    {
        case D_SSL_OK:
            return D_SMTP_OK;
        case D_SSL_ERROR_TRANSPORT:
        case D_SSL_ERROR_WOULD_BLOCK:
            return (_cause == D_NET_OK) ? D_SMTP_ERROR_TRANSPORT
                                        : d_smtp_error_from_net(_cause);
        case D_SSL_ERROR_TRUNCATED:
        case D_SSL_ERROR_CLOSED:
            return D_SMTP_ERROR_CLOSED;
        case D_SSL_ERROR_OUT_OF_MEMORY:
            return D_SMTP_ERROR_OUT_OF_MEMORY;
        case D_SSL_ERROR_INVALID_ARGUMENT:
        case D_SSL_ERROR_MALFORMED:
        case D_SSL_ERROR_NOT_FOUND:
        case D_SSL_ERROR_UNSUPPORTED:
        case D_SSL_ERROR_CONFIG:
        case D_SSL_ERROR_STATE:
        case D_SSL_ERROR_NOT_TLS:
        case D_SSL_ERROR_HANDSHAKE:
        case D_SSL_ERROR_VERIFY:
        case D_SSL_ERROR_PROTOCOL:
        case D_SSL_ERROR_BUFFER_TOO_SMALL:
        case D_SSL_ERROR_WANT_READ:
        case D_SSL_ERROR_BACKEND:
            break;
    }

    return D_SMTP_ERROR_TLS;
}

/*
d_smtp_error_from_tls
  Only a session that failed is judged by its status: one still sound, or
ended by the peer's close_notify, leaves the stream's own judgment.
*/
enum d_smtp_error
d_smtp_error_from_tls(
    const struct d_ssl_session* _tls,
    enum d_smtp_error           _result,
    enum d_ssl_error*           _status,
    enum d_net_error*           _cause
)
{
    // nothing failed, or no TLS that could have failed beneath it
    if ( (_result == D_SMTP_OK) ||
         (!_tls) )
    {
        return _result;
    }

    const enum d_ssl_error status = d_ssl_session_failure(_tls);

    // a session that has not failed leaves the stream's judgment
    if (status == D_SSL_OK)
    {
        return _result;
    }

    const enum d_net_error cause = d_ssl_session_transport_error(_tls);

    if (_status)
    {
        *_status = status;
    }

    // the connection's own error is the cause beneath TLS
    if ( (_cause) &&
         (cause != D_NET_OK) )
    {
        *_cause = cause;
    }

    return d_smtp_error_from_ssl(status,
                                 cause);
}

/*
d_smtp_buffer_init
  A NULL buffer is ignored, and NULL storage gets a capacity of 0, so
every append on it fails cleanly; otherwise the text starts empty.
*/
void
d_smtp_buffer_init(
    struct d_smtp_buffer* _buffer,
    char*                 _data,
    size_t                _capacity
)
{
    // parameter validation first
    if (!_buffer)
    {
        return;
    }

    _buffer->data     = _data;
    _buffer->capacity = (_data) ? _capacity : 0;
    _buffer->length   = 0;

    // an empty buffer is also an empty C string
    if (_buffer->capacity > 0)
    {
        _buffer->data[0] = '\0';
    }

    return;
}

/*
d_smtp_buffer_append
  Checks the fit against capacity minus the reserved NUL before copying
anything, so a failed append leaves the buffer exactly as it was.
*/
enum d_smtp_error
d_smtp_buffer_append(
    struct d_smtp_buffer* _buffer,
    const void*           _data,
    size_t                _length
)
{
    // parameter validation first
    if ( (!_buffer) ||
         ( (!_data) &&
           (_length > 0) ) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    // one byte always stays free for the terminating NUL
    if ( (_buffer->capacity == 0) ||
         (_length >= _buffer->capacity - _buffer->length) )
    {
        return D_SMTP_ERROR_LINE_TOO_LONG;
    }

    if (_length > 0)
    {
        memcpy(_buffer->data + _buffer->length,
               _data,
               _length);
    }

    _buffer->length               += _length;
    _buffer->data[_buffer->length] = '\0';

    return D_SMTP_OK;
}

/*
d_smtp_buffer_append_text
  d_smtp_buffer_append() over the text's strlen(); NULL text is refused
before it is measured.
*/
enum d_smtp_error
d_smtp_buffer_append_text(
    struct d_smtp_buffer* _buffer,
    const char*           _text
)
{
    // parameter validation first
    if (!_text)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    return d_smtp_buffer_append(_buffer,
                                _text,
                                strlen(_text));
}

/*
d_smtp_buffer_append_number
  Formats into a local buffer wide enough for any size_t, then
appends the digits.
*/
enum d_smtp_error
d_smtp_buffer_append_number(
    struct d_smtp_buffer* _buffer,
    size_t                _value
)
{
    char      digits[32] = { 0 };
    const int length     = snprintf(digits,
                                    sizeof(digits),
                                    "%zu",
                                    _value);

    // a size_t always fits in 32 digits; this guards a broken libc
    if (length <= 0)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    return d_smtp_buffer_append(_buffer,
                                digits,
                                (size_t)length);
}

/*
d_smtp_reply_clear
  Zeroes every field and empties the text, so a cleared reply reads as
no reply at all.
*/
void
d_smtp_reply_clear(
    struct d_smtp_reply* _reply
)
{
    // parameter validation first
    if (!_reply)
    {
        return;
    }

    _reply->code                = 0;
    _reply->status.status_class = 0;
    _reply->status.subject      = 0;
    _reply->status.detail       = 0;
    _reply->line_count          = 0;
    _reply->length              = 0;
    _reply->complete            = false;
    _reply->truncated           = false;
    _reply->text[0]             = '\0';

    return;
}

/*
d_internal_smtp_parse_status
  Reads "class.subject.detail" at the start of reply text: one digit, then one
to three, then one to three. The class must equal the reply code's first digit
-- that is how RFC 3463 tells a status code from text that merely begins with a
number -- and the status must end the text or be followed by a space.
*/
static bool
d_internal_smtp_parse_status(
    const char*           _text,
    size_t                _length,
    unsigned int          _class,
    struct d_smtp_status* _status
)
{
    unsigned int parts[3] = { 0, 0, 0 };
    size_t       position = 0;

    // three dot-separated numbers
    for (size_t part = 0; part < 3; ++part)
    {
        const size_t limit  = (part == 0) ? 1 : 3;
        size_t       digits = 0;

        while ( (position < _length)                          &&
                (digits < limit)                              &&
                (d_net_ascii_is_digit(_text[position])) )
        {
            parts[part] = (parts[part] * 10u) +
                          (unsigned int)(_text[position] - '0');
            ++position;
            ++digits;
        }

        // every part needs a digit, and the first two end with a dot
        if (digits == 0)
        {
            return false;
        }

        if (part < 2)
        {
            if ( (position >= _length) ||
                 (_text[position] != '.') )
            {
                return false;
            }

            ++position;
        }
    }

    // the status must stand alone and agree with the reply code
    if ( ( (position < _length) &&
           (_text[position] != ' ') ) ||
         (parts[0] != _class) )
    {
        return false;
    }

    _status->status_class = parts[0];
    _status->subject      = parts[1];
    _status->detail       = parts[2];

    return true;
}

/*
d_internal_smtp_reply_store
  Appends one line's text to a reply, separated from the previous line by
'\n'. What does not fit is dropped and the reply is marked truncated; the NUL
terminator is always preserved.
*/
static void
d_internal_smtp_reply_store(
    struct d_smtp_reply* _reply,
    const char*          _text,
    size_t               _length
)
{
    size_t room = (D_SMTP_REPLY_TEXT_MAX - 1u) - _reply->length;

    // lines after the first are separated by a line feed
    if (_reply->line_count > 0)
    {
        if (room == 0)
        {
            _reply->truncated = true;

            return;
        }

        _reply->text[_reply->length] = '\n';
        _reply->length              += 1;
        room                        -= 1;
    }

    // keep what fits, and record that the rest was lost
    const size_t kept = (_length <= room) ? _length : room;

    if (kept < _length)
    {
        _reply->truncated = true;
    }

    memcpy(_reply->text + _reply->length,
           _text,
           kept);

    _reply->length              += kept;
    _reply->text[_reply->length] = '\0';

    return;
}

/*
d_smtp_reply_feed
  Validates the code and separator, fixes the code (and any enhanced status)
from the first line, and requires later lines to repeat that code.
*/
enum d_smtp_error
d_smtp_reply_feed(
    struct d_smtp_reply* _reply,
    const char*          _line,
    size_t               _length
)
{
    // parameter validation first
    if ( (!_reply)          ||
         (!_line)           ||
         (_reply->complete) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    // a reply line opens with a three-digit code whose first digit is 2 to 5
    if ( (_length < 3)                     ||
         (_line[0] < '2')                  ||
         (_line[0] > '5')                  ||
         (!d_net_ascii_is_digit(_line[1])) ||
         (!d_net_ascii_is_digit(_line[2])) )
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    // the code is followed by a hyphen (more lines follow), a space, or nothing
    if ( (_length > 3)     &&
         (_line[3] != '-') &&
         (_line[3] != ' ') )
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    const unsigned int code = ((unsigned int)(_line[0] - '0') * 100u) +
                              ((unsigned int)(_line[1] - '0') * 10u)  +
                              ((unsigned int)(_line[2] - '0'));

    // every line of a reply carries the same code
    if ( (_reply->line_count > 0) &&
         (_reply->code != code) )
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    const bool   last  = ( (_length == 3) ||
                           (_line[3] == ' ') );
    const size_t start = (_length > 3) ? 4 : 3;

    // the first line fixes the code, and may carry the enhanced status
    if (_reply->line_count == 0)
    {
        _reply->code = code;

        (void)d_internal_smtp_parse_status(_line + start,
                                           _length - start,
                                           code / 100u,
                                           &_reply->status);
    }

    d_internal_smtp_reply_store(_reply,
                                _line + start,
                                _length - start);

    _reply->line_count += 1;
    _reply->complete    = last;

    return D_SMTP_OK;
}

/*
d_smtp_reply_line
  Walks the stored text, counting line feeds to the requested line.
*/
const char*
d_smtp_reply_line(
    const struct d_smtp_reply* _reply,
    size_t                     _index,
    size_t*                    _length
)
{
    // parameter validation first
    if ( (!_reply) ||
         (_index >= _reply->line_count) )
    {
        return NULL;
    }

    const char* cursor = _reply->text;
    const char* end    = _reply->text + _reply->length;

    // step over `_index` line feeds; a missing one means truncation ate it
    for (size_t i = 0; i < _index; ++i)
    {
        const char* feed = memchr(cursor,
                                  '\n',
                                  (size_t)(end - cursor));

        if (!feed)
        {
            return NULL;
        }

        cursor = feed + 1;
    }

    const char* stop = memchr(cursor,
                              '\n',
                              (size_t)(end - cursor));

    if (_length)
    {
        *_length = (stop) ? (size_t)(stop - cursor) : (size_t)(end - cursor);
    }

    return cursor;
}

/*
d_smtp_code_is_positive
  A range test on the code: 2yz.
*/
bool
d_smtp_code_is_positive(
    unsigned int _code
)
{
    return ( (_code >= 200u) &&
             (_code <= 299u) );
}

/*
d_smtp_code_is_intermediate
  A range test on the code: 3yz.
*/
bool
d_smtp_code_is_intermediate(
    unsigned int _code
)
{
    return ( (_code >= 300u) &&
             (_code <= 399u) );
}

/*
d_smtp_code_is_transient
  A range test on the code: 4yz.
*/
bool
d_smtp_code_is_transient(
    unsigned int _code
)
{
    return ( (_code >= 400u) &&
             (_code <= 499u) );
}

/*
d_smtp_code_is_permanent
  A range test on the code: 5yz.
*/
bool
d_smtp_code_is_permanent(
    unsigned int _code
)
{
    return ( (_code >= 500u) &&
             (_code <= 599u) );
}

/*
d_internal_smtp_reply_head
  Formats the start of one reply line -- code, separator, and the enhanced
status when there is one -- returning its length, or 0 on a formatting error.
*/
static size_t
d_internal_smtp_reply_head(
    char*                       _head,
    size_t                      _capacity,
    unsigned int                _code,
    char                        _separator,
    const struct d_smtp_status* _status
)
{
    const bool enhanced = ( (_status) &&
                            (_status->status_class != 0) );
    const int  length   = (enhanced) ? snprintf(_head,
                                                _capacity,
                                                "%u%c%u.%u.%u ",
                                                _code,
                                                _separator,
                                                _status->status_class,
                                                _status->subject,
                                                _status->detail)
                                     : snprintf(_head,
                                                _capacity,
                                                "%u%c",
                                                _code,
                                                _separator);

    // a truncated head is a formatting failure, never a partial line
    if ( (length <= 0) ||
         ((size_t)length >= _capacity) )
    {
        return 0;
    }

    return (size_t)length;
}

/*
d_internal_smtp_reply_append
  Appends one reply line: its head, its text, and CRLF. A head of size 0, one
d_internal_smtp_reply_head could not write, makes the reply invalid.
*/
static enum d_smtp_error
d_internal_smtp_reply_append(
    struct d_smtp_buffer* _buffer,
    const char*           _head,
    size_t                _head_size,
    const char*           _text,
    size_t                _length
)
{
    enum d_smtp_error result = (_head_size > 0)
                               ? d_smtp_buffer_append(_buffer,
                                                      _head,
                                                      _head_size)
                               : D_SMTP_ERROR_INVALID_ARGUMENT;

    if (result == D_SMTP_OK)
    {
        result = d_smtp_buffer_append(_buffer,
                                      _text,
                                      _length);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_buffer_append(_buffer,
                                      "\r\n",
                                      2);
    }

    return result;
}

/*
d_smtp_reply_format
  Emits one reply line per '\n'-separated line of text. Any failure rolls the
buffer back to its length on entry, so callers never send half a reply.
*/
enum d_smtp_error
d_smtp_reply_format(
    struct d_smtp_buffer*       _buffer,
    unsigned int                _code,
    const struct d_smtp_status* _status,
    const char*                 _text
)
{
    // parameter validation first
    if ( (!_buffer)               ||
         (!_text)                 ||
         (_code < 200u)           ||
         (_code > 599u)           ||
         (strchr(_text,
                 '\r')) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t      saved  = _buffer->length;
    const char*       cursor = _text;
    enum d_smtp_error result = D_SMTP_OK;

    // one reply line per text line; all but the last carry a hyphen
    while (result == D_SMTP_OK)
    {
        const char*  feed   = strchr(cursor,
                                     '\n');
        const size_t length = (feed) ? (size_t)(feed - cursor) : strlen(cursor);
        char         head[64];  // filled before it is read
        const size_t size   = d_internal_smtp_reply_head(head,
                                                         sizeof(head),
                                                         _code,
                                                         (feed) ? '-' : ' ',
                                                         _status);

        result = d_internal_smtp_reply_append(_buffer,
                                              head,
                                              size,
                                              cursor,
                                              length);

        // the last line has been written
        if (!feed)
        {
            break;
        }

        cursor = feed + 1;
    }

    // roll back a partial reply
    if (result != D_SMTP_OK)
    {
        _buffer->length = saved;

        if (_buffer->capacity > 0)
        {
            _buffer->data[saved] = '\0';
        }
    }

    return result;
}

/*
d_smtp_capabilities_clear
  Back to what a server that never said EHLO offers: no extensions,
no mechanisms, no size limit.
*/
void
d_smtp_capabilities_clear(
    struct d_smtp_capabilities* _capabilities
)
{
    // parameter validation first
    if (!_capabilities)
    {
        return;
    }

    _capabilities->extensions = D_SMTP_EXTENSION_NONE;
    _capabilities->auth       = D_SMTP_AUTH_NONE;
    _capabilities->size_limit = 0;
    _capabilities->extended   = false;

    return;
}

/*
d_internal_smtp_decimal
  Parses a leading decimal number, saturating at SIZE_MAX rather than
wrapping; 0 when the text does not start with a digit.
*/
static size_t
d_internal_smtp_decimal(
    const char* _text,
    size_t      _length
)
{
    size_t value = 0;

    for (size_t i = 0; i < _length; ++i)
    {
        // the number ends at the first non-digit
        if (!d_net_ascii_is_digit(_text[i]))
        {
            break;
        }

        const size_t digit = (size_t)(_text[i] - '0');

        // saturate instead of overflowing
        if (value > (SIZE_MAX - digit) / 10u)
        {
            return SIZE_MAX;
        }

        value = (value * 10u) + digit;
    }

    return value;
}

/*
d_internal_smtp_mechanisms
  Collects the mechanism bits named in a space-separated list; unknown names
are skipped.
*/
static unsigned int
d_internal_smtp_mechanisms(
    const char* _text,
    size_t      _length
)
{
    unsigned int mechanisms = D_SMTP_AUTH_NONE;
    size_t       position   = 0;

    while (position < _length)
    {
        // skip separating spaces, then take one name
        if (_text[position] == ' ')
        {
            ++position;

            continue;
        }

        const size_t start = position;

        while ( (position < _length) &&
                (_text[position] != ' ') )
        {
            ++position;
        }

        mechanisms |= (unsigned int)d_smtp_auth_from_name(_text + start,
                                                          position - start);
    }

    return mechanisms;
}

/*
d_internal_smtp_capability
  Interprets one EHLO line. The keyword runs to the first space or '=', the
latter accepting the pre-standard "AUTH=LOGIN PLAIN" some servers still send.
*/
static void
d_internal_smtp_capability(
    struct d_smtp_capabilities* _capabilities,
    const char*                 _line,
    size_t                      _length
)
{
    size_t keyword = 0;

    while ( (keyword < _length)       &&
            (_line[keyword] != ' ')   &&
            (_line[keyword] != '=') )
    {
        ++keyword;
    }

    const size_t skip   = (keyword < _length) ? 1 : 0;
    const char*  rest   = _line + keyword + skip;
    const size_t length = _length - keyword - skip;
    const size_t count  = sizeof(EXTENSION_NAMES) / sizeof(EXTENSION_NAMES[0]);
    const unsigned int bit = d_internal_smtp_lookup(EXTENSION_NAMES,
                                                    count,
                                                    _line,
                                                    keyword,
                                                    D_SMTP_EXTENSION_NONE);

    _capabilities->extensions |= bit;

    // AUTH lists mechanisms; SIZE may declare the largest message accepted
    if (bit == D_SMTP_EXTENSION_AUTH)
    {
        _capabilities->auth |= d_internal_smtp_mechanisms(rest,
                                                          length);
    }
    else if (bit == D_SMTP_EXTENSION_SIZE)
    {
        _capabilities->size_limit = d_internal_smtp_decimal(rest,
                                                            length);
    }

    return;
}

/*
d_smtp_capabilities_parse
  Line 0 of an EHLO reply is the server's greeting; each later line names one
extension.
*/
enum d_smtp_error
d_smtp_capabilities_parse(
    struct d_smtp_capabilities* _capabilities,
    const struct d_smtp_reply*  _reply
)
{
    // parameter validation first
    if ( (!_capabilities)      ||
         (!_reply)             ||
         (!_reply->complete)   ||
         (_reply->code != 250u) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    d_smtp_capabilities_clear(_capabilities);
    _capabilities->extended = true;

    for (size_t i = 1; i < _reply->line_count; ++i)
    {
        size_t      length = 0;
        const char* line   = d_smtp_reply_line(_reply,
                                               i,
                                               &length);

        // lines lost to truncation cannot be read
        if (!line)
        {
            break;
        }

        d_internal_smtp_capability(_capabilities,
                                   line,
                                   length);
    }

    return D_SMTP_OK;
}

/*
d_smtp_auth_name
  A linear search of the mechanism table, which is short; a value it
does not hold gives NULL.
*/
const char*
d_smtp_auth_name(
    enum d_smtp_auth _mechanism
)
{
    // a table walk rather than a switch keeps names in one place
    for (size_t i = 0; i < sizeof(AUTH_NAMES) / sizeof(AUTH_NAMES[0]); ++i)
    {
        if (AUTH_NAMES[i].value == (unsigned int)_mechanism)
        {
            return AUTH_NAMES[i].name;
        }
    }

    return NULL;
}

/*
d_smtp_auth_from_name
  The table lookup d_smtp_auth_name() uses, the other way: matched
case-insensitively over the given length, D_SMTP_AUTH_NONE when no name
matches.
*/
enum d_smtp_auth
d_smtp_auth_from_name(
    const char* _name,
    size_t      _length
)
{
    // parameter validation first
    if (!_name)
    {
        return D_SMTP_AUTH_NONE;
    }

    return (enum d_smtp_auth)d_internal_smtp_lookup(
               AUTH_NAMES,
               sizeof(AUTH_NAMES) / sizeof(AUTH_NAMES[0]),
               _name,
               _length,
               D_SMTP_AUTH_NONE);
}

/*
d_internal_smtp_utf8_sequence
  Length of the well-formed UTF-8 sequence at `_text`, or 0 when malformed.
The second byte's range is narrowed after E0, ED, F0 and F4, which is what
rules out overlong forms, surrogates, and code points above U+10FFFF.
*/
static size_t
d_internal_smtp_utf8_sequence(
    const unsigned char* _text,
    size_t               _length
)
{
    const unsigned int lead = _text[0];

    if (lead < 0x80u)
    {
        return 1;
    }

    const size_t needed = ( (lead >= 0xC2u) && (lead <= 0xDFu) ) ? 2
                        : ( (lead >= 0xE0u) && (lead <= 0xEFu) ) ? 3
                        : ( (lead >= 0xF0u) && (lead <= 0xF4u) ) ? 4
                        : 0;

    if ( (needed == 0) ||
         (needed > _length) )
    {
        return 0;
    }

    const unsigned int low  = (lead == 0xE0u) ? 0xA0u
                            : (lead == 0xF0u) ? 0x90u
                            : 0x80u;
    const unsigned int high = (lead == 0xEDu) ? 0x9Fu
                            : (lead == 0xF4u) ? 0x8Fu
                            : 0xBFu;

    if ( (_text[1] < low) ||
         (_text[1] > high) )
    {
        return 0;
    }

    // the remaining bytes are plain continuation bytes
    for (size_t i = 2; i < needed; ++i)
    {
        if ( (_text[i] < 0x80u) ||
             (_text[i] > 0xBFu) )
        {
            return 0;
        }
    }

    return needed;
}

/*
d_internal_smtp_utf8_valid
  Whole-span UTF-8 well-formedness, one sequence at a time.
*/
static bool
d_internal_smtp_utf8_valid(
    const char* _text,
    size_t      _length
)
{
    const unsigned char* bytes    = (const unsigned char*)_text;
    size_t               position = 0;

    while (position < _length)
    {
        const size_t step = d_internal_smtp_utf8_sequence(bytes + position,
                                                          _length - position);

        if (step == 0)
        {
            return false;
        }

        position += step;
    }

    return true;
}

/*
d_internal_smtp_is_atext
  RFC 5322 atext: letters, digits and the specials; under SMTPUTF8, also the
bytes of UTF-8 sequences (their well-formedness is checked separately).
*/
static bool
d_internal_smtp_is_atext(
    char _character,
    bool _allow_utf8
)
{
    const unsigned char octet = (unsigned char)_character;

    if ( (_allow_utf8) &&
         (octet >= 0x80u) )
    {
        return true;
    }

    return ( (d_net_ascii_is_alnum(_character)) ||
             ( (octet != 0) &&
               (strchr(ATEXT_SPECIALS,
                       octet) != NULL) ) );
}

/*
d_internal_smtp_quoted_local
  Length of a quoted-string local-part (RFC 5321 Quoted-string), closing quote
included, or 0. qtextSMTP is printable ASCII except '"' and '\', and a
backslash quotes exactly one printable ASCII character.
*/
static size_t
d_internal_smtp_quoted_local(
    const char* _text,
    size_t      _length,
    bool        _allow_utf8
)
{
    size_t position = 1;

    while (position < _length)
    {
        const unsigned char octet = (unsigned char)_text[position];

        // the closing quote ends the local-part
        if (octet == '"')
        {
            return position + 1;
        }

        // quoted-pairSMTP: a backslash and one printable ASCII character
        if (octet == '\\')
        {
            if ( (position + 1 >= _length)          ||
                 (_text[position + 1] < 32)         ||
                 (_text[position + 1] > 126) )
            {
                return 0;
            }

            position += 2;

            continue;
        }

        if ( ( (octet < 32u) ||
               (octet > 126u) ) &&
             ( (!_allow_utf8) ||
               (octet < 0x80u) ) )
        {
            return 0;
        }

        ++position;
    }

    // no closing quote
    return 0;
}

/*
d_internal_smtp_local_part
  Length of the local-part at the start of an address -- a quoted string or a
dot-string of non-empty atoms -- or 0 when it is malformed.
*/
static size_t
d_internal_smtp_local_part(
    const char* _text,
    size_t      _length,
    bool        _allow_utf8
)
{
    if (_text[0] == '"')
    {
        return d_internal_smtp_quoted_local(_text,
                                            _length,
                                            _allow_utf8);
    }

    size_t position = 0;
    size_t atom     = 0;

    // atoms separated by single dots, ending at '@'
    while ( (position < _length) &&
            (_text[position] != '@') )
    {
        if (_text[position] == '.')
        {
            // an empty atom: a leading or doubled dot
            if (atom == 0)
            {
                return 0;
            }

            atom = 0;
        }
        else if (d_internal_smtp_is_atext(_text[position],
                                          _allow_utf8))
        {
            ++atom;
        }
        else
        {
            return 0;
        }

        ++position;
    }

    // a trailing dot leaves the last atom empty
    return (atom == 0) ? 0 : position;
}

/*
d_internal_smtp_ipv4_literal
  Dotted-quad check: four decimal groups of 1 to 3 digits, each at most 255.
*/
static bool
d_internal_smtp_ipv4_literal(
    const char* _text,
    size_t      _length
)
{
    size_t position = 0;

    for (size_t group = 0; group < 4; ++group)
    {
        size_t       digits = 0;
        unsigned int value  = 0;

        while ( (position < _length)                    &&
                (digits < 3)                            &&
                (d_net_ascii_is_digit(_text[position])) )
        {
            value = (value * 10u) + (unsigned int)(_text[position] - '0');
            ++position;
            ++digits;
        }

        if ( (digits == 0) ||
             (value > 255u) )
        {
            return false;
        }

        // groups are separated by dots; the last ends the literal
        if (group < 3)
        {
            if ( (position >= _length) ||
                 (_text[position] != '.') )
            {
                return false;
            }

            ++position;
        }
    }

    return (position == _length);
}

/*
d_internal_smtp_address_literal
  "[a.b.c.d]" or "[IPv6:...]". The IPv6 form is checked for its alphabet (hex
digits, ':' and '.') rather than parsed in full; the peer that sent it is the
authority on its own address.
*/
static bool
d_internal_smtp_address_literal(
    const char* _text,
    size_t      _length
)
{
    if ( (_length < 3)                 ||
         (_text[0] != '[')             ||
         (_text[_length - 1] != ']') )
    {
        return false;
    }

    const char*              inner   = _text + 1;
    const size_t             length  = _length - 2;
    const struct d_pack_text literal = { inner,
                                         length };

    // IPv6 literals carry a tag
    if ( (length > 5) &&
         (d_net_ascii_istarts(literal,
                              "IPv6:")) )
    {
        // after it, hex digits in either case, ':' and '.' only
        for (size_t i = 5; i < length; ++i)
        {
            // any other character refuses the literal
            if ( (!d_net_hex_is_digit(inner[i])) &&
                 (inner[i] != ':')               &&
                 (inner[i] != '.') )
            {
                return false;
            }
        }

        return (length > 6);
    }

    return d_internal_smtp_ipv4_literal(inner,
                                        length);
}

/*
d_smtp_domain_is_valid
  An address literal goes to its own parser. Otherwise the name is
read in one pass as labels of 1 to 63 characters -- letters, digits, and
hyphens neither leading nor trailing -- joined by single dots; with UTF-8
allowed, bytes from 0x80 count as letters once the whole name has been
checked as valid UTF-8.
*/
bool
d_smtp_domain_is_valid(
    const char* _domain,
    size_t      _length,
    bool        _allow_utf8
)
{
    // parameter validation first
    if ( (!_domain)                 ||
         (_length == 0)             ||
         (_length > D_SMTP_DOMAIN_MAX) )
    {
        return false;
    }

    if (_domain[0] == '[')
    {
        return d_internal_smtp_address_literal(_domain,
                                               _length);
    }

    if ( (_allow_utf8) &&
         (!d_internal_smtp_utf8_valid(_domain,
                                      _length)) )
    {
        return false;
    }

    size_t label = 0;

    // labels of 1 to 63 letters, digits and inner hyphens, joined by dots
    for (size_t i = 0; i < _length; ++i)
    {
        const char character = _domain[i];

        if (character == '.')
        {
            if ( (label == 0) ||
                 (_domain[i - 1] == '-') )
            {
                return false;
            }

            label = 0;

            continue;
        }

        const bool allowed = ( (d_net_ascii_is_alnum(character)) ||
                               ( (character == '-') &&
                                 (label > 0) )                   ||
                               ( (_allow_utf8) &&
                                 ((unsigned char)character >= 0x80u) ) );

        if ( (!allowed) ||
             (++label > 63u) )
        {
            return false;
        }
    }

    return ( (label > 0) &&
             (_domain[_length - 1] != '-') );
}

/*
d_smtp_address_is_valid
  The local-part is measured first, by its own grammar; then it must
be followed by "@" and a domain that d_smtp_domain_is_valid() accepts.
The path's limit leaves room for the angle brackets.
*/
bool
d_smtp_address_is_valid(
    const char* _address,
    size_t      _length,
    bool        _allow_utf8
)
{
    // parameter validation first
    if ( (!_address)                         ||
         (_length == 0)                      ||
         (_length > D_SMTP_PATH_MAX - 2u) )
    {
        return false;
    }

    if ( (_allow_utf8) &&
         (!d_internal_smtp_utf8_valid(_address,
                                      _length)) )
    {
        return false;
    }

    const size_t local = d_internal_smtp_local_part(_address,
                                                    _length,
                                                    _allow_utf8);

    // the local-part is followed by "@" and a domain
    if ( (local == 0)                       ||
         (local > D_SMTP_LOCAL_PART_MAX)    ||
         (local + 1 >= _length)             ||
         (_address[local] != '@') )
    {
        return false;
    }

    return d_smtp_domain_is_valid(_address + local + 1,
                                  _length - local - 1,
                                  _allow_utf8);
}

/*
d_smtp_command_parse
  The verb runs to the first space; the argument is the rest of the line with
leading and trailing spaces removed.
*/
enum d_smtp_error
d_smtp_command_parse(
    const char*            _line,
    size_t                 _length,
    struct d_smtp_command* _command
)
{
    // parameter validation first
    if ( (!_line) ||
         (!_command) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    size_t verb = 0;

    while ( (verb < _length) &&
            (_line[verb] != ' ') )
    {
        ++verb;
    }

    size_t start = verb;
    size_t end   = _length;

    while ( (start < _length) &&
            (_line[start] == ' ') )
    {
        ++start;
    }

    while ( (end > start) &&
            (_line[end - 1] == ' ') )
    {
        --end;
    }

    _command->verb = (enum d_smtp_verb)d_internal_smtp_lookup(
                         VERB_NAMES,
                         sizeof(VERB_NAMES) / sizeof(VERB_NAMES[0]),
                         _line,
                         verb,
                         D_SMTP_VERB_UNKNOWN);
    _command->argument        = _line + start;
    _command->argument_length = end - start;

    return D_SMTP_OK;
}

/*
d_internal_smtp_path_end
  Index of the '>' that closes a path opened just before `_start`, or
`_length` when there is none. Quoted local-parts may contain '>' and escaped
quotes, so the scan tracks quoting.
*/
static size_t
d_internal_smtp_path_end(
    const char* _text,
    size_t      _length,
    size_t      _start
)
{
    bool quoted = false;

    for (size_t i = _start; i < _length; ++i)
    {
        // a quoted backslash protects the next character
        if ( (quoted) &&
             (_text[i] == '\\') )
        {
            ++i;

            continue;
        }

        if (_text[i] == '"')
        {
            quoted = !quoted;
        }
        else if ( (!quoted) &&
                  (_text[i] == '>') )
        {
            return i;
        }
    }

    return _length;
}

/*
d_internal_smtp_path_route
  Moves the address's start past a source route ("@a,@b:"), which RFC 5321
section 4.1.1.3 says to accept and ignore, through its colon. False when a
route has no colon before the closing bracket.
*/
static bool
d_internal_smtp_path_route(
    const char* _argument,
    size_t      _close,
    size_t*     _address
)
{
    // only an address that opens with '@' carries a route
    if ( (*_address >= _close) ||
         (_argument[*_address] != '@') )
    {
        return true;
    }

    const char* colon = memchr(_argument + *_address,
                               ':',
                               _close - *_address);

    if (!colon)
    {
        return false;
    }

    *_address = (size_t)(colon - _argument) + 1;

    return true;
}

/*
d_internal_smtp_path_store
  Fills the path from the brackets' contents and what follows them: the
parameters, separated from the path by at least one space, or none.
*/
static enum d_smtp_error
d_internal_smtp_path_store(
    const char*         _argument,
    size_t              _length,
    size_t              _address,
    size_t              _close,
    struct d_smtp_path* _path
)
{
    size_t parameters = _close + 1;

    // parameters are separated from the path by at least one space
    if ( (parameters < _length) &&
         (_argument[parameters] != ' ') )
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    while ( (parameters < _length) &&
            (_argument[parameters] == ' ') )
    {
        ++parameters;
    }

    _path->address           = _argument + _address;
    _path->address_length    = _close - _address;
    _path->parameters        = (parameters < _length) ? _argument + parameters
                                                      : NULL;
    _path->parameters_length = _length - parameters;

    return D_SMTP_OK;
}

/*
d_smtp_path_parse
  The keyword, a space some clients put after it, then the bracketed path:
its end is the first '>' outside a quoted local part, and a source route
inside it is skipped. What follows the brackets is the parameters.
*/
enum d_smtp_error
d_smtp_path_parse(
    const char*         _argument,
    size_t              _length,
    const char*         _keyword,
    struct d_smtp_path* _path
)
{
    // parameter validation first
    if ( (!_argument) ||
         (!_keyword)  ||
         (!_path) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    const struct d_pack_text argument = { _argument,
                                          _length };
    const size_t             keyword  = strlen(_keyword);

    // "FROM:" or "TO:", in any case, opens the argument
    if (!d_net_ascii_istarts(argument,
                             _keyword))
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    size_t open = keyword;

    // tolerate the space some clients put after the colon
    while ( (open < _length) &&
            (_argument[open] == ' ') )
    {
        ++open;
    }

    if ( (open >= _length) ||
         (_argument[open] != '<') )
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    const size_t close   = d_internal_smtp_path_end(_argument,
                                                    _length,
                                                    open + 1);
    size_t       address = open + 1;

    if ( (close >= _length) ||
         (!d_internal_smtp_path_route(_argument,
                                      close,
                                      &address)) )
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    return d_internal_smtp_path_store(_argument,
                                      _length,
                                      address,
                                      close,
                                      _path);
}

/*
d_smtp_parameter_next
  Skips spaces, then takes the next space-delimited word; its first
"=", if any, splits keyword from value. The cursor stops after the word, so
repeated calls walk the list; nothing is copied.
*/
bool
d_smtp_parameter_next(
    const char**             _cursor,
    const char*              _end,
    struct d_smtp_parameter* _parameter
)
{
    // parameter validation first
    if ( (!_cursor)     ||
         (!*_cursor)    ||
         (!_end)        ||
         (!_parameter) )
    {
        return false;
    }

    const char* start = *_cursor;

    while ( (start < _end) &&
            (*start == ' ') )
    {
        ++start;
    }

    // nothing but spaces remains
    if (start >= _end)
    {
        *_cursor = _end;

        return false;
    }

    const char* stop = start;

    while ( (stop < _end) &&
            (*stop != ' ') )
    {
        ++stop;
    }

    const char* equals = memchr(start,
                                '=',
                                (size_t)(stop - start));

    _parameter->keyword        = start;
    _parameter->keyword_length = (size_t)(((equals) ? equals : stop) - start);
    _parameter->value          = (equals) ? equals + 1 : NULL;
    _parameter->value_length   = (equals) ? (size_t)(stop - equals - 1) : 0;
    *_cursor                   = stop;

    return true;
}

/*
d_smtp_buffer_append_base64
  The codec writes the encoding straight into the buffer's free space, its NUL
included; a refusal writes only a NUL over the one already there, so the
buffer is as it was.
*/
enum d_smtp_error
d_smtp_buffer_append_base64(
    struct d_smtp_buffer* _buffer,
    const void*           _data,
    size_t                _length
)
{
    // parameter validation first
    if ( (!_buffer) ||
         ( (!_data) &&
           (_length > 0) ) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    // a buffer without storage has no room even for the NUL
    if (_buffer->capacity == 0)
    {
        return D_SMTP_ERROR_LINE_TOO_LONG;
    }

    const struct d_pack_bytes    data    = { _data,
                                             _length };
    size_t                       written = 0;
    const enum d_net_codec_error status  =
        d_net_base64_encode(data,
                            D_NET_BASE64_STANDARD,
                            _buffer->data + _buffer->length,
                            _buffer->capacity - _buffer->length,
                            &written);

    // a success grows the buffer; a refusal says why
    if (status == D_NET_CODEC_OK)
    {
        _buffer->length += written;

        return D_SMTP_OK;
    }

    return (status == D_NET_CODEC_ERROR_BUFFER_TOO_SMALL)
               ? D_SMTP_ERROR_LINE_TOO_LONG
               : D_SMTP_ERROR_INVALID_ARGUMENT;
}

/*
d_smtp_buffer_append_from_base64
  Decodes into the free space less the byte held for the NUL; the codec judges
the whole text before writing, so a refusal leaves the buffer untouched.
*/
enum d_smtp_error
d_smtp_buffer_append_from_base64(
    struct d_smtp_buffer* _buffer,
    const char*           _text,
    size_t                _length
)
{
    // parameter validation first
    if ( (!_text) ||
         (!_buffer) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    // a buffer without storage has no room even for the NUL
    if (_buffer->capacity == 0)
    {
        return D_SMTP_ERROR_LINE_TOO_LONG;
    }

    const struct d_pack_text     text    = { _text,
                                             _length };
    size_t                       decoded = 0;
    const enum d_net_codec_error status  =
        d_net_base64_decode(text,
                            D_NET_BASE64_STANDARD,
                            _buffer->data + _buffer->length,
                            _buffer->capacity - _buffer->length - 1u,
                            &decoded);

    // a success grows the buffer and ends it with a NUL
    if (status == D_NET_CODEC_OK)
    {
        _buffer->length               += decoded;
        _buffer->data[_buffer->length] = '\0';

        return D_SMTP_OK;
    }

    // text that is not base64 is the peer's fault
    if (status == D_NET_CODEC_ERROR_MALFORMED)
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    return (status == D_NET_CODEC_ERROR_BUFFER_TOO_SMALL)
               ? D_SMTP_ERROR_LINE_TOO_LONG
               : D_SMTP_ERROR_INVALID_ARGUMENT;
}

/*
d_smtp_data_encoder_init
  A new message starts at the beginning of a line, after no CR.
*/
void
d_smtp_data_encoder_init(
    struct d_smtp_data_encoder* _encoder
)
{
    // parameter validation first
    if (!_encoder)
    {
        return;
    }

    _encoder->line_start = true;
    _encoder->after_cr   = false;

    return;
}

/*
d_internal_smtp_encode_byte
  One byte of the transparency procedure. CR, LF and CRLF all become CRLF: a
CR is emitted as CRLF at once, and the LF that may follow it is absorbed. A
dot that begins a line is doubled (RFC 5321 section 4.5.2). The caller
guarantees two free bytes.
*/
static void
d_internal_smtp_encode_byte(
    struct d_smtp_data_encoder* _encoder,
    char                        _character,
    struct d_smtp_buffer*       _buffer
)
{
    char* output = _buffer->data + _buffer->length;

    // the LF of a CRLF whose CR was already sent is absorbed
    if ( (_character == '\n') &&
         (_encoder->after_cr) )
    {
        _encoder->after_cr = false;

        return;
    }

    _encoder->after_cr = (_character == '\r');

    // any line ending becomes CRLF
    if ( (_character == '\r') ||
         (_character == '\n') )
    {
        output[0]            = '\r';
        output[1]            = '\n';
        _buffer->length     += 2;
        _encoder->line_start = true;

        return;
    }

    size_t used = 0;

    // transparency: a leading dot is doubled
    if ( (_encoder->line_start) &&
         (_character == '.') )
    {
        output[used++] = '.';
    }

    output[used++]       = _character;
    _buffer->length     += used;
    _encoder->line_start = false;

    return;
}

/*
d_smtp_data_encode
  Encodes byte by byte while the buffer has room for the two bytes a
byte can become plus the terminator; what is left is for the next call,
and the encoder's state carries line starts across calls.
*/
size_t
d_smtp_data_encode(
    struct d_smtp_data_encoder* _encoder,
    const char*                 _input,
    size_t                      _length,
    struct d_smtp_buffer*       _buffer
)
{
    // parameter validation first
    if ( (!_encoder)               ||
         (!_input)                 ||
         (!_buffer)                ||
         (_buffer->capacity == 0) )
    {
        return 0;
    }

    size_t consumed = 0;

    // each byte needs up to two output bytes, plus the reserved NUL
    while ( (consumed < _length) &&
            (_buffer->capacity - _buffer->length > 2) )
    {
        d_internal_smtp_encode_byte(_encoder,
                                    _input[consumed],
                                    _buffer);
        ++consumed;
    }

    _buffer->data[_buffer->length] = '\0';

    return consumed;
}

/*
d_smtp_data_finish
  The end-of-data marker needs a line start: ".\r\n" when the message
ended at one, "\r\n.\r\n" otherwise. The encoder is reset only once the
marker fits.
*/
enum d_smtp_error
d_smtp_data_finish(
    struct d_smtp_data_encoder* _encoder,
    struct d_smtp_buffer*       _buffer
)
{
    // parameter validation first
    if ( (!_encoder) ||
         (!_buffer) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    // an unterminated last line gets its CRLF before the lone dot
    const char* marker = (_encoder->line_start) ? ".\r\n" : "\r\n.\r\n";
    const enum d_smtp_error result = d_smtp_buffer_append_text(_buffer,
                                                               marker);

    if (result == D_SMTP_OK)
    {
        d_smtp_data_encoder_init(_encoder);
    }

    return result;
}

/*
d_smtp_data_unstuff
  A lone "." ends the data; otherwise one leading dot is the sender's
stuffing, dropped from the payload, which points into the line.
*/
bool
d_smtp_data_unstuff(
    const char*  _line,
    size_t       _length,
    const char** _payload,
    size_t*      _payload_length
)
{
    // a lone dot ends the data
    if ( (_length == 1) &&
         (_line[0] == '.') )
    {
        *_payload        = _line + 1;
        *_payload_length = 0;

        return true;
    }

    // otherwise one leading dot is transparency, not content
    const size_t skip = ( (_length > 0) &&
                          (_line[0] == '.') ) ? 1 : 0;

    *_payload        = _line + skip;
    *_payload_length = _length - skip;

    return false;
}
