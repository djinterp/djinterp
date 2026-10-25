/*******************************************************************************
* djinterp [net]                                                net_tests_peer.c
*
* The scripted peer and the toy engine net's protocol suites run against.
*   Definitions for net_tests_peer.h: a net.h connection that plays the
* other party one turn at a time, each released by what the session under
* test has sent, and an engine of the SSL kernel whose handshake and cipher
* a test can write out by hand.
*
*
* path:      /tests/djinterp/net/net_tests_peer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./net_tests_peer.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // snprintf
#include <stdlib.h>   // calloc, free
#include <string.h>   // memcmp, memcpy, memmove, strlen
// re_std
#include "../../../inc/re_std/cstdint/dstdint.h"  // SIZE_MAX, uint32_t


// the scripted peer
/*
d_tests_net_peer_find
  Whether `_text` occurs in the `_size` bytes at `_bytes`, and if so the
offset just past its first occurrence, in *_end.
*/
static bool
d_tests_net_peer_find(
    const unsigned char* _bytes,
    size_t               _size,
    const char*          _text,
    size_t*              _end
)
{
    const size_t length = strlen(_text);

    // every position the text could start at
    for (size_t at = 0; (length <= _size) && (at <= (_size - length)); ++at)
    {
        if (memcmp(_bytes + at,
                   _text,
                   length) == 0)
        {
            *_end = at + length;

            return true;
        }
    }

    return false;
}

/*
d_tests_net_peer_release
  Releases each turn whose `after` the session has now sent, in order: a
turn waits for its text after the point where the turn before it matched,
and an empty `after` comes with the turn before.
*/
static void
d_tests_net_peer_release(
    struct d_tests_net_peer* _peer
)
{
    // release turns until one is still waiting
    while (_peer->released < _peer->count)
    {
        const struct d_tests_net_turn* turn =
            &_peer->turns[_peer->released];
        size_t end = 0;

        if ( (turn->after[0] != '\0') &&
             (!d_tests_net_peer_find(_peer->output + _peer->matched,
                                     _peer->sent - _peer->matched,
                                     turn->after,
                                     &end)) )
        {
            return;
        }

        _peer->matched  += end;
        _peer->released += 1;
    }

    return;
}

/*
d_tests_net_peer_waiting
  The bytes released and not yet read.
*/
static size_t
d_tests_net_peer_waiting(
    struct d_tests_net_peer* _peer
)
{
    size_t waiting = 0;

    d_tests_net_peer_release(_peer);

    // the unread part of every released turn
    for (size_t i = _peer->current; i < _peer->released; ++i)
    {
        waiting += _peer->turns[i].length - _peer->turns[i].taken;
    }

    return waiting;
}

/*
d_tests_net_peer_read
  The connection's read: bytes from the first released turn not yet read,
no more than one turn's worth.
*/
static struct d_net_io_result
d_tests_net_peer_read(
    struct d_net_connection* _connection,
    void*                    _buffer,
    size_t                   _size
)
{
    struct d_tests_net_peer* const peer =
        (struct d_tests_net_peer*)_connection;

    peer->reads += 1;
    d_tests_net_peer_release(peer);

    // a read that claims more than it had room for
    if (peer->lies)
    {
        return d_net_io_result_make(_size + 1u,
                                    D_NET_OK);
    }

    // step past turns already read
    while ( (peer->current < peer->released) &&
            (peer->turns[peer->current].taken ==
             peer->turns[peer->current].length) )
    {
        peer->current += 1;
    }

    // nothing released: the end, or a stall
    if (peer->current == peer->released)
    {
        return d_net_io_result_make(0u,
                                    ( (peer->current == peer->count) &&
                                      (peer->hangs_up) )
                                    ? D_NET_OK
                                    : D_NET_ERROR_TIMED_OUT);
    }

    struct d_tests_net_turn* turn  = &peer->turns[peer->current];
    size_t                   count = turn->length - turn->taken;

    if (count > _size)
    {
        count = _size;
    }

    if ( (peer->recv_chunk > 0) &&
         (count > peer->recv_chunk) )
    {
        count = peer->recv_chunk;
    }

    memcpy(_buffer,
           turn->text + turn->taken,
           count);
    turn->taken += count;

    return d_net_io_result_make(count,
                                D_NET_OK);
}

