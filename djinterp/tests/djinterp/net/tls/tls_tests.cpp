/*******************************************************************************
* djinterp [net]                                                   tls_tests.cpp
*
* Tests of the TLS layer, net/tls/tls.hpp.
*   A server thread of dmutex.h's accepts on a loopback port with the
* certificate below, self-signed for localhost and 127.0.0.1 and valid until
* 2126, and answers one message; the test connects as the client. Another
* takes a plaintext connection that asks for TLS, and runs it over the same
* connection. A copy of the OpenSSL-family engine with its context and
* session creation watched shows when a context is made live, and refuses
* sessions where a test asks it to; adopt's refusals are tested over a
* connection that carries nothing. The suite runs from C++98: connect and
* accept fill an open_result in place at every level, and the by-value forms
* and the moves are checked from C++11. Link the SSL kernel (ssl.c,
* ssl_openssl.c), tcp.c, net.c, dmutex.c, sink_common.c and the codec, and
* -lssl -lcrypto.
*
*
* path:      /tests/djinterp/net/tls/tls_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./tls_tests.hpp"  // corresponding header


#if (D_INTERNAL_NET_HAS_INT64 == 1)

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::FILE, std::fopen, std::printf, std::remove
#include <cstring>  // std::memcmp, std::strlen
#include <new>      // std::nothrow
#include <string>   // std::string
// posix
#include <fcntl.h>   // open, O_RDONLY
#include <unistd.h>  // close, getpid
// djinterp
#include "../../../../inc/djinterp/c/sync/dmutex.h"  // d_thread_create
#include "./tls_test_certificate.hpp"                 // the certificate
#if (D_MOVE_ENABLED == 1)
    // std
    #include <utility>  // std::move
#endif  // D_MOVE_ENABLED


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

    // CERTIFICATE, KEY
    //   constant: the shared test certificate and its key
    // (tls_test_certificate.hpp).
    static const char* const CERTIFICATE = tls_certificate_pem();
    static const char* const KEY         = tls_key_pem();

    // files
    //   struct: the certificate and the key, written where OpenSSL can load
    // them, and removed with the struct.
    struct files
    {
        files()
            : certificate(),
              key(),
              written(false)
        {
            char      pid[32];
            const int length = std::sprintf(pid,
                                            "%ld",
                                            static_cast<long>(::getpid()));
            const std::string stem("/tmp/djinterp_tls_cpp_" +
                                   std::string(pid,
                                               static_cast<std::size_t>(
                                                   length)));

            certificate = stem + "_cert.pem";
            key         = stem + "_key.pem";
            written     = ( (write(certificate,
                                   CERTIFICATE)) &&
                            (write(key,
                                   KEY)) );
        }

        ~files()
        {
            (void)std::remove(certificate.c_str());
            (void)std::remove(key.c_str());
        }

        // write -- a text, whole, to a new file
        static bool
        write(
            const std::string& _path,
            const char*        _text
        )
        {
            std::FILE* const  out    = std::fopen(_path.c_str(),
                                                  "wb");
            const std::size_t length = std::strlen(_text);

            // a file that cannot be made holds nothing
            if (!out)
            {
                return false;
            }

            const bool whole = (std::fwrite(_text,
                                            1u,
                                            length,
                                            out) == length);

            return ( (std::fclose(out) == 0) &&
                     (whole) );
        }

        std::string certificate;
        std::string key;
        bool        written;
    };

    // the suite's files, made once
    static files&
    pem()
    {
        static files instance;

        return instance;
    }

    // server
    //   struct: an acceptor with the suite's certificate, and what its one
    // connection reported: the bytes it read, how the exchange ended, and
    // whether the client then ended the stream with close_notify. With
    // `abrupt`, it drops the connection after its answer, without
    // close_notify.
    struct server
    {
        server()
            : acceptor(),
              received(),
              outcome(net::io_error::unknown),
              ended(false),
              abrupt(false),
              thread(),
              running(false)
        {}

        net::tls_acceptor<>  acceptor;
        std::string          received;
        net::io_error::value outcome;
        bool                 ended;
        bool                 abrupt;
        d_thread_t           thread;
        bool                 running;
    };

    // serve
    //   function: the server thread: accepts one connection, reads a short
    // message, answers it with "world", and reads on for the end.
    static d_thread_result_t
    serve(
        void* _server
    )
    {
        server* const    work = static_cast<server*>(_server);
        net::open_result accepted;
        char             buffer[64];

        work->outcome = work->acceptor.accept(accepted);

        // a refused accept ends the exchange
        if (work->outcome != net::io_error::none)
        {
            return D_THREAD_SUCCESS;
        }

        const net::io_result got = accepted->read(buffer,
                                                  sizeof(buffer));

        work->outcome = got.error;

        // a refused handshake or read ends the exchange
        if (work->outcome != net::io_error::none)
        {
            return D_THREAD_SUCCESS;
        }

        work->received.assign(buffer,
                              got.count);
        work->outcome = net::write_all(*accepted,
                                       "world",
                                       5u);

        // an abrupt server drops the connection here, saying nothing
        if (work->abrupt)
        {
            return D_THREAD_SUCCESS;
        }

        work->ended = accepted->read(buffer,
                                     sizeof(buffer)).eof();
        accepted->close();

        return D_THREAD_SUCCESS;
    }

    // start
    //   function: loads the certificate, binds to loopback, and starts the
    // server thread.
    static bool
    start(
        server& _server
    )
    {
        net::tls_context& context = _server.acceptor.context();

        _server.running =
            ( (pem().written)       &&
              (context.use_certificate_file(pem().certificate) ==
               net::io_error::none) &&
              (context.use_private_key_file(pem().key) ==
               net::io_error::none) &&
              (_server.acceptor.bind(net::endpoint("127.0.0.1",
                                                   0u)) ==
               net::io_error::none) &&
              (::d_thread_create(&_server.thread,
                                 &serve,
                                 &_server) == 0) );

        return _server.running;
    }

    // finish
    //   function: waits for the server thread, if it ran.
    static void
    finish(
        server& _server
    )
    {
        // only a started thread is waited for
        if (_server.running)
        {
            (void)::d_thread_join(_server.thread,
                                  D_NULLPTR);
            _server.running = false;
        }

        _server.acceptor.close();

        return;
    }

    // exchange
    //   function: sends "hello" on a client connection and reads the answer.
    static bool
    exchange(
        net::connection& _client
    )
    {
        std::string answer(5u,
                           '\0');

        return ( (net::write_all(_client,
                                 "hello",
                                 5u) == net::io_error::none)               &&
                 (net::read_exactly(_client,
                                    &answer[0],
                                    answer.size()) == net::io_error::none) &&
                 (answer == "world") );
    }

    // trusting
    //   function: points a connector's trust at the suite's certificate.
    static bool
    trusting(
        net::tls_connector<>& _connector
    )
    {
        return (_connector.context().use_verify_file(pem().certificate) ==
                net::io_error::none);
    }

    // counted
    //   class: a connection over another, which it owns, counting the
    // closes asked of it.
    class counted : public net::connection
    {
    public:
        counted(
            net::connection* _inner,
            int*             _closes
        )
            : connection(),
              m_inner(_inner),
              m_closes(_closes)
        {}

        ~counted() D_OVERRIDE
        {
            delete m_inner;
        }

        net::io_result
        read(
            void*       _buffer,
            std::size_t _size
        ) D_OVERRIDE
        {
            return m_inner->read(_buffer,
                                 _size);
        }

        net::io_result
        write(
            const void* _data,
            std::size_t _size
        ) D_OVERRIDE
        {
            return m_inner->write(_data,
                                  _size);
        }

        bool
        is_open() const D_OVERRIDE
        {
            return m_inner->is_open();
        }

        void
        close() D_OVERRIDE
        {
            *m_closes += 1;
            m_inner->close();

            return;
        }

    private:
        D_DELETED_FN(counted(const counted&))
        D_DELETED_FN(counted& operator=(const counted&))

        net::connection* m_inner;
        int*             m_closes;
    };

    // counting_connector
    //   class: a transport connector whose connections are TCP ones that
    // count their closes into `closes`.
    class counting_connector
    {
    public:
        explicit counting_connector(
            int* _closes
        )
            : m_tcp(),
              m_closes(_closes)
        {}

        net::io_error::value
        connect(
            const net::endpoint& _endpoint,
            net::open_result&    _out
        )
        {
            const net::io_error::value opened = m_tcp.connect(_endpoint,
                                                              _out);

            // a failed connect has nothing to count
            if (opened != net::io_error::none)
            {
                return opened;
            }

            counted* const made = new (std::nothrow) counted(_out.get(),
                                                             m_closes);

            // without a counter, the connection goes
            if (!made)
            {
                _out.fail(net::io_error::out_of_memory);

                return net::io_error::out_of_memory;
            }

            // the TCP connection passes into the counter that owns it
            (void)_out.release();
            _out.reset(made);

            return net::io_error::none;
        }

    private:
        net::tcp_connector m_tcp;
        int*               m_closes;
    };

    // idle
    //   class: a connection that carries nothing, and counts its deletions:
    // what adopt is handed where it must refuse before a byte moves.
    class idle : public net::connection
    {
    public:
        explicit idle(
            int* _deletes
        )
            : connection(),
              m_deletes(_deletes)
        {}

        ~idle() D_OVERRIDE
        {
            *m_deletes += 1;
        }

        net::io_result
        read(
            void*       _buffer,
            std::size_t _size
        ) D_OVERRIDE
        {
            (void)_buffer;
            (void)_size;

            return net::io_result(0u,
                                  net::io_error::closed);
        }

        net::io_result
        write(
            const void* _data,
            std::size_t _size
        ) D_OVERRIDE
        {
            (void)_data;
            (void)_size;

            return net::io_result(0u,
                                  net::io_error::closed);
        }

        bool
        is_open() const D_OVERRIDE
        {
            return false;
        }

        void
        close() D_OVERRIDE
        {
            return;
        }

    private:
        D_DELETED_FN(idle(const idle&))
        D_DELETED_FN(idle& operator=(const idle&))

        int* m_deletes;
    };

    // free_descriptor
    //   function: the lowest file descriptor not in use, which the next one
    // opened takes; -1 if none can be opened.
    static int
    free_descriptor()
    {
        const int opened = ::open("/dev/null",
                                  O_RDONLY);

        // a descriptor opened to look is closed again at once
        if (opened >= 0)
        {
            (void)::close(opened);
        }

        return opened;
    }

    // watcher
    //   struct: the OpenSSL-family engine with its context and session
    // creation watched: `contexts` counts the contexts made in it, and
    // `refuses` makes it refuse sessions, as a backend might.
    struct watcher
    {
        ::d_ssl_engine engine;
        int            contexts;
        bool           refuses;
    };

    // watching
    //   function: the suite's one watcher, which the engine's callbacks
    // report to.
    static watcher&
    watching() D_NOEXCEPT
    {
        static watcher instance;

        return instance;
    }

    extern "C"
    {
        // watch_context
        //   function: counts a context, then makes it as the engine would.
        static enum d_ssl_error
        watch_context(
            const struct d_ssl_config* _config,
            const struct d_ssl_plan*   _plan,
            void**                     _out_context
        )
        {
            watching().contexts += 1;

            return ::d_ssl_engine_openssl()->context_create(_config,
                                                            _plan,
                                                            _out_context);
        }

        // watch_session
        //   function: refuses a session, as a backend might, when told to;
        // otherwise makes it as the engine would.
        static enum d_ssl_error
        watch_session(
            void*       _context,
            const char* _host,
            const char* _sni,
            void**      _out_session
        )
        {
            const struct d_ssl_engine* const engine = ::d_ssl_engine_openssl();

            return (watching().refuses)
                       ? D_SSL_ERROR_BACKEND
                       : engine->session_create(_context,
                                                _host,
                                                _sni,
                                                _out_session);
        }
    }

    // watched_engine
    //   function: the watcher's engine, with its counts cleared.
    static const ::d_ssl_engine*
    watched_engine()
    {
        watcher& watch = watching();

        watch.engine                = *::d_ssl_engine_openssl();
        watch.engine.context_create = &watch_context;
        watch.engine.session_create = &watch_session;
        watch.contexts              = 0;
        watch.refuses               = false;

        return &watch.engine;
    }

    // refuses
    //   function: whether adopt, given an idle transport and `_context` for
    // `_host`, refused with `_error`, recorded it in the result, and deleted
    // the transport.
    static bool
    refuses(
        net::tls_context&    _context,
        const std::string&   _host,
        net::io_error::value _error
    )
    {
        int                        deletes = 0;
        net::open_result           out;
        const net::io_error::value adopted =
            net::tls_connection::adopt(new (std::nothrow) idle(&deletes),
                                       _context,
                                       _host,
                                       out);

        return ( (adopted == _error)   &&
                 (out.error == _error) &&
                 (!out.ok())           &&
                 (deletes == 1) );
    }

    // shut
    //   function: whether a result moved nothing and found the connection
    // closed.
    static bool
    shut(
        const net::io_result& _result
    )
    {
        return ( (_result.count == 0u) &&
                 (_result.error == net::io_error::closed) );
    }

    // upgrader
    //   struct: a plaintext acceptor whose one connection asks for TLS before
    // it says anything else, the server context it is answered with, and
    // what the connection reported.
    struct upgrader
    {
        upgrader()
            : acceptor(),
              context(net::tls_context::role::server),
              received(),
              outcome(net::io_error::unknown),
              thread(),
              running(false)
        {}

        net::tcp_acceptor    acceptor;
        net::tls_context     context;
        std::string          received;
        net::io_error::value outcome;
        d_thread_t           thread;
        bool                 running;
    };

    // asks_for_tls
    //   function: reads a plaintext request for TLS from `_plain` and grants
    // it, as a STARTTLS-style protocol would.
    static net::io_error::value
    asks_for_tls(
        net::connection& _plain
    )
    {
        char                       request[10] = { 0 };
        const net::io_error::value read        =
            net::read_exactly(_plain,
                              request,
                              sizeof(request));

        // anything but the request is refused
        if ( (read != net::io_error::none) ||
             (std::memcmp(request,
                          "STARTTLS\r\n",
                          sizeof(request)) != 0) )
        {
            return net::io_error::protocol;
        }

        return net::write_all(_plain,
                              "GO\r\n",
                              4u);
    }

    // upgrade
    //   function: the upgrader's thread: accepts a plaintext connection,
    // grants its request for TLS, runs TLS as a server over the same
    // connection, and answers a short message with "world".
    static d_thread_result_t
    upgrade(
        void* _upgrader
    )
    {
        upgrader* const  work   = static_cast<upgrader*>(_upgrader);
        net::open_result plain;
        net::open_result secure;
        char             buffer[64];

        work->outcome = work->acceptor.accept(plain);

        // the plaintext exchange first, then TLS over the same connection
        if (work->outcome == net::io_error::none)
        {
            work->outcome = asks_for_tls(*plain);
        }

        if (work->outcome == net::io_error::none)
        {
            work->outcome = net::tls_connection::adopt(plain.release(),
                                                       work->context,
                                                       std::string(),
                                                       secure);
        }

        const net::io_result got =
            (work->outcome == net::io_error::none)
                ? secure->read(buffer,
                               sizeof(buffer))
                : net::io_result(0u,
                                 work->outcome);

        work->outcome = got.error;

        // a refused handshake or read ends the exchange
        if (work->outcome == net::io_error::none)
        {
            work->received.assign(buffer,
                                  got.count);
            work->outcome = net::write_all(*secure,
                                           "world",
                                           5u);
            secure->close();
        }

        return D_THREAD_SUCCESS;
    }

    // start_upgrader
    //   function: loads the certificate, binds to loopback, and starts the
    // upgrader's thread.
    static bool
    start_upgrader(
        upgrader& _server
    )
    {
        _server.running =
            ( (pem().written)                                 &&
              (_server.context.use_certificate_file(
                   pem().certificate) == net::io_error::none) &&
              (_server.context.use_private_key_file(pem().key) ==
               net::io_error::none)                           &&
              (_server.acceptor.bind(net::endpoint("127.0.0.1",
                                                   0u)) ==
               net::io_error::none)                           &&
              (::d_thread_create(&_server.thread,
                                 &upgrade,
                                 &_server) == 0) );

        return _server.running;
    }

    // finish_upgrader
    //   function: waits for the upgrader's thread, if it ran.
    static void
    finish_upgrader(
        upgrader& _server
    )
    {
        // only a started thread is waited for
        if (_server.running)
        {
            (void)::d_thread_join(_server.thread,
                                  D_NULLPTR);
            _server.running = false;
        }

        _server.acceptor.close();

        return;
    }

NS_END  // internal


/*
tests_tls_engine
  Whether the build has the engine every other test runs in.
*/
bool
tests_tls_engine()
{
    return net::tls_context(net::tls_context::role::client).is_valid();
}

