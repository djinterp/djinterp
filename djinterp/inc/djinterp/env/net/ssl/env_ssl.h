/*******************************************************************************
* djinterp [env]                                                       env_ssl.h
*
* djinterp SSL/TLS environment detection.
*   Compile-time detection of everything the SSL modules may depend on,
* expressed through the unified D_ENV_SSL_* interface. It covers:
*     - the stream transport TLS runs over                              [1]
*     - the TLS libraries an engine can bind, from env_tls.h            [2]
*     - the OpenSSL-family API surface, version-gated                   [3]
*     - the platform trust store and its candidate CA locations         [4]
*     - rolled-up capability summaries (CAN_OPENSSL, AVAILABLE)         [5]
*   The common kernel, net/ssl/ssl.h, links no TLS library and needs none
* of this to build; these flags gate the engines derived from it, by way of
* the D_CFG_SSL_* defaults in cfg_ssl.h.
*   Naming: D_ENV_SSL_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_SSL_<FEATURE> is a non-boolean detected value or identifier.
*   Every flag is #ifndef-guarded, so a project may pre-define any
* D_ENV_SSL_* macro before inclusion to override detection.
*   It requires env_net.h and env_tls.h, and includes them itself. It is an
* opt-in module and may be included directly.
*
*
* path:      /inc/djinterp/env/net/ssl/env_ssl.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSPORT PREREQUISITES
    -----------------------
    1.  Stream transport
         1.  D_ENV_SSL_HAS_TCP
2.  ENGINE LIBRARIES
    ----------------
    1.  Library availability
         1.  D_ENV_SSL_BACKEND
         2.  D_ENV_SSL_BACKEND_NAME
         3.  D_ENV_SSL_HAS_LIBRARY
3.  OPENSSL ENGINE API
    ------------------
    1.  Version floors
         1.  D_ENV_SSL_OPENSSL_MIN_VERSION
         2.  D_ENV_SSL_OPENSSL_1_1_1_VERSION
         3.  D_ENV_SSL_OPENSSL_3_0_VERSION
    2.  API surface
         1.  D_ENV_SSL_OPENSSL_USABLE
         2.  D_ENV_SSL_OPENSSL_HAS_TLS1_3
         3.  D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES
         4.  D_ENV_SSL_OPENSSL_HAS_KEYLOG
         5.  D_ENV_SSL_OPENSSL_HAS_GET1_PEER
4.  PLATFORM TRUST
    --------------
    1.  Trust store identifiers
         1.  D_ENV_SSL_TRUST_*
              1.  D_ENV_SSL_TRUST_NONE
              2.  D_ENV_SSL_TRUST_FILES
              3.  D_ENV_SSL_TRUST_WINDOWS
              4.  D_ENV_SSL_TRUST_KEYCHAIN
    2.  Trust store selection
         1.  D_ENV_SSL_TRUST_STORE
         2.  D_ENV_SSL_TRUST_STORE_NAME
    3.  Bundle locations
         1.  D_ENV_SSL_CA_FILES
         2.  D_ENV_SSL_CA_DIRS
5.  CAPABILITY SUMMARIES
    --------------------
    1.  Rolled-up capabilities
         1.  D_ENV_SSL_CAN_OPENSSL
         2.  D_ENV_SSL_AVAILABLE
*/

#ifndef DJINTERP_ENV_NET_SSL_ENV_SSL_H
#define DJINTERP_ENV_NET_SSL_ENV_SSL_H 1

// djinterp
#include "../env_net.h"      // D_ENV_NET_CAN_TCP, D_ENV_OS_ID, D_ENV_IS_OS_*
#include "../tls/env_tls.h"  // D_ENV_TLS_BACKEND, D_ENV_TLS_OPENSSL_VERSION_NUMBER


//==============================================================================
// 1.  TRANSPORT PREREQUISITES
//==============================================================================
// TLS runs over a reliable, ordered byte stream -- in practice TCP, which
// env_net.h already answers for. It is restated under the D_ENV_SSL_* name so
// an SSL module reads one family. The kernel itself runs over any
// d_ssl_transport, memory included, and needs none of this to build.


// 1.1    Stream transport
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_SSL_HAS_TCP
//   feature: 1 if a TCP stream with name resolution is usable.
#ifndef D_ENV_SSL_HAS_TCP
    #if D_ENV_NET_CAN_TCP
        #define D_ENV_SSL_HAS_TCP 1
    #else
        #define D_ENV_SSL_HAS_TCP 0
    #endif
