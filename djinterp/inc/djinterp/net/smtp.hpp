/*******************************************************************************
* djinterp [net]                                                        smtp.hpp
*
* djinterp SMTP -- C++ facade.
*   The C++ face of the SMTP kernel in c/net/smtp. It derives from the kernel
* rather than re-implementing it: statuses and capabilities are the kernel's
* own structs extended with C++ accessors, the enumerations carry the
* kernel's values, and the client and server hold the kernel's session
* state, lowering std::string arguments into it and lifting replies back out
* as owning values. What the facade adds is ownership (RAII over sockets and
* TLS), failure by value through re_std::expected, a server that runs each
* session on a thread of its own, and an adapter that runs a client session
* over any net::connection.
*   Nothing here throws on the SMTP path. Callbacks the C kernel invokes are
* guarded, so where exceptions are enabled one thrown by a handler becomes a
* 451 reply instead of crossing the C boundary.
*
*
* path:      /inc/djinterp/net/smtp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
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
         3.  result
2.  CLIENT
    ------
    1.  Options
         1.  tls_options
         2.  client_options
         3.  credentials
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

#ifndef DJINTERP_NET_SMTP_HPP
#define DJINTERP_NET_SMTP_HPP 1

// std
#include <cstddef>      // std::size_t
#include <functional>   // std::function
#include <memory>       // std::shared_ptr, std::unique_ptr
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <vector>       // std::vector
// djinterp
#include "../djinterp.hpp"                  // framework root
#include "../re_std/expected/expected.hpp"  // re_std::expected
#include "./net.hpp"                        // net::connection
// the C kernel comes last: in C++, GCC's <stdbool.h> defines _Bool as a
// macro, which breaks the `_Bool` template parameter of
// core/binary/decode.hpp (reached through net.hpp) if it is seen first
#include "../c/net/smtp/smtp.h"             // the C SMTP kernel


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
// value, so crossing between the two is a cast, never a table.


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// error
//   enum: d_smtp_error, scoped.
enum class error : int
{
    none        = D_SMTP_OK,
    invalid     = D_SMTP_ERROR_INVALID,
    state       = D_SMTP_ERROR_STATE,
    no_memory   = D_SMTP_ERROR_NO_MEMORY,
    resolve     = D_SMTP_ERROR_RESOLVE,
    connect     = D_SMTP_ERROR_CONNECT,
    timeout     = D_SMTP_ERROR_TIMEOUT,
    io          = D_SMTP_ERROR_IO,
    closed      = D_SMTP_ERROR_CLOSED,
    tls         = D_SMTP_ERROR_TLS,
    protocol    = D_SMTP_ERROR_PROTOCOL,
    too_long    = D_SMTP_ERROR_TOO_LONG,
    rejected    = D_SMTP_ERROR_REJECTED,
    unsupported = D_SMTP_ERROR_UNSUPPORTED,
    auth        = D_SMTP_ERROR_AUTH,
    insecure    = D_SMTP_ERROR_INSECURE
};

// 1.1.2
// security
//   enum: d_smtp_security, scoped; `starttls` fails closed.
enum class security : int
{
    none              = D_SMTP_SECURITY_NONE,
    starttls_optional = D_SMTP_SECURITY_STARTTLS_OPTIONAL,
    starttls          = D_SMTP_SECURITY_STARTTLS,
    tls               = D_SMTP_SECURITY_TLS
};

// 1.1.3
// mechanism
//   enum: d_smtp_auth, scoped; `automatic` picks the best password
// mechanism offered.
enum class mechanism : unsigned int
{
    automatic = D_SMTP_AUTH_NONE,
    plain     = D_SMTP_AUTH_PLAIN,
    login     = D_SMTP_AUTH_LOGIN,
    cram_md5  = D_SMTP_AUTH_CRAM_MD5,
    xoauth2   = D_SMTP_AUTH_XOAUTH2
};

// 1.1.4
// extension
//   enum: d_smtp_extension, scoped.
enum class extension : unsigned int
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

// to_string -- a short label for an error; O(1), never nullptr
[[nodiscard]] const char* to_string(error _error) noexcept;

// 1.2    Status and capabilities
//------------------------------------------------------------------------------
// 1.2.1
// status
//   struct: an RFC 3463 enhanced status -- the kernel's d_smtp_status.
struct status : public ::d_smtp_status
{
    status() noexcept;
    explicit status(const ::d_smtp_status& _status) noexcept;

    // present -- whether the reply carried a status; to_string -- "5.1.1",
    // or "" when absent
    [[nodiscard]] bool        present() const noexcept;
    [[nodiscard]] std::string to_string() const;
};

// 1.2.2
// capabilities
//   struct: what a server offered in EHLO -- the kernel's
// d_smtp_capabilities.
struct capabilities : public ::d_smtp_capabilities
{
    capabilities() noexcept;
    explicit capabilities(const ::d_smtp_capabilities& _capabilities) noexcept;

    // queries -- O(1), never fail
    [[nodiscard]] bool supports(extension _extension) const noexcept;
    [[nodiscard]] bool offers(mechanism _mechanism) const noexcept;
};

// 1.3    Replies and failures
//------------------------------------------------------------------------------
// 1.3.1
// reply
//   struct: an owning copy of a kernel reply, one string per line.
struct reply
{
    unsigned int             code      = 0u;
    smtp::status             status;
    std::vector<std::string> lines;
    bool                     truncated = false;

    reply() = default;
    explicit reply(const ::d_smtp_reply& _reply);

    // text -- the lines joined by '\n'; classification by first digit
    [[nodiscard]] std::string text() const;
    [[nodiscard]] bool        positive() const noexcept;
    [[nodiscard]] bool        transient() const noexcept;
    [[nodiscard]] bool        permanent() const noexcept;
};

// 1.3.2
// failure
//   struct: why an operation failed, and the server's reply when there was
// one (`server_reply.code` is 0 otherwise).
struct failure
{
    smtp::error code = smtp::error::none;
    smtp::reply server_reply;
};

// 1.3.3
// result
//   type: the value of an operation, or the failure that prevented it.
template<typename Type>
using result = ::re_std::expected<Type, failure>;


//==============================================================================
// 2.  CLIENT
//==============================================================================
// A client session in owning C++ form. connect() runs the kernel's whole
// opening -- greeting, EHLO and the STARTTLS policy -- and attach() runs a
// session over any net::connection (without STARTTLS, which needs the
// kernel's own transport).


// 2.1    Options
//------------------------------------------------------------------------------
// 2.1.1
// tls_options
//   struct: d_smtp_tls_options with owned strings; empty means "not set".
struct tls_options
{
    std::string server_name;       // client: defaults to the host
    std::string ca_file;           // client: empty for the system store
    std::string certificate_file;  // server
    std::string private_key_file;  // server
    bool        verify_peer = true;
};

// 2.1.2
// client_options
//   struct: d_smtp_client_options with owned strings and the same defaults.
struct client_options
{
    std::string    host;
    unsigned int   port                = 0u;
    smtp::security mode                = smtp::security::starttls;
    tls_options    tls;
    std::string    helo_domain;
    long           connect_timeout_ms  = D_SMTP_CLIENT_CONNECT_TIMEOUT_MS;
    long           io_timeout_ms       = D_SMTP_CLIENT_IO_TIMEOUT_MS;
    bool           allow_insecure_auth = false;
};

// 2.1.3
// credentials
//   struct: an identity, its secret, and the mechanism to use.
struct credentials
{
    std::string     user;
    std::string     secret;
    smtp::mechanism method = smtp::mechanism::automatic;
};

// 2.2    Transactions
//------------------------------------------------------------------------------
// 2.2.1
// envelope
//   struct: d_smtp_envelope with owned strings.
struct envelope
{
    std::string              sender;
    std::vector<std::string> recipients;
    bool                     eight_bit = false;
    bool                     utf8      = false;
};

// 2.2.2
// recipient_result
//   struct: what the server said to one RCPT.
struct recipient_result
{
    std::string  address;
    unsigned int code = 0u;
    smtp::status status;
};

// 2.2.3
// send_report
//   struct: what became of a transaction.
struct send_report
{
    std::vector<recipient_result> recipients;
    std::size_t                   accepted  = 0u;
    unsigned int                  data_code = 0u;
};

// 2.3    Client
//------------------------------------------------------------------------------
// 2.3.1
// client
//   class: one client session; movable, not copyable. The session closes,
// without QUIT, when the client is destroyed.
class client
{
public:
    client() noexcept;
    client(client&& _other) noexcept;
    client& operator=(client&& _other) noexcept;
    client(const client& _other)            = delete;
    client& operator=(const client& _other) = delete;
    ~client();

    /**
     * @brief Connects and opens a session per the options' security mode.
     *
     * @param[in] _options  where and how to connect.
     * @return the open session, or the failure with the server's reply.
     */
    [[nodiscard]] static result<client> connect(
                                            const client_options& _options);

    /**
     * @brief Runs a session over a caller's connection: reads the greeting
     *        and says EHLO.
     *
     * @param[in] _connection   the stream; owned and closed by the client.
     * @param[in] _secure       whether the stream is already encrypted.
     * @param[in] _helo_domain  the identity to announce.
     * @return the open session, or the failure.
     */
    [[nodiscard]] static result<client> attach(
                             std::unique_ptr<net::connection> _connection,
                             bool                             _secure,
                             const std::string&               _helo_domain);

    // session state -- O(1), never fail; a moved-from client is closed
    [[nodiscard]] bool               is_open() const noexcept;
    [[nodiscard]] bool               is_secure() const noexcept;
    [[nodiscard]] bool               is_authenticated() const noexcept;
    [[nodiscard]] smtp::capabilities features() const noexcept;
    [[nodiscard]] smtp::reply        last_reply() const;

    // commands -- each returns the server's reply, or the failure; errors
    // are those of the d_smtp_client_* function of the same name
    [[nodiscard]] result<smtp::reply> starttls(const tls_options& _options);
    [[nodiscard]] result<smtp::reply> authenticate(
                                          const credentials& _credentials);
    [[nodiscard]] result<send_report> send(const envelope&  _envelope,
                                           std::string_view _message);
    [[nodiscard]] result<smtp::reply> reset();
    [[nodiscard]] result<smtp::reply> noop();
    [[nodiscard]] result<smtp::reply> quit();

    // close -- releases the connection without QUIT; idempotent
    void close() noexcept;

