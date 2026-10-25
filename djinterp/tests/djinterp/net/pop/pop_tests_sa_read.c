/*******************************************************************************
* djinterp [net]                                             pop_tests_sa_read.c
*
* Tests of the POP3 kernel's reader over a net.h connection, and of a client
* and a server talking through it.
*   Lines and bodies arrive in pieces from a memory stream, stall and resume
* on a pipe, and fail on cue from a script; each failure is checked for the
* error it becomes here and the cause the reader keeps.
*
*
* path:      /tests/djinterp/net/pop/pop_tests_sa_read.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./pop_tests_sa.h"  // the suite
// std
#include <string.h>  // memcpy, memset, strlen


// CONVERSATION
//   constant: three status lines, a body with a stuffed line, and a status
// line pipelined after it.
static const char CONVERSATION[] =
    "+OK ready\r\n"
    "-ERR [IN-USE] busy\r\n"
    "+OK 2 lines\r\n"
    "one\r\n"
    "..dot\r\n"
    ".\r\n"
    "+OK next\r\n";

/*
d_tests_pop_line_is
  Reads one line and checks its status, and its text when it has one.
*/
static bool
d_tests_pop_line_is(
    struct d_pop_reader*     _reader,
    struct d_net_connection* _connection,
    enum d_pop_error         _status,
    const char*              _text
)
{
    struct d_pack_text     line   = { NULL, 0u };
    const enum d_pop_error status = d_pop_reader_read_line(_reader,
                                                           _connection,
                                                           &line);

    return ( (status == _status) &&
             ( (_text == NULL) ||
               (d_tests_pop_text_is(line,
                                    _text)) ) );
}

/*
d_tests_pop_converse
  Reads CONVERSATION from a memory stream cutting every read to `_chunk`
bytes, 0 for none, and then the end of the stream.
*/
static bool
d_tests_pop_converse(
    size_t _chunk
)
{
    struct d_net_memory        memory  = { .input_size = 0u };
    struct d_pop_reader        reader  = { .strict = 1 };
    struct d_pop_body_decoder  decoder = { .state = 0 };
    struct d_tests_pop_capture body    = { .length = 0u };
    const struct d_pack_sink   none    = { NULL,
                                           NULL };

    d_net_memory_init(&memory,
                      CONVERSATION,
                      strlen(CONVERSATION),
                      none);
    memory.chunk = _chunk;
    d_pop_reader_init(&reader);
    d_pop_body_decoder_init(&decoder);

    return ( (d_tests_pop_line_is(&reader,
                                  &memory.base,
                                  D_POP_OK,
                                  "+OK ready"))          &&
             (d_tests_pop_line_is(&reader,
                                  &memory.base,
                                  D_POP_OK,
                                  "-ERR [IN-USE] busy")) &&
             (d_tests_pop_line_is(&reader,
                                  &memory.base,
                                  D_POP_OK,
                                  "+OK 2 lines"))        &&
             (d_pop_reader_read_body(&reader,
                                     &memory.base,
                                     &decoder,
                                     d_tests_pop_capture_sink(&body)) ==
              D_POP_OK)                                  &&
             (d_tests_pop_capture_is(&body,
                                     "one\r\n.dot\r\n")) &&
             (d_tests_pop_line_is(&reader,
                                  &memory.base,
                                  D_POP_OK,
                                  "+OK next"))           &&
             (d_tests_pop_line_is(&reader,
                                  &memory.base,
                                  D_POP_ERROR_CLOSED,
                                  NULL))                 &&
             (reader.net_error == D_NET_OK) );
}

/*
d_tests_sa_pop_read_pieces
  Tests the following:
  - status lines, a body, and a line pipelined after it are read alike
    whole and in pieces of 1, 2, 3 and 7 bytes
  - the end of the stream where a line is due is CLOSED, with D_NET_OK
    kept as its cause
*/
bool
d_tests_sa_pop_read_pieces(
    struct d_test_counter* _counter
)
{
    static const size_t CHUNKS[] = { 0u, 1u, 2u, 3u, 7u };
    bool                read     = true;

    // each way of cutting the stream
    for (size_t i = 0u; i < (sizeof(CHUNKS) / sizeof(CHUNKS[0])); ++i)
    {
        read = (d_tests_pop_converse(CHUNKS[i])) && read;
    }

    return d_tests_net_assert(read,
                              "d_pop_reader_read_line",
                              "reads a conversation whole and in pieces",
                              _counter);
}

