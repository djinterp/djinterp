/*******************************************************************************
* djinterp [c]                                           smtp_tests_sa_session.c
*
* djinterp SMTP kernel tests -- sessions.
*   Standalone tests of the client and server engines. Scripted transports
* check the exact bytes on the wire: pipelining, SASL, the HELO fallback,
* recipient refusal and reset, the STARTTLS response-injection guard, the
* refusal to authenticate in the clear, command-injection rejection, and on
* the server side dot removal, strict end-of-data framing, limits, policy and
* every reply. Live loopback sessions then run the client against the server
* over real sockets: plaintext, STARTTLS with authentication, implicit TLS
* verified against an IP address, and a certificate-name mismatch.
*
*
* path:      /tests/djinterp/c/net/smtp/smtp_tests_sa_session.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
// enable POSIX.1-2008 for threads and getpid() under a strict C compiler
#if !defined(_WIN32) && !defined(_WIN64)
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif  // _POSIX_C_SOURCE
#endif

#include "./smtp_tests_sa.h"
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // fopen, printf, remove, snprintf
#include <string.h>   // memcpy, memset, strcmp, strlen, strncmp, strstr

#if D_ENV_SMTP_CAN_SERVE
    // posix
    #include <pthread.h>  // pthread_create, pthread_join
    #include <unistd.h>   // getpid
#endif

#if D_ENV_SMTP_CAN_TLS
    // openssl
    #include <openssl/evp.h>     // EVP_PKEY_*
    #include <openssl/pem.h>     // PEM_write_*
    #include <openssl/x509.h>    // X509_*
    #include <openssl/x509v3.h>  // X509V3_EXT_conf_nid
#endif


// d_tests_smtp_script
//   struct: a scripted peer -- what it says, in order, and what it heard.
struct d_tests_smtp_script
{
    const char* input;       // what the peer sends
    const char* after_tls;   // what it sends once "upgraded"
    size_t      position;
    size_t      chunk;       // bytes per read; 0 for as many as fit
    size_t      writes;
    size_t      written;
    bool        upgraded;
    char        output[8192];
};

// d_tests_smtp_inbox
//   struct: what a server handler saw.
struct d_tests_smtp_inbox
{
    size_t messages;
    size_t recipients;
    char   sender[256];
    char   user[64];
    char   data[4096];
};

// d_tests_smtp_case
//   struct: one entry of the suite.
struct d_tests_smtp_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};


/*
d_tests_smtp_script_read
  Hands out the script in `chunk`-sized pieces and hangs up at its end.
*/
static enum d_smtp_error
d_tests_smtp_script_read(
    void*   _context,
    void*   _buffer,
    size_t  _capacity,
    size_t* _received
)
{
    struct d_tests_smtp_script* script    = _context;
    const size_t                remaining = strlen(script->input +
                                                   script->position);
    size_t                      count     = (remaining < _capacity)
                                            ? remaining
                                            : _capacity;

    if (remaining == 0)
    {
        return D_SMTP_ERROR_CLOSED;
    }

    if ( (script->chunk > 0) &&
         (count > script->chunk) )
    {
        count = script->chunk;
    }

    memcpy(_buffer,
           script->input + script->position,
           count);
    script->position += count;
    *_received        = count;

    return D_SMTP_OK;
}

/*
d_tests_smtp_script_write
  Records everything written, and how many writes it took.
*/
static enum d_smtp_error
d_tests_smtp_script_write(
    void*       _context,
    const void* _data,
    size_t      _length,
    size_t*     _sent
)
{
    struct d_tests_smtp_script* script = _context;
    const size_t                room   = sizeof(script->output) - 1u -
                                         script->written;
    const size_t                count  = (_length < room) ? _length : room;

    memcpy(script->output + script->written,
           _data,
           count);
    script->written                 += count;
    script->output[script->written]  = '\0';
    script->writes                  += 1;
    *_sent                           = _length;

    return D_SMTP_OK;
}

/*
d_tests_smtp_script_starttls
  A stand-in handshake: the peer switches to its post-TLS script.
*/
static enum d_smtp_error
d_tests_smtp_script_starttls(
    void*                            _context,
    const struct d_smtp_tls_options* _options
)
{
    struct d_tests_smtp_script* script = _context;

    (void)_options;
    script->upgraded = true;
    script->input    = (script->after_tls) ? script->after_tls : "";
    script->position = 0;

    return D_SMTP_OK;
}

/*
d_tests_smtp_script_init
  A fresh script and a transport that plays it.
*/
static void
d_tests_smtp_script_init(
    struct d_tests_smtp_script* _script,
    const char*                 _input,
    struct d_smtp_transport*    _transport
)
{
    memset(_script,
           0,
           sizeof(*_script));
    _script->input       = _input;
    _transport->context  = _script;
    _transport->read     = d_tests_smtp_script_read;
    _transport->write    = d_tests_smtp_script_write;
    _transport->starttls = d_tests_smtp_script_starttls;
    _transport->close    = NULL;

    return;
}

