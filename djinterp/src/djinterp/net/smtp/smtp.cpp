/*******************************************************************************
* djinterp [net]                                                        smtp.cpp
*
* djinterp SMTP -- C++ facade implementation.
*   Lowering and lifting between the facade's owning types and the kernel's
* borrowed ones, the net::connection bridge, and the server's accept loop and
* session threads. Every function the C kernel calls back into is noexcept
* and guarded, so no exception reaches C.
*
*
* path:      /src/djinterp/net/smtp/smtp.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/net/smtp/smtp.hpp"
// std
#include <atomic>              // std::atomic
#include <condition_variable>  // std::condition_variable
#include <cstddef>             // std::size_t
#include <memory>              // std::make_shared, std::make_unique
#include <mutex>               // std::lock_guard, std::mutex, std::unique_lock
#include <string>              // std::string, std::to_string
#include <string_view>         // std::string_view
#include <thread>              // std::thread
#include <utility>             // std::move
#include <vector>              // std::vector
// djinterp
#include "../../../../inc/djinterp/net/smtp/smtp.h"      // the kernel
#include "../../../../inc/djinterp/env/cpp/env_cpp98.h"  // exceptions
#include "../../../../inc/djinterp/net/net.hpp"          // net::connection


NS_DJINTERP
NS_NET
D_NAMESPACE(smtp)

NS_INTERNAL

    // client_state
    //   struct: a kernel client and, when attached, the connection it reads
    // through. Heap-allocated, so the kernel's pointers into it stay valid
    // however the owning client moves.
    struct client_state
    {
        ::d_smtp_client                  kernel;
        std::unique_ptr<net::connection> stream;

        client_state() noexcept
        {
            ::d_smtp_client_init(&kernel);
        }

        client_state(const client_state& _other)            = delete;
        client_state& operator=(const client_state& _other) = delete;

        ~client_state()
        {
            ::d_smtp_client_close(&kernel);
        }
    };

    // server_state
    //   struct: everything the accept loop and its sessions share.
    struct server_state
    {
        server_options          options;
        server_handler          handler;
        ::d_smtp_server_config  config   = {};
        ::d_smtp_listener       listener = {};
        std::atomic<bool>       stopping { false };
        mutable std::mutex      mutex;
        std::condition_variable changed;
        std::size_t             active   = 0u;
        bool                    running  = false;
    };

#if D_ENV_CPP98_HAS_EXCEPTION

    // guarded
    //   function: calls `_callable` on behalf of the C kernel; an exception
    // must not cross into C, so one becomes `_fallback`.
    template<typename Result,
             typename Callable>
    Result
    guarded(
        Result     _fallback,
        Callable&& _callable
    ) noexcept
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
        Result     _fallback,
        Callable&& _callable
    ) noexcept
    {
        (void)_fallback;

        return _callable();
    }

#endif  // D_ENV_CPP98_HAS_EXCEPTION

    // optional
    //   function: a string's text, or nullptr when it is empty.
    const char*
    optional(
        const std::string& _text
    ) noexcept
    {
        return (_text.empty()) ? nullptr : _text.c_str();
    }

    // lower
    //   function: tls_options as the kernel's borrowed d_smtp_tls_options.
    ::d_smtp_tls_options
    lower(
        const tls_options& _options
    ) noexcept
    {
        ::d_smtp_tls_options lowered = {};

        ::d_smtp_tls_options_init(&lowered);
        lowered.server_name      = optional(_options.server_name);
        lowered.ca_file          = optional(_options.ca_file);
        lowered.certificate_file = optional(_options.certificate_file);
        lowered.private_key_file = optional(_options.private_key_file);
        lowered.verify_peer      = _options.verify_peer;

        return lowered;
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

        lifted.code = static_cast<smtp::error>(_code);

        if (_reply)
        {
            lifted.server_reply = smtp::reply(*_reply);
        }

        return ::re_std::unexpected<failure>(std::move(lifted));
    }

    // finish
    //   function: a command's outcome: the reply, or the failure.
    result<smtp::reply>
    finish(
        ::d_smtp_error        _code,
        const ::d_smtp_reply& _reply
    )
    {
        if (_code != D_SMTP_OK)
        {
            return fail(_code,
                        &_reply);
        }

        return smtp::reply(_reply);
    }

    // kernel_error
    //   function: a net::io_error in the kernel's vocabulary.
    ::d_smtp_error
    kernel_error(
        io_error _error
    ) noexcept
    {
        switch (_error)
        {
            case io_error::none:
                return D_SMTP_OK;

            case io_error::closed:
            case io_error::connection_reset:
            case io_error::connection_aborted:
                return D_SMTP_ERROR_CLOSED;

            case io_error::timed_out:
            case io_error::would_block:
                return D_SMTP_ERROR_TIMEOUT;

            default:
                break;
        }

        return D_SMTP_ERROR_IO;
    }

    // bridge_read
    //   function: the kernel's read over a net::connection; a read of
    // nothing without an error is the end of the stream.
    ::d_smtp_error
    bridge_read(
        void*        _context,
        void*        _buffer,
        std::size_t  _capacity,
        std::size_t* _received
    ) noexcept
    {
        client_state*   state = static_cast<client_state*>(_context);
        const io_result got   = guarded(io_result(0u, io_error::unknown),
                                        [&]()
                                        {
                                            return state->stream->read(
                                                       _buffer,
                                                       _capacity);
                                        });

        if (got.error != io_error::none)
        {
            return kernel_error(got.error);
        }

        if (got.count == 0u)
        {
            return D_SMTP_ERROR_CLOSED;
        }

        *_received = got.count;

        return D_SMTP_OK;
    }

    // bridge_write
    //   function: the kernel's write over a net::connection.
    ::d_smtp_error
    bridge_write(
        void*        _context,
        const void*  _data,
        std::size_t  _length,
        std::size_t* _sent
    ) noexcept
    {
        client_state*   state = static_cast<client_state*>(_context);
        const io_result put   = guarded(io_result(0u, io_error::unknown),
                                        [&]()
                                        {
                                            return state->stream->write(
                                                       _data,
                                                       _length);
                                        });

        if (put.error != io_error::none)
        {
            return kernel_error(put.error);
        }

        *_sent = put.count;

        return D_SMTP_OK;
    }

    // bridge_close
    //   function: the kernel's release of a net::connection.
    void
    bridge_close(
        void* _context
    ) noexcept
    {
        client_state* state = static_cast<client_state*>(_context);

        (void)guarded(false,
                      [&]()
                      {
                          state->stream->close();

                          return true;
                      });

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

        for (std::size_t i = 0; i < _info.recipient_count; ++i)
        {
            lifted.recipients.emplace_back(_info.recipients[i]);
        }

        return lifted;
    }

    // on_mail, on_rcpt, on_message, on_auth
    //   function: the kernel's callbacks, forwarded to the std::function
    // handlers; a throwing handler answers 451 (or refuses AUTH).
    unsigned int
    on_mail(
        void*                        _context,
        const ::d_smtp_session_info* _info,
        const char*                  _sender
    ) noexcept
    {
        const server_state* state = static_cast<const server_state*>(_context);

        return guarded(451u,
                       [&]()
                       {
                           return state->handler.on_mail(lift(*_info),
                                                         std::string(_sender));
                       });
    }

    unsigned int
    on_rcpt(
        void*                        _context,
        const ::d_smtp_session_info* _info,
        const char*                  _recipient
    ) noexcept
    {
        const server_state* state = static_cast<const server_state*>(_context);

        return guarded(451u,
                       [&]()
                       {
                           return state->handler.on_rcpt(
                                      lift(*_info),
                                      std::string(_recipient));
                       });
    }

    unsigned int
    on_message(
        void*                        _context,
        const ::d_smtp_session_info* _info,
        const char*                  _data,
        std::size_t                  _length
    ) noexcept
    {
        const server_state* state = static_cast<const server_state*>(_context);

        return guarded(451u,
                       [&]()
                       {
                           return state->handler.on_message(
                                      lift(*_info),
                                      std::string_view(_data, _length));
                       });
    }

    bool
    on_auth(
        void*       _context,
        const char* _user,
        const char* _secret
    ) noexcept
    {
        const server_state* state = static_cast<const server_state*>(_context);

        return guarded(false,
                       [&]()
                       {
                           return state->handler.on_auth(std::string(_user),
                                                         std::string(_secret));
                       });
    }

    // configure
    //   function: the kernel configuration, pointing into the options the
    // state owns.
    void
    configure(
        server_state& _state
    ) noexcept
    {
        ::d_smtp_server_config_init(&_state.config);
        _state.config.domain              = _state.options.domain.c_str();
        _state.config.max_message_size    = _state.options.max_message_size;
        _state.config.max_recipients      = _state.options.max_recipients;
        _state.config.tls                 = lower(_state.options.tls);
        _state.config.auth_mechanisms     = _state.options.auth_mechanisms;
        _state.config.require_tls         = _state.options.require_tls;
        _state.config.require_auth        = _state.options.require_auth;
        _state.config.allow_insecure_auth = _state.options.allow_insecure_auth;

        return;
    }

    // serve
    //   function: one session on its own thread. The socket is this frame's
    // copy, which stays put while TLS points at it.
    void
    serve(
        std::shared_ptr<server_state> _state,
        ::d_smtp_socket               _socket
    ) noexcept
    {
        ::d_smtp_server_config  config    = _state->config;
        ::d_smtp_server_handler handler   = {};
        ::d_smtp_transport      transport = {};
        ::d_smtp_error          result    = D_SMTP_OK;

        config.secure   = _state->options.implicit_tls;
        handler.context = _state.get();
        handler.mail    = (_state->handler.on_mail) ? on_mail : nullptr;
        handler.rcpt    = (_state->handler.on_rcpt) ? on_rcpt : nullptr;
        handler.message = (_state->handler.on_message) ? on_message : nullptr;
        handler.auth    = (_state->handler.on_auth) ? on_auth : nullptr;

        // implicit TLS: the handshake comes before the greeting (RFC 8314)
        if (config.secure)
        {
            result = ::d_smtp_socket_start_tls(&_socket,
                                               &config.tls);
        }

        if (result == D_SMTP_OK)
        {
            ::d_smtp_socket_transport(&_socket,
                                      &transport);
            (void)::d_smtp_server_session(&transport,
                                          &config,
                                          &handler);
        }

        ::d_smtp_socket_close(&_socket);

        {
            std::lock_guard<std::mutex> lock(_state->mutex);

            _state->active -= 1u;
        }

        _state->changed.notify_all();

        return;
    }

NS_END  // internal

const char*
to_string(
    error _error
) noexcept
{
    return ::d_smtp_error_string(static_cast<::d_smtp_error>(_error));
}

status::status() noexcept
    : ::d_smtp_status{ 0u, 0u, 0u }
{}

status::status(
    const ::d_smtp_status& _status
) noexcept
    : ::d_smtp_status(_status)
{}

bool
status::present() const noexcept
{
    return (status_class != 0u);
}

std::string
status::to_string() const
{
    if (!present())
    {
        return std::string();
    }

    return std::to_string(status_class) + "." +
           std::to_string(subject) + "." +
           std::to_string(detail);
}

capabilities::capabilities() noexcept
    : ::d_smtp_capabilities{ 0u, 0u, 0u, false }
{}

capabilities::capabilities(
    const ::d_smtp_capabilities& _capabilities
) noexcept
    : ::d_smtp_capabilities(_capabilities)
{}

bool
capabilities::supports(
    extension _extension
) const noexcept
{
    return ((extensions & static_cast<unsigned int>(_extension)) != 0u);
}

bool
capabilities::offers(
    mechanism _mechanism
) const noexcept
{
    return ((auth & static_cast<unsigned int>(_mechanism)) != 0u);
}

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
    for (std::size_t i = 0; i < _reply.line_count; ++i)
    {
        std::size_t length = 0;
        const char* line   = ::d_smtp_reply_line(&_reply,
                                                 i,
                                                 &length);

        if (!line)
        {
            break;
        }

        lines.emplace_back(line,
                           length);
    }
}

std::string
reply::text() const
{
    std::string joined;

    for (std::size_t i = 0; i < lines.size(); ++i)
    {
        joined += (i > 0) ? "\n" : "";
        joined += lines[i];
    }

    return joined;
}

bool
reply::positive() const noexcept
{
    return ::d_smtp_code_is_positive(code);
}

bool
reply::transient() const noexcept
{
    return ::d_smtp_code_is_transient(code);
}

bool
reply::permanent() const noexcept
{
    return ::d_smtp_code_is_permanent(code);
}

client::client() noexcept = default;

client::client(
    std::unique_ptr<internal::client_state> _state
) noexcept
    : m_state(std::move(_state))
{}

client::client(client&& _other) noexcept = default;

client& client::operator=(client&& _other) noexcept = default;

client::~client() = default;

result<client>
client::connect(
    const client_options& _options
)
{
    ::d_smtp_client_options options = {};

    ::d_smtp_client_options_init(&options);
    options.host                = _options.host.c_str();
    options.port                = _options.port;
    options.security            = static_cast<::d_smtp_security>(_options.mode);
    options.tls                 = internal::lower(_options.tls);
    options.helo_domain         = internal::optional(_options.helo_domain);
    options.connect_timeout_ms  = _options.connect_timeout_ms;
    options.io_timeout_ms       = _options.io_timeout_ms;
    options.allow_insecure_auth = _options.allow_insecure_auth;

    auto                 state = std::make_unique<internal::client_state>();
    const ::d_smtp_error code  = ::d_smtp_client_connect(&state->kernel,
                                                         &options);

    if (code != D_SMTP_OK)
    {
        return internal::fail(code,
                              &state->kernel.reply);
    }

    return client(std::move(state));
}

/*
client::attach
  The bridge's context is the heap-held state itself, so it survives every
move of the returned client.
*/
result<client>
client::attach(
    std::unique_ptr<net::connection> _connection,
    bool                             _secure,
    const std::string&               _helo_domain
)
{
    if (!_connection)
    {
        return internal::fail(D_SMTP_ERROR_INVALID,
                              nullptr);
    }

    auto               state     = std::make_unique<internal::client_state>();
    ::d_smtp_transport transport = {};

    state->stream      = std::move(_connection);
    transport.context  = state.get();
    transport.read     = internal::bridge_read;
    transport.write    = internal::bridge_write;
    transport.starttls = nullptr;
    transport.close    = internal::bridge_close;

    ::d_smtp_error code = ::d_smtp_client_attach(&state->kernel,
                                                 &transport,
                                                 _secure);

    if (code == D_SMTP_OK)
    {
        code = ::d_smtp_client_greeting(&state->kernel);
    }

    if (code == D_SMTP_OK)
    {
        code = ::d_smtp_client_hello(&state->kernel,
                                     _helo_domain.c_str());
    }

    if (code != D_SMTP_OK)
    {
        return internal::fail(code,
                              &state->kernel.reply);
    }

    return client(std::move(state));
}

