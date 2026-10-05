/*******************************************************************************
* djinterp [net]                                                   smtp_server.c
*
* djinterp SMTP server -- implementation.
*   The session engine: a loop that reads one command, dispatches it on its
* verb, and answers with an RFC 3463 enhanced status on every reply that may
* carry one. Session state lives on the heap -- a session is long-lived and
* may run on a thread with a small stack -- and message data grows by
* doubling up to the configured limit, so memory tracks the largest message
* actually received rather than the largest allowed.
*
*
* path:      /src/djinterp/net/smtp/smtp_server.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../inc/djinterp/net/smtp/smtp_server.h"
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // SIZE_MAX
#include <stdlib.h>   // calloc, free, realloc
#include <string.h>   // memchr, memcpy, strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/net/smtp/smtp_common.h"          // replies
#include "../../../../inc/djinterp/net/smtp/smtp_transport.h"       // io


// D_INTERNAL_SMTP_USER_MAX
//   macro: storage for an authenticated identity, NUL included.
#define D_INTERNAL_SMTP_USER_MAX      256u

// D_INTERNAL_SMTP_MESSAGE_START
//   macro: first allocation for message data; it doubles from here.
#define D_INTERNAL_SMTP_MESSAGE_START 16384u

// d_internal_smtp_session
//   struct: everything one session knows. `info` points into the storage
// members, so callbacks always see stable strings.
struct d_internal_smtp_session
{
    struct d_smtp_reader                reader;
    const struct d_smtp_server_config*  config;
    const struct d_smtp_server_handler* handler;
    struct d_smtp_session_info          info;
    char*                               recipient_storage;
    const char**                        recipient_list;
    char*                               message;
    size_t                              message_length;
    size_t                              message_capacity;
    unsigned int                        errors;
    bool                                greeted;
    bool                                extended;
    bool                                in_mail;
    bool                                quit;
    char                                client_domain[D_SMTP_DOMAIN_MAX + 1u];
    char                                user[D_INTERNAL_SMTP_USER_MAX];
    char                                sender[D_SMTP_PATH_MAX];
    char                                line[D_SMTP_LINE_MAX];
};

// d_internal_smtp_verdict
//   struct: a reply decided before it is sent; a code of 0 accepts.
struct d_internal_smtp_verdict
{
    unsigned int code;
    const char*  status;
    const char*  text;
};

// d_internal_smtp_intake
//   struct: what went wrong while message data was being received. Once any
// flag is set, the rest of the data is consumed and discarded.
struct d_internal_smtp_intake
{
    bool oversize;
    bool overlong;
    bool no_memory;
};


/*
d_internal_smtp_respond
  Sends one reply. An enhanced status opens the text (RFC 2034); the text's
'\n's become continuation lines through d_smtp_reply_format().
*/
static enum d_smtp_error
d_internal_smtp_respond(
    struct d_internal_smtp_session* _session,
    unsigned int                    _code,
    const char*                     _status,
    const char*                     _text
)
{
    char                 text_storage[D_SMTP_REPLY_TEXT_MAX]  = { 0 };
    char                 reply_storage[D_SMTP_REPLY_TEXT_MAX] = { 0 };
    struct d_smtp_buffer text   = { .data     = text_storage,
                                    .capacity = sizeof(text_storage),
                                    .length   = 0 };
    struct d_smtp_buffer reply  = { .data     = reply_storage,
                                    .capacity = sizeof(reply_storage),
                                    .length   = 0 };
    enum d_smtp_error    result = D_SMTP_OK;

    if (_status)
    {
        result = d_smtp_buffer_append_text(&text, _status);

        if (result == D_SMTP_OK)
        {
            result = d_smtp_buffer_append_text(&text, " ");
        }
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_buffer_append_text(&text, _text);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_reply_format(&reply,
                                     _code,
                                     NULL,
                                     text.data);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_transport_write_all(&_session->reader.transport,
                                            reply.data,
                                            reply.length);
    }

    return result;
}

/*
d_internal_smtp_fault
  Answers a syntax or sequence error and counts it; past the limit, the
client is told why and dropped rather than served indefinitely.
*/
static enum d_smtp_error
d_internal_smtp_fault(
    struct d_internal_smtp_session* _session,
    unsigned int                    _code,
    const char*                     _status,
    const char*                     _text
)
{
    _session->errors += 1;

    if (_session->errors > D_SMTP_SERVER_ERRORS_MAX)
    {
        (void)d_internal_smtp_respond(_session,
                                      421u,
                                      "4.7.0",
                                      "Too many errors, closing connection");

        return D_SMTP_ERROR_PROTOCOL;
    }

    return d_internal_smtp_respond(_session,
                                   _code,
                                   _status,
                                   _text);
}

/*
d_internal_smtp_refuse
  Sends a verdict; refusals decided by policy are not client errors and are
not counted.
*/
static enum d_smtp_error
d_internal_smtp_refuse(
    struct d_internal_smtp_session*       _session,
    const struct d_internal_smtp_verdict* _verdict
)
{
    return d_internal_smtp_respond(_session,
                                   _verdict->code,
                                   _verdict->status,
                                   _verdict->text);
}

/*
d_internal_smtp_decide
  Turns a callback's answer into a verdict: 0 accepts, a 4yz or 5yz code
refuses with the matching status, and anything else is the callback's own
fault, answered as a local error.
*/
static struct d_internal_smtp_verdict
d_internal_smtp_decide(
    unsigned int _code,
    const char*  _permanent,
    const char*  _transient,
    const char*  _text
)
{
    struct d_internal_smtp_verdict verdict = { .code   = _code,
                                               .status = _permanent,
                                               .text   = _text };

    if (d_smtp_code_is_transient(_code))
    {
        verdict.status = _transient;
    }
    else if ( (_code != 0) &&
              (!d_smtp_code_is_permanent(_code)) )
    {
        verdict.code   = 451u;
        verdict.status = "4.3.0";
        verdict.text   = "Local error in processing";
    }

    return verdict;
}

