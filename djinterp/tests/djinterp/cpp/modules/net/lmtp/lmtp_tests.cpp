/*******************************************************************************
* djinterp [cpp][net]                                             lmtp_tests.cpp
*
* Tests of the LMTP C++ facade, net/lmtp/lmtp.hpp.
*   A server runs on loopback TCP, or on a unix-domain socket, its accept
* loop on a thread of dmutex.h's, its handlers refusing some recipients at
* RCPT and some after the data, and recording the rest under a lock; the
* tests connect as clients and check each recipient's final reply against
* what the handlers did. A scripted connection hangs up partway through the
* final replies, for last_report(). The live test speaks to a real LMTP
* server, as the C suite's does, where DJINTERP_TEST_LMTP_SOCKET, or
* DJINTERP_TEST_LMTP_HOST and _PORT, name one and DJINTERP_TEST_LMTP_ACCEPTS
* a mailbox that takes mail (with _REFUSES and _FULL as the C suite takes
* them); without them it passes, saying it skipped.
*   The suite runs from C++98; connect() and the moves are checked from
* C++11. Link the SMTP kernel's C sources, smtp.cpp, lmtp.cpp,
* net_common.c, tcp.c, the TLS kernel and its OpenSSL-family engine,
* dmutex.c, sink_common.c and net's codec; and -lssl -lcrypto.
*
*
* path:      /tests/djinterp/cpp/modules/net/lmtp/lmtp_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "./lmtp_tests.hpp"  // corresponding header


#if (D_INTERNAL_NET_HAS_INT64 == 1)

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf, std::snprintf
#include <cstdlib>  // std::free, std::getenv, std::strtoul
#include <cstring>  // std::memcpy
#include <ctime>    // std::time, std::clock
#include <string>   // std::string
#include <vector>   // std::vector
#if defined(_WIN32)
    // windows
    #include <io.h>      // _access
#else
    // posix
    #include <unistd.h>  // access
#endif  // _WIN32
// djinterp
#include "../../../../../../inc/djinterp/c/sync/dmutex.h"        // d_thread_create
#include "../../../../../../inc/djinterp/env/net/tcp/env_tcp.h"  // D_ENV_TCP_HAS_UNIX
#if (D_MOVE_ENABLED == 1)
    // std
    #include <type_traits>  // std::is_same
#endif  // D_MOVE_ENABLED


NS_DJINTERP
NS_TESTING

namespace net  = ::djinterp::net;
namespace lmtp = ::djinterp::net::lmtp;
namespace smtp = ::djinterp::net::smtp;

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
    static
    tally& counts() D_NOEXCEPT
    {
        static tally instance;

        return instance;
    }

    // check
    //   function: records one check, printing it if it failed.
    static
    bool check(
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

    // seen
    //   struct: what on_deliver was shown, under a lock of its own, as the
    // handlers run on the sessions' threads: each recipient and its place,
    // in the order they were asked, and the message and greeting once.
    struct seen
    {
        seen()
            : recipients(),
              places(),
              message(),
              client_domain()
        {
            (void)::d_mutex_init(&mutex);
        }

        ~seen()
        {
            (void)::d_mutex_destroy(&mutex);
        }

        std::vector<std::string> recipients;
        std::vector<std::size_t> places;
        std::string              message;
        std::string              client_domain;
        dj_mutex_t                mutex;
    };

    // refuse_rcpt, deliver
    //   struct: the handlers, as function objects: RCPT refuses a local
    // part of "reject" with 550; delivery refuses one of "full" with 452,
    // as a mailbox over its quota would, and records the rest.
    struct refuse_rcpt
    {
        unsigned int operator()(
            const lmtp::session& _session,
            const std::string&   _recipient
        ) const
        {
            (void)_session;

            return (_recipient.compare(0u, 7u, "reject@") == 0) ? 550u : 0u;
        }
    };

    struct deliver
    {
        explicit
        deliver(
            seen* _into
        )
            : into(_into)
        {}

        unsigned int operator()(
            const lmtp::session& _session,
            std::size_t          _place,
            const std::string&   _recipient,
            re_std::string_view  _message
        ) const
        {
            // a full mailbox takes nothing
            if (_recipient.compare(0u, 5u, "full@") == 0)
            {
                return 452u;
            }

            (void)::d_mutex_lock(&into->mutex);
            into->recipients.push_back(_recipient);
            into->places.push_back(_place);
            into->message       = std::string(_message.data(),
                                              _message.size());
            into->client_domain = _session.client_domain;
            (void)::d_mutex_unlock(&into->mutex);

            return 0u;
        }

        seen* into;
    };

    // running
    //   struct: an LMTP server, its loop on a thread, and what the loop
    // returned.
    struct running
    {
        running()
            : handled(),
              host(D_NULLPTR),
              port(0u),
              outcome(false),
              thread(),
              started(false)
        {}

        seen          handled;
        lmtp::server* host;
        unsigned int  port;
        bool          outcome;
        dj_thread_t    thread;
        bool          started;
    };

    // run_main
    //   function: the accept loop's thread.
    static
    dj_thread_result_t run_main(
        void* _running
    )
    {
        running* const          work   = static_cast<running*>(_running);
        const lmtp::void_result result = work->host->run();

        work->outcome = result.has_value();

        return D_THREAD_SUCCESS;
    }

    // start, finish
    //   function: a server with the test handlers, listening on 127.0.0.1
    // or, given a path, on a unix-domain socket, and running; then
    // stopped, and its loop joined.
    static
    bool start(
        running&           _server,
        const std::string& _path
    )
    {
        lmtp::server_options options;
        lmtp::server_handler handler;

        options.domain     = "lmtp.example";
        handler.on_rcpt    = refuse_rcpt();
        handler.on_deliver = deliver(&_server.handled);
        _server.host       = new lmtp::server(options,
                                              handler);

        bool listening = false;

        // a port of the system's, or the socket's path
        if (_path.empty())
        {
            const lmtp::port_result bound = _server.host->listen("127.0.0.1",
                                                                 0u);

            _server.port = (bound.has_value()) ? *bound : 0u;
            listening    = bound.has_value();
        }
        else
        {
            listening = _server.host->listen_local(_path).has_value();
        }

        _server.started = ( (listening) &&
                            (::d_thread_create(&_server.thread,
                                               &run_main,
                                               &_server) == 0) );

        return _server.started;
    }

    static
    void finish(
        running& _server
    )
    {
        // only a running loop is stopped and joined
        if (_server.started)
        {
            _server.host->stop();
            (void)::d_thread_join(_server.thread,
                                  D_NULLPTR);
        }

        delete _server.host;
        _server.host = D_NULLPTR;

        return;
    }

    // smtp_main
    //   function: an SMTP server's accept loop, on a thread.
    static
    dj_thread_result_t smtp_main(
        void* _server
    )
    {
        (void)static_cast<smtp::server*>(_server)->run();

        return D_THREAD_SUCCESS;
    }

    // options_for
    //   function: a plain-text client to the running server, over TCP or,
    // given a path, its socket.
    static
    lmtp::client_options options_for(
        const running&     _server,
        const std::string& _path
    )
    {
        lmtp::client_options options;

        options.host          = (_path.empty()) ? std::string("127.0.0.1")
                                                : _path;
        options.port          = _server.port;
        options.local         = (!_path.empty());
        options.mode          = lmtp::security::none;
        options.helo_domain   = "client.example";
        options.io_timeout_ms = 5000;

        return options;
    }

    // letter
    //   function: an envelope from alice to bob, reject, full and carol.
    static
    lmtp::envelope letter()
    {
        lmtp::envelope envelope;

        envelope.sender = "alice@example.org";
        envelope.recipients.push_back("bob@example.com");
        envelope.recipients.push_back("reject@example.com");
        envelope.recipients.push_back("full@example.com");
        envelope.recipients.push_back("carol@example.com");

        return envelope;
    }

    static const char* const BODY =
        "Subject: lmtp\r\n\r\n.dot\r\nend\r\n";

    // delivered
    //   function: whether a send's report and the server's records agree
    // with the handlers: bob and carol delivered, reject refused at RCPT
    // and given no final reply, full refused after the data, and each
    // delivery asked with its place among the accepted recipients.
    static
    bool delivered(
        const lmtp::send_report& _report,
        seen&                    _seen
    )
    {
        (void)::d_mutex_lock(&_seen.mutex);

        const bool records = ( (_seen.recipients.size() == 2u)               &&
                               (_seen.recipients[0] == "bob@example.com")    &&
                               (_seen.recipients[1] == "carol@example.com")  &&
                               (_seen.places[0] == 0u)                       &&
                               (_seen.places[1] == 2u)                       &&
                               (_seen.message ==
                                "Subject: lmtp\r\n\r\n.dot\r\nend\r\n")      &&
                               (_seen.client_domain == "client.example") );

        (void)::d_mutex_unlock(&_seen.mutex);

        return ( (records)                                   &&
                 (_report.accepted == 3u)                    &&
                 (_report.delivered == 2u)                   &&
                 (_report.recipients.size() == 4u)           &&
                 (_report.deliveries.size() == 4u)           &&
                 (_report.recipients[1].code == 550u)        &&
                 (_report.deliveries[0].code == 250u)        &&
                 (_report.deliveries[1].code == 0u)          &&
                 (_report.deliveries[2].code == 452u)        &&
                 (_report.deliveries[2].address ==
                  "full@example.com")                        &&
                 (_report.deliveries[3].code == 250u)        &&
                 (_report.deliveries[3].status.to_string() ==
                  "2.0.0") );
    }

    // round
    //   function: one client session against a running server: open, send
    // the letter, quit; whether every step and the report held.
    static
    bool round(
        running&           _server,
        const std::string& _path,
        const char*        _message
    )
    {
        lmtp::client            session;
        const lmtp::void_result opened = session.open(options_for(_server,
                                                                  _path));

        // without a session there is nothing more to check
        if (!opened.has_value())
        {
            return check(false,
                         _message);
        }

        const lmtp::report_result sent = session.send(letter(),
                                                      BODY);
        const lmtp::reply_result  bye  = session.quit();

        return check( (sent.has_value()) &&
                      (bye.has_value())  &&
                      (delivered(*sent,
                                 _server.handled)),
                      _message);
    }

    // setting
    //   function: an environment variable's value, or empty; MSVC's
    // runtime is asked through _dupenv_s, which /sdl accepts.
    static
    std::string setting(
        const char* _name
    )
    {
#if defined(_MSC_VER)
        char*       value  = D_NULLPTR;
        std::size_t length = 0u;
        std::string result;

        // a copy of the value, freed here
        if ( (::_dupenv_s(&value,
                          &length,
                          _name) == 0) &&
             (value) )
        {
            result = value;
        }

        std::free(value);

        return result;
#else
        const char* const value = std::getenv(_name);

        return (value) ? std::string(value)
                       : std::string();
#endif  // _MSC_VER
    }

    // exists
    //   function: whether a path names anything, a socket included.
    static
    bool exists(
        const std::string& _path
    )
    {
#if defined(_WIN32)
        return (::_access(_path.c_str(),
                          0) == 0);
#else
        return (::access(_path.c_str(),
                         F_OK) == 0);
#endif  // _WIN32
    }

    // socket_path
    //   function: a unix-domain socket's path under TMPDIR, or /tmp, made
    // unlikely to be another run's; empty when it does not fit.
    static
    std::string socket_path()
    {
        const std::string base      = setting("TMPDIR");
        char              path[104] = { 0 };
        const int         written   = std::snprintf(
                                          path,
                                          sizeof(path),
                                          "%s/d_lmtp_cpp_%lx_%lx.sock",
                                          (!base.empty()) ? base.c_str()
                                                          : "/tmp",
                                          static_cast<unsigned long>(
                                              std::time(D_NULLPTR)),
                                          static_cast<unsigned long>(
                                              std::clock()));

        return ( (written > 0) &&
                 (static_cast<std::size_t>(written) < sizeof(path)) )
                   ? std::string(path)
                   : std::string();
    }

    // script_connection
    //   class: a net::connection that plays a scripted server and records
    // what the client wrote; reading past the script is the end of the
    // stream, as a server that hangs up.
    class script_connection : public net::connection
    {
    public:
        script_connection(
            const std::string& _script,
            std::string*       _written
        )
            : net::connection(),
              m_script(_script),
              m_position(0u),
              m_written(_written),
              m_closed(false)
        {}

        net::io_result read(
            void*       _buffer,
            std::size_t _size
        ) D_OVERRIDE
        {
            const std::size_t left  = m_script.size() - m_position;
            const std::size_t count = (left < _size) ? left : _size;

            // the script's end is the stream's, with nothing to copy
            if (count > 0u)
            {
                std::memcpy(_buffer,
                            m_script.data() + m_position,
                            count);
            }

            m_position += count;

            return net::io_result(count,
                                  net::io_error::none);
        }

        net::io_result write(
            const void* _data,
            std::size_t _size
        ) D_OVERRIDE
        {
            m_written->append(static_cast<const char*>(_data),
                              _size);

            return net::io_result(_size,
                                  net::io_error::none);
        }

        bool is_open() const D_OVERRIDE
        {
            return !m_closed;
        }

        void close() D_OVERRIDE
        {
            m_closed = true;

            return;
        }

    private:
        std::string  m_script;
        std::size_t  m_position;
        std::string* m_written;
        bool         m_closed;
    };

NS_END  // internal


/*
tests_lmtp_vocabulary
  Tests the following:
  - LMTP's options default to its dialect: no port, STARTTLS where
    offered; SMTP's still default to SMTP's
  - attach and server options are LMTP's too
  - the names LMTP shares are SMTP's own types (checked from C++11)
*/
bool tests_lmtp_vocabulary()
{
    const lmtp::client_options client;
    const lmtp::attach_options attach;
    const lmtp::server_options server;
    const smtp::client_options plain;
    bool                       result = true;

    result = internal::check( (client.dialect == lmtp::dialect::lmtp) &&
                              (client.port == 0u)                     &&
                              (client.mode ==
                               lmtp::security::starttls_optional)     &&
                              (!client.local),
                              "LMTP's client options default to LMTP's") &&
             result;
    result = internal::check( (attach.dialect == lmtp::dialect::lmtp) &&
                              (server.dialect == lmtp::dialect::lmtp) &&
                              (plain.dialect == smtp::dialect::smtp),
                              "attach and server options are LMTP's; SMTP's "
                              "stay SMTP's") && result;
    result = internal::check( (static_cast<int>(lmtp::dialect::lmtp) ==
                               static_cast<int>(D_SMTP_DIALECT_LMTP)) &&
                              (static_cast<int>(lmtp::dialect::smtp) ==
                               static_cast<int>(D_SMTP_DIALECT_SMTP)),
                              "the dialects are the kernel's") && result;
#if (D_MOVE_ENABLED == 1)
    result = internal::check( (std::is_same<lmtp::send_report,
                                            smtp::send_report>::value) &&
                              (std::is_same<lmtp::failure,
                                            smtp::failure>::value)     &&
                              (std::is_same<lmtp::result<int>,
                                            smtp::result<int> >::value),
                              "LMTP's vocabulary is SMTP's") && result;
#endif  // D_MOVE_ENABLED

    return result;
}

