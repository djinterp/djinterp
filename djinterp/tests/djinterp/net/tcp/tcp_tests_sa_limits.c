/*******************************************************************************
* djinterp [net]                                           tcp_tests_sa_limits.c
*
* Standalone tests of the TCP transport's limits and counts.
*   Each wait a caller can bound -- a read, a write, an accept -- is made to
* run out over loopback, and then shown to work again, and each limit is
* read back from the socket that holds it; the bytes a socket holds unread
* are counted; and the machine's name is read as the system gives it.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests_sa_limits.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./tcp_tests_sa.h"  // corresponding header
// std
#include <stdlib.h>  // free, labs, malloc
#include <string.h>  // memset, strcmp, strlen


/*
d_tests_tcp_read_expires
  A read with nothing to read, under a limit: TIMED_OUT, once `_after_ms`
have passed, and well before a second has.
*/
static bool
d_tests_tcp_read_expires(
    struct d_tcp_connection* _connection,
    long                     _after_ms
)
{
    char                         byte    = 0;
    const long                   started = d_tests_tcp_now_ms();
    const struct d_net_io_result result  = d_tcp_connection_read(_connection,
                                                                 &byte,
                                                                 1u);
    const long                   took    = d_tests_tcp_now_ms() - started;

    return ( (result.count == 0u)                    &&
             (result.error == D_NET_ERROR_TIMED_OUT) &&
             (took >= _after_ms)                     &&
             (took < 1000L) );
}

/*
d_tests_sa_tcp_read_timeout
  Tests the following:
  - connect and accept give both ends the options' I/O timeout, and a read
    with nothing to read expires
  - the connection works again afterwards
  - set_timeout lifts a limit, then sets one again; it refuses NULL and a
    closed connection
*/
bool
d_tests_sa_tcp_read_timeout(
    struct d_test_counter* _counter
)
{
    const char* const       TEST    = "I/O timeouts";
    struct d_tests_tcp_pair pair    = { .listener.socket = 0 };
    struct d_tcp_options    options = { .connect_timeout_ms = 0 };
    bool                    result  = true;

    d_tcp_options_init(&options);
    options.io_timeout_ms = 100;

    const bool opened = d_tests_tcp_pair_open(&pair,
                                              &options);
    const bool expired = ( (opened)                        &&
                           (pair.client.timeout_ms == 100) &&
                           (pair.server.timeout_ms == 100) &&
                           (d_tests_tcp_read_expires(&pair.client,
                                                     90L)) &&
                           (d_tests_tcp_read_expires(&pair.server,
                                                     90L)) );
    const bool again = ( (opened) &&
                         (d_tests_tcp_exchange(&pair.server,
                                               &pair.client,
                                               "still here")) );
    const bool reset = ( (opened)                                        &&
                         (d_tcp_connection_set_timeout(&pair.client,
                                                       0) == D_NET_OK)   &&
                         (pair.client.timeout_ms == 0)                   &&
                         (d_tcp_connection_set_timeout(&pair.client,
                                                       -5) == D_NET_OK)  &&
                         (pair.client.timeout_ms == 0)                   &&
                         (d_tcp_connection_set_timeout(&pair.client,
                                                       100) == D_NET_OK) &&
                         (d_tests_tcp_read_expires(&pair.client,
                                                   90L)) );

    d_tests_tcp_pair_close(&pair);

    const bool refused = ( (d_tcp_connection_set_timeout(NULL,
                                                         10) ==
                            D_NET_ERROR_INVALID_ARGUMENT) &&
                           (d_tcp_connection_set_timeout(&pair.client,
                                                         10) ==
                            D_NET_ERROR_CLOSED) );

    result = d_tests_net_assert(expired,
                                TEST,
                                "reads under a limit expire, on both ends",
                                _counter) && result;
    result = d_tests_net_assert(again,
                                TEST,
                                "an expired read leaves the connection usable",
                                _counter) && result;
    result = d_tests_net_assert(reset,
                                TEST,
                                "limits are lifted and set again",
                                _counter) && result;
    result = d_tests_net_assert(refused,
                                TEST,
                                "NULL and closed connections are refused",
                                _counter) && result;

    return result;
}

