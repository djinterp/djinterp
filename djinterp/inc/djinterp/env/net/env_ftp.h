/*******************************************************************************
* djinterp [env]                                                       env_ftp.h
*
* djinterp FTP environment detection.
*   The single home for the compile-time detection an FTP backend needs,
* expressed through the D_ENV_FTP_* interface. It covers:
*     - the transport a native protocol engine runs on: TCP with name
*       resolution for both channels, and IPv6 for EPSV / EPRT             [1]
*     - FTPS (FTP over TLS, RFC 4217) through the TLS layer                [1]
*     - FTP-capable libraries: libcurl, WinINet, ftplib, and Apple's
*       CFFTPStream, with a preferred-backend selection                    [2]
*     - version-gated libcurl FTP options and default changes              [3]
*     - rolled-up client, server, and FTPS summaries                       [4]
*   Detection versus runtime features: every flag here is a compile-time
* claim about headers and platforms. Whether an installed libcurl was built
* with FTP, or with a TLS backend for FTPS, is a property of the binary that
* only curl_version_info() reports at runtime; that check belongs to the curl
* FTP backend, and this header does not guess at it.
*   SFTP is not FTP. It is a subsystem of SSH and shares no wire syntax with
* RFC 959, so SSH libraries are deliberately not detected here.
*   Naming: D_ENV_FTP_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_FTP_CAN_<ROLE> is a rolled-up capability; D_ENV_FTP_<FEATURE> is a
* non-boolean detected value or identifier; D_ENV_FTP_CURL_* describes the
* libcurl FTP surface.
*   Every flag is #ifndef-guarded, so a project may pre-define any D_ENV_FTP_*
* macro before inclusion to override detection, for example to force a backend
* or to describe a cross-compilation target.
*   It requires env_net.h, env_tls.h, and env_curl.h, and includes them itself.
* It is an opt-in module and may be included directly.
*
*
* path:      /inc/djinterp/env/net/env_ftp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSPORT PREREQUISITES
    -----------------------
    1.  Channels
         1.  D_ENV_FTP_HAS_TRANSPORT
         2.  D_ENV_FTP_HAS_IPV6
    2.  Security layer
         1.  D_ENV_FTP_HAS_TLS
2.  BACKEND LIBRARIES
    -----------------
    1.  Library detection
         1.  D_ENV_FTP_HAS_CURL
         2.  D_ENV_FTP_HAS_WININET
         3.  D_ENV_FTP_HAS_FTPLIB
         4.  D_ENV_FTP_HAS_CFNETWORK
    2.  Backend identifiers
         1.  D_ENV_FTP_BACKEND_*
              1.  D_ENV_FTP_BACKEND_NONE
              2.  D_ENV_FTP_BACKEND_NATIVE
              3.  D_ENV_FTP_BACKEND_CURL
              4.  D_ENV_FTP_BACKEND_WININET
              5.  D_ENV_FTP_BACKEND_FTPLIB
              6.  D_ENV_FTP_BACKEND_CFNETWORK
    3.  Backend selection
         1.  D_ENV_FTP_BACKEND
         2.  D_ENV_FTP_BACKEND_NAME
3.  LIBCURL FTP SURFACE
    -------------------
    1.  Version-gated options
         1.  D_ENV_FTP_CURL_HAS_USE_SSL
         2.  D_ENV_FTP_CURL_HAS_PRET
         3.  D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT
         4.  D_ENV_FTP_CURL_HAS_WILDCARD
         5.  D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT_MS
    2.  Default changes
         1.  D_ENV_FTP_CURL_SKIPS_PASV_IP
4.  CAPABILITY SUMMARIES
    --------------------
    1.  Rolled-up capabilities
         1.  D_ENV_FTP_CAN_NATIVE
         2.  D_ENV_FTP_CAN_SERVE
         3.  D_ENV_FTP_CAN_FTPS
         4.  D_ENV_FTP_CAN_CLIENT
         5.  D_ENV_FTP_AVAILABLE
*/

#ifndef DJINTERP_ENV_NET_ENV_FTP_H
#define DJINTERP_ENV_NET_ENV_FTP_H 1

// djinterp
#include "../env.h"      // D_ENV_OS_ID, D_ENV_IS_OS_WINDOWS
#include "./env_curl.h"  // D_ENV_CURL_AVAILABLE, D_ENV_CURL_VERSION_AT_LEAST
#include "./env_net.h"   // D_ENV_NET_HAS_INCLUDE, _CAN_TCP, _HAS_IPV6
#include "./env_tls.h"   // D_ENV_TLS_AVAILABLE


