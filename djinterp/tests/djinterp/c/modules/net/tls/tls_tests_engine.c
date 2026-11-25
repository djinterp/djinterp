/*******************************************************************************
* djinterp [c][net]                                           tls_tests_engine.c
*
* Tests of what the TLS kernel hands an engine, and of how it holds one to
* its contract.
*   The engine is the OpenSSL-family one with three of its operations behind
* a switch: session_create, which notes the names it is handed, feed, and
* drain. Thrown, the switch has its operation answer in the engine's place:
* WANT_READ or CLOSED where the contract allows neither, a failure, or a
* count larger than the room it was given. The kernel must make BACKEND of
* each, so no engine can keep a session waiting or pass for its transport.
*
*
* path:      /tests/djinterp/c/modules/net/tls/tls_tests_engine.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./tls_tests.h"  // corresponding header
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memset, strcmp, strlen


// dj_tests_tls_switched
//   enum: the switched engine's operations, any one of which the switch can
// throw.
enum dj_tests_tls_switched
{
    D_TESTS_TLS_SWITCHED_NONE = 0,
    D_TESTS_TLS_SWITCHED_CREATE,
    D_TESTS_TLS_SWITCHED_FEED,
    D_TESTS_TLS_SWITCHED_DRAIN
};

// dj_tests_tls_switch
//   struct: the operation thrown and what it answers instead - for drain,
// OK is the lie of a byte more than its room - and the names the last
// session_create was handed, "(null)" for none.
struct dj_tests_tls_switch
{
    enum dj_tests_tls_switched thrown;
    enum dj_tls_error          answer;
    char                      host[D_TLS_HOSTNAME_MAX + 2];
    char                      sni[D_TLS_HOSTNAME_MAX + 2];
};

// dj_tests_tls_switched_set
//   struct: the switched engine, and a context over it for a client that
// checks names, a client that does not, and a server. It must not move once
// opened: the contexts point at its engine.
struct dj_tests_tls_switched_set
{
    struct dj_tls_engine  engine;
    struct dj_tls_context checking;
    struct dj_tls_context trusting;
    struct dj_tls_context serving;
};

// d_tests_tls_the_switch
//   variable: the run's switch, thrown by a test and reset when it is done.
static struct dj_tests_tls_switch d_tests_tls_the_switch =
{
    D_TESTS_TLS_SWITCHED_NONE,
    D_TLS_OK,
    { '\0' },
    { '\0' }
};

/*
d_tests_tls_switch_throw
  Throws the switch: `_operation` answers `_answer` until the switch is
thrown again. NONE resets it.
*/
static void
d_tests_tls_switch_throw(
    enum dj_tests_tls_switched _operation,
    enum dj_tls_error          _answer
)
{
    d_tests_tls_the_switch.thrown = _operation;
    d_tests_tls_the_switch.answer = _answer;

    return;
}

/*
d_tests_tls_switch_note
  Copies a name session_create was handed, or "(null)" for none.
*/
static
void d_tests_tls_switch_note(
    char*       _out,
    size_t      _capacity,
    const char* _name
)
{
    (void)snprintf(_out,
                   _capacity,
                   "%s",
                   (_name != NULL) ? _name : "(null)");

    return;
}

/*
d_tests_tls_switched_create
  Notes the names, then answers in the engine's place, making no session,
or passes the call on.
*/
static enum dj_tls_error
d_tests_tls_switched_create(
    void*       _context,
    const char* _host,
    const char* _sni,
    void**      _out_session
)
{
    d_tests_tls_switch_note(d_tests_tls_the_switch.host,
                            sizeof(d_tests_tls_the_switch.host),
                            _host);
    d_tests_tls_switch_note(d_tests_tls_the_switch.sni,
                            sizeof(d_tests_tls_the_switch.sni),
                            _sni);

    // thrown, the switch answers
    if (d_tests_tls_the_switch.thrown == D_TESTS_TLS_SWITCHED_CREATE)
    {
        return d_tests_tls_the_switch.answer;
    }

    return d_tls_engine_openssl()->session_create(_context,
                                                  _host,
                                                  _sni,
                                                  _out_session);
}

