/*******************************************************************************
* djinterp [c/net]                                           starttls_tests_sa.h
*
* Standalone tests of the STARTTLS common module.
*   Most tests run a session against net_tests_peer.h's scripted peer: a
* net.h connection that releases each of its turns once the session has
* sent the text the turn waits for, so a test reads as the exchange it
* checks and needs no thread. Behind the peer, the same file's toy engine
* of the SSL kernel stands in for a real one: it shakes hands with nine
* bytes each way, shaped as TLS record headers so the kernel takes them for
* TLS, and seals data by rotating letters and digits, so sealed text stays
* readable. Sessions over TCP are tested over
* socket pairs that tcp.c adopts, and the OpenSSL-family engine, where it is
* compiled, over a socket pair with a server thread and the certificates
* net_tests_pki.c makes for the run. Each test returns true when every
* assertion held.
*
*
* path:      /tests/djinterp/c/modules/net/starttls/starttls_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Constants
         1.  D_TESTS_STARTTLS_UNREACHED
    2.  Types
         1.  dj_tests_starttls_case
         2.  dj_tests_starttls_smtp
         3.  d_tests_starttls_forget
         4.  dj_tests_starttls_fixture
    3.  Helpers
2.  TESTS
    -----
    1.  Lifecycle and queries
    2.  Line and byte I/O
    3.  Negotiation and upgrade
    4.  Once secure
    5.  Over TCP
    6.  OpenSSL-family engine
    7.  The runner
*/

#ifndef DJINTERP_NET_STARTTLS_STARTTLS_TESTS_SA_H
#define DJINTERP_NET_STARTTLS_STARTTLS_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/ssl/ssl.h"            // the kernel
#include "../../../../../../inc/djinterp/c/modules/net/starttls/starttls.h"  // the module
#include "../../../../../../inc/djinterp/c/modules/net/tcp/tcp.h"            // TCP
#include "../net_tests_peer.h"                               // peer, toy
#include "../net_tests_sa_assert.h"                          // asserts


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_TESTS_STARTTLS_UNREACHED
//   constant: a status no call returns, for a step a test never reached.
#define D_TESTS_STARTTLS_UNREACHED ((enum dj_starttls_error)99)

// 1.2    Types
//------------------------------------------------------------------------------
// 1.2.1
// dj_tests_starttls_case
//   struct: a named test, as the runner's table lists it.
struct dj_tests_starttls_case
{
    const char* name;
    bool        (*run)(struct dj_test_counter* _counter);
};

// 1.2.2
// dj_tests_starttls_smtp
//   struct: an SMTP-like negotiator's context. The request is STARTTLS; a
// reply of 220 proceeds, 454 refuses, anything else is malformed. `exact`
// reads the reply as that many bytes instead of as a line, leaving whatever
// follows in the connection.
struct dj_tests_starttls_smtp
{
    enum dj_starttls_error request_status;  // a failure to report instead
    enum dj_starttls_error reply_status;    // a failure to report instead
    size_t                exact;           // bytes to read; 0 for a line
    int                   requests;        // calls to request
    int                   replies;         // calls to reply
    char                  reply[128];      // the reply read
};

// 1.2.3
// d_tests_starttls_forget
//   struct: what an on_upgrade callback saw.
struct dj_tests_starttls_forget
{
    int                        calls;    // times it ran
    struct dj_starttls_session* session;  // the session it was given
    bool                       secure;   // whether it was already secure
    unsigned int               epoch;    // its epoch then
};

// 1.2.4
// dj_tests_starttls_fixture
//   struct: what most scripted tests start from: a peer, a toy context in
// the session's role that verifies as a client, the countable connection of
// the peer, and options for that role, which own the connection. It must
// not move once opened: the context points into it.
struct dj_tests_starttls_fixture
{
    struct dj_tests_net_peer*  peer;        // the other party
    struct dj_tests_net_toy    toy;         // the engine's record
    struct dj_ssl_context      tls;         // the context over `toy`
    struct dj_net_connection*  connection;  // the peer's
    struct dj_starttls_options options;     // for the role, using `tls`
};

