/*******************************************************************************
* djinterp [net]                                     starttls_tests_sa_upgrade.c
*
* Standalone tests of STARTTLS negotiation and the upgrade, against a
* scripted peer and the SSL kernel's toy engine: the policies at the two
* points where TLS can be lost, the upgrade on either side, the refusal of
* plaintext left waiting at the switch, the failures of the TLS session, and
* the end of a secure stream.
*
*
* path:      /tests/djinterp/net/starttls/starttls_tests_sa_upgrade.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./starttls_tests_sa.h"  // the suite
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memcpy, memset, strcmp


// d_tests_starttls_outcome
//   struct: how one negotiation ended.
struct d_tests_starttls_outcome
{
    enum d_starttls_error status;     // what negotiate reported
    enum d_starttls_state state;      // the state after it
    bool                  satisfied;  // whether the policy was then met
    int                   requests;   // upgrade requests sent
    char                  sent[64];   // everything the session sent
};

/*
d_tests_starttls_output
  What the peer has received, as text, truncated to fit `_out`.
*/
static void
d_tests_starttls_output(
    const struct d_tests_net_peer* _peer,
    char*                          _out,
    size_t                         _capacity
)
{
    const size_t count = (_peer->sent < _capacity) ? _peer->sent
                                                   : (_capacity - 1);

    memcpy(_out,
           _peer->output,
           count);
    _out[count] = '\0';

    return;
}

/*
d_tests_starttls_negotiation
  One client negotiation under `_policy`: the peer answers STARTTLS with
`_reply` (no answer if NULL) and completes the toy handshake; the
negotiator's callbacks report `_request` and `_reply_status` instead of
running when those are not D_STARTTLS_OK.
*/
static struct d_tests_starttls_outcome
d_tests_starttls_negotiation(
    enum d_starttls_policy _policy,
    bool                   _offered,
    const char*            _reply,
    enum d_starttls_error  _request,
    enum d_starttls_error  _reply_status
)
{
    struct d_tests_starttls_outcome outcome =
    {
        .status = D_TESTS_STARTTLS_UNREACHED
    };
    struct d_tests_starttls_smtp    smtp    =
    {
        .request_status = _request,
        .reply_status   = _reply_status
    };
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };

    if ( (d_tests_starttls_fixture_open(&fixture,
                                        D_STARTTLS_ROLE_CLIENT,
                                        _policy))      &&
         ( (!_reply) ||
           (d_tests_net_peer_say(fixture.peer,
                                 "STARTTLS\r\n",
                                 _reply)) )            &&
         (d_tests_net_peer_say(fixture.peer,
                               D_TESTS_NET_TOY_HELLO,
                               D_TESTS_NET_TOY_REPLY)) &&
         (d_starttls_init(&session,
                          &fixture.options,
                          fixture.connection) == D_STARTTLS_OK) )
    {
        outcome.status    =
            d_starttls_negotiate(&session,
                                 &d_tests_starttls_smtp_negotiator,
                                 &smtp,
                                 _offered);
        outcome.state     = d_starttls_get_state(&session);
        outcome.satisfied = d_starttls_is_satisfied(&session);
        outcome.requests  = smtp.requests;
        d_tests_starttls_output(fixture.peer,
                                outcome.sent,
                                sizeof(outcome.sent));
    }

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);

    return outcome;
}

/*
d_tests_starttls_ended
  Whether `_outcome` is `_status` in `_state`, satisfied or not as given,
having sent `_sent`.
*/
static bool
d_tests_starttls_ended(
    const struct d_tests_starttls_outcome* _outcome,
    enum d_starttls_error                  _status,
    enum d_starttls_state                  _state,
    bool                                   _satisfied,
    const char*                            _sent
)
{
    return ( (_outcome->status == _status)       &&
             (_outcome->state == _state)         &&
             (_outcome->satisfied == _satisfied) &&
             (strcmp(_outcome->sent,
                     _sent) == 0) );
}