/*
tests_tls_context
  Tests the following:
  - a context is valid in either role
  - the suite's certificate and key load into a server context
  - a missing file is refused as an invalid argument
*/
bool
tests_tls_context()
{
    net::tls_context client(net::tls_context::role::client);
    net::tls_context server(net::tls_context::role::server);
    bool             result = true;

    result = internal::check( (client.is_valid()) &&
                              (server.is_valid()),
                              "a context is valid in either role") && result;
    result = internal::check( (internal::pem().written) &&
                              (server.use_certificate_file(
                                   internal::pem().certificate) ==
                               net::io_error::none)     &&
                              (server.use_private_key_file(
                                   internal::pem().key) ==
                               net::io_error::none),
                              "the certificate and key load") && result;
    result = internal::check(server.use_certificate_file(
                                 "/nonexistent/djinterp.pem") ==
                             net::io_error::invalid_argument,
                             "a missing file is refused") && result;

    return result;
}

/*
tests_tls_prepare
  Tests the following:
  - a server context without a certificate cannot be made live, the
    kernel's reason kept, and an acceptor with one refuses before it takes
    a client
  - a key that is no key is refused once the pair is named
  - a context without an engine is not valid, and cannot be made live
*/
bool
tests_tls_prepare()
{
    net::tls_context    bare(net::tls_context::role::server);
    net::tls_context    wrong(net::tls_context::role::server);
    net::tls_context    none(net::tls_context::role::client,
                             D_NULLPTR);
    net::tls_acceptor<> acceptor;
    net::open_result    refused;
    bool                result = true;

    result = internal::check( (bare.prepare() ==
                               net::io_error::invalid_argument)      &&
                              (bare.failure() == D_SSL_ERROR_CONFIG) &&
                              (acceptor.accept(refused) ==
                               net::io_error::invalid_argument)      &&
                              (!refused.ok())                        &&
                              (refused.error ==
                               net::io_error::invalid_argument),
                              "a server without a certificate is refused") &&
             result;
    result = internal::check( (internal::pem().written)         &&
                              (wrong.use_certificate_file(
                                   internal::pem().certificate) ==
                               net::io_error::none)             &&
                              (wrong.use_private_key_file(
                                   internal::pem().certificate) ==
                               net::io_error::invalid_argument) &&
                              (wrong.failure() == D_SSL_ERROR_CONFIG),
                              "a key that is no key is refused") && result;
    result = internal::check( (!none.is_valid())                &&
                              (none.prepare() ==
                               net::io_error::invalid_argument) &&
                              (none.failure() == D_SSL_ERROR_UNSUPPORTED),
                              "without an engine nothing is made live") &&
             result;

    return result;
}

