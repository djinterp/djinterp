/*******************************************************************************
* djinterp [net]                                                  smtp_tests.cpp
*
* Tests of the SMTP C++ facade, net/smtp/smtp.hpp.
*   A server runs on a loopback port, its accept loop on a thread of
* dmutex.h's, its handlers recording what they are shown; the tests connect
* as clients, plain, over STARTTLS, in TLS from the first byte, or over a
* net::connection of their own, which STARTTLS secures as well, and
* deliver. Two tests need no server: kernel values lifted into the
* facade's, and a session over a scripted connection. Those two and the
* STARTTLS test are the facade's first suite's (2026.09.26), carried to the
* C++98 facade. The TLS tests take their certificates from net_tests_pki.c,
* made once for the run, and pass as skipped without the OpenSSL-family
* engine.
*   The suite runs from C++98: clients open in place at every level, and the
* static connect and attach, the result alias and the moves are checked from
* C++11. Link the SMTP kernel's C sources, smtp.cpp, net.c, tcp.c, the SSL
* kernel and its OpenSSL-family engine, dmutex.c, sink_common.c, net's
* codec, and tests/djinterp/net/net_tests_pki.c; and -lssl -lcrypto.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./smtp_tests.hpp"  // corresponding header


#if (D_INTERNAL_NET_HAS_INT64 == 1)

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf, std::remove
#include <cstring>  // std::memcpy
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../../../inc/djinterp/c/sync/dmutex.h"          // d_thread_create
#include "../../../../inc/djinterp/net/ssl/ssl_openssl.h"    // the engine
#include "../../../../inc/djinterp/net/tcp/tcp.hpp"          // tcp_connector
#include "../net_tests_pki.h"                                // certificates
#if (D_MOVE_ENABLED == 1)
    // std
    #include <memory>   // std::unique_ptr
    #include <utility>  // std::move
#endif  // D_MOVE_ENABLED


NS_DJINTERP
NS_TESTING

namespace net  = ::djinterp::net;
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

    // seen
    //   struct: what the handlers were shown, under a lock of its own, as
    // they run on the sessions' threads.
    struct seen
    {
        seen()
            : sender(),
              recipients(),
              message(),
              user()
        {
            (void)::d_mutex_init(&mutex);
        }

        ~seen()
        {
            (void)::d_mutex_destroy(&mutex);
        }

        std::string              sender;
        std::vector<std::string> recipients;
        std::string              message;
        std::string              user;
        d_mutex_t                mutex;
    };

    // record_mail, record_rcpt, record_message, record_auth
    //   struct: the handlers, as function objects over a seen; a recipient
    // at refused.example is refused with 550, and only alice's secret
    // authenticates.
    struct record_mail
    {
        unsigned int
        operator()(
            const smtp::session& _session,
            const std::string&   _sender
        ) const
        {
            (void)_session;
            (void)::d_mutex_lock(&into->mutex);
            into->sender = _sender;
            (void)::d_mutex_unlock(&into->mutex);

            return 0u;
        }

        seen* into;
    };

    struct record_rcpt
    {
        unsigned int
        operator()(
            const smtp::session& _session,
            const std::string&   _recipient
        ) const
        {
            (void)_session;

            // the refused domain is refused
            if (_recipient.find("@refused.example") != std::string::npos)
            {
                return 550u;
            }

            (void)::d_mutex_lock(&into->mutex);
            into->recipients.push_back(_recipient);
            (void)::d_mutex_unlock(&into->mutex);

            return 0u;
        }

        seen* into;
    };

    struct record_message
    {
        unsigned int
        operator()(
            const smtp::session& _session,
            re_std::string_view  _message
        ) const
        {
            (void)::d_mutex_lock(&into->mutex);
            into->message.assign(_message.data(),
                                 _message.size());
            into->user = _session.user;
            (void)::d_mutex_unlock(&into->mutex);

            return 0u;
        }

        seen* into;
    };

    struct record_auth
    {
        bool
        operator()(
            const std::string& _user,
            const std::string& _secret
        ) const
        {
            return ( (_user == "alice") &&
                     (_secret == "secret") );
        }
    };

    // running
    //   struct: a server on loopback, its loop on a thread, what the loop
    // returned, and the sessions still active once it had.
    struct running
    {
        running()
            : handled(),
              host(D_NULLPTR),
              port(0u),
              outcome(false),
              remaining(0u),
              thread(),
              started(false)
        {}

        seen          handled;
        smtp::server* host;
        unsigned int  port;
        bool          outcome;
        std::size_t   remaining;
        d_thread_t    thread;
        bool          started;
    };

    // run_main
    //   function: the accept loop's thread.
    static d_thread_result_t
    run_main(
        void* _running
    )
    {
        running* const          work   = static_cast<running*>(_running);
        const smtp::void_result result = work->host->run();

        work->outcome = result.has_value();

        return D_THREAD_SUCCESS;
    }

    // options_for
    //   function: a plain-text client to the running server.
    static smtp::client_options
    options_for(
        const running& _server
    )
    {
        smtp::client_options options;

        options.host                = "127.0.0.1";
        options.port                = _server.port;
        options.mode                = smtp::security::none;
        options.helo_domain         = "client.example";
        options.allow_insecure_auth = true;
        options.io_timeout_ms       = 5000;

        return options;
    }

    // start, finish
    //   function: a server with the recording handlers, listening and
    // running -- by default a plain one that allows AUTH in the clear --
    // then stopped, its loop joined, and the sessions it left counted.
    static bool
    start(
        running&                    _server,
        const smtp::server_options& _options
    )
    {
        smtp::server_handler handler;

        handler.on_mail    = record_mail();
        handler.on_rcpt    = record_rcpt();
        handler.on_message = record_message();
        handler.on_auth    = record_auth();

        // the handlers record into the server's own `seen`
        handler.on_mail.target<record_mail>()->into       = &_server.handled;
        handler.on_rcpt.target<record_rcpt>()->into       = &_server.handled;
        handler.on_message.target<record_message>()->into = &_server.handled;

        _server.host = new smtp::server(_options,
                                        handler);

        const smtp::port_result bound = _server.host->listen("127.0.0.1",
                                                             0u);

        _server.port    = (bound.has_value()) ? *bound : 0u;
        _server.started = ( (bound.has_value()) &&
                            (::d_thread_create(&_server.thread,
                                               &run_main,
                                               &_server) == 0) );

        return _server.started;
    }

    static bool
    start(
        running& _server
    )
    {
        smtp::server_options options;

        options.domain              = "server.example";
        options.allow_insecure_auth = true;

        return start(_server,
                     options);
    }

    static void
    finish(
        running& _server
    )
    {
        // only a running loop is stopped, joined, and asked what it left
        if (_server.started)
        {
            _server.host->stop();
            (void)::d_thread_join(_server.thread,
                                  D_NULLPTR);
            _server.remaining = _server.host->active_sessions();
        }

        delete _server.host;
        _server.host = D_NULLPTR;

        return;
    }

    // letter
    //   function: an envelope from alice to two recipients, one of them at
    // the refused domain when asked.
    static smtp::envelope
    letter(
        bool _with_refused
    )
    {
        smtp::envelope envelope;

        envelope.sender = "alice@client.example";
        envelope.recipients.push_back("bob@server.example");
        envelope.recipients.push_back((_with_refused)
                                      ? "eve@refused.example"
                                      : "carol@server.example");

        return envelope;
    }

    // BODY
    //   constant: the message the tests send.
    static const char* const BODY =
        "Subject: hello\r\n\r\nA message from the C++ facade.\r\n";

    // certificates
    //   struct: the run's certificates, and whether they were made: 0 not
    // yet tried, 1 made, 2 not to be had.
    struct certificates
    {
        ::d_tests_net_pki files;
        int               state;
    };

    // run_certificates
    //   function: the one record of the run's certificates.
    static certificates&
    run_certificates() D_NOEXCEPT
    {
        static certificates instance;

        return instance;
    }

    // pki, drop_pki
    //   function: the run's certificates, made the first time a test asks
    // for them; null without the engine, or when they cannot be made. And
    // their removal, at the end of the run.
    static const ::d_tests_net_pki*
    pki() D_NOEXCEPT
    {
        certificates& run = run_certificates();

        // the first ask makes them, or finds they cannot be made
        if (run.state == 0)
        {
            run.state = ( (::d_ssl_engine_openssl() != D_NULLPTR) &&
                          (::d_tests_net_pki_make(&run.files)) ) ? 1 : 2;
        }

        return (run.state == 1) ? &run.files : D_NULLPTR;
    }

    static void
    drop_pki() D_NOEXCEPT
    {
        certificates& run = run_certificates();

        // only certificates made are removed
        if (run.state == 1)
        {
            ::d_tests_net_pki_remove(&run.files);
        }

        run.state = 2;

        return;
    }

    // tls_server
    //   function: options for a server presenting the run's certificate,
    // requiring TLS and AUTH, implicitly or by STARTTLS.
    static smtp::server_options
    tls_server(
        const ::d_tests_net_pki* _files,
        bool                     _implicit
    )
    {
        smtp::server_options options;

        options.domain               = "mail.example.com";
        options.tls.certificate_file = _files->server_cert;
        options.tls.private_key_file = _files->server_key;
        options.implicit_tls         = _implicit;
        options.require_tls          = true;
        options.require_auth         = true;
        options.io_timeout_ms        = 10000;

        return options;
    }

    // tls_client
    //   function: options for a client of `_server` trusting the run's CA
    // alone, by `_mode`, checking the certificate against `_name` -- the
    // host, when empty.
    static smtp::client_options
    tls_client(
        const running&           _server,
        const ::d_tests_net_pki* _files,
        smtp::security::value    _mode,
        const std::string&       _name
    )
    {
        smtp::client_options options;

        options.host            = "127.0.0.1";
        options.port            = _server.port;
        options.mode            = _mode;
        options.tls.ca_file     = _files->ca;
        options.tls.server_name = _name;
        options.helo_domain     = "client.example";
        options.io_timeout_ms   = 10000;

        return options;
    }

    // alice
    //   function: the credentials the handlers accept.
    static smtp::credentials
    alice()
    {
        smtp::credentials credentials;

        credentials.user   = "alice";
        credentials.secret = "secret";
        credentials.method = smtp::mechanism::automatic;

        return credentials;
    }

    // script_connection
    //   class: a net::connection that plays a scripted server and records
    // what the client wrote, and whether it was closed; reading past the
    // script is the end of the stream.
    class script_connection : public net::connection
    {
    public:
        script_connection(
            const std::string& _script,
            std::string*       _written,
            bool*              _closed
        )
            : net::connection(),
              m_script(_script),
              m_position(0u),
              m_written(_written),
              m_closed(_closed)
        {}

        net::io_result
        read(
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

        net::io_result
        write(
            const void* _data,
            std::size_t _size
        ) D_OVERRIDE
        {
            m_written->append(static_cast<const char*>(_data),
                              _size);

            return net::io_result(_size,
                                  net::io_error::none);
        }

        bool
        is_open() const D_OVERRIDE
        {
            return !*m_closed;
        }

        void
        close() D_OVERRIDE
        {
            *m_closed = true;

            return;
        }

    private:
        std::string  m_script;
        std::size_t  m_position;
        std::string* m_written;
        bool*        m_closed;
    };

NS_END  // internal


/*
tests_smtp_vocabulary
  Tests the following:
  - each enumerator is the kernel's value, and an error has a name
  - a default status is absent, and a kernel status prints as x.y.z
  - a reply's class follows its first digit
  - to_net is d_smtp_error_to_net, error for error
*/
bool
tests_smtp_vocabulary()
{
    ::d_smtp_status raw    = ::d_smtp_status();
    bool            mapped = true;
    bool            result = true;

    // each error through the C++ face and the C one
    for (int i = 0; i <= static_cast<int>(D_SMTP_ERROR_INSECURE); ++i)
    {
        const ::d_smtp_error error = static_cast< ::d_smtp_error>(i);

        mapped = ( (mapped) &&
                   (net::to_c(smtp::to_net(static_cast<smtp::error::value>(
                        i))) ==
                    ::d_smtp_error_to_net(error)) );
    }

    raw.status_class = 5u;
    raw.subject      = 1u;
    raw.detail       = 1u;

    smtp::reply ok;

    ok.code = 250u;
    result  = internal::check( (static_cast<int>(smtp::error::timed_out) ==
                                D_SMTP_ERROR_TIMED_OUT)   &&
                               (static_cast<int>(smtp::security::starttls) ==
                                D_SMTP_SECURITY_STARTTLS) &&
                               (smtp::to_string(smtp::error::none) !=
                                D_NULLPTR),
                               "the enumerators are the kernel's") && result;
    result  = internal::check( (!smtp::status().present()) &&
                               (smtp::status(raw).to_string() == "5.1.1"),
                               "statuses print as x.y.z") && result;
    result  = internal::check( (ok.positive())   &&
                               (!ok.transient()) &&
                               (!ok.permanent()),
                               "a 250 is positive") && result;
    result  = internal::check(mapped,
                              "errors map onto io_error as onto d_net_error")
              && result;

    return result;
}

