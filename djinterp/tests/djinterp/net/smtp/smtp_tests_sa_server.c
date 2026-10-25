/*******************************************************************************
* djinterp [net]                                          smtp_tests_sa_server.c
*
* djinterp SMTP kernel tests -- server sessions.
*   Standalone tests of smtp_server.c over net_tests_peer.h's scripted peer:
* dot removal, strict end-of-data framing, limits, policy, every reply,
* AUTH and the ways an exchange ends, the session's arguments and contexts,
* and the connection failing beneath it, in the middle of SASL among other
* places.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_sa_server.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./smtp_tests_sa.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memcpy, memset, strcmp, strstr
// djinterp
#include "../net_tests_sa_support.h"  // d_tests_net_mock


// SCRIPTED_COMMANDS, SCRIPTED_REPLIES
//   constant: what d_tests_sa_smtp_server_script's client says, all at once,
// and what the server must answer, in order.
static const char SCRIPTED_COMMANDS[] =
    "EHLO client.example\r\n"
    "MAIL FROM:<alice@example.org> SIZE=50\r\n"
    "RCPT TO:<bob@example.com>\r\nRCPT TO:<reject@example.com>\r\n"
    "DATA\r\n"
    "Subject: t\r\n\r\n..leading dot\r\nsmuggle\n.\nstill data\r\n.\r\n"
    "MAIL FROM:<> BODY=8BITMIME\r\nRCPT TO:<postmaster>\r\n"
    "RSET\r\nXYZZY\r\n"
    "MAIL FROM:<x@example.com> SIZE=999999999\r\n"
    "AUTH PLAIN AGFsaWNlAHMzY3JldA==\r\nQUIT\r\n";
static const char SCRIPTED_REPLIES[] =
    "220 mail.example.com ESMTP ready\r\n"
    "250-mail.example.com Hello client.example\r\n"
    "250-PIPELINING\r\n250-SIZE 10485760\r\n250-8BITMIME\r\n"
    "250-ENHANCEDSTATUSCODES\r\n250 SMTPUTF8\r\n"
    "250 2.1.0 Sender OK\r\n250 2.1.5 Recipient OK\r\n"
    "550 5.1.1 Recipient rejected\r\n"
    "354 Start mail input; end with <CRLF>.<CRLF>\r\n"
    "250 2.0.0 Message accepted\r\n"
    "250 2.1.0 Sender OK\r\n250 2.1.5 Recipient OK\r\n"
    "250 2.0.0 Reset state\r\n"
    "500 5.5.2 Command not recognized\r\n"
    "552 5.3.4 Message size exceeds fixed maximum message size\r\n"
    "502 5.5.1 AUTH not available\r\n"
    "221 2.0.0 Closing connection\r\n";

/*
d_tests_sa_smtp_server_script
  Tests the following:
  - the greeting and the EHLO list (no STARTTLS or AUTH without their
                                    prerequisites)
  - a pipelined transaction with a refused recipient
  - transparency dots are removed, bare LFs are normalized, and a dot framed
    by bare LFs is data, not the end marker
  - the null sender, postmaster, RSET, unknown commands, SIZE above the
    limit, AUTH without a verifier, and QUIT
*/
bool
d_tests_sa_smtp_server_script(
    struct d_test_counter* _counter
)
{
    const char* const              TEST   =
        "d_smtp_server_session (scripted)";
    struct d_tests_smtp_inbox      inbox  = { .messages = 0u };
    struct d_tests_net_peer* const peer   =
        d_tests_smtp_peer(SCRIPTED_COMMANDS);
    struct d_smtp_server_config    config = { .domain = NULL };
    bool                           result = true;

    if (!d_tests_net_assert( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _counter))
    {
        return false;
    }

    d_smtp_server_config_init(&config);
    config.domain = "mail.example.com";

    const enum d_smtp_error served = d_tests_smtp_serve(
                                         d_tests_net_peer_connection(peer,
                                                                     true),
                                         &config,
                                         &inbox,
                                         false);

    result = d_tests_net_assert( (served == D_SMTP_OK),
                                 TEST,
                                 "the session ends cleanly at QUIT",
                                 _counter) && result;
    result = d_tests_net_assert( (d_tests_net_peer_sent(peer,
                                                        SCRIPTED_REPLIES)),
                                 TEST,
                                 "every reply is exactly as expected",
                                 _counter) && result;
    result = d_tests_net_assert(
                 ( (inbox.messages == 1u)             &&
                   (inbox.recipients == 1u)           &&
                   (strcmp(inbox.sender,
                           "alice@example.org") == 0) &&
                   (strcmp(inbox.data,
                           "Subject: t\r\n\r\n.leading dot\r\n"
                           "smuggle\r\n.\r\nstill data\r\n") == 0) ),
                 TEST,
                 "dots unstuffed, bare LFs normalized, no early end",
                 _counter) && result;
    result = d_tests_net_assert( (peer->closes == 0),
                                 TEST,
                                 "the connection is left to its owner",
                                 _counter) && result;
    d_tests_net_peer_free(peer);

    return result;
}

