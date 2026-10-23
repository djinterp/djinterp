/*******************************************************************************
* djinterp [net]                                                           pop.c
*
* Implementation of the shared POP3 kernel declared in pop.h.
*   Pure computation over caller-owned memory: table lookups, the line and
* listing codecs, the body state machines, the reader, and the APOP digest,
* over net's own MD5. The only I/O is through the net.h connection and
* d_pack_sink the caller passes in.
*
*
* path:      /src/djinterp/net/pop/pop.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.25
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../inc/djinterp/net/pop/pop.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memchr, memcpy, strlen
// djinterp
#include "../../../../inc/djinterp/env/net/pop/env_pop.h"     // D_ENV_POP_CAN_PLAIN
#include "../../../../inc/djinterp/net/codec/codec_digest.h"  // d_net_digest_*
#include "../../../../inc/djinterp/net/codec/codec_hex.h"     // d_net_hex_*
#include "../../../../inc/djinterp/net/codec/codec_line.h"    // d_net_line_*
#include "../../../../inc/djinterp/net/net_common.h"          // connections
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t, PRIu32, PRIu64


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
        return d_internal_pop_text(_text.data,
                                   0);
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
        return d_internal_pop_text_after(_text,
                                         1);
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
        return d_internal_pop_text(_rest->data,
                                   0);
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

    *_rest = d_internal_pop_text_after(*_rest,
                                       end);

    return token;
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
             (memchr(_text.data,
                     _c,
                     _text.length) != NULL) );
}

/*
d_internal_pop_text_is_clean
  True when a span holds no CR, LF, or NUL -- the three bytes that would split
or truncate a line -- and, unless `_allow_space`, no SP, which would split an
argument in two.
*/
static bool
d_internal_pop_text_is_clean(
    struct d_pack_text _text,
    bool               _allow_space
)
{
    return ( (d_net_line_is_clean(_text)) &&
             ( (_allow_space) ||
               (!d_internal_pop_text_has(_text,
                                         ' ')) ) );
}

/*
d_internal_pop_copy_text
  Copies a span into a fixed array as a C string. On overflow it stores the
empty string rather than a truncated one, because a truncated mechanism list
reads as a valid, shorter list.
*/
static enum d_pop_error
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

        return D_POP_ERROR_BUFFER_TOO_SMALL;
    }

    // an empty span may have a NULL base, which memcpy must not see
    if (_text.length > 0)
    {
        memcpy(_dest,
               _text.data,
               _text.length);
    }

    _dest[_text.length] = '\0';

    return D_POP_OK;
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
static enum d_pop_error
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
        return D_POP_OK;
    }

    // the requirement is already recorded, so a caller can grow and retry
    if (_capacity < _size)
    {
        return D_POP_ERROR_BUFFER_TOO_SMALL;
    }

    return D_POP_OK;
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
        memcpy(_out + *_pos,
               _data,
               _length);
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
static enum d_pop_error
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
        return D_POP_ERROR_MALFORMED;
    }

    const size_t     size   = (size_t)_written;
    enum d_pop_error status = d_internal_pop_out_check(_out,
                                                       _capacity,
                                                       size,
                                                       _out_size);

    // nothing to write on a measure pass or a failure
    if ( (status != D_POP_OK) ||
         (_out == NULL) )
    {
        return status;
    }

    memcpy(_out,
           _scratch,
           size);

    return D_POP_OK;
}

/*
d_internal_pop_emit
  Pushes bytes to a sink, treating a NULL `write` as discard. Deliberately not
d_sink_emit, which reports a NULL sink as a failure: here, discarding is how a
caller skips a body it does not want.
*/
static enum d_pop_error
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
        return D_POP_OK;
    }

    // a short write aborts: a stream that cannot rewind cannot resume
    if (_sink.write(_sink.context,
                    _data,
                    _size) != _size)
    {
        return D_POP_ERROR_SINK;
    }

    return D_POP_OK;
}

//==============================================================================
// 3.  STATUS
//==============================================================================

// d_internal_pop_status_names
//   the name of each status. Searched rather than indexed, because the values
// are deliberately sparse.
static const struct
{
    enum d_pop_error status;
    const char*      name;
} d_internal_pop_status_names[] =
{
    { D_POP_OK,                     "ok"                },
    { D_POP_ERROR_INVALID_ARGUMENT, "invalid argument"  },
    { D_POP_ERROR_MALFORMED,        "malformed"         },
    { D_POP_ERROR_UNKNOWN_COMMAND,  "unknown command"   },
    { D_POP_ERROR_STATE,            "wrong state"       },
    { D_POP_ERROR_UNSUPPORTED,      "unsupported"       },
    { D_POP_ERROR_LINE_TOO_LONG,    "line too long"     },
    { D_POP_ERROR_BUFFER_TOO_SMALL, "buffer too small"  },
    { D_POP_ERROR_WOULD_BLOCK,      "would block"       },
    { D_POP_ERROR_TRANSPORT,        "transport error"   },
    { D_POP_ERROR_CLOSED,           "connection closed" },
    { D_POP_ERROR_SINK,             "sink error"        }
};