/*
d_tests_smtp_open
  Attaches a client to a script and runs the greeting and EHLO.
*/
static enum d_smtp_error
d_tests_smtp_open(
    struct d_smtp_client*          _client,
    const struct d_smtp_transport* _transport,
    bool                           _secure
)
{
    d_smtp_client_init(_client);

    enum d_smtp_error result = d_smtp_client_attach(_client,
                                                    _transport,
                                                    _secure);

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_greeting(_client);
    }

    return (result == D_SMTP_OK) ? d_smtp_client_hello(_client,
                                                       "c.example")
                                 : result;
}

/*
d_tests_sa_smtp_client_script
  Tests the following:
  - greeting, EHLO and AUTH PLAIN over an already-secure transport
  - MAIL, both RCPTs and DATA leave in a single pipelined write, with SIZE
  - the message is dot-stuffed and terminated
  - a refused recipient is reported while the message is still delivered
*/
bool
d_tests_sa_smtp_client_script(
    struct d_test_counter* _counter
)
{
    static const char* const RECIPIENTS[] = { "bob@example.com",
                                              "nobody@example.com" };
    static const char        MESSAGE[]    = "Subject: hi\r\n\r\n.dot\r\nbody";
    static const char        PEER[]       =
        "220 mail.example.com ESMTP\r\n"
        "250-mail.example.com Hello\r\n"
        "250-PIPELINING\r\n"
        "250-SIZE 1000000\r\n"
        "250-AUTH PLAIN LOGIN\r\n"
        "250 ENHANCEDSTATUSCODES\r\n"
        "235 2.7.0 Authentication successful\r\n"
        "250 2.1.0 Sender OK\r\n"
        "250 2.1.5 Recipient OK\r\n"
        "550 5.1.1 No such user\r\n"
        "354 Go ahead\r\n"
        "250 2.0.0 Queued\r\n"
        "221 2.0.0 Bye\r\n";
    const char* const              TEST        = "d_smtp_client (pipelined)";
    const struct d_smtp_credentials credentials = { .user      = "alice",
                                                    .secret    = "s3cret",
                                                    .mechanism = 0u };
    struct d_smtp_envelope         envelope    = { .recipient_count = 2u };
    struct d_smtp_recipient_status statuses[2] = { { .code = 0u } };
    struct d_smtp_send_report      report      = { .recipients = statuses };
    struct d_tests_smtp_script     script;
    struct d_smtp_transport        transport;
    struct d_smtp_client           client;
    char                           expected[1024] = { 0 };
    struct d_smtp_buffer           text = { .data     = expected,
                                            .capacity = sizeof(expected),
                                            .length   = 0 };
    bool                           result = true;

    envelope.sender     = "alice@example.org";
    envelope.recipients = RECIPIENTS;
    (void)d_smtp_buffer_append_text(&text, "EHLO c.example\r\nAUTH PLAIN ");
    (void)d_smtp_base64_encode("\0alice\0s3cret", 13u, &text);
    (void)d_smtp_buffer_append_text(&text, "\r\nMAIL FROM:<alice@example.org>"
                                           " SIZE=");
    (void)d_smtp_buffer_append_number(&text, strlen(MESSAGE));
    (void)d_smtp_buffer_append_text(&text, "\r\nRCPT TO:<bob@example.com>\r\n"
                                           "RCPT TO:<nobody@example.com>\r\n"
                                           "DATA\r\nSubject: hi\r\n\r\n..dot"
                                           "\r\nbody\r\n.\r\nQUIT\r\n");

    d_tests_smtp_script_init(&script,
                             PEER,
                             &transport);
    script.chunk = 7u;

    enum d_smtp_error step = d_tests_smtp_open(&client,
                                               &transport,
                                               true);

    if (step == D_SMTP_OK)
    {
        step = d_smtp_client_authenticate(&client, &credentials);
    }

    if (step == D_SMTP_OK)
    {
        step = d_smtp_client_send(&client,
                                  &envelope,
                                  MESSAGE,
                                  strlen(MESSAGE),
                                  &report);
    }

    if (step == D_SMTP_OK)
    {
        step = d_smtp_client_quit(&client);
    }

    result = d_assert_standalone( (step == D_SMTP_OK),
                                  TEST,
                                  "the scripted session succeeds",
                                  _counter) && result;
    result = d_assert_standalone( (strcmp(script.output, expected) == 0),
                                  TEST,
                                  "exactly the expected bytes are written",
                                  _counter) && result;
    result = d_assert_standalone( (script.writes == 6u),
                                  TEST,
                                  "MAIL, RCPTs and DATA leave in one write",
                                  _counter) && result;
    result = d_assert_standalone( ( (report.accepted == 1u) &&
                                    (statuses[0].code == 250u) &&
                                    (statuses[1].code == 550u) &&
                                    (statuses[1].status.detail == 1u) &&
                                    (report.data_code == 250u) ),
                                  TEST,
                                  "the report records each recipient",
                                  _counter) && result;
    d_smtp_client_close(&client);

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
    static const char* const RECIPIENTS[] = { "d@example.net" };
    static const char        PEER[]       =
        "220 old.example.com\r\n"
        "502 5.5.1 Command not implemented\r\n"
        "250 old.example.com\r\n"
        "250 OK\r\n"
        "550 5.1.1 Unknown user\r\n"
        "250 OK\r\n";
    const char* const            TEST     = "d_smtp_client (serial)";
    const struct d_smtp_envelope envelope = { .sender = "a@example.org",
                                              .recipients = RECIPIENTS,
                                              .recipient_count = 1u };
    struct d_tests_smtp_script   script;
    struct d_smtp_transport      transport;
    struct d_smtp_client         client;
    bool                         result = true;

    d_tests_smtp_script_init(&script,
                             PEER,
                             &transport);

    const enum d_smtp_error opened = d_tests_smtp_open(&client,
                                                       &transport,
                                                       false);
    const enum d_smtp_error sent   = d_smtp_client_send(&client,
                                                        &envelope,
                                                        "x",
                                                        1u,
                                                        NULL);

    result = d_assert_standalone( ( (opened == D_SMTP_OK) &&
                                    (!client.capabilities.extended) ),
                                  TEST,
                                  "HELO follows a 502 to EHLO",
                                  _counter) && result;
    result = d_assert_standalone( ( (sent == D_SMTP_ERROR_REJECTED) &&
                                    (client.reply.code == 550u) &&
                                    (client.state == D_SMTP_CLIENT_READY) ),
                                  TEST,
                                  "the refusal survives the reset",
                                  _counter) && result;
    result = d_assert_standalone(
                 (strcmp(script.output,
                         "EHLO c.example\r\nHELO c.example\r\n"
                         "MAIL FROM:<a@example.org>\r\n"
                         "RCPT TO:<d@example.net>\r\nRSET\r\n") == 0),
                 TEST,
                 "commands go one at a time, ending with RSET",
                 _counter) && result;
    d_smtp_client_close(&client);

    return result;
}