/*
tests_tls_settings
  Tests the following:
  - a client verifies its peer and the peer's name until told not to, and
    again when told to; a server verifies nothing until told to, and then
    asks for a client's certificate, and checks no name
  - use_default_verify_paths trusts the system's anchors again
  - a change forgets why the last prepare failed
  - naming a file leaves it closed
*/
bool
tests_tls_settings()
{
    net::tls_context      client(net::tls_context::role::client);
    net::tls_context      server(net::tls_context::role::server);
    const ::d_ssl_config& mine   =
        static_cast<const net::tls_context&>(client).config();
    const ::d_ssl_config& theirs =
        static_cast<const net::tls_context&>(server).config();
    bool                  result = true;

    const bool defaults = ( (mine.verify == D_SSL_VERIFY_REQUIRED) &&
                            (mine.verify_hostname)                 &&
                            (theirs.verify == D_SSL_VERIFY_NONE) );

    client.set_verify_peer(false);

    const bool off = ( (mine.verify == D_SSL_VERIFY_NONE) &&
                       (!mine.verify_hostname) );

    client.set_verify_peer(true);
    server.set_verify_peer(true);
    result = internal::check( (defaults)                               &&
                              (off)                                    &&
                              (mine.verify == D_SSL_VERIFY_REQUIRED)   &&
                              (mine.verify_hostname)                   &&
                              (theirs.verify == D_SSL_VERIFY_OPTIONAL) &&
                              (!theirs.verify_hostname),
                              "set_verify_peer, in either role") && result;
    client.config().use_system_trust = false;
    result = internal::check( (client.use_default_verify_paths() ==
                               net::io_error::none) &&
                              (mine.use_system_trust),
                              "the system's anchors are trusted again") &&
             result;

    const bool failed = ( (server.prepare() ==
                           net::io_error::invalid_argument) &&
                          (server.failure() == D_SSL_ERROR_CONFIG) );

    server.set_min_version(net::tls_context::version::tls1_2);
    result = internal::check( (failed) &&
                              (server.failure() == D_SSL_OK),
                              "a change forgets the last failure") && result;

    const int  before = internal::free_descriptor();
    const bool named  = ( (internal::pem().written) &&
                          (server.use_certificate_file(
                               internal::pem().certificate) ==
                           net::io_error::none) );

    result = internal::check( (named)       &&
                              (before >= 0) &&
                              (internal::free_descriptor() == before),
                              "naming a file leaves it closed") && result;

    return result;
}