/*
tests_smtp_lifting
  Tests the following:
  - a kernel reply lifts to owned lines, text, status and class
  - kernel capabilities answer by scoped enumerator
  - an error's name is the kernel's own
*/
bool
tests_smtp_lifting()
{
    ::d_smtp_reply        raw     = ::d_smtp_reply();
    ::d_smtp_capabilities offered = ::d_smtp_capabilities();
    bool                  result  = true;

    ::d_smtp_reply_clear(&raw);
    (void)::d_smtp_reply_feed(&raw,
                              "250-2.1.0 first",
                              15u);
    (void)::d_smtp_reply_feed(&raw,
                              "250 second",
                              10u);

    const smtp::reply lifted(raw);

    result = internal::check( (lifted.code == 250u)                    &&
                              (lifted.lines.size() == 2u)              &&
                              (lifted.text() == "2.1.0 first\nsecond") &&
                              (lifted.status.to_string() == "2.1.0")   &&
                              (lifted.positive())                      &&
                              (!lifted.permanent()),
                              "a kernel reply lifts line by line") && result;

    offered.extensions = D_SMTP_EXTENSION_PIPELINING;
    offered.auth       = D_SMTP_AUTH_PLAIN;

    const smtp::capabilities features(offered);

    result = internal::check( (features.supports(
                                   smtp::extension::pipelining))        &&
                              (!features.supports(
                                   smtp::extension::starttls))          &&
                              (features.offers(smtp::mechanism::plain)) &&
                              (!features.offers(smtp::mechanism::login)),
                              "capabilities answer by enumerator") && result;
    result = internal::check( (static_cast<int>(smtp::error::rejected) ==
                               D_SMTP_ERROR_REJECTED) &&
                              (std::string(smtp::to_string(
                                   smtp::error::tls)) ==
                               ::d_smtp_error_string(D_SMTP_ERROR_TLS)),
                              "an error's name is the kernel's") && result;

    return result;
}