/*
d_tests_smtp_guard_starttls
  STARTTLS twice: once with a reply injected behind the 220, which must be
refused, and once cleanly, which must upgrade and repeat EHLO.
*/
static bool
d_tests_smtp_guard_starttls(
    struct d_test_counter* _counter,
    const char*            _test
)
{
    const struct d_smtp_tls_options options = { .verify_peer = true };
    struct d_tests_smtp_script      script;
    struct d_smtp_transport         transport;
    struct d_smtp_client            client;
    bool                            result = true;

    d_tests_smtp_script_init(&script,
                             "220 x\r\n250-x\r\n250 STARTTLS\r\n"
                             "220 2.0.0 Ready\r\n250 2.0.0 injected\r\n",
                             &transport);
    (void)d_tests_smtp_open(&client, &transport, false);
    result = d_assert_standalone(
                 ( (d_smtp_client_starttls(&client, &options) ==
                    D_SMTP_ERROR_PROTOCOL) &&
                   (!script.upgraded) ),
                 _test,
                 "a reply injected behind STARTTLS's 220 is refused",
                 _counter) && result;
    d_smtp_client_close(&client);

    d_tests_smtp_script_init(&script,
                             "220 x\r\n250-x\r\n250 STARTTLS\r\n"
                             "220 2.0.0 Ready\r\n",
                             &transport);
    script.after_tls = "250-x\r\n250 AUTH PLAIN\r\n";
    (void)d_tests_smtp_open(&client, &transport, false);
    result = d_assert_standalone(
                 ( (d_smtp_client_starttls(&client, &options) == D_SMTP_OK) &&
                   (script.upgraded) &&
                   (client.secure) &&
                   (client.capabilities.extensions == D_SMTP_EXTENSION_AUTH) &&
                   (strcmp(script.output,
                           "EHLO c.example\r\nSTARTTLS\r\n"
                           "EHLO c.example\r\n") == 0) ),
                 _test,
                 "STARTTLS upgrades, forgets, and repeats EHLO",
                 _counter) && result;
    d_smtp_client_close(&client);

    return result;
}

