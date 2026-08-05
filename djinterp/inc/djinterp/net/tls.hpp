/******************************************************************************
* djinterp [net]                                                       tls.hpp
*
*   TLS backend for the net subframework (OpenSSL family). It layers TLS over
* an arbitrary net transport rather than over a raw descriptor: a tls_connection
* wraps any net::connection and drives OpenSSL through a pair of memory BIOs,
* shuttling ciphertext to and from the underlying transport with its ordinary
* read()/write(). TLS therefore composes with every backend -- TCP today, unix
* sockets, or a test transport -- and the connector/acceptor are templated on
* the transport connector/acceptor (defaulting to the TCP ones in tcp.hpp).
*
* CONTENTS (namespace djinterp::net):
*   internal:      SSL error mapping, memory-BIO setup, small helpers
*   tls_context    an RAII SSL_CTX (client or server); verification + cert/key
*   tls_connection a connection layering TLS over an owned transport connection
*   tls_connector  models connector -- connects the transport, then handshakes
*                  (eagerly, so verification failures surface at connect time)
*   tls_acceptor   models acceptor  -- accepts the transport; the TLS handshake
*                  is deferred to the first I/O (off the accept loop)
*
*   ROBUSTNESS: the blocking handshake/read/write pump handles WANT_READ /
* WANT_WRITE against the memory BIOs; a clean close_notify maps to EOF; the
* OpenSSL error queue is cleared around calls; SIGPIPE etc. from the transport
* are already handled beneath. Client verification is on by default (system CA
* paths, plus hostname or IP matching); it can be disabled per connector.
*
*   Requires:  net.hpp, tcp.hpp (default transport), env_tls.h, and the OpenSSL
*              API family (D_ENV_TLS_HAS_OPENSSL). Link: -lssl -lcrypto.
*
* path:      /inc/djinterp/net/tls.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.17
******************************************************************************/

#ifndef DJINTERP_NET_TLS_
#define DJINTERP_NET_TLS_ 1

// djinterp
#include "./net.hpp"
#include "./tcp.hpp"
#include "../core/env/net/env_tls.h"


#if !D_ENV_TLS_HAS_OPENSSL
    #error "net/tls.hpp requires the OpenSSL API family (see env_tls.h)"
#endif


// std / OpenSSL / POSIX
#include <cerrno>
#include <climits>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include <arpa/inet.h>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>


NS_DJINTERP
NS_NET


