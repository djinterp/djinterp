/*******************************************************************************
* djinterp [net]                                               server_tests.cpp
*
* Tests of net/server.hpp.
*   A server answers on a loopback port, its run() on a thread of dmutex.h's;
* the tests speak HTTP/1.1 to it over plain TCP connections of tcp.hpp's,
* byte for byte, so nothing between them hides what the server sends; one
* test serves https through tls_acceptor, the TLS suite's certificate on its
* context, where the SSL kernel has its OpenSSL-family engine. The suite runs
* from C++98. Link net.c, tcp.c and dmutex.c, the kernel (ssl.c,
* ssl_openssl.c), sink_common.c and the codec, and with OpenSSL -lssl
* -lcrypto.
*
*
* path:      /tests/djinterp/net/server_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.05
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./server_tests.hpp"  // corresponding header
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
#include <cstdlib>  // std::strtoul
#include <string>   // std::string
// djinterp
#include "../../../inc/djinterp/c/sync/dmutex.h"  // d_thread_create
#if (D_INTERNAL_NET_HAS_INT64 == 1)
    // std
    #include <cstdio>    // std::fopen, std::fputs, std::remove
    // posix
    #include <unistd.h>  // getpid
    // djinterp
    #include "../../../inc/djinterp/net/tls/tls.hpp"  // tls_acceptor
    #include "./tls/tls_test_certificate.hpp"          // the certificate
#endif  // D_INTERNAL_NET_HAS_INT64


NS_DJINTERP
NS_TESTING

namespace net  = ::djinterp::net;
namespace http = ::djinterp::net::http;

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

    // site
    //   struct: the handler: /hello says hello, /echo sends the body back
    // with the method in a field, anything else is not found.
    struct site
    {
        net::response
        operator()(
            const net::request& _request
        ) const
        {
            net::response answer;

            // the two paths the site knows
            if (_request.url == "/hello")
            {
                answer.status = 200;
                answer.body   = "hello";
            }
            else if (_request.url == "/echo")
            {
                answer.status = 200;
                answer.body   = _request.body;
                answer.headers.push_back(net::header_field(
                                             "X-Method",
                                             net::to_string(_request.method)));
            }
            else
            {
                answer.status = 404;
            }

            return answer;
        }
    };

    // running
    //   struct: a server, its run()'s thread, and what run() returned.
    struct running
    {
        running()
            : host(),
              outcome(net::io_error::unknown),
              thread(),
              started(false)
        {}

        http::server<>       host;
        net::io_error::value outcome;
        d_thread_t           thread;
        bool                 started;
    };

    // run_main
    //   function: the server's thread.
    static d_thread_result_t
    run_main(
        void* _running
    )
    {
        running* const work = static_cast<running*>(_running);

        work->outcome = work->host.run(http::request_handler(site()));

        return D_THREAD_SUCCESS;
    }

    // start, finish
    //   function: a server on loopback with small caps, running; then
    // stopped, and its thread joined.
    static bool
    start(
        running& _live
    )
    {
        _live.host.options().max_header_bytes = 1024u;
        _live.host.options().max_body_bytes   = 4096u;
        _live.started =
            ( (_live.host.bind(net::endpoint("127.0.0.1",
                                             0u)) ==
               net::io_error::none) &&
              (::d_thread_create(&_live.thread,
                                 &run_main,
                                 &_live) == 0) );

        return _live.started;
    }

    static void
    finish(
        running& _live
    )
    {
        _live.host.stop();

        // only a started thread is joined
        if (_live.started)
        {
            (void)::d_thread_join(_live.thread,
                                  D_NULLPTR);
        }

        return;
    }

    // dial
    //   function: a TCP connection to the server.
    static bool
    dial(
        running&          _live,
        net::open_result& _out
    )
    {
        net::tcp_connector connector;

        return (connector.connect(_live.host.local_endpoint(),
                                  _out) == net::io_error::none);
    }

    // reply
    //   struct: one response as it came: the status line, the head, the
    // body by its Content-Length.
    struct reply
    {
        reply()
            : status(),
              head(),
              body(),
              complete(false)
        {}

        std::string status;
        std::string head;
        std::string body;
        bool        complete;
    };

    // field
    //   function: a field's value in a response head, "" when absent.
    static std::string
    field(
        const reply&       _reply,
        const std::string& _name
    )
    {
        const std::string key = "\r\n" + _name + ": ";
        const std::size_t at  = _reply.head.find(key);

        // absent
        if (at == std::string::npos)
        {
            return std::string();
        }

        const std::size_t from = at + key.size();

        return _reply.head.substr(from,
                                  _reply.head.find("\r\n",
                                                   from) - from);
    }

    // receive
    //   function: reads one response off a connection; a HEAD's body is
    // not read. `_held` keeps bytes read past one response for the next.
    static reply
    receive(
        net::connection& _connection,
        std::string&     _held,
        bool             _head
    )
    {
        reply got;
        char  chunk[2048];

        // the head, to its blank line
        while (_held.find("\r\n\r\n") == std::string::npos)
        {
            const net::io_result r = _connection.read(chunk,
                                                      sizeof(chunk));

            // the server closed, or failed, first
            if ( (r.error != net::io_error::none) ||
                 (r.count == 0u) )
            {
                return got;
            }

            _held.append(chunk,
                         r.count);
        }

        const std::size_t end = _held.find("\r\n\r\n");

        got.head   = _held.substr(0u,
                                  end + 2u);
        got.status = got.head.substr(0u,
                                     got.head.find("\r\n"));
        _held.erase(0u,
                    end + 4u);

        const std::size_t length =
            (_head) ? 0u
                    : std::strtoul(field(got,
                                         "Content-Length").c_str(),
                                   D_NULLPTR,
                                   10);

        // the body, to its length
        while (_held.size() < length)
        {
            const net::io_result r = _connection.read(chunk,
                                                      sizeof(chunk));

            // the server closed, or failed, first
            if ( (r.error != net::io_error::none) ||
                 (r.count == 0u) )
            {
                return got;
            }

            _held.append(chunk,
                         r.count);
        }

        got.body     = _held.substr(0u,
                                    length);
        got.complete = true;
        _held.erase(0u,
                    length);

        return got;
    }

    // closed_after
    //   function: whether the server closes the connection next.
    static bool
    closed_after(
        net::connection& _connection
    )
    {
        char                 one = 0;
        const net::io_result r   = _connection.read(&one,
                                                    1u);

        return ( (r.error == net::io_error::none) &&
                 (r.count == 0u) );
    }

    // ask
    //   function: sends a request's text.
    static bool
    ask(
        net::connection&   _connection,
        const std::string& _text
    )
    {
        return (net::write_all(_connection,
                               _text) == net::io_error::none);
    }

#if (D_INTERNAL_NET_HAS_INT64 == 1)
    // secure
    //   struct: an https server with the test certificate, its thread, and
    // what run() returned.
    struct secure
    {
        secure()
            : host(),
              outcome(net::io_error::unknown),
              thread(),
              started(false)
        {}

        http::server<net::tls_acceptor<> > host;
        net::io_error::value               outcome;
        d_thread_t                         thread;
        bool                               started;
    };

    // secure_main
    //   function: the https server's thread.
    static d_thread_result_t
    secure_main(
        void* _secure
    )
    {
        secure* const work = static_cast<secure*>(_secure);

        work->outcome = work->host.run(http::request_handler(site()));

        return D_THREAD_SUCCESS;
    }

    // pem_file
    //   function: writes PEM text to a new file; whether it all went.
    static bool
    pem_file(
        const std::string& _path,
        const char*        _text
    )
    {
        std::FILE* const out = std::fopen(_path.c_str(),
                                          "wb");

        // a file that cannot be made holds nothing
        if (!out)
        {
            return false;
        }

        const bool whole = (std::fputs(_text,
                                       out) >= 0);

        return ( (std::fclose(out) == 0) &&
                 (whole) );
    }
#endif  // D_INTERNAL_NET_HAS_INT64

NS_END  // internal


/*
tests_server_requests
  Tests the following:
  - a GET is answered with its status, Server, Content-Length and a kept
    connection, and then, on the same connection, a POST by Content-Length
    and a chunked POST with extensions and a trailer echo their bodies
  - HEAD has the head and no body; a missing path is a 404
  - Connection: close, and an HTTP/1.0 request without keep-alive, are
    answered and then closed
  It runs past 75 lines because one running server and its connections serve
every check.
*/
bool
tests_server_requests()
{
    internal::running live;
    net::open_result  kept;
    net::open_result  closing;
    net::open_result  old;
    std::string       held;
    bool              result = true;

    const bool started = internal::start(live);
    const bool dialed  = ( (started) &&
                           (internal::dial(live,
                                           kept)) );

    (void)( (dialed) &&
            (internal::ask(*kept,
                           "GET /hello HTTP/1.1\r\nHost: x\r\n\r\n")) );

    const internal::reply hello =
        (dialed) ? internal::receive(*kept,
                                     held,
                                     false) : internal::reply();

    (void)( (dialed) &&
            (internal::ask(*kept,
                           "POST /echo HTTP/1.1\r\nHost: x\r\n"
                           "Content-Length: 3\r\n\r\nabc")) );

    const internal::reply sized =
        (dialed) ? internal::receive(*kept,
                                     held,
                                     false) : internal::reply();

    (void)( (dialed) &&
            (internal::ask(*kept,
                           "POST /echo HTTP/1.1\r\nHost: x\r\n"
                           "Transfer-Encoding: chunked\r\n\r\n"
                           "3\r\nabc\r\n2;note=1\r\nde\r\n0\r\n"
                           "X-Trailer: 1\r\n\r\n")) );

    const internal::reply chunked =
        (dialed) ? internal::receive(*kept,
                                     held,
                                     false) : internal::reply();

    (void)( (dialed) &&
            (internal::ask(*kept,
                           "HEAD /hello HTTP/1.1\r\nHost: x\r\n\r\n")) );

    const internal::reply head =
        (dialed) ? internal::receive(*kept,
                                     held,
                                     true) : internal::reply();

    (void)( (dialed) &&
            (internal::ask(*kept,
                           "GET /nowhere HTTP/1.1\r\nHost: x\r\n\r\n")) );

    const internal::reply missing =
        (dialed) ? internal::receive(*kept,
                                     held,
                                     false) : internal::reply();

    kept.reset();
    held.clear();

    const bool       closer = ( (started)                 &&
                                (internal::dial(live,
                                                closing)) &&
                                (internal::ask(*closing,
                                               "GET /hello HTTP/1.1\r\n"
                                               "Host: x\r\n"
                                               "Connection: close\r\n\r\n")) );
    internal::reply  last   =
        (closer) ? internal::receive(*closing,
                                     held,
                                     false) : internal::reply();
    const bool       ended  = (closer) &&
                              (internal::closed_after(*closing));

    held.clear();

    const bool      older = ( (started)             &&
                              (internal::dial(live,
                                              old)) &&
                              (internal::ask(*old,
                                             "GET /hello HTTP/1.0\r\n\r\n")) );
    internal::reply once  =
        (older) ? internal::receive(*old,
                                    held,
                                    false) : internal::reply();
    const bool      done  = (older) &&
                            (internal::closed_after(*old));

    internal::finish(live);
    result = internal::check( (hello.complete)                           &&
                              (hello.status == "HTTP/1.1 200 OK")        &&
                              (hello.body == "hello")                    &&
                              (internal::field(hello,
                                               "Content-Length") == "5") &&
                              (internal::field(hello,
                                               "Connection") ==
                               "keep-alive")                             &&
                              (!internal::field(hello,
                                                "Server").empty()),
                              "a GET is answered, the connection kept") &&
             result;
    result = internal::check( (sized.complete)                        &&
                              (sized.body == "abc")                   &&
                              (internal::field(sized,
                                               "X-Method") == "POST") &&
                              (chunked.complete)                      &&
                              (chunked.body == "abcde"),
                              "bodies by length and chunked echo back") &&
             result;
    result = internal::check( (head.complete)                            &&
                              (head.status == "HTTP/1.1 200 OK")         &&
                              (internal::field(head,
                                               "Content-Length") == "5") &&
                              (missing.complete)                         &&
                              (missing.status.find("404") !=
                               std::string::npos),
                              "HEAD has no body; a missing path is 404") &&
             result;
    result = internal::check( (last.complete)                            &&
                              (internal::field(last,
                                               "Connection") == "close") &&
                              (ended)                                    &&
                              (once.complete)                            &&
                              (once.body == "hello")                     &&
                              (done),
                              "close, and HTTP/1.0, end the connection") &&
             result;
    result = internal::check(live.outcome == net::io_error::none,
                             "stop() ends run() cleanly") && result;

    return result;
}

