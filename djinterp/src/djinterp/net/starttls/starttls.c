/*******************************************************************************
* djinterp [net]                                                      starttls.c
*
*   Definitions for starttls.h: the STARTTLS session -- its buffered plaintext
* and TLS I/O over a net.h connection, the policy-driven negotiation, and the
* upgrade with its check for leftover plaintext, which hands the connection
* to a session of the SSL kernel.
*
*
* path:      /src/djinterp/net/starttls/starttls.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "../../../../inc/djinterp/net/starttls/starttls.h"  // corresponding header
// std
#include <stdarg.h>   // va_list, va_start, va_copy, va_end
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // vsnprintf
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, memset, strlen
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../inc/djinterp/config/net/starttls/cfg_starttls.h"  // knobs
#include "../../../../inc/djinterp/net/codec/codec_line.h"  // d_net_line_*
#include "../../../../inc/djinterp/net/net_common.h"        // connections
#include "../../../../inc/djinterp/net/ssl/ssl.h"           // TLS sessions
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t


// the policy enumeration mirrors its configuration sentinels, which is what
// lets D_CFG_STARTTLS_POLICY seed d_starttls_options_default directly
D_STATIC_ASSERT(D_STARTTLS_POLICY_DISABLED ==
                    D_CFG_STARTTLS_POLICY_DISABLED,
                "d_starttls_policy is out of step with cfg_starttls.h");
D_STATIC_ASSERT(D_STARTTLS_POLICY_OPPORTUNISTIC ==
                    D_CFG_STARTTLS_POLICY_OPPORTUNISTIC,
                "d_starttls_policy is out of step with cfg_starttls.h");
D_STATIC_ASSERT(D_STARTTLS_POLICY_REQUIRED ==
                    D_CFG_STARTTLS_POLICY_REQUIRED,
                "d_starttls_policy is out of step with cfg_starttls.h");

// cfg_starttls.h bounds the default buffer with its own copy of the minimum
D_STATIC_ASSERT(D_INTERNAL_STARTTLS_BUFFER_BYTES >= D_STARTTLS_BUFFER_MIN,
                "D_CFG_STARTTLS_BUFFER_BYTES is below D_STARTTLS_BUFFER_MIN");

// D_INTERNAL_STARTTLS_LINE_STACK
//   constant: the stack scratch for composing one outgoing line, in bytes:
// SMTP's command-line limit, CRLF included, so a typical command never
// touches the heap. A longer line borrows it for the length of the call.
#define D_INTERNAL_STARTTLS_LINE_STACK 512

// session internals
/*
d_starttls_internal_fail
  File-local, so its contract lives here: moves the session to
D_STARTTLS_STATE_FAILED and returns `_status`, so each fatal path is a single
return at its call site. Errors that leave the stream where it was --
INVALID_ARGUMENT, STATE, an orderly CLOSED, a refused send -- never come
through here.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_fail(
    struct d_starttls_session* _session,
    enum d_starttls_error      _status
)
{
    _session->state = D_STARTTLS_STATE_FAILED;

    return _status;
}

/*
d_starttls_internal_require_open
  File-local: D_STARTTLS_OK if the session can carry I/O -- it is plaintext
or secure -- and D_STARTTLS_ERROR_STATE otherwise.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_require_open(
    const struct d_starttls_session* _session
)
{
    if ( (_session->state == D_STARTTLS_STATE_PLAINTEXT) ||
         (_session->state == D_STARTTLS_STATE_SECURE) )
    {
        return D_STARTTLS_OK;
    }

    return D_STARTTLS_ERROR_STATE;
}

/*
d_starttls_internal_decline
  File-local: the outcome when TLS will not happen in this session: `_status`,
fatally, under REQUIRED, and D_STARTTLS_OK -- staying in plaintext -- under
OPPORTUNISTIC.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_decline(
    struct d_starttls_session* _session,
    enum d_starttls_error      _status
)
{
    if (_session->policy == D_STARTTLS_POLICY_REQUIRED)
    {
        return d_starttls_internal_fail(_session,
                                        _status);
    }

    return D_STARTTLS_OK;
}

/*
d_starttls_internal_from_connection
  File-local: what the plaintext connection reported, in this module's
terms, with the connection's own error kept as the cause.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_from_connection(
    struct d_starttls_session* _session,
    enum d_net_error           _error
)
{
    _session->net_error = _error;

    return d_starttls_error_from_net(_error);
}

/*
d_starttls_internal_tls_error
  File-local: an SSL kernel status in this module's terms. During the
handshake, whatever ends it but the peer's certificate is a handshake
failure, a peer that hangs up included; after it, a connection that ends
without close_notify is a truncation, and a record that breaks the protocol
is PROTOCOL. A connection that fails beneath TLS is TRANSPORT, or TIMED_OUT
when its deadline expired; and one that would block is TRANSPORT too, since
every call here blocks. Engine failures are the handshake's, or TRANSPORT.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_tls_error(
    enum d_ssl_error _status,
    bool             _handshake,
    enum d_net_error _cause
)
{
    switch (_status)
    {
        case D_SSL_OK:
            return D_STARTTLS_OK;
        case D_SSL_ERROR_CLOSED:
            return D_STARTTLS_ERROR_CLOSED;
        case D_SSL_ERROR_VERIFY:
            return D_STARTTLS_ERROR_VERIFY;
        case D_SSL_ERROR_TRUNCATED:
            return (_handshake) ? D_STARTTLS_ERROR_HANDSHAKE
                                : D_STARTTLS_ERROR_TRUNCATED;
        case D_SSL_ERROR_PROTOCOL:
            return (_handshake) ? D_STARTTLS_ERROR_HANDSHAKE
                                : D_STARTTLS_ERROR_PROTOCOL;
        case D_SSL_ERROR_NOT_TLS:
        case D_SSL_ERROR_HANDSHAKE:
            return D_STARTTLS_ERROR_HANDSHAKE;
        case D_SSL_ERROR_TRANSPORT:
        case D_SSL_ERROR_WOULD_BLOCK:
            return (_cause == D_NET_ERROR_TIMED_OUT)
                       ? D_STARTTLS_ERROR_TIMED_OUT
                       : D_STARTTLS_ERROR_TRANSPORT;
        case D_SSL_ERROR_OUT_OF_MEMORY:
            return D_STARTTLS_ERROR_OUT_OF_MEMORY;
        case D_SSL_ERROR_INVALID_ARGUMENT:
            return D_STARTTLS_ERROR_INVALID_ARGUMENT;
        case D_SSL_ERROR_STATE:
            return D_STARTTLS_ERROR_STATE;
        case D_SSL_ERROR_MALFORMED:
        case D_SSL_ERROR_NOT_FOUND:
        case D_SSL_ERROR_UNSUPPORTED:
        case D_SSL_ERROR_CONFIG:
            return D_STARTTLS_ERROR_CONFIG;
        case D_SSL_ERROR_BUFFER_TOO_SMALL:
        case D_SSL_ERROR_WANT_READ:
        case D_SSL_ERROR_BACKEND:
            break;
    }

    return (_handshake) ? D_STARTTLS_ERROR_HANDSHAKE
                        : D_STARTTLS_ERROR_TRANSPORT;
}

/*
d_starttls_internal_tls_detail
  File-local: a static description of why TLS failed, from the kernel's
record of the session: the first reason the peer's certificate failed, the
alert either end sent, or the kernel's name for the failure.
*/
D_STATIC const char*
d_starttls_internal_tls_detail(
    const struct d_ssl_session* _ssl,
    enum d_ssl_error            _status
)
{
    struct d_ssl_info info   = { .version = D_SSL_VERSION_NONE };
    const uint32_t    benign = (uint32_t)D_SSL_VERIFY_FLAG_NOT_PERFORMED;

    // without the kernel's record, its name for the failure is all there is
    if (!d_ssl_session_info(_ssl,
                            &info))
    {
        return d_ssl_error_name(_status);
    }

    const uint32_t failed = info.verify & ~benign;

    // the lowest reason the certificate failed, by itself
    if ( (_status == D_SSL_ERROR_VERIFY) &&
         (failed != 0u) )
    {
        return d_ssl_verify_flag_name(failed & (~failed + 1u));
    }

    // what either end said in its alert
    if (info.alert_received != D_SSL_ALERT_NONE)
    {
        return d_ssl_alert_name(info.alert_received);
    }

    if (info.alert_sent != D_SSL_ALERT_NONE)
    {
        return d_ssl_alert_name(info.alert_sent);
    }

    return d_ssl_error_name(_status);
}

