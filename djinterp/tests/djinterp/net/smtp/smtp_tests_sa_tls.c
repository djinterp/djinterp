/*******************************************************************************
* djinterp [net]                                             smtp_tests_sa_tls.c
*
* djinterp SMTP kernel tests -- STARTTLS.
*   Standalone tests of STARTTLS on both sides, with net_tests_peer.h's
* scripted peer as the other party and its toy engine as the SSL kernel's
* engine, so every byte on the wire -- plaintext, handshake, and sealed --
* is checked, and every way TLS fails can be had on cue: a session the
* engine will not make, a handshake it fails, a certificate it doubts, a
* peer that hangs up or stalls mid-handshake, plaintext injected behind the
* go-ahead, and failures after the handshake, which are judged by the TLS
* session as those during it are.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_sa_tls.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./smtp_tests_sa.h"  // the suite
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // snprintf
#include <string.h>   // memset, strcmp, strlen, strstr


// d_tests_smtp_world
//   struct: a STARTTLS test's world: a scripted peer, the toy engine's
// record and a context over it, a client, and a server's settings, which
// use the context. It must not move once opened: the context points into
// it.
struct d_tests_smtp_world
{
    struct d_tests_net_peer*    peer;
    struct d_tests_net_toy      toy;
    struct d_ssl_context        tls;
    struct d_smtp_client        client;
    struct d_smtp_server_config config;
    struct d_tests_smtp_inbox   inbox;
    bool                        made;
};

/*
d_tests_smtp_world_open
  A peer, and a toy context in `_role`, verifying as asked; the client is
initialized, and the settings use the context. False when either could not
be made.
*/
static bool
d_tests_smtp_world_open(
    struct d_tests_smtp_world* _world,
    enum d_ssl_role            _role,
    bool                       _verify
)
{
    const struct d_tests_net_toy toy = { .create_status = D_SSL_OK };

    memset(_world,
           0,
           sizeof(*_world));
    _world->toy  = toy;
    _world->peer = d_tests_net_peer_new();
    d_smtp_client_init(&_world->client);
    d_smtp_server_config_init(&_world->config);
    _world->config.tls = &_world->tls;
    _world->made       = ( (_world->peer != NULL) &&
                           (d_tests_net_toy_context(&_world->tls,
                                                    &_world->toy,
                                                    _role,
                                                    _verify)) );

    return _world->made;
}

/*
d_tests_smtp_world_close
  Closes the client, then destroys the context and frees the peer.
*/
static void
d_tests_smtp_world_close(
    struct d_tests_smtp_world* _world
)
{
    d_smtp_client_close(&_world->client);

    // a context not made holds nothing
    if (_world->made)
    {
        d_ssl_context_destroy(&_world->tls);
    }

    d_tests_net_peer_free(_world->peer);
    _world->peer = NULL;
    _world->made = false;

    return;
}

// the client
/*
d_tests_smtp_preface
  The plaintext part of a client's upgrade: a greeting, an EHLO list that
offers STARTTLS and SIZE, and `_go_ahead` once STARTTLS is sent.
*/
static bool
d_tests_smtp_preface(
    struct d_tests_net_peer* _peer,
    const char*              _go_ahead
)
{
    return ( (d_tests_net_peer_say(_peer,
                                   "",
                                   "220 mail.example ESMTP\r\n")) &&
             (d_tests_net_peer_say(_peer,
                                   "EHLO c.example\r\n",
                                   "250-mail.example\r\n250-STARTTLS\r\n"
                                   "250 SIZE 100\r\n"))           &&
             (d_tests_net_peer_say(_peer,
                                   "STARTTLS\r\n",
                                   _go_ahead)) );
}

/*
d_tests_smtp_handshake
  The toy's handshake answered, and the EHLO that follows it answered under
TLS with AUTH PLAIN alone.
*/
static bool
d_tests_smtp_handshake(
    struct d_tests_net_peer* _peer
)
{
    return ( (d_tests_net_peer_say(_peer,
                                   D_TESTS_NET_TOY_HELLO,
                                   D_TESTS_NET_TOY_REPLY)) &&
             (d_tests_net_peer_say_sealed(_peer,
                                          "EHLO c.example\r\n",
                                          "250-mail.example\r\n"
                                          "250 AUTH PLAIN\r\n")) );
}

/*
d_tests_smtp_upgrade
  Opens the world's client in plaintext and upgrades it, checking the
server's certificate against `_name`.
*/
static enum d_smtp_error
d_tests_smtp_upgrade(
    struct d_tests_smtp_world* _world,
    const char*                _name
)
{
    const enum d_smtp_error opened =
        d_tests_smtp_open(&_world->client,
                          d_tests_net_peer_connection(_world->peer,
                                                      true),
                          false);

    return (opened == D_SMTP_OK) ? d_smtp_client_starttls(&_world->client,
                                                          &_world->tls,
                                                          _name)
                                 : opened;
}

/*
d_tests_smtp_upgraded
  Whether the client came through the upgrade as RFC 3207 asks: secure,
ready, its plaintext capabilities forgotten for those EHLO listed under
TLS, with the TLS session it made, named as asked.
*/
static bool
d_tests_smtp_upgraded(
    const struct d_tests_smtp_world* _world,
    enum d_smtp_error                _upgraded
)
{
    const struct d_smtp_client* client = &_world->client;

    return ( (_upgraded == D_SMTP_OK)                            &&
             (client->secure)                                    &&
             (client->state == D_SMTP_CLIENT_READY)              &&
             (client->capabilities.extensions ==
              D_SMTP_EXTENSION_AUTH)                             &&
             (client->capabilities.auth == D_SMTP_AUTH_PLAIN)    &&
             (client->capabilities.size_limit == 0u)             &&
             (client->tls_status == D_SSL_OK)                    &&
             (d_smtp_client_tls_session(client) == &client->tls) &&
             (strcmp(_world->toy.last_host,
                     D_TESTS_NET_TOY_SERVER_NAME) == 0)          &&
             (strcmp(_world->toy.last_sni,
                     D_TESTS_NET_TOY_SERVER_NAME) == 0) );
}

/*
d_tests_smtp_full_script
  Opens a client's world and scripts a whole upgraded session: the
plaintext preface, the handshake, then AUTH's 235 and QUIT's 221 sealed.
*/
static bool
d_tests_smtp_full_script(
    struct d_tests_smtp_world* _world
)
{
    return ( (d_tests_smtp_world_open(_world,
                                      D_SSL_ROLE_CLIENT,
                                      true))                    &&
             (d_tests_smtp_preface(_world->peer,
                                   "220 2.0.0 Ready\r\n"))      &&
             (d_tests_smtp_handshake(_world->peer))             &&
             (d_tests_net_peer_say_sealed(_world->peer,
                                          "AUTH PLAIN",
                                          "235 2.7.0 Yes\r\n")) &&
             (d_tests_net_peer_say_sealed(_world->peer,
                                          "QUIT\r\n",
                                          "221 2.0.0 Bye\r\n")) );
}

