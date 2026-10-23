/*******************************************************************************
* djinterp [net]                                                   smtp_client.c
*
* djinterp SMTP client -- implementation.
*   The client state machine over a net.h connection, with TLS from the SSL
* kernel and connections from tcp.h. Command lines are assembled from
* validated parts into fixed buffers, so nothing a caller passes can smuggle
* CR or LF onto the wire; SASL buffers holding secrets are wiped through
* volatile stores once used.
*
*
* path:      /src/djinterp/net/smtp/smtp_client.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/net/smtp/smtp_client.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // snprintf
#include <string.h>   // memchr, memcpy, memset, strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/util/sink_common.h"      // d_pack_bytes, d_pack_text
#include "../../../../inc/djinterp/net/codec/codec_digest.h"  // d_net_hmac, d_net_wipe
#include "../../../../inc/djinterp/net/codec/codec_hex.h"     // d_net_hex_encode
#include "../../../../inc/djinterp/net/net_common.h"          // connections
#include "../../../../inc/djinterp/net/smtp/smtp_common.h"    // replies
#include "../../../../inc/djinterp/net/smtp/smtp_reader.h"    // d_smtp_reader
#include "../../../../inc/djinterp/net/ssl/ssl.h"             // TLS sessions
#include "../../../../inc/djinterp/net/tcp/tcp.h"             // connecting
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint8_t


// D_INTERNAL_SMTP_CHUNK
//   macro: bytes of encoded message data, or of pipelined commands, gathered
// per write.
#define D_INTERNAL_SMTP_CHUNK     8192u

// D_INTERNAL_SMTP_SASL_MAX
//   macro: the largest SASL message the client assembles, before base64.
#define D_INTERNAL_SMTP_SASL_MAX  2048u

// D_INTERNAL_SMTP_DATA_LINE
//   macro: the longest message line, without its CRLF (RFC 5321 4.5.3.1.6).
#define D_INTERNAL_SMTP_DATA_LINE 998u

// d_internal_smtp_pipeline
//   struct: what the replies to a pipelined batch said: whether MAIL was
// accepted, how many recipients were, and the refusal to keep for the
// caller.
struct d_internal_smtp_pipeline
{
    bool                mail_ok;
    size_t              accepted;
    struct d_smtp_reply cause;
};

/*
d_internal_smtp_judge
  What a failed read or write of the session's stream is: judged by the TLS
session beneath it, when this client made one and it failed, whose status
is then kept in `tls_status`.
*/
static enum d_smtp_error
d_internal_smtp_judge(
    struct d_smtp_client* _client,
    enum d_smtp_error     _result
)
{
    return d_smtp_error_from_tls((_client->has_tls) ? &_client->tls
                                                    : NULL,
                                 _result,
                                 &_client->tls_status,
                                 &_client->reader.net_error);
}

/*
d_internal_smtp_send
  Writes every byte through the stream the session reads, keeping what the
connection reported when it fails, as the reader does for reads.
*/
static enum d_smtp_error
d_internal_smtp_send(
    struct d_smtp_client* _client,
    const void*           _data,
    size_t                _length
)
{
    const enum d_net_error written =
        d_net_write_all(_client->reader.connection,
                        _data,
                        _length,
                        NULL);

    if (written == D_NET_OK)
    {
        return D_SMTP_OK;
    }

    _client->reader.net_error = written;

    return d_internal_smtp_judge(_client,
                                 d_smtp_error_from_net(written));
}

/*
d_internal_smtp_compose
  Appends every part, each a span, or none: on failure the buffer is rolled
back to its length on entry.
*/
static enum d_smtp_error
d_internal_smtp_compose(
    struct d_smtp_buffer*     _buffer,
    const struct d_pack_text* _parts,
    size_t                    _count
)
{
    const size_t saved = _buffer->length;

    for (size_t i = 0; i < _count; ++i)
    {
        const enum d_smtp_error result = d_smtp_buffer_append(_buffer,
                                                              _parts[i].data,
                                                              _parts[i].length);

        if (result != D_SMTP_OK)
        {
            _buffer->length      = saved;
            _buffer->data[saved] = '\0';

            return result;
        }
    }

    return D_SMTP_OK;
}

/*
d_internal_smtp_read_reply
  Reads one complete reply into the session. A line longer than the kernel's
line buffer is kept in truncated form rather than failing: servers may send
long greetings and EHLO lists, and the code and status always survive.
*/
static enum d_smtp_error
d_internal_smtp_read_reply(
    struct d_smtp_client* _client
)
{
    char line[D_SMTP_LINE_MAX] = { 0 };

    d_smtp_reply_clear(&_client->reply);

    // lines accumulate until the one without a hyphen
    while (!_client->reply.complete)
    {
        size_t                  length  = 0;
        const enum d_smtp_error outcome = d_smtp_reader_line(&_client->reader,
                                                             line,
                                                             sizeof(line),
                                                             &length);

        if ( (outcome != D_SMTP_OK) &&
             (outcome != D_SMTP_ERROR_LINE_TOO_LONG) )
        {
            return d_internal_smtp_judge(_client,
                                         outcome);
        }

        const enum d_smtp_error parsed = d_smtp_reply_feed(&_client->reply,
                                                           line,
                                                           length);

        if (parsed != D_SMTP_OK)
        {
            return parsed;
        }

        if (outcome == D_SMTP_ERROR_LINE_TOO_LONG)
        {
            _client->reply.truncated = true;
        }
    }

    return D_SMTP_OK;
}

/*
d_internal_smtp_exchange
  Sends one complete command line and reads its reply. The reply's class
(its first digit) must match `_class`; any other reply is a rejection, left
in the session for the caller.
*/
static enum d_smtp_error
d_internal_smtp_exchange(
    struct d_smtp_client* _client,
    const char*           _line,
    size_t                _length,
    unsigned int          _class
)
{
    const enum d_smtp_error sent = d_internal_smtp_send(_client,
                                                        _line,
                                                        _length);

    if (sent != D_SMTP_OK)
    {
        return sent;
    }

    const enum d_smtp_error received = d_internal_smtp_read_reply(_client);

    if (received != D_SMTP_OK)
    {
        return received;
    }

    return ((_client->reply.code / 100u) == _class) ? D_SMTP_OK
                                                    : D_SMTP_ERROR_REJECTED;
}

/*
d_internal_smtp_command
  d_internal_smtp_exchange() for a fixed command such as "RSET\r\n".
*/
static enum d_smtp_error
d_internal_smtp_command(
    struct d_smtp_client* _client,
    const char*           _line,
    unsigned int          _class
)
{
    return d_internal_smtp_exchange(_client,
                                    _line,
                                    strlen(_line),
                                    _class);
}

/*
d_smtp_client_options_init
  Every field to the documented default: STARTTLS, no TLS context,
the connect and I/O timeouts of smtp_client.h, AUTH in the clear refused.
*/
void
d_smtp_client_options_init(
    struct d_smtp_client_options* _options
)
{
    // parameter validation first
    if (!_options)
    {
        return;
    }

    _options->host                = NULL;
    _options->port                = 0;
    _options->security            = D_SMTP_SECURITY_STARTTLS;
    _options->tls                 = NULL;
    _options->server_name         = NULL;
    _options->helo_domain         = NULL;
    _options->connect_timeout_ms  = D_SMTP_CLIENT_CONNECT_TIMEOUT_MS;
    _options->io_timeout_ms       = D_SMTP_CLIENT_IO_TIMEOUT_MS;
    _options->allow_insecure_auth = false;

    return;
}

/*
d_smtp_client_init
  Zeroes the whole client, then sets what zero does not mean: the
reader with no connection, an unopened TCP connection, the closed state,
and D_SSL_OK.
*/
void
d_smtp_client_init(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return;
    }

    memset(_client,
           0,
           sizeof(*_client));
    d_smtp_reader_init(&_client->reader,
                       NULL);
    d_tcp_connection_init(&_client->tcp);
    _client->state      = D_SMTP_CLIENT_CLOSED;
    _client->tls_status = D_SSL_OK;

    return;
}

