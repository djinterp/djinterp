/*******************************************************************************
* djinterp [env]                                                       env_tls.h
*
* djinterp TLS backend environment detection.
*   Compile-time detection of the TLS libraries available to the net TLS
* transport, expressed through the unified D_ENV_TLS_* interface along two
* axes: which library (D_ENV_TLS_BACKEND), and, within the OpenSSL family,
* which variant (D_ENV_TLS_OPENSSL_VARIANT). It covers:
*     - library detection: the OpenSSL family, mbedTLS, GnuTLS, wolfSSL,
*       Schannel, and Secure Transport, and the preferred library        [1]
*     - each library's version and build configuration, as far as its
*       own headers tell, with a comparison helper per version number    [2]
*     - capabilities by library (TLS 1.3, built-in hostname
*       verification, OpenSSL's implicit initialisation), with a roll-up
*       for the selected library where every library can answer          [3]
*     - a rolled-up availability gate, D_ENV_TLS_AVAILABLE                [4]
*     - a runtime probe for what no build can know: whether Schannel
*       negotiates TLS 1.3 depends on the Windows release it runs on     [5]
*   Like env_curl.h, it is backend-specific detection built on the
* transport-level environment of env_net.h.
*   Naming: D_ENV_TLS_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_TLS_<LIBRARY>_HAS_<FEATURE> asks the same of one library, and the
* unqualified flag forwards the selected library's answer;
* D_ENV_TLS_<FEATURE> is a non-boolean detected value or identifier.
*   Every flag is #ifndef-guarded, so a project may pre-define any D_ENV_TLS_*
* macro before inclusion to override detection.
*   It requires env.h, for the OS identification that Schannel and Secure
* Transport detection reads, env_net.h, for D_ENV_NET_HAS_INCLUDE and
* D_ENV_NET_CAN_TCP, and cfg_env_tls.h, for the switches that decide which
* libraries' headers it reads; it includes all three itself.
*
*
* path:      /inc/djinterp/env/net/tls/env_tls.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BACKEND LIBRARIES
    -----------------
    1.  Library detection
         1.  D_ENV_TLS_HAS_SSL_HEADER
         2.  D_ENV_TLS_HAS_VERSION_HEADER
         3.  D_ENV_TLS_HAS_OPENSSL
         4.  D_ENV_TLS_HAS_MBEDTLS
         5.  D_ENV_TLS_HAS_GNUTLS
         6.  D_ENV_TLS_HAS_WOLFSSL
         7.  D_ENV_TLS_HAS_SCHANNEL
         8.  D_ENV_TLS_HAS_SECURETRANSPORT
    2.  Backend identifiers
         1.  D_ENV_TLS_BACKEND_*
              1.  D_ENV_TLS_BACKEND_NONE
              2.  D_ENV_TLS_BACKEND_OPENSSL
              3.  D_ENV_TLS_BACKEND_MBEDTLS
              4.  D_ENV_TLS_BACKEND_GNUTLS
              5.  D_ENV_TLS_BACKEND_WOLFSSL
              6.  D_ENV_TLS_BACKEND_SCHANNEL
              7.  D_ENV_TLS_BACKEND_SECURETRANSPORT
    3.  Backend selection
         1.  D_ENV_TLS_BACKEND
         2.  D_ENV_TLS_BACKEND_NAME
2.  LIBRARY VERSIONS AND CONFIGURATION
    ----------------------------------
    1.  Version probes
         1.  <openssl/opensslv.h>
         2.  <mbedtls/version.h>
         3.  <gnutls/gnutls.h>
         4.  <wolfssl/options.h>
    2.  OpenSSL variant identifiers
         1.  D_ENV_TLS_OPENSSL_VARIANT_*
              1.  D_ENV_TLS_OPENSSL_VARIANT_NONE
              2.  D_ENV_TLS_OPENSSL_VARIANT_OPENSSL
              3.  D_ENV_TLS_OPENSSL_VARIANT_LIBRESSL
              4.  D_ENV_TLS_OPENSSL_VARIANT_BORINGSSL
              5.  D_ENV_TLS_OPENSSL_VARIANT_AWSLC
    3.  OpenSSL variant detection
         1.  D_ENV_TLS_IS_BORINGSSL / D_ENV_TLS_IS_LIBRESSL / D_ENV_TLS_IS_AWSLC
         2.  D_ENV_TLS_OPENSSL_VARIANT
         3.  D_ENV_TLS_OPENSSL_VARIANT_NAME
    4.  OpenSSL family version numbers
         1.  D_ENV_TLS_OPENSSL_VERSION_NUMBER
         2.  D_ENV_TLS_OPENSSL_VERSION_MAJOR
         3.  D_ENV_TLS_OPENSSL_AT_LEAST
         4.  D_ENV_TLS_LIBRESSL_VERSION_NUMBER
         5.  D_ENV_TLS_LIBRESSL_AT_LEAST
    5.  Other libraries' version numbers
         1.  D_ENV_TLS_MBEDTLS_VERSION_NUMBER
         2.  D_ENV_TLS_MBEDTLS_AT_LEAST
         3.  D_ENV_TLS_GNUTLS_VERSION_NUMBER
         4.  D_ENV_TLS_GNUTLS_AT_LEAST
3.  CAPABILITIES BY LIBRARY
    -----------------------
    1.  TLS 1.3
         1.  D_ENV_TLS_OPENSSL_HAS_TLS1_3
         2.  D_ENV_TLS_MBEDTLS_HAS_TLS1_3
         3.  D_ENV_TLS_GNUTLS_HAS_TLS1_3
         4.  D_ENV_TLS_WOLFSSL_HAS_TLS1_3
         5.  D_ENV_TLS_SCHANNEL_HAS_TLS1_3
         6.  D_ENV_TLS_SECURETRANSPORT_HAS_TLS1_3
         7.  D_ENV_TLS_HAS_TLS1_3
    2.  Hostname validation
         1.  D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION
         2.  D_ENV_TLS_MBEDTLS_HAS_HOSTNAME_VALIDATION
         3.  D_ENV_TLS_GNUTLS_HAS_HOSTNAME_VALIDATION
         4.  D_ENV_TLS_WOLFSSL_HAS_HOSTNAME_VALIDATION
         5.  D_ENV_TLS_SCHANNEL_HAS_HOSTNAME_VALIDATION
         6.  D_ENV_TLS_SECURETRANSPORT_HAS_HOSTNAME_VALIDATION
         7.  D_ENV_TLS_HAS_HOSTNAME_VALIDATION
    3.  OpenSSL initialization
         1.  D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT
4.  AVAILABILITY GATE
    -----------------
    1.  Rolled-up availability
         1.  D_ENV_TLS_HAS_TLS
         2.  D_ENV_TLS_AVAILABLE
5.  RUNTIME PROBES
    --------------
    1.  Runtime probes
*/