/*
tests_lmtp_loopback
  Tests the following:
  - an LMTP client and server over loopback TCP: each accepted recipient
    gets its own final reply, in order; one refused at RCPT gets none
  - on_deliver is asked once for each accepted recipient, with its place
    among them, and sees the message unstuffed
  - lmtp::send() delivers in one call, as smtp::send() does
*/
bool tests_lmtp_loopback()
{
    internal::running server;
    bool              result = internal::check(internal::start(server,
                                                               ""),
                                               "a loopback LMTP server");

    // with no server there is nothing to connect to
    if (!result)
    {
        internal::finish(server);

        return false;
    }

    result = internal::round(server,
                             "",
                             "each recipient's final reply crosses "
                             "loopback") && result;

    lmtp::envelope one;

    one.sender = "alice@example.org";
    one.recipients.push_back("dave@example.com");

    const lmtp::report_result once = lmtp::send(
                                         internal::options_for(server,
                                                               ""),
                                         lmtp::credentials(),
                                         one,
                                         "Subject: once\r\n\r\nx\r\n");

    result = internal::check( (once.has_value())                 &&
                              (once->delivered == 1u)            &&
                              (once->deliveries.size() == 1u)    &&
                              (once->deliveries[0].code == 250u),
                              "lmtp::send() delivers in one call") && result;
    internal::finish(server);

    return internal::check(server.outcome,
                           "the accept loop ends cleanly") && result;
}