///////////////////////////////////////////////////////////////////////////////
///                        INTERNAL HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // the pieces of a freshly created SSL wired to memory BIOs
    struct ssl_setup
    {
        SSL* ssl;
        BIO* rbio;   // SSL reads from here; we BIO_write inbound ciphertext in
        BIO* wbio;   // SSL writes here; we BIO_read outbound ciphertext out

        ssl_setup()
            : ssl(nullptr),
              rbio(nullptr),
              wbio(nullptr)
        {
        }
    };

    // make_ssl
    //   function: creates an SSL from the context, attaches two memory BIOs, and
    // transfers BIO ownership to the SSL (freed by SSL_free). Returns false on
    // allocation failure (nothing leaked).
    D_NODISCARD inline bool
    make_ssl(
        SSL_CTX*   _ctx,
        ssl_setup& _out
    )
    {
        if (_ctx == nullptr)
        {
            return false;
        }

        SSL* ssl = SSL_new(_ctx);

        if (ssl == nullptr)
        {
            return false;
        }

        BIO* rbio = BIO_new(BIO_s_mem());
        BIO* wbio = BIO_new(BIO_s_mem());

        if ( (rbio == nullptr) ||
             (wbio == nullptr) )
        {
            if (rbio != nullptr)
            {
                BIO_free(rbio);
            }

            if (wbio != nullptr)
            {
                BIO_free(wbio);
            }

            SSL_free(ssl);
            return false;
        }

        // SSL takes ownership of both BIOs
        SSL_set_bio(ssl, rbio, wbio);

        _out.ssl  = ssl;
        _out.rbio = rbio;
        _out.wbio = wbio;

        return true;
    }

    // ssl_result_to_error
    //   function: maps a fatal SSL_get_error result onto io_error. WANT_READ /
    // WANT_WRITE are handled by the pump and never reach here.
    D_NODISCARD inline io_error
    ssl_result_to_error(
        int _ssl_err
    )
    {
        switch (_ssl_err)
        {
            case SSL_ERROR_ZERO_RETURN:
                // peer sent close_notify -- an orderly TLS shutdown
                return io_error::closed;

            case SSL_ERROR_SYSCALL:
                // a transport-level failure; errno may carry detail, otherwise
                // it is an unexpected EOF mid-protocol
                if (errno != 0)
                {
                    return from_errno(errno);
                }

                return io_error::connection_reset;

            case SSL_ERROR_SSL:
                // a protocol or certificate-verification failure
                return io_error::access_denied;

            default:
                return io_error::unknown;
        }
    }

    // clamp_to_int
    //   function: clamps a size to INT_MAX for the int-typed OpenSSL I/O calls.
    D_NODISCARD inline int
    clamp_to_int(
        std::size_t _n
    )
    {
        if (_n > static_cast<std::size_t>(INT_MAX))
        {
            return INT_MAX;
        }

        return static_cast<int>(_n);
    }

    // is_ip_literal
    //   function: whether the host string is a numeric IPv4/IPv6 literal (so SNI
    // is suppressed and IP-based certificate matching is used).
    D_NODISCARD inline bool
    is_ip_literal(
        const std::string& _host
    )
    {
        unsigned char scratch[16];

        if (::inet_pton(AF_INET, _host.c_str(), scratch) == 1)
        {
            return true;
        }

        if (::inet_pton(AF_INET6, _host.c_str(), scratch) == 1)
        {
            return true;
        }

        return false;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                           TLS CONTEXT                                  ///
///////////////////////////////////////////////////////////////////////////////

// tls_context
//   class: an RAII wrapper over an SSL_CTX, created for a client or a server
// role. Carries shared configuration -- protocol floor (TLS 1.2), peer
// verification, trust anchors, and (server) certificate/key -- and is reused
// across many connections. Movable, non-copyable. Note that each SSL created
// from the context holds its own reference, so the context may be destroyed
// while its connections live on.
class tls_context
{
public:
    enum class role
    {
        client,
        server
    };

    // client / server
    //   function: named constructors for the two roles.
    D_NODISCARD static tls_context
    client()
    {
        return tls_context(role::client);
    }

    D_NODISCARD static tls_context
    server()
    {
        return tls_context(role::server);
    }

    explicit tls_context(
        role _role
    )
        : m_ctx(nullptr)
    {
        const SSL_METHOD* method =
            (_role == role::client) ? TLS_client_method()
                                    : TLS_server_method();

        m_ctx = SSL_CTX_new(method);

        if (m_ctx != nullptr)
        {
            // a sane protocol floor and blocking-friendly retry behaviour
            (void)SSL_CTX_set_min_proto_version(m_ctx, TLS1_2_VERSION);
            (void)SSL_CTX_set_mode(m_ctx, SSL_MODE_AUTO_RETRY);
        }
    }

    ~tls_context()
    {
        if (m_ctx != nullptr)
        {
            SSL_CTX_free(m_ctx);
            m_ctx = nullptr;
        }
    }

    tls_context(
        tls_context&& _other
    ) D_NOEXCEPT
        : m_ctx(_other.m_ctx)
    {
        _other.m_ctx = nullptr;
    }

    tls_context&
    operator=(
        tls_context&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            if (m_ctx != nullptr)
            {
                SSL_CTX_free(m_ctx);
            }

            m_ctx        = _other.m_ctx;
            _other.m_ctx = nullptr;
        }

        return *this;
    }

    tls_context(const tls_context&)            D_DELETE;
    tls_context& operator=(const tls_context&) D_DELETE;

    // is_valid
    //   function: whether the underlying SSL_CTX was created.
    D_NODISCARD bool
    is_valid() const
    {
        return (m_ctx != nullptr);
    }

    // native
    //   function: the underlying SSL_CTX (for advanced configuration).
    D_NODISCARD SSL_CTX*
    native() const
    {
        return m_ctx;
    }

    // set_min_version
    //   function: sets the minimum protocol version (e.g. TLS1_2_VERSION,
    // TLS1_3_VERSION).
    void
    set_min_version(
        int _version
    )
    {
        if (m_ctx != nullptr)
        {
            (void)SSL_CTX_set_min_proto_version(m_ctx, _version);
        }

        return;
    }

    // set_verify_peer
    //   function: turns peer-certificate verification on or off at the context
    // level (a connector may also set this per-connection).
    void
    set_verify_peer(
        bool _on
    )
    {
        if (m_ctx != nullptr)
        {
            SSL_CTX_set_verify(m_ctx,
                               _on ? SSL_VERIFY_PEER : SSL_VERIFY_NONE,
                               nullptr);
        }

        return;
    }

    // use_default_verify_paths
    //   function: loads the system default trust anchors for verification.
    D_NODISCARD io_error
    use_default_verify_paths()
    {
        if (m_ctx == nullptr)
        {
            return io_error::invalid_argument;
        }

        if (SSL_CTX_set_default_verify_paths(m_ctx) != 1)
        {
            return io_error::unknown;
        }

        return io_error::none;
    }

    // use_verify_file
    //   function: loads a specific CA bundle (PEM) as the trust anchors.
    D_NODISCARD io_error
    use_verify_file(
        const std::string& _path
    )
    {
        if (m_ctx == nullptr)
        {
            return io_error::invalid_argument;
        }

        if (SSL_CTX_load_verify_locations(m_ctx, _path.c_str(), nullptr) != 1)
        {
            return io_error::invalid_argument;
        }

        return io_error::none;
    }

    // use_certificate_file
    //   function: loads the server certificate chain (PEM).
    D_NODISCARD io_error
    use_certificate_file(
        const std::string& _path
    )
    {
        if (m_ctx == nullptr)
        {
            return io_error::invalid_argument;
        }

        if (SSL_CTX_use_certificate_chain_file(m_ctx, _path.c_str()) != 1)
        {
            return io_error::invalid_argument;
        }

        return io_error::none;
    }

    // use_private_key_file
    //   function: loads the private key (PEM) and checks it against the loaded
    // certificate.
    D_NODISCARD io_error
    use_private_key_file(
        const std::string& _path
    )
    {
        if (m_ctx == nullptr)
        {
            return io_error::invalid_argument;
        }

        if (SSL_CTX_use_PrivateKey_file(m_ctx,
                                        _path.c_str(),
                                        SSL_FILETYPE_PEM) != 1)
        {
            return io_error::invalid_argument;
        }

        if (SSL_CTX_check_private_key(m_ctx) != 1)
        {
            return io_error::invalid_argument;
        }

        return io_error::none;
    }

private:
    SSL_CTX* m_ctx;
};


