/*******************************************************************************
* djinterp [c][net]                                            curl_tests_peer.c
*
* The loopback peer of the libcurl binding's suite: HTTP/1.1 always, and
* HTTP/2 where it is made to speak it.
*   It reads what arrives into one buffer and decides by the first bytes:
* HTTP/2's 24-byte preface is prior knowledge, anything else an HTTP/1
* request, read to its blank line. An HTTP/1 request asking to upgrade to
* h2c gets 101 from a peer that speaks HTTP/2, which then answers stream 1
* in frames once the client's preface arrives (RFC 7540, section 3.2; RFC
* 9113 dropped the Upgrade, and libcurl keeps it). A request that arrives
* as frames - prior knowledge - is answered on its own stream once its
* header block ends. The answer is a 200 with one field, x-peer, and an
* eight-byte body, as HTTP/2 or HTTP/1 by how it was asked. The peer reads
* every frame's head, acknowledges SETTINGS and PING, and decodes no header
* block: its own is HPACK's indexed :status 200 and a literal field never
* indexed, with no Huffman coding (RFC 7541, sections 6.1 and 6.2.3).
*
*
* path:      /tests/djinterp/c/modules/net/curl/curl_tests_peer.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/
// POSIX.1-2008 under a strict C compiler, for the threads dmutex.h declares;
// a feature-test macro must precede every include, so it sits ahead of the
// suite's header. Windows needs none.
#if !defined(_WIN32) && !defined(_WIN64)
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif  // _POSIX_C_SOURCE
#endif

#include "./curl_tests.h"  // the suite
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memchr, memcmp, memcpy, memmove, memset, strlen
#include <ctype.h>   // tolower
// djinterp
#include "../../../../../../inc/djinterp/c/sync/dmutex.h"  // threads


// D_TESTS_CURL_PEER_BYTES
//   constant: the peer's read buffer: a request head, or a frame and the
// next one's head. libcurl's frames stay far below HTTP/2's default 16,384.
#define D_TESTS_CURL_PEER_BYTES 32768u

// D_TESTS_CURL_PEER_WAIT_MS
//   constant: how long the peer waits for a connection, and for each read
// and write, so a test whose transfer never comes still ends.
#define D_TESTS_CURL_PEER_WAIT_MS 10000L

// HTTP/2's frame types and flags, those the peer reads or writes
// (RFC 9113, section 6)
#define D_TESTS_H2_DATA          0x0u
#define D_TESTS_H2_HEADERS       0x1u
#define D_TESTS_H2_SETTINGS      0x4u
#define D_TESTS_H2_PING          0x6u
#define D_TESTS_H2_GOAWAY        0x7u
#define D_TESTS_H2_CONTINUATION  0x9u
#define D_TESTS_H2_END_STREAM    0x1u
#define D_TESTS_H2_ACK           0x1u
#define D_TESTS_H2_END_HEADERS   0x4u

// D_TESTS_H2_PREFACE
//   constant: the client's connection preface (RFC 9113, section 3.4).
static const char D_TESTS_H2_PREFACE[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
#define D_TESTS_H2_PREFACE_SIZE 24u

// D_TESTS_CURL_BODY
//   constant: every answer's body; the HTTP/1 answer says "h1" instead.
#define D_TESTS_CURL_BODY_H2 "hello h2"
#define D_TESTS_CURL_BODY_H1 "hello h1"

// g_peer_thread
//   variable: the running peer's thread; one peer runs at a time.
static dj_thread_t g_peer_thread;

// dj_tests_curl_conn
//   struct: the accepted connection and what has arrived on it, not yet
// consumed: bytes [start, end) of data.
struct dj_tests_curl_conn
{
    struct dj_tcp_connection tcp;
    unsigned char            data[D_TESTS_CURL_PEER_BYTES];
    size_t                   start;
    size_t                   end;
};


/*
d_tests_curl_fill
  Makes at least _count bytes available, reading as needed; the bytes kept
move to the front first, so the buffer never runs out while there is room.
Returns false at the end of the stream, on an error, or for a count larger
than the buffer.
*/
static
bool d_tests_curl_fill(
    struct dj_tests_curl_conn* _conn,
    size_t                     _count
)
{
    // more than the buffer could ever hold
    if (_count > D_TESTS_CURL_PEER_BYTES)
    {
        return false;
    }

    // the kept bytes to the front
    if (_conn->start > 0u)
    {
        memmove(_conn->data,
                _conn->data + _conn->start,
                _conn->end - _conn->start);
        _conn->end  -= _conn->start;
        _conn->start = 0u;
    }

    // read until enough are here
    while ((_conn->end - _conn->start) < _count)
    {
        const struct dj_net_io_result got =
            d_tcp_connection_read(&_conn->tcp,
                                  _conn->data + _conn->end,
                                  D_TESTS_CURL_PEER_BYTES - _conn->end);

        if ( (got.error != D_NET_OK) ||
             (got.count == 0u) )
        {
            return false;
        }

        _conn->end += got.count;
    }

    return true;
}

