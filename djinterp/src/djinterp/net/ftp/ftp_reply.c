/*******************************************************************************
* djinterp [net]                                                     ftp_reply.c
*
* Implementation of the reply handling declared in ftp_reply.h.
*   The parser holds each line's first four bytes back until the line shows
* whether they are the reply's code, so a reply's text never carries the
* prefixes of its own continuation lines, and chunks may split anywhere.
*
*
* path:      /src/djinterp/net/ftp/ftp_reply.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_reply.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <string.h>   // memcmp, memcpy, memset, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"          // framework root
#include "../../../../inc/djinterp/net/ftp/ftp_common.h"  // d_ftp_error
#include "./ftp_internal.h"                               // shared helpers


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_ftp_internal_line
//   enum: how a finished line leaves the reply being parsed.
enum d_ftp_internal_line
{
    D_FTP_INTERNAL_LINE_MORE = 0,  // the reply continues
    D_FTP_INTERNAL_LINE_LAST,      // the reply is complete
    D_FTP_INTERNAL_LINE_BAD        // the line cannot belong to a reply
};

//==============================================================================
// 2.  REPLIES
//==============================================================================

/*
d_ftp_reply_is_valid
  Strict: the class digit 1 through 6 and the category digit 0 through 5.
*/
bool
d_ftp_reply_is_valid(
    unsigned _code
)
{
    return ( (d_ftp_reply_class_of(_code) != D_FTP_REPLY_CLASS_INVALID) &&
             (d_ftp_reply_category_of(_code) !=
              D_FTP_REPLY_CATEGORY_INVALID) );
}

/*
d_ftp_reply_class_of
  The enumerators equal their digits, so the class is a cast once the range
is checked.
*/
enum d_ftp_reply_class
d_ftp_reply_class_of(
    unsigned _code
)
{
    // three digits, the first of them 1 through 6
    if ( (_code < 100u) ||
         (_code > 699u) )
    {
        return D_FTP_REPLY_CLASS_INVALID;
    }

    return (enum d_ftp_reply_class)(_code / 100u);
}

/*
d_ftp_reply_category_of
  As d_ftp_reply_class_of(), for the second digit.
*/
enum d_ftp_reply_category
d_ftp_reply_category_of(
    unsigned _code
)
{
    const unsigned second = (_code / 10u) % 10u;

    // a category exists only within a valid class, and stops at 5
    if ( (d_ftp_reply_class_of(_code) == D_FTP_REPLY_CLASS_INVALID) ||
         (second > 5u) )
    {
        return D_FTP_REPLY_CATEGORY_INVALID;
    }

    return (enum d_ftp_reply_category)second;
}