/*
d_internal_smtp_set_helo
  Validates and stores the identity announced in EHLO; NULL keeps the one
already stored, which must then exist.
*/
static enum d_smtp_error
d_internal_smtp_set_helo(
    struct d_smtp_client* _client,
    const char*           _domain
)
{
    if (!_domain)
    {
        return (_client->helo[0] != '\0') ? D_SMTP_OK
                                          : D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    const size_t length = strlen(_domain);

    if ( (length >= sizeof(_client->helo)) ||
         (!d_smtp_domain_is_valid(_domain,
                                  length,
                                  false)) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    memcpy(_client->helo,
           _domain,
           length + 1u);

    return D_SMTP_OK;
}

/*
d_smtp_client_attach
  d_smtp_client_attach_with() with the defaults and `_secure`.
*/
enum d_smtp_error
d_smtp_client_attach(
    struct d_smtp_client*    _client,
    struct d_net_connection* _connection,
    bool                     _secure
)
{
    struct d_smtp_client_attach_options options = { .secure = false };

    d_smtp_client_attach_options_init(&options);
    options.secure = _secure;

    return d_smtp_client_attach_with(_client,
                                     _connection,
                                     &options);
}

/*
d_smtp_client_attach_options_init
  The defaults are the refusals: nothing is taken as encrypted, and AUTH
waits for TLS.
*/
void
d_smtp_client_attach_options_init(
    struct d_smtp_client_attach_options* _options
)
{
    if (_options)
    {
        _options->secure              = false;
        _options->helo_domain         = NULL;
        _options->allow_insecure_auth = false;
    }

    return;
}

/*
d_smtp_client_attach_with
  The connection is borrowed: close leaves it to its owner. The identity is
checked and kept before anything else changes, so a malformed one leaves
the client as it was. What explained an earlier session's end -- its reply,
its causes -- is forgotten here, where a new one starts, and so is an
earlier session's leave to authenticate in the clear.
*/
enum d_smtp_error
d_smtp_client_attach_with(
    struct d_smtp_client*                      _client,
    struct d_net_connection*                   _connection,
    const struct d_smtp_client_attach_options* _options
)
{
    // parameter validation first
    if ( (!_client)  ||
         (!_options) ||
         (!d_net_connection_is_complete(_connection)) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->state != D_SMTP_CLIENT_CLOSED)
    {
        return D_SMTP_ERROR_STATE;
    }

    // an identity given now is the one hello() reuses
    if ( (_options->helo_domain) &&
         (d_internal_smtp_set_helo(_client,
                                   _options->helo_domain) != D_SMTP_OK) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    _client->connection = _connection;
    d_smtp_reader_init(&_client->reader,
                       _connection);
    d_smtp_capabilities_clear(&_client->capabilities);
    d_smtp_reply_clear(&_client->reply);

    _client->state               = D_SMTP_CLIENT_GREETING;
    _client->tls_status          = D_SSL_OK;
    _client->secure              = _options->secure;
    _client->allow_insecure_auth = _options->allow_insecure_auth;
    _client->authenticated       = false;
    _client->utf8                = false;

    return D_SMTP_OK;
}

/*
d_smtp_client_greeting
  Reads one reply. Only 220 opens a session; 554, or any other code,
is the server refusing it, D_SMTP_ERROR_REJECTED, and the client stays
where it was.
*/
enum d_smtp_error
d_smtp_client_greeting(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->state != D_SMTP_CLIENT_GREETING)
    {
        return D_SMTP_ERROR_STATE;
    }

    const enum d_smtp_error result = d_internal_smtp_read_reply(_client);

    if (result != D_SMTP_OK)
    {
        return result;
    }

    // 220 opens the session; 554 or anything else refuses it
    if (_client->reply.code != 220u)
    {
        return D_SMTP_ERROR_REJECTED;
    }

    _client->state = D_SMTP_CLIENT_HELLO;

    return D_SMTP_OK;
}

/*
d_internal_smtp_greet
  One EHLO or HELO exchange. EHLO's reply lists the extensions; HELO's
carries none, so the capabilities are cleared.
*/
static enum d_smtp_error
d_internal_smtp_greet(
    struct d_smtp_client* _client,
    bool                  _extended
)
{
    char                              storage[D_SMTP_COMMAND_LINE_MAX] = { 0 };
    struct d_smtp_buffer              line    = { .data     = storage,
                                                  .capacity = sizeof(storage),
                                                  .length   = 0 };
    const struct d_pack_text parts[] =
    {
        { (_extended) ? "EHLO " : "HELO ", 5 },
        { _client->helo,                   strlen(_client->helo) },
        { "\r\n",                          2 }
    };
    enum d_smtp_error result = d_internal_smtp_compose(&line,
                                                       parts,
                                                       3);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_exchange(_client,
                                          line.data,
                                          line.length,
                                          2u);
    }

    if (result != D_SMTP_OK)
    {
        return result;
    }

    d_smtp_capabilities_clear(&_client->capabilities);

    if (_extended)
    {
        result = d_smtp_capabilities_parse(&_client->capabilities,
                                           &_client->reply);
    }

    if (result == D_SMTP_OK)
    {
        _client->state = D_SMTP_CLIENT_READY;
    }

    return result;
}

/*
d_smtp_client_hello
  Keeps the identity first, then sends EHLO. A server that does not
know EHLO answers 500 or 502 (RFC 5321 section 3.2), and HELO is tried
once; any other refusal stands. A client already READY says hello again,
as after STARTTLS.
*/
enum d_smtp_error
d_smtp_client_hello(
    struct d_smtp_client* _client,
    const char*           _domain
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if ( (_client->state != D_SMTP_CLIENT_HELLO) &&
         (_client->state != D_SMTP_CLIENT_READY) )
    {
        return D_SMTP_ERROR_STATE;
    }

    const enum d_smtp_error named = d_internal_smtp_set_helo(_client,
                                                             _domain);

    if (named != D_SMTP_OK)
    {
        return named;
    }

    const enum d_smtp_error result = d_internal_smtp_greet(_client,
                                                           true);

    // a server that does not know EHLO answers 500 or 502 (RFC 5321 3.2)
    if ( (result == D_SMTP_ERROR_REJECTED) &&
         ( (_client->reply.code == 500u) ||
           (_client->reply.code == 502u) ) )
    {
        return d_internal_smtp_greet(_client,
                                     false);
    }

    return result;
}

/*
d_internal_smtp_tls_usable
  Whether `_tls` and `_server_name` can make a client's TLS session: a
context made, and made for a client, and a name the kernel can use -- one
there must be when the context checks names. The name is checked here, so
that a bad one is refused before anything is sent.
*/
static bool
d_internal_smtp_tls_usable(
    const struct d_ssl_context* _tls,
    const char*                 _server_name
)
{
    const struct d_pack_text name =
    {
        (_server_name) ? _server_name : "",
        (_server_name) ? strlen(_server_name) : 0u
    };

    // a context, for a client
    if ( (!_tls)         ||
         (!_tls->engine) ||
         (_tls->plan.role != D_SSL_ROLE_CLIENT) )
    {
        return false;
    }

    // no name, or one a TLS session can carry
    if (name.length == 0u)
    {
        return (!_tls->plan.verify_hostname);
    }

    return ( (d_ssl_host_is_ip(name)) ||
             (d_ssl_hostname_is_valid(name)) );
}

/*
d_internal_smtp_injected
  Whether plaintext arrived after the go-ahead: bytes the reader already
holds, or bytes waiting in a connection that can count them. Either would be
read as though TLS had protected it.
*/
static bool
d_internal_smtp_injected(
    const struct d_smtp_client* _client
)
{
    size_t waiting = 0;

    return ( (d_smtp_reader_pending(&_client->reader) > 0) ||
             ( (d_net_connection_available(_client->connection,
                                           &waiting)) &&
               (waiting > 0) ) );
}

/*
d_internal_smtp_unmade
  What a TLS session the kernel would not make is: a lack of memory, or
anything else the engine refused, since the arguments were checked first.
*/
static enum d_smtp_error
d_internal_smtp_unmade(
    enum d_ssl_error _status
)
{
    return (_status == D_SSL_ERROR_OUT_OF_MEMORY) ? D_SMTP_ERROR_OUT_OF_MEMORY
                                                  : D_SMTP_ERROR_TLS;
}

/*
d_internal_smtp_secure
  Makes TLS over the session's connection from `_tls`, a client naming
`_server_name`, and runs its handshake. On success the reader reads the
session's view. On failure the kernel's status is kept in `tls_status`, the
connection's error in the reader, and a session made is kept, for its
record of why, until close.
*/
static enum d_smtp_error
d_internal_smtp_secure(
    struct d_smtp_client*       _client,
    const struct d_ssl_context* _tls,
    const char*                 _server_name
)
{
    const char* const        name   = (_server_name) ? _server_name : "";
    const struct d_pack_text host   = { name,
                                        strlen(name) };
    enum d_ssl_error         status = d_ssl_session_init(&_client->tls,
                                                         _tls,
                                                         _client->connection,
                                                         host);

    // a session not made holds nothing to release
    if (status != D_SSL_OK)
    {
        _client->tls_status = status;

        return d_internal_smtp_unmade(status);
    }

    _client->has_tls = true;
    status           = d_ssl_session_handshake(&_client->tls);

    if (status != D_SSL_OK)
    {
        const enum d_net_error cause =
            d_ssl_session_transport_error(&_client->tls);

        _client->tls_status = status;

        // the connection's own error is the cause beneath TLS
        if (cause != D_NET_OK)
        {
            _client->reader.net_error = cause;
        }

        return d_smtp_error_from_ssl(status,
                                     cause);
    }

    d_smtp_reader_init(&_client->reader,
                       &_client->tls.base);
    _client->secure = true;

    return D_SMTP_OK;
}

/*
d_smtp_client_starttls
  Everything is checked before STARTTLS is sent. After the go-ahead the
server awaits a handshake, so a failure from there on leaves nothing to say
in plaintext, and the client is left to be closed.
*/
enum d_smtp_error
d_smtp_client_starttls(
    struct d_smtp_client*       _client,
    const struct d_ssl_context* _tls,
    const char*                 _server_name
)
{
    // parameter validation first
    if ( (!_client) ||
         (!d_internal_smtp_tls_usable(_tls,
                                      _server_name)) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if ( (_client->state != D_SMTP_CLIENT_READY) ||
         (_client->secure) )
    {
        return D_SMTP_ERROR_STATE;
    }

    if ((_client->capabilities.extensions & D_SMTP_EXTENSION_STARTTLS) == 0u)
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    enum d_smtp_error result = d_internal_smtp_command(_client,
                                                       "STARTTLS\r\n",
                                                       2u);

    if (result != D_SMTP_OK)
    {
        return result;
    }

    // plaintext after the 220 would be read as though protected: a
    // response-injection attempt
    result = (d_internal_smtp_injected(_client))
                 ? D_SMTP_ERROR_PROTOCOL
                 : d_internal_smtp_secure(_client,
                                          _tls,
                                          _server_name);

    if (result != D_SMTP_OK)
    {
        _client->state = D_SMTP_CLIENT_QUIT;

        return result;
    }

    // everything learnt before the handshake is discarded (RFC 3207 4.2)
    _client->state = D_SMTP_CLIENT_HELLO;
    d_smtp_capabilities_clear(&_client->capabilities);

    return d_smtp_client_hello(_client,
                               NULL);
}

/*
d_internal_smtp_auth_result
  A negative reply during AUTH is a refusal of the credentials.
*/
static enum d_smtp_error
d_internal_smtp_auth_result(
    enum d_smtp_error _result
)
{
    return (_result == D_SMTP_ERROR_REJECTED) ? D_SMTP_ERROR_AUTH : _result;
}

/*
d_internal_smtp_auth_respond
  Sends `_prefix` and the base64 of `_data` as one line and expects a reply
of `_class`. The line holds a secret, so it is wiped once sent.
*/
static enum d_smtp_error
d_internal_smtp_auth_respond(
    struct d_smtp_client* _client,
    const char*           _prefix,
    const void*           _data,
    size_t                _length,
    unsigned int          _class
)
{
    char                 storage[D_SMTP_LINE_MAX] = { 0 };
    struct d_smtp_buffer line = { .data     = storage,
                                  .capacity = sizeof(storage),
                                  .length   = 0 };
    enum d_smtp_error    result = d_smtp_buffer_append_text(&line,
                                                            _prefix);

    if (result == D_SMTP_OK)
    {
        result = d_smtp_buffer_append_base64(&line,
                                             _data,
                                             _length);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_buffer_append_text(&line,
                                           "\r\n");
    }

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_exchange(_client,
                                          line.data,
                                          line.length,
                                          _class);
    }

    d_net_wipe(storage,
               sizeof(storage));

    return result;
}

/*
d_internal_smtp_auth_plain
  RFC 4616: an empty authorization identity, the user and the password,
separated by NULs, sent as the AUTH command's initial response.
*/
static enum d_smtp_error
d_internal_smtp_auth_plain(
    struct d_smtp_client*            _client,
    const struct d_smtp_credentials* _credentials
)
{
    char                              raw[D_INTERNAL_SMTP_SASL_MAX] = { 0 };
    struct d_smtp_buffer              message = { .data     = raw,
                                                  .capacity = sizeof(raw),
                                                  .length   = 0 };
    const struct d_pack_text parts[] =
    {
        { "",                   1 },
        { _credentials->user,   strlen(_credentials->user) },
        { "",                   1 },
        { _credentials->secret, strlen(_credentials->secret) }
    };
    enum d_smtp_error result = d_internal_smtp_compose(&message,
                                                       parts,
                                                       4);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_auth_respond(_client,
                                              "AUTH PLAIN ",
                                              message.data,
                                              message.length,
                                              2u);
    }

    d_net_wipe(raw,
               sizeof(raw));

    return d_internal_smtp_auth_result(result);
}