///////////////////////////////////////////////////////////////////////////////
///                         TLS CONNECTION                                 ///
///////////////////////////////////////////////////////////////////////////////

// tls_connection
//   class: a net::connection that layers TLS over an owned transport
// connection. read()/write() run the SSL engine against memory BIOs, moving
// ciphertext through the transport. The handshake runs on demand (the client
// connector forces it eagerly; a server defers it to the first I/O). Movable,
// non-copyable. Normally constructed by tls_connector / tls_acceptor.
class tls_connection : public connection
{
public:
    tls_connection(
        std::unique_ptr<connection> _transport,
        SSL*                        _ssl,
        BIO*                        _rbio,
        BIO*                        _wbio
    )
        : m_transport(std::move(_transport)),
          m_ssl(_ssl),
          m_rbio(_rbio),
          m_wbio(_wbio),
          m_handshaked(false)
    {
    }

    ~tls_connection() override
    {
        // no network I/O in the destructor: free the SSL and let the transport
        // close. Callers wanting a clean close_notify use close().
        if (m_ssl != nullptr)
        {
            SSL_free(m_ssl);
            m_ssl  = nullptr;
            m_rbio = nullptr;
            m_wbio = nullptr;
        }
    }

    tls_connection(
        tls_connection&& _other
    ) D_NOEXCEPT
        : connection(),
          m_transport(std::move(_other.m_transport)),
          m_ssl(_other.m_ssl),
          m_rbio(_other.m_rbio),
          m_wbio(_other.m_wbio),
          m_handshaked(_other.m_handshaked)
    {
        _other.m_ssl  = nullptr;
        _other.m_rbio = nullptr;
        _other.m_wbio = nullptr;
    }

