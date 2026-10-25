/*******************************************************************************
* djinterp [net]                                           ssl_tests_sa_engine.c
*
* Tests of what the SSL kernel hands an engine, and of how it holds one to
* its contract.
*   The engine is the OpenSSL-family one with three of its operations behind
* a switch: session_create, which notes the names it is handed, feed, and
* drain. Thrown, the switch has its operation answer in the engine's place:
* WANT_READ or CLOSED where the contract allows neither, a failure, or a
* count larger than the room it was given. The kernel must make BACKEND of
* each, so no engine can keep a session waiting or pass for its transport.
*
*
* path:      /tests/djinterp/net/ssl/ssl_tests_sa_engine.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./ssl_tests_sa.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memset, strcmp, strlen


// d_tests_ssl_switched
//   enum: the switched engine's operations, any one of which the switch can
// throw.
enum d_tests_ssl_switched
{
    D_TESTS_SSL_SWITCHED_NONE = 0,
    D_TESTS_SSL_SWITCHED_CREATE,
    D_TESTS_SSL_SWITCHED_FEED,
    D_TESTS_SSL_SWITCHED_DRAIN
};

// d_tests_ssl_switch
//   struct: the operation thrown and what it answers instead -- for drain,
// OK is the lie of a byte more than its room -- and the names the last
// session_create was handed, "(null)" for none.
struct d_tests_ssl_switch
{
    enum d_tests_ssl_switched thrown;
    enum d_ssl_error          answer;
    char                      host[D_SSL_HOSTNAME_MAX + 2];
    char                      sni[D_SSL_HOSTNAME_MAX + 2];
};

// d_tests_ssl_switched_set
//   struct: the switched engine, and a context over it for a client that
// checks names, a client that does not, and a server. It must not move once
// opened: the contexts point at its engine.
struct d_tests_ssl_switched_set
{
    struct d_ssl_engine  engine;
    struct d_ssl_context checking;
    struct d_ssl_context trusting;
    struct d_ssl_context serving;
};

// d_tests_ssl_the_switch
//   variable: the run's switch, thrown by a test and reset when it is done.
static struct d_tests_ssl_switch d_tests_ssl_the_switch =
{
    D_TESTS_SSL_SWITCHED_NONE,
    D_SSL_OK,
    { '\0' },
    { '\0' }
};

/*
d_tests_ssl_switch_throw
  Throws the switch: `_operation` answers `_answer` until the switch is
thrown again. NONE resets it.
*/
static void
d_tests_ssl_switch_throw(
    enum d_tests_ssl_switched _operation,
    enum d_ssl_error          _answer
)
{
    d_tests_ssl_the_switch.thrown = _operation;
    d_tests_ssl_the_switch.answer = _answer;

    return;
}