/*
d_tests_sa_smtp_client_guards
  Tests the following:
  - the STARTTLS response-injection guard, and a clean upgrade
  - STARTTLS the server does not offer is unsupported
  - AUTH is refused on a plaintext channel, before anything is sent
  - CR and LF in a recipient cannot inject a command
*/
bool
d_tests_sa_smtp_client_guards(
    struct d_test_counter* _counter
)
{
    static const char* const EVIL[] =
        { "e@example.com>\r\nRCPT TO:<x@example.com" };
    const char* const               TEST        = "d_smtp_client (guards)";
    const struct d_smtp_tls_options options     = { .verify_peer = true };
    const struct d_smtp_credentials credentials = { .user   = "u",
                                                    .secret = "p" };
    const struct d_smtp_envelope    envelope    = { .sender          = "",
                                                    .recipients      = EVIL,
                                                    .recipient_count = 1u };
    struct d_tests_smtp_script      script;
    struct d_smtp_transport         transport;
    struct d_smtp_client            client;
    bool                            result = d_tests_smtp_guard_starttls(
                                                 _counter,
                                                 TEST);

    d_tests_smtp_script_init(&script,
                             "220 x\r\n250 x\r\n",
                             &transport);
    (void)d_tests_smtp_open(&client, &transport, false);
    result = d_assert_standalone( (d_smtp_client_starttls(&client,
                                                          &options) ==
                                   D_SMTP_ERROR_UNSUPPORTED),
                                  TEST,
                                  "STARTTLS not offered is unsupported",
                                  _counter) && result;
    d_smtp_client_close(&client);

    d_tests_smtp_script_init(&script,
                             "220 x\r\n250-x\r\n250-PIPELINING\r\n"
                             "250 AUTH PLAIN\r\n",
                             &transport);
    (void)d_tests_smtp_open(&client, &transport, false);
    result = d_assert_standalone(
                 ( (d_smtp_client_authenticate(&client, &credentials) ==
                    D_SMTP_ERROR_INSECURE) &&
                   (d_smtp_client_send(&client, &envelope, "x", 1u, NULL) ==
                    D_SMTP_ERROR_INVALID) &&
                   (strcmp(script.output, "EHLO c.example\r\n") == 0) ),
                 TEST,
                 "no cleartext AUTH, no injected RCPT, nothing sent",
                 _counter) && result;
    d_smtp_client_close(&client);

    return result;
}

/*
d_tests_smtp_on_rcpt
  Refuses recipients whose local-part starts with "reject".
*/
static unsigned int
d_tests_smtp_on_rcpt(
    void*                             _context,
    const struct d_smtp_session_info* _session,
    const char*                       _recipient
)
{
    (void)_context;
    (void)_session;

    return (strncmp(_recipient, "reject", 6) == 0) ? 550u : 0u;
}

/*
d_tests_smtp_on_message
  Records a delivered message in the inbox.
*/
static unsigned int
d_tests_smtp_on_message(
    void*                             _context,
    const struct d_smtp_session_info* _session,
    const char*                       _data,
    size_t                            _length
)
{
    struct d_tests_smtp_inbox* inbox = _context;
    const size_t               kept  = (_length < sizeof(inbox->data) - 1u)
                                       ? _length
                                       : sizeof(inbox->data) - 1u;

    inbox->messages  += 1;
    inbox->recipients = _session->recipient_count;
    memcpy(inbox->data, _data, kept);
    inbox->data[kept] = '\0';
    (void)snprintf(inbox->sender,
                   sizeof(inbox->sender),
                   "%s",
                   _session->sender);
    (void)snprintf(inbox->user,
                   sizeof(inbox->user),
                   "%s",
                   (_session->user) ? _session->user : "");

    return 0u;
}

/*
d_tests_smtp_on_auth
  Accepts alice with the password s3cret.
*/
static bool
d_tests_smtp_on_auth(
    void*       _context,
    const char* _user,
    const char* _secret
)
{
    (void)_context;

    return ( (strcmp(_user, "alice") == 0) &&
             (strcmp(_secret, "s3cret") == 0) );
}