    tls_connection&
    operator=(
        tls_connection&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            if (m_ssl != nullptr)
            {
                SSL_free(m_ssl);
            }

            m_transport  = std::move(_other.m_transport);
            m_ssl        = _other.m_ssl;
            m_rbio       = _other.m_rbio;
            m_wbio       = _other.m_wbio;
            m_handshaked = _other.m_handshaked;

            _other.m_ssl  = nullptr;
            _other.m_rbio = nullptr;
            _other.m_wbio = nullptr;
        }

        return *this;
    }

    tls_connection(const tls_connection&)            D_DELETE;
    tls_connection& operator=(const tls_connection&) D_DELETE;

    // handshake
    //   function: drives the TLS handshake to completion (idempotent once done).
    // Called eagerly by the client connector; otherwise triggered by the first
    // read()/write().
    D_NODISCARD io_error
    handshake()
    {
        if (m_handshaked)
        {
            return io_error::none;
        }

        if ( (m_ssl == nullptr) ||
             (!m_transport) )
        {
            return io_error::closed;
        }

        for (;;)
        {
            ERR_clear_error();

            const int rc = SSL_do_handshake(m_ssl);

            if (rc == 1)
            {
                // send any final handshake output
                const io_error e = flush_out();

                if (e != io_error::none)
                {
                    return e;
                }

                m_handshaked = true;
                return io_error::none;
            }

            const int err = SSL_get_error(m_ssl, rc);

            if (err == SSL_ERROR_WANT_READ)
            {
                io_error e = flush_out();

                if (e != io_error::none)
                {
                    return e;
                }

                e = feed_in();

                if (e != io_error::none)
                {
                    return e;
                }
            }
            else if (err == SSL_ERROR_WANT_WRITE)
            {
                const io_error e = flush_out();

                if (e != io_error::none)
                {
                    return e;
                }
            }
            else
            {
                return internal::ssl_result_to_error(err);
            }
        }
    }

    // read
    //   function: decrypts up to `_size` bytes of application data; a clean
    // close_notify from the peer reads as EOF (0, none).
    D_NODISCARD io_result
    read(
        void*       _buffer,
        std::size_t _size
    ) override
    {
        if (m_ssl == nullptr)
        {
            return io_result(0, io_error::closed);
        }

        if (_size == 0)
        {
            return io_result(0, io_error::none);
        }

        if (!m_handshaked)
        {
            const io_error e = handshake();

            if (e != io_error::none)
            {
                return io_result(0, e);
            }
        }

        for (;;)
        {
            ERR_clear_error();

            const int rc = SSL_read(m_ssl, _buffer, internal::clamp_to_int(_size));

            if (rc > 0)
            {
                return io_result(static_cast<std::size_t>(rc), io_error::none);
            }

            const int err = SSL_get_error(m_ssl, rc);

            if (err == SSL_ERROR_ZERO_RETURN)
            {
                return io_result(0, io_error::none);
            }

            if (err == SSL_ERROR_WANT_READ)
            {
                io_error e = flush_out();

                if (e != io_error::none)
                {
                    return io_result(0, e);
                }

                e = feed_in();

                if (e != io_error::none)
                {
                    return io_result(0, e);
                }

                continue;
            }

            if (err == SSL_ERROR_WANT_WRITE)
            {
                const io_error e = flush_out();

                if (e != io_error::none)
                {
                    return io_result(0, e);
                }

                continue;
            }

            return io_result(0, internal::ssl_result_to_error(err));
        }
    }

    // write
    //   function: encrypts and sends up to `_size` bytes. On success the full
    // amount is accepted by the SSL engine and its ciphertext flushed to the
    // transport.
    D_NODISCARD io_result
    write(
        const void* _data,
        std::size_t _size
    ) override
    {
        if (m_ssl == nullptr)
        {
            return io_result(0, io_error::closed);
        }

        if (_size == 0)
        {
            return io_result(0, io_error::none);
        }

        if (!m_handshaked)
        {
            const io_error e = handshake();

            if (e != io_error::none)
            {
                return io_result(0, e);
            }
        }

        for (;;)
        {
            ERR_clear_error();

            const int rc = SSL_write(m_ssl, _data, internal::clamp_to_int(_size));

            if (rc > 0)
            {
                const io_error e = flush_out();

                if (e != io_error::none)
                {
                    return io_result(0, e);
                }

                return io_result(static_cast<std::size_t>(rc), io_error::none);
            }

            const int err = SSL_get_error(m_ssl, rc);

            if (err == SSL_ERROR_WANT_READ)
            {
                io_error e = flush_out();

                if (e != io_error::none)
                {
                    return io_result(0, e);
                }

                e = feed_in();

                if (e != io_error::none)
                {
                    return io_result(0, e);
                }

                continue;
            }

            if (err == SSL_ERROR_WANT_WRITE)
            {
                const io_error e = flush_out();

                if (e != io_error::none)
                {
                    return io_result(0, e);
                }

                continue;
            }

            return io_result(0, internal::ssl_result_to_error(err));
        }
    }

    // is_open
    //   function: whether the TLS layer and its transport are both live.
    D_NODISCARD bool
    is_open() const override
    {
        return ( (m_ssl != nullptr) &&
                 (m_transport) &&
                 m_transport->is_open() );
    }

    // close
    //   function: sends a best-effort close_notify, frees the SSL, and closes
    // the transport (idempotent).
    void
    close() override
    {
        if (m_ssl != nullptr)
        {
            ERR_clear_error();
            (void)SSL_shutdown(m_ssl);   // queue close_notify
            (void)flush_out();           // best-effort delivery

            SSL_free(m_ssl);
            m_ssl  = nullptr;
            m_rbio = nullptr;
            m_wbio = nullptr;
        }

        if (m_transport)
        {
            m_transport->close();
        }

        return;
    }

    // shutdown
    //   function: for the write direction, sends close_notify; delegates the
    // transport-level shutdown to the underlying connection.
    D_NODISCARD io_error
    shutdown(
        shutdown_mode _mode
    ) override
    {
        if (m_ssl == nullptr)
        {
            return io_error::closed;
        }

        if ( (_mode == shutdown_mode::write) ||
             (_mode == shutdown_mode::both) )
        {
            ERR_clear_error();
            (void)SSL_shutdown(m_ssl);

            const io_error e = flush_out();

            if (e != io_error::none)
            {
                return e;
            }
        }

        if (m_transport)
        {
            return m_transport->shutdown(_mode);
        }

        return io_error::none;
    }

    // remote_endpoint / local_endpoint
    //   function: the addresses of the underlying transport.
    D_NODISCARD endpoint
    remote_endpoint() const override
    {
        if (m_transport)
        {
            return m_transport->remote_endpoint();
        }

        return endpoint();
    }

    D_NODISCARD endpoint
    local_endpoint() const override
    {
        if (m_transport)
        {
            return m_transport->local_endpoint();
        }

        return endpoint();
    }

    // native_ssl
    //   function: the underlying SSL object (for advanced inspection, e.g. the
    // negotiated cipher or peer certificate).
    D_NODISCARD SSL*
    native_ssl() const
    {
        return m_ssl;
    }