#endif  // D_ENV_SSL_HAS_TCP


//==============================================================================
// 2.  ENGINE LIBRARIES
//==============================================================================
// An engine is the derived module that binds the kernel to one TLS library.
// Which libraries are installed is env_tls.h's question and is not asked a
// second time: these forward its answers, so the C SSL modules and the C++
// tls.hpp cannot disagree about what is present.


// 2.1    Library availability
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_SSL_BACKEND
//   value: the preferred TLS library, as a D_ENV_TLS_BACKEND_* identifier.
// The kernel's enum d_ssl_backend uses the same values, and ssl.c asserts it.
#ifndef D_ENV_SSL_BACKEND
    #define D_ENV_SSL_BACKEND D_ENV_TLS_BACKEND
#endif  // D_ENV_SSL_BACKEND

// 2.1.2
// D_ENV_SSL_BACKEND_NAME
//   value: a human-readable name for D_ENV_SSL_BACKEND.
#ifndef D_ENV_SSL_BACKEND_NAME
    #define D_ENV_SSL_BACKEND_NAME D_ENV_TLS_BACKEND_NAME
#endif  // D_ENV_SSL_BACKEND_NAME

// 2.1.3
// D_ENV_SSL_HAS_LIBRARY
//   feature: 1 if any recognized TLS library is present.
#ifndef D_ENV_SSL_HAS_LIBRARY
    #if (D_ENV_SSL_BACKEND != D_ENV_TLS_BACKEND_NONE)
        #define D_ENV_SSL_HAS_LIBRARY 1
    #else
        #define D_ENV_SSL_HAS_LIBRARY 0
    #endif
#endif  // D_ENV_SSL_HAS_LIBRARY


//==============================================================================
// 3.  OPENSSL ENGINE API
//==============================================================================
// The OpenSSL-family engine is written against the 1.1.0 API: opaque handles,
// automatic initialization, SSL_CTX_set_min_proto_version, and memory BIOs.
// Releases below that floor have been out of support since 2019 and are not
// built for. A few calls the engine uses are newer still, and are gated here.
//   Thresholds are compared against OPENSSL_VERSION_NUMBER in the 1.x packing
// (0xMNNFFPPS), not through D_ENV_TLS_OPENSSL_AT_LEAST, which is exact below
// 3.0 only at minor boundaries. LibreSSL pins the number at 0x20000000 and
// BoringSSL at 0x1010107F, so each fork is decided by name before any number
// is compared, and both are assumed to be current releases, as env_tls.h
// assumes. Everything in this section reads 0 when the family is absent.


// 3.1    Version floors
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_SSL_OPENSSL_MIN_VERSION
//   constant: the oldest OPENSSL_VERSION_NUMBER the engine accepts, 1.1.0.
#define D_ENV_SSL_OPENSSL_MIN_VERSION   0x10100000L

// 3.1.2
// D_ENV_SSL_OPENSSL_1_1_1_VERSION
//   constant: OpenSSL 1.1.1, which added TLS 1.3, SSL_CTX_set_ciphersuites,
// and SSL_CTX_set_keylog_callback.
#define D_ENV_SSL_OPENSSL_1_1_1_VERSION 0x10101000L

// 3.1.3
// D_ENV_SSL_OPENSSL_3_0_VERSION
//   constant: OpenSSL 3.0, which added SSL_get1_peer_certificate and
// deprecated SSL_get_peer_certificate.
#define D_ENV_SSL_OPENSSL_3_0_VERSION   0x30000000L

// 3.2    API surface
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_SSL_OPENSSL_USABLE
//   feature: 1 if the OpenSSL family is present at or above the engine's
// floor. An unknown version -- D_CFG_ENV_TLS_PROBE_VERSION set to 0 -- reads
// as unusable; pre-define this to 1 to vouch for the library instead.
#ifndef D_ENV_SSL_OPENSSL_USABLE
    #if !D_ENV_TLS_HAS_OPENSSL
        #define D_ENV_SSL_OPENSSL_USABLE 0
    #elif ( (D_ENV_TLS_IS_LIBRESSL) ||                                         \
            (D_ENV_TLS_IS_BORINGSSL) )
        #define D_ENV_SSL_OPENSSL_USABLE 1
    #elif (D_ENV_TLS_OPENSSL_VERSION_NUMBER >= D_ENV_SSL_OPENSSL_MIN_VERSION)
        #define D_ENV_SSL_OPENSSL_USABLE 1
    #else
        #define D_ENV_SSL_OPENSSL_USABLE 0
    #endif