/*
d_tests_pop_body_ends
  Whether a status line and then the body `_body`, its terminator the
stream's last byte, read a byte at a time, give the body whole and end
cleanly on that byte.
*/
static bool
d_tests_pop_body_ends(
    const char* _stream,
    const char* _body
)
{
    struct d_net_memory        ending  = { .input_size = 0u };
    struct d_pop_reader        reader  = { .strict = 1 };
    struct d_pop_body_decoder  decoder = { .state = 0 };
    struct d_tests_pop_capture body    = { .length = 0u };
    const struct d_pack_sink   none    = { NULL,
                                           NULL };

    d_net_memory_init(&ending,
                      _stream,
                      strlen(_stream),
                      none);
    ending.chunk = 1u;
    d_pop_reader_init(&reader);
    d_pop_body_decoder_init(&decoder);

    return ( (d_tests_pop_line_is(&reader,
                                  &ending.base,
                                  D_POP_OK,
                                  "+OK")) &&
             (d_pop_reader_read_body(&reader,
                                     &ending.base,
                                     &decoder,
                                     d_tests_pop_capture_sink(&body)) ==
              D_POP_OK)                   &&
             (d_tests_pop_capture_is(&body,
                                     _body)) );
}

/*
d_tests_pop_body_at_end
  The last part of d_tests_sa_pop_read_body: a body whose terminator is the
stream's last byte, arriving a byte at a time, ends cleanly on that byte,
whether that byte is the first of a read the reader had to make with
nothing held, or follows one already held.
*/
static bool
d_tests_pop_body_at_end(void)
{
    return ( (d_tests_pop_body_ends("+OK\r\nx\r\n.\r\n",
                                    "x\r\n")) &&
             (d_tests_pop_body_ends("+OK\r\nxy\r\n.\r\n",
                                    "xy\r\n")) );
}

