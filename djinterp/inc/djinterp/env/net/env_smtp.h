/*******************************************************************************
* djinterp [env]                                                      env_smtp.h
*
* djinterp SMTP environment detection.
*   Compile-time detection of everything the SMTP modules take from the
* platform or from a third-party library, expressed through the D_ENV_SMTP_*
* interface. The C kernel (c/net/smtp) and its C++ facade (net/smtp.hpp) read
* these flags rather than raw platform macros, so porting the SMTP stack means
* revisiting this file and nothing else. It covers:
*     - the socket transport: BSD sockets, name resolution, poll(), and the
*       two mechanisms that stop a dead peer from raising SIGPIPE        [1]
*     - the local host name offered as the default EHLO identity        [1]
*     - TLS through the OpenSSL family, including the custom-BIO and
*       certificate-name APIs the transport is built on, and the HMAC-MD5
*       that CRAM-MD5 authentication computes                           [2]
*     - rolled-up capabilities and the libraries a build must link      [3]
*   Detection is derived from env_net.h and env_tls.h rather than repeated,
* so the three headers can never disagree about the platform.
*   Naming: D_ENV_SMTP_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_SMTP_CAN_<ACTION> is a capability rolled up from several features.
*   Every flag is #ifndef-guarded, so a project may pre-define any
* D_ENV_SMTP_* macro before inclusion -- to describe a cross-compilation
* target, or to build without OpenSSL on a machine where its headers happen to
* be installed (pre-define D_ENV_SMTP_HAS_TLS as 0).
*
*
* path:      /inc/djinterp/env/net/env_smtp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSPORT
    ---------
    1.  Sockets
         1.  D_ENV_SMTP_HAS_SOCKETS
         2.  D_ENV_SMTP_HAS_IPV6
    2.  Signal suppression
         1.  D_ENV_SMTP_HAS_MSG_NOSIGNAL
         2.  D_ENV_SMTP_HAS_SO_NOSIGPIPE
    3.  Host identity
         1.  D_ENV_SMTP_HAS_GETHOSTNAME
2.  SECURITY
    --------
    1.  Transport security
         1.  D_ENV_SMTP_HAS_TLS
    2.  Authentication
         1.  D_ENV_SMTP_HAS_CRAM_MD5
3.  CAPABILITIES
    ------------
    1.  Rolled-up capabilities
         1.  D_ENV_SMTP_CAN_CONNECT
         2.  D_ENV_SMTP_CAN_SERVE
         3.  D_ENV_SMTP_CAN_TLS
    2.  Link requirements
         1.  D_ENV_SMTP_LINKS_OPENSSL
    3.  Consistency checks
*/

#ifndef DJINTERP_ENV_NET_ENV_SMTP_H
#define DJINTERP_ENV_NET_ENV_SMTP_H 1

// djinterp
#include "./env_net.h"  // D_ENV_NET_HAS_*, D_ENV_NET_CAN_TCP
#include "./env_tls.h"  // D_ENV_TLS_HAS_OPENSSL, D_ENV_TLS_HAS_HOSTNAME_*


//==============================================================================
// 1.  TRANSPORT
//==============================================================================
// What the kernel's built-in socket transport needs from the platform. The
// transport is optional -- the protocol engine runs over any caller-supplied
// transport -- so a platform without it still builds everything else.


// 1.1    Sockets
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_SMTP_HAS_SOCKETS
//   feature: the BSD-socket transport can be built: sockets, getaddrinfo(),
// poll(), fcntl(), inet_ntop() and close() are all present.
#ifndef D_ENV_SMTP_HAS_SOCKETS
    #if ( (D_ENV_NET_HAS_BSD_SOCKETS) &&                                       \
          (D_ENV_NET_HAS_GETADDRINFO) &&                                       \
          (D_ENV_NET_HAS_POLL_H)      &&                                       \
          (D_ENV_NET_HAS_FCNTL_H)     &&                                       \
          (D_ENV_NET_HAS_ARPA_INET_H) &&                                       \
          (D_ENV_NET_HAS_UNISTD_H) )
        #define D_ENV_SMTP_HAS_SOCKETS 1
    #else
        #define D_ENV_SMTP_HAS_SOCKETS 0
    #endif