/*
d_ftp_reply_text
  The texts follow RFC 959 4.2.2 and RFC 2228 wording where those give one.
*/
const char*
d_ftp_reply_text(
    unsigned _code
)
{
    switch (_code)
    {
        case D_FTP_REPLY_RESTART_MARKER:
            return "Restart marker reply.";
        case D_FTP_REPLY_SERVICE_READY_IN:
            return "Service ready in a few minutes.";
        case D_FTP_REPLY_TRANSFER_STARTING:
            return "Data connection already open; transfer starting.";
        case D_FTP_REPLY_OPENING_DATA:
            return "File status okay; about to open data connection.";
        case D_FTP_REPLY_COMMAND_OK:
            return "Command okay.";
        case D_FTP_REPLY_SUPERFLUOUS:
            return "Command not implemented, superfluous at this site.";
        case D_FTP_REPLY_SYSTEM_STATUS:
            return "System status.";
        case D_FTP_REPLY_DIRECTORY_STATUS:
            return "Directory status.";
        case D_FTP_REPLY_FILE_STATUS:
            return "File status.";
        case D_FTP_REPLY_HELP:
            return "Help message.";
        case D_FTP_REPLY_SYSTEM_TYPE:
            return "System type.";
        case D_FTP_REPLY_SERVICE_READY:
            return "Service ready for new user.";
        case D_FTP_REPLY_CLOSING_CONTROL:
            return "Service closing control connection.";
        case D_FTP_REPLY_DATA_OPEN:
            return "Data connection open; no transfer in progress.";
        case D_FTP_REPLY_CLOSING_DATA:
            return "Closing data connection.";
        case D_FTP_REPLY_PASSIVE:
            return "Entering Passive Mode.";
        case D_FTP_REPLY_LONG_PASSIVE:
            return "Entering Long Passive Mode.";
        case D_FTP_REPLY_EXTENDED_PASSIVE:
            return "Entering Extended Passive Mode.";
        case D_FTP_REPLY_LOGGED_IN:
            return "User logged in, proceed.";
        case D_FTP_REPLY_SECURITY_LOGGED_IN:
            return "User logged in, authorized by security data exchange.";
        case D_FTP_REPLY_SECURITY_ACCEPTED:
            return "Security data exchange complete.";
        case D_FTP_REPLY_SECURITY_DATA_DONE:
            return "Security data exchange completed successfully.";
        case D_FTP_REPLY_FILE_ACTION_OK:
            return "Requested file action okay, completed.";
        case D_FTP_REPLY_PATHNAME:
            return "Pathname created.";
        case D_FTP_REPLY_NEED_PASSWORD:
            return "User name okay, need password.";
        case D_FTP_REPLY_NEED_ACCOUNT:
            return "Need account for login.";
        case D_FTP_REPLY_SECURITY_MECHANISM_OK:
            return "Security mechanism accepted; send security data.";
        case D_FTP_REPLY_SECURITY_DATA_MORE:
            return "Security data acceptable; more is required.";
        case D_FTP_REPLY_NEED_PASSWORD_CHALLENGE:
            return "User name okay, need password; challenge follows.";
        case D_FTP_REPLY_FILE_ACTION_PENDING:
            return "Requested file action pending further information.";
        case D_FTP_REPLY_SERVICE_UNAVAILABLE:
            return "Service not available, closing control connection.";
        case D_FTP_REPLY_CANNOT_OPEN_DATA:
            return "Can't open data connection.";
        case D_FTP_REPLY_TRANSFER_ABORTED:
            return "Connection closed; transfer aborted.";
        case D_FTP_REPLY_SECURITY_RESOURCE:
            return "Need some unavailable resource to process security.";
        case D_FTP_REPLY_FILE_BUSY:
            return "Requested file action not taken; file unavailable.";
        case D_FTP_REPLY_LOCAL_ERROR:
            return "Requested action aborted: local error in processing.";
        case D_FTP_REPLY_INSUFFICIENT_STORAGE:
            return "Requested action not taken; insufficient storage.";
        case D_FTP_REPLY_SYNTAX_ERROR:
            return "Syntax error, command unrecognized.";
        case D_FTP_REPLY_ARGUMENT_ERROR:
            return "Syntax error in parameters or arguments.";
        case D_FTP_REPLY_NOT_IMPLEMENTED:
            return "Command not implemented.";
        case D_FTP_REPLY_BAD_SEQUENCE:
            return "Bad sequence of commands.";
        case D_FTP_REPLY_PARAMETER_UNSUPPORTED:
            return "Command not implemented for that parameter.";
        case D_FTP_REPLY_PROTOCOL_UNSUPPORTED:
            return "Network protocol not supported.";
        case D_FTP_REPLY_NOT_LOGGED_IN:
            return "Not logged in.";
        case D_FTP_REPLY_NEED_ACCOUNT_TO_STORE:
            return "Need account for storing files.";
        case D_FTP_REPLY_PROTECTION_DENIED:
            return "Command protection level denied for policy reasons.";
        case D_FTP_REPLY_POLICY_DENIED:
            return "Request denied for policy reasons.";
        case D_FTP_REPLY_SECURITY_CHECK_FAILED:
            return "Failed security check.";
        case D_FTP_REPLY_PROT_UNSUPPORTED:
            return "Requested PROT level not supported by mechanism.";
        case D_FTP_REPLY_COMMAND_PROT_UNSUPPORTED:
            return "Command protection level not supported.";
        case D_FTP_REPLY_FILE_UNAVAILABLE:
            return "Requested action not taken; file unavailable.";
        case D_FTP_REPLY_PAGE_TYPE_UNKNOWN:
            return "Requested action aborted: page type unknown.";
        case D_FTP_REPLY_STORAGE_EXCEEDED:
            return "Requested file action aborted: storage exceeded.";
        case D_FTP_REPLY_NAME_NOT_ALLOWED:
            return "Requested action not taken; file name not allowed.";
        case D_FTP_REPLY_INTEGRITY_PROTECTED:
            return "Integrity protected reply.";
        case D_FTP_REPLY_PRIVATE_PROTECTED:
            return "Confidentiality and integrity protected reply.";
        case D_FTP_REPLY_CONFIDENTIAL_PROTECTED:
            return "Confidentiality protected reply.";
        default:
            break;
    }

    return NULL;
}

