/*******************************************************************************
* djinterp [net]                                                        smtp.hpp
*
* djinterp SMTP -- C++ facade.
*   The C++ face of the SMTP kernel in net/smtp. It derives from the kernel
* rather than re-implementing it: statuses and capabilities are the kernel's
* own structs extended with C++ accessors, the enumerations carry the
* kernel's values, and the client and server hold the kernel's session
* state, lowering std::string arguments into it and lifting replies back out
* as owning values. What the facade adds is ownership -- RAII over the
* kernel's sessions, the connections it is given, and the TLS contexts it
* makes -- failure by value through re_std::expected, a server that runs
* each session on a thread of its own behind a tcp.h listener, and sessions
* over any net::connection, through net.hpp's adapter, which STARTTLS can
* secure like any other.
*   TLS is the SSL kernel's. By default the facade makes a context from
* tls_options with the OpenSSL-family engine, where the build has one; a
* caller who wants another engine, pins, ALPN or a version floor makes a
* context of their own and passes it instead.
*   Nothing here throws on the SMTP path. Callbacks the C kernel invokes are
* guarded, so where exceptions are enabled one thrown by a handler becomes a
* 451 reply instead of crossing the C boundary.
*   C++98 and later. A client opens its session in place at every level, and
* from C++11 connect() and attach() also return one, which only C++11 can
* move; results are re_std::expected, named per payload (reply_result and
* the rest) at every level and also result<T> from C++11. Handlers are
* re_std::function, message bodies re_std::string_view, and the server's
* threads, lock and condition are dmutex.h's. Enumerations are scoped in
* structs (decision 3.4): smtp::error::value. Link smtp's C sources, net.c,
* tcp.c, the SSL kernel and its OpenSSL-family engine, net's codec,
* sink_common.c and dmutex.c, with the TLS library. The client holds the
* kernel's TLS session, so the header needs a 64-bit integer type, and
* compiles to nothing where the build has none, as smtp_client.h does.
*
*   Where D_INTERNAL_NET_HAS_INT64 is 0, with no 64-bit integer type, it
* compiles to nothing, unless D_CFG_NET_REQUIRE_BACKENDS asks for an #error
* (decision 101 of the register).
*
*
* path:      /inc/djinterp/net/smtp/smtp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Enumerations
         1.  error
         2.  security
         3.  mechanism
         4.  extension
    2.  Status and capabilities
         1.  status
         2.  capabilities
    3.  Replies and failures
         1.  reply
         2.  failure
         3.  results
2.  CLIENT
    ------
    1.  Options
         1.  tls_options
         2.  client_options
         3.  attach_options
         4.  credentials
    2.  Transactions
         1.  envelope
         2.  recipient_result
         3.  send_report
    3.  Client
         1.  client
         2.  send
3.  SERVER
    ------
    1.  Options and handlers
         1.  server_options
         2.  session
         3.  server_handler
    2.  Server
         1.  server
*/

#ifndef DJINTERP_NET_SMTP_SMTP_HPP
#define DJINTERP_NET_SMTP_SMTP_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "../net_common.hpp"   // net::connection
#include "../../config/net/cfg_net.h"  // D_INTERNAL_NET_REQUIRE_BACKENDS
// the C kernel comes after net.hpp: in C++, GCC's <stdbool.h> defines _Bool
// as a macro, which breaks the `_Bool` template parameter of
// core/binary/decode.hpp (reached through net.hpp) if it is seen first
#include "smtp.h"              // the C SMTP kernel
#include "../net_common.h"      // D_INTERNAL_NET_HAS_INT64
#include "../ssl/ssl.h"        // d_ssl_context
#include "../tls/tls.hpp"      // tls_lease
#if (D_MOVE_ENABLED == 1)
    // std
    #include <memory>  // std::unique_ptr, for attach
#endif  // D_MOVE_ENABLED
// re_std
#include "../../../re_std/expected/expected.hpp"      // re_std::expected
#include "../../../re_std/functional/function.hpp"    // re_std::function
#include "../../../re_std/string_view/string_view_typedefs.hpp"  // string_view