/*
d_internal_smtp_join
  Appends up to three strings; NULL ones are skipped.
*/
static enum d_smtp_error
d_internal_smtp_join(
    struct d_smtp_buffer* _buffer,
    const char*           _first,
    const char*           _second,
    const char*           _third
)
{
    const char* const parts[3] = { _first, _second, _third };
    enum d_smtp_error result   = D_SMTP_OK;

    for (size_t i = 0; ( (i < 3) && (result == D_SMTP_OK) ); ++i)
    {
        if (parts[i])
        {
            result = d_smtp_buffer_append_text(_buffer, parts[i]);
        }
    }

    return result;
}

/*
d_internal_smtp_reset
  Ends the transaction in progress (RSET, a new EHLO, or a finished DATA);
the message buffer is kept for reuse.
*/
static void
d_internal_smtp_reset(
    struct d_internal_smtp_session* _session
)
{
    _session->in_mail              = false;
    _session->sender[0]            = '\0';
    _session->message_length       = 0;
    _session->info.sender          = NULL;
    _session->info.recipient_count = 0;
    _session->info.eight_bit       = false;
    _session->info.utf8            = false;

    return;
}

/*
d_internal_smtp_auth_offer
  The mechanisms AUTH may use now: a verifier is needed, and a protected
channel unless the configuration explicitly waives it.
*/
static unsigned int
d_internal_smtp_auth_offer(
    const struct d_internal_smtp_session* _session
)
{
    if ( (!_session->handler->auth) ||
         ( (!_session->info.secure) &&
           (!_session->config->allow_insecure_auth) ) )
    {
        return D_SMTP_AUTH_NONE;
    }

    return _session->config->auth_mechanisms &
           (unsigned int)(D_SMTP_AUTH_PLAIN | D_SMTP_AUTH_LOGIN);
}

/*
d_internal_smtp_can_starttls
  STARTTLS needs a plaintext session, an upgradeable transport, and a
certificate with its key.
*/
static bool
d_internal_smtp_can_starttls(
    const struct d_internal_smtp_session* _session
)
{
    const struct d_smtp_tls_options* tls = &_session->config->tls;

    return ( (!_session->info.secure)                     &&
             (_session->reader.transport.starttls != NULL) &&
             (tls->certificate_file != NULL)              &&
             (tls->private_key_file != NULL) );
}

/*
d_internal_smtp_ehlo
  The EHLO reply: a greeting line, then one line per extension offered now.
*/
static enum d_smtp_error
d_internal_smtp_ehlo(
    struct d_internal_smtp_session* _session
)
{
    char                 storage[D_SMTP_REPLY_TEXT_MAX] = { 0 };
    struct d_smtp_buffer text   = { .data     = storage,
                                    .capacity = sizeof(storage),
                                    .length   = 0 };
    const unsigned int   auth   = d_internal_smtp_auth_offer(_session);
    enum d_smtp_error    result = d_internal_smtp_join(&text,
                                                       _session->config->domain,
                                                       " Hello ",
                                                       _session->client_domain);

    if (result == D_SMTP_OK)
    {
        result = d_smtp_buffer_append_text(&text, "\nPIPELINING\nSIZE ");
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_buffer_append_number(
                     &text,
                     _session->config->max_message_size);
    }

    if (result == D_SMTP_OK)
    {
        result = d_smtp_buffer_append_text(
                     &text,
                     "\n8BITMIME\nENHANCEDSTATUSCODES\nSMTPUTF8");
    }

    if ( (result == D_SMTP_OK) &&
         (d_internal_smtp_can_starttls(_session)) )
    {
        result = d_smtp_buffer_append_text(&text, "\nSTARTTLS");
    }

    if ( (result == D_SMTP_OK) &&
         (auth != D_SMTP_AUTH_NONE) )
    {
        result = d_internal_smtp_join(
                     &text,
                     "\nAUTH",
                     ((auth & D_SMTP_AUTH_PLAIN) != 0u) ? " PLAIN" : NULL,
                     ((auth & D_SMTP_AUTH_LOGIN) != 0u) ? " LOGIN" : NULL);
    }

    return (result == D_SMTP_OK) ? d_internal_smtp_respond(_session,
                                                           250u,
                                                           NULL,
                                                           text.data)
                                 : result;
}

/*
d_internal_smtp_on_hello
  EHLO and HELO. The argument is taken as one token -- a domain, an address
literal, or whatever a misconfigured client calls itself -- as long as it is
safe to echo. A greeting ends any transaction (RFC 5321 section 4.1.4).
*/
static enum d_smtp_error
d_internal_smtp_on_hello(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_command*    _command,
    bool                            _extended
)
{
    const size_t length = _command->argument_length;

    if ( (length == 0)                                      ||
         (length > D_SMTP_DOMAIN_MAX)                       ||
         (!d_smtp_text_is_safe(_command->argument, length)) ||
         (memchr(_command->argument, ' ', length)) )
    {
        return d_internal_smtp_fault(_session,
                                     501u,
                                     "5.5.4",
                                     "Invalid domain name");
    }

    memcpy(_session->client_domain,
           _command->argument,
           length);
    _session->client_domain[length] = '\0';

    d_internal_smtp_reset(_session);
    _session->greeted  = true;
    _session->extended = _extended;

    if (_extended)
    {
        return d_internal_smtp_ehlo(_session);
    }

    char                 storage[D_SMTP_REPLY_TEXT_MAX] = { 0 };
    struct d_smtp_buffer text   = { .data     = storage,
                                    .capacity = sizeof(storage),
                                    .length   = 0 };
    enum d_smtp_error    result = d_internal_smtp_join(&text,
                                                       _session->config->domain,
                                                       " Hello ",
                                                       _session->client_domain);

    return (result == D_SMTP_OK) ? d_internal_smtp_respond(_session,
                                                           250u,
                                                           NULL,
                                                           text.data)
                                 : result;
}

