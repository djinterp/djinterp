/*******************************************************************************
* djinterp [net]                                                        smtp.cpp
*
* djinterp SMTP -- C++ facade implementation.
*   Lowering and lifting between the facade's owning types and the kernel's
* borrowed ones, the TLS contexts the facade makes and keeps for as long as
* the sessions made from them, and the server's accept loop over a tcp.h
* listener and its session threads. Every function the C kernel calls back
* into is guarded, so no exception reaches C (and from C++11 each is
* noexcept besides).
*   C++98 and later: the guards call named function objects, the server's
* threads, lock and condition are dmutex.h's, and the state the sessions
* share is counted by hand where std::shared_ptr counted it. Empty where the
* header is: without a 64-bit integer type.
*
*
* path:      /src/djinterp/net/smtp/smtp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/net/smtp/smtp.hpp"  // corresponding header


// the header's 64-bit floor
#if (D_INTERNAL_NET_HAS_INT64 == 1)

// std
#include <algorithm>  // std::swap
#include <cstddef>    // std::size_t
#include <cstdio>     // std::sprintf
#include <cstring>    // std::memcpy
#include <new>        // std::nothrow
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../../inc/djinterp/c/sync/dmutex.h"          // threads, lock
#include "../../../../inc/djinterp/env/cpp/env_cpp98.h"      // exceptions
#include "../../../../inc/djinterp/net/net_common.hpp"       // connections
#include "../../../../inc/djinterp/net/smtp/smtp.h"          // the kernel
#include "../../../../inc/djinterp/net/ssl/ssl.h"            // TLS
#include "../../../../inc/djinterp/net/ssl/ssl_openssl.h"    // the engine
#include "../../../../inc/djinterp/net/tcp/tcp.h"            // TCP


NS_DJINTERP
NS_NET
D_NAMESPACE(smtp)

NS_INTERNAL

    // client_state
    //   struct: a kernel client; when attached, the connection it reads
    // through, which it owns, and the adapter that makes a C connection of
    // it; the host open() connected to; and the TLS context the facade made
    // for the kernel's TLS session, which must outlive it. Heap-allocated,
    // so the kernel's pointers into it stay valid however the owning client
    // moves.
    struct client_state
    {
        client_state() D_NOEXCEPT
            : kernel(),
              stream(D_NULLPTR),
              adapter(D_NULLPTR),
              host(),
              context(D_NULLPTR),
              lease()
        {
            ::d_smtp_client_init(&kernel);
        }

        ~client_state();

        ::d_smtp_client            kernel;
        net::connection*           stream;   // owned; null unless attached
        net::c_connection_adapter* adapter;  // owned; over `stream`
        std::string                host;     // open()'s; empty if attached
        ::d_ssl_context*           context;  // owned; null unless made
        tls_lease                  lease;    // held while TLS may use it

    private:
        D_DELETED_FN(client_state(const client_state&))
        D_DELETED_FN(client_state& operator=(const client_state&))
    };

    // server_state
    //   struct: everything the accept loop and its sessions share: the
    // TLS context the server's sessions make TLS with -- the caller's, or
    // `made`, which the state owns -- among it. The server holds one count
    // and each running session one; whoever lets the last go deletes it, so
    // a session finishing after the server is gone still has its lock and
    // condition to signal, and its context.
    struct server_state
    {
        server_state(
            const server_options& _options,
            const server_handler& _handler
        )
            : options(_options),
              handler(_handler),
              config(),
              listener(),
              made(),
              tls(D_NULLPTR),
              port(0u),
              stopping(false),
              active(0u),
              running(false),
              counts(1u)
        {
            ::d_tcp_listener_init(&listener);
            (void)::d_mutex_init(&mutex);
            (void)::d_cond_init(&changed);
        }

        ~server_state()
        {
            // the context goes once no session can use it
            if (tls == &made)
            {
                ::d_ssl_context_destroy(&made);
            }

            (void)::d_cond_destroy(&changed);
            (void)::d_mutex_destroy(&mutex);
        }

        server_options         options;
        server_handler         handler;
        ::d_smtp_server_config config;
        ::d_tcp_listener       listener;
        ::d_ssl_context        made;      // the context made from options
        const ::d_ssl_context* tls;       // the context in use, or null
        unsigned int           port;      // bound by listen()
        bool                   stopping;  // under mutex
        std::size_t            active;    // under mutex
        bool                   running;   // under mutex
        std::size_t            counts;    // under mutex
        mutable d_mutex_t      mutex;
        d_cond_t               changed;

    private:
        D_DELETED_FN(server_state(const server_state&))
        D_DELETED_FN(server_state& operator=(const server_state&))
    };

    // hold, release
    //   function: take one count on a server state, and give one back;
    // giving back the last deletes it.
    void
    hold(
        server_state* _state
    ) D_NOEXCEPT
    {
        (void)::d_mutex_lock(&_state->mutex);
        _state->counts += 1u;
        (void)::d_mutex_unlock(&_state->mutex);

        return;
    }

    void
    release(
        server_state* _state
    ) D_NOEXCEPT
    {
        (void)::d_mutex_lock(&_state->mutex);
        _state->counts -= 1u;

        const bool last = (_state->counts == 0u);

        (void)::d_mutex_unlock(&_state->mutex);

        // the last count deletes; no one else can reach the state now
        if (last)
        {
            delete _state;
        }

        return;
    }

#if D_ENV_CPP98_HAS_EXCEPTION

    // guarded
    //   function: calls `_callable` on behalf of the C kernel; an exception
    // must not cross into C, so one becomes `_fallback`.
    template<typename Result,
             typename Callable>
    Result
    guarded(
        Result          _fallback,
        const Callable& _callable
    ) D_NOEXCEPT
    {
        try
        {
            return _callable();
        }
        catch (...)
        {
            return _fallback;
        }
    }

#else

    // guarded
    //   function: without exceptions there is nothing to catch.
    template<typename Result,
             typename Callable>
    Result
    guarded(
        Result          _fallback,
        const Callable& _callable
    ) D_NOEXCEPT
    {
        (void)_fallback;

        return _callable();
    }

#endif  // D_ENV_CPP98_HAS_EXCEPTION

    // optional
    //   function: a string's text, or null when it is empty.
    const char*
    optional(
        const std::string& _text
    ) D_NOEXCEPT
    {
        return (_text.empty()) ? D_NULLPTR
                               : _text.c_str();
    }

    // decimal
    //   function: an unsigned number in decimal; std::to_string is C++11's.
    std::string
    decimal(
        unsigned int _number
    )
    {
        char      digits[24];
        const int length = std::sprintf(digits,
                                        "%u",
                                        _number);

        return std::string(digits,
                           static_cast<std::size_t>(length));
    }

    // fail
    //   function: a kernel error, and the reply that explains it if any, as
    // the unexpected half of a result.
    ::re_std::unexpected<failure>
    fail(
        ::d_smtp_error        _code,
        const ::d_smtp_reply* _reply
    )
    {
        failure lifted;

        lifted.code = static_cast<smtp::error::value>(_code);

        // the reply that explains it, when there was one
        if (_reply)
        {
            lifted.server_reply = smtp::reply(*_reply);
        }

        return ::re_std::unexpected<failure>(lifted);
    }

    // finish
    //   function: a command's outcome: the reply, or the failure.
    reply_result
    finish(
        ::d_smtp_error        _code,
        const ::d_smtp_reply& _reply
    )
    {
        // a failed command carries its reply in the failure
        if (_code != D_SMTP_OK)
        {
            return fail(_code,
                        &_reply);
        }

        return smtp::reply(_reply);
    }

    // stream_close
    //   struct: an attached connection's close, as a function object the
    // guard can make.
    struct stream_close
    {
        bool
        operator()() const
        {
            stream->close();

            return true;
        }

        net::connection* stream;
    };

    // close_stream
    //   function: closes the connection a client was attached to, if any;
    // the kernel leaves an attached connection to its owner, which the
    // facade is.
    void
    close_stream(
        client_state* _state
    ) D_NOEXCEPT
    {
        // only an attached client has a stream of its own
        if (_state->stream)
        {
            const stream_close call = { _state->stream };

            (void)guarded(false,
                          call);
        }

        return;
    }

    client_state::~client_state()
    {
        ::d_smtp_client_close(&kernel);
        close_stream(this);
        delete adapter;
        delete stream;

        // the context goes after the TLS session made from it
        if (context)
        {
            ::d_ssl_context_destroy(context);
            delete context;
        }
    }

    // make_context
    //   function: a context of the SSL kernel for `_role`, made in `_out`
    // from `_options` with the OpenSSL-family engine: a client checks the
    // server, unless told not to, against the CA file or else the system's
    // trust store; a server presents its certificate and key.
    ::d_smtp_error
    make_context(
        ::d_ssl_context*   _out,
        const tls_options& _options,
        ::d_ssl_role       _role
    ) D_NOEXCEPT
    {
        const ::d_ssl_engine* const engine = ::d_ssl_engine_openssl();
        ::d_ssl_config              config = ::d_ssl_config();

        // a build without the engine has no TLS to make
        if (!engine)
        {
            return D_SMTP_ERROR_UNSUPPORTED;
        }

        ::d_ssl_config_init(&config,
                            _role);

        if (_role == D_SSL_ROLE_CLIENT)
        {
            config.verify           = (_options.verify_peer)
                                          ? D_SSL_VERIFY_REQUIRED
                                          : D_SSL_VERIFY_NONE;
            config.verify_hostname  = _options.verify_peer;
            config.use_system_trust = _options.ca_file.empty();
            config.ca_file          = optional(_options.ca_file);
        }
        else
        {
            config.certificate_file = optional(_options.certificate_file);
            config.private_key_file = optional(_options.private_key_file);
        }

        return ::d_smtp_error_from_ssl(::d_ssl_context_init(_out,
                                                            engine,
                                                            &config),
                                       D_NET_OK);
    }

    // client_context
    //   function: the context a client's TLS is made with: the caller's own,
    // or one made now from `_options`, on the heap so it never moves, and
    // handed back in *_made for keep_context to decide its fate. Null, with
    // the reason in *_error, when none can be had.
    const ::d_ssl_context*
    client_context(
        const tls_options& _options,
        ::d_ssl_context**  _made,
        ::d_smtp_error*    _error
    ) D_NOEXCEPT
    {
        *_made  = D_NULLPTR;
        *_error = D_SMTP_OK;

        // a context of the caller's own is used as it is
        if (_options.context)
        {
            return _options.context;
        }

        // so is a tls_context's, lent; the caller holds the lease
        if (_options.lease.get())
        {
            return _options.lease.get();
        }

        ::d_ssl_context* const made = new (std::nothrow) ::d_ssl_context();

        *_error = (made) ? make_context(made,
                                        _options,
                                        D_SSL_ROLE_CLIENT)
                         : D_SMTP_ERROR_OUT_OF_MEMORY;

        // a context not made holds nothing to release
        if (*_error != D_SMTP_OK)
        {
            delete made;

            return D_NULLPTR;
        }

        *_made = made;

        return made;
    }

    // kernel_options
    //   function: the kernel's options for `_options`, all but the context,
    // which client_context finds. The strings stay `_options`'s.
    void
    kernel_options(
        ::d_smtp_client_options* _out,
        const client_options&    _options
    ) D_NOEXCEPT
    {
        ::d_smtp_client_options_init(_out);
        _out->host                = _options.host.c_str();
        _out->port                = _options.port;
        _out->security            = static_cast< ::d_smtp_security>(
                                        _options.mode);
        _out->server_name         = optional(_options.tls.server_name);
        _out->helo_domain         = optional(_options.helo_domain);
        _out->connect_timeout_ms  = _options.connect_timeout_ms;
        _out->io_timeout_ms       = _options.io_timeout_ms;
        _out->allow_insecure_auth = _options.allow_insecure_auth;

        return;
    }

    // drop_context
    //   function: releases a context the facade made; null is ignored.
    void
    drop_context(
        ::d_ssl_context* _made
    ) D_NOEXCEPT
    {
        // only a context made has anything to release
        if (_made)
        {
            ::d_ssl_context_destroy(_made);
            delete _made;
        }

        return;
    }

    // keep_context
    //   function: what a kernel call that may make TLS leaves of the
    // context the facade made for it: kept, for as long as the state, when
    // the call made the kernel's TLS session from it -- the client had none
    // before, and has one now -- and released otherwise. An earlier context
    // goes then, as no session can use it any more.
    void
    keep_context(
        client_state*    _state,
        ::d_ssl_context* _made,
        bool             _had_tls
    ) D_NOEXCEPT
    {
        // the call made the session from it: it stays, the old one goes
        if ( (_made)     &&
             (!_had_tls) &&
             (_state->kernel.has_tls) )
        {
            std::swap(_state->context,
                      _made);
        }

        drop_context(_made);

        return;
    }

    // lift
    //   function: a kernel session view as an owning session.
    session
    lift(
        const ::d_smtp_session_info& _info
    )
    {
        session lifted;

        lifted.client_domain = (_info.client_domain) ? _info.client_domain : "";
        lifted.user          = (_info.user) ? _info.user : "";
        lifted.sender        = (_info.sender) ? _info.sender : "";
        lifted.secure        = _info.secure;
        lifted.eight_bit     = _info.eight_bit;
        lifted.utf8          = _info.utf8;

        // each recipient, owned
        for (std::size_t i = 0; i < _info.recipient_count; ++i)
        {
            lifted.recipients.push_back(std::string(_info.recipients[i]));
        }

        return lifted;
    }

    // ask_mail, ask_rcpt, ask_message, ask_auth
    //   struct: each handler's call, as a function object the guard can
    // make.
    struct ask_mail
    {
        unsigned int
        operator()() const
        {
            return state->handler.on_mail(lift(*info),
                                          std::string(sender));
        }

        const server_state*          state;
        const ::d_smtp_session_info* info;
        const char*                  sender;
    };

    struct ask_rcpt
    {
        unsigned int
        operator()() const
        {
            return state->handler.on_rcpt(lift(*info),
                                          std::string(recipient));
        }

        const server_state*          state;
        const ::d_smtp_session_info* info;
        const char*                  recipient;
    };

    struct ask_message
    {
        unsigned int
        operator()() const
        {
            return state->handler.on_message(lift(*info),
                                             re_std::string_view(data,
                                                                 length));
        }

        const server_state*          state;
        const ::d_smtp_session_info* info;
        const char*                  data;
        std::size_t                  length;
    };

    struct ask_auth
    {
        bool
        operator()() const
        {
            return state->handler.on_auth(std::string(user),
                                          std::string(secret));
        }

        const server_state* state;
        const char*         user;
        const char*         secret;
    };

    // on_mail, on_rcpt, on_message, on_auth
    //   function: the kernel's callbacks, forwarded to the handlers; a
    // throwing handler answers 451 (or refuses AUTH).
    unsigned int
    on_mail(
        void*                        _context,
        const ::d_smtp_session_info* _info,
        const char*                  _sender
    ) D_NOEXCEPT
    {
        const ask_mail call = { static_cast<const server_state*>(_context),
                                _info,
                                _sender };

        return guarded(451u,
                       call);
    }

    unsigned int
    on_rcpt(
        void*                        _context,
        const ::d_smtp_session_info* _info,
        const char*                  _recipient
    ) D_NOEXCEPT
    {
        const ask_rcpt call = { static_cast<const server_state*>(_context),
                                _info,
                                _recipient };

        return guarded(451u,
                       call);
    }

    unsigned int
    on_message(
        void*                        _context,
        const ::d_smtp_session_info* _info,
        const char*                  _data,
        std::size_t                  _length
    ) D_NOEXCEPT
    {
        const ask_message call = { static_cast<const server_state*>(
                                       _context),
                                   _info,
                                   _data,
                                   _length };

        return guarded(451u,
                       call);
    }

    bool
    on_auth(
        void*       _context,
        const char* _user,
        const char* _secret
    ) D_NOEXCEPT
    {
        const ask_auth call = { static_cast<const server_state*>(_context),
                                _user,
                                _secret };

        return guarded(false,
                       call);
    }

    // configure
    //   function: the kernel configuration, pointing into the options the
    // state owns; its context is set by listen(), which makes it.
    void
    configure(
        server_state& _state
    ) D_NOEXCEPT
    {
        ::d_smtp_server_config_init(&_state.config);
        _state.config.domain              = _state.options.domain.c_str();
        _state.config.max_message_size    = _state.options.max_message_size;
        _state.config.max_recipients      = _state.options.max_recipients;
        _state.config.auth_mechanisms     = _state.options.auth_mechanisms;
        _state.config.require_tls         = _state.options.require_tls;
        _state.config.require_auth        = _state.options.require_auth;
        _state.config.allow_insecure_auth = _state.options.allow_insecure_auth;

        return;
    }

    // server_tls
    //   function: the context the server's sessions use, settled once: the
    // caller's own, or one made from the certificate and key, or none.
    // STARTTLS offers it; implicit TLS cannot do without it.
    ::d_smtp_error
    server_tls(
        server_state& _state
    ) D_NOEXCEPT
    {
        const tls_options& tls  = _state.options.tls;
        ::d_smtp_error     code = D_SMTP_OK;

        // a context of the caller's, or one made from files given
        if (tls.context)
        {
            _state.tls = tls.context;
        }
        else if (tls.lease.get())
        {
            // the state's copy of the options holds the lease
            _state.tls = tls.lease.get();
        }
        else if ( (!tls.certificate_file.empty()) ||
                  (!tls.private_key_file.empty()) )
        {
            code       = make_context(&_state.made,
                                      tls,
                                      D_SSL_ROLE_SERVER);
            _state.tls = (code == D_SMTP_OK) ? &_state.made : D_NULLPTR;
        }

        if (code != D_SMTP_OK)
        {
            return code;
        }

        // implicit TLS needs a context; STARTTLS is offered with one
        if ( (_state.options.implicit_tls) &&
             (!_state.tls) )
        {
            return D_SMTP_ERROR_INVALID_ARGUMENT;
        }

        _state.config.tls = (_state.options.implicit_tls) ? D_NULLPTR
                                                          : _state.tls;

        return D_SMTP_OK;
    }

    // listen_error
    //   function: why a listener would not open, in the kernel's terms: a
    // host that does not resolve here is RESOLVE; anything else that is not
    // the caller's or the memory's fault is TRANSPORT.
    ::d_smtp_error
    listen_error(
        ::d_net_error _error
    ) D_NOEXCEPT
    {
        switch (_error)
        {
            case D_NET_ERROR_INVALID_ARGUMENT:
                return D_SMTP_ERROR_INVALID_ARGUMENT;

            case D_NET_ERROR_OUT_OF_MEMORY:
                return D_SMTP_ERROR_OUT_OF_MEMORY;

            case D_NET_ERROR_ADDRESS_INVALID:
            case D_NET_ERROR_HOST_UNREACHABLE:
                return D_SMTP_ERROR_RESOLVE;

            default:
                break;
        }

        return D_SMTP_ERROR_TRANSPORT;
    }

    // serve_implicit
    //   function: implicit TLS: the handshake comes before the greeting
    // (RFC 8314), and the session runs over the TLS session's view, which
    // says close_notify as the session ends, where it still can.
    void
    serve_implicit(
        server_state*                  _state,
        ::d_tcp_connection*            _connection,
        const ::d_smtp_server_config*  _config,
        const ::d_smtp_server_handler* _handler
    ) D_NOEXCEPT
    {
        const ::d_pack_text none = { "",
                                     0u };
        ::d_ssl_session     tls  = ::d_ssl_session();

        // a session not made holds nothing to release
        if (::d_ssl_session_init(&tls,
                                 _state->tls,
                                 &_connection->base,
                                 none) != D_SSL_OK)
        {
            return;
        }

        // a session only once TLS is up
        if (::d_ssl_session_handshake(&tls) == D_SSL_OK)
        {
            (void)::d_smtp_server_session(&tls.base,
                                          _config,
                                          _handler);
        }

        // an orderly end, where TLS can still say it: a failed session
        // cannot, and one the client closed owes nothing more
        (void)::d_ssl_session_shutdown(&tls);
        ::d_ssl_session_destroy(&tls);

        return;
    }

    // serve
    //   function: one session over an accepted connection, which it closes;
    // then the session's count of active ones goes.
    void
    serve(
        server_state*       _state,
        ::d_tcp_connection* _connection
    ) D_NOEXCEPT
    {
        ::d_smtp_server_config  config  = _state->config;
        ::d_smtp_server_handler handler = ::d_smtp_server_handler();

        config.secure   = _state->options.implicit_tls;
        handler.context = _state;
        handler.mail    = (!_state->handler.on_mail.empty()) ? on_mail
                                                             : D_NULLPTR;
        handler.rcpt    = (!_state->handler.on_rcpt.empty()) ? on_rcpt
                                                             : D_NULLPTR;
        handler.message = (!_state->handler.on_message.empty()) ? on_message
                                                                 : D_NULLPTR;
        handler.auth    = (!_state->handler.on_auth.empty()) ? on_auth
                                                             : D_NULLPTR;

        // implicit TLS first, or the session over the connection itself
        if (config.secure)
        {
            serve_implicit(_state,
                           _connection,
                           &config,
                           &handler);
        }
        else
        {
            (void)::d_smtp_server_session(&_connection->base,
                                          &config,
                                          &handler);
        }

        ::d_tcp_connection_close(_connection);

        (void)::d_mutex_lock(&_state->mutex);
        _state->active -= 1u;
        (void)::d_cond_broadcast(&_state->changed);
        (void)::d_mutex_unlock(&_state->mutex);

        return;
    }

    // session_work
    //   struct: what a session thread is started with: its count on the
    // state, and the accepted connection, which it owns.
    struct session_work
    {
        server_state*      state;
        ::d_tcp_connection connection;
    };

    // session_main
    //   function: a session thread's body: the session, then its count back.
    d_thread_result_t
    session_main(
        void* _work
    )
    {
        session_work* const work  = static_cast<session_work*>(_work);
        server_state* const state = work->state;

        serve(state,
              &work->connection);
        delete work;
        release(state);

        return D_THREAD_SUCCESS;
    }

    // stopping
    //   function: whether stop() has been called, read under the lock.
    bool
    stopping(
        server_state* _state
    ) D_NOEXCEPT
    {
        (void)::d_mutex_lock(&_state->mutex);

        const bool result = _state->stopping;

        (void)::d_mutex_unlock(&_state->mutex);

        return result;
    }

    // start_session
    //   function: hands an accepted connection to a detached thread of its
    // own, which holds a count on the state until its session ends. Without
    // the memory or the thread for it, the connection is dropped.
    void
    start_session(
        server_state*      _state,
        ::d_tcp_connection _accepted
    ) D_NOEXCEPT
    {
        session_work* const work = new (std::nothrow) session_work();

        // no memory for the session: the connection is dropped
        if (!work)
        {
            ::d_tcp_connection_close(&_accepted);

            return;
        }

        work->state      = _state;
        work->connection = _accepted;
        hold(_state);
        (void)::d_mutex_lock(&_state->mutex);
        _state->active += 1u;
        (void)::d_mutex_unlock(&_state->mutex);

        d_thread_t thread;

        // the session's thread; one that cannot start serves nobody
        if (::d_thread_create(&thread,
                              &session_main,
                              work) == 0)
        {
            (void)::d_thread_detach(thread);
        }
        else
        {
            ::d_tcp_connection_close(&work->connection);
            delete work;
            (void)::d_mutex_lock(&_state->mutex);
            _state->active -= 1u;
            (void)::d_mutex_unlock(&_state->mutex);
            release(_state);
        }

        return;
    }

    // accept_goes_on
    //   function: whether an accept that failed leaves the loop going: one
    // that waited its slice out, or was interrupted, or lost a client that
    // gave up before it was accepted. Anything else is the listener's end.
    bool
    accept_goes_on(
        ::d_net_error _error
    ) D_NOEXCEPT
    {
        return ( (_error == D_NET_ERROR_TIMED_OUT)          ||
                 (_error == D_NET_ERROR_INTERRUPTED)        ||
                 (_error == D_NET_ERROR_CONNECTION_ABORTED) ||
                 (_error == D_NET_ERROR_CONNECTION_RESET) );
    }

    // finish_run
    //   function: the end of a run: every session finishes before the loop
    // does, and then the server is no longer running.
    void
    finish_run(
        server_state* _state
    ) D_NOEXCEPT
    {
        (void)::d_mutex_lock(&_state->mutex);

        // every session finishes before the loop does
        while (_state->active != 0u)
        {
            (void)::d_cond_wait(&_state->changed,
                                &_state->mutex);
        }

        _state->running = false;
        (void)::d_cond_broadcast(&_state->changed);
        (void)::d_mutex_unlock(&_state->mutex);

        return;
    }

NS_END  // internal


const char*
to_string(
    error::value _error
) D_NOEXCEPT
{
    return ::d_smtp_error_string(static_cast< ::d_smtp_error>(_error));
}

io_error::value
to_net(
    error::value _error
) D_NOEXCEPT
{
    return net::from_c(
        ::d_smtp_error_to_net(static_cast< ::d_smtp_error>(_error)));
}

status::status() D_NOEXCEPT
    : ::d_smtp_status()
{}

status::status(
    const ::d_smtp_status& _status
) D_NOEXCEPT
    : ::d_smtp_status(_status)
{}

bool
status::present() const D_NOEXCEPT
{
    return (status_class != 0u);
}

std::string
status::to_string() const
{
    // no status, no text
    if (!present())
    {
        return std::string();
    }

    return internal::decimal(status_class) + "." +
           internal::decimal(subject) + "." +
           internal::decimal(detail);
}

capabilities::capabilities() D_NOEXCEPT
    : ::d_smtp_capabilities()
{}

capabilities::capabilities(
    const ::d_smtp_capabilities& _capabilities
) D_NOEXCEPT
    : ::d_smtp_capabilities(_capabilities)
{}

bool
capabilities::supports(
    extension::value _extension
) const D_NOEXCEPT
{
    return ((extensions & static_cast<unsigned int>(_extension)) != 0u);
}

bool
capabilities::offers(
    mechanism::value _mechanism
) const D_NOEXCEPT
{
    return ((auth & static_cast<unsigned int>(_mechanism)) != 0u);
}

reply::reply()
    : code(0u),
      status(),
      lines(),
      truncated(false)
{}

/*
reply::reply
  Walks the kernel's stored text line by line; a line lost to truncation
ends the walk, and `truncated` says so.
*/
reply::reply(
    const ::d_smtp_reply& _reply
)
    : code(_reply.code),
      status(_reply.status),
      lines(),
      truncated(_reply.truncated)
{
    // each stored line, until one is missing
    for (std::size_t i = 0; i < _reply.line_count; ++i)
    {
        std::size_t length = 0;
        const char* line   = ::d_smtp_reply_line(&_reply,
                                                 i,
                                                 &length);

        // a line lost to truncation ends the walk
        if (!line)
        {
            break;
        }

        lines.push_back(std::string(line,
                                    length));
    }
}

