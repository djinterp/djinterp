/*******************************************************************************
* djinterp [net]                                          pop_tests_sa_session.c
*
* A POP3 client and server talking over net_memory.h's pipe, in one thread.
*   Both ends use only this module and net.h: each sends with the helpers,
* reads with its own reader, and the server streams a body through its
* encoder straight onto the connection. Every read and write is cut to 5
* bytes, so each line and the body cross in pieces.
*
*
* path:      /tests/djinterp/net/pop/pop_tests_sa_session.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./pop_tests_sa.h"  // the suite


// d_tests_pop_ends
//   struct: the two ends of a session, each with its reader.
struct d_tests_pop_ends
{
    struct d_net_connection* server;
    struct d_net_connection* client;
    struct d_pop_reader      server_reader;
    struct d_pop_reader      client_reader;
};

/*
d_tests_pop_greet
  The server's greeting, as the client reads and parses it.
*/
static bool
d_tests_pop_greet(
    struct d_tests_pop_ends* _ends
)
{
    struct d_pack_text          line      = { NULL, 0u };
    struct d_pack_text          timestamp = { NULL, 0u };
    struct d_pop_response       response  =
        { .indicator = D_POP_INDICATOR_NONE };
    const struct d_pop_response greeting  =
        d_tests_pop_response(D_POP_INDICATOR_OK,
                             "POP3 ready <1.2@host>");

    return ( (d_pop_send_response(_ends->server,
                                  &greeting) == D_POP_OK)   &&
             (d_pop_reader_read_line(&_ends->client_reader,
                                     _ends->client,
                                     &line) == D_POP_OK)    &&
             (d_pop_response_parse(line,
                                   &response) == D_POP_OK)  &&
             (response.indicator == D_POP_INDICATOR_OK)     &&
             (d_pop_apop_timestamp(response.text,
                                   &timestamp) == D_POP_OK) &&
             (d_tests_pop_text_is(timestamp,
                                  "<1.2@host>")) );
}

/*
d_tests_pop_serve_retr
  The server's half of a RETR: the command read and checked, then a status
line and a stuffed body, encoded straight onto the connection.
*/
static bool
d_tests_pop_serve_retr(
    struct d_tests_pop_ends* _ends
)
{
    static const char           BODY[]  = "a\nb\r\n.c\r\n";
    struct d_pack_text          line    = { NULL, 0u };
    struct d_pop_command_line   command =
        { .command = D_POP_COMMAND_UNKNOWN };
    struct d_pop_body_encoder   encoder = { .normalize = 1 };
    const struct d_pop_response status  =
        d_tests_pop_response(D_POP_INDICATOR_OK,
                             "message follows");
    const struct d_pack_sink    out     =
        d_tests_pop_connection_sink(_ends->server);

    d_pop_body_encoder_init(&encoder);

    return ( (d_pop_reader_read_line(&_ends->server_reader,
                                     _ends->server,
                                     &line) == D_POP_OK) &&
             (d_pop_command_parse(line,
                                  &command) == D_POP_OK) &&
             (command.command == D_POP_COMMAND_RETR)     &&
             (command.arg_count == 1u)                   &&
             (d_tests_pop_text_is(command.args[0],
                                  "1"))                  &&
             (d_pop_send_response(_ends->server,
                                  &status) == D_POP_OK)  &&
             (d_pop_body_encode(&encoder,
                                BODY,
                                sizeof(BODY) - 1u,
                                out) == D_POP_OK)        &&
             (d_pop_body_encode_finish(&encoder,
                                       out) == D_POP_OK) );
}

