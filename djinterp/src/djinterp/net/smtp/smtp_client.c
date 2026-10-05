/*******************************************************************************
* djinterp [net]                                                   smtp_client.c
*
* djinterp SMTP client -- implementation.
*   The client state machine over a d_smtp_transport. Command lines are
* assembled from validated parts into fixed buffers, so nothing a caller
* passes can smuggle CR or LF onto the wire; SASL buffers holding secrets are
* wiped through volatile stores once used.
*
*
* path:      /src/djinterp/net/smtp/smtp_client.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/net/smtp/smtp_client.h"
// std
#include <limits.h>   // INT_MAX
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memchr, memcpy, memset, strlen
// djinterp
#include "../../../../inc/djinterp/net/smtp/smtp_common.h"          // replies
#include "../../../../inc/djinterp/net/smtp/smtp_transport.h"       // io
#include "../../../../inc/djinterp/env/net/smtp/env_smtp.h"         // flags

#if D_ENV_SMTP_HAS_CRAM_MD5
    // openssl
    #include <openssl/evp.h>   // EVP_md5
    #include <openssl/hmac.h>  // HMAC
#endif


// D_INTERNAL_SMTP_CHUNK
//   macro: bytes of encoded message data, or of pipelined commands, gathered
// per transport write.
#define D_INTERNAL_SMTP_CHUNK     8192u

// D_INTERNAL_SMTP_SASL_MAX
//   macro: the largest SASL message the client assembles, before base64.
#define D_INTERNAL_SMTP_SASL_MAX  2048u

// D_INTERNAL_SMTP_DATA_LINE
//   macro: the longest message line, without its CRLF (RFC 5321 4.5.3.1.6).
#define D_INTERNAL_SMTP_DATA_LINE 998u

// d_internal_smtp_part
//   struct: one piece of a line under assembly.
struct d_internal_smtp_part
{
    const char* data;
    size_t      length;
};


/*
d_internal_smtp_wipe
  Zeroes memory through volatile stores, which the compiler may not elide the
way it may elide a memset() of a buffer that is about to go out of scope.
*/
static void
d_internal_smtp_wipe(
    void*  _data,
    size_t _length
)
{
    volatile unsigned char* cursor = _data;

    for (size_t i = 0; i < _length; ++i)
    {
        cursor[i] = 0;
    }

    return;
}

/*
d_internal_smtp_compose
  Appends every part or none: on failure the buffer is rolled back to its
length on entry.
*/
static enum d_smtp_error
d_internal_smtp_compose(
    struct d_smtp_buffer*              _buffer,
    const struct d_internal_smtp_part* _parts,
    size_t                             _count
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
             (outcome != D_SMTP_ERROR_TOO_LONG) )
        {
            return outcome;
        }

        const enum d_smtp_error parsed = d_smtp_reply_feed(&_client->reply,
                                                           line,
                                                           length);

        if (parsed != D_SMTP_OK)
        {
            return parsed;
        }

        if (outcome == D_SMTP_ERROR_TOO_LONG)
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
    const enum d_smtp_error sent = d_smtp_transport_write_all(
                                       &_client->reader.transport,
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
    _options->helo_domain         = NULL;
    _options->connect_timeout_ms  = D_SMTP_CLIENT_CONNECT_TIMEOUT_MS;
    _options->io_timeout_ms       = D_SMTP_CLIENT_IO_TIMEOUT_MS;
    _options->allow_insecure_auth = false;
    d_smtp_tls_options_init(&_options->tls);

    return;
}

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
    d_smtp_socket_init(&_client->socket);
    _client->state = D_SMTP_CLIENT_CLOSED;

    return;
}