/*
tests_smtp_delivery
  Tests the following:
  - a client opened in place reaches the server and delivers to two
    recipients; the handlers see the sender, both, and the message
  - a recipient a handler refuses is reported with its code, the rest
    delivered
  - the session quits, and a closed client refuses commands
*/
bool
tests_smtp_delivery()
{
    internal::running server;
    bool              result = true;

    const bool started = internal::start(server);

    smtp::client            first;
    const smtp::void_result opened = first.open(internal::options_for(server));
    const smtp::report_result sent =
        (opened.has_value())
            ? first.send(internal::letter(false),
                         internal::BODY)
            : smtp::report_result(::re_std::unexpected<smtp::failure>(
                                      smtp::failure()));
    const smtp::reply_result  bye  = first.quit();

    result = internal::check( (started)            &&
                              (opened.has_value()) &&
                              (first.features().supports(
                                   smtp::extension::eight_bit_mime)),
                              "a client opens in place") && result;
    result = internal::check( (sent.has_value())        &&
                              (sent->accepted == 2u)    &&
                              (sent->data_code == 250u) &&
                              (sent->recipients.size() == 2u),
                              "two recipients take the message") && result;
    result = internal::check( (bye.has_value()) &&
                              (bye->code == 221u),
                              "the session quits") && result;

    smtp::client            second;
    const smtp::void_result reopened =
        second.open(internal::options_for(server));
    const smtp::report_result partial =
        (reopened.has_value())
            ? second.send(internal::letter(true),
                          internal::BODY)
            : smtp::report_result(::re_std::unexpected<smtp::failure>(
                                      smtp::failure()));

    (void)second.quit();
    second.close();

    const smtp::reply_result closed = second.noop();

    internal::finish(server);
    result = internal::check( (partial.has_value())              &&
                              (partial->accepted == 1u)          &&
                              (partial->recipients.size() == 2u) &&
                              (partial->recipients[1].code == 550u),
                              "a refused recipient is reported") && result;
    result = internal::check( (server.handled.sender ==
                               "alice@client.example")                 &&
                              (server.handled.recipients.size() == 3u) &&
                              (server.handled.message.find(
                                   "A message from the C++ facade.") !=
                               std::string::npos),
                              "the handlers saw the sender, recipients and "
                              "message") && result;
    result = internal::check( (!closed.has_value()) &&
                              (closed.error().code == smtp::error::state),
                              "a closed client refuses commands") && result;
    result = internal::check(server.outcome,
                             "the server's loop ends cleanly") && result;

    return result;
}

