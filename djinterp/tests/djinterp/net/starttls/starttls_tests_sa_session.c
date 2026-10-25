/*******************************************************************************
* djinterp [net]                                     starttls_tests_sa_session.c
*
* Standalone tests of a STARTTLS session before any upgrade: its defaults,
* its initialization, lifecycle and queries, the names of its values, and
* its line and byte I/O, each against a scripted peer.
*
*
* path:      /tests/djinterp/net/starttls/starttls_tests_sa_session.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./starttls_tests_sa.h"  // the suite
// std
#include <string.h>  // memcmp, memcpy, memset, strcmp, strlen
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // SIZE_MAX


/*
d_tests_starttls_repeat
  Fills `_out` with `_count` copies of `_byte`, then `_tail`, and terminates
it; `_out` must hold them.
*/
static void
d_tests_starttls_repeat(
    char*       _out,
    char        _byte,
    size_t      _count,
    const char* _tail
)
{
    memset(_out,
           _byte,
           _count);
    memcpy(_out + _count,
           _tail,
           strlen(_tail) + 1);

    return;
}

/*
d_tests_sa_starttls_defaults
  Tests the following:
  - d_starttls_options_default takes every default from cfg_starttls.h,
    leaves the context, the server name, and the callback unset, borrows
    the connection, and keeps the role it is given
*/
bool
d_tests_sa_starttls_defaults(
    struct d_test_counter* _counter
)
{
    const char* const               TEST   = "defaults";
    const struct d_starttls_options client =
        d_starttls_options_default(D_STARTTLS_ROLE_CLIENT);
    const struct d_starttls_options server =
        d_starttls_options_default(D_STARTTLS_ROLE_SERVER);

    const bool options =
        ( (client.role == D_STARTTLS_ROLE_CLIENT)                        &&
          (server.role == D_STARTTLS_ROLE_SERVER)                        &&
          ((int)client.policy == D_INTERNAL_STARTTLS_POLICY)             &&
          (client.buffer_bytes == D_INTERNAL_STARTTLS_BUFFER_BYTES)      &&
          (client.reject_leftover ==
           (D_INTERNAL_STARTTLS_REJECT_LEFTOVER == 1))                   &&
          (client.strict_crlf == (D_INTERNAL_STARTTLS_STRICT_CRLF == 1)) &&
          (!client.owns_connection)                                      &&
          (!client.tls)                                                  &&
          (!client.server_name)                                          &&
          (!client.on_upgrade)                                           &&
          (!client.on_upgrade_context)                                   &&
          (server.policy == client.policy)                               &&
          (server.buffer_bytes == client.buffer_bytes)                   &&
          (!server.owns_connection) );

    return d_tests_net_assert(options,
                              TEST,
                              "options from cfg_starttls.h; borrowed",
                              _counter);
}

/*
d_tests_starttls_refused_with
  Whether d_starttls_init answers `_options` and `_connection` with
`_expected`, leaving the session's every byte as it was.
*/
static bool
d_tests_starttls_refused_with(
    const struct d_starttls_options* _options,
    struct d_net_connection*         _connection,
    enum d_starttls_error            _expected
)
{
    struct d_starttls_session session = { .state = D_STARTTLS_STATE_CLOSED };
    unsigned char             before[sizeof(session)] = { 0 };

    memset(&session,
           0xA5,
           sizeof(session));
    memcpy(before,
           &session,
           sizeof(session));

    return ( (d_starttls_init(&session,
                              _options,
                              _connection) == _expected) &&
             (memcmp(before,
                     &session,
                     sizeof(session)) == 0) );
}

/*
d_tests_starttls_accepted
  Whether d_starttls_init accepts `_options`; the session is destroyed again.
*/
static bool
d_tests_starttls_accepted(
    const struct d_starttls_options* _options,
    struct d_net_connection*         _connection
)
{
    struct d_starttls_session session = { .state = D_STARTTLS_STATE_CLOSED };

    const enum d_starttls_error status = d_starttls_init(&session,
                                                         _options,
                                                         _connection);

    if (status == D_STARTTLS_OK)
    {
        d_starttls_destroy(&session);
    }

    return (status == D_STARTTLS_OK);
}

/*
d_tests_starttls_answers
  Whether d_starttls_init answers each of `_count` options with its expected
status: D_STARTTLS_OK as a session accepted and destroyed again, anything
else as a refusal that leaves the session untouched.
*/
static bool
d_tests_starttls_answers(
    const struct d_starttls_options* _options,
    const enum d_starttls_error*     _expected,
    size_t                           _count,
    struct d_net_connection*         _connection
)
{
    bool answered = true;

    // each option against its expected answer, until one differs
    for (size_t i = 0; (answered) && (i < _count); ++i)
    {
        if (_expected[i] == D_STARTTLS_OK)
        {
            answered = d_tests_starttls_accepted(&_options[i],
                                                 _connection);
        }
        else
        {
            answered = d_tests_starttls_refused_with(&_options[i],
                                                     _connection,
                                                     _expected[i]);
        }
    }

    return answered;
}