/*
d_tests_net_peer_write
  The connection's write: keeps what it takes, up to `send_chunk` bytes a
call, and fails a write that would pass `fail_after`, or that `output` has
no room for.
*/
static struct d_net_io_result
d_tests_net_peer_write(
    struct d_net_connection* _connection,
    const void*              _data,
    size_t                   _size
)
{
    struct d_tests_net_peer* const peer  =
        (struct d_tests_net_peer*)_connection;
    size_t                         count = _size;

    peer->writes += 1;

    // a write that takes nothing, and says nothing of why
    if (peer->stalls)
    {
        return d_net_io_result_make(0u,
                                    D_NET_OK);
    }

    if ( (peer->send_chunk > 0) &&
         (count > peer->send_chunk) )
    {
        count = peer->send_chunk;
    }

    if ( (count > (peer->fail_after - peer->sent)) ||
         (count > (sizeof(peer->output) - peer->sent)) )
    {
        return d_net_io_result_make(0u,
                                    (peer->resets)
                                    ? D_NET_ERROR_CONNECTION_RESET
                                    : D_NET_ERROR_UNKNOWN);
    }

    memcpy(peer->output + peer->sent,
           _data,
           count);
    peer->sent += count;

    return d_net_io_result_make(count,
                                D_NET_OK);
}

/*
d_tests_net_peer_is_open
  Open until closed once.
*/
static bool
d_tests_net_peer_is_open(
    const struct d_net_connection* _connection
)
{
    return (((const struct d_tests_net_peer*)_connection)->closes == 0);
}

/*
d_tests_net_peer_close
  Counted; closing again is harmless.
*/
static void
d_tests_net_peer_close(
    struct d_net_connection* _connection
)
{
    ((struct d_tests_net_peer*)_connection)->closes += 1;

    return;
}

/*
d_tests_net_peer_destroy
  Counted; the peer's storage is the test's, freed by
d_tests_net_peer_free.
*/
static void
d_tests_net_peer_destroy(
    struct d_net_connection* _connection
)
{
    ((struct d_tests_net_peer*)_connection)->destroys += 1;

    return;
}

/*
d_tests_net_peer_available
  Every byte released and unread, unless the peer will not count.
*/
static bool
d_tests_net_peer_available(
    const struct d_net_connection* _connection,
    size_t*                        _out_count
)
{
    struct d_tests_net_peer* const peer =
        (struct d_tests_net_peer*)_connection;

    if (peer->uncounted)
    {
        return false;
    }

    *_out_count = d_tests_net_peer_waiting(peer);

    return true;
}

/*
COUNTING_OPERATIONS
  The scripted peer's operations, available included.
*/
static const struct d_net_connection_vtable COUNTING_OPERATIONS =
{
    d_tests_net_peer_read,
    d_tests_net_peer_write,
    d_tests_net_peer_is_open,
    d_tests_net_peer_close,
    NULL,
    NULL,
    NULL,
    d_tests_net_peer_destroy,
    d_tests_net_peer_available
};

/*
SILENT_OPERATIONS
  The same, without available: a connection that cannot count.
*/
static const struct d_net_connection_vtable SILENT_OPERATIONS =
{
    d_tests_net_peer_read,
    d_tests_net_peer_write,
    d_tests_net_peer_is_open,
    d_tests_net_peer_close,
    NULL,
    NULL,
    NULL,
    d_tests_net_peer_destroy,
    NULL
};

