/*******************************************************************************
* djinterp [net]                                          ssh_tests_sa_channel.c
*
* Tests of what a channel asks of its engine, and keeps of its failures.
*   A read must bring at least a byte and no more than fit, or a read of
* nothing could pass for the stream's end; a fatal status fails the channel
* and is kept, from whichever call it came; and a read the channel cannot
* make is refused before the engine is asked.
*
*
* path:      /tests/djinterp/net/ssh/ssh_tests_sa_channel.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./ssh_tests_sa.h"  // corresponding header


/*
d_tests_ssh_lied_to
  Opens a fixture whose engine's next read claims success with `_claims`
bytes and reads none, then reads into eight bytes: whether that read was a
stalled transport that failed the channel and the session, with nothing
counted.
*/
static bool
d_tests_ssh_lied_to(
    size_t _claims
)
{
    struct d_tests_ssh_fixture fixture  = { .channel.exit_status = 0 };
    unsigned char              read[8]  = { 0 };
    size_t                     received = 1u;
    const bool                 open     = d_tests_ssh_fixture_open(&fixture);

    d_tests_ssh_fixture_toy(&fixture)->lies   = true;
    d_tests_ssh_fixture_toy(&fixture)->claims = _claims;

    const bool stalled =
        ( (open)                                                   &&
          (d_ssh_channel_read(&fixture.channel,
                              D_SSH_STREAM_STDOUT,
                              read,
                              sizeof(read),
                              &received) == D_SSH_ERROR_TRANSPORT) &&
          (received == 0u)                                         &&
          (fixture.channel.state == D_SSH_CHANNEL_FAILED)          &&
          (fixture.channel.failure == D_SSH_ERROR_TRANSPORT)       &&
          (d_ssh_get_state(&fixture.session) == D_SSH_STATE_FAILED) );

    d_tests_ssh_fixture_close(&fixture);

    return stalled;
}