/*
d_pop_error_string
  Linear search over twelve entries; a status is named once per failure, not
per byte.
*/
const char*
d_pop_error_string(
    enum d_pop_error _error
)
{
    const size_t count = sizeof(d_internal_pop_status_names) /
                         sizeof(d_internal_pop_status_names[0]);

    // find the entry for this status
    for (size_t i = 0; i < count; ++i)
    {
        if (d_internal_pop_status_names[i].status == _error)
        {
            return d_internal_pop_status_names[i].name;
        }
    }

    return "unknown status";
}

/*
d_pop_error_is_formal
  A range test against the mechanical floor; see the header's section 2.
*/
bool
d_pop_error_is_formal(
    enum d_pop_error _error
)
{
    return ( (_error != D_POP_OK) &&
             ((int)_error < D_POP_ERROR_MECHANICAL_FLOOR) );
}

/*
d_pop_error_is_mechanical
  The complement of formal among failures.
*/
bool
d_pop_error_is_mechanical(
    enum d_pop_error _error
)
{
    return ((int)_error >= D_POP_ERROR_MECHANICAL_FLOOR);
}

/*
d_pop_error_to_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules. A value outside the enum is UNKNOWN.
*/
enum d_net_error
d_pop_error_to_net(
    enum d_pop_error _error
)
{
    switch (_error)
    {
        case D_POP_OK:
            return D_NET_OK;
        case D_POP_ERROR_INVALID_ARGUMENT:
        case D_POP_ERROR_STATE:
        case D_POP_ERROR_BUFFER_TOO_SMALL:
            return D_NET_ERROR_INVALID_ARGUMENT;
        case D_POP_ERROR_MALFORMED:
        case D_POP_ERROR_UNKNOWN_COMMAND:
            return D_NET_ERROR_PROTOCOL;
        case D_POP_ERROR_LINE_TOO_LONG:
            return D_NET_ERROR_MESSAGE_TOO_LARGE;
        case D_POP_ERROR_CLOSED:
            return D_NET_ERROR_CLOSED;
        case D_POP_ERROR_WOULD_BLOCK:
            return D_NET_ERROR_WOULD_BLOCK;
        case D_POP_ERROR_UNSUPPORTED:
        case D_POP_ERROR_TRANSPORT:
        case D_POP_ERROR_SINK:
            return D_NET_ERROR_UNKNOWN;
    }

    return D_NET_ERROR_UNKNOWN;
}

