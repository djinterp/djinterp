/*******************************************************************************
* djinterp [net]                                          smtp_tests_sa_client.c
*
* djinterp SMTP kernel tests -- client sessions.
*   Standalone tests of smtp_client.c over net_tests_peer.h's scripted peer,
* which checks the exact bytes on the wire: pipelining, SASL (CRAM-MD5
* against RFC 2195's own example among it), the HELO fallback, recipient
* refusal and reset, the refusal to authenticate in the clear,
* command-injection rejection, attaching, failing reads and writes, and the
* edges: replies longer than a line, a reply's class, a message's first
* line, and messages and batches larger than one write.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_sa_client.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./smtp_tests_sa.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf
#include <string.h>   // memcpy, memset, strlen


/*
d_tests_smtp_pipelined_wire
  What the pipelined session must write: EHLO, AUTH PLAIN, then MAIL with
SIZE, both RCPTs and DATA, the message dot-stuffed and ended, and QUIT.
*/
static void
d_tests_smtp_pipelined_wire(
    struct d_smtp_buffer* _text,
    size_t                _size
)
{
    (void)d_smtp_buffer_append_text(_text,
                                    "EHLO c.example\r\nAUTH PLAIN ");
    (void)d_smtp_buffer_append_base64(_text,
                                      "\0alice\0s3cret",
                                      13u);
    (void)d_smtp_buffer_append_text(_text,
                                    "\r\nMAIL FROM:<alice@example.org>"
                                    " SIZE=");
    (void)d_smtp_buffer_append_number(_text,
                                      _size);
    (void)d_smtp_buffer_append_text(_text,
                                    "\r\nRCPT TO:<bob@example.com>\r\n"
                                    "RCPT TO:<nobody@example.com>\r\n"
                                    "DATA\r\nSubject: hi\r\n\r\n..dot"
                                    "\r\nbody\r\n.\r\nQUIT\r\n");

    return;
}

/*
d_tests_smtp_pipelined_run
  Opens over an already-secure connection, authenticates, sends and quits;
the first failure ends it.
*/
static enum d_smtp_error
d_tests_smtp_pipelined_run(
    struct d_smtp_client*      _client,
    struct d_tests_net_peer*   _peer,
    const char*                _message,
    struct d_smtp_send_report* _report
)
{
    static const char* const RECIPIENTS[] = { "bob@example.com",
                                              "nobody@example.com" };
    const struct d_smtp_credentials credentials = { .user      = "alice",
                                                    .secret    = "s3cret",
                                                    .mechanism = 0u };
    const struct d_smtp_envelope    envelope    =
    {
        .sender          = "alice@example.org",
        .recipients      = RECIPIENTS,
        .recipient_count = 2u
    };
    enum d_smtp_error step = d_tests_smtp_open(
                                 _client,
                                 d_tests_net_peer_connection(_peer,
                                                             true),
                                 true);

    if (step == D_SMTP_OK)
    {
        step = d_smtp_client_authenticate(_client,
                                          &credentials);
    }

    if (step == D_SMTP_OK)
    {
        step = d_smtp_client_send(_client,
                                  &envelope,
                                  _message,
                                  strlen(_message),
                                  _report);
    }

    return (step == D_SMTP_OK) ? d_smtp_client_quit(_client)
                               : step;
}

