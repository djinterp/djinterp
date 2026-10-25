/*******************************************************************************
* djinterp [net]                                                 curl_tests.cpp
*
* Tests of the libcurl binding's C++ face, net/curl/curl.hpp.
*   A small HTTP server of net's TCP layer answers on a loopback port, on a
* thread of dmutex.h's: /hello with a short body and a field of its own, /big
* with 100000 bytes, anything else 404, each connection closed after its
* answer. The suite runs from C++98; a throwing sink's exception arrives as
* itself from C++11 and as its nearest standard kind below. Link curl.c,
* net.c, tcp.c and dmutex.c, and -lcurl.
*
*
* path:      /tests/djinterp/net/curl/curl_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.12
*******************************************************************************/
#include "./curl_tests.hpp"  // corresponding header
// std
#include <cstddef>    // std::size_t
#include <cstdio>     // std::printf, std::sprintf
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string
// djinterp
#include "../../../../inc/djinterp/c/sync/dmutex.h"  // d_thread_create
#include "../../../../inc/djinterp/net/tcp/tcp.hpp"  // tcp_acceptor


NS_DJINTERP
NS_TESTING

namespace net  = ::djinterp::net;
namespace curl = ::djinterp::net::curl;

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

    // BIG
    //   constant: the size of /big's body.
    static const std::size_t BIG = 100000u;

    // answer
    //   function: the response for a request's first line.
    static std::string
    answer(
        const std::string& _request
    )
    {
        std::string status = "404 Not Found";
        std::string body   = "missing";
        std::string extra;

        // the two paths the server knows
        if (_request.compare(0u,
                             11u,
                             "GET /hello ") == 0)
        {
            status = "200 OK";
            body   = "hello world";
            extra  = "X-Test: yes\r\n";
        }
        else if (_request.compare(0u,
                                  9u,
                                  "GET /big ") == 0)
        {
            status = "200 OK";
            body.assign(BIG,
                        'b');
        }

        char      length[32];
        const int digits = std::sprintf(length,
                                        "%lu",
                                        static_cast<unsigned long>(
                                            body.size()));

        return "HTTP/1.1 " + status + "\r\n"                          +
               "Content-Type: text/plain\r\n" + extra                 +
               "Content-Length: "                                     +
               std::string(length,
                           static_cast<std::size_t>(digits)) + "\r\n" +
               "Connection: close\r\n\r\n" + body;
    }

    // http_server
    //   struct: an acceptor on loopback, served on a thread for a number of
    // connections.
    struct http_server
    {
        http_server()
            : acceptor(),
              serving(0u),
              thread(),
              started(false)
        {}

        net::tcp_acceptor acceptor;
        std::size_t       serving;
        d_thread_t        thread;
        bool              started;
    };

    // serve_main
    //   function: the server thread: per connection, reads the request's
    // head and writes the answer.
    static d_thread_result_t
    serve_main(
        void* _server
    )
    {
        http_server* const server = static_cast<http_server*>(_server);

        // one connection at a time, as many as asked for
        for (std::size_t i = 0u; i < server->serving; ++i)
        {
            net::open_result accepted;

            // a closed acceptor ends the serving
            if (server->acceptor.accept(accepted) != net::io_error::none)
            {
                break;
            }

            std::string head;
            char        chunk[512];

            // the request's head, up to its blank line
            while (head.find("\r\n\r\n") == std::string::npos)
            {
                const net::io_result got = accepted->read(chunk,
                                                          sizeof(chunk));

                // the client went away
                if ( (got.error != net::io_error::none) ||
                     (got.count == 0u) )
                {
                    break;
                }

                head.append(chunk,
                            got.count);
            }

            (void)net::write_all(*accepted,
                                 answer(head));
            (void)accepted->shutdown(net::shutdown_mode::write);
        }

        return D_THREAD_SUCCESS;
    }

    // start, finish
    //   function: binds loopback and serves `_count` connections; then
    // closes the acceptor and joins the thread.
    static bool
    start(
        http_server& _server,
        std::size_t  _count
    )
    {
        _server.serving = _count;
        _server.started =
            ( (_server.acceptor.bind(net::endpoint("127.0.0.1",
                                                   0u)) ==
               net::io_error::none) &&
              (::d_thread_create(&_server.thread,
                                 &serve_main,
                                 &_server) == 0) );

        return _server.started;
    }

    static void
    finish(
        http_server& _server
    )
    {
        _server.acceptor.close();

        // only a started thread is joined
        if (_server.started)
        {
            (void)::d_thread_join(_server.thread,
                                  D_NULLPTR);
        }

        return;
    }

    // url
    //   function: a path on the server.
    static std::string
    url(
        const http_server& _server,
        const char*        _path
    )
    {
        char      port[16];
        const int digits = std::sprintf(port,
                                        "%u",
                                        static_cast<unsigned int>(
                                            _server.acceptor
                                                .local_endpoint().port));

        return "http://127.0.0.1:" +
               std::string(port,
                           static_cast<std::size_t>(digits)) + _path;
    }

    // quiet
    //   function: options that take no proxy, so the loopback is reached
    // directly whatever the environment says.
    static curl::options
    quiet()
    {
        curl::options options;

        options.bypass_proxy = true;
        options.timeout_ms   = 10000;

        return options;
    }

    // counter, refuser, thrower, odd_thrower
    //   struct: sinks: one that counts, one that ends the transfer, one
    // that throws a std::runtime_error, one that throws an int.
    struct counter
    {
        explicit counter(
            std::size_t* _total
        )
            : total(_total)
        {}

        bool
        operator()(
            const char* _data,
            std::size_t _size
        ) const
        {
            (void)_data;
            *total += _size;

            return true;
        }

        std::size_t* total;
    };

    struct refuser
    {
        bool
        operator()(
            const char* _data,
            std::size_t _size
        ) const
        {
            (void)_data;
            (void)_size;

            return false;
        }
    };

    struct thrower
    {
        bool
        operator()(
            const char* _data,
            std::size_t _size
        ) const
        {
            (void)_data;
            (void)_size;

            throw std::runtime_error("boom");
        }
    };

    struct odd_thrower
    {
        bool
        operator()(
            const char* _data,
            std::size_t _size
        ) const
        {
            (void)_data;
            (void)_size;

            throw 7;
        }
    };