bool
client::is_open() const noexcept
{
    return ( (m_state) &&
             (m_state->kernel.state != D_SMTP_CLIENT_CLOSED) );
}

bool
client::is_secure() const noexcept
{
    return ( (m_state) &&
             (m_state->kernel.secure) );
}

bool
client::is_authenticated() const noexcept
{
    return ( (m_state) &&
             (m_state->kernel.authenticated) );
}

smtp::capabilities
client::features() const noexcept
{
    return (m_state) ? smtp::capabilities(m_state->kernel.capabilities)
                     : smtp::capabilities();
}

smtp::reply
client::last_reply() const
{
    return (m_state) ? smtp::reply(m_state->kernel.reply) : smtp::reply();
}

result<smtp::reply>
client::starttls(
    const tls_options& _options
)
{
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              nullptr);
    }

    const ::d_smtp_tls_options lowered = internal::lower(_options);

    return internal::finish(::d_smtp_client_starttls(&m_state->kernel,
                                                     &lowered),
                            m_state->kernel.reply);
}

result<smtp::reply>
client::authenticate(
    const credentials& _credentials
)
{
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              nullptr);
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
result<send_report>
client::send(
    const envelope&  _envelope,
    std::string_view _message
)
{
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              nullptr);
    }

    std::vector<const char*>               recipients;
    std::vector<::d_smtp_recipient_status> statuses(
                                               _envelope.recipients.size());

    recipients.reserve(_envelope.recipients.size());

    for (const std::string& recipient : _envelope.recipients)
    {
        recipients.push_back(recipient.c_str());
    }

    const ::d_smtp_envelope lowered =
    {
        _envelope.sender.c_str(),
        recipients.data(),
        recipients.size(),
        _envelope.eight_bit,
        _envelope.utf8
    };
    ::d_smtp_send_report report = { statuses.data(), 0u, 0u };
    const ::d_smtp_error code   = ::d_smtp_client_send(&m_state->kernel,
                                                       &lowered,
                                                       _message.data(),
                                                       _message.size(),
                                                       &report);

    if (code != D_SMTP_OK)
    {
        return internal::fail(code,
                              &m_state->kernel.reply);
    }

    send_report lifted;

    lifted.accepted  = report.accepted;
    lifted.data_code = report.data_code;

    for (std::size_t i = 0; i < statuses.size(); ++i)
    {
        recipient_result entry;

        entry.address = _envelope.recipients[i];
        entry.code    = statuses[i].code;
        entry.status  = smtp::status(statuses[i].status);
        lifted.recipients.push_back(std::move(entry));
    }

    return lifted;
}