// 64-bit floor: this header needs a 64-bit integer type, and compiles to
// nothing where the build has none -- ISO strict C++98 on a 32-bit target
// (the owner's ruling of 2026.10.03 on round 3's question 1, (a)). The
// answer is net_common.h's, taken once per translation unit, so that net's
// gated headers agree.
#if (D_INTERNAL_NET_HAS_INT64 == 1)


NS_DJINTERP
NS_NET
D_NAMESPACE(smtp)

NS_INTERNAL

    // client_state, server_state
    //   struct: the kernel sessions behind client and server (smtp.cpp).
    struct client_state;
    struct server_state;

NS_END  // internal


//==============================================================================
// 1.  VOCABULARY
//==============================================================================
// The kernel's vocabulary in C++ dress. Every enumerator is the kernel's own
// value, so crossing between the two is a cast, never a table. Each
// enumeration is scoped in a struct at every level (decision 3.4): the type
// of an error is error::value.


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// error
//   enum: d_smtp_error, scoped.
struct error
{
    enum value
    {
        none             = D_SMTP_OK,
        invalid_argument = D_SMTP_ERROR_INVALID_ARGUMENT,
        state            = D_SMTP_ERROR_STATE,
        out_of_memory    = D_SMTP_ERROR_OUT_OF_MEMORY,
        resolve          = D_SMTP_ERROR_RESOLVE,
        connect          = D_SMTP_ERROR_CONNECT,
        timed_out        = D_SMTP_ERROR_TIMED_OUT,
        transport        = D_SMTP_ERROR_TRANSPORT,
        closed           = D_SMTP_ERROR_CLOSED,
        tls              = D_SMTP_ERROR_TLS,
        protocol         = D_SMTP_ERROR_PROTOCOL,
        line_too_long    = D_SMTP_ERROR_LINE_TOO_LONG,
        rejected         = D_SMTP_ERROR_REJECTED,
        unsupported      = D_SMTP_ERROR_UNSUPPORTED,
        auth             = D_SMTP_ERROR_AUTH,
        insecure         = D_SMTP_ERROR_INSECURE
    };
};

// 1.1.2
// security
//   enum: d_smtp_security, scoped; `starttls` fails closed.
struct security
{
    enum value
    {
        none              = D_SMTP_SECURITY_NONE,
        starttls_optional = D_SMTP_SECURITY_STARTTLS_OPTIONAL,
        starttls          = D_SMTP_SECURITY_STARTTLS,
        tls               = D_SMTP_SECURITY_TLS
    };
};

// 1.1.3
// mechanism
//   enum: d_smtp_auth, scoped; `automatic` picks the best password
// mechanism offered.
struct mechanism
{
    enum value
    {
        automatic = D_SMTP_AUTH_NONE,
        plain     = D_SMTP_AUTH_PLAIN,
        login     = D_SMTP_AUTH_LOGIN,
        cram_md5  = D_SMTP_AUTH_CRAM_MD5,
        xoauth2   = D_SMTP_AUTH_XOAUTH2
    };
};

// 1.1.4
// extension
//   enum: d_smtp_extension, scoped.
struct extension
{
    enum value
    {
        pipelining            = D_SMTP_EXTENSION_PIPELINING,
        size                  = D_SMTP_EXTENSION_SIZE,
        eight_bit_mime        = D_SMTP_EXTENSION_8BITMIME,
        starttls              = D_SMTP_EXTENSION_STARTTLS,
        auth                  = D_SMTP_EXTENSION_AUTH,
        enhanced_status_codes = D_SMTP_EXTENSION_ENHANCEDSTATUSCODES,
        smtputf8              = D_SMTP_EXTENSION_SMTPUTF8,
        chunking              = D_SMTP_EXTENSION_CHUNKING,
        dsn                   = D_SMTP_EXTENSION_DSN
    };
};