NS_END  // internal


/*
tests_curl_library
  Tests the following:
  - libcurl is available, reports a version, and speaks HTTP
  - the global state initializes, through ensure_global and scoped_global
*/
bool
tests_curl_library()
{
    const curl::scoped_global global;
    bool                      result = true;

    result = internal::check( (curl::available())            &&
                              (curl::version_number() != 0u) &&
                              (curl::version_string()[0] != '\0'),
                             "libcurl is there, with a version") && result;
    result = internal::check( (curl::ensure_global()) &&
                              (global.ok())           &&
                              (global.error() == net::transport_error::none),
                             "the global state initializes") && result;

    return result;
}

/*
tests_curl_perform
  Tests the following:
  - perform returns a 200, its fields, and the whole body
  - a missing path is a 404, still with no transport error
*/
bool
tests_curl_perform()
{
    internal::http_server server;
    net::request          hello;
    net::request          missing;
    bool                  result = true;

    const bool started = internal::start(server,
                                         2u);

    hello.url   = internal::url(server,
                                "/hello");
    missing.url = internal::url(server,
                                "/nowhere");

    const net::response got  = curl::perform(hello,
                                             internal::quiet());
    const net::response lost = curl::perform(missing,
                                             internal::quiet());

    internal::finish(server);
    result = internal::check( (started)                                 &&
                              (got.error == net::transport_error::none) &&
                              (got.status == 200)                       &&
                              (got.body == "hello world")               &&
                              (got.header("x-test") == "yes"),
                             "a 200 with its field and body") && result;
    result = internal::check( (lost.error == net::transport_error::none) &&
                              (lost.status == 404),
                             "a 404 is not a transport error") && result;

    return result;
}

/*
tests_curl_stream
  Tests the following:
  - perform_stream hands every byte of a 100000-byte body to the sink
  - a sink that returns false ends the transfer as a write error
*/
bool
tests_curl_stream()
{
    internal::http_server server;
    net::request          big;
    net::response         meta;
    net::response         cut;
    std::size_t           total  = 0u;
    bool                  result = true;

    const bool started = internal::start(server,
                                         2u);

    big.url = internal::url(server,
                            "/big");

    const curl::body_sink counting  = internal::counter(&total);
    const curl::body_sink refusing  = internal::refuser();
    const net::transport_error::value whole =
        curl::perform_stream(big,
                             counting,
                             meta,
                             internal::quiet());
    const net::transport_error::value ended =
        curl::perform_stream(big,
                             refusing,
                             cut,
                             internal::quiet());

    internal::finish(server);
    result = internal::check( (started)                             &&
                              (whole == net::transport_error::none) &&
                              (meta.status == 200)                  &&
                              (total == internal::BIG),
                             "every byte reaches the sink") && result;
    result = internal::check(ended == net::transport_error::write_error,
                             "a refusing sink is a write error") && result;

    return result;
}