#endif  // D_ENV_SSL_OPENSSL_USABLE

// 3.2.2
// D_ENV_SSL_OPENSSL_HAS_TLS1_3
//   feature: 1 if the usable library negotiates TLS 1.3 (env_tls.h's
// D_ENV_TLS_HAS_TLS1_3, restricted to a usable library).
#ifndef D_ENV_SSL_OPENSSL_HAS_TLS1_3
    #if ( (D_ENV_SSL_OPENSSL_USABLE) &&                                        \
          (D_ENV_TLS_HAS_TLS1_3) )
        #define D_ENV_SSL_OPENSSL_HAS_TLS1_3 1
    #else
        #define D_ENV_SSL_OPENSSL_HAS_TLS1_3 0
    #endif
#endif  // D_ENV_SSL_OPENSSL_HAS_TLS1_3

// 3.2.3
// D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES
//   feature: 1 if SSL_CTX_set_ciphersuites can choose the TLS 1.3 suites.
// BoringSSL fixes those suites and has no such call. LibreSSL gained it later
// than any version macro this header can read reliably, so it is not assumed;
// pre-define this to 1 for a LibreSSL known to provide it.
#ifndef D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES
    #if ( (!D_ENV_SSL_OPENSSL_USABLE) ||                                       \
          (D_ENV_TLS_IS_LIBRESSL)     ||                                       \
          (D_ENV_TLS_IS_BORINGSSL) )
        #define D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES 0
    #elif (D_ENV_TLS_OPENSSL_VERSION_NUMBER >= D_ENV_SSL_OPENSSL_1_1_1_VERSION)
        #define D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES 1
    #else
        #define D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES 0
    #endif
#endif  // D_ENV_SSL_OPENSSL_HAS_CIPHERSUITES

// 3.2.4
// D_ENV_SSL_OPENSSL_HAS_KEYLOG
//   feature: 1 if SSL_CTX_set_keylog_callback exists (OpenSSL 1.1.1 and
// BoringSSL; LibreSSL is not assumed, as for 3.2.3).
#ifndef D_ENV_SSL_OPENSSL_HAS_KEYLOG
    #if ( (!D_ENV_SSL_OPENSSL_USABLE) ||                                       \
          (D_ENV_TLS_IS_LIBRESSL) )
        #define D_ENV_SSL_OPENSSL_HAS_KEYLOG 0
    #elif D_ENV_TLS_IS_BORINGSSL
        #define D_ENV_SSL_OPENSSL_HAS_KEYLOG 1
    #elif (D_ENV_TLS_OPENSSL_VERSION_NUMBER >= D_ENV_SSL_OPENSSL_1_1_1_VERSION)
        #define D_ENV_SSL_OPENSSL_HAS_KEYLOG 1
    #else
        #define D_ENV_SSL_OPENSSL_HAS_KEYLOG 0
    #endif
#endif  // D_ENV_SSL_OPENSSL_HAS_KEYLOG

// 3.2.5
// D_ENV_SSL_OPENSSL_HAS_GET1_PEER
//   feature: 1 if SSL_get1_peer_certificate exists (OpenSSL 3.0). Where it
// does not, the engine falls back to SSL_get_peer_certificate, which the
// forks still provide undeprecated.
#ifndef D_ENV_SSL_OPENSSL_HAS_GET1_PEER
    #if ( (!D_ENV_SSL_OPENSSL_USABLE) ||                                       \
          (D_ENV_TLS_IS_LIBRESSL)     ||                                       \
          (D_ENV_TLS_IS_BORINGSSL) )
        #define D_ENV_SSL_OPENSSL_HAS_GET1_PEER 0
    #elif (D_ENV_TLS_OPENSSL_VERSION_NUMBER >= D_ENV_SSL_OPENSSL_3_0_VERSION)
        #define D_ENV_SSL_OPENSSL_HAS_GET1_PEER 1
    #else
        #define D_ENV_SSL_OPENSSL_HAS_GET1_PEER 0
    #endif
#endif  // D_ENV_SSL_OPENSSL_HAS_GET1_PEER