/*
tests_server_refusals
  Tests the following:
  - a malformed request line is a 400, then the close
  - a header past the cap is a 413, as is a body past it
  - a chunked body with a bad size line is a 400
  It runs past 75 lines because one running server serves every check.
*/
bool
tests_server_refusals()
{
    internal::running live;
    net::open_result  garbled;
    net::open_result  headed;
    net::open_result  heavy;
    net::open_result  broken;
    std::string       held;
    bool              result = true;

    const bool started = internal::start(live);

    const bool sent_garble = ( (started)                 &&
                               (internal::dial(live,
                                               garbled)) &&
                               (internal::ask(*garbled,
                                              "garbage\r\n\r\n")) );
    internal::reply bad =
        (sent_garble) ? internal::receive(*garbled,
                                          held,
                                          false)
                      : internal::reply();
    const bool bad_closed = (sent_garble) &&
                            (internal::closed_after(*garbled));

    held.clear();

    const bool sent_header = ( (started)                &&
                               (internal::dial(live,
                                               headed)) &&
                               (internal::ask(*headed,
                                              "GET /hello HTTP/1.1\r\n"
                                              "X-Long: "       +
                                              std::string(2000u,
                                                          'h') +
                                              "\r\n\r\n")) );
    internal::reply long_head =
        (sent_header) ? internal::receive(*headed,
                                          held,
                                          false)
                      : internal::reply();

    held.clear();

    const bool sent_body = ( (started)               &&
                             (internal::dial(live,
                                             heavy)) &&
                             (internal::ask(*heavy,
                                            "POST /echo HTTP/1.1\r\n"
                                            "Content-Length: 9999\r\n\r\n")) );
    internal::reply long_body =
        (sent_body) ? internal::receive(*heavy,
                                        held,
                                        false)
                    : internal::reply();

    held.clear();

    const bool sent_chunks = ( (started)                &&
                               (internal::dial(live,
                                               broken)) &&
                               (internal::ask(*broken,
                                              "POST /echo HTTP/1.1\r\n"
                                              "Transfer-Encoding: chunked"
                                              "\r\n\r\nzz\r\n")) );
    internal::reply bad_chunks =
        (sent_chunks) ? internal::receive(*broken,
                                          held,
                                          false)
                      : internal::reply();

    internal::finish(live);
    result = internal::check( (bad.complete)                                &&
                              (bad.status.find("400") != std::string::npos) &&
                              (bad_closed),
                              "a malformed request is a 400, then closed") &&
             result;
    result = internal::check( (long_head.complete) &&
                              (long_head.status.find("413") !=
                               std::string::npos)  &&
                              (long_body.complete) &&
                              (long_body.status.find("413") !=
                               std::string::npos),
                              "past a cap is a 413") && result;
    result = internal::check( (bad_chunks.complete) &&
                              (bad_chunks.status.find("400") !=
                               std::string::npos),
                              "a bad chunk size is a 400") && result;

    return result;
}