/*
d_tests_sa_ssh_read_promise
  Tests the following:
  - an engine read that claims success with no byte is a stalled transport:
    it fails the channel, keeping the status, and the session
  - so is one that claims more than there was room for, with nothing
    counted
  - through the view such a read is a failure, never the stream's end
*/
bool
d_tests_sa_ssh_read_promise(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_fixture fixture = { .channel.exit_status = 0 };
    unsigned char              read[8] = { 0 };
    bool                       result  = true;

    result = d_tests_net_assert(d_tests_ssh_lied_to(0u),
                                "read promise",
                                "success with no byte is a stalled transport",
                                _counter) && result;
    result = d_tests_net_assert(d_tests_ssh_lied_to(sizeof(read) + 1u),
                                "read promise",
                                "success with more than fits is one too",
                                _counter) && result;

    const bool open = d_tests_ssh_fixture_open(&fixture);

    d_tests_ssh_fixture_toy(&fixture)->lies = true;

    const bool failed =
        ( (open)                                &&
          (d_tests_ssh_is(d_net_connection_read(&fixture.channel.base,
                                                read,
                                                sizeof(read)),
                          0u,
                          D_NET_ERROR_UNKNOWN)) &&
          (fixture.channel.failure == D_SSH_ERROR_TRANSPORT) );

    d_tests_ssh_fixture_close(&fixture);

    result = d_tests_net_assert(failed,
                                "read promise",
                                "through the view it is no end of stream",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_ssh_failure_kept
  Tests the following:
  - a fatal status fails the channel and is kept in `failure`, whichever
    call it came from: a request's, an EOF's
  - a channel freed starts again with nothing kept
*/
bool
d_tests_sa_ssh_failure_kept(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_fixture fixture = { .channel.exit_status = 0 };
    struct d_ssh_channel       second  = { .exit_status = 0 };
    bool                       result  = true;

    // a request refused on a second channel, then an EOF on the first
    const bool open = d_tests_ssh_fixture_open(&fixture);

    fixture.toy.channels[1].request_fails = D_SSH_ERROR_PROTOCOL;

    const bool fatal =
        ( (open)                                                    &&
          (d_ssh_channel_open(&fixture.session,
                              &second) == D_SSH_OK)                 &&
          (d_ssh_channel_subsystem(&second,
                                   "sftp") == D_SSH_ERROR_PROTOCOL) &&
          (second.state == D_SSH_CHANNEL_FAILED)                    &&
          (second.failure == D_SSH_ERROR_PROTOCOL)                  &&
          (fixture.channel.state == D_SSH_CHANNEL_OPEN)             &&
          (fixture.channel.failure == D_SSH_OK) );

    d_ssh_channel_free(&second);

    const bool freed = ( (second.state == D_SSH_CHANNEL_CLOSED) &&
                         (second.failure == D_SSH_OK) );

    d_tests_ssh_fixture_close(&fixture);

    const bool reopened = d_tests_ssh_fixture_open(&fixture);

    d_tests_ssh_fixture_toy(&fixture)->eof_fails = D_SSH_ERROR_TIMED_OUT;

    const bool eof =
        ( (reopened)                                      &&
          (d_ssh_channel_send_eof(&fixture.channel) ==
           D_SSH_ERROR_TIMED_OUT)                         &&
          (fixture.channel.state == D_SSH_CHANNEL_FAILED) &&
          (fixture.channel.failure == D_SSH_ERROR_TIMED_OUT) );

    d_tests_ssh_fixture_close(&fixture);

    result = d_tests_net_assert(fatal,
                                "failure kept",
                                "a refused request fails its channel alone",
                                _counter) && result;
    result = d_tests_net_assert(freed,
                                "failure kept",
                                "a freed channel keeps nothing",
                                _counter) && result;
    result = d_tests_net_assert(eof,
                                "failure kept",
                                "a refused EOF fails the channel, kept",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_ssh_failure_mild
  Tests the following:
  - a status that is not fatal leaves the channel open, its session
    authenticated, and `failure` as it was, from a read or a request alike
*/
bool
d_tests_sa_ssh_failure_mild(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_fixture fixture  = { .channel.exit_status = 0 };
    struct d_ssh_channel       second   = { .exit_status = 0 };
    unsigned char              read[8]  = { 0 };
    size_t                     received = 0u;
    const bool                 again    = d_tests_ssh_fixture_open(&fixture);

    d_tests_ssh_fixture_toy(&fixture)->read_fails = D_SSH_ERROR_CHANNEL;
    fixture.toy.channels[1].request_fails         = D_SSH_ERROR_CHANNEL;

    const bool mild =
        ( (again)                                                  &&
          (d_ssh_channel_read(&fixture.channel,
                              D_SSH_STREAM_STDOUT,
                              read,
                              sizeof(read),
                              &received) == D_SSH_ERROR_CHANNEL)   &&
          (fixture.channel.state == D_SSH_CHANNEL_OPEN)            &&
          (fixture.channel.failure == D_SSH_OK)                    &&
          (d_ssh_channel_open(&fixture.session,
                              &second) == D_SSH_OK)                &&
          (d_ssh_channel_subsystem(&second,
                                   "sftp") == D_SSH_ERROR_CHANNEL) &&
          (second.state == D_SSH_CHANNEL_OPEN)                     &&
          (second.failure == D_SSH_OK)                             &&
          (d_ssh_get_state(&fixture.session) ==
           D_SSH_STATE_AUTHENTICATED) );

    d_ssh_channel_free(&second);
    d_tests_ssh_fixture_close(&fixture);

    return d_tests_net_assert(mild,
                              "failure kept",
                              "a status that is not fatal is not kept",
                              _counter);
}

/*
d_tests_ssh_refused_read
  Reads as told with the count's storage first set to a value no read
gives: whether the read was refused with `_expected` and the count, where
there was one, was zeroed.
*/
static bool
d_tests_ssh_refused_read(
    struct d_ssh_channel* _channel,
    void*                 _buffer,
    size_t                _capacity,
    enum d_ssh_stream     _stream,
    enum d_ssh_error      _expected
)
{
    size_t received = 99u;

    return ( (d_ssh_channel_read(_channel,
                                 _stream,
                                 _buffer,
                                 _capacity,
                                 &received) == _expected) &&
             (received == 0u) );
}

/*
d_tests_sa_ssh_read_arguments
  Tests the following:
  - a read without a channel, a buffer, or room, or of a stream that is
    neither, is refused before the engine is asked, its count zeroed
  - so is one with nowhere to put its count
  - a read of a channel that has closed is refused as a state, its count
    zeroed too
*/
bool
d_tests_sa_ssh_read_arguments(
    struct d_test_counter* _counter
)
{
    struct d_tests_ssh_fixture  fixture = { .channel.exit_status = 0 };
    unsigned char               read[8] = { 0 };
    const bool                  open    = d_tests_ssh_fixture_open(&fixture);
    struct d_ssh_channel* const channel = &fixture.channel;

    const bool refused =
        ( (open)                                                     &&
          (d_tests_ssh_refused_read(NULL,
                                    read,
                                    sizeof(read),
                                    D_SSH_STREAM_STDOUT,
                                    D_SSH_ERROR_INVALID_ARGUMENT))   &&
          (d_tests_ssh_refused_read(channel,
                                    NULL,
                                    sizeof(read),
                                    D_SSH_STREAM_STDOUT,
                                    D_SSH_ERROR_INVALID_ARGUMENT))   &&
          (d_tests_ssh_refused_read(channel,
                                    read,
                                    0u,
                                    D_SSH_STREAM_STDOUT,
                                    D_SSH_ERROR_INVALID_ARGUMENT))   &&
          (d_tests_ssh_refused_read(channel,
                                    read,
                                    sizeof(read),
                                    (enum d_ssh_stream)2,
                                    D_SSH_ERROR_INVALID_ARGUMENT))   &&
          (d_ssh_channel_read(channel,
                              D_SSH_STREAM_STDOUT,
                              read,
                              sizeof(read),
                              NULL) == D_SSH_ERROR_INVALID_ARGUMENT) &&
          (d_tests_ssh_fixture_toy(&fixture)->reads == 0) );

    (void)d_ssh_channel_close(channel);

    const bool closed =
        ( (refused)                                     &&
          (d_tests_ssh_refused_read(channel,
                                    read,
                                    sizeof(read),
                                    D_SSH_STREAM_STDOUT,
                                    D_SSH_ERROR_STATE)) &&
          (d_tests_ssh_fixture_toy(&fixture)->reads == 0) );

    d_tests_ssh_fixture_close(&fixture);

    return d_tests_net_assert(closed,
                              "read arguments",
                              "reads are refused before the engine is asked",
                              _counter);
}