//==============================================================================
// 1.  TRANSPORT PREREQUISITES
//==============================================================================
// What a native protocol engine -- one built on net/ftp.h and the framework's
// own socket layer -- needs from the platform. FTP runs a control connection
// and a separate data connection per transfer, both over TCP, so the socket
// layer's TCP capability is the whole prerequisite; IPv6 decides which
// data-connection commands are usable, and a TLS library decides whether the
// engine can speak FTPS.


// 1.1    Channels
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_FTP_HAS_TRANSPORT
//   feature: 1 if TCP stream sockets with name resolution are usable
// (D_ENV_NET_CAN_TCP), which serves both the control and the data connection.
#ifndef D_ENV_FTP_HAS_TRANSPORT
    #if D_ENV_NET_CAN_TCP
        #define D_ENV_FTP_HAS_TRANSPORT 1
    #else
        #define D_ENV_FTP_HAS_TRANSPORT 0
    #endif
#endif  // D_ENV_FTP_HAS_TRANSPORT

// 1.1.2
// D_ENV_FTP_HAS_IPV6
//   feature: 1 if the transport can reach IPv6 peers. PASV and PORT encode
// IPv4 addresses only, so an IPv6 control connection must use EPSV and EPRT
// (RFC 2428); EPSV is also the better choice behind NAT on IPv4.
#ifndef D_ENV_FTP_HAS_IPV6
    #if ( (D_ENV_FTP_HAS_TRANSPORT) &&                                         \
          (D_ENV_NET_HAS_IPV6) )
        #define D_ENV_FTP_HAS_IPV6 1
    #else
        #define D_ENV_FTP_HAS_IPV6 0
    #endif
#endif  // D_ENV_FTP_HAS_IPV6


// 1.2    Security layer
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_FTP_HAS_TLS
//   feature: 1 if a TLS library and a TCP transport are both present
// (D_ENV_TLS_AVAILABLE), so a native engine can run FTPS: explicit, with
// AUTH TLS on the control port, or implicit, with TLS from the first byte on
// port 990.
#ifndef D_ENV_FTP_HAS_TLS
    #if D_ENV_TLS_AVAILABLE
        #define D_ENV_FTP_HAS_TLS 1
    #else
        #define D_ENV_FTP_HAS_TLS 0
    #endif
#endif  // D_ENV_FTP_HAS_TLS


//==============================================================================
// 2.  BACKEND LIBRARIES
//==============================================================================
// Libraries that already speak FTP, detected by header presence. Each flag
// stays queryable on its own, and D_ENV_FTP_BACKEND picks one. Only libcurl
// covers the protocol broadly (FTPS, EPSV / EPRT, resume, wildcards); the
// others are narrower and are recorded so a backend can fall back to them.


// 2.1    Library detection
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_FTP_HAS_CURL
//   feature: 1 if libcurl is usable (D_ENV_CURL_AVAILABLE). Its FTP options
// are declared in every build of the header, so presence of the library is
// presence of the API; whether the binary was built with FTP is the runtime
// question curl_version_info() answers.
#ifndef D_ENV_FTP_HAS_CURL
    #if D_ENV_CURL_AVAILABLE
        #define D_ENV_FTP_HAS_CURL 1
    #else
        #define D_ENV_FTP_HAS_CURL 0
    #endif
#endif  // D_ENV_FTP_HAS_CURL

// 2.1.2
// D_ENV_FTP_HAS_WININET
//   feature: 1 if the WinINet FTP API is available: a Windows target whose
// SDK carries <wininet.h> (InternetConnect with INTERNET_SERVICE_FTP,
// FtpGetFile, FtpFindFirstFile, ...). It is a client API only, its FTP
// functions have no TLS, and Microsoft rules out its use from a service.
// <wininet.h> must follow <windows.h>; link against wininet.lib.
#ifndef D_ENV_FTP_HAS_WININET
    #if !(D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID))
        #define D_ENV_FTP_HAS_WININET 0
    #elif D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<wininet.h>)
            #define D_ENV_FTP_HAS_WININET 1
        #else
            #define D_ENV_FTP_HAS_WININET 0
        #endif
    #else
        // every Windows SDK ships the header; assume it without a probe
        #define D_ENV_FTP_HAS_WININET 1
    #endif
#endif  // D_ENV_FTP_HAS_WININET

