/*******************************************************************************
* djinterp [c][net]                                                 lmtp_tests.h
*
* djinterp LMTP tests, in DTest's C face.
*   Declarations for the tests of net/lmtp and of the SMTP kernel's LMTP
* dialect: the client over net_tests_peer.h's scripted peer - LHLO, no
* HELO, a reply for each recipient, a server that hangs up partway, the
* ports refused - and SMTP's deliveries, which the same report carries
* (lmtp_tests_client.c); the server, its peer scripted as the client -
* LHLO, EHLO and HELO refused, 503, a verdict for each recipient or one for
* all (lmtp_tests_server.c); the client and the server together over
* loopback TCP and a unix-domain socket, and an LMTP client against an SMTP
* server (lmtp_tests_loopback.c); a real LMTP server the environment
* names, such as Dovecot's (lmtp_tests_live.c); what they share
* (lmtp_tests_support.c); and the section (lmtp_tests.c).
*   The loopback tests need a TCP backend, and the unix-domain one a
* platform that has them; the live one, a server named in
* DJINTERP_TEST_LMTP_SOCKET, or DJINTERP_TEST_LMTP_HOST and _PORT, with
* DJINTERP_TEST_LMTP_ACCEPTS naming a mailbox that takes mail. Without them
* they pass as skipped.
*
*
* path:      /tests/djinterp/c/modules/net/lmtp/lmtp_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Types
         1.  dj_tests_lmtp_case
         2.  dj_tests_lmtp_mailboxes
    2.  Helpers
2.  TESTS
    -----
    1.  Client sessions
    2.  Server sessions
    3.  Over loopback
    4.  Against a real server
    5.  The section
*/

#ifndef DJINTERP_NET_LMTP_LMTP_TESTS_H
#define DJINTERP_NET_LMTP_LMTP_TESTS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/lmtp/lmtp.h"  // LMTP
#include "../net_tests_peer.h"                                      // the peer
#include "../net_tests_assert.h"                                    // asserts
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// dj_tests_lmtp_case
//   struct: a named test, as the section's table lists it.
struct dj_tests_lmtp_case
{
    const char* name;
    bool        (*run)(struct dj_test_suite* _suite);
};

// D_TESTS_LMTP_MAILBOXES
//   constant: the most recipients a test server records.
#define D_TESTS_LMTP_MAILBOXES  8u

// 1.1.2
// dj_tests_lmtp_mailboxes
//   struct: what a test server's handler saw: the domain the client named,
// each delivery's recipient and index, in the order they came, and the last
// message's data; `messages` counts calls to `message`.
struct dj_tests_lmtp_mailboxes
{
    size_t deliveries;
    size_t messages;
    size_t index[D_TESTS_LMTP_MAILBOXES];
    char   recipient[D_TESTS_LMTP_MAILBOXES][64];
    char   domain[256];
    char   data[1024];
};

// 1.2    Helpers
//------------------------------------------------------------------------------
// peers - a scripted peer that says `_script` at once and hangs up after
// it, or NULL when it cannot be made
struct dj_tests_net_peer* d_tests_lmtp_peer(
    const char* _script
);

// clients - a fresh LMTP client attached to `_connection`, greeted, and
// past LHLO as "c.example"
enum dj_smtp_error d_tests_lmtp_open(
    struct dj_lmtp_client*    _client,
    struct dj_net_connection* _connection
);

// servers - a handler that refuses recipients whose local part starts with
// "reject" (550), and, with `_each`, delivers each recipient, refusing those
// whose local part starts with "full" (452), or else takes each message once
// and refuses one whose data holds "refuse" (554); all of it recorded in
// `_mailboxes`
void d_tests_lmtp_handler(
    struct dj_smtp_server_handler*  _handler,
    struct dj_tests_lmtp_mailboxes* _mailboxes,
    bool                            _each
);

// the environment - a variable's value in `_out`, false when it is unset,
// empty, or longer than `_capacity` allows
bool d_tests_lmtp_setting(
    const char* _name,
    char*       _out,
    size_t      _capacity
);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Client sessions
//------------------------------------------------------------------------------
// lmtp_tests_client.c
bool d_tests_lmtp_client_pipelined(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_client_refused(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_client_hang_up(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_client_no_helo(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_client_serial(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_client_data(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_client_guards(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_smtp_deliveries(
    struct dj_test_suite* _suite
);

// 2.2    Server sessions
//------------------------------------------------------------------------------
// lmtp_tests_server.c
bool d_tests_lmtp_server_each(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_server_hellos(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_server_no_recipient(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_server_one_verdict(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_server_oversize(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_server_smtp(
    struct dj_test_suite* _suite
);

// 2.3    Over loopback
//------------------------------------------------------------------------------
// lmtp_tests_loopback.c
bool d_tests_lmtp_loopback_tcp(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_loopback_unix(
    struct dj_test_suite* _suite
);
bool d_tests_lmtp_loopback_smtp_server(
    struct dj_test_suite* _suite
);

// 2.4    Against a real server
//------------------------------------------------------------------------------
// lmtp_tests_live.c
bool d_tests_lmtp_live(
    struct dj_test_suite* _suite
);

// 2.5    The section
//------------------------------------------------------------------------------
// lmtp_tests.c - opens the lmtp module and runs every test in a test of its
// own; true when all held
bool d_tests_lmtp(
    struct dj_test_suite* _suite
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_LMTP_LMTP_TESTS_H
