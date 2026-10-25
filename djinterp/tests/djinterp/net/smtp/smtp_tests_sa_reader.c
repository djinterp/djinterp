/*******************************************************************************
* djinterp [net]                                          smtp_tests_sa_reader.c
*
* djinterp SMTP kernel tests -- the line reader.
*   Standalone tests of smtp_reader.c over net_tests_peer.h's scripted peer:
* lines split over reads, CR LF and bare LF, the end of the stream, lines
* longer than the caller's buffer or the reader's own, what a failing
* connection leaves it holding and the cause it keeps, starting again, and
* the arguments it refuses.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_sa_reader.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./smtp_tests_sa.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdlib.h>   // free, malloc
#include <string.h>   // memcpy, memset, strcmp, strlen, strspn


/*
d_tests_smtp_line_is
  Reads one line, which must come back as `_status` holding `_expected`.
*/
static bool
d_tests_smtp_line_is(
    struct d_smtp_reader* _reader,
    enum d_smtp_error     _status,
    const char*           _expected
)
{
    char                    line[16] = { 0 };
    size_t                  length   = 99u;
    const enum d_smtp_error read     = d_smtp_reader_line(_reader,
                                                          line,
                                                          sizeof(line),
                                                          &length);

    return ( (read == _status)             &&
             (length == strlen(_expected)) &&
             (strcmp(line,
                     _expected) == 0) );
}

/*
d_tests_smtp_reader_ends
  Whether a reader at its peer's last line, unfinished, gives up what it
holds of it as CLOSED, then reads only CLOSED, with no cause; and whether,
started again over `_connection`, it has forgotten the last line's
terminator and holds nothing.
*/
static bool
d_tests_smtp_reader_ends(
    struct d_smtp_reader*    _reader,
    struct d_net_connection* _connection
)
{
    const bool ended = ( (d_tests_smtp_line_is(_reader,
                                               D_SMTP_ERROR_CLOSED,
                                               "partial")) &&
                         (d_tests_smtp_line_is(_reader,
                                               D_SMTP_ERROR_CLOSED,
                                               ""))        &&
                         (_reader->net_error == D_NET_OK) );

    d_smtp_reader_init(_reader,
                       _connection);

    return ( (ended)                          &&
             (!_reader->crlf)                 &&
             (_reader->net_error == D_NET_OK) &&
             (d_smtp_reader_pending(_reader) == 0u) );
}

