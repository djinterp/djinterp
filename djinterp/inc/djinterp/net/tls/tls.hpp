/*******************************************************************************
* djinterp [net]                                                         tls.hpp
*
* TLS for net.hpp: the SSL kernel's contexts and sessions as C++ classes.
*   tls_context is a configuration of ssl.h's kernel, made live in an engine
* -- the OpenSSL family's unless another is given -- by prepare(), by the
* first connection that needs it, or as soon as a use_* call completes it.
* tls_connection is a net::connection running a kernel session over a
* transport connection it owns, which adopt makes of any connection, one a
* protocol opened in plaintext included. tls_connector connects a transport
* and handshakes at once, so that a refused certificate is the connect's
* error, and tls_acceptor accepts one and leaves the handshake to the
* connection's first read or write, off the accept loop. Both are templated
* on the transport's connector or acceptor, tcp.hpp's by default.
*   The names and signatures are the replaced tls.hpp's, save where it named
* OpenSSL: config() and the engine take native()'s place, session() and
* info() native_ssl()'s, and set_min_version takes a version::value. The
* kernel checks names, versions and pins itself, the same under every
* engine, and this header names no TLS library: without an engine a context
* is not valid, and connects and accepts with it fail.
*   A context's settings apply to the connections made after them. Each
* connection keeps a share of the kernel context it was made with, so a
* context, or its connector or acceptor, may go while its connections live
* on; the shares are counted under a dmutex.h lock, since connections end on
* threads of their own. A context is changed only while no connect or
* accept is using it.
*   C++98 or later, wherever net.h has a 64-bit integer type, as the kernel
* needs; elsewhere the header is empty. connect and accept fill an
* open_result in place at every level, and from C++11 also return one; the
* context and the connection move from C++11 and copy at no level.
* Enumerations are scoped in structs (decision 3.4). Link ssl.c,
* ssl_openssl.c, net.c, tcp.c, dmutex.c, sink_common.c and the codec's
* sources, with the engine's libraries: -lssl -lcrypto for OpenSSL's.
*
*   Where D_INTERNAL_NET_HAS_INT64 is 0, with no 64-bit integer type, it
* compiles to nothing, unless D_CFG_NET_REQUIRE_BACKENDS asks for an #error
* (decision 101 of the register).
*
*
* path:      /inc/djinterp/net/tls/tls.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONTEXTS
    --------
    1.  Shared kernel contexts
         1.  internal::tls_share
         2.  internal::tls_share_make
         3.  internal::tls_share_retain
         4.  internal::tls_share_release
    2.  Contexts
         1.  tls_lease
         2.  tls_context
2.  CONNECTIONS
    -----------
    1.  Errors and names
         1.  internal::tls_error
         2.  internal::tls_host
    2.  TLS connections
         1.  tls_connection
3.  OPENING
    -------
    1.  Connecting
         1.  tls_connector
    2.  Accepting
         1.  tls_acceptor
*/

#ifndef DJINTERP_NET_TLS_TLS_HPP
#define DJINTERP_NET_TLS_TLS_HPP 1

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::fopen, std::fclose
#include <new>      // std::nothrow
#include <string>   // std::string
// djinterp
#include "../../c/sync/dmutex.h"  // d_mutex_t
#include "../../djinterp.hpp"     // framework root
#include "../net_common.hpp"      // connection, open_result, io_error
#include "../net_common.h"         // D_INTERNAL_NET_HAS_INT64
#include "../ssl/ssl.h"           // d_ssl_context, d_ssl_session
#include "../ssl/ssl_openssl.h"   // d_ssl_engine_openssl
#include "../tcp/tcp.hpp"         // tcp_connector, tcp_acceptor
#include "../../config/net/cfg_net.h"  // D_INTERNAL_NET_REQUIRE_BACKENDS


// the kernel needs a 64-bit integer type; where there is none (ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS)
// there is no TLS, and nothing here
#if (D_INTERNAL_NET_HAS_INT64 == 1)


NS_DJINTERP
NS_NET


//==============================================================================
// 1.  CONTEXTS
//==============================================================================