// 1.3    Helpers
//------------------------------------------------------------------------------
// negotiation -- the SMTP-like negotiator, and an on_upgrade that records
extern const struct dj_starttls_negotiator d_tests_starttls_smtp_negotiator;
void d_tests_starttls_forget(
    struct dj_starttls_session* _session,
    void*                       _context
);

// sessions -- options for a scripted session: the module's defaults with
// the role, the policy, the context, and for a client the server name; a
// plaintext client over a peer that may never upgrade, with a buffer of
// `_buffer_bytes` (0 for the default); and whether the next line received
// is `_expected`
struct dj_starttls_options d_tests_starttls_options(
    enum dj_starttls_role        _role,
    enum dj_starttls_policy      _policy,
    const struct dj_ssl_context* _tls
);
bool d_tests_starttls_plain(
    struct dj_starttls_session* _session,
    struct dj_tests_net_peer*   _peer,
    size_t                      _buffer_bytes,
    bool                        _strict_crlf
);
bool d_tests_starttls_line_is(
    struct dj_starttls_session* _session,
    const char*                 _expected
);

// fixtures -- opened for a role and a policy, and closed again
bool d_tests_starttls_fixture_open(
    struct dj_tests_starttls_fixture* _fixture,
    enum dj_starttls_role             _role,
    enum dj_starttls_policy           _policy
);
void d_tests_starttls_fixture_close(
    struct dj_tests_starttls_fixture* _fixture
);

// sockets -- a connected AF_UNIX stream pair with five-second deadlines;
// whether a descriptor is closed; and a TCP connection adopting a socket,
// with five-second I/O deadlines, the socket closed if adopting fails
bool d_tests_starttls_socket_pair(
    int _sockets[2]
);
bool d_tests_starttls_is_closed(
    int _socket
);
bool d_tests_starttls_adopt(
    struct dj_tcp_connection* _connection,
    int                       _socket
);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Lifecycle and queries
//------------------------------------------------------------------------------
bool d_tests_sa_starttls_defaults(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_init_arguments(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_init_values(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_init_contexts(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_new_session(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_close(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_ownership(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_null_queries(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_names(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_errors_to_net(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_errors_from_net(
    struct dj_test_counter* _counter
);

// 2.2    Line and byte I/O
//------------------------------------------------------------------------------
bool d_tests_sa_starttls_lines(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_lines_trickled(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_line_endings(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_failed_session(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_line_limit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_literal(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_byte_refusals(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_stream_ends(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_send_line(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_send_limit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_send_long(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_broken_writes(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_broken_reads(
    struct dj_test_counter* _counter
);

// 2.3    Negotiation and upgrade
//------------------------------------------------------------------------------
bool d_tests_sa_starttls_policy_offer(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_policy_reply(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_policy_callbacks(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_negotiate_refusals(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_upgrade_client(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_secure_traffic(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_upgrade_server(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_upgrade_implicit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_injection_client(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_injection_server(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_leftover_dropped(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_engine_failures(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_failure_details(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_upgrade_disabled(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_secure_end(
    struct dj_test_counter* _counter
);

// 2.4    Once secure
//------------------------------------------------------------------------------
bool d_tests_sa_starttls_secure_failures(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_optional_client(
    struct dj_test_counter* _counter
);

// 2.5    Over TCP
//------------------------------------------------------------------------------
bool d_tests_sa_starttls_tcp_exchange(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_tcp_ownership(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_tcp_deadline(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_tcp_vanished(
    struct dj_test_counter* _counter
);

// 2.6    OpenSSL-family engine
//------------------------------------------------------------------------------
// setup makes the run's certificates and reports whether the engine and
// they are there to test with; teardown removes them
bool d_tests_sa_starttls_openssl_setup(void);
void d_tests_sa_starttls_openssl_teardown(void);
bool d_tests_sa_starttls_openssl_starttls(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_openssl_implicit(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_openssl_verify(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_openssl_unverified(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_openssl_truncation(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_openssl_injection(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_openssl_pipelined(
    struct dj_test_counter* _counter
);
bool d_tests_sa_starttls_openssl_client_cert(
    struct dj_test_counter* _counter
);

// 2.7    The runner
//------------------------------------------------------------------------------
bool d_tests_sa_starttls_run_all(
    struct dj_test_counter* _counter
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_STARTTLS_STARTTLS_TESTS_SA_H