/*
d_tests_sa_smtp_reader_lines
  Tests the following:
  - lines arriving a byte a read come back whole, terminators removed
  - CRLF and a bare LF both end a line, and `crlf` tells which
  - an empty line is a line
  - the end of the stream gives up the part of a line held, as CLOSED, and
    every read after it is CLOSED with nothing
  - a reader started again forgets the last line's terminator and holds
    nothing
*/
bool
d_tests_sa_smtp_reader_lines(
    struct d_test_counter* _counter
)
{
    const char* const        TEST   = "d_smtp_reader (lines)";
    struct d_tests_net_peer* peer   = d_tests_smtp_peer("220 a\r\n250-b\n"
                                                        "250 c\r\n\r\n"
                                                        "partial");
    struct d_smtp_reader     reader = { .connection = NULL };
    bool                     result = true;

    if (!d_tests_net_assert( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _counter))
    {
        return false;
    }

    peer->recv_chunk = 1u;
    d_smtp_reader_init(&reader,
                       d_tests_net_peer_connection(peer,
                                                   true));
    d_smtp_reader_init(NULL,
                       NULL);

    const bool crlf  = ( (d_tests_smtp_line_is(&reader,
                                               D_SMTP_OK,
                                               "220 a")) &&
                         (reader.crlf) );
    const bool bare  = ( (d_tests_smtp_line_is(&reader,
                                               D_SMTP_OK,
                                               "250-b")) &&
                         (!reader.crlf) );
    const bool empty = ( (d_tests_smtp_line_is(&reader,
                                               D_SMTP_OK,
                                               "250 c")) &&
                         (d_tests_smtp_line_is(&reader,
                                               D_SMTP_OK,
                                               ""))      &&
                         (reader.crlf) );

    result = d_tests_net_assert(crlf,
                                TEST,
                                "a line split over reads, ended by CRLF",
                                _counter) && result;
    result = d_tests_net_assert(bare,
                                TEST,
                                "a bare LF ends a line too, and is noted",
                                _counter) && result;
    result = d_tests_net_assert(empty,
                                TEST,
                                "an empty line is a line",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_smtp_reader_ends(&reader,
                                                         &peer->base),
                                TEST,
                                "the end, as CLOSED; started again, afresh",
                                _counter) && result;
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_smtp_edge
  A peer whose first turn is a line of `_count` 'a's and a CR, exactly the
reader's buffer, and whose second is the LF that ends it and one more line.
*/
static struct d_tests_net_peer*
d_tests_smtp_edge(
    size_t _count
)
{
    static unsigned char     bytes[D_SMTP_READER_BUFFER] = { 0 };
    struct d_tests_net_peer* peer = d_tests_net_peer_new();

    memset(bytes,
           'a',
           _count);
    bytes[_count] = '\r';

    // the CR fills the buffer; its LF comes with the next read
    if ( (peer) &&
         ( (!d_tests_net_peer_say_bytes(peer,
                                        "",
                                        bytes,
                                        _count + 1u)) ||
           (!d_tests_net_peer_say(peer,
                                  "",
                                  "\nnext\r\n")) ) )
    {
        d_tests_net_peer_free(peer);
        peer = NULL;
    }

    return peer;
}

/*
d_tests_smtp_edge_line
  Reads the edge peer's long line into `_capacity` bytes: its status, its
length, and whether the line after it is still found.
*/
static bool
d_tests_smtp_edge_line(
    size_t            _capacity,
    enum d_smtp_error _status,
    size_t            _length
)
{
    static char              line[D_SMTP_READER_BUFFER + 8u] = { 0 };
    struct d_tests_net_peer* peer   = d_tests_smtp_edge(
                                          D_SMTP_READER_BUFFER - 1u);
    struct d_smtp_reader     reader = { .connection = NULL };
    size_t                   length = 0;
    bool                     held   = false;

    if (peer)
    {
        d_smtp_reader_init(&reader,
                           d_tests_net_peer_connection(peer,
                                                       true));
        held = ( (d_smtp_reader_line(&reader,
                                     line,
                                     _capacity,
                                     &length) == _status) &&
                 (length == _length)                      &&
                 (line[length] == '\0')                   &&
                 (reader.crlf)                            &&
                 (d_tests_smtp_line_is(&reader,
                                       D_SMTP_OK,
                                       "next")) );
        d_tests_net_peer_free(peer);
    }

    return held;
}

/*
d_tests_smtp_spilled_line
  Reads a line of 6000 'b's -- said in two turns, the first filling the
reader's buffer -- into `_capacity` bytes of the heap, which hold no more:
its status, its length, whether what was kept is all 'b's, and whether the
line after it is still found.
*/
static bool
d_tests_smtp_spilled_line(
    size_t            _capacity,
    enum d_smtp_error _status,
    size_t            _length
)
{
    static char              bytes[D_SMTP_READER_BUFFER] = { 0 };
    static char              rest[1909u]                 = { 0 };
    struct d_tests_net_peer* peer   = d_tests_net_peer_new();
    char* const              line   = malloc(_capacity);
    struct d_smtp_reader     reader = { .connection = NULL };
    size_t                   length = 0;
    bool                     whole  = false;

    memset(bytes,
           'b',
           sizeof(bytes));
    memset(rest,
           'b',
           1904u);
    memcpy(rest + 1904u,
           "\r\nok",
           4u);

    // the buffer's worth, then the rest of the line and the next one
    if ( (peer)                                      &&
         (line)                                      &&
         (d_tests_net_peer_say_bytes(peer,
                                     "",
                                     bytes,
                                     sizeof(bytes))) &&
         (d_tests_net_peer_say(peer,
                               "",
                               rest))                &&
         (d_tests_net_peer_say(peer,
                               "",
                               "\r\n")) )
    {
        d_smtp_reader_init(&reader,
                           d_tests_net_peer_connection(peer,
                                                       true));
        whole = ( (d_smtp_reader_line(&reader,
                                      line,
                                      _capacity,
                                      &length) == _status) &&
                  (length == _length)                      &&
                  (strspn(line,
                          "b") == _length)                 &&
                  (line[_length] == '\0')                  &&
                  (d_tests_smtp_line_is(&reader,
                                        D_SMTP_OK,
                                        "ok")) );
    }

    free(line);
    d_tests_net_peer_free(peer);

    return whole;
}

/*
d_tests_sa_smtp_reader_limits
  Tests the following:
  - a line longer than the caller's buffer is LINE_TOO_LONG, holds its
    beginning, and is consumed whole, so the next line is found
  - a line the reader's own buffer cannot hold arrives in pieces: a CR at
    the buffer's end is held back, so a line that exactly fits the caller's
    buffer is not reported as too long, and one a byte longer is
  - the pieces of such a line are put together whole when the caller's
    buffer holds them, and up to its end when it does not
  - a bare CR inside a line is kept
*/
bool
d_tests_sa_smtp_reader_limits(
    struct d_test_counter* _counter
)
{
    const char* const        TEST       = "d_smtp_reader (limits)";
    struct d_tests_net_peer* peer       = d_tests_smtp_peer(
                                              "0123456789\r\nok\r\n"
                                              "a\rb\r\n");
    struct d_smtp_reader     reader     = { .connection = NULL };
    char                     line[8]    = { 0 };
    size_t                   length     = 0;
    bool                     long_lines = false;
    bool                     result     = true;

    if (peer)
    {
        d_smtp_reader_init(&reader,
                           d_tests_net_peer_connection(peer,
                                                       true));
        long_lines = ( (d_smtp_reader_line(&reader,
                                           line,
                                           sizeof(line),
                                           &length) ==
                        D_SMTP_ERROR_LINE_TOO_LONG) &&
                       (strcmp(line,
                               "0123456") == 0)     &&
                       (length == 7u)               &&
                       (d_tests_smtp_line_is(&reader,
                                             D_SMTP_OK,
                                             "ok")) &&
                       (d_tests_smtp_line_is(&reader,
                                             D_SMTP_OK,
                                             "a\rb")) );
        d_tests_net_peer_free(peer);
    }

    result = d_tests_net_assert(long_lines,
                                TEST,
                                "too long, its start kept, the stream in step",
                                _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_edge_line(D_SMTP_READER_BUFFER,
                                        D_SMTP_OK,
                                        D_SMTP_READER_BUFFER - 1u),
                 TEST,
                 "a CR at the buffer's end is not counted against the line",
                 _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_edge_line(D_SMTP_READER_BUFFER - 1u,
                                        D_SMTP_ERROR_LINE_TOO_LONG,
                                        D_SMTP_READER_BUFFER - 2u),
                 TEST,
                 "a byte longer than the caller's buffer is too long",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_smtp_spilled_line(8000u,
                                              D_SMTP_OK,
                                              6000u)) &&
                   (d_tests_smtp_spilled_line(5000u,
                                              D_SMTP_ERROR_LINE_TOO_LONG,
                                              4999u)) ),
                 TEST,
                 "a line in pieces: whole, or kept up to the buffer's end",
                 _counter) && result;

    return result;
}