// to_string -- a short label for an error; O(1), never null. to_net -- the
// io_error an error maps to, by the rules net.h gives with d_net_error
D_NODISCARD const char*      to_string(error::value _error) D_NOEXCEPT;
D_NODISCARD io_error::value to_net(error::value _error) D_NOEXCEPT;

// 1.2    Status and capabilities
//------------------------------------------------------------------------------
// 1.2.1
// status
//   struct: an RFC 3463 enhanced status -- the kernel's d_smtp_status.
struct status : public ::d_smtp_status
{
    status() D_NOEXCEPT;
    explicit status(const ::d_smtp_status& _status) D_NOEXCEPT;

    // present -- whether the reply carried a status; to_string -- "5.1.1",
    // or "" when absent
    D_NODISCARD bool        present() const D_NOEXCEPT;
    D_NODISCARD std::string to_string() const;
};

// 1.2.2
// capabilities
//   struct: what a server offered in EHLO -- the kernel's
// d_smtp_capabilities.
struct capabilities : public ::d_smtp_capabilities
{
    capabilities() D_NOEXCEPT;
    explicit capabilities(
        const ::d_smtp_capabilities& _capabilities) D_NOEXCEPT;

    // queries -- O(1), never fail
    D_NODISCARD bool supports(extension::value _extension) const D_NOEXCEPT;
    D_NODISCARD bool offers(mechanism::value _mechanism) const D_NOEXCEPT;
};

// 1.3    Replies and failures
//------------------------------------------------------------------------------
// 1.3.1
// reply
//   struct: an owning copy of a kernel reply, one string per line.
struct reply
{
    reply();
    explicit reply(const ::d_smtp_reply& _reply);

    // text -- the lines joined by '\n'; classification by first digit
    D_NODISCARD std::string text() const;
    D_NODISCARD bool        positive() const D_NOEXCEPT;
    D_NODISCARD bool        transient() const D_NOEXCEPT;
    D_NODISCARD bool        permanent() const D_NOEXCEPT;

    unsigned int             code;
    smtp::status             status;
    std::vector<std::string> lines;
    bool                     truncated;
};

// 1.3.2
// failure
//   struct: why an operation failed, and the server's reply when there was
// one (`server_reply.code` is 0 otherwise).
struct failure
{
    failure()
        : code(error::none),
          server_reply()
    {}

    smtp::error::value code;
    smtp::reply        server_reply;
};

// 1.3.3
// results
//   type: the value of an operation, or the failure that prevented it: one
// name for each payload at every level, and from C++11 the alias template
// result<T> as well, which names the same types.
struct send_report;

typedef ::re_std::expected<smtp::reply, failure>  reply_result;
typedef ::re_std::expected<send_report, failure>  report_result;
typedef ::re_std::expected<unsigned int, failure> port_result;
typedef ::re_std::expected<void, failure>         void_result;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename Type>
using result = ::re_std::expected<Type, failure>;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


//==============================================================================
// 2.  CLIENT
//==============================================================================
// A client session in owning C++ form. open() runs the kernel's whole
// opening -- greeting, EHLO and the STARTTLS policy -- or runs a session
// over any net::connection, which starttls() can secure too.


// 2.1    Options
//------------------------------------------------------------------------------
// 2.1.1
// tls_options
//   struct: how TLS is made. By default the facade makes a context of the
// SSL kernel from these fields with the OpenSSL-family engine -- a client
// checks the server against `ca_file`, or the system's trust store when it
// is empty, unless `verify_peer` is false; a server presents its
// certificate and key. `context` opts out of all that: a context of the
// caller's own, of any engine, borrowed for as long as any session made
// with it lasts, stands in for every field but `server_name`. `lease` does
// the same with a tls_context's live context (tls_context::lease()), which
// each session made with it holds, so a C++ program configures TLS once;
// `context` is used before it. Empty strings mean "not set".
struct tls_options
{
    tls_options()
        : server_name(),
          ca_file(),
          certificate_file(),
          private_key_file(),
          verify_peer(true),
          context(D_NULLPTR),
          lease()
    {}

