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
*     - the OpenSSL version and variant (OpenSSL, LibreSSL, BoringSSL),
*       and the D_ENV_TLS_OPENSSL_AT_LEAST comparison helper             [2]
*     - version-gated OpenSSL capabilities (TLS 1.3, built-in hostname
*       verification, implicit library initialisation)                   [3]
*     - a rolled-up availability gate, D_ENV_TLS_AVAILABLE                [4]
*   Like env_curl.h, it is backend-specific detection built on the
* transport-level environment of env_net.h.
*   Naming: D_ENV_TLS_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_TLS_<FEATURE> is a non-boolean detected value or identifier.
*   Every flag is #ifndef-guarded, so a project may pre-define any D_ENV_TLS_*
* macro before inclusion to override detection.
*   It requires env_net.h, for D_ENV_NET_HAS_INCLUDE and D_ENV_NET_CAN_TCP,
* and includes it itself.
*
* path:      /inc/djinterp/env/net/env_tls.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.09.23
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
2.  OPENSSL VERSION AND VARIANT
    ---------------------------
    1.  Version probe
         1.  D_CFG_ENV_TLS_PROBE_VERSION
         2.  <openssl/opensslv.h>
    2.  Variant identifiers
         1.  D_ENV_TLS_OPENSSL_VARIANT_*
              1.  D_ENV_TLS_OPENSSL_VARIANT_NONE
              2.  D_ENV_TLS_OPENSSL_VARIANT_OPENSSL
              3.  D_ENV_TLS_OPENSSL_VARIANT_LIBRESSL
              4.  D_ENV_TLS_OPENSSL_VARIANT_BORINGSSL
    3.  Variant detection
         1.  D_ENV_TLS_IS_BORINGSSL / D_ENV_TLS_IS_LIBRESSL
         2.  D_ENV_TLS_OPENSSL_VARIANT
         3.  D_ENV_TLS_OPENSSL_VARIANT_NAME
    4.  Version number
         1.  D_ENV_TLS_OPENSSL_VERSION_NUMBER
         2.  D_ENV_TLS_OPENSSL_VERSION_MAJOR
         3.  D_ENV_TLS_OPENSSL_AT_LEAST
3.  VERSION-GATED CAPABILITIES
    --------------------------
    1.  Capability flags
         1.  D_ENV_TLS_HAS_TLS1_3
         2.  D_ENV_TLS_HAS_HOSTNAME_VALIDATION
         3.  D_ENV_TLS_HAS_IMPLICIT_INIT
4.  AVAILABILITY GATE
    -----------------
    1.  Rolled-up availability
         1.  D_ENV_TLS_HAS_TLS
         2.  D_ENV_TLS_AVAILABLE
*/

#ifndef DJINTERP_ENV_NET_ENV_TLS_H
#define DJINTERP_ENV_NET_ENV_TLS_H 1

// djinterp
#include "./env_net.h"  // D_ENV_NET_HAS_INCLUDE, D_ENV_NET_CAN_TCP, D_ENV_OS_ID


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
// 2.  OPENSSL VERSION AND VARIANT
//==============================================================================
// OpenSSL encodes its version as OPENSSL_VERSION_NUMBER. The 1.x line packs
// it as 0xMNNFFPPS and the 3.x line as 0xMNN00PP0, and the high nibble is the
// major version in both. The lower fields differ, though, so
// D_ENV_TLS_OPENSSL_AT_LEAST, which always packs its threshold the 3.x way,
// is exact from 3.0 on and at minor boundaries below it; see its note.
// LibreSSL and BoringSSL define their own identifying macros while pinning
// OPENSSL_VERSION_NUMBER for compatibility.


// 2.1    Version probe
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_TLS_PROBE_VERSION
//   config: set 0 before inclusion to suppress the automatic
// <openssl/opensslv.h> include. Version macros then fall back to "unknown"
// unless pre-defined.
#ifndef D_CFG_ENV_TLS_PROBE_VERSION
    #define D_CFG_ENV_TLS_PROBE_VERSION 1
#endif  // D_CFG_ENV_TLS_PROBE_VERSION

// 2.1.2
// <openssl/opensslv.h>
//   include: included when D_CFG_ENV_TLS_PROBE_VERSION is 1 and the header
// exists. It defines only the version macros and pulls in no code.
#if ( (D_CFG_ENV_TLS_PROBE_VERSION) &&                                         \
      (D_ENV_TLS_HAS_VERSION_HEADER) )
    // openssl
    #include <openssl/opensslv.h>  // OPENSSL_VERSION_NUMBER, ...
#endif

// 2.2    Variant identifiers
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

// 2.3    Variant detection
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_TLS_IS_BORINGSSL / D_ENV_TLS_IS_LIBRESSL
//   feature: distinguish the API-compatible forks from OpenSSL proper.
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

