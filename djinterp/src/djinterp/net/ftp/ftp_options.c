/*******************************************************************************
* djinterp [net]                                                   ftp_options.c
*
* Implementation of the client options declared in ftp_options.h.
*
*
* path:      /src/djinterp/net/ftp/ftp_options.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "../../../../inc/djinterp/net/ftp/ftp_options.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// djinterp
#include "../../../../inc/djinterp/net/ftp/ftp_transfer.h"  // D_FTP_TYPE_IMAGE
#include "../../../../inc/djinterp/net/ftp/ftp_endpoint.h"  // data connections
#include "../../../../inc/djinterp/net/ftp/ftp_security.h"  // protection


//==============================================================================
// 2.  CLIENT OPTIONS
//==============================================================================

/*
d_ftp_options_init
  Every field is written explicitly, so the defaults read as a list rather
than hiding in a memset.
*/
void
d_ftp_options_init(
    struct d_ftp_options* _options
)
{
    // parameter validation
    if (!_options)
    {
        return;
    }

    _options->user                = NULL;
    _options->password            = NULL;
    _options->account             = NULL;
    _options->security            = D_FTP_SECURITY_NONE;
    _options->require_security    = true;
    _options->data_protection     = D_FTP_PROTECTION_PRIVATE;
    _options->clear_control       = false;
    _options->data_mode           = D_FTP_DATA_PASSIVE_AUTO;
    _options->type.data_type      = D_FTP_TYPE_IMAGE;
    _options->type.format         = D_FTP_FORMAT_NONE;
    _options->type.byte_size      = 8u;
    _options->ignore_pasv_address = true;
    _options->verify_peer         = true;
    _options->verify_host         = true;
    _options->use_utf8            = true;
    _options->create_directories  = false;
    _options->connect_timeout_ms  = 30000u;
    _options->response_timeout_ms = 60000u;
    _options->idle_timeout_ms     = 0u;

    return;
}
