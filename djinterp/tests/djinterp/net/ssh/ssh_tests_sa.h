/*******************************************************************************
* djinterp [net]                                                  ssh_tests_sa.h
*
* Standalone tests of SSH channels, and of a channel as a net.h connection.
*   Each test runs a real session -- options, connect, authenticate -- over
* the toy SSH engine of net_tests_ssh_toy.h, which needs no SSH library and
* no server: a channel reads what the test scripted and keeps what it is
* sent, and each engine failure is placed exactly where a test asks for it.
* The channel tests hold the engine to its promises and check what a failed
* channel keeps; the view tests reach the channel only through its base, as
* code written for a socket would. Each test returns true when every
* assertion held.
*
*
* path:      /tests/djinterp/net/ssh/ssh_tests_sa.h
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
         1.  d_tests_ssh_case
         2.  d_tests_ssh_fixture
    2.  Helpers
2.  TESTS
    -----
    1.  Channels
    2.  Channels as connections
    3.  The runner
*/

#ifndef DJINTERP_NET_SSH_SSH_TESTS_SA_H
#define DJINTERP_NET_SSH_SSH_TESTS_SA_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../../inc/djinterp/net/net_common.h"       // d_net_connection
#include "../../../../inc/djinterp/net/ssh/ssh_channel.h"  // d_ssh_channel
#include "../../../../inc/djinterp/net/ssh/ssh_session.h"  // d_ssh_session
#include "../net_tests_sa_assert.h"                        // asserts
#include "../net_tests_ssh_toy.h"                          // the toy engine


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  SUPPORT
//==============================================================================


// 1.1    Types
//------------------------------------------------------------------------------
// 1.1.1
// d_tests_ssh_case
//   struct: a named test, as the runner's table lists it.
struct d_tests_ssh_case
{
    const char* name;
    bool        (*run)(struct d_test_counter* _counter);
};

// 1.1.2
// d_tests_ssh_fixture
//   struct: an authenticated session over a toy, and one channel of it
// running a command. It must not move once opened: the session's engine
// handle is the toy, and the channel's base points into the channel.
struct d_tests_ssh_fixture
{
    struct d_tests_ssh_toy toy;
    struct d_ssh_session   session;
    struct d_ssh_channel   channel;
};

// 1.2    Helpers
//------------------------------------------------------------------------------
// the fixture -- open connects the session and opens the channel, false if
// either failed; close frees the channel and ends the session, whatever
// state either is in; toy is the toy channel behind the fixture's channel;
// is says whether a result moved `_count` bytes and reported `_error`
bool d_tests_ssh_fixture_open(struct d_tests_ssh_fixture* _fixture);
void d_tests_ssh_fixture_close(struct d_tests_ssh_fixture* _fixture);
struct d_tests_ssh_toy_channel*
     d_tests_ssh_fixture_toy(struct d_tests_ssh_fixture* _fixture);
bool d_tests_ssh_is(struct d_net_io_result _result,
                    size_t                 _count,
                    enum d_net_error       _error);


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Channels
//------------------------------------------------------------------------------
bool d_tests_sa_ssh_read_promise(struct d_test_counter* _counter);
bool d_tests_sa_ssh_failure_kept(struct d_test_counter* _counter);
bool d_tests_sa_ssh_failure_mild(struct d_test_counter* _counter);
bool d_tests_sa_ssh_read_arguments(struct d_test_counter* _counter);

// 2.2    Channels as connections
//------------------------------------------------------------------------------
bool d_tests_sa_ssh_view_transfers(struct d_test_counter* _counter);
bool d_tests_sa_ssh_view_ends(struct d_test_counter* _counter);
bool d_tests_sa_ssh_view_closing(struct d_test_counter* _counter);
bool d_tests_sa_ssh_view_failures(struct d_test_counter* _counter);
bool d_tests_sa_ssh_view_elsewhere(struct d_test_counter* _counter);
bool d_tests_sa_ssh_view_unopened(struct d_test_counter* _counter);

// 2.3    The runner
//------------------------------------------------------------------------------
bool d_tests_sa_ssh_run_all(struct d_test_counter* _counter);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_TESTS_SA_H
