/*******************************************************************************
* djinterp [net]                                                  smtp_tests.cpp
*
* djinterp SMTP C++ facade tests.
*   The facade is thin by design, so these tests aim at its seams: values
* lifted from the kernel, the net::connection bridge (EOF, writes, close),
* result propagation, and the server's thread-per-session loop with
* std::function handlers. The certificate fixture is shared with the C
* suite, whose session tests object must be linked in.
*
*
* path:      /tests/djinterp/net/smtp/smtp_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "./smtp_tests.hpp"
// std
#include <cstddef>      // std::size_t
#include <cstdio>       // std::printf, std::remove
#include <cstring>      // std::memcpy
#include <memory>       // std::make_unique
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <thread>       // std::thread
#include <utility>      // std::move


// d_tests_sa_smtp_certificate
//   fixture: the C suite's certificate maker (smtp_tests_sa_session.c).
extern "C" bool d_tests_sa_smtp_certificate(char*       _certificate,
                                            char*       _key,
                                            std::size_t _capacity);


NS_DJINTERP
NS_TESTING

namespace smtp = ::djinterp::net::smtp;

NS_INTERNAL

    // tally
    //   struct: checks run and failed across the suite.
    struct tally
    {
        std::size_t checks   = 0u;
        std::size_t failures = 0u;
    };

    // counts
    //   function: the suite's tally.
    tally&
    counts() noexcept
    {
        static tally instance;

        return instance;
    }

    // check
    //   function: records one check, printing it if it failed.
    bool
    check(
        bool        _condition,
        const char* _message
    ) noexcept
    {
        counts().checks += 1u;

        if (!_condition)
        {
            counts().failures += 1u;
            std::printf("    FAIL %s\n",
                        _message);
        }

        return _condition;
    }

    // script_connection
    //   class: a net::connection that plays a scripted server and records
    // what the client wrote; reading past the script is end of stream.
    class script_connection : public net::connection
    {
    public:
        script_connection(
            std::string  _script,
            std::string* _written,
            bool*        _closed
        )
            : m_script(std::move(_script)),
              m_position(0u),
              m_written(_written),
              m_closed(_closed)
        {}

        net::io_result
        read(
            void*       _buffer,
            std::size_t _size
        ) override
        {
            const std::size_t left  = m_script.size() - m_position;
            const std::size_t count = (left < _size) ? left : _size;

            std::memcpy(_buffer,
                        m_script.data() + m_position,
                        count);
            m_position += count;

            return net::io_result(count,
                                  net::io_error::none);
        }

        net::io_result
        write(
            const void* _data,
            std::size_t _size
        ) override
        {
            m_written->append(static_cast<const char*>(_data),
                              _size);

            return net::io_result(_size,
                                  net::io_error::none);
        }

        bool
        is_open() const override
        {
            return !*m_closed;
        }

        void
        close() override
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
  - a kernel reply lifts to owned lines, text, status and class
  - capabilities answer by scoped enumerator
  - enumerators carry the kernel's values, and to_string is the kernel's
*/
bool
tests_smtp_vocabulary()
{
    ::d_smtp_reply        raw     = {};
    ::d_smtp_capabilities offered = {};
    bool                  result  = true;

    ::d_smtp_reply_clear(&raw);
    (void)::d_smtp_reply_feed(&raw, "250-2.1.0 first", 15u);
    (void)::d_smtp_reply_feed(&raw, "250 second", 10u);

    const smtp::reply lifted(raw);

    result = internal::check( (lifted.code == 250u) &&
                              (lifted.lines.size() == 2u) &&
                              (lifted.text() == "2.1.0 first\nsecond") &&
                              (lifted.status.to_string() == "2.1.0") &&
                              (lifted.positive()) &&
                              (!lifted.permanent()),
                              "a kernel reply lifts line by line") && result;

    offered.extensions = D_SMTP_EXTENSION_PIPELINING;
    offered.auth       = D_SMTP_AUTH_PLAIN;

    const smtp::capabilities features(offered);

    result = internal::check(
                 (features.supports(smtp::extension::pipelining)) &&
                 (!features.supports(smtp::extension::starttls)) &&
                 (features.offers(smtp::mechanism::plain)) &&
                 (!features.offers(smtp::mechanism::login)),
                 "capabilities answer by enumerator") && result;
    result = internal::check(
                 (static_cast<int>(smtp::error::rejected) ==
                  D_SMTP_ERROR_REJECTED) &&
                 (std::string(smtp::to_string(smtp::error::tls)) ==
                  ::d_smtp_error_string(D_SMTP_ERROR_TLS)) &&
                 (!smtp::status().present()),
                 "enumerators are the kernel's own values") && result;

    return result;
}