/*
d_tests_net_peer_new
  Zeroed, so nothing is scripted and every limit is off but `fail_after`,
which starts out unreachable; its connection counts.
*/
struct d_tests_net_peer*
d_tests_net_peer_new(
    void
)
{
    struct d_tests_net_peer* peer = calloc(1,
                                           sizeof(*peer));

    if (peer)
    {
        peer->fail_after = SIZE_MAX;
        (void)d_net_connection_init(&peer->base,
                                    &COUNTING_OPERATIONS);
    }

    return peer;
}

/*
d_tests_net_peer_free
  free(), for symmetry with d_tests_net_peer_new.
*/
void
d_tests_net_peer_free(
    struct d_tests_net_peer* _peer
)
{
    free(_peer);

    return;
}

/*
d_tests_net_peer_connection
  One of the two operation tables, by whether the test wants the connection
to count what is waiting.
*/
struct d_net_connection*
d_tests_net_peer_connection(
    struct d_tests_net_peer* _peer,
    bool                     _counts
)
{
    (void)d_net_connection_init(&_peer->base,
                                (_counts) ? &COUNTING_OPERATIONS
                                : &SILENT_OPERATIONS);

    return &_peer->base;
}

/*
d_tests_net_peer_say_bytes
  Appends a turn; false if the script or the turn is full.
*/
bool
d_tests_net_peer_say_bytes(
    struct d_tests_net_peer* _peer,
    const char*              _after,
    const void*              _bytes,
    size_t                   _length
)
{
    if ( (_peer->count == D_TESTS_NET_PEER_TURNS) ||
         (_length > D_TESTS_NET_PEER_TURN_BYTES)  ||
         (strlen(_after) >= sizeof(_peer->turns[0].after)) )
    {
        return false;
    }

    struct d_tests_net_turn* turn = &_peer->turns[_peer->count];

    memcpy(turn->after,
           _after,
           strlen(_after) + 1);
    memcpy(turn->text,
           _bytes,
           _length);
    turn->length  = _length;
    turn->taken   = 0;
    _peer->count += 1;

    return true;
}

/*
d_tests_net_peer_say
  A text turn.
*/
bool
d_tests_net_peer_say(
    struct d_tests_net_peer* _peer,
    const char*              _after,
    const char*              _text
)
{
    return d_tests_net_peer_say_bytes(_peer,
                                      _after,
                                      _text,
                                      strlen(_text));
}

/*
d_tests_net_peer_say_sealed
  A turn the toy engine will read: the text and what releases it are both
sealed, since both travel under the toy cipher.
*/
bool
d_tests_net_peer_say_sealed(
    struct d_tests_net_peer* _peer,
    const char*              _after,
    const char*              _text
)
{
    char after[64]                         = { 0 };
    char text[D_TESTS_NET_PEER_TURN_BYTES] = { 0 };

    if ( (strlen(_after) >= sizeof(after)) ||
         (strlen(_text) >= sizeof(text)) )
    {
        return false;
    }

    d_tests_net_toy_seal(after,
                         sizeof(after),
                         _after);
    d_tests_net_toy_seal(text,
                         sizeof(text),
                         _text);

    return d_tests_net_peer_say(_peer,
                                after,
                                text);
}

/*
d_tests_net_peer_sent
  Whether the session has sent exactly `_expected`, and nothing more.
*/
bool
d_tests_net_peer_sent(
    const struct d_tests_net_peer* _peer,
    const char*                    _expected
)
{
    const size_t length = strlen(_expected);

    return ( (_peer->sent == length) &&
             (memcmp(_peer->output,
                     _expected,
                     length) == 0) );
}

// the toy engine
// D_TESTS_NET_TOY_BYTES
//   constant: the most bytes a toy session holds each way.
#define D_TESTS_NET_TOY_BYTES 65536u

// D_TESTS_NET_TOY_STEP
//   constant: the handshake each side sends, in bytes.
#define D_TESTS_NET_TOY_STEP 9u

