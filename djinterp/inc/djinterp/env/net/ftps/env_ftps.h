/*******************************************************************************
* djinterp [env]                                                      env_ftps.h
*
* FTPS environment detection.
*   What the FTP client needs to reach a server over TLS (RFC 4217),
* explicitly with AUTH TLS or implicitly on port 990. FTPS is FTP secured,
* so this file builds on env_ftp.h, whose transport it shares, and adds only
* what TLS needs: an engine the SSL kernel can be built with, as env_ssl.h
* finds one. The kernel itself is plain C with no library of its own; the
* engine is what a caller's TLS context brings at run time.
*   Include it after the framework root, as env_tcp.h requires.
*
*
* path:      /inc/djinterp/env/net/ftps/env_ftps.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PREREQUISITES
    -------------
    1.  Layers
         1.  D_ENV_FTPS_HAS_TLS
2.  CAPABILITIES
    ------------
    1.  Summaries
         1.  D_ENV_FTPS_CAN_CLIENT
*/

#ifndef DJINTERP_ENV_NET_FTPS_ENV_FTPS_H
#define DJINTERP_ENV_NET_FTPS_ENV_FTPS_H 1

// djinterp
#include "../ftp/env_ftp.h"  // D_ENV_FTP_CAN_CLIENT
#include "../ssl/env_ssl.h"  // D_ENV_SSL_AVAILABLE


//==============================================================================
// 1.  PREREQUISITES
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier wins. The transport is
// env_ftp.h's; only TLS is added here.


// 1.1    Layers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_FTPS_HAS_TLS
//   flag: 1 when TLS can run here, as env_ssl.h judges it: an engine the
// SSL kernel can be built with -- today, over an OpenSSL-family library --
// and a TCP stream to carry it.
#ifndef D_ENV_FTPS_HAS_TLS
    #if D_ENV_SSL_AVAILABLE
        #define D_ENV_FTPS_HAS_TLS 1
    #else
        #define D_ENV_FTPS_HAS_TLS 0
    #endif
#endif  // D_ENV_FTPS_HAS_TLS


//==============================================================================
// 2.  CAPABILITIES
//==============================================================================
// What the prerequisites add up to. Each is #ifndef-guarded.


// 2.1    Summaries
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_FTPS_CAN_CLIENT
//   flag: 1 when the FTP client can reach a server here over TLS: FTP's
// transport, and TLS to secure it.
#ifndef D_ENV_FTPS_CAN_CLIENT
    #if ( (D_ENV_FTP_CAN_CLIENT) &&                                            \
          (D_ENV_FTPS_HAS_TLS) )
        #define D_ENV_FTPS_CAN_CLIENT 1
    #else
        #define D_ENV_FTPS_CAN_CLIENT 0
    #endif
#endif  // D_ENV_FTPS_CAN_CLIENT


#endif  // DJINTERP_ENV_NET_FTPS_ENV_FTPS_H