// 2.1.3
// D_ENV_FTP_HAS_FTPLIB
//   feature: 1 if Thomas Pfau's ftplib (<ftplib.h>: FtpConnect, FtpLogin,
// FtpGet, FtpPut, ...) is includable and a TCP transport exists. A small,
// blocking, plain-FTP client library with no TLS.
#ifndef D_ENV_FTP_HAS_FTPLIB
    #if !D_ENV_FTP_HAS_TRANSPORT
        #define D_ENV_FTP_HAS_FTPLIB 0
    #elif D_ENV_NET_HAS_INCLUDE(<ftplib.h>)
        #define D_ENV_FTP_HAS_FTPLIB 1
    #else
        #define D_ENV_FTP_HAS_FTPLIB 0
    #endif
#endif  // D_ENV_FTP_HAS_FTPLIB

// 2.1.4
// D_ENV_FTP_HAS_CFNETWORK
//   feature: 1 if Apple's CFFTPStream API (<CFNetwork/CFFTPStream.h>) is
// includable. Deprecated since the macOS 10.11 and iOS 9 SDKs, absent from
// watchOS, and without FTPS; recorded only as a last resort for Apple targets.
#ifndef D_ENV_FTP_HAS_CFNETWORK
    #if D_ENV_NET_HAS_INCLUDE(<CFNetwork/CFFTPStream.h>)
        #define D_ENV_FTP_HAS_CFNETWORK 1
    #else
        #define D_ENV_FTP_HAS_CFNETWORK 0
    #endif
#endif  // D_ENV_FTP_HAS_CFNETWORK


// 2.2    Backend identifiers
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_FTP_BACKEND_*
//   constant: identifiers for D_ENV_FTP_BACKEND. Compare against these names,
// never against their values.

// 2.2.1.1
// D_ENV_FTP_BACKEND_NONE
//   constant: no FTP client backend is available.
#define D_ENV_FTP_BACKEND_NONE      0

// 2.2.1.2
// D_ENV_FTP_BACKEND_NATIVE
//   constant: the framework's own engine, net/ftp.h over the socket layer.
#define D_ENV_FTP_BACKEND_NATIVE    1

// 2.2.1.3
// D_ENV_FTP_BACKEND_CURL
//   constant: libcurl.
#define D_ENV_FTP_BACKEND_CURL      2

// 2.2.1.4
// D_ENV_FTP_BACKEND_WININET
//   constant: the Windows WinINet FTP API.
#define D_ENV_FTP_BACKEND_WININET   3

// 2.2.1.5
// D_ENV_FTP_BACKEND_FTPLIB
//   constant: ftplib.
#define D_ENV_FTP_BACKEND_FTPLIB    4

// 2.2.1.6
// D_ENV_FTP_BACKEND_CFNETWORK
//   constant: Apple's CFFTPStream.
#define D_ENV_FTP_BACKEND_CFNETWORK 5


// 2.3    Backend selection
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_FTP_BACKEND
//   feature: the preferred client backend, as a D_ENV_FTP_BACKEND_*
// identifier. libcurl leads as the most complete client; the native engine
// follows, needing nothing beyond the socket layer; the platform and
// third-party libraries are fallbacks for targets without that socket layer.
// Servers have only the native engine; see D_ENV_FTP_CAN_SERVE.
#ifndef D_ENV_FTP_BACKEND
    #if D_ENV_FTP_HAS_CURL
        #define D_ENV_FTP_BACKEND D_ENV_FTP_BACKEND_CURL
    #elif D_ENV_FTP_HAS_TRANSPORT
        #define D_ENV_FTP_BACKEND D_ENV_FTP_BACKEND_NATIVE
    #elif D_ENV_FTP_HAS_WININET
        #define D_ENV_FTP_BACKEND D_ENV_FTP_BACKEND_WININET
    #elif D_ENV_FTP_HAS_FTPLIB
        #define D_ENV_FTP_BACKEND D_ENV_FTP_BACKEND_FTPLIB
    #elif D_ENV_FTP_HAS_CFNETWORK
        #define D_ENV_FTP_BACKEND D_ENV_FTP_BACKEND_CFNETWORK
    #else
        #define D_ENV_FTP_BACKEND D_ENV_FTP_BACKEND_NONE
    #endif
#endif  // D_ENV_FTP_BACKEND