/*
d_starttls_internal_tls_outcome
  File-local: a TLS call's status in this module's terms. A failure keeps
the connection's error as its cause, when the connection had one, and its
description as the session's detail.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_tls_outcome(
    struct d_starttls_session* _session,
    enum d_ssl_error           _status,
    bool                       _handshake
)
{
    const enum d_net_error cause =
        d_ssl_session_transport_error(_session->ssl);

    // success, and the peer's close_notify, describe nothing
    if ( (_status == D_SSL_OK) ||
         (_status == D_SSL_ERROR_CLOSED) )
    {
        return d_starttls_internal_tls_error(_status,
                                             _handshake,
                                             cause);
    }

    // the connection's own error is the cause beneath TLS
    if (cause != D_NET_OK)
    {
        _session->net_error = cause;
    }

    _session->detail = d_starttls_internal_tls_detail(_session->ssl,
                                                      _status);

    return d_starttls_internal_tls_error(_status,
                                         _handshake,
                                         cause);
}

/*
d_starttls_internal_read_plain
  File-local: one read from the plaintext connection. Bytes it delivers
count whatever it reported with them; with none, the end of its stream is
CLOSED, and a failure is its error in this module's terms.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_read_plain(
    struct d_starttls_session* _session,
    void*                      _buffer,
    size_t                     _capacity,
    size_t*                    _received
)
{
    const struct d_net_io_result result =
        d_net_connection_read(_session->connection,
                              _buffer,
                              _capacity);

    *_received = result.count;

    // bytes arrived, so the read succeeded
    if (result.count > 0)
    {
        return D_STARTTLS_OK;
    }

    // nothing, and no error: the end of the stream
    if (result.error == D_NET_OK)
    {
        return D_STARTTLS_ERROR_CLOSED;
    }

    return d_starttls_internal_from_connection(_session,
                                               result.error);
}

/*
d_starttls_internal_read
  File-local: one read straight into `_buffer`, from the connection before
the upgrade and through TLS after it. The end of the stream -- or the peer's
close_notify -- is returned as an orderly CLOSED, for the caller to judge;
any other failure is fatal.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_read(
    struct d_starttls_session* _session,
    void*                      _buffer,
    size_t                     _capacity,
    size_t*                    _received
)
{
    enum d_starttls_error status = D_STARTTLS_OK;

    *_received = 0;

    // after the upgrade every byte comes through TLS
    if (_session->state == D_STARTTLS_STATE_SECURE)
    {
        status = d_starttls_internal_tls_outcome(
                     _session,
                     d_ssl_session_read(_session->ssl,
                                        _buffer,
                                        _capacity,
                                        _received),
                     false);
    }
    else
    {
        status = d_starttls_internal_read_plain(_session,
                                                _buffer,
                                                _capacity,
                                                _received);
    }

    // the end of the stream is not yet a failure
    if ( (status == D_STARTTLS_OK) ||
         (status == D_STARTTLS_ERROR_CLOSED) )
    {
        return status;
    }

    return d_starttls_internal_fail(_session,
                                    status);
}

/*
d_starttls_internal_write_all
  File-local: writes every byte, through TLS once the session is secure. Any
failure is fatal: once part of a message has gone out, the peer's view of
the stream no longer matches this side's.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_write_all(
    struct d_starttls_session* _session,
    const void*                _data,
    size_t                     _size
)
{
    enum d_starttls_error status = D_STARTTLS_OK;

    // after the upgrade every byte goes through TLS
    if (_session->state == D_STARTTLS_STATE_SECURE)
    {
        status = d_starttls_internal_tls_outcome(
                     _session,
                     d_ssl_session_write_all(_session->ssl,
                                             _data,
                                             _size,
                                             NULL),
                     false);
    }
    else
    {
        const enum d_net_error written =
            d_net_write_all(_session->connection,
                            _data,
                            _size,
                            NULL);

        status = (written == D_NET_OK)
                     ? D_STARTTLS_OK
                     : d_starttls_internal_from_connection(_session,
                                                           written);
    }

    if (status != D_STARTTLS_OK)
    {
        return d_starttls_internal_fail(_session,
                                        status);
    }

    return D_STARTTLS_OK;
}

/*
d_starttls_internal_fill
  File-local: reads once into the free room of the receive buffer, which
moves the unread bytes to the front when they have reached its end. A
buffer already full of unread bytes is D_STARTTLS_ERROR_LINE_TOO_LONG,
returned without reading and without failing the session: whether that is
fatal is the caller's decision.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_fill(
    struct d_starttls_session* _session
)
{
    size_t      room = 0;
    char* const into = d_net_line_buffer_room(&_session->lines,
                                              _session->buffer,
                                              &room);

    // a full buffer has no room left for a line end to arrive in
    if (room == 0)
    {
        return D_STARTTLS_ERROR_LINE_TOO_LONG;
    }

    size_t                      received = 0;
    const enum d_starttls_error status   =
        d_starttls_internal_read(_session,
                                 into,
                                 room,
                                 &received);

    if (status == D_STARTTLS_OK)
    {
        d_net_line_buffer_commit(&_session->lines,
                                 received);
    }

    return status;
}

/*
d_starttls_internal_take_line
  File-local: hands the caller the line the line buffer found. A CR just
before the LF belongs to the terminator; without one the LF stands alone,
which strict_crlf rejects. A CR or NUL left inside the line is malformed in
every line-oriented STARTTLS protocol, and is where two parsers come to
disagree about where a line ends, so it is rejected whatever the setting.
The terminator's first byte becomes the NUL, so the line needs no copy. A
line refused is not consumed.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_take_line(
    struct d_starttls_session* _session,
    const struct d_net_line*   _line,
    const char**               _out,
    size_t*                    _length
)
{
    // a bare LF, where CR LF is required
    if ( (!_line->crlf) &&
         (_session->strict_crlf) )
    {
        return d_starttls_internal_fail(_session,
                                        D_STARTTLS_ERROR_PROTOCOL);
    }

    // no CR or NUL may remain inside the line
    if (!d_net_line_is_clean(_line->text))
    {
        return d_starttls_internal_fail(_session,
                                        D_STARTTLS_ERROR_PROTOCOL);
    }

    const size_t at = (size_t)(_line->text.data - _session->buffer);

    _session->buffer[at + _line->text.length] = '\0';
    *_out    = _line->text.data;
    *_length = _line->text.length;

    d_net_line_buffer_consume(&_session->lines,
                              _line->size);

    return D_STARTTLS_OK;
}

/*
d_starttls_internal_send_text_line
  File-local: sends `_text` and a CRLF as one write, so the line is never
split across TLS records. It refuses, sending nothing, a line that holds CR,
LF, or NUL, and one longer than the session's own line limit -- the limit
the peer's lines are held to. A line that fits the stack scratch is
assembled there; a longer one borrows the heap for the length of the call.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_send_text_line(
    struct d_starttls_session* _session,
    const char*                _text,
    size_t                     _length
)
{
    const struct d_pack_text text = { _text,
                                      _length };

    // a line break inside a command line starts a second command
    if (!d_net_line_is_clean(text))
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    // the line and its CRLF must fit the session's line limit
    if (_length > (_session->lines.capacity - 2))
    {
        return D_STARTTLS_ERROR_LINE_TOO_LONG;
    }

    // filled before it is read, so left uninitialized (#111)
    char         stack[D_INTERNAL_STARTTLS_LINE_STACK];
    const size_t total = _length + 2;
    char*        line  = (total <= sizeof(stack)) ? stack : malloc(total);

    if (!line)
    {
        return D_STARTTLS_ERROR_OUT_OF_MEMORY;
    }

    memcpy(line,
           _text,
           _length);
    line[_length]     = '\r';
    line[_length + 1] = '\n';

    const enum d_starttls_error status =
        d_starttls_internal_write_all(_session,
                                      line,
                                      total);

    // release the heap copy, if one was needed
    if (line != stack)
    {
        free(line);
    }

    return status;
}

/*
d_starttls_internal_send_long_line
  File-local: sends a formatted line too long for d_starttls_send_linef's
stack scratch, `_measured` characters as that first attempt counted them. It
is held to the line limit before any memory is spent on it, then formatted
again from `_arguments` into memory of exactly its size.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_send_long_line(
    struct d_starttls_session* _session,
    size_t                     _measured,
    const char*                _format,
    va_list                    _arguments
)
{
    // the line and its CRLF must fit the session's line limit
    if (_measured > (_session->lines.capacity - 2))
    {
        return D_STARTTLS_ERROR_LINE_TOO_LONG;
    }

    const size_t size = _measured + 1;
    char*        heap = malloc(size);

    if (!heap)
    {
        return D_STARTTLS_ERROR_OUT_OF_MEMORY;
    }

    (void)vsnprintf(heap,
                    size,
                    _format,
                    _arguments);

    const enum d_starttls_error status =
        d_starttls_internal_send_text_line(_session,
                                           heap,
                                           _measured);

    free(heap);

    return status;
}

/*
d_starttls_internal_copy_string
  File-local: a heap copy of `_text`, or NULL for a NULL `_text` or a failed
allocation.
*/
D_STATIC char*
d_starttls_internal_copy_string(
    const char* _text
)
{
    if (!_text)
    {
        return NULL;
    }

    const size_t size = strlen(_text) + 1;
    char*        copy = malloc(size);

    if (copy)
    {
        memcpy(copy,
               _text,
               size);
    }

    return copy;
}