/*
d_tests_sa_pop_read_body
  Tests the following:
  - a body the stream ends inside is CLOSED, keeping what was decoded
  - a line too long for the reader is refused once, and reading resumes at
    the line after it
  - a body whose terminator is the stream's last byte, arriving alone, ends
    cleanly (d_tests_pop_body_at_end)
*/
bool
d_tests_sa_pop_read_body(
    struct d_test_counter* _counter
)
{
    static char                long_line[D_POP_LINE_CAPACITY + 64] = { 0 };
    struct d_net_memory        cut     = { .input_size = 0u };
    struct d_net_memory        longer  = { .input_size = 0u };
    struct d_pop_reader        reader  = { .strict = 1 };
    struct d_pop_body_decoder  decoder = { .state = 0 };
    struct d_tests_pop_capture body    = { .length = 0u };
    const struct d_pack_sink   none    = { NULL,
                                           NULL };
    bool                       result  = true;

    d_net_memory_init(&cut,
                      "+OK\r\nab\r\nc",
                      10u,
                      none);
    d_pop_reader_init(&reader);
    d_pop_body_decoder_init(&decoder);

    const bool cut_short =
        ( (d_tests_pop_line_is(&reader,
                               &cut.base,
                               D_POP_OK,
                               "+OK")) &&
          (d_pop_reader_read_body(&reader,
                                  &cut.base,
                                  &decoder,
                                  d_tests_pop_capture_sink(&body)) ==
           D_POP_ERROR_CLOSED)         &&
          (d_tests_pop_capture_is(&body,
                                  "ab\r\nc")) );

    memset(long_line,
           'x',
           sizeof(long_line) - 9u);
    memcpy(long_line + sizeof(long_line) - 9u,
           "\r\n+OK\r\n",
           8u);
    d_net_memory_init(&longer,
                      long_line,
                      sizeof(long_line) - 1u,
                      none);
    d_pop_reader_init(&reader);

    const bool resumed =
        ( (d_tests_pop_line_is(&reader,
                               &longer.base,
                               D_POP_ERROR_LINE_TOO_LONG,
                               NULL)) &&
          (d_tests_pop_line_is(&reader,
                               &longer.base,
                               D_POP_OK,
                               "+OK")) );

    result = d_tests_net_assert(cut_short,
                                "d_pop_reader_read_body",
                                "a body cut short is CLOSED, its start kept",
                                _counter) && result;
    result = d_tests_net_assert(resumed,
                                "d_pop_reader_read_line",
                                "resumes after a line too long",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_pop_body_at_end(),
                                "d_pop_reader_read_body",
                                "a body ending with the stream ends cleanly",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_pop_read_strict
  Tests the following:
  - a reader that is not strict ends a line at a bare LF
  - a strict one refuses such a line as MALFORMED, consumed, and reads on
    at the line after it
*/
bool
d_tests_sa_pop_read_strict(
    struct d_test_counter* _counter
)
{
    static const char        LINES[]  = "+OK one\n+OK two\r\n";
    struct d_net_memory      lenient  = { .input_size = 0u };
    struct d_net_memory      strict   = { .input_size = 0u };
    struct d_pop_reader      reader   = { .strict = 0 };
    const struct d_pack_sink none     = { NULL,
                                          NULL };
    bool                     result   = true;

    d_net_memory_init(&lenient,
                      LINES,
                      sizeof(LINES) - 1u,
                      none);
    d_pop_reader_init(&reader);
    reader.strict = 0;

    const bool accepted = ( (d_tests_pop_line_is(&reader,
                                                 &lenient.base,
                                                 D_POP_OK,
                                                 "+OK one")) &&
                            (d_tests_pop_line_is(&reader,
                                                 &lenient.base,
                                                 D_POP_OK,
                                                 "+OK two")) );

    d_net_memory_init(&strict,
                      LINES,
                      sizeof(LINES) - 1u,
                      none);
    d_pop_reader_init(&reader);
    reader.strict = 1;

    const bool refused = ( (d_tests_pop_line_is(&reader,
                                                &strict.base,
                                                D_POP_ERROR_MALFORMED,
                                                NULL)) &&
                           (d_tests_pop_line_is(&reader,
                                                &strict.base,
                                                D_POP_OK,
                                                "+OK two")) );

    result = d_tests_net_assert(accepted,
                                "d_pop_reader_read_line",
                                "a bare LF ends a line when not strict",
                                _counter) && result;
    result = d_tests_net_assert(refused,
                                "d_pop_reader_read_line",
                                "strict, a bare LF is refused, and read on",
                                _counter) && result;

    return result;
}

/*
d_tests_pop_stalled_body
  The body half of d_tests_sa_pop_read_stalls: a body read stalls between
the peer's writes, and the same decoder resumes it.
*/
static bool
d_tests_pop_stalled_body(
    struct d_pop_reader*     _reader,
    struct d_net_connection* _server,
    struct d_net_connection* _client
)
{
    struct d_pop_body_decoder  decoder = { .state = 0 };
    struct d_tests_pop_capture body    = { .length = 0u };

    d_pop_body_decoder_init(&decoder);

    return ( (d_net_write_all(_server,
                              "+OK\r\nbo",
                              7u,
                              NULL) == D_NET_OK) &&
             (d_tests_pop_line_is(_reader,
                                  _client,
                                  D_POP_OK,
                                  "+OK"))        &&
             (d_pop_reader_read_body(_reader,
                                     _client,
                                     &decoder,
                                     d_tests_pop_capture_sink(&body)) ==
              D_POP_ERROR_WOULD_BLOCK)           &&
             (d_net_write_all(_server,
                              "dy\r\n.\r\n",
                              7u,
                              NULL) == D_NET_OK) &&
             (d_pop_reader_read_body(_reader,
                                     _client,
                                     &decoder,
                                     d_tests_pop_capture_sink(&body)) ==
              D_POP_OK)                          &&
             (d_tests_pop_capture_is(&body,
                                     "body\r\n")) );
}

/*
d_tests_sa_pop_read_stalls
  Tests the following:
  - an empty pipe is WOULD_BLOCK, kept as the cause, and so is half a line
  - the rest of the line completes it
  - a body stalls and resumes (d_tests_pop_stalled_body)
  - the peer's shutdown is CLOSED, with D_NET_OK as its cause
*/
bool
d_tests_sa_pop_read_stalls(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe   pipe      = { .rings[0].size = 0u };
    unsigned char       to_b[64]  = { 0 };
    unsigned char       to_a[64]  = { 0 };
    struct d_pop_reader reader    = { .strict = 1 };
    bool                result    = true;

    (void)d_net_pipe_init(&pipe,
                          to_b,
                          sizeof(to_b),
                          to_a,
                          sizeof(to_a));
    d_pop_reader_init(&reader);

    struct d_net_connection* const server =
        d_net_pipe_connection(&pipe,
                              D_NET_PIPE_A);
    struct d_net_connection* const client =
        d_net_pipe_connection(&pipe,
                              D_NET_PIPE_B);

    const bool stalled =
        ( (d_tests_pop_line_is(&reader,
                               client,
                               D_POP_ERROR_WOULD_BLOCK,
                               NULL))                   &&
          (reader.net_error == D_NET_ERROR_WOULD_BLOCK) &&
          (d_net_write_all(server,
                           "+OK hal",
                           7u,
                           NULL) == D_NET_OK)           &&
          (d_tests_pop_line_is(&reader,
                               client,
                               D_POP_ERROR_WOULD_BLOCK,
                               NULL)) );
    const bool resumed =
        ( (d_net_write_all(server,
                           "f\r\n",
                           3u,
                           NULL) == D_NET_OK) &&
          (d_tests_pop_line_is(&reader,
                               client,
                               D_POP_OK,
                               "+OK half"))   &&
          (d_tests_pop_stalled_body(&reader,
                                    server,
                                    client)) );
    const bool ended =
        ( (d_net_connection_shutdown(server,
                                     D_NET_SHUTDOWN_WRITE) == D_NET_OK) &&
          (d_tests_pop_line_is(&reader,
                               client,
                               D_POP_ERROR_CLOSED,
                               NULL))                                   &&
          (reader.net_error == D_NET_OK) );

    result = d_tests_net_assert(stalled,
                                "d_pop_reader_read_line",
                                "an empty pipe or half a line would block",
                                _counter) && result;
    result = d_tests_net_assert(resumed,
                                "d_pop_reader_read_body",
                                "lines and bodies resume where they stalled",
                                _counter) && result;
    result = d_tests_net_assert(ended,
                                "d_pop_reader_read_line",
                                "the peer's shutdown is CLOSED",
                                _counter) && result;

    return result;
}

/*
FAILURES
  A script of reads: a line whose end comes with a timeout, a timeout alone,
a line, then a reset, a refusal, and the end of the steps.
*/
static const struct d_tests_pop_step FAILURES[] =
{
    { "+OK pa",         D_NET_OK                     },
    { "rt\r\n",         D_NET_ERROR_TIMED_OUT        },
    { "",               D_NET_ERROR_TIMED_OUT        },
    { "-ERR again\r\n", D_NET_OK                     },
    { "",               D_NET_ERROR_CONNECTION_RESET },
    { "",               D_NET_ERROR_ACCESS_DENIED    }
};

/*
d_tests_sa_pop_read_failures
  Tests the following:
  - bytes that come with an error are kept, and the line they complete is
    read; the error is kept as the read's report
  - a timeout alone is TRANSPORT, its cause kept, and the call may be
    repeated
  - a reset is CLOSED, and any other failure TRANSPORT, each with its cause
  - after the last step the stream has ended: CLOSED, with D_NET_OK
*/
bool
d_tests_sa_pop_read_failures(
    struct d_test_counter* _counter
)
{
    struct d_tests_pop_script script = { .count = 0u };
    struct d_pop_reader       reader = { .strict = 1 };
    bool                      result = true;

    d_tests_pop_script_init(&script,
                            FAILURES,
                            sizeof(FAILURES) / sizeof(FAILURES[0]));
    d_pop_reader_init(&reader);

    const bool kept =
        ( (d_tests_pop_line_is(&reader,
                               &script.base,
                               D_POP_OK,
                               "+OK part")) &&
          (reader.net_error == D_NET_ERROR_TIMED_OUT) );
    const bool repeated =
        ( (d_tests_pop_line_is(&reader,
                               &script.base,
                               D_POP_ERROR_TRANSPORT,
                               NULL))                 &&
          (reader.net_error == D_NET_ERROR_TIMED_OUT) &&
          (d_tests_pop_line_is(&reader,
                               &script.base,
                               D_POP_OK,
                               "-ERR again"))         &&
          (reader.net_error == D_NET_OK) );
    const bool causes =
        ( (d_tests_pop_line_is(&reader,
                               &script.base,
                               D_POP_ERROR_CLOSED,
                               NULL))                        &&
          (reader.net_error == D_NET_ERROR_CONNECTION_RESET) &&
          (d_tests_pop_line_is(&reader,
                               &script.base,
                               D_POP_ERROR_TRANSPORT,
                               NULL))                        &&
          (reader.net_error == D_NET_ERROR_ACCESS_DENIED)    &&
          (d_tests_pop_line_is(&reader,
                               &script.base,
                               D_POP_ERROR_CLOSED,
                               NULL))                        &&
          (reader.net_error == D_NET_OK) );

    result = d_tests_net_assert(kept,
                                "d_pop_reader_read_line",
                                "keeps bytes that come with an error",
                                _counter) && result;
    result = d_tests_net_assert(repeated,
                                "d_pop_reader_read_line",
                                "a failure is reported, and may be repeated",
                                _counter) && result;
    result = d_tests_net_assert(causes,
                                "d_pop_reader_read_line",
                                "each failure becomes its error, cause kept",
                                _counter) && result;

    return result;
}

/*
d_tests_pop_read_nulls
  The NULL half of d_tests_sa_pop_read_refusals, over a reader holding a
whole line: each call is refused before the buffer is looked at, so the line
stays buffered and the connection unread.
*/
static bool
d_tests_pop_read_nulls(
    struct d_pop_reader*     _reader,
    struct d_net_connection* _connection
)
{
    struct d_pop_body_decoder decoder = { .state = 0 };
    struct d_pack_text        line    = { NULL, 0u };
    const struct d_pack_sink  none    = { NULL,
                                          NULL };

    d_pop_body_decoder_init(&decoder);

    return ( (d_pop_reader_read_line(NULL,
                                     _connection,
                                     &line) ==
              D_POP_ERROR_INVALID_ARGUMENT) &&
             (d_pop_reader_read_line(_reader,
                                     NULL,
                                     &line) ==
              D_POP_ERROR_INVALID_ARGUMENT) &&
             (d_pop_reader_read_line(_reader,
                                     _connection,
                                     NULL) ==
              D_POP_ERROR_INVALID_ARGUMENT) &&
             (d_pop_reader_read_body(NULL,
                                     _connection,
                                     &decoder,
                                     none) ==
              D_POP_ERROR_INVALID_ARGUMENT) &&
             (d_pop_reader_read_body(_reader,
                                     NULL,
                                     &decoder,
                                     none) ==
              D_POP_ERROR_INVALID_ARGUMENT) &&
             (d_pop_reader_read_body(_reader,
                                     _connection,
                                     NULL,
                                     none) ==
              D_POP_ERROR_INVALID_ARGUMENT) &&
             (d_pop_reader_buffered(_reader) == 6u) );
}

/*
d_tests_sa_pop_read_refusals
  Tests the following:
  - init makes a reader new whatever it held: cfg_pop.h's strictness,
    nothing discarding or buffered, and D_NET_OK as its cause
  - a NULL reader, connection, line, or decoder is INVALID_ARGUMENT, even
    with a line buffered, which stays buffered (d_tests_pop_read_nulls)
  - a connection without its operations is the connection's refusal,
    INVALID_ARGUMENT, kept as the cause
*/
bool
d_tests_sa_pop_read_refusals(
    struct d_test_counter* _counter
)
{
    struct d_net_memory      memory = { .input_size = 0u };
    struct d_net_connection  empty  = { NULL };
    struct d_pop_reader      reader = { .strict = 1 };
    const struct d_pack_sink none   = { NULL,
                                        NULL };
    bool                     result = true;

    d_net_memory_init(&memory,
                      "+OK\r\n-ERR\r\n",
                      11u,
                      none);
    memset(&reader,
           0xA5,
           sizeof(reader));
    d_pop_reader_init(&reader);

    const bool fresh = ( (reader.net_error == D_NET_OK)           &&
                         (reader.strict == D_INTERNAL_POP_STRICT) &&
                         (reader.discarding == 0)                 &&
                         (d_pop_reader_buffered(&reader) == 0u) );
    const bool nulls =
        ( (d_tests_pop_line_is(&reader,
                               &memory.base,
                               D_POP_OK,
                               "+OK"))           &&
          (memory.input_offset == 11u)           &&
          (d_tests_pop_read_nulls(&reader,
                                  &memory.base)) &&
          (reader.net_error == D_NET_OK)         &&
          (d_tests_pop_line_is(&reader,
                               &memory.base,
                               D_POP_OK,
                               "-ERR")) );
    const bool unusable =
        ( (d_tests_pop_line_is(&reader,
                               &empty,
                               D_POP_ERROR_INVALID_ARGUMENT,
                               NULL)) &&
          (reader.net_error == D_NET_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(fresh,
                                "d_pop_reader_init",
                                "makes a reader that held anything new",
                                _counter) && result;
    result = d_tests_net_assert(nulls,
                                "d_pop_reader_read_line",
                                "NULL arguments are refused, nothing taken",
                                _counter) && result;
    result = d_tests_net_assert(unusable,
                                "d_pop_reader_read_line",
                                "a connection without operations is refused",
                                _counter) && result;

    return result;
}