/*
tests_tls_naming
  Tests the following:
  - a client naming a trust bundle that does not load is refused at once;
    the bundle stays named, and the context goes on refusing until one that
    loads replaces it
  - a server's certificate waits for its key: one replaced before its key is
    not refused when named, but when the context is next made live
  - a server without its identity takes a bundle without being made live
*/
bool
tests_tls_naming()
{
    net::tls_context   client(net::tls_context::role::client);
    net::tls_context   server(net::tls_context::role::server);
    net::tls_context   waiting(net::tls_context::role::server);
    const std::string& certificate = internal::pem().certificate;
    const std::string& key         = internal::pem().key;
    bool               result      = true;

    result = internal::check( (internal::pem().written)                &&
                              (client.use_verify_file(key) ==
                               net::io_error::invalid_argument)        &&
                              (client.failure() == D_SSL_ERROR_CONFIG) &&
                              (client.prepare() ==
                               net::io_error::invalid_argument)        &&
                              (client.use_verify_file(certificate) ==
                               net::io_error::none),
                              "a bundle that does not load is refused when "
                              "named") && result;
    result = internal::check( (server.use_certificate_file(certificate) ==
                               net::io_error::none)             &&
                              (server.use_private_key_file(key) ==
                               net::io_error::none)             &&
                              (server.use_certificate_file(key) ==
                               net::io_error::none)             &&
                              (server.prepare() ==
                               net::io_error::invalid_argument) &&
                              (server.use_certificate_file(certificate) ==
                               net::io_error::none)             &&
                              (server.prepare() == net::io_error::none),
                              "a certificate waits for its key") && result;
    result = internal::check( (waiting.use_verify_file(key) ==
                               net::io_error::none) &&
                              (waiting.failure() == D_SSL_OK),
                              "a server without its identity waits") &&
             result;

    return result;
}

/*
tests_tls_remaking
  Tests the following:
  - a context is made live once however often it is prepared, and again
    after each change: a version, verification, the configuration asked for
    to change it, the system's anchors, a file named
  - looking at the configuration changes nothing
  - a server is made live by its key, not by its certificate
*/
bool
tests_tls_remaking()
{
    const ::d_ssl_engine* const engine  = internal::watched_engine();
    const int&                  made    = internal::watching().contexts;
    net::tls_context            client(net::tls_context::role::client,
                                       engine);
    net::tls_context            server(net::tls_context::role::server,
                                       engine);
    int                         seen[7] = { 0 };
    bool                        result  = true;

    (void)client.prepare();
    (void)client.prepare();
    seen[0] = made;
    client.set_min_version(net::tls_context::version::tls1_2);
    (void)client.prepare();
    seen[1] = made;
    client.set_verify_peer(true);
    (void)client.prepare();
    seen[2] = made;
    (void)client.config();
    (void)client.prepare();
    seen[3] = made;
    (void)static_cast<const net::tls_context&>(client).config();
    (void)client.prepare();
    seen[4] = made;
    (void)client.use_default_verify_paths();
    seen[5] = made;
    (void)client.use_verify_file(internal::pem().certificate);
    seen[6] = made;
    result = internal::check( (seen[0] == 1) &&
                              (seen[1] == 2) &&
                              (seen[2] == 3) &&
                              (seen[3] == 4) &&
                              (seen[4] == 4) &&
                              (seen[5] == 5) &&
                              (seen[6] == 6),
                              "a context is made again after each change, "
                              "and only then") && result;

    const int before = made;

    result = internal::check( (server.use_certificate_file(
                                   internal::pem().certificate) ==
                               net::io_error::none) &&
                              (made == before)      &&
                              (server.use_private_key_file(
                                   internal::pem().key) ==
                               net::io_error::none) &&
                              (made == before + 1),
                              "a server is made live by its key") && result;

    return result;
}