/*
d_tests_tls_switched_feed
  Answers in the engine's place, the ciphertext untaken, or passes the call
on.
*/
static enum dj_tls_error
d_tests_tls_switched_feed(
    void*       _session,
    const void* _data,
    size_t      _size
)
{
    // thrown, the switch answers
    if (d_tests_tls_the_switch.thrown == D_TESTS_TLS_SWITCHED_FEED)
    {
        return d_tests_tls_the_switch.answer;
    }

    return d_tls_engine_openssl()->feed(_session,
                                        _data,
                                        _size);
}

/*
d_tests_tls_switched_drain
  Answers in the engine's place, moving nothing - with OK, it claims a byte
more than `_capacity` - or passes the call on.
*/
static enum dj_tls_error
d_tests_tls_switched_drain(
    void*   _session,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_size
)
{
    // thrown, the switch answers
    if (d_tests_tls_the_switch.thrown == D_TESTS_TLS_SWITCHED_DRAIN)
    {
        *_out_size = (d_tests_tls_the_switch.answer == D_TLS_OK)
                         ? (_capacity + 1u)
                         : 0u;

        return d_tests_tls_the_switch.answer;
    }

    return d_tls_engine_openssl()->drain(_session,
                                         _buffer,
                                         _capacity,
                                         _out_size);
}

/*
d_tests_tls_switched_close
  Each context's destroy accepts one never made.
*/
static
void d_tests_tls_switched_close(
    struct dj_tests_tls_switched_set* _set
)
{
    d_tls_context_destroy(&_set->checking);
    d_tls_context_destroy(&_set->trusting);
    d_tls_context_destroy(&_set->serving);

    return;
}

/*
d_tests_tls_switched_open
  The OpenSSL-family engine with session_create, feed and drain switched,
then the three contexts over it, configured as the world's are; false,
leaving none, on any failure.
*/
static
bool d_tests_tls_switched_open(
    struct dj_tests_tls_switched_set* _set,
    const struct dj_tests_tls_world*  _world
)
{
    struct dj_tls_config config = { .verify_hostname = true };
    bool                made   = true;

    memset(_set,
           0,
           sizeof(*_set));
    _set->engine                = *d_tls_engine_openssl();
    _set->engine.session_create = d_tests_tls_switched_create;
    _set->engine.feed           = d_tests_tls_switched_feed;
    _set->engine.drain          = d_tests_tls_switched_drain;

    d_tests_tls_client_config(&config);
    made = (d_tls_context_init(&_set->checking,
                               &_set->engine,
                               &config) == D_TLS_OK);
    config.verify_hostname = false;
    made = (d_tls_context_init(&_set->trusting,
                               &_set->engine,
                               &config) == D_TLS_OK) && made;
    d_tests_tls_server_config(&config,
                              _world->pki.server_cert,
                              _world->pki.server_key);
    made = (d_tls_context_init(&_set->serving,
                               &_set->engine,
                               &config) == D_TLS_OK) && made;

    // part of a set is no set
    if (!made)
    {
        d_tests_tls_switched_close(_set);
    }

    return made;
}

/*
d_tests_tls_names_handed
  Whether a session over `_context` naming `_host` is made, its engine
handed `_name` as the identity to check and `_sni` as the name to send.
*/
static
bool d_tests_tls_names_handed(
    const struct dj_tls_context* _context,
    const char*                  _host,
    const char*                  _name,
    const char*                  _sni
)
{
    struct dj_net_pipe        pipe    = { .rings[0].size = 0u };
    unsigned char            a[64]   = { 0 };
    unsigned char            b[64]   = { 0 };
    struct dj_tls_session     session = { .owns_transport = false };
    const struct dj_pack_text host    = { _host,
                                         strlen(_host) };

    (void)d_net_pipe_init(&pipe,
                          a,
                          sizeof(a),
                          b,
                          sizeof(b));

    const bool handed =
        ( (d_tls_session_init(&session,
                              _context,
                              d_net_pipe_connection(&pipe,
                                                    D_NET_PIPE_A),
                              host) == D_TLS_OK) &&
          (strcmp(d_tests_tls_the_switch.host,
                  _name) == 0)                   &&
          (strcmp(d_tests_tls_the_switch.sni,
                  _sni) == 0) );

    d_tls_session_destroy(&session);

    return handed;
}

