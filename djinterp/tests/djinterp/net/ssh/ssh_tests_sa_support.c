/*******************************************************************************
* djinterp [net]                                          ssh_tests_sa_support.c
*
* The SSH suite's fixture: a session over the toy engine with one channel.
*
*
* path:      /tests/djinterp/net/ssh/ssh_tests_sa_support.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./ssh_tests_sa.h"  // corresponding header
// std
#include <string.h>  // memset


/*
d_tests_ssh_fixture_open
  Zeroes the fixture first, so that closing it is safe however far opening
got: a zeroed session and channel end harmlessly.
*/
bool
d_tests_ssh_fixture_open(
    struct d_tests_ssh_fixture* _fixture
)
{
    memset(_fixture,
           0,
           sizeof(*_fixture));
    d_tests_ssh_toy_init(&_fixture->toy);

    return ( (d_tests_ssh_toy_connect(&_fixture->toy,
                                      &_fixture->session))        &&
             (d_ssh_channel_open(&_fixture->session,
                                 &_fixture->channel) == D_SSH_OK) &&
             (d_ssh_channel_exec(&_fixture->channel,
                                 "cat") == D_SSH_OK) );
}

/*
d_tests_ssh_fixture_close
  The channel before the session, as a program would.
*/
void
d_tests_ssh_fixture_close(
    struct d_tests_ssh_fixture* _fixture
)
{
    d_ssh_channel_free(&_fixture->channel);
    (void)d_ssh_disconnect(&_fixture->session,
                           NULL);
    d_ssh_destroy(&_fixture->session);

    return;
}

/*
d_tests_ssh_fixture_toy
  The fixture's channel is the first its session opened.
*/
struct d_tests_ssh_toy_channel*
d_tests_ssh_fixture_toy(
    struct d_tests_ssh_fixture* _fixture
)
{
    return &_fixture->toy.channels[0];
}

/*
d_tests_ssh_is
  Whether a result moved `_count` bytes and reported `_error`.
*/
bool
d_tests_ssh_is(
    struct d_net_io_result _result,
    size_t                 _count,
    enum d_net_error       _error
)
{
    return ( (_result.count == _count) &&
             (_result.error == _error) );
}