private:
    // flush_out
    //   function: drains ciphertext OpenSSL has produced (write BIO) and sends
    // it over the transport.
    D_NODISCARD io_error
    flush_out()
    {
        char buffer[16384];

        for (;;)
        {
            const int n = BIO_read(m_wbio, buffer, static_cast<int>(sizeof(buffer)));

            if (n <= 0)
            {
                // nothing more pending (memory BIO returns <= 0 when empty)
                break;
            }

            const io_error e =
                transport_write_all(buffer, static_cast<std::size_t>(n));

            if (e != io_error::none)
            {
                return e;
            }
        }

        return io_error::none;
    }

    // feed_in
    //   function: reads ciphertext from the transport (blocking) and hands it to
    // OpenSSL (read BIO). A transport EOF before the engine is satisfied is a
    // truncation.
    D_NODISCARD io_error
    feed_in()
    {
        char buffer[16384];

        const io_result r = m_transport->read(buffer, sizeof(buffer));

        if (r.error != io_error::none)
        {
            return r.error;
        }

        if (r.count == 0)
        {
            return io_error::closed;
        }

        const int written =
            BIO_write(m_rbio, buffer, static_cast<int>(r.count));

        if (written < static_cast<int>(r.count))
        {
            // a memory BIO should accept everything; short write is unexpected
            return io_error::unknown;
        }

        return io_error::none;
    }

    // transport_write_all
    //   function: writes the whole buffer to the transport, looping over partial
    // writes.
    D_NODISCARD io_error
    transport_write_all(
        const char* _data,
        std::size_t _length
    )
    {
        std::size_t offset = 0;

        while (offset < _length)
        {
            const io_result r =
                m_transport->write(_data + offset, _length - offset);

            if (r.error != io_error::none)
            {
                return r.error;
            }

            if (r.count == 0)
            {
                return io_error::closed;
            }

            offset += r.count;
        }

        return io_error::none;
    }

    std::unique_ptr<connection> m_transport;
    SSL*                        m_ssl;
    BIO*                        m_rbio;
    BIO*                        m_wbio;
    bool                        m_handshaked;
};