/*
tests_lmtp_local
  Tests the following:
  - listen_local() serves LMTP on a unix-domain socket, and a client whose
    options are `local` reaches it by path
  - a server already listening is refused another listener, and an
    empty path is refused; the path is gone once the server is
*/
bool tests_lmtp_local()
{
#if (D_ENV_TCP_HAS_UNIX == 1)
    const std::string path = internal::socket_path();

    // a path that cannot fit, or no sockets: nothing to test
    if ( (path.empty()) ||
         (!::d_tcp_available()) )
    {
        std::printf("    skipped: no socket path, or no TCP backend\n");

        return true;
    }

    internal::running server;
    bool              result = internal::check(internal::start(server,
                                                               path),
                                               "an LMTP server on a "
                                               "unix-domain socket");

    // with no server there is nothing to connect to
    if (!result)
    {
        internal::finish(server);

        return false;
    }

    result = internal::round(server,
                             path,
                             "each recipient's final reply crosses the "
                             "socket") && result;

    const lmtp::server_options options;
    lmtp::server               other(options,
                                     lmtp::server_handler());
    const lmtp::void_result    twice = server.host->listen_local(path);
    const lmtp::void_result    empty = other.listen_local("");

    result = internal::check( (!twice.has_value())                         &&
                              (twice.error().code == lmtp::error::state)   &&
                              (!empty.has_value())                         &&
                              (empty.error().code ==
                               lmtp::error::invalid_argument),
                              "listening twice, and on no path, are "
                              "refused") && result;
    internal::finish(server);
    result = internal::check(!internal::exists(path),
                              "the socket's path goes with the server") &&
             result;

    return internal::check(server.outcome,
                           "the accept loop ends cleanly") && result;
#else
    std::printf("    skipped: no unix-domain sockets here\n");

    return true;
#endif  // D_ENV_TCP_HAS_UNIX
}

