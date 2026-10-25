/*******************************************************************************
* djinterp [net]                                                  tcp_tests_sa.h
*
* Standalone tests of the TCP transport.
*   Real sockets over loopback: every test listens on an ephemeral port or a
* unix-domain path of its own, so the suite needs no network and no fixed
* port. The support helpers open and close connected pairs; the tests that
* need a second party -- a blocked accept, bulk transfers -- run it on a
* POSIX thread. Needs the BSD sockets backend. Each test returns true when
* every assertion held.
*
*
* path:      /tests/djinterp/net/tcp/tcp_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.10.18
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  SUPPORT
    -------
    1.  Types
         1.  d_tests_tcp_case
         2.  d_tests_tcp_pair
         3.  d_tests_tcp_codes
    2.  Helpers
    3.  The platform
2.  TESTS
    -----
    1.  Connections
    2.  Listeners
    3.  Two parties
    4.  Limits and counts
    5.  The runner
*/

#ifndef DJINTERP_NET_TCP_TCP_TESTS_SA_H
#define DJINTERP_NET_TCP_TCP_TESTS_SA_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../../../inc/djinterp/net/tcp/tcp.h"  // the module
#include "../net_tests_sa_assert.h"                // asserts


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_tests_tcp_case
//   struct: a named test, as the runner's table lists it.
struct d_tests_tcp_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};

// 1.1.2
// d_tests_tcp_pair
//   struct: both ends of one loopback connection, and the listener that
// made it.
struct d_tests_tcp_pair
{
    struct d_tcp_listener   listener;
    struct d_tcp_connection client;
    struct d_tcp_connection server;
};

// 1.1.3
// d_tests_tcp_codes
//   struct: the native codes the platform gives a call that would block, a
// refused connection, and a write the connection no longer takes: EAGAIN,
// ECONNREFUSED and EPIPE, or Winsock's counterparts.
struct d_tests_tcp_codes
{
    int would_block;
    int refused;
    int reset;
};

// 1.2    Helpers
//------------------------------------------------------------------------------
// endpoints and pairs -- a pair listens on 127.0.0.1 port 0, connects, and
// accepts; opening reports false, leaving everything closed, on any failure
bool d_tests_tcp_endpoint(struct d_net_endpoint* _out,
                          const char*            _host,
                          d_net_port             _port);
bool d_tests_tcp_pair_open(struct d_tests_tcp_pair*    _pair,
                           const struct d_tcp_options* _options);
void d_tests_tcp_pair_close(struct d_tests_tcp_pair* _pair);
bool d_tests_tcp_exchange(struct d_tcp_connection* _from,
                          struct d_tcp_connection* _to,
                          const char*              _text);


// 1.3    The platform
//------------------------------------------------------------------------------
// what the tests ask of the system, written once for BSD sockets and once
// for Winsock (tcp_tests_sa_native.c); the tests call only tcp.h and these
// time -- a pause of at least the time given, a monotonic clock in
// milliseconds, and a watchdog that ends the process when time runs out
void d_tests_tcp_pause_ms(long _milliseconds);
long d_tests_tcp_now_ms(void);
void d_tests_tcp_watchdog(unsigned _seconds);

// sockets -- whether one is closed; an inheritable copy of one (a dup() of
// a BSD socket, a Winsock socket itself marked inheritable), whether one is
// inherited, and the copy dropped; TCP_NODELAY as read back; non-blocking
// mode set; and a SO_RCVTIMEO or SO_SNDTIMEO limit as read back, in
// milliseconds, or -1 when unreadable
bool         d_tests_tcp_descriptor_closed(d_tcp_socket _socket);
d_tcp_socket d_tests_tcp_inheritable(d_tcp_socket _socket);
bool         d_tests_tcp_inherited(d_tcp_socket _socket);
void         d_tests_tcp_drop(d_tcp_socket _copy);
bool         d_tests_tcp_no_delay(d_tcp_socket _socket,
                                  bool*        _on);
bool         d_tests_tcp_nonblocking(d_tcp_socket _socket);
long         d_tests_tcp_timeout_ms(d_tcp_socket _socket,
                                    bool         _receive);

// the system -- its native error codes, its name for the machine, a
// unix-domain path for this process and tag, and whether a path exists
struct d_tests_tcp_codes
     d_tests_tcp_native_codes(void);
bool d_tests_tcp_system_name(char*  _buffer,
                             size_t _capacity);
bool d_tests_tcp_unix_path(char*       _buffer,
                           size_t      _capacity,
                           const char* _tag);
bool d_tests_tcp_path_exists(const char* _path);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Connections
//------------------------------------------------------------------------------
bool d_tests_sa_tcp_options(struct d_test_counter* _counter);
bool d_tests_sa_tcp_closed(struct d_test_counter* _counter);
bool d_tests_sa_tcp_refusals(struct d_test_counter* _counter);
bool d_tests_sa_tcp_loopback(struct d_test_counter* _counter);
bool d_tests_sa_tcp_half_close(struct d_test_counter* _counter);
bool d_tests_sa_tcp_net_view(struct d_test_counter* _counter);
bool d_tests_sa_tcp_refused(struct d_test_counter* _counter);
bool d_tests_sa_tcp_names(struct d_test_counter* _counter);
bool d_tests_sa_tcp_adopt(struct d_test_counter* _counter);
bool d_tests_sa_tcp_adopt_udp(struct d_test_counter* _counter);
bool d_tests_sa_tcp_would_block(struct d_test_counter* _counter);
bool d_tests_sa_tcp_nagle(struct d_test_counter* _counter);
bool d_tests_sa_tcp_native(struct d_test_counter* _counter);

// 2.2    Listeners
//------------------------------------------------------------------------------
bool d_tests_sa_tcp_listener_errors(struct d_test_counter* _counter);
bool d_tests_sa_tcp_unix(struct d_test_counter* _counter);
bool d_tests_sa_tcp_unix_paths(struct d_test_counter* _counter);

// 2.3    Two parties
//------------------------------------------------------------------------------
bool d_tests_sa_tcp_accept_wake(struct d_test_counter* _counter);
bool d_tests_sa_tcp_bulk(struct d_test_counter* _counter);
bool d_tests_sa_tcp_frames(struct d_test_counter* _counter);
bool d_tests_sa_tcp_pump(struct d_test_counter* _counter);

// 2.4    Limits and counts
//------------------------------------------------------------------------------
bool d_tests_sa_tcp_read_timeout(struct d_test_counter* _counter);
bool d_tests_sa_tcp_limit_values(struct d_test_counter* _counter);
bool d_tests_sa_tcp_write_timeout(struct d_test_counter* _counter);
bool d_tests_sa_tcp_accept_wait(struct d_test_counter* _counter);
bool d_tests_sa_tcp_available(struct d_test_counter* _counter);
bool d_tests_sa_tcp_host_name(struct d_test_counter* _counter);

// 2.5    The runner
//------------------------------------------------------------------------------
bool d_tests_sa_tcp_run_all(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_TCP_TCP_TESTS_SA_H