/*
d_tests_tls_engine_names
  Tests the following:
  - a client that checks names hands its engine the host to check and, a
    name, to send as SNI - a name of one letter as much as any
  - an address is checked but never sent
  - a trailing dot reaches the engine stripped
  - a client that checks no names still sends one
  - a server hands over neither
*/
bool d_tests_tls_engine_names(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_tls_world* const world = d_tests_tls_world();
    struct dj_tests_tls_switched_set set   = { .engine.name = NULL };

    const bool opened = ( (world) &&
                          (d_tests_tls_switched_open(&set,
                                                     world)) );
    const bool handed =
        ( (opened)                                &&
          (d_tests_tls_names_handed(&set.checking,
                                    "localhost",
                                    "localhost",
                                    "localhost")) &&
          (d_tests_tls_names_handed(&set.checking,
                                    "h",
                                    "h",
                                    "h"))         &&
          (d_tests_tls_names_handed(&set.checking,
                                    "127.0.0.1",
                                    "127.0.0.1",
                                    "(null)"))    &&
          (d_tests_tls_names_handed(&set.checking,
                                    "localhost.",
                                    "localhost",
                                    "localhost")) &&
          (d_tests_tls_names_handed(&set.trusting,
                                    "localhost",
                                    "(null)",
                                    "localhost")) &&
          (d_tests_tls_names_handed(&set.serving,
                                    "",
                                    "(null)",
                                    "(null)")) );

    d_tests_tls_switched_close(&set);

    return D_TESTS_NET_ASSERT(handed,
                              "engine names",
                              "the identity to check, and SNI, as the host "
                              "has them",
                              _suite);
}

/*
d_tests_tls_create_refused
  Whether a session over `_context`, its engine's session_create answering
`_answer`, is refused with `_expected` and left an empty record and a
closed connection.
*/
static bool
d_tests_tls_create_refused(
    const struct dj_tls_context* _context,
    enum dj_tls_error            _answer,
    enum dj_tls_error            _expected
)
{
    struct dj_net_pipe        pipe    = { .rings[0].size = 0u };
    unsigned char            a[64]   = { 0 };
    unsigned char            b[64]   = { 0 };
    struct dj_tls_session     session = { .owns_transport = false };
    const struct dj_pack_text host    = { "localhost", 9u };

    (void)d_net_pipe_init(&pipe,
                          a,
                          sizeof(a),
                          b,
                          sizeof(b));
    d_tests_tls_switch_throw(D_TESTS_TLS_SWITCHED_CREATE,
                             _answer);

    const enum dj_tls_error status =
        d_tls_session_init(&session,
                           _context,
                           d_net_pipe_connection(&pipe,
                                                 D_NET_PIPE_A),
                           host);

    d_tests_tls_switch_throw(D_TESTS_TLS_SWITCHED_NONE,
                             D_TLS_OK);

    const bool refused = ( (status == _expected)     &&
                           (session.context == NULL) &&
                           (session.host[0] == '\0') &&
                           (!d_net_connection_is_open(&session.base)) );

    d_tls_session_destroy(&session);

    return refused;
}

/*
d_tests_tls_engine_creates
  Tests the following:
  - an engine whose session_create answers WANT_READ or CLOSED, neither of
    which it may, has init fail with BACKEND
  - one answering OUT_OF_MEMORY, which it may, is heard as it is
  - each refused session is an empty record, its host forgotten, and a
    closed connection
*/
bool d_tests_tls_engine_creates(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_tls_world* const world = d_tests_tls_world();
    struct dj_tests_tls_switched_set set   = { .engine.name = NULL };

    const bool opened  = ( (world) &&
                           (d_tests_tls_switched_open(&set,
                                                      world)) );
    const bool refused =
        ( (opened)                                          &&
          (d_tests_tls_create_refused(&set.checking,
                                      D_TLS_ERROR_WANT_READ,
                                      D_TLS_ERROR_BACKEND)) &&
          (d_tests_tls_create_refused(&set.checking,
                                      D_TLS_ERROR_CLOSED,
                                      D_TLS_ERROR_BACKEND)) &&
          (d_tests_tls_create_refused(&set.checking,
                                      D_TLS_ERROR_OUT_OF_MEMORY,
                                      D_TLS_ERROR_OUT_OF_MEMORY)) );

    d_tests_tls_switched_close(&set);

    return D_TESTS_NET_ASSERT(refused,
                              "engine contract",
                              "session_create's forbidden answers are "
                              "BACKEND",
                              _suite);
}