/*
d_ftp_internal_reply_clear
  File-local: forgets the reply in progress. The stored text is left in place,
so a reply just handed out stays readable until new bytes overwrite it; the
Telnet state is kept, since a sequence may straddle two replies.
*/
D_STATIC void
d_ftp_internal_reply_clear(
    struct d_ftp_reply_parser* _parser
)
{
    _parser->length      = 0u;
    _parser->line_length = 0u;
    _parser->line_count  = 0u;
    _parser->code        = 0u;
    _parser->truncated   = false;

    return;
}

/*
d_ftp_internal_reply_store
  File-local: keeps one byte of reply text, or marks the reply truncated. The
last byte of storage is reserved for the terminator.
*/
D_STATIC void
d_ftp_internal_reply_store(
    struct d_ftp_reply_parser* _parser,
    char                       _c
)
{
    // no room: the text is cut, but parsing goes on
    if ( (_parser->capacity == 0u) ||
         (_parser->length >= (_parser->capacity - 1u)) )
    {
        _parser->truncated = true;

        return;
    }

    _parser->storage[_parser->length] = _c;
    _parser->length++;

    return;
}

/*
d_ftp_internal_reply_repeats_code
  File-local: reports whether the first `_count` bytes of a later line repeat
the reply's code, bare or followed by a space or hyphen -- the prefix that
marks the line as the reply's own rather than as text.
*/
D_STATIC bool
d_ftp_internal_reply_repeats_code(
    const struct d_ftp_reply_parser* _parser,
    size_t                           _count
)
{
    // the code is three digits, so a shorter line cannot repeat it
    if (_count < 3u)
    {
        return false;
    }

    const bool same = (memcmp(_parser->prefix,
                              _parser->code_text,
                              3u) == 0);

    return ( (same) &&
             ( (_count == 3u)              ||
               (_parser->prefix[3] == ' ') ||
               (_parser->prefix[3] == '-') ) );
}

/*
d_ftp_internal_reply_settle
  File-local: decides what a line's first `_count` bytes were once the line
shows it. On the first line they are the code and separator; on a later line
they are text unless they repeat the code.
*/
D_STATIC void
d_ftp_internal_reply_settle(
    struct d_ftp_reply_parser* _parser,
    size_t                     _count
)
{
    // the first line's prefix is its code, never text
    if (_parser->line_count == 0u)
    {
        return;
    }

    // a later line repeating the code carries it as a marker
    if (d_ftp_internal_reply_repeats_code(_parser,
                                          _count))
    {
        return;
    }

    // otherwise the bytes were text all along
    for (size_t index = 0; index < _count; index++)
    {
        d_ftp_internal_reply_store(_parser,
                                   _parser->prefix[index]);
    }

    return;
}