/*
tests_smtp_attach
  Tests the following:
  - a client opened over a net::connection of the caller's (TCP here) runs
    the session through the bridge and delivers
  - over that plain stream AUTH is refused as insecure, the default
  - opened with attach_options allowing it, an attached session
    authenticates over the plain stream (decision 107 of the register)
  - a client opened with insecure AUTH allowed authenticates, and the
    handlers see the user; wrong credentials are refused
  It runs past 75 lines because one running server and its stream serve
every check.
*/
bool
tests_smtp_attach()
{
    internal::running  server;
    net::tcp_connector connector;
    net::open_result   stream;
    bool               result = true;

    const bool started = internal::start(server);
    const bool linked  = ( (started) &&
                           (connector.connect(
                                net::endpoint("127.0.0.1",
                                              static_cast<net::port_type>(
                                                  server.port)),
                                stream) == net::io_error::none) );

    smtp::client            bridged;
    const smtp::void_result opened =
        (linked) ? bridged.open(stream.release(),
                                false,
                                "bridge.example")
                 : smtp::void_result(::re_std::unexpected<smtp::failure>(
                                         smtp::failure()));
    smtp::credentials       alice;

    alice.user   = "alice";
    alice.secret = "secret";
    alice.method = smtp::mechanism::plain;

    const smtp::reply_result  plain = bridged.authenticate(alice);
    const smtp::report_result sent  = bridged.send(internal::letter(false),
                                                   internal::BODY);

    (void)bridged.quit();
    result = internal::check( (opened.has_value()) &&
                              (sent.has_value())   &&
                              (sent->accepted == 2u),
                              "over the caller's connection, delivery") &&
             result;
    result = internal::check( (!plain.has_value())                          &&
                              (plain.error().code == smtp::error::insecure) &&
                              (!bridged.is_authenticated()),
                              "AUTH over a plain attached stream is refused") &&
             result;

    net::open_result     restream;
    smtp::attach_options allowing;

    allowing.helo_domain         = "bridge.example";
    allowing.allow_insecure_auth = true;

    const bool relinked = ( (started) &&
                            (connector.connect(
                                 net::endpoint("127.0.0.1",
                                               static_cast<net::port_type>(
                                                   server.port)),
                                 restream) == net::io_error::none) );

    smtp::client             allowed;
    const smtp::void_result  reopened =
        (relinked) ? allowed.open(restream.release(),
                                  allowing)
                   : smtp::void_result(::re_std::unexpected<smtp::failure>(
                                           smtp::failure()));
    const smtp::reply_result through  = allowed.authenticate(alice);

    (void)allowed.quit();
    result = internal::check( (reopened.has_value()) &&
                              (through.has_value())  &&
                              (allowed.is_authenticated()),
                              "attach_options allowing it: AUTH over the "
                              "plain attached stream") && result;

    smtp::client            trusted;
    smtp::client            refused;
    smtp::credentials       mallory;
    const smtp::void_result first  =
        trusted.open(internal::options_for(server));
    const smtp::reply_result signed_in = trusted.authenticate(alice);
    const smtp::report_result again    = trusted.send(internal::letter(false),
                                                      internal::BODY);

    (void)trusted.quit();
    mallory.user   = "mallory";
    mallory.secret = "guess";
    mallory.method = smtp::mechanism::plain;

    const smtp::void_result  second = refused.open(
                                          internal::options_for(server));
    const smtp::reply_result denied = refused.authenticate(mallory);

    refused.close();
    internal::finish(server);
    result = internal::check( (first.has_value())     &&
                              (signed_in.has_value()) &&
                              (again.has_value())     &&
                              (server.handled.user == "alice"),
                              "insecure AUTH allowed: alice signs in, and "
                              "the handler sees her") && result;
    result = internal::check( (second.has_value())  &&
                              (!denied.has_value()) &&
                              (denied.error().code == smtp::error::auth),
                              "wrong credentials are refused") && result;

    return result;
}