#ifndef DJINTERP_ENV_NET_TLS_ENV_TLS_H
#define DJINTERP_ENV_NET_TLS_ENV_TLS_H 1

// djinterp
#include "../../env.h"   // D_ENV_OS_ID, D_ENV_IS_OS_WINDOWS,
                         // D_ENV_IS_OS_FLAG_IN_BLOCK, D_ENV_LANG_USING_CPP
#include "../env_net.h"  // D_ENV_NET_HAS_INCLUDE, D_ENV_NET_CAN_TCP
#include "../../../config/core/env/net/tls/cfg_env_tls.h"  // D_CFG_ENV_TLS_*


//==============================================================================
// 1.  BACKEND LIBRARIES
//==============================================================================
// The TLS libraries this layer recognizes: the OpenSSL API family (OpenSSL,
// LibreSSL, and BoringSSL, which all present <openssl/ssl.h>, with
// <openssl/opensslv.h> carrying the version macros), the standalone mbedTLS,
// GnuTLS, and wolfSSL, detected by header, and the OS-integrated Schannel and
// Secure Transport, detected by platform. Every library's flag stays
// queryable; D_ENV_TLS_BACKEND picks one.


// 1.1    Library detection
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_TLS_HAS_SSL_HEADER
//   feature: <openssl/ssl.h> is includable.
#ifndef D_ENV_TLS_HAS_SSL_HEADER
    #if D_ENV_NET_HAS_INCLUDE(<openssl/ssl.h>)
        #define D_ENV_TLS_HAS_SSL_HEADER 1
    #else
        #define D_ENV_TLS_HAS_SSL_HEADER 0
    #endif
#endif  // D_ENV_TLS_HAS_SSL_HEADER

// 1.1.2
// D_ENV_TLS_HAS_VERSION_HEADER
//   feature: the standalone <openssl/opensslv.h> version header is includable.
#ifndef D_ENV_TLS_HAS_VERSION_HEADER
    #if D_ENV_NET_HAS_INCLUDE(<openssl/opensslv.h>)
        #define D_ENV_TLS_HAS_VERSION_HEADER 1
    #else
        #define D_ENV_TLS_HAS_VERSION_HEADER 0
    #endif
#endif  // D_ENV_TLS_HAS_VERSION_HEADER

// 1.1.3
// D_ENV_TLS_HAS_OPENSSL
//   feature: 1 if the OpenSSL API family is present (the core SSL header).
#ifndef D_ENV_TLS_HAS_OPENSSL
    #if D_ENV_TLS_HAS_SSL_HEADER
        #define D_ENV_TLS_HAS_OPENSSL 1
    #else
        #define D_ENV_TLS_HAS_OPENSSL 0
    #endif
#endif  // D_ENV_TLS_HAS_OPENSSL

// 1.1.4
// D_ENV_TLS_HAS_MBEDTLS
//   feature: detect if mbedTLS is available.
#ifndef D_ENV_TLS_HAS_MBEDTLS
    #if D_ENV_NET_HAS_INCLUDE(<mbedtls/ssl.h>)
        #define D_ENV_TLS_HAS_MBEDTLS 1
    #else
        #define D_ENV_TLS_HAS_MBEDTLS 0
    #endif
#endif  // D_ENV_TLS_HAS_MBEDTLS

// 1.1.5
// D_ENV_TLS_HAS_GNUTLS
//   feature: detect if GnuTLS is available.
#ifndef D_ENV_TLS_HAS_GNUTLS
    #if D_ENV_NET_HAS_INCLUDE(<gnutls/gnutls.h>)
        #define D_ENV_TLS_HAS_GNUTLS 1
    #else
        #define D_ENV_TLS_HAS_GNUTLS 0
    #endif
#endif  // D_ENV_TLS_HAS_GNUTLS

// 1.1.6
// D_ENV_TLS_HAS_WOLFSSL
//   feature: detect if wolfSSL is available.
#ifndef D_ENV_TLS_HAS_WOLFSSL
    #if D_ENV_NET_HAS_INCLUDE(<wolfssl/ssl.h>)
        #define D_ENV_TLS_HAS_WOLFSSL 1
    #else
        #define D_ENV_TLS_HAS_WOLFSSL 0
    #endif
#endif  // D_ENV_TLS_HAS_WOLFSSL

// 1.1.7
// D_ENV_TLS_HAS_SCHANNEL
//   feature: detect if the Windows-integrated Schannel SSP is available
// (present on every Windows target).
#ifndef D_ENV_TLS_HAS_SCHANNEL
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_TLS_HAS_SCHANNEL 1
    #else
        #define D_ENV_TLS_HAS_SCHANNEL 0
    #endif
#endif  // D_ENV_TLS_HAS_SCHANNEL

// 1.1.8
// D_ENV_TLS_HAS_SECURETRANSPORT
//   feature: detect if Apple's Secure Transport is available (Apple-family
// targets). Apple has deprecated it in favor of Network.framework, but it
// remains the C-level TLS provider many clients bind to.
#ifndef D_ENV_TLS_HAS_SECURETRANSPORT
    #if D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x0)
        #define D_ENV_TLS_HAS_SECURETRANSPORT 1
    #else
        #define D_ENV_TLS_HAS_SECURETRANSPORT 0
    #endif
#endif  // D_ENV_TLS_HAS_SECURETRANSPORT

// 1.2    Backend identifiers
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_TLS_BACKEND_*
//   constant: identifiers for D_ENV_TLS_BACKEND, the TLS library in use.

// 1.2.1.1
// D_ENV_TLS_BACKEND_NONE
//   constant: identifies no TLS backend.
#define D_ENV_TLS_BACKEND_NONE            0

// 1.2.1.2
// D_ENV_TLS_BACKEND_OPENSSL
//   constant: identifies the OpenSSL-compatible family (OpenSSL, LibreSSL,
// BoringSSL).
#define D_ENV_TLS_BACKEND_OPENSSL         1