/*
d_internal_smtp_auth_login
  LOGIN: two 334 challenges, answered with the base64 user, then password.
*/
static enum d_smtp_error
d_internal_smtp_auth_login(
    struct d_smtp_client*            _client,
    const struct d_smtp_credentials* _credentials
)
{
    enum d_smtp_error result = d_internal_smtp_command(_client,
                                                       "AUTH LOGIN\r\n",
                                                       3u);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_auth_respond(_client,
                                              "",
                                              _credentials->user,
                                              strlen(_credentials->user),
                                              3u);
    }

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_auth_respond(_client,
                                              "",
                                              _credentials->secret,
                                              strlen(_credentials->secret),
                                              2u);
    }

    return d_internal_smtp_auth_result(result);
}

/*
d_internal_smtp_cram_digest
  HMAC-MD5 of the challenge keyed with the secret, as 32 lower-case hex digits
(RFC 2195). The MAC is a secret's derivative until it is sent, so it is wiped
once rendered; d_net_hmac wipes its own copies of the key.
*/
static enum d_smtp_error
d_internal_smtp_cram_digest(
    const char* _secret,
    const char* _challenge,
    size_t      _length,
    char*       _hex
)
{
    const struct d_pack_bytes key       = { _secret,
                                            strlen(_secret) };
    const struct d_pack_bytes challenge = { _challenge,
                                            _length };
    uint8_t                   mac[D_NET_DIGEST_MD5_SIZE] = { 0 };
    const struct d_pack_bytes octets    = { mac,
                                            sizeof(mac) };

    // the MAC, then its digits; each refuses only an argument that is missing
    const bool done = ( (d_net_hmac(D_NET_DIGEST_MD5,
                                    key,
                                    challenge,
                                    mac))                        &&
                        (d_net_hex_encode(octets,
                                          D_NET_HEX_LOWER,
                                          _hex,
                                          (2u * sizeof(mac)) + 1u,
                                          NULL) == D_NET_CODEC_OK) );

    d_net_wipe(mac,
               sizeof(mac));

    return (done) ? D_SMTP_OK
                  : D_SMTP_ERROR_INVALID_ARGUMENT;
}

/*
d_internal_smtp_cram_answer
  Decodes the 334 challenge and assembles "user digest" into `_answer`.
*/
static enum d_smtp_error
d_internal_smtp_cram_answer(
    const struct d_smtp_client*      _client,
    const struct d_smtp_credentials* _credentials,
    struct d_smtp_buffer*            _answer
)
{
    char                 storage[D_SMTP_REPLY_TEXT_MAX] = { 0 };
    struct d_smtp_buffer challenge = { .data     = storage,
                                       .capacity = sizeof(storage),
                                       .length   = 0 };
    char                 digest[33] = { 0 };
    size_t               length     = 0;
    const char*          text       = d_smtp_reply_line(&_client->reply,
                                                        0,
                                                        &length);
    enum d_smtp_error    result     = (text)
                                      ? d_smtp_buffer_append_from_base64(
                                            &challenge,
                                            text,
                                            length)
                                      : D_SMTP_ERROR_PROTOCOL;

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_cram_digest(_credentials->secret,
                                             challenge.data,
                                             challenge.length,
                                             digest);
    }

    if (result == D_SMTP_OK)
    {
        const struct d_pack_text parts[] =
        {
            { _credentials->user, strlen(_credentials->user) },
            { " ",                1 },
            { digest,             32 }
        };

        result = d_internal_smtp_compose(_answer,
                                         parts,
                                         3);
    }

    d_net_wipe(digest,
               sizeof(digest));

    return result;
}

