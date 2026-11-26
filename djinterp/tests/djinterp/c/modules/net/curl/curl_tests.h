/*******************************************************************************
* djinterp [c][net]                                                 curl_tests.h
*
* Tests of the libcurl binding, net/curl/curl.h: the HTTP version a transfer
* asks for, and the one its response came in.
*   A peer on a loopback port, on a thread of dmutex.h's, answers one
* connection each: it speaks HTTP/1.1 always, and where it is made to, HTTP/2
* too, from the first byte (prior knowledge) or after an Upgrade to h2c. It
* writes HTTP/2's frames itself, its one header block in HPACK's plainest
* forms, and reads only the frames' heads, so it needs no HTTP/2 library.
* HTTP/3 runs over QUIC, beyond such a peer: its transfer goes to a server
* the environment names (curl_tests_version.c). Every version's checks run
* where libcurl has it; where it lacks it, a request for it must fail as an
* unsupported protocol, and without libcurl a transfer as unsupported. Each
* test returns true when every check held.
*
*
* path:      /tests/djinterp/c/modules/net/curl/curl_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Types
         1.  dj_tests_curl_case
         2.  dj_tests_curl_peer
    2.  The peer
2.  TESTS
    -----
    1.  Options and refusals
    2.  The versions
    3.  The section
*/

#ifndef DJINTERP_NET_CURL_CURL_TESTS_H
#define DJINTERP_NET_CURL_CURL_TESTS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/curl/curl.h"  // module
#include "../../../../../../inc/djinterp/c/modules/net/tcp/tcp.h"  // peer
#include "../net_tests_assert.h"  // asserts
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// dj_tests_curl_case
//   struct: a named test, as the section's table lists it.
struct dj_tests_curl_case
{
    const char* name;
    bool        (*run)(struct dj_test_suite* _suite);
};

// 1.1.2
// dj_tests_curl_peer
//   struct: a loopback peer answering one connection, and what it saw.
//     listener       where it listens: 127.0.0.1, an ephemeral port.
//     url            "http://127.0.0.1:<port>/hello".
//     speaks_h2      answer HTTP/2 from the first byte, and an Upgrade to
//                    h2c with 101; without it, HTTP/1.1 alone, as an old
//                    server answers.
//     saw_preface    the connection began with HTTP/2's preface.
//     saw_upgrade    an HTTP/1 request asked to upgrade to h2c.
//     answered_h2    the answer went out as HTTP/2 frames.
//     version        an HTTP/1 request's version, "HTTP/1.0" or "HTTP/1.1";
//                    empty for none.
//     outcome        D_NET_OK when the exchange ran to the client's close.
//     running        its thread was started and not yet joined; the thread
//                    itself is curl_tests_peer.c's, one peer at a time, so
//                    this header needs no POSIX threads.
struct dj_tests_curl_peer
{
    struct dj_tcp_listener listener;
    char                   url[64];
    bool                   speaks_h2;
    bool                   saw_preface;
    bool                   saw_upgrade;
    bool                   answered_h2;
    char                   version[9];
    enum dj_net_error      outcome;
    bool                   running;
};

// 1.2    The peer
//------------------------------------------------------------------------------
// d_tests_curl_peer_start listens and starts the peer's thread, which takes
// one connection, answers it, and waits for the client's close; false, with
// nothing left open, if it couldn't. d_tests_curl_peer_finish joins the
// thread and closes the listener; it may be called on a peer that never
// started.
bool d_tests_curl_peer_start(
    struct dj_tests_curl_peer* _peer,
    bool                       _speaks_h2
);
void d_tests_curl_peer_finish(
    struct dj_tests_curl_peer* _peer
);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Options and refusals
//------------------------------------------------------------------------------
bool d_tests_curl_options_default(struct dj_test_suite* _suite);
bool d_tests_curl_version_refused(struct dj_test_suite* _suite);
bool d_tests_curl_without_http2(struct dj_test_suite* _suite);

// 2.2    The versions
//------------------------------------------------------------------------------
bool d_tests_curl_prior_knowledge(struct dj_test_suite* _suite);
bool d_tests_curl_upgrade(struct dj_test_suite* _suite);
bool d_tests_curl_upgrade_declined(struct dj_test_suite* _suite);
bool d_tests_curl_http1_only(struct dj_test_suite* _suite);
bool d_tests_curl_http1_0(struct dj_test_suite* _suite);
bool d_tests_curl_tls_only_in_clear(struct dj_test_suite* _suite);
bool d_tests_curl_default_in_clear(struct dj_test_suite* _suite);
bool d_tests_curl_prior_knowledge_refused(struct dj_test_suite* _suite);
bool d_tests_curl_without_http3(struct dj_test_suite* _suite);
bool d_tests_curl_http3_server(struct dj_test_suite* _suite);
bool d_tests_curl_http3_in_clear(struct dj_test_suite* _suite);

// 2.3    The section
//------------------------------------------------------------------------------
// d_tests_curl - the suite's section: opens module curl, a test per case
bool d_tests_curl(
    struct dj_test_suite* _suite
);


D_EXTERN_C_END


#endif  // DJINTERP_NET_CURL_CURL_TESTS_H