// 1.2.1.3
// D_ENV_TLS_BACKEND_MBEDTLS
//   constant: identifies mbedTLS.
#define D_ENV_TLS_BACKEND_MBEDTLS         2

// 1.2.1.4
// D_ENV_TLS_BACKEND_GNUTLS
//   constant: identifies GnuTLS.
#define D_ENV_TLS_BACKEND_GNUTLS          3

// 1.2.1.5
// D_ENV_TLS_BACKEND_WOLFSSL
//   constant: identifies wolfSSL.
#define D_ENV_TLS_BACKEND_WOLFSSL         4

// 1.2.1.6
// D_ENV_TLS_BACKEND_SCHANNEL
//   constant: identifies Windows Schannel.
#define D_ENV_TLS_BACKEND_SCHANNEL        5

// 1.2.1.7
// D_ENV_TLS_BACKEND_SECURETRANSPORT
//   constant: identifies Apple Secure Transport.
#define D_ENV_TLS_BACKEND_SECURETRANSPORT 6

// 1.3    Backend selection
//------------------------------------------------------------------------------
// 1.3.1
// D_ENV_TLS_BACKEND
//   feature: the preferred TLS library, as a D_ENV_TLS_BACKEND_* identifier.
// The order favors the portable, widely deployed OpenSSL family, then the
// standalone libraries, then the OS-integrated providers as a last resort.
#ifndef D_ENV_TLS_BACKEND
    #if D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_BACKEND D_ENV_TLS_BACKEND_OPENSSL
    #elif D_ENV_TLS_HAS_MBEDTLS
        #define D_ENV_TLS_BACKEND D_ENV_TLS_BACKEND_MBEDTLS
    #elif D_ENV_TLS_HAS_GNUTLS
        #define D_ENV_TLS_BACKEND D_ENV_TLS_BACKEND_GNUTLS
    #elif D_ENV_TLS_HAS_WOLFSSL
        #define D_ENV_TLS_BACKEND D_ENV_TLS_BACKEND_WOLFSSL
    #elif D_ENV_TLS_HAS_SCHANNEL
        #define D_ENV_TLS_BACKEND D_ENV_TLS_BACKEND_SCHANNEL
    #elif D_ENV_TLS_HAS_SECURETRANSPORT
        #define D_ENV_TLS_BACKEND D_ENV_TLS_BACKEND_SECURETRANSPORT
    #else
        #define D_ENV_TLS_BACKEND D_ENV_TLS_BACKEND_NONE
    #endif
#endif  // D_ENV_TLS_BACKEND

// 1.3.2
// D_ENV_TLS_BACKEND_NAME
//   value: a human-readable name for the preferred TLS library.
#ifndef D_ENV_TLS_BACKEND_NAME
    #if (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_OPENSSL)
        #define D_ENV_TLS_BACKEND_NAME "OpenSSL"
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_MBEDTLS)
        #define D_ENV_TLS_BACKEND_NAME "mbedTLS"
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_GNUTLS)
        #define D_ENV_TLS_BACKEND_NAME "GnuTLS"
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_WOLFSSL)
        #define D_ENV_TLS_BACKEND_NAME "wolfSSL"
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_SCHANNEL)
        #define D_ENV_TLS_BACKEND_NAME "Schannel"
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_SECURETRANSPORT)
        #define D_ENV_TLS_BACKEND_NAME "Secure Transport"
    #else
        #define D_ENV_TLS_BACKEND_NAME "none"
    #endif
#endif  // D_ENV_TLS_BACKEND_NAME


//==============================================================================
// 2.  LIBRARY VERSIONS AND CONFIGURATION
//==============================================================================
// What each library's own headers say about its release and its build. The
// OpenSSL family's number is OPENSSL_VERSION_NUMBER: the 1.x line packs it
// as 0xMNNFFPPS (major, minor, fix, patch letter, status) and the 3.x line
// as 0xMNN00PP0 (major, minor, patch), and the high nibble is the major
// version in both. LibreSSL and BoringSSL define their own identifying
// macros while pinning OPENSSL_VERSION_NUMBER for compatibility, so LibreSSL
// is read by its own number, and BoringSSL, which makes no releases, is taken
// to be current. mbedTLS packs MBEDTLS_VERSION_NUMBER as 0xMMNNPP00, and
// GnuTLS packs GNUTLS_VERSION_NUMBER as 0xMMNNPP. What wolfSSL can do is a
// matter of its build options, not its version, so it is read from the
// header its build wrote. Each version helper packs its threshold the way
// its library packs its own number, and is vacuously 0 when the version is
// unknown.


// 2.1    Version probes
//------------------------------------------------------------------------------
// The switches are cfg_env_tls.h's: D_CFG_ENV_TLS_PROBE_VERSION, 1 by default,
// turns the probes on, and D_CFG_ENV_TLS_PROBE_ALL, 0 by default, limits
// mbedTLS's, GnuTLS's and wolfSSL's to the library D_ENV_TLS_BACKEND selects.
// Each header carries macros and declarations, never code.

// 2.1.1
// <openssl/opensslv.h>
//   include: included when D_CFG_ENV_TLS_PROBE_VERSION is 1 and the header
// exists, whichever library is selected. It defines only the version macros.
#if ( (D_CFG_ENV_TLS_PROBE_VERSION) &&                                         \
      (D_ENV_TLS_HAS_VERSION_HEADER) )
    // openssl
    #include <openssl/opensslv.h>  // OPENSSL_VERSION_NUMBER, ...
#endif

// 2.1.2
// <mbedtls/version.h>
//   include: mbedTLS's version and, through it, its build configuration
// (MBEDTLS_SSL_PROTO_TLS1_3, MBEDTLS_X509_CRT_PARSE_C), which the header
// includes in every release.
#if ( (D_CFG_ENV_TLS_PROBE_VERSION) &&                                         \
      (D_ENV_TLS_HAS_MBEDTLS)        &&                                        \
      ( (D_CFG_ENV_TLS_PROBE_ALL) ||                                           \
        (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_MBEDTLS) ) )
    // mbedtls
    #include <mbedtls/version.h>  // MBEDTLS_VERSION_NUMBER, MBEDTLS_SSL_*
#endif