/*
tests_smtp_attach
  Tests the following:
  - attach() reads the greeting and says EHLO over a net::connection
  - AUTH PLAIN, a pipelined send with one refused recipient, and QUIT
  - the report lifts per recipient; the connection is closed by close()
*/
bool
tests_smtp_attach()
{
    const std::string script =
        "220 mail.example.com ESMTP\r\n"
        "250-mail.example.com\r\n250-PIPELINING\r\n250 AUTH PLAIN\r\n"
        "235 2.7.0 OK\r\n"
        "250 2.1.0 OK\r\n250 2.1.5 OK\r\n550 5.1.1 No such user\r\n"
        "354 Go ahead\r\n250 2.0.0 Queued\r\n"
        "221 2.0.0 Bye\r\n";
    std::string written;
    bool        closed = false;
    auto        opened = smtp::client::attach(
                             std::make_unique<internal::script_connection>(
                                 script,
                                 &written,
                                 &closed),
                             true,
                             "c.example");

    if (!internal::check(opened.has_value(),
                         "attach reads the greeting and says EHLO"))
    {
        return false;
    }

    smtp::client            session = std::move(*opened);
    const smtp::credentials credentials{ "alice",
                                         "s3cret",
                                         smtp::mechanism::plain };
    smtp::envelope          envelope;

    envelope.sender     = "alice@example.org";
    envelope.recipients = { "bob@example.com", "nobody@example.com" };

    const auto authenticated = session.authenticate(credentials);
    const auto sent          = session.send(envelope,
                                            "Subject: x\r\n\r\nhello\r\n");
    const auto quit          = session.quit();
    bool       result        = true;

    result = internal::check( (authenticated) &&
                              (authenticated->code == 235u) &&
                              (session.is_authenticated()),
                              "AUTH PLAIN succeeds") && result;
    result = internal::check(
                 (sent) &&
                 (sent->accepted == 1u) &&
                 (sent->recipients.size() == 2u) &&
                 (sent->recipients[1].code == 550u) &&
                 (sent->recipients[1].status.to_string() == "5.1.1") &&
                 (sent->data_code == 250u),
                 "the report lifts per recipient") && result;
    result = internal::check( (quit) &&
                              (session.features().supports(
                                   smtp::extension::pipelining)),
                              "QUIT, and the features stay visible") && result;
    session.close();
    result = internal::check(
                 (closed) &&
                 (!session.is_open()) &&
                 (written.find("MAIL FROM:<alice@example.org>\r\n"
                               "RCPT TO:<bob@example.com>\r\n"
                               "RCPT TO:<nobody@example.com>\r\n"
                               "DATA\r\n") != std::string::npos),
                 "one pipelined batch, then the connection closes") && result;

    return result;
}

/*
tests_smtp_loopback
  Tests the following:
  - server::listen on a system-chosen port and run() on a thread
  - smtp::send over STARTTLS, verified against "localhost", with AUTH,
    against a server that requires both
  - the handler sees the unstuffed message and the authenticated user
  - a certificate for another name fails with error::tls
  - stop() ends run(), which waits for its sessions
*/
bool
tests_smtp_loopback()
{
    char certificate[128] = {};
    char key[128]         = {};

    if (!d_tests_sa_smtp_certificate(certificate,
                                     key,
                                     sizeof(key)))
    {
        return internal::check(false,
                               "a test certificate can be made");
    }

    smtp::server_options options;
    smtp::server_handler handler;
    std::string          received;
    std::string          user;

    options.domain               = "mail.example.com";
    options.tls.certificate_file = certificate;
    options.tls.private_key_file = key;
    options.require_tls          = true;
    options.require_auth         = true;
    options.io_timeout_ms        = 10000;
    handler.on_auth    = [](const std::string& _user,
                            const std::string& _secret)
                         {
                             return ( (_user == "alice") &&
                                      (_secret == "s3cret") );
                         };
    handler.on_message = [&received, &user](const smtp::session& _session,
                                            std::string_view     _data)
                         {
                             received.assign(_data);
                             user = _session.user;

                             return 0u;
                         };

    smtp::server mail_server(options,
                             handler);
    const auto   bound = mail_server.listen("127.0.0.1",
                                            0u);

    if (!internal::check(bound.has_value(),
                         "the server listens"))
    {
        return false;
    }

    std::thread runner([&mail_server]()
                       {
                           (void)mail_server.run();
                       });

    smtp::client_options    settings;
    smtp::envelope          envelope;
    const smtp::credentials credentials{ "alice",
                                         "s3cret",
                                         smtp::mechanism::automatic };

    settings.host            = "127.0.0.1";
    settings.port            = *bound;
    settings.mode            = smtp::security::starttls;
    settings.tls.ca_file     = certificate;
    settings.tls.server_name = "localhost";
    settings.helo_domain     = "client.example";
    settings.io_timeout_ms   = 10000;
    envelope.sender          = "alice@example.org";
    envelope.recipients      = { "bob@example.com" };

    const auto sent = smtp::send(settings,
                                 credentials,
                                 envelope,
                                 "Subject: facade\r\n\r\n.dot\r\n");

    settings.tls.server_name = "wrong.example";

    const auto refused = smtp::send(settings,
                                    credentials,
                                    envelope,
                                    "x\r\n");

    mail_server.stop();
    runner.join();
    (void)std::remove(certificate);
    (void)std::remove(key);

    bool result = true;

    result = internal::check(
                 (sent) &&
                 (sent->accepted == 1u) &&
                 (received == "Subject: facade\r\n\r\n.dot\r\n") &&
                 (user == "alice"),
                 "STARTTLS, AUTH and delivery through the facade") && result;
    result = internal::check( (!refused) &&
                              (refused.error().code == smtp::error::tls),
                              "a certificate for another name is refused")
             && result;
    result = internal::check( (mail_server.active_sessions() == 0u),
                              "run() waited for its sessions") && result;

    return result;
}

/*
tests_smtp_run_all
  Runs every facade test, reporting each and the tally.
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
        { "vocabulary",                  tests_smtp_vocabulary },
        { "attach over net::connection", tests_smtp_attach },
        { "loopback over STARTTLS",      tests_smtp_loopback }
    };
    bool all = true;

    for (const entry& test : CASES)
    {
        const bool passed = test.run();

        all = ( (all) &&
                (passed) );
        std::printf("  [%s] %s\n",
                    (passed) ? "PASS" : "FAIL",
                    test.name);
    }

    std::printf("%zu/%zu checks\n",
                internal::counts().checks - internal::counts().failures,
                internal::counts().checks);

    return all;
}

NS_END  // testing
NS_END  // djinterp