/*
d_tests_pop_fetch
  The client's half of a RETR: the command sent, the status line read, and
the body decoded into `_body`, its stuffing and line endings undone.
*/
static bool
d_tests_pop_fetch(
    struct d_tests_pop_ends*    _ends,
    struct d_tests_pop_capture* _body
)
{
    struct d_pack_text              line     = { NULL, 0u };
    struct d_pop_response           response =
        { .indicator = D_POP_INDICATOR_NONE };
    struct d_pop_body_decoder       decoder  = { .state = 0 };
    const struct d_pop_command_line retr     =
        d_tests_pop_command(D_POP_COMMAND_RETR,
                            "1");

    d_pop_body_decoder_init(&decoder);

    return ( (d_pop_send_command(_ends->client,
                                 &retr) == D_POP_OK)       &&
             (d_tests_pop_serve_retr(_ends))               &&
             (d_pop_reader_read_line(&_ends->client_reader,
                                     _ends->client,
                                     &line) == D_POP_OK)   &&
             (d_pop_response_parse(line,
                                   &response) == D_POP_OK) &&
             (response.indicator == D_POP_INDICATOR_OK)    &&
             (d_pop_reader_read_body(&_ends->client_reader,
                                     _ends->client,
                                     &decoder,
                                     d_tests_pop_capture_sink(_body)) ==
              D_POP_OK)                                    &&
             (d_tests_pop_capture_is(_body,
                                     "a\r\nb\r\n.c\r\n")) );
}

/*
d_tests_pop_quit
  QUIT, its answer, and the server's goodbye: after the last line the client
reads the end of the stream, with nothing left buffered.
*/
static bool
d_tests_pop_quit(
    struct d_tests_pop_ends* _ends
)
{
    struct d_pack_text              line  = { NULL, 0u };
    const struct d_pop_command_line quit  =
        d_tests_pop_command(D_POP_COMMAND_QUIT,
                            NULL);
    const struct d_pop_response     bye   =
        d_tests_pop_response(D_POP_INDICATOR_OK,
                             "bye");

    return ( (d_pop_send_command(_ends->client,
                                 &quit) == D_POP_OK)               &&
             (d_pop_reader_read_line(&_ends->server_reader,
                                     _ends->server,
                                     &line) == D_POP_OK)           &&
             (d_tests_pop_text_is(line,
                                  "QUIT"))                         &&
             (d_pop_send_response(_ends->server,
                                  &bye) == D_POP_OK)               &&
             (d_net_connection_shutdown(_ends->server,
                                        D_NET_SHUTDOWN_WRITE) ==
              D_NET_OK)                                            &&
             (d_pop_reader_read_line(&_ends->client_reader,
                                     _ends->client,
                                     &line) == D_POP_OK)           &&
             (d_tests_pop_text_is(line,
                                  "+OK bye"))                      &&
             (d_pop_reader_read_line(&_ends->client_reader,
                                     _ends->client,
                                     &line) == D_POP_ERROR_CLOSED) &&
             (d_pop_reader_buffered(&_ends->client_reader) == 0u) );
}

/*
d_tests_sa_pop_session
  Tests the following:
  - the server's greeting reaches the client, its APOP timestamp intact
  - a RETR crosses both ways: the server reads the command, and the client
    reads the status line and the body, unstuffed, in CR LF
  - QUIT ends the session: the server's goodbye, then the end of its stream
*/
bool
d_tests_sa_pop_session(
    struct d_test_counter* _counter
)
{
    struct d_net_pipe          pipe           = { .rings[0].size = 0u };
    unsigned char              to_client[128] = { 0 };
    unsigned char              to_server[128] = { 0 };
    struct d_tests_pop_ends    ends           = { .server = NULL };
    struct d_tests_pop_capture body           = { .length = 0u };

    (void)d_net_pipe_init(&pipe,
                          to_client,
                          sizeof(to_client),
                          to_server,
                          sizeof(to_server));
    d_net_pipe_set_chunk(&pipe,
                         D_NET_PIPE_A,
                         5u);
    d_net_pipe_set_chunk(&pipe,
                         D_NET_PIPE_B,
                         5u);
    ends.server = d_net_pipe_connection(&pipe,
                                        D_NET_PIPE_A);
    ends.client = d_net_pipe_connection(&pipe,
                                        D_NET_PIPE_B);
    d_pop_reader_init(&ends.server_reader);
    d_pop_reader_init(&ends.client_reader);

    const bool talked = ( (d_tests_pop_greet(&ends)) &&
                          (d_tests_pop_fetch(&ends,
                                             &body)) &&
                          (d_tests_pop_quit(&ends)) );

    return d_tests_net_assert(talked,
                              "d_pop_reader_read_body",
                              "a client and a server talk over a pipe",
                              _counter);
}