/*
tests_tls_identity
  Tests the following:
  - an identity set through config(), in files or in memory, completes a
    server as one named through use_* does: a trust call makes it live
  - half of one, in either form, leaves a client waiting for the rest
  - PEM of any length counts: a server whose certificate and key are one
    byte each is complete, and refused when a trust call makes it live
*/
bool
tests_tls_identity()
{
    const ::d_ssl_engine* const engine = internal::watched_engine();
    const int&                  made   = internal::watching().contexts;
    const std::string&          bundle = internal::pem().certificate;
    const ::d_pack_text         chain  = { internal::CERTIFICATE,
                                           std::strlen(
                                               internal::CERTIFICATE) };
    const ::d_pack_text         key    = { internal::KEY,
                                           std::strlen(internal::KEY) };
    const ::d_pack_text         byte   = { "x",
                                           1u };
    net::tls_context            files(net::tls_context::role::server,
                                      engine);
    net::tls_context            memory(net::tls_context::role::server,
                                       engine);
    net::tls_context            keyed(net::tls_context::role::client,
                                      engine);
    net::tls_context            chained(net::tls_context::role::client,
                                        engine);
    net::tls_context            tiny(net::tls_context::role::server,
                                     engine);
    bool                        result = true;

    files.config().certificate_file  = bundle.c_str();
    files.config().private_key_file  = internal::pem().key.c_str();
    memory.config().certificate_pem  = chain;
    memory.config().private_key_pem  = key;
    keyed.config().private_key_file  = internal::pem().key.c_str();
    chained.config().certificate_pem = chain;
    tiny.config().certificate_pem    = byte;
    tiny.config().private_key_pem    = byte;

    const int before = made;

    result = internal::check( (internal::pem().written) &&
                              (files.use_verify_file(bundle) ==
                               net::io_error::none)     &&
                              (made == before + 1)      &&
                              (memory.use_default_verify_paths() ==
                               net::io_error::none)     &&
                              (made == before + 2),
                              "an identity set through config() completes "
                              "a server") && result;
    result = internal::check( (keyed.use_verify_file(bundle) ==
                               net::io_error::none) &&
                              (chained.use_default_verify_paths() ==
                               net::io_error::none) &&
                              (made == before + 2),
                              "half an identity leaves a client waiting") &&
             result;
    result = internal::check( (tiny.use_verify_file(bundle) ==
                               net::io_error::invalid_argument) &&
                              (tiny.failure() == D_SSL_ERROR_CONFIG),
                              "a one-byte identity is complete, and refused") &&
             result;

    return result;
}

/*
tests_tls_adopt
  Tests the following:
  - adopt refuses no transport, a name it cannot check where the context
    checks names, and a context that cannot be made live, as invalid
    arguments, recording the error in the result
  - a session the engine cannot make is refused for the engine's reason
  - whatever adopt refuses, it deletes the transport it was given
*/
bool
tests_tls_adopt()
{
    net::tls_context client(net::tls_context::role::client);
    net::tls_context bare(net::tls_context::role::server);
    net::tls_context refusing(net::tls_context::role::client,
                              internal::watched_engine());
    net::open_result out;
    bool             result = true;

    const net::io_error::value nothing =
        net::tls_connection::adopt(D_NULLPTR,
                                   client,
                                   "localhost",
                                   out);
    const bool                 unnamed =
        internal::refuses(client,
                          "not a host!",
                          net::io_error::invalid_argument);
    const bool                 unready =
        internal::refuses(bare,
                          std::string(),
                          net::io_error::invalid_argument);

    internal::watching().refuses = true;

    const bool unmade = internal::refuses(refusing,
                                          "localhost",
                                          net::io_error::unknown);

    internal::watching().refuses = false;
    result = internal::check( (nothing == net::io_error::invalid_argument) &&
                              (out.error == net::io_error::invalid_argument),
                              "adopt refuses no transport") && result;
    result = internal::check(unnamed,
                             "adopt refuses a name it cannot check") &&
             result;
    result = internal::check(unready,
                             "adopt refuses a context that cannot be made "
                             "live") && result;
    result = internal::check(unmade,
                             "adopt refuses a session the engine cannot "
                             "make") && result;

    return result;
}

/*
tests_tls_loopback
  Tests the following:
  - a client trusting the certificate connects to 127.0.0.1, verifying it
    against the address, and the handshake completes at connect
  - bytes cross both ways, and the protocol is TLS 1.2 or later
*/
bool
tests_tls_loopback()
{
    internal::server     server;
    net::tls_connector<> connector;
    net::open_result     client;
    bool                 result = true;

    const bool started = ( (internal::trusting(connector)) &&
                           (internal::start(server)) );
    const bool opened  = ( (started) &&
                           (connector.connect(server.acceptor
                                              .local_endpoint(),
                                              client) ==
                            net::io_error::none) );
    const bool crossed = ( (opened) &&
                           (internal::exchange(*client)) );
    const int  version = (opened)
                             ? static_cast<net::tls_connection*>(
                                   client.get())->info().version
                             : 0;

    client.reset();
    internal::finish(server);
    result = internal::check(opened,
                             "a verified connect completes its handshake") &&
             result;
    result = internal::check( (crossed)                    &&
                              (server.received == "hello") &&
                              (server.outcome == net::io_error::none),
                              "bytes cross both ways") && result;
    result = internal::check(version >= D_SSL_VERSION_TLS1_2,
                             "the protocol is TLS 1.2 or later") && result;

    return result;
}