// d_tests_net_toy_session
//   struct: one session of the toy engine: where its handshake is, the
// ciphertext fed to it and not yet consumed, and what it has queued.
struct d_tests_net_toy_session
{
    struct d_tests_net_toy* toy;      // the record
    int                     state;    // 0, 1 (hello sent), 2 (done)
    bool                    closed;   // close_notify received
    size_t                  in_size;  // bytes in `in`
    size_t                  out_size; // bytes in `out`
    unsigned char           in[D_TESTS_NET_TOY_BYTES];
    unsigned char           out[D_TESTS_NET_TOY_BYTES];
};

// d_tests_net_toy_making
//   the record a context is being made over; context_create takes it.
static struct d_tests_net_toy* d_tests_net_toy_making = NULL;

/*
d_tests_net_toy_byte
  The toy cipher on one byte: letters rotate by 13 and digits by 5, so
applying it twice restores the byte; everything else is left alone.
*/
static unsigned char
d_tests_net_toy_byte(
    unsigned char _byte
)
{
    if ( (_byte >= 'a') &&
         (_byte <= 'z') )
    {
        return (unsigned char)('a' + ((_byte - 'a' + 13) % 26));
    }

    if ( (_byte >= 'A') &&
         (_byte <= 'Z') )
    {
        return (unsigned char)('A' + ((_byte - 'A' + 13) % 26));
    }

    if ( (_byte >= '0') &&
         (_byte <= '9') )
    {
        return (unsigned char)('0' + ((_byte - '0' + 5) % 10));
    }

    return _byte;
}

/*
d_tests_net_toy_seal
  The toy cipher on a string, truncated to fit `_capacity`.
*/
void
d_tests_net_toy_seal(
    char*       _out,
    size_t      _capacity,
    const char* _text
)
{
    size_t i = 0;

    // every byte that fits, then the terminator
    for (; (_text[i] != '\0') && ((i + 1) < _capacity); ++i)
    {
        _out[i] = (char)d_tests_net_toy_byte((unsigned char)_text[i]);
    }

    _out[i] = '\0';

    return;
}

/*
d_tests_net_toy_queue
  Appends to what a session has queued; false if it does not fit.
*/
static bool
d_tests_net_toy_queue(
    struct d_tests_net_toy_session* _session,
    const void*                     _data,
    size_t                          _size
)
{
    if (_size > (sizeof(_session->out) - _session->out_size))
    {
        return false;
    }

    memcpy(_session->out + _session->out_size,
           _data,
           _size);
    _session->out_size += _size;

    return true;
}

/*
d_tests_net_toy_consume
  Drops the first `_size` bytes fed to a session.
*/
static void
d_tests_net_toy_consume(
    struct d_tests_net_toy_session* _session,
    size_t                          _size
)
{
    memmove(_session->in,
            _session->in + _size,
            _session->in_size - _size);
    _session->in_size -= _size;

    return;
}

/*
d_tests_net_toy_context_create
  The context is the record being made over, which learns its role here.
*/
static enum d_ssl_error
d_tests_net_toy_context_create(
    const struct d_ssl_config* _config,
    const struct d_ssl_plan*   _plan,
    void**                     _out_context
)
{
    (void)_config;

    // only d_tests_net_toy_context makes toy contexts
    if (!d_tests_net_toy_making)
    {
        return D_SSL_ERROR_BACKEND;
    }

    d_tests_net_toy_making->role = _plan->role;
    *_out_context                = d_tests_net_toy_making;

    return D_SSL_OK;
}

/*
d_tests_net_toy_context_destroy
  Counts; the record is the test's own.
*/
static void
d_tests_net_toy_context_destroy(
    void* _context
)
{
    ((struct d_tests_net_toy*)_context)->releases += 1;

    return;
}

