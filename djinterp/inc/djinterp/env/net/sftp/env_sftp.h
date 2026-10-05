/*******************************************************************************
* djinterp [env]                                                      env_sftp.h
*
* SFTP environment detection.
*   What the SFTP client needs to reach a server: SFTP rides the "sftp"
* subsystem of an SSH channel, so it needs an SSH connection, which env_ssh.h
* judges -- a usable SSH library and a TCP transport. The codec and client
* contain no OS code of their own, and over any other byte stream
* (d_sftp_client_open_transport) they need nothing from here at all.
*   Include it after the framework root.
*
*
* path:      /inc/djinterp/env/net/sftp/env_sftp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PREREQUISITES
    -------------
    1.  Layers
         1.  D_ENV_SFTP_HAS_SSH
2.  CAPABILITIES
    ------------
    1.  Summaries
         1.  D_ENV_SFTP_CAN_CLIENT
*/

#ifndef DJINTERP_ENV_NET_SFTP_ENV_SFTP_H
#define DJINTERP_ENV_NET_SFTP_ENV_SFTP_H 1

// djinterp
#include "../ssh/env_ssh.h"  // D_ENV_SSH_AVAILABLE


//==============================================================================
// 1.  PREREQUISITES
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier wins.


// 1.1    Layers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_SFTP_HAS_SSH
//   flag: 1 when an SSH connection can be made here, as env_ssh.h judges it:
// a usable library -- libssh2, libssh, or wolfSSH -- and a TCP transport.
#ifndef D_ENV_SFTP_HAS_SSH
    #if D_ENV_SSH_AVAILABLE
        #define D_ENV_SFTP_HAS_SSH 1
    #else
        #define D_ENV_SFTP_HAS_SSH 0
    #endif
#endif  // D_ENV_SFTP_HAS_SSH


//==============================================================================
// 2.  CAPABILITIES
//==============================================================================
// What the prerequisites add up to. Each is #ifndef-guarded.


// 2.1    Summaries
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_SFTP_CAN_CLIENT
//   flag: 1 when the SFTP client can reach a server here over SSH.
#ifndef D_ENV_SFTP_CAN_CLIENT
    #define D_ENV_SFTP_CAN_CLIENT D_ENV_SFTP_HAS_SSH
#endif  // D_ENV_SFTP_CAN_CLIENT


#endif  // DJINTERP_ENV_NET_SFTP_ENV_SFTP_H
