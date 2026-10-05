/*******************************************************************************
* djinterp [config]                                                   cfg_curl.h
*
* Build configuration for the libcurl binding, net/curl/curl.h.
*   One knob: whether the binding is built over libcurl or compiled to stubs
* that report D_CURL_STATUS_UNSUPPORTED. It defaults to what env_curl.h
* detected, and to off where the version cannot be read, since the binding
* needs libcurl 7.21.6, the first with CURLOPT_ACCEPT_ENCODING.
*
*   targets:  net/curl/curl.h and curl.c -> D_INTERNAL_CURL
*   requires: cfg_common.h (helpers); env_curl.h (library and version)
*
*
* path:      /inc/djinterp/config/net/curl/cfg_curl.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BINDING
    -------
    1.  Environment-defaulted enable
         1.  D_CFG_CURL
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_CURL
*/

#ifndef DJINTERP_CONFIG_NET_CURL_CFG_CURL_H
#define DJINTERP_CONFIG_NET_CURL_CFG_CURL_H 1

// djinterp
#include "../../cfg_common.h"                // D_CFG_IS_BOOL, D_CFG_NORM
#include "../../../env/net/curl/env_curl.h"  // D_ENV_CURL_AVAILABLE, version


//==============================================================================
// 1.  BINDING
//==============================================================================
// Whether net/curl/curl.h runs over libcurl. Without it every transfer
// reports D_CURL_STATUS_UNSUPPORTED, so code using the binding compiles and
// links everywhere and decides at run time.


// 1.1    Environment-defaulted enable
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_CURL
//   brief: build the binding over libcurl (1) or as stubs (0). Defaults to 1
// where env_curl.h finds libcurl 7.21.6 or later, and to 0 where it finds
// none or cannot read its version (D_CFG_ENV_CURL_PROBE_VERSION set to 0);
// define it as 1 to vouch for such a library.
#ifndef D_CFG_CURL
#   if ( (D_ENV_CURL_AVAILABLE) &&                                             \
         (D_ENV_CURL_VERSION_AT_LEAST(7, 21, 6)) )
#       define D_CFG_CURL 1
#   else
#       define D_CFG_CURL 0
#   endif  // D_ENV_CURL_AVAILABLE
#endif  // D_CFG_CURL


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_CURL)
#   error "cfg_curl: D_CFG_CURL must be 0 or 1"
#endif  // D_CFG_CURL

#if ( (D_CFG_IS_ON(D_CFG_CURL)) &&                                             \
      (!D_ENV_CURL_AVAILABLE) )
#   error "cfg_curl: D_CFG_CURL requires libcurl's headers"
#endif  // D_CFG_CURL


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================
// The effective values modules read. Module code reads these, never the
// D_CFG_CURL knob directly.


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_CURL
//   value: 1 if the binding is built over libcurl.
#define D_INTERNAL_CURL D_CFG_NORM(D_CFG_CURL)


#endif  // DJINTERP_CONFIG_NET_CURL_CFG_CURL_H