///////////////////////////////////////////////////////////////////////////////
///                          TLS CONNECTOR                                 ///
///////////////////////////////////////////////////////////////////////////////

// tls_connector
//   class: a net::connector that establishes a transport connection with the
// underlying connector, then negotiates TLS over it. Verification is on by
// default (system trust anchors, hostname/IP matching against the target); the
// handshake runs eagerly so a certificate or verification failure is reported
// as the connect error. Templated on the transport connector (default TCP).
template <typename _TransportConnector = tcp_connector>
class tls_connector
{
public:
    tls_connector()
        : m_context(tls_context::client()),
          m_transport(),
          m_verify(true),
          m_server_name()
    {
        init_client_defaults();
    }

    explicit tls_connector(
        const _TransportConnector& _transport
    )
        : m_context(tls_context::client()),
          m_transport(_transport),
          m_verify(true),
          m_server_name()
    {
        init_client_defaults();
    }

    // context / transport
    //   function: access to the TLS context and the transport connector for
    // further configuration.
    D_NODISCARD tls_context&
    context()
    {
        return m_context;
    }

    D_NODISCARD _TransportConnector&
    transport()
    {
        return m_transport;
    }

    // set_verify
    //   function: enables/disables peer verification for connections made by
    // this connector.
    void
    set_verify(
        bool _on
    )
    {
        m_verify = _on;
        m_context.set_verify_peer(_on);

        return;
    }

    // set_server_name
    //   function: overrides the name used for SNI and certificate matching
    // (defaults to the endpoint host).
    void
    set_server_name(
        const std::string& _name
    )
    {
        m_server_name = _name;

        return;
    }

    // connect
    //   function: connects the transport, wraps it in TLS, and handshakes.
    D_NODISCARD open_result
    connect(
        const endpoint& _endpoint
    )
    {
        if (!m_context.is_valid())
        {
            return open_result(io_error::invalid_argument);
        }

        // establish the underlying transport connection first
        open_result transport = m_transport.connect(_endpoint);

        if (!transport.ok())
        {
            return transport;
        }

        internal::ssl_setup setup;

        if (!internal::make_ssl(m_context.native(), setup))
        {
            return open_result(io_error::out_of_memory);
        }

        SSL_set_connect_state(setup.ssl);

        const std::string name =
            m_server_name.empty() ? _endpoint.host : m_server_name;

        configure_verification(setup.ssl, name);

        std::unique_ptr<connection> conn(
            new tls_connection(std::move(transport.conn),
                               setup.ssl,
                               setup.rbio,
                               setup.wbio));

        // eager handshake: surface verification failures at connect time
        tls_connection* tls = static_cast<tls_connection*>(conn.get());

        const io_error e = tls->handshake();

        if (e != io_error::none)
        {
            return open_result(e);
        }

        return open_result(std::move(conn));
    }

private:
    // init_client_defaults
    //   function: loads system trust anchors and sets the default verify mode.
    void
    init_client_defaults()
    {
        (void)m_context.use_default_verify_paths();
        m_context.set_verify_peer(m_verify);

        return;
    }