/*
d_tests_ssl_switch_note
  Copies a name session_create was handed, or "(null)" for none.
*/
static void
d_tests_ssl_switch_note(
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
d_tests_ssl_switched_create
  Notes the names, then answers in the engine's place, making no session,
or passes the call on.
*/
static enum d_ssl_error
d_tests_ssl_switched_create(
    void*       _context,
    const char* _host,
    const char* _sni,
    void**      _out_session
)
{
    d_tests_ssl_switch_note(d_tests_ssl_the_switch.host,
                            sizeof(d_tests_ssl_the_switch.host),
                            _host);
    d_tests_ssl_switch_note(d_tests_ssl_the_switch.sni,
                            sizeof(d_tests_ssl_the_switch.sni),
                            _sni);

    // thrown, the switch answers
    if (d_tests_ssl_the_switch.thrown == D_TESTS_SSL_SWITCHED_CREATE)
    {
        return d_tests_ssl_the_switch.answer;
    }

    return d_ssl_engine_openssl()->session_create(_context,
                                                  _host,
                                                  _sni,
                                                  _out_session);
}

/*
d_tests_ssl_switched_feed
  Answers in the engine's place, the ciphertext untaken, or passes the call
on.
*/
static enum d_ssl_error
d_tests_ssl_switched_feed(
    void*       _session,
    const void* _data,
    size_t      _size
)
{
    // thrown, the switch answers
    if (d_tests_ssl_the_switch.thrown == D_TESTS_SSL_SWITCHED_FEED)
    {
        return d_tests_ssl_the_switch.answer;
    }

    return d_ssl_engine_openssl()->feed(_session,
                                        _data,
                                        _size);
}

/*
d_tests_ssl_switched_drain
  Answers in the engine's place, moving nothing -- with OK, it claims a byte
more than `_capacity` -- or passes the call on.
*/
static enum d_ssl_error
d_tests_ssl_switched_drain(
    void*   _session,
    void*   _buffer,
    size_t  _capacity,
    size_t* _out_size
)
{
    // thrown, the switch answers
    if (d_tests_ssl_the_switch.thrown == D_TESTS_SSL_SWITCHED_DRAIN)
    {
        *_out_size = (d_tests_ssl_the_switch.answer == D_SSL_OK)
                         ? (_capacity + 1u)
                         : 0u;

        return d_tests_ssl_the_switch.answer;
    }

    return d_ssl_engine_openssl()->drain(_session,
                                         _buffer,
                                         _capacity,
                                         _out_size);
}

/*
d_tests_ssl_switched_close
  Each context's destroy accepts one never made.
*/
static void
d_tests_ssl_switched_close(
    struct d_tests_ssl_switched_set* _set
)
{
    d_ssl_context_destroy(&_set->checking);
    d_ssl_context_destroy(&_set->trusting);
    d_ssl_context_destroy(&_set->serving);

    return;
}

/*
d_tests_ssl_switched_open
  The OpenSSL-family engine with session_create, feed and drain switched,
then the three contexts over it, configured as the world's are; false,
leaving none, on any failure.
*/
static bool
d_tests_ssl_switched_open(
    struct d_tests_ssl_switched_set* _set,
    const struct d_tests_ssl_world*  _world
)
{
    struct d_ssl_config config = { .verify_hostname = true };
    bool                made   = true;

    memset(_set,
           0,
           sizeof(*_set));
    _set->engine                = *d_ssl_engine_openssl();
    _set->engine.session_create = d_tests_ssl_switched_create;
    _set->engine.feed           = d_tests_ssl_switched_feed;
    _set->engine.drain          = d_tests_ssl_switched_drain;

    d_tests_ssl_client_config(&config);
    made = (d_ssl_context_init(&_set->checking,
                               &_set->engine,
                               &config) == D_SSL_OK);
    config.verify_hostname = false;
    made = (d_ssl_context_init(&_set->trusting,
                               &_set->engine,
                               &config) == D_SSL_OK) && made;
    d_tests_ssl_server_config(&config,
                              _world->pki.server_cert,
                              _world->pki.server_key);
    made = (d_ssl_context_init(&_set->serving,
                               &_set->engine,
                               &config) == D_SSL_OK) && made;

    // part of a set is no set
    if (!made)
    {
        d_tests_ssl_switched_close(_set);
    }

    return made;
}

/*
d_tests_ssl_names_handed
  Whether a session over `_context` naming `_host` is made, its engine
handed `_name` as the identity to check and `_sni` as the name to send.
*/
static bool
d_tests_ssl_names_handed(
    const struct d_ssl_context* _context,
    const char*                 _host,
    const char*                 _name,
    const char*                 _sni
)
{
    struct d_net_pipe        pipe    = { .rings[0].size = 0u };
    unsigned char            a[64]   = { 0 };
    unsigned char            b[64]   = { 0 };
    struct d_ssl_session     session = { .owns_transport = false };
    const struct d_pack_text host    = { _host,
                                         strlen(_host) };

    (void)d_net_pipe_init(&pipe,
                          a,
                          sizeof(a),
                          b,
                          sizeof(b));

    const bool handed =
        ( (d_ssl_session_init(&session,
                              _context,
                              d_net_pipe_connection(&pipe,
                                                    D_NET_PIPE_A),
                              host) == D_SSL_OK) &&
          (strcmp(d_tests_ssl_the_switch.host,
                  _name) == 0)                   &&
          (strcmp(d_tests_ssl_the_switch.sni,
                  _sni) == 0) );

    d_ssl_session_destroy(&session);

    return handed;
}

/*
d_tests_sa_ssl_engine_names
  Tests the following:
  - a client that checks names hands its engine the host to check and, a
    name, to send as SNI -- a name of one letter as much as any
  - an address is checked but never sent
  - a trailing dot reaches the engine stripped
  - a client that checks no names still sends one
  - a server hands over neither
*/
bool
d_tests_sa_ssl_engine_names(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world = d_tests_ssl_world();
    struct d_tests_ssl_switched_set set   = { .engine.name = NULL };

    const bool opened = ( (world) &&
                          (d_tests_ssl_switched_open(&set,
                                                     world)) );
    const bool handed =
        ( (opened)                                &&
          (d_tests_ssl_names_handed(&set.checking,
                                    "localhost",
                                    "localhost",
                                    "localhost")) &&
          (d_tests_ssl_names_handed(&set.checking,
                                    "h",
                                    "h",
                                    "h"))         &&
          (d_tests_ssl_names_handed(&set.checking,
                                    "127.0.0.1",
                                    "127.0.0.1",
                                    "(null)"))    &&
          (d_tests_ssl_names_handed(&set.checking,
                                    "localhost.",
                                    "localhost",
                                    "localhost")) &&
          (d_tests_ssl_names_handed(&set.trusting,
                                    "localhost",
                                    "(null)",
                                    "localhost")) &&
          (d_tests_ssl_names_handed(&set.serving,
                                    "",
                                    "(null)",
                                    "(null)")) );

    d_tests_ssl_switched_close(&set);

    return d_tests_net_assert(handed,
                              "engine names",
                              "the identity to check, and SNI, as the host "
                              "has them",
                              _counter);
}

/*
d_tests_ssl_create_refused
  Whether a session over `_context`, its engine's session_create answering
`_answer`, is refused with `_expected` and left an empty record and a
closed connection.
*/
static bool
d_tests_ssl_create_refused(
    const struct d_ssl_context* _context,
    enum d_ssl_error            _answer,
    enum d_ssl_error            _expected
)
{
    struct d_net_pipe        pipe    = { .rings[0].size = 0u };
    unsigned char            a[64]   = { 0 };
    unsigned char            b[64]   = { 0 };
    struct d_ssl_session     session = { .owns_transport = false };
    const struct d_pack_text host    = { "localhost", 9u };

    (void)d_net_pipe_init(&pipe,
                          a,
                          sizeof(a),
                          b,
                          sizeof(b));
    d_tests_ssl_switch_throw(D_TESTS_SSL_SWITCHED_CREATE,
                             _answer);

    const enum d_ssl_error status =
        d_ssl_session_init(&session,
                           _context,
                           d_net_pipe_connection(&pipe,
                                                 D_NET_PIPE_A),
                           host);

    d_tests_ssl_switch_throw(D_TESTS_SSL_SWITCHED_NONE,
                             D_SSL_OK);

    const bool refused = ( (status == _expected)     &&
                           (session.context == NULL) &&
                           (session.host[0] == '\0') &&
                           (!d_net_connection_is_open(&session.base)) );

    d_ssl_session_destroy(&session);

    return refused;
}

/*
d_tests_sa_ssl_engine_creates
  Tests the following:
  - an engine whose session_create answers WANT_READ or CLOSED, neither of
    which it may, has init fail with BACKEND
  - one answering OUT_OF_MEMORY, which it may, is heard as it is
  - each refused session is an empty record, its host forgotten, and a
    closed connection
*/
bool
d_tests_sa_ssl_engine_creates(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world = d_tests_ssl_world();
    struct d_tests_ssl_switched_set set   = { .engine.name = NULL };

    const bool opened  = ( (world) &&
                           (d_tests_ssl_switched_open(&set,
                                                      world)) );
    const bool refused =
        ( (opened)                                          &&
          (d_tests_ssl_create_refused(&set.checking,
                                      D_SSL_ERROR_WANT_READ,
                                      D_SSL_ERROR_BACKEND)) &&
          (d_tests_ssl_create_refused(&set.checking,
                                      D_SSL_ERROR_CLOSED,
                                      D_SSL_ERROR_BACKEND)) &&
          (d_tests_ssl_create_refused(&set.checking,
                                      D_SSL_ERROR_OUT_OF_MEMORY,
                                      D_SSL_ERROR_OUT_OF_MEMORY)) );

    d_tests_ssl_switched_close(&set);

    return d_tests_net_assert(refused,
                              "engine contract",
                              "session_create's forbidden answers are "
                              "BACKEND",
                              _counter);
}

/*
d_tests_ssl_fed
  Whether a server over `_server`, its engine's feed answering `_answer` to
the hello of a client over `_client`, fails its handshake with `_expected`.
*/
static bool
d_tests_ssl_fed(
    const struct d_ssl_context* _client,
    const struct d_ssl_context* _server,
    enum d_ssl_error            _answer,
    enum d_ssl_error            _expected
)
{
    struct d_tests_ssl_pair pair = { .pipe.rings[0].size = 0u };

    const bool hello =
        ( (d_tests_ssl_pair_open(&pair,
                                 _client,
                                 _server,
                                 "localhost")) &&
          (d_ssl_session_handshake(&pair.client) ==
           D_SSL_ERROR_WOULD_BLOCK) );

    d_tests_ssl_switch_throw(D_TESTS_SSL_SWITCHED_FEED,
                             _answer);

    const bool failed =
        ( (hello)                                              &&
          (d_ssl_session_handshake(&pair.server) == _expected) &&
          (d_ssl_session_failure(&pair.server) == _expected) );

    d_tests_ssl_switch_throw(D_TESTS_SSL_SWITCHED_NONE,
                             D_SSL_OK);
    d_tests_ssl_pair_close(&pair);

    return failed;
}

/*
d_tests_ssl_drained
  Whether a client over `_client`, its engine's drain answering `_answer`
when the hello should go out, fails its handshake with BACKEND.
*/
static bool
d_tests_ssl_drained(
    const struct d_ssl_context* _client,
    const struct d_ssl_context* _server,
    enum d_ssl_error            _answer
)
{
    struct d_tests_ssl_pair pair = { .pipe.rings[0].size = 0u };

    const bool opened = d_tests_ssl_pair_open(&pair,
                                              _client,
                                              _server,
                                              "localhost");

    d_tests_ssl_switch_throw(D_TESTS_SSL_SWITCHED_DRAIN,
                             _answer);

    const bool failed =
        ( (opened)                                                       &&
          (d_ssl_session_handshake(&pair.client) == D_SSL_ERROR_BACKEND) &&
          (d_ssl_session_failure(&pair.client) == D_SSL_ERROR_BACKEND) );

    d_tests_ssl_switch_throw(D_TESTS_SSL_SWITCHED_NONE,
                             D_SSL_OK);
    d_tests_ssl_pair_close(&pair);

    return failed;
}

/*
d_tests_sa_ssl_engine_steps
  Tests the following:
  - a server whose engine's feed answers WANT_READ or CLOSED, neither of
    which it may, fails its handshake with BACKEND; one answering
    OUT_OF_MEMORY, which it may, fails with that
  - a client whose engine's drain fails, or claims more than its room,
    fails its handshake with BACKEND
*/
bool
d_tests_sa_ssl_engine_steps(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssl_world* const world  = d_tests_ssl_world();
    struct d_tests_ssl_switched_set set    = { .engine.name = NULL };
    bool                            result = true;

    const bool opened  = ( (world) &&
                           (d_tests_ssl_switched_open(&set,
                                                      world)) );
    const bool fed     =
        ( (opened)                               &&
          (d_tests_ssl_fed(&world->client,
                           &set.serving,
                           D_SSL_ERROR_WANT_READ,
                           D_SSL_ERROR_BACKEND)) &&
          (d_tests_ssl_fed(&world->client,
                           &set.serving,
                           D_SSL_ERROR_CLOSED,
                           D_SSL_ERROR_BACKEND)) &&
          (d_tests_ssl_fed(&world->client,
                           &set.serving,
                           D_SSL_ERROR_OUT_OF_MEMORY,
                           D_SSL_ERROR_OUT_OF_MEMORY)) );
    const bool drained =
        ( (opened)                                   &&
          (d_tests_ssl_drained(&set.checking,
                               &world->server,
                               D_SSL_ERROR_BACKEND)) &&
          (d_tests_ssl_drained(&set.checking,
                               &world->server,
                               D_SSL_OK)) );

    d_tests_ssl_switched_close(&set);

    result = d_tests_net_assert(fed,
                                "engine contract",
                                "feed's forbidden answers are BACKEND",
                                _counter) && result;

    return d_tests_net_assert(drained,
                              "engine contract",
                              "a failed or overfull drain is BACKEND",
                              _counter) && result;
}