/*
d_internal_smtp_on_starttls
  After the 220, anything the client pipelined in plaintext is discarded
before the handshake (CVE-2011-0411), and afterwards the session starts over:
nothing learnt in plaintext carries forward (RFC 3207 section 4.2). A failed
handshake leaves no usable channel, so it ends the session.
*/
static enum d_smtp_error
d_internal_smtp_on_starttls(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_command*    _command
)
{
    if (_command->argument_length > 0)
    {
        return d_internal_smtp_fault(_session,
                                     501u,
                                     "5.5.4",
                                     "Syntax error, no parameters allowed");
    }

    if ( (!_session->greeted) ||
         (_session->info.secure) )
    {
        return d_internal_smtp_fault(_session,
                                     503u,
                                     "5.5.1",
                                     "Bad sequence of commands");
    }

    if (!d_internal_smtp_can_starttls(_session))
    {
        return d_internal_smtp_fault(_session,
                                     502u,
                                     "5.5.1",
                                     "STARTTLS not available");
    }

    const enum d_smtp_error ready = d_internal_smtp_respond(_session,
                                                            220u,
                                                            "2.0.0",
                                                            "Ready to start "
                                                            "TLS");

    if (ready != D_SMTP_OK)
    {
        return ready;
    }

    d_smtp_reader_discard(&_session->reader);

    const struct d_smtp_transport* transport = &_session->reader.transport;
    const enum d_smtp_error        upgraded  = transport->starttls(
                                                   transport->context,
                                                   &_session->config->tls);

    if (upgraded != D_SMTP_OK)
    {
        return upgraded;
    }

    d_internal_smtp_reset(_session);
    _session->greeted          = false;
    _session->extended         = false;
    _session->client_domain[0] = '\0';
    _session->user[0]          = '\0';
    _session->info.user        = NULL;
    _session->info.secure      = true;

    return D_SMTP_OK;
}

/*
d_internal_smtp_sasl_decode
  One SASL response: "*" cancels (reported as D_SMTP_ERROR_AUTH), "=" is an
empty initial response (RFC 4954 section 4), and anything else is base64.
*/
static enum d_smtp_error
d_internal_smtp_sasl_decode(
    const char*           _text,
    size_t                _length,
    struct d_smtp_buffer* _buffer
)
{
    if ( (_length == 1) &&
         (_text[0] == '*') )
    {
        return D_SMTP_ERROR_AUTH;
    }

    if ( (_length == 1) &&
         (_text[0] == '=') )
    {
        return D_SMTP_OK;
    }

    return d_smtp_base64_decode(_text,
                                _length,
                                _buffer);
}

/*
d_internal_smtp_sasl_read
  Sends a 334 challenge and decodes the client's response.
*/
static enum d_smtp_error
d_internal_smtp_sasl_read(
    struct d_internal_smtp_session* _session,
    const char*                     _challenge,
    struct d_smtp_buffer*           _buffer
)
{
    size_t                  length = 0;
    const enum d_smtp_error asked  = d_internal_smtp_respond(_session,
                                                             334u,
                                                             NULL,
                                                             _challenge);

    if (asked != D_SMTP_OK)
    {
        return asked;
    }

    const enum d_smtp_error read = d_smtp_reader_line(&_session->reader,
                                                      _session->line,
                                                      sizeof(_session->line),
                                                      &length);

    // an overlong response cannot be decoded, but the stream is still in step
    if (read != D_SMTP_OK)
    {
        return (read == D_SMTP_ERROR_TOO_LONG) ? D_SMTP_ERROR_PROTOCOL : read;
    }

    return d_internal_smtp_sasl_decode(_session->line,
                                       length,
                                       _buffer);
}

/*
d_internal_smtp_sasl_failure
  A cancelled or undecodable response ends the exchange, not the session;
transport errors pass through and do end it.
*/
static enum d_smtp_error
d_internal_smtp_sasl_failure(
    struct d_internal_smtp_session* _session,
    enum d_smtp_error               _result
)
{
    if (_result == D_SMTP_ERROR_AUTH)
    {
        return d_internal_smtp_fault(_session,
                                     501u,
                                     "5.7.0",
                                     "Authentication cancelled");
    }

    if ( (_result == D_SMTP_ERROR_PROTOCOL) ||
         (_result == D_SMTP_ERROR_TOO_LONG) )
    {
        return d_internal_smtp_fault(_session,
                                     501u,
                                     "5.5.2",
                                     "Cannot decode response");
    }

    return _result;
}

/*
d_internal_smtp_verify
  Asks the handler, records the identity on success, and counts a refusal
as an error so that password guessing ends in a dropped connection.
*/
static enum d_smtp_error
d_internal_smtp_verify(
    struct d_internal_smtp_session* _session,
    const char*                     _user,
    const char*                     _secret
)
{
    const struct d_smtp_server_handler* handler = _session->handler;
    const size_t                        length  = strlen(_user);

    if ( (length == 0)                           ||
         (length >= sizeof(_session->user))      ||
         (!d_smtp_text_is_safe(_user, length))   ||
         (!handler->auth(handler->context,
                         _user,
                         _secret)) )
    {
        return d_internal_smtp_fault(_session,
                                     535u,
                                     "5.7.8",
                                     "Authentication credentials invalid");
    }

    memcpy(_session->user,
           _user,
           length + 1u);
    _session->info.user = _session->user;

    return d_internal_smtp_respond(_session,
                                   235u,
                                   "2.7.0",
                                   "Authentication successful");
}

