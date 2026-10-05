/*******************************************************************************
* djinterp [env]                                                      env_imap.h
*
* djinterp IMAP environment detection.
*   The single home for all compile-time detection tied to IMAP, expressed
* through the D_ENV_IMAP_* interface:
*     - the version-probe switch shared by the libraries below            [1]
*     - readiness of the transport a native engine runs on (TCP, TLS)     [2]
*     - libcurl's IMAP surface, read from env_curl.h's version            [3]
*     - libetpan, the C IMAP client library, and its version              [4]
*     - GNU Mailutils, and VMime for C++, by presence and build config    [5]
*     - SASL libraries for AUTHENTICATE (Cyrus SASL, GNU SASL)            [6]
*     - backend identifiers and the preferred library backend             [7]
*     - rolled-up availability gates                                      [8]
*   It consumes detection that already has a home rather than repeating it:
* TCP and TLS come from env_net.h and env_tls.h, and every libcurl fact is a
* threshold on env_curl.h's version number. What is new here is the IMAP
* reading of those facts, and the libraries that exist only for mail.
*   Detection versus runtime: header presence and version macros are reliable
* at compile time. Whether a particular libcurl or libetpan binary was built
* with IMAP or with TLS is invisible to the preprocessor (libcurl reports it at
* runtime, through curl_version_info). VMime is the exception, since its
* generated config header records its build. This header answers "is this API
* present to call?", not "will it succeed?".
*   The native engine is the framework's own module rather than an installed
* library, so detection cannot see whether it exists: [2] reports only whether
* its transport does, and [7] never selects it on its own.
*   Every flag is #ifndef-guarded, so a project may pre-define any
* D_ENV_IMAP_* macro to override detection, for example to describe a
* cross-compilation target whose headers the probing compiler cannot see.
*   It requires env_net.h, env_tls.h, and env_curl.h, and includes them
* itself. It is an opt-in module and may be included directly.
*
*
* path:      /inc/djinterp/env/net/imap/env_imap.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VERSION PROBE
    -------------
    1.  Probe control
         1.  D_CFG_ENV_IMAP_PROBE_VERSION
2.  NATIVE TRANSPORT
    ----------------
    1.  Transport readiness
         1.  D_ENV_IMAP_CAN_NATIVE
         2.  D_ENV_IMAP_CAN_NATIVE_TLS
3.  LIBCURL
    -------
    1.  IMAP API surface
         1.  D_ENV_IMAP_CURL_HAS_PROTOCOL
         2.  D_ENV_IMAP_CURL_HAS_SASL_IR
         3.  D_ENV_IMAP_CURL_HAS_XOAUTH2
         4.  D_ENV_IMAP_CURL_HAS_LOGIN_OPTIONS
         5.  D_ENV_IMAP_CURL_HAS_SASL_AUTHZID
    2.  Availability
         1.  D_ENV_IMAP_CURL_AVAILABLE
4.  LIBETPAN
    --------
    1.  Headers
         1.  D_ENV_IMAP_HAS_LIBETPAN
         2.  D_ENV_IMAP_HAS_LIBETPAN_VERSION_HEADER
    2.  Version
         1.  <libetpan/libetpan_version.h>
         2.  D_ENV_IMAP_LIBETPAN_VERSION_MAJOR
         3.  D_ENV_IMAP_LIBETPAN_VERSION_MINOR
         4.  D_ENV_IMAP_LIBETPAN_VERSION_NUM
         5.  D_ENV_IMAP_LIBETPAN_AT_LEAST
    3.  Availability
         1.  D_ENV_IMAP_LIBETPAN_AVAILABLE
5.  OTHER CLIENT LIBRARIES
    ----------------------
    1.  GNU Mailutils
         1.  D_ENV_IMAP_HAS_MAILUTILS
         2.  D_ENV_IMAP_MAILUTILS_AVAILABLE
    2.  VMime
         1.  D_ENV_IMAP_HAS_VMIME
         2.  <vmime/config.hpp>
         3.  D_ENV_IMAP_VMIME_VERSION_STRING
         4.  D_ENV_IMAP_VMIME_HAS_IMAP
         5.  D_ENV_IMAP_VMIME_AVAILABLE
6.  SASL LIBRARIES
    --------------
    1.  Cyrus SASL
         1.  D_ENV_IMAP_HAS_CYRUS_SASL
         2.  D_ENV_IMAP_CYRUS_SASL_VERSION_NUM
    2.  GNU SASL
         1.  D_ENV_IMAP_HAS_GSASL
         2.  D_ENV_IMAP_HAS_GSASL_VERSION_HEADER
         3.  <gsasl-version.h>
         4.  D_ENV_IMAP_GSASL_VERSION_NUM
         5.  D_ENV_IMAP_GSASL_AT_LEAST
    3.  SASL selection
         1.  D_ENV_IMAP_SASL_BACKEND_*
              1.  D_ENV_IMAP_SASL_BACKEND_NONE
              2.  D_ENV_IMAP_SASL_BACKEND_CYRUS
              3.  D_ENV_IMAP_SASL_BACKEND_GSASL
         2.  D_ENV_IMAP_SASL_BACKEND
         3.  D_ENV_IMAP_SASL_BACKEND_NAME
         4.  D_ENV_IMAP_HAS_SASL
7.  BACKEND SELECTION
    -----------------
    1.  Backend identifiers
         1.  D_ENV_IMAP_BACKEND_*
              1.  D_ENV_IMAP_BACKEND_NONE
              2.  D_ENV_IMAP_BACKEND_NATIVE
              3.  D_ENV_IMAP_BACKEND_CURL
              4.  D_ENV_IMAP_BACKEND_LIBETPAN
              5.  D_ENV_IMAP_BACKEND_MAILUTILS
              6.  D_ENV_IMAP_BACKEND_VMIME
    2.  Preferred library backend
         1.  D_ENV_IMAP_BACKEND
         2.  D_ENV_IMAP_BACKEND_NAME
8.  AVAILABILITY SUMMARY
    --------------------
    1.  Rolled-up availability
         1.  D_ENV_IMAP_HAS_LIBRARY
         2.  D_ENV_IMAP_AVAILABLE
*/