/*
d_tests_net_toy_session_create
  Records the names it was given, then makes a session unless told not to.
*/
static enum d_ssl_error
d_tests_net_toy_session_create(
    void*       _context,
    const char* _host,
    const char* _sni,
    void**      _out_session
)
{
    struct d_tests_net_toy* const toy = _context;

    toy->creates += 1;
    (void)snprintf(toy->last_host,
                   sizeof(toy->last_host),
                   "%s",
                   (_host) ? _host : "(null)");
    (void)snprintf(toy->last_sni,
                   sizeof(toy->last_sni),
                   "%s",
                   (_sni) ? _sni : "(null)");

    if (toy->create_status != D_SSL_OK)
    {
        return toy->create_status;
    }

    struct d_tests_net_toy_session* const session =
        calloc(1,
               sizeof(*session));

    if (!session)
    {
        return D_SSL_ERROR_OUT_OF_MEMORY;
    }

    session->toy  = toy;
    *_out_session = session;

    return D_SSL_OK;
}

/*
d_tests_net_toy_session_destroy
  Counts, then frees.
*/
static void
d_tests_net_toy_session_destroy(
    void* _session
)
{
    struct d_tests_net_toy_session* const session = _session;

    session->toy->destroys += 1;
    free(session);

    return;
}

/*
d_tests_net_toy_feed
  Keeps the ciphertext until a step consumes it.
*/
static enum d_ssl_error
d_tests_net_toy_feed(
    void*       _session,
    const void* _data,
    size_t      _size
)
{
    struct d_tests_net_toy_session* const session = _session;

    if (_size > (sizeof(session->in) - session->in_size))
    {
        return D_SSL_ERROR_OUT_OF_MEMORY;
    }

    memcpy(session->in + session->in_size,
           _data,
           _size);
    session->in_size += _size;

    return D_SSL_OK;
}

/*
d_tests_net_toy_drain
  Hands over what is queued, from the front.
*/
static enum d_ssl_error
d_tests_net_toy_drain(
    void*   _session,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_size
)
{
    struct d_tests_net_toy_session* const session = _session;
    const size_t                          count   =
        (session->out_size < _capacity) ? session->out_size
                                        : _capacity;

    memcpy(_buffer,
           session->out,
           count);
    memmove(session->out,
            session->out + count,
            session->out_size - count);
    session->out_size -= count;
    *_out_size         = count;

    return D_SSL_OK;
}

/*
d_tests_net_toy_handshake
  A client queues its hello and waits for the reply; a server waits for the
hello and replies. Anything else where the other's step belongs is a failed
handshake.
*/
static enum d_ssl_error
d_tests_net_toy_handshake(
    void* _session
)
{
    struct d_tests_net_toy_session* const session = _session;
    const bool client = (session->toy->role == D_SSL_ROLE_CLIENT);

    // a failure the test asked for, or a handshake already done
    if ( (session->toy->handshake_status != D_SSL_OK) ||
         (session->state == 2) )
    {
        return session->toy->handshake_status;
    }

    // the client speaks first
    if ( (client) &&
         (session->state == 0) )
    {
        (void)d_tests_net_toy_queue(session,
                                    D_TESTS_NET_TOY_HELLO,
                                    D_TESTS_NET_TOY_STEP);
        session->state = 1;

        return D_SSL_ERROR_WANT_READ;
    }

    // the other side's step, whole
    if (session->in_size < D_TESTS_NET_TOY_STEP)
    {
        return D_SSL_ERROR_WANT_READ;
    }

    if (memcmp(session->in,
               (client) ? D_TESTS_NET_TOY_REPLY
               : D_TESTS_NET_TOY_HELLO,
               D_TESTS_NET_TOY_STEP) != 0)
    {
        return D_SSL_ERROR_HANDSHAKE;
    }

    d_tests_net_toy_consume(session,
                            D_TESTS_NET_TOY_STEP);

    // the server answers, and both are done
    if (!client)
    {
        (void)d_tests_net_toy_queue(session,
                                    D_TESTS_NET_TOY_REPLY,
                                    D_TESTS_NET_TOY_STEP);
    }

    session->state = 2;

    return D_SSL_OK;
}