// 2.3.2
// D_ENV_TLS_OPENSSL_VARIANT
//   feature: which member of the OpenSSL family is present, as a
// D_ENV_TLS_OPENSSL_VARIANT_* identifier.
#ifndef D_ENV_TLS_OPENSSL_VARIANT
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_OPENSSL_VARIANT D_ENV_TLS_OPENSSL_VARIANT_NONE
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
    #if (D_ENV_TLS_OPENSSL_VARIANT == D_ENV_TLS_OPENSSL_VARIANT_BORINGSSL)
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "BoringSSL"
    #elif (D_ENV_TLS_OPENSSL_VARIANT == D_ENV_TLS_OPENSSL_VARIANT_LIBRESSL)
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "LibreSSL"
    #elif (D_ENV_TLS_OPENSSL_VARIANT == D_ENV_TLS_OPENSSL_VARIANT_OPENSSL)
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "OpenSSL"
    #else
        #define D_ENV_TLS_OPENSSL_VARIANT_NAME "none"
    #endif
#endif  // D_ENV_TLS_OPENSSL_VARIANT_NAME

// 2.4    Version number
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
//   macro: 1 if the OpenSSL version is at least major.minor.patch, comparing
// against a threshold packed the 3.x way (0xMNN00PP0). Vacuously 0 when the
// version is unknown. Not meaningful for LibreSSL (which pins the number); use
// the capability flags below instead on that fork. Below 3.0 the 1.x layout
// holds a patch letter where 3.x holds the patch number, so a nonzero patch
// is off there: (1, 1, 1) is met from 1.1.0a on, which is why
// D_ENV_TLS_HAS_TLS1_3 reads 1 for the lettered 1.1.0 releases.
#ifndef D_ENV_TLS_OPENSSL_AT_LEAST
    #define D_ENV_TLS_OPENSSL_AT_LEAST(major, minor, patch)                    \
        ( D_ENV_TLS_OPENSSL_VERSION_NUMBER >=                                  \
          ( ((major) << 28) | ((minor) << 20) | ((patch) << 4) ) )
#endif  // D_ENV_TLS_OPENSSL_AT_LEAST


//==============================================================================
// 3.  VERSION-GATED CAPABILITIES
//==============================================================================
// These describe the OpenSSL family only; they read 0 when it is absent,
// whichever other library is present.


// 3.1    Capability flags
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_TLS_HAS_TLS1_3
//   feature: TLS 1.3 is supported (OpenSSL >= 1.1.1; the forks advertise it via
// their own version macros -- assumed present there).
#ifndef D_ENV_TLS_HAS_TLS1_3
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_HAS_TLS1_3 0
    #elif ( (D_ENV_TLS_IS_LIBRESSL) ||                                         \
            (D_ENV_TLS_IS_BORINGSSL) )
        #define D_ENV_TLS_HAS_TLS1_3 1
    #elif D_ENV_TLS_OPENSSL_AT_LEAST(1, 1, 1)
        #define D_ENV_TLS_HAS_TLS1_3 1
    #else
        #define D_ENV_TLS_HAS_TLS1_3 0
    #endif
#endif  // D_ENV_TLS_HAS_TLS1_3

// 3.1.2
// D_ENV_TLS_HAS_HOSTNAME_VALIDATION
//   feature: built-in certificate hostname verification via
// SSL_set1_host / X509_VERIFY_PARAM_set1_host (OpenSSL >= 1.1.0; present on the
// forks).
#ifndef D_ENV_TLS_HAS_HOSTNAME_VALIDATION
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION 0
    #elif ( (D_ENV_TLS_IS_LIBRESSL) ||                                         \
            (D_ENV_TLS_IS_BORINGSSL) )
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION 1
    #elif D_ENV_TLS_OPENSSL_AT_LEAST(1, 1, 0)
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION 1
    #else
        #define D_ENV_TLS_HAS_HOSTNAME_VALIDATION 0
    #endif
#endif  // D_ENV_TLS_HAS_HOSTNAME_VALIDATION

// 3.1.3
// D_ENV_TLS_HAS_IMPLICIT_INIT
//   feature: the library initialises itself on first use (OpenSSL >= 1.1.0),
// so explicit SSL_library_init / OpenSSL_add_all_algorithms is unnecessary.
#ifndef D_ENV_TLS_HAS_IMPLICIT_INIT
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_TLS_HAS_IMPLICIT_INIT 0
    #elif ( (D_ENV_TLS_IS_LIBRESSL) ||                                         \
            (D_ENV_TLS_IS_BORINGSSL) )
        #define D_ENV_TLS_HAS_IMPLICIT_INIT 1
    #elif D_ENV_TLS_OPENSSL_AT_LEAST(1, 1, 0)
        #define D_ENV_TLS_HAS_IMPLICIT_INIT 1
    #else
        #define D_ENV_TLS_HAS_IMPLICIT_INIT 0
    #endif
#endif  // D_ENV_TLS_HAS_IMPLICIT_INIT


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


#endif  // DJINTERP_ENV_NET_ENV_TLS_H