/*
d_internal_smtp_plain_check
  Splits a decoded PLAIN message, "authzid NUL authcid NUL password". Proxy
authorization is not supported, so an authorization identity must be empty
or equal to the user.
*/
static enum d_smtp_error
d_internal_smtp_plain_check(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_buffer*     _decoded
)
{
    const char* data   = _decoded->data;
    const char* end    = data + _decoded->length;
    const char* first  = memchr(data,
                                '\0',
                                _decoded->length);
    const char* second = (first) ? memchr(first + 1,
                                          '\0',
                                          (size_t)(end - first - 1))
                                 : NULL;

    if ( (!second) ||
         (memchr(second + 1, '\0', (size_t)(end - second - 1))) )
    {
        return d_internal_smtp_fault(_session,
                                     501u,
                                     "5.5.2",
                                     "Malformed PLAIN response");
    }

    if ( (first != data) &&
         (strcmp(data, first + 1) != 0) )
    {
        return d_internal_smtp_fault(_session,
                                     535u,
                                     "5.7.8",
                                     "Authorization identity refused");
    }

    return d_internal_smtp_verify(_session,
                                  first + 1,
                                  second + 1);
}

/*
d_internal_smtp_auth_plain
  PLAIN, with the message as the initial response or, failing that, after
an empty challenge.
*/
static enum d_smtp_error
d_internal_smtp_auth_plain(
    struct d_internal_smtp_session* _session,
    const char*                     _initial,
    size_t                          _length
)
{
    char                 storage[D_SMTP_LINE_MAX] = { 0 };
    struct d_smtp_buffer decoded = { .data     = storage,
                                     .capacity = sizeof(storage),
                                     .length   = 0 };
    enum d_smtp_error    result  = (_initial)
                                   ? d_internal_smtp_sasl_decode(_initial,
                                                                 _length,
                                                                 &decoded)
                                   : d_internal_smtp_sasl_read(_session,
                                                               "",
                                                               &decoded);

    result = (result == D_SMTP_OK)
             ? d_internal_smtp_plain_check(_session,
                                           &decoded)
             : d_internal_smtp_sasl_failure(_session,
                                            result);

    for (size_t i = 0; i < sizeof(storage); ++i)
    {
        ((volatile char*)storage)[i] = 0;
    }

    return result;
}

/*
d_internal_smtp_auth_login
  LOGIN: the user (possibly as an initial response), then the password, each
prompted with the traditional base64 of "Username:" and "Password:".
*/
static enum d_smtp_error
d_internal_smtp_auth_login(
    struct d_internal_smtp_session* _session,
    const char*                     _initial,
    size_t                          _length
)
{
    char                 user_storage[D_INTERNAL_SMTP_USER_MAX] = { 0 };
    char                 secret_storage[D_SMTP_LINE_MAX]        = { 0 };
    struct d_smtp_buffer user   = { .data     = user_storage,
                                    .capacity = sizeof(user_storage),
                                    .length   = 0 };
    struct d_smtp_buffer secret = { .data     = secret_storage,
                                    .capacity = sizeof(secret_storage),
                                    .length   = 0 };
    enum d_smtp_error    result = (_initial)
                                  ? d_internal_smtp_sasl_decode(_initial,
                                                                _length,
                                                                &user)
                                  : d_internal_smtp_sasl_read(_session,
                                                              "VXNlcm5hbWU6",
                                                              &user);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_sasl_read(_session,
                                           "UGFzc3dvcmQ6",
                                           &secret);
    }

    if (result != D_SMTP_OK)
    {
        result = d_internal_smtp_sasl_failure(_session,
                                              result);
    }
    else if ( (strlen(user.data) != user.length) ||
              (strlen(secret.data) != secret.length) )
    {
        result = d_internal_smtp_fault(_session,
                                       501u,
                                       "5.5.2",
                                       "Malformed LOGIN response");
    }
    else
    {
        result = d_internal_smtp_verify(_session,
                                        user.data,
                                        secret.data);
    }

    for (size_t i = 0; i < sizeof(secret_storage); ++i)
    {
        ((volatile char*)secret_storage)[i] = 0;
    }

    return result;
}

/*
d_internal_smtp_on_auth
  AUTH follows EHLO, precedes any transaction, and succeeds once (RFC 4954
section 4). Offered only over TLS unless waived; asked for anyway, it draws
538, the code RFC 4954 reserves for "encryption required".
*/
static enum d_smtp_error
d_internal_smtp_on_auth(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_command*    _command
)
{
    const unsigned int offered = d_internal_smtp_auth_offer(_session);
    const char*        text    = _command->argument;
    const size_t       length  = _command->argument_length;
    size_t             name    = 0;

    if ( (!_session->extended) ||
         (_session->in_mail)   ||
         (_session->info.user) )
    {
        return d_internal_smtp_fault(_session,
                                     503u,
                                     "5.5.1",
                                     "Bad sequence of commands");
    }

    if (!_session->handler->auth)
    {
        return d_internal_smtp_fault(_session,
                                     502u,
                                     "5.5.1",
                                     "AUTH not available");
    }

    if (offered == D_SMTP_AUTH_NONE)
    {
        return d_internal_smtp_fault(_session,
                                     538u,
                                     "5.7.11",
                                     "Encryption required for requested "
                                     "authentication mechanism");
    }

    while ( (name < length) &&
            (text[name] != ' ') )
    {
        ++name;
    }

    const unsigned int mechanism = (unsigned int)d_smtp_auth_from_name(text,
                                                                       name);
    const char*        initial   = (name < length) ? text + name + 1 : NULL;
    const size_t       rest      = (name < length) ? length - name - 1 : 0;

    if ((mechanism & offered) == 0u)
    {
        return d_internal_smtp_fault(_session,
                                     504u,
                                     "5.5.4",
                                     "Unrecognized authentication type");
    }

    return (mechanism == D_SMTP_AUTH_PLAIN)
           ? d_internal_smtp_auth_plain(_session,
                                        initial,
                                        rest)
           : d_internal_smtp_auth_login(_session,
                                        initial,
                                        rest);
}