/*
tests_tls_hostname
  Tests the following:
  - a server name the certificate carries verifies
  - a name it does not carry is refused at connect, as access denied
*/
bool
tests_tls_hostname()
{
    internal::server     named;
    internal::server     wrong;
    net::tls_connector<> good;
    net::tls_connector<> bad;
    net::open_result     client;
    net::open_result     refused;
    bool                 result = true;

    good.set_server_name("localhost");
    bad.set_server_name("example.com");

    const bool started = ( (internal::trusting(good)) &&
                           (internal::trusting(bad))  &&
                           (internal::start(named))   &&
                           (internal::start(wrong)) );
    const bool matched = ( (started)                                     &&
                           (good.connect(named.acceptor.local_endpoint(),
                                         client) == net::io_error::none) &&
                           (internal::exchange(*client)) );
    const net::io_error::value mismatched =
        (started) ? bad.connect(wrong.acceptor.local_endpoint(),
                                refused)
                  : net::io_error::unknown;

    client.reset();
    internal::finish(named);
    internal::finish(wrong);
    result = internal::check(matched,
                             "a name the certificate carries verifies") &&
             result;
    result = internal::check( (mismatched == net::io_error::access_denied) &&
                              (!refused.ok())                              &&
                              (refused.error == net::io_error::access_denied),
                              "a name it lacks is refused at connect") &&
             result;

    return result;
}

/*
tests_tls_untrusted
  Tests the following:
  - with only the system's trust anchors, the self-signed certificate is
    refused at connect, and the server's first read fails
*/
bool
tests_tls_untrusted()
{
    internal::server     server;
    net::tls_connector<> connector;
    net::open_result     client;
    bool                 result = true;

    const bool                 started = internal::start(server);
    const net::io_error::value refused =
        (started) ? connector.connect(server.acceptor.local_endpoint(),
                                      client)
                  : net::io_error::unknown;

    client.reset();
    internal::finish(server);
    result = internal::check( (refused == net::io_error::access_denied) &&
                              (client.get() == D_NULLPTR),
                              "an untrusted certificate is refused") &&
             result;
    result = internal::check( (started)                               &&
                              (server.outcome != net::io_error::none) &&
                              (server.received.empty()),
                              "the server's read fails with the handshake") &&
             result;

    return result;
}

/*
tests_tls_verify_off
  Tests the following:
  - with verification off, the self-signed certificate is accepted
  - so is a server name that is neither a host name nor an address, which
    goes unsent, where verified names would refuse it
*/
bool
tests_tls_verify_off()
{
    internal::server     server;
    internal::server     odd;
    net::tls_connector<> connector;
    net::tls_connector<> verifying;
    net::open_result     client;
    net::open_result     refused;
    bool                 result = true;

    connector.set_verify(false);

    const bool started = internal::start(server);
    const bool crossed = ( (started)             &&
                           (connector.connect(server.acceptor
                                              .local_endpoint(),
                                              client) ==
                            net::io_error::none) &&
                           (internal::exchange(*client)) );

    client.reset();
    internal::finish(server);
    connector.set_server_name("not a host!");
    verifying.set_server_name("not a host!");

    const bool unnamed =
        ( (internal::start(odd))                             &&
          (verifying.connect(odd.acceptor.local_endpoint(),
                             refused) ==
           net::io_error::invalid_argument)                  &&
          (refused.error == net::io_error::invalid_argument) &&
          (connector.connect(odd.acceptor.local_endpoint(),
                             client) == net::io_error::none) &&
          (internal::exchange(*client)) );

    client.reset();
    internal::finish(odd);
    result = internal::check(crossed,
                             "unverified, the connection still carries "
                             "bytes") && result;
    result = internal::check(unnamed,
                             "a name that is no host goes unsent") &&
             result;

    return result;
}

/*
tests_tls_versions
  Tests the following:
  - a client that settles for TLS 1.2 agrees it with a server that offers
    no more
  - one that will have no less than TLS 1.3 is refused at connect, as a
    protocol error, and the server's first read fails
*/
bool
tests_tls_versions()
{
    internal::server     older;
    internal::server     oldest;
    net::tls_connector<> settling;
    net::tls_connector<> demanding;
    net::open_result     client;
    net::open_result     refused;
    bool                 result = true;

    older.acceptor.context().config().max_version  = D_SSL_VERSION_TLS1_2;
    oldest.acceptor.context().config().max_version = D_SSL_VERSION_TLS1_2;
    settling.set_verify(false);
    demanding.set_verify(false);
    demanding.context().set_min_version(net::tls_context::version::tls1_3);

    const bool                 started  = ( (internal::start(older)) &&
                                            (internal::start(oldest)) );
    const bool                 settled  =
        ( (started)                                         &&
          (settling.connect(older.acceptor.local_endpoint(),
                            client) == net::io_error::none) &&
          (static_cast<net::tls_connection*>(client.get())
           ->info().version == D_SSL_VERSION_TLS1_2)    &&
          (internal::exchange(*client)) );
    const net::io_error::value demanded =
        (started) ? demanding.connect(oldest.acceptor.local_endpoint(),
                                      refused)
                  : net::io_error::unknown;

    client.reset();
    internal::finish(older);
    internal::finish(oldest);
    result = internal::check(settled,
                             "TLS 1.2 is agreed where it is all a server "
                             "offers") && result;
    result = internal::check( (demanded == net::io_error::protocol)      &&
                              (refused.error == net::io_error::protocol) &&
                              (oldest.outcome != net::io_error::none),
                              "a client that will have TLS 1.3 is refused") &&
             result;

    return result;
}