/*
tests_smtp_script
  Tests the following:
  - open() over a scripted, already secure net::connection reads the
    greeting and says EHLO
  - AUTH PLAIN, a pipelined send with one refused recipient, and QUIT
  - the report lifts per recipient, its enhanced status too; the features
    stay visible after QUIT, and close() closes the connection
*/
bool
tests_smtp_script()
{
    const std::string script =
        "220 mail.example.com ESMTP\r\n"
        "250-mail.example.com\r\n250-PIPELINING\r\n250 AUTH PLAIN\r\n"
        "235 2.7.0 OK\r\n"
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n550 5.1.1 No such user\r\n"
        "354 Go ahead\r\n250 2.0.0 Queued\r\n"
        "221 2.0.0 Bye\r\n";
    std::string             written;
    bool                    closed = false;
    smtp::client            session;
    const smtp::void_result opened =
        session.open(new internal::script_connection(script,
                                                     &written,
                                                     &closed),
                     true,
                     "c.example");

    // without a session there is nothing more to check
    if (!internal::check(opened.has_value(),
                         "open() reads the greeting and says EHLO"))
    {
        return false;
    }

    smtp::credentials alice;
    smtp::envelope    envelope;

    alice.user      = "alice";
    alice.secret    = "s3cret";
    alice.method    = smtp::mechanism::plain;
    envelope.sender = "alice@example.org";
    envelope.recipients.push_back("bob@example.com");
    envelope.recipients.push_back("nobody@example.com");

    const smtp::reply_result  signed_in = session.authenticate(alice);
    const smtp::report_result sent      = session.send(
                                              envelope,
                                              "Subject: x\r\n\r\nhello\r\n");
    const smtp::reply_result  bye       = session.quit();
    bool                      result    = true;

    result = internal::check( (signed_in.has_value())   &&
                              (signed_in->code == 235u) &&
                              (session.is_authenticated()),
                              "AUTH PLAIN succeeds") && result;
    result = internal::check( (sent.has_value())                 &&
                              (sent->accepted == 1u)             &&
                              (sent->recipients.size() == 2u)    &&
                              (sent->recipients[1].code == 550u) &&
                              (sent->recipients[1].status.to_string() ==
                               "5.1.1")                          &&
                              (sent->data_code == 250u),
                              "the report lifts per recipient") && result;
    result = internal::check( (bye.has_value()) &&
                              (session.features().supports(
                                   smtp::extension::pipelining)),
                              "QUIT, and the features stay visible") && result;
    session.close();
    result = internal::check( (closed)             &&
                              (!session.is_open()) &&
                              (written.find("MAIL FROM:<alice@example.org>\r\n"
                                            "RCPT TO:<bob@example.com>\r\n"
                                            "RCPT TO:<nobody@example.com>\r\n"
                                            "DATA\r\n") != std::string::npos),
                              "one pipelined batch, then the connection "
                              "closes") && result;

    return result;
}