private:
    explicit client(std::unique_ptr<internal::client_state> _state) noexcept;

    std::unique_ptr<internal::client_state> m_state;
};

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
[[nodiscard]] result<send_report> send(const client_options& _options,
                                       const credentials&    _credentials,
                                       const envelope&       _envelope,
                                       std::string_view      _message);


//==============================================================================
// 3.  SERVER
//==============================================================================
// The kernel's server engine behind a listener and a thread per session.


// 3.1    Options and handlers
//------------------------------------------------------------------------------
// 3.1.1
// server_options
//   struct: d_smtp_server_config with owned strings, plus implicit TLS and
// the I/O limit for accepted connections.
struct server_options
{
    std::string  domain              = "localhost";
    std::size_t  max_message_size    = D_SMTP_SERVER_MESSAGE_MAX;
    std::size_t  max_recipients      = D_SMTP_SERVER_RECIPIENTS_MAX;
    tls_options  tls;
    bool         implicit_tls        = false;
    unsigned int auth_mechanisms     = D_SMTP_AUTH_PLAIN | D_SMTP_AUTH_LOGIN;
    bool         require_tls         = false;
    bool         require_auth        = false;
    bool         allow_insecure_auth = false;
    long         io_timeout_ms       = D_SMTP_CLIENT_IO_TIMEOUT_MS;
};

