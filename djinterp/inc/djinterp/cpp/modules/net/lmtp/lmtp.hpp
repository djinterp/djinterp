/*******************************************************************************
* djinterp [cpp][net]                                                   lmtp.hpp
*
* djinterp LMTP - C++ facade.
*   LMTP (RFC 2033) in C++: the SMTP face, net/smtp/smtp.hpp, held to LMTP's
* dialect, as the C module lmtp.h holds the SMTP kernel to it. A client
* greets with LHLO and never falls back to HELO; it is refused over TCP on
* port 25, which RFC 2033 forbids, and on no port at all, since LMTP has no
* standard one; and a send's report gives each recipient's own final reply,
* in `deliveries`. A server greets as LMTP, refuses EHLO and HELO, and
* answers the data once for each recipient, through the handler's
* on_deliver where there is one. Unix-domain sockets are LMTP's usual home:
* client_options' `local` makes `host` a socket's path, and a server's
* listen_local() listens on one.
*   The vocabulary - errors, statuses, replies, envelopes, reports, handlers
* and the results - is smtp.hpp's, named here as well, so the two faces'
* values are one type. The client and server hold an smtp::client and
* smtp::server privately, so no session of theirs can be opened in SMTP's
* dialect; every other member is the SMTP face's, unchanged. Options
* derive from SMTP's with LMTP's defaults: no port, so one must be given
* for TCP, and STARTTLS where the server offers it.
*   C++98 and later, as smtp.hpp is; from C++11 connect() and attach() also
* return a client, which only C++11 can move. Link lmtp.cpp with what
* smtp.hpp names; the C module lmtp.h is not needed, as the dialect is the
* SMTP kernel's.
*
*   Where D_INTERNAL_NET_HAS_INT64 is 0, with no 64-bit integer type, it
* compiles to nothing, unless D_CFG_NET_REQUIRE_BACKENDS asks for an #error
* (decision 101 of the register).
*
*
* path:      /inc/djinterp/cpp/modules/net/lmtp/lmtp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  smtp.hpp's names
2.  CLIENT
    ------
    1.  Options
         1.  client_options
         2.  attach_options
    2.  Client
         1.  client
         2.  send
3.  SERVER
    ------
    1.  Options
         1.  server_options
    2.  Server
         1.  server
*/

#ifndef DJINTERP_NET_LMTP_LMTP_HPP
#define DJINTERP_NET_LMTP_LMTP_HPP 1

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
// djinterp
#include "../../../djinterp.hpp"  // framework root
#include "../net_common.hpp"   // net::connection
#include "../smtp/smtp.hpp"    // the SMTP face, which this holds to LMTP
#if (D_MOVE_ENABLED == 1)
    // std
    #include <memory>  // std::unique_ptr, for attach
#endif  // D_MOVE_ENABLED

// 64-bit floor: smtp.hpp's, whose client this holds
#if (D_INTERNAL_NET_HAS_INT64 == 1)


NS_DJINTERP
NS_NET
D_NAMESPACE(lmtp)


//==============================================================================
// 1.  VOCABULARY
//==============================================================================
// LMTP's vocabulary is SMTP's: the same types, named here too.


// 1.1    smtp.hpp's names
//------------------------------------------------------------------------------
using smtp::error;
using smtp::security;
using smtp::mechanism;
using smtp::extension;
using smtp::dialect;
using smtp::status;
using smtp::capabilities;
using smtp::reply;
using smtp::failure;
using smtp::reply_result;
using smtp::report_result;
using smtp::port_result;
using smtp::void_result;
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
using smtp::result;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
using smtp::tls_options;
using smtp::credentials;
using smtp::envelope;
using smtp::recipient_result;
using smtp::send_report;
using smtp::session;
using smtp::server_handler;


//==============================================================================
// 2.  CLIENT
//==============================================================================


// 2.1    Options
//------------------------------------------------------------------------------
// 2.1.1
// client_options
//   struct: smtp::client_options with LMTP's defaults: LMTP's dialect, no
// port, and STARTTLS where the server offers it. A port must be given for
// TCP; for a unix-domain socket, `local` and its path in `host`.
struct client_options : public smtp::client_options
{
    client_options()
        : smtp::client_options()
    {
        dialect = smtp::dialect::lmtp;
        port    = 0u;
        mode    = smtp::security::starttls_optional;
    }
};

// 2.1.2
// attach_options
//   struct: smtp::attach_options in LMTP's dialect.
struct attach_options : public smtp::attach_options
{
    attach_options()
        : smtp::attach_options()
    {
        dialect = smtp::dialect::lmtp;
    }
};

// 2.2    Client
//------------------------------------------------------------------------------
// 2.2.1
// client
//   class: one LMTP client session: smtp::client, held to LMTP's dialect.
// Not copyable, movable from C++11; the session closes, without QUIT, when
// the client is destroyed.
class client : private smtp::client
{
public:
    // a closed client, for open() to open in place
    client() D_NOEXCEPT;
#if (D_MOVE_ENABLED == 1)
    client(
        client&& _other
    ) D_NOEXCEPT;
    client& operator=(
        client&& _other
    ) D_NOEXCEPT;
#endif  // D_MOVE_ENABLED

