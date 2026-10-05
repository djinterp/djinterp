/*******************************************************************************
* djinterp [env]                                                       env_pop.h
*
* djinterp POP3 environment detection.
*   Compile-time detection of everything the POP3 modules may depend on,
* expressed through the unified D_ENV_POP_* interface. It covers:
*     - the TCP and TLS transports POP3 runs over                         [1]
*     - libcurl's POP3 / POP3S support, version-gated                     [2]
*     - SASL libraries for the AUTH command (Cyrus, GNU SASL, SSPI)       [3]
*     - rolled-up capability summaries (CAN_PLAIN, CAN_TLS, CAN_CURL)     [4]
*   The common kernel, net/pop/pop.h, is pure protocol grammar and needs
* none of this to build; these flags gate the transport-bound modules
* derived from it, by way of the D_CFG_POP_* defaults in cfg_pop.h.
*   Naming: D_ENV_POP_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_POP_<FEATURE> is a non-boolean detected value or identifier.
*   Every flag is #ifndef-guarded, so a project may pre-define any D_ENV_POP_*
* macro before inclusion to override detection.
*   It requires env.h, env_net.h, env_tls.h, and env_curl.h, and includes
* them itself. It is an opt-in module and may be included directly.
*
*
* path:      /inc/djinterp/env/net/pop/env_pop.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.25
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSPORT PREREQUISITES
    -----------------------
    1.  Plain and secure transports
         1.  D_ENV_POP_HAS_TCP
         2.  D_ENV_POP_HAS_TLS
2.  LIBCURL POP3 SUPPORT
    --------------------
    1.  Version floors
         1.  D_ENV_POP_CURL_MIN_VERSION
         2.  D_ENV_POP_CURL_SASL_IR_VERSION
    2.  Protocol availability
         1.  D_ENV_POP_HAS_CURL
         2.  D_ENV_POP_HAS_CURL_SASL_IR
3.  SASL LIBRARIES
    --------------
    1.  Library detection
         1.  D_ENV_POP_HAS_CYRUS_SASL
         2.  D_ENV_POP_HAS_GSASL
         3.  D_ENV_POP_HAS_SSPI
    2.  Backend identifiers
         1.  D_ENV_POP_SASL_BACKEND_*
              1.  D_ENV_POP_SASL_BACKEND_NONE
              2.  D_ENV_POP_SASL_BACKEND_CYRUS
              3.  D_ENV_POP_SASL_BACKEND_GSASL
              4.  D_ENV_POP_SASL_BACKEND_SSPI
    3.  Backend selection
         1.  D_ENV_POP_SASL_BACKEND
         2.  D_ENV_POP_SASL_BACKEND_NAME
         3.  D_ENV_POP_HAS_SASL
4.  CAPABILITY SUMMARIES
    --------------------
    1.  Rolled-up capabilities
         1.  D_ENV_POP_CAN_PLAIN
         2.  D_ENV_POP_CAN_TLS
         3.  D_ENV_POP_CAN_CURL
         4.  D_ENV_POP_AVAILABLE
*/

#ifndef DJINTERP_ENV_NET_POP_ENV_POP_H
#define DJINTERP_ENV_NET_POP_ENV_POP_H 1

// djinterp
#include "../../env.h"         // D_ENV_OS_ID, D_ENV_IS_OS_WINDOWS
#include "../env_net.h"        // D_ENV_NET_HAS_INCLUDE, D_ENV_NET_CAN_TCP
#include "../tls/env_tls.h"    // D_ENV_TLS_AVAILABLE
#include "../curl/env_curl.h"  // D_ENV_CURL_AVAILABLE, D_ENV_CURL_VERSION_NUM


//==============================================================================
// 1.  TRANSPORT PREREQUISITES
//==============================================================================
// POP3 runs over a TCP stream: in the clear on port 110, upgraded in place by
// STLS (RFC 2595), or wrapped in TLS from the first byte on port 995. These
// restate the transport-level answers of env_net.h and env_tls.h under the
// D_ENV_POP_* names, so a POP module reads one family and a later change to
// what POP requires has one place to land.


// 1.1    Plain and secure transports
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_POP_HAS_TCP
//   feature: 1 if a TCP stream with name resolution is usable, which is all a
// cleartext POP3 session needs.
#ifndef D_ENV_POP_HAS_TCP
    #if D_ENV_NET_CAN_TCP
        #define D_ENV_POP_HAS_TCP 1
    #else
        #define D_ENV_POP_HAS_TCP 0
    #endif
#endif  // D_ENV_POP_HAS_TCP

// 1.1.2
// D_ENV_POP_HAS_TLS
//   feature: 1 if a TLS library and a TCP transport to carry it are both
// present. STLS and implicit-TLS (POP3S) need exactly the same library
// support, so one flag answers for both.
#ifndef D_ENV_POP_HAS_TLS
    #if D_ENV_TLS_AVAILABLE
        #define D_ENV_POP_HAS_TLS 1
    #else
        #define D_ENV_POP_HAS_TLS 0
    #endif
