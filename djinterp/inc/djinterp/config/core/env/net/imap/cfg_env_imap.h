/*******************************************************************************
* djinterp [config]                                               cfg_env_imap.h
*
* Configuration for env_imap.h, the IMAP library detection header under
* env/net/imap/.
*   Resolves the one switch that decides whether env_imap.h includes the
* version headers libetpan, GNU SASL and VMime publish (decision 105 of the
* register, as decision 60 did for TLS).
*   targets:  env/net/imap/env_imap.h -> D_CFG_ENV_IMAP_PROBE_VERSION
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/net/imap/cfg_env_imap.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#ifndef DJINTERP_CONFIG_CORE_ENV_NET_IMAP_CFG_ENV_IMAP_H
#define DJINTERP_CONFIG_CORE_ENV_NET_IMAP_CFG_ENV_IMAP_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_IMAP_PROBE_VERSION
//   configuration: 1, the default, has env_imap.h include the version headers
// libetpan (libetpan_version.h), GNU SASL (gsasl-version.h) and VMime
// (vmime/config.hpp, C++ only) publish, each where it exists. 0 keeps
// detection from touching any third-party header: the version macros then
// read 0, unknown, unless pre-defined, and VMime reports no IMAP support,
// since its build configuration is where that is recorded.
#ifndef D_CFG_ENV_IMAP_PROBE_VERSION
    #define D_CFG_ENV_IMAP_PROBE_VERSION 1
#endif  // D_CFG_ENV_IMAP_PROBE_VERSION

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_IMAP_PROBE_VERSION)
    #error "D_CFG_ENV_IMAP_PROBE_VERSION must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_NET_IMAP_CFG_ENV_IMAP_H