//==============================================================================
// 4.  PLATFORM TRUST
//==============================================================================
// Where this platform keeps the certificate authorities it trusts. Windows
// and Apple systems keep them in an OS store reached through an API; the Unix
// family keeps them in PEM files whose location varies by distribution. An
// engine whose library already knows the platform store uses it; one that
// does not (an embedded library, say) probes the candidate files below, which
// the kernel exposes at run time through d_ssl_ca_file_candidates.
//   The lists follow the ones Go's crypto/x509 has converged on. They are
// candidates, not facts: a path is only worth opening if it exists on the
// target, which only the target can tell.


// 4.1    Trust store identifiers
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_SSL_TRUST_*
//   constant: identifiers for D_ENV_SSL_TRUST_STORE.

// 4.1.1.1
// D_ENV_SSL_TRUST_NONE
//   constant: identifies no known platform trust store.
#define D_ENV_SSL_TRUST_NONE     0

// 4.1.1.2
// D_ENV_SSL_TRUST_FILES
//   constant: identifies PEM bundles and hashed directories on the file
// system.
#define D_ENV_SSL_TRUST_FILES    1

// 4.1.1.3
// D_ENV_SSL_TRUST_WINDOWS
//   constant: identifies the Windows certificate store (CryptoAPI "ROOT").
#define D_ENV_SSL_TRUST_WINDOWS  2

// 4.1.1.4
// D_ENV_SSL_TRUST_KEYCHAIN
//   constant: identifies the Apple keychain trust settings.
#define D_ENV_SSL_TRUST_KEYCHAIN 3

// 4.2    Trust store selection
//------------------------------------------------------------------------------
// 4.2.1
// D_ENV_SSL_TRUST_STORE
//   value: the platform's native trust store.
#ifndef D_ENV_SSL_TRUST_STORE
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        #define D_ENV_SSL_TRUST_STORE D_ENV_SSL_TRUST_WINDOWS
    #elif ( (D_ENV_OS_ID == D_ENV_OS_FLAG_APPLE) ||                            \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) ||                            \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_IOS) )
        #define D_ENV_SSL_TRUST_STORE D_ENV_SSL_TRUST_KEYCHAIN
    #elif ( (D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)) ||                           \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID) )
        #define D_ENV_SSL_TRUST_STORE D_ENV_SSL_TRUST_FILES
    #else
        #define D_ENV_SSL_TRUST_STORE D_ENV_SSL_TRUST_NONE
    #endif
#endif  // D_ENV_SSL_TRUST_STORE

// 4.2.2
// D_ENV_SSL_TRUST_STORE_NAME
//   value: a human-readable name for D_ENV_SSL_TRUST_STORE.
#ifndef D_ENV_SSL_TRUST_STORE_NAME
    #if (D_ENV_SSL_TRUST_STORE == D_ENV_SSL_TRUST_WINDOWS)
        #define D_ENV_SSL_TRUST_STORE_NAME "Windows certificate store"
    #elif (D_ENV_SSL_TRUST_STORE == D_ENV_SSL_TRUST_KEYCHAIN)
        #define D_ENV_SSL_TRUST_STORE_NAME "keychain"
    #elif (D_ENV_SSL_TRUST_STORE == D_ENV_SSL_TRUST_FILES)
        #define D_ENV_SSL_TRUST_STORE_NAME "files"
    #else
        #define D_ENV_SSL_TRUST_STORE_NAME "none"
    #endif
#endif  // D_ENV_SSL_TRUST_STORE_NAME