#endif  // D_ENV_SMTP_HAS_SOCKETS

// 1.1.2
// D_ENV_SMTP_HAS_IPV6
//   feature: the transport can connect over IPv6 and write an IPv6 address
// literal ("[IPv6:...]") as its EHLO identity.
#ifndef D_ENV_SMTP_HAS_IPV6
    #if ( (D_ENV_SMTP_HAS_SOCKETS) &&                                          \
          (D_ENV_NET_HAS_IPV6) )
        #define D_ENV_SMTP_HAS_IPV6 1
    #else
        #define D_ENV_SMTP_HAS_IPV6 0
    #endif
#endif  // D_ENV_SMTP_HAS_IPV6

// 1.2    Signal suppression
//------------------------------------------------------------------------------
// A write to a peer that has already closed raises SIGPIPE, which terminates
// the process by default. Both mechanisms below are probed from the macros
// <sys/socket.h> defines, so no operating system is named here; a platform
// with neither leaves SIGPIPE to the application.
#if ( (D_ENV_SMTP_HAS_SOCKETS) &&                                              \
      (D_ENV_NET_HAS_SYS_SOCKET_H) )
    // posix
    #include <sys/socket.h>  // MSG_NOSIGNAL, SO_NOSIGPIPE
#endif

// 1.2.1
// D_ENV_SMTP_HAS_MSG_NOSIGNAL
//   feature: send() accepts MSG_NOSIGNAL, suppressing SIGPIPE per call (Linux,
// the BSDs, Solaris, POSIX.1-2008).
#ifndef D_ENV_SMTP_HAS_MSG_NOSIGNAL
    #if ( (D_ENV_SMTP_HAS_SOCKETS) &&                                          \
          (defined(MSG_NOSIGNAL)) )
        #define D_ENV_SMTP_HAS_MSG_NOSIGNAL 1
    #else
        #define D_ENV_SMTP_HAS_MSG_NOSIGNAL 0
    #endif
#endif  // D_ENV_SMTP_HAS_MSG_NOSIGNAL

// 1.2.2
// D_ENV_SMTP_HAS_SO_NOSIGPIPE
//   feature: the SO_NOSIGPIPE socket option suppresses SIGPIPE per socket
// (Apple platforms, which lack MSG_NOSIGNAL).
#ifndef D_ENV_SMTP_HAS_SO_NOSIGPIPE
    #if ( (D_ENV_SMTP_HAS_SOCKETS) &&                                          \
          (defined(SO_NOSIGPIPE)) )
        #define D_ENV_SMTP_HAS_SO_NOSIGPIPE 1
    #else
        #define D_ENV_SMTP_HAS_SO_NOSIGPIPE 0
    #endif
#endif  // D_ENV_SMTP_HAS_SO_NOSIGPIPE

// 1.3    Host identity
//------------------------------------------------------------------------------
// 1.3.1
// D_ENV_SMTP_HAS_GETHOSTNAME
//   feature: gethostname() is available, so a client can offer the machine's
// fully qualified name in EHLO. Without it, the client falls back to an
// address literal of its end of the connection (RFC 5321 section 4.1.4).
#ifndef D_ENV_SMTP_HAS_GETHOSTNAME
    #if ( (D_ENV_SMTP_HAS_SOCKETS) &&                                          \
          (D_ENV_NET_HAS_UNISTD_H) )
        #define D_ENV_SMTP_HAS_GETHOSTNAME 1
    #else
        #define D_ENV_SMTP_HAS_GETHOSTNAME 0
    #endif
#endif  // D_ENV_SMTP_HAS_GETHOSTNAME


//==============================================================================
// 2.  SECURITY
//==============================================================================
// Transport security and the one SASL mechanism that needs a digest. Both come
// from the OpenSSL family (OpenSSL 1.1.0 or later, LibreSSL, BoringSSL); the
// other libraries env_tls.h recognizes are not yet backends here.