    std::string            server_name;       // client: defaults to the host
    std::string            ca_file;           // client: empty for the system
    std::string            certificate_file;  // server
    std::string            private_key_file;  // server
    bool                   verify_peer;       // client
    const ::d_ssl_context* context;           // the caller's own, or null
    tls_lease              lease;             // a tls_context's, or empty
};

// 2.1.2
// client_options
//   struct: d_smtp_client_options with owned strings and the same defaults.
struct client_options
{
    client_options()
        : host(),
          port(0u),
          mode(smtp::security::starttls),
          tls(),
          helo_domain(),
          connect_timeout_ms(D_SMTP_CLIENT_CONNECT_TIMEOUT_MS),
          io_timeout_ms(D_SMTP_CLIENT_IO_TIMEOUT_MS),
          allow_insecure_auth(false)
    {}

    std::string           host;
    unsigned int          port;
    smtp::security::value mode;
    tls_options           tls;
    std::string           helo_domain;
    long                  connect_timeout_ms;
    long                  io_timeout_ms;
    bool                  allow_insecure_auth;
};

// 2.1.3
// attach_options
//   struct: what open() and attach() take with a caller's stream: whether it
// is already encrypted, the identity to announce, and whether AUTH may run
// without TLS (decision 107 of the register).
struct attach_options
{
    attach_options()
        : secure(false),
          helo_domain(),
          allow_insecure_auth(false)
    {}

    bool        secure;
    std::string helo_domain;
    bool        allow_insecure_auth;
};

// 2.1.4
// credentials
//   struct: an identity, its secret, and the mechanism to use.
struct credentials
{
    credentials()
        : user(),
          secret(),
          method(smtp::mechanism::automatic)
    {}

    std::string            user;
    std::string            secret;
    smtp::mechanism::value method;
};

// 2.2    Transactions
//------------------------------------------------------------------------------
// 2.2.1
// envelope
//   struct: d_smtp_envelope with owned strings.
struct envelope
{
    envelope()
        : sender(),
          recipients(),
          eight_bit(false),
          utf8(false)
    {}

    std::string              sender;
    std::vector<std::string> recipients;
    bool                     eight_bit;
    bool                     utf8;
};

// 2.2.2
// recipient_result
//   struct: what the server said to one RCPT.
struct recipient_result
{
    recipient_result()
        : address(),
          code(0u),
          status()
    {}

    std::string  address;
    unsigned int code;
    smtp::status status;
};

// 2.2.3
// send_report
//   struct: what became of a transaction.
struct send_report
{
    send_report()
        : recipients(),
          accepted(0u),
          data_code(0u)
    {}

    std::vector<recipient_result> recipients;
    std::size_t                   accepted;
    unsigned int                  data_code;
};

// 2.3    Client
//------------------------------------------------------------------------------
// 2.3.1
// client
//   class: one client session; not copyable, movable from C++11. The
// session closes, without QUIT, when the client is destroyed.
class client
{
public:
    // a closed client, for open() to open in place
    client() D_NOEXCEPT;
#if (D_MOVE_ENABLED == 1)
    client(client&& _other) D_NOEXCEPT;
    client& operator=(client&& _other) D_NOEXCEPT;
#endif  // D_MOVE_ENABLED
    ~client();

    /**
     * @brief Connects and opens a session per the options' security mode,
     *        closing any session this client held first.
     *
     * @param[in] _options  where and how to connect.
     * @return nothing, or the failure with the server's reply.
     */
    D_NODISCARD void_result open(const client_options& _options);

    /**
     * @brief Runs a session over a caller's connection: reads the greeting
     *        and says EHLO, closing any session this client held first.
     *        starttls() can secure the connection afterwards.
     *
     * @param[in] _connection   the stream, which new allocated; the client
     *                          owns it from here, whatever the outcome, and
     *                          closes and deletes it.
     * @param[in] _secure       whether the stream is already encrypted.
     * @param[in] _helo_domain  the identity to announce.
     * @return nothing, or the failure.
     */
    D_NODISCARD void_result open(net::connection*   _connection,
                                 bool               _secure,
                                 const std::string& _helo_domain);