/*
d_tests_net_toy_read
  Unseals what was fed, up to the toy close_notify, which ends the data;
or reports the failure the record asks for instead.
*/
static enum d_ssl_error
d_tests_net_toy_read(
    void*   _session,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_read
)
{
    struct d_tests_net_toy_session* const session = _session;
    unsigned char* const                  out     = _buffer;
    size_t                                used    = 0;
    size_t                                count   = 0;

    *_out_read = 0;

    // a record the test has the toy refuse
    if (session->toy->read_status != D_SSL_OK)
    {
        return session->toy->read_status;
    }

    // unseal until the close_notify, the end of what was fed, or no room
    while ( (!session->closed)        &&
            (used < session->in_size) &&
            (count < _capacity) )
    {
        const unsigned char byte = session->in[used++];

        if (byte == (unsigned char)D_TESTS_NET_TOY_CLOSE[0])
        {
            session->closed = true;
        }
        else
        {
            out[count++] = d_tests_net_toy_byte(byte);
        }
    }

    d_tests_net_toy_consume(session,
                            used);

    // data first; then the close, or a wait for more
    if (count > 0)
    {
        *_out_read = count;

        return D_SSL_OK;
    }

    return (session->closed) ? D_SSL_ERROR_CLOSED
                             : D_SSL_ERROR_WANT_READ;
}

/*
d_tests_net_toy_write
  Seals and queues as much as fits.
*/
static enum d_ssl_error
d_tests_net_toy_write(
    void*       _session,
    const void* _data,
    size_t      _size,
    size_t*     _out_written
)
{
    struct d_tests_net_toy_session* const session = _session;
    const unsigned char* const            data    = _data;
    const size_t                          room    =
        sizeof(session->out) - session->out_size;
    const size_t                          count   = (_size < room)
                                                             ? _size
                                                             : room;

    // the queue has room, as the kernel drains before every step
    if (count == 0)
    {
        return D_SSL_ERROR_BACKEND;
    }

    for (size_t i = 0; i < count; ++i)
    {
        session->out[session->out_size + i] =
            d_tests_net_toy_byte(data[i]);
    }

    session->out_size += count;
    *_out_written      = count;

    return D_SSL_OK;
}

/*
d_tests_net_toy_close
  Queues the toy close_notify, once, and counts it.
*/
static enum d_ssl_error
d_tests_net_toy_close(
    void* _session
)
{
    struct d_tests_net_toy_session* const session = _session;

    session->toy->shutdowns += 1;

    return (d_tests_net_toy_queue(session,
                                  D_TESTS_NET_TOY_CLOSE,
                                  1u)) ? D_SSL_OK
                                            : D_SSL_ERROR_BACKEND;
}

/*
d_tests_net_toy_describe
  The alerts the record names, at any point; then TLS 1.3 once done, a
client's chain as the record says, and a server's as unchecked.
*/
static void
d_tests_net_toy_describe(
    void*              _session,
    struct d_ssl_info* _out
)
{
    const struct d_tests_net_toy_session* const session = _session;

    if (session->toy->alerts)
    {
        _out->alert_sent     = session->toy->alert_sent;
        _out->alert_received = session->toy->alert_received;
    }

    if (session->state != 2)
    {
        return;
    }

    _out->version   = D_SSL_VERSION_TLS1_3;
    _out->cipher_id = 0x1301u;
    (void)snprintf(_out->cipher,
                   sizeof(_out->cipher),
                   "%s",
                   "TOY");
    _out->verify    = (session->toy->role == D_SSL_ROLE_CLIENT)
                          ? session->toy->verdict
                          : (uint32_t)D_SSL_VERIFY_FLAG_NOT_PERFORMED;

    return;
}

/*
d_tests_net_toy_peer_name
  A client's peer has one name: the record's, or the suite's server name.
*/
static bool
d_tests_net_toy_peer_name(
    void*                   _session,
    size_t                  _index,
    struct d_ssl_peer_name* _out
)
{
    const struct d_tests_net_toy_session* const session = _session;
    const char* const name = (session->toy->cert_name)
                                 ? session->toy->cert_name
                                 : D_TESTS_NET_TOY_SERVER_NAME;

    if ( (session->toy->role != D_SSL_ROLE_CLIENT) ||
         (session->state != 2)                     ||
         (_index > 0) )
    {
        return false;
    }

    _out->kind       = D_SSL_NAME_DNS;
    _out->value.data = name;
    _out->value.size = strlen(name);

    return true;
}