/*
tests_curl_throwing_sink
  Tests the following:
  - a std::exception a sink throws reaches the caller once the transfer
    has ended, with its message: from C++11 as itself, below as a
    std::runtime_error
  - one that is no std::exception arrives as itself from C++11, and below
    as a std::runtime_error saying so
*/
bool
tests_curl_throwing_sink()
{
    internal::http_server server;
    net::request          hello;
    net::response         meta;
    bool                  result = true;

    const bool started = internal::start(server,
                                         2u);

    hello.url = internal::url(server,
                              "/hello");

    std::string what;
    bool        odd = false;

    try
    {
        (void)curl::perform_stream(hello,
                                   curl::body_sink(internal::thrower()),
                                   meta,
                                   internal::quiet());
    }
    catch (const std::runtime_error& _error)
    {
        what = _error.what();
    }

    try
    {
        (void)curl::perform_stream(hello,
                                   curl::body_sink(internal::odd_thrower()),
                                   meta,
                                   internal::quiet());
    }
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    catch (int _thrown)
    {
        odd = (_thrown == 7);
    }
#else
    catch (const std::runtime_error& _error)
    {
        odd = (std::string(_error.what()).find("not a std::exception") !=
               std::string::npos);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    internal::finish(server);
    result = internal::check( (started) &&
                              (what == "boom"),
                             "a sink's exception arrives with its "
                             "message") && result;
    result = internal::check(odd,
                             "an exception of no standard kind arrives as "
                             "the level allows") && result;

    return result;
}

/*
tests_curl_errors
  Tests the following:
  - a port nothing listens on is could_not_connect, with no status
  - transport errors have names
*/
bool
tests_curl_errors()
{
    internal::http_server closed;
    net::request          nowhere;
    bool                  result = true;

    // a port taken and released: nothing listens there now
    const bool bound = (closed.acceptor.bind(net::endpoint("127.0.0.1",
                                                           0u)) ==
                        net::io_error::none);

    nowhere.url = internal::url(closed,
                                "/hello");
    closed.acceptor.close();

    const net::response refused = curl::perform(nowhere,
                                                internal::quiet());

    result = internal::check( (bound)                                   &&
                              (refused.error ==
                               net::transport_error::could_not_connect) &&
                              (refused.status == 0),
                             "nothing listening is could_not_connect") &&
             result;
    result = internal::check(net::to_string(
                                 net::transport_error::could_not_connect)
                                 [0] != '\0',
                             "transport errors have names") && result;

    return result;
}

/*
tests_curl_errors_to_net
  Tests the following:
  - each of curl.h's errors maps onto the d_net_error net.h's rules give it
  - a value outside the enum maps to D_NET_ERROR_UNKNOWN
*/
bool
tests_curl_errors_to_net()
{
    struct mapping
    {
        ::d_curl_error error;
        ::d_net_error  net;
    };

    static const mapping MAP[] =
    {
        { D_CURL_OK,                         D_NET_OK                       },
        { D_CURL_ERROR_INVALID_ARGUMENT,     D_NET_ERROR_INVALID_ARGUMENT   },
        { D_CURL_ERROR_UNSUPPORTED,          D_NET_ERROR_UNKNOWN            },
        { D_CURL_ERROR_UNSUPPORTED_PROTOCOL, D_NET_ERROR_ADDRESS_INVALID    },
        { D_CURL_ERROR_RESOLVE,              D_NET_ERROR_HOST_UNREACHABLE   },
        { D_CURL_ERROR_CONNECT,              D_NET_ERROR_CONNECTION_REFUSED },
        { D_CURL_ERROR_TIMED_OUT,            D_NET_ERROR_TIMED_OUT          },
        { D_CURL_ERROR_TLS,                  D_NET_ERROR_PROTOCOL           },
        { D_CURL_ERROR_TOO_MANY_REDIRECTS,   D_NET_ERROR_UNKNOWN            },
        { D_CURL_ERROR_SINK,                 D_NET_ERROR_UNKNOWN            },
        { D_CURL_ERROR_SOURCE,               D_NET_ERROR_UNKNOWN            },
        { D_CURL_ERROR_CANCELED,             D_NET_ERROR_UNKNOWN            },
        { D_CURL_ERROR_OUT_OF_MEMORY,        D_NET_ERROR_OUT_OF_MEMORY      },
        { D_CURL_ERROR_UNKNOWN,              D_NET_ERROR_UNKNOWN            }
    };
    bool mapped = true;
    bool result = true;

    // each error against the table
    for (std::size_t i = 0u; i < sizeof(MAP) / sizeof(MAP[0]); ++i)
    {
        mapped = ( (mapped) &&
                   (::d_curl_error_to_net(MAP[i].error) == MAP[i].net) );
    }

    // 15 lies in the enum's range, and no enumerator has it
    const bool outside =
        (::d_curl_error_to_net(static_cast< ::d_curl_error>(15)) ==
         D_NET_ERROR_UNKNOWN);

    result = internal::check(mapped,
                             "each error maps by net.h's rules") && result;
    result = internal::check(outside,
                             "a value outside the enum is unknown") &&
             result;

    return result;
}

/*
tests_curl_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_curl_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "library",        tests_curl_library },
        { "perform",        tests_curl_perform },
        { "stream",         tests_curl_stream },
        { "throwing sinks", tests_curl_throwing_sink },
        { "errors",         tests_curl_errors },
        { "errors to net",  tests_curl_errors_to_net }
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