/*
d_tests_curl_send
  Writes every byte, or reports false.
*/
static
bool d_tests_curl_send(
    struct dj_tests_curl_conn* _conn,
    const void*                _data,
    size_t                     _size
)
{
    size_t written = 0u;

    return (d_net_write_all(&_conn->tcp.base,
                            _data,
                            _size,
                            &written) == D_NET_OK) &&
           (written == _size);
}

/*
d_tests_curl_frame
  Writes one HTTP/2 frame: the nine-byte head, then the payload.
*/
static
bool d_tests_curl_frame(
    struct dj_tests_curl_conn* _conn,
    unsigned int               _type,
    unsigned int               _flags,
    unsigned long              _stream,
    const void*                _payload,
    size_t                     _size
)
{
    unsigned char head[9];

    head[0] = (unsigned char)((_size >> 16) & 0xFFu);
    head[1] = (unsigned char)((_size >> 8) & 0xFFu);
    head[2] = (unsigned char)(_size & 0xFFu);
    head[3] = (unsigned char)_type;
    head[4] = (unsigned char)_flags;
    head[5] = (unsigned char)((_stream >> 24) & 0x7Fu);
    head[6] = (unsigned char)((_stream >> 16) & 0xFFu);
    head[7] = (unsigned char)((_stream >> 8) & 0xFFu);
    head[8] = (unsigned char)(_stream & 0xFFu);

    return (d_tests_curl_send(_conn,
                              head,
                              sizeof(head))) &&
           ( (_size == 0u) ||
             (d_tests_curl_send(_conn,
                                _payload,
                                _size)) );
}

/*
d_tests_curl_answer_h2
  Answers a stream: a header block of :status 200 (static table index 8,
indexed) and x-peer: h2 (a literal never indexed, its name new), then the
body, which ends the stream.
*/
static
bool d_tests_curl_answer_h2(
    struct dj_tests_curl_conn* _conn,
    unsigned long              _stream
)
{
    static const unsigned char block[] =
    {
        0x88u,                                  // :status 200
        0x10u,                                  // never indexed, new name
        0x06u, 'x', '-', 'p', 'e', 'e', 'r',    // the name, no Huffman
        0x02u, 'h', '2'                         // the value
    };

    return (d_tests_curl_frame(_conn,
                               D_TESTS_H2_HEADERS,
                               D_TESTS_H2_END_HEADERS,
                               _stream,
                               block,
                               sizeof(block))) &&
           (d_tests_curl_frame(_conn,
                               D_TESTS_H2_DATA,
                               D_TESTS_H2_END_STREAM,
                               _stream,
                               D_TESTS_CURL_BODY_H2,
                               strlen(D_TESTS_CURL_BODY_H2)));
}