#endif  // D_ENV_POP_HAS_TLS


//==============================================================================
// 2.  LIBCURL POP3 SUPPORT
//==============================================================================
// libcurl speaks POP3 and POP3S from 7.20.0, and gained SASL initial-response
// control (CURLOPT_SASL_IR) in 7.31.0. As with env_curl.h, these answer "is
// the API present to call?". Whether a particular libcurl binary was built
// with POP3 enabled is a runtime question: check for "pop3" in
// curl_version_info()->protocols before relying on it.


// 2.1    Version floors
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_POP_CURL_MIN_VERSION
//   constant: the packed libcurl version (0xMMNNPP) that introduced the pop3
// and pop3s URL schemes.
#define D_ENV_POP_CURL_MIN_VERSION     0x071400

// 2.1.2
// D_ENV_POP_CURL_SASL_IR_VERSION
//   constant: the packed libcurl version that introduced CURLOPT_SASL_IR.
#define D_ENV_POP_CURL_SASL_IR_VERSION 0x071F00

// 2.2    Protocol availability
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_POP_HAS_CURL
//   feature: 1 if libcurl is available and new enough to carry POP3.
#ifndef D_ENV_POP_HAS_CURL
    #if ( (D_ENV_CURL_AVAILABLE) &&                                            \
          (D_ENV_CURL_VERSION_NUM >= D_ENV_POP_CURL_MIN_VERSION) )
        #define D_ENV_POP_HAS_CURL 1
    #else
        #define D_ENV_POP_HAS_CURL 0
    #endif
#endif  // D_ENV_POP_HAS_CURL

// 2.2.2
// D_ENV_POP_HAS_CURL_SASL_IR
//   feature: 1 if the libcurl POP3 path can control the SASL initial response
// (RFC 5034 section 4).
#ifndef D_ENV_POP_HAS_CURL_SASL_IR
    #if ( (D_ENV_POP_HAS_CURL) &&                                              \
          (D_ENV_CURL_VERSION_NUM >= D_ENV_POP_CURL_SASL_IR_VERSION) )
        #define D_ENV_POP_HAS_CURL_SASL_IR 1
    #else
        #define D_ENV_POP_HAS_CURL_SASL_IR 0
    #endif
#endif  // D_ENV_POP_HAS_CURL_SASL_IR


//==============================================================================
// 3.  SASL LIBRARIES
//==============================================================================
// The AUTH command (RFC 5034) delegates to SASL. The simple mechanisms (PLAIN,
// LOGIN) are base64 framing and need no library; GSSAPI, SCRAM and the like
// do. These detect the general-purpose SASL libraries a derived AUTH module
// may delegate to. Detection is presence-only: no SASL header is included and
// no link dependency is added.


// 3.1    Library detection
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_POP_HAS_CYRUS_SASL
//   feature: Cyrus SASL's <sasl/sasl.h> is includable.
#ifndef D_ENV_POP_HAS_CYRUS_SASL
    #if D_ENV_NET_HAS_INCLUDE(<sasl/sasl.h>)
        #define D_ENV_POP_HAS_CYRUS_SASL 1
    #else
        #define D_ENV_POP_HAS_CYRUS_SASL 0
    #endif
#endif  // D_ENV_POP_HAS_CYRUS_SASL

// 3.1.2
// D_ENV_POP_HAS_GSASL
//   feature: GNU SASL's <gsasl.h> is includable.
#ifndef D_ENV_POP_HAS_GSASL
    #if D_ENV_NET_HAS_INCLUDE(<gsasl.h>)
        #define D_ENV_POP_HAS_GSASL 1
    #else
        #define D_ENV_POP_HAS_GSASL 0
    #endif
#endif  // D_ENV_POP_HAS_GSASL

// 3.1.3
// D_ENV_POP_HAS_SSPI
//   feature: the Windows Security Support Provider Interface is available
// (present on every Windows target, like Schannel in env_tls.h).
#ifndef D_ENV_POP_HAS_SSPI
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_POP_HAS_SSPI 1
    #else
        #define D_ENV_POP_HAS_SSPI 0
    #endif
#endif  // D_ENV_POP_HAS_SSPI

// 3.2    Backend identifiers
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_POP_SASL_BACKEND_*
//   constant: identifiers for D_ENV_POP_SASL_BACKEND, for switch-style
// dispatch.

// 3.2.1.1
// D_ENV_POP_SASL_BACKEND_NONE
//   constant: identifies no SASL library.
#define D_ENV_POP_SASL_BACKEND_NONE  0

// 3.2.1.2
// D_ENV_POP_SASL_BACKEND_CYRUS
//   constant: identifies Cyrus SASL.
#define D_ENV_POP_SASL_BACKEND_CYRUS 1

// 3.2.1.3
// D_ENV_POP_SASL_BACKEND_GSASL
//   constant: identifies GNU SASL.
#define D_ENV_POP_SASL_BACKEND_GSASL 2