    /**
     * @brief open() over a caller's connection with the options that apply
     *        to one, AUTH without TLS among them.
     *
     * @param[in] _connection  the stream, which new allocated; the client
     *                         owns it from here, whatever the outcome, and
     *                         closes and deletes it.
     * @param[in] _options     whether the stream is encrypted, the identity
     *                         to announce, and whether AUTH may run without
     *                         TLS.
     * @return nothing, or the failure.
     */
    D_NODISCARD void_result open(net::connection*      _connection,
                                 const attach_options& _options);

#if (D_MOVE_ENABLED == 1)
    /**
     * @brief open(), returning the opened client, which C++11 can move.
     *
     * @param[in] _options  where and how to connect.
     * @return the open session, or the failure with the server's reply.
     */
    D_NODISCARD static result<client> connect(
                                          const client_options& _options);

    /**
     * @brief open() over a caller's connection, returning the client.
     *
     * @param[in] _connection   the stream; owned and closed by the client.
     * @param[in] _secure       whether the stream is already encrypted.
     * @param[in] _helo_domain  the identity to announce.
     * @return the open session, or the failure.
     */
    D_NODISCARD static result<client> attach(
                             std::unique_ptr<net::connection> _connection,
                             bool                             _secure,
                             const std::string&               _helo_domain);

    /**
     * @brief open() over a caller's connection with its options, returning
     *        the client.
     *
     * @param[in] _connection  the stream; owned and closed by the client.
     * @param[in] _options     as open() takes them.
     * @return the open session, or the failure.
     */
    D_NODISCARD static result<client> attach(
                             std::unique_ptr<net::connection> _connection,
                             const attach_options&            _options);
#endif  // D_MOVE_ENABLED

    // session state -- O(1), never fail; a closed client reports closed
    D_NODISCARD bool               is_open() const D_NOEXCEPT;
    D_NODISCARD bool               is_secure() const D_NOEXCEPT;
    D_NODISCARD bool               is_authenticated() const D_NOEXCEPT;
    D_NODISCARD smtp::capabilities features() const D_NOEXCEPT;
    D_NODISCARD smtp::reply        last_reply() const;

    // commands -- each returns the server's reply, or the failure; errors
    // are those of the d_smtp_client_* function of the same name. starttls
    // also fails with error::unsupported when it must make a context and
    // the build has no engine to make it with, and error::tls when the
    // options' files will not load; its name defaults to the host open()
    // connected to
    D_NODISCARD reply_result  starttls(const tls_options& _options);
    D_NODISCARD reply_result  authenticate(const credentials& _credentials);
    D_NODISCARD report_result send(const envelope&    _envelope,
                                   re_std::string_view _message);
    D_NODISCARD reply_result  reset();
    D_NODISCARD reply_result  noop();
    D_NODISCARD reply_result  quit();

    // close -- releases the connection without QUIT; idempotent
    void close() D_NOEXCEPT;

private:
    D_DELETED_FN(client(const client&))
    D_DELETED_FN(client& operator=(const client&))

    internal::client_state* m_state;  // owned; null when closed
};

// 2.3.2
// send
/**
 * @brief Delivers one message: connect, authenticate when a user is given,
 *        send, and quit.
 *
 * @param[in] _options      where and how to connect.
 * @param[in] _credentials  used when `user` is not empty.
 * @param[in] _envelope     sender and recipients.
 * @param[in] _message      the message, headers included.
 * @return the report, or the first failure.
 */
D_NODISCARD report_result send(const client_options& _options,
                               const credentials&    _credentials,
                               const envelope&       _envelope,
                               re_std::string_view   _message);


//==============================================================================
// 3.  SERVER
//==============================================================================
// The kernel's server engine behind a listener and a thread per session.