// 2.3.2
// D_ENV_FTP_BACKEND_NAME
//   feature: the selected backend's name, as a string literal.
#ifndef D_ENV_FTP_BACKEND_NAME
    #if (D_ENV_FTP_BACKEND == D_ENV_FTP_BACKEND_CURL)
        #define D_ENV_FTP_BACKEND_NAME "libcurl"
    #elif (D_ENV_FTP_BACKEND == D_ENV_FTP_BACKEND_NATIVE)
        #define D_ENV_FTP_BACKEND_NAME "native"
    #elif (D_ENV_FTP_BACKEND == D_ENV_FTP_BACKEND_WININET)
        #define D_ENV_FTP_BACKEND_NAME "WinINet"
    #elif (D_ENV_FTP_BACKEND == D_ENV_FTP_BACKEND_FTPLIB)
        #define D_ENV_FTP_BACKEND_NAME "ftplib"
    #elif (D_ENV_FTP_BACKEND == D_ENV_FTP_BACKEND_CFNETWORK)
        #define D_ENV_FTP_BACKEND_NAME "CFNetwork"
    #else
        #define D_ENV_FTP_BACKEND_NAME "none"
    #endif
#endif  // D_ENV_FTP_BACKEND_NAME


//==============================================================================
// 3.  LIBCURL FTP SURFACE
//==============================================================================
// FTP options whose availability or default is tied to a libcurl release,
// each keyed to the version that introduced it. Like the API flags in
// env_curl.h they are reliable at compile time and say nothing about runtime
// success, and all of them read 0 when libcurl is absent. Options older than
// the 7.17 floor the curl modules already require are not listed.


// 3.1    Version-gated options
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_FTP_CURL_HAS_USE_SSL
//   feature: the CURLOPT_USE_SSL / CURLUSESSL_* spelling (libcurl 7.17.0)
// that selects explicit FTPS. The option dates from 7.11.0 as CURLOPT_FTP_SSL.
#ifndef D_ENV_FTP_CURL_HAS_USE_SSL
    #if !D_ENV_FTP_HAS_CURL
        #define D_ENV_FTP_CURL_HAS_USE_SSL 0
    #elif D_ENV_CURL_VERSION_AT_LEAST(7, 17, 0)
        #define D_ENV_FTP_CURL_HAS_USE_SSL 1
    #else
        #define D_ENV_FTP_CURL_HAS_USE_SSL 0
    #endif
#endif  // D_ENV_FTP_CURL_HAS_USE_SSL

// 3.1.2
// D_ENV_FTP_CURL_HAS_PRET
//   feature: CURLOPT_FTP_USE_PRET (libcurl 7.20.0), which sends the
// non-standard PRET ahead of PASV or EPSV for the servers that demand it.
#ifndef D_ENV_FTP_CURL_HAS_PRET
    #if !D_ENV_FTP_HAS_CURL
        #define D_ENV_FTP_CURL_HAS_PRET 0
    #elif D_ENV_CURL_VERSION_AT_LEAST(7, 20, 0)
        #define D_ENV_FTP_CURL_HAS_PRET 1
    #else
        #define D_ENV_FTP_CURL_HAS_PRET 0
    #endif
#endif  // D_ENV_FTP_CURL_HAS_PRET

// 3.1.3
// D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT
//   feature: CURLOPT_SERVER_RESPONSE_TIMEOUT, in seconds (libcurl 7.20.0).
// It replaced CURLOPT_FTP_RESPONSE_TIMEOUT, deprecated in 7.85.0.
#ifndef D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT
    #if !D_ENV_FTP_HAS_CURL
        #define D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT 0
    #elif D_ENV_CURL_VERSION_AT_LEAST(7, 20, 0)
        #define D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT 1
    #else
        #define D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT 0
    #endif
#endif  // D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT

// 3.1.4
// D_ENV_FTP_CURL_HAS_WILDCARD
//   feature: CURLOPT_WILDCARDMATCH (libcurl 7.21.0), which downloads every
// file matching a pattern in the URL's last path segment.
#ifndef D_ENV_FTP_CURL_HAS_WILDCARD
    #if !D_ENV_FTP_HAS_CURL
        #define D_ENV_FTP_CURL_HAS_WILDCARD 0
    #elif D_ENV_CURL_VERSION_AT_LEAST(7, 21, 0)
        #define D_ENV_FTP_CURL_HAS_WILDCARD 1
    #else
        #define D_ENV_FTP_CURL_HAS_WILDCARD 0
    #endif
#endif  // D_ENV_FTP_CURL_HAS_WILDCARD