std::string
reply::text() const
{
    std::string joined;

    // the lines, newline-separated
    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        joined += (i > 0) ? "\n" : "";
        joined += lines[i];
    }

    return joined;
}

bool
reply::positive() const D_NOEXCEPT
{
    return ::d_smtp_code_is_positive(code);
}

bool
reply::transient() const D_NOEXCEPT
{
    return ::d_smtp_code_is_transient(code);
}

bool
reply::permanent() const D_NOEXCEPT
{
    return ::d_smtp_code_is_permanent(code);
}

client::client() D_NOEXCEPT
    : m_state(D_NULLPTR)
{}

#if (D_MOVE_ENABLED == 1)
client::client(
    client&& _other
) D_NOEXCEPT
    : m_state(_other.m_state)
{
    _other.m_state = D_NULLPTR;
}

client&
client::operator=(
    client&& _other
) D_NOEXCEPT
{
    // assigning a client to itself keeps its session
    if (this != &_other)
    {
        delete m_state;
        m_state        = _other.m_state;
        _other.m_state = D_NULLPTR;
    }

    return *this;
}
#endif  // D_MOVE_ENABLED

client::~client()
{
    delete m_state;
}

/*
client::open
  The state is made first and kept only when the opening succeeds, so a
failed open leaves the client closed. A mode that can use TLS gets a
context; only STARTTLS_OPTIONAL goes on without one when the build has no
engine to make it with, as it would go on without the upgrade.
*/
void_result
client::open(
    const client_options& _options
)
{
    ::d_smtp_client_options options = ::d_smtp_client_options();
    ::d_ssl_context*        made    = D_NULLPTR;
    ::d_smtp_error          code    = D_SMTP_OK;

    internal::kernel_options(&options,
                             _options);
    close();
    delete m_state;
    m_state = D_NULLPTR;

    // a context for a mode that can use one
    if (_options.mode != smtp::security::none)
    {
        options.tls = internal::client_context(_options.tls,
                                               &made,
                                               &code);
    }

    // without one, only the optional upgrade goes on, in plaintext
    if ( (code != D_SMTP_OK) &&
         ( (code != D_SMTP_ERROR_UNSUPPORTED) ||
           (_options.mode != smtp::security::starttls_optional) ) )
    {
        return internal::fail(code,
                              D_NULLPTR);
    }

    internal::client_state* const state =
        new (std::nothrow) internal::client_state();

    // no state to open a session in
    if (!state)
    {
        internal::drop_context(made);

        return internal::fail(D_SMTP_ERROR_OUT_OF_MEMORY,
                              D_NULLPTR);
    }

    // a lent context is held for as long as the kernel may use it
    if ( (!_options.tls.context) &&
         (_options.tls.lease.get()) )
    {
        state->lease = _options.tls.lease;
    }

    state->host = _options.host;
    code        = ::d_smtp_client_connect(&state->kernel,
                                          &options);
    internal::keep_context(state,
                           made,
                           false);

    // a failed opening keeps nothing but its reply
    if (code != D_SMTP_OK)
    {
        const ::re_std::unexpected<failure> failed =
            internal::fail(code,
                           &state->kernel.reply);

        delete state;

        return failed;
    }

    m_state = state;

    return void_result();
}