#ifndef DJINTERP_ENV_NET_IMAP_ENV_IMAP_H
#define DJINTERP_ENV_NET_IMAP_ENV_IMAP_H 1

// djinterp
#include "../env_net.h"        // D_ENV_NET_HAS_INCLUDE, D_ENV_NET_CAN_TCP
#include "../tls/env_tls.h"    // D_ENV_TLS_AVAILABLE
#include "../curl/env_curl.h"  // D_ENV_CURL_AVAILABLE, D_ENV_CURL_VERSION_AT_LEAST


//==============================================================================
// 1.  VERSION PROBE
//==============================================================================
// Three libraries publish their version in a header that can be included on
// its own: libetpan (libetpan_version.h), GNU SASL (gsasl-version.h), and
// VMime (vmime/config.hpp, C++ only). One switch governs all three, as
// D_CFG_ENV_CURL_PROBE_VERSION does for libcurl.


// 1.1    Probe control
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_IMAP_PROBE_VERSION
//   config: set 0 before inclusion to suppress the automatic version-header
// includes below, so that detection touches no third-party header. Version
// macros then fall back to "unknown" unless pre-defined, and VMime reports no
// IMAP support, since its build configuration is where that is recorded.
#ifndef D_CFG_ENV_IMAP_PROBE_VERSION
    #define D_CFG_ENV_IMAP_PROBE_VERSION 1
#endif  // D_CFG_ENV_IMAP_PROBE_VERSION


//==============================================================================
// 2.  NATIVE TRANSPORT
//==============================================================================
// What a native engine -- the framework speaking IMAP itself over the net
// transport -- would run on. IMAP is a TCP protocol; implicit TLS (port 993)
// and STARTTLS both need a TLS backend on top. Most servers accept
// credentials only over TLS (RFC 8314), so plaintext-only readiness serves
// loopback and test servers but rarely production.


// 2.1    Transport readiness
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_IMAP_CAN_NATIVE
//   feature: 1 if TCP stream sockets with name resolution are usable
// (D_ENV_NET_CAN_TCP), which is all a plaintext IMAP session needs.
#ifndef D_ENV_IMAP_CAN_NATIVE
    #if D_ENV_NET_CAN_TCP
        #define D_ENV_IMAP_CAN_NATIVE 1
    #else
        #define D_ENV_IMAP_CAN_NATIVE 0
    #endif
#endif  // D_ENV_IMAP_CAN_NATIVE

// 2.1.2
// D_ENV_IMAP_CAN_NATIVE_TLS
//   feature: 1 if a TLS backend can also run over that transport
// (D_ENV_TLS_AVAILABLE), so a native engine could offer implicit TLS and
// STARTTLS. The net TLS transport (net/tls.hpp) is written against the
// OpenSSL family specifically; a native engine built on it should also test
// D_ENV_TLS_HAS_OPENSSL.
#ifndef D_ENV_IMAP_CAN_NATIVE_TLS
    #if ( (D_ENV_IMAP_CAN_NATIVE) &&                                           \
          (D_ENV_TLS_AVAILABLE) )
        #define D_ENV_IMAP_CAN_NATIVE_TLS 1
    #else
        #define D_ENV_IMAP_CAN_NATIVE_TLS 0
    #endif
#endif  // D_ENV_IMAP_CAN_NATIVE_TLS


//==============================================================================
// 3.  LIBCURL
//==============================================================================
// libcurl speaks IMAP through imap:// and imaps:// URLs: LIST, SELECT with
// FETCH, SEARCH, and APPEND map onto URLs and uploads, and any other command
// is sent with CURLOPT_CUSTOMREQUEST. It has no IDLE. Each flag below is a
// threshold on env_curl.h's D_ENV_CURL_VERSION_NUM, taken from libcurl's
// symbols-in-versions record, so this section probes nothing itself. An
// unknown libcurl version (0) reads as absent throughout.