/*
d_tests_smtp_auth_script
  Builds the AUTH conversation for d_tests_sa_smtp_server_auth.
*/
static void
d_tests_smtp_auth_script(
    struct d_smtp_buffer* _text
)
{
    (void)d_smtp_buffer_append_text(_text,
                                    "EHLO c.example\r\nAUTH PLAIN ");
    (void)d_smtp_buffer_append_base64(_text,
                                      "\0alice\0wrong",
                                      12u);
    (void)d_smtp_buffer_append_text(_text,
                                    "\r\nAUTH LOGIN\r\n");
    (void)d_smtp_buffer_append_base64(_text,
                                      "alice",
                                      5u);
    (void)d_smtp_buffer_append_text(_text,
                                    "\r\n");
    (void)d_smtp_buffer_append_base64(_text,
                                      "s3cret",
                                      6u);
    (void)d_smtp_buffer_append_text(_text,
                                    "\r\nAUTH PLAIN\r\n"
                                    "MAIL FROM:<alice@example.org>\r\n"
                                    "QUIT\r\n");

    return;
}

/*
d_tests_smtp_serve_text
  One session with AUTH over a peer that says `_script`; true when it ends
at QUIT and every check on its output holds: each of `_present` written,
none of `_absent`.
*/
static bool
d_tests_smtp_serve_text(
    const char*                        _script,
    const struct d_smtp_server_config* _config,
    const char* const*                 _present,
    const char* const*                 _absent
)
{
    struct d_tests_smtp_inbox inbox  = { .messages = 0u };
    struct d_tests_net_peer*  peer   = d_tests_smtp_peer(_script);
    bool                      served = false;

    if (peer)
    {
        const char* const output = (const char*)peer->output;

        served = (d_tests_smtp_serve(d_tests_net_peer_connection(peer,
                                                                 true),
                                     _config,
                                     &inbox,
                                     true) == D_SMTP_OK);

        // what must be there, and what must not
        for (size_t i = 0; (_present) && (_present[i]); ++i)
        {
            served = ( (served) &&
                       (strstr(output,
                               _present[i]) != NULL) );
        }

        for (size_t i = 0; (_absent) && (_absent[i]); ++i)
        {
            served = ( (served) &&
                       (strstr(output,
                               _absent[i]) == NULL) );
        }
    }

    d_tests_net_peer_free(peer);

    return served;
}