/*
d_internal_smtp_mail_parameter
  One MAIL parameter. Every parameter belongs to an extension, so none is
valid after HELO; SIZE is checked against the limit here, before any data
is sent (RFC 1870).
*/
static struct d_internal_smtp_verdict
d_internal_smtp_mail_parameter(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_parameter*  _parameter
)
{
    const struct d_internal_smtp_verdict accepted = { 0u, NULL, NULL };
    const struct d_internal_smtp_verdict unknown  = { 555u,
                                                      "5.5.4",
                                                      "Unsupported MAIL "
                                                      "parameter" };
    const char*  keyword = _parameter->keyword;
    const size_t length  = _parameter->keyword_length;
    const char*  value   = _parameter->value;
    const size_t size    = _parameter->value_length;

    if (!_session->extended)
    {
        return unknown;
    }

    if ( (value) &&
         (d_smtp_keyword_is(keyword, length, "BODY")) )
    {
        _session->info.eight_bit = d_smtp_keyword_is(value, size, "8BITMIME");

        return ( (_session->info.eight_bit) ||
                 (d_smtp_keyword_is(value, size, "7BIT")) ) ? accepted
                                                            : unknown;
    }

    if ( (!value) &&
         (d_smtp_keyword_is(keyword, length, "SMTPUTF8")) )
    {
        _session->info.utf8 = true;

        return accepted;
    }

    if ( (!value) ||
         (!d_smtp_keyword_is(keyword, length, "SIZE")) )
    {
        return unknown;
    }

    size_t declared = 0;

    for (size_t i = 0; i < size; ++i)
    {
        const bool digit = ( (value[i] >= '0') &&
                             (value[i] <= '9') );

        if ( (!digit) ||
             (declared > (SIZE_MAX - 9u) / 10u) )
        {
            return unknown;
        }

        declared = (declared * 10u) + (size_t)(value[i] - '0');
    }

    if (declared > _session->config->max_message_size)
    {
        const struct d_internal_smtp_verdict oversize =
            { 552u,
              "5.3.4",
              "Message size exceeds fixed maximum message size" };

        return oversize;
    }

    return (size > 0) ? accepted : unknown;
}

/*
d_internal_smtp_on_mail
  MAIL FROM: sequencing and policy first, then syntax, parameters, the
sender itself, and finally the handler's decision.
*/
static enum d_smtp_error
d_internal_smtp_on_mail(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_command*    _command
)
{
    const struct d_smtp_server_config*  config  = _session->config;
    const struct d_smtp_server_handler* handler = _session->handler;
    struct d_smtp_path                  path    = { .address = NULL };

    if ( (!_session->greeted) ||
         (_session->in_mail) )
    {
        return d_internal_smtp_fault(_session,
                                     503u,
                                     "5.5.1",
                                     "Bad sequence of commands");
    }

    if ( ( (config->require_tls) &&
           (!_session->info.secure) ) ||
         ( (config->require_auth) &&
           (!_session->info.user) ) )
    {
        return d_internal_smtp_fault(_session,
                                     530u,
                                     "5.7.0",
                                     (config->require_tls &&
                                      !_session->info.secure)
                                     ? "Must issue a STARTTLS command first"
                                     : "Authentication required");
    }

    if (d_smtp_path_parse(_command->argument,
                          _command->argument_length,
                          "FROM:",
                          &path) != D_SMTP_OK)
    {
        return d_internal_smtp_fault(_session,
                                     501u,
                                     "5.5.4",
                                     "Syntax: MAIL FROM:<address>");
    }

    d_internal_smtp_reset(_session);

    const char*             cursor    = path.parameters;
    const char*             end       = (cursor) ? cursor +
                                                   path.parameters_length
                                                 : NULL;
    struct d_smtp_parameter parameter = { .keyword = NULL };
    struct d_internal_smtp_verdict verdict = { 0u, NULL, NULL };

    while ( (verdict.code == 0) &&
            (cursor) &&
            (d_smtp_parameter_next(&cursor, end, &parameter)) )
    {
        verdict = d_internal_smtp_mail_parameter(_session,
                                                 &parameter);
    }

    // the null reverse-path is legal; anything else must be a mailbox
    if ( (verdict.code == 0)       &&
         (path.address_length > 0) &&
         (!d_smtp_address_is_valid(path.address,
                                   path.address_length,
                                   _session->info.utf8)) )
    {
        verdict.code   = 553u;
        verdict.status = "5.1.7";
        verdict.text   = "Bad sender address syntax";
    }

    if (verdict.code == 0)
    {
        memcpy(_session->sender,
               path.address,
               path.address_length);
        _session->sender[path.address_length] = '\0';

        verdict = d_internal_smtp_decide(
                      (handler->mail) ? handler->mail(handler->context,
                                                      &_session->info,
                                                      _session->sender)
                                      : 0u,
                      "5.7.1",
                      "4.7.1",
                      "Sender rejected");
    }

    if (verdict.code != 0)
    {
        d_internal_smtp_reset(_session);

        return d_internal_smtp_refuse(_session,
                                      &verdict);
    }

    _session->in_mail     = true;
    _session->info.sender = _session->sender;

    return d_internal_smtp_respond(_session,
                                   250u,
                                   "2.1.0",
                                   "Sender OK");
}