/*
d_tests_tls_fed
  Whether a server over `_server`, its engine's feed answering `_answer` to
the hello of a client over `_client`, fails its handshake with `_expected`.
*/
static bool
d_tests_tls_fed(
    const struct dj_tls_context* _client,
    const struct dj_tls_context* _server,
    enum dj_tls_error            _answer,
    enum dj_tls_error            _expected
)
{
    struct dj_tests_tls_pair pair = { .pipe.rings[0].size = 0u };

    const bool hello =
        ( (d_tests_tls_pair_open(&pair,
                                 _client,
                                 _server,
                                 "localhost")) &&
          (d_tls_session_handshake(&pair.client) ==
           D_TLS_ERROR_WOULD_BLOCK) );

    d_tests_tls_switch_throw(D_TESTS_TLS_SWITCHED_FEED,
                             _answer);

    const bool failed =
        ( (hello)                                              &&
          (d_tls_session_handshake(&pair.server) == _expected) &&
          (d_tls_session_failure(&pair.server) == _expected) );

    d_tests_tls_switch_throw(D_TESTS_TLS_SWITCHED_NONE,
                             D_TLS_OK);
    d_tests_tls_pair_close(&pair);

    return failed;
}

/*
d_tests_tls_drained
  Whether a client over `_client`, its engine's drain answering `_answer`
when the hello should go out, fails its handshake with BACKEND.
*/
static bool
d_tests_tls_drained(
    const struct dj_tls_context* _client,
    const struct dj_tls_context* _server,
    enum dj_tls_error            _answer
)
{
    struct dj_tests_tls_pair pair = { .pipe.rings[0].size = 0u };

    const bool opened = d_tests_tls_pair_open(&pair,
                                              _client,
                                              _server,
                                              "localhost");

    d_tests_tls_switch_throw(D_TESTS_TLS_SWITCHED_DRAIN,
                             _answer);

    const bool failed =
        ( (opened)                                                       &&
          (d_tls_session_handshake(&pair.client) == D_TLS_ERROR_BACKEND) &&
          (d_tls_session_failure(&pair.client) == D_TLS_ERROR_BACKEND) );

    d_tests_tls_switch_throw(D_TESTS_TLS_SWITCHED_NONE,
                             D_TLS_OK);
    d_tests_tls_pair_close(&pair);

    return failed;
}

/*
d_tests_tls_engine_steps
  Tests the following:
  - a server whose engine's feed answers WANT_READ or CLOSED, neither of
    which it may, fails its handshake with BACKEND; one answering
    OUT_OF_MEMORY, which it may, fails with that
  - a client whose engine's drain fails, or claims more than its room,
    fails its handshake with BACKEND
*/
bool d_tests_tls_engine_steps(
    struct dj_test_suite* _suite
)
{
    struct dj_tests_tls_world* const world  = d_tests_tls_world();
    struct dj_tests_tls_switched_set set    = { .engine.name = NULL };
    bool                            result = true;

    const bool opened  = ( (world) &&
                           (d_tests_tls_switched_open(&set,
                                                      world)) );
    const bool fed     =
        ( (opened)                               &&
          (d_tests_tls_fed(&world->client,
                           &set.serving,
                           D_TLS_ERROR_WANT_READ,
                           D_TLS_ERROR_BACKEND)) &&
          (d_tests_tls_fed(&world->client,
                           &set.serving,
                           D_TLS_ERROR_CLOSED,
                           D_TLS_ERROR_BACKEND)) &&
          (d_tests_tls_fed(&world->client,
                           &set.serving,
                           D_TLS_ERROR_OUT_OF_MEMORY,
                           D_TLS_ERROR_OUT_OF_MEMORY)) );
    const bool drained =
        ( (opened)                                   &&
          (d_tests_tls_drained(&set.checking,
                               &world->server,
                               D_TLS_ERROR_BACKEND)) &&
          (d_tests_tls_drained(&set.checking,
                               &world->server,
                               D_TLS_OK)) );

    d_tests_tls_switched_close(&set);

    result = D_TESTS_NET_ASSERT(fed,
                                "engine contract",
                                "feed's forbidden answers are BACKEND",
                                _suite) && result;

    return D_TESTS_NET_ASSERT(drained,
                              "engine contract",
                              "a failed or overfull drain is BACKEND",
                              _suite) && result;
}