/*
d_tests_sa_smtp_server_auth
  Tests the following:
  - over an encrypted session AUTH is offered; a wrong password draws 535
    and LOGIN's prompts lead to 235; a second AUTH draws 503
  - over plaintext AUTH is not offered, and asking draws 538
*/
bool
d_tests_sa_smtp_server_auth(
    struct d_test_counter* _counter
)
{
    static const char* const    SECURE[] =
    {
        "250 AUTH PLAIN LOGIN\r\n",
        "535 5.7.8 Authentication credentials",
        "334 VXNlcm5hbWU6\r\n334 UGFzc3dvcmQ6\r\n"
        "235 2.7.0 Authentication successful\r\n"
        "503 5.5.1 Bad sequence of commands\r\n"
        "250 2.1.0 Sender OK\r\n",
        NULL
    };
    static const char* const    NOT_SECURE[] = { "STARTTLS",
                                                 NULL };
    static const char* const    REFUSED[]    =
        { "538 5.7.11 Encryption required",
          NULL };
    static const char* const    NOT_OFFERED[] = { "AUTH PLAIN",
                                                  NULL };
    const char* const           TEST          =
        "d_smtp_server_session (AUTH)";
    char                        storage[1024] = { 0 };
    struct d_smtp_buffer        text          =
    {
        .data     = storage,
        .capacity = sizeof(storage),
        .length   = 0
    };
    struct d_smtp_server_config config        = { .domain = NULL };
    bool                        result        = true;

    d_tests_smtp_auth_script(&text);
    d_smtp_server_config_init(&config);
    config.secure = true;
    result = d_tests_net_assert(d_tests_smtp_serve_text(storage,
                                                        &config,
                                                        SECURE,
                                                        NOT_SECURE),
                                TEST,
                                "535, then LOGIN succeeds, then 503",
                                _counter) && result;
    config.secure = false;
    result = d_tests_net_assert(
                 d_tests_smtp_serve_text("EHLO c.example\r\n"
                                         "AUTH PLAIN AGFiAGM=\r\nQUIT\r\n",
                                         &config,
                                         REFUSED,
                                         NOT_OFFERED),
                 TEST,
                 "no AUTH in the clear: 538",
                 _counter) && result;

    return result;
}

/*
d_tests_smtp_long_answer
  A session, secure and with AUTH, over a client that answers AUTH LOGIN's
first prompt with 4100 'A's, the first 4096 a turn of their own: the
answer is too long for the line it is read into, so the server must say it
cannot decode it, and stay in step for the QUIT that follows.
*/
static bool
d_tests_smtp_long_answer(
    void
)
{
    static unsigned char           answer[D_TESTS_NET_PEER_TURN_BYTES] = { 0 };
    struct d_tests_net_peer* const peer   = d_tests_net_peer_new();
    struct d_tests_smtp_inbox      inbox  = { .messages = 0u };
    struct d_smtp_server_config    config = { .domain = NULL };
    bool                           ended  = false;

    memset(answer,
           'A',
           sizeof(answer));
    d_smtp_server_config_init(&config);
    config.secure = true;

    // EHLO and AUTH, then the answer, past a turn and past the line
    if ( (peer)                                                     &&
         (d_tests_net_peer_say(peer,
                               "",
                               "EHLO c.example\r\nAUTH LOGIN\r\n")) &&
         (d_tests_net_peer_say_bytes(peer,
                                     "334 VXNlcm5hbWU6\r\n",
                                     answer,
                                     sizeof(answer)))               &&
         (d_tests_net_peer_say(peer,
                               "",
                               "AAAA\r\nQUIT\r\n")) )
    {
        ended = ( (d_tests_smtp_serve(d_tests_net_peer_connection(peer,
                                                                  true),
                                      &config,
                                      &inbox,
                                      true) == D_SMTP_OK) &&
                  (strstr((const char*)peer->output,
                          "334 VXNlcm5hbWU6\r\n"
                          "501 5.5.2 Cannot decode response\r\n"
                          "221 2.0.0 Closing connection\r\n") != NULL) );
    }

    d_tests_net_peer_free(peer);

    return ended;
}