/*
d_internal_smtp_recipient_valid
  A mailbox, or the bare "postmaster" every server must accept (RFC 5321
section 4.5.1).
*/
static bool
d_internal_smtp_recipient_valid(
    const struct d_internal_smtp_session* _session,
    const struct d_smtp_path*             _path
)
{
    return ( (d_smtp_keyword_is(_path->address,
                                _path->address_length,
                                "postmaster")) ||
             (d_smtp_address_is_valid(_path->address,
                                      _path->address_length,
                                      _session->info.utf8)) );
}

/*
d_internal_smtp_on_rcpt
  RCPT TO: no RCPT parameters are offered (DSN is not advertised), and the
recipient limit answers 452, which tells the client to retry the rest in a
later transaction (RFC 5321 section 4.5.3.1.10).
*/
static enum d_smtp_error
d_internal_smtp_on_rcpt(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_command*    _command
)
{
    const struct d_smtp_server_handler* handler = _session->handler;
    struct d_smtp_path                  path    = { .address = NULL };

    if (!_session->in_mail)
    {
        return d_internal_smtp_fault(_session,
                                     503u,
                                     "5.5.1",
                                     "Need MAIL before RCPT");
    }

    if ( (d_smtp_path_parse(_command->argument,
                            _command->argument_length,
                            "TO:",
                            &path) != D_SMTP_OK) ||
         (path.parameters) )
    {
        return d_internal_smtp_fault(_session,
                                     501u,
                                     "5.5.4",
                                     "Syntax: RCPT TO:<address>");
    }

    if (!d_internal_smtp_recipient_valid(_session, &path))
    {
        return d_internal_smtp_fault(_session,
                                     553u,
                                     "5.1.3",
                                     "Bad recipient address syntax");
    }

    const size_t count = _session->info.recipient_count;

    if (count >= _session->config->max_recipients)
    {
        return d_internal_smtp_respond(_session,
                                       452u,
                                       "4.5.3",
                                       "Too many recipients");
    }

    char* slot = _session->recipient_storage + (count * D_SMTP_PATH_MAX);

    memcpy(slot,
           path.address,
           path.address_length);
    slot[path.address_length] = '\0';

    const struct d_internal_smtp_verdict verdict = d_internal_smtp_decide(
        (handler->rcpt) ? handler->rcpt(handler->context,
                                        &_session->info,
                                        slot)
                        : 0u,
        "5.1.1",
        "4.2.1",
        "Recipient rejected");

    if (verdict.code != 0)
    {
        return d_internal_smtp_refuse(_session,
                                      &verdict);
    }

    _session->recipient_list[count] = slot;
    _session->info.recipient_count  = count + 1u;

    return d_internal_smtp_respond(_session,
                                   250u,
                                   "2.1.5",
                                   "Recipient OK");
}

/*
d_internal_smtp_reserve
  Grows the message buffer by doubling, never past what the size limit can
use, so a session holds memory in proportion to what it actually received.
*/
static bool
d_internal_smtp_reserve(
    struct d_internal_smtp_session* _session,
    size_t                          _capacity
)
{
    if (_capacity <= _session->message_capacity)
    {
        return true;
    }

    const size_t ceiling = _session->config->max_message_size + 1u;
    size_t       grown   = (_session->message_capacity > 0)
                           ? _session->message_capacity
                           : D_INTERNAL_SMTP_MESSAGE_START;

    while (grown < _capacity)
    {
        grown = (grown > SIZE_MAX / 2u) ? _capacity : grown * 2u;
    }

    if ( (ceiling >= _capacity) &&
         (grown > ceiling) )
    {
        grown = ceiling;
    }

    char* resized = realloc(_session->message,
                            grown);

    if (!resized)
    {
        return false;
    }

    _session->message          = resized;
    _session->message_capacity = grown;

    return true;
}

/*
d_internal_smtp_store
  Appends one data line and its CRLF. Line endings are thereby normalized:
a line that arrived with a bare LF is stored with CRLF like any other.
*/
static void
d_internal_smtp_store(
    struct d_internal_smtp_session* _session,
    const char*                     _payload,
    size_t                          _length,
    struct d_internal_smtp_intake*  _intake
)
{
    const size_t needed = _session->message_length + _length + 2u;

    if ( (_intake->oversize) ||
         (_intake->overlong) ||
         (_intake->no_memory) )
    {
        return;
    }

    if (needed > _session->config->max_message_size)
    {
        _intake->oversize = true;

        return;
    }

    if (!d_internal_smtp_reserve(_session, needed + 1u))
    {
        _intake->no_memory = true;

        return;
    }

    char* cursor = _session->message + _session->message_length;

    if (_length > 0)
    {
        memcpy(cursor,
               _payload,
               _length);
    }

    cursor[_length]          = '\r';
    cursor[_length + 1u]     = '\n';
    cursor[_length + 2u]     = '\0';
    _session->message_length = needed;

    return;
}

/*
d_internal_smtp_receive
  Reads message data up to the end marker. Only a "." line terminated by
CRLF and preceded by a CRLF ends the data; a dot framed by a bare LF is
content. That removes the disagreement between servers that SMTP smuggling
exploits.
*/
static enum d_smtp_error
d_internal_smtp_receive(
    struct d_internal_smtp_session* _session,
    struct d_internal_smtp_intake*  _intake
)
{
    bool previous_crlf = true;

    for (;;)
    {
        size_t                  length = 0;
        const char*             data   = NULL;
        size_t                  size   = 0;
        const enum d_smtp_error read   = d_smtp_reader_line(
                                             &_session->reader,
                                             _session->line,
                                             sizeof(_session->line),
                                             &length);

        if ( (read != D_SMTP_OK) &&
             (read != D_SMTP_ERROR_TOO_LONG) )
        {
            return read;
        }

        const bool marker = d_smtp_data_unstuff(_session->line,
                                                length,
                                                &data,
                                                &size);

        if ( (read == D_SMTP_OK)       &&
             (marker)                  &&
             (previous_crlf)           &&
             (_session->reader.crlf) )
        {
            return D_SMTP_OK;
        }

        if (read == D_SMTP_ERROR_TOO_LONG)
        {
            _intake->overlong = true;
        }

        // a loosely framed marker is stored as the content it is
        d_internal_smtp_store(_session,
                              (marker) ? _session->line : data,
                              (marker) ? length : size,
                              _intake);

        previous_crlf = _session->reader.crlf;
    }
}