/*
tests_lmtp_held
  Tests the following:
  - options whose dialect was set to SMTP's still open an LMTP session:
    the LMTP server, which refuses EHLO, accepts the client's LHLO
  - over TCP, port 25 and no port are refused before connecting
  - an LMTP client against an SMTP server: LHLO refused with 500, and the
    open fails, rejected
*/
bool tests_lmtp_held()
{
    internal::running server;
    bool              result = internal::check(internal::start(server,
                                                               ""),
                                               "a loopback LMTP server");

    // with no server there is nothing to connect to
    if (!result)
    {
        internal::finish(server);

        return false;
    }

    lmtp::client_options    swapped = internal::options_for(server,
                                                            "");
    lmtp::client            session;

    swapped.dialect = smtp::dialect::smtp;
    swapped.mode    = lmtp::security::starttls_optional;

    const lmtp::void_result opened  = session.open(swapped);
    const lmtp::reply_result bye    = session.quit();

    result = internal::check( (opened.has_value()) &&
                              (bye.has_value()),
                              "the dialect is LMTP's whatever the options "
                              "say") && result;
    internal::finish(server);

    lmtp::client_options    smtp_port = internal::options_for(server,
                                                              "");
    lmtp::client_options    no_port   = internal::options_for(server,
                                                              "");

    smtp_port.port = 25u;
    no_port.port   = 0u;

    lmtp::client            refused;
    const lmtp::void_result on_25   = refused.open(smtp_port);
    const lmtp::void_result on_none = refused.open(no_port);

    result = internal::check( (!on_25.has_value())                         &&
                              (on_25.error().code ==
                               lmtp::error::invalid_argument)              &&
                              (!on_none.has_value())                       &&
                              (on_none.error().code ==
                               lmtp::error::invalid_argument),
                              "port 25 and no port are refused over TCP") &&
             result;

    // an SMTP server, which knows no LHLO
    smtp::server_options     plain;
    smtp::server             smtp_server(plain,
                                         smtp::server_handler());
    const smtp::port_result  bound = smtp_server.listen("127.0.0.1",
                                                        0u);
    dj_thread_t               thread;
    const bool               looping = ( (bound.has_value()) &&
                                         (::d_thread_create(
                                              &thread,
                                              &internal::smtp_main,
                                              &smtp_server) == 0) );

    if (looping)
    {
        lmtp::client_options    to_smtp = internal::options_for(server,
                                                                "");
        lmtp::client            stranger;

        to_smtp.port = *bound;

        const lmtp::void_result met = stranger.open(to_smtp);

        result = internal::check( (!met.has_value())                     &&
                                  (met.error().code ==
                                   lmtp::error::rejected)                &&
                                  (met.error().server_reply.code == 500u),
                                  "an SMTP server refuses LHLO, and the "
                                  "open fails") && result;
        smtp_server.stop();
        (void)::d_thread_join(thread,
                              D_NULLPTR);
    }

    return internal::check(looping,
                           "an SMTP server to try") && result;
}