enum d_smtp_error
d_smtp_client_attach(
    struct d_smtp_client*          _client,
    const struct d_smtp_transport* _transport,
    bool                           _secure
)
{
    // parameter validation first
    if ( (!_client)           ||
         (!_transport)        ||
         (!_transport->read)  ||
         (!_transport->write) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    if (_client->state != D_SMTP_CLIENT_CLOSED)
    {
        return D_SMTP_ERROR_STATE;
    }

    d_smtp_reader_init(&_client->reader,
                       _transport);
    d_smtp_capabilities_clear(&_client->capabilities);
    d_smtp_reply_clear(&_client->reply);

    _client->state         = D_SMTP_CLIENT_GREETING;
    _client->secure        = _secure;
    _client->authenticated = false;
    _client->utf8          = false;

    return D_SMTP_OK;
}

enum d_smtp_error
d_smtp_client_greeting(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID;
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
        return (_client->helo[0] != '\0') ? D_SMTP_OK : D_SMTP_ERROR_INVALID;
    }

    const size_t length = strlen(_domain);

    if ( (length >= sizeof(_client->helo)) ||
         (!d_smtp_domain_is_valid(_domain, length, false)) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    memcpy(_client->helo,
           _domain,
           length + 1u);

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
    const struct d_internal_smtp_part parts[] =
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

enum d_smtp_error
d_smtp_client_hello(
    struct d_smtp_client* _client,
    const char*           _domain
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID;
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

enum d_smtp_error
d_smtp_client_starttls(
    struct d_smtp_client*            _client,
    const struct d_smtp_tls_options* _options
)
{
    // parameter validation first
    if ( (!_client) ||
         (!_options) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    if ( (_client->state != D_SMTP_CLIENT_READY) ||
         (_client->secure) )
    {
        return D_SMTP_ERROR_STATE;
    }

    if ( ((_client->capabilities.extensions &
           D_SMTP_EXTENSION_STARTTLS) == 0u) ||
         (!_client->reader.transport.starttls) )
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

    // bytes already buffered arrived in plaintext after the 220 and would be
    // read as though protected: a response-injection attempt
    if (d_smtp_reader_pending(&_client->reader) > 0)
    {
        return D_SMTP_ERROR_PROTOCOL;
    }

    const struct d_smtp_transport* transport = &_client->reader.transport;

    result = transport->starttls(transport->context,
                                 _options);

    if (result != D_SMTP_OK)
    {
        return result;
    }

    // everything learnt before the handshake is discarded (RFC 3207 4.2)
    _client->secure = true;
    _client->state  = D_SMTP_CLIENT_HELLO;
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
        result = d_smtp_base64_encode(_data,
                                      _length,
                                      &line);
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

    d_internal_smtp_wipe(storage,
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
    const struct d_internal_smtp_part parts[] =
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

    d_internal_smtp_wipe(raw,
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

#if D_ENV_SMTP_HAS_CRAM_MD5

/*
d_internal_smtp_cram_digest
  HMAC-MD5 of the challenge keyed with the secret, as 32 lower-case hex
digits (RFC 2195). A FIPS-restricted libcrypto may refuse MD5; that is
reported as D_SMTP_ERROR_UNSUPPORTED.
*/
static enum d_smtp_error
d_internal_smtp_cram_digest(
    const char* _secret,
    const char* _challenge,
    size_t      _length,
    char*       _hex
)
{
    static const char HEX_DIGITS[] = "0123456789abcdef";
    unsigned char     mac[EVP_MAX_MD_SIZE] = { 0 };
    unsigned int      mac_length           = 0;
    const size_t      key_length           = strlen(_secret);

    if (key_length > (size_t)INT_MAX)
    {
        return D_SMTP_ERROR_TOO_LONG;
    }

    if ( (!HMAC(EVP_md5(),
                _secret,
                (int)key_length,
                (const unsigned char*)_challenge,
                _length,
                mac,
                &mac_length)) ||
         (mac_length != 16u) )
    {
        return D_SMTP_ERROR_UNSUPPORTED;
    }

    for (size_t i = 0; i < 16u; ++i)
    {
        _hex[2u * i]      = HEX_DIGITS[mac[i] >> 4];
        _hex[2u * i + 1u] = HEX_DIGITS[mac[i] & 0x0Fu];
    }

    _hex[32] = '\0';
    d_internal_smtp_wipe(mac,
                         sizeof(mac));

    return D_SMTP_OK;
}

#else

/*
d_internal_smtp_cram_digest
  CRAM-MD5 is not compiled in (D_ENV_SMTP_HAS_CRAM_MD5 is 0).
*/
static enum d_smtp_error
d_internal_smtp_cram_digest(
    const char* _secret,
    const char* _challenge,
    size_t      _length,
    char*       _hex
)
{
    (void)_secret;
    (void)_challenge;
    (void)_length;
    (void)_hex;

    return D_SMTP_ERROR_UNSUPPORTED;
}

#endif  // D_ENV_SMTP_HAS_CRAM_MD5

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
                                      ? d_smtp_base64_decode(text,
                                                             length,
                                                             &challenge)
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
        const struct d_internal_smtp_part parts[] =
        {
            { _credentials->user, strlen(_credentials->user) },
            { " ",                1 },
            { digest,             32 }
        };

        result = d_internal_smtp_compose(_answer,
                                         parts,
                                         3);
    }

    d_internal_smtp_wipe(digest,
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
    d_internal_smtp_wipe(storage,
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
    if ( (memchr(_credentials->user, '\x01', user_length)) ||
         (memchr(_credentials->secret, '\x01', secret_length)) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    char                              raw[D_INTERNAL_SMTP_SASL_MAX] = { 0 };
    struct d_smtp_buffer              message = { .data     = raw,
                                                  .capacity = sizeof(raw),
                                                  .length   = 0 };
    const struct d_internal_smtp_part parts[] =
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

    d_internal_smtp_wipe(raw,
                         sizeof(raw));

    return d_internal_smtp_auth_result(result);
}

/*
d_internal_smtp_choose
  The mechanism to use: the most preferred one that is acceptable, offered,
and compiled in. XOAUTH2 ranks first but is acceptable only on request.
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
    const unsigned int built      = (D_ENV_SMTP_HAS_CRAM_MD5)
                                    ? ~0u
                                    : ~(unsigned int)D_SMTP_AUTH_CRAM_MD5;
    const unsigned int usable     = acceptable &
                                    _capabilities->auth &
                                    built;

    for (size_t i = 0; i < sizeof(PREFERENCE) / sizeof(PREFERENCE[0]); ++i)
    {
        if ((usable & PREFERENCE[i]) != 0u)
        {
            return PREFERENCE[i];
        }
    }

    return D_SMTP_AUTH_NONE;
}

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
        return D_SMTP_ERROR_INVALID;
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
            result = d_internal_smtp_auth_plain(_client, _credentials);
            break;

        case D_SMTP_AUTH_LOGIN:
            result = d_internal_smtp_auth_login(_client, _credentials);
            break;

        case D_SMTP_AUTH_CRAM_MD5:
            result = d_internal_smtp_auth_cram(_client, _credentials);
            break;

        case D_SMTP_AUTH_XOAUTH2:
            result = d_internal_smtp_auth_xoauth2(_client, _credentials);
            break;

        default:
            break;
    }

    _client->authenticated = (result == D_SMTP_OK);

    return result;
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
         (!d_smtp_address_is_valid(sender, length, _envelope->utf8)) )
    {
        return D_SMTP_ERROR_INVALID;
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
        return D_SMTP_ERROR_TOO_LONG;
    }

    const struct d_internal_smtp_part parts[] =
    {
        { "MAIL FROM:<", 11 },
        { sender,        length },
        { ">",           1 }
    };
    enum d_smtp_error result = d_internal_smtp_compose(_line,
                                                       parts,
                                                       3);

    // SIZE lets the server refuse before the data is sent (RFC 1870)
    if ( (result == D_SMTP_OK) &&
         (sized)               &&
         (_size > 0) )
    {
        result = d_smtp_buffer_append_text(_line, " SIZE=");

        if (result == D_SMTP_OK)
        {
            result = d_smtp_buffer_append_number(_line, _size);
        }
    }

    if ( (result == D_SMTP_OK) &&
         (_envelope->eight_bit) )
    {
        result = d_smtp_buffer_append_text(_line, " BODY=8BITMIME");
    }

    if ( (result == D_SMTP_OK) &&
         (_envelope->utf8) )
    {
        result = d_smtp_buffer_append_text(_line, " SMTPUTF8");
    }

    return (result == D_SMTP_OK) ? d_smtp_buffer_append_text(_line, "\r\n")
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

    if (!d_smtp_address_is_valid(_recipient, length, _utf8))
    {
        return D_SMTP_ERROR_INVALID;
    }

    const struct d_internal_smtp_part parts[] =
    {
        { "RCPT TO:<", 9 },
        { _recipient,  length },
        { ">\r\n",     3 }
    };

    return d_internal_smtp_compose(_line,
                                   parts,
                                   3);
}

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
        return D_SMTP_ERROR_INVALID;
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
        return D_SMTP_ERROR_INVALID;
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
            return D_SMTP_ERROR_TOO_LONG;
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
        result  = d_smtp_transport_write_all(&_client->reader.transport,
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
        return D_SMTP_ERROR_INVALID;
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
        const enum d_smtp_error flushed = d_smtp_transport_write_all(
                                              &_client->reader.transport,
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
                     : d_smtp_buffer_append_text(&line, "DATA\r\n");
        }
    }

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_queue(_client,
                                       &batch,
                                       &line);
    }

    return (result == D_SMTP_OK)
           ? d_smtp_transport_write_all(&_client->reader.transport,
                                        batch.data,
                                        batch.length)
           : result;
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
    const size_t        count    = _envelope->recipient_count;
    struct d_smtp_reply cause    = { .code = 0 };
    size_t              accepted = 0;
    bool                mail_ok  = false;
    enum d_smtp_error   result   = d_internal_smtp_send_batch(_client,
                                                              _envelope,
                                                              _size);

    // MAIL's reply, one per recipient, then DATA's: all of them are read
    for (size_t i = 0; i < count + 2u; ++i)
    {
        if (result == D_SMTP_OK)
        {
            result = d_internal_smtp_read_reply(_client);
        }

        if (result != D_SMTP_OK)
        {
            return result;
        }

        const bool positive = d_smtp_code_is_positive(_client->reply.code);

        // the refusal kept for the caller: MAIL's, else the last recipient's
        if (i == 0)
        {
            mail_ok = positive;
            cause   = _client->reply;
        }
        else if (i <= count)
        {
            d_internal_smtp_record(_report,
                                   i - 1u,
                                   &_client->reply);
            accepted += (positive) ? 1u : 0u;

            if ( (mail_ok) &&
                 (!positive) )
            {
                cause = _client->reply;
            }
        }
    }

    const bool data_ok = (_client->reply.code == 354u);

    if ( (mail_ok)      &&
         (accepted > 0) &&
         (data_ok) )
    {
        _client->state = D_SMTP_CLIENT_MAIL;
        _client->utf8  = _envelope->utf8;

        return D_SMTP_OK;
    }

    // with MAIL and a recipient accepted, DATA itself was refused
    if ( (mail_ok) &&
         (accepted > 0) )
    {
        cause = _client->reply;
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
                                   &cause);
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
        return D_SMTP_ERROR_INVALID;
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

enum d_smtp_error
d_smtp_client_reset(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID;
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

enum d_smtp_error
d_smtp_client_noop(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID;
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

enum d_smtp_error
d_smtp_client_quit(
    struct d_smtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID;
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

    // the transport's own release, then the built-in socket (idempotent)
    if (_client->reader.transport.close)
    {
        _client->reader.transport.close(_client->reader.transport.context);
    }

    d_smtp_socket_close(&_client->socket);
    memset(&_client->reader.transport,
           0,
           sizeof(_client->reader.transport));
    d_smtp_reader_discard(&_client->reader);
    d_smtp_capabilities_clear(&_client->capabilities);

    _client->state         = D_SMTP_CLIENT_CLOSED;
    _client->secure        = false;
    _client->authenticated = false;
    _client->utf8          = false;

    return;
}

/*
d_internal_smtp_tls_for
  The client's TLS options, with the name to verify defaulting to the host
connected to.
*/
static struct d_smtp_tls_options
d_internal_smtp_tls_for(
    const struct d_smtp_client_options* _options
)
{
    struct d_smtp_tls_options tls = _options->tls;

    if (!tls.server_name)
    {
        tls.server_name = _options->host;
    }

    return tls;
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
d_internal_smtp_open
  Connects the built-in socket, negotiates implicit TLS, adopts the socket as
the transport, and settles the EHLO identity.
*/
static enum d_smtp_error
d_internal_smtp_open(
    struct d_smtp_client*               _client,
    const struct d_smtp_client_options* _options
)
{
    const struct d_smtp_tls_options tls  = d_internal_smtp_tls_for(_options);
    const unsigned int              port = d_internal_smtp_port(_options);
    const bool implicit = (_options->security == D_SMTP_SECURITY_TLS);
    struct d_smtp_transport transport = { .context = NULL };
    enum d_smtp_error       result    =
        d_smtp_socket_connect(&_client->socket,
                              _options->host,
                              port,
                              _options->connect_timeout_ms,
                              _options->io_timeout_ms);

    // implicit TLS negotiates before the server says a word (RFC 8314)
    if ( (result == D_SMTP_OK) &&
         (implicit) )
    {
        result = d_smtp_socket_start_tls(&_client->socket,
                                         &tls);
    }

    if (result != D_SMTP_OK)
    {
        return result;
    }

    d_smtp_socket_transport(&_client->socket,
                            &transport);
    result = d_smtp_client_attach(_client,
                                  &transport,
                                  implicit);

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

    return d_smtp_socket_identity(&_client->socket,
                                  &identity);
}

/*
d_internal_smtp_establish
  Greeting, EHLO, then the STARTTLS policy. A required upgrade that is not
offered fails closed: a missing STARTTLS is exactly what a downgrade attack
looks like.
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
           (offered) ) )
    {
        const struct d_smtp_tls_options tls = d_internal_smtp_tls_for(
                                                  _options);

        return d_smtp_client_starttls(_client,
                                      &tls);
    }

    return D_SMTP_OK;
}

enum d_smtp_error
d_smtp_client_connect(
    struct d_smtp_client*               _client,
    const struct d_smtp_client_options* _options
)
{
    // parameter validation first
    if ( (!_client)                                ||
         (!_options)                               ||
         (!_options->host)                         ||
         (_options->host[0] == '\0')               ||
         (_options->security > D_SMTP_SECURITY_TLS) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    if (_client->state != D_SMTP_CLIENT_CLOSED)
    {
        return D_SMTP_ERROR_STATE;
    }

    enum d_smtp_error result = d_internal_smtp_open(_client,
                                                    _options);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_establish(_client,
                                           _options);
    }

    // a failed opening leaves nothing behind but the reply explaining it
    if (result != D_SMTP_OK)
    {
        d_smtp_client_close(_client);
    }

    return result;
}
