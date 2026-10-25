/*******************************************************************************
* djinterp [net]                                                 smtp_tests_sa.h
*
* djinterp SMTP kernel tests -- standalone.
*   Declarations for the standalone tests of the C SMTP kernel: the protocol
* vocabulary (smtp_tests_sa_common.c); the line reader, and client and server
* sessions, scripted over net_tests_peer.h's peer (smtp_tests_sa_reader.c,
* _client.c and _server.c); STARTTLS on both sides, and how it fails, with
* the same file's toy engine (smtp_tests_sa_tls.c); live sessions over
* loopback TCP, in plaintext and with the OpenSSL-family engine
* (smtp_tests_sa_loopback.c); what they share (smtp_tests_sa_support.c);
* and the runner (smtp_tests_sa_main.c).
*   The loopback tests need a TCP backend, and the TLS ones the engine and
* the certificates net_tests_pki.c makes; without them they pass as skipped.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Types
         1.  d_tests_smtp_case
         2.  d_tests_smtp_inbox
    2.  Helpers
2.  TESTS
    -----
    1.  Vocabulary
    2.  The line reader
    3.  Client sessions
    4.  Server sessions
    5.  STARTTLS
    6.  Over loopback TCP
    7.  The runner
*/

#ifndef DJINTERP_NET_SMTP_SMTP_TESTS_SA_H
#define DJINTERP_NET_SMTP_SMTP_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../inc/djinterp/net/smtp/smtp.h"  // the kernel
#include "../net_tests_peer.h"                       // peer, toy engine
#include "../net_tests_sa_assert.h"                  // asserts


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_tests_smtp_case
//   struct: a named test, as the runner's table lists it.
struct d_tests_smtp_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};

// 1.1.2
// d_tests_smtp_inbox
//   struct: what the test handlers of a server saw: the messages delivered,
// the last one's recipients, sender, authenticated user and data.
struct d_tests_smtp_inbox
{
    size_t messages;
    size_t recipients;
    char   sender[256];
    char   user[64];
    char   domain[256];
    char   data[4096];
};

// 1.2    Helpers
//------------------------------------------------------------------------------
// peers -- a scripted peer that says `_script` at once and hangs up after
// it; and whether a peer's output is exactly `_expected`'s text
struct d_tests_net_peer*
     d_tests_smtp_peer(const char* _script);
bool d_tests_smtp_sent(const struct d_tests_net_peer* _peer,
                       const struct d_smtp_buffer*    _expected);

// expectations -- appends `_text` to `_out`, through the toy cipher when
// `_sealed`, as the toy engine would put it on the wire
void d_tests_smtp_expect(struct d_smtp_buffer* _out,
                         const char*           _text,
                         bool                  _sealed);

// contexts -- a toy context for each role in `_contexts`, the server's
// first, which never verifies; the client's verifies when `_verify`. False,
// with neither left to destroy, when either cannot be made
bool d_tests_smtp_toy_pair(struct d_ssl_context*   _contexts,
                           struct d_tests_net_toy* _toys,
                           bool                    _verify);

// clients -- a fresh client attached to `_connection`, greeted, and past
// EHLO as "c.example"
enum d_smtp_error
     d_tests_smtp_open(struct d_smtp_client*    _client,
                       struct d_net_connection* _connection,
                       bool                     _secure);

// servers -- a handler that refuses recipients whose local part starts
// with "reject", records messages in `_inbox`, and, when `_with_auth`,
// accepts alice with s3cret; and one session over `_connection` with it
void d_tests_smtp_handler(struct d_smtp_server_handler* _handler,
                          struct d_tests_smtp_inbox*    _inbox,
                          bool                          _with_auth);
enum d_smtp_error
     d_tests_smtp_serve(struct d_net_connection*           _connection,
                        const struct d_smtp_server_config* _config,
                        struct d_tests_smtp_inbox*         _inbox,
                        bool                               _with_auth);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Vocabulary