// 1.1    Shared kernel contexts
//------------------------------------------------------------------------------
NS_INTERNAL

    // 1.1.1
    // internal::tls_share
    //   struct: a kernel context made live, and how many hold it: the
    // tls_context that made it, for as long as it keeps it, and each
    // connection made with it. The last to let go destroys it.
    struct tls_share
    {
        ::d_ssl_context kernel;
        d_mutex_t       lock;
        long            holders;
    };

    // 1.1.2
    // internal::tls_share_make
    //   function: a share of `_config` made live in `_engine`, held once;
    // null, with the kernel's reason in *_out_status, when it cannot be.
    D_NODISCARD inline tls_share*
    tls_share_make(
        const ::d_ssl_engine* _engine,
        const ::d_ssl_config& _config,
        ::d_ssl_error*        _out_status
    ) D_NOEXCEPT
    {
        tls_share* const share = new (std::nothrow) tls_share();

        // no room for the share, or for its lock
        if ( (!share) ||
             (::d_mutex_init(&share->lock) != D_MUTEX_SUCCESS) )
        {
            delete share;
            *_out_status = D_SSL_ERROR_OUT_OF_MEMORY;

            return D_NULLPTR;
        }

        *_out_status = ::d_ssl_context_init(&share->kernel,
                                            _engine,
                                            &_config);

        // a configuration the kernel or the engine refuses
        if (*_out_status != D_SSL_OK)
        {
            (void)::d_mutex_destroy(&share->lock);
            delete share;

            return D_NULLPTR;
        }

        share->holders = 1;

        return share;
    }

    // 1.1.3
    // internal::tls_share_retain
    //   function: one more holder of `_share`, which it returns.
    inline tls_share*
    tls_share_retain(
        tls_share* _share
    ) D_NOEXCEPT
    {
        (void)::d_mutex_lock(&_share->lock);
        _share->holders += 1;
        (void)::d_mutex_unlock(&_share->lock);

        return _share;
    }

    // 1.1.4
    // internal::tls_share_release
    //   function: one holder fewer of `_share`, destroying it with the last;
    // null is ignored.
    inline void
    tls_share_release(
        tls_share* _share
    ) D_NOEXCEPT
    {
        // nothing held
        if (!_share)
        {
            return;
        }

        (void)::d_mutex_lock(&_share->lock);
        _share->holders -= 1;

        const long left = _share->holders;

        (void)::d_mutex_unlock(&_share->lock);

        // the last holder takes the context with it
        if (left == 0)
        {
            ::d_ssl_context_destroy(&_share->kernel);
            (void)::d_mutex_destroy(&_share->lock);
            delete _share;
        }

        return;
    }

NS_END  // internal

// 1.2    Contexts
//------------------------------------------------------------------------------
class tls_connection;

// 1.2.1
// tls_lease
//   class: a share of a tls_context's live kernel context, for another
// facade's sessions to use -- smtp.hpp's tls_options take one -- which
// keeps that context alive for as long as the lease lives, whatever becomes
// of the tls_context; copies share it. An empty lease holds nothing.
class tls_context;

class tls_lease
{
public:
    tls_lease() D_NOEXCEPT
        : m_share(D_NULLPTR)
    {}

    tls_lease(
        const tls_lease& _other
    ) D_NOEXCEPT
        : m_share((_other.m_share)
                      ? internal::tls_share_retain(_other.m_share)
                      : D_NULLPTR)
    {}

    ~tls_lease()
    {
        internal::tls_share_release(m_share);
    }

    tls_lease&
    operator=(
        const tls_lease& _other
    ) D_NOEXCEPT
    {
        // retained first, so a lease assigned to itself keeps its share
        internal::tls_share* const held =
            (_other.m_share) ? internal::tls_share_retain(_other.m_share)
                             : D_NULLPTR;

        internal::tls_share_release(m_share);
        m_share = held;

        return *this;
    }

    // get
    //   function: the lent kernel context, or null for an empty lease.
    D_NODISCARD const ::d_ssl_context*
    get() const D_NOEXCEPT
    {
        return (m_share) ? &m_share->kernel : D_NULLPTR;
    }

private:
    friend class tls_context;

    explicit tls_lease(
        internal::tls_share* _share
    ) D_NOEXCEPT
        : m_share(_share)
    {}

    internal::tls_share* m_share;
};

// 1.2.2
// tls_context
//   class: a TLS configuration for one role, and the kernel context made live
// from it, which it shares with each connection made since. The defaults
// are the kernel's: a client verifies the server's chain against the
// system's trust anchors, and its name; a server presents the certificate
// and key it is given. Each change lets go of the live context, and the next
// prepare, connect or accept makes a new one, so settings apply to the
// connections made after them.
//   The use_* calls name PEM files, which the kernel loads when the context
// is made live; a file that cannot be read is refused at once, and named
// nowhere. All but use_certificate_file then make a complete context live
// -- a client's, unless it has half an identity; a server's, once it has a
// certificate and key -- so a file that does not load, a key that does not
// match, or any setting the kernel refuses is that call's error, with the
// kernel's reason in failure(). A file refused so stays named, and connects
// and accepts go on failing until one that loads takes its place.
//   The engine is the OpenSSL family's unless one is given; a build without
// it, or a context moved from, is not valid. Not copyable; movable from
// C++11.
class tls_context
{
public:
    // role
    //   enum: which end of a connection the context serves; the type is
    // role::value (decision 3.4).
    struct role
    {
        enum value
        {
            client,
            server
        };
    };