// 2.1.3
// <gnutls/gnutls.h>
//   include: GnuTLS's version, which only its main header defines; the
// header also declares the library's API.
#if ( (D_CFG_ENV_TLS_PROBE_VERSION) &&                                         \
      (D_ENV_TLS_HAS_GNUTLS)         &&                                        \
      ( (D_CFG_ENV_TLS_PROBE_ALL) ||                                           \
        (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_GNUTLS) ) )
    // gnutls
    #include <gnutls/gnutls.h>  // GNUTLS_VERSION_NUMBER
#endif

// 2.1.4
// <wolfssl/options.h>
//   include: wolfSSL's build options (WOLFSSL_TLS13), which its configure
// writes to <wolfssl/options.h>. A build configured by hand defines
// WOLFSSL_USER_SETTINGS instead and keeps its options in user_settings.h,
// which <wolfssl/wolfcrypt/settings.h> reads. A build with neither tells
// nothing, and its flags read 0 unless pre-defined.
#if ( (D_CFG_ENV_TLS_PROBE_VERSION) &&                                         \
      (D_ENV_TLS_HAS_WOLFSSL)        &&                                        \
      ( (D_CFG_ENV_TLS_PROBE_ALL) ||                                           \
        (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_WOLFSSL) ) )
    #if defined(WOLFSSL_USER_SETTINGS)
        // wolfssl
        #include <wolfssl/wolfcrypt/settings.h>  // user_settings.h
    #elif D_ENV_NET_HAS_INCLUDE(<wolfssl/options.h>)
        // wolfssl
        #include <wolfssl/options.h>  // WOLFSSL_TLS13, WOLFSSL_TLS13_DRAFT
    #endif
#endif

// 2.2    OpenSSL variant identifiers
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_TLS_OPENSSL_VARIANT_*
//   constant: identifiers for D_ENV_TLS_OPENSSL_VARIANT.

// 2.2.1.1
// D_ENV_TLS_OPENSSL_VARIANT_NONE
//   constant: identifies no OpenSSL-family library.
#define D_ENV_TLS_OPENSSL_VARIANT_NONE        0

// 2.2.1.2
// D_ENV_TLS_OPENSSL_VARIANT_OPENSSL
//   constant: identifies OpenSSL proper.
#define D_ENV_TLS_OPENSSL_VARIANT_OPENSSL     1

// 2.2.1.3
// D_ENV_TLS_OPENSSL_VARIANT_LIBRESSL
//   constant: identifies LibreSSL.
#define D_ENV_TLS_OPENSSL_VARIANT_LIBRESSL    2

// 2.2.1.4
// D_ENV_TLS_OPENSSL_VARIANT_BORINGSSL
//   constant: identifies BoringSSL.
#define D_ENV_TLS_OPENSSL_VARIANT_BORINGSSL   3

// 2.2.1.5
// D_ENV_TLS_OPENSSL_VARIANT_AWSLC
//   constant: identifies AWS-LC, Amazon's fork of BoringSSL (decision 103 of
// the register).
#define D_ENV_TLS_OPENSSL_VARIANT_AWSLC       4

// 2.3    OpenSSL variant detection
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_TLS_IS_BORINGSSL / D_ENV_TLS_IS_LIBRESSL / D_ENV_TLS_IS_AWSLC
//   feature: distinguish the API-compatible forks from OpenSSL proper, each by
// the macro it defines of itself. AWS-LC defines OPENSSL_IS_AWSLC and, in
// every release from 1.10 to 5.11, not OPENSSL_IS_BORINGSSL, with the
// OPENSSL_VERSION_NUMBER of OpenSSL 1.1.1's API, by which env_ssl.h answers
// its calls; D_ENV_TLS_IS_BORINGSSL stays 0 for it (decision 103 of the
// register).
#ifndef D_ENV_TLS_IS_BORINGSSL
    #if defined(OPENSSL_IS_BORINGSSL)
        #define D_ENV_TLS_IS_BORINGSSL 1
    #else
        #define D_ENV_TLS_IS_BORINGSSL 0
    #endif
#endif  // D_ENV_TLS_IS_BORINGSSL

#ifndef D_ENV_TLS_IS_LIBRESSL
    #if defined(LIBRESSL_VERSION_NUMBER)
        #define D_ENV_TLS_IS_LIBRESSL 1
    #else
        #define D_ENV_TLS_IS_LIBRESSL 0
    #endif
#endif  // D_ENV_TLS_IS_LIBRESSL

#ifndef D_ENV_TLS_IS_AWSLC
    #if defined(OPENSSL_IS_AWSLC)
        #define D_ENV_TLS_IS_AWSLC 1
    #else
        #define D_ENV_TLS_IS_AWSLC 0
    #endif
#endif  // D_ENV_TLS_IS_AWSLC

// 2.3.2
// D_ENV_TLS_OPENSSL_VARIANT
//   feature: which member of the OpenSSL family is present, as a
// D_ENV_TLS_OPENSSL_VARIANT_* identifier.
#ifndef D_ENV_TLS_OPENSSL_VARIANT
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_OPENSSL_VARIANT D_ENV_TLS_OPENSSL_VARIANT_NONE
    #elif D_ENV_TLS_IS_AWSLC
        #define D_ENV_TLS_OPENSSL_VARIANT D_ENV_TLS_OPENSSL_VARIANT_AWSLC
    #elif D_ENV_TLS_IS_BORINGSSL
        #define D_ENV_TLS_OPENSSL_VARIANT D_ENV_TLS_OPENSSL_VARIANT_BORINGSSL
    #elif D_ENV_TLS_IS_LIBRESSL
        #define D_ENV_TLS_OPENSSL_VARIANT D_ENV_TLS_OPENSSL_VARIANT_LIBRESSL
    #else
        #define D_ENV_TLS_OPENSSL_VARIANT D_ENV_TLS_OPENSSL_VARIANT_OPENSSL
    #endif
#endif  // D_ENV_TLS_OPENSSL_VARIANT

// 2.3.3
// D_ENV_TLS_OPENSSL_VARIANT_NAME
//   value: a human-readable name for the OpenSSL variant.
#ifndef D_ENV_TLS_OPENSSL_VARIANT_NAME
    #if (D_ENV_TLS_OPENSSL_VARIANT == D_ENV_TLS_OPENSSL_VARIANT_AWSLC)
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "AWS-LC"
    #elif (D_ENV_TLS_OPENSSL_VARIANT == D_ENV_TLS_OPENSSL_VARIANT_BORINGSSL)
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "BoringSSL"
    #elif (D_ENV_TLS_OPENSSL_VARIANT == D_ENV_TLS_OPENSSL_VARIANT_LIBRESSL)
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "LibreSSL"
    #elif (D_ENV_TLS_OPENSSL_VARIANT == D_ENV_TLS_OPENSSL_VARIANT_OPENSSL)
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "OpenSSL"
    #else
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "none"
    #endif
