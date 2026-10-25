/*******************************************************************************
* djinterp [net]                                              reactor_tests.cpp
*
* Tests of net/reactor.hpp.
*   The reactor dispatches over a pipe; then an event_server echoes on a
* loopback port, its loop on a thread of dmutex.h's, while blocking clients of
* tcp.hpp's talk to it: a short exchange, a megabyte that must queue behind a
* full socket, and a goodbye the server answers before it closes. The suite
* runs from C++98. Link net.c, tcp.c and dmutex.c.
*
*
* path:      /tests/djinterp/net/reactor_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./reactor_tests.hpp"  // corresponding header


#if D_ENV_NET_HAS_EPOLL

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <string>   // std::string
#include <vector>   // std::vector
// posix
#include <unistd.h>  // pipe, write, close
// djinterp
#include "../../../inc/djinterp/c/sync/dmutex.h"  // d_thread_create


NS_DJINTERP
NS_TESTING

namespace net = ::djinterp::net;

NS_INTERNAL

    // tally
    //   struct: checks run and failed across the suite.
    struct tally
    {
        tally()
            : checks(0u),
              failures(0u)
        {}

        std::size_t checks;
        std::size_t failures;
    };

    // counts
    //   function: the suite's tally.
    static tally&
    counts() D_NOEXCEPT
    {
        static tally instance;

        return instance;
    }

    // check
    //   function: records one check, printing it if it failed.
    static bool
    check(
        bool        _condition,
        const char* _message
    ) D_NOEXCEPT
    {
        counts().checks += 1u;

        // only failures are worth a line
        if (!_condition)
        {
            counts().failures += 1u;
            std::printf("    FAIL %s\n",
                        _message);
        }

        return _condition;
    }

    // remover
    //   struct: a reactor callback that counts its calls, records the mask,
    // and removes its own descriptor -- a removal the reactor must defer.
    struct remover
    {
        remover(
            net::reactor*      _reactor,
            int*               _calls,
            net::event_mask::value* _seen
        )
            : owner(_reactor),
              calls(_calls),
              seen(_seen)
        {}

        void
        operator()(
            int                    _fd,
            net::event_mask::value _ready
        ) const
        {
            *calls += 1;
            *seen   = _ready;
            (void)owner->remove(_fd);

            return;
        }

        net::reactor*           owner;
        int*                    calls;
        net::event_mask::value* seen;
    };

    // echo
    //   struct: the server's three callbacks: connections and closes are
    // counted, data is sent straight back, and "bye" is answered and closes.
    struct echo_counts
    {
        echo_counts()
            : connects(0),
              closes(0),
              received(0u)
        {}

        int         connects;
        int         closes;
        std::size_t received;
    };

    struct on_connect
    {
        void
        operator()(
            net::event_connection& _connection
        ) const
        {
            (void)_connection;
            tallied->connects += 1;

            return;
        }

        echo_counts* tallied;
    };

    struct on_data
    {
        void
        operator()(
            net::event_connection& _connection,
            const net::byte*       _data,
            std::size_t            _size
        ) const
        {
            const std::string chunk(reinterpret_cast<const char*>(_data),
                                    _size);

            tallied->received += _size;

            // a goodbye is answered, then the connection closes gracefully
            if (chunk == "bye")
            {
                (void)_connection.send(std::string("farewell"));
                _connection.close();

                return;
            }

            (void)_connection.send(_data,
                                   _size);

            return;
        }

        echo_counts* tallied;
    };

    struct on_close
    {
        void
        operator()(
            net::event_connection& _connection
        ) const
        {
            (void)_connection;
            tallied->closes += 1;

            return;
        }

        echo_counts* tallied;
    };

    // running
    //   struct: an event_server and its loop's thread.
    struct running
    {
        running()
            : server(),
              tallied(),
              outcome(net::io_error::unknown),
              thread(),
              started(false)
        {}

        net::event_server    server;
        echo_counts          tallied;
        net::io_error::value outcome;
        d_thread_t           thread;
        bool                 started;
    };

    // run_main
    //   function: the server loop's thread.
    static d_thread_result_t
    run_main(
        void* _running
    )
    {
        running* const                   work = static_cast<running*>(_running);
        net::event_server::callbacks     calls;
        const on_connect                 connected = { &work->tallied };
        const on_data                    data      = { &work->tallied };
        const on_close                   closed    = { &work->tallied };

        calls.on_connect = connected;
        calls.on_data    = data;
        calls.on_close   = closed;
        work->outcome    = work->server.run(calls);

        return D_THREAD_SUCCESS;
    }

    // pause_ms
    //   function: sleeps the calling thread.
    static void
    pause_ms(
        long _milliseconds
    )
    {
        struct timespec span;

        span.tv_sec  = static_cast<time_t>(_milliseconds / 1000L);
        span.tv_nsec = (_milliseconds % 1000L) * 1000000L;
        (void)::d_thread_sleep(&span,
                               D_NULLPTR);

        return;
    }

NS_END  // internal


/*
tests_reactor_mask
  Tests the following:
  - event masks combine, intersect, and are tested by flag
*/
bool
tests_reactor_mask()
{
    net::event_mask::value mask   = net::event_mask::readable;
    bool                   result = true;

    mask |= net::event_mask::hangup;
    result = internal::check( (net::has_event(mask,
                                              net::event_mask::readable))  &&
                              (net::has_event(mask,
                                              net::event_mask::hangup))    &&
                              (!net::has_event(mask,
                                               net::event_mask::writable)) &&
                              (net::any_event(mask))                       &&
                              (!net::any_event(net::event_mask::none))     &&
                              ((mask & net::event_mask::readable) ==
                               net::event_mask::readable),
                              "masks combine and test") && result;

    return result;
}