// 3.1    IMAP API surface
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_IMAP_CURL_HAS_PROTOCOL
//   feature: the IMAP and IMAPS protocols and their CURLPROTO_IMAP /
// CURLPROTO_IMAPS identifiers, introduced in libcurl 7.20.0. Whether a given
// libcurl binary was built with them is a runtime question
// (curl_version_info()->protocols).
#ifndef D_ENV_IMAP_CURL_HAS_PROTOCOL
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 20, 0)
        #define D_ENV_IMAP_CURL_HAS_PROTOCOL 1
    #else
        #define D_ENV_IMAP_CURL_HAS_PROTOCOL 0
    #endif
#endif  // D_ENV_IMAP_CURL_HAS_PROTOCOL

// 3.1.2
// D_ENV_IMAP_CURL_HAS_SASL_IR
//   feature: CURLOPT_SASL_IR, which sends the SASL initial response on the
// AUTHENTICATE line itself (RFC 4959) and saves a round trip; introduced in
// libcurl 7.31.0.
#ifndef D_ENV_IMAP_CURL_HAS_SASL_IR
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 31, 0)
        #define D_ENV_IMAP_CURL_HAS_SASL_IR 1
    #else
        #define D_ENV_IMAP_CURL_HAS_SASL_IR 0
    #endif
#endif  // D_ENV_IMAP_CURL_HAS_SASL_IR

// 3.1.3
// D_ENV_IMAP_CURL_HAS_XOAUTH2
//   feature: CURLOPT_XOAUTH2_BEARER, the OAuth 2.0 bearer token for the
// XOAUTH2 mechanism, introduced in libcurl 7.33.0. Later releases pass the
// same option to OAUTHBEARER.
#ifndef D_ENV_IMAP_CURL_HAS_XOAUTH2
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 33, 0)
        #define D_ENV_IMAP_CURL_HAS_XOAUTH2 1
    #else
        #define D_ENV_IMAP_CURL_HAS_XOAUTH2 0
    #endif
#endif  // D_ENV_IMAP_CURL_HAS_XOAUTH2

// 3.1.4
// D_ENV_IMAP_CURL_HAS_LOGIN_OPTIONS
//   feature: CURLOPT_LOGIN_OPTIONS, which pins the mechanism (for example
// "AUTH=PLAIN") rather than leaving the choice to libcurl; introduced in
// libcurl 7.34.0.
#ifndef D_ENV_IMAP_CURL_HAS_LOGIN_OPTIONS
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 34, 0)
        #define D_ENV_IMAP_CURL_HAS_LOGIN_OPTIONS 1
    #else
        #define D_ENV_IMAP_CURL_HAS_LOGIN_OPTIONS 0
    #endif
#endif  // D_ENV_IMAP_CURL_HAS_LOGIN_OPTIONS

// 3.1.5
// D_ENV_IMAP_CURL_HAS_SASL_AUTHZID
//   feature: CURLOPT_SASL_AUTHZID, the authorization identity SASL PLAIN may
// carry to act as another user; introduced in libcurl 7.66.0.
#ifndef D_ENV_IMAP_CURL_HAS_SASL_AUTHZID
    #if D_ENV_CURL_VERSION_AT_LEAST(7, 66, 0)
        #define D_ENV_IMAP_CURL_HAS_SASL_AUTHZID 1
    #else
        #define D_ENV_IMAP_CURL_HAS_SASL_AUTHZID 0
    #endif
#endif  // D_ENV_IMAP_CURL_HAS_SASL_AUTHZID

// 3.2    Availability
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_IMAP_CURL_AVAILABLE
//   feature: 1 when an IMAP backend over libcurl can be compiled here --
// libcurl itself is usable (D_ENV_CURL_AVAILABLE) and its version carries the
// IMAP protocol. This is the gate a libcurl IMAP module keys on.
#ifndef D_ENV_IMAP_CURL_AVAILABLE
    #if ( (D_ENV_CURL_AVAILABLE) &&                                            \
          (D_ENV_IMAP_CURL_HAS_PROTOCOL) )
        #define D_ENV_IMAP_CURL_AVAILABLE 1
    #else
        #define D_ENV_IMAP_CURL_AVAILABLE 0
    #endif
#endif  // D_ENV_IMAP_CURL_AVAILABLE


//==============================================================================
// 4.  LIBETPAN
//==============================================================================
// libEtPan! is a C mail library whose mailimap API covers IMAP4rev1 and the
// widely deployed extensions (IDLE, CONDSTORE, QRESYNC, ...), with its own
// socket and TLS layer. It publishes major.minor in a standalone header.


// 4.1    Headers
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_IMAP_HAS_LIBETPAN
//   feature: detect if the IMAP client API header <libetpan/mailimap.h> is
// includable.
#ifndef D_ENV_IMAP_HAS_LIBETPAN
    #if D_ENV_NET_HAS_INCLUDE(<libetpan/mailimap.h>)
        #define D_ENV_IMAP_HAS_LIBETPAN 1
    #else
        #define D_ENV_IMAP_HAS_LIBETPAN 0
    #endif
#endif  // D_ENV_IMAP_HAS_LIBETPAN