#endif  // D_ENV_TLS_OPENSSL_VARIANT_NAME

// 2.4    OpenSSL family version numbers
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_TLS_OPENSSL_VERSION_NUMBER
//   value: the raw OPENSSL_VERSION_NUMBER, or 0 when unknown.
#ifndef D_ENV_TLS_OPENSSL_VERSION_NUMBER
    #if defined(OPENSSL_VERSION_NUMBER)
        #define D_ENV_TLS_OPENSSL_VERSION_NUMBER OPENSSL_VERSION_NUMBER
    #else
        #define D_ENV_TLS_OPENSSL_VERSION_NUMBER 0
    #endif
#endif  // D_ENV_TLS_OPENSSL_VERSION_NUMBER

// 2.4.2
// D_ENV_TLS_OPENSSL_VERSION_MAJOR
//   value: the major version (high nibble), or 0 when unknown.
#ifndef D_ENV_TLS_OPENSSL_VERSION_MAJOR
    #define D_ENV_TLS_OPENSSL_VERSION_MAJOR                                    \
        ((D_ENV_TLS_OPENSSL_VERSION_NUMBER >> 28) & 0xF)
#endif  // D_ENV_TLS_OPENSSL_VERSION_MAJOR

// 2.4.3
// D_ENV_TLS_OPENSSL_AT_LEAST
//   macro: 1 if the OpenSSL version is at least major.minor.patch. The
// threshold is packed by release line: from 3.0 on as 0xMNN00PP0, and below
// it as 0xMNNFF000, the third number in the fix field with the patch letter
// and status left zero, so every lettered release of that fix meets it.
// (1, 1, 1) is met by 1.1.1 and 1.1.1w, and not by 1.1.0l; (3, 0, 2) by
// 3.0.2 and later. Vacuously 0 when the version is unknown. Not meaningful
// for LibreSSL (which pins the number); use the capability flags below
// instead on that fork.
#ifndef D_ENV_TLS_OPENSSL_AT_LEAST
    #define D_ENV_TLS_OPENSSL_AT_LEAST(major, minor, patch)                    \
        ( D_ENV_TLS_OPENSSL_VERSION_NUMBER >=                                  \
          ( ((major) >= 3) ? ( ((major) << 28) | ((minor) << 20) |             \
                               ((patch) << 4) )                                \
                           : ( ((major) << 28) | ((minor) << 20) |             \
                               ((patch) << 12) ) ) )
#endif  // D_ENV_TLS_OPENSSL_AT_LEAST

// 2.4.4
// D_ENV_TLS_LIBRESSL_VERSION_NUMBER
//   value: the raw LIBRESSL_VERSION_NUMBER, packed as 0xMNNFF00F, or 0 when
// the library is not LibreSSL or its version is unknown.
#ifndef D_ENV_TLS_LIBRESSL_VERSION_NUMBER
    #if defined(LIBRESSL_VERSION_NUMBER)
        #define D_ENV_TLS_LIBRESSL_VERSION_NUMBER LIBRESSL_VERSION_NUMBER
    #else
        #define D_ENV_TLS_LIBRESSL_VERSION_NUMBER 0
    #endif
#endif  // D_ENV_TLS_LIBRESSL_VERSION_NUMBER

// 2.4.5
// D_ENV_TLS_LIBRESSL_AT_LEAST
//   macro: 1 if the LibreSSL version is at least major.minor.fix, the
// threshold packed as LibreSSL packs its number with the status left zero:
// (3, 4, 1) is met by 3.4.1 (0x3040100F) and not by 3.4.0. Vacuously 0 when
// the version is unknown, and so on every other library.
#ifndef D_ENV_TLS_LIBRESSL_AT_LEAST
    #define D_ENV_TLS_LIBRESSL_AT_LEAST(major, minor, fix)                     \
        ( D_ENV_TLS_LIBRESSL_VERSION_NUMBER >=                                 \
          ( ((major) << 28) | ((minor) << 20) | ((fix) << 12) ) )
#endif  // D_ENV_TLS_LIBRESSL_AT_LEAST

// 2.5    Other libraries' version numbers
//------------------------------------------------------------------------------
// 2.5.1
// D_ENV_TLS_MBEDTLS_VERSION_NUMBER
//   value: the raw MBEDTLS_VERSION_NUMBER, packed as 0xMMNNPP00, or 0 when
// unknown.
#ifndef D_ENV_TLS_MBEDTLS_VERSION_NUMBER
    #if defined(MBEDTLS_VERSION_NUMBER)
        #define D_ENV_TLS_MBEDTLS_VERSION_NUMBER MBEDTLS_VERSION_NUMBER
    #else
        #define D_ENV_TLS_MBEDTLS_VERSION_NUMBER 0
    #endif
#endif  // D_ENV_TLS_MBEDTLS_VERSION_NUMBER

// 2.5.2
// D_ENV_TLS_MBEDTLS_AT_LEAST
//   macro: 1 if the mbedTLS version is at least major.minor.patch: (3, 2, 0)
// is 0x03020000. Vacuously 0 when the version is unknown.
#ifndef D_ENV_TLS_MBEDTLS_AT_LEAST
    #define D_ENV_TLS_MBEDTLS_AT_LEAST(major, minor, patch)                    \
        ( D_ENV_TLS_MBEDTLS_VERSION_NUMBER >=                                  \
          ( ((major) << 24) | ((minor) << 16) | ((patch) << 8) ) )
#endif  // D_ENV_TLS_MBEDTLS_AT_LEAST

// 2.5.3
// D_ENV_TLS_GNUTLS_VERSION_NUMBER
//   value: the raw GNUTLS_VERSION_NUMBER, packed as 0xMMNNPP, or 0 when
// unknown.
#ifndef D_ENV_TLS_GNUTLS_VERSION_NUMBER
    #if defined(GNUTLS_VERSION_NUMBER)
        #define D_ENV_TLS_GNUTLS_VERSION_NUMBER GNUTLS_VERSION_NUMBER
    #else
        #define D_ENV_TLS_GNUTLS_VERSION_NUMBER 0
    #endif