/*
d_tests_net_toy_peer_certificate
  A client's peer presents a token certificate, enough to fingerprint.
*/
static bool
d_tests_net_toy_peer_certificate(
    void*                _session,
    struct d_pack_bytes* _out_der
)
{
    static const unsigned char DER[] = { 0x30, 0x03, 0x02, 0x01, 0x01 };
    const struct d_tests_net_toy_session* const session = _session;

    if ( (session->toy->role != D_SSL_ROLE_CLIENT) ||
         (session->state != 2) )
    {
        return false;
    }

    _out_der->data = DER;
    _out_der->size = sizeof(DER);

    return true;
}

/*
TOY_ENGINE
  The toy engine's table: every operation, and the features a context for
either role needs.
*/
static const struct d_ssl_engine TOY_ENGINE =
{
    "toy",
    D_SSL_BACKEND_OTHER,
    (uint32_t)(D_SSL_ENGINE_TLS1_3       |
               D_SSL_ENGINE_SYSTEM_TRUST |
               D_SSL_ENGINE_MEMORY_PEM   |
               D_SSL_ENGINE_CLIENT_AUTH),
    d_tests_net_toy_context_create,
    d_tests_net_toy_context_destroy,
    d_tests_net_toy_session_create,
    d_tests_net_toy_session_destroy,
    d_tests_net_toy_feed,
    d_tests_net_toy_drain,
    d_tests_net_toy_handshake,
    d_tests_net_toy_read,
    d_tests_net_toy_write,
    d_tests_net_toy_close,
    d_tests_net_toy_describe,
    d_tests_net_toy_peer_name,
    d_tests_net_toy_peer_certificate,
    NULL
};

/*
d_tests_net_toy_context
  A server presents a token certificate and key in PEM, which the kernel
checks only for their shape, and verifying, asks for a client certificate
without requiring one; a client trusts the system store, which the toy
claims to have, and verifies or not as asked.
*/
bool
d_tests_net_toy_context(
    struct d_ssl_context*   _out,
    struct d_tests_net_toy* _toy,
    enum d_ssl_role         _role,
    bool                    _verify
)
{
    static const char CERT_PEM[] = "-----BEGIN CERTIFICATE-----\n"
                                   "MAMCAQE=\n"
                                   "-----END CERTIFICATE-----\n";
    static const char KEY_PEM[]  = "-----BEGIN PRIVATE KEY-----\n"
                                   "MAMCAQE=\n"
                                   "-----END PRIVATE KEY-----\n";
    struct d_ssl_config config   = { .verify_hostname = false };
    const bool          server   = (_role == D_SSL_ROLE_SERVER);

    d_ssl_config_init(&config,
                      _role);

    // a server's identity, and what it asks of clients; a client's
    // verification
    if (server)
    {
        config.certificate_pem.data   = CERT_PEM;
        config.certificate_pem.length = sizeof(CERT_PEM) - 1u;
        config.private_key_pem.data   = KEY_PEM;
        config.private_key_pem.length = sizeof(KEY_PEM) - 1u;
        config.verify                 = (_verify) ? D_SSL_VERIFY_OPTIONAL
                                                  : D_SSL_VERIFY_NONE;
    }
    else
    {
        config.verify          = (_verify) ? D_SSL_VERIFY_REQUIRED
                                           : D_SSL_VERIFY_NONE;
        config.verify_hostname = _verify;
    }

    d_tests_net_toy_making = _toy;

    const enum d_ssl_error made = d_ssl_context_init(_out,
                                                     &TOY_ENGINE,
                                                     &config);

    d_tests_net_toy_making = NULL;

    return (made == D_SSL_OK);
}