/*
tests_server_concurrency
  Tests the following:
  - an idle kept-alive connection does not hold up another client: each
    connection has its own thread
  - stop() ends run() with a connection still open and idle: its reading
    side is shut, the server closes it, and the client sees the end
*/
bool
tests_server_concurrency()
{
    internal::running live;
    net::open_result  idle;
    net::open_result  busy;
    std::string       held;
    bool              result = true;

    const bool started = internal::start(live);
    const bool parked  = ( (started) &&
                           (internal::dial(live,
                                           idle)) );
    const bool asked   = ( (started)              &&
                           (internal::dial(live,
                                           busy)) &&
                           (internal::ask(*busy,
                                          "GET /hello HTTP/1.1\r\n"
                                          "Host: x\r\n\r\n")) );
    internal::reply served =
        (asked) ? internal::receive(*busy,
                                    held,
                                    false) : internal::reply();

    busy.reset();
    internal::finish(live);

    const bool ended = (parked) &&
                       (internal::closed_after(*idle));

    result = internal::check( (parked)          &&
                              (served.complete) &&
                              (served.body == "hello"),
                              "an idle connection holds up nobody") && result;
    result = internal::check( (live.outcome == net::io_error::none) &&
                              (ended),
                              "stop() ends an idle connection, and run()") &&
             result;

    return result;
}