#endif  // D_ENV_TLS_GNUTLS_VERSION_NUMBER

// 2.5.4
// D_ENV_TLS_GNUTLS_AT_LEAST
//   macro: 1 if the GnuTLS version is at least major.minor.patch: (3, 6, 4)
// is 0x030604. Vacuously 0 when the version is unknown.
#ifndef D_ENV_TLS_GNUTLS_AT_LEAST
    #define D_ENV_TLS_GNUTLS_AT_LEAST(major, minor, patch)                     \
        ( D_ENV_TLS_GNUTLS_VERSION_NUMBER >=                                   \
          ( ((major) << 16) | ((minor) << 8) | (patch) ) )
#endif  // D_ENV_TLS_GNUTLS_AT_LEAST


//==============================================================================
// 3.  CAPABILITIES BY LIBRARY
//==============================================================================
// Each capability is answered for each library, D_ENV_TLS_<LIBRARY>_HAS_*,
// from that library's version or build options, and reads 0 where the
// library is absent. Where every library can be asked the same question,
// D_ENV_TLS_HAS_* forwards the answer of the one D_ENV_TLS_BACKEND selects;
// code bound to one library reads that library's flag instead. A library is
// described as built: a capability its build leaves out reads 0, and so does
// one only the running system decides, which section 5 answers instead.
//   The facts are each library's own documentation's: OpenSSL's release
// notes; LibreSSL's release notes and the OpenBSD manuals, which date a
// function by the first OpenBSD release to ship it (6.3 with LibreSSL 2.7.2,
// 6.5 with 2.9.1, 7.0 with 3.4.1); the mbedTLS and GnuTLS change logs and
// manuals; wolfSSL's; and Microsoft's and Apple's developer documentation.


// 3.1    TLS 1.3
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_TLS_OPENSSL_HAS_TLS1_3
//   feature: the OpenSSL family negotiates TLS 1.3 through the OpenSSL 1.1.1
// API (TLS1_3_VERSION and its kin), which is how a caller asks for it.
// OpenSSL brought both in 1.1.1, and BoringSSL has both. LibreSSL negotiates
// TLS 1.3 by default from 3.2.2, but its OpenSSL 1.1.1 TLS 1.3 API came in
// the 3.4 series, stable in 3.4.1.
#ifndef D_ENV_TLS_OPENSSL_HAS_TLS1_3
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_OPENSSL_HAS_TLS1_3 0
    #elif D_ENV_TLS_IS_BORINGSSL
        #define D_ENV_TLS_OPENSSL_HAS_TLS1_3 1
    #elif ( (D_ENV_TLS_IS_LIBRESSL) &&                                         \
            (D_ENV_TLS_LIBRESSL_AT_LEAST(3, 4, 1)) )
        #define D_ENV_TLS_OPENSSL_HAS_TLS1_3 1
    #elif ( (!D_ENV_TLS_IS_LIBRESSL) &&                                        \
            (D_ENV_TLS_OPENSSL_AT_LEAST(1, 1, 1)) )
        #define D_ENV_TLS_OPENSSL_HAS_TLS1_3 1
    #else
        #define D_ENV_TLS_OPENSSL_HAS_TLS1_3 0
    #endif
#endif  // D_ENV_TLS_OPENSSL_HAS_TLS1_3

// 3.1.2
// D_ENV_TLS_MBEDTLS_HAS_TLS1_3
//   feature: mbedTLS negotiates TLS 1.3: the release is 3.2 or later and its
// build enables MBEDTLS_SSL_PROTO_TLS1_3, as the default configuration does
// from 3.6. 3.1 offered a client only, which could not fall back to TLS 1.2;
// 3.2 added the server and the version negotiation.
#ifndef D_ENV_TLS_MBEDTLS_HAS_TLS1_3
    #if ( (D_ENV_TLS_HAS_MBEDTLS)             &&                               \
          (defined(MBEDTLS_SSL_PROTO_TLS1_3)) &&                               \
          (D_ENV_TLS_MBEDTLS_AT_LEAST(3, 2, 0)) )
        #define D_ENV_TLS_MBEDTLS_HAS_TLS1_3 1
    #else
        #define D_ENV_TLS_MBEDTLS_HAS_TLS1_3 0
    #endif
#endif  // D_ENV_TLS_MBEDTLS_HAS_TLS1_3

// 3.1.3
// D_ENV_TLS_GNUTLS_HAS_TLS1_3
//   feature: GnuTLS negotiates TLS 1.3: 3.6.4 brought the final protocol
// (RFC 8446) and enabled it by default.
#ifndef D_ENV_TLS_GNUTLS_HAS_TLS1_3
    #if ( (D_ENV_TLS_HAS_GNUTLS) &&                                            \
          (D_ENV_TLS_GNUTLS_AT_LEAST(3, 6, 4)) )
        #define D_ENV_TLS_GNUTLS_HAS_TLS1_3 1
    #else
        #define D_ENV_TLS_GNUTLS_HAS_TLS1_3 0
    #endif
#endif  // D_ENV_TLS_GNUTLS_HAS_TLS1_3

// 3.1.4
// D_ENV_TLS_WOLFSSL_HAS_TLS1_3
//   feature: wolfSSL negotiates TLS 1.3: its build defines WOLFSSL_TLS13
// (configure's --enable-tls13), and not WOLFSSL_TLS13_DRAFT, which speaks a
// draft's version numbers instead of RFC 8446's.
#ifndef D_ENV_TLS_WOLFSSL_HAS_TLS1_3
    #if ( (D_ENV_TLS_HAS_WOLFSSL)   &&                                         \
          (defined(WOLFSSL_TLS13)) &&                                          \
          (!defined(WOLFSSL_TLS13_DRAFT)) )
        #define D_ENV_TLS_WOLFSSL_HAS_TLS1_3 1
    #else
        #define D_ENV_TLS_WOLFSSL_HAS_TLS1_3 0
    #endif
#endif  // D_ENV_TLS_WOLFSSL_HAS_TLS1_3