/*
d_internal_smtp_auth_cram
  CRAM-MD5: the challenge is answered with the user and an HMAC-MD5 digest.
An answer that cannot be computed cancels the exchange with "*" (RFC 4954
section 4) so the session stays usable.
*/
static enum d_smtp_error
d_internal_smtp_auth_cram(
    struct d_smtp_client*            _client,
    const struct d_smtp_credentials* _credentials
)
{
    char                 storage[D_INTERNAL_SMTP_SASL_MAX] = { 0 };
    struct d_smtp_buffer answer = { .data     = storage,
                                    .capacity = sizeof(storage),
                                    .length   = 0 };
    enum d_smtp_error    result = d_internal_smtp_command(_client,
                                                          "AUTH CRAM-MD5\r\n",
                                                          3u);

    if (result != D_SMTP_OK)
    {
        return d_internal_smtp_auth_result(result);
    }

    result = d_internal_smtp_cram_answer(_client,
                                         _credentials,
                                         &answer);

    if (result != D_SMTP_OK)
    {
        (void)d_internal_smtp_command(_client,
                                      "*\r\n",
                                      5u);

        return result;
    }

    result = d_internal_smtp_auth_respond(_client,
                                          "",
                                          answer.data,
                                          answer.length,
                                          2u);
    d_net_wipe(storage,
               sizeof(storage));

    return d_internal_smtp_auth_result(result);
}

/*
d_internal_smtp_auth_xoauth2
  XOAUTH2: "user=U^Aauth=Bearer T^A^A" as an initial response. A refused
token arrives as a 334 carrying an error report, which an empty line answers
to draw the final 5yz.
*/
static enum d_smtp_error
d_internal_smtp_auth_xoauth2(
    struct d_smtp_client*            _client,
    const struct d_smtp_credentials* _credentials
)
{
    const size_t user_length   = strlen(_credentials->user);
    const size_t secret_length = strlen(_credentials->secret);

    // ^A separates the fields, so neither value may contain one
    if ( (memchr(_credentials->user,
                 '\x01',
                 user_length)) ||
         (memchr(_credentials->secret,
                 '\x01',
                 secret_length)) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    char                              raw[D_INTERNAL_SMTP_SASL_MAX] = { 0 };
    struct d_smtp_buffer              message = { .data     = raw,
                                                  .capacity = sizeof(raw),
                                                  .length   = 0 };
    const struct d_pack_text parts[] =
    {
        { "user=",                5 },
        { _credentials->user,     user_length },
        { "\x01" "auth=Bearer ", 13 },
        { _credentials->secret,   secret_length },
        { "\x01\x01",             2 }
    };
    enum d_smtp_error result = d_internal_smtp_compose(&message,
                                                       parts,
                                                       5);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_auth_respond(_client,
                                              "AUTH XOAUTH2 ",
                                              message.data,
                                              message.length,
                                              2u);
    }

    if ( (result == D_SMTP_ERROR_REJECTED) &&
         (_client->reply.code == 334u) )
    {
        result = d_internal_smtp_command(_client,
                                         "\r\n",
                                         2u);
    }

    d_net_wipe(raw,
               sizeof(raw));

    return d_internal_smtp_auth_result(result);
}

/*
d_internal_smtp_choose
  The mechanism to use: the most preferred one that is acceptable and
offered. XOAUTH2 ranks first but is acceptable only on request.
*/
static unsigned int
d_internal_smtp_choose(
    const struct d_smtp_capabilities* _capabilities,
    unsigned int                      _requested
)
{
    static const unsigned int PREFERENCE[] =
    {
        D_SMTP_AUTH_XOAUTH2,
        D_SMTP_AUTH_PLAIN,
        D_SMTP_AUTH_LOGIN,
        D_SMTP_AUTH_CRAM_MD5
    };
    const unsigned int passwords  = D_SMTP_AUTH_PLAIN |
                                    D_SMTP_AUTH_LOGIN |
                                    D_SMTP_AUTH_CRAM_MD5;
    const unsigned int acceptable = (_requested != D_SMTP_AUTH_NONE)
                                    ? _requested
                                    : passwords;
    const unsigned int usable     = acceptable &
                                    _capabilities->auth;

    for (size_t i = 0; i < sizeof(PREFERENCE) / sizeof(PREFERENCE[0]); ++i)
    {
        if ((usable & PREFERENCE[i]) != 0u)
        {
            return PREFERENCE[i];
        }
    }

    return D_SMTP_AUTH_NONE;
}