#if (D_INTERNAL_NET_HAS_INT64 == 1)
/*
tests_server_https
  Tests the following:
  - server<tls_acceptor<>> with a certificate on its context serves a GET
    to a TLS client, the handshake running on the connection's thread
  Without the OpenSSL-family engine there is nothing to serve, and it
passes having said so.
*/
bool
tests_server_https()
{
    // a build without the engine has no https to serve
    if (!net::tls_context(net::tls_context::role::server).is_valid())
    {
        std::printf("    https skipped: no OpenSSL-family engine\n");

        return true;
    }

    internal::secure live;
    char             pid[32];
    const int        length = std::sprintf(pid,
                                           "%ld",
                                           static_cast<long>(::getpid()));
    const std::string stem  = "/tmp/djinterp_server_https_" +
                              std::string(pid,
                                          static_cast<std::size_t>(length));
    const std::string cert  = stem + "_cert.pem";
    const std::string key   = stem + "_key.pem";
    bool              result = true;

    net::tls_context& context = live.host.acceptor().context();

    live.started =
        ( (internal::pem_file(cert,
                              internal::tls_certificate_pem()))       &&
          (internal::pem_file(key,
                              internal::tls_key_pem()))               &&
          (context.use_certificate_file(cert) == net::io_error::none) &&
          (context.use_private_key_file(key) == net::io_error::none)  &&
          (live.host.bind(net::endpoint("127.0.0.1",
                                        0u)) == net::io_error::none)  &&
          (::d_thread_create(&live.thread,
                             &internal::secure_main,
                             &live) == 0) );

    net::tls_connector<> connector;
    net::open_result     opened;
    std::string          held;

    connector.set_verify(false);

    const bool asked = ( (live.started)                                     &&
                         (connector.connect(live.host.local_endpoint(),
                                            opened) == net::io_error::none) &&
                         (internal::ask(*opened,
                                        "GET /hello HTTP/1.1\r\n"
                                        "Host: x\r\n\r\n")) );
    internal::reply got =
        (asked) ? internal::receive(*opened,
                                    held,
                                    false) : internal::reply();

    opened.reset();
    live.host.stop();

    // only a started thread is joined
    if (live.started)
    {
        (void)::d_thread_join(live.thread,
                              D_NULLPTR);
    }

    (void)std::remove(cert.c_str());
    (void)std::remove(key.c_str());
    result = internal::check( (got.complete)                    &&
                              (got.status == "HTTP/1.1 200 OK") &&
                              (got.body == "hello")             &&
                              (live.outcome == net::io_error::none),
                              "https: a GET over TLS is served") && result;

    return result;
}
#endif  // D_INTERNAL_NET_HAS_INT64

/*
tests_server_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_server_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "requests",    tests_server_requests },
        { "refusals",    tests_server_refusals },
        { "concurrency", tests_server_concurrency },
#if (D_INTERNAL_NET_HAS_INT64 == 1)
        { "https",       tests_server_https },
#endif  // D_INTERNAL_NET_HAS_INT64
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