/*
d_tests_smtp_serve_script
  Runs a server session over a script with the test handlers.
*/
static enum d_smtp_error
d_tests_smtp_serve_script(
    struct d_tests_smtp_script*        _script,
    const struct d_smtp_server_config* _config,
    struct d_tests_smtp_inbox*         _inbox,
    bool                               _with_auth
)
{
    struct d_smtp_transport      transport;
    struct d_smtp_server_handler handler;

    d_tests_smtp_script_init(_script,
                             _script->input,
                             &transport);
    d_smtp_server_handler_init(&handler);
    handler.context = _inbox;
    handler.rcpt    = d_tests_smtp_on_rcpt;
    handler.message = d_tests_smtp_on_message;
    handler.auth    = (_with_auth) ? d_tests_smtp_on_auth : NULL;

    return d_smtp_server_session(&transport,
                                 _config,
                                 &handler);
}

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
    static const char CLIENT[] =
        "EHLO client.example\r\n"
        "MAIL FROM:<alice@example.org> SIZE=50\r\n"
        "RCPT TO:<bob@example.com>\r\n"
        "RCPT TO:<reject@example.com>\r\n"
        "DATA\r\n"
        "Subject: t\r\n\r\n..leading dot\r\nsmuggle\n.\nstill data\r\n.\r\n"
        "MAIL FROM:<> BODY=8BITMIME\r\n"
        "RCPT TO:<postmaster>\r\n"
        "RSET\r\n"
        "XYZZY\r\n"
        "MAIL FROM:<x@example.com> SIZE=999999999\r\n"
        "AUTH PLAIN AGFsaWNlAHMzY3JldA==\r\n"
        "QUIT\r\n";
    static const char REPLIES[] =
        "220 mail.example.com ESMTP ready\r\n"
        "250-mail.example.com Hello client.example\r\n"
        "250-PIPELINING\r\n"
        "250-SIZE 10485760\r\n"
        "250-8BITMIME\r\n"
        "250-ENHANCEDSTATUSCODES\r\n"
        "250 SMTPUTF8\r\n"
        "250 2.1.0 Sender OK\r\n"
        "250 2.1.5 Recipient OK\r\n"
        "550 5.1.1 Recipient rejected\r\n"
        "354 Start mail input; end with <CRLF>.<CRLF>\r\n"
        "250 2.0.0 Message accepted\r\n"
        "250 2.1.0 Sender OK\r\n"
        "250 2.1.5 Recipient OK\r\n"
        "250 2.0.0 Reset state\r\n"
        "500 5.5.2 Command not recognized\r\n"
        "552 5.3.4 Message size exceeds fixed maximum message size\r\n"
        "502 5.5.1 AUTH not available\r\n"
        "221 2.0.0 Closing connection\r\n";
    const char* const           TEST   = "d_smtp_server_session (scripted)";
    struct d_tests_smtp_inbox   inbox  = { .messages = 0u };
    struct d_tests_smtp_script  script = { .input = CLIENT };
    struct d_smtp_server_config config;
    bool                        result = true;

    d_smtp_server_config_init(&config);
    config.domain = "mail.example.com";

    const enum d_smtp_error served = d_tests_smtp_serve_script(&script,
                                                               &config,
                                                               &inbox,
                                                               false);

    result = d_assert_standalone( (served == D_SMTP_OK),
                                  TEST,
                                  "the session ends cleanly at QUIT",
                                  _counter) && result;
    result = d_assert_standalone( (strcmp(script.output, REPLIES) == 0),
                                  TEST,
                                  "every reply is exactly as expected",
                                  _counter) && result;
    result = d_assert_standalone(
                 ( (inbox.messages == 1u) &&
                   (inbox.recipients == 1u) &&
                   (strcmp(inbox.sender, "alice@example.org") == 0) &&
                   (strcmp(inbox.data,
                           "Subject: t\r\n\r\n.leading dot\r\nsmuggle\r\n"
                           ".\r\nstill data\r\n") == 0) ),
                 TEST,
                 "dots unstuffed, bare LFs normalized, no early end",
                 _counter) && result;

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
    (void)d_smtp_buffer_append_text(_text, "EHLO c.example\r\nAUTH PLAIN ");
    (void)d_smtp_base64_encode("\0alice\0wrong", 12u, _text);
    (void)d_smtp_buffer_append_text(_text, "\r\nAUTH LOGIN\r\n");
    (void)d_smtp_base64_encode("alice", 5u, _text);
    (void)d_smtp_buffer_append_text(_text, "\r\n");
    (void)d_smtp_base64_encode("s3cret", 6u, _text);
    (void)d_smtp_buffer_append_text(_text, "\r\nAUTH PLAIN\r\n"
                                           "MAIL FROM:<alice@example.org>\r\n"
                                           "QUIT\r\n");

    return;
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
    const char* const           TEST          = "d_smtp_server_session (AUTH)";
    char                        storage[1024] = { 0 };
    struct d_smtp_buffer        text   = { .data     = storage,
                                           .capacity = sizeof(storage),
                                           .length   = 0 };
    struct d_tests_smtp_inbox   inbox  = { .messages = 0u };
    struct d_tests_smtp_script  script = { .input = storage };
    struct d_smtp_server_config config;
    bool                        result = true;

    d_tests_smtp_auth_script(&text);
    d_smtp_server_config_init(&config);
    config.secure = true;

    const bool secure =
        ( (d_tests_smtp_serve_script(&script, &config, &inbox, true) ==
           D_SMTP_OK) &&
          (strstr(script.output, "250 AUTH PLAIN LOGIN\r\n")) &&
          (!strstr(script.output, "STARTTLS")) &&
          (strstr(script.output, "535 5.7.8 Authentication credentials")) &&
          (strstr(script.output, "334 VXNlcm5hbWU6\r\n334 UGFzc3dvcmQ6\r\n"
                                 "235 2.7.0 Authentication successful\r\n"
                                 "503 5.5.1 Bad sequence of commands\r\n"
                                 "250 2.1.0 Sender OK\r\n")) );

    result = d_assert_standalone(secure,
                                 TEST,
                                 "535, then LOGIN succeeds, then 503",
                                 _counter) && result;

    config.secure = false;
    script.input  = "EHLO c.example\r\nAUTH PLAIN AGFiAGM=\r\nQUIT\r\n";

    const bool plain =
        ( (d_tests_smtp_serve_script(&script, &config, &inbox, true) ==
           D_SMTP_OK) &&
          (!strstr(script.output, "AUTH PLAIN")) &&
          (strstr(script.output, "538 5.7.11 Encryption required")) );

    result = d_assert_standalone(plain,
                                 TEST,
                                 "no AUTH in the clear: 538",
                                 _counter) && result;

    return result;
}


#if D_ENV_SMTP_CAN_SERVE

// d_tests_smtp_peer
//   struct: a server thread's setup and outcome.
struct d_tests_smtp_peer
{
    struct d_smtp_listener       listener;
    struct d_smtp_server_config  config;
    struct d_smtp_server_handler handler;
    struct d_tests_smtp_inbox    inbox;
    bool                         implicit_tls;
    enum d_smtp_error            result;
};

