/*******************************************************************************
* djinterp [config]                                               cfg_env_curl.h
*
* Configuration for env_curl.h, the libcurl detection header under
* env/net/curl/.
*   Resolves the one switch that decides whether env_curl.h includes
* <curl/curlver.h> to read libcurl's version (decision 105 of the register,
* as decision 60 did for TLS).
*   targets:  env/net/curl/env_curl.h -> D_CFG_ENV_CURL_PROBE_VERSION
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/net/curl/cfg_env_curl.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#ifndef DJINTERP_CONFIG_CORE_ENV_NET_CURL_CFG_ENV_CURL_H
#define DJINTERP_CONFIG_CORE_ENV_NET_CURL_CFG_ENV_CURL_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_CURL_PROBE_VERSION
//   configuration: 1, the default, has env_curl.h include <curl/curlver.h>
// where it exists; it defines only the version macros, and brings in no
// declarations or linkage. 0 keeps detection from touching any curl header,
// and the version macros then read 0, unknown, unless pre-defined.
#ifndef D_CFG_ENV_CURL_PROBE_VERSION
    #define D_CFG_ENV_CURL_PROBE_VERSION 1
#endif  // D_CFG_ENV_CURL_PROBE_VERSION

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_CURL_PROBE_VERSION)
    #error "D_CFG_ENV_CURL_PROBE_VERSION must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_NET_CURL_CFG_ENV_CURL_H
