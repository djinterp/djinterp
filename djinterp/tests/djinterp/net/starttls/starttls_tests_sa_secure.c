/*******************************************************************************
* djinterp [net]                                      starttls_tests_sa_secure.c
*
* Standalone tests of a STARTTLS session once it is secure, against a
* scripted peer and the SSL kernel's toy engine: how each way its connection
* or its TLS can fail is reported, with the connection's own error kept as
* the cause and the failure described as itself; and a server that asks for
* a client certificate without requiring one.
*
*
* path:      /tests/djinterp/net/starttls/starttls_tests_sa_secure.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./starttls_tests_sa.h"  // the suite
// std
#include <string.h>  // strcmp


// d_tests_starttls_breakage
//   struct: a secure client session broken one way, the call that meets the
// breakage -- 'r' receives a line, 's' sends one, 'c' closes -- and what
// that call must report: its status, the connection's error it keeps as the
// cause, and the session's detail.
struct d_tests_starttls_breakage
{
    const char*           name;     // the assertion's message
    bool                  lies;     // the peer's reads claim too much
    bool                  refuses;  // its writes fail from now on
    bool                  resets;   // as a reset connection's would
    enum d_ssl_error      record;   // what the toy says of records
    char                  call;     // the call that meets it
    enum d_starttls_error status;   // what the call reports
    enum d_net_error      cause;    // the connection's error kept
    const char*           detail;   // the session's detail
};

/*
d_tests_starttls_secure_client
  Opens `_fixture` for a client that upgrades at once, as implicit TLS does,
and `_session` over it; true if the session is secure.
*/
static bool
d_tests_starttls_secure_client(
    struct d_starttls_session*       _session,
    struct d_tests_starttls_fixture* _fixture
)
{
    return ( (d_tests_starttls_fixture_open(_fixture,
                                            D_STARTTLS_ROLE_CLIENT,
                                            D_STARTTLS_POLICY_REQUIRED)) &&
             (d_tests_net_peer_say(_fixture->peer,
                                   D_TESTS_NET_TOY_HELLO,
                                   D_TESTS_NET_TOY_REPLY))               &&
             (d_starttls_init(_session,
                              &_fixture->options,
                              _fixture->connection) == D_STARTTLS_OK)    &&
             (d_starttls_upgrade(_session) == D_STARTTLS_OK) );
}

/*
d_tests_starttls_meets
  Makes the call `_case` names on `_session`.
*/
static enum d_starttls_error
d_tests_starttls_meets(
    struct d_starttls_session*              _session,
    const struct d_tests_starttls_breakage* _case
)
{
    const char* line   = NULL;
    size_t      length = 0;

    // a line in, a line out, or the end
    if (_case->call == 'r')
    {
        return d_starttls_recv_line(_session,
                                    &line,
                                    &length);
    }

    if (_case->call == 's')
    {
        return d_starttls_send_line(_session,
                                    "NOOP");
    }

    return d_starttls_close(_session);
}

/*
d_tests_starttls_breaks
  Opens a secure client, breaks it as `_case` says, and makes the call the
case names. True if the call reported what the case expects, kept its cause
and detail, and left the session failed -- or closed, after a close.
*/
static bool
d_tests_starttls_breaks(
    const struct d_tests_starttls_breakage* _case
)
{
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    enum d_starttls_error           status  = D_TESTS_STARTTLS_UNREACHED;
    const enum d_starttls_state     after   =
        (_case->call == 'c') ? D_STARTTLS_STATE_CLOSED
                             : D_STARTTLS_STATE_FAILED;

    if (d_tests_starttls_secure_client(&session,
                                       &fixture))
    {
        fixture.peer->lies      = _case->lies;
        fixture.peer->resets    = _case->resets;
        fixture.toy.read_status = _case->record;

        // every write fails from here on
        if (_case->refuses)
        {
            fixture.peer->fail_after = fixture.peer->sent;
        }

        status = d_tests_starttls_meets(&session,
                                        _case);
    }

    const char* const detail = d_starttls_detail(&session);
    const bool        held   =
        ( (status == _case->status)                        &&
          (d_starttls_get_state(&session) == after)        &&
          (d_starttls_net_error(&session) == _case->cause) &&
          (detail != NULL)                                 &&
          (strcmp(detail,
                  _case->detail) == 0) );

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);

    return held;
}