/*
d_internal_smtp_judge
  The verdict on received data: limits first, then the handler.
*/
static struct d_internal_smtp_verdict
d_internal_smtp_judge(
    struct d_internal_smtp_session*      _session,
    const struct d_internal_smtp_intake* _intake
)
{
    const struct d_smtp_server_handler* handler = _session->handler;

    if (_intake->oversize)
    {
        const struct d_internal_smtp_verdict oversize =
            { 552u,
              "5.3.4",
              "Message size exceeds fixed maximum message size" };

        return oversize;
    }

    if ( (_intake->overlong) ||
         (_intake->no_memory) )
    {
        const struct d_internal_smtp_verdict failed =
            (_intake->overlong)
            ? (struct d_internal_smtp_verdict){ 500u, "5.5.2", "Line too long" }
            : (struct d_internal_smtp_verdict){ 452u, "4.3.1",
                                                "Insufficient system storage" };

        return failed;
    }

    return d_internal_smtp_decide(
               (handler->message)
               ? handler->message(handler->context,
                                  &_session->info,
                                  (_session->message) ? _session->message : "",
                                  _session->message_length)
               : 0u,
               "5.6.0",
               "4.3.0",
               "Message rejected");
}

/*
d_internal_smtp_on_data
  DATA with no recipient accepted draws 554, the reply RFC 2920 requires so
that a pipelining client which already sent DATA knows no data should follow.
The transaction ends with the final reply, whatever it is.
*/
static enum d_smtp_error
d_internal_smtp_on_data(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_command*    _command
)
{
    struct d_internal_smtp_intake intake = { .oversize  = false,
                                             .overlong  = false,
                                             .no_memory = false };

    if (_command->argument_length > 0)
    {
        return d_internal_smtp_fault(_session,
                                     501u,
                                     "5.5.4",
                                     "Syntax error, no parameters allowed");
    }

    if (!_session->in_mail)
    {
        return d_internal_smtp_fault(_session,
                                     503u,
                                     "5.5.1",
                                     "Need MAIL before DATA");
    }

    if (_session->info.recipient_count == 0)
    {
        return d_internal_smtp_respond(_session,
                                       554u,
                                       "5.5.1",
                                       "No valid recipients");
    }

    enum d_smtp_error result = d_internal_smtp_respond(
                                   _session,
                                   354u,
                                   NULL,
                                   "Start mail input; end with <CRLF>.<CRLF>");

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_receive(_session,
                                         &intake);
    }

    if (result != D_SMTP_OK)
    {
        return result;
    }

    const struct d_internal_smtp_verdict verdict = d_internal_smtp_judge(
                                                       _session,
                                                       &intake);

    d_internal_smtp_reset(_session);

    return (verdict.code != 0) ? d_internal_smtp_refuse(_session,
                                                        &verdict)
                               : d_internal_smtp_respond(_session,
                                                         250u,
                                                         "2.0.0",
                                                         "Message accepted");
}

/*
d_internal_smtp_dispatch
  One command, by verb. VRFY answers 252 -- "cannot verify, but will try" --
as RFC 5321 section 3.5.3 recommends for a server that does not disclose its
users.
*/
static enum d_smtp_error
d_internal_smtp_dispatch(
    struct d_internal_smtp_session* _session,
    const struct d_smtp_command*    _command
)
{
    switch (_command->verb)
    {
        case D_SMTP_VERB_EHLO:
        case D_SMTP_VERB_HELO:
            return d_internal_smtp_on_hello(_session,
                                            _command,
                                            (_command->verb ==
                                             D_SMTP_VERB_EHLO));

        case D_SMTP_VERB_STARTTLS:
            return d_internal_smtp_on_starttls(_session, _command);

        case D_SMTP_VERB_AUTH:
            return d_internal_smtp_on_auth(_session, _command);

        case D_SMTP_VERB_MAIL:
            return d_internal_smtp_on_mail(_session, _command);

        case D_SMTP_VERB_RCPT:
            return d_internal_smtp_on_rcpt(_session, _command);

        case D_SMTP_VERB_DATA:
            return d_internal_smtp_on_data(_session, _command);

        case D_SMTP_VERB_RSET:
            d_internal_smtp_reset(_session);

            return d_internal_smtp_respond(_session,
                                           250u,
                                           "2.0.0",
                                           "Reset state");

        case D_SMTP_VERB_NOOP:
            return d_internal_smtp_respond(_session,
                                           250u,
                                           "2.0.0",
                                           "OK");

        case D_SMTP_VERB_QUIT:
            _session->quit = true;

            return d_internal_smtp_respond(_session,
                                           221u,
                                           "2.0.0",
                                           "Closing connection");

        case D_SMTP_VERB_VRFY:
            return d_internal_smtp_respond(_session,
                                           252u,
                                           "2.5.0",
                                           "Cannot VRFY user, but will "
                                           "accept message");

        case D_SMTP_VERB_HELP:
            return d_internal_smtp_respond(_session,
                                           214u,
                                           "2.0.0",
                                           "See RFC 5321");

        case D_SMTP_VERB_UNKNOWN:
        default:
            break;
    }

    return d_internal_smtp_fault(_session,
                                 500u,
                                 "5.5.2",
                                 "Command not recognized");
}

