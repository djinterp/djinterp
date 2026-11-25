/*******************************************************************************
* djinterp [c][net]                                                  tls_tests.h
*
* Tests of the TLS kernel's contexts and sessions, and of a
* session as a net.h connection.
*   Contexts are made over an engine that counts what the kernel asks of
* it, so a refusal can be placed before the engine or in it. A client and a
* server session talk over net_memory.h's pipe, in one thread: the pipe
* answers a full or empty ring with WOULD_BLOCK, so each handshake and
* transfer is driven by calling both sides in turn, and every resumable path
* of the pump is exercised. The OpenSSL-family engine does the cryptography,
* over certificates net_tests_pki.c makes for the run; without it the suite
* reports that it skipped. An engine whose operations a switch can make
* answer what the contract forbids tests how the kernel holds an engine to
* it. Each test returns true when every assertion held.
*
*
* path:      /tests/djinterp/c/modules/net/tls/tls_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Types
         1.  dj_tests_tls_case
         2.  d_tests_tls_world
         3.  dj_tests_tls_pair
    2.  Helpers
2.  TESTS
    -----
    1.  Contexts
    2.  Sessions
    3.  Sessions as connections
    4.  Engines
    5.  The section
*/

#ifndef DJINTERP_NET_TLS_TLS_TESTS_H
#define DJINTERP_NET_TLS_TLS_TESTS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/net_memory.h"       // dj_net_pipe
#include "../../../../../../inc/djinterp/c/modules/net/tls/tls.h"          // the kernel
#include "../../../../../../inc/djinterp/c/modules/net/tls/tls_openssl.h"  // the engine
#include "../net_tests_pki.h"                              // certificates
#include "../net_tests_assert.h"                        // asserts
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// dj_tests_tls_case
//   struct: a named test, as the section's table lists it.
struct dj_tests_tls_case
{
    const char* name;
    bool        (*run)(struct dj_test_suite* _suite);
};

// 1.1.2
// d_tests_tls_world
//   struct: what every test shares: the run's certificates, a client
// context that trusts their CA and checks names, and a server context that
// presents the server certificate.
struct dj_tests_tls_world
{
    struct dj_tests_net_pki pki;
    struct dj_tls_context   client;
    struct dj_tls_context   server;
};

// 1.1.3
// dj_tests_tls_pair
//   struct: a client session over end A of a pipe and a server session over
// end B, with the pipe's storage. It must not move once opened: the pipe and
// the sessions point into it.
struct dj_tests_tls_pair
{
    struct dj_net_pipe    pipe;
    unsigned char        to_server[4096];
    unsigned char        to_client[4096];
    struct dj_tls_session client;
    struct dj_tls_session server;
};

// 1.2    Helpers
//------------------------------------------------------------------------------
// the world - d_tests_tls_world returns the run's, made by
// d_tests_tls_world_open, NULL before then or after a failure; a context
// may be remade from a configuration the test adjusts
bool d_tests_tls_world_open(void);
void d_tests_tls_world_close(void);
bool d_tests_tls_context(
    struct dj_tls_context*      _out,
    const struct dj_tls_config* _config
);
void d_tests_tls_client_config(
    struct dj_tls_config* _config
);
void d_tests_tls_server_config(
    struct dj_tls_config* _config,
    const char*           _certificate,
    const char*           _key
);
struct dj_tests_tls_world* d_tests_tls_world(void);

// pairs - open makes the pipe and both sessions, the client naming `_host`;
// handshake calls both sides in turn until both are established or one
// fails, reporting the two outcomes; move sends `_size` bytes one way,
// reading them as they go, and checks they arrived whole
bool d_tests_tls_pair_open(
    struct dj_tests_tls_pair*    _pair,
    const struct dj_tls_context* _client,
    const struct dj_tls_context* _server,
    const char*                  _host
);
void d_tests_tls_pair_close(
    struct dj_tests_tls_pair* _pair
);
bool d_tests_tls_pair_handshake(
    struct dj_tests_tls_pair* _pair,
    enum dj_tls_error*        _client_status,
    enum dj_tls_error*        _server_status
);
bool d_tests_tls_pair_move(
    struct dj_tls_session* _from,
    struct dj_tls_session* _to,
    size_t                 _size
);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Contexts
//------------------------------------------------------------------------------
bool d_tests_tls_contexts(
    struct dj_test_suite* _suite
);

// 2.2    Sessions
//------------------------------------------------------------------------------
bool d_tests_tls_init(
    struct dj_test_suite* _suite
);
bool d_tests_tls_handshake(
    struct dj_test_suite* _suite
);
bool d_tests_tls_exchange(
    struct dj_test_suite* _suite
);
bool d_tests_tls_wants(
    struct dj_test_suite* _suite
);
bool d_tests_tls_resume(
    struct dj_test_suite* _suite
);
bool d_tests_tls_close_notify(
    struct dj_test_suite* _suite
);
bool d_tests_tls_truncation(
    struct dj_test_suite* _suite
);
bool d_tests_tls_transport_failure(
    struct dj_test_suite* _suite
);
bool d_tests_tls_names(
    struct dj_test_suite* _suite
);
bool d_tests_tls_trust(
    struct dj_test_suite* _suite
);
bool d_tests_tls_alpn(
    struct dj_test_suite* _suite
);
bool d_tests_tls_plaintext_first(
    struct dj_test_suite* _suite
);
bool d_tests_tls_not_tls(
    struct dj_test_suite* _suite
);

// 2.3    Sessions as connections
//------------------------------------------------------------------------------
bool d_tests_tls_view_transfers(
    struct dj_test_suite* _suite
);
bool d_tests_tls_view_ends(
    struct dj_test_suite* _suite
);
bool d_tests_tls_view_failures(
    struct dj_test_suite* _suite
);
bool d_tests_tls_view_ownership(
    struct dj_test_suite* _suite
);
bool d_tests_tls_view_shutdown(
    struct dj_test_suite* _suite
);
bool d_tests_tls_view_destroyed(
    struct dj_test_suite* _suite
);
bool d_tests_tls_view_endpoints(
    struct dj_test_suite* _suite
);
bool d_tests_tls_nested(
    struct dj_test_suite* _suite
);

// 2.4    Engines
//------------------------------------------------------------------------------
bool d_tests_tls_engine_names(
    struct dj_test_suite* _suite
);
bool d_tests_tls_engine_creates(
    struct dj_test_suite* _suite
);
bool d_tests_tls_engine_steps(
    struct dj_test_suite* _suite
);

// 2.5    The section
//------------------------------------------------------------------------------
bool d_tests_tls(
    struct dj_test_suite* _suite
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_TLS_TLS_TESTS_H