/*
client::open
  The adapter makes a C connection of the caller's, and lives in the
heap-held state with it, so both survive every move of the client.
*/
void_result
client::open(
    net::connection*      _connection,
    const attach_options& _options
)
{
    close();
    delete m_state;
    m_state = D_NULLPTR;

    // no stream to run a session over
    if (!_connection)
    {
        return internal::fail(D_SMTP_ERROR_INVALID_ARGUMENT,
                              D_NULLPTR);
    }

    internal::client_state* const state =
        new (std::nothrow) internal::client_state();

    // no state for the session: the stream, owned here, goes
    if (!state)
    {
        delete _connection;

        return internal::fail(D_SMTP_ERROR_OUT_OF_MEMORY,
                              D_NULLPTR);
    }

    state->stream  = _connection;
    state->adapter = new (std::nothrow) net::c_connection_adapter(
                                            *_connection);

    ::d_smtp_client_attach_options kernel_options;

    ::d_smtp_client_attach_options_init(&kernel_options);
    kernel_options.secure              = _options.secure;
    kernel_options.allow_insecure_auth = _options.allow_insecure_auth;

    ::d_smtp_error code = (state->adapter)
                              ? ::d_smtp_client_attach_with(
                                    &state->kernel,
                                    state->adapter->get(),
                                    &kernel_options)
                              : D_SMTP_ERROR_OUT_OF_MEMORY;

    // the greeting, once attached
    if (code == D_SMTP_OK)
    {
        code = ::d_smtp_client_greeting(&state->kernel);
    }

    // then EHLO
    if (code == D_SMTP_OK)
    {
        code = ::d_smtp_client_hello(&state->kernel,
                                     _options.helo_domain.c_str());
    }

    // a failed opening keeps nothing but its reply
    if (code != D_SMTP_OK)
    {
        const ::re_std::unexpected<failure> failed =
            internal::fail(code,
                           &state->kernel.reply);

        delete state;

        return failed;
    }

    m_state = state;

    return void_result();
}