/*
d_starttls_internal_check_context
  File-local: validates the context of a session that may upgrade. It must be
initialized, made for the session's own role, and -- for a client whose
context checks host names -- come with a name to check against. That name
comes from the caller, because the peer can never be trusted to supply it.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_check_context(
    const struct d_starttls_options* _options
)
{
    const struct d_ssl_context* const tls  = _options->tls;
    const enum d_ssl_role             role =
        (_options->role == D_STARTTLS_ROLE_SERVER) ? D_SSL_ROLE_SERVER
                                                   : D_SSL_ROLE_CLIENT;

    // an empty context has no engine to upgrade with
    if ( (!tls) ||
         (!tls->engine) )
    {
        return D_STARTTLS_ERROR_NO_BACKEND;
    }

    if (tls->plan.role != role)
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    // a client checking names must be told whom to expect
    if ( (_options->role == D_STARTTLS_ROLE_CLIENT) &&
         (tls->plan.verify_hostname)                &&
         ( (!_options->server_name) ||
           (_options->server_name[0] == '\0') ) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    return D_STARTTLS_OK;
}

/*
d_starttls_internal_check_options
  File-local: validates what d_starttls_init is given, before anything is
allocated. The policy's valid range starts at 1, so options that were only
zero-initialized fail here rather than being read as DISABLED.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_check_options(
    const struct d_starttls_options* _options,
    const struct d_net_connection*   _connection
)
{
    // the stream must offer net.h's required operations
    if (!d_net_connection_is_complete(_connection))
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    // enumerations in range, and room for the shortest line limit in use
    if ( ((unsigned int)_options->role   > D_STARTTLS_ROLE_SERVER)     ||
         ((unsigned int)_options->policy < D_STARTTLS_POLICY_DISABLED) ||
         ((unsigned int)_options->policy > D_STARTTLS_POLICY_REQUIRED) ||
         (_options->buffer_bytes < D_STARTTLS_BUFFER_MIN) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    // a session that never upgrades needs no context
    if (_options->policy == D_STARTTLS_POLICY_DISABLED)
    {
        return D_STARTTLS_OK;
    }

    return d_starttls_internal_check_context(_options);
}

/*
d_starttls_internal_drain
  File-local: reads and drops `_count` bytes that the connection reported as
waiting, so they can never be read as TLS data. The receive buffer is empty
at this point and serves as the scratch space.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_drain(
    struct d_starttls_session* _session,
    size_t                     _count
)
{
    size_t remaining = _count;

    // take exactly what was counted, which cannot block
    while (remaining > 0)
    {
        const size_t want     = (remaining < _session->lines.capacity)
                                    ? remaining
                                    : _session->lines.capacity;
        size_t       received = 0;

        const enum d_starttls_error status =
            d_starttls_internal_read_plain(_session,
                                           _session->buffer,
                                           want,
                                           &received);

        if (status != D_STARTTLS_OK)
        {
            return status;
        }

        remaining           -= received;
        _session->discarded += received;
    }

    return D_STARTTLS_OK;
}

/*
d_starttls_internal_check_leftover
  File-local: the upgrade point's defense. A client has read exactly the
peer's go-ahead, and a conforming server sends nothing more until the
ClientHello, so any plaintext still waiting was appended by someone else.
Bytes already in the receive buffer are checked on either side. Only a
client asks the connection about bytes it has not read yet: by the time a
server upgrades, the client may legitimately have sent its ClientHello.
Waiting bytes are refused, or with reject_leftover off, dropped and counted.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_check_leftover(
    struct d_starttls_session* _session
)
{
    const size_t buffered = d_net_line_buffer_pending(&_session->lines);
    size_t       waiting  = 0;

    // plaintext already read past the switch point
    if (buffered > 0)
    {
        if (_session->reject_leftover)
        {
            return D_STARTTLS_ERROR_INJECTION;
        }

        _session->discarded += buffered;
        d_net_line_buffer_clear(&_session->lines);
    }

    // a server must not look further, and a connection that cannot count
    // leaves the rest to the handshake
    if ( (_session->role != D_STARTTLS_ROLE_CLIENT)          ||
         (!d_net_connection_available(_session->connection,
                                      &waiting))             ||
         (waiting == 0) )
    {
        return D_STARTTLS_OK;
    }

    return (_session->reject_leftover)
               ? D_STARTTLS_ERROR_INJECTION
               : d_starttls_internal_drain(_session,
                                           waiting);
}

/*
d_starttls_internal_tls_start
  File-local: the TLS session the upgrade runs, over the session's own
connection, from its context; a client names the server it expects, and a
server names none. The kernel validates the name, so one it cannot use is
CONFIG, as is a session the engine would not make for any reason but a lack
of memory: the upgrade's own arguments were sound, whatever the engine
called its refusal.
*/
D_STATIC enum d_starttls_error
d_starttls_internal_tls_start(
    struct d_starttls_session* _session
)
{
    struct d_ssl_session* const ssl  = malloc(sizeof(*ssl));
    const char* const           name =
        ( (_session->role == D_STARTTLS_ROLE_CLIENT) &&
          (_session->server_name) ) ? _session->server_name
                                    : "";
    const struct d_pack_text    host = { name,
                                         strlen(name) };

    if (!ssl)
    {
        return D_STARTTLS_ERROR_OUT_OF_MEMORY;
    }

    const enum d_ssl_error made = d_ssl_session_init(ssl,
                                                     _session->tls,
                                                     _session->connection,
                                                     host);

    // a session that was not made holds nothing
    if (made != D_SSL_OK)
    {
        free(ssl);

        return (made == D_SSL_ERROR_OUT_OF_MEMORY)
                   ? D_STARTTLS_ERROR_OUT_OF_MEMORY
                   : D_STARTTLS_ERROR_CONFIG;
    }

    _session->ssl = ssl;

    return D_STARTTLS_OK;
}