/*
d_tests_smtp_serve
  Server thread: accept one connection, negotiate implicit TLS if asked,
and run one session.
*/
static void*
d_tests_smtp_serve(
    void* _peer
)
{
    struct d_tests_smtp_peer* peer       = _peer;
    struct d_smtp_socket      connection;
    struct d_smtp_transport   transport  = { .context = NULL };

    d_smtp_socket_init(&connection);
    peer->result = d_smtp_listener_accept(&peer->listener,
                                          10000,
                                          10000,
                                          &connection);

    if ( (peer->result == D_SMTP_OK) &&
         (peer->implicit_tls) )
    {
        peer->result = d_smtp_socket_start_tls(&connection,
                                               &peer->config.tls);
    }

    if (peer->result == D_SMTP_OK)
    {
        d_smtp_socket_transport(&connection,
                                &transport);
        peer->result = d_smtp_server_session(&transport,
                                             &peer->config,
                                             &peer->handler);
    }

    d_smtp_socket_close(&connection);

    return NULL;
}

/*
d_tests_smtp_peer_start
  Opens a loopback listener on a system-chosen port and starts the server
thread on it.
*/
static bool
d_tests_smtp_peer_start(
    struct d_tests_smtp_peer* _peer,
    pthread_t*                _thread
)
{
    d_smtp_listener_init(&_peer->listener);
    d_smtp_server_handler_init(&_peer->handler);
    _peer->handler.context = &_peer->inbox;
    _peer->handler.rcpt    = d_tests_smtp_on_rcpt;
    _peer->handler.message = d_tests_smtp_on_message;
    _peer->handler.auth    = d_tests_smtp_on_auth;
    _peer->result          = D_SMTP_ERROR_STATE;

    return ( (d_smtp_listener_open(&_peer->listener,
                                   "127.0.0.1",
                                   0u) == D_SMTP_OK) &&
             (pthread_create(_thread,
                             NULL,
                             d_tests_smtp_serve,
                             _peer) == 0) );
}

/*
d_tests_smtp_deliver
  One client session against a started peer: connect, authenticate if asked,
send, and quit. Returns the first error.
*/
static enum d_smtp_error
d_tests_smtp_deliver(
    const struct d_smtp_client_options* _options,
    bool                                _authenticate,
    const char*                         _message
)
{
    static const char* const RECIPIENTS[] = { "bob@example.com",
                                              "carol@example.com" };
    const struct d_smtp_credentials credentials = { .user   = "alice",
                                                    .secret = "s3cret" };
    const struct d_smtp_envelope    envelope    = { .sender          =
                                                        "alice@example.org",
                                                    .recipients      =
                                                        RECIPIENTS,
                                                    .recipient_count = 2u };
    struct d_smtp_client            client;

    d_smtp_client_init(&client);

    enum d_smtp_error result = d_smtp_client_connect(&client,
                                                     _options);

    if ( (result == D_SMTP_OK) &&
         (_authenticate) )
    {
        result = d_smtp_client_authenticate(&client,
                                            &credentials);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_send(&client,
                                    &envelope,
                                    _message,
                                    strlen(_message),
                                    NULL);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_quit(&client);
    }

    d_smtp_client_close(&client);

    return result;
}

/*
d_tests_sa_smtp_loopback
  Tests the following:
  - a real client and server over loopback TCP, in plaintext
  - the message arrives with transparency undone and CRLF line endings
*/
bool
d_tests_sa_smtp_loopback(
    struct d_test_counter* _counter
)
{
    const char* const            TEST    = "loopback (plaintext)";
    struct d_tests_smtp_peer     peer    = { .implicit_tls = false };
    struct d_smtp_client_options options;
    pthread_t                    thread;
    bool                         result  = true;

    d_smtp_server_config_init(&peer.config);

    if (!d_tests_smtp_peer_start(&peer, &thread))
    {
        return d_assert_standalone(false,
                                   TEST,
                                   "the loopback server starts",
                                   _counter);
    }

    d_smtp_client_options_init(&options);
    options.host          = "127.0.0.1";
    options.port          = peer.listener.port;
    options.security      = D_SMTP_SECURITY_NONE;
    options.helo_domain   = "client.example";
    options.io_timeout_ms = 10000;

    const enum d_smtp_error sent = d_tests_smtp_deliver(
                                       &options,
                                       false,
                                       "Subject: loop\r\n\r\n.dot\nend");

    (void)pthread_join(thread, NULL);
    d_smtp_listener_close(&peer.listener);
    result = d_assert_standalone(
                 ( (sent == D_SMTP_OK) &&
                   (peer.result == D_SMTP_OK) &&
                   (peer.inbox.messages == 1u) &&
                   (peer.inbox.recipients == 2u) &&
                   (strcmp(peer.inbox.data,
                           "Subject: loop\r\n\r\n.dot\r\nend\r\n") == 0) ),
                 TEST,
                 "a message crosses loopback intact",
                 _counter) && result;

    return result;
}

#else