/*
d_ftp_internal_reply_take
  File-local: accepts one text byte of the current line. The first four wait
in `prefix` until the fifth, or the line's end, shows whether they were a
code; every later byte is stored directly.
*/
D_STATIC void
d_ftp_internal_reply_take(
    struct d_ftp_reply_parser* _parser,
    char                       _c
)
{
    // the first four bytes wait
    if (_parser->line_length < 4u)
    {
        _parser->prefix[_parser->line_length] = _c;
        _parser->line_length++;

        return;
    }

    // the fifth byte settles them
    if (_parser->line_length == 4u)
    {
        d_ftp_internal_reply_settle(_parser,
                                    4u);
    }

    _parser->line_length++;
    d_ftp_internal_reply_store(_parser,
                               _c);

    return;
}

/*
d_ftp_internal_reply_open
  File-local: reads the first line's code and separator. The class digit must
be 1 through 6; the other two are only required to be digits, since servers
occasionally send categories RFC 959 does not define.
*/
D_STATIC enum d_ftp_internal_line
d_ftp_internal_reply_open(
    struct d_ftp_reply_parser* _parser,
    size_t                     _count
)
{
    const char* const prefix = _parser->prefix;

    // a reply opens with a three-digit code
    if ( (_count < 3u)                          ||
         (prefix[0] < '1')                      ||
         (prefix[0] > '6')                      ||
         (!d_ftp_internal_is_digit(prefix[1])) ||
         (!d_ftp_internal_is_digit(prefix[2])) )
    {
        return D_FTP_INTERNAL_LINE_BAD;
    }

    memcpy(_parser->code_text,
           prefix,
           3u);

    _parser->code       = d_ftp_internal_digits_value(prefix,
                                                      3u);
    _parser->line_count = 1u;

    // a bare code, or code and space, is a single-line reply
    if ( (_count == 3u) ||
         (prefix[3] == ' ') )
    {
        return D_FTP_INTERNAL_LINE_LAST;
    }

    // code and hyphen announce more lines
    if (prefix[3] == '-')
    {
        d_ftp_internal_reply_store(_parser,
                                   '\n');

        return D_FTP_INTERNAL_LINE_MORE;
    }

    return D_FTP_INTERNAL_LINE_BAD;
}

/*
d_ftp_internal_reply_end_line
  File-local: finishes the current line. The reply ends at the first later
line that repeats the code followed by a space, or bare (RFC 959 4.2); every
other line is text, joined to the next by '\n'.
*/
D_STATIC enum d_ftp_internal_line
d_ftp_internal_reply_end_line(
    struct d_ftp_reply_parser* _parser
)
{
    const size_t count = _parser->line_length;

    // a line of four bytes or fewer never reached a fifth to settle it
    if (count <= 4u)
    {
        d_ftp_internal_reply_settle(_parser,
                                    count);
    }

    _parser->line_length = 0u;

    // the first line opens the reply
    if (_parser->line_count == 0u)
    {
        return d_ftp_internal_reply_open(_parser,
                                         count);
    }

    const bool closes = ( (d_ftp_internal_reply_repeats_code(_parser,
                                                             count)) &&
                          ( (count == 3u) ||
                            (_parser->prefix[3] == ' ') ) );

    _parser->line_count++;

    // the closing line ends the reply
    if (closes)
    {
        return D_FTP_INTERNAL_LINE_LAST;
    }

    d_ftp_internal_reply_store(_parser,
                               '\n');

    return D_FTP_INTERNAL_LINE_MORE;
}