// 4.1.2
// D_ENV_IMAP_HAS_LIBETPAN_VERSION_HEADER
//   feature: detect if the standalone version header
// <libetpan/libetpan_version.h> is includable.
#ifndef D_ENV_IMAP_HAS_LIBETPAN_VERSION_HEADER
    #if D_ENV_NET_HAS_INCLUDE(<libetpan/libetpan_version.h>)
        #define D_ENV_IMAP_HAS_LIBETPAN_VERSION_HEADER 1
    #else
        #define D_ENV_IMAP_HAS_LIBETPAN_VERSION_HEADER 0
    #endif
#endif  // D_ENV_IMAP_HAS_LIBETPAN_VERSION_HEADER

// 4.2    Version
//------------------------------------------------------------------------------
// 4.2.1
// <libetpan/libetpan_version.h>
//   include: included when D_CFG_ENV_IMAP_PROBE_VERSION is 1 and the header
// exists. Besides its macros it declares libetpan_get_version_major() and
// libetpan_get_version_minor() with no linkage specification of its own;
// libetpan.h supplies one by including it inside an extern "C" block. A C++
// translation unit reaching it here first, outside such a block, would fix
// C++ linkage on both declarations behind the header's include guard, and a
// later call through libetpan.h would fail to link. So the include below
// supplies the same block.
#if ( (D_CFG_ENV_IMAP_PROBE_VERSION) &&                                        \
      (D_ENV_IMAP_HAS_LIBETPAN_VERSION_HEADER) )
    #ifdef __cplusplus
        extern "C" {
    #endif  // __cplusplus
    // libetpan
    #include <libetpan/libetpan_version.h>  // LIBETPAN_VERSION_MAJOR / _MINOR
    #ifdef __cplusplus
        }
    #endif  // __cplusplus
#endif

// 4.2.2
// D_ENV_IMAP_LIBETPAN_VERSION_MAJOR
//   feature: the major component of the detected version (0 when unknown).
#ifndef D_ENV_IMAP_LIBETPAN_VERSION_MAJOR
    #ifdef LIBETPAN_VERSION_MAJOR
        #define D_ENV_IMAP_LIBETPAN_VERSION_MAJOR LIBETPAN_VERSION_MAJOR
    #else
        #define D_ENV_IMAP_LIBETPAN_VERSION_MAJOR 0
    #endif  // LIBETPAN_VERSION_MAJOR
#endif  // D_ENV_IMAP_LIBETPAN_VERSION_MAJOR

// 4.2.3
// D_ENV_IMAP_LIBETPAN_VERSION_MINOR
//   feature: the minor component of the detected version (0 when unknown).
#ifndef D_ENV_IMAP_LIBETPAN_VERSION_MINOR
    #ifdef LIBETPAN_VERSION_MINOR
        #define D_ENV_IMAP_LIBETPAN_VERSION_MINOR LIBETPAN_VERSION_MINOR
    #else
        #define D_ENV_IMAP_LIBETPAN_VERSION_MINOR 0
    #endif  // LIBETPAN_VERSION_MINOR
#endif  // D_ENV_IMAP_LIBETPAN_VERSION_MINOR

// 4.2.4
// D_ENV_IMAP_LIBETPAN_VERSION_NUM
//   feature: the version packed as 0xMMNN (major, minor), or 0 when unknown.
#ifndef D_ENV_IMAP_LIBETPAN_VERSION_NUM
    #define D_ENV_IMAP_LIBETPAN_VERSION_NUM                                    \
        ( (D_ENV_IMAP_LIBETPAN_VERSION_MAJOR << 8) |                           \
          (D_ENV_IMAP_LIBETPAN_VERSION_MINOR) )
#endif  // D_ENV_IMAP_LIBETPAN_VERSION_NUM

// 4.2.5
// D_ENV_IMAP_LIBETPAN_AT_LEAST
//   macro: evaluates to 1 if the detected libetpan version is at least
// major.minor. False for any nonzero threshold when the version is unknown
// (0), so it doubles as an "is libetpan present at this version" test.
#ifndef D_ENV_IMAP_LIBETPAN_AT_LEAST
    #define D_ENV_IMAP_LIBETPAN_AT_LEAST(major, minor)                         \
        ( D_ENV_IMAP_LIBETPAN_VERSION_NUM >=                                   \
          ( ((major) << 8) | (minor) ) )
#endif  // D_ENV_IMAP_LIBETPAN_AT_LEAST

// 4.3    Availability
//------------------------------------------------------------------------------
// 4.3.1
// D_ENV_IMAP_LIBETPAN_AVAILABLE
//   feature: 1 when a libetpan IMAP backend can be compiled here -- the
// mailimap header is present, and the platform has the TCP transport that
// libetpan's own socket layer drives. No version floor is imposed; mailimap is
// part of every libetpan release.
#ifndef D_ENV_IMAP_LIBETPAN_AVAILABLE
    #if ( (D_ENV_IMAP_HAS_LIBETPAN) &&                                         \
          (D_ENV_NET_CAN_TCP) )
        #define D_ENV_IMAP_LIBETPAN_AVAILABLE 1
    #else
        #define D_ENV_IMAP_LIBETPAN_AVAILABLE 0
    #endif