// 3.2.1.4
// D_ENV_POP_SASL_BACKEND_SSPI
//   constant: identifies Windows SSPI.
#define D_ENV_POP_SASL_BACKEND_SSPI  3

// 3.3    Backend selection
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_POP_SASL_BACKEND
//   feature: the preferred available SASL library. The OS-integrated SSPI
// wins on Windows, where it needs no third-party dependency; elsewhere Cyrus
// is preferred for its mechanism coverage, then GNU SASL.
#ifndef D_ENV_POP_SASL_BACKEND
    #if D_ENV_POP_HAS_SSPI
        #define D_ENV_POP_SASL_BACKEND D_ENV_POP_SASL_BACKEND_SSPI
    #elif D_ENV_POP_HAS_CYRUS_SASL
        #define D_ENV_POP_SASL_BACKEND D_ENV_POP_SASL_BACKEND_CYRUS
    #elif D_ENV_POP_HAS_GSASL
        #define D_ENV_POP_SASL_BACKEND D_ENV_POP_SASL_BACKEND_GSASL
    #else
        #define D_ENV_POP_SASL_BACKEND D_ENV_POP_SASL_BACKEND_NONE
    #endif
#endif  // D_ENV_POP_SASL_BACKEND

// 3.3.2
// D_ENV_POP_SASL_BACKEND_NAME
//   value: a human-readable name for the selected SASL library.
#ifndef D_ENV_POP_SASL_BACKEND_NAME
    #if (D_ENV_POP_SASL_BACKEND == D_ENV_POP_SASL_BACKEND_SSPI)
        #define D_ENV_POP_SASL_BACKEND_NAME "SSPI"
    #elif (D_ENV_POP_SASL_BACKEND == D_ENV_POP_SASL_BACKEND_CYRUS)
        #define D_ENV_POP_SASL_BACKEND_NAME "Cyrus SASL"
    #elif (D_ENV_POP_SASL_BACKEND == D_ENV_POP_SASL_BACKEND_GSASL)
        #define D_ENV_POP_SASL_BACKEND_NAME "GNU SASL"
    #else
        #define D_ENV_POP_SASL_BACKEND_NAME "none"
    #endif
#endif  // D_ENV_POP_SASL_BACKEND_NAME

// 3.3.3
// D_ENV_POP_HAS_SASL
//   feature: 1 if any SASL library is available. The built-in PLAIN and LOGIN
// mechanisms do not depend on this.
#ifndef D_ENV_POP_HAS_SASL
    #if (D_ENV_POP_SASL_BACKEND != D_ENV_POP_SASL_BACKEND_NONE)
        #define D_ENV_POP_HAS_SASL 1
    #else
        #define D_ENV_POP_HAS_SASL 0
    #endif
#endif  // D_ENV_POP_HAS_SASL


//==============================================================================
// 4.  CAPABILITY SUMMARIES
//==============================================================================
// What a POP module can actually do on this target. None of these gate the
// common kernel (pop.h), which is pure protocol grammar and builds anywhere;
// they gate the transport-bound modules derived from it.


// 4.1    Rolled-up capabilities
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_POP_CAN_PLAIN
//   feature: 1 if a cleartext POP3 session (port 110) can be opened.
#ifndef D_ENV_POP_CAN_PLAIN
    #define D_ENV_POP_CAN_PLAIN D_ENV_POP_HAS_TCP
#endif  // D_ENV_POP_CAN_PLAIN

// 4.1.2
// D_ENV_POP_CAN_TLS
//   feature: 1 if a session can be secured, either by STLS or as POP3S.
#ifndef D_ENV_POP_CAN_TLS
    #if ( (D_ENV_POP_HAS_TCP) &&                                               \
          (D_ENV_POP_HAS_TLS) )
        #define D_ENV_POP_CAN_TLS 1
    #else
        #define D_ENV_POP_CAN_TLS 0
    #endif
#endif  // D_ENV_POP_CAN_TLS

// 4.1.3
// D_ENV_POP_CAN_CURL
//   feature: 1 if a libcurl-backed POP3 module can be built.
#ifndef D_ENV_POP_CAN_CURL
    #define D_ENV_POP_CAN_CURL D_ENV_POP_HAS_CURL
#endif  // D_ENV_POP_CAN_CURL

// 4.1.4
// D_ENV_POP_AVAILABLE
//   feature: 1 if any transport can reach a POP3 server from this target. The
// gate a transport-bound POP module should #error on.
#ifndef D_ENV_POP_AVAILABLE
    #if ( (D_ENV_POP_CAN_PLAIN) ||                                             \
          (D_ENV_POP_CAN_CURL) )
        #define D_ENV_POP_AVAILABLE 1
    #else
        #define D_ENV_POP_AVAILABLE 0
    #endif
#endif  // D_ENV_POP_AVAILABLE


#endif  // DJINTERP_ENV_NET_POP_ENV_POP_H