/*
client::open
  The three-argument form: open() with attach_options of `_secure` and
`_helo_domain`, AUTH without TLS refused.
*/
void_result
client::open(
    net::connection*   _connection,
    bool               _secure,
    const std::string& _helo_domain
)
{
    attach_options options;

    options.secure      = _secure;
    options.helo_domain = _helo_domain;

    return open(_connection,
                options);
}

#if (D_MOVE_ENABLED == 1)
result<client>
client::connect(
    const client_options& _options
)
{
    client            opened;
    const void_result outcome = opened.open(_options);

    // the failure, or the open client
    if (!outcome.has_value())
    {
        return ::re_std::unexpected<failure>(outcome.error());
    }

    return result<client>(static_cast<client&&>(opened));
}

result<client>
client::attach(
    std::unique_ptr<net::connection> _connection,
    const attach_options&            _options
)
{
    client            opened;
    const void_result outcome = opened.open(_connection.release(),
                                            _options);

    // the failure, or the open client
    if (!outcome.has_value())
    {
        return ::re_std::unexpected<failure>(outcome.error());
    }

    return result<client>(static_cast<client&&>(opened));
}

result<client>
client::attach(
    std::unique_ptr<net::connection> _connection,
    bool                             _secure,
    const std::string&               _helo_domain
)
{
    attach_options options;

    options.secure      = _secure;
    options.helo_domain = _helo_domain;

    return attach(static_cast<std::unique_ptr<net::connection>&&>(
                      _connection),
                  options);
}
#endif  // D_MOVE_ENABLED