#endif  // D_ENV_IMAP_LIBETPAN_AVAILABLE


//==============================================================================
// 5.  OTHER CLIENT LIBRARIES
//==============================================================================
// GNU Mailutils ships an IMAP client API (mu_imap_*) but no version macro, so
// it is detected by presence alone. VMime is a C++ library whose generated
// config header records which protocols the installed build includes, which
// makes "built with IMAP" a compile-time fact for VMime alone. That header
// includes <cstdint>, so only a C++ compiler reads it.


// 5.1    GNU Mailutils
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_IMAP_HAS_MAILUTILS
//   feature: detect if the Mailutils IMAP client header <mailutils/imap.h> is
// includable.
#ifndef D_ENV_IMAP_HAS_MAILUTILS
    #if D_ENV_NET_HAS_INCLUDE(<mailutils/imap.h>)
        #define D_ENV_IMAP_HAS_MAILUTILS 1
    #else
        #define D_ENV_IMAP_HAS_MAILUTILS 0
    #endif
#endif  // D_ENV_IMAP_HAS_MAILUTILS

// 5.1.2
// D_ENV_IMAP_MAILUTILS_AVAILABLE
//   feature: 1 when a Mailutils IMAP backend can be compiled here -- the
// header is present and the platform has a TCP transport.
#ifndef D_ENV_IMAP_MAILUTILS_AVAILABLE
    #if ( (D_ENV_IMAP_HAS_MAILUTILS) &&                                        \
          (D_ENV_NET_CAN_TCP) )
        #define D_ENV_IMAP_MAILUTILS_AVAILABLE 1
    #else
        #define D_ENV_IMAP_MAILUTILS_AVAILABLE 0
    #endif
#endif  // D_ENV_IMAP_MAILUTILS_AVAILABLE

// 5.2    VMime
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_IMAP_HAS_VMIME
//   feature: detect if the VMime umbrella header <vmime/vmime.hpp> is
// includable. Presence is reported to both languages; only C++ can use it.
#ifndef D_ENV_IMAP_HAS_VMIME
    #if D_ENV_NET_HAS_INCLUDE(<vmime/vmime.hpp>)
        #define D_ENV_IMAP_HAS_VMIME 1
    #else
        #define D_ENV_IMAP_HAS_VMIME 0
    #endif
#endif  // D_ENV_IMAP_HAS_VMIME

// 5.2.2
// <vmime/config.hpp>
//   include: included when the compiler is C++, D_CFG_ENV_IMAP_PROBE_VERSION
// is 1, and VMime is present. Apart from <cstdint> and a handful of global
// vmime_int* typedefs it defines only macros, and adds no link dependency.
#if ( (defined(__cplusplus))         &&                                        \
      (D_CFG_ENV_IMAP_PROBE_VERSION) &&                                        \
      (D_ENV_IMAP_HAS_VMIME) )
    // vmime
    #include <vmime/config.hpp>  // VMIME_VERSION, VMIME_HAVE_MESSAGING_*
#endif

// 5.2.3
// D_ENV_IMAP_VMIME_VERSION_STRING
//   feature: the dotted version string, or "unknown".
#ifndef D_ENV_IMAP_VMIME_VERSION_STRING
    #ifdef VMIME_VERSION
        #define D_ENV_IMAP_VMIME_VERSION_STRING VMIME_VERSION
    #else
        #define D_ENV_IMAP_VMIME_VERSION_STRING "unknown"
    #endif  // VMIME_VERSION
#endif  // D_ENV_IMAP_VMIME_VERSION_STRING

// 5.2.4
// D_ENV_IMAP_VMIME_HAS_IMAP
//   feature: 1 if the installed VMime was built with its IMAP messaging
// service (VMIME_HAVE_MESSAGING_PROTO_IMAP). 0 when the config header was not
// read, so a C++ build that suppresses the probe must pre-define this to use
// VMime.
#ifndef D_ENV_IMAP_VMIME_HAS_IMAP
    #ifdef VMIME_HAVE_MESSAGING_PROTO_IMAP
        #if VMIME_HAVE_MESSAGING_PROTO_IMAP
            #define D_ENV_IMAP_VMIME_HAS_IMAP 1
        #else
            #define D_ENV_IMAP_VMIME_HAS_IMAP 0
        #endif
    #else
        #define D_ENV_IMAP_VMIME_HAS_IMAP 0
    #endif  // VMIME_HAVE_MESSAGING_PROTO_IMAP
#endif  // D_ENV_IMAP_VMIME_HAS_IMAP

// 5.2.5
// D_ENV_IMAP_VMIME_AVAILABLE
//   feature: 1 when a VMime IMAP backend can be compiled here -- the compiler
// is C++, VMime is present and was built with IMAP, and the platform has a
// TCP transport. Always 0 in C.
#ifndef D_ENV_IMAP_VMIME_AVAILABLE
    #if ( (defined(__cplusplus))      &&                                       \
          (D_ENV_IMAP_HAS_VMIME)      &&                                       \
          (D_ENV_IMAP_VMIME_HAS_IMAP) &&                                       \
          (D_ENV_NET_CAN_TCP) )
        #define D_ENV_IMAP_VMIME_AVAILABLE 1
    #else
        #define D_ENV_IMAP_VMIME_AVAILABLE 0
    #endif