/*
d_tests_curl_serve_h2
  Speaks HTTP/2 once the client's preface has been consumed: SETTINGS first,
as the server's preface, then frame by frame until the client goes. An
upgraded connection's stream 1 is answered at once; any other stream when
its header block ends. Returns D_NET_OK when the client closed or sent
GOAWAY after the answer.
*/
static
enum dj_net_error d_tests_curl_serve_h2(
    struct dj_tests_curl_peer* _peer,
    struct dj_tests_curl_conn* _conn,
    bool                       _upgraded
)
{
    bool answered = false;

    // the server's preface, and an upgraded request's answer
    if ( (!d_tests_curl_frame(_conn,
                              D_TESTS_H2_SETTINGS,
                              0u,
                              0u,
                              NULL,
                              0u)) ||
         ( (_upgraded) &&
           (!d_tests_curl_answer_h2(_conn,
                                    1u)) ) )
    {
        return D_NET_ERROR_UNKNOWN;
    }

    answered            = _upgraded;
    _peer->answered_h2  = _upgraded;

    // frame by frame until the client goes
    while (d_tests_curl_fill(_conn,
                             9u))
    {
        const unsigned char* const head   = _conn->data + _conn->start;
        const size_t               size   = ((size_t)head[0] << 16) |
                                            ((size_t)head[1] << 8)  |
                                            (size_t)head[2];
        const unsigned int         type   = head[3];
        const unsigned int         flags  = head[4];
        const unsigned long        stream =
            ((unsigned long)(head[5] & 0x7Fu) << 24) |
            ((unsigned long)head[6] << 16)          |
            ((unsigned long)head[7] << 8)           |
            (unsigned long)head[8];
        unsigned char              ping[8];

        // the payload, which only PING's echo needs
        if (!d_tests_curl_fill(_conn,
                               9u + size))
        {
            return D_NET_ERROR_PROTOCOL;
        }

        if ( (type == D_TESTS_H2_PING) &&
             (size == 8u) )
        {
            memcpy(ping,
                   _conn->data + _conn->start + 9u,
                   8u);
        }

        _conn->start += 9u + size;

        // the frames the peer answers
        if ( (type == D_TESTS_H2_SETTINGS) &&
             ((flags & D_TESTS_H2_ACK) == 0u) )
        {
            if (!d_tests_curl_frame(_conn,
                                    D_TESTS_H2_SETTINGS,
                                    D_TESTS_H2_ACK,
                                    0u,
                                    NULL,
                                    0u))
            {
                return D_NET_ERROR_UNKNOWN;
            }
        }
        else if ( (type == D_TESTS_H2_PING) &&
                  ((flags & D_TESTS_H2_ACK) == 0u) &&
                  (size == 8u) )
        {
            if (!d_tests_curl_frame(_conn,
                                    D_TESTS_H2_PING,
                                    D_TESTS_H2_ACK,
                                    0u,
                                    ping,
                                    8u))
            {
                return D_NET_ERROR_UNKNOWN;
            }
        }
        else if ( ( (type == D_TESTS_H2_HEADERS) ||
                    (type == D_TESTS_H2_CONTINUATION) ) &&
                  ((flags & D_TESTS_H2_END_HEADERS) != 0u) &&
                  (stream != 0u) &&
                  (!answered) )
        {
            if (!d_tests_curl_answer_h2(_conn,
                                        stream))
            {
                return D_NET_ERROR_UNKNOWN;
            }

            answered           = true;
            _peer->answered_h2 = true;
        }
        else if (type == D_TESTS_H2_GOAWAY)
        {
            break;
        }
    }

    return (answered) ? D_NET_OK : D_NET_ERROR_PROTOCOL;
}

/*
d_tests_curl_head_end
  Where an HTTP/1 request head ends - the offset past its blank line - in
the bytes kept, or 0 if it hasn't ended yet.
*/
static
size_t d_tests_curl_head_end(
    const struct dj_tests_curl_conn* _conn
)
{
    const unsigned char* const at   = _conn->data + _conn->start;
    const size_t               size = _conn->end - _conn->start;

    // the first CR LF CR LF
    for (size_t i = 0u; (i + 4u) <= size; ++i)
    {
        if (memcmp(at + i,
                   "\r\n\r\n",
                   4u) == 0)
        {
            return i + 4u;
        }
    }

    return 0u;
}

/*
d_tests_curl_asks_h2c
  Whether a request head asks to upgrade to h2c: a line "Upgrade: h2c",
the name in any case, as libcurl sends it.
*/
static
bool d_tests_curl_asks_h2c(
    const unsigned char* _head,
    size_t               _size
)
{
    static const char wanted[] = "\r\nupgrade: h2c\r\n";
    const size_t      length   = sizeof(wanted) - 1u;

    // each place the line could begin
    for (size_t i = 0u; (i + length) <= _size; ++i)
    {
        size_t j = 0u;

        while ( (j < length) &&
                (tolower(_head[i + j]) == wanted[j]) )
        {
            ++j;
        }

        if (j == length)
        {
            return true;
        }
    }

    return false;
}