/*
tests_reactor_dispatch
  Tests the following:
  - a readable pipe's callback runs once, sees it readable, and can remove
    its own descriptor during the dispatch; the removal then takes effect
  - with nothing ready, run_once returns at its timeout
  - a descriptor that is not registered cannot be modified or removed
*/
bool
tests_reactor_dispatch()
{
    net::reactor           loop;
    int                    ends[2] = { -1, -1 };
    int                    calls   = 0;
    net::event_mask::value seen    = net::event_mask::none;
    bool                   result  = true;

    const bool piped = (::pipe(ends) == 0);
    const bool added =
        ( (piped) &&
          (loop.add(ends[0],
                    net::event_mask::readable,
                    net::reactor::callback(internal::remover(&loop,
                                                             &calls,
                                                             &seen))) ==
           net::io_error::none) );
    const ssize_t written = (piped) ? ::write(ends[1],
                                              "x",
                                              1u)
                                    : -1;
    const bool    first   = (loop.run_once(1000) == net::io_error::none);
    const bool    quiet   = (loop.run_once(20) == net::io_error::none);

    result = internal::check( (loop.is_valid()) &&
                              (added)           &&
                              (written == 1)    &&
                              (first)           &&
                              (calls == 1)      &&
                              (net::has_event(seen,
                                              net::event_mask::readable)),
                              "a ready descriptor's callback runs") && result;
    result = internal::check( (quiet)      &&
                              (calls == 1) &&
                              (loop.handle_count() == 0u),
                              "removing itself took effect after dispatch") &&
             result;
    result = internal::check( (loop.modify(ends[0],
                                           net::event_mask::writable) ==
                               net::io_error::invalid_argument) &&
                              (loop.remove(ends[0]) ==
                               net::io_error::invalid_argument),
                              "unknown descriptors are refused") && result;

    // the pipe's ends, if made
    if (piped)
    {
        (void)::close(ends[0]);
        (void)::close(ends[1]);
    }

    return result;
}