bool
client::is_open() const D_NOEXCEPT
{
    return ( (m_state) &&
             (m_state->kernel.state != D_SMTP_CLIENT_CLOSED) );
}

bool
client::is_secure() const D_NOEXCEPT
{
    return ( (m_state) &&
             (m_state->kernel.secure) );
}

bool
client::is_authenticated() const D_NOEXCEPT
{
    return ( (m_state) &&
             (m_state->kernel.authenticated) );
}

smtp::capabilities
client::features() const D_NOEXCEPT
{
    return (m_state) ? smtp::capabilities(m_state->kernel.capabilities)
                     : smtp::capabilities();
}

smtp::reply
client::last_reply() const
{
    return (m_state) ? smtp::reply(m_state->kernel.reply)
                     : smtp::reply();
}

/*
client::starttls
  The context is made, if the caller gave none, before the kernel is asked;
it is kept only when the kernel made the TLS session from it. The name
checked is the one given, or else the host open() connected to.
*/
reply_result
client::starttls(
    const tls_options& _options
)
{
    ::d_ssl_context* made = D_NULLPTR;
    ::d_smtp_error   code = D_SMTP_OK;

    // a closed client has no session to secure
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              D_NULLPTR);
    }

    const ::d_ssl_context* const tls = internal::client_context(_options,
                                                                &made,
                                                                &code);

    // a lent context is held for as long as the kernel may use it
    if ( (!_options.context) &&
         (_options.lease.get()) )
    {
        m_state->lease = _options.lease;
    }

    // without a context there is no TLS to make
    if (!tls)
    {
        return internal::fail(code,
                              D_NULLPTR);
    }

    const bool        had  = m_state->kernel.has_tls;
    const std::string name = (_options.server_name.empty())
                                 ? m_state->host
                                 : _options.server_name;

    code = ::d_smtp_client_starttls(&m_state->kernel,
                                    tls,
                                    internal::optional(name));
    internal::keep_context(m_state,
                           made,
                           had);

    return internal::finish(code,
                            m_state->kernel.reply);
}