/*
tests_lmtp_hang_up
  Tests the following:
  - over a scripted connection, open() says LHLO, and send pipelines the
    transaction
  - a server that hangs up after the first of two final replies: send()
    fails, closed, and last_report() keeps the reply that came and 0 for
    the other, a temporary failure (RFC 2033 section 5)
  - the report outlives close(), as last_reply() does
*/
bool tests_lmtp_hang_up()
{
    const std::string script =
        "220 lmtp.example LMTP\r\n"
        "250-lmtp.example\r\n250-PIPELINING\r\n250 ENHANCEDSTATUSCODES\r\n"
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n250 2.1.5 OK\r\n"
        "354 Go ahead\r\n"
        "250 2.0.0 <bob@example.com> Saved\r\n";
    std::string             written;
    lmtp::attach_options    options;
    lmtp::client            session;

    options.helo_domain = "c.example";

    const lmtp::void_result opened =
        session.open(new internal::script_connection(script,
                                                     &written),
                     options);

    // without a session there is nothing more to check
    if (!internal::check(opened.has_value(),
                         "open() reads the greeting and says LHLO"))
    {
        return false;
    }

    lmtp::envelope envelope;

    envelope.sender = "alice@example.org";
    envelope.recipients.push_back("bob@example.com");
    envelope.recipients.push_back("carol@example.com");

    const lmtp::report_result sent   = session.send(envelope,
                                                    "x");
    const lmtp::send_report   report = session.last_report();
    bool                      result = true;

    result = internal::check( (written.compare(0u, 16u,
                                               "LHLO c.example\r\n") == 0) &&
                              (written.find("MAIL FROM:<alice@example.org>\r\n"
                                            "RCPT TO:<bob@example.com>\r\n"
                                            "RCPT TO:<carol@example.com>\r\n"
                                            "DATA\r\n") != std::string::npos),
                              "LHLO, then one pipelined batch") && result;
    result = internal::check( (!sent.has_value())                        &&
                              (sent.error().code == lmtp::error::closed) &&
                              (report.accepted == 2u)                    &&
                              (report.delivered == 1u)                   &&
                              (report.deliveries.size() == 2u)           &&
                              (report.deliveries[0].code == 250u)        &&
                              (report.deliveries[0].address ==
                               "bob@example.com")                        &&
                              (report.deliveries[1].code == 0u)          &&
                              (report.data_code == 250u),
                              "the reply that came is kept, the other is 0") &&
             result;
    session.close();

    return internal::check( (!session.is_open()) &&
                            (session.last_report().delivered == 1u),
                            "the report outlives the session, as the last "
                            "reply does") && result;
}