/*
d_tests_sa_starttls_secure_failures
  Tests the following:
  - a read through TLS that stalls is TIMED_OUT, and one the connection
    fails is TRANSPORT, each keeping the connection's error as the cause
  - a record the engine refuses is PROTOCOL
  - a write through TLS the connection fails is TRANSPORT; one that finds
    the connection reset is TRUNCATED, and so is a close_notify that does
  - each is fatal, but for the close, which closes; each is described
*/
bool
d_tests_sa_starttls_secure_failures(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_starttls_breakage CASES[] =
    {
        { "a stalled read times out",
          false, false, false, D_SSL_OK,              'r',
          D_STARTTLS_ERROR_TIMED_OUT,  D_NET_ERROR_TIMED_OUT,
          "TRANSPORT" },
        { "a broken read is TRANSPORT, with its cause",
          true,  false, false, D_SSL_OK,              'r',
          D_STARTTLS_ERROR_TRANSPORT,  D_NET_ERROR_UNKNOWN,
          "TRANSPORT" },
        { "a refused record is PROTOCOL",
          false, false, false, D_SSL_ERROR_PROTOCOL,  'r',
          D_STARTTLS_ERROR_PROTOCOL,   D_NET_OK,
          "PROTOCOL" },
        { "a broken write is TRANSPORT, with its cause",
          false, true,  false, D_SSL_OK,              's',
          D_STARTTLS_ERROR_TRANSPORT,  D_NET_ERROR_UNKNOWN,
          "TRANSPORT" },
        { "a write into a reset is TRUNCATED",
          false, true,  true,  D_SSL_OK,              's',
          D_STARTTLS_ERROR_TRUNCATED,  D_NET_ERROR_CONNECTION_RESET,
          "TRUNCATED" },
        { "a close_notify into a reset is TRUNCATED",
          false, true,  true,  D_SSL_OK,              'c',
          D_STARTTLS_ERROR_TRUNCATED,  D_NET_ERROR_CONNECTION_RESET,
          "TRUNCATED" }
    };
    bool result = true;

    // each breakage on a session of its own
    for (size_t i = 0; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        result = d_tests_net_assert(d_tests_starttls_breaks(&CASES[i]),
                                    "secure failures",
                                    CASES[i].name,
                                    _counter) && result;
    }

    return result;
}

/*
d_tests_starttls_optional_server
  Opens `_fixture` for a server whose context asks for a client certificate
without requiring one, with a client that sends the toy hello and presents
none, and `_session` over it, upgraded; true if the session is secure.
*/
static bool
d_tests_starttls_optional_server(
    struct d_starttls_session*       _session,
    struct d_tests_starttls_fixture* _fixture
)
{
    // the fixture's server asks nothing of clients, so it is made again
    if ( (!d_tests_starttls_fixture_open(_fixture,
                                         D_STARTTLS_ROLE_SERVER,
                                         D_STARTTLS_POLICY_REQUIRED)) ||
         (!d_tests_net_peer_say(_fixture->peer,
                                "",
                                D_TESTS_NET_TOY_HELLO)) )
    {
        return false;
    }

    d_ssl_context_destroy(&_fixture->tls);

    return ( (d_tests_net_toy_context(&_fixture->tls,
                                      &_fixture->toy,
                                      D_SSL_ROLE_SERVER,
                                      true))                          &&
             (d_starttls_init(_session,
                              &_fixture->options,
                              _fixture->connection) == D_STARTTLS_OK) &&
             (d_starttls_upgrade(_session) == D_STARTTLS_OK) );
}

/*
d_tests_sa_starttls_optional_client
  Tests the following:
  - a server that asks for a client certificate without requiring one
    upgrades a client that presents none, which the kernel records
  - a later failure is described as itself, not as the certificate the
    client never presented
*/
bool
d_tests_sa_starttls_optional_client(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "optional client certificate";
    const uint32_t                  NONE    =
        (uint32_t)(D_SSL_VERIFY_FLAG_NO_CERTIFICATE |
                   D_SSL_VERIFY_FLAG_NOT_PERFORMED);
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    struct d_ssl_info               info    = { .verify = 0u };
    const char*                     line    = NULL;
    size_t                          length  = 0;
    bool                            result  = true;

    const bool upgraded =
        ( (d_tests_starttls_optional_server(&session,
                                            &fixture)) &&
          (d_ssl_session_info(d_starttls_tls_session(&session),
                              &info))                  &&
          (info.verify == NONE) );

    // the client goes without close_notify
    if (fixture.peer)
    {
        fixture.peer->hangs_up = true;
    }

    const bool described =
        ( (upgraded)                            &&
          (d_starttls_recv_line(&session,
                                &line,
                                &length) ==
           D_STARTTLS_ERROR_TRUNCATED)          &&
          (d_starttls_detail(&session) != NULL) &&
          (strcmp(d_starttls_detail(&session),
                  "TRUNCATED") == 0) );

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(upgraded,
                                TEST,
                                "a client without one is upgraded",
                                _counter) && result;
    result = d_tests_net_assert(described,
                                TEST,
                                "a later failure is described as itself",
                                _counter) && result;

    return result;
}
