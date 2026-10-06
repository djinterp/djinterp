/*******************************************************************************
* djinterp [net]                                          tcp_tests_sa_support.c
*
* Support for the TCP transport's standalone tests.
*   Endpoints, loopback pairs, and the few questions the tests put to the
* system directly: whether a descriptor is closed, whether a path exists.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests_sa_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
// enable POSIX.1-2008 (fcntl, nanosleep, getpid) under a strict C compiler;
// a feature-test macro must precede every include, so it sits ahead of the
// corresponding header
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif  // _POSIX_C_SOURCE

#include "./tcp_tests_sa.h"  // corresponding header
// std
#include <errno.h>     // errno, EBADF, EINTR
#include <stdio.h>     // snprintf
#include <string.h>    // strlen, memcmp
#include <time.h>      // nanosleep, struct timespec
// posix
#include <fcntl.h>     // fcntl, F_GETFD
#include <sys/stat.h>  // stat
#include <unistd.h>    // getpid


/*
d_tests_tcp_endpoint
  A TCP endpoint from a host and port; the host must fit a record.
*/
bool
d_tests_tcp_endpoint(
    struct d_net_endpoint* _out,
    const char*            _host,
    d_net_port             _port
)
{
    const struct d_pack_text host = { _host,
                                      strlen(_host) };

    return (d_net_endpoint_set(_out,
                               host,
                               _port,
                               D_NET_PROTOCOL_TCP) == D_NET_ERROR_NONE);
}

/*
d_tests_tcp_pair_open
  Listens on an ephemeral loopback port, connects, and accepts: the kernel
completes the handshake into the backlog, so one thread can do all three.
*/
bool
d_tests_tcp_pair_open(
    struct d_tests_tcp_pair*    _pair,
    const struct d_tcp_options* _options
)
{
    struct d_net_endpoint where = { .port = 0u };

    d_tcp_listener_init(&_pair->listener);
    d_tcp_connection_init(&_pair->client);
    d_tcp_connection_init(&_pair->server);

    const bool opened = ( (d_tests_tcp_endpoint(&where,
                                                "127.0.0.1",
                                                0u)) &&
                          (d_tcp_listen(&_pair->listener,
                                        &where,
                                        _options) == D_NET_ERROR_NONE) &&
                          (d_tcp_listener_local_endpoint(&_pair->listener,
                                                         &where)) &&
                          (d_tcp_connect(&_pair->client,
                                         &where,
                                         _options) == D_NET_ERROR_NONE) &&
                          (d_tcp_accept(&_pair->listener,
                                        &_pair->server) ==
                           D_NET_ERROR_NONE) );

    // a half-made pair is taken down whole
    if (!opened)
    {
        d_tests_tcp_pair_close(_pair);
    }

    return opened;
}

/*
d_tests_tcp_pair_close
  Closes all three; each close tolerates what is already closed.
*/
void
d_tests_tcp_pair_close(
    struct d_tests_tcp_pair* _pair
)
{
    d_tcp_connection_close(&_pair->client);
    d_tcp_connection_close(&_pair->server);
    d_tcp_listener_close(&_pair->listener);

    return;
}

/*
d_tests_tcp_exchange
  Writes a short text on one end and reads it whole on the other. The text
is far below any socket buffer, so one thread suffices.
*/
bool
d_tests_tcp_exchange(
    struct d_tcp_connection* _from,
    struct d_tcp_connection* _to,
    const char*              _text
)
{
    const size_t length   = strlen(_text);
    size_t       received = 0u;

    // the text's bytes, as they arrive
    char buffer[256] = { 0 };

    // only a text that fits the buffer can be compared
    if (length > sizeof(buffer))
    {
        return false;
    }

    const bool sent = (d_net_write_all(&_from->base,
                                       _text,
                                       length,
                                       NULL) == D_NET_ERROR_NONE);

    return ( (sent) &&
             (d_net_read_exactly(&_to->base,
                                 buffer,
                                 length,
                                 &received) == D_NET_ERROR_NONE) &&
             (received == length) &&
             (memcmp(buffer,
                     _text,
                     length) == 0) );
}

/*
d_tests_tcp_descriptor_closed
  A descriptor fcntl calls bad is closed.
*/
bool
d_tests_tcp_descriptor_closed(
    d_tcp_socket _socket
)
{
    return ( (fcntl((int)_socket,
                    F_GETFD) < 0) &&
             (errno == EBADF) );
}

/*
d_tests_tcp_unix_path
  A unix-domain path unique to this process and tag, under /tmp.
*/
bool
d_tests_tcp_unix_path(
    char*       _buffer,
    size_t      _capacity,
    const char* _tag
)
{
    const int written = snprintf(_buffer,
                                 _capacity,
                                 "/tmp/djinterp_tcp_%ld_%s.sock",
                                 (long)getpid(),
                                 _tag);

    return ( (written > 0) &&
             ((size_t)written < _capacity) );
}

/*
d_tests_tcp_path_exists
  Whether anything is at a path.
*/
bool
d_tests_tcp_path_exists(
    const char* _path
)
{
    struct stat status;

    return (stat(_path,
                 &status) == 0);
}

/*
d_tests_tcp_pause_ms
  Sleeps, resuming after signals, for at least the time given.
*/
void
d_tests_tcp_pause_ms(
    long _milliseconds
)
{
    struct timespec left = { .tv_sec  = _milliseconds / 1000L,
                             .tv_nsec = (_milliseconds % 1000L) * 1000000L };

    // a signal cuts the sleep short, leaving the rest in left
    while ( (nanosleep(&left,
                       &left) != 0) &&
            (errno == EINTR) )
    {
        continue;
    }

    return;
}