/*
d_tests_curl_serve_h1
  Answers an HTTP/1 request whose head has arrived: 101 and HTTP/2 for an
Upgrade to h2c, where the peer speaks it; otherwise a 200 in the request's
own version, the connection closed after it. A request that isn't HTTP/1 -
HTTP/2's preface sent to a peer that only speaks HTTP/1 - gets a 400, as
such a server answers.
*/
static
enum dj_net_error d_tests_curl_serve_h1(
    struct dj_tests_curl_peer* _peer,
    struct dj_tests_curl_conn* _conn,
    size_t                     _head
)
{
    static const char switching[] =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: h2c\r\n\r\n";
    static const char refusal[] =
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n";
    const unsigned char* const at    = _conn->data + _conn->start;
    const unsigned char* const space = (const unsigned char*)memchr(at,
                                                                    ' ',
                                                                    _head);
    const unsigned char* const line  = (const unsigned char*)memchr(at,
                                                                    '\r',
                                                                    _head);
    char                       answer[160];

    // the request line's version: its last eight bytes
    if ( (space != NULL) &&
         (line != NULL)  &&
         ((size_t)(line - at) >= 8u) &&
         (memcmp(line - 8,
                 "HTTP/1.",
                 7u) == 0) )
    {
        memcpy(_peer->version,
               line - 8,
               8u);
        _peer->version[8] = '\0';
    }

    _peer->saw_upgrade = d_tests_curl_asks_h2c(at,
                                               _head);
    _conn->start      += _head;

    // not HTTP/1 at all
    if (_peer->version[0] == '\0')
    {
        return (d_tests_curl_send(_conn,
                                  refusal,
                                  sizeof(refusal) - 1u))
                   ? D_NET_OK
                   : D_NET_ERROR_UNKNOWN;
    }

    // the upgrade, taken: 101, the client's preface, then HTTP/2
    if ( (_peer->saw_upgrade) &&
         (_peer->speaks_h2) )
    {
        if ( (!d_tests_curl_send(_conn,
                                 switching,
                                 sizeof(switching) - 1u)) ||
             (!d_tests_curl_fill(_conn,
                                 D_TESTS_H2_PREFACE_SIZE)) ||
             (memcmp(_conn->data + _conn->start,
                     D_TESTS_H2_PREFACE,
                     D_TESTS_H2_PREFACE_SIZE) != 0) )
        {
            return D_NET_ERROR_PROTOCOL;
        }

        _conn->start += D_TESTS_H2_PREFACE_SIZE;

        return d_tests_curl_serve_h2(_peer,
                                     _conn,
                                     true);
    }

    // HTTP/1, in the request's version
    (void)snprintf(answer,
                   sizeof(answer),
                   "%s 200 OK\r\n"
                   "Content-Length: 8\r\n"
                   "x-peer: h1\r\n"
                   "Connection: close\r\n\r\n"
                   D_TESTS_CURL_BODY_H1,
                   _peer->version);

    return (d_tests_curl_send(_conn,
                              answer,
                              strlen(answer)))
               ? D_NET_OK
               : D_NET_ERROR_UNKNOWN;
}

/*
d_tests_curl_serve
  Takes the first bytes and decides: HTTP/2's preface to a peer that speaks
it, or an HTTP/1 request read to its blank line.
*/
static
enum dj_net_error d_tests_curl_serve(
    struct dj_tests_curl_peer* _peer,
    struct dj_tests_curl_conn* _conn
)
{
    size_t head = 0u;

    // enough to tell the preface from a request line
    if (!d_tests_curl_fill(_conn,
                           D_TESTS_H2_PREFACE_SIZE))
    {
        return D_NET_ERROR_PROTOCOL;
    }

    _peer->saw_preface = (memcmp(_conn->data + _conn->start,
                                 D_TESTS_H2_PREFACE,
                                 D_TESTS_H2_PREFACE_SIZE) == 0);

    // HTTP/2 from the first byte
    if ( (_peer->saw_preface) &&
         (_peer->speaks_h2) )
    {
        _conn->start += D_TESTS_H2_PREFACE_SIZE;

        return d_tests_curl_serve_h2(_peer,
                                     _conn,
                                     false);
    }

    // an HTTP/1 request head, to its blank line
    while ((head = d_tests_curl_head_end(_conn)) == 0u)
    {
        if (!d_tests_curl_fill(_conn,
                               (_conn->end - _conn->start) + 1u))
        {
            return D_NET_ERROR_PROTOCOL;
        }
    }

    return d_tests_curl_serve_h1(_peer,
                                 _conn,
                                 head);
}