    // version
    //   enum: a protocol version, as the kernel numbers it.
    struct version
    {
        enum value
        {
            tls1_2 = D_SSL_VERSION_TLS1_2,
            tls1_3 = D_SSL_VERSION_TLS1_3
        };
    };

#if (D_MOVE_ENABLED == 1)
    // client / server
    //   function: named constructors for the two roles; they return the
    // context by value, which C++11 can move.
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
#endif  // D_MOVE_ENABLED

    // a context for `_role` with the kernel's defaults, made live in
    // `_engine` when first needed
    explicit tls_context(
        role::value           _role,
        const ::d_ssl_engine* _engine = ::d_ssl_engine_openssl()
    )
        : m_engine(_engine),
          m_config(),
          m_ca_file(),
          m_certificate_file(),
          m_private_key_file(),
          m_share(D_NULLPTR),
          m_failure(D_SSL_OK)
    {
        ::d_ssl_config_init(&m_config,
                            (_role == role::client) ? D_SSL_ROLE_CLIENT
                                                    : D_SSL_ROLE_SERVER);
    }

    ~tls_context()
    {
        internal::tls_share_release(m_share);
    }

#if (D_MOVE_ENABLED == 1)
    tls_context(
        tls_context&& _other
    ) D_NOEXCEPT
        : m_engine(_other.m_engine),
          m_config(_other.m_config),
          m_ca_file(static_cast<std::string&&>(_other.m_ca_file)),
          m_certificate_file(
              static_cast<std::string&&>(_other.m_certificate_file)),
          m_private_key_file(
              static_cast<std::string&&>(_other.m_private_key_file)),
          m_share(_other.m_share),
          m_failure(_other.m_failure)
    {
        _other.m_engine = D_NULLPTR;
        _other.m_share  = D_NULLPTR;
    }