/*
d_smtp_client_authenticate
  Refused before anything is sent on a plain channel unless the
caller allowed it. The mechanism is the most preferred one the server
offers and the credentials accept, XOAUTH2 only on request
(d_internal_smtp_choose); with no AUTH extension, or none in common,
D_SMTP_ERROR_UNSUPPORTED. Each mechanism has its own exchange.
*/
enum d_smtp_error
d_smtp_client_authenticate(
    struct d_smtp_client*            _client,
    const struct d_smtp_credentials* _credentials
)
{
    // parameter validation first
    if ( (!_client)               ||
         (!_credentials)          ||
         (!_credentials->user)    ||
         (!_credentials->secret) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if ( (_client->state != D_SMTP_CLIENT_READY) ||
         (_client->authenticated) )
    {
        return D_SMTP_ERROR_STATE;
    }

    // credentials never cross a plaintext channel unless the caller insists
    if ( (!_client->secure) &&
         (!_client->allow_insecure_auth) )
    {
        return D_SMTP_ERROR_INSECURE;
    }

    const unsigned int mechanism = ((_client->capabilities.extensions &
                                     D_SMTP_EXTENSION_AUTH) != 0u)
                                   ? d_internal_smtp_choose(
                                         &_client->capabilities,
                                         _credentials->mechanism)
                                   : D_SMTP_AUTH_NONE;
    enum d_smtp_error  result    = D_SMTP_ERROR_UNSUPPORTED;

    switch (mechanism)
    {
        case D_SMTP_AUTH_PLAIN:
            result = d_internal_smtp_auth_plain(_client,
                                                _credentials);
            break;

        case D_SMTP_AUTH_LOGIN:
            result = d_internal_smtp_auth_login(_client,
                                                _credentials);
            break;

        case D_SMTP_AUTH_CRAM_MD5:
            result = d_internal_smtp_auth_cram(_client,
                                               _credentials);
            break;

        case D_SMTP_AUTH_XOAUTH2:
            result = d_internal_smtp_auth_xoauth2(_client,
                                                  _credentials);
            break;

        default:
            break;
    }

    _client->authenticated = (result == D_SMTP_OK);

    return result;
}

/*
d_internal_smtp_mail_parameters
  What follows MAIL's path, and the line's end: SIZE when the server offered
it, then BODY=8BITMIME and SMTPUTF8 when the envelope needs them.
*/
static enum d_smtp_error
d_internal_smtp_mail_parameters(
    const struct d_smtp_envelope* _envelope,
    bool                          _sized,
    size_t                        _size,
    struct d_smtp_buffer*         _line
)
{
    enum d_smtp_error result = D_SMTP_OK;

    // SIZE lets the server refuse before the data is sent (RFC 1870)
    if ( (_sized) &&
         (_size > 0) )
    {
        result = d_smtp_buffer_append_text(_line,
                                           " SIZE=");

        if (result == D_SMTP_OK)
        {
            result = d_smtp_buffer_append_number(_line,
                                                 _size);
        }
    }

    if ( (result == D_SMTP_OK) &&
         (_envelope->eight_bit) )
    {
        result = d_smtp_buffer_append_text(_line,
                                           " BODY=8BITMIME");
    }

    if ( (result == D_SMTP_OK) &&
         (_envelope->utf8) )
    {
        result = d_smtp_buffer_append_text(_line,
                                           " SMTPUTF8");
    }

    return (result == D_SMTP_OK) ? d_smtp_buffer_append_text(_line,
                                                             "\r\n")
                                 : result;
}

/*
d_internal_smtp_build_mail
  "MAIL FROM:<sender>" and its parameters. Extensions the envelope requires
but the server lacks cannot be dropped silently, and a message above the
server's declared SIZE is refused before anything is sent.
*/
static enum d_smtp_error
d_internal_smtp_build_mail(
    const struct d_smtp_client*   _client,
    const struct d_smtp_envelope* _envelope,
    size_t                        _size,
    struct d_smtp_buffer*         _line
)
{
    const char*        sender  = (_envelope->sender) ? _envelope->sender : "";
    const size_t       length  = strlen(sender);
    const unsigned int offered = _client->capabilities.extensions;

    // the null reverse-path "<>" is legal; anything else must be a mailbox
    if ( (length > 0) &&
         (!d_smtp_address_is_valid(sender,
                                   length,
                                   _envelope->utf8)) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if ( ( (_envelope->eight_bit) &&
           ((offered & D_SMTP_EXTENSION_8BITMIME) == 0u) ) ||
         ( (_envelope->utf8) &&
           ((offered & D_SMTP_EXTENSION_SMTPUTF8) == 0u) ) )
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    const bool sized = ((offered & D_SMTP_EXTENSION_SIZE) != 0u);

    if ( (sized)                                   &&
         (_client->capabilities.size_limit > 0)    &&
         (_size > _client->capabilities.size_limit) )
    {
        return D_SMTP_ERROR_LINE_TOO_LONG;
    }

    const struct d_pack_text parts[] =
    {
        { "MAIL FROM:<", 11 },
        { sender,        length },
        { ">",           1 }
    };
    const enum d_smtp_error result = d_internal_smtp_compose(_line,
                                                             parts,
                                                             3);

    return (result == D_SMTP_OK) ? d_internal_smtp_mail_parameters(_envelope,
                                                                   sized,
                                                                   _size,
                                                                   _line)
                                 : result;
}

/*
d_internal_smtp_build_rcpt
  "RCPT TO:<recipient>", the recipient validated first.
*/
static enum d_smtp_error
d_internal_smtp_build_rcpt(
    const char*           _recipient,
    bool                  _utf8,
    struct d_smtp_buffer* _line
)
{
    const size_t length = (_recipient) ? strlen(_recipient) : 0;

    if (!d_smtp_address_is_valid(_recipient,
                                 length,
                                 _utf8))
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    const struct d_pack_text parts[] =
    {
        { "RCPT TO:<", 9 },
        { _recipient,  length },
        { ">\r\n",     3 }
    };

    return d_internal_smtp_compose(_line,
                                   parts,
                                   3);
}

/*
d_smtp_client_mail
  Builds the MAIL FROM line, with its parameters, in a buffer of the
longest line the protocol allows, and sends it, expecting 2yz. The
envelope's SMTPUTF8 choice holds for the transaction.
*/
enum d_smtp_error
d_smtp_client_mail(
    struct d_smtp_client*         _client,
    const struct d_smtp_envelope* _envelope,
    size_t                        _size
)
{
    // parameter validation first
    if ( (!_client) ||
         (!_envelope) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->state != D_SMTP_CLIENT_READY)
    {
        return D_SMTP_ERROR_STATE;
    }

    char                    storage[D_SMTP_LINE_MAX] = { 0 };
    struct d_smtp_buffer    line  = { .data     = storage,
                                      .capacity = sizeof(storage),
                                      .length   = 0 };
    const enum d_smtp_error built = d_internal_smtp_build_mail(_client,
                                                               _envelope,
                                                               _size,
                                                               &line);

    if (built != D_SMTP_OK)
    {
        return built;
    }

    const enum d_smtp_error result = d_internal_smtp_exchange(_client,
                                                              line.data,
                                                              line.length,
                                                              2u);

    if (result == D_SMTP_OK)
    {
        _client->state = D_SMTP_CLIENT_MAIL;
        _client->utf8  = _envelope->utf8;
    }

    return result;
}

/*
d_smtp_client_rcpt
  One RCPT TO line, built as MAIL's is, expecting 2yz; the
transaction's SMTPUTF8 choice decides whether the address may carry it.
*/
enum d_smtp_error
d_smtp_client_rcpt(
    struct d_smtp_client* _client,
    const char*           _recipient
)
{
    // parameter validation first
    if ( (!_client) ||
         (!_recipient) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->state != D_SMTP_CLIENT_MAIL)
    {
        return D_SMTP_ERROR_STATE;
    }

    char                    storage[D_SMTP_LINE_MAX] = { 0 };
    struct d_smtp_buffer    line  = { .data     = storage,
                                      .capacity = sizeof(storage),
                                      .length   = 0 };
    const enum d_smtp_error built = d_internal_smtp_build_rcpt(_recipient,
                                                               _client->utf8,
                                                               &line);

    if (built != D_SMTP_OK)
    {
        return built;
    }

    return d_internal_smtp_exchange(_client,
                                    line.data,
                                    line.length,
                                    2u);
}

/*
d_internal_smtp_check_lines
  Rejects a message with a line longer than RFC 5321 allows; checked before
DATA, since a server may truncate or refuse such a line mid-message.
*/
static enum d_smtp_error
d_internal_smtp_check_lines(
    const char* _message,
    size_t      _length
)
{
    size_t run = 0;

    for (size_t i = 0; i < _length; ++i)
    {
        const bool ending = ( (_message[i] == '\r') ||
                              (_message[i] == '\n') );

        run = (ending) ? 0 : run + 1;

        if (run > D_INTERNAL_SMTP_DATA_LINE)
        {
            return D_SMTP_ERROR_LINE_TOO_LONG;
        }
    }

    return D_SMTP_OK;
}

/*
d_internal_smtp_body
  Streams the encoded message after a 354 in chunks, then the end-of-data
marker, and reads the final reply. The transaction is over either way.
*/
static enum d_smtp_error
d_internal_smtp_body(
    struct d_smtp_client* _client,
    const char*           _message,
    size_t                _length
)
{
    char                       storage[D_INTERNAL_SMTP_CHUNK] = { 0 };
    struct d_smtp_data_encoder encoder = { .line_start = true,
                                           .after_cr   = false };
    size_t                     offset  = 0;
    enum d_smtp_error          result  = D_SMTP_OK;

    while ( (result == D_SMTP_OK) &&
            (offset < _length) )
    {
        struct d_smtp_buffer chunk = { .data     = storage,
                                       .capacity = sizeof(storage),
                                       .length   = 0 };

        offset += d_smtp_data_encode(&encoder,
                                     _message + offset,
                                     _length - offset,
                                     &chunk);
        result  = d_internal_smtp_send(_client,
                                       chunk.data,
                                       chunk.length);
    }

    struct d_smtp_buffer tail = { .data     = storage,
                                  .capacity = sizeof(storage),
                                  .length   = 0 };

    if (result == D_SMTP_OK)
    {
        result = d_smtp_data_finish(&encoder,
                                    &tail);
    }

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_exchange(_client,
                                          tail.data,
                                          tail.length,
                                          2u);
    }

    // a final reply of either kind ends the transaction
    if ( (result == D_SMTP_OK) ||
         (result == D_SMTP_ERROR_REJECTED) )
    {
        _client->state = D_SMTP_CLIENT_READY;
        _client->utf8  = false;
    }

    return result;
}

/*
d_smtp_client_data
  Every line of the message is checked against RFC 5321's limit before
DATA goes out, since a server may truncate or refuse a long line partway
through; then DATA, expecting 3yz, and the dot-stuffed body.
*/
enum d_smtp_error
d_smtp_client_data(
    struct d_smtp_client* _client,
    const char*           _message,
    size_t                _length
)
{
    // parameter validation first
    if ( (!_client) ||
         ( (!_message) &&
           (_length > 0) ) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->state != D_SMTP_CLIENT_MAIL)
    {
        return D_SMTP_ERROR_STATE;
    }

    const enum d_smtp_error checked = d_internal_smtp_check_lines(_message,
                                                                  _length);

    if (checked != D_SMTP_OK)
    {
        return checked;
    }

    const enum d_smtp_error opened = d_internal_smtp_command(_client,
                                                             "DATA\r\n",
                                                             3u);

    if (opened != D_SMTP_OK)
    {
        return opened;
    }

    return d_internal_smtp_body(_client,
                                (_message) ? _message : "",
                                _length);
}

/*
d_internal_smtp_record
  Stores one RCPT outcome in the caller's report, if there is one.
*/
static void
d_internal_smtp_record(
    struct d_smtp_send_report* _report,
    size_t                     _index,
    const struct d_smtp_reply* _reply
)
{
    if (!_report)
    {
        return;
    }

    if (_report->recipients)
    {
        _report->recipients[_index].code   = _reply->code;
        _report->recipients[_index].status = _reply->status;
    }

    if (d_smtp_code_is_positive(_reply->code))
    {
        _report->accepted += 1;
    }

    return;
}

/*
d_internal_smtp_abandon
  Ends a refused transaction with RSET, then restores the reply that refused
it, which is what the caller needs to see.
*/
static enum d_smtp_error
d_internal_smtp_abandon(
    struct d_smtp_client*      _client,
    const struct d_smtp_reply* _failure
)
{
    _client->state = D_SMTP_CLIENT_MAIL;

    const enum d_smtp_error reset = d_smtp_client_reset(_client);

    _client->reply = *_failure;

    return ( (reset == D_SMTP_OK) ||
             (reset == D_SMTP_ERROR_REJECTED) ) ? D_SMTP_ERROR_REJECTED
                                                : reset;
}

/*
d_internal_smtp_open_serial
  MAIL, each RCPT, then DATA, one exchange at a time.
*/
static enum d_smtp_error
d_internal_smtp_open_serial(
    struct d_smtp_client*         _client,
    const struct d_smtp_envelope* _envelope,
    size_t                        _size,
    struct d_smtp_send_report*    _report
)
{
    struct d_smtp_reply failure  = { .code = 0 };
    size_t              accepted = 0;
    enum d_smtp_error   result   = d_smtp_client_mail(_client,
                                                      _envelope,
                                                      _size);

    if (result != D_SMTP_OK)
    {
        return result;
    }

    for (size_t i = 0; i < _envelope->recipient_count; ++i)
    {
        result = d_smtp_client_rcpt(_client,
                                    _envelope->recipients[i]);

        // a refused recipient is recorded; a broken session ends the send
        if ( (result != D_SMTP_OK) &&
             (result != D_SMTP_ERROR_REJECTED) )
        {
            return result;
        }

        d_internal_smtp_record(_report,
                               i,
                               &_client->reply);
        accepted += (result == D_SMTP_OK) ? 1u : 0u;

        if (result == D_SMTP_ERROR_REJECTED)
        {
            failure = _client->reply;
        }
    }

    if (accepted == 0)
    {
        return d_internal_smtp_abandon(_client,
                                       &failure);
    }

    result = d_internal_smtp_command(_client,
                                     "DATA\r\n",
                                     3u);

    return (result == D_SMTP_ERROR_REJECTED)
           ? d_internal_smtp_abandon(_client,
                                     &_client->reply)
           : result;
}

/*
d_internal_smtp_queue
  Adds a complete command to a pipelining batch, flushing the batch first
when the command would not fit.
*/
static enum d_smtp_error
d_internal_smtp_queue(
    struct d_smtp_client*       _client,
    struct d_smtp_buffer*       _batch,
    const struct d_smtp_buffer* _line
)
{
    if (_line->length >= _batch->capacity - _batch->length)
    {
        const enum d_smtp_error flushed = d_internal_smtp_send(
                                              _client,
                                              _batch->data,
                                              _batch->length);

        if (flushed != D_SMTP_OK)
        {
            return flushed;
        }

        _batch->length = 0;
    }

    return d_smtp_buffer_append(_batch,
                                _line->data,
                                _line->length);
}

/*
d_internal_smtp_send_batch
  Writes MAIL, every RCPT and DATA as one pipelined group (RFC 2920 3.1).
*/
static enum d_smtp_error
d_internal_smtp_send_batch(
    struct d_smtp_client*         _client,
    const struct d_smtp_envelope* _envelope,
    size_t                        _size
)
{
    char                 batch_storage[D_INTERNAL_SMTP_CHUNK] = { 0 };
    char                 line_storage[D_SMTP_LINE_MAX]        = { 0 };
    struct d_smtp_buffer batch  = { .data     = batch_storage,
                                    .capacity = sizeof(batch_storage),
                                    .length   = 0 };
    struct d_smtp_buffer line   = { .data     = line_storage,
                                    .capacity = sizeof(line_storage),
                                    .length   = 0 };
    enum d_smtp_error    result = d_internal_smtp_build_mail(_client,
                                                             _envelope,
                                                             _size,
                                                             &line);

    for (size_t i = 0; i <= _envelope->recipient_count; ++i)
    {
        if (result == D_SMTP_OK)
        {
            result = d_internal_smtp_queue(_client,
                                           &batch,
                                           &line);
        }

        // the next command: a recipient, or DATA after the last one
        line.length = 0;

        if (result == D_SMTP_OK)
        {
            result = (i < _envelope->recipient_count)
                     ? d_internal_smtp_build_rcpt(_envelope->recipients[i],
                                                  _envelope->utf8,
                                                  &line)
                     : d_smtp_buffer_append_text(&line,
                                                 "DATA\r\n");
        }
    }

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_queue(_client,
                                       &batch,
                                       &line);
    }

    return (result == D_SMTP_OK)
           ? d_internal_smtp_send(_client,
                                  batch.data,
                                  batch.length)
           : result;
}