/*
tests_tls_closing
  Tests the following:
  - shutdown(WRITE) says close_notify, which the server reads as the end of
    the stream, and reading goes on: the answer still arrives
  - close() says close_notify too, which the server reads as the end, then
    closes the transport, after which the connection reads nothing
*/
bool
tests_tls_closing()
{
    typedef net::tls_connector<internal::counting_connector> counting_tls;

    int                          closes    = 0;
    internal::server             polite;
    internal::server             closing;
    net::tls_connector<>         connector;
    internal::counting_connector transport(&closes);
    counting_tls                 counting(transport);
    net::open_result             client;
    char                         buffer[8] = { 0 };
    bool                         result    = true;

    connector.set_verify(false);
    counting.set_verify(false);

    const bool half =
        ( (internal::start(polite))                          &&
          (connector.connect(polite.acceptor.local_endpoint(),
                             client) == net::io_error::none) &&
          (net::write_all(*client,
                          "hello",
                          5u) == net::io_error::none)        &&
          (client->shutdown(net::shutdown_mode::write) ==
           net::io_error::none)                              &&
          (net::read_exactly(*client,
                             buffer,
                             5u) == net::io_error::none)     &&
          (std::memcmp(buffer,
                       "world",
                       5u) == 0) );

    client.reset();
    internal::finish(polite);

    const bool opened =
        ( (internal::start(closing))                        &&
          (counting.connect(closing.acceptor.local_endpoint(),
                            client) == net::io_error::none) &&
          (internal::exchange(*client)) );

    // only a connection opened has anything to close
    if (opened)
    {
        client->close();
    }

    const bool closed = ( (opened)                                       &&
                          (closes == 1)                                  &&
                          (internal::shut(client->read(buffer,
                                                       sizeof(buffer)))) &&
                          (!client->is_open()) );

    client.reset();
    internal::finish(closing);
    result = internal::check( (half) &&
                              (polite.ended),
                              "shutdown(WRITE) ends the stream, and reads go "
                              "on") && result;
    result = internal::check( (closed) &&
                              (closing.ended),
                              "close() ends the stream, then closes the "
                              "transport") && result;

    return result;
}

/*
tests_tls_truncation
  Tests the following:
  - a server that drops its connection without close_notify has truncated
    it: the client's next read fails as a closed connection, and the
    connection, open until then, is open no longer
*/
bool
tests_tls_truncation()
{
    internal::server     rude;
    net::tls_connector<> connector;
    net::open_result     client;
    char                 buffer[8] = { 0 };
    bool                 result    = true;

    connector.set_verify(false);
    rude.abrupt = true;

    const bool opened = ( (internal::start(rude)) &&
                          (connector.connect(rude.acceptor.local_endpoint(),
                                             client) ==
                           net::io_error::none)   &&
                          (client->is_open())     &&
                          (internal::exchange(*client)) );
    const bool cut    = ( (opened)                &&
                          (client->read(buffer,
                                        sizeof(buffer)).error ==
                           net::io_error::closed) &&
                          (!client->is_open()) );

    client.reset();
    internal::finish(rude);
    result = internal::check(cut,
                             "a dropped connection is truncated") && result;

    return result;
}

/*
tests_tls_outlives
  Tests the following:
  - a connection outlives the connector and the context it was made with,
    carrying bytes after both are gone
  - a context changed after a connection is made leaves the connection as
    it was
*/
bool
tests_tls_outlives()
{
    internal::server server;
    net::open_result client;
    bool             result = true;

    const bool started = internal::start(server);

    // the connector, and its context, end here
    {
        net::tls_connector<> connector;

        // a connector trusting the certificate connects, then changes
        if ( (started) &&
             (internal::trusting(connector)) )
        {
            (void)connector.connect(server.acceptor.local_endpoint(),
                                    client);
        }

        connector.context().set_min_version(
            net::tls_context::version::tls1_3);
    }

    const bool crossed = ( (client.ok()) &&
                           (internal::exchange(*client)) );

    client.reset();
    internal::finish(server);
    result = internal::check(crossed,
                             "a connection outlives its connector") &&
             result;

    return result;
}

/*
tests_tls_alpn
  Tests the following:
  - what no call sets is set through config(): both ends offer ALPN, the
    server chooses, and the client's info() reports the choice, the
    version, and the server's certificate
*/
bool
tests_tls_alpn()
{
    static const unsigned char PROTOCOLS[] =
    {
        2, 'h', '2',
        8, 'h', 't', 't', 'p', '/', '1', '.', '1'
    };
    const ::d_pack_bytes       alpn        = { PROTOCOLS,
                                               sizeof(PROTOCOLS) };
    internal::server           server;
    net::tls_connector<>       connector;
    net::open_result           client;
    bool                       result      = true;

    server.acceptor.context().config().alpn = alpn;
    connector.context().config().alpn       = alpn;

    const bool        started = ( (internal::trusting(connector)) &&
                                  (internal::start(server)) );
    const bool        opened  =
        ( (started) &&
          (connector.connect(server.acceptor.local_endpoint(),
                             client) == net::io_error::none) );
    const ::d_ssl_info info   =
        (opened) ? static_cast<net::tls_connection*>(client.get())->info()
                 : ::d_ssl_info();
    const bool        crossed = ( (opened) &&
                                  (internal::exchange(*client)) );

    client.reset();
    internal::finish(server);
    result = internal::check( (crossed)                               &&
                              (std::string(info.alpn,
                                           info.alpn_length) == "h2") &&
                              (info.version >= D_SSL_VERSION_TLS1_2)  &&
                              (info.has_peer_certificate),
                              "ALPN set through config() is agreed") &&
             result;

    return result;
}

/*
tests_tls_upgrade
  Tests the following:
  - a plaintext connection that has asked for TLS runs it in place, each
    end adopting its own connection, and the server's certificate is
    verified against the address
*/
bool
tests_tls_upgrade()
{
    internal::upgrader server;
    net::tcp_connector plain;
    net::tls_context   context(net::tls_context::role::client);
    net::open_result   client;
    char               go[4] = { 0 };
    bool               result = true;

    const bool started = ( (internal::start_upgrader(server)) &&
                           (context.use_verify_file(
                                internal::pem().certificate) ==
                            net::io_error::none) );
    const bool asked   =
        ( (started)                                              &&
          (plain.connect(server.acceptor.local_endpoint(),
                         client) == net::io_error::none)         &&
          (net::write_all(*client,
                          "STARTTLS\r\n",
                          10u) == net::io_error::none)           &&
          (net::read_exactly(*client,
                             go,
                             sizeof(go)) == net::io_error::none) &&
          (std::memcmp(go,
                       "GO\r\n",
                       sizeof(go)) == 0) );
    const bool secured =
        ( (asked)                                                     &&
          (net::tls_connection::adopt(client.release(),
                                      context,
                                      "127.0.0.1",
                                      client) == net::io_error::none) &&
          (static_cast<net::tls_connection*>(client.get())
           ->handshake() == net::io_error::none)                  &&
          (internal::exchange(*client)) );

    client.reset();
    internal::finish_upgrader(server);
    result = internal::check( (secured)                    &&
                              (server.received == "hello") &&
                              (server.outcome == net::io_error::none),
                              "a plaintext connection upgrades in place") &&
             result;

    return result;
}