    // configure_verification
    //   function: sets SNI (for hostnames) and the certificate-matching target
    // (hostname or IP) when verification is enabled.
    void
    configure_verification(
        SSL*               _ssl,
        const std::string& _name
    )
    {
        if (_name.empty())
        {
            SSL_set_verify(_ssl,
                           m_verify ? SSL_VERIFY_PEER : SSL_VERIFY_NONE,
                           nullptr);
            return;
        }

        const bool ip = internal::is_ip_literal(_name);

        // SNI is only meaningful for hostnames, never IP literals
        if (!ip)
        {
            (void)SSL_set_tlsext_host_name(_ssl, _name.c_str());
        }

        if (m_verify)
        {
            SSL_set_verify(_ssl, SSL_VERIFY_PEER, nullptr);

        #if D_ENV_TLS_HAS_HOSTNAME_VALIDATION
            if (ip)
            {
                X509_VERIFY_PARAM* param = SSL_get0_param(_ssl);
                (void)X509_VERIFY_PARAM_set1_ip_asc(param, _name.c_str());
            }
            else
            {
                (void)SSL_set1_host(_ssl, _name.c_str());
            }
        #endif
        }
        else
        {
            SSL_set_verify(_ssl, SSL_VERIFY_NONE, nullptr);
        }

        return;
    }

    tls_context         m_context;
    _TransportConnector m_transport;
    bool                m_verify;
    std::string         m_server_name;
};


///////////////////////////////////////////////////////////////////////////////
///                          TLS ACCEPTOR                                  ///
///////////////////////////////////////////////////////////////////////////////

// tls_acceptor
//   class: a net::acceptor that accepts a transport connection and wraps it in
// TLS. The handshake is deferred to the accepted connection's first read()/
// write(), so it runs in whatever thread the server dispatches the connection
// to rather than on the accept loop -- a slow or hostile client cannot stall
// acceptance. The context must carry a server certificate and key. Templated on
// the transport acceptor (default TCP).
template <typename _TransportAcceptor = tcp_acceptor>
class tls_acceptor
{
public:
    tls_acceptor()
        : m_context(tls_context::server()),
          m_transport()
    {
    }

    explicit tls_acceptor(
        tls_context&& _context
    )
        : m_context(std::move(_context)),
          m_transport()
    {
    }

    // context / transport
    //   function: access to the TLS context (to load cert/key) and the
    // transport acceptor.
    D_NODISCARD tls_context&
    context()
    {
        return m_context;
    }

    D_NODISCARD _TransportAcceptor&
    transport()
    {
        return m_transport;
    }

    // bind
    //   function: binds the transport acceptor.
    D_NODISCARD io_error
    bind(
        const endpoint& _endpoint
    )
    {
        return m_transport.bind(_endpoint);
    }

    // accept
    //   function: accepts a transport connection and returns it wrapped in TLS
    // (handshake deferred to first I/O).
    D_NODISCARD open_result
    accept()
    {
        if (!m_context.is_valid())
        {
            return open_result(io_error::invalid_argument);
        }

        open_result transport = m_transport.accept();

        if (!transport.ok())
        {
            return transport;
        }

        internal::ssl_setup setup;

        if (!internal::make_ssl(m_context.native(), setup))
        {
            return open_result(io_error::out_of_memory);
        }

        SSL_set_accept_state(setup.ssl);

        return open_result(
            std::unique_ptr<connection>(
                new tls_connection(std::move(transport.conn),
                                   setup.ssl,
                                   setup.rbio,
                                   setup.wbio)));
    }

    // is_open
    //   function: whether the transport acceptor is open.
    D_NODISCARD bool
    is_open() const
    {
        return m_transport.is_open();
    }

    // close
    //   function: closes the transport acceptor (unblocks a pending accept).
    void
    close()
    {
        m_transport.close();

        return;
    }

    // local_endpoint
    //   function: the transport acceptor's bound address.
    D_NODISCARD endpoint
    local_endpoint() const
    {
        return m_transport.local_endpoint();
    }

private:
    tls_context        m_context;
    _TransportAcceptor m_transport;
};


NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_TLS_