//------------------------------------------------------------------------------
// smtp_tests_sa_common.c
bool d_tests_sa_smtp_buffer(struct d_test_counter* _counter);
bool d_tests_sa_smtp_reply(struct d_test_counter* _counter);
bool d_tests_sa_smtp_capabilities(struct d_test_counter* _counter);
bool d_tests_sa_smtp_validation(struct d_test_counter* _counter);
bool d_tests_sa_smtp_commands(struct d_test_counter* _counter);
bool d_tests_sa_smtp_base64(struct d_test_counter* _counter);
bool d_tests_sa_smtp_data(struct d_test_counter* _counter);
bool d_tests_sa_smtp_errors_to_net(struct d_test_counter* _counter);
bool d_tests_sa_smtp_errors_from_net(struct d_test_counter* _counter);
bool d_tests_sa_smtp_errors_from_ssl(struct d_test_counter* _counter);

// 2.2    The line reader
//------------------------------------------------------------------------------
// smtp_tests_sa_reader.c
bool d_tests_sa_smtp_reader_lines(struct d_test_counter* _counter);
bool d_tests_sa_smtp_reader_limits(struct d_test_counter* _counter);
bool d_tests_sa_smtp_reader_failures(struct d_test_counter* _counter);

// 2.3    Client sessions
//------------------------------------------------------------------------------
// smtp_tests_sa_client.c
bool d_tests_sa_smtp_client_script(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_cram(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_serial(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_guards(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_attach(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_attach_with(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_failures(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_replies(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_messages(struct d_test_counter* _counter);
bool d_tests_sa_smtp_client_batches(struct d_test_counter* _counter);

// 2.4    Server sessions
//------------------------------------------------------------------------------
// smtp_tests_sa_server.c
bool d_tests_sa_smtp_server_script(struct d_test_counter* _counter);
bool d_tests_sa_smtp_server_auth(struct d_test_counter* _counter);
bool d_tests_sa_smtp_server_sasl_ends(struct d_test_counter* _counter);
bool d_tests_sa_smtp_server_arguments(struct d_test_counter* _counter);
bool d_tests_sa_smtp_server_contexts(struct d_test_counter* _counter);
bool d_tests_sa_smtp_server_failures(struct d_test_counter* _counter);

// 2.5    STARTTLS
//------------------------------------------------------------------------------
// smtp_tests_sa_tls.c
bool d_tests_sa_smtp_starttls_client(struct d_test_counter* _counter);
bool d_tests_sa_smtp_starttls_unnamed(struct d_test_counter* _counter);
bool d_tests_sa_smtp_starttls_refusals(struct d_test_counter* _counter);
bool d_tests_sa_smtp_starttls_injection(struct d_test_counter* _counter);
bool d_tests_sa_smtp_starttls_failures(struct d_test_counter* _counter);
bool d_tests_sa_smtp_starttls_after(struct d_test_counter* _counter);
bool d_tests_sa_smtp_errors_from_tls(struct d_test_counter* _counter);
bool d_tests_sa_smtp_starttls_server(struct d_test_counter* _counter);
bool d_tests_sa_smtp_starttls_server_refusals(struct d_test_counter* _counter);
bool d_tests_sa_smtp_starttls_server_failures(struct d_test_counter* _counter);

// 2.6    Over loopback TCP
//------------------------------------------------------------------------------
// smtp_tests_sa_loopback.c: setup makes the certificates the TLS tests use,
// and reports whether they and the engine are there; teardown removes them
bool d_tests_sa_smtp_loopback_setup(void);
void d_tests_sa_smtp_loopback_teardown(void);
bool d_tests_sa_smtp_loopback(struct d_test_counter* _counter);
bool d_tests_sa_smtp_loopback_tls(struct d_test_counter* _counter);
bool d_tests_sa_smtp_loopback_mismatch(struct d_test_counter* _counter);
bool d_tests_sa_smtp_loopback_policy(struct d_test_counter* _counter);
bool d_tests_sa_smtp_connect_arguments(struct d_test_counter* _counter);
bool d_tests_sa_smtp_connect_failures(struct d_test_counter* _counter);
bool d_tests_sa_smtp_connect_options(struct d_test_counter* _counter);
bool d_tests_sa_smtp_identity(struct d_test_counter* _counter);

// 2.7    The runner
//------------------------------------------------------------------------------
// smtp_tests_sa_main.c's table, run in order, each reported
bool d_tests_sa_smtp_run_all(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SMTP_SMTP_TESTS_SA_H
