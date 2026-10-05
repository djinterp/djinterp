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
*                                                            revised: 2026.09.28
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

// d_ftp_internal_reply_text
//   struct: a reply code and its standard text.
struct d_ftp_internal_reply_text
{
    unsigned    code;  // the three-digit code
    const char* text;  // RFC 959 or RFC 2228 wording
};

// REPLY_TEXTS
//   constant: the standard text of every code RFC 959, 2228, and 2428
// define, in RFC 959's order.
static const struct d_ftp_internal_reply_text REPLY_TEXTS[] =
{
    { D_FTP_REPLY_RESTART_MARKER, "Restart marker reply." },
    { D_FTP_REPLY_SERVICE_READY_IN, "Service ready in a few minutes." },
    { D_FTP_REPLY_TRANSFER_STARTING,
      "Data connection already open; transfer starting." },
    { D_FTP_REPLY_OPENING_DATA,
      "File status okay; about to open data connection." },
    { D_FTP_REPLY_COMMAND_OK, "Command okay." },
    { D_FTP_REPLY_SUPERFLUOUS,
      "Command not implemented, superfluous at this site." },
    { D_FTP_REPLY_SYSTEM_STATUS, "System status." },
    { D_FTP_REPLY_DIRECTORY_STATUS, "Directory status." },
    { D_FTP_REPLY_FILE_STATUS, "File status." },
    { D_FTP_REPLY_HELP, "Help message." },
    { D_FTP_REPLY_SYSTEM_TYPE, "System type." },
    { D_FTP_REPLY_SERVICE_READY, "Service ready for new user." },
    { D_FTP_REPLY_CLOSING_CONTROL, "Service closing control connection." },
    { D_FTP_REPLY_DATA_OPEN, "Data connection open; no transfer in progress." },
    { D_FTP_REPLY_CLOSING_DATA, "Closing data connection." },
    { D_FTP_REPLY_PASSIVE, "Entering Passive Mode." },
    { D_FTP_REPLY_LONG_PASSIVE, "Entering Long Passive Mode." },
    { D_FTP_REPLY_EXTENDED_PASSIVE, "Entering Extended Passive Mode." },
    { D_FTP_REPLY_LOGGED_IN, "User logged in, proceed." },
    { D_FTP_REPLY_SECURITY_LOGGED_IN,
      "User logged in, authorized by security data exchange." },
    { D_FTP_REPLY_SECURITY_ACCEPTED, "Security data exchange complete." },
    { D_FTP_REPLY_SECURITY_DATA_DONE,
      "Security data exchange completed successfully." },
    { D_FTP_REPLY_FILE_ACTION_OK, "Requested file action okay, completed." },
    { D_FTP_REPLY_PATHNAME, "Pathname created." },
    { D_FTP_REPLY_NEED_PASSWORD, "User name okay, need password." },
    { D_FTP_REPLY_NEED_ACCOUNT, "Need account for login." },
    { D_FTP_REPLY_SECURITY_MECHANISM_OK,
      "Security mechanism accepted; send security data." },
    { D_FTP_REPLY_SECURITY_DATA_MORE,
      "Security data acceptable; more is required." },
    { D_FTP_REPLY_NEED_PASSWORD_CHALLENGE,
      "User name okay, need password; challenge follows." },
    { D_FTP_REPLY_FILE_ACTION_PENDING,
      "Requested file action pending further information." },
    { D_FTP_REPLY_SERVICE_UNAVAILABLE,
      "Service not available, closing control connection." },
    { D_FTP_REPLY_CANNOT_OPEN_DATA, "Can't open data connection." },
    { D_FTP_REPLY_TRANSFER_ABORTED, "Connection closed; transfer aborted." },
    { D_FTP_REPLY_SECURITY_RESOURCE,
      "Need some unavailable resource to process security." },
    { D_FTP_REPLY_FILE_BUSY,
      "Requested file action not taken; file unavailable." },
    { D_FTP_REPLY_LOCAL_ERROR,
      "Requested action aborted: local error in processing." },
    { D_FTP_REPLY_INSUFFICIENT_STORAGE,
      "Requested action not taken; insufficient storage." },
    { D_FTP_REPLY_SYNTAX_ERROR, "Syntax error, command unrecognized." },
    { D_FTP_REPLY_ARGUMENT_ERROR, "Syntax error in parameters or arguments." },
    { D_FTP_REPLY_NOT_IMPLEMENTED, "Command not implemented." },
    { D_FTP_REPLY_BAD_SEQUENCE, "Bad sequence of commands." },
    { D_FTP_REPLY_PARAMETER_UNSUPPORTED,
      "Command not implemented for that parameter." },
    { D_FTP_REPLY_PROTOCOL_UNSUPPORTED, "Network protocol not supported." },
    { D_FTP_REPLY_NOT_LOGGED_IN, "Not logged in." },
    { D_FTP_REPLY_NEED_ACCOUNT_TO_STORE, "Need account for storing files." },
    { D_FTP_REPLY_PROTECTION_DENIED,
      "Command protection level denied for policy reasons." },
    { D_FTP_REPLY_POLICY_DENIED, "Request denied for policy reasons." },
    { D_FTP_REPLY_SECURITY_CHECK_FAILED, "Failed security check." },
    { D_FTP_REPLY_PROT_UNSUPPORTED,
      "Requested PROT level not supported by mechanism." },
    { D_FTP_REPLY_COMMAND_PROT_UNSUPPORTED,
      "Command protection level not supported." },
    { D_FTP_REPLY_FILE_UNAVAILABLE,
      "Requested action not taken; file unavailable." },
    { D_FTP_REPLY_PAGE_TYPE_UNKNOWN,
      "Requested action aborted: page type unknown." },
    { D_FTP_REPLY_STORAGE_EXCEEDED,
      "Requested file action aborted: storage exceeded." },
    { D_FTP_REPLY_NAME_NOT_ALLOWED,
      "Requested action not taken; file name not allowed." },
    { D_FTP_REPLY_INTEGRITY_PROTECTED, "Integrity protected reply." },
    { D_FTP_REPLY_PRIVATE_PROTECTED,
      "Confidentiality and integrity protected reply." },
    { D_FTP_REPLY_CONFIDENTIAL_PROTECTED, "Confidentiality protected reply." }
};