/*
d_ftp_reply_parser_init
  Zeroes the parser first so every private field starts defined, whatever
the caller's storage held.
*/
void
d_ftp_reply_parser_init(
    struct d_ftp_reply_parser* _parser,
    char*                      _storage,
    size_t                     _capacity
)
{
    // parameter validation
    if (!_parser)
    {
        return;
    }

    memset(_parser,
           0,
           sizeof(*_parser));

    _parser->storage  = _storage;
    _parser->capacity = (_storage) ? _capacity : 0u;
    _parser->telnet   = (unsigned char)D_FTP_INTERNAL_TELNET_STATE_DATA;

    // an empty reply's text is still a valid empty string
    if (_parser->capacity > 0u)
    {
        _parser->storage[0] = '\0';
    }

    return;
}

/*
d_ftp_reply_parser_reset
  Unlike the clearing between replies, this also drops the Telnet state and
the failure flag: the caller is starting over.
*/
void
d_ftp_reply_parser_reset(
    struct d_ftp_reply_parser* _parser
)
{
    // parameter validation
    if (!_parser)
    {
        return;
    }

    d_ftp_internal_reply_clear(_parser);

    _parser->telnet = (unsigned char)D_FTP_INTERNAL_TELNET_STATE_DATA;
    _parser->failed = false;

    return;
}

/*
d_ftp_reply_parser_feed
  Each byte passes the Telnet filter, then either ends a line (LF), vanishes
(CR and NUL are line control, never text), or joins the line. Stopping right
after a completing LF is what lets pipelined replies come back one per call.
*/
enum d_ftp_status
d_ftp_reply_parser_feed(
    struct d_ftp_reply_parser* _parser,
    const char*                _data,
    size_t                     _length,
    size_t*                    _out_used,
    struct d_ftp_reply*        _out
)
{
    // parameter validation
    if ( (!_parser)   ||
         (!_out_used) ||
         (!_out)      ||
         ( (!_data) &&
           (_length > 0u) ) )
    {
        return D_FTP_STATUS_FAILED;
    }

    *_out_used = 0u;

    // a failed parser stays failed until it is reset
    if (_parser->failed)
    {
        return D_FTP_STATUS_FAILED;
    }

    size_t index = 0;

    // consume until a reply completes or the input runs out
    while (index < _length)
    {
        const unsigned char byte = (unsigned char)_data[index];

        index++;

        // Telnet command sequences are not reply text
        if (!d_ftp_internal_telnet_accept(&_parser->telnet,
                                          byte))
        {
            continue;
        }

        // CR and NUL are line control, never text
        if ( (byte == '\r') ||
             (byte == '\0') )
        {
            continue;
        }

        // every other byte but LF joins the line
        if (byte != '\n')
        {
            d_ftp_internal_reply_take(_parser,
                                      (char)byte);

            continue;
        }

        const enum d_ftp_internal_line line =
            d_ftp_internal_reply_end_line(_parser);

        // a line that cannot belong to a reply poisons the stream
        if (line == D_FTP_INTERNAL_LINE_BAD)
        {
            _parser->failed = true;
            *_out_used      = index;

            return D_FTP_STATUS_FAILED;
        }

        // the closing line hands the reply over
        if (line == D_FTP_INTERNAL_LINE_LAST)
        {
            // the reserved last byte always has room for the terminator
            if (_parser->capacity > 0u)
            {
                _parser->storage[_parser->length] = '\0';
            }

            _out->code        = _parser->code;
            _out->line_count  = _parser->line_count;
            _out->text.data   = _parser->storage;
            _out->text.length = _parser->length;
            _out->truncated   = _parser->truncated;
            *_out_used        = index;

            d_ftp_internal_reply_clear(_parser);

            return D_FTP_STATUS_COMPLETE;
        }
    }

    *_out_used = index;

    return D_FTP_STATUS_PENDING;
}