/*
tests_smtp_send_once
  Tests the following:
  - smtp::send connects, delivers and quits in one call
  - a port nothing listens on is a connect failure
*/
bool
tests_smtp_send_once()
{
    internal::running server;
    smtp::credentials nobody;
    bool              result = true;

    const bool                started = internal::start(server);
    const smtp::client_options options = internal::options_for(server);
    const smtp::report_result sent    = smtp::send(options,
                                                   nobody,
                                                   internal::letter(false),
                                                   internal::BODY);

    internal::finish(server);

    const smtp::report_result missing = smtp::send(options,
                                                   nobody,
                                                   internal::letter(false),
                                                   internal::BODY);

    result = internal::check( (started)          &&
                              (sent.has_value()) &&
                              (sent->accepted == 2u),
                              "one call delivers") && result;
    result = internal::check( (!missing.has_value()) &&
                              (missing.error().code == smtp::error::connect),
                              "nothing listening is a connect failure") &&
             result;

    return result;
}

/*
tests_smtp_listen
  Tests the following:
  - a port past 65535 is refused, and run() without a listener is out of
    sequence
  - a server listens once, on the port it reports; a second listen is out
    of sequence
  - implicit TLS with neither a certificate nor a context is refused
  - a certificate that will not load is a TLS failure -- an unsupported one
    without the engine -- before anything is bound
*/
bool
tests_smtp_listen()
{
    smtp::server_options options;
    smtp::server_options implicit;
    smtp::server_options missing;
    smtp::server_handler handler;
    bool                 result = true;

    implicit.implicit_tls        = true;
    missing.tls.certificate_file = "/nonexistent/server.pem";
    missing.tls.private_key_file = "/nonexistent/server.key";

    smtp::server            plain(options,
                                  handler);
    smtp::server            unready(implicit,
                                    handler);
    smtp::server            unloadable(missing,
                                       handler);
    const smtp::port_result far    = plain.listen("127.0.0.1",
                                                  70000u);
    const smtp::void_result early  = plain.run();
    const smtp::port_result first  = plain.listen("127.0.0.1",
                                                  0u);
    const smtp::port_result second = plain.listen("127.0.0.1",
                                                  0u);
    const smtp::port_result no_tls = unready.listen("127.0.0.1",
                                                    0u);
    const smtp::port_result broken = unloadable.listen("127.0.0.1",
                                                       0u);

    result = internal::check( (!far.has_value())              &&
                              (far.error().code ==
                               smtp::error::invalid_argument) &&
                              (!early.has_value())            &&
                              (early.error().code == smtp::error::state),
                              "a port past 65535; run() before listen()")
             && result;
    result = internal::check( (first.has_value())      &&
                              (*first != 0u)           &&
                              (*first == plain.port()) &&
                              (!second.has_value())    &&
                              (second.error().code == smtp::error::state),
                              "one listen, on the port reported") && result;
    result = internal::check( (!no_tls.has_value()) &&
                              (no_tls.error().code ==
                               smtp::error::invalid_argument),
                              "implicit TLS needs a certificate") && result;
    result = internal::check( (!broken.has_value()) &&
                              (broken.error().code ==
                               ((::d_ssl_engine_openssl())
                                ? smtp::error::tls
                                : smtp::error::unsupported)),
                              "a certificate that will not load") && result;

    return result;
}