/*
d_tests_sa_smtp_server_sasl_ends
  Tests the following:
  - a client that cancels AUTH with "*" draws 501 5.7.0, and the session
    goes on
  - an answer longer than the line it is read into, or one that decodes to
    more than a user name may hold, draws 501 5.5.2, as an undecodable one
    does, and the stream stays in step
*/
bool
d_tests_sa_smtp_server_sasl_ends(
    struct d_test_counter* _counter
)
{
    static const char* const    CANCELLED[] =
        { "334 VXNlcm5hbWU6\r\n501 5.7.0 Authentication cancelled\r\n"
          "221 2.0.0 Closing connection\r\n",
          NULL };
    static const char* const    UNDECODED[] =
        { "334 VXNlcm5hbWU6\r\n501 5.5.2 Cannot decode response\r\n"
          "221 2.0.0 Closing connection\r\n",
          NULL };
    static char                 oversized[512] = { 0 };
    const char* const           TEST           =
        "d_smtp_server_session (SASL ends)";
    struct d_smtp_server_config config         = { .domain = NULL };
    bool                        result         = true;

    // a user name of 300 bytes, in 400 characters of base64
    memcpy(oversized,
           "EHLO c.example\r\nAUTH LOGIN\r\n",
           28u);
    memset(oversized + 28u,
           'A',
           400u);
    memcpy(oversized + 428u,
           "\r\nQUIT\r\n",
           9u);
    d_smtp_server_config_init(&config);
    config.secure = true;
    result = d_tests_net_assert(
                 d_tests_smtp_serve_text("EHLO c.example\r\nAUTH LOGIN\r\n"
                                         "*\r\nQUIT\r\n",
                                         &config,
                                         CANCELLED,
                                         NULL),
                 TEST,
                 "a cancelled exchange: 501 5.7.0, then QUIT",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_smtp_long_answer()) &&
                   (d_tests_smtp_serve_text(oversized,
                                            &config,
                                            UNDECODED,
                                            NULL)) ),
                 TEST,
                 "past the line, or past a user name: 501 5.5.2",
                 _counter) && result;

    return result;
}

// d_tests_smtp_bad_config
//   struct: what one configuration a session must refuse differs in.
struct d_tests_smtp_bad_config
{
    const char*                 domain;
    size_t                      max_message_size;
    size_t                      max_recipients;
    const struct d_ssl_context* tls;
};

