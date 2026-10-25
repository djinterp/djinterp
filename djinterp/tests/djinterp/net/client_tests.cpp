/*******************************************************************************
* djinterp [net]                                               client_tests.cpp
*
* Tests of net/client.hpp.
*   A scripted connector fails as many times as it is told, with the error it
* is told, so the retry driver's attempts can be counted; the facade then
* reaches a real loopback acceptor through tcp_connector. The suite runs
* from C++98; the by-value forms are checked from C++11, and the connector
* concept from C++20. Link net.c, tcp.c and dmutex.c.
*
*
* path:      /tests/djinterp/net/client_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./client_tests.hpp"  // corresponding header
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <ctime>    // std::time
// djinterp
#include "../../../inc/djinterp/net/tcp/tcp.hpp"  // tcp_connector, acceptor


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

    // scripted
    //   struct: a connector that fails `failures` times with `error`, then
    // hands over a closed connection of net.hpp's; it counts its calls.
    struct scripted
    {
        scripted(
            unsigned             _failures,
            net::io_error::value _error
        )
            : failures(_failures),
              error(_error),
              calls(0u)
        {}

        net::io_error::value
        connect(
            const net::endpoint& _endpoint,
            net::open_result&    _out
        )
        {
            (void)_endpoint;
            calls += 1u;

            // still failing
            if (calls <= failures)
            {
                _out.fail(error);

                return error;
            }

            _out.reset(new net::c_connection(D_NULLPTR));

            return net::io_error::none;
        }

        unsigned             failures;
        net::io_error::value error;
        unsigned             calls;
    };

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    static_assert(net::connector<scripted>,
                  "a type with the in-place connect is a connector");
    static_assert(net::connector<net::tcp_connector>,
                  "tcp_connector is a connector");
    static_assert(!net::connector<tally>,
                  "a type without it is not");
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

NS_END  // internal


/*
tests_client_retryable
  Tests the following:
  - transient failures are retryable, permanent ones are not
*/
bool
tests_client_retryable()
{
    bool result = true;

    result = internal::check( (net::is_retryable(net::io_error::timed_out)) &&
                              (net::is_retryable(
                                   net::io_error::connection_refused))      &&
                              (net::is_retryable(net::io_error::interrupted)),
                              "transient failures retry") && result;
    result = internal::check( (!net::is_retryable(
                                   net::io_error::invalid_argument))    &&
                              (!net::is_retryable(net::io_error::none)) &&
                              (!net::is_retryable(
                                   net::io_error::access_denied)),
                              "permanent failures do not") && result;

    return result;
}

/*
tests_client_retry
  Tests the following:
  - connect_with_retry tries until a connection, sleeping between
  - it stops at the first permanent failure, and after max_attempts
  - a policy of 0 attempts tries once
*/
bool
tests_client_retry()
{
    const net::endpoint      where("127.0.0.1",
                                   1u);
    const net::retry_policy  three(3u,
                                   10,
                                   2.0,
                                   20);
    internal::scripted       flaky(2u,
                                   net::io_error::connection_refused);
    internal::scripted       hopeless(5u,
                                      net::io_error::invalid_argument);
    internal::scripted       stubborn(9u,
                                      net::io_error::timed_out);
    internal::scripted       once(9u,
                                  net::io_error::timed_out);
    net::open_result         first;
    net::open_result         second;
    net::open_result         third;
    net::open_result         fourth;
    bool                     result = true;

    const net::io_error::value recovered =
        net::connect_with_retry(flaky,
                                where,
                                first,
                                three);
    const net::io_error::value refused   =
        net::connect_with_retry(hopeless,
                                where,
                                second,
                                three);
    const net::io_error::value exhausted =
        net::connect_with_retry(stubborn,
                                where,
                                third,
                                three);
    const net::io_error::value single    =
        net::connect_with_retry(once,
                                where,
                                fourth,
                                net::retry_policy(0u,
                                                  10,
                                                  2.0,
                                                  0));

    result = internal::check( (recovered == net::io_error::none) &&
                              (first.ok())                       &&
                              (flaky.calls == 3u),
                              "two transient failures, then a connection") &&
             result;
    result = internal::check( (refused == net::io_error::invalid_argument) &&
                              (hopeless.calls == 1u)                       &&
                              (!second.ok()),
                              "a permanent failure stops at once") && result;
    result = internal::check( (exhausted == net::io_error::timed_out) &&
                              (stubborn.calls == 3u)                  &&
                              (third.error == net::io_error::timed_out),
                              "max_attempts bounds the tries") && result;
    result = internal::check( (single == net::io_error::timed_out) &&
                              (once.calls == 1u),
                              "zero attempts means one") && result;

#if (D_MOVE_ENABLED == 1)
    internal::scripted     later(1u,
                                 net::io_error::connection_reset);
    const net::open_result value = net::connect_with_retry(later,
                                                           where,
                                                           three);

    result = internal::check( (value.ok()) &&
                              (later.calls == 2u),
                              "the by-value form retries the same") && result;
#endif  // D_MOVE_ENABLED

    return result;
}

/*
tests_client_facade
  Tests the following:
  - a client over tcp_connector reaches a loopback acceptor by host and port
  - its retry policy is set and read back, and its backend reachable
*/
bool
tests_client_facade()
{
    net::tcp_acceptor                acceptor;
    net::client<net::tcp_connector>  client;
    net::open_result                 opened;
    net::open_result                 accepted;
    bool                             result = true;

    const bool bound = (acceptor.bind(net::endpoint("127.0.0.1",
                                                    0u)) ==
                        net::io_error::none);

    client.set_retry(net::retry_policy(2u,
                                       10,
                                       2.0,
                                       0));
    client.backend().options().no_delay = true;

    const net::io_error::value reached =
        client.connect("127.0.0.1",
                       acceptor.local_endpoint().port,
                       opened);

    (void)acceptor.accept(accepted);
    result = internal::check( (bound)                          &&
                              (reached == net::io_error::none) &&
                              (opened.ok())                    &&
                              (accepted.ok()),
                              "the facade connects by host and port") &&
             result;
    result = internal::check( (client.retry().max_attempts == 2u) &&
                              (client.backend().options().no_delay),
                              "the policy and the backend are the client's") &&
             result;

#if (D_MOVE_ENABLED == 1)
    net::open_result by_value = client.connect(acceptor.local_endpoint());
    net::open_result taken;

    (void)acceptor.accept(taken);
    result = internal::check( (by_value.ok()) &&
                              (taken.ok()),
                              "the by-value connect reaches it too") &&
             result;
#endif  // D_MOVE_ENABLED

    return result;
}

/*
tests_client_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_client_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "retryable", tests_client_retryable },
        { "retry",     tests_client_retry },
        { "facade",    tests_client_facade }
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