#if (D_MOVE_ENABLED == 1)
/*
tests_lmtp_moves
  Tests the following:
  - connect() returns an open LMTP client, which moves with its session
  - a moved-from client is closed; the moved-to one delivers
*/
bool tests_lmtp_moves()
{
    internal::running server;
    bool              result = internal::check(internal::start(server,
                                                               ""),
                                               "a loopback LMTP server");

    // with no server there is nothing to connect to
    if (!result)
    {
        internal::finish(server);

        return false;
    }

    lmtp::result<lmtp::client> made = lmtp::client::connect(
                                          internal::options_for(server,
                                                                ""));

    result = internal::check(made.has_value(),
                             "connect() returns an open client") && result;

    // the rest needs the client
    if (made.has_value())
    {
        lmtp::client moved(static_cast<lmtp::client&&>(*made));
        lmtp::client assigned;

        assigned = static_cast<lmtp::client&&>(moved);

        const lmtp::report_result sent = assigned.send(internal::letter(),
                                                       internal::BODY);

        result = internal::check( (!made->is_open())   &&
                                  (!moved.is_open())   &&
                                  (sent.has_value())   &&
                                  (sent->delivered == 2u),
                                  "the session moves, and delivers") &&
                 result;
        (void)assigned.quit();
    }

    internal::finish(server);

    return result;
}
#endif  // D_MOVE_ENABLED