/*
d_ftp_internal_after_break
  File-local: the position after the line break at `_position`, treating
CR LF as one break; `_length` when no break remains.
*/
D_STATIC size_t
d_ftp_internal_after_break(
    const char* _text,
    size_t      _length,
    size_t      _position
)
{
    // no break left
    if (_position >= _length)
    {
        return _length;
    }

    // CR LF is a single break
    if ( (_text[_position] == '\r')        &&
         ((_position + 1u) < _length)      &&
         (_text[_position + 1u] == '\n') )
    {
        return _position + 2u;
    }

    return _position + 1u;
}

/*
d_ftp_internal_append_line
  File-local: appends a lead, a line of text, and CR LF, all or nothing.
*/
D_STATIC bool
d_ftp_internal_append_line(
    struct d_ftp_buffer* _out,
    const char*          _lead,
    size_t               _lead_length,
    const char*          _text,
    size_t               _text_length
)
{
    // the whole line or none of it
    if ((_lead_length + _text_length + 2u) > d_ftp_internal_room(_out))
    {
        return false;
    }

    d_ftp_internal_append(_out,
                          _lead,
                          _lead_length);
    d_ftp_internal_append(_out,
                          _text,
                          _text_length);
    d_ftp_internal_append(_out,
                          D_FTP_EOL,
                          2u);

    return true;
}

/*
d_ftp_reply_format
  Splits the text at every CR, LF, or CR LF and gives each piece its lead:
the code and a hyphen on the first line of several, the code and a space on
the last, and a single space on any middle line that begins with a digit.
Since only the last line ever carries "ddd ", no text can end the reply
early or forge a second one.
*/
enum d_ftp_error
d_ftp_reply_format(
    unsigned             _code,
    const char*          _text,
    struct d_ftp_buffer* _out
)
{
    // parameter validation
    if ( (!d_ftp_reply_is_valid(_code)) ||
         (!d_ftp_internal_buffer_ok(_out)) )
    {
        return D_FTP_ERROR_INVALID_ARGUMENT;
    }

    const char* const standard  = d_ftp_reply_text(_code);
    const char* const text      = (_text)    ? _text
                                : (standard) ? standard
                                             : "";
    const size_t      length    = strlen(text);
    const size_t      mark      = _out->length;
    const char        digits[3] =
    {
        (char)('0' + (int)(_code / 100u)),
        (char)('0' + (int)((_code / 10u) % 10u)),
        (char)('0' + (int)(_code % 10u))
    };
    size_t            position  = 0;
    bool              first     = true;

    // one reply line per line of text; empty text is one empty line
    while (true)
    {
        size_t end = position;

        // the line runs to the next CR or LF
        while ( (end < length)       &&
                (text[end] != '\r')  &&
                (text[end] != '\n') )
        {
            end++;
        }

        const size_t next    = d_ftp_internal_after_break(text,
                                                          length,
                                                          end);
        const bool   last    = (next >= length);
        char         lead[4] =
        {
            digits[0],
            digits[1],
            digits[2],
            (char)((last) ? ' ' : '-')
        };
        size_t       lead_length = 0;

        // the code opens the first and the last line
        if ( (first) ||
             (last) )
        {
            lead_length = 4u;
        }
        else if ( (end > position) &&
                  (d_ftp_internal_is_digit(text[position])) )
        {
            // a middle line must never look like a closing line
            lead[0]     = ' ';
            lead_length = 1u;
        }

        const bool fits = d_ftp_internal_append_line(_out,
                                                     lead,
                                                     lead_length,
                                                     text + position,
                                                     end - position);

        // a line that does not fit abandons the whole reply
        if (!fits)
        {
            d_ftp_internal_rollback(_out,
                                    mark);

            return D_FTP_ERROR_BUFFER_TOO_SMALL;
        }

        // the last line ends the reply
        if (last)
        {
            break;
        }

        position = next;
        first    = false;
    }

    return D_FTP_OK;
}