/*
d_tests_smtp_refused_config
  Whether a session over `_peer` with the defaults, changed as `_bad` says,
is refused before anything is written.
*/
static bool
d_tests_smtp_refused_config(
    struct d_tests_net_peer*              _peer,
    const struct d_tests_smtp_bad_config* _bad
)
{
    struct d_tests_smtp_inbox   inbox  = { .messages = 0u };
    struct d_smtp_server_config config = { .domain = NULL };

    d_smtp_server_config_init(&config);
    config.domain           = _bad->domain;
    config.max_message_size = _bad->max_message_size;
    config.max_recipients   = _bad->max_recipients;
    config.tls              = _bad->tls;

    return ( (d_tests_smtp_serve(d_tests_net_peer_connection(_peer,
                                                             true),
                                 &config,
                                 &inbox,
                                 false) ==
              D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (_peer->sent == 0u) );
}

/*
d_tests_smtp_refused_parts
  Whether a session is refused for a NULL or incomplete connection, a NULL
configuration, or a NULL handler.
*/
static bool
d_tests_smtp_refused_parts(
    struct d_tests_net_peer* _peer
)
{
    struct d_net_connection_vtable partial = *_peer->base.vtable;
    struct d_net_connection        broken  = { &partial };
    struct d_smtp_server_handler   handler = { .context = NULL };
    struct d_smtp_server_config    config  = { .domain = NULL };

    partial.is_open = NULL;
    d_smtp_server_config_init(&config);
    d_smtp_server_handler_init(&handler);
    d_smtp_server_config_init(NULL);
    d_smtp_server_handler_init(NULL);

    return ( (d_smtp_server_session(NULL,
                                    &config,
                                    &handler) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)                         &&
             (d_smtp_server_session(&broken,
                                    &config,
                                    &handler) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)                         &&
             (d_smtp_server_session(&_peer->base,
                                    NULL,
                                    &handler) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)                         &&
             (d_smtp_server_session(&_peer->base,
                                    &config,
                                    NULL) ==
              D_SMTP_ERROR_INVALID_ARGUMENT)                         &&
             (strcmp(config.domain,
                     "localhost") == 0)                              &&
             (config.max_message_size == D_SMTP_SERVER_MESSAGE_MAX)  &&
             (config.max_recipients == D_SMTP_SERVER_RECIPIENTS_MAX) &&
             (config.tls == NULL)                                    &&
             (handler.auth == NULL)                                  &&
             (_peer->sent == 0u) );
}

/*
d_tests_sa_smtp_server_arguments
  Tests the following:
  - a NULL or incomplete connection, a NULL configuration or handler are
    refused before anything is written
  - so is a configuration without a domain, with an invalid one or one in
    UTF-8, which a greeting cannot carry, or with a limit of zero; limits
    of one are taken
  - the defaults: "localhost", the D_SMTP_SERVER_* limits, no context, and
    no callbacks; NULL is ignored
*/
bool
d_tests_sa_smtp_server_arguments(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_smtp_bad_config BAD[] =
    {
        { NULL,         D_SMTP_SERVER_MESSAGE_MAX,
          D_SMTP_SERVER_RECIPIENTS_MAX, NULL },
        { "bad domain", D_SMTP_SERVER_MESSAGE_MAX,
          D_SMTP_SERVER_RECIPIENTS_MAX, NULL },
        { "b\xc3\xbc" "cher.example",
                        D_SMTP_SERVER_MESSAGE_MAX,
          D_SMTP_SERVER_RECIPIENTS_MAX, NULL },
        { "localhost",  0u,
          D_SMTP_SERVER_RECIPIENTS_MAX, NULL },
        { "localhost",  D_SMTP_SERVER_MESSAGE_MAX,
          0u,                           NULL }
    };
    const char* const              TEST   =
        "d_smtp_server_session (arguments)";
    struct d_tests_net_peer* const peer   = d_tests_smtp_peer("QUIT\r\n");
    struct d_tests_smtp_inbox      inbox  = { .messages = 0u };
    struct d_smtp_server_config    config = { .domain = NULL };

    if (!d_tests_net_assert( (peer != NULL),
                             TEST,
                             "a scripted peer",
                             _counter))
    {
        return false;
    }

    bool refused = d_tests_smtp_refused_parts(peer);

    // each configuration the session refuses
    for (size_t i = 0; i < sizeof(BAD) / sizeof(BAD[0]); ++i)
    {
        refused = ( (d_tests_smtp_refused_config(peer,
                                                 &BAD[i])) &&
                    (refused) );
    }

    d_smtp_server_config_init(&config);
    config.max_message_size = 1u;
    config.max_recipients   = 1u;

    const bool least  = (d_tests_smtp_serve(&peer->base,
                                            &config,
                                            &inbox,
                                            false) == D_SMTP_OK);
    const bool result = d_tests_net_assert(
                            refused,
                            TEST,
                            "every bad argument refused, nothing written",
                            _counter);

    d_tests_net_peer_free(peer);

    return d_tests_net_assert(least,
                              TEST,
                              "limits of one are taken",
                              _counter) && result;
}

/*
d_tests_sa_smtp_server_contexts
  Tests the following:
  - a context made for a client is refused before anything is written, and
    so is one never made, whatever its plan says
  - a server's is taken, and a session that never asks for STARTTLS runs
    without making a TLS session
*/
bool
d_tests_sa_smtp_server_contexts(
    struct d_test_counter* _counter
)
{
    const char* const              TEST        =
        "d_smtp_server_session (contexts)";
    struct d_tests_net_toy         toys[2]     = { { .alerts = false } };
    struct d_ssl_context           contexts[2] = { { .engine = NULL } };
    struct d_ssl_context           empty       =
    {
        .engine    = NULL,
        .plan.role = D_SSL_ROLE_SERVER
    };
    struct d_tests_net_peer* const peer        = d_tests_smtp_peer("QUIT\r\n");
    bool                           result      = true;
    const bool                     made        =
        ( (peer) &&
          (d_tests_smtp_toy_pair(contexts,
                                 toys,
                                 false)) );

    if (!d_tests_net_assert(made,
                            TEST,
                            "a peer and two toy contexts",
                            _counter))
    {
        d_tests_net_peer_free(peer);

        return false;
    }

    const struct d_tests_smtp_bad_config bad[] =
    {
        { "localhost", D_SMTP_SERVER_MESSAGE_MAX,
          D_SMTP_SERVER_RECIPIENTS_MAX, &contexts[1] },
        { "localhost", D_SMTP_SERVER_MESSAGE_MAX,
          D_SMTP_SERVER_RECIPIENTS_MAX, &empty }
    };
    const bool refused = ( (d_tests_smtp_refused_config(peer,
                                                        &bad[0])) &&
                           (d_tests_smtp_refused_config(peer,
                                                        &bad[1])) );
    struct d_tests_smtp_inbox   inbox  = { .messages = 0u };
    struct d_smtp_server_config config = { .domain = NULL };

    d_smtp_server_config_init(&config);
    config.tls = &contexts[0];

    const bool taken = ( (d_tests_smtp_serve(&peer->base,
                                             &config,
                                             &inbox,
                                             false) == D_SMTP_OK)  &&
                         (d_tests_net_peer_sent(
                              peer,
                              "220 localhost ESMTP ready\r\n"
                              "221 2.0.0 Closing connection\r\n")) &&
                         (toys[0].creates == 0) );

    result = d_tests_net_assert(refused,
                                TEST,
                                "a client's context, or none, is refused",
                                _counter) && result;
    result = d_tests_net_assert(taken,
                                TEST,
                                "a server's context is taken",
                                _counter) && result;
    d_ssl_context_destroy(&contexts[1]);
    d_ssl_context_destroy(&contexts[0]);
    d_tests_net_peer_free(peer);

    return result;
}

// d_tests_smtp_ending
//   struct: how a client ends a session, and how the session must end.
struct d_tests_smtp_ending
{
    const char*       script;    // what the client says
    bool              hangs_up;  // whether it hangs up after
    bool              fails;     // every write fails
    bool              resets;    // as a reset connection's would
    enum d_smtp_error status;    // how the session must end
    const char*       absent;    // a reply that must not be written
};

/*
d_tests_smtp_ends
  One session, secure and with AUTH, over a client that behaves as `_case`
says: it must end as the case expects, without writing what it must not.
*/
static bool
d_tests_smtp_ends(
    const struct d_tests_smtp_ending* _case
)
{
    struct d_tests_smtp_inbox   inbox  = { .messages = 0u };
    struct d_tests_net_peer*    peer   = d_tests_net_peer_new();
    struct d_smtp_server_config config = { .domain = NULL };
    bool                        ended  = false;

    d_smtp_server_config_init(&config);
    config.secure = true;

    if ( (peer) &&
         (d_tests_net_peer_say(peer,
                               "",
                               _case->script)) )
    {
        peer->hangs_up   = _case->hangs_up;
        peer->resets     = _case->resets;
        peer->fail_after = (_case->fails) ? 0u : peer->fail_after;
        ended = ( (d_tests_smtp_serve(d_tests_net_peer_connection(peer,
                                                                  true),
                                      &config,
                                      &inbox,
                                      true) == _case->status) &&
                  ( (!_case->absent) ||
                    (strstr((const char*)peer->output,
                            _case->absent) == NULL) ) );
    }

    d_tests_net_peer_free(peer);

    return ended;
}

/*
d_tests_smtp_protocol_mid_sasl
  A connection that answers AUTH LOGIN's prompt with a protocol failure of
its own -- a tunnel's malformed frame, say -- but would still take a reply:
the session must end on it, PROTOCOL, without the 501 it gives a client's
undecodable answer.
*/
static bool
d_tests_smtp_protocol_mid_sasl(
    void
)
{
    static const char           SCRIPT[]     = "EHLO c.example\r\n"
                                               "AUTH LOGIN\r\n";
    char                        output[1024] = { 0 };
    struct d_tests_smtp_inbox   inbox        = { .messages = 0u };
    struct d_tests_net_mock     mock         = { .input = NULL };
    struct d_smtp_server_config config       = { .domain = NULL };

    d_tests_net_mock_init(&mock,
                          false);
    d_tests_net_mock_input(&mock,
                           SCRIPT,
                           sizeof(SCRIPT) - 1u);
    d_tests_net_mock_output(&mock,
                            output,
                            sizeof(output) - 1u);
    mock.read_error = D_NET_ERROR_PROTOCOL;
    d_smtp_server_config_init(&config);
    config.secure = true;

    return ( (d_tests_smtp_serve(&mock.base,
                                 &config,
                                 &inbox,
                                 true) == D_SMTP_ERROR_PROTOCOL) &&
             (strstr(output,
                     "334 VXNlcm5hbWU6\r\n") != NULL)            &&
             (strstr(output,
                     "501") == NULL) );
}

/*
d_tests_sa_smtp_server_failures
  Tests the following:
  - a client that hangs up without QUIT ends the session CLOSED, and one
    that stalls ends it TIMED_OUT
  - a greeting the connection will not take ends it at once: CLOSED for a
    reset, TRANSPORT otherwise
  - a client that stalls in the middle of AUTH LOGIN ends the session
    TIMED_OUT, with no 501: the connection, not the client, failed; and so
    does a connection reporting a protocol failure of its own, PROTOCOL
  - an undecodable SASL answer ends only the exchange: 501, then QUIT
*/
bool
d_tests_sa_smtp_server_failures(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_smtp_ending CASES[] =
    {
        // script                             hangs  fails  resets
        { "EHLO c.example\r\n",               true,  false, false,
          D_SMTP_ERROR_CLOSED,    NULL },
        { "EHLO c.example\r\n",               false, false, false,
          D_SMTP_ERROR_TIMED_OUT, NULL },
        { "",                                 true,  true,  true,
          D_SMTP_ERROR_CLOSED,    NULL },
        { "",                                 true,  true,  false,
          D_SMTP_ERROR_TRANSPORT, NULL },
        { "EHLO c.example\r\nAUTH LOGIN\r\n", false, false, false,
          D_SMTP_ERROR_TIMED_OUT, "501" }
    };
    static const char* const CANNOT[] =
        { "334 VXNlcm5hbWU6\r\n501 5.5.2 Cannot decode response\r\n"
          "221 2.0.0 Closing connection\r\n",
          NULL };
    const char* const           TEST   = "d_smtp_server_session (failures)";
    struct d_smtp_server_config config = { .domain = NULL };
    bool                        ended  = true;
    bool                        result = true;

    // each way a session ends early
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        ended = ( (d_tests_smtp_ends(&CASES[i])) &&
                  (ended) );
    }

    d_smtp_server_config_init(&config);
    config.secure = true;
    result = d_tests_net_assert(ended,
                                TEST,
                                "each early end, as the connection reported",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_smtp_protocol_mid_sasl(),
                                TEST,
                                "a connection's own PROTOCOL ends AUTH and all",
                                _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_serve_text("EHLO c.example\r\nAUTH LOGIN\r\n"
                                         "!!!\r\nQUIT\r\n",
                                         &config,
                                         CANNOT,
                                         NULL),
                 TEST,
                 "an undecodable answer ends the exchange, not the session",
                 _counter) && result;

    return result;
}
