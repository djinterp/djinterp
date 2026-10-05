/*******************************************************************************
* djinterp [net]                                                           pop.c
*
* Implementation of the shared POP3 kernel declared in pop.h.
*   Pure computation over caller-owned memory: table lookups, the line and
* listing codecs, the body state machines, the reader, and MD5 for APOP. The
* only I/O is through the d_pop_transport and d_pack_sink the caller passes
* in.
*
*
* path:      /src/djinterp/net/pop/pop.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.25
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/net/pop/pop.h"  // corresponding header
// std
#include <inttypes.h>  // PRIu32, PRIu64
#include <stdio.h>     // snprintf
#include <string.h>    // memchr, memcpy, memmove, strlen
// djinterp
#include "../../../../inc/djinterp/env/net/pop/env_pop.h"  // D_ENV_POP_CAN_PLAIN


//==============================================================================
// 1.  TEXT HELPERS
//==============================================================================

/*
d_internal_pop_text
  Builds a span. Exists so every span in this file is spelled one way, and so a
zero-length span never carries arithmetic on a NULL base.
*/
static struct d_pack_text
d_internal_pop_text(
    const char* _data,
    size_t      _length
)
{
    struct d_pack_text text = { _data, _length };

    return text;
}

/*
d_internal_pop_text_ok
  A span is usable when it has storage or is empty. A NULL base with a non-zero
length is a caller bug, and every public entry point rejects it here rather
than dereferencing it later.
*/
static bool
d_internal_pop_text_ok(
    struct d_pack_text _text
)
{
    return ( (_text.data != NULL) ||
             (_text.length == 0) );
}

/*
d_internal_pop_lower
  ASCII-only case folding. POP3 keywords and indicators are ASCII by definition,
and tolower() would consult the locale, which a protocol parser must not.
*/
static char
d_internal_pop_lower(
    char _c
)
{
    if ( (_c >= 'A') &&
         (_c <= 'Z') )
    {
        return (char)(_c - 'A' + 'a');
    }

    return _c;
}