// 3.1.5
// D_ENV_TLS_SCHANNEL_HAS_TLS1_3
//   feature: always 0. Schannel negotiates TLS 1.3 from Windows 11 and
// Windows Server 2022, so the answer belongs to the system a program runs
// on, not to its build; d_env_tls_backend_has_tls1_3 (section 5) asks it.
#ifndef D_ENV_TLS_SCHANNEL_HAS_TLS1_3
    #define D_ENV_TLS_SCHANNEL_HAS_TLS1_3 0
#endif  // D_ENV_TLS_SCHANNEL_HAS_TLS1_3

// 3.1.6
// D_ENV_TLS_SECURETRANSPORT_HAS_TLS1_3
//   feature: always 0. Secure Transport never negotiated TLS 1.3, which on
// Apple systems is Network.framework's.
#ifndef D_ENV_TLS_SECURETRANSPORT_HAS_TLS1_3
    #define D_ENV_TLS_SECURETRANSPORT_HAS_TLS1_3 0
#endif  // D_ENV_TLS_SECURETRANSPORT_HAS_TLS1_3

// 3.1.7
// D_ENV_TLS_HAS_TLS1_3
//   feature: the selected library negotiates TLS 1.3: its
// D_ENV_TLS_<LIBRARY>_HAS_TLS1_3, or 0 when no library is selected.
#ifndef D_ENV_TLS_HAS_TLS1_3
    #if (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_OPENSSL)
        #define D_ENV_TLS_HAS_TLS1_3 D_ENV_TLS_OPENSSL_HAS_TLS1_3
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_MBEDTLS)
        #define D_ENV_TLS_HAS_TLS1_3 D_ENV_TLS_MBEDTLS_HAS_TLS1_3
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_GNUTLS)
        #define D_ENV_TLS_HAS_TLS1_3 D_ENV_TLS_GNUTLS_HAS_TLS1_3
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_WOLFSSL)
        #define D_ENV_TLS_HAS_TLS1_3 D_ENV_TLS_WOLFSSL_HAS_TLS1_3
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_SCHANNEL)
        #define D_ENV_TLS_HAS_TLS1_3 D_ENV_TLS_SCHANNEL_HAS_TLS1_3
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_SECURETRANSPORT)
        #define D_ENV_TLS_HAS_TLS1_3 D_ENV_TLS_SECURETRANSPORT_HAS_TLS1_3
    #else
        #define D_ENV_TLS_HAS_TLS1_3 0
    #endif
#endif  // D_ENV_TLS_HAS_TLS1_3

// 3.2    Hostname validation
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION
//   feature: the OpenSSL family verifies a certificate's host name itself,
// given one by SSL_set1_host or X509_VERIFY_PARAM_set1_host: OpenSSL from
// 1.1.0, LibreSSL from 2.9.1 (OpenBSD 6.5), and BoringSSL, which added
// SSL_set1_host at the end of 2021.
#ifndef D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION 0
    #elif D_ENV_TLS_IS_BORINGSSL
        #define D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION 1
    #elif ( (D_ENV_TLS_IS_LIBRESSL) &&                                         \
            (D_ENV_TLS_LIBRESSL_AT_LEAST(2, 9, 1)) )
        #define D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION 1
    #elif ( (!D_ENV_TLS_IS_LIBRESSL) &&                                        \
            (D_ENV_TLS_OPENSSL_AT_LEAST(1, 1, 0)) )
        #define D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION 1
    #else
        #define D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION 0
    #endif
#endif  // D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION

// 3.2.2
// D_ENV_TLS_MBEDTLS_HAS_HOSTNAME_VALIDATION
//   feature: mbedTLS checks the peer's certificate against the name given to
// mbedtls_ssl_set_hostname, which its build provides with
// MBEDTLS_X509_CRT_PARSE_C.
#ifndef D_ENV_TLS_MBEDTLS_HAS_HOSTNAME_VALIDATION
    #if ( (D_ENV_TLS_HAS_MBEDTLS) &&                                           \
          (defined(MBEDTLS_X509_CRT_PARSE_C)) )
        #define D_ENV_TLS_MBEDTLS_HAS_HOSTNAME_VALIDATION 1
    #else
        #define D_ENV_TLS_MBEDTLS_HAS_HOSTNAME_VALIDATION 0
    #endif
#endif  // D_ENV_TLS_MBEDTLS_HAS_HOSTNAME_VALIDATION

// 3.2.3
// D_ENV_TLS_GNUTLS_HAS_HOSTNAME_VALIDATION
//   feature: GnuTLS verifies the peer's certificate against a host name
// during the handshake, given one by gnutls_session_set_verify_cert, which
// arrived in 3.4.6.
#ifndef D_ENV_TLS_GNUTLS_HAS_HOSTNAME_VALIDATION
    #if ( (D_ENV_TLS_HAS_GNUTLS) &&                                            \
          (D_ENV_TLS_GNUTLS_AT_LEAST(3, 4, 6)) )
        #define D_ENV_TLS_GNUTLS_HAS_HOSTNAME_VALIDATION 1
    #else
        #define D_ENV_TLS_GNUTLS_HAS_HOSTNAME_VALIDATION 0
    #endif
#endif  // D_ENV_TLS_GNUTLS_HAS_HOSTNAME_VALIDATION

// 3.2.4
// D_ENV_TLS_WOLFSSL_HAS_HOSTNAME_VALIDATION
//   feature: wolfSSL checks the peer's name during the handshake, given one
// by wolfSSL_check_domain_name, which every build declares.
#ifndef D_ENV_TLS_WOLFSSL_HAS_HOSTNAME_VALIDATION
    #if D_ENV_TLS_HAS_WOLFSSL
        #define D_ENV_TLS_WOLFSSL_HAS_HOSTNAME_VALIDATION 1
    #else
        #define D_ENV_TLS_WOLFSSL_HAS_HOSTNAME_VALIDATION 0
    #endif
#endif  // D_ENV_TLS_WOLFSSL_HAS_HOSTNAME_VALIDATION

// 3.2.5
// D_ENV_TLS_SCHANNEL_HAS_HOSTNAME_VALIDATION
//   feature: Schannel compares the target name a client gives it with the
// names in the server's certificate, by default; SCH_CRED_NO_SERVERNAME_CHECK
// is how a client opts out.
#ifndef D_ENV_TLS_SCHANNEL_HAS_HOSTNAME_VALIDATION
    #if D_ENV_TLS_HAS_SCHANNEL
        #define D_ENV_TLS_SCHANNEL_HAS_HOSTNAME_VALIDATION 1
    #else
        #define D_ENV_TLS_SCHANNEL_HAS_HOSTNAME_VALIDATION 0
    #endif