// 3.1    Options and handlers
//------------------------------------------------------------------------------
// 3.1.1
// server_options
//   struct: d_smtp_server_config with owned strings, plus implicit TLS and
// the I/O limit for accepted connections. With a certificate and key in
// `tls`, or a context of the caller's, the server offers STARTTLS, or
// speaks TLS from the first byte when `implicit_tls`; with neither, it
// offers no TLS at all.
struct server_options
{
    server_options()
        : domain("localhost"),
          max_message_size(D_SMTP_SERVER_MESSAGE_MAX),
          max_recipients(D_SMTP_SERVER_RECIPIENTS_MAX),
          tls(),
          implicit_tls(false),
          auth_mechanisms(D_SMTP_AUTH_PLAIN | D_SMTP_AUTH_LOGIN),
          require_tls(false),
          require_auth(false),
          allow_insecure_auth(false),
          io_timeout_ms(D_SMTP_CLIENT_IO_TIMEOUT_MS)
    {}

    std::string  domain;
    std::size_t  max_message_size;
    std::size_t  max_recipients;
    tls_options  tls;
    bool         implicit_tls;
    unsigned int auth_mechanisms;
    bool         require_tls;
    bool         require_auth;
    bool         allow_insecure_auth;
    long         io_timeout_ms;
};

// 3.1.2
// session
//   struct: d_smtp_session_info as owned values, for handlers.
struct session
{
    session()
        : client_domain(),
          user(),
          sender(),
          recipients(),
          secure(false),
          eight_bit(false),
          utf8(false)
    {}

    std::string              client_domain;
    std::string              user;
    std::string              sender;
    std::vector<std::string> recipients;
    bool                     secure;
    bool                     eight_bit;
    bool                     utf8;
};

// 3.1.3
// server_handler
//   struct: the policy callbacks; each returns 0 to accept or a 4yz/5yz
// code to refuse. An empty function takes the kernel's default: accept,
// or, for `on_auth`, do not offer AUTH.
struct server_handler
{
    re_std::function<unsigned int(const session&,
                                  const std::string&)>   on_mail;
    re_std::function<unsigned int(const session&,
                                  const std::string&)>   on_rcpt;
    re_std::function<unsigned int(const session&,
                                  re_std::string_view)>  on_message;
    re_std::function<bool(const std::string&,
                          const std::string&)>           on_auth;
};

// 3.2    Server
//------------------------------------------------------------------------------
// 3.2.1
// server
//   class: a listener whose sessions each run on a thread of their own.
// Handlers may be called from several threads at once. Not copyable.
class server
{
public:
    server(const server_options& _options,
           const server_handler& _handler);
    ~server();

    /**
     * @brief Makes the server's TLS context, when its options ask for one,
     *        then binds and listens.
     *
     * @param[in] _host  address to bind; empty for every interface.
     * @param[in] _port  port; 0 for one the system chooses.
     * @return the bound port, or the failure: error::invalid_argument for
     *         a port past 65535, or implicit TLS with no certificate or
     *         context; error::state when already listening;
     *         error::unsupported without a TCP backend, or without an
     *         engine to make the context with; error::tls when the
     *         certificate or key will not load; error::resolve for a host
     *         that is not this machine's; or error::transport when the
     *         address cannot be bound.
     */
    D_NODISCARD port_result listen(const std::string& _host,
                                   unsigned int       _port);

    /**
     * @brief Accepts and serves connections until stop(), then waits for
     *        the sessions still running.
     *
     * @pre listen() succeeded.
     * @return nothing, or the failure that stopped the accept loop.
     */
    D_NODISCARD void_result run();

    // control and state -- stop() is safe from any thread, including a
    // handler; the queries are O(1)
    void                     stop() D_NOEXCEPT;
    D_NODISCARD unsigned int port() const D_NOEXCEPT;
    D_NODISCARD std::size_t  active_sessions() const D_NOEXCEPT;

private:
    D_DELETED_FN(server(const server&))
    D_DELETED_FN(server& operator=(const server&))

    // shared with the session threads, which each hold a count on it; the
    // last to let go deletes it
    internal::server_state* m_state;
};


NS_END  // smtp
NS_END  // net
NS_END  // djinterp


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "smtp.hpp: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64

#endif  // DJINTERP_NET_SMTP_SMTP_HPP