/*
d_internal_smtp_read_pipelined
  Reads every reply to a pipelined batch, in order, as RFC 2920 requires even
after a refusal: MAIL's, then one per recipient, each recorded in the
report, then DATA's, which stays in the client's reply. The refusal kept
for the caller is MAIL's, else the last recipient's.
*/
static enum d_smtp_error
d_internal_smtp_read_pipelined(
    struct d_smtp_client*            _client,
    size_t                           _count,
    struct d_smtp_send_report*       _report,
    struct d_internal_smtp_pipeline* _tally
)
{
    // MAIL's reply, one per recipient, then DATA's: all of them are read
    for (size_t i = 0; i < _count + 2u; ++i)
    {
        const enum d_smtp_error result = d_internal_smtp_read_reply(_client);

        if (result != D_SMTP_OK)
        {
            return result;
        }

        const bool positive = d_smtp_code_is_positive(_client->reply.code);

        // the refusal kept for the caller: MAIL's, else the last recipient's
        if (i == 0)
        {
            _tally->mail_ok = positive;
            _tally->cause   = _client->reply;
        }
        else if (i <= _count)
        {
            d_internal_smtp_record(_report,
                                   i - 1u,
                                   &_client->reply);
            _tally->accepted += (positive) ? 1u : 0u;

            if ( (_tally->mail_ok) &&
                 (!positive) )
            {
                _tally->cause = _client->reply;
            }
        }
    }

    return D_SMTP_OK;
}

/*
d_internal_smtp_open_pipelined
  Sends the batch, then reads every reply in order, as RFC 2920 requires
even after a refusal. A 354 that should not have come (no recipient
accepted) is answered with an empty message so the session stays in step.
*/
static enum d_smtp_error
d_internal_smtp_open_pipelined(
    struct d_smtp_client*         _client,
    const struct d_smtp_envelope* _envelope,
    size_t                        _size,
    struct d_smtp_send_report*    _report
)
{
    struct d_internal_smtp_pipeline tally  = { .mail_ok = false };
    enum d_smtp_error               result = d_internal_smtp_send_batch(
                                                 _client,
                                                 _envelope,
                                                 _size);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_read_pipelined(_client,
                                                _envelope->recipient_count,
                                                _report,
                                                &tally);
    }

    if (result != D_SMTP_OK)
    {
        return result;
    }

    const bool data_ok = (_client->reply.code == 354u);

    if ( (tally.mail_ok)      &&
         (tally.accepted > 0) &&
         (data_ok) )
    {
        _client->state = D_SMTP_CLIENT_MAIL;
        _client->utf8  = _envelope->utf8;

        return D_SMTP_OK;
    }

    // with MAIL and a recipient accepted, DATA itself was refused
    if ( (tally.mail_ok) &&
         (tally.accepted > 0) )
    {
        tally.cause = _client->reply;
    }

    // a 354 must be answered before anything else can be said
    if (data_ok)
    {
        result = d_internal_smtp_command(_client,
                                         ".\r\n",
                                         2u);

        if ( (result != D_SMTP_OK) &&
             (result != D_SMTP_ERROR_REJECTED) )
        {
            return result;
        }
    }

    return d_internal_smtp_abandon(_client,
                                   &tally.cause);
}