reply_result
client::authenticate(
    const credentials& _credentials
)
{
    // a closed client has no session to authenticate
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              D_NULLPTR);
    }

    const ::d_smtp_credentials lowered =
    {
        _credentials.user.c_str(),
        _credentials.secret.c_str(),
        static_cast<unsigned int>(_credentials.method)
    };

    return internal::finish(::d_smtp_client_authenticate(&m_state->kernel,
                                                         &lowered),
                            m_state->kernel.reply);
}

/*
client::send
  The recipients are lowered to a pointer array for the kernel, and its
per-recipient report is lifted back alongside the addresses.
*/
report_result
client::send(
    const envelope&     _envelope,
    re_std::string_view _message
)
{
    // a closed client has no session to send in
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              D_NULLPTR);
    }

    const std::size_t                       count = _envelope.recipients.size();
    std::vector<const char*>                recipients;
    std::vector< ::d_smtp_recipient_status> statuses(count);

    recipients.reserve(count);

    // each address, borrowed for the kernel
    for (std::size_t i = 0; i < count; ++i)
    {
        recipients.push_back(_envelope.recipients[i].c_str());
    }

    const ::d_smtp_envelope lowered =
    {
        _envelope.sender.c_str(),
        (count) ? &recipients[0] : D_NULLPTR,
        count,
        _envelope.eight_bit,
        _envelope.utf8
    };
    ::d_smtp_send_report    report  = { (count) ? &statuses[0] : D_NULLPTR,
                                        0u,
                                        0u };
    const ::d_smtp_error    code    = ::d_smtp_client_send(&m_state->kernel,
                                                           &lowered,
                                                           _message.data(),
                                                           _message.size(),
                                                           &report);

    // a failed transaction carries the reply that ended it
    if (code != D_SMTP_OK)
    {
        return internal::fail(code,
                              &m_state->kernel.reply);
    }

    send_report lifted;

    lifted.accepted  = report.accepted;
    lifted.data_code = report.data_code;

    // each recipient's answer, beside its address
    for (std::size_t i = 0; i < count; ++i)
    {
        recipient_result entry;

        entry.address = _envelope.recipients[i];
        entry.code    = statuses[i].code;
        entry.status  = smtp::status(statuses[i].status);
        lifted.recipients.push_back(entry);
    }

    return lifted;
}