/*
d_internal_smtp_next
  Reads and runs one command. An idle client is told why before it is
dropped (RFC 5321 section 4.5.3.2).
*/
static enum d_smtp_error
d_internal_smtp_next(
    struct d_internal_smtp_session* _session
)
{
    size_t                  length  = 0;
    struct d_smtp_command   command = { .verb = D_SMTP_VERB_UNKNOWN };
    const enum d_smtp_error read    = d_smtp_reader_line(&_session->reader,
                                                         _session->line,
                                                         sizeof(_session->line),
                                                         &length);

    if (read == D_SMTP_ERROR_TOO_LONG)
    {
        return d_internal_smtp_fault(_session,
                                     500u,
                                     "5.5.2",
                                     "Line too long");
    }

    if (read == D_SMTP_ERROR_TIMEOUT)
    {
        (void)d_internal_smtp_respond(_session,
                                      421u,
                                      "4.4.2",
                                      "Timeout, closing connection");
    }

    if (read != D_SMTP_OK)
    {
        return read;
    }

    (void)d_smtp_command_parse(_session->line,
                               length,
                               &command);

    return d_internal_smtp_dispatch(_session,
                                    &command);
}

/*
d_internal_smtp_session_new
  Allocates a session and its recipient storage in one go, all or nothing.
*/
static struct d_internal_smtp_session*
d_internal_smtp_session_new(
    const struct d_smtp_transport*      _transport,
    const struct d_smtp_server_config*  _config,
    const struct d_smtp_server_handler* _handler
)
{
    struct d_internal_smtp_session* session = calloc(1, sizeof(*session));
    char*                           storage = (session)
                                              ? calloc(_config->max_recipients,
                                                       D_SMTP_PATH_MAX)
                                              : NULL;
    const char**                    list    = (storage)
                                              ? calloc(_config->max_recipients,
                                                       sizeof(*list))
                                              : NULL;

    if (!list)
    {
        free(storage);
        free(session);

        return NULL;
    }

    d_smtp_reader_init(&session->reader,
                       _transport);
    session->config               = _config;
    session->handler              = _handler;
    session->recipient_storage    = storage;
    session->recipient_list       = list;
    session->info.client_domain   = session->client_domain;
    session->info.recipients      = list;
    session->info.secure          = _config->secure;

    return session;
}

/*
d_internal_smtp_session_free
  Releases a session and everything it allocated.
*/
static void
d_internal_smtp_session_free(
    struct d_internal_smtp_session* _session
)
{
    free(_session->message);
    free(_session->recipient_storage);
    free((void*)_session->recipient_list);
    free(_session);

    return;
}

void
d_smtp_server_config_init(
    struct d_smtp_server_config* _config
)
{
    // parameter validation first
    if (!_config)
    {
        return;
    }

    _config->domain              = "localhost";
    _config->max_message_size    = D_SMTP_SERVER_MESSAGE_MAX;
    _config->max_recipients      = D_SMTP_SERVER_RECIPIENTS_MAX;
    _config->auth_mechanisms     = D_SMTP_AUTH_PLAIN | D_SMTP_AUTH_LOGIN;
    _config->require_tls         = false;
    _config->require_auth        = false;
    _config->allow_insecure_auth = false;
    _config->secure              = false;
    d_smtp_tls_options_init(&_config->tls);

    return;
}

void
d_smtp_server_handler_init(
    struct d_smtp_server_handler* _handler
)
{
    // parameter validation first
    if (!_handler)
    {
        return;
    }

    _handler->context = NULL;
    _handler->mail    = NULL;
    _handler->rcpt    = NULL;
    _handler->message = NULL;
    _handler->auth    = NULL;

    return;
}

/*
d_smtp_server_session
  Greets, then runs one command per turn until QUIT or until the connection
ends. The session never closes the transport: the caller that opened it
decides its fate.
*/
enum d_smtp_error
d_smtp_server_session(
    const struct d_smtp_transport*      _transport,
    const struct d_smtp_server_config*  _config,
    const struct d_smtp_server_handler* _handler
)
{
    // parameter validation first
    if ( (!_transport)                     ||
         (!_transport->read)               ||
         (!_transport->write)              ||
         (!_config)                        ||
         (!_handler)                       ||
         (!_config->domain)                ||
         (_config->max_message_size == 0)  ||
         (_config->max_recipients == 0)    ||
         (!d_smtp_domain_is_valid(_config->domain,
                                  strlen(_config->domain),
                                  false)) )
    {
        return D_SMTP_ERROR_INVALID;
    }

    struct d_internal_smtp_session* session = d_internal_smtp_session_new(
                                                  _transport,
                                                  _config,
                                                  _handler);

    if (!session)
    {
        return D_SMTP_ERROR_NO_MEMORY;
    }

    char                 storage[D_SMTP_COMMAND_LINE_MAX] = { 0 };
    struct d_smtp_buffer text   = { .data     = storage,
                                    .capacity = sizeof(storage),
                                    .length   = 0 };
    enum d_smtp_error    result = d_internal_smtp_join(&text,
                                                       _config->domain,
                                                       " ESMTP ready",
                                                       NULL);

    if (result == D_SMTP_OK)
    {
        result = d_internal_smtp_respond(session,
                                         220u,
                                         NULL,
                                         text.data);
    }

    while ( (result == D_SMTP_OK) &&
            (!session->quit) )
    {
        result = d_internal_smtp_next(session);
    }

    d_internal_smtp_session_free(session);

    return result;
}