/*
d_tests_sa_smtp_starttls_client
  Tests the following:
  - STARTTLS, the toy's handshake, and EHLO again under TLS: the client is
    secure, and knows only what EHLO listed under TLS
  - the name checked and sent is the one given
  - AUTH and QUIT go sealed; close sends close_notify, releases the TLS
    session, leaves the attached connection open, and leaves the client
    neither secure nor authenticated
*/
bool
d_tests_sa_smtp_starttls_client(
    struct d_test_counter* _counter
)
{
    const char* const               TEST          = "d_smtp_client_starttls";
    const struct d_smtp_credentials credentials   = { .user      = "alice",
                                                      .secret    = "s3cret",
                                                      .mechanism = 0u };
    struct d_tests_smtp_world       world         = { .peer = NULL };
    char                            expected[512] = { 0 };
    struct d_smtp_buffer            wire          =
    {
        .data     = expected,
        .capacity = sizeof(expected),
        .length   = 0
    };
    bool                            result        = true;

    if (!d_tests_net_assert(d_tests_smtp_full_script(&world),
                            TEST,
                            "a scripted server and a toy context",
                            _counter))
    {
        d_tests_smtp_world_close(&world);

        return false;
    }

    const enum d_smtp_error upgraded = d_tests_smtp_upgrade(
                                           &world,
                                           D_TESTS_NET_TOY_SERVER_NAME);
    const bool              secured  = d_tests_smtp_upgraded(&world,
                                                             upgraded);
    const bool              spoke    =
        ( (d_smtp_client_authenticate(&world.client,
                                      &credentials) == D_SMTP_OK) &&
          (d_smtp_client_quit(&world.client) == D_SMTP_OK) );

    d_smtp_client_close(&world.client);
    d_tests_smtp_expect(&wire,
                        "EHLO c.example\r\nSTARTTLS\r\n" D_TESTS_NET_TOY_HELLO,
                        false);
    d_tests_smtp_expect(&wire,
                        "EHLO c.example\r\n"
                        "AUTH PLAIN AGFsaWNlAHMzY3JldA==\r\nQUIT\r\n",
                        true);
    d_tests_smtp_expect(&wire,
                        D_TESTS_NET_TOY_CLOSE,
                        false);
    result = d_tests_net_assert(secured,
                                TEST,
                                "secure, and only what EHLO said under TLS",
                                _counter) && result;
    result = d_tests_net_assert(spoke,
                                TEST,
                                "AUTH and QUIT under TLS",
                                _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_smtp_sent(world.peer,
                                      &wire))                         &&
                   (world.toy.shutdowns == 1)                         &&
                   (world.toy.destroys == 1)                          &&
                   (d_smtp_client_tls_session(&world.client) == NULL) &&
                   (!world.client.secure)                             &&
                   (!world.client.authenticated)                      &&
                   (world.peer->closes == 0) ),
                 TEST,
                 "the wire, close_notify at close, the connection kept",
                 _counter) && result;
    d_tests_smtp_world_close(&world);

    return result;
}

/*
d_tests_sa_smtp_starttls_unnamed
  Tests the following:
  - with a context that checks no names, STARTTLS needs none: the session
    is made with neither a name to check nor SNI, and the upgrade completes
*/
bool
d_tests_sa_smtp_starttls_unnamed(
    struct d_test_counter* _counter
)
{
    struct d_tests_smtp_world world    = { .peer = NULL };
    const bool                scripted =
        ( (d_tests_smtp_world_open(&world,
                                   D_SSL_ROLE_CLIENT,
                                   false))              &&
          (d_tests_smtp_preface(world.peer,
                                "220 2.0.0 Ready\r\n")) &&
          (d_tests_smtp_handshake(world.peer)) );
    const bool                upgraded =
        ( (scripted)                                &&
          (d_tests_smtp_upgrade(&world,
                                NULL) == D_SMTP_OK) &&
          (world.client.secure)                     &&
          (strcmp(world.toy.last_host,
                  "(null)") == 0)                   &&
          (strcmp(world.toy.last_sni,
                  "(null)") == 0) );

    d_tests_smtp_world_close(&world);

    return d_tests_net_assert(upgraded,
                              "d_smtp_client_starttls (unnamed)",
                              "no name needed when none is checked",
                              _counter);
}