// 3.1.2
// session
//   struct: d_smtp_session_info as owned values, for handlers.
struct session
{
    std::string              client_domain;
    std::string              user;
    std::string              sender;
    std::vector<std::string> recipients;
    bool                     secure    = false;
    bool                     eight_bit = false;
    bool                     utf8      = false;
};

// 3.1.3
// server_handler
//   struct: the policy callbacks; each returns 0 to accept or a 4yz/5yz
// code to refuse. An empty function takes the kernel's default: accept,
// or, for `on_auth`, do not offer AUTH.
struct server_handler
{
    std::function<unsigned int(const session&, const std::string&)> on_mail;
    std::function<unsigned int(const session&, const std::string&)> on_rcpt;
    std::function<unsigned int(const session&, std::string_view)>   on_message;
    std::function<bool(const std::string&, const std::string&)>     on_auth;
};

// 3.2    Server
//------------------------------------------------------------------------------
// 3.2.1
// server
//   class: a listener whose sessions each run on a thread of their own.
// Handlers may be called from several threads at once.
class server
{
public:
    server(server_options _options,
           server_handler _handler);
    server(const server& _other)            = delete;
    server& operator=(const server& _other) = delete;
    ~server();

    /**
     * @brief Binds and listens.
     *
     * @param[in] _host  address to bind; empty for every interface.
     * @param[in] _port  port; 0 for one the system chooses.
     * @return the bound port, or the failure.
     */
    [[nodiscard]] result<unsigned int> listen(const std::string& _host,
                                              unsigned int       _port);

    /**
     * @brief Accepts and serves connections until stop(), then waits for
     *        the sessions still running.
     *
     * @pre listen() succeeded.
     * @return nothing, or the failure that stopped the accept loop.
     */
    [[nodiscard]] result<void> run();

    // control and state -- stop() is safe from any thread, including a
    // handler; the queries are O(1)
    void                      stop() noexcept;
    [[nodiscard]] unsigned int port() const noexcept;
    [[nodiscard]] std::size_t  active_sessions() const noexcept;

private:
    std::shared_ptr<internal::server_state> m_state;
};


NS_END  // smtp
NS_END  // net
NS_END  // djinterp


#endif  // DJINTERP_NET_SMTP_HPP