/*
d_ftp_error_from_reply
  Class first, then code: the class settles success, and only the negative
codes RFC 959, 2228, and 2428 give a meaning of their own need the table.
*/
enum d_ftp_error
d_ftp_error_from_reply(
    unsigned _code
)
{
    const enum d_ftp_reply_class reply_class = d_ftp_reply_class_of(_code);

    // not a reply code at all
    if (reply_class == D_FTP_REPLY_CLASS_INVALID)
    {
        return D_FTP_ERROR_MALFORMED;
    }

    // every positive class is success
    if ( (reply_class == D_FTP_REPLY_CLASS_PRELIMINARY)  ||
         (reply_class == D_FTP_REPLY_CLASS_COMPLETION)   ||
         (reply_class == D_FTP_REPLY_CLASS_INTERMEDIATE) )
    {
        return D_FTP_OK;
    }

    // a protected reply means nothing until it is unwrapped
    if (reply_class == D_FTP_REPLY_CLASS_PROTECTED)
    {
        return D_FTP_ERROR_UNSUPPORTED;
    }

    // the negative codes with a meaning of their own
    switch (_code)
    {
        case D_FTP_REPLY_SERVICE_UNAVAILABLE:
            return D_FTP_ERROR_SERVICE_UNAVAILABLE;
        case D_FTP_REPLY_CANNOT_OPEN_DATA:
            return D_FTP_ERROR_DATA_CONNECTION;
        case D_FTP_REPLY_TRANSFER_ABORTED:
            return D_FTP_ERROR_TRANSFER_ABORTED;
        case D_FTP_REPLY_FILE_BUSY:
        case D_FTP_REPLY_FILE_UNAVAILABLE:
            return D_FTP_ERROR_FILE_UNAVAILABLE;
        case D_FTP_REPLY_LOCAL_ERROR:
            return D_FTP_ERROR_LOCAL_ERROR;
        case D_FTP_REPLY_INSUFFICIENT_STORAGE:
        case D_FTP_REPLY_STORAGE_EXCEEDED:
            return D_FTP_ERROR_INSUFFICIENT_STORAGE;
        case D_FTP_REPLY_SYNTAX_ERROR:
            return D_FTP_ERROR_COMMAND_UNRECOGNIZED;
        case D_FTP_REPLY_ARGUMENT_ERROR:
            return D_FTP_ERROR_SYNTAX;
        case D_FTP_REPLY_NOT_IMPLEMENTED:
        case D_FTP_REPLY_PARAMETER_UNSUPPORTED:
            return D_FTP_ERROR_NOT_IMPLEMENTED;
        case D_FTP_REPLY_BAD_SEQUENCE:
            return D_FTP_ERROR_BAD_SEQUENCE;
        case D_FTP_REPLY_PROTOCOL_UNSUPPORTED:
            return D_FTP_ERROR_PROTOCOL_UNSUPPORTED;
        case D_FTP_REPLY_NOT_LOGGED_IN:
            return D_FTP_ERROR_LOGIN_DENIED;
        case D_FTP_REPLY_NEED_ACCOUNT_TO_STORE:
            return D_FTP_ERROR_ACCOUNT_REQUIRED;
        case D_FTP_REPLY_PAGE_TYPE_UNKNOWN:
            return D_FTP_ERROR_PAGE_TYPE_UNKNOWN;
        case D_FTP_REPLY_NAME_NOT_ALLOWED:
            return D_FTP_ERROR_NAME_NOT_ALLOWED;
        case D_FTP_REPLY_SECURITY_RESOURCE:
        case D_FTP_REPLY_PROTECTION_DENIED:
        case D_FTP_REPLY_POLICY_DENIED:
        case D_FTP_REPLY_SECURITY_CHECK_FAILED:
        case D_FTP_REPLY_PROT_UNSUPPORTED:
        case D_FTP_REPLY_COMMAND_PROT_UNSUPPORTED:
            return D_FTP_ERROR_SECURITY;
        default:
            break;
    }

    return D_FTP_ERROR_REJECTED;
}