/*
tests_smtp_starttls
  Tests the following:
  - smtp::send over STARTTLS, checked against "localhost", with AUTH, to a
    server that requires both
  - the handler sees the message unstuffed, and the authenticated user
  - a certificate for another name fails with error::tls
  - stop() ends run(), which waits for its sessions
  Passes as skipped without the engine or the certificates.
*/
bool
tests_smtp_starttls()
{
    const ::d_tests_net_pki* const files = internal::pki();

    // without the engine, or its certificates, there is no TLS to test
    if (!files)
    {
        return internal::check(true,
                               "skipped: no engine, or no certificates");
    }

    internal::running    server;
    smtp::envelope       envelope;
    const bool           started  = internal::start(
                                        server,
                                        internal::tls_server(files,
                                                             false));
    smtp::client_options settings = internal::tls_client(
                                        server,
                                        files,
                                        smtp::security::starttls,
                                        "localhost");

    envelope.sender = "alice@example.org";
    envelope.recipients.push_back("bob@example.com");

    const smtp::report_result sent = smtp::send(
                                         settings,
                                         internal::alice(),
                                         envelope,
                                         "Subject: facade\r\n\r\n.dot\r\n");

    settings.tls.server_name = "wrong.example";

    const smtp::report_result refused = smtp::send(settings,
                                                   internal::alice(),
                                                   envelope,
                                                   "x\r\n");

    internal::finish(server);

    bool result = true;

    result = internal::check( (started)                           &&
                              (sent.has_value())                  &&
                              (sent->accepted == 1u)              &&
                              (server.handled.message ==
                               "Subject: facade\r\n\r\n.dot\r\n") &&
                              (server.handled.user == "alice"),
                              "STARTTLS, AUTH and delivery through the "
                              "facade") && result;
    result = internal::check( (!refused.has_value()) &&
                              (refused.error().code == smtp::error::tls),
                              "a certificate for another name is refused") &&
             result;
    result = internal::check( (server.outcome) &&
                              (server.remaining == 0u),
                              "run() waited for its sessions") && result;

    return result;
}

/*
tests_smtp_attached_tls
  STARTTLS over a TCP net::connection the test opened and handed over: the
client checks the certificate against "localhost", then authenticates,
delivers and quits.
*/
static bool
tests_smtp_attached_tls(
    internal::running&       _server,
    const ::d_tests_net_pki* _files
)
{
    net::tcp_connector connector;
    net::open_result   stream;
    smtp::tls_options  tls;
    smtp::client       session;

    tls.ca_file     = _files->ca;
    tls.server_name = "localhost";

    const bool               linked   =
        (connector.connect(net::endpoint("127.0.0.1",
                                         static_cast<net::port_type>(
                                             _server.port)),
                           stream) == net::io_error::none);
    const smtp::void_result  opened   =
        (linked) ? session.open(stream.release(),
                                false,
                                "bridge.example")
                 : smtp::void_result(::re_std::unexpected<smtp::failure>(
                                         smtp::failure()));
    const smtp::reply_result upgraded = session.starttls(tls);
    const bool               secure   = session.is_secure();
    const smtp::reply_result signed_in = session.authenticate(
                                             internal::alice());
    const smtp::report_result sent    = session.send(
                                            internal::letter(false),
                                            internal::BODY);

    (void)session.quit();

    return ( (opened.has_value())    &&
             (upgraded.has_value())  &&
             (secure)                &&
             (signed_in.has_value()) &&
             (sent.has_value())      &&
             (sent->accepted == 2u) );
}

/*
tests_smtp_lent_context
  A client given a tls_context's lease -- the context trusting the run's CA,
made in C++ -- upgrades with it and signs in, after the tls_context itself
has gone: the lease keeps the live context (phase 3's finding).
*/
                                            static bool
                                            tests_smtp_lent_context(
    const internal::running& _server,
    const ::d_tests_net_pki* _files
)
                                            {
    smtp::client_options options = internal::tls_client(
                                       _server,
                                       _files,
                                       smtp::security::starttls,
                                       "localhost");

    options.tls.ca_file = "/nonexistent/ca.pem";

    // the context goes first; only the lease is left holding it
    {
        net::tls_context context(net::tls_context::role::client);

        if (context.use_verify_file(_files->ca) != net::io_error::none)
        {
            return false;
        }

        options.tls.lease = context.lease();
    }

    smtp::client            session;
    const smtp::void_result opened = session.open(options);
    const bool              lent   = (options.tls.lease.get() != D_NULLPTR);

    return ( (lent)                                                &&
             (opened.has_value())                                  &&
             (session.is_secure())                                 &&
             (session.authenticate(internal::alice()).has_value()) &&
             (session.quit().has_value()) );
}

/*
tests_smtp_own_context
  A client that brings a context of its own -- the OpenSSL-family engine's,
trusting the run's CA, made here -- upgrades with it, and delivers.
*/
static bool
tests_smtp_own_context(
    const internal::running& _server,
    const ::d_tests_net_pki* _files
)
{
    ::d_ssl_config  config = ::d_ssl_config();
    ::d_ssl_context context;

    ::d_ssl_config_init(&config,
                        D_SSL_ROLE_CLIENT);
    config.use_system_trust = false;
    config.ca_file          = _files->ca;

    // a context of the test's own, or nothing to test
    if (::d_ssl_context_init(&context,
                             ::d_ssl_engine_openssl(),
                             &config) != D_SSL_OK)
    {
        return false;
    }

    smtp::client_options options = internal::tls_client(
                                       _server,
                                       _files,
                                       smtp::security::starttls,
                                       "localhost");
    bool                 secure  = false;

    options.tls.ca_file = "/nonexistent/ca.pem";
    options.tls.context = &context;

    // the client goes before the context its session was made from
    {
        smtp::client            session;
        const smtp::void_result opened = session.open(options);

        secure = ( (opened.has_value())                                  &&
                   (session.is_secure())                                 &&
                   (session.authenticate(internal::alice()).has_value()) &&
                   (session.quit().has_value()) );
    }

    ::d_ssl_context_destroy(&context);

    return secure;
}