// 3.1.5
// D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT_MS
//   feature: CURLOPT_SERVER_RESPONSE_TIMEOUT_MS, in milliseconds
// (libcurl 8.6.0).
#ifndef D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT_MS
    #if !D_ENV_FTP_HAS_CURL
        #define D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT_MS 0
    #elif D_ENV_CURL_VERSION_AT_LEAST(8, 6, 0)
        #define D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT_MS 1
    #else
        #define D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT_MS 0
    #endif
#endif  // D_ENV_FTP_CURL_HAS_RESPONSE_TIMEOUT_MS


// 3.2    Default changes
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_FTP_CURL_SKIPS_PASV_IP
//   feature: 1 if libcurl ignores the address in a 227 reply by default and
// connects to the control peer instead (CURLOPT_FTP_SKIP_PASV_IP on, from
// 7.74.0; the option exists since 7.15.0). A backend that wants this on every
// version sets the option explicitly rather than reading this flag.
#ifndef D_ENV_FTP_CURL_SKIPS_PASV_IP
    #if !D_ENV_FTP_HAS_CURL
        #define D_ENV_FTP_CURL_SKIPS_PASV_IP 0
    #elif D_ENV_CURL_VERSION_AT_LEAST(7, 74, 0)
        #define D_ENV_FTP_CURL_SKIPS_PASV_IP 1
    #else
        #define D_ENV_FTP_CURL_SKIPS_PASV_IP 0
    #endif
#endif  // D_ENV_FTP_CURL_SKIPS_PASV_IP


//==============================================================================
// 4.  CAPABILITY SUMMARIES
//==============================================================================
// The rolled-up questions an FTP module actually asks. A backend header
// should #error on the role it needs rather than on D_ENV_FTP_AVAILABLE alone.


// 4.1    Rolled-up capabilities
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_FTP_CAN_NATIVE
//   feature: 1 if the native client engine -- net/ftp.h over the socket
// layer -- can be built here.
#ifndef D_ENV_FTP_CAN_NATIVE
    #if D_ENV_FTP_HAS_TRANSPORT
        #define D_ENV_FTP_CAN_NATIVE 1
    #else
        #define D_ENV_FTP_CAN_NATIVE 0
    #endif
#endif  // D_ENV_FTP_CAN_NATIVE

// 4.1.2
// D_ENV_FTP_CAN_SERVE
//   feature: 1 if an FTP server can be built here. None of the detected
// libraries serves, so this is the native engine's transport requirement.
#ifndef D_ENV_FTP_CAN_SERVE
    #if D_ENV_FTP_HAS_TRANSPORT
        #define D_ENV_FTP_CAN_SERVE 1
    #else
        #define D_ENV_FTP_CAN_SERVE 0
    #endif
#endif  // D_ENV_FTP_CAN_SERVE

// 4.1.3
// D_ENV_FTP_CAN_FTPS
//   feature: 1 if some backend can speak FTPS: the native engine with a TLS
// library, or libcurl, whose TLS build is then confirmed at runtime.
#ifndef D_ENV_FTP_CAN_FTPS
    #if ( (D_ENV_FTP_HAS_TLS) ||                                               \
          (D_ENV_FTP_HAS_CURL) )
        #define D_ENV_FTP_CAN_FTPS 1
    #else
        #define D_ENV_FTP_CAN_FTPS 0
    #endif
#endif  // D_ENV_FTP_CAN_FTPS

// 4.1.4
// D_ENV_FTP_CAN_CLIENT
//   feature: 1 if any client backend is available, which is to say that
// D_ENV_FTP_BACKEND is not D_ENV_FTP_BACKEND_NONE.
#ifndef D_ENV_FTP_CAN_CLIENT
    #if (D_ENV_FTP_BACKEND != D_ENV_FTP_BACKEND_NONE)
        #define D_ENV_FTP_CAN_CLIENT 1
    #else
        #define D_ENV_FTP_CAN_CLIENT 0
    #endif
#endif  // D_ENV_FTP_CAN_CLIENT

// 4.1.5
// D_ENV_FTP_AVAILABLE
//   feature: 1 if FTP is usable here in any role, as client or as server.
#ifndef D_ENV_FTP_AVAILABLE
    #if ( (D_ENV_FTP_CAN_CLIENT) ||                                            \
          (D_ENV_FTP_CAN_SERVE) )
        #define D_ENV_FTP_AVAILABLE 1
    #else
        #define D_ENV_FTP_AVAILABLE 0
    #endif
#endif  // D_ENV_FTP_AVAILABLE


#endif  // DJINTERP_ENV_NET_ENV_FTP_H