    /**
     * @brief Connects and opens an LMTP session - the greeting, LHLO, and
     *        the options' STARTTLS policy - closing any session this
     *        client held first. The options' dialect is LMTP's, whatever
     *        it was set to.
     *
     * @param[in] _options  where and how to connect.
     * @return nothing, or the failure with the server's reply:
     *         error::invalid_argument for TCP on port 0 or 25, or a server
     *         that refuses LHLO, error::rejected; the rest as
     *         smtp::client::open() gives them.
     */
    D_NODISCARD void_result open(
        const client_options& _options
    );

    /**
     * @brief Runs an LMTP session over a caller's connection: reads the
     *        greeting and says LHLO, closing any session this client held
     *        first.
     *
     * @param[in] _connection  the stream, which new allocated; the client
     *                         owns it from here, whatever the outcome, and
     *                         closes and deletes it.
     * @param[in] _options     whether the stream is encrypted, the identity
     *                         to announce, and whether AUTH may run without
     *                         TLS; the dialect is LMTP's.
     * @return nothing, or the failure.
     */
    D_NODISCARD void_result open(
        net::connection*      _connection,
        const attach_options& _options
    );

#if (D_MOVE_ENABLED == 1)
    /**
     * @brief open(), returning the opened client, which C++11 can move.
     *
     * @param[in] _options  where and how to connect.
     * @return the open session, or the failure with the server's reply.
     */
    D_NODISCARD static result<client> connect(
        const client_options& _options
    );

    /**
     * @brief open() over a caller's connection, returning the client.
     *
     * @param[in] _connection  the stream; owned and closed by the client.
     * @param[in] _options     as open() takes them.
     * @return the open session, or the failure.
     */
    D_NODISCARD static result<client> attach(
        std::unique_ptr<net::connection> _connection,
        const attach_options&            _options
    );
#endif  // D_MOVE_ENABLED

    // the SMTP face's members, unchanged: send()'s report gives each
    // recipient's final reply in `deliveries`, and succeeds when any
    // recipient was delivered to; error::rejected when none was, with the
    // last refusal; and after a connection closed partway through the
    // replies, last_report() says which came
    using smtp::client::is_open;
    using smtp::client::is_secure;
    using smtp::client::is_authenticated;
    using smtp::client::features;
    using smtp::client::last_reply;
    using smtp::client::last_report;
    using smtp::client::starttls;
    using smtp::client::authenticate;
    using smtp::client::send;
    using smtp::client::reset;
    using smtp::client::noop;
    using smtp::client::quit;
    using smtp::client::close;

private:
    D_DELETED_FN(client(const client&))
    D_DELETED_FN(client& operator=(const client&))
};

// 2.2.2
// send
/**
 * @brief Delivers one message over LMTP: connect, authenticate when a user
 *        is given, send, and quit.
 *
 * @param[in] _options      where and how to connect.
 * @param[in] _credentials  used when `user` is not empty.
 * @param[in] _envelope     sender and recipients.
 * @param[in] _message      the message, headers included.
 * @return the report, with each recipient's final reply, or the first
 *         failure.
 */
D_NODISCARD report_result send(
    const client_options& _options,
    const credentials&    _credentials,
    const envelope&       _envelope,
    re_std::string_view   _message
);


//==============================================================================
// 3.  SERVER
//==============================================================================


// 3.1    Options
//------------------------------------------------------------------------------
// 3.1.1
// server_options
//   struct: smtp::server_options in LMTP's dialect.
struct server_options : public smtp::server_options
{
    server_options()
        : smtp::server_options()
    {
        dialect = smtp::dialect::lmtp;
    }
};

// 3.2    Server
//------------------------------------------------------------------------------
// 3.2.1
// server
//   class: smtp::server, held to LMTP's dialect: a listener, over TCP or a
// unix-domain socket, whose sessions each run on a thread of their own.
// Each accepted recipient's final reply is the handler's on_deliver's, or
// on_message's for all of them where on_deliver is empty. Not copyable.
class server : private smtp::server
{
public:
    // the options' dialect is LMTP's, whatever it was set to
    server(
        const server_options& _options,
        const server_handler& _handler
    );

    // the SMTP face's members, unchanged
    using smtp::server::listen;
    using smtp::server::listen_local;
    using smtp::server::run;
    using smtp::server::stop;
    using smtp::server::port;
    using smtp::server::active_sessions;

private:
    D_DELETED_FN(server(const server&))
    D_DELETED_FN(server& operator=(const server&))
};


NS_END  // lmtp
NS_END  // net
NS_END  // djinterp


#elif (D_INTERNAL_NET_REQUIRE_BACKENDS == 1)
    #error "lmtp.hpp: no 64-bit integer type (D_INTERNAL_NET_HAS_INT64)"
#endif  // D_INTERNAL_NET_HAS_INT64

#endif  // DJINTERP_NET_LMTP_LMTP_HPP