/*
d_tests_curl_peer_main
  The peer's thread: one connection, answered, then read until the client
closes it, so the client never sees a reset. The connection's state is
static, its buffer too big for a thread's stack; one peer runs at a time.
*/
static
dj_thread_result_t d_tests_curl_peer_main(
    void* _argument
)
{
    struct dj_tests_curl_peer* const peer =
        (struct dj_tests_curl_peer*)_argument;
    static struct dj_tests_curl_conn conn;
    unsigned char                    rest[256];

    memset(&conn,
           0,
           sizeof(conn));
    d_tcp_connection_init(&conn.tcp);
    peer->outcome = d_tcp_accept_wait(&peer->listener,
                                      &conn.tcp,
                                      D_TESTS_CURL_PEER_WAIT_MS);

    // the connection, answered; an HTTP/1 answer closes it, as its
    // Connection: close says, so the client sees the end at once
    if (peer->outcome == D_NET_OK)
    {
        peer->outcome = d_tests_curl_serve(peer,
                                           &conn);

        if (!peer->answered_h2)
        {
            (void)d_tcp_connection_shutdown(&conn.tcp,
                                            D_NET_SHUTDOWN_WRITE);
        }
    }

    // whatever the client sends until it goes
    while (d_tcp_connection_is_open(&conn.tcp))
    {
        const struct dj_net_io_result got =
            d_tcp_connection_read(&conn.tcp,
                                  rest,
                                  sizeof(rest));

        if ( (got.error != D_NET_OK) ||
             (got.count == 0u) )
        {
            break;
        }
    }

    d_tcp_connection_close(&conn.tcp);

    return D_THREAD_SUCCESS;
}

/*
d_tests_curl_peer_start
  Listens on 127.0.0.1's ephemeral port, with the peer's limit on each read
and write, and starts the thread.
*/
bool d_tests_curl_peer_start(
    struct dj_tests_curl_peer* _peer,
    bool                       _speaks_h2
)
{
    static const char          loopback[] = "127.0.0.1";
    const struct dj_pack_text  host       = { loopback,
                                              sizeof(loopback) - 1u };
    struct dj_net_endpoint     where;
    struct dj_net_endpoint     bound;
    struct dj_tcp_options      options;

    memset(_peer,
           0,
           sizeof(*_peer));
    d_tcp_listener_init(&_peer->listener);
    d_tcp_options_init(&options);
    options.io_timeout_ms = D_TESTS_CURL_PEER_WAIT_MS;
    _peer->speaks_h2      = _speaks_h2;
    _peer->outcome        = D_NET_ERROR_UNKNOWN;

    // the listener, and the port it got
    if ( (d_net_endpoint_set(&where,
                             host,
                             0u,
                             D_NET_PROTOCOL_TCP) != D_NET_OK)       ||
         (d_tcp_listen(&_peer->listener,
                       &where,
                       &options) != D_NET_OK)                       ||
         (!d_tcp_listener_local_endpoint(&_peer->listener,
                                         &bound)) )
    {
        d_tcp_listener_close(&_peer->listener);

        return false;
    }

    (void)snprintf(_peer->url,
                   sizeof(_peer->url),
                   "http://127.0.0.1:%u/hello",
                   (unsigned int)bound.port);

    // the thread
    if (d_thread_create(&g_peer_thread,
                        d_tests_curl_peer_main,
                        _peer) != 0)
    {
        d_tcp_listener_close(&_peer->listener);

        return false;
    }

    _peer->running = true;

    return true;
}

/*
d_tests_curl_peer_finish
  Joins the thread, then closes the listener; closing first would cut short
an accept still waiting for a client that never came, which the wait's
limit ends anyway.
*/
void d_tests_curl_peer_finish(
    struct dj_tests_curl_peer* _peer
)
{
    // a peer that never started has no thread
    if (_peer->running)
    {
        (void)d_thread_join(g_peer_thread,
                            NULL);
        _peer->running = false;
    }

    d_tcp_listener_close(&_peer->listener);

    return;
}