// d_ftp_internal_reply_error
//   struct: a negative reply code and the error it means.
struct d_ftp_internal_reply_error
{
    unsigned         code;   // the three-digit code
    enum d_ftp_error error;  // what it means to a client
};

// REPLY_ERRORS
//   constant: the negative codes RFC 959, 2228, and 2428 give a meaning of
// their own; every other negative code is D_FTP_ERROR_REJECTED.
static const struct d_ftp_internal_reply_error REPLY_ERRORS[] =
{
    { D_FTP_REPLY_SERVICE_UNAVAILABLE, D_FTP_ERROR_SERVICE_UNAVAILABLE },
    { D_FTP_REPLY_CANNOT_OPEN_DATA, D_FTP_ERROR_DATA_CONNECTION },
    { D_FTP_REPLY_TRANSFER_ABORTED, D_FTP_ERROR_TRANSFER_ABORTED },
    { D_FTP_REPLY_FILE_BUSY, D_FTP_ERROR_FILE_UNAVAILABLE },
    { D_FTP_REPLY_FILE_UNAVAILABLE, D_FTP_ERROR_FILE_UNAVAILABLE },
    { D_FTP_REPLY_LOCAL_ERROR, D_FTP_ERROR_LOCAL_ERROR },
    { D_FTP_REPLY_INSUFFICIENT_STORAGE, D_FTP_ERROR_INSUFFICIENT_STORAGE },
    { D_FTP_REPLY_STORAGE_EXCEEDED, D_FTP_ERROR_INSUFFICIENT_STORAGE },
    { D_FTP_REPLY_SYNTAX_ERROR, D_FTP_ERROR_COMMAND_UNRECOGNIZED },
    { D_FTP_REPLY_ARGUMENT_ERROR, D_FTP_ERROR_SYNTAX },
    { D_FTP_REPLY_NOT_IMPLEMENTED, D_FTP_ERROR_NOT_IMPLEMENTED },
    { D_FTP_REPLY_PARAMETER_UNSUPPORTED, D_FTP_ERROR_NOT_IMPLEMENTED },
    { D_FTP_REPLY_BAD_SEQUENCE, D_FTP_ERROR_BAD_SEQUENCE },
    { D_FTP_REPLY_PROTOCOL_UNSUPPORTED, D_FTP_ERROR_PROTOCOL_UNSUPPORTED },
    { D_FTP_REPLY_NOT_LOGGED_IN, D_FTP_ERROR_LOGIN_DENIED },
    { D_FTP_REPLY_NEED_ACCOUNT_TO_STORE, D_FTP_ERROR_ACCOUNT_REQUIRED },
    { D_FTP_REPLY_PAGE_TYPE_UNKNOWN, D_FTP_ERROR_PAGE_TYPE_UNKNOWN },
    { D_FTP_REPLY_NAME_NOT_ALLOWED, D_FTP_ERROR_NAME_NOT_ALLOWED },
    { D_FTP_REPLY_SECURITY_RESOURCE, D_FTP_ERROR_SECURITY },
    { D_FTP_REPLY_PROTECTION_DENIED, D_FTP_ERROR_SECURITY },
    { D_FTP_REPLY_POLICY_DENIED, D_FTP_ERROR_SECURITY },
    { D_FTP_REPLY_SECURITY_CHECK_FAILED, D_FTP_ERROR_SECURITY },
    { D_FTP_REPLY_PROT_UNSUPPORTED, D_FTP_ERROR_SECURITY },
    { D_FTP_REPLY_COMMAND_PROT_UNSUPPORTED, D_FTP_ERROR_SECURITY }
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
  The texts follow RFC 959 4.2.2 and RFC 2228 wording where those give one;
the table is short enough that a scan beats any index.
*/
const char*
d_ftp_reply_text(
    unsigned _code
)
{
    const size_t count = sizeof(REPLY_TEXTS) / sizeof(REPLY_TEXTS[0]);

    // the code's row, if it has one
    for (size_t index = 0u; index < count; index++)
    {
        if (REPLY_TEXTS[index].code == _code)
        {
            return REPLY_TEXTS[index].text;
        }
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
d_ftp_internal_reply_step
  File-local: takes one byte. Telnet command sequences, CR, and NUL vanish,
being line control, never text; LF ends the line; every other byte joins
it. Returns what the byte ended: MORE when it ended nothing, or a line
short of the last.
*/
D_STATIC enum d_ftp_internal_line
d_ftp_internal_reply_step(
    struct d_ftp_reply_parser* _parser,
    unsigned char              _byte
)
{
    // Telnet command sequences, CR, and NUL are not reply text
    if ( (!d_ftp_internal_telnet_accept(&_parser->telnet,
                                        _byte)) ||
         (_byte == '\r')                        ||
         (_byte == '\0') )
    {
        return D_FTP_INTERNAL_LINE_MORE;
    }

    // every other byte but LF joins the line
    if (_byte != '\n')
    {
        d_ftp_internal_reply_take(_parser,
                                  (char)_byte);

        return D_FTP_INTERNAL_LINE_MORE;
    }

    return d_ftp_internal_reply_end_line(_parser);
}

/*
d_ftp_internal_reply_deliver
  File-local: hands a completed reply over and readies the parser for the
next. The reserved last byte of the storage always has room for the
terminator.
*/
D_STATIC void
d_ftp_internal_reply_deliver(
    struct d_ftp_reply_parser* _parser,
    struct d_ftp_reply*        _out
)
{
    // the terminator, where there is storage at all
    if (_parser->capacity > 0u)
    {
        _parser->storage[_parser->length] = '\0';
    }

    _out->code        = _parser->code;
    _out->line_count  = _parser->line_count;
    _out->text.data   = _parser->storage;
    _out->text.length = _parser->length;
    _out->truncated   = _parser->truncated;

    d_ftp_internal_reply_clear(_parser);

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

    // consume until a reply completes or the input runs out
    for (size_t index = 0u; index < _length; )
    {
        const enum d_ftp_internal_line line =
            d_ftp_internal_reply_step(_parser,
                                      (unsigned char)_data[index++]);

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
            d_ftp_internal_reply_deliver(_parser,
                                         _out);
            *_out_used = index;

            return D_FTP_STATUS_COMPLETE;
        }
    }

    *_out_used = _length;

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
d_ftp_internal_reply_lead
  File-local: writes the lead of one formatted line into `_lead`, four
bytes, and returns its length: the code and a hyphen opening the first line
of several, the code and a space opening the last, a single space before a
middle line that begins with a digit -- which could otherwise pass for a
closing line -- and nothing before any other.
*/
D_STATIC size_t
d_ftp_internal_reply_lead(
    unsigned _code,
    bool     _first,
    bool     _last,
    bool     _digit,
    char*    _lead
)
{
    _lead[0] = (char)('0' + (int)(_code / 100u));
    _lead[1] = (char)('0' + (int)((_code / 10u) % 10u));
    _lead[2] = (char)('0' + (int)(_code % 10u));
    _lead[3] = (_last) ? ' ' : '-';

    // the code opens the first and the last line
    if ( (_first) ||
         (_last) )
    {
        return 4u;
    }

    // a middle line must never look like a closing line
    if (_digit)
    {
        _lead[0] = ' ';

        return 1u;
    }

    return 0u;
}

/*
d_ftp_reply_format
  Splits the text at every CR, LF, or CR LF and gives each piece its lead.
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

    const char* const standard = d_ftp_reply_text(_code);
    const char* const text     = (_text)    ? _text
                               : (standard) ? standard
                                            : "";
    const size_t      length   = strlen(text);
    const size_t      mark     = _out->length;

    // one reply line per line of text; empty text is one empty line
    for (size_t position = 0u; ; )
    {
        const size_t end         = position + strcspn(text + position,
                                                      "\r\n");
        const size_t next        = d_ftp_internal_after_break(text,
                                                              length,
                                                              end);
        char         lead[4]     = { 0 };
        const size_t lead_length =
            d_ftp_internal_reply_lead(_code,
                                      (mark == _out->length),
                                      (next >= length),
                                      ( (end > position) &&
                                        (d_ftp_internal_is_digit(
                                             text[position])) ),
                                      lead);

        // a line that does not fit abandons the whole reply
        if (!d_ftp_internal_append_line(_out,
                                        lead,
                                        lead_length,
                                        text + position,
                                        end - position))
        {
            d_ftp_internal_rollback(_out,
                                    mark);

            return D_FTP_ERROR_BUFFER_TOO_SMALL;
        }

        // the last line ends the reply
        if (next >= length)
        {
            return D_FTP_OK;
        }

        position = next;
    }
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
    const size_t                 count       =
        sizeof(REPLY_ERRORS) / sizeof(REPLY_ERRORS[0]);

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
    for (size_t index = 0u; index < count; index++)
    {
        if (REPLY_ERRORS[index].code == _code)
        {
            return REPLY_ERRORS[index].error;
        }
    }

    return D_FTP_ERROR_REJECTED;
}
