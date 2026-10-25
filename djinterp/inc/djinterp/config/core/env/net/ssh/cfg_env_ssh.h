/*******************************************************************************
* djinterp [config]                                                cfg_env_ssh.h
*
* Configuration for env_ssh.h, the SSH library detection header under
* env/net/ssh/.
*   Resolves the one switch that decides whether env_ssh.h includes the
* libraries' own headers to read their versions (decision 105 of the
* register, as decision 60 did for TLS).
*   targets:  env/net/ssh/env_ssh.h -> D_CFG_ENV_SSH_PROBE_VERSION
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/net/ssh/cfg_env_ssh.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#ifndef DJINTERP_CONFIG_CORE_ENV_NET_SSH_CFG_ENV_SSH_H
#define DJINTERP_CONFIG_CORE_ENV_NET_SSH_CFG_ENV_SSH_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_SSH_PROBE_VERSION
//   configuration: 1, the default, has env_ssh.h read library versions by
// including <libssh2.h> and <libssh/libssh_version.h>; 0 keeps both out of
// every translation unit that includes it, leaving each version at 0
// (unknown). On Windows, <libssh2.h> brings <winsock2.h> with it.
#ifndef D_CFG_ENV_SSH_PROBE_VERSION
    #define D_CFG_ENV_SSH_PROBE_VERSION 1
#endif  // D_CFG_ENV_SSH_PROBE_VERSION

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_SSH_PROBE_VERSION)
    #error "D_CFG_ENV_SSH_PROBE_VERSION must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_NET_SSH_CFG_ENV_SSH_H