result<smtp::reply>
client::reset()
{
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              nullptr);
    }

    return internal::finish(::d_smtp_client_reset(&m_state->kernel),
                            m_state->kernel.reply);
}

result<smtp::reply>
client::noop()
{
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              nullptr);
    }

    return internal::finish(::d_smtp_client_noop(&m_state->kernel),
                            m_state->kernel.reply);
}

result<smtp::reply>
client::quit()
{
    if (!m_state)
    {
        return internal::fail(D_SMTP_ERROR_STATE,
                              nullptr);
    }

    return internal::finish(::d_smtp_client_quit(&m_state->kernel),
                            m_state->kernel.reply);
}

void
client::close() noexcept
{
    if (m_state)
    {
        ::d_smtp_client_close(&m_state->kernel);
    }

    return;
}

result<send_report>
send(
    const client_options& _options,
    const credentials&    _credentials,
    const envelope&       _envelope,
    std::string_view      _message
)
{
    result<client> opened = client::connect(_options);

    if (!opened)
    {
        return ::re_std::unexpected<failure>(opened.error());
    }

    client& session = *opened;

    if (!_credentials.user.empty())
    {
        const result<smtp::reply> authenticated = session.authenticate(
                                                      _credentials);

        if (!authenticated)
        {
            return ::re_std::unexpected<failure>(authenticated.error());
        }
    }

    result<send_report> sent = session.send(_envelope,
                                            _message);

    (void)session.quit();

    return sent;
}

