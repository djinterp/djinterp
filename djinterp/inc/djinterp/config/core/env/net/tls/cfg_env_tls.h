/*******************************************************************************
* djinterp [config]                                                cfg_env_tls.h
*
* Configuration for env_tls.h, the TLS library detection header under
* env/net/tls/.
*   Resolves the two switches that decide which libraries' version and
* configuration headers env_tls.h includes to answer its per-library
* capability flags: whether it probes at all, and whether it probes every
* library present or only the selected one.
*   targets:  env/net/tls/env_tls.h -> D_CFG_ENV_TLS_PROBE_VERSION,
*                                      D_CFG_ENV_TLS_PROBE_ALL
*   requires: cfg_common.h (D_CFG_IS_BOOL)
*
*
* path:      /inc/djinterp/config/core/env/net/tls/cfg_env_tls.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.06
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_ENV_NET_TLS_CFG_ENV_TLS_H
#define DJINTERP_CONFIG_CORE_ENV_NET_TLS_CFG_ENV_TLS_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


// D_CFG_ENV_TLS_PROBE_VERSION
//   configuration: 1, the default, has env_tls.h include the version and
// configuration headers below; 0 keeps detection from touching any TLS
// library's header beyond the __has_include checks, and every version then
// reads as unknown unless pre-defined. The headers carry macros and
// declarations only, never code:
//     - <openssl/opensslv.h>, the OpenSSL family's version macros, whenever
//       it exists;
//     - <mbedtls/version.h>, mbedTLS's version and build configuration;
//     - <gnutls/gnutls.h>, the one GnuTLS header with its version;
//     - wolfSSL's <wolfssl/options.h>, written by its configure, or with
//       WOLFSSL_USER_SETTINGS <wolfssl/wolfcrypt/settings.h>, which reads the
//       build's user_settings.h.
// The last three are probed for the selected library only, unless
// D_CFG_ENV_TLS_PROBE_ALL says otherwise.
#ifndef D_CFG_ENV_TLS_PROBE_VERSION
    #define D_CFG_ENV_TLS_PROBE_VERSION 1
#endif  // D_CFG_ENV_TLS_PROBE_VERSION

// D_CFG_ENV_TLS_PROBE_ALL
//   configuration: 0, the default, has env_tls.h probe mbedTLS's, GnuTLS's
// and wolfSSL's headers only for the library D_ENV_TLS_BACKEND selects, so a
// build that uses OpenSSL takes nothing from the others -- wolfSSL's options
// header, for one, redefines _FORTIFY_SOURCE. 1 probes every one of them
// that is present, so each library's D_ENV_TLS_<LIBRARY>_HAS_* flags answer
// for it; at 0 an unselected library's version-gated flags read 0 unless
// its own headers came first (decision 60 of the register).
#ifndef D_CFG_ENV_TLS_PROBE_ALL
    #define D_CFG_ENV_TLS_PROBE_ALL 0
#endif  // D_CFG_ENV_TLS_PROBE_ALL

// validation: #if reads the switches as numbers, so each must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_TLS_PROBE_VERSION)
    #error "D_CFG_ENV_TLS_PROBE_VERSION must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_ENV_TLS_PROBE_ALL)
    #error "D_CFG_ENV_TLS_PROBE_ALL must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_NET_TLS_CFG_ENV_TLS_H