/*
d_tests_sa_starttls_policy_offer
  Tests the following:
  - REQUIRED without an offer fails, and sends nothing
  - OPPORTUNISTIC without an offer continues in plaintext
  - DISABLED does nothing, even when offered
*/
bool
d_tests_sa_starttls_policy_offer(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "policy and offer";
    bool              result = true;

    const struct d_tests_starttls_outcome stripped =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_REQUIRED,
                                     false,
                                     NULL,
                                     D_STARTTLS_OK,
                                     D_STARTTLS_OK);
    const struct d_tests_starttls_outcome missing =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_OPPORTUNISTIC,
                                     false,
                                     NULL,
                                     D_STARTTLS_OK,
                                     D_STARTTLS_OK);
    const struct d_tests_starttls_outcome disabled =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_DISABLED,
                                     true,
                                     "220 Go ahead\r\n",
                                     D_STARTTLS_OK,
                                     D_STARTTLS_OK);

    result = d_tests_net_assert( (d_tests_starttls_ended(
                                      &stripped,
                                      D_STARTTLS_ERROR_NOT_OFFERED,
                                      D_STARTTLS_STATE_FAILED,
                                      false,
                                      "")) &&
                                 (stripped.requests == 0),
                                 TEST,
                                 "REQUIRED without an offer fails, silently",
                                 _counter) && result;
    result = d_tests_net_assert(d_tests_starttls_ended(
                                    &missing,
                                    D_STARTTLS_OK,
                                    D_STARTTLS_STATE_PLAINTEXT,
                                    true,
                                    ""),
                                TEST,
                                "OPPORTUNISTIC without an offer stays plain",
                                _counter) && result;
    result = d_tests_net_assert( (d_tests_starttls_ended(
                                      &disabled,
                                      D_STARTTLS_OK,
                                      D_STARTTLS_STATE_PLAINTEXT,
                                      true,
                                      "")) &&
                                 (disabled.requests == 0),
                                 TEST,
                                 "DISABLED does nothing, even when offered",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_policy_reply
  Tests the following:
  - a refusal fails REQUIRED and leaves OPPORTUNISTIC in plaintext
  - a reply no negotiator can classify is fatal under every policy
*/
bool
d_tests_sa_starttls_policy_reply(
    struct d_test_counter* _counter
)
{
    const char* const TEST    = "policy and reply";
    const char* const REFUSAL = "454 TLS not available\r\n";
    const char* const GARBLE  = "500 what?\r\n";
    bool              result  = true;

    const struct d_tests_starttls_outcome refused =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_REQUIRED,
                                     true,
                                     REFUSAL,
                                     D_STARTTLS_OK,
                                     D_STARTTLS_OK);
    const struct d_tests_starttls_outcome declined =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_OPPORTUNISTIC,
                                     true,
                                     REFUSAL,
                                     D_STARTTLS_OK,
                                     D_STARTTLS_OK);
    const struct d_tests_starttls_outcome garbled =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_REQUIRED,
                                     true,
                                     GARBLE,
                                     D_STARTTLS_OK,
                                     D_STARTTLS_OK);
    const struct d_tests_starttls_outcome garbled_too =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_OPPORTUNISTIC,
                                     true,
                                     GARBLE,
                                     D_STARTTLS_OK,
                                     D_STARTTLS_OK);

    result = d_tests_net_assert(d_tests_starttls_ended(
                                    &refused,
                                    D_STARTTLS_ERROR_REJECTED,
                                    D_STARTTLS_STATE_FAILED,
                                    false,
                                    "STARTTLS\r\n"),
                                TEST,
                                "REQUIRED and refused fails",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_starttls_ended(
                                    &declined,
                                    D_STARTTLS_OK,
                                    D_STARTTLS_STATE_PLAINTEXT,
                                    true,
                                    "STARTTLS\r\n"),
                                TEST,
                                "OPPORTUNISTIC and refused stays plain",
                                _counter) && result;
    result = d_tests_net_assert( (d_tests_starttls_ended(
                                      &garbled,
                                      D_STARTTLS_ERROR_PROTOCOL,
                                      D_STARTTLS_STATE_FAILED,
                                      false,
                                      "STARTTLS\r\n")) &&
                                 (d_tests_starttls_ended(
                                      &garbled_too,
                                      D_STARTTLS_ERROR_PROTOCOL,
                                      D_STARTTLS_STATE_FAILED,
                                      false,
                                      "STARTTLS\r\n")),
                                 TEST,
                                 "an unclassifiable reply is always fatal",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_policy_callbacks
  Tests the following:
  - a failure inside either callback is fatal whatever the policy
  - a reply to proceed upgrades at once
*/
bool
d_tests_sa_starttls_policy_callbacks(
    struct d_test_counter* _counter
)
{
    const char* const TEST     = "policy and callbacks";
    const char* const GO_AHEAD = "220 Go ahead\r\n";
    bool              result   = true;

    const struct d_tests_starttls_outcome unsent =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_OPPORTUNISTIC,
                                     true,
                                     GO_AHEAD,
                                     D_STARTTLS_ERROR_TRANSPORT,
                                     D_STARTTLS_OK);
    const struct d_tests_starttls_outcome unread =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_OPPORTUNISTIC,
                                     true,
                                     GO_AHEAD,
                                     D_STARTTLS_OK,
                                     D_STARTTLS_ERROR_TIMED_OUT);
    const struct d_tests_starttls_outcome upgraded =
        d_tests_starttls_negotiation(D_STARTTLS_POLICY_REQUIRED,
                                     true,
                                     GO_AHEAD,
                                     D_STARTTLS_OK,
                                     D_STARTTLS_OK);

    result = d_tests_net_assert( (d_tests_starttls_ended(
                                      &unsent,
                                      D_STARTTLS_ERROR_TRANSPORT,
                                      D_STARTTLS_STATE_FAILED,
                                      false,
                                      "")) &&
                                 (d_tests_starttls_ended(
                                      &unread,
                                      D_STARTTLS_ERROR_TIMED_OUT,
                                      D_STARTTLS_STATE_FAILED,
                                      false,
                                      "STARTTLS\r\n")),
                                 TEST,
                                 "a failing callback is fatal",
                                 _counter) && result;
    result = d_tests_net_assert(d_tests_starttls_ended(
                                    &upgraded,
                                    D_STARTTLS_OK,
                                    D_STARTTLS_STATE_SECURE,
                                    true,
                                    "STARTTLS\r\n"
                                    D_TESTS_NET_TOY_HELLO),
                                TEST,
                                "the go-ahead upgrades at once",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_negotiate_refusals
  Tests the following:
  - NULL arguments and a negotiator without a request are refused
  - a server cannot negotiate: it grants with an upgrade instead
  - none of these sends anything or changes the session
*/
bool
d_tests_sa_starttls_negotiate_refusals(
    struct d_test_counter* _counter
)
{
    const char* const                   TEST       = "negotiate refusals";
    const struct d_starttls_negotiator* negotiator =
        &d_tests_starttls_smtp_negotiator;
    struct d_starttls_negotiator        partial    = *negotiator;
    struct d_tests_starttls_smtp        smtp       =
    {
        .request_status = D_STARTTLS_OK
    };
    struct d_starttls_session           session    =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture     fixture    = { .peer = NULL };
    bool                                refuses    = false;
    bool                                result     = true;

    partial.request = NULL;

    if ( (d_tests_starttls_fixture_open(&fixture,
                                        D_STARTTLS_ROLE_SERVER,
                                        D_STARTTLS_POLICY_REQUIRED)) &&
         (d_starttls_init(&session,
                          &fixture.options,
                          fixture.connection) == D_STARTTLS_OK) )
    {
        refuses = ( (d_starttls_negotiate(NULL,
                                          negotiator,
                                          &smtp,
                                          true) ==
                     D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
                    (d_starttls_negotiate(&session,
                                          NULL,
                                          &smtp,
                                          true) ==
                     D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
                    (d_starttls_negotiate(&session,
                                          &partial,
                                          &smtp,
                                          true) ==
                     D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
                    (d_starttls_negotiate(&session,
                                          negotiator,
                                          &smtp,
                                          true) ==
                     D_STARTTLS_ERROR_STATE)            &&
                    (d_starttls_get_state(&session) ==
                     D_STARTTLS_STATE_PLAINTEXT)        &&
                    (smtp.requests == 0)                &&
                    (fixture.peer->sent == 0) );
    }

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(refuses,
                                TEST,
                                "NULLs, a partial negotiator, and a server",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_upgraded_client
  Scripts a whole client conversation -- greeting, go-ahead, toy handshake,
a sealed farewell, and the toy close_notify -- then opens `_session` with
on_upgrade recording into `_forget`, reads the greeting, and negotiates.
True if the negotiation succeeded.
*/
static bool
d_tests_starttls_upgraded_client(
    struct d_starttls_session*       _session,
    struct d_tests_starttls_fixture* _fixture,
    struct d_tests_starttls_smtp*    _smtp,
    struct d_tests_starttls_forget*  _forget
)
{
    struct d_tests_net_peer* peer = _fixture->peer;

    _fixture->options.on_upgrade         = d_tests_starttls_forget;
    _fixture->options.on_upgrade_context = _forget;

    return ( (d_tests_net_peer_say(peer,
                                   "",
                                   "220 mail.example ESMTP\r\n"))     &&
             (d_tests_net_peer_say(peer,
                                   "STARTTLS\r\n",
                                   "220 Go ahead\r\n"))               &&
             (d_tests_net_peer_say(peer,
                                   D_TESTS_NET_TOY_HELLO,
                                   D_TESTS_NET_TOY_REPLY))            &&
             (d_tests_net_peer_say_sealed(peer,
                                          "QUIT\r\n",
                                          "221 Bye\r\n"))             &&
             (d_tests_net_peer_say(peer,
                                   "",
                                   D_TESTS_NET_TOY_CLOSE))            &&
             (d_starttls_init(_session,
                              &_fixture->options,
                              _fixture->connection) == D_STARTTLS_OK) &&
             (d_tests_starttls_line_is(_session,
                                       "220 mail.example ESMTP"))     &&
             (d_starttls_negotiate(_session,
                                   &d_tests_starttls_smtp_negotiator,
                                   _smtp,
                                   true) == D_STARTTLS_OK) );
}

/*
d_tests_starttls_made_for
  Whether `_fixture`'s toy made one TLS session, which checked `_name` as its
host and sent it as SNI -- "(null)" for neither -- and `_session` holds it.
*/
static bool
d_tests_starttls_made_for(
    const struct d_tests_starttls_fixture* _fixture,
    const struct d_starttls_session*       _session,
    const char*                            _name
)
{
    return ( (_fixture->toy.creates == 1) &&
             (strcmp(_fixture->toy.last_host,
                     _name) == 0)         &&
             (strcmp(_fixture->toy.last_sni,
                     _name) == 0)         &&
             (d_starttls_tls_session(_session) != NULL) );
}

/*
d_tests_sa_starttls_upgrade_client
  Tests the following:
  - a negotiation the peer accepts makes the session secure at epoch 1,
    satisfied, with no detail, after one request and one reply
  - on_upgrade runs once, after the switch: it sees the session already
    secure and at its new epoch
  - the TLS session was made for the configured server name, checked and
    sent as SNI
  - a second upgrade or negotiation is refused
*/
bool
d_tests_sa_starttls_upgrade_client(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "client upgrade";
    struct d_tests_starttls_smtp    smtp    =
    {
        .request_status = D_STARTTLS_OK
    };
    struct d_tests_starttls_forget  forget  = { .calls = 0 };
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    bool                            result  = true;

    const bool upgraded =
        ( (d_tests_starttls_fixture_open(&fixture,
                                         D_STARTTLS_ROLE_CLIENT,
                                         D_STARTTLS_POLICY_REQUIRED)) &&
          (d_tests_starttls_upgraded_client(&session,
                                            &fixture,
                                            &smtp,
                                            &forget)) );
    const bool secure =
        ( (upgraded)                                                  &&
          (d_starttls_get_state(&session) == D_STARTTLS_STATE_SECURE) &&
          (d_starttls_is_satisfied(&session))                         &&
          (d_starttls_epoch(&session) == 1)                           &&
          (d_starttls_detail(&session) == NULL)                       &&
          (smtp.requests == 1)                                        &&
          (smtp.replies == 1)                                         &&
          (strcmp(smtp.reply,
                  "220 Go ahead") == 0) );
    const bool forgotten = ( (forget.calls == 1)          &&
                             (forget.session == &session) &&
                             (forget.secure)              &&
                             (forget.epoch == 1) );
    const bool named     =
        d_tests_starttls_made_for(&fixture,
                                  &session,
                                  D_TESTS_NET_TOY_SERVER_NAME);
    const bool once      =
        ( (upgraded)                                               &&
          (d_starttls_upgrade(&session) == D_STARTTLS_ERROR_STATE) &&
          (d_starttls_negotiate(&session,
                                &d_tests_starttls_smtp_negotiator,
                                &smtp,
                                true) == D_STARTTLS_ERROR_STATE)   &&
          (d_starttls_epoch(&session) == 1) );

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(secure,
                                TEST,
                                "secure at epoch 1, satisfied, no detail",
                                _counter) && result;
    result = d_tests_net_assert(forgotten,
                                TEST,
                                "on_upgrade runs once, after the switch",
                                _counter) && result;
    result = d_tests_net_assert(named,
                                TEST,
                                "TLS is made for the configured name",
                                _counter) && result;
    result = d_tests_net_assert(once,
                                TEST,
                                "a second upgrade or negotiation is refused",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_notifies
  Whether closing the secure session sends the toy close_notify as its last
byte and then closes the connection, once.
*/
static bool
d_tests_starttls_notifies(
    struct d_starttls_session*             _session,
    const struct d_tests_starttls_fixture* _fixture
)
{
    const struct d_tests_net_peer* peer = _fixture->peer;

    return ( (d_starttls_close(_session) == D_STARTTLS_OK) &&
             (_fixture->toy.shutdowns == 1)                &&
             (peer->sent > 0)                              &&
             (peer->output[peer->sent - 1] ==
              (unsigned char)D_TESTS_NET_TOY_CLOSE[0])     &&
             (peer->closes == 1)                           &&
             (d_starttls_get_state(_session) == D_STARTTLS_STATE_CLOSED) );
}

/*
d_tests_sa_starttls_secure_traffic
  Tests the following:
  - after the upgrade, lines travel sealed both ways
  - the peer's close_notify is an orderly end that leaves the session secure
  - closing sends close_notify, then closes the connection; destroying
    frees the TLS session
*/
bool
d_tests_sa_starttls_secure_traffic(
    struct d_test_counter* _counter
)
{
    const char* const               TEST         = "secure traffic";
    struct d_tests_starttls_smtp    smtp         =
    {
        .request_status = D_STARTTLS_OK
    };
    struct d_tests_starttls_forget  forget       = { .calls = 0 };
    struct d_starttls_session       session      =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture      = { .peer = NULL };
    const char*                     line         = NULL;
    size_t                          length       = 0;
    char                            quit[16]     = { 0 };
    char                            expected[64] = { 0 };
    bool                            result       = true;

    d_tests_net_toy_seal(quit,
                         sizeof(quit),
                         "QUIT\r\n");
    (void)snprintf(expected,
                   sizeof(expected),
                   "STARTTLS\r\n%s%s",
                   D_TESTS_NET_TOY_HELLO,
                   quit);

    const bool sealed   =
        ( (d_tests_starttls_fixture_open(&fixture,
                                         D_STARTTLS_ROLE_CLIENT,
                                         D_STARTTLS_POLICY_REQUIRED)) &&
          (d_tests_starttls_upgraded_client(&session,
                                            &fixture,
                                            &smtp,
                                            &forget))                 &&
          (d_starttls_send_line(&session,
                                "QUIT") == D_STARTTLS_OK)             &&
          (d_tests_net_peer_sent(fixture.peer,
                                 expected))                           &&
          (d_tests_starttls_line_is(&session,
                                    "221 Bye")) );
    const bool ended    =
        ( (sealed)                                                   &&
          (d_starttls_recv_line(&session,
                                &line,
                                &length) == D_STARTTLS_ERROR_CLOSED) &&
          (d_starttls_is_secure(&session)) );
    const bool notified = ( (ended) &&
                            (d_tests_starttls_notifies(&session,
                                                       &fixture)) );

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(sealed,
                                TEST,
                                "lines travel sealed both ways",
                                _counter) && result;
    result = d_tests_net_assert(ended,
                                TEST,
                                "the peer's close_notify is an orderly end",
                                _counter) && result;
    result = d_tests_net_assert( (notified) &&
                                 (fixture.toy.destroys == 1),
                                 TEST,
                                 "close sends close_notify; destroy frees",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_upgrade_server
  Tests the following:
  - a server reads the request, grants it, and upgrades: its TLS session
    names no server, even with a name in its options, and the session is
    secure at epoch 1
  - the client's next line arrives sealed and is read in the clear
*/
bool
d_tests_sa_starttls_upgrade_server(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "server upgrade";
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    char                            sealed[32] = { 0 };
    bool                            result     = true;

    d_tests_net_toy_seal(sealed,
                         sizeof(sealed),
                         "EHLO again\r\n");

    const bool scripted =
        ( (d_tests_starttls_fixture_open(&fixture,
                                         D_STARTTLS_ROLE_SERVER,
                                         D_STARTTLS_POLICY_REQUIRED)) &&
          (d_tests_net_peer_say(fixture.peer,
                                "",
                                "STARTTLS\r\n"))                      &&
          (d_tests_net_peer_say(fixture.peer,
                                "220 Go ahead\r\n",
                                D_TESTS_NET_TOY_HELLO))               &&
          (d_tests_net_peer_say(fixture.peer,
                                D_TESTS_NET_TOY_REPLY,
                                sealed)) );

    // a name in a server's options is not one its TLS session sends
    fixture.options.server_name = D_TESTS_NET_TOY_SERVER_NAME;

    const bool granted =
        ( (scripted)                                              &&
          (d_starttls_init(&session,
                           &fixture.options,
                           fixture.connection) == D_STARTTLS_OK)  &&
          (d_tests_starttls_line_is(&session,
                                    "STARTTLS"))                  &&
          (d_starttls_send_line(&session,
                                "220 Go ahead") == D_STARTTLS_OK) &&
          (d_starttls_upgrade(&session) == D_STARTTLS_OK)         &&
          (d_starttls_is_secure(&session))                        &&
          (d_starttls_epoch(&session) == 1)                       &&
          (d_tests_starttls_made_for(&fixture,
                                     &session,
                                     "(null)")) );
    const bool heard =
        ( (granted) &&
          (d_tests_starttls_line_is(&session,
                                    "EHLO again")) );

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(granted,
                                TEST,
                                "a server grants and upgrades, unnamed",
                                _counter) && result;
    result = d_tests_net_assert(heard,
                                TEST,
                                "the client's next line arrives sealed",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_upgrade_implicit
  Tests the following:
  - implicit TLS: a client upgrades as the first thing it does, sending
    only its handshake
*/
bool
d_tests_sa_starttls_upgrade_implicit(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "implicit upgrade";
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    bool                            result  = true;

    const bool implicit =
        ( (d_tests_starttls_fixture_open(&fixture,
                                         D_STARTTLS_ROLE_CLIENT,
                                         D_STARTTLS_POLICY_REQUIRED)) &&
          (d_tests_net_peer_say(fixture.peer,
                                D_TESTS_NET_TOY_HELLO,
                                D_TESTS_NET_TOY_REPLY))               &&
          (d_starttls_init(&session,
                           &fixture.options,
                           fixture.connection) == D_STARTTLS_OK)      &&
          (d_starttls_upgrade(&session) == D_STARTTLS_OK)             &&
          (d_starttls_is_secure(&session))                            &&
          (d_tests_net_peer_sent(fixture.peer,
                                 D_TESTS_NET_TOY_HELLO)) );

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(implicit,
                                TEST,
                                "implicit TLS upgrades at once",
                                _counter) && result;

    return result;
}

// d_tests_starttls_leftover
//   struct: one leftover case: what follows the go-ahead and how, and the
// session's settings.
struct d_tests_starttls_leftover
{
    const char* extra;            // sent after the go-ahead, or NULL
    bool        together;         // in the go-ahead's read, or its own
    bool        reject_leftover;  // the session's setting
    bool        countable;        // whether the connection can count
    bool        uncounted;        // whether its count is refused
};

/*
d_tests_starttls_leftover_case
  One client negotiation over a toy fixture whose peer follows its go-ahead
as `_case` says and then completes the toy handshake. The session's buffer
is D_STARTTLS_BUFFER_MIN. *_creates receives the TLS sessions made, and
*_discarded the bytes the session dropped.
*/
static enum d_starttls_error
d_tests_starttls_leftover_case(
    const struct d_tests_starttls_leftover* _case,
    int*                                    _creates,
    size_t*                                 _discarded
)
{
    struct d_tests_starttls_smtp    smtp    =
    {
        .request_status = D_STARTTLS_OK
    };
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    enum d_starttls_error           status  = D_TESTS_STARTTLS_UNREACHED;
    const bool                      apart   = ( (_case->extra) &&
                                                (!_case->together) );
    static char                     first[1100] = { 0 };

    (void)snprintf(first,
                   sizeof(first),
                   "220 Go ahead\r\n%s",
                   ( (_case->extra) &&
                     (_case->together) ) ? _case->extra : "");

    if ( (d_tests_starttls_fixture_open(&fixture,
                                        D_STARTTLS_ROLE_CLIENT,
                                        D_STARTTLS_POLICY_REQUIRED)) &&
         (d_tests_net_peer_say(fixture.peer,
                               "STARTTLS\r\n",
                               first))                               &&
         ( (!apart) ||
           (d_tests_net_peer_say(fixture.peer,
                                 "",
                                 _case->extra)) )                    &&
         (d_tests_net_peer_say(fixture.peer,
                               D_TESTS_NET_TOY_HELLO,
                               D_TESTS_NET_TOY_REPLY)) )
    {
        fixture.peer->uncounted         = _case->uncounted;
        fixture.options.reject_leftover = _case->reject_leftover;
        fixture.options.buffer_bytes    = D_STARTTLS_BUFFER_MIN;
        fixture.connection              =
            d_tests_net_peer_connection(fixture.peer,
                                        _case->countable);

        if (d_starttls_init(&session,
                            &fixture.options,
                            fixture.connection) == D_STARTTLS_OK)
        {
            status = d_starttls_negotiate(&session,
                                          &d_tests_starttls_smtp_negotiator,
                                          &smtp,
                                          true);
        }
    }

    *_creates   = fixture.toy.creates;
    *_discarded = session.discarded;
    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);

    return status;
}

/*
d_tests_sa_starttls_injection_client
  Tests the following:
  - a client refuses plaintext the peer sent after its go-ahead, whether
    the reply's read already buffered it or it still waits in the
    connection, a single byte included; the refusal comes before any TLS
    session exists
  - a connection that cannot count leaves the rest to the handshake, whose
    sniff then finds the stray bytes are not TLS
  - a connection whose count is refused is taken as unable to count
*/
bool
d_tests_sa_starttls_injection_client(
    struct d_test_counter* _counter
)
{
    const char* const                      TEST     = "client injection";
    const char* const                      INJECTED = "250 injected\r\n";
    const struct d_tests_starttls_leftover cases[5] =
    {
        { INJECTED, true,  true, true,  false },
        { INJECTED, false, true, true,  false },
        { INJECTED, false, true, false, false },
        { NULL,     false, true, true,  true  },
        { "X",      true,  true, true,  false }
    };
    enum d_starttls_error                  status[5]  = { D_STARTTLS_OK };
    int                                    creates[5] = { 0 };
    size_t                                 discarded  = 0;
    bool                                   result     = true;

    for (size_t i = 0; i < 5; ++i)
    {
        status[i] = d_tests_starttls_leftover_case(&cases[i],
                                                   &creates[i],
                                                   &discarded);
    }

    result = d_tests_net_assert( (status[0] == D_STARTTLS_ERROR_INJECTION) &&
                                 (creates[0] == 0)                         &&
                                 (status[4] == D_STARTTLS_ERROR_INJECTION) &&
                                 (creates[4] == 0),
                                 TEST,
                                 "buffered plaintext is refused, unencrypted",
                                 _counter) && result;
    result = d_tests_net_assert( (status[1] == D_STARTTLS_ERROR_INJECTION) &&
                                 (creates[1] == 0),
                                 TEST,
                                 "plaintext waiting in the connection too",
                                 _counter) && result;
    result = d_tests_net_assert( (status[2] == D_STARTTLS_ERROR_HANDSHAKE) &&
                                 (creates[2] == 1),
                                 TEST,
                                 "uncounted, the handshake trips on it",
                                 _counter) && result;
    result = d_tests_net_assert( (status[3] == D_STARTTLS_OK) &&
                                 (creates[3] == 1),
                                 TEST,
                                 "a count that fails is no count",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_leftover_dropped
  Tests the following:
  - with reject_leftover off, plaintext buffered past the go-ahead is
    dropped and counted, and the upgrade completes
  - so is plaintext still waiting in the connection, a single byte of it,
    or more of it than the receive buffer holds
*/
bool
d_tests_sa_starttls_leftover_dropped(
    struct d_test_counter* _counter
)
{
    const char* const TEST      = "leftover dropped";
    const char* const INJECTED  = "250 injected\r\n";
    static char       flood[1001] = { 0 };
    int               creates   = 0;
    size_t            dropped[4];
    bool              result    = true;

    memset(flood,
           'j',
           sizeof(flood) - 1);
    flood[sizeof(flood) - 1] = '\0';

    const struct d_tests_starttls_leftover cases[4] =
    {
        { INJECTED, true,  false, true, false },
        { INJECTED, false, false, true, false },
        { flood,    false, false, true, false },
        { "x",      false, false, true, false }
    };
    enum d_starttls_error                  status[4] = { D_STARTTLS_OK };

    for (size_t i = 0; i < 4; ++i)
    {
        status[i] = d_tests_starttls_leftover_case(&cases[i],
                                                   &creates,
                                                   &dropped[i]);
    }

    result = d_tests_net_assert( (status[0] == D_STARTTLS_OK) &&
                                 (dropped[0] == 14),
                                 TEST,
                                 "buffered plaintext is dropped and counted",
                                 _counter) && result;
    result = d_tests_net_assert( (status[1] == D_STARTTLS_OK) &&
                                 (dropped[1] == 14)           &&
                                 (status[3] == D_STARTTLS_OK) &&
                                 (dropped[3] == 1),
                                 TEST,
                                 "waiting plaintext is drained and counted",
                                 _counter) && result;
    result = d_tests_net_assert( (status[2] == D_STARTTLS_OK) &&
                                 (dropped[2] == 1000),
                                 TEST,
                                 "more than the buffer holds is drained too",
                                 _counter) && result;

    return result;
}

/*
d_tests_starttls_server_upgrade_after
  A server fixture whose client sends `_first` and then, in a read of its
own, `_second` (none if NULL); the server reads the STARTTLS line and
upgrades. Returns what the upgrade reported; *_creates receives the TLS
sessions made.
*/
static enum d_starttls_error
d_tests_starttls_server_upgrade_after(
    const char* _first,
    const char* _second,
    int*        _creates
)
{
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    enum d_starttls_error           status  = D_TESTS_STARTTLS_UNREACHED;

    if ( (d_tests_starttls_fixture_open(&fixture,
                                        D_STARTTLS_ROLE_SERVER,
                                        D_STARTTLS_POLICY_REQUIRED)) &&
         (d_tests_net_peer_say(fixture.peer,
                               "",
                               _first))                              &&
         ( (!_second) ||
           (d_tests_net_peer_say(fixture.peer,
                                 "",
                                 _second)) )                         &&
         (d_starttls_init(&session,
                          &fixture.options,
                          fixture.connection) == D_STARTTLS_OK)      &&
         (d_tests_starttls_line_is(&session,
                                   "STARTTLS")) )
    {
        status = d_starttls_upgrade(&session);
    }

    *_creates = fixture.toy.creates;
    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);

    return status;
}

/*
d_tests_sa_starttls_injection_server
  Tests the following:
  - a server refuses what it buffered past the request -- the injection of
    CVE-2011-0411 -- before any TLS session exists
  - it does not look into the connection, where a client's first handshake
    bytes may already be waiting
*/
bool
d_tests_sa_starttls_injection_server(
    struct d_test_counter* _counter
)
{
    const char* const TEST       = "server injection";
    int               creates[2] = { 0, 0 };
    bool              result     = true;

    const enum d_starttls_error pipelined =
        d_tests_starttls_server_upgrade_after("STARTTLS\r\n"
                                              "MAIL FROM:<evil>\r\n",
                                              NULL,
                                              &creates[0]);
    const enum d_starttls_error prompt =
        d_tests_starttls_server_upgrade_after("STARTTLS\r\n",
                                              D_TESTS_NET_TOY_HELLO,
                                              &creates[1]);

    result = d_tests_net_assert( (pipelined == D_STARTTLS_ERROR_INJECTION) &&
                                 (creates[0] == 0),
                                 TEST,
                                 "a server refuses a pipelined command",
                                 _counter) && result;
    result = d_tests_net_assert( (prompt == D_STARTTLS_OK) &&
                                 (creates[1] == 1),
                                 TEST,
                                 "a server lets a waiting ClientHello be",
                                 _counter) && result;

    return result;
}

/*
d_tests_starttls_failing
  A client negotiation whose go-ahead is followed, if `_answers`, by the toy
reply, with the engine set up as `_toy` says; the peer hangs up after its
last turn. The result is D_TESTS_STARTTLS_UNREACHED unless any failure
also left the session failed at epoch 0. *_toy receives the engine's
record, and *_detail the session's detail before it was closed.
*/
static enum d_starttls_error
d_tests_starttls_failing(
    struct d_tests_net_toy*         _toy,
    bool                            _answers,
    struct d_tests_starttls_forget* _forget,
    const char**                    _detail
)
{
    struct d_tests_starttls_smtp    smtp    =
    {
        .request_status = D_STARTTLS_OK
    };
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    enum d_starttls_error           status  = D_TESTS_STARTTLS_UNREACHED;

    if ( (d_tests_starttls_fixture_open(&fixture,
                                        D_STARTTLS_ROLE_CLIENT,
                                        D_STARTTLS_POLICY_REQUIRED)) &&
         (d_tests_net_peer_say(fixture.peer,
                               "STARTTLS\r\n",
                               "220 Go ahead\r\n"))                  &&
         ( (!_answers) ||
           (d_tests_net_peer_say(fixture.peer,
                                 D_TESTS_NET_TOY_HELLO,
                                 D_TESTS_NET_TOY_REPLY)) ) )
    {
        _toy->role                         = D_SSL_ROLE_CLIENT;
        fixture.toy                        = *_toy;
        fixture.peer->hangs_up             = true;
        fixture.options.on_upgrade         = d_tests_starttls_forget;
        fixture.options.on_upgrade_context = _forget;

        if (d_starttls_init(&session,
                            &fixture.options,
                            fixture.connection) == D_STARTTLS_OK)
        {
            status = d_starttls_negotiate(&session,
                                          &d_tests_starttls_smtp_negotiator,
                                          &smtp,
                                          true);
        }
    }

    if ( (status != D_STARTTLS_OK) &&
         ( (d_starttls_get_state(&session) != D_STARTTLS_STATE_FAILED) ||
           (d_starttls_epoch(&session) != 0) ) )
    {
        status = D_TESTS_STARTTLS_UNREACHED;
    }

    *_detail = d_starttls_detail(&session);
    (void)d_starttls_close(&session);
    d_starttls_destroy(&session);
    *_toy = fixture.toy;
    d_tests_starttls_fixture_close(&fixture);

    return status;
}

/*
d_tests_sa_starttls_engine_failures
  Tests the following:
  - a TLS session the engine will not make fails the upgrade before any
    handshake: CONFIG, whatever the engine called it, or OUT_OF_MEMORY when
    it ran out; an engine that runs out during the handshake is
    OUT_OF_MEMORY too
  - a peer whose certificate expired, or names another host, is
    D_STARTTLS_ERROR_VERIFY with the reason as its detail
  - a peer that hangs up mid-handshake is a handshake failure
  - every failure is fatal and leaves the epoch at 0; on_upgrade never runs,
    a failed session sends no close_notify, and its TLS session is freed
*/
bool
d_tests_sa_starttls_engine_failures(
    struct d_test_counter* _counter
)
{
    const char* const              TEST       = "engine failures";
    struct d_tests_net_toy         toys[7]    =
    {
        { .create_status = D_SSL_OK }
    };
    struct d_tests_starttls_forget forget     = { .calls = 0 };
    const char*                    details[7] = { NULL };
    enum d_starttls_error          status[7]  = { D_STARTTLS_OK };
    bool                           result     = true;

    toys[0].create_status    = D_SSL_ERROR_CONFIG;
    toys[1].create_status    = D_SSL_ERROR_OUT_OF_MEMORY;
    toys[2].verdict          = (uint32_t)D_SSL_VERIFY_FLAG_EXPIRED;
    toys[3].cert_name        = "other.example";
    toys[5].create_status    = D_SSL_ERROR_INVALID_ARGUMENT;
    toys[6].handshake_status = D_SSL_ERROR_OUT_OF_MEMORY;

    // each case; the fifth one's peer never answers the hello
    for (size_t i = 0; i < 7; ++i)
    {
        status[i] = d_tests_starttls_failing(&toys[i],
                                             (i != 4),
                                             &forget,
                                             &details[i]);
    }

    const bool unmade =
        ( (status[0] == D_STARTTLS_ERROR_CONFIG)        &&
          (toys[0].destroys == 0)                       &&
          (details[0] == NULL)                          &&
          (status[1] == D_STARTTLS_ERROR_OUT_OF_MEMORY) &&
          (status[5] == D_STARTTLS_ERROR_CONFIG)        &&
          (details[5] == NULL)                          &&
          (status[6] == D_STARTTLS_ERROR_OUT_OF_MEMORY) );

    result = d_tests_net_assert(unmade,
                                TEST,
                                "a session not made, or out of memory",
                                _counter) && result;
    result = d_tests_net_assert( (status[2] == D_STARTTLS_ERROR_VERIFY) &&
                                 (details[2] != NULL)                   &&
                                 (strcmp(details[2],
                                         "expired") == 0)               &&
                                 (status[3] == D_STARTTLS_ERROR_VERIFY) &&
                                 (details[3] != NULL)                   &&
                                 (strcmp(details[3],
                                         "hostname mismatch") == 0)     &&
                                 (toys[2].shutdowns == 0)               &&
                                 (toys[2].destroys == 1),
                                 TEST,
                                 "a refused certificate, with its reason",
                                 _counter) && result;
    result = d_tests_net_assert( (status[4] == D_STARTTLS_ERROR_HANDSHAKE) &&
                                 (details[4] != NULL)                      &&
                                 (strcmp(details[4],
                                         "TRUNCATED") == 0)                &&
                                 (toys[4].destroys == 1),
                                 TEST,
                                 "a peer that hangs up mid-handshake",
                                 _counter) && result;
    result = d_tests_net_assert(forget.calls == 0,
                                TEST,
                                "on_upgrade never runs for a failure",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_alerting
  Sets `_toy` to fail its handshakes with `_status`, reporting `_sent` and
`_received` as the alerts of the failure.
*/
static void
d_tests_starttls_alerting(
    struct d_tests_net_toy* _toy,
    enum d_ssl_error        _status,
    int32_t                 _sent,
    int32_t                 _received
)
{
    _toy->handshake_status = _status;
    _toy->alerts           = true;
    _toy->alert_sent       = _sent;
    _toy->alert_received   = _received;

    return;
}

/*
d_tests_starttls_failed_as
  Whether a failure that reported `_status`, described as `_detail`, was
`_expected`, described as `_described`.
*/
static bool
d_tests_starttls_failed_as(
    enum d_starttls_error _status,
    const char*           _detail,
    enum d_starttls_error _expected,
    const char*           _described
)
{
    return ( (_status == _expected) &&
             (_detail != NULL)      &&
             (strcmp(_detail,
                     _described) == 0) );
}

/*
d_tests_starttls_detail_cases
  The six engine records of d_tests_sa_starttls_failure_details: two
certificates refused for more than one reason, three handshakes failed with
alerts, and one failed by the engine's configuration.
*/
static void
d_tests_starttls_detail_cases(
    struct d_tests_net_toy* _toys
)
{
    _toys[0].verdict          = (uint32_t)(D_SSL_VERIFY_FLAG_EXPIRED |
                                           D_SSL_VERIFY_FLAG_UNTRUSTED);
    _toys[1].verdict          = (uint32_t)(D_SSL_VERIFY_FLAG_NOT_PERFORMED |
                                           D_SSL_VERIFY_FLAG_EXPIRED);
    _toys[5].handshake_status = D_SSL_ERROR_CONFIG;
    d_tests_starttls_alerting(&_toys[2],
                              D_SSL_ERROR_HANDSHAKE,
                              D_SSL_ALERT_NONE,
                              D_SSL_ALERT_HANDSHAKE_FAILURE);
    d_tests_starttls_alerting(&_toys[3],
                              D_SSL_ERROR_PROTOCOL,
                              D_SSL_ALERT_DECODE_ERROR,
                              D_SSL_ALERT_NONE);
    d_tests_starttls_alerting(&_toys[4],
                              D_SSL_ERROR_HANDSHAKE,
                              D_SSL_ALERT_DECODE_ERROR,
                              D_SSL_ALERT_PROTOCOL_VERSION);

    return;
}

/*
d_tests_sa_starttls_failure_details
  Tests the following:
  - a certificate refused for several reasons is described by the lowest of
    them, and one the engine also reports unchecked by the reason it failed
  - a handshake the engine fails is described by the alert the peer sent,
    or failing that by the one this end sent; a protocol violation during
    it is a handshake failure
  - a handshake failure the engine puts down to its configuration is CONFIG
*/
bool
d_tests_sa_starttls_failure_details(
    struct d_test_counter* _counter
)
{
    const char* const              TEST       = "failure details";
    struct d_tests_net_toy         toys[6]    =
    {
        { .create_status = D_SSL_OK }
    };
    struct d_tests_starttls_forget forget     = { .calls = 0 };
    const char*                    details[6] = { NULL };
    enum d_starttls_error          status[6]  = { D_STARTTLS_OK };
    bool                           result     = true;

    d_tests_starttls_detail_cases(toys);

    for (size_t i = 0; i < 6; ++i)
    {
        status[i] = d_tests_starttls_failing(&toys[i],
                                             true,
                                             &forget,
                                             &details[i]);
    }

    result = d_tests_net_assert( (d_tests_starttls_failed_as(
                                      status[0],
                                      details[0],
                                      D_STARTTLS_ERROR_VERIFY,
                                      "untrusted issuer")) &&
                                 (d_tests_starttls_failed_as(
                                      status[1],
                                      details[1],
                                      D_STARTTLS_ERROR_VERIFY,
                                      "expired")),
                                 TEST,
                                 "a refused certificate's first reason",
                                 _counter) && result;
    result = d_tests_net_assert( (d_tests_starttls_failed_as(
                                      status[2],
                                      details[2],
                                      D_STARTTLS_ERROR_HANDSHAKE,
                                      "handshake_failure")) &&
                                 (d_tests_starttls_failed_as(
                                      status[3],
                                      details[3],
                                      D_STARTTLS_ERROR_HANDSHAKE,
                                      "decode_error"))      &&
                                 (d_tests_starttls_failed_as(
                                      status[4],
                                      details[4],
                                      D_STARTTLS_ERROR_HANDSHAKE,
                                      "protocol_version")),
                                 TEST,
                                 "a failed handshake's alerts",
                                 _counter) && result;
    result = d_tests_net_assert(d_tests_starttls_failed_as(
                                    status[5],
                                    details[5],
                                    D_STARTTLS_ERROR_CONFIG,
                                    "CONFIG"),
                                TEST,
                                "an engine's configuration is CONFIG",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_upgrade_disabled
  Tests the following:
  - a session whose policy is DISABLED refuses to upgrade, and stays
    plaintext
*/
bool
d_tests_sa_starttls_upgrade_disabled(
    struct d_test_counter* _counter
)
{
    const char* const         TEST    = "upgrade disabled";
    struct d_tests_net_peer*  peer    = d_tests_net_peer_new();
    struct d_starttls_session session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    bool                      refused = false;
    bool                      result  = true;

    if ( (peer) &&
         (d_tests_starttls_plain(&session,
                                 peer,
                                 0,
                                 true)) )
    {
        refused = ( (d_starttls_upgrade(&session) == D_STARTTLS_ERROR_STATE) &&
                    (d_starttls_get_state(&session) ==
                     D_STARTTLS_STATE_PLAINTEXT) );
    }

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(refused,
                                TEST,
                                "an upgrade under DISABLED is refused",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_sealed_session
  A client fixture that upgrades at once, sends a few bytes and a line
under the toy engine, and reads the sealed line its peer sends before
hanging up without close_notify. True if all of that went as expected.
*/
static bool
d_tests_starttls_sealed_session(
    struct d_starttls_session*       _session,
    struct d_tests_starttls_fixture* _fixture
)
{
    char sealed[32]   = { 0 };
    char expected[64] = { 0 };

    d_tests_net_toy_seal(sealed,
                         sizeof(sealed),
                         "abca1 NOOP\r\n");
    (void)snprintf(expected,
                   sizeof(expected),
                   "%s%s",
                   D_TESTS_NET_TOY_HELLO,
                   sealed);
    _fixture->peer->hangs_up = true;

    return ( (d_tests_net_peer_say(_fixture->peer,
                                   D_TESTS_NET_TOY_HELLO,
                                   D_TESTS_NET_TOY_REPLY))            &&
             (d_tests_net_peer_say_sealed(_fixture->peer,
                                          "",
                                          "* OK ready\r\n"))          &&
             (d_starttls_init(_session,
                              &_fixture->options,
                              _fixture->connection) == D_STARTTLS_OK) &&
             (d_starttls_upgrade(_session) == D_STARTTLS_OK)          &&
             (d_starttls_send(_session,
                              "abc",
                              3) == D_STARTTLS_OK)                    &&
             (d_starttls_send_line(_session,
                                   "a1 NOOP") == D_STARTTLS_OK)       &&
             (d_tests_net_peer_sent(_fixture->peer,
                                    expected))                        &&
             (d_tests_starttls_line_is(_session,
                                       "* OK ready")) );
}

/*
d_tests_sa_starttls_secure_end
  Tests the following:
  - bytes and lines sent after the upgrade go out sealed
  - a connection that ends without close_notify is
    D_STARTTLS_ERROR_TRUNCATED, fatal, with the kernel's name for it as the
    detail
  - closing a session that failed sends no close_notify
*/
bool
d_tests_sa_starttls_secure_end(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "secure end";
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    const char*                     line    = NULL;
    size_t                          length  = 0;
    bool                            result  = true;

    const bool upgraded =
        ( (d_tests_starttls_fixture_open(&fixture,
                                         D_STARTTLS_ROLE_CLIENT,
                                         D_STARTTLS_POLICY_REQUIRED)) &&
          (d_tests_starttls_sealed_session(&session,
                                           &fixture)) );
    const bool truncated =
        ( (upgraded)                                                    &&
          (d_starttls_recv_line(&session,
                                &line,
                                &length) == D_STARTTLS_ERROR_TRUNCATED) &&
          (d_starttls_get_state(&session) == D_STARTTLS_STATE_FAILED)   &&
          (d_starttls_detail(&session) != NULL)                         &&
          (strcmp(d_starttls_detail(&session),
                  "TRUNCATED") == 0) );
    const size_t sent   = (upgraded) ? fixture.peer->sent : 0;
    const bool   silent = ( (truncated)                                   &&
                            (d_starttls_close(&session) == D_STARTTLS_OK) &&
                            (fixture.toy.shutdowns == 0)                  &&
                            (fixture.peer->sent == sent) );

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(upgraded,
                                TEST,
                                "bytes and lines go out sealed",
                                _counter) && result;
    result = d_tests_net_assert(truncated,
                                TEST,
                                "an end without close_notify is truncation",
                                _counter) && result;
    result = d_tests_net_assert(silent,
                                TEST,
                                "a failed session closes without notifying",
                                _counter) && result;

    return result;
}