reply_result
client::reset()
{
    // a closed client has no transaction to reset
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              D_NULLPTR);
    }

    return internal::finish(::d_smtp_client_reset(&m_state->kernel),
                            m_state->kernel.reply);
}

reply_result
client::noop()
{
    // a closed client has no one to ask
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              D_NULLPTR);
    }

    return internal::finish(::d_smtp_client_noop(&m_state->kernel),
                            m_state->kernel.reply);
}

reply_result
client::quit()
{
    // a closed client has no session to end
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              D_NULLPTR);
    }

    return internal::finish(::d_smtp_client_quit(&m_state->kernel),
                            m_state->kernel.reply);
}

/*
client::close
  The kernel leaves an attached connection to its owner, which the client
is: it closes it here, as it closes what connect() opened.
*/
void
client::close() D_NOEXCEPT
{
    // only an opened client has anything to release
    if (m_state)
    {
        ::d_smtp_client_close(&m_state->kernel);
        internal::close_stream(m_state);
    }

    return;
}

report_result
send(
    const client_options& _options,
    const credentials&    _credentials,
    const envelope&       _envelope,
    re_std::string_view   _message
)
{
    client            session;
    const void_result opened = session.open(_options);

    // no session, no delivery
    if (!opened.has_value())
    {
        return ::re_std::unexpected<failure>(opened.error());
    }

    // a user means authentication first
    if (!_credentials.user.empty())
    {
        const reply_result authenticated = session.authenticate(
                                               _credentials);

        // refused credentials end the delivery
        if (!authenticated.has_value())
        {
            return ::re_std::unexpected<failure>(authenticated.error());
        }
    }

    const report_result sent = session.send(_envelope,
                                            _message);

    (void)session.quit();

    return sent;
}

