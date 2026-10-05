/*******************************************************************************
* djinterp [env]                                                       env_ftp.h
*
* FTP environment detection.
*   What the FTP client can be built on: a TCP backend, from env_tcp.h, for
* the control and every data connection. The client itself contains no OS
* code and needs no library of its own. FTP over TLS adds its prerequisites
* in env_ftps.h, which builds on this file.
*   Include it after the framework root, as env_tcp.h requires.
*
*
* path:      /inc/djinterp/env/net/ftp/env_ftp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PREREQUISITES
    -------------
    1.  Layers
         1.  D_ENV_FTP_HAS_TRANSPORT
2.  CAPABILITIES
    ------------
    1.  Summaries
         1.  D_ENV_FTP_CAN_CLIENT
*/

#ifndef DJINTERP_ENV_NET_FTP_ENV_FTP_H
#define DJINTERP_ENV_NET_FTP_ENV_FTP_H 1

// djinterp
#include "../tcp/env_tcp.h"  // D_ENV_TCP_AVAILABLE, D_ENV_TCP_HAS_POLL


//==============================================================================
// 1.  PREREQUISITES
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier wins.


// 1.1    Layers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_FTP_HAS_TRANSPORT
//   flag: 1 when the TCP transport builds its socket backend here -- a
// socket API with poll, the condition cfg_tcp.h defaults D_CFG_TCP by.
// Without it tcp.c builds stubs, and every connection reports the network
// down.
#ifndef D_ENV_FTP_HAS_TRANSPORT
    #if ( (D_ENV_TCP_AVAILABLE) &&                                             \
          (D_ENV_TCP_HAS_POLL) )
        #define D_ENV_FTP_HAS_TRANSPORT 1
    #else
        #define D_ENV_FTP_HAS_TRANSPORT 0
    #endif
#endif  // D_ENV_FTP_HAS_TRANSPORT


//==============================================================================
// 2.  CAPABILITIES
//==============================================================================
// What the prerequisites add up to. Each is #ifndef-guarded.


// 2.1    Summaries
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_FTP_CAN_CLIENT
//   flag: 1 when the FTP client can reach a server here, in the clear.
#ifndef D_ENV_FTP_CAN_CLIENT
    #define D_ENV_FTP_CAN_CLIENT D_ENV_FTP_HAS_TRANSPORT
#endif  // D_ENV_FTP_CAN_CLIENT


#endif  // DJINTERP_ENV_NET_FTP_ENV_FTP_H