/*
tests_reactor_echo
  Tests the following:
  - an event_server echoes a short message back
  - a megabyte, more than a socket holds, queues and still comes back whole
  - "bye" is answered with "farewell" before the server closes; the
    client then sees the end of the stream
  - connects and closes are counted, and stop() ends run() cleanly
  It runs past 75 lines because one running reactor and its connections
serve every check.
*/
bool
tests_reactor_echo()
{
    internal::running  live;
    net::tcp_connector connector;
    bool               result = true;

    const bool bound = (live.server.bind(net::endpoint("127.0.0.1",
                                                       0u)) ==
                        net::io_error::none);

    live.started = ( (bound) &&
                     (::d_thread_create(&live.thread,
                                        &internal::run_main,
                                        &live) == 0) );

    const net::endpoint where = live.server.local_endpoint();
    net::open_result    first;
    std::string         reply(4u,
                              '\0');

    const bool short_ok =
        ( (live.started)                                           &&
          (connector.connect(where,
                             first) == net::io_error::none)        &&
          (net::write_all(*first,
                          "ping",
                          4u) == net::io_error::none)              &&
          (net::read_exactly(*first,
                             &reply[0],
                             reply.size()) == net::io_error::none) &&
          (reply == "ping") );

    first.reset();

    net::open_result  bulk;
    const std::size_t size = 1024u * 1024u;
    std::string       sent(size,
                           'q');
    std::string       back(size,
                           '\0');

    // a varied payload, so a reordering would show
    for (std::size_t i = 0u; i < size; ++i)
    {
        sent[i] = static_cast<char>('a' + (i % 23u));
    }

    const bool bulk_ok =
        ( (live.started)                                          &&
          (connector.connect(where,
                             bulk) == net::io_error::none)        &&
          (net::write_all(*bulk,
                          sent.data(),
                          sent.size()) == net::io_error::none)    &&
          (net::read_exactly(*bulk,
                             &back[0],
                             back.size()) == net::io_error::none) &&
          (back == sent) );

    bulk.reset();

    net::open_result last;
    std::string      farewell(8u,
                              '\0');
    char             after = 0;

    const bool bye_ok =
        ( (live.started)                                              &&
          (connector.connect(where,
                             last) == net::io_error::none)            &&
          (net::write_all(*last,
                          "bye",
                          3u) == net::io_error::none)                 &&
          (net::read_exactly(*last,
                             &farewell[0],
                             farewell.size()) == net::io_error::none) &&
          (farewell == "farewell") );
    const net::io_result end =
        (bye_ok) ? last->read(&after,
                              1u)
                 : net::io_result(0u,
                                  net::io_error::unknown);

    last.reset();

    // the server's closes of the three are counted before it stops
    for (int i = 0; ( (i < 100) &&
                      (live.tallied.closes < 3) ); ++i)
    {
        internal::pause_ms(10);
    }

    live.server.stop();

    // only a started loop is joined
    if (live.started)
    {
        (void)::d_thread_join(live.thread,
                              D_NULLPTR);
    }

    result = internal::check(short_ok,
                             "a short message echoes") && result;
    result = internal::check(bulk_ok,
                             "a megabyte queues and comes back whole") &&
             result;
    result = internal::check( (bye_ok)                           &&
                              (end.error == net::io_error::none) &&
                              (end.count == 0u),
                              "a goodbye is answered before the close") &&
             result;
    result = internal::check( (live.tallied.connects == 3)          &&
                              (live.tallied.closes == 3)            &&
                              (live.outcome == net::io_error::none) &&
                              (live.server.connection_count() == 0u),
                              "three connects, three closes, a clean "
                              "stop") && result;

    return result;
}

/*
tests_reactor_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_reactor_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "event masks", tests_reactor_mask },
        { "dispatch",    tests_reactor_dispatch },
        { "echo server", tests_reactor_echo }
    };
    bool all = true;

    // run every case, reporting each
    for (std::size_t i = 0u; i < sizeof(CASES) / sizeof(CASES[0]); ++i)
    {
        const entry& test   = CASES[i];
        const bool   passed = test.run();

        all = ( (all) &&
                (passed) );
        std::printf("  [%s] %s\n",
                    (passed) ? "PASS" : "FAIL",
                    test.name);
    }

    std::printf("%lu/%lu checks\n",
                static_cast<unsigned long>(internal::counts().checks -
                                           internal::counts().failures),
                static_cast<unsigned long>(internal::counts().checks));

    return all;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_ENV_NET_HAS_EPOLL