server::server(
    server_options _options,
    server_handler _handler
)
    : m_state(std::make_shared<internal::server_state>())
{
    m_state->options = std::move(_options);
    m_state->handler = std::move(_handler);
    ::d_smtp_listener_init(&m_state->listener);
    internal::configure(*m_state);
}

/*
server::~server
  A running accept loop is stopped and allowed to finish -- it waits for
its sessions -- before the listener is closed beneath it.
*/
server::~server()
{
    std::unique_lock<std::mutex> lock(m_state->mutex);

    m_state->stopping.store(true);
    m_state->changed.wait(lock,
                          [this]()
                          {
                              return !m_state->running;
                          });
    lock.unlock();
    ::d_smtp_listener_close(&m_state->listener);
}

result<unsigned int>
server::listen(
    const std::string& _host,
    unsigned int       _port
)
{
    const ::d_smtp_error code = ::d_smtp_listener_open(
                                    &m_state->listener,
                                    internal::optional(_host),
                                    _port);

    if (code != D_SMTP_OK)
    {
        return internal::fail(code,
                              nullptr);
    }

    return m_state->listener.port;
}

/*
server::run
  Accepts in 100 ms slices so that stop() is noticed promptly, starts a
detached thread per session (the shared state outlives them all), and on
the way out waits for every session to finish.
*/
result<void>
server::run()
{
    const std::shared_ptr<internal::server_state> state = m_state;
    ::d_smtp_error                                failed = D_SMTP_OK;

    {
        std::lock_guard<std::mutex> lock(state->mutex);

        if ( (state->running) ||
             (state->listener.descriptor < 0) )
        {
            return internal::fail(D_SMTP_ERROR_STATE,
                                  nullptr);
        }

        state->running = true;
    }

    while ( (!state->stopping.load()) &&
            (failed == D_SMTP_OK) )
    {
        ::d_smtp_socket accepted = {};

        ::d_smtp_socket_init(&accepted);

        const ::d_smtp_error code = ::d_smtp_listener_accept(
                                        &state->listener,
                                        100,
                                        state->options.io_timeout_ms,
                                        &accepted);

        if (code == D_SMTP_ERROR_TIMEOUT)
        {
            continue;
        }

        if (code != D_SMTP_OK)
        {
            failed = code;

            continue;
        }

        {
            std::lock_guard<std::mutex> lock(state->mutex);

            state->active += 1u;
        }

        std::thread(internal::serve,
                    state,
                    accepted).detach();
    }

    std::unique_lock<std::mutex> lock(state->mutex);

    state->changed.wait(lock,
                        [&state]()
                        {
                            return (state->active == 0u);
                        });
    state->running = false;
    lock.unlock();
    state->changed.notify_all();

    if (failed != D_SMTP_OK)
    {
        return internal::fail(failed,
                              nullptr);
    }

    return {};
}

void
server::stop() noexcept
{
    m_state->stopping.store(true);

    return;
}

unsigned int
server::port() const noexcept
{
    return m_state->listener.port;
}

std::size_t
server::active_sessions() const noexcept
{
    std::lock_guard<std::mutex> lock(m_state->mutex);

    return m_state->active;
}


NS_END  // smtp
NS_END  // net
NS_END  // djinterp