/*
d_internal_smtp_check_envelope
  Validates the whole envelope before a byte is sent, so a pipelined batch
never has to be abandoned half-written because of a caller's mistake.
*/
static enum d_smtp_error
d_internal_smtp_check_envelope(
    const struct d_smtp_client*   _client,
    const struct d_smtp_envelope* _envelope,
    size_t                        _size
)
{
    char                 storage[D_SMTP_LINE_MAX] = { 0 };
    struct d_smtp_buffer line   = { .data     = storage,
                                    .capacity = sizeof(storage),
                                    .length   = 0 };
    enum d_smtp_error    result = d_internal_smtp_build_mail(_client,
                                                             _envelope,
                                                             _size,
                                                             &line);

    for (size_t i = 0;
         ( (result == D_SMTP_OK) &&
           (i < _envelope->recipient_count) );
         ++i)
    {
        line.length = 0;
        result      = d_internal_smtp_build_rcpt(_envelope->recipients[i],
                                                 _envelope->utf8,
                                                 &line);
    }

    return result;
}

/*
d_smtp_client_send
  Checks the envelope and the message's lines before anything is sent.
With PIPELINING, MAIL, every RCPT and DATA go out together and their
replies are read in order; without it, one command at a time. The body
follows, and the report records the accepted recipients and DATA's final
code.
*/
enum d_smtp_error
d_smtp_client_send(
    struct d_smtp_client*         _client,
    const struct d_smtp_envelope* _envelope,
    const char*                   _message,
    size_t                        _length,
    struct d_smtp_send_report*    _report
)
{
    // parameter validation first
    if ( (!_client)                          ||
         (!_envelope)                        ||
         (!_envelope->recipients)            ||
         (_envelope->recipient_count == 0)   ||
         ( (!_message) &&
           (_length > 0) ) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->state != D_SMTP_CLIENT_READY)
    {
        return D_SMTP_ERROR_STATE;
    }

    if (_report)
    {
        _report->accepted  = 0;
        _report->data_code = 0;
    }

    enum d_smtp_error result = d_internal_smtp_check_envelope(_client,
                                                              _envelope,
                                                              _length);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_check_lines(_message,
                                             _length);
    }

    if (result != D_SMTP_OK)
    {
        return result;
    }

    const bool pipelined = ((_client->capabilities.extensions &
                             D_SMTP_EXTENSION_PIPELINING) != 0u);

    result = (pipelined) ? d_internal_smtp_open_pipelined(_client,
                                                          _envelope,
                                                          _length,
                                                          _report)
                         : d_internal_smtp_open_serial(_client,
                                                       _envelope,
                                                       _length,
                                                       _report);

    if (result != D_SMTP_OK)
    {
        return result;
    }

    result = d_internal_smtp_body(_client,
                                  (_message) ? _message : "",
                                  _length);

    if (_report)
    {
        _report->data_code = _client->reply.code;
    }

    return result;
}

/*
d_smtp_client_reset
  RSET, expecting 2yz; a success ends any transaction, and its
SMTPUTF8 choice with it.
*/
enum d_smtp_error
d_smtp_client_reset(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if ( (_client->state != D_SMTP_CLIENT_READY) &&
         (_client->state != D_SMTP_CLIENT_MAIL) )
    {
        return D_SMTP_ERROR_STATE;
    }

    const enum d_smtp_error result = d_internal_smtp_command(_client,
                                                             "RSET\r\n",
                                                             2u);

    if (result == D_SMTP_OK)
    {
        _client->state = D_SMTP_CLIENT_READY;
        _client->utf8  = false;
    }

    return result;
}

/*
d_smtp_client_noop
  NOOP, expecting 2yz, in any state with a session to keep alive.
*/
enum d_smtp_error
d_smtp_client_noop(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if ( (_client->state != D_SMTP_CLIENT_HELLO) &&
         (_client->state != D_SMTP_CLIENT_READY) &&
         (_client->state != D_SMTP_CLIENT_MAIL) )
    {
        return D_SMTP_ERROR_STATE;
    }

    return d_internal_smtp_command(_client,
                                   "NOOP\r\n",
                                   2u);
}

/*
d_smtp_client_quit
  QUIT, expecting 2yz. The client is QUIT whatever the reply, since
the session ends either way; close() releases it.
*/
enum d_smtp_error
d_smtp_client_quit(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if ( (_client->state != D_SMTP_CLIENT_HELLO) &&
         (_client->state != D_SMTP_CLIENT_READY) &&
         (_client->state != D_SMTP_CLIENT_MAIL) )
    {
        return D_SMTP_ERROR_STATE;
    }

    const enum d_smtp_error result = d_internal_smtp_command(_client,
                                                             "QUIT\r\n",
                                                             2u);

    _client->state = D_SMTP_CLIENT_QUIT;

    return result;
}

/*
d_smtp_client_close
  The TLS session goes first, sending close_notify where it still can; then
a connection connect() opened. What explains the session's end -- the last
reply, the causes kept -- survives for the caller.
*/
void
d_smtp_client_close(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return;
    }

    // the TLS session this client made, ended in an orderly way if it can be
    if (_client->has_tls)
    {
        (void)d_ssl_session_shutdown(&_client->tls);
        d_ssl_session_destroy(&_client->tls);
        _client->has_tls = false;
    }

    // a connection connect() opened; an attached one is its owner's
    if (_client->owns_tcp)
    {
        d_tcp_connection_close(&_client->tcp);
        _client->owns_tcp = false;
    }

    d_smtp_reader_discard(&_client->reader);
    d_smtp_capabilities_clear(&_client->capabilities);

    _client->reader.connection = NULL;
    _client->connection        = NULL;
    _client->state             = D_SMTP_CLIENT_CLOSED;
    _client->secure            = false;
    _client->authenticated     = false;
    _client->utf8              = false;

    return;
}

/*
d_smtp_client_tls_session
  Only a live session is offered: close releases it.
*/
const struct d_ssl_session*
d_smtp_client_tls_session(
    const struct d_smtp_client* _client
)
{
    return ( (_client) &&
             (_client->has_tls) ) ? &_client->tls
                                  : NULL;
}

/*
d_internal_smtp_host_name
  The machine's name, when it is fully qualified and valid as EHLO's
argument; anything else leaves the caller to fall back to an address
literal.
*/
static enum d_smtp_error
d_internal_smtp_host_name(
    struct d_smtp_buffer* _buffer
)
{
    char name[D_SMTP_DOMAIN_MAX + 1u] = { 0 };

    if (d_tcp_host_name(name,
                        sizeof(name)) != D_NET_OK)
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    const size_t length = strlen(name);

    if ( (!strchr(name,
                  '.')) ||
         (!d_smtp_domain_is_valid(name,
                                  length,
                                  false)) )
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    return d_smtp_buffer_append(_buffer,
                                name,
                                length);
}

/*
d_internal_smtp_local_literal
  "[a.b.c.d]" or "[IPv6:...]" for the connection's own address, which must
make a valid address literal: a connection that knows none, or reports
something else, leaves nothing to offer.
*/
static enum d_smtp_error
d_internal_smtp_local_literal(
    const struct d_net_connection* _connection,
    struct d_smtp_buffer*          _buffer
)
{
    struct d_net_endpoint local                      = { .port = 0 };
    char                  text[D_NET_HOST_MAX + 16u] = { 0 };

    d_net_endpoint_init(&local);

    if ( (!d_net_connection_local_endpoint(_connection,
                                           &local)) ||
         (local.host[0] == '\0') )
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    const int written = snprintf(text,
                                 sizeof(text),
                                 (strchr(local.host,
                                         ':')) ? "[IPv6:%s]" : "[%s]",
                                 local.host);

    // only a literal EHLO would take
    if ( (written < 0)                     ||
         ((size_t)written >= sizeof(text)) ||
         (!d_smtp_domain_is_valid(text,
                                  (size_t)written,
                                  false)) )
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    return d_smtp_buffer_append(_buffer,
                                text,
                                (size_t)written);
}