/*
d_tests_sa_starttls_init_arguments
  Tests the following:
  - a NULL session, options, or connection is refused
  - a connection without operations, or without read or write, is refused,
    and the session is left untouched
*/
bool
d_tests_sa_starttls_init_arguments(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "init arguments";
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    bool                            result  = true;

    if (!d_tests_net_assert(d_tests_starttls_fixture_open(
                                &fixture,
                                D_STARTTLS_ROLE_CLIENT,
                                D_STARTTLS_POLICY_REQUIRED),
                            TEST,
                            "a fixture",
                            _counter))
    {
        return false;
    }

    struct d_net_connection_vtable deaf     = *fixture.connection->vtable;
    struct d_net_connection_vtable mute     = *fixture.connection->vtable;
    struct d_net_connection        nothing  = { NULL };
    struct d_net_connection        no_read  = { &deaf };
    struct d_net_connection        no_write = { &mute };

    deaf.read  = NULL;
    mute.write = NULL;

    const bool nulls =
        ( (d_starttls_init(NULL,
                           &fixture.options,
                           fixture.connection) ==
           D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
          (d_starttls_init(&session,
                           NULL,
                           fixture.connection) ==
           D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
          (d_starttls_init(&session,
                           &fixture.options,
                           NULL) == D_STARTTLS_ERROR_INVALID_ARGUMENT) );
    const bool streams =
        ( (d_tests_starttls_refused_with(&fixture.options,
                                         &nothing,
                                         D_STARTTLS_ERROR_INVALID_ARGUMENT)) &&
          (d_tests_starttls_refused_with(&fixture.options,
                                         &no_read,
                                         D_STARTTLS_ERROR_INVALID_ARGUMENT)) &&
          (d_tests_starttls_refused_with(&fixture.options,
                                         &no_write,
                                         D_STARTTLS_ERROR_INVALID_ARGUMENT)) );

    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(nulls,
                                TEST,
                                "a NULL session, options, or connection",
                                _counter) && result;
    result = d_tests_net_assert(streams,
                                TEST,
                                "a connection without operations, read, write",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_init_values
  Tests the following:
  - zero-initialized options, a role or policy out of range, and a buffer
    below D_STARTTLS_BUFFER_MIN are refused; the minimum itself is accepted
  - a policy that allows TLS needs a context; DISABLED needs none
*/
bool
d_tests_sa_starttls_init_values(
    struct d_test_counter* _counter
)
{
    const char* const               TEST      = "init values";
    const enum d_starttls_error     RANGES[5] =
    {
        D_STARTTLS_ERROR_INVALID_ARGUMENT,
        D_STARTTLS_ERROR_INVALID_ARGUMENT,
        D_STARTTLS_ERROR_INVALID_ARGUMENT,
        D_STARTTLS_ERROR_INVALID_ARGUMENT,
        D_STARTTLS_OK
    };
    const enum d_starttls_error     NEEDS[3]  =
    {
        D_STARTTLS_ERROR_NO_BACKEND,
        D_STARTTLS_ERROR_NO_BACKEND,
        D_STARTTLS_OK
    };
    struct d_starttls_options       o[8]      =
    {
        { .role = D_STARTTLS_ROLE_CLIENT }
    };
    struct d_tests_starttls_fixture fixture   = { .peer = NULL };
    bool                            result    = true;

    if (!d_tests_net_assert(d_tests_starttls_fixture_open(
                                &fixture,
                                D_STARTTLS_ROLE_CLIENT,
                                D_STARTTLS_POLICY_REQUIRED),
                            TEST,
                            "a fixture",
                            _counter))
    {
        return false;
    }

    // o[0] stays zeroed; the rest change one thing each
    for (size_t i = 1; i < (sizeof(o) / sizeof(o[0])); ++i)
    {
        o[i] = fixture.options;
    }

    o[1].role         = (enum d_starttls_role)2;
    o[2].policy       = (enum d_starttls_policy)4;
    o[3].buffer_bytes = D_STARTTLS_BUFFER_MIN - 1;
    o[4].buffer_bytes = D_STARTTLS_BUFFER_MIN;
    o[5].tls          = NULL;
    o[6].tls          = NULL;
    o[6].policy       = D_STARTTLS_POLICY_OPPORTUNISTIC;
    o[7].tls          = NULL;
    o[7].policy       = D_STARTTLS_POLICY_DISABLED;

    const bool ranges   = d_tests_starttls_answers(o,
                                                   RANGES,
                                                   5,
                                                   fixture.connection);
    const bool contexts = d_tests_starttls_answers(o + 5,
                                                   NEEDS,
                                                   3,
                                                   fixture.connection);

    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(ranges,
                                TEST,
                                "zeroed options; bad roles, policies, buffers",
                                _counter) && result;
    result = d_tests_net_assert(contexts,
                                TEST,
                                "TLS needs a context; DISABLED needs none",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_unusable
  Fills five options from `_fixture`'s: an empty context, a server's
context, a verifying client named by an empty string and by nothing, and a
client whose context does not verify, unnamed. `_server` and `_trusting`
are the contexts the second and the last use.
*/
static void
d_tests_starttls_unusable(
    struct d_starttls_options*             _options,
    const struct d_ssl_context*            _empty,
    const struct d_ssl_context*            _server,
    const struct d_ssl_context*            _trusting,
    const struct d_tests_starttls_fixture* _fixture
)
{
    // each changes one thing of the fixture's
    for (size_t i = 0; i < 5; ++i)
    {
        _options[i] = _fixture->options;
    }

    _options[0].tls         = _empty;
    _options[1].tls         = _server;
    _options[2].server_name = "";
    _options[3].server_name = NULL;
    _options[4].tls         = _trusting;
    _options[4].server_name = NULL;

    return;
}

/*
d_tests_sa_starttls_init_contexts
  Tests the following:
  - an empty context is no backend; a context for the other role is
    refused, and so is a client whose context checks names given no server,
    by an empty name or by none
  - a client whose context does not verify needs no name
*/
bool
d_tests_sa_starttls_init_contexts(
    struct d_test_counter* _counter
)
{
    const char* const               TEST     = "init contexts";
    const enum d_starttls_error     WANTS[5] =
    {
        D_STARTTLS_ERROR_NO_BACKEND,
        D_STARTTLS_ERROR_INVALID_ARGUMENT,
        D_STARTTLS_ERROR_INVALID_ARGUMENT,
        D_STARTTLS_ERROR_INVALID_ARGUMENT,
        D_STARTTLS_OK
    };
    struct d_tests_net_toy          host     = { .create_status = D_SSL_OK };
    struct d_tests_net_toy          plain    = { .create_status = D_SSL_OK };
    struct d_ssl_context            empty    = { .engine = NULL };
    struct d_ssl_context            server   = { .engine = NULL };
    struct d_ssl_context            trusting = { .engine = NULL };
    struct d_starttls_options       o[5]     =
    {
        { .role = D_STARTTLS_ROLE_CLIENT }
    };
    struct d_tests_starttls_fixture fixture  = { .peer = NULL };
    bool                            answered = false;

    if (!d_tests_net_assert( (d_tests_starttls_fixture_open(
                                  &fixture,
                                  D_STARTTLS_ROLE_CLIENT,
                                  D_STARTTLS_POLICY_REQUIRED)) &&
                             (d_tests_net_toy_context(
                                  &server,
                                  &host,
                                  D_SSL_ROLE_SERVER,
                                  false))                      &&
                             (d_tests_net_toy_context(
                                  &trusting,
                                  &plain,
                                  D_SSL_ROLE_CLIENT,
                                  false)),
                             TEST,
                             "a fixture and two contexts",
                             _counter))
    {
        d_ssl_context_destroy(&server);
        d_tests_starttls_fixture_close(&fixture);

        return false;
    }

    d_tests_starttls_unusable(o,
                              &empty,
                              &server,
                              &trusting,
                              &fixture);
    answered = d_tests_starttls_answers(o,
                                        WANTS,
                                        5,
                                        fixture.connection);

    d_ssl_context_destroy(&server);
    d_ssl_context_destroy(&trusting);
    d_tests_starttls_fixture_close(&fixture);

    return d_tests_net_assert(answered,
                              TEST,
                              "unusable contexts; a verifier without a name",
                              _counter);
}

/*
d_tests_sa_starttls_new_session
  Tests the following:
  - a new session is plaintext, not secure, at epoch 0, with nothing
    buffered, no detail, no cause, and no TLS session; under REQUIRED it is
    not yet satisfied
  - the server name is the session's own copy
*/
bool
d_tests_sa_starttls_new_session(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "new session";
    char                            name[]  = D_TESTS_NET_TOY_SERVER_NAME;
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    bool                            result  = true;

    if (!d_tests_net_assert(d_tests_starttls_fixture_open(
                                &fixture,
                                D_STARTTLS_ROLE_CLIENT,
                                D_STARTTLS_POLICY_REQUIRED),
                            TEST,
                            "a fixture",
                            _counter))
    {
        return false;
    }

    fixture.options.server_name = name;

    const bool opened = (d_starttls_init(&session,
                                         &fixture.options,
                                         fixture.connection) ==
                         D_STARTTLS_OK);

    // the name is copied, so the caller's string may change
    name[0] = 'X';

    const bool fresh = ( (opened)                                     &&
                         (d_starttls_get_state(&session) ==
                          D_STARTTLS_STATE_PLAINTEXT)                 &&
                         (!d_starttls_is_secure(&session))            &&
                         (!d_starttls_is_satisfied(&session))         &&
                         (d_starttls_epoch(&session) == 0)            &&
                         (d_starttls_buffered(&session) == 0)         &&
                         (d_starttls_detail(&session) == NULL)        &&
                         (d_starttls_net_error(&session) == D_NET_OK) &&
                         (d_starttls_tls_session(&session) == NULL) );
    const bool copied = ( (opened)                      &&
                          (session.server_name != name) &&
                          (strcmp(session.server_name,
                                  D_TESTS_NET_TOY_SERVER_NAME) == 0) );

    d_starttls_destroy(&session);
    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(fresh,
                                TEST,
                                "plaintext, epoch 0, unsatisfied, no TLS",
                                _counter) && result;
    result = d_tests_net_assert(copied,
                                TEST,
                                "the server name is the session's copy",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_all_refused
  Whether every call that needs an open session reports
D_STARTTLS_ERROR_STATE.
*/
static bool
d_tests_starttls_all_refused(
    struct d_starttls_session* _session
)
{
    struct d_tests_starttls_smtp smtp   = { .request_status = D_STARTTLS_OK };
    char                         byte   = 'x';
    size_t                       count  = 0;
    const char*                  line   = NULL;

    return ( (d_starttls_send(_session,
                              &byte,
                              1) == D_STARTTLS_ERROR_STATE)           &&
             (d_starttls_recv(_session,
                              &byte,
                              1,
                              &count) == D_STARTTLS_ERROR_STATE)      &&
             (d_starttls_recv_line(_session,
                                   &line,
                                   &count) == D_STARTTLS_ERROR_STATE) &&
             (d_starttls_send_line(_session,
                                   "NOOP") == D_STARTTLS_ERROR_STATE) &&
             (d_starttls_upgrade(_session) == D_STARTTLS_ERROR_STATE) &&
             (d_starttls_negotiate(_session,
                                   &d_tests_starttls_smtp_negotiator,
                                   &smtp,
                                   true) == D_STARTTLS_ERROR_STATE) );
}

/*
d_tests_sa_starttls_close
  Tests the following:
  - closing a plaintext session sends nothing, closes an owned connection
    once, is idempotent, and refuses every later I/O call
  - destroying after close destroys the connection, closing it again
    harmlessly, zeroes the session, and may repeat; NULL is accepted
*/
bool
d_tests_sa_starttls_close(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "close";
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    bool                            result  = true;

    if (!d_tests_net_assert( (d_tests_starttls_fixture_open(
                                  &fixture,
                                  D_STARTTLS_ROLE_CLIENT,
                                  D_STARTTLS_POLICY_REQUIRED)) &&
                             (d_starttls_init(&session,
                                              &fixture.options,
                                              fixture.connection) ==
                              D_STARTTLS_OK),
                             TEST,
                             "a session",
                             _counter))
    {
        d_tests_starttls_fixture_close(&fixture);

        return false;
    }

    const enum d_starttls_error closed = d_starttls_close(&session);
    const enum d_starttls_error again  = d_starttls_close(&session);
    const bool                  final  =
        ( (closed == D_STARTTLS_OK)                                   &&
          (again == D_STARTTLS_OK)                                    &&
          (fixture.peer->closes == 1)                                 &&
          (fixture.peer->sent == 0)                                   &&
          (d_starttls_get_state(&session) == D_STARTTLS_STATE_CLOSED) &&
          (!d_starttls_is_satisfied(&session))                        &&
          (d_tests_starttls_all_refused(&session)) );

    d_starttls_destroy(&session);
    d_starttls_destroy(&session);
    d_starttls_destroy(NULL);

    const bool released =
        ( (fixture.peer->closes == 2)   &&
          (fixture.peer->destroys == 1) &&
          (session.buffer == NULL)      &&
          (session.server_name == NULL) &&
          (d_starttls_get_state(&session) == D_STARTTLS_STATE_CLOSED) );

    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert(final,
                                TEST,
                                "close: once, idempotent, and final",
                                _counter) && result;
    result = d_tests_net_assert(released,
                                TEST,
                                "destroy after close; again; NULL",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_satisfied_by_plaintext
  Whether a new session under `_options` counts as satisfied.
*/
static bool
d_tests_starttls_satisfied_by_plaintext(
    const struct d_starttls_options* _options,
    struct d_net_connection*         _connection
)
{
    struct d_starttls_session session = { .state = D_STARTTLS_STATE_CLOSED };

    if (d_starttls_init(&session,
                        _options,
                        _connection) != D_STARTTLS_OK)
    {
        return false;
    }

    const bool satisfied = d_starttls_is_satisfied(&session);

    d_starttls_destroy(&session);

    return satisfied;
}

/*
d_tests_sa_starttls_ownership
  Tests the following:
  - destroying without close destroys an owned connection, closing it once
  - a borrowed connection is never closed or destroyed
  - OPPORTUNISTIC and DISABLED are satisfied by plaintext
*/
bool
d_tests_sa_starttls_ownership(
    struct d_test_counter* _counter
)
{
    const char* const               TEST    = "ownership";
    struct d_starttls_session       session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_starttls_fixture fixture = { .peer = NULL };
    bool                            result  = true;

    if (!d_tests_net_assert(d_tests_starttls_fixture_open(
                                &fixture,
                                D_STARTTLS_ROLE_CLIENT,
                                D_STARTTLS_POLICY_REQUIRED),
                            TEST,
                            "a fixture",
                            _counter))
    {
        return false;
    }

    const bool owned = (d_starttls_init(&session,
                                        &fixture.options,
                                        fixture.connection) ==
                        D_STARTTLS_OK);

    d_starttls_destroy(&session);

    struct d_starttls_options borrowed      = fixture.options;
    struct d_starttls_options opportunistic = fixture.options;
    struct d_starttls_options disabled      = fixture.options;

    borrowed.owns_connection = false;
    opportunistic.policy     = D_STARTTLS_POLICY_OPPORTUNISTIC;
    disabled.policy          = D_STARTTLS_POLICY_DISABLED;
    disabled.tls             = NULL;

    const bool lent = ( (d_starttls_init(&session,
                                         &borrowed,
                                         fixture.connection) ==
                         D_STARTTLS_OK) &&
                        (d_starttls_close(&session) == D_STARTTLS_OK) );

    d_starttls_destroy(&session);

    const bool satisfied =
        ( (d_tests_starttls_satisfied_by_plaintext(&opportunistic,
                                                   fixture.connection)) &&
          (d_tests_starttls_satisfied_by_plaintext(&disabled,
                                                   fixture.connection)) );
    const int  closes    = fixture.peer->closes;
    const int  destroys  = fixture.peer->destroys;

    d_tests_starttls_fixture_close(&fixture);
    result = d_tests_net_assert( (owned)       &&
                                 (lent)        &&
                                 (closes == 3) &&
                                 (destroys == 3),
                                 TEST,
                                 "owned destroyed once each; borrowed never",
                                 _counter) && result;
    result = d_tests_net_assert(satisfied,
                                TEST,
                                "OPPORTUNISTIC and DISABLED accept plaintext",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_null_queries
  Tests the following:
  - every query reads NULL as a closed session; close and upgrade refuse it
*/
bool
d_tests_sa_starttls_null_queries(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "NULL queries";
    bool              result = true;

    const bool nulls =
        ( (d_starttls_get_state(NULL) == D_STARTTLS_STATE_CLOSED)       &&
          (!d_starttls_is_secure(NULL))                                 &&
          (!d_starttls_is_satisfied(NULL))                              &&
          (d_starttls_epoch(NULL) == 0)                                 &&
          (d_starttls_detail(NULL) == NULL)                             &&
          (d_starttls_net_error(NULL) == D_NET_OK)                      &&
          (d_starttls_tls_session(NULL) == NULL)                        &&
          (d_starttls_buffered(NULL) == 0)                              &&
          (d_starttls_close(NULL) == D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
          (d_starttls_upgrade(NULL) == D_STARTTLS_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(nulls,
                                TEST,
                                "the queries read NULL as closed",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_names_distinct
  Whether every status from D_STARTTLS_OK to D_STARTTLS_ERROR_VERIFY has a
description of its own, none of them the fallback.
*/
static bool
d_tests_starttls_names_distinct(
    void
)
{
    bool distinct = true;

    // every status against the ones before it
    for (int i = 0; (distinct) && (i <= (int)D_STARTTLS_ERROR_VERIFY); ++i)
    {
        const char* text =
            d_starttls_error_string((enum d_starttls_error)i);

        distinct = ( (text != NULL)    &&
                     (text[0] != '\0') &&
                     (strcmp(text,
                             "unknown status") != 0) );

        for (int j = 0; (distinct) && (j < i); ++j)
        {
            distinct = (strcmp(text,
                               d_starttls_error_string(
                                   (enum d_starttls_error)j)) != 0);
        }
    }

    return distinct;
}

/*
d_tests_sa_starttls_names
  Tests the following:
  - every status has its own description, none of them the fallback
  - a status, state, or policy outside its enumeration reads as unknown
  - each state and policy has its name
*/
bool
d_tests_sa_starttls_names(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "names";
    bool              result = true;

    const bool unknown =
        ( (strcmp(d_starttls_error_string((enum d_starttls_error)99),
                  "unknown status") == 0) &&
          (strcmp(d_starttls_state_string((enum d_starttls_state)9),
                  "unknown") == 0)        &&
          (strcmp(d_starttls_policy_string((enum d_starttls_policy)0),
                  "unknown") == 0) );
    const bool named =
        ( (strcmp(d_starttls_state_string(D_STARTTLS_STATE_PLAINTEXT),
                  "plaintext") == 0)     &&
          (strcmp(d_starttls_state_string(D_STARTTLS_STATE_SECURE),
                  "secure") == 0)        &&
          (strcmp(d_starttls_state_string(D_STARTTLS_STATE_FAILED),
                  "failed") == 0)        &&
          (strcmp(d_starttls_state_string(D_STARTTLS_STATE_CLOSED),
                  "closed") == 0)        &&
          (strcmp(d_starttls_policy_string(D_STARTTLS_POLICY_DISABLED),
                  "disabled") == 0)      &&
          (strcmp(d_starttls_policy_string(
                      D_STARTTLS_POLICY_OPPORTUNISTIC),
                  "opportunistic") == 0) &&
          (strcmp(d_starttls_policy_string(D_STARTTLS_POLICY_REQUIRED),
                  "required") == 0) );

    result = d_tests_net_assert(d_tests_starttls_names_distinct(),
                                TEST,
                                "every status has a description of its own",
                                _counter) && result;
    result = d_tests_net_assert(unknown,
                                TEST,
                                "values outside the enumerations are unknown",
                                _counter) && result;
    result = d_tests_net_assert(named,
                                TEST,
                                "each state and policy by name",
                                _counter) && result;

    return result;
}

// d_tests_starttls_mapping
//   struct: an error, and the d_net_error it maps to.
struct d_tests_starttls_mapping
{
    enum d_starttls_error error;
    enum d_net_error      net;
};

/*
d_tests_sa_starttls_errors_to_net
  Tests the following:
  - each error maps onto the d_net_error net.h's rules give it
  - a value outside the enum maps to D_NET_ERROR_UNKNOWN
*/
bool
d_tests_sa_starttls_errors_to_net(
    struct d_test_counter* _counter
)
{
    const struct d_tests_starttls_mapping MAP[] =
    {
        { D_STARTTLS_OK,                     D_NET_OK                      },
        { D_STARTTLS_ERROR_INVALID_ARGUMENT, D_NET_ERROR_INVALID_ARGUMENT  },
        { D_STARTTLS_ERROR_STATE,            D_NET_ERROR_INVALID_ARGUMENT  },
        { D_STARTTLS_ERROR_OUT_OF_MEMORY,    D_NET_ERROR_OUT_OF_MEMORY     },
        { D_STARTTLS_ERROR_TRANSPORT,        D_NET_ERROR_UNKNOWN           },
        { D_STARTTLS_ERROR_CLOSED,           D_NET_ERROR_CLOSED            },
        { D_STARTTLS_ERROR_TRUNCATED,        D_NET_ERROR_CLOSED            },
        { D_STARTTLS_ERROR_TIMED_OUT,        D_NET_ERROR_TIMED_OUT         },
        { D_STARTTLS_ERROR_LINE_TOO_LONG,    D_NET_ERROR_MESSAGE_TOO_LARGE },
        { D_STARTTLS_ERROR_PROTOCOL,         D_NET_ERROR_PROTOCOL          },
        { D_STARTTLS_ERROR_NOT_OFFERED,      D_NET_ERROR_UNKNOWN           },
        { D_STARTTLS_ERROR_REJECTED,         D_NET_ERROR_UNKNOWN           },
        { D_STARTTLS_ERROR_INJECTION,        D_NET_ERROR_PROTOCOL          },
        { D_STARTTLS_ERROR_NO_BACKEND,       D_NET_ERROR_UNKNOWN           },
        { D_STARTTLS_ERROR_CONFIG,           D_NET_ERROR_INVALID_ARGUMENT  },
        { D_STARTTLS_ERROR_HANDSHAKE,        D_NET_ERROR_PROTOCOL          },
        { D_STARTTLS_ERROR_VERIFY,           D_NET_ERROR_ACCESS_DENIED     }
    };
    const size_t count  = sizeof(MAP) / sizeof(MAP[0]);
    bool         mapped = true;
    bool         result = true;

    // each error against the table
    for (size_t i = 0u; i < count; ++i)
    {
        mapped = ( (mapped) &&
                   (d_starttls_error_to_net(MAP[i].error) == MAP[i].net) );
    }

    const bool outside =
        (d_starttls_error_to_net((enum d_starttls_error)99) ==
         D_NET_ERROR_UNKNOWN);

    result = d_tests_net_assert(mapped,
                                "d_starttls_error_to_net",
                                "each error maps by net.h's rules",
                                _counter) && result;
    result = d_tests_net_assert(outside,
                                "d_starttls_error_to_net",
                                "a value outside the enum is unknown",
                                _counter) && result;

    return result;
}

// d_tests_starttls_from_net
//   struct: a connection's error, and the module's error it becomes.
struct d_tests_starttls_from_net
{
    enum d_net_error      net;
    enum d_starttls_error error;
};

/*
FROM_NET
  Every d_net_error in the enum's order, placed by net.h's rules the other
way: the namesakes, the three ways a connection ends, and TRANSPORT.
*/
static const struct d_tests_starttls_from_net FROM_NET[] =
{
    { D_NET_OK,                        D_STARTTLS_OK                     },
    { D_NET_ERROR_CLOSED,              D_STARTTLS_ERROR_CLOSED           },
    { D_NET_ERROR_WOULD_BLOCK,         D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_TIMED_OUT,           D_STARTTLS_ERROR_TIMED_OUT        },
    { D_NET_ERROR_INTERRUPTED,         D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_CONNECTION_RESET,    D_STARTTLS_ERROR_CLOSED           },
    { D_NET_ERROR_CONNECTION_REFUSED,  D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_CONNECTION_ABORTED,  D_STARTTLS_ERROR_CLOSED           },
    { D_NET_ERROR_ADDRESS_IN_USE,      D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_ADDRESS_INVALID,     D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_HOST_UNREACHABLE,    D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_NETWORK_DOWN,        D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_ACCESS_DENIED,       D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_INVALID_ARGUMENT,    D_STARTTLS_ERROR_INVALID_ARGUMENT },
    { D_NET_ERROR_MESSAGE_TOO_LARGE,   D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_TOO_MANY_OPEN_FILES, D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_OUT_OF_MEMORY,       D_STARTTLS_ERROR_OUT_OF_MEMORY    },
    { D_NET_ERROR_UNKNOWN,             D_STARTTLS_ERROR_TRANSPORT        },
    { D_NET_ERROR_PROTOCOL,            D_STARTTLS_ERROR_PROTOCOL         }
};

/*
d_tests_sa_starttls_errors_from_net
  Tests the following:
  - every d_net_error becomes the error net.h's rules give it
  - a value outside the enum is TRANSPORT
  - the namesakes come back from d_starttls_error_to_net unchanged
*/
bool
d_tests_sa_starttls_errors_from_net(
    struct d_test_counter* _counter
)
{
    const size_t count  = sizeof(FROM_NET) / sizeof(FROM_NET[0]);
    bool         table  = (count == (size_t)D_NET_ERROR_PROTOCOL + 1u);
    bool         result = true;

    // each row, in the enum's order
    for (size_t i = 0u; i < count; ++i)
    {
        table = ( (FROM_NET[i].net == (enum d_net_error)i) &&
                  (d_starttls_error_from_net(FROM_NET[i].net) ==
                   FROM_NET[i].error)                      &&
                  (table) );
    }

    const bool outside =
        (d_starttls_error_from_net((enum d_net_error)0x7FFF) ==
         D_STARTTLS_ERROR_TRANSPORT);
    const bool back =
        ( (d_starttls_error_from_net(d_starttls_error_to_net(
                                         D_STARTTLS_ERROR_TIMED_OUT)) ==
           D_STARTTLS_ERROR_TIMED_OUT) &&
          (d_starttls_error_from_net(d_starttls_error_to_net(
                                         D_STARTTLS_ERROR_CLOSED)) ==
           D_STARTTLS_ERROR_CLOSED)    &&
          (d_starttls_error_from_net(d_starttls_error_to_net(
                                         D_STARTTLS_ERROR_PROTOCOL)) ==
           D_STARTTLS_ERROR_PROTOCOL)  &&
          (d_starttls_error_from_net(d_starttls_error_to_net(
                                         D_STARTTLS_ERROR_OUT_OF_MEMORY)) ==
           D_STARTTLS_ERROR_OUT_OF_MEMORY) );

    result = d_tests_net_assert(table,
                                "d_starttls_error_from_net",
                                "places every d_net_error by net.h's rules",
                                _counter) && result;
    result = d_tests_net_assert(outside,
                                "d_starttls_error_from_net",
                                "a value outside the enum is TRANSPORT",
                                _counter) && result;
    result = d_tests_net_assert(back,
                                "d_starttls_error_from_net",
                                "namesakes come back from to_net unchanged",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_lines
  Tests the following:
  - several lines in one read come back one at a time, terminators removed,
    an empty line included
  - a line split across two reads is joined
  - the end of the stream between lines is an orderly D_STARTTLS_ERROR_CLOSED,
    which leaves the session plaintext, and no line or length behind
*/
bool
d_tests_sa_starttls_lines(
    struct d_test_counter* _counter
)
{
    const char* const         TEST    = "lines";
    struct d_tests_net_peer*  peer    = d_tests_net_peer_new();
    struct d_starttls_session session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    const char*               line    = NULL;
    size_t                    length  = 0;
    bool                      result  = true;

    if (!d_tests_net_assert(peer != NULL,
                            TEST,
                            "a scripted peer",
                            _counter))
    {
        return false;
    }

    peer->hangs_up = true;
    (void)d_tests_net_peer_say(peer,
                               "",
                               "220 mail.example ready\r\n"
                               "250-first\r\n250 last\r\n\r\n");
    (void)d_tests_net_peer_say(peer,
                               "",
                               "250-PIPE");
    (void)d_tests_net_peer_say(peer,
                               "",
                               "LINING\r\n");

    const bool lines =
        ( (d_tests_starttls_plain(&session,
                                  peer,
                                  0,
                                  true))                       &&
          (d_tests_starttls_line_is(&session,
                                    "220 mail.example ready")) &&
          (d_tests_starttls_line_is(&session,
                                    "250-first"))              &&
          (d_tests_starttls_line_is(&session,
                                    "250 last"))               &&
          (d_tests_starttls_line_is(&session,
                                    ""))                       &&
          (d_tests_starttls_line_is(&session,
                                    "250-PIPELINING")) );
    const bool ended =
        ( (d_starttls_recv_line(&session,
                                &line,
                                &length) == D_STARTTLS_ERROR_CLOSED) &&
          (line == NULL)                                             &&
          (length == 0)                                              &&
          (d_starttls_get_state(&session) ==
           D_STARTTLS_STATE_PLAINTEXT) );

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(lines,
                                TEST,
                                "lines one at a time, empty and split ones",
                                _counter) && result;
    result = d_tests_net_assert(ended,
                                TEST,
                                "the end between lines is orderly",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_lines_trickled
  Tests the following:
  - lines that arrive a byte at a time are assembled
*/
bool
d_tests_sa_starttls_lines_trickled(
    struct d_test_counter* _counter
)
{
    const char* const         TEST    = "lines trickled";
    struct d_tests_net_peer*  peer    = d_tests_net_peer_new();
    struct d_starttls_session session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    bool                      result  = true;

    if (!d_tests_net_assert(peer != NULL,
                            TEST,
                            "a scripted peer",
                            _counter))
    {
        return false;
    }

    peer->recv_chunk = 1;
    (void)d_tests_net_peer_say(peer,
                               "",
                               "EHLO client.example\r\nNOOP\r\n");

    const bool trickled =
        ( (d_tests_starttls_plain(&session,
                                  peer,
                                  0,
                                  true))                    &&
          (d_tests_starttls_line_is(&session,
                                    "EHLO client.example")) &&
          (d_tests_starttls_line_is(&session,
                                    "NOOP")) );

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(trickled,
                                TEST,
                                "lines that arrive a byte at a time",
                                _counter) && result;

    return result;
}

// d_tests_starttls_first
//   struct: what receiving one line produced.
struct d_tests_starttls_first
{
    enum d_starttls_error status;    // what the receive reported
    enum d_starttls_state state;     // the session's state after it
    char                  line[64];  // the line, when there was one
};

/*
d_tests_starttls_first_line
  Opens a plaintext session over one turn of `_length` bytes, with the peer
hanging up after it, and receives one line: what the receive reported, the
line when there is one, and the state that follows.
*/
static struct d_tests_starttls_first
d_tests_starttls_first_line(
    const void* _bytes,
    size_t      _length,
    bool        _strict_crlf
)
{
    struct d_tests_starttls_first first   =
    {
        .status = D_STARTTLS_ERROR_OUT_OF_MEMORY,
        .state  = D_STARTTLS_STATE_CLOSED
    };
    struct d_tests_net_peer*      peer    = d_tests_net_peer_new();
    struct d_starttls_session     session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    const char*                   line    = NULL;
    size_t                        length  = 0;

    if ( (peer)                                &&
         (d_tests_net_peer_say_bytes(peer,
                                     "",
                                     _bytes,
                                     _length)) &&
         (d_tests_starttls_plain(&session,
                                 peer,
                                 0,
                                 _strict_crlf)) )
    {
        peer->hangs_up = true;
        first.status   = d_starttls_recv_line(&session,
                                              &line,
                                              &length);
        first.state    = d_starttls_get_state(&session);
    }

    if ( (first.status == D_STARTTLS_OK) &&
         (length < sizeof(first.line)) )
    {
        memcpy(first.line,
               line,
               length + 1);
    }

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);

    return first;
}

/*
d_tests_starttls_malformed
  Whether one line of `_text` (`_length` bytes) is refused as a protocol
violation, fatally, both with strict_crlf on and with it off -- or, when
`_strict_only`, with it on alone.
*/
static bool
d_tests_starttls_malformed(
    const void* _text,
    size_t      _length,
    bool        _strict_only
)
{
    const struct d_tests_starttls_first strict  =
        d_tests_starttls_first_line(_text,
                                    _length,
                                    true);
    const struct d_tests_starttls_first lenient =
        d_tests_starttls_first_line(_text,
                                    _length,
                                    false);

    return ( (strict.status == D_STARTTLS_ERROR_PROTOCOL) &&
             (strict.state == D_STARTTLS_STATE_FAILED)    &&
             ( (_strict_only) ||
               ( (lenient.status == D_STARTTLS_ERROR_PROTOCOL) &&
                 (lenient.state == D_STARTTLS_STATE_FAILED) ) ) );
}

/*
d_tests_sa_starttls_line_endings
  Tests the following:
  - under strict_crlf a bare LF is a fatal protocol violation; without it,
    a bare LF ends a line
  - a CR inside a line, and a NUL, are refused either way, and so is a CR
    left before the CRLF
  - a stream that ends inside a line, even one byte into it, is a fatal
    protocol violation
*/
bool
d_tests_sa_starttls_line_endings(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "line endings";
    bool              result = true;

    const struct d_tests_starttls_first lenient =
        d_tests_starttls_first_line("bare\n",
                                    5,
                                    false);
    const bool bare_lf =
        ( (d_tests_starttls_malformed("bare\n",
                                      5,
                                      true)) &&
          (lenient.status == D_STARTTLS_OK)  &&
          (strcmp(lenient.line,
                  "bare") == 0)              &&
          (lenient.state == D_STARTTLS_STATE_PLAINTEXT) );
    const bool inside =
        ( (d_tests_starttls_malformed("a\rb\r\n",
                                      5,
                                      false)) &&
          (d_tests_starttls_malformed("a\0b\r\n",
                                      5,
                                      false)) &&
          (d_tests_starttls_malformed("x\r\r\n",
                                      4,
                                      false)) );
    const bool cut =
        ( (d_tests_starttls_malformed("partial",
                                      7,
                                      false)) &&
          (d_tests_starttls_malformed("p",
                                      1,
                                      false)) );

    result = d_tests_net_assert(bare_lf,
                                TEST,
                                "a bare LF: refused when strict, else an end",
                                _counter) && result;
    result = d_tests_net_assert(inside,
                                TEST,
                                "a CR or NUL inside a line, either way",
                                _counter) && result;
    result = d_tests_net_assert(cut,
                                TEST,
                                "a stream that ends inside a line",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_failed_session
  Tests the following:
  - after a fatal error the session refuses every receive and send, and
    nothing more reaches the peer
  - a refused receive returns no line, no length, and no count; a NULL
    buffer is still refused as an argument first
*/
bool
d_tests_sa_starttls_failed_session(
    struct d_test_counter* _counter
)
{
    const char* const         TEST    = "failed session";
    struct d_tests_net_peer*  peer    = d_tests_net_peer_new();
    struct d_starttls_session session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    const char*               text    = NULL;
    size_t                    length  = 0;
    size_t                    count   = 9;
    char                      byte    = 'x';
    bool                      stopped = false;
    bool                      result  = true;

    if ( (peer)                                   &&
         (d_tests_net_peer_say(peer,
                               "",
                               "bare\nmore\r\n")) &&
         (d_tests_starttls_plain(&session,
                                 peer,
                                 0,
                                 true)) )
    {
        stopped = ( (d_starttls_recv_line(&session,
                                          &text,
                                          &length) ==
                     D_STARTTLS_ERROR_PROTOCOL)                         &&
                    (d_starttls_recv_line(&session,
                                          &text,
                                          &length) ==
                     D_STARTTLS_ERROR_STATE)                            &&
                    (text == NULL)                                      &&
                    (length == 0)                                       &&
                    (d_starttls_recv(&session,
                                     &byte,
                                     1,
                                     &count) == D_STARTTLS_ERROR_STATE) &&
                    (count == 0)                                        &&
                    (d_starttls_recv_exact(&session,
                                           NULL,
                                           1) ==
                     D_STARTTLS_ERROR_INVALID_ARGUMENT)                 &&
                    (d_starttls_send_line(&session,
                                          "NOOP") ==
                     D_STARTTLS_ERROR_STATE)                            &&
                    (peer->sent == 0) );
    }

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(stopped,
                                TEST,
                                "a failed session refuses further I/O",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_long_line
  Whether a line of `_count` 'a's and a CRLF, delivered `_chunk` bytes a
read (0 for all at once), is received whole by a session whose buffer is
`_buffer_bytes`; *_status receives what the receive reported, or
D_STARTTLS_ERROR_STATE for a failure that did not end the session.
*/
static bool
d_tests_starttls_long_line(
    size_t                 _count,
    size_t                 _chunk,
    size_t                 _buffer_bytes,
    enum d_starttls_error* _status
)
{
    static char               text[D_TESTS_NET_PEER_TURN_BYTES] = { 0 };
    struct d_tests_net_peer*  peer    = d_tests_net_peer_new();
    struct d_starttls_session session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    const char*               line    = NULL;
    size_t                    length  = 0;

    *_status = D_STARTTLS_ERROR_OUT_OF_MEMORY;

    if ( (peer) &&
         ((_count + 3) <= sizeof(text)) )
    {
        d_tests_starttls_repeat(text,
                                'a',
                                _count,
                                "\r\n");
        peer->recv_chunk = _chunk;

        if ( (d_tests_net_peer_say(peer,
                                   "",
                                   text)) &&
             (d_tests_starttls_plain(&session,
                                     peer,
                                     _buffer_bytes,
                                     true)) )
        {
            *_status = d_starttls_recv_line(&session,
                                            &line,
                                            &length);
        }
    }

    const bool whole = ( (*_status == D_STARTTLS_OK) &&
                         (length == _count)          &&
                         (memcmp(line,
                                 text,
                                 _count) == 0) );

    if ( (*_status != D_STARTTLS_OK) &&
         (d_starttls_get_state(&session) != D_STARTTLS_STATE_FAILED) )
    {
        *_status = D_STARTTLS_ERROR_STATE;
    }

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);

    return whole;
}

/*
d_tests_sa_starttls_line_limit
  Tests the following:
  - the buffer bounds a line, terminator included: with 512 bytes, a line
    of 510 characters is received and one of 511 is a fatal
    D_STARTTLS_ERROR_LINE_TOO_LONG
  - a line at the limit that arrives in pieces is received whole
  - a larger buffer takes a longer line
*/
bool
d_tests_sa_starttls_line_limit(
    struct d_test_counter* _counter
)
{
    const char* const     TEST     = "line limit";
    enum d_starttls_error at_limit = D_STARTTLS_OK;
    enum d_starttls_error over     = D_STARTTLS_OK;
    enum d_starttls_error pieces   = D_STARTTLS_OK;
    enum d_starttls_error larger   = D_STARTTLS_OK;
    bool                  result   = true;

    const bool fits  = d_tests_starttls_long_line(510,
                                                  0,
                                                  512,
                                                  &at_limit);
    const bool spill = d_tests_starttls_long_line(511,
                                                  0,
                                                  512,
                                                  &over);
    const bool built = d_tests_starttls_long_line(510,
                                                  7,
                                                  512,
                                                  &pieces);
    const bool wide  = d_tests_starttls_long_line(600,
                                                  0,
                                                  1024,
                                                  &larger);

    result = d_tests_net_assert( (fits) &&
                                 (at_limit == D_STARTTLS_OK),
                                 TEST,
                                 "510 characters and CRLF fill 512 bytes",
                                 _counter) && result;
    result = d_tests_net_assert( (!spill) &&
                                 (over == D_STARTTLS_ERROR_LINE_TOO_LONG),
                                 TEST,
                                 "511 characters are too long, fatally",
                                 _counter) && result;
    result = d_tests_net_assert( (built)                   &&
                                 (pieces == D_STARTTLS_OK) &&
                                 (wide)                    &&
                                 (larger == D_STARTTLS_OK),
                                 TEST,
                                 "a line in pieces; a longer one, more room",
                                 _counter) && result;

    return result;
}

/*
d_tests_starttls_bytes_are
  Whether the next receive of at most `_capacity` bytes returns exactly
`_expected`.
*/
static bool
d_tests_starttls_bytes_are(
    struct d_starttls_session* _session,
    size_t                     _capacity,
    const char*                _expected
)
{
    char   bytes[32] = { 0 };
    size_t count     = 0;

    return ( (_capacity <= sizeof(bytes))               &&
             (d_starttls_recv(_session,
                              bytes,
                              _capacity,
                              &count) == D_STARTTLS_OK) &&
             (count == strlen(_expected))               &&
             (memcmp(bytes,
                     _expected,
                     count) == 0) );
}

/*
d_tests_starttls_literal_peer
  A peer that sends an IMAP literal between two lines and three bytes more,
then three, four, and three bytes in reads of their own; NULL if it could
not be made.
*/
static struct d_tests_net_peer*
d_tests_starttls_literal_peer(
    void
)
{
    struct d_tests_net_peer* peer = d_tests_net_peer_new();

    if ( (peer)                                 &&
         (d_tests_net_peer_say(peer,
                               "",
                               "* 1 FETCH (BODY[] {5}\r\nhello)\r\n"
                               "A1 OK\r\nxyz")) &&
         (d_tests_net_peer_say(peer,
                               "",
                               "123"))          &&
         (d_tests_net_peer_say(peer,
                               "",
                               "abc"))          &&
         (d_tests_net_peer_say(peer,
                               "",
                               "defg")) )
    {
        return peer;
    }

    d_tests_net_peer_free(peer);

    return NULL;
}

/*
d_tests_starttls_reads_literal
  Whether `_session` receives the literal peer's opening: a line, exactly
its five counted bytes from the bytes the line reader buffered, and the two
lines after them, leaving three bytes buffered.
*/
static bool
d_tests_starttls_reads_literal(
    struct d_starttls_session* _session
)
{
    char bytes[5] = { 0 };

    return ( (d_tests_starttls_line_is(_session,
                                       "* 1 FETCH (BODY[] {5}")) &&
             (d_starttls_buffered(_session) == 18)               &&
             (d_starttls_recv_exact(_session,
                                    bytes,
                                    5) == D_STARTTLS_OK)         &&
             (memcmp(bytes,
                     "hello",
                     5) == 0)                                    &&
             (d_tests_starttls_line_is(_session,
                                       ")"))                     &&
             (d_tests_starttls_line_is(_session,
                                       "A1 OK"))                 &&
             (d_starttls_buffered(_session) == 3) );
}

/*
d_tests_sa_starttls_literal
  Tests the following:
  - an IMAP literal: a line, then exactly its counted bytes from what the
    line reader already buffered, then lines again
  - bytes already buffered come first, and a receive returns no more than
    them; the next one reads the connection
  - an exact receive gathers across reads; one of a single byte reads it,
    and one of nothing succeeds
*/
bool
d_tests_sa_starttls_literal(
    struct d_test_counter* _counter
)
{
    const char* const         TEST     = "literal";
    struct d_tests_net_peer*  peer     = d_tests_starttls_literal_peer();
    struct d_starttls_session session  =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    char                      bytes[8] = { 0 };
    bool                      result   = true;

    if (!d_tests_net_assert(peer != NULL,
                            TEST,
                            "a scripted peer",
                            _counter))
    {
        return false;
    }

    const bool literal  = ( (d_tests_starttls_plain(&session,
                                                    peer,
                                                    0,
                                                    true)) &&
                            (d_tests_starttls_reads_literal(&session)) );
    const bool first    = ( (d_tests_starttls_bytes_are(&session,
                                                        2,
                                                        "xy")) &&
                            (d_tests_starttls_bytes_are(&session,
                                                        16,
                                                        "z"))  &&
                            (d_tests_starttls_bytes_are(&session,
                                                        16,
                                                        "123")) );
    const bool gathered = ( (d_starttls_recv_exact(&session,
                                                   bytes,
                                                   6) == D_STARTTLS_OK) &&
                            (d_starttls_recv_exact(&session,
                                                   bytes + 6,
                                                   1) == D_STARTTLS_OK) &&
                            (memcmp(bytes,
                                    "abcdefg",
                                    7) == 0)                            &&
                            (d_starttls_recv_exact(&session,
                                                   NULL,
                                                   0) == D_STARTTLS_OK) );

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(literal,
                                TEST,
                                "an IMAP literal between lines",
                                _counter) && result;
    result = d_tests_net_assert(first,
                                TEST,
                                "buffered bytes first, then the connection",
                                _counter) && result;
    result = d_tests_net_assert(gathered,
                                TEST,
                                "exact receives: across reads, of one, of none",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_refuses_bytes
  Whether every byte call refuses NULL buffers, a zero capacity, and NULL
outputs, and a send of nothing succeeds without reaching `_peer`.
*/
static bool
d_tests_starttls_refuses_bytes(
    struct d_starttls_session*     _session,
    const struct d_tests_net_peer* _peer
)
{
    char        bytes[4] = { 0 };
    size_t      count    = 0;
    const char* line     = NULL;

    return ( (d_starttls_recv(NULL,
                              bytes,
                              1,
                              &count) == D_STARTTLS_ERROR_INVALID_ARGUMENT)  &&
             (d_starttls_recv(_session,
                              NULL,
                              1,
                              &count) == D_STARTTLS_ERROR_INVALID_ARGUMENT)  &&
             (d_starttls_recv(_session,
                              bytes,
                              0,
                              &count) == D_STARTTLS_ERROR_INVALID_ARGUMENT)  &&
             (d_starttls_recv(_session,
                              bytes,
                              1,
                              NULL) == D_STARTTLS_ERROR_INVALID_ARGUMENT)    &&
             (d_starttls_recv_exact(_session,
                                    NULL,
                                    1) == D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
             (d_starttls_recv_exact(NULL,
                                    bytes,
                                    1) == D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
             (d_starttls_recv_line(_session,
                                   NULL,
                                   &count) ==
              D_STARTTLS_ERROR_INVALID_ARGUMENT)                             &&
             (d_starttls_recv_line(_session,
                                   &line,
                                   NULL) ==
              D_STARTTLS_ERROR_INVALID_ARGUMENT)                             &&
             (d_starttls_send(_session,
                              NULL,
                              1) == D_STARTTLS_ERROR_INVALID_ARGUMENT)       &&
             (d_starttls_send(NULL,
                              bytes,
                              1) == D_STARTTLS_ERROR_INVALID_ARGUMENT)       &&
             (d_starttls_send(_session,
                              NULL,
                              0) == D_STARTTLS_OK)                           &&
             (_peer->writes == 0) );
}

/*
d_tests_sa_starttls_byte_refusals
  Tests the following:
  - NULL buffers, a zero capacity, and NULL outputs are refused
  - a send of nothing succeeds without touching the connection, and bytes of
    every value are sent as they are, a single byte included
*/
bool
d_tests_sa_starttls_byte_refusals(
    struct d_test_counter* _counter
)
{
    const char* const         TEST      = "byte refusals";
    const unsigned char       binary[4] = { 0x00, 0x0D, 0x0A, 0xFF };
    struct d_tests_net_peer*  peer      = d_tests_net_peer_new();
    struct d_starttls_session session   =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    bool                      refused   = false;
    bool                      raw       = false;
    bool                      result    = true;

    if ( (peer) &&
         (d_tests_starttls_plain(&session,
                                 peer,
                                 0,
                                 true)) )
    {
        refused = d_tests_starttls_refuses_bytes(&session,
                                                 peer);
        raw     = ( (d_starttls_send(&session,
                                     binary,
                                     sizeof(binary)) == D_STARTTLS_OK) &&
                    (d_starttls_send(&session,
                                     "!",
                                     1) == D_STARTTLS_OK)              &&
                    (peer->sent == (sizeof(binary) + 1))               &&
                    (memcmp(peer->output,
                            binary,
                            sizeof(binary)) == 0)                      &&
                    (peer->output[sizeof(binary)] == '!') );
    }

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(refused,
                                TEST,
                                "NULLs and a zero capacity; an empty send",
                                _counter) && result;
    result = d_tests_net_assert(raw,
                                TEST,
                                "a send carries any byte",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_ends
  Whether a stream of `_text` that then ends makes an exact receive of five
bytes report `_expected`, leaving the session in `_state`.
*/
static bool
d_tests_starttls_ends(
    const char*           _text,
    enum d_starttls_error _expected,
    enum d_starttls_state _state
)
{
    struct d_tests_net_peer*  peer    = d_tests_net_peer_new();
    struct d_starttls_session session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    char                      bytes[5] = { 0 };
    bool                      ended    = false;

    if (!peer)
    {
        return false;
    }

    peer->hangs_up = true;

    if ( ( (_text[0] == '\0') ||
           (d_tests_net_peer_say(peer,
                                 "",
                                 _text)) ) &&
         (d_tests_starttls_plain(&session,
                                 peer,
                                 0,
                                 true)) )
    {
        ended = ( (d_starttls_recv_exact(&session,
                                         bytes,
                                         sizeof(bytes)) == _expected) &&
                  (d_starttls_get_state(&session) == _state) );
    }

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);

    return ended;
}

/*
d_tests_sa_starttls_stream_ends
  Tests the following:
  - a stream that ends part-way through an exact receive is a fatal
    protocol violation; one that ends before it is an orderly end
*/
bool
d_tests_sa_starttls_stream_ends(
    struct d_test_counter* _counter
)
{
    const char* const TEST   = "stream ends";
    bool              result = true;

    const bool cut   = d_tests_starttls_ends("abc",
                                             D_STARTTLS_ERROR_PROTOCOL,
                                             D_STARTTLS_STATE_FAILED);
    const bool clean = d_tests_starttls_ends("",
                                             D_STARTTLS_ERROR_CLOSED,
                                             D_STARTTLS_STATE_PLAINTEXT);

    result = d_tests_net_assert(cut,
                                TEST,
                                "cut short inside a counted payload is fatal",
                                _counter) && result;
    result = d_tests_net_assert(clean,
                                TEST,
                                "an end before it is orderly",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_refuses_line
  Whether the session refuses to send `_line`, sending nothing.
*/
static bool
d_tests_starttls_refuses_line(
    struct d_starttls_session*     _session,
    const struct d_tests_net_peer* _peer,
    const char*                    _line
)
{
    const size_t before = _peer->sent;

    return ( (d_starttls_send_line(_session,
                                   _line) ==
              D_STARTTLS_ERROR_INVALID_ARGUMENT) &&
             (_peer->sent == before) );
}

/*
d_tests_starttls_refuses_lines
  Whether `_session` refuses every line that holds a break or a NUL --
plain, or put there by a formatted argument -- a NULL line, and a NULL
session, sending nothing, and stays plaintext.
*/
static bool
d_tests_starttls_refuses_lines(
    struct d_starttls_session*     _session,
    const struct d_tests_net_peer* _peer
)
{
    return ( (d_tests_starttls_refuses_line(_session,
                                            _peer,
                                            "RCPT TO:<a>\r\nDATA")) &&
             (d_tests_starttls_refuses_line(_session,
                                            _peer,
                                            "bad\rline"))           &&
             (d_tests_starttls_refuses_line(_session,
                                            _peer,
                                            "bad\nline"))           &&
             (d_tests_starttls_refuses_line(_session,
                                            _peer,
                                            NULL))                  &&
             (d_starttls_send_linef(_session,
                                    "MAIL FROM:<%s>",
                                    "a@b>\r\nRCPT TO:<c@d") ==
              D_STARTTLS_ERROR_INVALID_ARGUMENT)                    &&
             (d_starttls_send_linef(_session,
                                    "x%cy",
                                    '\0') ==
              D_STARTTLS_ERROR_INVALID_ARGUMENT)                    &&
             (d_starttls_send_linef(NULL,
                                    "NOOP") ==
              D_STARTTLS_ERROR_INVALID_ARGUMENT)                    &&
             (d_starttls_get_state(_session) ==
              D_STARTTLS_STATE_PLAINTEXT) );
}

/*
d_tests_sa_starttls_send_line
  Tests the following:
  - a line goes out with CRLF appended, formatted or not
  - a CR, LF, or NUL inside a line is refused and nothing is sent, the
    injection through a formatted argument included; so is a NULL line, and
    a NULL session
*/
bool
d_tests_sa_starttls_send_line(
    struct d_test_counter* _counter
)
{
    const char* const         TEST    = "send line";
    struct d_tests_net_peer*  peer    = d_tests_net_peer_new();
    struct d_starttls_session session =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    bool                      plain   = false;
    bool                      refused = false;
    bool                      result  = true;

    if ( (peer) &&
         (d_tests_starttls_plain(&session,
                                 peer,
                                 0,
                                 true)) )
    {
        plain   = ( (d_starttls_send_line(&session,
                                          "EHLO client.example") ==
                     D_STARTTLS_OK)                                &&
                    (d_starttls_send_linef(&session,
                                           "MAIL FROM:<%s> SIZE=%d",
                                           "a@example.org",
                                           1024) == D_STARTTLS_OK) &&
                    (d_tests_net_peer_sent(peer,
                                           "EHLO client.example\r\n"
                                           "MAIL FROM:<a@example.org>"
                                           " SIZE=1024\r\n")) );
        refused = d_tests_starttls_refuses_lines(&session,
                                                 peer);
    }

    const bool unsent = ( (plain) &&
                          (peer->sent == strlen("EHLO client.example\r\n"
                                                "MAIL FROM:<a@example.org>"
                                                " SIZE=1024\r\n")) );

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(plain,
                                TEST,
                                "a line and a formatted line, with CRLF",
                                _counter) && result;
    result = d_tests_net_assert( (refused) &&
                                 (unsent),
                                 TEST,
                                 "line breaks and NULs refused, nothing sent",
                                 _counter) && result;

    return result;
}

/*
d_tests_sa_starttls_send_limit
  Tests the following:
  - a line and its CRLF must fit the session's buffer: 510 characters fit
    512 bytes and go out; 511 do not, and nothing is sent
*/
bool
d_tests_sa_starttls_send_limit(
    struct d_test_counter* _counter
)
{
    const char* const         TEST     = "send limit";
    struct d_tests_net_peer*  peer     = d_tests_net_peer_new();
    struct d_starttls_session session  =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    static char               text[600] = { 0 };
    enum d_starttls_error     too_long = D_TESTS_STARTTLS_UNREACHED;
    enum d_starttls_error     at_limit = D_TESTS_STARTTLS_UNREACHED;
    size_t                    unsent   = 1;
    bool                      result   = true;

    if ( (peer) &&
         (d_tests_starttls_plain(&session,
                                 peer,
                                 512,
                                 true)) )
    {
        d_tests_starttls_repeat(text,
                                'a',
                                511,
                                "");
        too_long  = d_starttls_send_line(&session,
                                         text);
        unsent    = peer->sent;
        text[510] = '\0';
        at_limit  = d_starttls_send_line(&session,
                                         text);
    }

    const bool limited = ( (too_long == D_STARTTLS_ERROR_LINE_TOO_LONG) &&
                           (unsent == 0)                                &&
                           (at_limit == D_STARTTLS_OK)                  &&
                           (peer->sent == 512) );

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(limited,
                                TEST,
                                "511 characters are refused, unsent; 510 go",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_sends_edges
  Whether `_session`, whose buffer is 1024 bytes, sends whole a formatted
line of exactly the stack scratch's 512 characters, then one of exactly its
limit, 1022, after the `_before` bytes `_peer` already holds.
*/
static bool
d_tests_starttls_sends_edges(
    struct d_starttls_session*     _session,
    const struct d_tests_net_peer* _peer,
    size_t                         _before
)
{
    static char text[1100]     = { 0 };
    static char expected[1100] = { 0 };

    d_tests_starttls_repeat(text,
                            'd',
                            512,
                            "");
    d_tests_starttls_repeat(expected,
                            'd',
                            512,
                            "\r\n");

    const bool scratch = ( (d_starttls_send_linef(_session,
                                                  "%s",
                                                  text) == D_STARTTLS_OK) &&
                           (_peer->sent == (_before + 514))               &&
                           (memcmp(_peer->output + _before,
                                   expected,
                                   514) == 0) );

    d_tests_starttls_repeat(text,
                            'e',
                            1022,
                            "");
    d_tests_starttls_repeat(expected,
                            'e',
                            1022,
                            "\r\n");

    return ( (scratch)                                      &&
             (d_starttls_send_linef(_session,
                                    "%s",
                                    text) == D_STARTTLS_OK) &&
             (_peer->sent == (_before + 514 + 1024))        &&
             (memcmp(_peer->output + _before + 514,
                     expected,
                     1024) == 0) );
}

/*
d_tests_sa_starttls_send_long
  Tests the following:
  - lines longer than the stack scratch go out whole, formatted or not
  - a formatted line of exactly the scratch's size, and one of exactly the
    session's limit, go out whole
  - a formatted line over the session's limit is refused before it is sent
*/
bool
d_tests_sa_starttls_send_long(
    struct d_test_counter* _counter
)
{
    const char* const         TEST     = "send long";
    struct d_tests_net_peer*  peer     = d_tests_net_peer_new();
    struct d_starttls_session session  =
    {
        .state = D_STARTTLS_STATE_CLOSED
    };
    static char               text[1100]     = { 0 };
    static char               expected[1300] = { 0 };
    bool                      heap     = false;
    bool                      edges    = false;
    bool                      limited  = false;
    bool                      result   = true;

    d_tests_starttls_repeat(text,
                            'b',
                            600,
                            "");
    d_tests_starttls_repeat(expected,
                            'b',
                            600,
                            "\r\n");
    d_tests_starttls_repeat(expected + 602,
                            'b',
                            600,
                            "\r\n");

    if ( (peer) &&
         (d_tests_starttls_plain(&session,
                                 peer,
                                 1024,
                                 true)) )
    {
        heap = ( (d_starttls_send_line(&session,
                                       text) == D_STARTTLS_OK)       &&
                 (d_starttls_send_linef(&session,
                                        "bbbbbbbbbb%s",
                                        text + 10) == D_STARTTLS_OK) &&
                 (d_tests_net_peer_sent(peer,
                                        expected)) );
        edges   = d_tests_starttls_sends_edges(&session,
                                               peer,
                                               strlen(expected));
        d_tests_starttls_repeat(text,
                                'c',
                                1023,
                                "");
        limited = ( (d_starttls_send_linef(&session,
                                           "%s",
                                           text) ==
                     D_STARTTLS_ERROR_LINE_TOO_LONG) &&
                    (peer->sent == (strlen(expected) + 514 + 1024)) );
    }

    d_starttls_destroy(&session);
    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(heap,
                                TEST,
                                "long lines go out whole",
                                _counter) && result;
    result = d_tests_net_assert(edges,
                                TEST,
                                "lines of the scratch's size and the limit",
                                _counter) && result;
    result = d_tests_net_assert(limited,
                                TEST,
                                "a formatted line too long is not sent",
                                _counter) && result;

    return result;
}

/*
d_tests_starttls_write_fails
  Whether, over `_peer` as the caller set it up, sending NOOP reports
`_expected`, with `_cause` kept as the connection's error, and leaves the
session failed when that is an error.
*/
static bool
d_tests_starttls_write_fails(
    struct d_tests_net_peer* _peer,
    enum d_starttls_error    _expected,
    enum d_net_error         _cause
)
{
    struct d_starttls_session session = { .state = D_STARTTLS_STATE_CLOSED };
    bool                      failed  = false;

    if (d_tests_starttls_plain(&session,
                               _peer,
                               0,
                               true))
    {
        failed = ( (d_starttls_send_line(&session,
                                         "NOOP") == _expected) &&
                   (d_starttls_net_error(&session) == _cause)  &&
                   ( (_expected == D_STARTTLS_OK) ||
                     ( (d_starttls_get_state(&session) ==
                        D_STARTTLS_STATE_FAILED) &&
                       (d_starttls_send_line(&session,
                                             "NOOP") ==
                        D_STARTTLS_ERROR_STATE) ) ) );
    }

    d_starttls_destroy(&session);

    return failed;
}

/*
d_tests_sa_starttls_broken_writes
  Tests the following:
  - short writes are repeated until the whole line is out
  - a write the connection fails is a fatal TRANSPORT, its cause kept, and
    one that takes nothing is a fatal CLOSED, the stream being gone
*/
bool
d_tests_sa_starttls_broken_writes(
    struct d_test_counter* _counter
)
{
    const char* const        TEST    = "broken writes";
    struct d_tests_net_peer* peer    = d_tests_net_peer_new();
    bool                     trickle = false;
    bool                     failed  = false;
    bool                     stalled = false;
    bool                     result  = true;

    if (peer)
    {
        peer->send_chunk = 1;
        trickle          = ( (d_tests_starttls_write_fails(peer,
                                                           D_STARTTLS_OK,
                                                           D_NET_OK)) &&
                             (d_tests_net_peer_sent(peer,
                                                    "NOOP\r\n"))      &&
                             (peer->writes == 6) );
        peer->send_chunk = 0;
        peer->sent       = 0;
        peer->fail_after = 3;
        failed           =
            d_tests_starttls_write_fails(peer,
                                         D_STARTTLS_ERROR_TRANSPORT,
                                         D_NET_ERROR_UNKNOWN);
        peer->fail_after = SIZE_MAX;
        peer->stalls     = true;
        stalled          =
            d_tests_starttls_write_fails(peer,
                                         D_STARTTLS_ERROR_CLOSED,
                                         D_NET_ERROR_CLOSED);
    }

    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(trickle,
                                TEST,
                                "short writes are repeated",
                                _counter) && result;
    result = d_tests_net_assert( (failed) &&
                                 (stalled),
                                 TEST,
                                 "a failed write, and an empty one, are fatal",
                                 _counter) && result;

    return result;
}

/*
d_tests_starttls_read_fails
  Whether, over `_peer` as the caller set it up, receiving a line reports
`_expected`, with `_cause` kept as the connection's error, and leaves the
session failed.
*/
static bool
d_tests_starttls_read_fails(
    struct d_tests_net_peer* _peer,
    enum d_starttls_error    _expected,
    enum d_net_error         _cause
)
{
    struct d_starttls_session session = { .state = D_STARTTLS_STATE_CLOSED };
    const char*               line    = NULL;
    size_t                    length  = 0;
    bool                      failed  = false;

    if (d_tests_starttls_plain(&session,
                               _peer,
                               0,
                               true))
    {
        failed = ( (d_starttls_recv_line(&session,
                                         &line,
                                         &length) == _expected) &&
                   (d_starttls_net_error(&session) == _cause)   &&
                   (d_starttls_get_state(&session) ==
                    D_STARTTLS_STATE_FAILED) );
    }

    d_starttls_destroy(&session);

    return failed;
}

/*
d_tests_sa_starttls_broken_reads
  Tests the following:
  - a read that claims more bytes than it had room for is a fatal
    TRANSPORT, net.c having refused it as UNKNOWN
  - a read that times out is a fatal timeout, with its cause kept
*/
bool
d_tests_sa_starttls_broken_reads(
    struct d_test_counter* _counter
)
{
    const char* const        TEST      = "broken reads";
    struct d_tests_net_peer* peer      = d_tests_net_peer_new();
    bool                     empty     = false;
    bool                     timed_out = false;
    bool                     result    = true;

    if (peer)
    {
        peer->lies = true;
        empty      = d_tests_starttls_read_fails(peer,
                                                 D_STARTTLS_ERROR_TRANSPORT,
                                                 D_NET_ERROR_UNKNOWN);
        peer->lies = false;
        timed_out  = d_tests_starttls_read_fails(peer,
                                                 D_STARTTLS_ERROR_TIMED_OUT,
                                                 D_NET_ERROR_TIMED_OUT);
    }

    d_tests_net_peer_free(peer);
    result = d_tests_net_assert(empty,
                                TEST,
                                "a read past its room is fatal",
                                _counter) && result;
    result = d_tests_net_assert(timed_out,
                                TEST,
                                "a timeout is fatal",
                                _counter) && result;

    return result;
}
