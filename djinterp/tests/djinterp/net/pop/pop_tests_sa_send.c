/*******************************************************************************
* djinterp [net]                                             pop_tests_sa_send.c
*
* Tests of how a connection's errors become the POP3 kernel's, and of the
* send helpers over a connection.
*   The helpers are tried over a memory stream, whose sink keeps what they
* wrote, and over a pipe, whose limits cut writes short and fill up; each way
* a write can fail arrives through d_pop_error_from_net.
*
*
* path:      /tests/djinterp/net/pop/pop_tests_sa_send.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./pop_tests_sa.h"  // the suite
// std
#include <string.h>  // memcmp


// d_tests_pop_from_net_case
//   struct: one row of the from_net table: a connection's error, and the
// module's error it becomes.
struct d_tests_pop_from_net_case
{
    enum d_net_error from;
    enum d_pop_error to;
};

/*
FROM_NET
  Every d_net_error, placed by net.h's rules the other way: the namesakes,
the three ways a connection ends, and TRANSPORT for the rest.
*/
static const struct d_tests_pop_from_net_case FROM_NET[] =
{
    { D_NET_OK,                        D_POP_OK                     },
    { D_NET_ERROR_CLOSED,              D_POP_ERROR_CLOSED           },
    { D_NET_ERROR_WOULD_BLOCK,         D_POP_ERROR_WOULD_BLOCK      },
    { D_NET_ERROR_TIMED_OUT,           D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_INTERRUPTED,         D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_CONNECTION_RESET,    D_POP_ERROR_CLOSED           },
    { D_NET_ERROR_CONNECTION_REFUSED,  D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_CONNECTION_ABORTED,  D_POP_ERROR_CLOSED           },
    { D_NET_ERROR_ADDRESS_IN_USE,      D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_ADDRESS_INVALID,     D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_HOST_UNREACHABLE,    D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_NETWORK_DOWN,        D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_ACCESS_DENIED,       D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_INVALID_ARGUMENT,    D_POP_ERROR_INVALID_ARGUMENT },
    { D_NET_ERROR_MESSAGE_TOO_LARGE,   D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_TOO_MANY_OPEN_FILES, D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_OUT_OF_MEMORY,       D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_UNKNOWN,             D_POP_ERROR_TRANSPORT        },
    { D_NET_ERROR_PROTOCOL,            D_POP_ERROR_TRANSPORT        }
};

/*
d_tests_pop_refuse
  A sink that takes nothing: a peer that stopped listening.
*/
static size_t
d_tests_pop_refuse(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;
    (void)_data;
    (void)_size;

    return 0u;
}