#endif  // D_ENV_IMAP_VMIME_AVAILABLE


//==============================================================================
// 6.  SASL LIBRARIES
//==============================================================================
// AUTHENTICATE carries SASL (RFC 4422). PLAIN, LOGIN, XOAUTH2, and OAUTHBEARER
// are simple enough to encode directly; SCRAM, GSSAPI, and the channel-binding
// variants are what these libraries are for. Cyrus SASL keeps its version
// macros in the full API header, so its version is read only when the
// translation unit has already included that header. GNU SASL publishes a
// standalone, macro-only version header, probed as libcurl's is.


// 6.1    Cyrus SASL
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_IMAP_HAS_CYRUS_SASL
//   feature: detect if Cyrus SASL's API header <sasl/sasl.h> is includable.
#ifndef D_ENV_IMAP_HAS_CYRUS_SASL
    #if D_ENV_NET_HAS_INCLUDE(<sasl/sasl.h>)
        #define D_ENV_IMAP_HAS_CYRUS_SASL 1
    #else
        #define D_ENV_IMAP_HAS_CYRUS_SASL 0
    #endif
#endif  // D_ENV_IMAP_HAS_CYRUS_SASL

// 6.1.2
// D_ENV_IMAP_CYRUS_SASL_VERSION_NUM
//   feature: the version packed as 0xMMNNSS (major, minor, step) when
// <sasl/sasl.h> was included before this header, otherwise 0. The value is
// fixed at the point of first inclusion.
#ifndef D_ENV_IMAP_CYRUS_SASL_VERSION_NUM
    #if ( (defined(SASL_VERSION_MAJOR)) &&                                     \
          (defined(SASL_VERSION_MINOR)) &&                                     \
          (defined(SASL_VERSION_STEP)) )
        #define D_ENV_IMAP_CYRUS_SASL_VERSION_NUM                              \
            ( (SASL_VERSION_MAJOR << 16) |                                     \
              (SASL_VERSION_MINOR << 8)  |                                     \
              (SASL_VERSION_STEP) )
    #else
        #define D_ENV_IMAP_CYRUS_SASL_VERSION_NUM 0
    #endif
#endif  // D_ENV_IMAP_CYRUS_SASL_VERSION_NUM

// 6.2    GNU SASL
//------------------------------------------------------------------------------
// 6.2.1
// D_ENV_IMAP_HAS_GSASL
//   feature: detect if GNU SASL's API header <gsasl.h> is includable.
#ifndef D_ENV_IMAP_HAS_GSASL
    #if D_ENV_NET_HAS_INCLUDE(<gsasl.h>)
        #define D_ENV_IMAP_HAS_GSASL 1
    #else
        #define D_ENV_IMAP_HAS_GSASL 0
    #endif
#endif  // D_ENV_IMAP_HAS_GSASL

// 6.2.2
// D_ENV_IMAP_HAS_GSASL_VERSION_HEADER
//   feature: detect if the standalone version header <gsasl-version.h> is
// includable.
#ifndef D_ENV_IMAP_HAS_GSASL_VERSION_HEADER
    #if D_ENV_NET_HAS_INCLUDE(<gsasl-version.h>)
        #define D_ENV_IMAP_HAS_GSASL_VERSION_HEADER 1
    #else
        #define D_ENV_IMAP_HAS_GSASL_VERSION_HEADER 0
    #endif
#endif  // D_ENV_IMAP_HAS_GSASL_VERSION_HEADER

// 6.2.3
// <gsasl-version.h>
//   include: included when D_CFG_ENV_IMAP_PROBE_VERSION is 1 and the header
// exists. It defines only the version macros, and brings in no declarations
// or linkage.
#if ( (D_CFG_ENV_IMAP_PROBE_VERSION) &&                                        \
      (D_ENV_IMAP_HAS_GSASL_VERSION_HEADER) )
    // gsasl
    #include <gsasl-version.h>  // GSASL_VERSION_NUMBER
#endif

// 6.2.4
// D_ENV_IMAP_GSASL_VERSION_NUM
//   feature: the packed 0xMMNNPP version number, or 0 when unknown.
#ifndef D_ENV_IMAP_GSASL_VERSION_NUM
    #ifdef GSASL_VERSION_NUMBER
        #define D_ENV_IMAP_GSASL_VERSION_NUM GSASL_VERSION_NUMBER
    #else
        #define D_ENV_IMAP_GSASL_VERSION_NUM 0
    #endif  // GSASL_VERSION_NUMBER
#endif  // D_ENV_IMAP_GSASL_VERSION_NUM