#endif  // D_ENV_TLS_SCHANNEL_HAS_HOSTNAME_VALIDATION

// 3.2.6
// D_ENV_TLS_SECURETRANSPORT_HAS_HOSTNAME_VALIDATION
//   feature: Secure Transport fails the handshake when the peer's certificate
// does not match the name given to SSLSetPeerDomainName.
#ifndef D_ENV_TLS_SECURETRANSPORT_HAS_HOSTNAME_VALIDATION
    #if D_ENV_TLS_HAS_SECURETRANSPORT
        #define D_ENV_TLS_SECURETRANSPORT_HAS_HOSTNAME_VALIDATION 1
    #else
        #define D_ENV_TLS_SECURETRANSPORT_HAS_HOSTNAME_VALIDATION 0
    #endif
#endif  // D_ENV_TLS_SECURETRANSPORT_HAS_HOSTNAME_VALIDATION

// 3.2.7
// D_ENV_TLS_HAS_HOSTNAME_VALIDATION
//   feature: the selected library verifies a certificate's host name itself:
// its D_ENV_TLS_<LIBRARY>_HAS_HOSTNAME_VALIDATION, or 0 when no library is
// selected.
#ifndef D_ENV_TLS_HAS_HOSTNAME_VALIDATION
    #if (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_OPENSSL)
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION                              \
            D_ENV_TLS_OPENSSL_HAS_HOSTNAME_VALIDATION
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_MBEDTLS)
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION                              \
            D_ENV_TLS_MBEDTLS_HAS_HOSTNAME_VALIDATION
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_GNUTLS)
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION                              \
            D_ENV_TLS_GNUTLS_HAS_HOSTNAME_VALIDATION
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_WOLFSSL)
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION                              \
            D_ENV_TLS_WOLFSSL_HAS_HOSTNAME_VALIDATION
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_SCHANNEL)
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION                              \
            D_ENV_TLS_SCHANNEL_HAS_HOSTNAME_VALIDATION
    #elif (D_ENV_TLS_BACKEND == D_ENV_TLS_BACKEND_SECURETRANSPORT)
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION                              \
            D_ENV_TLS_SECURETRANSPORT_HAS_HOSTNAME_VALIDATION
    #else
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION 0
    #endif
#endif  // D_ENV_TLS_HAS_HOSTNAME_VALIDATION

// 3.3    OpenSSL initialization
//------------------------------------------------------------------------------
// 3.3.1
// D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT
//   feature: the OpenSSL family initializes itself on first use, so explicit
// SSL_library_init / OpenSSL_add_all_algorithms is unnecessary: OpenSSL from
// 1.1.0, LibreSSL from 2.7.2 (OpenBSD 6.3), and BoringSSL. The question has
// no counterpart elsewhere -- wolfSSL, for one, still requires wolfSSL_Init
// -- so there is no roll-up. It was D_ENV_TLS_HAS_IMPLICIT_INIT.
#ifndef D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT 0
    #elif D_ENV_TLS_IS_BORINGSSL
        #define D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT 1
    #elif ( (D_ENV_TLS_IS_LIBRESSL) &&                                         \
            (D_ENV_TLS_LIBRESSL_AT_LEAST(2, 7, 2)) )
        #define D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT 1
    #elif ( (!D_ENV_TLS_IS_LIBRESSL) &&                                        \
            (D_ENV_TLS_OPENSSL_AT_LEAST(1, 1, 0)) )
        #define D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT 1
    #else
        #define D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT 0
    #endif
#endif  // D_ENV_TLS_OPENSSL_HAS_IMPLICIT_INIT


//==============================================================================
// 4.  AVAILABILITY GATE
//==============================================================================


// 4.1    Rolled-up availability
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_TLS_HAS_TLS
//   feature: 1 if any of the recognized TLS libraries is present.
#ifndef D_ENV_TLS_HAS_TLS
    #if (D_ENV_TLS_BACKEND != D_ENV_TLS_BACKEND_NONE)
        #define D_ENV_TLS_HAS_TLS 1
    #else
        #define D_ENV_TLS_HAS_TLS 0
    #endif
#endif  // D_ENV_TLS_HAS_TLS

// 4.1.2
// D_ENV_TLS_AVAILABLE
//   feature: 1 if TLS can actually run here -- a TLS backend plus a TCP
// transport to carry it. The gate a TLS backend header should #error on.
#ifndef D_ENV_TLS_AVAILABLE
    #if ( (D_ENV_TLS_HAS_TLS) &&                                               \
          (D_ENV_NET_CAN_TCP) )
        #define D_ENV_TLS_AVAILABLE 1
    #else
        #define D_ENV_TLS_AVAILABLE 0
    #endif
#endif  // D_ENV_TLS_AVAILABLE


//==============================================================================
// 5.  RUNTIME PROBES
//==============================================================================
// Declared here and defined in src/djinterp/env/net/tls/env_tls.c, for what
// the build cannot know about the system the program runs on.


// 5.1    Runtime probes
//------------------------------------------------------------------------------
//   C linkage for C++ callers. The env headers sit below djinterp.h, so the
// D_EXTERN_C_BEGIN / D_EXTERN_C_END pair is not available here.
#if D_ENV_LANG_USING_CPP
    extern "C" {
#endif

/**
 * @brief Reports whether a TLS library negotiates TLS 1.3 on the system the
 *        program runs on.
 *
 * @note Schannel's answer is the running Windows release's: Windows Server
 *       2022 (build 20348), Windows 11 and later negotiate TLS 1.3. A
 *       system policy that turns the protocol off is not consulted. Every
 *       other library's answer is its D_ENV_TLS_<LIBRARY>_HAS_TLS1_3 flag,
 *       which its build decides.
 *
 * @param[in] _backend  a D_ENV_TLS_BACKEND_* identifier; D_ENV_TLS_BACKEND
 *                      asks about the selected library.
 * @return `1` if the library negotiates TLS 1.3 here; `0` if it does not, if
 *         the build lacks it, or if `_backend` names no library.
 */
int d_env_tls_backend_has_tls1_3(int _backend);

#if D_ENV_LANG_USING_CPP
    }
#endif


#endif  // DJINTERP_ENV_NET_TLS_ENV_TLS_H