// session lifecycle
/*
d_starttls_options_default
  Every default comes from cfg_starttls.h through its D_INTERNAL_STARTTLS_*
values; the fields a caller must supply are left NULL, and the connection is
borrowed.
*/
struct d_starttls_options
d_starttls_options_default(
    enum d_starttls_role _role
)
{
    const struct d_starttls_options options =
    {
        .role               = _role,
        .policy             = D_INTERNAL_STARTTLS_POLICY,
        .tls                = NULL,
        .server_name        = NULL,
        .buffer_bytes       = D_INTERNAL_STARTTLS_BUFFER_BYTES,
        .owns_connection    = false,
        .reject_leftover    = (D_INTERNAL_STARTTLS_REJECT_LEFTOVER == 1),
        .strict_crlf        = (D_INTERNAL_STARTTLS_STRICT_CRLF == 1),
        .on_upgrade         = NULL,
        .on_upgrade_context = NULL
    };

    return options;
}

/*
d_starttls_init
  Everything is validated before anything is allocated, and the session is
written only on success, in one assignment, so a failed call leaves it
untouched. The server name is copied, so the caller's string need not
outlive the call.
*/
enum d_starttls_error
d_starttls_init(
    struct d_starttls_session*       _session,
    const struct d_starttls_options* _options,
    struct d_net_connection*         _connection
)
{
    if ( (!_session) ||
         (!_options) ||
         (!_connection) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    const enum d_starttls_error valid =
        d_starttls_internal_check_options(_options,
                                          _connection);

    if (valid != D_STARTTLS_OK)
    {
        return valid;
    }

    char* buffer      = malloc(_options->buffer_bytes);
    char* server_name =
        d_starttls_internal_copy_string(_options->server_name);

    // either allocation failing undoes both
    if ( (!buffer) ||
         ( (_options->server_name) &&
           (!server_name) ) )
    {
        free(buffer);
        free(server_name);

        return D_STARTTLS_ERROR_OUT_OF_MEMORY;
    }

    const struct d_starttls_session session =
    {
        .connection         = _connection,
        .tls                = _options->tls,
        .ssl                = NULL,
        .net_error          = D_NET_OK,
        .role               = _options->role,
        .policy             = _options->policy,
        .state              = D_STARTTLS_STATE_PLAINTEXT,
        .owns_connection    = _options->owns_connection,
        .reject_leftover    = _options->reject_leftover,
        .strict_crlf        = _options->strict_crlf,
        .server_name        = server_name,
        .buffer             = buffer,
        .lines              = { .capacity = _options->buffer_bytes },
        .discarded          = 0,
        .epoch              = 0,
        .detail             = NULL,
        .on_upgrade         = _options->on_upgrade,
        .on_upgrade_context = _options->on_upgrade_context
    };

    *_session = session;

    return D_STARTTLS_OK;
}

/*
d_starttls_close
  close_notify is attempted only on a session that is still secure: after a
fatal error the TLS session is not to be trusted with another record.
Closing an already closed session does nothing.
*/
enum d_starttls_error
d_starttls_close(
    struct d_starttls_session* _session
)
{
    if (!_session)
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    if (_session->state == D_STARTTLS_STATE_CLOSED)
    {
        return D_STARTTLS_OK;
    }

    enum d_starttls_error status = D_STARTTLS_OK;

    // close_notify lets the peer tell a deliberate end from a truncation
    if ( (_session->state == D_STARTTLS_STATE_SECURE) &&
         (_session->ssl) )
    {
        status = d_starttls_internal_tls_outcome(
                     _session,
                     d_ssl_session_shutdown(_session->ssl),
                     false);
    }

    // a borrowed connection is the caller's to close
    if (_session->owns_connection)
    {
        d_net_connection_close(_session->connection);
    }

    _session->state = D_STARTTLS_STATE_CLOSED;

    return status;
}

/*
d_starttls_destroy
  The TLS session goes first, then an owned connection, whose destroy closes
it again harmlessly if d_starttls_close already did. The session is zeroed
and then marked closed, so a second destroy -- or a stray call through a
stale pointer -- finds nothing to release and nothing to use.
*/
void
d_starttls_destroy(
    struct d_starttls_session* _session
)
{
    if (!_session)
    {
        return;
    }

    // the TLS session, which sends nothing as it goes
    if (_session->ssl)
    {
        d_ssl_session_destroy(_session->ssl);
        free(_session->ssl);
    }

    // an abortive close: no close_notify, but no leaked stream either
    if (_session->owns_connection)
    {
        d_net_connection_destroy(_session->connection);
    }

    free(_session->buffer);
    free(_session->server_name);
    memset(_session,
           0,
           sizeof(*_session));
    _session->state = D_STARTTLS_STATE_CLOSED;

    return;
}

// I/O
/*
d_starttls_send
  A zero-length send succeeds without touching the stream, on an open
session.
*/
enum d_starttls_error
d_starttls_send(
    struct d_starttls_session* _session,
    const void*                _data,
    size_t                     _size
)
{
    if ( (!_session) ||
         ( (!_data) &&
           (_size > 0) ) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    const enum d_starttls_error status =
        d_starttls_internal_require_open(_session);

    // nowhere to send, or nothing to send
    if ( (status != D_STARTTLS_OK) ||
         (_size == 0) )
    {
        return status;
    }

    return d_starttls_internal_write_all(_session,
                                         _data,
                                         _size);
}

/*
d_starttls_recv
  Bytes the line reader already pulled in are delivered first and in order,
so byte and line reads can be mixed freely -- as an IMAP literal requires.
*/
enum d_starttls_error
d_starttls_recv(
    struct d_starttls_session* _session,
    void*                      _buffer,
    size_t                     _capacity,
    size_t*                    _received
)
{
    if ( (!_session)  ||
         (!_buffer)   ||
         (!_received) ||
         (_capacity == 0) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    *_received = 0;

    const enum d_starttls_error status =
        d_starttls_internal_require_open(_session);

    if (status != D_STARTTLS_OK)
    {
        return status;
    }

    const struct d_pack_text held =
        d_net_line_buffer_held(&_session->lines,
                               _session->buffer);

    // buffered bytes come first
    if (held.length > 0)
    {
        const size_t count = (held.length < _capacity) ? held.length
                                                       : _capacity;

        memcpy(_buffer,
               held.data,
               count);
        d_net_line_buffer_consume(&_session->lines,
                                  count);
        *_received = count;

        return D_STARTTLS_OK;
    }

    return d_starttls_internal_read(_session,
                                    _buffer,
                                    _capacity,
                                    _received);
}

/*
d_starttls_recv_exact
  An end of stream before the first byte is an ordinary close; one after it
means the payload the protocol announced was cut short, which is fatal.
*/
enum d_starttls_error
d_starttls_recv_exact(
    struct d_starttls_session* _session,
    void*                      _buffer,
    size_t                     _size
)
{
    if ( (!_session) ||
         ( (!_buffer) &&
           (_size > 0) ) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    unsigned char*        cursor    = _buffer;
    size_t                remaining = _size;
    enum d_starttls_error status    =
        d_starttls_internal_require_open(_session);

    // gather until the announced length has arrived
    while ( (status == D_STARTTLS_OK) &&
            (remaining > 0) )
    {
        size_t received = 0;

        status = d_starttls_recv(_session,
                                 cursor,
                                 remaining,
                                 &received);
        cursor    += received;
        remaining -= received;
    }

    // the stream ended part-way through a counted payload
    if ( (status == D_STARTTLS_ERROR_CLOSED) &&
         (remaining < _size) )
    {
        return d_starttls_internal_fail(_session,
                                        D_STARTTLS_ERROR_PROTOCOL);
    }

    return status;
}

/*
d_starttls_buffered
  The line buffer's count of bytes not yet consumed.
*/
size_t
d_starttls_buffered(
    const struct d_starttls_session* _session
)
{
    return (_session) ? d_net_line_buffer_pending(&_session->lines)
                      : 0;
}

/*
d_starttls_send_line
  strlen() bounds the line, so no NUL can appear inside it; CR and LF are
checked by the shared line writer.
*/
enum d_starttls_error
d_starttls_send_line(
    struct d_starttls_session* _session,
    const char*                _line
)
{
    if ( (!_session) ||
         (!_line) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    const enum d_starttls_error status =
        d_starttls_internal_require_open(_session);

    if (status != D_STARTTLS_OK)
    {
        return status;
    }

    return d_starttls_internal_send_text_line(_session,
                                              _line,
                                              strlen(_line));
}

/*
d_starttls_send_linef
  Formats into the stack scratch first. A line that does not fit is measured
by that attempt, held to the line limit before any memory is spent on it,
and formatted again from a copy of the arguments into an exact heap buffer.
*/
enum d_starttls_error
d_starttls_send_linef(
    struct d_starttls_session* _session,
    const char*                _format,
    ...
)
{
    if ( (!_session) ||
         (!_format) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    const enum d_starttls_error ready =
        d_starttls_internal_require_open(_session);

    if (ready != D_STARTTLS_OK)
    {
        return ready;
    }

    char    stack[D_INTERNAL_STARTTLS_LINE_STACK];  // filled before read
    va_list arguments;  // va_start() sets it
    va_list again;

    va_start(arguments,
             _format);
    va_copy(again,
            arguments);

    const int measured = vsnprintf(stack,
                                   sizeof(stack),
                                   _format,
                                   arguments);

    va_end(arguments);

    // a formatting error produces no line at all
    enum d_starttls_error status = D_STARTTLS_ERROR_INVALID_ARGUMENT;

    // a short line is already in the scratch
    if ( (measured >= 0) &&
         ((size_t)measured < sizeof(stack)) )
    {
        status = d_starttls_internal_send_text_line(_session,
                                                    stack,
                                                    (size_t)measured);
    }
    // a long one is formatted again, if it fits the line limit
    else if (measured >= 0)
    {
        status = d_starttls_internal_send_long_line(_session,
                                                    (size_t)measured,
                                                    _format,
                                                    again);
    }

    va_end(again);

    return status;
}

/*
d_starttls_recv_line
  Reads until an LF arrives or the buffer is full. An end of stream between
lines is an ordinary close; one inside a line leaves a fragment no protocol
can act on, so it is fatal.
*/
enum d_starttls_error
d_starttls_recv_line(
    struct d_starttls_session* _session,
    const char**               _line,
    size_t*                    _length
)
{
    if ( (!_session) ||
         (!_line)    ||
         (!_length) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    *_line   = NULL;
    *_length = 0;

    enum d_starttls_error status =
        d_starttls_internal_require_open(_session);

    // look for a line end, reading more until one arrives
    while (status == D_STARTTLS_OK)
    {
        struct d_net_line line   = { { NULL, 0 }, 0, false };
        const size_t      unread = d_net_line_buffer_pending(&_session->lines);

        if (d_net_line_buffer_find(&_session->lines,
                                   _session->buffer,
                                   &line) == D_NET_LINE_FOUND)
        {
            return d_starttls_internal_take_line(_session,
                                                 &line,
                                                 _line,
                                                 _length);
        }

        status = d_starttls_internal_fill(_session);

        // the buffer filled up without a line end
        if (status == D_STARTTLS_ERROR_LINE_TOO_LONG)
        {
            return d_starttls_internal_fail(_session,
                                            status);
        }

        // the stream ended inside a line
        if ( (status == D_STARTTLS_ERROR_CLOSED) &&
             (unread > 0) )
        {
            return d_starttls_internal_fail(_session,
                                            D_STARTTLS_ERROR_PROTOCOL);
        }
    }

    return status;
}

// upgrade
/*
d_starttls_negotiate
  The policy is applied at the two points where TLS can be lost -- an offer
that never arrived, and a request the peer refused -- and nowhere else. A
failure inside either callback is fatal whatever the policy: the peer may
have seen part of the request, or sent part of an answer.
*/
enum d_starttls_error
d_starttls_negotiate(
    struct d_starttls_session*          _session,
    const struct d_starttls_negotiator* _negotiator,
    void*                               _context,
    bool                                _offered
)
{
    if ( (!_session)             ||
         (!_negotiator)          ||
         (!_negotiator->request) ||
         (!_negotiator->reply) )
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    // negotiation is the client's half; a server grants with an upgrade
    if ( (_session->role  != D_STARTTLS_ROLE_CLIENT) ||
         (_session->state != D_STARTTLS_STATE_PLAINTEXT) )
    {
        return D_STARTTLS_ERROR_STATE;
    }

    // an explicit opt-out: there is nothing to negotiate
    if (_session->policy == D_STARTTLS_POLICY_DISABLED)
    {
        return D_STARTTLS_OK;
    }

    // a missing offer is exactly what a stripping attacker produces
    if (!_offered)
    {
        return d_starttls_internal_decline(_session,
                                           D_STARTTLS_ERROR_NOT_OFFERED);
    }

    enum d_starttls_error status = _negotiator->request(_session,
                                                        _context);
    enum d_starttls_reply reply  = D_STARTTLS_REPLY_MALFORMED;

    // the answer is read only if the whole request went out
    if (status == D_STARTTLS_OK)
    {
        status = _negotiator->reply(_session,
                                    _context,
                                    &reply);
    }

    if (status != D_STARTTLS_OK)
    {
        return d_starttls_internal_fail(_session,
                                        status);
    }

    // the go-ahead: switch before anything else is read
    if (reply == D_STARTTLS_REPLY_PROCEED)
    {
        return d_starttls_upgrade(_session);
    }

    // a refusal is survivable only where the policy permits plaintext
    if (reply == D_STARTTLS_REPLY_REFUSED)
    {
        return d_starttls_internal_decline(_session,
                                           D_STARTTLS_ERROR_REJECTED);
    }

    return d_starttls_internal_fail(_session,
                                    D_STARTTLS_ERROR_PROTOCOL);
}

/*
d_starttls_upgrade
  The leftover check runs before any TLS session exists, so injected
plaintext is refused while it is still plaintext rather than surfacing as an
opaque handshake error. The epoch advances, and on_upgrade runs, only after
the handshake -- the kernel's checks of the peer included -- has succeeded.
*/
enum d_starttls_error
d_starttls_upgrade(
    struct d_starttls_session* _session
)
{
    if (!_session)
    {
        return D_STARTTLS_ERROR_INVALID_ARGUMENT;
    }

    // only a plaintext session switches, and only one allowed to use TLS
    if ( (_session->state  != D_STARTTLS_STATE_PLAINTEXT) ||
         (_session->policy == D_STARTTLS_POLICY_DISABLED) )
    {
        return D_STARTTLS_ERROR_STATE;
    }

    // d_starttls_init guarantees a context under any other policy
    if (!_session->tls)
    {
        return d_starttls_internal_fail(_session,
                                        D_STARTTLS_ERROR_NO_BACKEND);
    }

    enum d_starttls_error status =
        d_starttls_internal_check_leftover(_session);

    if (status == D_STARTTLS_OK)
    {
        status = d_starttls_internal_tls_start(_session);
    }

    // the handshake runs to completion on a blocking connection
    if (status == D_STARTTLS_OK)
    {
        status = d_starttls_internal_tls_outcome(
                     _session,
                     d_ssl_session_handshake(_session->ssl),
                     true);
    }

    if (status != D_STARTTLS_OK)
    {
        return d_starttls_internal_fail(_session,
                                        status);
    }

    _session->state = D_STARTTLS_STATE_SECURE;
    _session->epoch++;

    // what was learned in plaintext may have been written by an attacker
    if (_session->on_upgrade)
    {
        _session->on_upgrade(_session,
                             _session->on_upgrade_context);
    }

    return D_STARTTLS_OK;
}

// queries
/*
d_starttls_get_state
  A NULL session reads as closed: there is nothing to use.
*/
enum d_starttls_state
d_starttls_get_state(
    const struct d_starttls_session* _session
)
{
    return (_session) ? _session->state : D_STARTTLS_STATE_CLOSED;
}

/*
d_starttls_is_secure
  Only D_STARTTLS_STATE_SECURE counts; a failed session is not secure even if
its handshake once completed.
*/
bool
d_starttls_is_secure(
    const struct d_starttls_session* _session
)
{
    return ( (_session) &&
             (_session->state == D_STARTTLS_STATE_SECURE) );
}

/*
d_starttls_is_satisfied
  Plaintext satisfies every policy except REQUIRED; a failed or closed
session satisfies none.
*/
bool
d_starttls_is_satisfied(
    const struct d_starttls_session* _session
)
{
    if (!_session)
    {
        return false;
    }

    return ( (_session->state == D_STARTTLS_STATE_SECURE) ||
             ( (_session->state  == D_STARTTLS_STATE_PLAINTEXT) &&
               (_session->policy != D_STARTTLS_POLICY_REQUIRED) ) );
}

/*
d_starttls_epoch
  Read directly from the session.
*/
unsigned int
d_starttls_epoch(
    const struct d_starttls_session* _session
)
{
    return (_session) ? _session->epoch : 0;
}

/*
d_starttls_detail
  Read directly from the session.
*/
const char*
d_starttls_detail(
    const struct d_starttls_session* _session
)
{
    return (_session) ? _session->detail : NULL;
}

/*
d_starttls_net_error
  Read directly from the session.
*/
enum d_net_error
d_starttls_net_error(
    const struct d_starttls_session* _session
)
{
    return (_session) ? _session->net_error : D_NET_OK;
}

/*
d_starttls_tls_session
  Read directly from the session.
*/
const struct d_ssl_session*
d_starttls_tls_session(
    const struct d_starttls_session* _session
)
{
    return (_session) ? _session->ssl : NULL;
}

/*
d_starttls_error_string
  One entry per enumerator; with no default, the compiler flags any status
added later without a description.
*/
const char*
d_starttls_error_string(
    enum d_starttls_error _error
)
{
    switch (_error)
    {
        case D_STARTTLS_OK:
            return "success";
        case D_STARTTLS_ERROR_INVALID_ARGUMENT:
            return "invalid argument";
        case D_STARTTLS_ERROR_STATE:
            return "not valid in the session's state";
        case D_STARTTLS_ERROR_OUT_OF_MEMORY:
            return "out of memory";
        case D_STARTTLS_ERROR_TRANSPORT:
            return "transport error";
        case D_STARTTLS_ERROR_CLOSED:
            return "connection closed by the peer";
        case D_STARTTLS_ERROR_TRUNCATED:
            return "connection closed without TLS close_notify";
        case D_STARTTLS_ERROR_TIMED_OUT:
            return "timed out";
        case D_STARTTLS_ERROR_LINE_TOO_LONG:
            return "line too long";
        case D_STARTTLS_ERROR_PROTOCOL:
            return "protocol violation";
        case D_STARTTLS_ERROR_NOT_OFFERED:
            return "TLS required but not offered";
        case D_STARTTLS_ERROR_REJECTED:
            return "TLS required but refused";
        case D_STARTTLS_ERROR_INJECTION:
            return "plaintext waiting at the TLS upgrade";
        case D_STARTTLS_ERROR_NO_BACKEND:
            return "no TLS context available";
        case D_STARTTLS_ERROR_CONFIG:
            return "TLS configuration rejected";
        case D_STARTTLS_ERROR_HANDSHAKE:
            return "TLS handshake failed";
        case D_STARTTLS_ERROR_VERIFY:
            return "peer certificate verification failed";
    }

    return "unknown status";
}

/*
d_starttls_state_string
  One entry per enumerator, as above.
*/
const char*
d_starttls_state_string(
    enum d_starttls_state _state
)
{
    switch (_state)
    {
        case D_STARTTLS_STATE_PLAINTEXT:
            return "plaintext";
        case D_STARTTLS_STATE_SECURE:
            return "secure";
        case D_STARTTLS_STATE_FAILED:
            return "failed";
        case D_STARTTLS_STATE_CLOSED:
            return "closed";
    }

    return "unknown";
}

/*
d_starttls_policy_string
  One entry per enumerator, as above.
*/
const char*
d_starttls_policy_string(
    enum d_starttls_policy _policy
)
{
    switch (_policy)
    {
        case D_STARTTLS_POLICY_DISABLED:
            return "disabled";
        case D_STARTTLS_POLICY_OPPORTUNISTIC:
            return "opportunistic";
        case D_STARTTLS_POLICY_REQUIRED:
            return "required";
    }

    return "unknown";
}

/*
d_starttls_error_to_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules. A value outside the enum is UNKNOWN.
*/
enum d_net_error
d_starttls_error_to_net(
    enum d_starttls_error _error
)
{
    switch (_error)
    {
        case D_STARTTLS_OK:
            return D_NET_OK;
        case D_STARTTLS_ERROR_INVALID_ARGUMENT:
        case D_STARTTLS_ERROR_STATE:
        case D_STARTTLS_ERROR_CONFIG:
            return D_NET_ERROR_INVALID_ARGUMENT;
        case D_STARTTLS_ERROR_PROTOCOL:
        case D_STARTTLS_ERROR_INJECTION:
        case D_STARTTLS_ERROR_HANDSHAKE:
            return D_NET_ERROR_PROTOCOL;
        case D_STARTTLS_ERROR_LINE_TOO_LONG:
            return D_NET_ERROR_MESSAGE_TOO_LARGE;
        case D_STARTTLS_ERROR_CLOSED:
        case D_STARTTLS_ERROR_TRUNCATED:
            return D_NET_ERROR_CLOSED;
        case D_STARTTLS_ERROR_TIMED_OUT:
            return D_NET_ERROR_TIMED_OUT;
        case D_STARTTLS_ERROR_OUT_OF_MEMORY:
            return D_NET_ERROR_OUT_OF_MEMORY;
        case D_STARTTLS_ERROR_VERIFY:
            return D_NET_ERROR_ACCESS_DENIED;
        case D_STARTTLS_ERROR_TRANSPORT:
        case D_STARTTLS_ERROR_NOT_OFFERED:
        case D_STARTTLS_ERROR_REJECTED:
        case D_STARTTLS_ERROR_NO_BACKEND:
            return D_NET_ERROR_UNKNOWN;
    }

    return D_NET_ERROR_UNKNOWN;
}

/*
d_starttls_error_from_net
  One case per enumerator and no default, so the compiler reports a missing
case; the groups follow net.h's rules the other way. A value outside the
enum is TRANSPORT.
*/
enum d_starttls_error
d_starttls_error_from_net(
    enum d_net_error _error
)
{
    switch (_error)
    {
        case D_NET_OK:
            return D_STARTTLS_OK;
        case D_NET_ERROR_INVALID_ARGUMENT:
            return D_STARTTLS_ERROR_INVALID_ARGUMENT;
        case D_NET_ERROR_OUT_OF_MEMORY:
            return D_STARTTLS_ERROR_OUT_OF_MEMORY;
        case D_NET_ERROR_TIMED_OUT:
            return D_STARTTLS_ERROR_TIMED_OUT;
        case D_NET_ERROR_PROTOCOL:
            return D_STARTTLS_ERROR_PROTOCOL;
        case D_NET_ERROR_CLOSED:
        case D_NET_ERROR_CONNECTION_RESET:
        case D_NET_ERROR_CONNECTION_ABORTED:
            return D_STARTTLS_ERROR_CLOSED;
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
            return D_STARTTLS_ERROR_TRANSPORT;
    }

    return D_STARTTLS_ERROR_TRANSPORT;
}