server::server(
    const server_options& _options,
    const server_handler& _handler
)
    : m_state(new internal::server_state(_options,
                                         _handler))
{
    internal::configure(*m_state);
}

/*
server::~server
  A running accept loop is stopped and allowed to finish -- it waits for
its sessions -- before the listener is closed beneath it; the server's
count goes last, so a session still unwinding keeps the state alive.
*/
server::~server()
{
    (void)::d_mutex_lock(&m_state->mutex);
    m_state->stopping = true;

    // the accept loop finishes first
    while (m_state->running)
    {
        (void)::d_cond_wait(&m_state->changed,
                            &m_state->mutex);
    }

    (void)::d_mutex_unlock(&m_state->mutex);
    ::d_tcp_listener_close(&m_state->listener);
    internal::release(m_state);
}

/*
server::listen
  The TLS context is settled before anything is bound, so a server that
cannot have the TLS its options ask for never listens.
*/
port_result
server::listen(
    const std::string& _host,
    unsigned int       _port
)
{
    ::d_net_endpoint endpoint = ::d_net_endpoint();
    ::d_tcp_options  options  = ::d_tcp_options();
    ::d_smtp_error   code     = D_SMTP_OK;

    // a port that cannot be, a listener already open, or no sockets
    if ( (_port > 65535u) ||
         (_host.size() > D_NET_HOST_MAX) )
    {
        code = D_SMTP_ERROR_INVALID_ARGUMENT;
    }
    else if (::d_tcp_listener_is_open(&m_state->listener))
    {
        code = D_SMTP_ERROR_STATE;
    }
    else if (!::d_tcp_available())
    {
        code = D_SMTP_ERROR_UNSUPPORTED;
    }
    else if (!m_state->tls)
    {
        code = internal::server_tls(*m_state);
    }

    if (code != D_SMTP_OK)
    {
        return internal::fail(code,
                              D_NULLPTR);
    }

    ::d_net_endpoint_init(&endpoint);
    ::d_tcp_options_init(&options);
    std::memcpy(endpoint.host,
                _host.c_str(),
                _host.size() + 1u);
    endpoint.port         = static_cast< ::d_net_port>(_port);
    endpoint.protocol     = D_NET_PROTOCOL_TCP;
    options.io_timeout_ms = m_state->options.io_timeout_ms;

    const ::d_net_error opened = ::d_tcp_listen(&m_state->listener,
                                                &endpoint,
                                                &options);

    // a listener that will not open, or that cannot say where it is
    if ( (opened != D_NET_OK) ||
         (!::d_tcp_listener_local_endpoint(&m_state->listener,
                                           &endpoint)) )
    {
        ::d_tcp_listener_close(&m_state->listener);

        return internal::fail((opened != D_NET_OK)
                                  ? internal::listen_error(opened)
                                  : D_SMTP_ERROR_TRANSPORT,
                              D_NULLPTR);
    }

    m_state->port = endpoint.port;

    return port_result(m_state->port);
}

/*
server::run
  Accepts in 100 ms slices so that stop() is noticed promptly, starts a
detached thread per session, each holding a count on the state, and on the
way out waits for every session to finish.
*/
void_result
server::run()
{
    internal::server_state* const state  = m_state;
    ::d_smtp_error                failed = D_SMTP_OK;

    (void)::d_mutex_lock(&state->mutex);

    const bool refused = ( (state->running) ||
                           (!::d_tcp_listener_is_open(&state->listener)) );

    state->running = (state->running) || (!refused);
    (void)::d_mutex_unlock(&state->mutex);

    // one loop at a time, and only over a listener
    if (refused)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              D_NULLPTR);
    }

    // accept until stopped or broken
    while ( (failed == D_SMTP_OK) &&
            (!internal::stopping(state)) )
    {
        ::d_tcp_connection accepted = ::d_tcp_connection();

        ::d_tcp_connection_init(&accepted);

        const ::d_net_error code = ::d_tcp_accept_wait(&state->listener,
                                                       &accepted,
                                                       100L);

        // a connection gets a session; a quiet slice, a look at the flag;
        // a broken listener, the end
        if (code == D_NET_OK)
        {
            internal::start_session(state,
                                    accepted);
        }
        else if (!internal::accept_goes_on(code))
        {
            failed = ::d_smtp_error_from_net(code);
        }
    }

    internal::finish_run(state);

    // a broken listener is the run's failure
    if (failed != D_SMTP_OK)
    {
        return internal::fail(failed,
                              D_NULLPTR);
    }

    return void_result();
}

void
server::stop() D_NOEXCEPT
{
    (void)::d_mutex_lock(&m_state->mutex);
    m_state->stopping = true;
    (void)::d_mutex_unlock(&m_state->mutex);

    return;
}

unsigned int
server::port() const D_NOEXCEPT
{
    return m_state->port;
}

std::size_t
server::active_sessions() const D_NOEXCEPT
{
    (void)::d_mutex_lock(&m_state->mutex);

    const std::size_t active = m_state->active;

    (void)::d_mutex_unlock(&m_state->mutex);

    return active;
}

NS_END  // smtp
NS_END  // net
NS_END  // djinterp

#endif  // D_INTERNAL_NET_HAS_INT64
