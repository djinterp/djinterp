/*******************************************************************************
* djinterp [c][net]                                                lmtp_client.c
*
* Implementation of the LMTP client declared in lmtp_client.h: LMTP's
* defaults, and the kernel's client held to LMTP's dialect. The protocol
* itself - LHLO, the reply for each recipient, port 25 refused - is the SMTP
* kernel's (smtp_client.c), which speaks both.
*
*
* path:      /src/djinterp/c/modules/net/lmtp/lmtp_client.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/net/lmtp/lmtp_client.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t, NULL
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/net_common.h"         // dj_net_connection
#include "../../../../../../inc/djinterp/c/modules/net/smtp/smtp_client.h"   // the kernel's client
#include "../../../../../../inc/djinterp/c/modules/net/smtp/smtp_common.h"   // dj_smtp_dialect


#if (D_INTERNAL_NET_HAS_INT64 == 1)


//==============================================================================
// 1.  SETUP
//==============================================================================

/*
d_lmtp_client_options_init
  The kernel's defaults, then LMTP's: its dialect, no port, and STARTTLS
taken when offered rather than required - LMTP runs between programs on
one host or one trusted network (RFC 2033 section 5), where a server that
offers no STARTTLS is the common case, not a downgrade.
*/
void d_lmtp_client_options_init(
    struct dj_smtp_client_options* _options
)
{
    // parameter validation first
    if (!_options)
    {
        return;
    }

    d_smtp_client_options_init(_options);
    _options->port     = 0u;
    _options->security = D_SMTP_SECURITY_STARTTLS_OPTIONAL;
    _options->dialect  = D_SMTP_DIALECT_LMTP;

    return;
}

/*
d_lmtp_client_init
  A closed kernel client, in LMTP's dialect.
*/
void d_lmtp_client_init(
    struct dj_lmtp_client* _client
)
{
    // parameter validation first
    if (!_client)
    {
        return;
    }

    d_smtp_client_init(&_client->smtp);
    _client->smtp.dialect = D_SMTP_DIALECT_LMTP;

    return;
}


//==============================================================================
// 2.  SESSIONS
//==============================================================================

/*
d_lmtp_client_connect
  The kernel's connect, on a copy of the options that names LMTP: the
kernel refuses port 0 and 25 for it, over TCP.
*/
enum dj_smtp_error
d_lmtp_client_connect(
    struct dj_lmtp_client*               _client,
    const struct dj_smtp_client_options* _options
)
{
    // parameter validation first
    if ( (!_client) ||
         (!_options) )
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    struct dj_smtp_client_options options = *_options;

    options.dialect = D_SMTP_DIALECT_LMTP;

    const enum dj_smtp_error result = d_smtp_client_connect(&_client->smtp,
                                                           &options);

    // a closed client is still an LMTP one
    _client->smtp.dialect = D_SMTP_DIALECT_LMTP;

    return result;
}

/*
d_lmtp_client_attach
  The kernel's attach, on a copy of the options, or of the defaults for
none, that names LMTP.
*/
enum dj_smtp_error
d_lmtp_client_attach(
    struct dj_lmtp_client*                      _client,
    struct dj_net_connection*                   _connection,
    const struct dj_smtp_client_attach_options* _options
)
{
    struct dj_smtp_client_attach_options options = { .secure = false };

    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_options)
    {
        options = *_options;
    }
    else
    {
        d_smtp_client_attach_options_init(&options);
    }

    options.dialect = D_SMTP_DIALECT_LMTP;

    const enum dj_smtp_error result = d_smtp_client_attach_with(
                                         &_client->smtp,
                                         _connection,
                                         &options);

    // a client left closed is still an LMTP one
    if (_client->smtp.state == D_SMTP_CLIENT_CLOSED)
    {
        _client->smtp.dialect = D_SMTP_DIALECT_LMTP;
    }

    return result;
}


//==============================================================================
// 3.  TRANSACTIONS
//==============================================================================

/*
d_lmtp_client_send
  The kernel's send, for a client that speaks LMTP.
*/
enum dj_smtp_error
d_lmtp_client_send(
    struct dj_lmtp_client*         _client,
    const struct dj_smtp_envelope* _envelope,
    const char*                    _message,
    size_t                         _length,
    struct dj_smtp_send_report*    _report
)
{
    // parameter validation first
    if (!_client)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->smtp.dialect != D_SMTP_DIALECT_LMTP)
    {
        return D_SMTP_ERROR_STATE;
    }

    return d_smtp_client_send(&_client->smtp,
                              _envelope,
                              _message,
                              _length,
                              _report);
}

/*
d_lmtp_client_data
  The kernel's data with its replies, for a client that speaks LMTP.
*/
enum dj_smtp_error
d_lmtp_client_data(
    struct dj_lmtp_client*           _client,
    const char*                      _message,
    size_t                           _length,
    struct dj_smtp_recipient_status* _replies,
    size_t                           _capacity,
    size_t*                          _count
)
{
    // parameter validation first
    if (!_client)
    {
        if (_count)
        {
            *_count = 0u;
        }

        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    if (_client->smtp.dialect != D_SMTP_DIALECT_LMTP)
    {
        if (_count)
        {
            *_count = 0u;
        }

        return D_SMTP_ERROR_STATE;
    }

    return d_smtp_client_data_replies(&_client->smtp,
                                      _message,
                                      _length,
                                      _replies,
                                      _capacity,
                                      _count);
}


#endif  // D_INTERNAL_NET_HAS_INT64