// 6.2.5
// D_ENV_IMAP_GSASL_AT_LEAST
//   macro: evaluates to 1 if the detected GNU SASL version is at least
// major.minor.patch. False for any nonzero threshold when the version is
// unknown (0).
#ifndef D_ENV_IMAP_GSASL_AT_LEAST
    #define D_ENV_IMAP_GSASL_AT_LEAST(major, minor, patch)                     \
        ( D_ENV_IMAP_GSASL_VERSION_NUM >=                                      \
          ( ((major) << 16) | ((minor) << 8) | (patch) ) )
#endif  // D_ENV_IMAP_GSASL_AT_LEAST

// 6.3    SASL selection
//------------------------------------------------------------------------------
// 6.3.1
// D_ENV_IMAP_SASL_BACKEND_*
//   constant: identifiers for D_ENV_IMAP_SASL_BACKEND, for switch-style
// dispatch.

// 6.3.1.1
// D_ENV_IMAP_SASL_BACKEND_NONE
//   constant: identifies no SASL library.
#define D_ENV_IMAP_SASL_BACKEND_NONE  0

// 6.3.1.2
// D_ENV_IMAP_SASL_BACKEND_CYRUS
//   constant: identifies Cyrus SASL.
#define D_ENV_IMAP_SASL_BACKEND_CYRUS 1

// 6.3.1.3
// D_ENV_IMAP_SASL_BACKEND_GSASL
//   constant: identifies GNU SASL.
#define D_ENV_IMAP_SASL_BACKEND_GSASL 2

// 6.3.2
// D_ENV_IMAP_SASL_BACKEND
//   feature: the preferred SASL library, as a D_ENV_IMAP_SASL_BACKEND_*
// identifier. Cyrus SASL, the more widely deployed of the two, comes first.
#ifndef D_ENV_IMAP_SASL_BACKEND
    #if D_ENV_IMAP_HAS_CYRUS_SASL
        #define D_ENV_IMAP_SASL_BACKEND D_ENV_IMAP_SASL_BACKEND_CYRUS
    #elif D_ENV_IMAP_HAS_GSASL
        #define D_ENV_IMAP_SASL_BACKEND D_ENV_IMAP_SASL_BACKEND_GSASL
    #else
        #define D_ENV_IMAP_SASL_BACKEND D_ENV_IMAP_SASL_BACKEND_NONE
    #endif
#endif  // D_ENV_IMAP_SASL_BACKEND

// 6.3.3
// D_ENV_IMAP_SASL_BACKEND_NAME
//   value: a human-readable name for the preferred SASL library.
#ifndef D_ENV_IMAP_SASL_BACKEND_NAME
    #if (D_ENV_IMAP_SASL_BACKEND == D_ENV_IMAP_SASL_BACKEND_CYRUS)
        #define D_ENV_IMAP_SASL_BACKEND_NAME "Cyrus SASL"
    #elif (D_ENV_IMAP_SASL_BACKEND == D_ENV_IMAP_SASL_BACKEND_GSASL)
        #define D_ENV_IMAP_SASL_BACKEND_NAME "GNU SASL"
    #else
        #define D_ENV_IMAP_SASL_BACKEND_NAME "none"
    #endif
#endif  // D_ENV_IMAP_SASL_BACKEND_NAME

// 6.3.4
// D_ENV_IMAP_HAS_SASL
//   feature: 1 if either SASL library is present.
#ifndef D_ENV_IMAP_HAS_SASL
    #if (D_ENV_IMAP_SASL_BACKEND != D_ENV_IMAP_SASL_BACKEND_NONE)
        #define D_ENV_IMAP_HAS_SASL 1
    #else
        #define D_ENV_IMAP_HAS_SASL 0
    #endif
#endif  // D_ENV_IMAP_HAS_SASL


//==============================================================================
// 7.  BACKEND SELECTION
//==============================================================================
// Every IMAP implementation the framework can sit on, native engine included,
// has an identifier, so a module or facade can name its backend in one
// vocabulary. The automatic selection chooses among installed libraries only.


// 7.1    Backend identifiers
//------------------------------------------------------------------------------
// 7.1.1
// D_ENV_IMAP_BACKEND_*
//   constant: identifiers for D_ENV_IMAP_BACKEND, for switch-style dispatch.
// The values are pinned: net/imap/imap.h mirrors them as enum d_imap_backend.

// 7.1.1.1
// D_ENV_IMAP_BACKEND_NONE
//   constant: identifies no backend.
#define D_ENV_IMAP_BACKEND_NONE      0

// 7.1.1.2
// D_ENV_IMAP_BACKEND_NATIVE
//   constant: identifies the framework's own engine over the net transport.
#define D_ENV_IMAP_BACKEND_NATIVE    1

// 7.1.1.3
// D_ENV_IMAP_BACKEND_CURL
//   constant: identifies libcurl.
#define D_ENV_IMAP_BACKEND_CURL      2

// 7.1.1.4
// D_ENV_IMAP_BACKEND_LIBETPAN
//   constant: identifies libetpan.
#define D_ENV_IMAP_BACKEND_LIBETPAN  3