/*
tests_smtp_tls_paths
  Tests the following:
  - STARTTLS over a net::connection of the caller's, through net.hpp's
    adapter: checked, then AUTH and delivery
  - a context of the caller's own stands in for the files: one naming a CA
    file that does not exist is not consulted
  - implicit TLS, checked against the address connected to, to a server
    speaking TLS from the first byte
  Passes as skipped without the engine or the certificates.
*/
bool
tests_smtp_tls_paths()
{
    const ::d_tests_net_pki* const files = internal::pki();

    // without the engine, or its certificates, there is no TLS to test
    if (!files)
    {
        return internal::check(true,
                               "skipped: no engine, or no certificates");
    }

    internal::running server;
    internal::running implicit;
    smtp::envelope    envelope;
    bool              result  = true;
    const bool        started = internal::start(
                                    server,
                                    internal::tls_server(files,
                                                         false));

    result = internal::check( (started) &&
                              (tests_smtp_attached_tls(server,
                                                       files)),
                              "STARTTLS over the caller's connection") &&
             result;
    result = internal::check(tests_smtp_own_context(server,
                                                    files),
                             "a context of the caller's own") && result;
    result = internal::check(tests_smtp_lent_context(server,
                                                     files),
                             "a tls_context's lease") && result;
    internal::finish(server);

    const bool                started2 = internal::start(
                                             implicit,
                                             internal::tls_server(files,
                                                                  true));
    const smtp::report_result sent     = smtp::send(
                                             internal::tls_client(
                                                 implicit,
                                                 files,
                                                 smtp::security::tls,
                                                 ""),
                                             internal::alice(),
                                             internal::letter(false),
                                             internal::BODY);

    internal::finish(implicit);
    result = internal::check( (started2)             &&
                              (sent.has_value())     &&
                              (sent->accepted == 2u) &&
                              (implicit.handled.user == "alice"),
                              "implicit TLS, checked against 127.0.0.1") &&
             result;

    return result;
}

#if (D_MOVE_ENABLED == 1)
/*
tests_smtp_moves
  Tests the following:
  - client::connect and client::attach return open clients by value
  - a client moves, the source closing
  - result<T> names the same types as the named results
*/
bool
tests_smtp_moves()
{
    static_assert(std::is_same<smtp::result<smtp::reply>,
                  smtp::reply_result>::value,
                  "result<reply> is reply_result");

    internal::running  server;
    net::tcp_connector connector;
    bool               result = true;

    const bool                 started  = internal::start(server);
    smtp::result<smtp::client> direct   =
        smtp::client::connect(internal::options_for(server));
    net::open_result           stream   =
        connector.connect(net::endpoint("127.0.0.1",
                                        static_cast<net::port_type>(
                                            server.port)));
    smtp::result<smtp::client> attached =
        smtp::client::attach(std::unique_ptr<net::connection>(
                                 stream.release()),
                             false,
                             "moves.example");

    result = internal::check( (started)              &&
                              (direct.has_value())   &&
                              (direct->is_open())    &&
                              (attached.has_value()) &&
                              (attached->is_open()),
                              "connect and attach return open clients") &&
             result;

    smtp::client moved(std::move(*direct));

    result = internal::check( (moved.is_open())    &&
                              (!direct->is_open()) &&
                              (moved.quit().has_value()),
                              "a client moves") && result;
    (void)attached->quit();
    internal::finish(server);

    return result;
}
#endif  // D_MOVE_ENABLED

/*
tests_smtp_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_smtp_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "vocabulary",       tests_smtp_vocabulary },
        { "lifting",          tests_smtp_lifting },
        { "delivery",         tests_smtp_delivery },
        { "attach",           tests_smtp_attach },
        { "attach, scripted", tests_smtp_script },
        { "send once",        tests_smtp_send_once },
        { "listen",           tests_smtp_listen },
        { "STARTTLS",         tests_smtp_starttls },
        { "TLS paths",        tests_smtp_tls_paths },
#if (D_MOVE_ENABLED == 1)
        { "moves",            tests_smtp_moves },
#endif  // D_MOVE_ENABLED
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

    internal::drop_pki();
    std::printf("%lu/%lu checks\n",
                static_cast<unsigned long>(internal::counts().checks -
                                           internal::counts().failures),
                static_cast<unsigned long>(internal::counts().checks));

    return all;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_INTERNAL_NET_HAS_INT64