#if (D_MOVE_ENABLED == 1)
/*
tests_tls_moves
  Tests the following:
  - the named constructors return valid contexts, which move
  - a context made live moves with its live context, by construction and
    by assignment, which lets go of the one assigned over; where it goes,
    it runs TLS over a connection it adopts
  - an acceptor takes a configured context by move, and accept returns its
    result by value
*/
bool
tests_tls_moves()
{
    net::tls_context client = net::tls_context::client();
    net::tls_context server = net::tls_context::server();
    net::tls_context live(net::tls_context::role::client);
    bool             result = true;

    result = internal::check( (client.is_valid()) &&
                              (server.is_valid()),
                              "the named constructors make valid contexts") &&
             result;
    live.set_verify_peer(false);

    const bool       made = ( (live.prepare() == net::io_error::none) &&
                              (client.prepare() == net::io_error::none) );
    net::tls_context carried(std::move(live));

    client = std::move(carried);

    internal::server   listening;
    net::tcp_connector plain;
    net::open_result   opened;
    const bool         crossed =
        ( (made)                                                      &&
          (internal::start(listening))                                &&
          (plain.connect(listening.acceptor.local_endpoint(),
                         opened) == net::io_error::none)              &&
          (net::tls_connection::adopt(opened.release(),
                                      client,
                                      "127.0.0.1",
                                      opened) == net::io_error::none) &&
          (internal::exchange(*opened)) );

    opened.reset();
    internal::finish(listening);
    result = internal::check( (crossed)          &&
                              (!live.is_valid()) &&
                              (!carried.is_valid()),
                              "a live context moves, and runs TLS where it "
                              "goes") && result;

    const bool          loaded =
        ( (server.use_certificate_file(internal::pem().certificate) ==
           net::io_error::none) &&
          (server.use_private_key_file(internal::pem().key) ==
           net::io_error::none) );
    net::tls_acceptor<> taking(std::move(server));
    const bool          bound  =
        ( (loaded)                                                &&
          (taking.bind(net::endpoint("127.0.0.1",
                                     0u)) == net::io_error::none) &&
          (plain.connect(taking.local_endpoint(),
                         opened) == net::io_error::none) );
    net::open_result    accepted =
        (bound) ? taking.accept()
                : net::open_result(net::io_error::unknown);

    result = internal::check( (accepted.ok()) &&
                              (!server.is_valid()),
                              "an acceptor takes a context by move, and "
                              "accepts by value") && result;

    return result;
}

/*
tests_tls_moving
  Tests the following:
  - connect returns its result by value, and a TLS connection moves: the
    one moved to carries the bytes, and the one moved from is closed -- it
    reads, writes, handshakes and shuts down nothing, has no session, tells
    nothing, and has no endpoints
  - assigning a connection over another releases the other, whose server
    finds it dropped, and close() reaches the server of the one assigned
*/
bool
tests_tls_moving()
{
    internal::server     first;
    internal::server     second;
    net::tls_connector<> connector;
    char                 byte   = 0;
    bool                 result = true;

    connector.set_verify(false);

    const bool       started = ( (internal::start(first)) &&
                                 (internal::start(second)) );
    net::open_result one     =
        (started) ? connector.connect(first.acceptor.local_endpoint())
                  : net::open_result(net::io_error::unknown);
    net::open_result two     =
        (started) ? connector.connect(second.acceptor.local_endpoint())
                  : net::open_result(net::io_error::unknown);

    // without both connections there is nothing to move
    if ( (!one.ok()) ||
         (!two.ok()) )
    {
        one.reset();
        two.reset();
        internal::finish(first);
        internal::finish(second);

        return internal::check(false,
                               "two connections to move");
    }

    net::tls_connection& from  = *static_cast<net::tls_connection*>(one.get());
    net::tls_connection& over  = *static_cast<net::tls_connection*>(two.get());
    net::tls_connection  moved(std::move(from));
    const bool           empty =
        ( (!from.is_open())                           &&
          (internal::shut(from.read(&byte,
                                    1u)))             &&
          (internal::shut(from.write(&byte,
                                     1u)))            &&
          (from.handshake() == net::io_error::closed) &&
          (from.shutdown(net::shutdown_mode::write) ==
           net::io_error::closed)                     &&
          (from.session() == D_NULLPTR)               &&
          (from.info().version == D_SSL_VERSION_NONE) &&
          (from.remote_endpoint().port == 0u)         &&
          (from.local_endpoint().port == 0u) );
    const bool           carried = internal::exchange(moved);

    over = std::move(moved);
    over.close();
    one.reset();
    two.reset();
    internal::finish(first);
    internal::finish(second);
    result = internal::check(empty,
                             "a connection moved from is closed") && result;
    result = internal::check( (carried) &&
                              (first.ended),
                              "the connection moved to carries the bytes, "
                              "and closes") && result;
    result = internal::check(second.outcome != net::io_error::none,
                             "assigning over a connection releases it") &&
             result;

    return result;
}
#endif  // D_MOVE_ENABLED

/*
tests_tls_run_all
  Runs every test, reporting each, and the tally.
*/
bool
tests_tls_run_all()
{
    struct entry
    {
        const char* name;
        bool        (*run)();
    };

    static const entry CASES[] =
    {
        { "contexts",     tests_tls_context },
        { "preparing",    tests_tls_prepare },
        { "settings",     tests_tls_settings },
        { "naming",       tests_tls_naming },
        { "remaking",     tests_tls_remaking },
        { "identities",   tests_tls_identity },
        { "adopting",     tests_tls_adopt },
        { "loopback",     tests_tls_loopback },
        { "server names", tests_tls_hostname },
        { "untrusted",    tests_tls_untrusted },
        { "verify off",   tests_tls_verify_off },
        { "versions",     tests_tls_versions },
        { "closing",      tests_tls_closing },
        { "truncation",   tests_tls_truncation },
        { "outliving",    tests_tls_outlives },
        { "ALPN",         tests_tls_alpn },
        { "upgrades",     tests_tls_upgrade },
#if (D_MOVE_ENABLED == 1)
        { "moves",        tests_tls_moves },
        { "moving",       tests_tls_moving },
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

    std::printf("%lu/%lu checks\n",
                static_cast<unsigned long>(internal::counts().checks -
                                           internal::counts().failures),
                static_cast<unsigned long>(internal::counts().checks));

    return all;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_INTERNAL_NET_HAS_INT64