// 4.3    Bundle locations
//------------------------------------------------------------------------------
// 4.3.1
// D_ENV_SSL_CA_FILES
//   value: the PEM bundle paths worth probing on this platform, most likely
// first, as string literals EACH FOLLOWED BY A COMMA, so the list can open an
// initializer that the user closes: `{ D_ENV_SSL_CA_FILES NULL }`. Empty where
// the platform keeps no bundle. On macOS the file is a fallback for engines
// that cannot read the keychain.
#ifndef D_ENV_SSL_CA_FILES
    #if (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)
        #define D_ENV_SSL_CA_FILES                                             \
            "/etc/ssl/certs/ca-certificates.crt",                              \
            "/etc/pki/tls/certs/ca-bundle.crt",                                \
            "/etc/ssl/ca-bundle.pem",                                          \
            "/etc/pki/tls/cacert.pem",                                         \
            "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",               \
            "/etc/ssl/cert.pem",
    #elif ( (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_FREE) ||                         \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_DRAGONFLY) )
        #define D_ENV_SSL_CA_FILES                                             \
            "/usr/local/etc/ssl/cert.pem",                                     \
            "/etc/ssl/cert.pem",                                               \
            "/usr/local/share/certs/ca-root-nss.crt",
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_OPEN)
        #define D_ENV_SSL_CA_FILES                                             \
            "/etc/ssl/cert.pem",
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_NET)
        #define D_ENV_SSL_CA_FILES                                             \
            "/etc/openssl/certs/ca-certificates.crt",
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_SOLARIS)
        #define D_ENV_SSL_CA_FILES                                             \
            "/etc/certs/ca-certificates.crt",                                  \
            "/etc/ssl/certs/ca-certificates.crt",                              \
            "/etc/ssl/cacert.pem",
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_AIX)
        #define D_ENV_SSL_CA_FILES                                             \
            "/var/ssl/certs/ca-bundle.crt",
    #elif ( (D_ENV_OS_ID == D_ENV_OS_FLAG_APPLE) ||                            \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) )
        #define D_ENV_SSL_CA_FILES                                             \
            "/etc/ssl/cert.pem",
    #elif (D_ENV_SSL_TRUST_STORE == D_ENV_SSL_TRUST_FILES)
        #define D_ENV_SSL_CA_FILES                                             \
            "/etc/ssl/certs/ca-certificates.crt",                              \
            "/etc/ssl/cert.pem",
    #else
        #define D_ENV_SSL_CA_FILES
    #endif
#endif  // D_ENV_SSL_CA_FILES

// 4.3.2
// D_ENV_SSL_CA_DIRS
//   value: the directories of individually stored CA certificates worth
// probing, in the same comma-terminated form as D_ENV_SSL_CA_FILES. Most are
// OpenSSL "hashed" directories and useful only to an OpenSSL-family engine;
// Android's hold one DER or PEM certificate per file under a hashed name.
#ifndef D_ENV_SSL_CA_DIRS
    #if (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID)
        #define D_ENV_SSL_CA_DIRS                                              \
            "/apex/com.android.conscrypt/cacerts",                             \
            "/system/etc/security/cacerts",
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)
        #define D_ENV_SSL_CA_DIRS                                              \
            "/etc/ssl/certs",                                                  \
            "/etc/pki/tls/certs",
    #elif ( (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_FREE) ||                         \
            (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_DRAGONFLY) )
        #define D_ENV_SSL_CA_DIRS                                              \
            "/etc/ssl/certs",
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_BSD_NET)
        #define D_ENV_SSL_CA_DIRS                                              \
            "/etc/openssl/certs",
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_SOLARIS)
        #define D_ENV_SSL_CA_DIRS                                              \
            "/etc/certs/CA",
    #elif (D_ENV_OS_ID == D_ENV_OS_FLAG_AIX)
        #define D_ENV_SSL_CA_DIRS                                              \
            "/var/ssl/certs",
    #else
        #define D_ENV_SSL_CA_DIRS
    #endif
#endif  // D_ENV_SSL_CA_DIRS


//==============================================================================
// 5.  CAPABILITY SUMMARIES
//==============================================================================
// What the SSL modules can do on this target. None of these gate the common
// kernel (net/ssl/ssl.h), which is library-free and builds anywhere; they gate
// the engines derived from it, by way of the D_CFG_SSL_* defaults in
// cfg_ssl.h. As engines for further libraries arrive, each adds a CAN_ flag
// here and joins D_ENV_SSL_AVAILABLE.


// 5.1    Rolled-up capabilities
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_SSL_CAN_OPENSSL
//   feature: 1 if the OpenSSL-family engine can be built. The engine moves
// no bytes itself, so it needs the library and nothing else.
#ifndef D_ENV_SSL_CAN_OPENSSL
    #define D_ENV_SSL_CAN_OPENSSL D_ENV_SSL_OPENSSL_USABLE
#endif  // D_ENV_SSL_CAN_OPENSSL

// 5.1.2
// D_ENV_SSL_AVAILABLE
//   feature: 1 if TLS can run end to end here: some buildable engine, and a
// TCP transport to carry it. The gate a socket-bound SSL module should
// #error on.
#ifndef D_ENV_SSL_AVAILABLE
    #if ( (D_ENV_SSL_CAN_OPENSSL) &&                                           \
          (D_ENV_SSL_HAS_TCP) )
        #define D_ENV_SSL_AVAILABLE 1
    #else
        #define D_ENV_SSL_AVAILABLE 0
    #endif
#endif  // D_ENV_SSL_AVAILABLE


#endif  // DJINTERP_ENV_NET_SSL_ENV_SSL_H