/*
d_tests_smtp_reader_refusals
  Every argument the reader refuses, and the queries on NULL. A reader
without a connection refuses to read, and keeps no cause: nothing failed
beneath it.
*/
static bool
d_tests_smtp_reader_refusals(
    struct d_smtp_reader* _reader
)
{
    struct d_smtp_reader unbound = { .connection = NULL };
    char                 line[8] = { 0 };
    size_t               length  = 0;

    d_smtp_reader_init(&unbound,
                       NULL);
    d_smtp_reader_discard(NULL);

    return ( (d_smtp_reader_line(NULL,
                                 line,
                                 sizeof(line),
                                 &length) == D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (d_smtp_reader_line(&unbound,
                                 line,
                                 sizeof(line),
                                 &length) == D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (unbound.net_error == D_NET_OK)                                &&
             (d_smtp_reader_line(_reader,
                                 NULL,
                                 sizeof(line),
                                 &length) == D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (d_smtp_reader_line(_reader,
                                 line,
                                 0u,
                                 &length) == D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (d_smtp_reader_line(_reader,
                                 line,
                                 sizeof(line),
                                 NULL) == D_SMTP_ERROR_INVALID_ARGUMENT)    &&
             (d_smtp_reader_pending(NULL) == 0u) );
}

/*
d_tests_smtp_reader_cause
  A reader over a peer that says `_script` and then stalls -- or, with
`_lies`, claims more than each read had room for -- must fail its first
read with `_status`, giving up `_held` and holding nothing more, and keep
`_cause` until it starts again.
*/
static bool
d_tests_smtp_reader_cause(
    const char*       _script,
    bool              _lies,
    enum d_smtp_error _status,
    const char*       _held,
    enum d_net_error  _cause
)
{
    struct d_tests_net_peer* peer   = d_tests_net_peer_new();
    struct d_smtp_reader     reader = { .connection = NULL };
    bool                     failed = false;

    if ( (peer) &&
         (d_tests_net_peer_say(peer,
                               "",
                               _script)) )
    {
        peer->lies = _lies;
        d_smtp_reader_init(&reader,
                           d_tests_net_peer_connection(peer,
                                                       true));
        failed = ( (d_tests_smtp_line_is(&reader,
                                         _status,
                                         _held))          &&
                   (d_smtp_reader_pending(&reader) == 0u) &&
                   (reader.net_error == _cause) );
        d_smtp_reader_init(&reader,
                           &peer->base);
        failed = ( (failed) &&
                   (reader.net_error == D_NET_OK) );
    }

    d_tests_net_peer_free(peer);

    return failed;
}

/*
d_tests_sa_smtp_reader_failures
  Tests the following:
  - a peer that stalls mid-line times out: TIMED_OUT, the part of the line
    held given up -- a CR that may have begun its CR LF dropped -- and the
    cause kept
  - a read claiming more than it was given room for is TRANSPORT, net.h
    having made it UNKNOWN
  - pending counts what was read but not consumed, and discard drops it
  - every argument the reader refuses
*/
bool
d_tests_sa_smtp_reader_failures(
    struct d_test_counter* _counter
)
{
    const char* const        TEST   = "d_smtp_reader (failures)";
    struct d_tests_net_peer* peer   = d_tests_smtp_peer("a\r\nb\r\n");
    struct d_smtp_reader     reader = { .connection = NULL };
    bool                     kept   = false;
    bool                     result = true;

    if (peer)
    {
        d_smtp_reader_init(&reader,
                           d_tests_net_peer_connection(peer,
                                                       true));
        kept = ( (d_tests_smtp_line_is(&reader,
                                       D_SMTP_OK,
                                       "a"))             &&
                 (d_smtp_reader_pending(&reader) == 3u)  &&
                 (d_tests_smtp_reader_refusals(&reader)) );
        d_smtp_reader_discard(&reader);
        kept = ( (kept)                                 &&
                 (d_smtp_reader_pending(&reader) == 0u) &&
                 (d_tests_smtp_line_is(&reader,
                                       D_SMTP_ERROR_CLOSED,
                                       "")) );
        d_tests_net_peer_free(peer);
    }

    result = d_tests_net_assert(
                 d_tests_smtp_reader_cause("250-par\r",
                                           false,
                                           D_SMTP_ERROR_TIMED_OUT,
                                           "250-par",
                                           D_NET_ERROR_TIMED_OUT),
                 TEST,
                 "a stall times out, the line's start given up",
                 _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_reader_cause("250 x\r\n",
                                           true,
                                           D_SMTP_ERROR_TRANSPORT,
                                           "",
                                           D_NET_ERROR_UNKNOWN),
                 TEST,
                 "a read that claims too much is TRANSPORT",
                 _counter) && result;
    result = d_tests_net_assert(kept,
                                TEST,
                                "pending, discard, and the refusals",
                                _counter) && result;

    return result;
}