/*
d_tests_sa_pop_from_net
  Tests the following:
  - every d_net_error becomes the error net.h's rules give it
  - a value outside the enum is TRANSPORT
  - the module's own namesakes come back from d_pop_error_to_net unchanged
*/
bool
d_tests_sa_pop_from_net(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(FROM_NET) / sizeof(FROM_NET[0]);
    bool         table  = (count == (size_t)D_NET_ERROR_PROTOCOL + 1u);
    bool         result = true;

    // each row, in the enum's order
    for (size_t i = 0u; i < count; ++i)
    {
        table = ( (FROM_NET[i].from == (enum d_net_error)i) &&
                  (d_pop_error_from_net(FROM_NET[i].from) ==
                   FROM_NET[i].to)                          &&
                  (table) );
    }

    const bool outside =
        (d_pop_error_from_net((enum d_net_error)0x7FFF) ==
         D_POP_ERROR_TRANSPORT);
    const bool back =
        ( (d_pop_error_from_net(d_pop_error_to_net(D_POP_OK)) ==
           D_POP_OK)                &&
          (d_pop_error_from_net(d_pop_error_to_net(
                                    D_POP_ERROR_WOULD_BLOCK)) ==
           D_POP_ERROR_WOULD_BLOCK) &&
          (d_pop_error_from_net(d_pop_error_to_net(
                                    D_POP_ERROR_CLOSED)) ==
           D_POP_ERROR_CLOSED)      &&
          (d_pop_error_from_net(d_pop_error_to_net(
                                    D_POP_ERROR_INVALID_ARGUMENT)) ==
           D_POP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(table,
                                "d_pop_error_from_net",
                                "places every d_net_error by net.h's rules",
                                _counter) && result;
    result = d_tests_net_assert(outside,
                                "d_pop_error_from_net",
                                "a value outside the enum is TRANSPORT",
                                _counter) && result;
    result = d_tests_net_assert(back,
                                "d_pop_error_from_net",
                                "namesakes come back from d_pop_error_to_net",
                                _counter) && result;

    return result;
}

/*
d_tests_pop_send_pipe
  The pipe half of d_tests_sa_pop_send: a write limit of 3 bytes cuts every
write short, and the whole line still arrives at the other end.
*/
static bool
d_tests_pop_send_pipe(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe               pipe     = { .rings[0].size = 0u };
    unsigned char                   to_b[64] = { 0 };
    unsigned char                   to_a[64] = { 0 };
    char                            got[64]  = { 0 };
    size_t                          count    = 0u;
    const struct d_pop_command_line command  =
        d_tests_pop_command(D_POP_COMMAND_RETR,
                            "17");

    (void)d_net_pipe_init(&pipe,
                          to_b,
                          sizeof(to_b),
                          to_a,
                          sizeof(to_a));
    d_net_pipe_set_chunk(&pipe,
                         D_NET_PIPE_A,
                         3u);

    const bool sent =
        (d_pop_send_command(d_net_pipe_connection(&pipe,
                                                  D_NET_PIPE_A),
                            &command) == D_POP_OK);
    const bool arrived =
        ( (d_net_read_exactly(d_net_pipe_connection(&pipe,
                                                    D_NET_PIPE_B),
                              got,
                              9u,
                              &count) == D_NET_OK) &&
          (memcmp(got,
                  "RETR 17\r\n",
                  9u) == 0) );

    return d_tests_net_assert( ( (sent) &&
                                 (arrived) ),
                               "d_pop_send_command",
                               "writes the whole line 3 bytes at a time",
                               _counter);
}

/*
d_tests_sa_pop_send
  Tests the following:
  - a command and then responses, with and without text, reach the memory
    stream's sink exactly as the formatters write them
  - a line cut into short writes arrives whole (d_tests_pop_send_pipe)
*/
bool
d_tests_sa_pop_send(
    struct d_test_counter* _counter
)
{
    struct d_net_memory             memory  = { .input_size = 0u };
    struct d_tests_pop_capture      capture = { .length = 0u };
    const struct d_pop_command_line user    =
        d_tests_pop_command(D_POP_COMMAND_USER,
                            "alice");
    const struct d_pop_command_line quit    =
        d_tests_pop_command(D_POP_COMMAND_QUIT,
                            NULL);
    const struct d_pop_response     ready   =
        d_tests_pop_response(D_POP_INDICATOR_OK,
                             "ready");
    const struct d_pop_response     bare    =
        d_tests_pop_response(D_POP_INDICATOR_ERR,
                             "");
    bool                            result  = true;

    d_net_memory_init(&memory,
                      NULL,
                      0u,
                      d_tests_pop_capture_sink(&capture));

    const bool sent =
        ( (d_pop_send_command(&memory.base,
                              &user) == D_POP_OK)   &&
          (d_pop_send_response(&memory.base,
                               &ready) == D_POP_OK) &&
          (d_pop_send_command(&memory.base,
                              &quit) == D_POP_OK)   &&
          (d_pop_send_response(&memory.base,
                               &bare) == D_POP_OK) );

    result = d_tests_net_assert( ( (sent) &&
                                   (d_tests_pop_capture_is(
                                        &capture,
                                        "USER alice\r\n"
                                        "+OK ready\r\n"
                                        "QUIT\r\n"
                                        "-ERR\r\n")) ),
                                 "d_pop_send_command",
                                 "commands and responses go out exactly",
                                 _counter) && result;

    return ( (d_tests_pop_send_pipe(_counter)) &&
             (result) );
}

/*
d_tests_pop_send_full
  The pipe half of d_tests_sa_pop_send_failures: a ring with room for 4
bytes takes the start of a line, and then the send would block.
*/
static bool
d_tests_pop_send_full(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe               pipe    = { .rings[0].size = 0u };
    unsigned char                   to_b[4] = { 0 };
    unsigned char                   to_a[4] = { 0 };
    char                            got[4]  = { 0 };
    const struct d_pop_command_line user    =
        d_tests_pop_command(D_POP_COMMAND_USER,
                            "alice");

    (void)d_net_pipe_init(&pipe,
                          to_b,
                          sizeof(to_b),
                          to_a,
                          sizeof(to_a));

    const bool blocked =
        ( (d_pop_send_command(d_net_pipe_connection(&pipe,
                                                    D_NET_PIPE_A),
                              &user) == D_POP_ERROR_WOULD_BLOCK) &&
          (d_net_read_exactly(d_net_pipe_connection(&pipe,
                                                    D_NET_PIPE_B),
                              got,
                              4u,
                              NULL) == D_NET_OK)                 &&
          (memcmp(got,
                  "USER",
                  4u) == 0) );

    return d_tests_net_assert(blocked,
                              "d_pop_send_command",
                              "a full pipe blocks after part of the line",
                              _counter);
}

/*
d_tests_sa_pop_send_failures
  Tests the following:
  - a NULL connection is INVALID_ARGUMENT, but only after the line is
    checked: a malformed command is MALFORMED first
  - a sink that stops taking bytes is a connection reset, so CLOSED
  - a closed stream is CLOSED
  - a full pipe is WOULD_BLOCK (d_tests_pop_send_full)
*/
bool
d_tests_sa_pop_send_failures(
    struct d_test_counter* _counter
)
{
    struct d_net_memory             deaf      = { .input_size = 0u };
    struct d_net_memory             closed    = { .input_size = 0u };
    const struct d_pack_sink        refusing  = { d_tests_pop_refuse,
                                                  NULL };
    const struct d_pack_sink        discard   = { NULL,
                                                  NULL };
    const struct d_pop_command_line user      =
        d_tests_pop_command(D_POP_COMMAND_USER,
                            "alice");
    const struct d_pop_command_line malformed =
        d_tests_pop_command(D_POP_COMMAND_USER,
                            NULL);
    const struct d_pop_response     ready     =
        d_tests_pop_response(D_POP_INDICATOR_OK,
                             "ready");
    bool                            result    = true;

    d_net_memory_init(&deaf,
                      NULL,
                      0u,
                      refusing);
    d_net_memory_init(&closed,
                      NULL,
                      0u,
                      discard);
    d_net_connection_close(&closed.base);

    const bool refused =
        ( (d_pop_send_command(NULL,
                              &user) == D_POP_ERROR_INVALID_ARGUMENT)   &&
          (d_pop_send_response(NULL,
                               &ready) == D_POP_ERROR_INVALID_ARGUMENT) &&
          (d_pop_send_command(NULL,
                              &malformed) == D_POP_ERROR_MALFORMED) );
    const bool ended =
        ( (d_pop_send_command(&deaf.base,
                              &user) == D_POP_ERROR_CLOSED) &&
          (d_pop_send_response(&closed.base,
                               &ready) == D_POP_ERROR_CLOSED) );

    result = d_tests_net_assert(refused,
                                "d_pop_send_command",
                                "the line is checked before the connection",
                                _counter) && result;
    result = d_tests_net_assert(ended,
                                "d_pop_send_response",
                                "a deaf sink or a closed stream is CLOSED",
                                _counter) && result;

    return ( (d_tests_pop_send_full(_counter)) &&
             (result) );
}