// 2.1    Transport security
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_SMTP_HAS_TLS
//   feature: STARTTLS and implicit TLS are compiled in. Requires the socket
// transport and an OpenSSL-family library with built-in hostname
// verification, which also guarantees the BIO_meth_* API (both arrived in
// OpenSSL 1.1.0). The build must then link -lssl -lcrypto.
#ifndef D_ENV_SMTP_HAS_TLS
    #if ( (D_ENV_SMTP_HAS_SOCKETS) &&                                          \
          (D_ENV_TLS_HAS_OPENSSL)  &&                                          \
          (D_ENV_TLS_HAS_HOSTNAME_VALIDATION) )
        #define D_ENV_SMTP_HAS_TLS 1
    #else
        #define D_ENV_SMTP_HAS_TLS 0
    #endif
#endif  // D_ENV_SMTP_HAS_TLS

// 2.2    Authentication
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_SMTP_HAS_CRAM_MD5
//   feature: the client can answer CRAM-MD5 challenges (RFC 2195), computing
// HMAC-MD5 with libcrypto. PLAIN, LOGIN and XOAUTH2 need no library and are
// always available.
#ifndef D_ENV_SMTP_HAS_CRAM_MD5
    #if D_ENV_SMTP_HAS_TLS
        #define D_ENV_SMTP_HAS_CRAM_MD5 1
    #else
        #define D_ENV_SMTP_HAS_CRAM_MD5 0
    #endif
#endif  // D_ENV_SMTP_HAS_CRAM_MD5


//==============================================================================
// 3.  CAPABILITIES
//==============================================================================
// What a build can do, rolled up from the features above. Code that decides
// whether to offer an operation reads these; code that implements one reads
// the individual features.


// 3.1    Rolled-up capabilities
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_SMTP_CAN_CONNECT
//   capability: d_smtp_client_connect() can open its own connection.
#ifndef D_ENV_SMTP_CAN_CONNECT
    #define D_ENV_SMTP_CAN_CONNECT D_ENV_SMTP_HAS_SOCKETS
#endif  // D_ENV_SMTP_CAN_CONNECT

// 3.1.2
// D_ENV_SMTP_CAN_SERVE
//   capability: d_smtp_listener_* can accept connections for the server.
#ifndef D_ENV_SMTP_CAN_SERVE
    #define D_ENV_SMTP_CAN_SERVE D_ENV_SMTP_HAS_SOCKETS
#endif  // D_ENV_SMTP_CAN_SERVE

// 3.1.3
// D_ENV_SMTP_CAN_TLS
//   capability: the built-in transport negotiates STARTTLS and implicit TLS,
// as a client and as a server.
#ifndef D_ENV_SMTP_CAN_TLS
    #define D_ENV_SMTP_CAN_TLS D_ENV_SMTP_HAS_TLS
#endif  // D_ENV_SMTP_CAN_TLS

// 3.2    Link requirements
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_SMTP_LINKS_OPENSSL
//   constant: 1 when the SMTP sources reference OpenSSL, and a build must
// therefore link -lssl -lcrypto.
#ifndef D_ENV_SMTP_LINKS_OPENSSL
    #if ( (D_ENV_SMTP_HAS_TLS) ||                                              \
          (D_ENV_SMTP_HAS_CRAM_MD5) )
        #define D_ENV_SMTP_LINKS_OPENSSL 1
    #else
        #define D_ENV_SMTP_LINKS_OPENSSL 0
    #endif
#endif  // D_ENV_SMTP_LINKS_OPENSSL

// 3.3    Consistency checks
//------------------------------------------------------------------------------
// Pre-definition can describe a platform detection would not; these reject
// combinations no platform can satisfy, before they mis-gate a build.
#if ( (D_ENV_SMTP_HAS_TLS) &&                                                  \
      (!D_ENV_SMTP_HAS_SOCKETS) )
    #error "D_ENV_SMTP_HAS_TLS requires D_ENV_SMTP_HAS_SOCKETS"
#endif

#if ( (D_ENV_SMTP_HAS_CRAM_MD5) &&                                             \
      (!D_ENV_TLS_HAS_OPENSSL) )
    #error "D_ENV_SMTP_HAS_CRAM_MD5 requires the OpenSSL family (libcrypto)"
#endif


#endif  // DJINTERP_ENV_NET_ENV_SMTP_H
