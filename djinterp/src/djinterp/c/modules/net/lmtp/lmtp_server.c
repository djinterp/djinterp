/*******************************************************************************
* djinterp [c][net]                                                lmtp_server.c
*
* Implementation of the LMTP server declared in lmtp_server.h: the kernel's
* session engine, held to LMTP's dialect. LHLO, the 503, and the reply for
* each recipient are the SMTP kernel's (smtp_server.c), which serves both.
*
*
* path:      /src/djinterp/c/modules/net/lmtp/lmtp_server.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.24
*                                                            revised: 2026.11.24
*******************************************************************************/
#include "../../../../../../inc/djinterp/c/modules/net/lmtp/lmtp_server.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// djinterp
#include "../../../../../../inc/djinterp/c/modules/net/net_common.h"         // dj_net_connection
#include "../../../../../../inc/djinterp/c/modules/net/smtp/smtp_common.h"   // dj_smtp_dialect
#include "../../../../../../inc/djinterp/c/modules/net/smtp/smtp_server.h"   // the kernel's engine


/*
d_lmtp_server_config_init
  The kernel's defaults, in LMTP's dialect.
*/
void d_lmtp_server_config_init(
    struct dj_smtp_server_config* _config
)
{
    // parameter validation first
    if (!_config)
    {
        return;
    }

    d_smtp_server_config_init(_config);
    _config->dialect = D_SMTP_DIALECT_LMTP;

    return;
}

/*
d_lmtp_server_session
  The kernel's session, on a copy of the configuration that names LMTP;
the copy borrows what the configuration borrows.
*/
enum dj_smtp_error
d_lmtp_server_session(
    struct dj_net_connection*            _connection,
    const struct dj_smtp_server_config*  _config,
    const struct dj_smtp_server_handler* _handler
)
{
    // parameter validation first
    if (!_config)
    {
        return D_SMTP_ERROR_INVALID_ARGUMENT;
    }

    struct dj_smtp_server_config config = *_config;

    config.dialect = D_SMTP_DIALECT_LMTP;

    return d_smtp_server_session(_connection,
                                 &config,
                                 _handler);
}