/*
d_tests_sa_smtp_loopback
  Skipped: no socket transport on this platform.
*/
bool
d_tests_sa_smtp_loopback(
    struct d_test_counter* _counter
)
{
    return d_assert_standalone(true,
                               "loopback (plaintext)",
                               "skipped: D_ENV_SMTP_CAN_SERVE is 0",
                               _counter);
}

#endif  // D_ENV_SMTP_CAN_SERVE


#if D_ENV_SMTP_CAN_TLS

/*
d_tests_smtp_sign
  Fills in and signs a self-signed certificate for "localhost" and
127.0.0.1, valid from a minute ago for a day.
*/
static bool
d_tests_smtp_sign(
    X509*     _certificate,
    EVP_PKEY* _key
)
{
    X509_NAME*     name = X509_get_subject_name(_certificate);
    X509V3_CTX     context;
    X509_EXTENSION* san = NULL;

    X509V3_set_ctx_nodb(&context);
    X509V3_set_ctx(&context, _certificate, _certificate, NULL, NULL, 0);
    san = X509V3_EXT_conf_nid(NULL,
                              &context,
                              NID_subject_alt_name,
                              "DNS:localhost,IP:127.0.0.1");

    const bool built =
        ( (san != NULL) &&
          (X509_set_version(_certificate, 2) == 1) &&
          (ASN1_INTEGER_set(X509_get_serialNumber(_certificate), 1) == 1) &&
          (X509_gmtime_adj(X509_getm_notBefore(_certificate), -60) != NULL) &&
          (X509_gmtime_adj(X509_getm_notAfter(_certificate), 86400) != NULL) &&
          (X509_set_pubkey(_certificate, _key) == 1) &&
          (X509_NAME_add_entry_by_txt(name,
                                      "CN",
                                      MBSTRING_ASC,
                                      (const unsigned char*)"localhost",
                                      -1,
                                      -1,
                                      0) == 1) &&
          (X509_set_issuer_name(_certificate, name) == 1) &&
          (X509_add_ext(_certificate, san, -1) == 1) &&
          (X509_sign(_certificate, _key, EVP_sha256()) > 0) );

    X509_EXTENSION_free(san);

    return built;
}

/*
d_tests_smtp_write_pem
  Writes the certificate, or the private key, to a PEM file.
*/
static bool
d_tests_smtp_write_pem(
    const char* _path,
    X509*       _certificate,
    EVP_PKEY*   _key
)
{
    FILE* file = fopen(_path, "w");

    if (!file)
    {
        return false;
    }

    const int written = (_certificate)
                        ? PEM_write_X509(file, _certificate)
                        : PEM_write_PrivateKey(file,
                                               _key,
                                               NULL,
                                               NULL,
                                               0,
                                               NULL,
                                               NULL);

    return ( (fclose(file) == 0) &&
             (written == 1) );
}

bool
d_tests_sa_smtp_certificate(
    char*  _certificate,
    char*  _key,
    size_t _capacity
)
{
    EVP_PKEY_CTX* context     = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    EVP_PKEY*     key         = NULL;
    X509*         certificate = X509_new();
    const long    process     = (long)getpid();

    (void)snprintf(_certificate,
                   _capacity,
                   "/tmp/d_smtp_%ld_cert.pem",
                   process);
    (void)snprintf(_key,
                   _capacity,
                   "/tmp/d_smtp_%ld_key.pem",
                   process);

    const bool made =
        ( (context != NULL) &&
          (certificate != NULL) &&
          (EVP_PKEY_keygen_init(context) == 1) &&
          (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(
               context,
               NID_X9_62_prime256v1) == 1) &&
          (EVP_PKEY_keygen(context, &key) == 1) &&
          (d_tests_smtp_sign(certificate, key)) &&
          (d_tests_smtp_write_pem(_certificate, certificate, NULL)) &&
          (d_tests_smtp_write_pem(_key, NULL, key)) );

    X509_free(certificate);
    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(context);

    return made;
}

/*
d_tests_smtp_tls_round
  One TLS loopback session. `_implicit` selects implicit TLS (verified
against 127.0.0.1) over STARTTLS (verified against `_name`).
*/
static enum d_smtp_error
d_tests_smtp_tls_round(
    struct d_tests_smtp_peer* _peer,
    const char*               _certificate,
    const char*               _key,
    bool                      _implicit,
    const char*               _name
)
{
    struct d_smtp_client_options options;
    pthread_t                    thread;

    d_smtp_server_config_init(&_peer->config);
    _peer->config.tls.certificate_file = _certificate;
    _peer->config.tls.private_key_file = _key;
    _peer->config.secure               = _implicit;
    _peer->config.require_tls          = true;
    _peer->config.require_auth         = true;
    _peer->implicit_tls                = _implicit;

    if (!d_tests_smtp_peer_start(_peer, &thread))
    {
        return D_SMTP_ERROR_IO;
    }

    d_smtp_client_options_init(&options);
    options.host            = "127.0.0.1";
    options.port            = _peer->listener.port;
    options.security        = (_implicit) ? D_SMTP_SECURITY_TLS
                                          : D_SMTP_SECURITY_STARTTLS;
    options.tls.ca_file     = _certificate;
    options.tls.server_name = _name;
    options.helo_domain     = "client.example";
    options.io_timeout_ms   = 10000;

    const enum d_smtp_error sent = d_tests_smtp_deliver(&options,
                                                        true,
                                                        "Subject: tls\r\n\r\n"
                                                        "secret\r\n");

    (void)pthread_join(thread, NULL);
    d_smtp_listener_close(&_peer->listener);

    return sent;
}