/*
d_internal_pop_text_ieq
  Case-insensitive equality of a span against a NUL-terminated literal.
*/
static bool
d_internal_pop_text_ieq(
    struct d_pack_text _text,
    const char*        _literal
)
{
    const size_t length = strlen(_literal);

    // a length mismatch settles it without touching the bytes
    if (_text.length != length)
    {
        return false;
    }

    // compare folded bytes one by one
    for (size_t i = 0; i < length; ++i)
    {
        if (d_internal_pop_lower(_text.data[i]) !=
            d_internal_pop_lower(_literal[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_pop_text_istarts
  Case-insensitive prefix test, used for the status indicators.
*/
static bool
d_internal_pop_text_istarts(
    struct d_pack_text _text,
    const char*        _prefix
)
{
    const size_t length = strlen(_prefix);

    // a span shorter than the prefix cannot start with it
    if (_text.length < length)
    {
        return false;
    }

    return d_internal_pop_text_ieq(d_internal_pop_text(_text.data, length),
                                   _prefix);
}

/*
d_internal_pop_text_after
  The part of a span past its first `_count` bytes. `_count` never exceeds the
length at any call site; the guard keeps a zero-length result off a NULL base.
*/
static struct d_pack_text
d_internal_pop_text_after(
    struct d_pack_text _text,
    size_t             _count
)
{
    // nothing remains, so return an empty span without offsetting the base
    if (_count >= _text.length)
    {
        return d_internal_pop_text(_text.data, 0);
    }

    return d_internal_pop_text(_text.data + _count,
                               _text.length - _count);
}

/*
d_internal_pop_skip_space
  Drops exactly one leading SP, if present. The grammar separates fields with a
single SP, and PASS's argument may itself begin with a space, so this never
drops more than one.
*/
static struct d_pack_text
d_internal_pop_skip_space(
    struct d_pack_text _text
)
{
    // only a single separator belongs to the grammar
    if ( (_text.length > 0) &&
         (_text.data[0] == ' ') )
    {
        return d_internal_pop_text_after(_text, 1);
    }

    return _text;
}

/*
d_internal_pop_next_token
  Splits the next SP-delimited token off `*_rest`. Runs of SP are tolerated as
one separator: RFC 1939 forbids them, but refusing them buys nothing.
*/
static struct d_pack_text
d_internal_pop_next_token(
    struct d_pack_text* _rest
)
{
    // an exhausted span yields an empty token and stays exhausted
    if (_rest->length == 0)
    {
        return d_internal_pop_text(_rest->data, 0);
    }

    size_t begin = 0;

    // skip the separator run
    while ( (begin < _rest->length) &&
            (_rest->data[begin] == ' ') )
    {
        ++begin;
    }

    size_t end = begin;

    // the token runs to the next separator or the end
    while ( (end < _rest->length) &&
            (_rest->data[end] != ' ') )
    {
        ++end;
    }

    const struct d_pack_text token = d_internal_pop_text(_rest->data + begin,
                                                         end - begin);

    *_rest = d_internal_pop_text_after(*_rest, end);

    return token;
}

/*
d_internal_pop_text_is_clean
  True when a span holds no CR, LF, or NUL -- the three bytes that would split
or truncate a line -- and, unless `_allow_space`, no SP.
*/
static bool
d_internal_pop_text_is_clean(
    struct d_pack_text _text,
    bool               _allow_space
)
{
    for (size_t i = 0; i < _text.length; ++i)
    {
        const char c = _text.data[i];

        // any of these would change the line structure on the wire
        if ( (c == '\r') ||
             (c == '\n') ||
             (c == '\0') ||
             ( (c == ' ') &&
               (!_allow_space) ) )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_pop_text_has
  True when a span contains `_c`. Guards the empty span, whose base may be
NULL and so may not reach memchr.
*/
static bool
d_internal_pop_text_has(
    struct d_pack_text _text,
    char               _c
)
{
    return ( (_text.length > 0) &&
             (memchr(_text.data, _c, _text.length) != NULL) );
}

/*
d_internal_pop_copy_text
  Copies a span into a fixed array as a C string. On overflow it stores the
empty string rather than a truncated one, because a truncated mechanism list
reads as a valid, shorter list.
*/
static enum d_pop_status
d_internal_pop_copy_text(
    char*              _dest,
    size_t             _capacity,
    struct d_pack_text _text
)
{
    // the text and its NUL must both fit
    if (_text.length >= _capacity)
    {
        _dest[0] = '\0';

        return D_POP_STATUS_BUFFER_TOO_SMALL;
    }

    // an empty span may have a NULL base, which memcpy must not see
    if (_text.length > 0)
    {
        memcpy(_dest, _text.data, _text.length);
    }

    _dest[_text.length] = '\0';

    return D_POP_STATUS_OK;
}

//==============================================================================
// 2.  OUTPUT HELPERS
//==============================================================================

/*
d_internal_pop_out_args_ok
  The two-call protocol's argument check, shared by every formatter: a size
pointer is mandatory, and a NULL buffer is only legal with zero capacity.
*/
static bool
d_internal_pop_out_args_ok(
    const char*   _out,
    size_t        _capacity,
    const size_t* _out_size
)
{
    return ( (_out_size != NULL) &&
             ( (_out != NULL) ||
               (_capacity == 0) ) );
}

/*
d_internal_pop_out_check
  Applies the tail of the two-call protocol once the exact size is known:
record it, then decide between measure, too-small, and produce. Every
formatter reaches the byte-writing step only through here, so none of them can
write past a buffer.
*/
static enum d_pop_status
d_internal_pop_out_check(
    char*   _out,
    size_t  _capacity,
    size_t  _size,
    size_t* _out_size
)
{
    *_out_size = _size;

    // a measure pass stops here
    if ( (_out == NULL) &&
         (_capacity == 0) )
    {
        return D_POP_STATUS_OK;
    }

    // the requirement is already recorded, so a caller can grow and retry
    if (_capacity < _size)
    {
        return D_POP_STATUS_BUFFER_TOO_SMALL;
    }

    return D_POP_STATUS_OK;
}

/*
d_internal_pop_put
  Appends bytes at `*_pos`. Only called after d_internal_pop_out_check has
proven the whole line fits.
*/
static void
d_internal_pop_put(
    char*       _out,
    size_t*     _pos,
    const char* _data,
    size_t      _length
)
{
    // an empty span may have a NULL base
    if (_length > 0)
    {
        memcpy(_out + *_pos, _data, _length);
        *_pos += _length;
    }

    return;
}

/*
d_internal_pop_emit_scratch
  Hands a line formatted into a scratch buffer to the caller under the two-call
protocol. The listing formatters go through snprintf, which insists on a NUL
the wire does not want; formatting to scratch keeps that NUL out of the
caller's buffer and out of the reported size.
*/
static enum d_pop_status
d_internal_pop_emit_scratch(
    const char* _scratch,
    int         _written,
    char*       _out,
    size_t      _capacity,
    size_t*     _out_size
)
{
    // snprintf reports an encoding failure as a negative count
    if (_written < 0)
    {
        return D_POP_STATUS_MALFORMED;
    }

    const size_t      size   = (size_t)_written;
    enum d_pop_status status = d_internal_pop_out_check(_out,
                                                        _capacity,
                                                        size,
                                                        _out_size);

    // nothing to write on a measure pass or a failure
    if ( (status != D_POP_STATUS_OK) ||
         (_out == NULL) )
    {
        return status;
    }

    memcpy(_out, _scratch, size);

    return D_POP_STATUS_OK;
}

/*
d_internal_pop_emit
  Pushes bytes to a sink, treating a NULL `write` as discard. Deliberately not
d_sink_emit, which reports a NULL sink as a failure: here, discarding is how a
caller skips a body it does not want.
*/
static enum d_pop_status
d_internal_pop_emit(
    struct d_pack_sink _sink,
    const void*        _data,
    size_t             _size
)
{
    // nothing to push, or a discarding sink
    if ( (_size == 0) ||
         (_sink.write == NULL) )
    {
        return D_POP_STATUS_OK;
    }

    // a short write aborts: a stream that cannot rewind cannot resume
    if (_sink.write(_sink.context, _data, _size) != _size)
    {
        return D_POP_STATUS_SINK_ERROR;
    }

    return D_POP_STATUS_OK;
}

//==============================================================================
// 3.  STATUS
//==============================================================================

// d_internal_pop_status_names
//   the name of each status. Searched rather than indexed, because the values
// are deliberately sparse.
static const struct
{
    enum d_pop_status status;
    const char*       name;
} d_internal_pop_status_names[] =
{
    { D_POP_STATUS_OK,                "ok"                },
    { D_POP_STATUS_INVALID_ARGUMENT,  "invalid argument"  },
    { D_POP_STATUS_MALFORMED,         "malformed"         },
    { D_POP_STATUS_UNKNOWN_COMMAND,   "unknown command"   },
    { D_POP_STATUS_WRONG_STATE,       "wrong state"       },
    { D_POP_STATUS_UNSUPPORTED,       "unsupported"       },
    { D_POP_STATUS_LINE_TOO_LONG,     "line too long"     },
    { D_POP_STATUS_BUFFER_TOO_SMALL,  "buffer too small"  },
    { D_POP_STATUS_WOULD_BLOCK,       "would block"       },
    { D_POP_STATUS_TRANSPORT_ERROR,   "transport error"   },
    { D_POP_STATUS_CONNECTION_CLOSED, "connection closed" },
    { D_POP_STATUS_SINK_ERROR,        "sink error"        }
};

/*
d_pop_status_name
  Linear search over twelve entries; a status is named once per failure, not
per byte.
*/
const char*
d_pop_status_name(
    enum d_pop_status _status
)
{
    const size_t count = sizeof(d_internal_pop_status_names) /
                         sizeof(d_internal_pop_status_names[0]);

    // find the entry for this status
    for (size_t i = 0; i < count; ++i)
    {
        if (d_internal_pop_status_names[i].status == _status)
        {
            return d_internal_pop_status_names[i].name;
        }
    }

    return "unknown status";
}

/*
d_pop_status_is_formal
  A range test against the mechanical floor; see the header's section 2.
*/
bool
d_pop_status_is_formal(
    enum d_pop_status _status
)
{
    return ( (_status != D_POP_STATUS_OK) &&
             ((int)_status < D_POP_STATUS_MECHANICAL_FLOOR) );
}

/*
d_pop_status_is_mechanical
  The complement of formal among failures.
*/
bool
d_pop_status_is_mechanical(
    enum d_pop_status _status
)
{
    return ((int)_status >= D_POP_STATUS_MECHANICAL_FLOOR);
}

//==============================================================================
// 4.  SESSION MODEL
//==============================================================================

// D_INTERNAL_POP_IN_AUTH / D_INTERNAL_POP_IN_TRANS
//   the state masks the command table is written in.
#define D_INTERNAL_POP_IN_AUTH  D_POP_STATE_BIT(D_POP_STATE_AUTHORIZATION)
#define D_INTERNAL_POP_IN_TRANS D_POP_STATE_BIT(D_POP_STATE_TRANSACTION)

// d_internal_pop_commands
//   the command table, indexed by d_pop_command. This is the single statement
// of which command is legal where, takes what, and answers with a body.
// USER and PASS are gated on the USER capability (RFC 2449 section 6.6); APOP
// has no capability and is discovered from the greeting.
static const struct d_pop_command_info
d_internal_pop_commands[D_POP_COMMAND_COUNT] =
{
    { NULL,   0, 0, 0, 0, D_POP_BODY_NEVER, 0 },
    { "USER", D_INTERNAL_POP_IN_AUTH,
              D_POP_CAPABILITY_USER, 1, 1, D_POP_BODY_NEVER, 0 },
    { "PASS", D_INTERNAL_POP_IN_AUTH,
              D_POP_CAPABILITY_USER, 1, 1, D_POP_BODY_NEVER, 1 },
    { "APOP", D_INTERNAL_POP_IN_AUTH,
              0, 2, 2, D_POP_BODY_NEVER, 0 },
    { "AUTH", D_INTERNAL_POP_IN_AUTH,
              D_POP_CAPABILITY_SASL, 1, 2, D_POP_BODY_NEVER, 0 },
    { "STLS", D_INTERNAL_POP_IN_AUTH,
              D_POP_CAPABILITY_STLS, 0, 0, D_POP_BODY_NEVER, 0 },
    { "CAPA", D_INTERNAL_POP_IN_AUTH | D_INTERNAL_POP_IN_TRANS,
              0, 0, 0, D_POP_BODY_ALWAYS, 0 },
    { "QUIT", D_INTERNAL_POP_IN_AUTH | D_INTERNAL_POP_IN_TRANS,
              0, 0, 0, D_POP_BODY_NEVER, 0 },
    { "STAT", D_INTERNAL_POP_IN_TRANS,
              0, 0, 0, D_POP_BODY_NEVER, 0 },
    { "LIST", D_INTERNAL_POP_IN_TRANS,
              0, 0, 1, D_POP_BODY_WITHOUT_ARGUMENT, 0 },
    { "RETR", D_INTERNAL_POP_IN_TRANS,
              0, 1, 1, D_POP_BODY_ALWAYS, 0 },
    { "DELE", D_INTERNAL_POP_IN_TRANS,
              0, 1, 1, D_POP_BODY_NEVER, 0 },
    { "NOOP", D_INTERNAL_POP_IN_TRANS,
              0, 0, 0, D_POP_BODY_NEVER, 0 },
    { "RSET", D_INTERNAL_POP_IN_TRANS,
              0, 0, 0, D_POP_BODY_NEVER, 0 },
    { "TOP",  D_INTERNAL_POP_IN_TRANS,
              D_POP_CAPABILITY_TOP, 2, 2, D_POP_BODY_ALWAYS, 0 },
    { "UIDL", D_INTERNAL_POP_IN_TRANS,
              D_POP_CAPABILITY_UIDL, 0, 1, D_POP_BODY_WITHOUT_ARGUMENT, 0 },
    { "UTF8", D_INTERNAL_POP_IN_AUTH,
              D_POP_CAPABILITY_UTF8, 0, 0, D_POP_BODY_NEVER, 0 },
    { "LANG", D_INTERNAL_POP_IN_AUTH | D_INTERNAL_POP_IN_TRANS,
              D_POP_CAPABILITY_LANG, 0, 1, D_POP_BODY_WITHOUT_ARGUMENT, 0 }
};

/*
d_pop_command_info_of
  Bounds-checked table access. UNKNOWN maps to NULL because its row carries no
keyword and describes nothing.
*/
const struct d_pop_command_info*
d_pop_command_info_of(
    enum d_pop_command _command
)
{
    // UNKNOWN and out-of-range values have no metadata
    if ( ((int)_command <= (int)D_POP_COMMAND_UNKNOWN) ||
         ((int)_command >= D_POP_COMMAND_COUNT) )
    {
        return NULL;
    }

    return &d_internal_pop_commands[_command];
}

/*
d_pop_command_keyword
  The canonical upper-case keyword, or NULL where there is none.
*/
const char*
d_pop_command_keyword(
    enum d_pop_command _command
)
{
    const struct d_pop_command_info* info = d_pop_command_info_of(_command);

    return (info != NULL) ? info->keyword
                          : NULL;
}

/*
d_pop_command_from_keyword
  Linear search of seventeen rows. Keywords arrive once per command line, so a
hash would be optimizing the wrong thing.
*/
enum d_pop_command
d_pop_command_from_keyword(
    struct d_pack_text _keyword
)
{
    // a span without storage matches nothing
    if (!d_internal_pop_text_ok(_keyword))
    {
        return D_POP_COMMAND_UNKNOWN;
    }

    // row 0 is UNKNOWN, so the search starts at 1
    for (int i = 1; i < D_POP_COMMAND_COUNT; ++i)
    {
        if (d_internal_pop_text_ieq(_keyword,
                                    d_internal_pop_commands[i].keyword))
        {
            return (enum d_pop_command)i;
        }
    }

    return D_POP_COMMAND_UNKNOWN;
}

/*
d_pop_command_is_allowed
  A mask test. States outside the enumeration are refused rather than shifted
by an arbitrary amount.
*/
bool
d_pop_command_is_allowed(
    enum d_pop_command _command,
    enum d_pop_state   _state
)
{
    const struct d_pop_command_info* info = d_pop_command_info_of(_command);

    // unknown commands and invalid states are never allowed
    if ( (info == NULL)                ||
         ((int)_state < 0)             ||
         ((int)_state >= D_POP_STATE_COUNT) )
    {
        return false;
    }

    return ((info->states & D_POP_STATE_BIT(_state)) != 0);
}

/*
d_pop_command_expects_body
  Applies the command's body rule. The rule is evaluated for a +OK reply; a
-ERR reply never carries a body, which is the caller's to check.
*/
bool
d_pop_command_expects_body(
    enum d_pop_command _command,
    size_t             _arg_count
)
{
    const struct d_pop_command_info* info = d_pop_command_info_of(_command);

    // an unknown command's reply shape cannot be known
    if (info == NULL)
    {
        return false;
    }

    // LIST, UIDL, and LANG list everything only when not asked about one item
    if (info->body_rule == D_POP_BODY_WITHOUT_ARGUMENT)
    {
        return (_arg_count == 0);
    }

    return (info->body_rule == D_POP_BODY_ALWAYS);
}

/*
d_pop_state_name
  Indexed by state; an out-of-range value gets a fixed placeholder.
*/
const char*
d_pop_state_name(
    enum d_pop_state _state
)
{
    static const char* const names[D_POP_STATE_COUNT] =
    {
        "CLOSED", "AUTHORIZATION", "TRANSACTION", "UPDATE"
    };

    // anything outside the enumeration has no name
    if ( ((int)_state < 0) ||
         ((int)_state >= D_POP_STATE_COUNT) )
    {
        return "INVALID";
    }

    return names[_state];
}

/*
d_pop_state_next
  The transitions of RFC 1939 section 3. QUIT is checked before the indicator
because the transition happens on the command, not the reply: a server in
UPDATE may answer -ERR if it could not remove every message, and closes
regardless.
*/
enum d_pop_state
d_pop_state_next(
    enum d_pop_state     _state,
    enum d_pop_command   _command,
    enum d_pop_indicator _indicator
)
{
    // a session that has ended stays ended
    if ( (_state != D_POP_STATE_AUTHORIZATION) &&
         (_state != D_POP_STATE_TRANSACTION) )
    {
        return D_POP_STATE_CLOSED;
    }

    // QUIT ends a live session whatever the reply says
    if (_command == D_POP_COMMAND_QUIT)
    {
        return (_state == D_POP_STATE_TRANSACTION) ? D_POP_STATE_UPDATE
                                                   : D_POP_STATE_CLOSED;
    }

    // only a completed authentication moves the session forward
    if ( (_state == D_POP_STATE_AUTHORIZATION) &&
         (_indicator == D_POP_INDICATOR_OK)    &&
         ( (_command == D_POP_COMMAND_PASS) ||
           (_command == D_POP_COMMAND_APOP) ||
           (_command == D_POP_COMMAND_AUTH) ) )
    {
        return D_POP_STATE_TRANSACTION;
    }

    return _state;
}

//==============================================================================
// 5.  EXTENSIONS
//==============================================================================

// d_internal_pop_resp_code_names
//   the wire spelling of each response code, indexed by d_pop_resp_code.
static const char* const d_internal_pop_resp_code_names[] =
{
    NULL, NULL, "LOGIN-DELAY", "IN-USE", "SYS/TEMP", "SYS/PERM", "AUTH", "UTF8"
};

/*
d_pop_resp_code_name
  Indexed lookup; NONE, UNKNOWN, and out-of-range values have no fixed name.
*/
const char*
d_pop_resp_code_name(
    enum d_pop_resp_code _code
)
{
    const int count = (int)(sizeof(d_internal_pop_resp_code_names) /
                            sizeof(d_internal_pop_resp_code_names[0]));

    // guard the index
    if ( ((int)_code < 0) ||
         ((int)_code >= count) )
    {
        return NULL;
    }

    return d_internal_pop_resp_code_names[_code];
}

/*
d_pop_resp_code_from_text
  Exact, case-insensitive match. RFC 2449 lets a client fall back to a code's
parent for an unknown hierarchical child ("SYS/FOO" as "SYS"), but none of the
defined parents is meaningful alone, so an unknown child is simply UNKNOWN.
*/
enum d_pop_resp_code
d_pop_resp_code_from_text(
    struct d_pack_text _text
)
{
    // no code at all
    if ( (_text.length == 0) ||
         (!d_internal_pop_text_ok(_text)) )
    {
        return D_POP_RESP_CODE_NONE;
    }

    // match against every named code
    for (int i = (int)D_POP_RESP_CODE_LOGIN_DELAY;
         i <= (int)D_POP_RESP_CODE_UTF8;
         ++i)
    {
        if (d_internal_pop_text_ieq(_text, d_internal_pop_resp_code_names[i]))
        {
            return (enum d_pop_resp_code)i;
        }
    }

    return D_POP_RESP_CODE_UNKNOWN;
}

// d_internal_pop_capability_names
//   the CAPA keyword of each capability bit.
static const struct
{
    enum d_pop_capability capability;
    const char*           keyword;
} d_internal_pop_capability_names[] =
{
    { D_POP_CAPABILITY_TOP,            "TOP"            },
    { D_POP_CAPABILITY_USER,           "USER"           },
    { D_POP_CAPABILITY_SASL,           "SASL"           },
    { D_POP_CAPABILITY_RESP_CODES,     "RESP-CODES"     },
    { D_POP_CAPABILITY_LOGIN_DELAY,    "LOGIN-DELAY"    },
    { D_POP_CAPABILITY_PIPELINING,     "PIPELINING"     },
    { D_POP_CAPABILITY_EXPIRE,         "EXPIRE"         },
    { D_POP_CAPABILITY_UIDL,           "UIDL"           },
    { D_POP_CAPABILITY_IMPLEMENTATION, "IMPLEMENTATION" },
    { D_POP_CAPABILITY_STLS,           "STLS"           },
    { D_POP_CAPABILITY_UTF8,           "UTF8"           },
    { D_POP_CAPABILITY_LANG,           "LANG"           },
    { D_POP_CAPABILITY_AUTH_RESP_CODE, "AUTH-RESP-CODE" }
};

// D_INTERNAL_POP_CAPABILITY_COUNT
//   the number of rows in d_internal_pop_capability_names.
#define D_INTERNAL_POP_CAPABILITY_COUNT                                        \
    (sizeof(d_internal_pop_capability_names) /                                 \
     sizeof(d_internal_pop_capability_names[0]))

/*
d_pop_capabilities_init
  Absent is not zero: a LOGIN-DELAY of 0 is a real advertisement, so absence
has its own sentinel.
*/
void
d_pop_capabilities_init(
    struct d_pop_capabilities* _caps
)
{
    // nothing to initialize
    if (_caps == NULL)
    {
        return;
    }

    _caps->flags                = 0;
    _caps->login_delay          = D_POP_VALUE_ABSENT;
    _caps->login_delay_per_user = 0;
    _caps->expire               = D_POP_VALUE_ABSENT;
    _caps->expire_per_user      = 0;
    _caps->sasl_mechanisms[0]   = '\0';
    _caps->implementation[0]    = '\0';

    return;
}

/*
d_pop_capabilities_has
  A mask test.
*/
bool
d_pop_capabilities_has(
    const struct d_pop_capabilities* _caps,
    enum d_pop_capability            _cap
)
{
    return ( (_caps != NULL) &&
             ((_caps->flags & (uint32_t)_cap) != 0) );
}

/*
d_pop_capability_keyword
  Searched, so a combined mask (two bits) correctly matches nothing.
*/
const char*
d_pop_capability_keyword(
    enum d_pop_capability _cap
)
{
    // find the single bit
    for (size_t i = 0; i < D_INTERNAL_POP_CAPABILITY_COUNT; ++i)
    {
        if (d_internal_pop_capability_names[i].capability == _cap)
        {
            return d_internal_pop_capability_names[i].keyword;
        }
    }

    return NULL;
}

/*
d_internal_pop_capability_lookup
  Keyword to bit, or 0 for a keyword this module does not know.
*/
static uint32_t
d_internal_pop_capability_lookup(
    struct d_pack_text _keyword
)
{
    // find the matching keyword
    for (size_t i = 0; i < D_INTERNAL_POP_CAPABILITY_COUNT; ++i)
    {
        if (d_internal_pop_text_ieq(_keyword,
                                    d_internal_pop_capability_names[i].keyword))
        {
            return (uint32_t)d_internal_pop_capability_names[i].capability;
        }
    }

    return 0;
}

/*
d_internal_pop_parse_period
  The shared parameter grammar of LOGIN-DELAY and EXPIRE: a number, or NEVER
where `_allow_never`, optionally followed by USER. RFC 2449 lets the AUTHORIZATION
state advertise USER ("varies by user") and TRANSACTION the actual value; both
parse the same way.
*/
static enum d_pop_status
d_internal_pop_parse_period(
    struct d_pack_text _params,
    bool               _allow_never,
    int32_t*           _out_value,
    int32_t*           _out_per_user
)
{
    struct d_pack_text       rest  = _params;
    const struct d_pack_text value = d_internal_pop_next_token(&rest);
    const struct d_pack_text tail  = d_internal_pop_next_token(&rest);

    *_out_per_user = d_internal_pop_text_ieq(tail, "USER") ? 1
                                                           : 0;

    // EXPIRE NEVER has no number
    if ( (_allow_never) &&
         (d_internal_pop_text_ieq(value, "NEVER")) )
    {
        *_out_value = D_POP_EXPIRE_NEVER;

        return D_POP_STATUS_OK;
    }

    uint64_t number = 0;

    // anything else must be a non-negative integer
    if (d_pop_parse_decimal(value, INT32_MAX, &number) != D_POP_STATUS_OK)
    {
        return D_POP_STATUS_MALFORMED;
    }

    *_out_value = (int32_t)number;

    return D_POP_STATUS_OK;
}

/*
d_pop_capabilities_parse_line
  One keyword, then parameters whose grammar depends on the keyword. The flag is
set before the parameters are examined, so a server that advertises SASL with
an over-long list is still known to offer SASL.
*/
enum d_pop_status
d_pop_capabilities_parse_line(
    struct d_pop_capabilities* _caps,
    struct d_pack_text         _line
)
{
    // parameter validation
    if ( (_caps == NULL) ||
         (!d_internal_pop_text_ok(_line)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    struct d_pack_text rest       = _line;
    const uint32_t     capability =
        d_internal_pop_capability_lookup(d_internal_pop_next_token(&rest));
    const struct d_pack_text params = d_internal_pop_skip_space(rest);

    _caps->flags |= capability;

    // only four capabilities carry parameters worth keeping
    switch (capability)
    {
        case D_POP_CAPABILITY_SASL:
        {
            return d_internal_pop_copy_text(_caps->sasl_mechanisms,
                                            sizeof(_caps->sasl_mechanisms),
                                            params);
        }
        case D_POP_CAPABILITY_IMPLEMENTATION:
        {
            return d_internal_pop_copy_text(_caps->implementation,
                                            sizeof(_caps->implementation),
                                            params);
        }
        case D_POP_CAPABILITY_LOGIN_DELAY:
        {
            return d_internal_pop_parse_period(params,
                                               false,
                                               &_caps->login_delay,
                                               &_caps->login_delay_per_user);
        }
        case D_POP_CAPABILITY_EXPIRE:
        {
            return d_internal_pop_parse_period(params,
                                               true,
                                               &_caps->expire,
                                               &_caps->expire_per_user);
        }
        default:
        {
            return D_POP_STATUS_OK;
        }
    }
}

/*
d_pop_build_features
  Each term is a compile-time constant, so this folds to a single constant; it
is a function so a C++ face or a derived module can query the build without
re-running detection in its own translation unit.
*/
uint32_t
d_pop_build_features(
    void
)
{
    const uint32_t features =
        ( (D_ENV_POP_CAN_PLAIN ? (uint32_t)D_POP_FEATURE_TCP  : 0u) |
          (D_INTERNAL_POP_TLS  ? (uint32_t)D_POP_FEATURE_TLS  : 0u) |
          (D_INTERNAL_POP_CURL ? (uint32_t)D_POP_FEATURE_CURL : 0u) |
          (D_INTERNAL_POP_SASL ? (uint32_t)D_POP_FEATURE_SASL : 0u) |
          (D_INTERNAL_POP_APOP ? (uint32_t)D_POP_FEATURE_APOP : 0u) );

    return features;
}

//==============================================================================
// 6.  LINE CODEC
//==============================================================================

/*
d_internal_pop_keyword_is_valid
  RFC 2449 section 3: three or four characters. Letters and digits are
accepted, since UTF8 is a keyword.
*/
static bool
d_internal_pop_keyword_is_valid(
    struct d_pack_text _keyword
)
{
    // the length bound comes first
    if ( (_keyword.length < D_POP_KEYWORD_MIN) ||
         (_keyword.length > D_POP_KEYWORD_MAX) )
    {
        return false;
    }

    // every character must be alphanumeric
    for (size_t i = 0; i < _keyword.length; ++i)
    {
        const char c = _keyword.data[i];

        if ( !( ( (c >= 'A') && (c <= 'Z') ) ||
                ( (c >= 'a') && (c <= 'z') ) ||
                ( (c >= '0') && (c <= '9') ) ) )
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_pop_split_args
  Splits the text after the keyword into arguments. A rest-of-line command
takes everything after the single separating SP, spaces and all. Returns true
when more tokens remain than `_max` allows.
*/
static bool
d_internal_pop_split_args(
    struct d_pack_text         _rest,
    size_t                     _max,
    bool                       _rest_of_line,
    struct d_pop_command_line* _out
)
{
    // PASS: the argument is the verbatim remainder of the line
    if (_rest_of_line)
    {
        const struct d_pack_text remainder = d_internal_pop_skip_space(_rest);

        if (remainder.length > 0)
        {
            _out->args[0]   = remainder;
            _out->arg_count = 1;
        }

        return false;
    }

    // everything else: SP-delimited tokens
    for (;;)
    {
        const struct d_pack_text token = d_internal_pop_next_token(&_rest);

        // no tokens remain
        if (token.length == 0)
        {
            return false;
        }

        // one token too many
        if (_out->arg_count >= _max)
        {
            return true;
        }

        _out->args[_out->arg_count++] = token;
    }
}

/*
d_pop_command_parse
  Keyword, then arguments by the command's own rule. An unknown keyword is not
a parse failure -- the line is well-formed -- so it is filled in as far as it
goes and reported distinctly, leaving the "-ERR unknown command" reply to the
server.
*/
enum d_pop_status
d_pop_command_parse(
    struct d_pack_text         _line,
    struct d_pop_command_line* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_line)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    struct d_pack_text       rest    = _line;
    const struct d_pack_text keyword = d_internal_pop_next_token(&rest);

    _out->command   = D_POP_COMMAND_UNKNOWN;
    _out->keyword   = keyword;
    _out->arg_count = 0;

    // the keyword must be well-formed before anything else matters
    if (!d_internal_pop_keyword_is_valid(keyword))
    {
        return D_POP_STATUS_MALFORMED;
    }

    _out->command = d_pop_command_from_keyword(keyword);

    const struct d_pop_command_info* info = d_pop_command_info_of(_out->command);

    // an extension command: split generically, tolerate extra tokens
    if (info == NULL)
    {
        (void)d_internal_pop_split_args(rest, D_POP_ARGS_MAX, false, _out);

        return D_POP_STATUS_UNKNOWN_COMMAND;
    }

    const bool too_many = d_internal_pop_split_args(rest,
                                                    (size_t)info->max_args,
                                                    (info->rest_of_line != 0),
                                                    _out);

    // the argument count must match the command
    if ( (too_many) ||
         (_out->arg_count < (size_t)info->min_args) )
    {
        return D_POP_STATUS_MALFORMED;
    }

    return D_POP_STATUS_OK;
}

/*
d_internal_pop_command_measure
  Validates a command for sending and computes its wire length. Separated from
formatting so the two-call protocol measures exactly what it later writes.
*/
static enum d_pop_status
d_internal_pop_command_measure(
    const struct d_pop_command_line* _command,
    struct d_pack_text*              _out_keyword,
    size_t*                          _out_size
)
{
    const struct d_pop_command_info* info = d_pop_command_info_of(
                                                _command->command);
    const bool rest_of_line = ( (info != NULL) &&
                                (info->rest_of_line != 0) );

    *_out_keyword = (info != NULL)
                  ? d_internal_pop_text(info->keyword, strlen(info->keyword))
                  : _command->keyword;

    // the keyword, and the argument count for a known command
    if ( (!d_internal_pop_text_ok(*_out_keyword))                   ||
         (!d_internal_pop_keyword_is_valid(*_out_keyword))          ||
         (_command->arg_count > D_POP_ARGS_MAX)                     ||
         ( (info != NULL) &&
           ( (_command->arg_count < (size_t)info->min_args) ||
             (_command->arg_count > (size_t)info->max_args) ) ) )
    {
        return D_POP_STATUS_MALFORMED;
    }

    size_t size = _out_keyword->length + 2;

    // every argument is non-empty and cannot break the line
    for (size_t i = 0; i < _command->arg_count; ++i)
    {
        const struct d_pack_text arg = _command->args[i];

        if ( (arg.length == 0)                                ||
             (!d_internal_pop_text_ok(arg))                   ||
             (!d_internal_pop_text_is_clean(arg, rest_of_line)) )
        {
            return D_POP_STATUS_MALFORMED;
        }

        size += 1 + arg.length;
    }

    *_out_size = size;

    return D_POP_STATUS_OK;
}

/*
d_pop_command_format
  Measure, check, write. The length limit is enforced on every call, measure
included, because a line the server may reject is not worth sizing a buffer
for.
*/
enum d_pop_status
d_pop_command_format(
    const struct d_pop_command_line* _command,
    char*                            _out,
    size_t                           _capacity,
    size_t*                          _out_size
)
{
    // parameter validation
    if ( (_command == NULL) ||
         (!d_internal_pop_out_args_ok(_out, _capacity, _out_size)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    struct d_pack_text keyword = { NULL, 0 };
    size_t             size    = 0;
    enum d_pop_status  status  = d_internal_pop_command_measure(_command,
                                                                &keyword,
                                                                &size);

    // an invalid command reports nothing further
    if (status != D_POP_STATUS_OK)
    {
        return status;
    }

    // RFC 2449's limit applies whatever buffer the caller has
    if (size > D_POP_COMMAND_MAX)
    {
        *_out_size = size;

        return D_POP_STATUS_LINE_TOO_LONG;
    }

    status = d_internal_pop_out_check(_out, _capacity, size, _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_POP_STATUS_OK) ||
         (_out == NULL) )
    {
        return status;
    }

    size_t pos = 0;

    d_internal_pop_put(_out, &pos, keyword.data, keyword.length);

    // each argument is preceded by its single separator
    for (size_t i = 0; i < _command->arg_count; ++i)
    {
        d_internal_pop_put(_out, &pos, " ", 1);
        d_internal_pop_put(_out,
                           &pos,
                           _command->args[i].data,
                           _command->args[i].length);
    }

    d_internal_pop_put(_out, &pos, D_POP_TOKEN_CRLF, 2);

    return D_POP_STATUS_OK;
}

/*
d_internal_pop_parse_resp_code
  Splits "[CODE] text" on a -ERR line. A `[` with no closing `]` is left as
text: the code grammar did not match, and guessing where it ends would
misreport the server's words.
*/
static void
d_internal_pop_parse_resp_code(
    struct d_pop_response* _out
)
{
    const struct d_pack_text text = _out->text;

    // no bracket, no code
    if ( (text.length == 0) ||
         (text.data[0] != '[') )
    {
        return;
    }

    const char* close = memchr(text.data, ']', text.length);

    // an unterminated bracket is ordinary text
    if (close == NULL)
    {
        return;
    }

    const size_t inner = (size_t)(close - text.data) - 1;

    _out->code_text = d_internal_pop_text(text.data + 1, inner);
    _out->code      = d_pop_resp_code_from_text(_out->code_text);
    _out->text      = d_internal_pop_skip_space(
                          d_internal_pop_text_after(text, inner + 2));

    return;
}

/*
d_pop_response_parse
  "+OK" must be tested before "+" -- the continuation indicator is its prefix --
and each indicator must be followed by SP or end of line, so "+OKAY" is not
mistaken for "+OK".
*/
enum d_pop_status
d_pop_response_parse(
    struct d_pack_text     _line,
    struct d_pop_response* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_line)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    _out->indicator = D_POP_INDICATOR_NONE;
    _out->code      = D_POP_RESP_CODE_NONE;
    _out->code_text = d_internal_pop_text(_line.data, 0);
    _out->text      = d_internal_pop_text(_line.data, 0);

    static const struct
    {
        const char*          token;
        enum d_pop_indicator indicator;
    } tokens[] =
    {
        { D_POP_TOKEN_OK,       D_POP_INDICATOR_OK       },
        { D_POP_TOKEN_ERR,      D_POP_INDICATOR_ERR      },
        { D_POP_TOKEN_CONTINUE, D_POP_INDICATOR_CONTINUE }
    };

    // try each indicator in order; the first whole-token match wins
    for (size_t i = 0; i < (sizeof(tokens) / sizeof(tokens[0])); ++i)
    {
        const size_t length = strlen(tokens[i].token);

        if ( (d_internal_pop_text_istarts(_line, tokens[i].token)) &&
             ( (_line.length == length) ||
               (_line.data[length] == ' ') ) )
        {
            _out->indicator = tokens[i].indicator;
            _out->text      = d_internal_pop_skip_space(
                                  d_internal_pop_text_after(_line, length));
            break;
        }
    }

    // a response code is only recognized on a negative reply
    if (_out->indicator == D_POP_INDICATOR_ERR)
    {
        d_internal_pop_parse_resp_code(_out);
    }

    return (_out->indicator == D_POP_INDICATOR_NONE) ? D_POP_STATUS_MALFORMED
                                                     : D_POP_STATUS_OK;
}

/*
d_internal_pop_response_parts
  Resolves what a response will write: its indicator token and its code text.
Kept apart from the length arithmetic so the validation reads as one list.
*/
static enum d_pop_status
d_internal_pop_response_parts(
    const struct d_pop_response* _response,
    const char**                 _out_token,
    struct d_pack_text*          _out_code
)
{
    const char* const tokens[] =
    {
        NULL, D_POP_TOKEN_OK, D_POP_TOKEN_ERR, D_POP_TOKEN_CONTINUE
    };
    const int indicator = (int)_response->indicator;

    // the indicator must be a real one
    if ( (indicator <= (int)D_POP_INDICATOR_NONE) ||
         (indicator > (int)D_POP_INDICATOR_CONTINUE) )
    {
        return D_POP_STATUS_MALFORMED;
    }

    const char* name = d_pop_resp_code_name(_response->code);

    *_out_token = tokens[indicator];
    *_out_code  = (_response->code_text.length > 0)
                ? _response->code_text
                : d_internal_pop_text(name, (name != NULL) ? strlen(name) : 0);

    // a code belongs on -ERR only, and neither part may break the line
    if ( (!d_internal_pop_text_ok(_response->text))                  ||
         (!d_internal_pop_text_ok(*_out_code))                       ||
         (!d_internal_pop_text_is_clean(_response->text, true))      ||
         (!d_internal_pop_text_is_clean(*_out_code, false))          ||
         (d_internal_pop_text_has(*_out_code, ']'))                  ||
         ( (_out_code->length > 0) &&
           (_response->indicator != D_POP_INDICATOR_ERR) ) )
    {
        return D_POP_STATUS_MALFORMED;
    }

    return D_POP_STATUS_OK;
}

/*
d_pop_response_format
  A continuation always carries its SP (RFC 5034 section 4 spells an empty
challenge "+ "); other indicators carry one only when text or a code follows.
*/
enum d_pop_status
d_pop_response_format(
    const struct d_pop_response* _response,
    char*                        _out,
    size_t                       _capacity,
    size_t*                      _out_size
)
{
    // parameter validation
    if ( (_response == NULL) ||
         (!d_internal_pop_out_args_ok(_out, _capacity, _out_size)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    const char*        token  = NULL;
    struct d_pack_text code   = { NULL, 0 };
    enum d_pop_status  status = d_internal_pop_response_parts(_response,
                                                              &token,
                                                              &code);

    // an invalid response reports nothing further
    if (status != D_POP_STATUS_OK)
    {
        return status;
    }

    const size_t       token_length = strlen(token);
    const struct d_pack_text text   = _response->text;
    const bool         space        = ( (text.length > 0) ||
                                        (_response->indicator ==
                                         D_POP_INDICATOR_CONTINUE) );
    const size_t       size         = token_length
                                    + ((code.length > 0) ? code.length + 3 : 0)
                                    + (space ? text.length + 1 : 0)
                                    + 2;

    // RFC 2449's limit applies whatever buffer the caller has
    if (size > D_POP_RESPONSE_MAX)
    {
        *_out_size = size;

        return D_POP_STATUS_LINE_TOO_LONG;
    }

    status = d_internal_pop_out_check(_out, _capacity, size, _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_POP_STATUS_OK) ||
         (_out == NULL) )
    {
        return status;
    }

    size_t pos = 0;

    d_internal_pop_put(_out, &pos, token, token_length);

    // the bracketed code, when there is one
    if (code.length > 0)
    {
        d_internal_pop_put(_out, &pos, " [", 2);
        d_internal_pop_put(_out, &pos, code.data, code.length);
        d_internal_pop_put(_out, &pos, "]", 1);
    }

    // the separator and text, when either is due
    if (space)
    {
        d_internal_pop_put(_out, &pos, " ", 1);
        d_internal_pop_put(_out, &pos, text.data, text.length);
    }

    d_internal_pop_put(_out, &pos, D_POP_TOKEN_CRLF, 2);

    return D_POP_STATUS_OK;
}

//==============================================================================
// 7.  LISTING CODEC
//==============================================================================

/*
d_pop_parse_decimal
  Digits only, with the overflow check done before the multiply so the
accumulator never wraps. Leading zeros are accepted; RFC 1939 does not forbid
them and some servers pad.
*/
enum d_pop_status
d_pop_parse_decimal(
    struct d_pack_text _text,
    uint64_t           _maximum,
    uint64_t*          _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_text)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    // an empty field is not zero
    if (_text.length == 0)
    {
        return D_POP_STATUS_MALFORMED;
    }

    uint64_t value = 0;

    // accumulate, refusing any digit that would pass the maximum
    for (size_t i = 0; i < _text.length; ++i)
    {
        const char c = _text.data[i];

        if ( (c < '0') ||
             (c > '9') )
        {
            return D_POP_STATUS_MALFORMED;
        }

        const uint64_t digit = (uint64_t)(c - '0');

        if ( (digit > _maximum) ||
             (value > (_maximum - digit) / 10u) )
        {
            return D_POP_STATUS_MALFORMED;
        }

        value = (value * 10u) + digit;
    }

    *_out = value;

    return D_POP_STATUS_OK;
}

/*
d_internal_pop_parse_pair
  The first two fields of a listing line as numbers, with the first bounded by
`_first_min`. Trailing text is ignored, as the header documents.
*/
static enum d_pop_status
d_internal_pop_parse_pair(
    struct d_pack_text _text,
    uint64_t           _first_min,
    uint32_t*          _out_first,
    uint64_t*          _out_second
)
{
    struct d_pack_text       rest   = _text;
    const struct d_pack_text first  = d_internal_pop_next_token(&rest);
    const struct d_pack_text second = d_internal_pop_next_token(&rest);
    uint64_t                 a      = 0;
    uint64_t                 b      = 0;

    // both fields must be in range
    if ( (d_pop_parse_decimal(first, UINT32_MAX, &a) != D_POP_STATUS_OK)  ||
         (a < _first_min)                                                 ||
         (d_pop_parse_decimal(second, UINT64_MAX, &b) != D_POP_STATUS_OK) )
    {
        return D_POP_STATUS_MALFORMED;
    }

    *_out_first  = (uint32_t)a;
    *_out_second = b;

    return D_POP_STATUS_OK;
}

/*
d_pop_maildrop_parse
  "count octets". The count may be zero; an empty maildrop is valid.
*/
enum d_pop_status
d_pop_maildrop_parse(
    struct d_pack_text     _text,
    struct d_pop_maildrop* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_text)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    return d_internal_pop_parse_pair(_text, 0, &_out->count, &_out->octets);
}

/*
d_pop_scan_listing_parse
  "number octets". Message numbers start at 1 (RFC 1939 section 5).
*/
enum d_pop_status
d_pop_scan_listing_parse(
    struct d_pack_text         _text,
    struct d_pop_scan_listing* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_text)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    return d_internal_pop_parse_pair(_text, 1, &_out->number, &_out->octets);
}

/*
d_pop_uid_is_valid
  RFC 1939 section 7: 1 to 70 characters in the range 0x21 to 0x7E.
*/
bool
d_pop_uid_is_valid(
    struct d_pack_text _uid
)
{
    // the length bound comes first
    if ( (_uid.length == 0)            ||
         (_uid.length > D_POP_UID_MAX) ||
         (!d_internal_pop_text_ok(_uid)) )
    {
        return false;
    }

    // every character must be printable and not SP
    for (size_t i = 0; i < _uid.length; ++i)
    {
        const unsigned char c = (unsigned char)_uid.data[i];

        if ( (c < 0x21) ||
             (c > 0x7E) )
        {
            return false;
        }
    }

    return true;
}

/*
d_pop_uid_listing_parse
  "number uid". The uid is validated before it is copied, so a malformed one
never reaches the caller's record.
*/
enum d_pop_status
d_pop_uid_listing_parse(
    struct d_pack_text        _text,
    struct d_pop_uid_listing* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_text)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    struct d_pack_text       rest   = _text;
    const struct d_pack_text number = d_internal_pop_next_token(&rest);
    const struct d_pack_text uid    = d_internal_pop_next_token(&rest);
    uint64_t                 value  = 0;

    // a positive message number and a well-formed uid
    if ( (d_pop_parse_decimal(number, UINT32_MAX, &value) != D_POP_STATUS_OK) ||
         (value == 0)                                                         ||
         (!d_pop_uid_is_valid(uid)) )
    {
        return D_POP_STATUS_MALFORMED;
    }

    _out->number = (uint32_t)value;

    return d_internal_pop_copy_text(_out->uid, sizeof(_out->uid), uid);
}

/*
d_pop_maildrop_format
  snprintf into scratch; see d_internal_pop_emit_scratch for why.
*/
enum d_pop_status
d_pop_maildrop_format(
    const struct d_pop_maildrop* _maildrop,
    char*                        _out,
    size_t                       _capacity,
    size_t*                      _out_size
)
{
    // parameter validation
    if ( (_maildrop == NULL) ||
         (!d_internal_pop_out_args_ok(_out, _capacity, _out_size)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    char      scratch[48];
    const int written = snprintf(scratch,
                                 sizeof(scratch),
                                 "%" PRIu32 " %" PRIu64,
                                 _maildrop->count,
                                 _maildrop->octets);

    return d_internal_pop_emit_scratch(scratch,
                                       written,
                                       _out,
                                       _capacity,
                                       _out_size);
}

/*
d_pop_scan_listing_format
  As d_pop_maildrop_format, with message number 0 refused.
*/
enum d_pop_status
d_pop_scan_listing_format(
    const struct d_pop_scan_listing* _listing,
    char*                            _out,
    size_t                           _capacity,
    size_t*                          _out_size
)
{
    // parameter validation
    if ( (_listing == NULL) ||
         (!d_internal_pop_out_args_ok(_out, _capacity, _out_size)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    // message numbers start at 1
    if (_listing->number == 0)
    {
        return D_POP_STATUS_MALFORMED;
    }

    char      scratch[48];
    const int written = snprintf(scratch,
                                 sizeof(scratch),
                                 "%" PRIu32 " %" PRIu64,
                                 _listing->number,
                                 _listing->octets);

    return d_internal_pop_emit_scratch(scratch,
                                       written,
                                       _out,
                                       _capacity,
                                       _out_size);
}

/*
d_pop_uid_listing_format
  The uid is validated from the record's own C string, so a record assembled by
hand gets the same checks as one produced by the parser.
*/
enum d_pop_status
d_pop_uid_listing_format(
    const struct d_pop_uid_listing* _listing,
    char*                           _out,
    size_t                          _capacity,
    size_t*                         _out_size
)
{
    // parameter validation
    if ( (_listing == NULL) ||
         (!d_internal_pop_out_args_ok(_out, _capacity, _out_size)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    const char* terminator = memchr(_listing->uid, '\0', sizeof(_listing->uid));

    // a positive number and a well-formed, terminated uid
    if ( (_listing->number == 0)                                      ||
         (terminator == NULL)                                         ||
         (!d_pop_uid_is_valid(
              d_internal_pop_text(_listing->uid,
                                  (size_t)(terminator - _listing->uid)))) )
    {
        return D_POP_STATUS_MALFORMED;
    }

    char      scratch[D_POP_UID_MAX + 16];
    const int written = snprintf(scratch,
                                 sizeof(scratch),
                                 "%" PRIu32 " %s",
                                 _listing->number,
                                 _listing->uid);

    return d_internal_pop_emit_scratch(scratch,
                                       written,
                                       _out,
                                       _capacity,
                                       _out_size);
}

//==============================================================================
// 8.  TRANSPORT
//==============================================================================

/*
d_internal_pop_memory_read
  Serves the input in pieces of at most `chunk`, so a test can prove a reader
reassembles lines split anywhere.
*/
static enum d_pop_status
d_internal_pop_memory_read(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_read
)
{
    struct d_pop_memory_transport* memory    = _context;
    const size_t                   remaining = memory->input_size -
                                               memory->input_offset;

    *_out_read = 0;

    // the end of the input is either a close or a stall
    if (remaining == 0)
    {
        return (memory->block_at_end != 0) ? D_POP_STATUS_WOULD_BLOCK
                                           : D_POP_STATUS_CONNECTION_CLOSED;
    }

    size_t count = (_capacity < remaining) ? _capacity
                                           : remaining;

    // honor the fragmentation cap
    if ( (memory->chunk != 0) &&
         (count > memory->chunk) )
    {
        count = memory->chunk;
    }

    memcpy(_buffer, memory->input + memory->input_offset, count);
    memory->input_offset += count;
    *_out_read            = count;

    return D_POP_STATUS_OK;
}

/*
d_internal_pop_memory_write
  Forwards to the output sink. A NULL sink discards, matching the rest of this
module; a refusing sink is a transport error, since from the session's point of
view the peer stopped listening.
*/
static enum d_pop_status
d_internal_pop_memory_write(
    void*       _context,
    const void* _data,
    size_t      _size,
    size_t*     _out_written
)
{
    const struct d_pop_memory_transport* memory = _context;

    *_out_written = 0;

    // refused bytes are not written
    if (d_internal_pop_emit(memory->output, _data, _size) != D_POP_STATUS_OK)
    {
        return D_POP_STATUS_TRANSPORT_ERROR;
    }

    *_out_written = _size;

    return D_POP_STATUS_OK;
}

/*
d_pop_memory_transport_init
  Zero offset, no fragmentation cap, and close (not stall) at end of input.
*/
void
d_pop_memory_transport_init(
    struct d_pop_memory_transport* _memory,
    const void*                    _input,
    size_t                         _input_size,
    struct d_pack_sink             _output
)
{
    // nothing to initialize
    if (_memory == NULL)
    {
        return;
    }

    _memory->input        = _input;
    _memory->input_size   = (_input != NULL) ? _input_size
                                             : 0;
    _memory->input_offset = 0;
    _memory->chunk        = 0;
    _memory->block_at_end = 0;
    _memory->output       = _output;

    return;
}

/*
d_pop_transport_from_memory
  Binds the two memory callbacks to the context.
*/
struct d_pop_transport
d_pop_transport_from_memory(
    struct d_pop_memory_transport* _memory
)
{
    struct d_pop_transport transport =
    {
        d_internal_pop_memory_read,
        d_internal_pop_memory_write,
        _memory
    };

    return transport;
}

/*
d_pop_transport_write_all
  Loops over short writes. A transport that reports success while writing
nothing would spin this loop forever, so that is treated as an error.
*/
enum d_pop_status
d_pop_transport_write_all(
    struct d_pop_transport _transport,
    const void*            _data,
    size_t                 _size,
    size_t*                _out_written
)
{
    // parameter validation
    if ( (_transport.write == NULL) ||
         ( (_data == NULL) &&
           (_size != 0) ) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    const unsigned char* bytes  = _data;
    size_t               done   = 0;
    enum d_pop_status    status = D_POP_STATUS_OK;

    // write until everything is out or the transport stops us
    while (done < _size)
    {
        size_t written = 0;

        status = _transport.write(_transport.context,
                                  bytes + done,
                                  _size - done,
                                  &written);
        done  += written;

        if (status != D_POP_STATUS_OK)
        {
            break;
        }

        if (written == 0)
        {
            status = D_POP_STATUS_TRANSPORT_ERROR;
            break;
        }
    }

    // report progress so a caller can resume
    if (_out_written != NULL)
    {
        *_out_written = done;
    }

    return status;
}

/*
d_pop_send_command
  The buffer is exactly D_POP_COMMAND_MAX, which the formatter never exceeds.
*/
enum d_pop_status
d_pop_send_command(
    struct d_pop_transport           _transport,
    const struct d_pop_command_line* _command
)
{
    char              line[D_POP_COMMAND_MAX];
    size_t            size   = 0;
    enum d_pop_status status = d_pop_command_format(_command,
                                                    line,
                                                    sizeof(line),
                                                    &size);

    // a command that cannot be formatted is not sent
    if (status != D_POP_STATUS_OK)
    {
        return status;
    }

    return d_pop_transport_write_all(_transport, line, size, NULL);
}

/*
d_pop_send_response
  The buffer is exactly D_POP_RESPONSE_MAX, which the formatter never exceeds.
*/
enum d_pop_status
d_pop_send_response(
    struct d_pop_transport       _transport,
    const struct d_pop_response* _response
)
{
    char              line[D_POP_RESPONSE_MAX];
    size_t            size   = 0;
    enum d_pop_status status = d_pop_response_format(_response,
                                                     line,
                                                     sizeof(line),
                                                     &size);

    // a response that cannot be formatted is not sent
    if (status != D_POP_STATUS_OK)
    {
        return status;
    }

    return d_pop_transport_write_all(_transport, line, size, NULL);
}

//==============================================================================
// 9.  MULTI-LINE BODIES
//==============================================================================

// d_internal_pop_decode_state
//   decoder states. LINE_START follows a line end; DOT and DOT_CR have seen a
// leading "." and ".\r" that may yet turn out to be the terminator.
enum d_internal_pop_decode_state
{
    D_INTERNAL_POP_DECODE_LINE_START = 0,
    D_INTERNAL_POP_DECODE_DOT        = 1,
    D_INTERNAL_POP_DECODE_DOT_CR     = 2,
    D_INTERNAL_POP_DECODE_IN_LINE    = 3,
    D_INTERNAL_POP_DECODE_CR         = 4,
    D_INTERNAL_POP_DECODE_DONE       = 5
};

// d_internal_pop_decode_action
//   what the decode loop does with the current byte. KEEP extends the current
// run of content; DROP ends the run and discards the byte; REPLAY re-examines
// the byte in the new state; REPLAY_CR first emits a withheld CR.
enum d_internal_pop_decode_action
{
    D_INTERNAL_POP_DECODE_KEEP      = 0,
    D_INTERNAL_POP_DECODE_DROP      = 1,
    D_INTERNAL_POP_DECODE_REPLAY    = 2,
    D_INTERNAL_POP_DECODE_REPLAY_CR = 3
};

/*
d_internal_pop_decode_step
  The transition function, kept free of I/O so the state machine can be read
on its own. A bare LF ends a line only when not strict. A "." at line start is
always dropped: RFC 1939 strips the termination octet whenever anything other
than CRLF follows, stuffed or not.
*/
static enum d_internal_pop_decode_action
d_internal_pop_decode_step(
    struct d_pop_body_decoder* _decoder,
    char                       _c
)
{
    const bool lf_ends = ( (_c == '\n') &&
                           (_decoder->strict == 0) );

    switch (_decoder->state)
    {
        case D_INTERNAL_POP_DECODE_LINE_START:
        {
            _decoder->state = (_c == '.')  ? D_INTERNAL_POP_DECODE_DOT
                            : (_c == '\r') ? D_INTERNAL_POP_DECODE_CR
                            : lf_ends      ? D_INTERNAL_POP_DECODE_LINE_START
                                           : D_INTERNAL_POP_DECODE_IN_LINE;

            return (_c == '.') ? D_INTERNAL_POP_DECODE_DROP
                               : D_INTERNAL_POP_DECODE_KEEP;
        }
        case D_INTERNAL_POP_DECODE_DOT:
        {
            _decoder->state = (_c == '\r') ? D_INTERNAL_POP_DECODE_DOT_CR
                            : lf_ends      ? D_INTERNAL_POP_DECODE_DONE
                                           : D_INTERNAL_POP_DECODE_IN_LINE;

            return ( (_c == '\r') || lf_ends ) ? D_INTERNAL_POP_DECODE_DROP
                                               : D_INTERNAL_POP_DECODE_REPLAY;
        }
        case D_INTERNAL_POP_DECODE_DOT_CR:
        {
            _decoder->state = (_c == '\n') ? D_INTERNAL_POP_DECODE_DONE
                                           : D_INTERNAL_POP_DECODE_CR;

            return (_c == '\n') ? D_INTERNAL_POP_DECODE_DROP
                                : D_INTERNAL_POP_DECODE_REPLAY_CR;
        }
        case D_INTERNAL_POP_DECODE_CR:
        {
            _decoder->state = (_c == '\n') ? D_INTERNAL_POP_DECODE_LINE_START
                            : (_c == '\r') ? D_INTERNAL_POP_DECODE_CR
                                           : D_INTERNAL_POP_DECODE_IN_LINE;

            return D_INTERNAL_POP_DECODE_KEEP;
        }
        default:
        {
            _decoder->state = (_c == '\r') ? D_INTERNAL_POP_DECODE_CR
                            : lf_ends      ? D_INTERNAL_POP_DECODE_LINE_START
                                           : D_INTERNAL_POP_DECODE_IN_LINE;

            return D_INTERNAL_POP_DECODE_KEEP;
        }
    }
}

/*
d_pop_body_decoder_init
  Bodies begin at a line start: the status line's CRLF has just been read.
*/
void
d_pop_body_decoder_init(
    struct d_pop_body_decoder* _decoder
)
{
    // nothing to initialize
    if (_decoder == NULL)
    {
        return;
    }

    _decoder->state  = D_INTERNAL_POP_DECODE_LINE_START;
    _decoder->strict = D_INTERNAL_POP_STRICT;
    _decoder->octets = 0;

    return;
}

/*
d_pop_body_decoder_is_done
  True once the terminator has been consumed.
*/
bool
d_pop_body_decoder_is_done(
    const struct d_pop_body_decoder* _decoder
)
{
    return ( (_decoder != NULL) &&
             (_decoder->state == D_INTERNAL_POP_DECODE_DONE) );
}

/*
d_internal_pop_decode_flush
  Emits the pending run of content and counts it.
*/
static enum d_pop_status
d_internal_pop_decode_flush(
    struct d_pop_body_decoder* _decoder,
    struct d_pack_sink         _sink,
    const char*                _run,
    size_t                     _length
)
{
    const enum d_pop_status status = d_internal_pop_emit(_sink, _run, _length);

    // only accepted bytes count
    if (status == D_POP_STATUS_OK)
    {
        _decoder->octets += _length;
    }

    return status;
}

/*
d_pop_body_decode
  Content is emitted in runs, not bytes: the only bytes the decoder removes are
the stuffed dots and the terminator, so a body arrives at the sink in as few
writes as its line structure allows.
*/
enum d_pop_status
d_pop_body_decode(
    struct d_pop_body_decoder* _decoder,
    const void*                _data,
    size_t                     _size,
    struct d_pack_sink         _sink,
    size_t*                    _consumed
)
{
    // parameter validation
    if ( (_decoder == NULL)  ||
         (_consumed == NULL) ||
         ( (_data == NULL) &&
           (_size != 0) ) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    const char*       bytes  = _data;
    size_t            run    = 0;
    size_t            i      = 0;
    enum d_pop_status status = D_POP_STATUS_OK;

    // walk the chunk until it ends or the body does
    while ( (i < _size)                                         &&
            (_decoder->state != D_INTERNAL_POP_DECODE_DONE)     &&
            (status == D_POP_STATUS_OK) )
    {
        const enum d_internal_pop_decode_action action =
            d_internal_pop_decode_step(_decoder, bytes[i]);

        if (action == D_INTERNAL_POP_DECODE_KEEP)
        {
            ++i;
        }
        else if (action == D_INTERNAL_POP_DECODE_DROP)
        {
            status = d_internal_pop_decode_flush(_decoder,
                                                 _sink,
                                                 bytes + run,
                                                 i - run);
            run    = ++i;
        }
        else if (action == D_INTERNAL_POP_DECODE_REPLAY_CR)
        {
            status = d_internal_pop_decode_flush(_decoder, _sink, "\r", 1);
        }
    }

    // flush whatever content remains in the run
    if (status == D_POP_STATUS_OK)
    {
        status = d_internal_pop_decode_flush(_decoder,
                                             _sink,
                                             bytes + run,
                                             i - run);
    }

    *_consumed = i;

    return status;
}

/*
d_pop_body_encoder_init
  An empty body is a line start, so finishing at once writes only ".CRLF".
*/
void
d_pop_body_encoder_init(
    struct d_pop_body_encoder* _encoder
)
{
    // nothing to initialize
    if (_encoder == NULL)
    {
        return;
    }

    _encoder->at_line_start = 1;
    _encoder->after_cr      = 0;
    _encoder->normalize     = 1;
    _encoder->finished      = 0;
    _encoder->octets        = 0;

    return;
}

/*
d_internal_pop_encode_emit
  Emits and counts, as the decoder's flush does.
*/
static enum d_pop_status
d_internal_pop_encode_emit(
    struct d_pop_body_encoder* _encoder,
    struct d_pack_sink         _sink,
    const char*                _data,
    size_t                     _length
)
{
    const enum d_pop_status status = d_internal_pop_emit(_sink,
                                                         _data,
                                                         _length);

    // only accepted bytes count
    if (status == D_POP_STATUS_OK)
    {
        _encoder->octets += _length;
    }

    return status;
}

/*
d_pop_body_encode
  Two edits, each made by ending the current run: a "." gains a second "." at
line start, and (when normalizing) a bare LF becomes CRLF. A line start is
recognized after any LF, so a body stored with bare LF is stuffed correctly
even with normalization off; with it off, the caller is expected to supply
CRLF text.
*/
enum d_pop_status
d_pop_body_encode(
    struct d_pop_body_encoder* _encoder,
    const void*                _data,
    size_t                     _size,
    struct d_pack_sink         _sink
)
{
    // parameter validation
    if ( (_encoder == NULL)          ||
         (_encoder->finished != 0)   ||
         ( (_data == NULL) &&
           (_size != 0) ) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    const char*       bytes  = _data;
    size_t            run    = 0;
    enum d_pop_status status = D_POP_STATUS_OK;

    // walk the chunk, ending the run wherever an edit is due
    for (size_t i = 0; (i < _size) && (status == D_POP_STATUS_OK); ++i)
    {
        const char c        = bytes[i];
        const bool stuff    = ( (_encoder->at_line_start != 0) &&
                                (c == '.') );
        const bool bare_lf  = ( (c == '\n')                &&
                                (_encoder->after_cr == 0)  &&
                                (_encoder->normalize != 0) );

        if ( (stuff) ||
             (bare_lf) )
        {
            status = d_internal_pop_encode_emit(_encoder,
                                                _sink,
                                                bytes + run,
                                                i - run);
            status = (status == D_POP_STATUS_OK)
                   ? d_internal_pop_encode_emit(_encoder,
                                                _sink,
                                                stuff ? "." : "\r\n",
                                                stuff ? 1u : 2u)
                   : status;
            run    = bare_lf ? i + 1
                             : i;
        }

        _encoder->at_line_start = (c == '\n') ? 1 : 0;
        _encoder->after_cr      = (c == '\r') ? 1 : 0;
    }

    // flush the tail of the run
    if (status == D_POP_STATUS_OK)
    {
        status = d_internal_pop_encode_emit(_encoder,
                                            _sink,
                                            bytes + run,
                                            _size - run);
    }

    return status;
}

/*
d_pop_body_encode_finish
  A body whose last line lacks its ending gets one, completing a dangling CR if
that is all that is missing, so the terminator always sits on a line of its
own.
*/
enum d_pop_status
d_pop_body_encode_finish(
    struct d_pop_body_encoder* _encoder,
    struct d_pack_sink         _sink
)
{
    // parameter validation
    if ( (_encoder == NULL) ||
         (_encoder->finished != 0) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    enum d_pop_status status = D_POP_STATUS_OK;

    // close an unterminated last line
    if (_encoder->at_line_start == 0)
    {
        status = (_encoder->after_cr != 0)
               ? d_internal_pop_encode_emit(_encoder, _sink, "\n", 1)
               : d_internal_pop_encode_emit(_encoder, _sink, "\r\n", 2);
    }

    // the terminator itself is framing, not content, so it is not counted
    if (status == D_POP_STATUS_OK)
    {
        status = d_internal_pop_emit(_sink, D_POP_TOKEN_TERMINATOR, 3);
    }

    _encoder->finished = 1;

    return status;
}

//==============================================================================
// 10. READER
//==============================================================================

/*
d_pop_reader_init
  An empty buffer at the configured default strictness.
*/
void
d_pop_reader_init(
    struct d_pop_reader* _reader
)
{
    // nothing to initialize
    if (_reader == NULL)
    {
        return;
    }

    _reader->start      = 0;
    _reader->end        = 0;
    _reader->scan       = 0;
    _reader->strict     = D_INTERNAL_POP_STRICT;
    _reader->discarding = 0;

    return;
}

/*
d_pop_reader_buffered
  Bytes read from the transport and not yet consumed. Non-zero after a body
means the peer pipelined; a derived module about to hand the stream to TLS
after STLS must see zero here, or plaintext would be misread as a handshake.
*/
size_t
d_pop_reader_buffered(
    const struct d_pop_reader* _reader
)
{
    return (_reader != NULL) ? (_reader->end - _reader->start)
                             : 0;
}

/*
d_internal_pop_reader_fill
  Makes room and pulls once from the transport. Room is made by compacting only
when the buffer is full, which is the only time it is needed. A full buffer
with nothing consumed is a line longer than the buffer: it is dropped and the
reader switches to discarding until the next LF, reporting the overflow once.
*/
static enum d_pop_status
d_internal_pop_reader_fill(
    struct d_pop_reader*   _reader,
    struct d_pop_transport _transport
)
{
    // make room at the end of the buffer
    if (_reader->end == sizeof(_reader->buffer))
    {
        if (_reader->start > 0)
        {
            memmove(_reader->buffer,
                    _reader->buffer + _reader->start,
                    _reader->end - _reader->start);
            _reader->end  -= _reader->start;
            _reader->scan -= _reader->start;
            _reader->start = 0;
        }
        else
        {
            const bool first  = (_reader->discarding == 0);

            _reader->end        = 0;
            _reader->scan       = 0;
            _reader->discarding = 1;

            if (first)
            {
                return D_POP_STATUS_LINE_TOO_LONG;
            }
        }
    }

    size_t                  count  = 0;
    const enum d_pop_status status =
        _transport.read(_transport.context,
                        _reader->buffer + _reader->end,
                        sizeof(_reader->buffer) - _reader->end,
                        &count);

    // only a successful read adds bytes
    if (status == D_POP_STATUS_OK)
    {
        _reader->end += count;
    }

    return status;
}

/*
d_internal_pop_reader_take
  Consumes the line ending at buffer index `_newline`. Returns false when the
line was the tail of an over-long one and was discarded rather than delivered.
*/
static bool
d_internal_pop_reader_take(
    struct d_pop_reader* _reader,
    size_t               _newline,
    struct d_pack_text*  _out_line,
    enum d_pop_status*   _out_status
)
{
    const size_t begin = _reader->start;
    size_t       end   = _newline;

    _reader->start = _newline + 1;
    _reader->scan  = _reader->start;

    // the remainder of an over-long line is dropped, and reading resumes
    if (_reader->discarding != 0)
    {
        _reader->discarding = 0;

        return false;
    }

    // strip the CR, or refuse its absence in strict mode
    if ( (end > begin) &&
         (_reader->buffer[end - 1] == '\r') )
    {
        --end;
        *_out_status = D_POP_STATUS_OK;
    }
    else
    {
        *_out_status = (_reader->strict != 0) ? D_POP_STATUS_MALFORMED
                                              : D_POP_STATUS_OK;
    }

    *_out_line = d_internal_pop_text(_reader->buffer + begin, end - begin);

    return true;
}

/*
d_pop_reader_read_line
  `scan` remembers how far the buffer has already been searched, so bytes are
examined once however many reads a line takes to arrive.
*/
enum d_pop_status
d_pop_reader_read_line(
    struct d_pop_reader*   _reader,
    struct d_pop_transport _transport,
    struct d_pack_text*    _out_line
)
{
    // parameter validation
    if ( (_reader == NULL)         ||
         (_out_line == NULL)       ||
         (_transport.read == NULL) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    // search what is buffered, and pull more until a line appears
    for (;;)
    {
        const char* newline = memchr(_reader->buffer + _reader->scan,
                                     '\n',
                                     _reader->end - _reader->scan);

        if (newline != NULL)
        {
            enum d_pop_status status = D_POP_STATUS_OK;

            if (d_internal_pop_reader_take(_reader,
                                           (size_t)(newline - _reader->buffer),
                                           _out_line,
                                           &status))
            {
                return status;
            }

            continue;
        }

        // the tail of an over-long line is dropped as it arrives
        if (_reader->discarding != 0)
        {
            _reader->start = 0;
            _reader->end   = 0;
        }

        _reader->scan = _reader->end;

        const enum d_pop_status status = d_internal_pop_reader_fill(_reader,
                                                                    _transport);

        if (status != D_POP_STATUS_OK)
        {
            return status;
        }
    }
}

/*
d_pop_reader_read_body
  Buffered bytes are decoded before any read, because the body usually began in
the same read as its status line. The decoder stops at the terminator, so
whatever follows stays buffered for the next read_line.
*/
enum d_pop_status
d_pop_reader_read_body(
    struct d_pop_reader*       _reader,
    struct d_pop_transport     _transport,
    struct d_pop_body_decoder* _decoder,
    struct d_pack_sink         _sink
)
{
    // parameter validation
    if ( (_reader == NULL)         ||
         (_decoder == NULL)        ||
         (_transport.read == NULL) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    // decode what is buffered, and pull more until the body ends
    while (!d_pop_body_decoder_is_done(_decoder))
    {
        if (_reader->start < _reader->end)
        {
            size_t                  consumed = 0;
            const enum d_pop_status status   =
                d_pop_body_decode(_decoder,
                                  _reader->buffer + _reader->start,
                                  _reader->end - _reader->start,
                                  _sink,
                                  &consumed);

            _reader->start += consumed;
            _reader->scan   = _reader->start;

            if (status != D_POP_STATUS_OK)
            {
                return status;
            }

            continue;
        }

        _reader->start = 0;
        _reader->end   = 0;
        _reader->scan  = 0;

        const enum d_pop_status status = d_internal_pop_reader_fill(_reader,
                                                                    _transport);

        if (status != D_POP_STATUS_OK)
        {
            return status;
        }
    }

    return D_POP_STATUS_OK;
}

//==============================================================================
// 11. APOP
//==============================================================================

/*
d_pop_apop_timestamp
  The first "<" and the first ">" after it, with an "@" between and no SP: the
msg-id shape RFC 1939 section 7 requires. Available with APOP compiled out,
since recognizing that a server offers APOP needs no digest.
*/
enum d_pop_status
d_pop_apop_timestamp(
    struct d_pack_text  _greeting,
    struct d_pack_text* _out_timestamp
)
{
    // parameter validation
    if ( (_out_timestamp == NULL) ||
         (!d_internal_pop_text_ok(_greeting)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    // an empty greeting has no timestamp, and must not reach memchr
    if (_greeting.length == 0)
    {
        return D_POP_STATUS_UNSUPPORTED;
    }

    const char* open = memchr(_greeting.data, '<', _greeting.length);

    // no opening bracket, no timestamp
    if (open == NULL)
    {
        return D_POP_STATUS_UNSUPPORTED;
    }

    const size_t offset = (size_t)(open - _greeting.data);
    const char*  close  = memchr(open, '>', _greeting.length - offset);

    // an unterminated bracket is not a timestamp
    if (close == NULL)
    {
        return D_POP_STATUS_UNSUPPORTED;
    }

    const struct d_pack_text stamp =
        d_internal_pop_text(open, (size_t)(close - open) + 1);

    // the msg-id shape: an @, and nothing that would split a token
    if ( (memchr(stamp.data, '@', stamp.length) == NULL) ||
         (!d_internal_pop_text_is_clean(stamp, false)) )
    {
        return D_POP_STATUS_UNSUPPORTED;
    }

    *_out_timestamp = stamp;

    return D_POP_STATUS_OK;
}

#if D_INTERNAL_POP_APOP

// d_internal_pop_md5
//   the running state of one MD5 computation (RFC 1321).
struct d_internal_pop_md5
{
    uint32_t      state[4];
    uint64_t      length;
    unsigned char block[64];
    size_t        used;
};

// d_internal_pop_md5_k
//   the per-round additive constants, floor(|sin(i + 1)| * 2^32).
static const uint32_t d_internal_pop_md5_k[64] =
{
    0xD76AA478u, 0xE8C7B756u, 0x242070DBu, 0xC1BDCEEEu,
    0xF57C0FAFu, 0x4787C62Au, 0xA8304613u, 0xFD469501u,
    0x698098D8u, 0x8B44F7AFu, 0xFFFF5BB1u, 0x895CD7BEu,
    0x6B901122u, 0xFD987193u, 0xA679438Eu, 0x49B40821u,
    0xF61E2562u, 0xC040B340u, 0x265E5A51u, 0xE9B6C7AAu,
    0xD62F105Du, 0x02441453u, 0xD8A1E681u, 0xE7D3FBC8u,
    0x21E1CDE6u, 0xC33707D6u, 0xF4D50D87u, 0x455A14EDu,
    0xA9E3E905u, 0xFCEFA3F8u, 0x676F02D9u, 0x8D2A4C8Au,
    0xFFFA3942u, 0x8771F681u, 0x6D9D6122u, 0xFDE5380Cu,
    0xA4BEEA44u, 0x4BDECFA9u, 0xF6BB4B60u, 0xBEBFBC70u,
    0x289B7EC6u, 0xEAA127FAu, 0xD4EF3085u, 0x04881D05u,
    0xD9D4D039u, 0xE6DB99E5u, 0x1FA27CF8u, 0xC4AC5665u,
    0xF4292244u, 0x432AFF97u, 0xAB9423A7u, 0xFC93A039u,
    0x655B59C3u, 0x8F0CCC92u, 0xFFEFF47Du, 0x85845DD1u,
    0x6FA87E4Fu, 0xFE2CE6E0u, 0xA3014314u, 0x4E0811A1u,
    0xF7537E82u, 0xBD3AF235u, 0x2AD7D2BBu, 0xEB86D391u
};

// d_internal_pop_md5_r
//   the per-round left-rotation amounts.
static const unsigned char d_internal_pop_md5_r[64] =
{
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

/*
d_internal_pop_md5_transform
  One 64-byte block, written as the loop form of RFC 1321 section 3.4: the four
rounds differ only in their mixing function and message-word order. Words are
assembled little-endian by hand, so host byte order never matters.
*/
static void
d_internal_pop_md5_transform(
    uint32_t            _state[4],
    const unsigned char _block[64]
)
{
    uint32_t words[16];

    // load the block as little-endian words
    for (size_t i = 0; i < 16; ++i)
    {
        words[i] = ( (uint32_t)_block[(i * 4) + 0]         |
                     ((uint32_t)_block[(i * 4) + 1] << 8)  |
                     ((uint32_t)_block[(i * 4) + 2] << 16) |
                     ((uint32_t)_block[(i * 4) + 3] << 24) );
    }

    uint32_t a = _state[0];
    uint32_t b = _state[1];
    uint32_t c = _state[2];
    uint32_t d = _state[3];

    // sixty-four steps in four rounds of sixteen
    for (size_t i = 0; i < 64; ++i)
    {
        const size_t   round = i / 16;
        const uint32_t f     = (round == 0) ? ((b & c) | (~b & d))
                             : (round == 1) ? ((d & b) | (~d & c))
                             : (round == 2) ? (b ^ c ^ d)
                                            : (c ^ (b | ~d));
        const size_t   g     = (round == 0) ? i
                             : (round == 1) ? ((5 * i) + 1) % 16
                             : (round == 2) ? ((3 * i) + 5) % 16
                                            : (7 * i) % 16;
        const uint32_t sum   = a + f + d_internal_pop_md5_k[i] + words[g];
        const unsigned shift = d_internal_pop_md5_r[i];

        a = d;
        d = c;
        c = b;
        b = b + ((sum << shift) | (sum >> (32u - shift)));
    }

    _state[0] += a;
    _state[1] += b;
    _state[2] += c;
    _state[3] += d;

    return;
}

/*
d_internal_pop_md5_init
  The RFC 1321 initial chaining values.
*/
static void
d_internal_pop_md5_init(
    struct d_internal_pop_md5* _md5
)
{
    _md5->state[0] = 0x67452301u;
    _md5->state[1] = 0xEFCDAB89u;
    _md5->state[2] = 0x98BADCFEu;
    _md5->state[3] = 0x10325476u;
    _md5->length   = 0;
    _md5->used     = 0;

    return;
}

/*
d_internal_pop_md5_update
  Buffers input into whole blocks and transforms each as it completes.
*/
static void
d_internal_pop_md5_update(
    struct d_internal_pop_md5* _md5,
    const void*                _data,
    size_t                     _size
)
{
    const unsigned char* bytes = _data;

    _md5->length += (uint64_t)_size;

    // fill the block, transforming whenever it is full
    for (size_t i = 0; i < _size; ++i)
    {
        _md5->block[_md5->used++] = bytes[i];

        if (_md5->used == sizeof(_md5->block))
        {
            d_internal_pop_md5_transform(_md5->state, _md5->block);
            _md5->used = 0;
        }
    }

    return;
}

/*
d_internal_pop_md5_final
  Pads with 0x80, zeros, and the bit length (little-endian), then serializes
the state little-endian.
*/
static void
d_internal_pop_md5_final(
    struct d_internal_pop_md5* _md5,
    unsigned char              _out[16]
)
{
    const uint64_t      bits      = _md5->length * 8u;
    const unsigned char pad_start = 0x80;
    const unsigned char zero      = 0x00;
    unsigned char       length[8];

    // encode the bit length before padding changes _md5->length
    for (size_t i = 0; i < 8; ++i)
    {
        length[i] = (unsigned char)(bits >> (8 * i));
    }

    d_internal_pop_md5_update(_md5, &pad_start, 1);

    // pad with zeros until eight bytes remain in the block
    while (_md5->used != 56)
    {
        d_internal_pop_md5_update(_md5, &zero, 1);
    }

    d_internal_pop_md5_update(_md5, length, sizeof(length));

    // serialize the chaining values
    for (size_t i = 0; i < 16; ++i)
    {
        _out[i] = (unsigned char)(_md5->state[i / 4] >> (8 * (i % 4)));
    }

    return;
}

/*
d_pop_apop_digest
  MD5 over the timestamp immediately followed by the secret, with no separator,
rendered as lowercase hex (RFC 1939 section 7).
*/
enum d_pop_status
d_pop_apop_digest(
    struct d_pack_text _timestamp,
    struct d_pack_text _secret,
    char               _out[D_POP_APOP_DIGEST_LENGTH + 1]
)
{
    // parameter validation
    if ( (_out == NULL)                        ||
         (_timestamp.length == 0)              ||
         (!d_internal_pop_text_ok(_timestamp)) ||
         (!d_internal_pop_text_ok(_secret)) )
    {
        return D_POP_STATUS_INVALID_ARGUMENT;
    }

    static const char         hex[] = "0123456789abcdef";
    struct d_internal_pop_md5 md5;
    unsigned char             digest[16];

    d_internal_pop_md5_init(&md5);
    d_internal_pop_md5_update(&md5, _timestamp.data, _timestamp.length);
    d_internal_pop_md5_update(&md5, _secret.data, _secret.length);
    d_internal_pop_md5_final(&md5, digest);

    // render as lowercase hex
    for (size_t i = 0; i < sizeof(digest); ++i)
    {
        _out[(i * 2) + 0] = hex[digest[i] >> 4];
        _out[(i * 2) + 1] = hex[digest[i] & 0x0Fu];
    }

    _out[D_POP_APOP_DIGEST_LENGTH] = '\0';

    return D_POP_STATUS_OK;
}

/*
d_pop_apop_verify
  Recomputes and compares. The comparison accumulates differences instead of
returning at the first, so its timing does not reveal how much of a guess was
right.
*/
bool
d_pop_apop_verify(
    struct d_pack_text _timestamp,
    struct d_pack_text _secret,
    struct d_pack_text _digest
)
{
    char expected[D_POP_APOP_DIGEST_LENGTH + 1];

    // a digest of the wrong length, or one we cannot compute, fails
    if ( (_digest.length != D_POP_APOP_DIGEST_LENGTH)                ||
         (!d_internal_pop_text_ok(_digest))                          ||
         (d_pop_apop_digest(_timestamp, _secret, expected) !=
          D_POP_STATUS_OK) )
    {
        return false;
    }

    unsigned int difference = 0;

    // compare every character, case-folded
    for (size_t i = 0; i < D_POP_APOP_DIGEST_LENGTH; ++i)
    {
        difference |= (unsigned int)(unsigned char)
                      (d_internal_pop_lower(_digest.data[i]) ^ expected[i]);
    }

    return (difference == 0);
}

#else

/*
d_pop_apop_digest
  APOP is compiled out (D_CFG_POP_APOP is 0): no MD5 exists in this build.
*/
enum d_pop_status
d_pop_apop_digest(
    struct d_pack_text _timestamp,
    struct d_pack_text _secret,
    char               _out[D_POP_APOP_DIGEST_LENGTH + 1]
)
{
    (void)_timestamp;
    (void)_secret;

    // leave a caller's buffer holding a valid, empty string
    if (_out != NULL)
    {
        _out[0] = '\0';
    }

    return D_POP_STATUS_UNSUPPORTED;
}

/*
d_pop_apop_verify
  APOP is compiled out: nothing verifies.
*/
bool
d_pop_apop_verify(
    struct d_pack_text _timestamp,
    struct d_pack_text _secret,
    struct d_pack_text _digest
)
{
    (void)_timestamp;
    (void)_secret;
    (void)_digest;

    return false;
}

#endif  // D_INTERNAL_POP_APOP