/*
d_smtp_client_identity
  A fully qualified name is preferred over an address literal, as RFC 5321
section 4.1.4 asks.
*/
enum d_smtp_error
d_smtp_client_identity(
    const struct d_net_connection* _connection,
    struct d_smtp_buffer*          _buffer
)
{
    // parameter validation first
    if ( (!_connection) ||
         (!_buffer) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    // a fully qualified host name is preferred over an address literal
    if (d_internal_smtp_host_name(_buffer) == D_SMTP_OK)
    {
        return D_SMTP_OK;
    }

    return d_internal_smtp_local_literal(_connection,
                                         _buffer);
}

/*
d_internal_smtp_name_for
  The name to check the server's certificate against: the one given, or
else the host connected to.
*/
static const char*
d_internal_smtp_name_for(
    const struct d_smtp_client_options* _options
)
{
    return (_options->server_name) ? _options->server_name
                                   : _options->host;
}

/*
d_internal_smtp_tls_fits
  Whether the options' context suits their mode: a mode that requires TLS
needs a usable client context, STARTTLS_OPTIONAL one if any is given, and a
plaintext session none at all.
*/
static bool
d_internal_smtp_tls_fits(
    const struct d_smtp_client_options* _options
)
{
    const bool required = ( (_options->security ==
                             D_SMTP_SECURITY_STARTTLS) ||
                            (_options->security == D_SMTP_SECURITY_TLS) );

    // plaintext, or an opportunistic upgrade with nothing to upgrade with
    if ( (_options->security == D_SMTP_SECURITY_NONE) ||
         ( (!required) &&
           (!_options->tls) ) )
    {
        return true;
    }

    return d_internal_smtp_tls_usable(_options->tls,
                                      d_internal_smtp_name_for(_options));
}

/*
d_internal_smtp_port
  The port by security mode, unless one was given: implicit TLS on 465,
STARTTLS submission on 587, plaintext relay on 25.
*/
static unsigned int
d_internal_smtp_port(
    const struct d_smtp_client_options* _options
)
{
    if (_options->port != 0)
    {
        return _options->port;
    }

    if (_options->security == D_SMTP_SECURITY_TLS)
    {
        return D_SMTP_PORT_SUBMISSIONS;
    }

    return (_options->security == D_SMTP_SECURITY_NONE)
           ? D_SMTP_PORT
           : D_SMTP_PORT_SUBMISSION;
}

/*
d_internal_smtp_connect_error
  What a failure to open the connection is. One tcp.h could not find the
host for -- by its name, or by any route, both of which it reports as
HOST_UNREACHABLE -- or whose address is malformed is RESOLVE; one the host
refused, or that failed on the way, is CONNECT. A timeout, a lack of memory,
and a bad argument are their namesakes.
*/
static enum d_smtp_error
d_internal_smtp_connect_error(
    enum d_net_error _error
)
{
    switch (_error)
    {
        case D_NET_OK:
            return D_SMTP_OK;
        case D_NET_ERROR_INVALID_ARGUMENT:
            return D_SMTP_ERROR_INVALID_ARGUMENT;
        case D_NET_ERROR_OUT_OF_MEMORY:
            return D_SMTP_ERROR_OUT_OF_MEMORY;
        case D_NET_ERROR_TIMED_OUT:
            return D_SMTP_ERROR_TIMED_OUT;
        case D_NET_ERROR_ADDRESS_INVALID:
        case D_NET_ERROR_HOST_UNREACHABLE:
            return D_SMTP_ERROR_RESOLVE;
        case D_NET_ERROR_CLOSED:
        case D_NET_ERROR_WOULD_BLOCK:
        case D_NET_ERROR_INTERRUPTED:
        case D_NET_ERROR_CONNECTION_RESET:
        case D_NET_ERROR_CONNECTION_REFUSED:
        case D_NET_ERROR_CONNECTION_ABORTED:
        case D_NET_ERROR_ADDRESS_IN_USE:
        case D_NET_ERROR_NETWORK_DOWN:
        case D_NET_ERROR_ACCESS_DENIED:
        case D_NET_ERROR_MESSAGE_TOO_LARGE:
        case D_NET_ERROR_TOO_MANY_OPEN_FILES:
        case D_NET_ERROR_UNKNOWN:
        case D_NET_ERROR_PROTOCOL:
            break;
    }

    return D_SMTP_ERROR_CONNECT;
}

/*
d_internal_smtp_open
  Opens the TCP connection the client owns, and adopts it.
*/
static enum d_smtp_error
d_internal_smtp_open(
    struct d_smtp_client*               _client,
    const struct d_smtp_client_options* _options
)
{
    struct d_net_endpoint endpoint = { .port = 0 };
    struct d_tcp_options  tcp      = { .connect_timeout_ms = 0 };
    const size_t          length   = strlen(_options->host);

    // without a socket backend there is nothing to connect with
    if (!d_tcp_available())
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    d_net_endpoint_init(&endpoint);
    d_tcp_options_init(&tcp);
    memcpy(endpoint.host,
           _options->host,
           length + 1u);
    endpoint.port          = (d_net_port)d_internal_smtp_port(_options);
    endpoint.protocol      = D_NET_PROTOCOL_TCP;
    tcp.connect_timeout_ms = _options->connect_timeout_ms;
    tcp.io_timeout_ms      = _options->io_timeout_ms;

    const enum d_net_error opened = d_tcp_connect(&_client->tcp,
                                                  &endpoint,
                                                  &tcp);

    if (opened != D_NET_OK)
    {
        _client->reader.net_error = opened;

        return d_internal_smtp_connect_error(opened);
    }

    _client->owns_tcp = true;

    return d_smtp_client_attach(_client,
                                &_client->tcp.base,
                                false);
}

/*
d_internal_smtp_settle
  Negotiates implicit TLS, then settles the EHLO identity: the one given,
or this machine's.
*/
static enum d_smtp_error
d_internal_smtp_settle(
    struct d_smtp_client*               _client,
    const struct d_smtp_client_options* _options
)
{
    enum d_smtp_error result = D_SMTP_OK;

    // implicit TLS negotiates before the server says a word (RFC 8314)
    if (_options->security == D_SMTP_SECURITY_TLS)
    {
        result = d_internal_smtp_secure(_client,
                                        _options->tls,
                                        d_internal_smtp_name_for(_options));
    }

    if (result != D_SMTP_OK)
    {
        return result;
    }

    _client->allow_insecure_auth = _options->allow_insecure_auth;

    if (_options->helo_domain)
    {
        return d_internal_smtp_set_helo(_client,
                                        _options->helo_domain);
    }

    struct d_smtp_buffer identity = { .data     = _client->helo,
                                      .capacity = sizeof(_client->helo),
                                      .length   = 0 };

    return d_smtp_client_identity(&_client->tcp.base,
                                  &identity);
}

/*
d_internal_smtp_establish
  Greeting, EHLO, then the STARTTLS policy. A required upgrade that is not
offered fails closed: a missing STARTTLS is exactly what a downgrade attack
looks like. An optional one is made when offered and there is a context to
make it with.
*/
static enum d_smtp_error
d_internal_smtp_establish(
    struct d_smtp_client*               _client,
    const struct d_smtp_client_options* _options
)
{
    enum d_smtp_error result = d_smtp_client_greeting(_client);

    if (result == D_SMTP_OK)
    {
        result = d_smtp_client_hello(_client,
                                     NULL);
    }

    if ( (result != D_SMTP_OK) ||
         (_client->secure) )
    {
        return result;
    }

    const bool offered = ((_client->capabilities.extensions &
                           D_SMTP_EXTENSION_STARTTLS) != 0u);

    if ( (_options->security == D_SMTP_SECURITY_STARTTLS) ||
         ( (_options->security == D_SMTP_SECURITY_STARTTLS_OPTIONAL) &&
           (offered)                                                 &&
           (_options->tls) ) )
    {
        return d_smtp_client_starttls(_client,
                                      _options->tls,
                                      d_internal_smtp_name_for(_options));
    }

    return D_SMTP_OK;
}

/*
d_smtp_client_connect
  Everything the options say is checked before a connection is opened.
*/
enum d_smtp_error
d_smtp_client_connect(
    struct d_smtp_client*               _client,
    const struct d_smtp_client_options* _options
)
{
    // parameter validation first
    if ( (!_client)                                 ||
         (!_options)                                ||
         (!_options->host)                          ||
         (_options->host[0] == '\0')                ||
         (strlen(_options->host) > D_NET_HOST_MAX)  ||
         (_options->port > 65535u)                  ||
         (_options->security > D_SMTP_SECURITY_TLS) ||
         (!d_internal_smtp_tls_fits(_options)) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->state != D_SMTP_CLIENT_CLOSED)
    {
        return D_SMTP_ERROR_STATE;
    }

    // what explained an earlier session's end is forgotten here, where a
    // new one starts, as attach forgets it
    d_smtp_reply_clear(&_client->reply);
    _client->reader.net_error = D_NET_OK;
    _client->tls_status       = D_SSL_OK;

    enum d_smtp_error result = d_internal_smtp_open(_client,
                                                    _options);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_settle(_client,
                                        _options);
    }

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_establish(_client,
                                           _options);
    }

    // a failed opening leaves nothing behind but what explains it
    if (result != D_SMTP_OK)
    {
        d_smtp_client_close(_client);
    }

    return result;
}