/*
d_tests_sa_smtp_loopback_tls
  Tests the following:
  - STARTTLS verified against a host name, then AUTH PLAIN, then delivery,
    against a server that requires both TLS and authentication
  - implicit TLS verified against an IP address
  - a certificate that does not carry the expected name fails the handshake
*/
bool
d_tests_sa_smtp_loopback_tls(
    struct d_test_counter* _counter
)
{
    const char* const        TEST             = "loopback (TLS)";
    char                     certificate[128] = { 0 };
    char                     key[128]         = { 0 };
    struct d_tests_smtp_peer peer             = { .implicit_tls = false };
    bool                     result           = true;

    if (!d_tests_sa_smtp_certificate(certificate, key, sizeof(key)))
    {
        return d_assert_standalone(false,
                                   TEST,
                                   "a test certificate can be made",
                                   _counter);
    }

    const enum d_smtp_error starttls = d_tests_smtp_tls_round(&peer,
                                                              certificate,
                                                              key,
                                                              false,
                                                              "localhost");

    result = d_assert_standalone(
                 ( (starttls == D_SMTP_OK) &&
                   (peer.result == D_SMTP_OK) &&
                   (peer.inbox.messages == 1u) &&
                   (strcmp(peer.inbox.user, "alice") == 0) &&
                   (strcmp(peer.inbox.data,
                           "Subject: tls\r\n\r\nsecret\r\n") == 0) ),
                 TEST,
                 "STARTTLS, AUTH PLAIN and delivery",
                 _counter) && result;

    memset(&peer.inbox, 0, sizeof(peer.inbox));

    const enum d_smtp_error implicit = d_tests_smtp_tls_round(&peer,
                                                              certificate,
                                                              key,
                                                              true,
                                                              NULL);

    result = d_assert_standalone( ( (implicit == D_SMTP_OK) &&
                                    (peer.inbox.messages == 1u) ),
                                  TEST,
                                  "implicit TLS verified against 127.0.0.1",
                                  _counter) && result;

    const enum d_smtp_error mismatch = d_tests_smtp_tls_round(
                                           &peer,
                                           certificate,
                                           key,
                                           false,
                                           "wrong.example");

    result = d_assert_standalone( (mismatch == D_SMTP_ERROR_TLS),
                                  TEST,
                                  "a certificate for another name is refused",
                                  _counter) && result;

    (void)remove(certificate);
    (void)remove(key);

    return result;
}

#else

bool
d_tests_sa_smtp_certificate(
    char*  _certificate,
    char*  _key,
    size_t _capacity
)
{
    (void)_certificate;
    (void)_key;
    (void)_capacity;

    return false;
}

/*
d_tests_sa_smtp_loopback_tls
  Skipped: TLS is not compiled in.
*/
bool
d_tests_sa_smtp_loopback_tls(
    struct d_test_counter* _counter
)
{
    return d_assert_standalone(true,
                               "loopback (TLS)",
                               "skipped: D_ENV_SMTP_CAN_TLS is 0",
                               _counter);
}

#endif  // D_ENV_SMTP_CAN_TLS


/*
d_tests_sa_smtp_run_all
  Runs every SMTP test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_smtp_run_all(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_smtp_case CASES[] =
    {
        { "buffers",           d_tests_sa_smtp_buffer },
        { "replies",           d_tests_sa_smtp_reply },
        { "capabilities",      d_tests_sa_smtp_capabilities },
        { "validation",        d_tests_sa_smtp_validation },
        { "commands",          d_tests_sa_smtp_commands },
        { "base64",            d_tests_sa_smtp_base64 },
        { "message data",      d_tests_sa_smtp_data },
        { "client, pipelined", d_tests_sa_smtp_client_script },
        { "client, serial",    d_tests_sa_smtp_client_serial },
        { "client, guards",    d_tests_sa_smtp_client_guards },
        { "server, scripted",  d_tests_sa_smtp_server_script },
        { "server, AUTH",      d_tests_sa_smtp_server_auth },
        { "loopback",          d_tests_sa_smtp_loopback },
        { "loopback, TLS",     d_tests_sa_smtp_loopback_tls }
    };
    bool all = true;

    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        const bool passed = CASES[i].run(_counter);

        _counter->tests_total  += 1;
        _counter->tests_passed += (passed) ? 1u : 0u;
        all                     = ( (all) &&
                                    (passed) );
        printf("  [%s] %s\n",
               (passed) ? "PASS" : "FAIL",
               CASES[i].name);
    }

    return all;
}