/*
d_pop_error_from_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules the other way. A value outside the
enum is TRANSPORT.
*/
enum d_pop_error
d_pop_error_from_net(
    enum d_net_error _error
)
{
    switch (_error)
    {
        case D_NET_OK:
            return D_POP_OK;
        case D_NET_ERROR_WOULD_BLOCK:
            return D_POP_ERROR_WOULD_BLOCK;
        case D_NET_ERROR_INVALID_ARGUMENT:
            return D_POP_ERROR_INVALID_ARGUMENT;
        case D_NET_ERROR_CLOSED:
        case D_NET_ERROR_CONNECTION_RESET:
        case D_NET_ERROR_CONNECTION_ABORTED:
            return D_POP_ERROR_CLOSED;
        case D_NET_ERROR_TIMED_OUT:
        case D_NET_ERROR_INTERRUPTED:
        case D_NET_ERROR_CONNECTION_REFUSED:
        case D_NET_ERROR_ADDRESS_IN_USE:
        case D_NET_ERROR_ADDRESS_INVALID:
        case D_NET_ERROR_HOST_UNREACHABLE:
        case D_NET_ERROR_NETWORK_DOWN:
        case D_NET_ERROR_ACCESS_DENIED:
        case D_NET_ERROR_MESSAGE_TOO_LARGE:
        case D_NET_ERROR_TOO_MANY_OPEN_FILES:
        case D_NET_ERROR_OUT_OF_MEMORY:
        case D_NET_ERROR_UNKNOWN:
        case D_NET_ERROR_PROTOCOL:
            return D_POP_ERROR_TRANSPORT;
    }

    return D_POP_ERROR_TRANSPORT;
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
        if (d_net_ascii_ieq_cstr(_keyword,
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
        if (d_net_ascii_ieq_cstr(_text,
                                 d_internal_pop_resp_code_names[i]))
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
        if (d_net_ascii_ieq_cstr(_keyword,
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
where `_allow_never`, optionally followed by USER. RFC 2449 lets the
AUTHORIZATION state advertise USER ("varies by user") and TRANSACTION the
actual value; both parse the same way.
*/
static enum d_pop_error
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

    *_out_per_user = d_net_ascii_ieq_cstr(tail,
                                          "USER") ? 1
                                                     : 0;

    // EXPIRE NEVER has no number
    if ( (_allow_never) &&
         (d_net_ascii_ieq_cstr(value,
                               "NEVER")) )
    {
        *_out_value = D_POP_EXPIRE_NEVER;

        return D_POP_OK;
    }

    uint64_t number = 0;

    // anything else must be a non-negative integer
    if (d_pop_parse_decimal(value,
                            INT32_MAX,
                            &number) != D_POP_OK)
    {
        return D_POP_ERROR_MALFORMED;
    }

    *_out_value = (int32_t)number;

    return D_POP_OK;
}

/*
d_pop_capabilities_parse_line
  One keyword, then parameters whose grammar depends on the keyword. The flag is
set before the parameters are examined, so a server that advertises SASL with
an over-long list is still known to offer SASL.
*/
enum d_pop_error
d_pop_capabilities_parse_line(
    struct d_pop_capabilities* _caps,
    struct d_pack_text         _line
)
{
    // parameter validation
    if ( (_caps == NULL) ||
         (!d_internal_pop_text_ok(_line)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
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
            return D_POP_OK;
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
        if (!d_net_ascii_is_alnum(_keyword.data[i]))
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
enum d_pop_error
d_pop_command_parse(
    struct d_pack_text         _line,
    struct d_pop_command_line* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_line)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    struct d_pack_text       rest    = _line;
    const struct d_pack_text keyword = d_internal_pop_next_token(&rest);

    _out->command   = D_POP_COMMAND_UNKNOWN;
    _out->keyword   = keyword;
    _out->arg_count = 0;

    // the keyword must be well-formed before anything else matters
    if (!d_internal_pop_keyword_is_valid(keyword))
    {
        return D_POP_ERROR_MALFORMED;
    }

    _out->command = d_pop_command_from_keyword(keyword);

    const struct d_pop_command_info* info = d_pop_command_info_of(
                                                _out->command);

    // an extension command: split generically, tolerate extra tokens
    if (info == NULL)
    {
        (void)d_internal_pop_split_args(rest,
                                        D_POP_ARGS_MAX,
                                        false,
                                        _out);

        return D_POP_ERROR_UNKNOWN_COMMAND;
    }

    const bool too_many = d_internal_pop_split_args(rest,
                                                    (size_t)info->max_args,
                                                    (info->rest_of_line != 0),
                                                    _out);

    // the argument count must match the command
    if ( (too_many) ||
         (_out->arg_count < (size_t)info->min_args) )
    {
        return D_POP_ERROR_MALFORMED;
    }

    return D_POP_OK;
}

/*
d_internal_pop_command_measure
  Validates a command for sending and computes its wire length. Separated from
formatting so the two-call protocol measures exactly what it later writes.
*/
static enum d_pop_error
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
                  ? d_internal_pop_text(info->keyword,
                                        strlen(info->keyword))
                  : _command->keyword;

    // the keyword, and the argument count for a known command
    if ( (!d_internal_pop_text_ok(*_out_keyword))                   ||
         (!d_internal_pop_keyword_is_valid(*_out_keyword))          ||
         (_command->arg_count > D_POP_ARGS_MAX)                     ||
         ( (info != NULL) &&
           ( (_command->arg_count < (size_t)info->min_args) ||
             (_command->arg_count > (size_t)info->max_args) ) ) )
    {
        return D_POP_ERROR_MALFORMED;
    }

    size_t size = _out_keyword->length + 2;

    // every argument is non-empty and cannot break the line
    for (size_t i = 0; i < _command->arg_count; ++i)
    {
        const struct d_pack_text arg = _command->args[i];

        if ( (arg.length == 0)                                ||
             (!d_internal_pop_text_ok(arg))                   ||
             (!d_internal_pop_text_is_clean(arg,
                                            rest_of_line)) )
        {
            return D_POP_ERROR_MALFORMED;
        }

        size += 1 + arg.length;
    }

    *_out_size = size;

    return D_POP_OK;
}

/*
d_internal_pop_command_write
  Writes the command line d_pop_command_format measured and made room for:
the keyword, each argument after its single separator, and CRLF.
*/
static void
d_internal_pop_command_write(
    const struct d_pop_command_line* _command,
    struct d_pack_text               _keyword,
    char*                            _out
)
{
    size_t pos = 0;

    d_internal_pop_put(_out,
                       &pos,
                       _keyword.data,
                       _keyword.length);

    // each argument is preceded by its single separator
    for (size_t i = 0; i < _command->arg_count; ++i)
    {
        d_internal_pop_put(_out,
                           &pos,
                           " ",
                           1);
        d_internal_pop_put(_out,
                           &pos,
                           _command->args[i].data,
                           _command->args[i].length);
    }

    d_internal_pop_put(_out,
                       &pos,
                       D_POP_TOKEN_CRLF,
                       2);

    return;
}

/*
d_pop_command_format
  Measure, check, write. The length limit is enforced on every call, measure
included, because a line the server may reject is not worth sizing a buffer
for.
*/
enum d_pop_error
d_pop_command_format(
    const struct d_pop_command_line* _command,
    char*                            _out,
    size_t                           _capacity,
    size_t*                          _out_size
)
{
    // parameter validation
    if ( (_command == NULL) ||
         (!d_internal_pop_out_args_ok(_out,
                                      _capacity,
                                      _out_size)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    struct d_pack_text keyword = { NULL, 0 };
    size_t             size    = 0;
    enum d_pop_error   status  = d_internal_pop_command_measure(_command,
                                                                &keyword,
                                                                &size);

    // an invalid command reports nothing further
    if (status != D_POP_OK)
    {
        return status;
    }

    // RFC 2449's limit applies whatever buffer the caller has
    if (size > D_POP_COMMAND_MAX)
    {
        *_out_size = size;

        return D_POP_ERROR_LINE_TOO_LONG;
    }

    status = d_internal_pop_out_check(_out,
                                      _capacity,
                                      size,
                                      _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_POP_OK) ||
         (_out == NULL) )
    {
        return status;
    }

    d_internal_pop_command_write(_command,
                                 keyword,
                                 _out);

    return D_POP_OK;
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

    const char* close = memchr(text.data,
                               ']',
                               text.length);

    // an unterminated bracket is ordinary text
    if (close == NULL)
    {
        return;
    }

    const size_t inner = (size_t)(close - text.data) - 1;

    _out->code_text = d_internal_pop_text(text.data + 1,
                                          inner);
    _out->code      = d_pop_resp_code_from_text(_out->code_text);
    _out->text      = d_internal_pop_skip_space(
                          d_internal_pop_text_after(text,
                                                    inner + 2));

    return;
}

/*
d_pop_response_parse
  "+OK" must be tested before "+" -- the continuation indicator is its prefix --
and each indicator must be followed by SP or end of line, so "+OKAY" is not
mistaken for "+OK".
*/
enum d_pop_error
d_pop_response_parse(
    struct d_pack_text     _line,
    struct d_pop_response* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_line)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    _out->indicator = D_POP_INDICATOR_NONE;
    _out->code      = D_POP_RESP_CODE_NONE;
    _out->code_text = d_internal_pop_text(_line.data,
                                          0);
    _out->text      = d_internal_pop_text(_line.data,
                                          0);

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

        if ( (d_net_ascii_istarts(_line,
                                  tokens[i].token)) &&
             ( (_line.length == length) ||
               (_line.data[length] == ' ') ) )
        {
            _out->indicator = tokens[i].indicator;
            _out->text      = d_internal_pop_skip_space(
                                  d_internal_pop_text_after(_line,
                                                            length));
            break;
        }
    }

    // a response code is only recognized on a negative reply
    if (_out->indicator == D_POP_INDICATOR_ERR)
    {
        d_internal_pop_parse_resp_code(_out);
    }

    return (_out->indicator == D_POP_INDICATOR_NONE) ? D_POP_ERROR_MALFORMED
                                                     : D_POP_OK;
}

/*
d_internal_pop_response_parts
  Resolves what a response will write: its indicator token and its code text.
Kept apart from the length arithmetic so the validation reads as one list.
*/
static enum d_pop_error
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
        return D_POP_ERROR_MALFORMED;
    }

    const char* name = d_pop_resp_code_name(_response->code);

    *_out_token = tokens[indicator];
    *_out_code  = (_response->code_text.length > 0)
                ? _response->code_text
                : d_internal_pop_text(name,
                                      (name != NULL) ? strlen(name) : 0);

    // a code belongs on -ERR only, and neither part may break the line
    if ( (!d_internal_pop_text_ok(_response->text)) ||
         (!d_internal_pop_text_ok(*_out_code))      ||
         (!d_internal_pop_text_is_clean(_response->text,
                                        true))      ||
         (!d_internal_pop_text_is_clean(*_out_code,
                                        false))     ||
         (d_internal_pop_text_has(*_out_code,
                                  ']'))             ||
         ( (_out_code->length > 0) &&
           (_response->indicator != D_POP_INDICATOR_ERR) ) )
    {
        return D_POP_ERROR_MALFORMED;
    }

    return D_POP_OK;
}

/*
d_internal_pop_response_spaced
  Whether a status line carries a SP after its indicator: a continuation
always does (RFC 5034 section 4 spells an empty challenge "+ "), other
indicators only when text follows.
*/
static bool
d_internal_pop_response_spaced(
    const struct d_pop_response* _response
)
{
    return ( (_response->text.length > 0) ||
             (_response->indicator == D_POP_INDICATOR_CONTINUE) );
}

/*
d_internal_pop_response_write
  Writes the status line d_pop_response_format measured and made room for:
the indicator, the bracketed code when there is one, the separator and the
text when either is due, and CRLF.
*/
static void
d_internal_pop_response_write(
    const struct d_pop_response* _response,
    const char*                  _token,
    struct d_pack_text           _code,
    char*                        _out
)
{
    size_t pos = 0;

    d_internal_pop_put(_out,
                       &pos,
                       _token,
                       strlen(_token));

    // the bracketed code, when there is one
    if (_code.length > 0)
    {
        d_internal_pop_put(_out,
                           &pos,
                           " [",
                           2);
        d_internal_pop_put(_out,
                           &pos,
                           _code.data,
                           _code.length);
        d_internal_pop_put(_out,
                           &pos,
                           "]",
                           1);
    }

    // the separator and text, when either is due
    if (d_internal_pop_response_spaced(_response))
    {
        d_internal_pop_put(_out,
                           &pos,
                           " ",
                           1);
        d_internal_pop_put(_out,
                           &pos,
                           _response->text.data,
                           _response->text.length);
    }

    d_internal_pop_put(_out,
                       &pos,
                       D_POP_TOKEN_CRLF,
                       2);

    return;
}

/*
d_pop_response_format
  A continuation always carries its SP (RFC 5034 section 4 spells an empty
challenge "+ "); other indicators carry one only when text or a code follows.
*/
enum d_pop_error
d_pop_response_format(
    const struct d_pop_response* _response,
    char*                        _out,
    size_t                       _capacity,
    size_t*                      _out_size
)
{
    // parameter validation
    if ( (_response == NULL) ||
         (!d_internal_pop_out_args_ok(_out,
                                      _capacity,
                                      _out_size)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    const char*        token  = NULL;
    struct d_pack_text code   = { NULL, 0 };
    enum d_pop_error   status = d_internal_pop_response_parts(_response,
                                                              &token,
                                                              &code);

    // an invalid response reports nothing further
    if (status != D_POP_OK)
    {
        return status;
    }

    const bool   space = d_internal_pop_response_spaced(_response);
    const size_t size  = strlen(token)
                       + ((code.length > 0) ? code.length + 3 : 0)
                       + (space ? _response->text.length + 1 : 0)
                       + 2;

    // RFC 2449's limit applies whatever buffer the caller has
    if (size > D_POP_RESPONSE_MAX)
    {
        *_out_size = size;

        return D_POP_ERROR_LINE_TOO_LONG;
    }

    status = d_internal_pop_out_check(_out,
                                      _capacity,
                                      size,
                                      _out_size);

    // a measure pass or a small buffer ends here
    if ( (status != D_POP_OK) ||
         (_out == NULL) )
    {
        return status;
    }

    d_internal_pop_response_write(_response,
                                  token,
                                  code,
                                  _out);

    return D_POP_OK;
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
enum d_pop_error
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
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    // an empty field is not zero
    if (_text.length == 0)
    {
        return D_POP_ERROR_MALFORMED;
    }

    uint64_t value = 0;

    // accumulate, refusing any digit that would pass the maximum
    for (size_t i = 0; i < _text.length; ++i)
    {
        const char c = _text.data[i];

        if ( (c < '0') ||
             (c > '9') )
        {
            return D_POP_ERROR_MALFORMED;
        }

        const uint64_t digit = (uint64_t)(c - '0');

        if ( (digit > _maximum) ||
             (value > (_maximum - digit) / 10u) )
        {
            return D_POP_ERROR_MALFORMED;
        }

        value = (value * 10u) + digit;
    }

    *_out = value;

    return D_POP_OK;
}

/*
d_internal_pop_parse_pair
  The first two fields of a listing line as numbers, with the first bounded by
`_first_min`. Trailing text is ignored, as the header documents.
*/
static enum d_pop_error
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
    if ( (d_pop_parse_decimal(first,
                              UINT32_MAX,
                              &a) != D_POP_OK) ||
         (a < _first_min)                      ||
         (d_pop_parse_decimal(second,
                              UINT64_MAX,
                              &b) != D_POP_OK) )
    {
        return D_POP_ERROR_MALFORMED;
    }

    *_out_first  = (uint32_t)a;
    *_out_second = b;

    return D_POP_OK;
}

/*
d_pop_maildrop_parse
  "count octets". The count may be zero; an empty maildrop is valid.
*/
enum d_pop_error
d_pop_maildrop_parse(
    struct d_pack_text     _text,
    struct d_pop_maildrop* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_text)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    return d_internal_pop_parse_pair(_text,
                                     0,
                                     &_out->count,
                                     &_out->octets);
}

/*
d_pop_scan_listing_parse
  "number octets". Message numbers start at 1 (RFC 1939 section 5).
*/
enum d_pop_error
d_pop_scan_listing_parse(
    struct d_pack_text         _text,
    struct d_pop_scan_listing* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_text)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    return d_internal_pop_parse_pair(_text,
                                     1,
                                     &_out->number,
                                     &_out->octets);
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
enum d_pop_error
d_pop_uid_listing_parse(
    struct d_pack_text        _text,
    struct d_pop_uid_listing* _out
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_text_ok(_text)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    struct d_pack_text       rest   = _text;
    const struct d_pack_text number = d_internal_pop_next_token(&rest);
    const struct d_pack_text uid    = d_internal_pop_next_token(&rest);
    uint64_t                 value  = 0;

    // a positive message number and a well-formed uid
    if ( (d_pop_parse_decimal(number,
                              UINT32_MAX,
                              &value) != D_POP_OK) ||
         (value == 0)                              ||
         (!d_pop_uid_is_valid(uid)) )
    {
        return D_POP_ERROR_MALFORMED;
    }

    _out->number = (uint32_t)value;

    return d_internal_pop_copy_text(_out->uid,
                                    sizeof(_out->uid),
                                    uid);
}

/*
d_pop_maildrop_format
  snprintf into scratch; see d_internal_pop_emit_scratch for why.
*/
enum d_pop_error
d_pop_maildrop_format(
    const struct d_pop_maildrop* _maildrop,
    char*                        _out,
    size_t                       _capacity,
    size_t*                      _out_size
)
{
    // parameter validation
    if ( (_maildrop == NULL) ||
         (!d_internal_pop_out_args_ok(_out,
                                      _capacity,
                                      _out_size)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    char      scratch[48];  // filled before it is read
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
enum d_pop_error
d_pop_scan_listing_format(
    const struct d_pop_scan_listing* _listing,
    char*                            _out,
    size_t                           _capacity,
    size_t*                          _out_size
)
{
    // parameter validation
    if ( (_listing == NULL) ||
         (!d_internal_pop_out_args_ok(_out,
                                      _capacity,
                                      _out_size)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    // message numbers start at 1
    if (_listing->number == 0)
    {
        return D_POP_ERROR_MALFORMED;
    }

    char      scratch[48];  // filled before it is read
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
enum d_pop_error
d_pop_uid_listing_format(
    const struct d_pop_uid_listing* _listing,
    char*                           _out,
    size_t                          _capacity,
    size_t*                         _out_size
)
{
    // parameter validation
    if ( (_listing == NULL) ||
         (!d_internal_pop_out_args_ok(_out,
                                      _capacity,
                                      _out_size)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    const char* terminator = memchr(_listing->uid,
                                    '\0',
                                    sizeof(_listing->uid));

    // a positive number and a well-formed, terminated uid
    if ( (_listing->number == 0)                                      ||
         (terminator == NULL)                                         ||
         (!d_pop_uid_is_valid(
              d_internal_pop_text(_listing->uid,
                                  (size_t)(terminator - _listing->uid)))) )
    {
        return D_POP_ERROR_MALFORMED;
    }

    char      scratch[D_POP_UID_MAX + 16];  // filled before it is read
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
// 8.  CONNECTIONS
//==============================================================================

/*
d_pop_send_command
  The buffer is exactly D_POP_COMMAND_MAX, which the formatter never exceeds.
The command is checked before the connection, which d_net_write_all refuses
when it is NULL.
*/
enum d_pop_error
d_pop_send_command(
    struct d_net_connection*         _connection,
    const struct d_pop_command_line* _command
)
{
    char                   line[D_POP_COMMAND_MAX] = { 0 };
    size_t                 size                    = 0;
    const enum d_pop_error status                  =
        d_pop_command_format(_command,
                             line,
                             sizeof(line),
                             &size);

    // a command that cannot be formatted is not sent
    if (status != D_POP_OK)
    {
        return status;
    }

    return d_pop_error_from_net(d_net_write_all(_connection,
                                                line,
                                                size,
                                                NULL));
}

/*
d_pop_send_response
  The buffer is exactly D_POP_RESPONSE_MAX, which the formatter never exceeds.
The response is checked before the connection, as for a command.
*/
enum d_pop_error
d_pop_send_response(
    struct d_net_connection*     _connection,
    const struct d_pop_response* _response
)
{
    char                   line[D_POP_RESPONSE_MAX] = { 0 };
    size_t                 size                     = 0;
    const enum d_pop_error status                   =
        d_pop_response_format(_response,
                              line,
                              sizeof(line),
                              &size);

    // a response that cannot be formatted is not sent
    if (status != D_POP_OK)
    {
        return status;
    }

    return d_pop_error_from_net(d_net_write_all(_connection,
                                                line,
                                                size,
                                                NULL));
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
static enum d_pop_error
d_internal_pop_decode_flush(
    struct d_pop_body_decoder* _decoder,
    struct d_pack_sink         _sink,
    const char*                _run,
    size_t                     _length
)
{
    const enum d_pop_error status = d_internal_pop_emit(_sink,
                                                        _run,
                                                        _length);

    // only accepted bytes count
    if (status == D_POP_OK)
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
enum d_pop_error
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
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    const char*      bytes  = _data;
    size_t           run    = 0;
    size_t           i      = 0;
    enum d_pop_error status = D_POP_OK;

    // walk the chunk until it ends or the body does
    while ( (i < _size)                                         &&
            (_decoder->state != D_INTERNAL_POP_DECODE_DONE)     &&
            (status == D_POP_OK) )
    {
        const enum d_internal_pop_decode_action action =
            d_internal_pop_decode_step(_decoder,
                                       bytes[i]);

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
            status = d_internal_pop_decode_flush(_decoder,
                                                 _sink,
                                                 "\r",
                                                 1);
        }
    }

    // flush whatever content remains in the run
    if (status == D_POP_OK)
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
static enum d_pop_error
d_internal_pop_encode_emit(
    struct d_pop_body_encoder* _encoder,
    struct d_pack_sink         _sink,
    const char*                _data,
    size_t                     _length
)
{
    const enum d_pop_error status = d_internal_pop_emit(_sink,
                                                        _data,
                                                        _length);

    // only accepted bytes count
    if (status == D_POP_OK)
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
enum d_pop_error
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
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    const char*      bytes  = _data;
    size_t           run    = 0;
    enum d_pop_error status = D_POP_OK;

    // walk the chunk, ending the run wherever an edit is due
    for (size_t i = 0; (i < _size) && (status == D_POP_OK); ++i)
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
            status = (status == D_POP_OK)
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
    if (status == D_POP_OK)
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
enum d_pop_error
d_pop_body_encode_finish(
    struct d_pop_body_encoder* _encoder,
    struct d_pack_sink         _sink
)
{
    // parameter validation
    if ( (_encoder == NULL) ||
         (_encoder->finished != 0) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    enum d_pop_error status = D_POP_OK;

    // close an unterminated last line
    if (_encoder->at_line_start == 0)
    {
        status = (_encoder->after_cr != 0)
               ? d_internal_pop_encode_emit(_encoder,
                                            _sink,
                                            "\n",
                                            1)
               : d_internal_pop_encode_emit(_encoder,
                                            _sink,
                                            "\r\n",
                                            2);
    }

    // the terminator itself is framing, not content, so it is not counted
    if (status == D_POP_OK)
    {
        status = d_internal_pop_emit(_sink,
                                     D_POP_TOKEN_TERMINATOR,
                                     3);
    }

    _encoder->finished = 1;

    return status;
}

//==============================================================================
// 10.  READER
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

    d_net_line_buffer_init(&_reader->lines,
                           sizeof(_reader->buffer));
    _reader->strict     = D_INTERNAL_POP_STRICT;
    _reader->discarding = 0;
    _reader->net_error  = D_NET_OK;

    return;
}

/*
d_pop_reader_buffered
  Bytes read from the connection and not yet consumed. Non-zero after a body
means the peer pipelined; a derived module about to hand the stream to TLS
after STLS must see zero here, or plaintext would be misread as a handshake.
*/
size_t
d_pop_reader_buffered(
    const struct d_pop_reader* _reader
)
{
    return (_reader != NULL) ? d_net_line_buffer_pending(&_reader->lines)
                             : 0;
}

/*
d_internal_pop_reader_fill
  Pulls once from the connection into the buffer's room, keeping what the
read reported. Bytes that arrive are kept whatever came with them, and the
read succeeds on them; a lasting failure is the next read's to report. With
nothing read, the end of the stream is CLOSED, a line being due, and a
failure is the connection's error in this module's terms.
*/
static enum d_pop_error
d_internal_pop_reader_fill(
    struct d_pop_reader*     _reader,
    struct d_net_connection* _connection
)
{
    size_t                       room   = 0;
    char* const                  into   =
        d_net_line_buffer_room(&_reader->lines,
                               _reader->buffer,
                               &room);
    const struct d_net_io_result result =
        d_net_connection_read(_connection,
                              into,
                              room);

    d_net_line_buffer_commit(&_reader->lines,
                             result.count);
    _reader->net_error = result.error;

    // bytes arrived, so there is work to do whatever came with them
    if (result.count > 0)
    {
        return D_POP_OK;
    }

    return (result.error == D_NET_OK) ? D_POP_ERROR_CLOSED
                                      : d_pop_error_from_net(result.error);
}

/*
d_internal_pop_reader_drop
  Drops what is held of a line too long for the buffer, as it arrives. The
first drop of a line reports the overflow; the rest of the line goes
quietly, until its LF.
*/
static bool
d_internal_pop_reader_drop(
    struct d_pop_reader* _reader
)
{
    const bool first = (_reader->discarding == 0);

    d_net_line_buffer_clear(&_reader->lines);
    _reader->discarding = 1;

    return first;
}

/*
d_pop_reader_read_line
  The line buffer remembers how far it has searched, so bytes are examined
once however many reads a line takes to arrive. The line that ends an
over-long one is consumed and dropped, and reading resumes after it.
*/
enum d_pop_error
d_pop_reader_read_line(
    struct d_pop_reader*     _reader,
    struct d_net_connection* _connection,
    struct d_pack_text*      _out_line
)
{
    // parameter validation
    if ( (_reader == NULL)     ||
         (_connection == NULL) ||
         (_out_line == NULL) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    // search what is buffered, and pull more until a line appears
    for (;;)
    {
        struct d_net_line           line  = { { NULL, 0 }, 0, false };
        const enum d_net_line_found found =
            d_net_line_buffer_find(&_reader->lines,
                                   _reader->buffer,
                                   &line);

        // a whole line: the tail of an over-long one is dropped
        if (found == D_NET_LINE_FOUND)
        {
            const bool tail = (_reader->discarding != 0);

            d_net_line_buffer_consume(&_reader->lines,
                                      line.size);
            _reader->discarding = 0;

            // a bare LF is refused in strict mode, the line consumed
            if (!tail)
            {
                *_out_line = line.text;

                return ( (!line.crlf) &&
                         (_reader->strict != 0) ) ? D_POP_ERROR_MALFORMED
                                                  : D_POP_OK;
            }

            continue;
        }

        // a line too long for the buffer goes as it arrives
        if ( ( (found == D_NET_LINE_FULL) ||
               (_reader->discarding != 0) )      &&
             (d_internal_pop_reader_drop(_reader)) )
        {
            return D_POP_ERROR_LINE_TOO_LONG;
        }

        const enum d_pop_error status = d_internal_pop_reader_fill(_reader,
                                                                   _connection);

        if (status != D_POP_OK)
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
enum d_pop_error
d_pop_reader_read_body(
    struct d_pop_reader*       _reader,
    struct d_net_connection*   _connection,
    struct d_pop_body_decoder* _decoder,
    struct d_pack_sink         _sink
)
{
    // parameter validation
    if ( (_reader == NULL)     ||
         (_connection == NULL) ||
         (_decoder == NULL) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    // decode what is buffered, and pull more until the body ends
    while (!d_pop_body_decoder_is_done(_decoder))
    {
        const struct d_pack_text held =
            d_net_line_buffer_held(&_reader->lines,
                                   _reader->buffer);

        if (held.length > 0)
        {
            size_t                 consumed = 0;
            const enum d_pop_error status   =
                d_pop_body_decode(_decoder,
                                  held.data,
                                  held.length,
                                  _sink,
                                  &consumed);

            d_net_line_buffer_consume(&_reader->lines,
                                      consumed);

            if (status != D_POP_OK)
            {
                return status;
            }

            continue;
        }

        const enum d_pop_error status = d_internal_pop_reader_fill(_reader,
                                                                   _connection);

        if (status != D_POP_OK)
        {
            return status;
        }
    }

    return D_POP_OK;
}

//==============================================================================
// 11.  APOP
//==============================================================================

/*
d_pop_apop_timestamp
  The first "<" and the first ">" after it, with an "@" between and no SP: the
msg-id shape RFC 1939 section 7 requires. Available with APOP compiled out,
since recognizing that a server offers APOP needs no digest.
*/
enum d_pop_error
d_pop_apop_timestamp(
    struct d_pack_text  _greeting,
    struct d_pack_text* _out_timestamp
)
{
    // parameter validation
    if ( (_out_timestamp == NULL) ||
         (!d_internal_pop_text_ok(_greeting)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    // an empty greeting has no timestamp, and must not reach memchr
    if (_greeting.length == 0)
    {
        return D_POP_ERROR_UNSUPPORTED;
    }

    const char* open = memchr(_greeting.data,
                              '<',
                              _greeting.length);

    // no opening bracket, no timestamp
    if (open == NULL)
    {
        return D_POP_ERROR_UNSUPPORTED;
    }

    const size_t offset = (size_t)(open - _greeting.data);
    const char*  close  = memchr(open,
                                 '>',
                                 _greeting.length - offset);

    // an unterminated bracket is not a timestamp
    if (close == NULL)
    {
        return D_POP_ERROR_UNSUPPORTED;
    }

    const struct d_pack_text stamp =
        d_internal_pop_text(open,
                            (size_t)(close - open) + 1);

    // the msg-id shape: an @, and nothing that would split a token
    if ( (memchr(stamp.data,
                 '@',
                 stamp.length) == NULL) ||
         (!d_internal_pop_text_is_clean(stamp,
                                        false)) )
    {
        return D_POP_ERROR_UNSUPPORTED;
    }

    *_out_timestamp = stamp;

    return D_POP_OK;
}

#if D_INTERNAL_POP_APOP

D_STATIC_ASSERT(D_POP_APOP_DIGEST_LENGTH == (2 * D_NET_DIGEST_MD5_SIZE),
                "pop: the APOP digest is an MD5 digest in hex");

/*
d_internal_pop_apop_args_ok
  What the digest needs: a timestamp, which cannot be empty, and a secret,
which can, each a usable span.
*/
static bool
d_internal_pop_apop_args_ok(
    struct d_pack_text _timestamp,
    struct d_pack_text _secret
)
{
    return ( (_timestamp.length > 0)              &&
             (d_internal_pop_text_ok(_timestamp)) &&
             (d_internal_pop_text_ok(_secret)) );
}

/*
d_internal_pop_apop_md5
  MD5 over the timestamp immediately followed by the secret, with no
separator (RFC 1939 section 7). The digest refuses only missing data, which
the callers' validation has already refused.
*/
static bool
d_internal_pop_apop_md5(
    struct d_pack_text _timestamp,
    struct d_pack_text _secret,
    uint8_t            _md5[D_NET_DIGEST_MD5_SIZE]
)
{
    struct d_net_digest md5 = { 0 };

    d_net_digest_init(&md5,
                      D_NET_DIGEST_MD5);
    d_net_digest_update(&md5,
                        _timestamp.data,
                        _timestamp.length);
    d_net_digest_update(&md5,
                        _secret.data,
                        _secret.length);

    return d_net_digest_final(&md5,
                              _md5);
}

/*
d_pop_apop_digest
  The MD5, rendered as lowercase hex.
*/
enum d_pop_error
d_pop_apop_digest(
    struct d_pack_text _timestamp,
    struct d_pack_text _secret,
    char               _out[D_POP_APOP_DIGEST_LENGTH + 1]
)
{
    // parameter validation
    if ( (_out == NULL) ||
         (!d_internal_pop_apop_args_ok(_timestamp,
                                       _secret)) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t                   digest[D_NET_DIGEST_MD5_SIZE] = { 0 };
    const struct d_pack_bytes octets = { digest,
                                         sizeof(digest) };

    // the digest, then its digits, neither of which can now fail
    if ( (!d_internal_pop_apop_md5(_timestamp,
                                   _secret,
                                   digest))                      ||
         (d_net_hex_encode(octets,
                           D_NET_HEX_LOWER,
                           _out,
                           D_POP_APOP_DIGEST_LENGTH + 1,
                           NULL) != D_NET_CODEC_OK) )
    {
        return D_POP_ERROR_INVALID_ARGUMENT;
    }

    return D_POP_OK;
}

/*
d_pop_apop_verify
  The given digest's digits are decoded, of either case, and its octets
compared with the recomputed ones in constant time, so the time taken does not
reveal how much of a guess was right. Digits that are not hex fail to decode,
and so fail.
*/
bool
d_pop_apop_verify(
    struct d_pack_text _timestamp,
    struct d_pack_text _secret,
    struct d_pack_text _digest
)
{
    uint8_t expected[D_NET_DIGEST_MD5_SIZE] = { 0 };
    uint8_t given[D_NET_DIGEST_MD5_SIZE]    = { 0 };
    size_t  size                            = 0;

    // a digest of the wrong length, or one we cannot compute, fails
    if ( (_digest.length != D_POP_APOP_DIGEST_LENGTH)          ||
         (!d_internal_pop_text_ok(_digest))                    ||
         (!d_internal_pop_apop_args_ok(_timestamp,
                                       _secret)) )
    {
        return false;
    }

    return ( (d_internal_pop_apop_md5(_timestamp,
                                      _secret,
                                      expected))                  &&
             (d_net_hex_decode(_digest,
                               given,
                               sizeof(given),
                               &size) == D_NET_CODEC_OK)          &&
             (d_net_digest_equal(expected,
                                 given,
                                 sizeof(given))) );
}

#else

/*
d_pop_apop_digest
  APOP is compiled out (D_CFG_POP_APOP is 0).
*/
enum d_pop_error
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

    return D_POP_ERROR_UNSUPPORTED;
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