// 7.1.1.5
// D_ENV_IMAP_BACKEND_MAILUTILS
//   constant: identifies GNU Mailutils.
#define D_ENV_IMAP_BACKEND_MAILUTILS 4

// 7.1.1.6
// D_ENV_IMAP_BACKEND_VMIME
//   constant: identifies VMime (C++ only).
#define D_ENV_IMAP_BACKEND_VMIME     5

// 7.2    Preferred library backend
//------------------------------------------------------------------------------
// 7.2.1
// D_ENV_IMAP_BACKEND
//   feature: the preferred available library backend, as a
// D_ENV_IMAP_BACKEND_* identifier. libetpan comes first, as the library whose
// API covers the most of the protocol; libcurl next, being ubiquitous but
// limited to what its URLs and custom requests express; then Mailutils; then
// VMime, which a C compiler never selects. The native engine is never chosen
// here, since detection cannot see whether it exists; pre-define this as
// D_ENV_IMAP_BACKEND_NATIVE to make it the default.
#ifndef D_ENV_IMAP_BACKEND
    #if D_ENV_IMAP_LIBETPAN_AVAILABLE
        #define D_ENV_IMAP_BACKEND D_ENV_IMAP_BACKEND_LIBETPAN
    #elif D_ENV_IMAP_CURL_AVAILABLE
        #define D_ENV_IMAP_BACKEND D_ENV_IMAP_BACKEND_CURL
    #elif D_ENV_IMAP_MAILUTILS_AVAILABLE
        #define D_ENV_IMAP_BACKEND D_ENV_IMAP_BACKEND_MAILUTILS
    #elif D_ENV_IMAP_VMIME_AVAILABLE
        #define D_ENV_IMAP_BACKEND D_ENV_IMAP_BACKEND_VMIME
    #else
        #define D_ENV_IMAP_BACKEND D_ENV_IMAP_BACKEND_NONE
    #endif
#endif  // D_ENV_IMAP_BACKEND

// 7.2.2
// D_ENV_IMAP_BACKEND_NAME
//   value: a human-readable name for the preferred backend.
#ifndef D_ENV_IMAP_BACKEND_NAME
    #if (D_ENV_IMAP_BACKEND == D_ENV_IMAP_BACKEND_NATIVE)
        #define D_ENV_IMAP_BACKEND_NAME "native"
    #elif (D_ENV_IMAP_BACKEND == D_ENV_IMAP_BACKEND_CURL)
        #define D_ENV_IMAP_BACKEND_NAME "libcurl"
    #elif (D_ENV_IMAP_BACKEND == D_ENV_IMAP_BACKEND_LIBETPAN)
        #define D_ENV_IMAP_BACKEND_NAME "libetpan"
    #elif (D_ENV_IMAP_BACKEND == D_ENV_IMAP_BACKEND_MAILUTILS)
        #define D_ENV_IMAP_BACKEND_NAME "GNU Mailutils"
    #elif (D_ENV_IMAP_BACKEND == D_ENV_IMAP_BACKEND_VMIME)
        #define D_ENV_IMAP_BACKEND_NAME "VMime"
    #else
        #define D_ENV_IMAP_BACKEND_NAME "none"
    #endif
#endif  // D_ENV_IMAP_BACKEND_NAME


//==============================================================================
// 8.  AVAILABILITY SUMMARY
//==============================================================================
// The common module, net/imap/imap.h, is pure protocol logic and compiles
// everywhere; it does not key on these. They are for the backend modules and
// for a facade deciding whether any backend can exist at all.


// 8.1    Rolled-up availability
//------------------------------------------------------------------------------
// 8.1.1
// D_ENV_IMAP_HAS_LIBRARY
//   feature: 1 if at least one library backend can be compiled here. Read
// from the individual gates rather than from D_ENV_IMAP_BACKEND, so a
// pre-defined preference cannot change the answer.
#ifndef D_ENV_IMAP_HAS_LIBRARY
    #if ( (D_ENV_IMAP_LIBETPAN_AVAILABLE)  ||                                  \
          (D_ENV_IMAP_CURL_AVAILABLE)      ||                                  \
          (D_ENV_IMAP_MAILUTILS_AVAILABLE) ||                                  \
          (D_ENV_IMAP_VMIME_AVAILABLE) )
        #define D_ENV_IMAP_HAS_LIBRARY 1
    #else
        #define D_ENV_IMAP_HAS_LIBRARY 0
    #endif
#endif  // D_ENV_IMAP_HAS_LIBRARY

// 8.1.2
// D_ENV_IMAP_AVAILABLE
//   feature: 1 if some IMAP backend could run here -- a library backend, or
// the native engine's transport.
#ifndef D_ENV_IMAP_AVAILABLE
    #if ( (D_ENV_IMAP_HAS_LIBRARY) ||                                          \
          (D_ENV_IMAP_CAN_NATIVE) )
        #define D_ENV_IMAP_AVAILABLE 1
    #else
        #define D_ENV_IMAP_AVAILABLE 0
    #endif
#endif  // D_ENV_IMAP_AVAILABLE


#endif  // DJINTERP_ENV_NET_IMAP_ENV_IMAP_H