    tls_context&
    operator=(
        tls_context&& _other
    ) D_NOEXCEPT
    {
        // assigning a context to itself keeps it
        if (this != &_other)
        {
            internal::tls_share_release(m_share);

            m_engine           = _other.m_engine;
            m_config           = _other.m_config;
            m_ca_file          = static_cast<std::string&&>(_other.m_ca_file);
            m_certificate_file =
                static_cast<std::string&&>(_other.m_certificate_file);
            m_private_key_file =
                static_cast<std::string&&>(_other.m_private_key_file);
            m_share            = _other.m_share;
            m_failure          = _other.m_failure;
            _other.m_engine    = D_NULLPTR;
            _other.m_share     = D_NULLPTR;
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // is_valid
    //   function: whether there is an engine to make connections in.
    D_NODISCARD bool
    is_valid() const
    {
        return (m_engine != D_NULLPTR);
    }

    // engine
    //   function: the engine connections are made in, or null.
    D_NODISCARD const ::d_ssl_engine*
    engine() const
    {
        return m_engine;
    }

    // config
    //   function: the kernel configuration the context is made live from,
    // for what no call here sets -- ALPN, pins, cipher strings, PEM held in
    // memory, a key log. Strings it is given must last until the context is
    // next prepared; the files use_* name are kept here, and take the
    // place of any set through config(). Asking for the configuration to
    // change it lets go of the live context.
    D_NODISCARD ::d_ssl_config&
    config()
    {
        m_drop();

        return m_config;
    }

    D_NODISCARD const ::d_ssl_config&
    config() const
    {
        return m_config;
    }

    // set_min_version
    //   function: the oldest version connections settle for.
    void
    set_min_version(
        version::value _version
    )
    {
        m_drop();
        m_config.min_version = static_cast<enum d_ssl_version>(_version);

        return;
    }

    // set_verify_peer
    //   function: whether the peer is verified. A client checks the server's
    // chain and name; a server asks for a client's certificate, and checks
    // one that is sent.
    void
    set_verify_peer(
        bool _on
    )
    {
        const bool                  client = (m_config.role ==
                                              D_SSL_ROLE_CLIENT);
        const enum d_ssl_verify_mode on     = (client)
                                                  ? D_SSL_VERIFY_REQUIRED
                                                  : D_SSL_VERIFY_OPTIONAL;

        m_drop();
        m_config.verify          = (_on) ? on
                                         : D_SSL_VERIFY_NONE;
        m_config.verify_hostname = ( (_on) &&
                                     (client) );

        return;
    }

    // use_default_verify_paths
    //   function: trusts the system's trust anchors, as a context does from
    // the start; a complete context is made live at once.
    D_NODISCARD io_error::value
    use_default_verify_paths()
    {
        m_drop();
        m_config.use_system_trust = true;

        return m_settle();
    }

    // use_verify_file
    //   function: trusts the certificates of a PEM bundle as well; a complete
    // context is made live at once, so a bundle that does not load is
    // refused here.
    D_NODISCARD io_error::value
    use_verify_file(
        const std::string& _path
    )
    {
        return m_name(m_ca_file,
                      _path,
                      true);
    }

    // use_certificate_file
    //   function: presents the certificate chain of a PEM file, leaf first.
    // It waits for its key -- the one named, or one to come -- and makes
    // nothing live, so a certificate replaced before its key is not refused
    // for the key it no longer matches.
    D_NODISCARD io_error::value
    use_certificate_file(
        const std::string& _path
    )
    {
        return m_name(m_certificate_file,
                      _path,
                      false);
    }

    // use_private_key_file
    //   function: the private key of a PEM file, for the certificate; with
    // the context complete it is made live at once, so a key that does not
    // load, or does not match, is refused here.
    D_NODISCARD io_error::value
    use_private_key_file(
        const std::string& _path
    )
    {
        return m_name(m_private_key_file,
                      _path,
                      true);
    }

    /**
     * @brief Makes the context live now, unless it is.
     *
     * Connects and accepts do this themselves; a server can do it early, to
     * learn of a configuration that will not load before it takes clients.
     *
     * @return io_error::none; io_error::invalid_argument without an engine,
     *         or for a configuration the kernel or the engine refuses, such
     *         as a file that does not load; or the engine's failure.
     *         failure() gives the kernel's reason.
     */
    D_NODISCARD io_error::value
    prepare()
    {
        // already live
        if (m_share)
        {
            return io_error::none;
        }

        // no engine to make it live in
        if (!m_engine)
        {
            m_failure = D_SSL_ERROR_UNSUPPORTED;

            return io_error::invalid_argument;
        }

        ::d_ssl_config config = m_config;

        config.ca_file          = m_file(m_ca_file,
                                         m_config.ca_file);
        config.certificate_file = m_file(m_certificate_file,
                                         m_config.certificate_file);
        config.private_key_file = m_file(m_private_key_file,
                                         m_config.private_key_file);
        m_share                 = internal::tls_share_make(m_engine,
                                                           config,
                                                           &m_failure);

        return (m_share) ? io_error::none
                         : from_c(::d_ssl_error_to_net(m_failure));
    }

    // lease
    //   function: a share of the live context for another facade's sessions,
    // made live now if need be; an empty lease, the reason in failure(),
    // when it cannot be.
    D_NODISCARD tls_lease
    lease()
    {
        io_error::value error = io_error::none;

        return tls_lease(m_take(&error));
    }

    // failure
    //   function: the kernel's reason the last prepare failed, or D_SSL_OK;
    // a change forgets it.
    D_NODISCARD ::d_ssl_error
    failure() const
    {
        return m_failure;
    }

private:
    friend class tls_connection;

    // m_take
    //   function: a share of the live context for a connection, made live
    // now if need be; null, with the reason in *_out_error, without one.
    internal::tls_share*
    m_take(
        io_error::value* _out_error
    )
    {
        *_out_error = prepare();

        return (*_out_error == io_error::none)
                   ? internal::tls_share_retain(m_share)
                   : D_NULLPTR;
    }

    // m_drop
    //   function: lets go of the live context, which connections made with
    // it keep for as long as they last.
    void
    m_drop()
    {
        internal::tls_share_release(m_share);
        m_share   = D_NULLPTR;
        m_failure = D_SSL_OK;

        return;
    }

    // m_name
    //   function: names a file for `_slot`, once it can be read; `_settle`
    // makes the context live if it is then complete.
    io_error::value
    m_name(
        std::string&       _slot,
        const std::string& _path,
        bool               _settle
    )
    {
        std::FILE* const file = (_path.empty()) ? D_NULLPTR
                                                : std::fopen(_path.c_str(),
                                                             "rb");

        // a file that cannot be read can name nothing
        if (!file)
        {
            return io_error::invalid_argument;
        }

        (void)std::fclose(file);
        m_drop();
        _slot = _path;

        return (_settle) ? m_settle()
                         : io_error::none;
    }

    // m_settle
    //   function: makes the context live if it is complete, as the kernel
    // judges an identity: a certificate and its key together, and a
    // server's at all. One that is not waits, and is not refused.
    io_error::value
    m_settle()
    {
        const bool certificate =
            ( (!m_certificate_file.empty())            ||
              (m_config.certificate_file != D_NULLPTR) ||
              (m_config.certificate_pem.length > 0u) );
        const bool key         =
            ( (!m_private_key_file.empty())            ||
              (m_config.private_key_file != D_NULLPTR) ||
              (m_config.private_key_pem.length > 0u) );

        // half an identity, or a server without one, waits for the rest
        if ( (certificate != key) ||
             ( (!certificate) &&
               (m_config.role == D_SSL_ROLE_SERVER) ) )
        {
            return io_error::none;
        }

        return prepare();
    }

    // m_file
    //   function: the file a use_* call named, or else the configuration's.
    static const char*
    m_file(
        const std::string& _named,
        const char*        _configured
    )
    {
        return (_named.empty()) ? _configured
                                : _named.c_str();
    }

    D_DELETED_FN(tls_context(const tls_context&))
    D_DELETED_FN(tls_context& operator=(const tls_context&))

    const ::d_ssl_engine* m_engine;            // null: not valid
    ::d_ssl_config        m_config;            // all but the use_* files
    std::string           m_ca_file;           // use_verify_file's
    std::string           m_certificate_file;  // use_certificate_file's
    std::string           m_private_key_file;  // use_private_key_file's
    internal::tls_share*  m_share;             // the live context, or null
    ::d_ssl_error         m_failure;           // the last prepare's reason
};


//==============================================================================
// 2.  CONNECTIONS
//==============================================================================


// 2.1    Errors and names
//------------------------------------------------------------------------------
NS_INTERNAL

    // 2.1.1
    // internal::tls_error
    //   function: a failed session call in net.hpp's terms: the transport's
    // own error behind a transport failure, d_ssl_error_to_net's otherwise,
    // as a session's connection view reports them.
    D_NODISCARD inline io_error::value
    tls_error(
        const ::d_ssl_session* _session,
        ::d_ssl_error          _status
    ) D_NOEXCEPT
    {
        return from_c( (_status == D_SSL_ERROR_TRANSPORT)
                           ? ::d_ssl_session_transport_error(_session)
                           : ::d_ssl_error_to_net(_status) );
    }

    // 2.1.2
    // internal::tls_host
    //   function: the host a client's session made from `_config` names, in
    // *_out: `_host`, unless it is neither a host name nor an address
    // literal, when a session that checks no names goes without one, and
    // without SNI, and one that does cannot be made: false.
    D_NODISCARD inline bool
    tls_host(
        const ::d_ssl_config& _config,
        const std::string&    _host,
        std::string*          _out
    )
    {
        const ::d_pack_text text   = { _host.c_str(),
                                       _host.size() };
        const bool          usable = ( (::d_ssl_hostname_is_valid(text)) ||
                                       (::d_ssl_host_is_ip(text)) );

        *_out = (usable) ? _host
                         : std::string();

        return ( (usable) ||
                 (!_config.verify_hostname) );
    }

NS_END  // internal

// 2.2    TLS connections
//------------------------------------------------------------------------------
// 2.2.1
// tls_connection
//   class: a net::connection running a kernel session over a transport
// connection it owns, through net.hpp's c_connection_adapter; reads and
// writes go through the session's connection view, which handshakes first
// where it has not. The session and the adapter live on the heap, as they
// point into each other, so the connection moves from C++11; it copies at
// no level. Destroying it sends nothing: close() says close_notify first.
// Made by adopt, which tls_connector and tls_acceptor call.
class tls_connection : public connection
{
public:
    /**
     * @brief Runs TLS over a transport connection, which it adopts.
     *
     * The handshake waits for handshake(), or the first read or write, so a
     * protocol that began in plaintext can upgrade the connection it has.
     *
     * @param[in]  _transport  the connection, from new; owned from here, and
     *                         deleted even when this fails.
     * @param[in]  _context    the context the session is made with; it is
     *                         prepared if it is not, and the connection
     *                         keeps a share of it.
     * @param[in]  _host       a client's server: a host name or an address
     *                         literal, which the certificate must carry when
     *                         the context verifies, and the name sent as
     *                         SNI. A server's is not used.
     * @param[out] _out        on success owns the TLS connection; otherwise
     *                         holds the error.
     * @return io_error::none, or the error _out records:
     *         io_error::invalid_argument for no transport, a context that
     *         cannot be made live, or a host that is neither a name nor an
     *         address where the context verifies names, which goes unsent
     *         where it does not; or io_error::out_of_memory.
     */
    static io_error::value
    adopt(
        connection*        _transport,
        tls_context&       _context,
        const std::string& _host,
        open_result&       _out
    )
    {
        std::string                host;
        const bool                 named =
            ( (_context.m_config.role == D_SSL_ROLE_SERVER) ||
              (internal::tls_host(_context.m_config,
                                  _host,
                                  &host)) );
        io_error::value            error = io_error::invalid_argument;
        internal::tls_share* const share =
            ( (_transport) &&
              (named) ) ? _context.m_take(&error)
                        : D_NULLPTR;
        tls_connection* const      made  =
            (share) ? m_make(_transport,
                             share,
                             host,
                             &error)
                    : D_NULLPTR;

        // without a share the transport went unused
        if (!share)
        {
            delete _transport;
        }

        // no connection, nothing for the result but the reason
        if (!made)
        {
            _out.fail(error);

            return error;
        }

        _out.reset(made);

        return io_error::none;
    }

    ~tls_connection() D_OVERRIDE
    {
        m_release();
    }

#if (D_MOVE_ENABLED == 1)
    tls_connection(
        tls_connection&& _other
    ) D_NOEXCEPT
        : connection(),
          m_transport(_other.m_transport),
          m_state(_other.m_state),
          m_share(_other.m_share)
    {
        _other.m_transport = D_NULLPTR;
        _other.m_state     = D_NULLPTR;
        _other.m_share     = D_NULLPTR;
    }

    tls_connection&
    operator=(
        tls_connection&& _other
    ) D_NOEXCEPT
    {
        // assigning a connection to itself keeps it
        if (this != &_other)
        {
            m_release();
            m_transport        = _other.m_transport;
            m_state            = _other.m_state;
            m_share            = _other.m_share;
            _other.m_transport = D_NULLPTR;
            _other.m_state     = D_NULLPTR;
            _other.m_share     = D_NULLPTR;
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // handshake
    //   function: runs the handshake to completion; once done, it only
    // flushes. tls_connector calls it at connect; otherwise the first read
    // or write runs it.
    D_NODISCARD io_error::value
    handshake()
    {
        // a connection moved from has nothing to shake
        if (!m_state)
        {
            return io_error::closed;
        }

        const ::d_ssl_error status =
            ::d_ssl_session_handshake(&m_state->session);

        return (status == D_SSL_OK) ? io_error::none
                                    : internal::tls_error(&m_state->session,
                                                          status);
    }

    // read
    //   function: decrypted application data; the peer's close_notify is
    // the end of the stream (0, none).
    D_NODISCARD io_result
    read(
        void*       _buffer,
        std::size_t _size
    ) D_OVERRIDE
    {
        return (m_state) ? from_c(::d_net_connection_read(
                                      &m_state->session.base,
                                      _buffer,
                                      _size))
                         : io_result(0u,
                                     io_error::closed);
    }

    // write
    //   function: encrypts and sends application data.
    D_NODISCARD io_result
    write(
        const void* _data,
        std::size_t _size
    ) D_OVERRIDE
    {
        return (m_state) ? from_c(::d_net_connection_write(
                                      &m_state->session.base,
                                      _data,
                                      _size))
                         : io_result(0u,
                                     io_error::closed);
    }

    // is_open
    //   function: whether the session and its transport are both live.
    D_NODISCARD bool
    is_open() const D_OVERRIDE
    {
        return ( (m_state != D_NULLPTR)                               &&
                 (::d_net_connection_is_open(&m_state->session.base)) &&
                 (m_transport->is_open()) );
    }

    // close
    //   function: close_notify where the session can still say it, then the
    // transport closed; closing again does nothing more.
    void
    close() D_OVERRIDE
    {
        // the session's view says close_notify, the transport closes after
        if (m_state)
        {
            ::d_net_connection_close(&m_state->session.base);
            m_transport->close();
        }

        return;
    }

    // shutdown
    //   function: WRITE and BOTH send close_notify, then the transport is
    // shut down the same way, as it is for READ alone.
    D_NODISCARD io_error::value
    shutdown(
        shutdown_mode::value _mode
    ) D_OVERRIDE
    {
        return (m_state) ? from_c(::d_net_connection_shutdown(
                                      &m_state->session.base,
                                      to_c(_mode)))
                         : io_error::closed;
    }

    // remote_endpoint / local_endpoint
    //   function: the addresses of the transport.
    D_NODISCARD endpoint
    remote_endpoint() const D_OVERRIDE
    {
        return (m_transport) ? m_transport->remote_endpoint()
                             : endpoint();
    }

    D_NODISCARD endpoint
    local_endpoint() const D_OVERRIDE
    {
        return (m_transport) ? m_transport->local_endpoint()
                             : endpoint();
    }

    // session
    //   function: the kernel session, for what net.hpp's errors cannot
    // say: its failure, state, and transport error; null once moved from.
    D_NODISCARD const ::d_ssl_session*
    session() const
    {
        return (m_state) ? &m_state->session
                         : D_NULLPTR;
    }

    // info
    //   function: what the session negotiated and learned -- the version,
    // the cipher, ALPN's protocol, the verification flags, the peer's
    // certificate fingerprint -- zeroed before the handshake.
    D_NODISCARD ::d_ssl_info
    info() const
    {
        ::d_ssl_info out = ::d_ssl_info();

        // only a session has anything to tell
        if (m_state)
        {
            (void)::d_ssl_session_info(&m_state->session,
                                       &out);
        }

        return out;
    }

private:
    // state
    //   struct: the adapter that shows the session its transport, and the
    // session; they point into each other, so they never move.
    struct state
    {
        explicit state(
            connection& _transport
        )
            : adapter(_transport),
              session()
        {}

        c_connection_adapter adapter;
        ::d_ssl_session      session;
    };

    tls_connection(
        connection*          _transport,
        state*               _state,
        internal::tls_share* _share
    )
        : connection(),
          m_transport(_transport),
          m_state(_state),
          m_share(_share)
    {}

    // m_make
    //   function: a connection whose session, over `_transport`, is made
    // with `_share`, naming `_host`; each is the connection's from here, or
    // released with it when it cannot be made, and null returned, with the
    // reason in *_out_error.
    static tls_connection*
    m_make(
        connection*          _transport,
        internal::tls_share* _share,
        const std::string&   _host,
        io_error::value*     _out_error
    )
    {
        state* const          made   = new (std::nothrow) state(*_transport);
        const ::d_pack_text   host   = { _host.c_str(),
                                         _host.size() };
        const ::d_ssl_error   status =
            (made) ? ::d_ssl_session_init(&made->session,
                                          &_share->kernel,
                                          made->adapter.get(),
                                          host)
                   : D_SSL_ERROR_OUT_OF_MEMORY;
        tls_connection* const tls    =
            (status == D_SSL_OK) ? new (std::nothrow) tls_connection(
                                       _transport,
                                       made,
                                       _share)
                                 : D_NULLPTR;

        // the session, the share and the transport go with a failure
        if (!tls)
        {
            *_out_error = (status == D_SSL_OK)
                              ? io_error::out_of_memory
                              : from_c(::d_ssl_error_to_net(status));

            // a state made holds a session to release
            if (made)
            {
                ::d_ssl_session_destroy(&made->session);
                delete made;
            }

            internal::tls_share_release(_share);
            delete _transport;
        }

        return tls;
    }

    // m_release
    //   function: the session before the context it was made with, and the
    // adapter before the transport it shows.
    void
    m_release()
    {
        // a session to end, and its adapter
        if (m_state)
        {
            ::d_ssl_session_destroy(&m_state->session);
            delete m_state;
            m_state = D_NULLPTR;
        }

        internal::tls_share_release(m_share);
        delete m_transport;
        m_share     = D_NULLPTR;
        m_transport = D_NULLPTR;

        return;
    }

    D_DELETED_FN(tls_connection(const tls_connection&))
    D_DELETED_FN(tls_connection& operator=(const tls_connection&))

    connection*          m_transport;  // owned
    state*               m_state;      // owned
    internal::tls_share* m_share;      // the context the session was made with
};


//==============================================================================
// 3.  OPENING
//==============================================================================


// 3.1    Connecting
//------------------------------------------------------------------------------
// 3.1.1
// tls_connector
//   class: a net::connector that connects the transport with its own
// connector, then runs TLS over it as a client of its context, which
// verifies by default: the server's chain against the system's trust
// anchors, and the server's name -- the endpoint's host, or the name set --
// against its certificate. The handshake runs at connect, so a certificate
// or verification failure is the connect's error. Templated on the
// transport connector (TCP by default).
template<typename TransportConnector = tcp_connector>
class tls_connector
{
public:
    tls_connector()
        : m_context(tls_context::role::client),
          m_transport(),
          m_server_name()
    {}

    explicit tls_connector(
        const TransportConnector& _transport
    )
        : m_context(tls_context::role::client),
          m_transport(_transport),
          m_server_name()
    {}

    // context / transport
    //   function: access to the TLS context and the transport connector for
    // further configuration.
    D_NODISCARD tls_context&
    context()
    {
        return m_context;
    }

    D_NODISCARD TransportConnector&
    transport()
    {
        return m_transport;
    }

    // set_verify
    //   function: whether connections made from here verify the server.
    void
    set_verify(
        bool _on
    )
    {
        m_context.set_verify_peer(_on);

        return;
    }

    // set_server_name
    //   function: the name sent as SNI and checked against the certificate,
    // in place of the endpoint's host; empty restores the host.
    void
    set_server_name(
        const std::string& _name
    )
    {
        m_server_name = _name;

        return;
    }

    /**
     * @brief Connects the transport, runs TLS over it, and handshakes.
     *
     * A context that cannot be made live, or a server name it would check
     * that is neither a host name nor an address, fails before the
     * transport is connected; the handshake runs here, so a certificate or
     * verification failure is the connect's error.
     *
     * @param[in]  _endpoint  where to connect.
     * @param[out] _out       on success owns the TLS connection; otherwise
     *                        holds the error.
     * @return io_error::none, or the error _out records:
     *         io_error::access_denied for a certificate that does not
     *         verify, and the transport's error for a connection lost.
     */
    io_error::value
    connect(
        const endpoint& _endpoint,
        open_result&    _out
    )
    {
        const tls_context&    settings = m_context;
        const std::string&    name     = (m_server_name.empty())
                                             ? _endpoint.host
                                             : m_server_name;
        std::string           host;
        const io_error::value prepared =
            (internal::tls_host(settings.config(),
                                name,
                                &host)) ? m_context.prepare()
                                        : io_error::invalid_argument;

        // a context that cannot be made live, or a name it cannot check,
        // costs no round trip
        if (prepared != io_error::none)
        {
            _out.fail(prepared);

            return prepared;
        }

        const io_error::value opened = m_transport.connect(_endpoint,
                                                           _out);
        const io_error::value made   =
            (opened == io_error::none)
                ? tls_connection::adopt(_out.release(),
                                        m_context,
                                        name,
                                        _out)
                : opened;

        // a transport that failed, or a session that could not be made
        if (made != io_error::none)
        {
            return made;
        }

        const io_error::value shaken =
            static_cast<tls_connection*>(_out.get())->handshake();

        // a refused handshake closes the connection and reports why
        if (shaken != io_error::none)
        {
            _out.fail(shaken);
        }

        return shaken;
    }

#if (D_MOVE_ENABLED == 1)
    // connect -- the same, returning the result, which C++11 can move
    D_NODISCARD open_result
    connect(
        const endpoint& _endpoint
    )
    {
        open_result result;

        (void)connect(_endpoint,
                      result);

        return result;
    }
#endif  // D_MOVE_ENABLED

private:
    tls_context        m_context;
    TransportConnector m_transport;
    std::string        m_server_name;
};

// 3.2    Accepting
//------------------------------------------------------------------------------
// 3.2.1
// tls_acceptor
//   class: a net::acceptor that accepts a transport connection and runs TLS
// over it as the server of its context, which needs a certificate and key.
// The handshake waits for the accepted connection's first read or write, so
// it runs on whatever thread serves the connection rather than on the accept
// loop: a slow or hostile client cannot stall acceptance. Templated on the
// transport acceptor (TCP by default).
template<typename TransportAcceptor = tcp_acceptor>
class tls_acceptor
{
public:
    tls_acceptor()
        : m_context(tls_context::role::server),
          m_transport()
    {}

#if (D_MOVE_ENABLED == 1)
    // tls_acceptor(tls_context&&) -- takes a configured context, which
    // C++11 can move in; below C++11 configure the acceptor's own through
    // context()
    explicit tls_acceptor(
        tls_context&& _context
    )
        : m_context(static_cast<tls_context&&>(_context)),
          m_transport()
    {}
#endif  // D_MOVE_ENABLED

    // context / transport
    //   function: access to the TLS context (to load cert/key) and the
    // transport acceptor.
    D_NODISCARD tls_context&
    context()
    {
        return m_context;
    }

    D_NODISCARD TransportAcceptor&
    transport()
    {
        return m_transport;
    }

    // bind
    //   function: binds the transport acceptor.
    D_NODISCARD io_error::value
    bind(
        const endpoint& _endpoint
    )
    {
        return m_transport.bind(_endpoint);
    }

    /**
     * @brief Accepts a transport connection and runs TLS over it.
     *
     * A context that cannot be made live fails before a connection is
     * taken. The handshake waits for the connection's first read or write,
     * so it runs on whatever thread serves the connection.
     *
     * @param[out] _out  on success owns the TLS connection; otherwise holds
     *                   the error.
     * @return io_error::none, or the error _out records.
     */
    io_error::value
    accept(
        open_result& _out
    )
    {
        const io_error::value prepared = m_context.prepare();

        // a context that cannot be made live takes no connection
        if (prepared != io_error::none)
        {
            _out.fail(prepared);

            return prepared;
        }

        const io_error::value accepted = m_transport.accept(_out);

        return (accepted == io_error::none)
                   ? tls_connection::adopt(_out.release(),
                                           m_context,
                                           std::string(),
                                           _out)
                   : accepted;
    }

#if (D_MOVE_ENABLED == 1)
    // accept -- the same, returning the result, which C++11 can move
    D_NODISCARD open_result
    accept()
    {
        open_result result;

        (void)accept(result);

        return result;
    }
#endif  // D_MOVE_ENABLED

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
    tls_context       m_context;
    TransportAcceptor m_transport;
};


NS_END  // net
NS_END  // djinterp


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "tls.hpp: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64


#endif  // DJINTERP_NET_TLS_TLS_HPP