/*
d_tests_tcp_limit_held
  Whether a connection given the limit `_timeout_ms` holds it on its socket,
receiving and sending, to within the kernel's tick.
*/
static bool
d_tests_tcp_limit_held(
    struct d_tcp_connection* _connection,
    long                     _timeout_ms
)
{
    const d_tcp_socket socket = d_tcp_connection_native(_connection);

    return ( (d_tcp_connection_set_timeout(_connection,
                                           _timeout_ms) == D_NET_OK) &&
             (labs(d_tests_tcp_timeout_ms(socket,
                                          true) -
                   _timeout_ms) <= 10L)                              &&
             (labs(d_tests_tcp_timeout_ms(socket,
                                          false) -
                   _timeout_ms) <= 10L) );
}

/*
d_tests_sa_tcp_limit_values
  Tests the following:
  - the socket holds the limit asked for, receiving and sending, to within
    the kernel's tick: a whole second, then 1998 milliseconds
  - a millisecond is a limit like any other: the options give it to both
    ends at connect and accept, and a read with nothing to read expires
*/
bool
d_tests_sa_tcp_limit_values(
    struct d_test_counter* _counter
)
{
    const char* const       TEST    = "limit values";
    struct d_tests_tcp_pair pair    = { .listener.socket = 0 };
    struct d_tcp_options    options = { .connect_timeout_ms = 0 };
    bool                    result  = true;

    const bool held = ( (d_tests_tcp_pair_open(&pair,
                                               NULL))   &&
                        (d_tests_tcp_limit_held(&pair.client,
                                                1000L)) &&
                        (d_tests_tcp_limit_held(&pair.client,
                                                1998L)) );

    d_tests_tcp_pair_close(&pair);
    d_tcp_options_init(&options);
    options.io_timeout_ms = 1;

    const bool opened = d_tests_tcp_pair_open(&pair,
                                              &options);
    const bool tiny   = ( (opened)                      &&
                          (pair.client.timeout_ms == 1) &&
                          (pair.server.timeout_ms == 1) &&
                          (d_tests_tcp_read_expires(&pair.client,
                                                    0L)) );

    d_tests_tcp_pair_close(&pair);

    result = d_tests_net_assert(held,
                                TEST,
                                "the socket holds the limit asked for",
                                _counter) && result;
    result = d_tests_net_assert(tiny,
                                TEST,
                                "a millisecond is a limit",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_tcp_write_timeout
  Tests the following:
  - a peer that never reads fills the socket's buffers, and a write under a
    limit then expires rather than waiting
  - without a limit the same connection would have blocked, so the test
    bounds its own loop
*/
bool
d_tests_sa_tcp_write_timeout(
    struct d_test_counter* _counter
)
{
    static char             block[65536] = { 0 };
    struct d_tests_tcp_pair pair         = { .listener.socket = 0 };
    struct d_net_io_result  last         = { 0u, D_NET_OK };
    size_t                  sent         = 0u;

    memset(block,
           'w',
           sizeof(block));

    const bool opened = ( (d_tests_tcp_pair_open(&pair,
                                                 NULL)) &&
                          (d_tcp_connection_set_timeout(&pair.client,
                                                        100) == D_NET_OK) );

    // write until the buffers are full and the limit runs out, or 256 MiB
    while ( (opened)                 &&
            (last.error == D_NET_OK) &&
            (sent < 268435456u) )
    {
        last  = d_tcp_connection_write(&pair.client,
                                       block,
                                       sizeof(block));
        sent += last.count;
    }

    d_tests_tcp_pair_close(&pair);

    return d_tests_net_assert( ( (opened)                              &&
                                 (last.error == D_NET_ERROR_TIMED_OUT) &&
                                 (sent > 0u) ),
                               "I/O timeouts",
                               "a write the peer does not drain expires",
                               _counter);
}

/*
d_tests_sa_tcp_accept_wait
  Tests the following:
  - a limited wait with no one connecting expires, a zero wait at once
  - a connection waiting is accepted under a limit
  - NULL arguments, and a closed listener, are refused
*/
bool
d_tests_sa_tcp_accept_wait(
    struct d_test_counter* _counter
)
{
    const char* const       TEST       = "d_tcp_accept_wait";
    struct d_tcp_listener   listener   = { .socket = D_TCP_SOCKET_INVALID };
    struct d_tcp_connection client     = { .socket = D_TCP_SOCKET_INVALID };
    struct d_tcp_connection server     = { .socket = D_TCP_SOCKET_INVALID };
    struct d_net_endpoint   where      = { .port = 0u };
    bool                    result     = true;

    d_tcp_listener_init(&listener);

    const bool bound = ( (d_tests_tcp_endpoint(&where,
                                               "127.0.0.1",
                                               0u))       &&
                         (d_tcp_listen(&listener,
                                       &where,
                                       NULL) == D_NET_OK) &&
                         (d_tcp_listener_local_endpoint(&listener,
                                                        &where)) );
    const long started = d_tests_tcp_now_ms();
    const bool expired = ( (bound)                                           &&
                           (d_tcp_accept_wait(&listener,
                                              &server,
                                              100) == D_NET_ERROR_TIMED_OUT) &&
                           (d_tests_tcp_now_ms() - started >= 90L)           &&
                           (d_tcp_accept_wait(&listener,
                                              &server,
                                              0) == D_NET_ERROR_TIMED_OUT)   &&
                           (!d_tcp_connection_is_open(&server)) );
    const bool accepted = ( (bound)                               &&
                            (d_tcp_connect(&client,
                                           &where,
                                           NULL) == D_NET_OK)     &&
                            (d_tcp_accept_wait(&listener,
                                               &server,
                                               5000) == D_NET_OK) &&
                            (d_tests_tcp_exchange(&client,
                                                  &server,
                                                  "accepted")) );

    d_tcp_connection_close(&client);
    d_tcp_connection_close(&server);
    d_tcp_listener_close(&listener);

    const bool refused = ( (d_tcp_accept_wait(NULL,
                                              &server,
                                              10) ==
                            D_NET_ERROR_INVALID_ARGUMENT) &&
                           (d_tcp_accept_wait(&listener,
                                              NULL,
                                              10) ==
                            D_NET_ERROR_INVALID_ARGUMENT) &&
                           (d_tcp_accept_wait(&listener,
                                              &server,
                                              10) == D_NET_ERROR_CLOSED) );

    result = d_tests_net_assert(expired,
                                TEST,
                                "limited waits expire with no connection",
                                _counter) && result;
    result = d_tests_net_assert(accepted,
                                TEST,
                                "a waiting connection is accepted",
                                _counter) && result;
    result = d_tests_net_assert(refused,
                                TEST,
                                "NULLs and a closed listener are refused",
                                _counter) && result;

    return result;
}

/*
d_tests_tcp_await_count
  Asks for the count until the expected bytes have all arrived, or a second
has passed; loopback delivers at once, but not within the same call.
*/
static bool
d_tests_tcp_await_count(
    struct d_tcp_connection* _connection,
    size_t                   _expected
)
{
    size_t count = 0u;

    // ask again until the bytes are all there, for at most a second
    for (int tries = 0;
         tries < 100;
         ++tries)
    {
        // a refused count fails at once
        if (!d_net_connection_available(&_connection->base,
                                        &count))
        {
            return false;
        }

        // all of them
        if (count == _expected)
        {
            return true;
        }

        d_tests_tcp_pause_ms(10L);
    }

    return false;
}

/*
d_tests_sa_tcp_available
  Tests the following:
  - nothing waiting counts 0; bytes sent are counted on the other end,
    through net.h's view, and reading them takes the count back down, to
    the one byte left
  - NULL and closed connections cannot count, and leave the count 0
*/
bool
d_tests_sa_tcp_available(
    struct d_test_counter* _counter
)
{
    const char* const       TEST     = "d_tcp_connection_available";
    struct d_tests_tcp_pair pair     = { .listener.socket = 0 };
    char                    read[8]  = { 0 };
    size_t                  count    = 3u;
    bool                    result   = true;

    const bool opened = d_tests_tcp_pair_open(&pair,
                                              NULL);
    const bool counted = ( (opened)                               &&
                           (d_tcp_connection_available(&pair.server,
                                                       &count))   &&
                           (count == 0u)                          &&
                           (d_net_write_all(&pair.client.base,
                                            "12345",
                                            5u,
                                            NULL) == D_NET_OK)    &&
                           (d_tests_tcp_await_count(&pair.server,
                                                    5u))          &&
                           (d_net_read_exactly(&pair.server.base,
                                               read,
                                               2u,
                                               NULL) == D_NET_OK) &&
                           (d_tcp_connection_available(&pair.server,
                                                       &count))   &&
                           (count == 3u)                          &&
                           (d_net_read_exactly(&pair.server.base,
                                               read,
                                               2u,
                                               NULL) == D_NET_OK) &&
                           (d_tcp_connection_available(&pair.server,
                                                       &count))   &&
                           (count == 1u) );

    d_tests_tcp_pair_close(&pair);
    count = 3u;

    const bool refused = ( (!d_tcp_connection_available(&pair.server,
                                                        &count)) &&
                           (count == 0u)                         &&
                           (!d_tcp_connection_available(NULL,
                                                        &count)) &&
                           (!d_tcp_connection_available(&pair.server,
                                                        NULL)) );

    result = d_tests_net_assert(counted,
                                TEST,
                                "waiting bytes are counted, and taken down",
                                _counter) && result;
    result = d_tests_net_assert(refused,
                                TEST,
                                "NULL and closed connections cannot count",
                                _counter) && result;

    return result;
}

/*
d_tests_tcp_host_name_fits
  Whether the name fills a buffer of exactly its size, terminator and all,
and is refused by one a byte short. The buffer is the heap's, so the
sanitizer sees a byte written or read past it.
*/
static bool
d_tests_tcp_host_name_fits(
    const char* _system
)
{
    const size_t length = strlen(_system);
    char* const  exact  = (char*)malloc(length + 1u);
    bool         fits   = false;

    // without the memory there is nothing to show
    if (exact != NULL)
    {
        memset(exact,
               'x',
               length + 1u);
        fits = ( (d_tcp_host_name(exact,
                                  length + 1u) == D_NET_OK) &&
                 (strcmp(exact,
                         _system) == 0)                     &&
                 (d_tcp_host_name(exact,
                                  length) ==
                  D_NET_ERROR_MESSAGE_TOO_LARGE)            &&
                 (exact[0] == '\0') );
        free(exact);
    }

    return fits;
}

/*
d_tests_sa_tcp_host_name
  Tests the following:
  - the name is the system's own, as gethostname gives it
  - it fills a buffer of exactly its size, and a byte less is refused
  - a buffer too small for it is refused, holding an empty string
  - NULL and an empty buffer are refused
*/
bool
d_tests_sa_tcp_host_name(
    struct d_test_counter* _counter
)
{
    const char* const TEST        = "d_tcp_host_name";
    char              system[256] = { 0 };
    char              name[256]   = { 0 };
    char              small[2]    = { 'x', 'x' };
    bool              result      = true;

    (void)d_tests_tcp_system_name(system,
                                  sizeof(system));

    const bool named = ( (d_tcp_host_name(name,
                                          sizeof(name)) == D_NET_OK) &&
                         (strcmp(name,
                                 system) == 0) );
    const bool small_refused = ( (strlen(system) >= 2u)          &&
                                 (d_tcp_host_name(small,
                                                  sizeof(small)) ==
                                  D_NET_ERROR_MESSAGE_TOO_LARGE) &&
                                 (small[0] == '\0') );
    const bool refused = ( (d_tcp_host_name(NULL,
                                            8u) ==
                            D_NET_ERROR_INVALID_ARGUMENT) &&
                           (d_tcp_host_name(name,
                                            0u) ==
                            D_NET_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(named,
                                TEST,
                                "the name is the system's",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_tcp_host_name_fits(system),
                                TEST,
                                "a buffer of exactly its size holds it",
                                _counter) && result;
    result = d_tests_net_assert(small_refused,
                                TEST,
                                "a buffer too small holds an empty string",
                                _counter) && result;
    result = d_tests_net_assert(refused,
                                TEST,
                                "NULL and empty buffers are refused",
                                _counter) && result;

    return result;
}