/*
d_tests_sa_smtp_client_script
  Tests the following:
  - greeting, EHLO and AUTH PLAIN over an already-secure connection, its
    replies trickling in seven bytes at a time
  - MAIL, both RCPTs and DATA leave in a single pipelined write, with SIZE
  - the message is dot-stuffed and terminated
  - a refused recipient is reported while the message is still delivered
*/
bool
d_tests_sa_smtp_client_script(
    struct d_test_counter* _counter
)
{
    static const char              MESSAGE[]      =
        "Subject: hi\r\n\r\n.dot\r\nbody";
    const char* const              TEST           =
        "d_smtp_client (pipelined)";
    struct d_smtp_recipient_status statuses[2]    = { { .code = 0u } };
    struct d_smtp_send_report      report         = { .recipients =
                                                          statuses };
    struct d_tests_net_peer*       peer           = d_tests_smtp_peer(
        "220 mail.example.com ESMTP\r\n"
        "250-mail.example.com Hello\r\n250-PIPELINING\r\n"
        "250-SIZE 1000000\r\n250-AUTH PLAIN LOGIN\r\n"
        "250 ENHANCEDSTATUSCODES\r\n"
        "235 2.7.0 Authentication successful\r\n"
        "250 2.1.0 Sender OK\r\n250 2.1.5 Recipient OK\r\n"
        "550 5.1.1 No such user\r\n354 Go ahead\r\n"
        "250 2.0.0 Queued\r\n221 2.0.0 Bye\r\n");
    struct d_smtp_client           client         = { .utf8 = false };
    char                           expected[1024] = { 0 };
    struct d_smtp_buffer           text           =
    {
        .data     = expected,
        .capacity = sizeof(expected),
        .length   = 0
    };
    bool                           result         = true;

    if (!d_tests_net_assert( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _counter))
    {
        return false;
    }

    peer->recv_chunk = 7u;
    d_tests_smtp_pipelined_wire(&text,
                                strlen(MESSAGE));

    const enum d_smtp_error step = d_tests_smtp_pipelined_run(&client,
                                                              peer,
                                                              MESSAGE,
                                                              &report);

    result = d_tests_net_assert( ( (step == D_SMTP_OK) &&
                                   (d_tests_smtp_sent(peer,
                                                      &text)) ),
                                 TEST,
                                 "exactly the expected bytes are written",
                                 _counter) && result;
    result = d_tests_net_assert( (peer->writes == 6),
                                 TEST,
                                 "MAIL, RCPTs and DATA leave in one write",
                                 _counter) && result;
    result = d_tests_net_assert( ( (report.accepted == 1u)           &&
                                   (statuses[0].code == 250u)        &&
                                   (statuses[1].code == 550u)        &&
                                   (statuses[1].status.detail == 1u) &&
                                   (report.data_code == 250u) ),
                                 TEST,
                                 "the report records each recipient",
                                 _counter) && result;
    d_smtp_client_close(&client);
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_sa_smtp_client_cram
  Tests the following:
  - CRAM-MD5 answers RFC 2195's own example: the challenge
    <1896.697170952@postoffice.reston.mci.net>, the user "tim" and the secret
    "tanstaaftanstaaf" give "tim b913a602c7eda7a495b4e6e7334d3890"
  - the mechanism is chosen when it is the only one the server offers
*/
bool
d_tests_sa_smtp_client_cram(
    struct d_test_counter* _counter
)
{
    static const char               EXPECTED[]  =
        "EHLO c.example\r\n"
        "AUTH CRAM-MD5\r\n"
        "dGltIGI5MTNhNjAyYzdlZGE3YTQ5NWI0ZTZlNzMzNGQzODkw\r\n";
    const char* const               TEST        = "d_smtp_client (CRAM-MD5)";
    const struct d_smtp_credentials credentials =
    {
        .user      = "tim",
        .secret    = "tanstaaftanstaaf",
        .mechanism = 0u
    };
    struct d_tests_net_peer*        peer        = d_tests_smtp_peer(
        "220 postoffice.reston.mci.net ESMTP\r\n"
        "250-postoffice.reston.mci.net\r\n"
        "250 AUTH CRAM-MD5\r\n"
        "334 PDE4OTYuNjk3MTcwOTUyQHBvc3RvZmZpY2UucmVzdG9uLm1jaS5uZXQ+\r\n"
        "235 2.7.0 Authentication successful\r\n");
    struct d_smtp_client            client      = { .utf8 = false };
    enum d_smtp_error               step        = D_SMTP_ERROR_STATE;
    bool                            result      = true;

    d_smtp_client_init(&client);

    if (peer)
    {
        step = d_tests_smtp_open(&client,
                                 d_tests_net_peer_connection(peer,
                                                             true),
                                 true);
    }

    // authenticate once the session is open
    if (step == D_SMTP_OK)
    {
        step = d_smtp_client_authenticate(&client,
                                          &credentials);
    }

    result = d_tests_net_assert( ( (step == D_SMTP_OK) &&
                                   (client.authenticated) ),
                                 TEST,
                                 "the RFC's challenge answered, and accepted",
                                 _counter) && result;
    result = d_tests_net_assert( ( (peer) &&
                                   (d_tests_net_peer_sent(peer,
                                                          EXPECTED)) ),
                                 TEST,
                                 "the answer is the RFC's, in base64",
                                 _counter) && result;
    d_smtp_client_close(&client);
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_sa_smtp_client_serial
  Tests the following:
  - a server without EHLO gets HELO, and no extensions are assumed
  - without PIPELINING, commands go one at a time
  - with every recipient refused, the transaction is reset and the refusal
    is what the caller sees
*/
bool
d_tests_sa_smtp_client_serial(
    struct d_test_counter* _counter
)
{
    static const char* const     RECIPIENTS[] = { "d@example.net" };
    const char* const            TEST         = "d_smtp_client (serial)";
    const struct d_smtp_envelope envelope     =
    {
        .sender          = "a@example.org",
        .recipients      = RECIPIENTS,
        .recipient_count = 1u
    };
    struct d_tests_net_peer*     peer         = d_tests_smtp_peer(
        "220 old.example.com\r\n"
        "502 5.5.1 Command not implemented\r\n"
        "250 old.example.com\r\n250 OK\r\n"
        "550 5.1.1 Unknown user\r\n250 OK\r\n");
    struct d_smtp_client         client       = { .utf8 = false };
    enum d_smtp_error            opened       = D_SMTP_ERROR_STATE;
    enum d_smtp_error            sent         = D_SMTP_ERROR_STATE;
    bool                         result       = true;

    d_smtp_client_init(&client);

    if (peer)
    {
        opened = d_tests_smtp_open(&client,
                                   d_tests_net_peer_connection(peer,
                                                               true),
                                   false);
        sent   = d_smtp_client_send(&client,
                                    &envelope,
                                    "x",
                                    1u,
                                    NULL);
    }

    result = d_tests_net_assert( ( (opened == D_SMTP_OK) &&
                                   (!client.capabilities.extended) ),
                                 TEST,
                                 "HELO follows a 502 to EHLO",
                                 _counter) && result;
    result = d_tests_net_assert( ( (sent == D_SMTP_ERROR_REJECTED) &&
                                   (client.reply.code == 550u)     &&
                                   (client.state == D_SMTP_CLIENT_READY) ),
                                 TEST,
                                 "the refusal survives the reset",
                                 _counter) && result;
    result = d_tests_net_assert(
                 ( (peer) &&
                   (d_tests_net_peer_sent(peer,
                                          "EHLO c.example\r\n"
                                          "HELO c.example\r\n"
                                          "MAIL FROM:<a@example.org>\r\n"
                                          "RCPT TO:<d@example.net>\r\n"
                                          "RSET\r\n")) ),
                 TEST,
                 "commands go one at a time, ending with RSET",
                 _counter) && result;
    d_smtp_client_close(&client);
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_sa_smtp_client_attach_with
  Tests the following (decision 107 of the register):
  - attach_with refuses NULL options and a malformed identity, leaving the
    client closed
  - over a plaintext stream, with allow_insecure_auth, AUTH PLAIN goes out,
    and hello(NULL) announces the identity the options gave
  - plain attach afterwards takes AUTH in the clear as refused again
  It runs past 75 lines because one client runs through both scripted
sessions.
*/
bool
d_tests_sa_smtp_client_attach_with(
    struct d_test_counter* _counter
)
{
    const char* const                   TEST        =
        "d_smtp_client_attach_with";
    const struct d_smtp_credentials     credentials = { .user   = "u",
                                                        .secret = "p" };
    struct d_tests_net_peer*            peer        = d_tests_smtp_peer(
                                                          "220 x\r\n"
                                                          "250-x\r\n"
                                                          "250 AUTH PLAIN\r\n"
                                                          "235 ok\r\n");
    struct d_tests_net_peer*            again       = d_tests_smtp_peer(
                                                          "220 x\r\n"
                                                          "250-x\r\n"
                                                          "250 AUTH PLAIN\r\n");
    struct d_smtp_client                client      = { .utf8 = false };
    struct d_smtp_client_attach_options options     = { .secure = false };
    bool                                result      = true;

    d_smtp_client_init(&client);
    d_smtp_client_attach_options_init(&options);
    d_smtp_client_attach_options_init(NULL);

    // a scripted peer for each session
    if (!d_tests_net_assert( ( (peer) &&
                               (again) ),
                             TEST,
                             "two scripted peers",
                             _counter))
    {
        d_tests_net_peer_free(peer);
        d_tests_net_peer_free(again);

        return false;
    }

    struct d_net_connection* const connection =
        d_tests_net_peer_connection(peer,
                                    true);

    options.helo_domain = "not a domain!";

    result = d_tests_net_assert(
                 ( (d_smtp_client_attach_with(&client,
                                              connection,
                                              NULL) ==
                    D_SMTP_ERROR_INVALID_ARGUMENT) &&
                   (d_smtp_client_attach_with(&client,
                                              connection,
                                              &options) ==
                    D_SMTP_ERROR_INVALID_ARGUMENT) &&
                   (client.state == D_SMTP_CLIENT_CLOSED) ),
                 TEST,
                 "NULL options and a malformed identity are refused",
                 _counter) && result;

    options.helo_domain         = "c.example";
    options.allow_insecure_auth = true;

    const bool authenticated =
        ( (d_smtp_client_attach_with(&client,
                                     connection,
                                     &options) == D_SMTP_OK) &&
          (d_smtp_client_greeting(&client) == D_SMTP_OK)     &&
          (d_smtp_client_hello(&client,
                               NULL) == D_SMTP_OK)           &&
          (d_smtp_client_authenticate(&client,
                                      &credentials) == D_SMTP_OK) );

    result = d_tests_net_assert(
                 ( (authenticated) &&
                   (d_tests_net_peer_sent(peer,
                                          "EHLO c.example\r\n"
                                          "AUTH PLAIN AHUAcA==\r\n")) ),
                 TEST,
                 "allowed, AUTH runs in the clear under the given identity",
                 _counter) && result;
    d_smtp_client_close(&client);

    const bool refused =
        ( (d_smtp_client_attach(&client,
                                d_tests_net_peer_connection(again,
                                                            true),
                                false) == D_SMTP_OK)     &&
          (d_smtp_client_greeting(&client) == D_SMTP_OK) &&
          (d_smtp_client_hello(&client,
                               NULL) == D_SMTP_OK)       &&
          (d_smtp_client_authenticate(&client,
                                      &credentials) ==
           D_SMTP_ERROR_INSECURE) );

    result = d_tests_net_assert(refused,
                                TEST,
                                "plain attach refuses AUTH in the clear again",
                                _counter) && result;
    d_smtp_client_close(&client);
    d_tests_net_peer_free(peer);
    d_tests_net_peer_free(again);

    return result;
}

/*
d_tests_sa_smtp_client_guards
  Tests the following:
  - AUTH is refused on a plaintext channel, before anything is sent
  - CR and LF in a recipient cannot inject a command
*/
bool
d_tests_sa_smtp_client_guards(
    struct d_test_counter* _counter
)
{
    static const char* const        EVIL[]      =
        { "e@example.com>\r\nRCPT TO:<x@example.com" };
    const char* const               TEST        = "d_smtp_client (guards)";
    const struct d_smtp_credentials credentials = { .user   = "u",
                                                    .secret = "p" };
    const struct d_smtp_envelope    envelope    = { .sender          = "",
                                                    .recipients      = EVIL,
                                                    .recipient_count = 1u };
    struct d_tests_net_peer*        peer        = d_tests_smtp_peer(
                                                      "220 x\r\n250-x\r\n"
                                                      "250-PIPELINING\r\n"
                                                      "250 AUTH PLAIN\r\n");
    struct d_smtp_client            client      = { .utf8 = false };
    bool                            guarded     = false;

    if ( (peer) &&
         (d_tests_smtp_open(&client,
                            d_tests_net_peer_connection(peer,
                                                        true),
                            false) == D_SMTP_OK) )
    {
        guarded = ( (d_smtp_client_authenticate(&client,
                                                &credentials) ==
                     D_SMTP_ERROR_INSECURE)         &&
                    (d_smtp_client_send(&client,
                                        &envelope,
                                        "x",
                                        1u,
                                        NULL) ==
                     D_SMTP_ERROR_INVALID_ARGUMENT) &&
                    (d_tests_net_peer_sent(peer,
                                           "EHLO c.example\r\n")) );
        d_smtp_client_close(&client);
    }

    d_tests_net_peer_free(peer);

    return d_tests_net_assert(guarded,
                              TEST,
                              "no cleartext AUTH, no injected RCPT, nothing "
                              "sent",
                              _counter);
}

/*
d_tests_smtp_attach_refusals
  Whether attach refuses a NULL client, a NULL connection and `_broken`,
leaving the client closed, then takes `_connection` once, and not twice.
*/
static bool
d_tests_smtp_attach_refusals(
    struct d_smtp_client*    _client,
    struct d_net_connection* _connection,
    struct d_net_connection* _broken
)
{
    return ( (d_smtp_client_attach(NULL,
                                   _connection,
                                   false) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)            &&
             (d_smtp_client_attach(_client,
                                   NULL,
                                   false) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)            &&
             (d_smtp_client_attach(_client,
                                   _broken,
                                   false) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)            &&
             (_client->state == D_SMTP_CLIENT_CLOSED)   &&
             (d_smtp_client_attach(_client,
                                   _connection,
                                   false) == D_SMTP_OK) &&
             (d_smtp_client_attach(_client,
                                   _connection,
                                   false) == D_SMTP_ERROR_STATE) );
}

/*
d_tests_smtp_attached_afresh
  Whether a closed client, given the causes an earlier session's end might
have left it, attaches to `_connection` as secure with them forgotten, and
its last reply too.
*/
static bool
d_tests_smtp_attached_afresh(
    struct d_smtp_client*    _client,
    struct d_net_connection* _connection
)
{
    _client->tls_status       = D_SSL_ERROR_VERIFY;
    _client->reader.net_error = D_NET_ERROR_TIMED_OUT;

    return ( (d_smtp_client_attach(_client,
                                   _connection,
                                   true) == D_SMTP_OK) &&
             (_client->reply.code == 0u)               &&
             (_client->tls_status == D_SSL_OK)         &&
             (_client->reader.net_error == D_NET_OK)   &&
             (_client->secure)                         &&
             (_client->state == D_SMTP_CLIENT_GREETING) );
}

/*
d_tests_sa_smtp_client_attach
  Tests the following:
  - attach refuses a NULL client, a NULL connection, and one whose table
    lacks a required operation; and a client already attached
  - close leaves an attached connection open and the client without one,
    keeps the last reply, and leaves no TLS session; closing again, or
    NULL, does nothing
  - a closed client attaches again, the old reply and causes forgotten
*/
bool
d_tests_sa_smtp_client_attach(
    struct d_test_counter* _counter
)
{
    const char* const              TEST    = "d_smtp_client_attach";
    struct d_tests_net_peer*       peer    = d_tests_smtp_peer("220 x\r\n"
                                                               "250 x\r\n");
    struct d_net_connection_vtable partial = { .read = NULL };
    struct d_net_connection        broken  = { &partial };
    struct d_smtp_client           client  = { .utf8 = false };
    bool                           result  = true;

    d_smtp_client_init(&client);
    d_smtp_client_init(NULL);

    if (!d_tests_net_assert( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _counter))
    {
        return false;
    }

    struct d_net_connection* const connection =
        d_tests_net_peer_connection(peer,
                                    true);

    partial       = *connection->vtable;
    partial.write = NULL;

    const bool refused = d_tests_smtp_attach_refusals(&client,
                                                      connection,
                                                      &broken);
    const bool greeted = ( (d_smtp_client_greeting(&client) == D_SMTP_OK) &&
                           (d_smtp_client_hello(&client,
                                                "c.example") == D_SMTP_OK) );

    d_smtp_client_close(&client);
    d_smtp_client_close(&client);
    d_smtp_client_close(NULL);

    const bool closed = ( (greeted)                                    &&
                          (client.state == D_SMTP_CLIENT_CLOSED)       &&
                          (client.connection == NULL)                  &&
                          (client.reader.connection == NULL)           &&
                          (client.reply.code == 250u)                  &&
                          (peer->closes == 0)                          &&
                          (d_smtp_client_tls_session(&client) == NULL) &&
                          (d_smtp_client_tls_session(NULL) == NULL) );
    const bool again  = d_tests_smtp_attached_afresh(&client,
                                                     connection);

    result = d_tests_net_assert(refused,
                                TEST,
                                "bad arguments and a second attach refused",
                                _counter) && result;
    result = d_tests_net_assert(closed,
                                TEST,
                                "close leaves the connection to its owner",
                                _counter) && result;
    result = d_tests_net_assert(again,
                                TEST,
                                "a closed client attaches again, afresh",
                                _counter) && result;
    d_smtp_client_close(&client);
    d_tests_net_peer_free(peer);

    return result;
}

// d_tests_smtp_breakage
//   struct: how a peer breaks, and what a client must make of it.
struct d_tests_smtp_breakage
{
    const char*       after_ehlo;  // what the peer says to EHLO, or NULL
    bool              fails;       // every write fails
    bool              resets;      // as a reset connection's would
    bool              stalls;      // writes take nothing
    enum d_smtp_error status;      // what EHLO must report
    enum d_net_error  cause;       // the cause the reader must keep
};

/*
d_tests_smtp_breaks
  Greets over a peer that says 220 and then breaks as `_case` says, then
says EHLO, which must fail as it expects, keeping the cause and leaving the
client where it was.
*/
static bool
d_tests_smtp_breaks(
    const struct d_tests_smtp_breakage* _case
)
{
    struct d_tests_net_peer* peer   = d_tests_net_peer_new();
    struct d_smtp_client     client = { .utf8 = false };
    bool                     broken = false;

    d_smtp_client_init(&client);

    if ( (peer)                              &&
         (d_tests_net_peer_say(peer,
                               "",
                               "220 x\r\n")) &&
         ( (!_case->after_ehlo) ||
           (d_tests_net_peer_say(peer,
                                 "EHLO",
                                 _case->after_ehlo)) ) )
    {
        peer->fail_after = (_case->fails) ? 0u : peer->fail_after;
        peer->resets     = _case->resets;
        peer->stalls     = _case->stalls;
        broken = ( (d_smtp_client_attach(&client,
                                         d_tests_net_peer_connection(peer,
                                                                     true),
                                         false) == D_SMTP_OK)     &&
                   (d_smtp_client_greeting(&client) == D_SMTP_OK) &&
                   (d_smtp_client_hello(&client,
                                        "c.example") ==
                    _case->status)                                &&
                   (client.reader.net_error == _case->cause)      &&
                   (client.tls_status == D_SSL_OK)                &&
                   (client.state == D_SMTP_CLIENT_HELLO) );
    }

    d_smtp_client_close(&client);
    d_tests_net_peer_free(peer);

    return broken;
}

/*
d_tests_sa_smtp_client_failures
  Tests the following:
  - a write the peer resets is CLOSED, the reset kept as the cause
  - a write that fails otherwise is TRANSPORT, its error kept
  - a write that takes nothing is CLOSED, as net.h's d_net_write_all says
  - a reply cut off by a stalled peer is TIMED_OUT, the timeout kept
  - a greeting other than 220 is REJECTED, and its reply kept; a peer that
    hangs up at once is CLOSED, with no cause beyond the end of its stream
*/
bool
d_tests_sa_smtp_client_failures(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_smtp_breakage CASES[] =
    {
        { NULL,       true,  true,  false, D_SMTP_ERROR_CLOSED,
          D_NET_ERROR_CONNECTION_RESET },
        { NULL,       true,  false, false, D_SMTP_ERROR_TRANSPORT,
          D_NET_ERROR_UNKNOWN },
        { NULL,       false, false, true,  D_SMTP_ERROR_CLOSED,
          D_NET_ERROR_CLOSED },
        { "250-x\r\n", false, false, false, D_SMTP_ERROR_TIMED_OUT,
          D_NET_ERROR_TIMED_OUT }
    };
    const char* const        TEST   = "d_smtp_client (failures)";
    struct d_tests_net_peer* peer   = d_tests_smtp_peer("554 go away\r\n");
    struct d_tests_net_peer* gone   = d_tests_smtp_peer("");
    struct d_smtp_client     client = { .utf8 = false };
    bool                     broken = true;
    bool                     result = true;

    // each breakage, in turn
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        broken = ( (d_tests_smtp_breaks(&CASES[i])) &&
                   (broken) );
    }

    d_smtp_client_init(&client);

    const bool refused = ( (peer)                    &&
                           (d_smtp_client_attach(
                                &client,
                                d_tests_net_peer_connection(peer,
                                                            true),
                                false) == D_SMTP_OK) &&
                           (d_smtp_client_greeting(&client) ==
                            D_SMTP_ERROR_REJECTED)   &&
                           (client.reply.code == 554u) );

    d_smtp_client_close(&client);

    const bool hung_up = ( (gone)                    &&
                           (d_smtp_client_attach(
                                &client,
                                d_tests_net_peer_connection(gone,
                                                            true),
                                false) == D_SMTP_OK) &&
                           (d_smtp_client_greeting(&client) ==
                            D_SMTP_ERROR_CLOSED)     &&
                           (client.reader.net_error == D_NET_OK) );

    result = d_tests_net_assert(broken,
                                TEST,
                                "each broken write or read, and its cause",
                                _counter) && result;
    result = d_tests_net_assert( ( (refused) &&
                                   (hung_up) ),
                                 TEST,
                                 "a refused greeting, and a peer gone at once",
                                 _counter) && result;
    d_smtp_client_close(&client);
    d_tests_net_peer_free(peer);
    d_tests_net_peer_free(gone);

    return result;
}

// edges
/*
d_tests_smtp_long_greeting
  A peer whose greeting is one line of 4100 bytes, longer than the line a
client reads into, said in two turns; it hangs up after.
*/
static struct d_tests_net_peer*
d_tests_smtp_long_greeting(
    void
)
{
    static unsigned char           first[D_TESTS_NET_PEER_TURN_BYTES] = { 0 };
    struct d_tests_net_peer* const peer = d_tests_net_peer_new();

    memset(first,
           'x',
           sizeof(first));
    memcpy(first,
           "220 ",
           4u);

    const bool said = ( (peer)                                      &&
                        (d_tests_net_peer_say_bytes(peer,
                                                    "",
                                                    first,
                                                    sizeof(first))) &&
                        (d_tests_net_peer_say(peer,
                                              "",
                                              "xxxx\r\n")) );

    // a peer that cannot say its greeting is no peer
    if (!said)
    {
        d_tests_net_peer_free(peer);

        return NULL;
    }

    peer->hangs_up = true;

    return peer;
}

/*
d_tests_smtp_truncated_greeting
  Whether a client takes the long greeting: its code intact, and the reply
marked truncated.
*/
static bool
d_tests_smtp_truncated_greeting(
    void
)
{
    struct d_tests_net_peer* const peer   = d_tests_smtp_long_greeting();
    struct d_smtp_client           client = { .utf8 = false };
    bool                           kept   = false;

    d_smtp_client_init(&client);

    // the greeting, read past the client's line
    if (peer)
    {
        kept = ( (d_smtp_client_attach(&client,
                                       d_tests_net_peer_connection(peer,
                                                                   true),
                                       false) == D_SMTP_OK)     &&
                 (d_smtp_client_greeting(&client) == D_SMTP_OK) &&
                 (client.reply.code == 220u)                    &&
                 (client.reply.truncated) );
        d_smtp_client_close(&client);
    }

    d_tests_net_peer_free(peer);

    return kept;
}

/*
d_tests_sa_smtp_client_replies
  Tests the following:
  - a greeting longer than the line a client reads into is kept truncated,
    its code intact
  - a reply's class is its first digit: a 299 or a 400 to DATA refuses it,
    the transaction left open, and a 354 goes on
  - a refusal of the message itself ends the transaction all the same
*/
bool
d_tests_sa_smtp_client_replies(
    struct d_test_counter* _counter
)
{
    const char* const              TEST     = "d_smtp_client (replies)";
    const struct d_smtp_envelope   envelope = { .sender = "a@example.org" };
    struct d_tests_net_peer* const peer     = d_tests_smtp_peer(
                                                  "220 x\r\n250 x\r\n"
                                                  "250 OK\r\n250 OK\r\n"
                                                  "299 odd\r\n400 odd\r\n"
                                                  "354 go\r\n554 no\r\n");
    struct d_smtp_client           client   = { .utf8 = false };
    bool                           classes  = false;
    bool                           result   = true;

    // a transaction whose DATA is refused by class twice, then taken
    if ( (peer) &&
         (d_tests_smtp_open(&client,
                            d_tests_net_peer_connection(peer,
                                                        true),
                            false) == D_SMTP_OK) )
    {
        classes = ( (d_smtp_client_mail(&client,
                                        &envelope,
                                        0u) == D_SMTP_OK)              &&
                    (d_smtp_client_rcpt(&client,
                                        "b@example.org") == D_SMTP_OK) &&
                    (d_smtp_client_data(&client,
                                        "x",
                                        1u) == D_SMTP_ERROR_REJECTED)  &&
                    (client.reply.code == 299u)                        &&
                    (d_smtp_client_data(&client,
                                        "x",
                                        1u) == D_SMTP_ERROR_REJECTED)  &&
                    (client.reply.code == 400u)                        &&
                    (client.state == D_SMTP_CLIENT_MAIL)               &&
                    (d_smtp_client_data(&client,
                                        "x",
                                        1u) == D_SMTP_ERROR_REJECTED)  &&
                    (client.reply.code == 554u)                        &&
                    (client.state == D_SMTP_CLIENT_READY)              &&
                    (d_tests_net_peer_sent(peer,
                                           "EHLO c.example\r\n"
                                           "MAIL FROM:<a@example.org>\r\n"
                                           "RCPT TO:<b@example.org>\r\n"
                                           "DATA\r\nDATA\r\nDATA\r\n"
                                           "x\r\n.\r\n")) );
        d_smtp_client_close(&client);
    }

    result = d_tests_net_assert(d_tests_smtp_truncated_greeting(),
                                TEST,
                                "a greeting past the line, kept truncated",
                                _counter) && result;
    result = d_tests_net_assert(classes,
                                TEST,
                                "a class is its first digit; 554 still ends",
                                _counter) && result;
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_smtp_command
  Appends `_verb`, `_address`, and the ">\r\n" that ends MAIL or RCPT.
*/
static void
d_tests_smtp_command(
    struct d_smtp_buffer* _out,
    const char*           _verb,
    const char*           _address
)
{
    (void)d_smtp_buffer_append_text(_out,
                                    _verb);
    (void)d_smtp_buffer_append_text(_out,
                                    _address);
    (void)d_smtp_buffer_append_text(_out,
                                    ">\r\n");

    return;
}

/*
d_tests_smtp_script_for
  What a peer that takes everything says to a pipelined session sending from
`_envelope` -- a greeting, an EHLO list of PIPELINING alone, and a positive
reply to every command -- into `_replies`; and what the session must write
into `_wire`, `_body` being the message as it must go, its end included.
*/
static void
d_tests_smtp_script_for(
    const struct d_smtp_envelope* _envelope,
    const char*                   _body,
    struct d_smtp_buffer*         _replies,
    struct d_smtp_buffer*         _wire
)
{
    (void)d_smtp_buffer_append_text(_replies,
                                    "220 x\r\n250-x\r\n250 PIPELINING\r\n"
                                    "250 OK\r\n");
    d_tests_smtp_command(_wire,
                         "EHLO c.example\r\nMAIL FROM:<",
                         _envelope->sender);

    // each recipient's RCPT, and its reply
    for (size_t i = 0; i < _envelope->recipient_count; ++i)
    {
        (void)d_smtp_buffer_append_text(_replies,
                                        "250 OK\r\n");
        d_tests_smtp_command(_wire,
                             "RCPT TO:<",
                             _envelope->recipients[i]);
    }

    (void)d_smtp_buffer_append_text(_replies,
                                    "354 go\r\n250 done\r\n221 bye\r\n");
    (void)d_smtp_buffer_append_text(_wire,
                                    "DATA\r\n");
    (void)d_smtp_buffer_append_text(_wire,
                                    _body);
    (void)d_smtp_buffer_append_text(_wire,
                                    "QUIT\r\n");

    return;
}

/*
d_tests_smtp_sent_as
  Sends `_message` from `_envelope` as d_tests_smtp_script_for has a peer
take it, then quits: whether each step succeeded, the transaction ending
with the client ready for another, and the session wrote exactly what it
must.
*/
static bool
d_tests_smtp_sent_as(
    const struct d_smtp_envelope* _envelope,
    const char*                   _message,
    size_t                        _length,
    const char*                   _body
)
{
    static char          script[D_TESTS_NET_PEER_TURN_BYTES] = { 0 };
    static char          wire[D_TESTS_NET_PEER_OUTPUT_BYTES] = { 0 };
    struct d_smtp_buffer replies  = { .data = NULL };
    struct d_smtp_buffer expected = { .data = NULL };
    struct d_smtp_client client   = { .utf8 = false };
    bool                 sent     = false;

    d_smtp_buffer_init(&replies,
                       script,
                       sizeof(script));
    d_smtp_buffer_init(&expected,
                       wire,
                       sizeof(wire));
    d_tests_smtp_script_for(_envelope,
                            _body,
                            &replies,
                            &expected);

    struct d_tests_net_peer* const peer = d_tests_smtp_peer(script);

    // the whole session: one transaction, then QUIT
    if ( (peer) &&
         (d_tests_smtp_open(&client,
                            d_tests_net_peer_connection(peer,
                                                        true),
                            false) == D_SMTP_OK) )
    {
        sent = ( (d_smtp_client_send(&client,
                                     _envelope,
                                     _message,
                                     _length,
                                     NULL) == D_SMTP_OK)    &&
                 (client.state == D_SMTP_CLIENT_READY)      &&
                 (d_smtp_client_quit(&client) == D_SMTP_OK) &&
                 (d_tests_net_peer_sent(peer,
                                        expected.data)) );
        d_smtp_client_close(&client);
    }

    d_tests_net_peer_free(peer);

    return sent;
}

/*
d_tests_sa_smtp_client_messages
  Tests the following:
  - a message that opens with a dot has it doubled, and one that opens with
    a bare LF has it made CR LF: a message opens at a line's start
  - a message larger than one write goes whole, and in order
*/
bool
d_tests_sa_smtp_client_messages(
    struct d_test_counter* _counter
)
{
    static const char* const     RECIPIENTS[] = { "b@example.org" };
    static char                  large[(130u * 81u) + 1u] = { 0 };
    static char                  body[(130u * 81u) + 4u]  = { 0 };
    const char* const            TEST     = "d_smtp_client (messages)";
    const struct d_smtp_envelope envelope =
    {
        .sender          = "a@example.org",
        .recipients      = RECIPIENTS,
        .recipient_count = 1u
    };
    bool                         result   = true;

    // 130 lines of 79 'm's; then the same with the end of the data
    for (size_t i = 0; i < 130u; ++i)
    {
        memset(large + (i * 81u),
               'm',
               79u);
        memcpy(large + (i * 81u) + 79u,
               "\r\n",
               2u);
    }

    memcpy(body,
           large,
           sizeof(large) - 1u);
    memcpy(body + sizeof(large) - 1u,
           ".\r\n",
           4u);
    result = d_tests_net_assert(
                 ( (d_tests_smtp_sent_as(&envelope,
                                         ".dot\r\n",
                                         6u,
                                         "..dot\r\n.\r\n")) &&
                   (d_tests_smtp_sent_as(&envelope,
                                         "\nx",
                                         2u,
                                         "\r\nx\r\n.\r\n")) ),
                 TEST,
                 "a message opens at a line's start",
                 _counter) && result;
    result = d_tests_net_assert(d_tests_smtp_sent_as(&envelope,
                                                     large,
                                                     sizeof(large) - 1u,
                                                     body),
                                TEST,
                                "larger than one write, whole and in order",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_smtp_client_batches
  Tests the following:
  - a pipelined batch larger than one write is written as it fills, and
    nothing in it is lost or repeated
  - a command that exactly fills what is left of a write goes with the
    next: MAIL and every RCPT take 64 bytes here, so the 127th RCPT fills
    the last 64 of the first
*/
bool
d_tests_sa_smtp_client_batches(
    struct d_test_counter* _counter
)
{
    static char                  sender[51u]      = { 0 };
    static char                  names[130u][53u] = { { 0 } };
    static const char*           recipients[130u] = { NULL };
    const struct d_smtp_envelope envelope         =
    {
        .sender          = sender,
        .recipients      = recipients,
        .recipient_count = 130u
    };

    memset(sender,
           's',
           38u);
    memcpy(sender + 38u,
           "@example.org",
           13u);

    // rNNN, at 39 'd's and .example
    for (size_t i = 0; i < 130u; ++i)
    {
        (void)snprintf(names[i],
                       sizeof(names[i]),
                       "r%03u@",
                       (unsigned int)i);
        memset(names[i] + 5u,
               'd',
               39u);
        memcpy(names[i] + 44u,
               ".example",
               9u);
        recipients[i] = names[i];
    }

    return d_tests_net_assert(d_tests_smtp_sent_as(&envelope,
                                                   "x\r\n",
                                                   3u,
                                                   "x\r\n.\r\n"),
                              "d_smtp_client (batches)",
                              "past one write, and exactly at its end",
                              _counter);
}