/*
tests_lmtp_live
  Tests the following, against each server the environment names:
  - an LMTP session to a real server: the mailbox that takes mail is
    delivered to; one unknown is refused at RCPT; one full is refused
    after the data, while the first is delivered
*/
bool tests_lmtp_live()
{
    const std::string accepts = internal::setting("DJINTERP_TEST_LMTP_ACCEPTS");
    const std::string refuses = internal::setting("DJINTERP_TEST_LMTP_REFUSES");
    const std::string full    = internal::setting("DJINTERP_TEST_LMTP_FULL");
    const std::string path    = internal::setting("DJINTERP_TEST_LMTP_SOCKET");
    const std::string host    = internal::setting("DJINTERP_TEST_LMTP_HOST");
    const std::string port    = internal::setting("DJINTERP_TEST_LMTP_PORT");
    std::vector<lmtp::client_options> servers;

    // a socket, and a host and port, each a server to try
    if (!path.empty())
    {
        lmtp::client_options options;

        options.host  = path;
        options.local = true;
        servers.push_back(options);
    }

    if ( (!host.empty()) &&
         (!port.empty()) )
    {
        lmtp::client_options options;

        options.host = host;
        options.port = static_cast<unsigned int>(std::strtoul(port.c_str(),
                                                              D_NULLPTR,
                                                              10));
        servers.push_back(options);
    }

    // nothing named: the run says it skipped
    if ( (accepts.empty()) ||
         (servers.empty()) )
    {
        std::printf("    skipped: DJINTERP_TEST_LMTP_ACCEPTS, and a socket "
                    "or a host and port, name no server\n");

        return true;
    }

    lmtp::envelope envelope;

    envelope.sender = "djinterp-tests@example.org";
    envelope.recipients.push_back(accepts);

    const std::size_t refused_at = (refuses.empty())
                                       ? 3u
                                       : envelope.recipients.size();

    if (!refuses.empty())
    {
        envelope.recipients.push_back(refuses);
    }

    const std::size_t full_at = (full.empty())
                                    ? 3u
                                    : envelope.recipients.size();

    if (!full.empty())
    {
        envelope.recipients.push_back(full);
    }

    bool result = true;

    // one session to each server named
    for (std::size_t i = 0u; i < servers.size(); ++i)
    {
        const lmtp::report_result sent =
            lmtp::send(servers[i],
                       lmtp::credentials(),
                       envelope,
                       "From: djinterp-tests@example.org\r\n"
                       "Subject: djinterp's C++ LMTP test\r\n\r\n"
                       "Delivered by cpp_net_lmtp_tests.\r\n");

        result = internal::check( (sent.has_value())                    &&
                                  (sent->deliveries[0].code / 100u == 2u),
                                  "the mailbox that takes mail took it") &&
                 result;

        // the rest needs the report
        if (!sent.has_value())
        {
            std::printf("    live: failure %d, reply %u\n",
                        static_cast<int>(sent.error().code),
                        sent.error().server_reply.code);

            continue;
        }

        if (refused_at < envelope.recipients.size())
        {
            result = internal::check(
                         (sent->recipients[refused_at].code / 100u == 5u) &&
                         (sent->deliveries[refused_at].code == 0u),
                         "the unknown mailbox is refused at RCPT") && result;
        }

        if (full_at < envelope.recipients.size())
        {
            const unsigned int last = sent->deliveries[full_at].code;

            result = internal::check(
                         (sent->recipients[full_at].code / 100u == 2u) &&
                         ( (last / 100u == 4u) ||
                           (last / 100u == 5u) ),
                         "the full mailbox is refused after the data, "
                         "alone") && result;
        }
    }

    return result;
}

/*
tests_lmtp_run_all
  Runs every test, reporting each, and the tally.
*/
bool tests_lmtp_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "vocabulary",         tests_lmtp_vocabulary },
        { "loopback",           tests_lmtp_loopback },
        { "unix-domain",        tests_lmtp_local },
        { "held to LMTP",       tests_lmtp_held },
        { "hung up on",         tests_lmtp_hang_up },
#if (D_MOVE_ENABLED == 1)
        { "moves",              tests_lmtp_moves },
#endif  // D_MOVE_ENABLED
        { "a real server",      tests_lmtp_live },
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

#endif  // D_INTERNAL_NET_HAS_INT64