/*
d_tests_smtp_refusals_before
  Over a client ready in plaintext, every STARTTLS refused before a byte is
sent: no client, no context, a server's, one never made, and no name or a
malformed one when the context checks names.
*/
static bool
d_tests_smtp_refusals_before(
    struct d_tests_smtp_world*  _world,
    const struct d_ssl_context* _server
)
{
    const char* const           NAME   = D_TESTS_NET_TOY_SERVER_NAME;
    const struct d_ssl_context  empty  = { .engine = NULL };
    struct d_smtp_client* const client = &_world->client;
    const size_t                sent   = _world->peer->sent;

    return ( (d_smtp_client_starttls(NULL,
                                     &_world->tls,
                                     NAME) ==
              D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (d_smtp_client_starttls(client,
                                     NULL,
                                     NAME) ==
              D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (d_smtp_client_starttls(client,
                                     _server,
                                     NAME) ==
              D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (d_smtp_client_starttls(client,
                                     &empty,
                                     NAME) ==
              D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (d_smtp_client_starttls(client,
                                     &_world->tls,
                                     NULL) ==
              D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (d_smtp_client_starttls(client,
                                     &_world->tls,
                                     "not a host") ==
              D_SMTP_ERROR_INVALID_ARGUMENT) &&
             (_world->peer->sent == sent)    &&
             (client->state == D_SMTP_CLIENT_READY) );
}

/*
d_tests_smtp_refused_states
  STARTTLS out of sequence: over a channel already secure, and before EHLO;
then STARTTLS a server does not offer. The peer greets twice, answering
each EHLO, so that two sessions run over it in turn.
*/
static bool
d_tests_smtp_refused_states(
    struct d_tests_smtp_world* _world
)
{
    struct d_net_connection* const connection =
        d_tests_net_peer_connection(_world->peer,
                                    true);
    struct d_smtp_client* const    client     = &_world->client;
    const char* const              NAME       = D_TESTS_NET_TOY_SERVER_NAME;
    const bool                     secure     =
        ( (d_tests_net_peer_say(_world->peer,
                                "",
                                "220 x\r\n"))    &&
          (d_tests_net_peer_say(_world->peer,
                                "EHLO c.example\r\n",
                                "250 x\r\n"))    &&
          (d_tests_net_peer_say(_world->peer,
                                "",
                                "220 x\r\n"))    &&
          (d_tests_net_peer_say(_world->peer,
                                "EHLO c.example\r\n",
                                "250 x\r\n"))    &&
          (d_tests_smtp_open(client,
                             connection,
                             true) == D_SMTP_OK) &&
          (d_smtp_client_starttls(client,
                                  &_world->tls,
                                  NAME) == D_SMTP_ERROR_STATE) );

    d_smtp_client_close(client);

    return ( (secure)                                             &&
             (d_smtp_client_attach(client,
                                   connection,
                                   false) == D_SMTP_OK)           &&
             (d_smtp_client_greeting(client) == D_SMTP_OK)        &&
             (d_smtp_client_starttls(client,
                                     &_world->tls,
                                     NAME) == D_SMTP_ERROR_STATE) &&
             (d_smtp_client_hello(client,
                                  "c.example") == D_SMTP_OK)      &&
             (d_smtp_client_starttls(client,
                                     &_world->tls,
                                     NAME) ==
              D_SMTP_ERROR_UNSUPPORTED)                           &&
             (d_tests_net_peer_sent(_world->peer,
                                    "EHLO c.example\r\n"
                                    "EHLO c.example\r\n")) );
}

/*
d_tests_sa_smtp_starttls_refusals
  Tests the following:
  - no client, no context, a server's context, one never made, no name and
    a malformed one are refused before anything is sent
  - STARTTLS over a channel already secure, or before EHLO, is out of
    sequence, and one the server does not offer is unsupported
  - a 454 is REJECTED, leaving the client ready in plaintext, no TLS
    session made
*/
bool
d_tests_sa_smtp_starttls_refusals(
    struct d_test_counter* _counter
)
{
    const char* const         TEST   = "d_smtp_client_starttls (refusals)";
    struct d_tests_net_toy    toy    = { .create_status = D_SSL_OK };
    struct d_ssl_context      server = { .engine = NULL };
    struct d_tests_smtp_world world  = { .peer = NULL };
    bool                      result = true;
    const bool                ready  =
        ( (d_tests_smtp_world_open(&world,
                                   D_SSL_ROLE_CLIENT,
                                   true))                 &&
          (d_tests_net_toy_context(&server,
                                   &toy,
                                   D_SSL_ROLE_SERVER,
                                   false))                &&
          (d_tests_smtp_preface(world.peer,
                                "454 4.7.0 Not now\r\n")) &&
          (d_tests_smtp_open(&world.client,
                             d_tests_net_peer_connection(world.peer,
                                                         true),
                             false) == D_SMTP_OK) );
    const bool before = ( (ready) &&
                          (d_tests_smtp_refusals_before(&world,
                                                        &server)) );
    const bool later  =
        ( (ready)                                     &&
          (d_smtp_client_starttls(&world.client,
                                  &world.tls,
                                  D_TESTS_NET_TOY_SERVER_NAME) ==
           D_SMTP_ERROR_REJECTED)                     &&
          (world.client.reply.code == 454u)           &&
          (world.client.state == D_SMTP_CLIENT_READY) &&
          (!world.client.secure)                      &&
          (world.toy.creates == 0) );

    d_ssl_context_destroy(&server);
    d_tests_smtp_world_close(&world);
    result = d_tests_net_assert(before,
                                TEST,
                                "every bad argument, before a byte is sent",
                                _counter) && result;
    result = d_tests_net_assert(later,
                                TEST,
                                "a 454 leaves the client ready in plaintext",
                                _counter) && result;
    result = d_tests_net_assert( (d_tests_smtp_world_open(&world,
                                                          D_SSL_ROLE_CLIENT,
                                                          true)) &&
                                 (d_tests_smtp_refused_states(&world)),
                                 TEST,
                                 "out of sequence, or not offered",
                                 _counter) && result;
    d_tests_smtp_world_close(&world);

    return result;
}

// d_tests_smtp_injection
//   struct: plaintext behind a go-ahead, and what STARTTLS must make of it.
struct d_tests_smtp_injection
{
    const char*       go_ahead;    // the 220, and what comes with it
    const char*       behind;      // a turn of its own after it, or NULL
    bool              uncounted;   // the connection cannot count what waits
    enum d_smtp_error status;      // what STARTTLS reports
    enum d_ssl_error  tls_status;  // what the client keeps
    int               creates;     // TLS sessions asked of the engine
};

/*
d_tests_smtp_injected
  A go-ahead as `_case` has it, with what it puts behind it -- in its own
turn, which only a connection that counts what waits can see -- over a
peer that counts or not: STARTTLS must end as the case expects, leaving
the client to be closed.
*/
static bool
d_tests_smtp_injected(
    const struct d_tests_smtp_injection* _case
)
{
    struct d_tests_smtp_world world   = { .peer = NULL };
    bool                      refused = false;

    if ( (d_tests_smtp_world_open(&world,
                                  D_SSL_ROLE_CLIENT,
                                  true))         &&
         (d_tests_smtp_preface(world.peer,
                               _case->go_ahead)) &&
         ( (!_case->behind) ||
           (d_tests_net_peer_say(world.peer,
                                 "",
                                 _case->behind)) ) )
    {
        world.peer->uncounted = _case->uncounted;
        refused = ( (d_tests_smtp_upgrade(&world,
                                          D_TESTS_NET_TOY_SERVER_NAME) ==
                     _case->status)                                &&
                    (world.client.state == D_SMTP_CLIENT_QUIT)     &&
                    (world.client.tls_status == _case->tls_status) &&
                    (world.toy.creates == _case->creates) );
    }

    d_tests_smtp_world_close(&world);

    return refused;
}

/*
d_tests_sa_smtp_starttls_injection
  Tests the following:
  - a reply injected behind the go-ahead, read with it, is PROTOCOL, and
    no TLS session is made; so is a single byte
  - one waiting in a connection that counts what waits is caught too, a
    single byte as well
  - one a connection cannot count reaches TLS, which finds it is not a
    handshake: TLS, the client keeping NOT_TLS
  - each leaves the client to be closed
*/
bool
d_tests_sa_smtp_starttls_injection(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_smtp_injection CASES[] =
    {
        { "220 2.0.0 Ready\r\n250 2.0.0 Injected\r\n", NULL,
          false, D_SMTP_ERROR_PROTOCOL, D_SSL_OK,            0 },
        { "220 2.0.0 Ready\r\n", "250 2.0.0 Injected\r\n",
          false, D_SMTP_ERROR_PROTOCOL, D_SSL_OK,            0 },
        { "220 2.0.0 Ready\r\n", "250 2.0.0 Injected\r\n",
          true,  D_SMTP_ERROR_TLS,      D_SSL_ERROR_NOT_TLS, 1 },
        { "220 2.0.0 Ready\r\nX", NULL,
          false, D_SMTP_ERROR_PROTOCOL, D_SSL_OK,            0 },
        { "220 2.0.0 Ready\r\n", "X",
          false, D_SMTP_ERROR_PROTOCOL, D_SSL_OK,            0 }
    };
    static const char* const WHAT[] =
    {
        "a reply read with the go-ahead is refused",
        "a reply waiting behind it is refused too",
        "one no one counted is no handshake: NOT_TLS",
        "a single byte read with it is refused",
        "a single byte waiting behind it is refused"
    };
    bool result = true;

    // each injection, caught where it can be
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        result = d_tests_net_assert(d_tests_smtp_injected(&CASES[i]),
                                    "d_smtp_client_starttls (injection)",
                                    WHAT[i],
                                    _counter) && result;
    }

    return result;
}

// d_tests_smtp_breach
//   struct: one way a client's handshake fails, and what the client must
// make of it.
struct d_tests_smtp_breach
{
    enum d_ssl_error  create_status;     // what session_create reports
    enum d_ssl_error  handshake_status;  // a failure the toy reports
    uint32_t          verdict;           // the chain, as the toy saw it
    const char*       cert_name;         // the certificate's name, or NULL
    bool              replies;           // the peer answers the hello
    bool              hangs_up;          // it hangs up, when it does not
    bool              resets;            // the hello's write is reset
    enum d_smtp_error status;            // what STARTTLS reports
    enum d_ssl_error  tls_status;        // what the client keeps
    enum d_net_error  cause;             // what the reader keeps
};

/*
d_tests_smtp_breached
  STARTTLS through a handshake that fails as `_case` says: the client ends
in QUIT, keeping the kernel's status and the connection's cause, and the
TLS session it made -- if it made one -- until close releases it.
*/
static bool
d_tests_smtp_breached(
    const struct d_tests_smtp_breach* _case
)
{
    struct d_tests_smtp_world world  = { .peer = NULL };
    bool                      failed = false;

    if ( (d_tests_smtp_world_open(&world,
                                  D_SSL_ROLE_CLIENT,
                                  true))               &&
         (d_tests_smtp_preface(world.peer,
                               "220 2.0.0 Ready\r\n")) &&
         ( (!_case->replies) ||
           (d_tests_smtp_handshake(world.peer)) ) )
    {
        const bool made = (_case->create_status == D_SSL_OK);

        world.toy.create_status    = _case->create_status;
        world.toy.handshake_status = _case->handshake_status;
        world.toy.verdict          = _case->verdict;
        world.toy.cert_name        = _case->cert_name;
        world.peer->hangs_up       = _case->hangs_up;
        world.peer->resets         = _case->resets;
        world.peer->fail_after     = (_case->resets)
                                         ? strlen("EHLO c.example\r\n"
                                                  "STARTTLS\r\n")
                                         : world.peer->fail_after;
        failed = ( (d_tests_smtp_upgrade(&world,
                                         D_TESTS_NET_TOY_SERVER_NAME) ==
                    _case->status)                                 &&
                   (world.client.state == D_SMTP_CLIENT_QUIT)      &&
                   (world.client.tls_status == _case->tls_status)  &&
                   (world.client.reader.net_error == _case->cause) &&
                   ( (d_smtp_client_tls_session(&world.client) != NULL) ==
                     made ) );
        d_smtp_client_close(&world.client);
        failed = ( (failed)                                           &&
                   (d_smtp_client_tls_session(&world.client) == NULL) &&
                   (world.client.tls_status == _case->tls_status)     &&
                   (world.toy.destroys == ((made) ? 1 : 0)) );
    }

    d_tests_smtp_world_close(&world);

    return failed;
}

/*
d_tests_sa_smtp_starttls_failures
  Tests the following:
  - a TLS session the engine will not make: OUT_OF_MEMORY for a lack of
    memory, TLS for anything else, and nothing to release
  - a handshake the engine fails, a chain it doubts, and a certificate for
    another name: TLS, the kernel's status kept, the session kept until
    close
  - a peer that hangs up mid-handshake: CLOSED, the session TRUNCATED
  - one that stalls: TIMED_OUT, the timeout kept as the cause
  - a hello the connection resets: CLOSED, the reset kept as the cause
*/
bool
d_tests_sa_smtp_starttls_failures(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_smtp_breach CASES[] =
    {
        { D_SSL_ERROR_OUT_OF_MEMORY, D_SSL_OK, 0u, NULL,
          false, false, false, D_SMTP_ERROR_OUT_OF_MEMORY,
          D_SSL_ERROR_OUT_OF_MEMORY, D_NET_OK },
        { D_SSL_ERROR_BACKEND, D_SSL_OK, 0u, NULL,
          false, false, false, D_SMTP_ERROR_TLS,
          D_SSL_ERROR_BACKEND, D_NET_OK },
        { D_SSL_OK, D_SSL_ERROR_HANDSHAKE, 0u, NULL,
          false, false, false, D_SMTP_ERROR_TLS,
          D_SSL_ERROR_HANDSHAKE, D_NET_OK },
        { D_SSL_OK, D_SSL_OK, (uint32_t)D_SSL_VERIFY_FLAG_EXPIRED, NULL,
          true, false, false, D_SMTP_ERROR_TLS,
          D_SSL_ERROR_VERIFY, D_NET_OK },
        { D_SSL_OK, D_SSL_OK, 0u, "other.example",
          true, false, false, D_SMTP_ERROR_TLS,
          D_SSL_ERROR_VERIFY, D_NET_OK },
        { D_SSL_OK, D_SSL_OK, 0u, NULL,
          false, true, false, D_SMTP_ERROR_CLOSED,
          D_SSL_ERROR_TRUNCATED, D_NET_OK },
        { D_SSL_OK, D_SSL_OK, 0u, NULL,
          false, false, false, D_SMTP_ERROR_TIMED_OUT,
          D_SSL_ERROR_TRANSPORT, D_NET_ERROR_TIMED_OUT },
        { D_SSL_OK, D_SSL_OK, 0u, NULL,
          false, false, true, D_SMTP_ERROR_CLOSED,
          D_SSL_ERROR_TRUNCATED, D_NET_ERROR_CONNECTION_RESET }
    };
    static const char* const WHAT[] =
    {
        "a session not made for want of memory: OUT_OF_MEMORY",
        "a session the engine refused: TLS",
        "a handshake the engine failed: TLS",
        "a chain the engine doubted: TLS, VERIFY kept",
        "a certificate for another name: TLS, VERIFY kept",
        "a peer gone mid-handshake: CLOSED, TRUNCATED kept",
        "a peer stalled mid-handshake: TIMED_OUT, its cause kept",
        "a hello the connection reset: CLOSED, the reset kept"
    };
    bool result = true;

    // each failure, reported as itself
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        result = d_tests_net_assert(d_tests_smtp_breached(&CASES[i]),
                                    "d_smtp_client_starttls (failures)",
                                    WHAT[i],
                                    _counter) && result;
    }

    return result;
}

// d_tests_smtp_aftermath
//   struct: how a channel fails once TLS is up, and what NOOP must make of
// it.
struct d_tests_smtp_aftermath
{
    bool              corrupt;     // the engine fails every read
    bool              answers;     // the peer answers NOOP...
    bool              closes;      // ...with its close_notify alone
    bool              hangs_up;    // it hangs up, when it does not answer
    bool              resets;      // writes are reset from here on
    enum d_smtp_error status;      // what NOOP reports
    enum d_ssl_error  tls_status;  // what the client keeps
    enum d_net_error  cause;       // what the reader keeps
};

/*
d_tests_smtp_after_upgrade
  Upgrades a client cleanly, breaks the channel as `_case` says, and sends
NOOP, which must fail as the case expects.
*/
static bool
d_tests_smtp_after_upgrade(
    const struct d_tests_smtp_aftermath* _case
)
{
    struct d_tests_smtp_world world  = { .peer = NULL };
    bool                      failed = false;

    if ( (d_tests_smtp_world_open(&world,
                                  D_SSL_ROLE_CLIENT,
                                  true))                             &&
         (d_tests_smtp_preface(world.peer,
                               "220 2.0.0 Ready\r\n"))               &&
         (d_tests_smtp_handshake(world.peer))                        &&
         ( (!_case->answers) ||
           (d_tests_net_peer_say_sealed(world.peer,
                                        "NOOP\r\n",
                                        (_case->closes)
                                        ? D_TESTS_NET_TOY_CLOSE
                                        : "250 2.0.0 OK\r\n")) ) &&
         (d_tests_smtp_upgrade(&world,
                               D_TESTS_NET_TOY_SERVER_NAME) == D_SMTP_OK) )
    {
        world.toy.read_status  = (_case->corrupt) ? D_SSL_ERROR_PROTOCOL
                                                  : D_SSL_OK;
        world.peer->hangs_up   = _case->hangs_up;
        world.peer->resets     = _case->resets;
        world.peer->fail_after = (_case->resets) ? world.peer->sent
                                                 : world.peer->fail_after;
        failed = ( (d_smtp_client_noop(&world.client) == _case->status) &&
                   (world.client.tls_status == _case->tls_status)       &&
                   (world.client.reader.net_error == _case->cause) );
    }

    d_tests_smtp_world_close(&world);

    return failed;
}

/*
d_tests_smtp_corrupt_hello
  An engine that fails every read under TLS fails the EHLO that follows the
handshake: STARTTLS reports TLS, as it would for the handshake itself, the
client stays where a failed EHLO leaves it, and what EHLO said in plaintext
is forgotten all the same.
*/
static bool
d_tests_smtp_corrupt_hello(
    void
)
{
    struct d_tests_smtp_world world  = { .peer = NULL };
    bool                      failed = false;

    if ( (d_tests_smtp_world_open(&world,
                                  D_SSL_ROLE_CLIENT,
                                  true))               &&
         (d_tests_smtp_preface(world.peer,
                               "220 2.0.0 Ready\r\n")) &&
         (d_tests_smtp_handshake(world.peer)) )
    {
        world.toy.read_status = D_SSL_ERROR_PROTOCOL;
        failed = ( (d_tests_smtp_upgrade(&world,
                                         D_TESTS_NET_TOY_SERVER_NAME) ==
                    D_SMTP_ERROR_TLS)                                &&
                   (world.client.secure)                             &&
                   (world.client.state == D_SMTP_CLIENT_HELLO)       &&
                   (world.client.capabilities.extensions == 0u)      &&
                   (world.client.capabilities.size_limit == 0u)      &&
                   (world.client.tls_status == D_SSL_ERROR_PROTOCOL) &&
                   (world.client.reader.net_error ==
                    D_NET_ERROR_PROTOCOL) );
    }

    d_tests_smtp_world_close(&world);

    return failed;
}

/*
d_tests_sa_smtp_starttls_after
  Tests the following:
  - a TLS failure after the handshake is TLS, as one during it is, the
    kernel's status kept: in the EHLO that follows the handshake, and in a
    later command
  - a peer gone without close_notify: CLOSED, the session TRUNCATED
  - a stall under TLS: TIMED_OUT, the timeout kept as the cause
  - a write the connection resets: CLOSED, TRUNCATED and the reset kept
  - the peer's close_notify is the end of the stream: CLOSED, and nothing
    failed
*/
bool
d_tests_sa_smtp_starttls_after(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_smtp_aftermath CASES[] =
    {
        { true,  true,  false, false, false, D_SMTP_ERROR_TLS,
          D_SSL_ERROR_PROTOCOL,  D_NET_ERROR_PROTOCOL },
        { false, false, false, true,  false, D_SMTP_ERROR_CLOSED,
          D_SSL_ERROR_TRUNCATED, D_NET_ERROR_CLOSED },
        { false, false, false, false, false, D_SMTP_ERROR_TIMED_OUT,
          D_SSL_ERROR_TRANSPORT, D_NET_ERROR_TIMED_OUT },
        { false, false, false, false, true,  D_SMTP_ERROR_CLOSED,
          D_SSL_ERROR_TRUNCATED, D_NET_ERROR_CONNECTION_RESET },
        { false, true,  true,  false, false, D_SMTP_ERROR_CLOSED,
          D_SSL_OK,              D_NET_OK }
    };
    static const char* const WHAT[] =
    {
        "a record the engine refuses: TLS, PROTOCOL kept",
        "a peer gone without close_notify: CLOSED, TRUNCATED kept",
        "a stall under TLS: TIMED_OUT, its cause kept",
        "a write the connection resets: CLOSED, the reset kept",
        "the peer's close_notify: CLOSED, and nothing failed"
    };
    const char* const TEST   = "d_smtp_client (after STARTTLS)";
    bool              result = d_tests_net_assert(
                                   d_tests_smtp_corrupt_hello(),
                                   TEST,
                                   "a TLS failure in the EHLO after it: TLS",
                                   _counter);

    // each failure, reported as itself
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        result = d_tests_net_assert(d_tests_smtp_after_upgrade(&CASES[i]),
                                    TEST,
                                    WHAT[i],
                                    _counter) && result;
    }

    return result;
}

/*
d_tests_smtp_failed_session
  A client TLS session over a peer, failed as asked: by its engine in the
handshake, or by a peer that stalls.
*/
static bool
d_tests_smtp_failed_session(
    struct d_tests_smtp_world* _world,
    struct d_ssl_session*      _session,
    bool                       _by_engine
)
{
    const struct d_pack_text host = { D_TESTS_NET_TOY_SERVER_NAME,
                                      strlen(D_TESTS_NET_TOY_SERVER_NAME) };

    if (!d_tests_smtp_world_open(_world,
                                 D_SSL_ROLE_CLIENT,
                                 true))
    {
        return false;
    }

    _world->toy.handshake_status = (_by_engine) ? D_SSL_ERROR_HANDSHAKE
                                                : D_SSL_OK;

    // a session not made leaves nothing to release
    if (d_ssl_session_init(_session,
                           &_world->tls,
                           d_tests_net_peer_connection(_world->peer,
                                                       true),
                           host) != D_SSL_OK)
    {
        return false;
    }

    // one that did not fail is released here, its caller seeing none
    if (d_ssl_session_handshake(_session) == D_SSL_OK)
    {
        d_ssl_session_destroy(_session);

        return false;
    }

    return true;
}

/*
d_tests_smtp_judged
  d_smtp_error_from_tls over a session failed as asked: `_result` must
become `_status`, writing `_tls_status` and `_cause` -- `_cause` left alone
when it is the sentinel D_NET_ERROR_ADDRESS_IN_USE -- and the call must
take NULL for both places too.
*/
static bool
d_tests_smtp_judged(
    bool              _by_engine,
    enum d_smtp_error _result,
    enum d_smtp_error _status,
    enum d_ssl_error  _tls_status,
    enum d_net_error  _cause
)
{
    struct d_tests_smtp_world world   = { .peer = NULL };
    struct d_ssl_session      session = { .base = { .vtable = NULL } };
    enum d_ssl_error          status  = D_SSL_OK;
    enum d_net_error          cause   = D_NET_ERROR_ADDRESS_IN_USE;
    bool                      judged  = false;

    if (d_tests_smtp_failed_session(&world,
                                    &session,
                                    _by_engine))
    {
        judged = ( (d_smtp_error_from_tls(&session,
                                          _result,
                                          &status,
                                          &cause) == _status) &&
                   (status == _tls_status)                    &&
                   (cause == _cause)                          &&
                   (d_smtp_error_from_tls(&session,
                                          _result,
                                          NULL,
                                          NULL) == _status)   &&
                   (d_smtp_error_from_tls(&session,
                                          D_SMTP_OK,
                                          &status,
                                          &cause) == D_SMTP_OK) );
        d_ssl_session_destroy(&session);
    }

    d_tests_smtp_world_close(&world);

    return judged;
}

/*
d_tests_smtp_unjudged
  d_smtp_error_from_tls where nothing is judged: no session, and a session
that has not failed, each leaving the result as given and writing nothing.
*/
static bool
d_tests_smtp_unjudged(
    void
)
{
    const struct d_pack_text  host    = { D_TESTS_NET_TOY_SERVER_NAME,
                                         strlen(D_TESTS_NET_TOY_SERVER_NAME) };
    struct d_tests_smtp_world world   = { .peer = NULL };
    struct d_ssl_session      session = { .base = { .vtable = NULL } };
    enum d_ssl_error          status  = D_SSL_ERROR_STATE;
    enum d_net_error          cause   = D_NET_ERROR_ADDRESS_IN_USE;
    bool                      left    = false;

    if ( (d_tests_smtp_world_open(&world,
                                  D_SSL_ROLE_CLIENT,
                                  true)) &&
         (d_ssl_session_init(&session,
                             &world.tls,
                             d_tests_net_peer_connection(world.peer,
                                                         true),
                             host) == D_SSL_OK) )
    {
        left = ( (d_smtp_error_from_tls(NULL,
                                        D_SMTP_ERROR_CLOSED,
                                        &status,
                                        &cause) == D_SMTP_ERROR_CLOSED)   &&
                 (d_smtp_error_from_tls(&session,
                                        D_SMTP_ERROR_PROTOCOL,
                                        &status,
                                        &cause) == D_SMTP_ERROR_PROTOCOL) &&
                 (status == D_SSL_ERROR_STATE)                            &&
                 (cause == D_NET_ERROR_ADDRESS_IN_USE) );
        d_ssl_session_destroy(&session);
    }

    d_tests_smtp_world_close(&world);

    return left;
}

/*
d_tests_sa_smtp_errors_from_tls
  Tests the following:
  - no session, or one that has not failed, leaves the result as given,
    and writes nothing
  - a session its engine failed is TLS, its status written and no cause,
    as its connection never failed
  - a session a stalled connection failed is that connection's TIMED_OUT,
    TRANSPORT and the timeout written
  - a result of D_SMTP_OK stays one, and NULL places to write are allowed
*/
bool
d_tests_sa_smtp_errors_from_tls(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "d_smtp_error_from_tls";
    bool              result = true;

    result = d_tests_net_assert(d_tests_smtp_unjudged(),
                                TEST,
                                "no session, or a sound one: as given",
                                _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_judged(true,
                                     D_SMTP_ERROR_PROTOCOL,
                                     D_SMTP_ERROR_TLS,
                                     D_SSL_ERROR_HANDSHAKE,
                                     D_NET_ERROR_ADDRESS_IN_USE),
                 TEST,
                 "a session its engine failed: TLS, no cause",
                 _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_judged(false,
                                     D_SMTP_ERROR_CLOSED,
                                     D_SMTP_ERROR_TIMED_OUT,
                                     D_SSL_ERROR_TRANSPORT,
                                     D_NET_ERROR_TIMED_OUT),
                 TEST,
                 "a session its connection failed: that connection's error",
                 _counter) && result;

    return result;
}

// the server
// D_TESTS_SMTP_EHLO_LIST
//   macro: the extensions the test server lists for c.example after the
// greeting line, before STARTTLS and AUTH.
#define D_TESTS_SMTP_EHLO_LIST                                                 \
    "250-localhost Hello c.example\r\n250-PIPELINING\r\n"                      \
    "250-SIZE 10485760\r\n250-8BITMIME\r\n250-ENHANCEDSTATUSCODES\r\n"         \
    "250-SMTPUTF8\r\n"

/*
d_tests_smtp_server_world
  A server's world, scripted as a client that says `_plaintext`, answers
the go-ahead with the toy's hello, and says `_sealed` under TLS once the
toy's reply has come; NULL for either part leaves it out.
*/
static bool
d_tests_smtp_server_world(
    struct d_tests_smtp_world* _world,
    const char*                _plaintext,
    const char*                _sealed
)
{
    char sealed[D_TESTS_NET_PEER_TURN_BYTES] = { 0 };

    if (_sealed)
    {
        d_tests_net_toy_seal(sealed,
                             sizeof(sealed),
                             _sealed);
    }

    return ( (d_tests_smtp_world_open(_world,
                                      D_SSL_ROLE_SERVER,
                                      false))              &&
             (d_tests_net_peer_say(_world->peer,
                                   "",
                                   _plaintext))            &&
             (d_tests_net_peer_say(_world->peer,
                                   "Ready to start TLS\r\n",
                                   D_TESTS_NET_TOY_HELLO)) &&
             ( (!_sealed) ||
               (d_tests_net_peer_say(_world->peer,
                                     D_TESTS_NET_TOY_REPLY,
                                     sealed)) ) );
}

/*
d_tests_smtp_serve_world
  One session over the world's peer with its settings, AUTH included.
*/
static enum d_smtp_error
d_tests_smtp_serve_world(
    struct d_tests_smtp_world* _world
)
{
    return d_tests_smtp_serve(d_tests_net_peer_connection(_world->peer,
                                                          true),
                              &_world->config,
                              &_world->inbox,
                              true);
}

/*
d_tests_smtp_upgraded_wire
  What the server must write to a client that upgrades, authenticates,
sends one message and quits: the plaintext list with STARTTLS, the
go-ahead, the toy's reply, then sealed the list with AUTH and each reply.
*/
static void
d_tests_smtp_upgraded_wire(
    struct d_smtp_buffer* _wire
)
{
    d_tests_smtp_expect(_wire,
                        "220 localhost ESMTP ready\r\n"
                        D_TESTS_SMTP_EHLO_LIST "250 STARTTLS\r\n"
                        "220 2.0.0 Ready to start TLS\r\n"
                        D_TESTS_NET_TOY_REPLY,
                        false);
    d_tests_smtp_expect(_wire,
                        D_TESTS_SMTP_EHLO_LIST "250 AUTH PLAIN LOGIN\r\n"
                        "235 2.7.0 Authentication successful\r\n"
                        "250 2.1.0 Sender OK\r\n250 2.1.5 Recipient OK\r\n"
                        "354 Start mail input; end with <CRLF>.<CRLF>\r\n"
                        "250 2.0.0 Message accepted\r\n"
                        "221 2.0.0 Closing connection\r\n",
                        true);
    d_tests_smtp_expect(_wire,
                        D_TESTS_NET_TOY_CLOSE,
                        false);

    return;
}

/*
d_tests_smtp_dropped_sealed
  A client that upgrades, then under TLS makes one error more than the
server tolerates: the session must end PROTOCOL with a sealed 421, and its
TLS session, still sound, must say close_notify all the same.
*/
static bool
d_tests_smtp_dropped_sealed(
    void
)
{
    char                      script[256] = { 0 };
    char                      sealed[96]  = { 0 };
    struct d_smtp_buffer      text        =
    {
        .data     = script,
        .capacity = sizeof(script),
        .length   = 0
    };
    struct d_tests_smtp_world world       = { .peer = NULL };
    bool                      dropped     = false;

    (void)d_smtp_buffer_append_text(&text,
                                    "EHLO c.example\r\n");

    // one unknown command more than the server tolerates
    for (unsigned int i = 0; i <= D_SMTP_SERVER_ERRORS_MAX; ++i)
    {
        (void)d_smtp_buffer_append_text(&text,
                                        "XYZZY\r\n");
    }

    d_tests_net_toy_seal(sealed,
                         sizeof(sealed),
                         "421 4.7.0 Too many errors, closing connection\r\n");

    // the 421 sealed, then close_notify
    if (d_tests_smtp_server_world(&world,
                                  "EHLO c.example\r\nSTARTTLS\r\n",
                                  script))
    {
        dropped = ( (d_tests_smtp_serve_world(&world) ==
                     D_SMTP_ERROR_PROTOCOL)    &&
                    (strstr((const char*)world.peer->output,
                            sealed) != NULL)   &&
                    (world.toy.shutdowns == 1) &&
                    (world.toy.destroys == 1) );
    }

    d_tests_smtp_world_close(&world);

    return dropped;
}

/*
d_tests_sa_smtp_starttls_server
  Tests the following:
  - STARTTLS is listed with a context, and AUTH is not, in plaintext
  - what the client pipelined behind STARTTLS is discarded, unanswered
    (CVE-2011-0411)
  - after the toy's handshake the session starts over: EHLO lists AUTH and
    no STARTTLS; AUTH PLAIN and a message go through, all sealed
  - after QUIT the server sends close_notify, and releases its TLS
    session; the connection is left open
  - a client dropped for its errors under TLS draws a sealed 421, and
    close_notify all the same
*/
bool
d_tests_sa_smtp_starttls_server(
    struct d_test_counter* _counter
)
{
    const char* const         TEST           =
        "d_smtp_server_session (STARTTLS)";
    struct d_tests_smtp_world world          = { .peer = NULL };
    char                      expected[1024] = { 0 };
    struct d_smtp_buffer      wire           =
    {
        .data     = expected,
        .capacity = sizeof(expected),
        .length   = 0
    };
    bool                      result         = true;

    if (!d_tests_net_assert(
             d_tests_smtp_server_world(
                 &world,
                 "EHLO c.example\r\nSTARTTLS\r\nNOOP\r\n",
                 "EHLO c.example\r\nAUTH PLAIN AGFsaWNlAHMzY3JldA==\r\n"
                 "MAIL FROM:<alice@example.org>\r\n"
                 "RCPT TO:<bob@example.com>\r\nDATA\r\n"
                 "Subject: s\r\n\r\nsecret\r\n.\r\nQUIT\r\n"),
             TEST,
             "a scripted client and a toy context",
             _counter))
    {
        d_tests_smtp_world_close(&world);

        return false;
    }

    const enum d_smtp_error served = d_tests_smtp_serve_world(&world);

    d_tests_smtp_upgraded_wire(&wire);
    result = d_tests_net_assert( (served == D_SMTP_OK) &&
                                 (d_tests_smtp_sent(world.peer,
                                                    &wire)),
                                 TEST,
                                 "every byte as expected, NOOP unanswered",
                                 _counter) && result;
    result = d_tests_net_assert(
                 ( (world.inbox.messages == 1u) &&
                   (strcmp(world.inbox.user,
                           "alice") == 0)       &&
                   (strcmp(world.inbox.data,
                           "Subject: s\r\n\r\nsecret\r\n") == 0) ),
                 TEST,
                 "AUTH and a message, under TLS",
                 _counter) && result;
    result = d_tests_net_assert( ( (world.toy.creates == 1)   &&
                                   (world.toy.shutdowns == 1) &&
                                   (world.toy.destroys == 1)  &&
                                   (world.peer->closes == 0) ),
                                 TEST,
                                 "close_notify, the session released",
                                 _counter) && result;
    result = d_tests_net_assert(d_tests_smtp_dropped_sealed(),
                                TEST,
                                "dropped under TLS: 421, then close_notify",
                                _counter) && result;
    d_tests_smtp_world_close(&world);

    return result;
}

/*
d_tests_smtp_refused_upgrade
  A server session over a client that says `_script` then QUIT, with a
context unless `_without`: it must end at QUIT, having written `_reply`
and nothing about STARTTLS in its EHLO list.
*/
static bool
d_tests_smtp_refused_upgrade(
    const char* _script,
    bool        _without,
    const char* _reply
)
{
    struct d_tests_smtp_world world   = { .peer = NULL };
    bool                      refused = false;

    if (d_tests_smtp_server_world(&world,
                                  _script,
                                  NULL))
    {
        const char* const output = (const char*)world.peer->output;

        world.peer->hangs_up = true;
        world.config.tls     = (_without) ? NULL : world.config.tls;
        refused = ( (d_tests_smtp_serve_world(&world) == D_SMTP_OK) &&
                    (strstr(output,
                            _reply) != NULL)                        &&
                    ( (!_without) ||
                      (strstr(output,
                              "250 STARTTLS") == NULL) )            &&
                    (world.toy.creates == 0) );
    }

    d_tests_smtp_world_close(&world);

    return refused;
}

/*
d_tests_smtp_sealed_refusal
  A client that upgrades, then says `_sealed` under TLS and QUIT: the
session must end at QUIT, having answered something out of sequence with
a sealed 503.
*/
static bool
d_tests_smtp_sealed_refusal(
    const char* _sealed
)
{
    char                      script[256] = { 0 };
    char                      sealed[64]  = { 0 };
    struct d_tests_smtp_world world       = { .peer = NULL };
    bool                      refused     = false;

    d_tests_net_toy_seal(sealed,
                         sizeof(sealed),
                         "503 5.5.1 Bad sequence of commands\r\n");
    (void)snprintf(script,
                   sizeof(script),
                   "%sQUIT\r\n",
                   _sealed);

    // the 503, sealed, after the one upgrade
    if (d_tests_smtp_server_world(&world,
                                  "EHLO c.example\r\nSTARTTLS\r\n",
                                  script))
    {
        refused = ( (d_tests_smtp_serve_world(&world) == D_SMTP_OK) &&
                    (strstr((const char*)world.peer->output,
                            sealed) != NULL)                        &&
                    (world.toy.creates == 1) );
    }

    d_tests_smtp_world_close(&world);

    return refused;
}

/*
d_tests_sa_smtp_starttls_server_refusals
  Tests the following:
  - STARTTLS before EHLO, or once TLS is up, is out of sequence: 503; and
    once it is up, the plaintext EHLO is forgotten, so MAIL before another
    is out of sequence too
  - STARTTLS with an argument, however short, is a syntax error: 501
  - without a context STARTTLS is neither listed nor done: 502
  - none of them makes a TLS session but the one that upgraded
*/
bool
d_tests_sa_smtp_starttls_server_refusals(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "d_smtp_server_session (STARTTLS refused)";
    bool              result = true;

    result = d_tests_net_assert(
                 d_tests_smtp_refused_upgrade(
                     "STARTTLS\r\nQUIT\r\n",
                     false,
                     "503 5.5.1 Bad sequence of commands\r\n"),
                 TEST,
                 "before EHLO: 503",
                 _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_refused_upgrade(
                     "EHLO c.example\r\nSTARTTLS x\r\nQUIT\r\n",
                     false,
                     "501 5.5.4 Syntax error, no parameters allowed\r\n"),
                 TEST,
                 "with an argument, even of one character: 501",
                 _counter) && result;
    result = d_tests_net_assert(
                 d_tests_smtp_refused_upgrade(
                     "EHLO c.example\r\nSTARTTLS\r\nQUIT\r\n",
                     true,
                     "502 5.5.1 STARTTLS not available\r\n"),
                 TEST,
                 "without a context: not listed, and 502",
                 _counter) && result;
    result = d_tests_net_assert(
                 ( (d_tests_smtp_sealed_refusal("EHLO c.example\r\n"
                                                "STARTTLS\r\n")) &&
                   (d_tests_smtp_sealed_refusal("MAIL FROM:<a@example.org>"
                                                "\r\n")) ),
                 TEST,
                 "once TLS is up, STARTTLS, or MAIL before EHLO: 503",
                 _counter) && result;

    return result;
}

// d_tests_smtp_server_breach
//   struct: one way a server's TLS fails, and how the session must end.
struct d_tests_smtp_server_breach
{
    enum d_ssl_error  create_status;     // what session_create reports
    enum d_ssl_error  handshake_status;  // a failure the toy reports
    enum d_ssl_error  read_status;       // a failure reads report
    bool              hello;             // the client says its hello
    bool              hangs_up;          // it hangs up after its script
    bool              replies;           // the toy's reply goes out
    enum d_smtp_error status;            // how the session ends
    int               sessions;          // TLS sessions made, and released
};

/*
d_tests_smtp_server_breached
  A server session whose TLS fails as `_case` says: it must end as the case
expects, having asked the engine for one TLS session, written nothing after
the toy's reply, if it got that far, and released the session if it was
made.
*/
static bool
d_tests_smtp_server_breached(
    const struct d_tests_smtp_server_breach* _case
)
{
    struct d_tests_smtp_world world         = { .peer = NULL };
    char                      expected[512] = { 0 };
    struct d_smtp_buffer      wire          =
    {
        .data     = expected,
        .capacity = sizeof(expected),
        .length   = 0
    };
    bool                      failed        = false;
    const bool                scripted      =
        ( (d_tests_smtp_world_open(&world,
                                   D_SSL_ROLE_SERVER,
                                   false))                         &&
          (d_tests_net_peer_say(world.peer,
                                "",
                                "EHLO c.example\r\nSTARTTLS\r\n")) &&
          ( (!_case->hello) ||
            (d_tests_net_peer_say(world.peer,
                                  "Ready to start TLS\r\n",
                                  D_TESTS_NET_TOY_HELLO)) ) );

    if (scripted)
    {
        world.toy.create_status    = _case->create_status;
        world.toy.handshake_status = _case->handshake_status;
        world.toy.read_status      = _case->read_status;
        world.peer->hangs_up       = _case->hangs_up;
        d_tests_smtp_expect(&wire,
                            "220 localhost ESMTP ready\r\n"
                            D_TESTS_SMTP_EHLO_LIST "250 STARTTLS\r\n"
                            "220 2.0.0 Ready to start TLS\r\n",
                            false);
        d_tests_smtp_expect(&wire,
                            (_case->replies) ? D_TESTS_NET_TOY_REPLY
                            : "",
                            false);
        failed = ( (d_tests_smtp_serve_world(&world) == _case->status) &&
                   (d_tests_smtp_sent(world.peer,
                                      &wire))                          &&
                   (world.toy.creates == 1)                            &&
                   (world.toy.destroys == _case->sessions)             &&
                   (world.toy.shutdowns == 0) );
    }

    d_tests_smtp_world_close(&world);

    return failed;
}

/*
d_tests_sa_smtp_starttls_server_failures
  Tests the following:
  - a TLS session the engine will not make: OUT_OF_MEMORY or TLS
  - a handshake the engine fails: TLS, the session released
  - a client gone mid-handshake: CLOSED
  - a TLS failure after the handshake, in the EHLO under TLS: TLS, as in
    the handshake, not the PROTOCOL net.h's view of it says
  - a client that stalls under TLS: TIMED_OUT
  - none of them sends close_notify, and each writes nothing after it
*/
bool
d_tests_sa_smtp_starttls_server_failures(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_smtp_server_breach CASES[] =
    {
        { D_SSL_ERROR_OUT_OF_MEMORY, D_SSL_OK,              D_SSL_OK,
          true,  true,  false, D_SMTP_ERROR_OUT_OF_MEMORY, 0 },
        { D_SSL_ERROR_BACKEND,       D_SSL_OK,              D_SSL_OK,
          true,  true,  false, D_SMTP_ERROR_TLS,           0 },
        { D_SSL_OK,                  D_SSL_ERROR_HANDSHAKE, D_SSL_OK,
          true,  true,  false, D_SMTP_ERROR_TLS,           1 },
        { D_SSL_OK,                  D_SSL_OK,              D_SSL_OK,
          false, true,  false, D_SMTP_ERROR_CLOSED,        1 },
        { D_SSL_OK,                  D_SSL_OK,  D_SSL_ERROR_PROTOCOL,
          true,  true,  true,  D_SMTP_ERROR_TLS,           1 },
        { D_SSL_OK,                  D_SSL_OK,              D_SSL_OK,
          true,  false, true,  D_SMTP_ERROR_TIMED_OUT,     1 }
    };
    static const char* const WHAT[] =
    {
        "a session not made for want of memory: OUT_OF_MEMORY",
        "a session the engine refused: TLS",
        "a handshake the engine failed: TLS, released",
        "a client gone mid-handshake: CLOSED",
        "a TLS failure after the handshake: TLS",
        "a client stalled under TLS: TIMED_OUT"
    };
    bool result = true;

    // each failure, reported as itself
    for (size_t i = 0; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        result = d_tests_net_assert(d_tests_smtp_server_breached(&CASES[i]),
                                    "d_smtp_server_session (TLS failures)",
                                    WHAT[i],
                                    _counter) && result;
    }

    return result;
}
