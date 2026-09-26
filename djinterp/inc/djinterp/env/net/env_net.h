/*******************************************************************************
* djinterp [env]                                                       env_net.h
*
* djinterp generic networking environment detection.
*   Compile-time detection of the socket-level capabilities the net/ transport
* backends are built on, expressed through the unified D_ENV_NET_* interface.
* It is library-agnostic, and covers:
*     - the __has_include feature probe (D_ENV_NET_HAS_INCLUDE)            [1]
*     - socket API and backend classification (BSD sockets vs Winsock)    [2]
*     - platform networking header availability                           [3]
*     - readiness / event-multiplexing backend (select, poll, epoll,
*       kqueue)                                                           [4]
*     - address families, socket types, and name resolution (IPv6,
*       unix domain, getaddrinfo)                                         [5]
*     - host versus network byte order, and whether swaps are needed      [6]
*     - rolled-up capability summaries (CAN_SOCKET, CAN_TCP, CAN_UNIX)    [7]
*   It describes the environment the generic net foundation and its backends
* require, and is deliberately agnostic of any particular backend (POSIX
* sockets, Winsock, TLS libraries). The backend-specific modules env_tls.h and
* env_curl.h build on it.
*   A capability that is a socket option or function rather than a header
* (MSG_NOSIGNAL, SO_REUSEPORT, SO_NOSIGPIPE, SOCK_CLOEXEC, accept4) is not
* asserted here: those are best probed with #ifdef at the point of use, once a
* backend has included the concrete headers. This header confines itself to
* claims it can make honestly at the preprocessor level.
*   Naming: D_ENV_NET_HAS_<FEATURE> is 1 if available, 0 otherwise;
* D_ENV_NET_<FEATURE> is a non-boolean detected value or identifier.
*   Every flag is #ifndef-guarded, so a project may pre-define any D_ENV_NET_*
* macro before inclusion to override detection, for example to force a
* readiness backend or to simulate a target during testing.
*   It requires env.h, for the D_ENV_OS_*, D_ENV_ARCH_*, D_ENV_POSIX_*, and
* D_ENV_C_HAS_* families it reads, and includes it itself. It is an opt-in
* module and may be included directly.
*
* path:      /inc/djinterp/env/net/env_net.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.17
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FEATURE PROBE
    -------------
    1.  The __has_include probe
         1.  D_ENV_NET_CAN_PROBE_INCLUDE
         2.  D_ENV_NET_HAS_INCLUDE
2.  SOCKET API AND BACKEND
    ----------------------
    1.  Backend identifiers
         1.  D_ENV_NET_SOCKET_BACKEND_*
              1.  D_ENV_NET_SOCKET_BACKEND_NONE
              2.  D_ENV_NET_SOCKET_BACKEND_BSD
              3.  D_ENV_NET_SOCKET_BACKEND_WINSOCK
    2.  Backend selection
         1.  D_ENV_NET_HAS_BSD_SOCKETS
         2.  D_ENV_NET_HAS_WINSOCK
         3.  D_ENV_NET_SOCKET_BACKEND
         4.  D_ENV_NET_SOCKET_BACKEND_NAME
         5.  D_ENV_NET_HAS_SOCKETS
3.  PLATFORM NETWORKING HEADERS
    ---------------------------
    1.  BSD / POSIX networking headers
         1.  D_ENV_NET_HAS_SYS_SOCKET_H
         2.  D_ENV_NET_HAS_NETINET_IN_H
         3.  D_ENV_NET_HAS_NETINET_TCP_H
         4.  D_ENV_NET_HAS_ARPA_INET_H
         5.  D_ENV_NET_HAS_NETDB_H
         6.  D_ENV_NET_HAS_UNISTD_H
         7.  D_ENV_NET_HAS_FCNTL_H
         8.  D_ENV_NET_HAS_SYS_UN_H
    2.  Windows networking headers
         1.  D_ENV_NET_HAS_WINSOCK2_H
         2.  D_ENV_NET_HAS_WS2TCPIP_H
4.  READINESS BACKENDS
    ------------------
    1.  Readiness identifiers
         1.  D_ENV_NET_READINESS_*
              1.  D_ENV_NET_READINESS_NONE
              2.  D_ENV_NET_READINESS_SELECT
              3.  D_ENV_NET_READINESS_POLL
              4.  D_ENV_NET_READINESS_EPOLL
              5.  D_ENV_NET_READINESS_KQUEUE
    2.  Readiness headers and selection
         1.  D_ENV_NET_HAS_POLL_H
         2.  D_ENV_NET_HAS_SYS_SELECT_H
         3.  D_ENV_NET_HAS_EPOLL
         4.  D_ENV_NET_HAS_KQUEUE
         5.  D_ENV_NET_READINESS_BACKEND
         6.  D_ENV_NET_READINESS_BACKEND_NAME
5.  ADDRESSING AND NAME RESOLUTION
    ------------------------------
    1.  Address families and socket types
         1.  D_ENV_NET_HAS_IPV6
         2.  D_ENV_NET_HAS_UNIX_SOCKETS
    2.  Name resolution
         1.  D_ENV_NET_HAS_GETADDRINFO
6.  BYTE ORDER
    ----------
    1.  Host and network byte order
         1.  D_ENV_NET_BYTE_ORDER
         2.  D_ENV_NET_IS_LITTLE_ENDIAN
         3.  D_ENV_NET_IS_BIG_ENDIAN
         4.  D_ENV_NET_NEEDS_BYTE_SWAP
7.  CAPABILITY SUMMARIES
    --------------------
    1.  Rolled-up capabilities
         1.  D_ENV_NET_CAN_SOCKET
         2.  D_ENV_NET_CAN_TCP
         3.  D_ENV_NET_CAN_UNIX
*/

#ifndef DJINTERP_ENV_NET_ENV_NET_H
#define DJINTERP_ENV_NET_ENV_NET_H 1

// djinterp
#include "../env.h"  // D_ENV_OS_*, D_ENV_ARCH_ENDIAN, D_ENV_C_HAS_*


//==============================================================================
// 1.  FEATURE PROBE
//==============================================================================
// __has_include is a preprocessor operator standardized in C++17 / C23 but
// available as an extension in GCC, Clang, and MSVC 2017+ well before that.
// The wrapper collapses to 0 on the rare toolchain that lacks it, so callers
// never have to spell the `defined(__has_include)` guard themselves.


// 1.1    The __has_include probe
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_NET_CAN_PROBE_INCLUDE
//   feature: 1 when the __has_include operator is usable, 0 otherwise.
#ifndef D_ENV_NET_CAN_PROBE_INCLUDE
    #if defined(__has_include)
        #define D_ENV_NET_CAN_PROBE_INCLUDE 1
    #else
        #define D_ENV_NET_CAN_PROBE_INCLUDE 0
    #endif
#endif  // D_ENV_NET_CAN_PROBE_INCLUDE

// 1.1.2
// D_ENV_NET_HAS_INCLUDE
//   macro: evaluates to 1 if HEADER is includable, 0 otherwise (and 0 on a
// toolchain without __has_include). HEADER is spelled exactly as in an
// #include directive, e.g. D_ENV_NET_HAS_INCLUDE(<sys/epoll.h>).
#ifndef D_ENV_NET_HAS_INCLUDE
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #define D_ENV_NET_HAS_INCLUDE(HEADER) __has_include(HEADER)
    #else
        #define D_ENV_NET_HAS_INCLUDE(HEADER) 0
    #endif
#endif  // D_ENV_NET_HAS_INCLUDE


//==============================================================================
// 2.  SOCKET API AND BACKEND
//==============================================================================
// The two mutually-exclusive transport families: the BSD sockets API (POSIX-
// like systems) and the Windows Sockets 2 API (Winsock). These fold the
// D_ENV_C_HAS_* network flags from env_c_lib.h into a single backend
// identifier for switch-style dispatch.


// 2.1    Backend identifiers
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_NET_SOCKET_BACKEND_*
//   constant: identifiers for D_ENV_NET_SOCKET_BACKEND, for switch-style
// dispatch.

// 2.1.1.1
// D_ENV_NET_SOCKET_BACKEND_NONE
//   constant: identifies no socket transport.
#define D_ENV_NET_SOCKET_BACKEND_NONE    0

// 2.1.1.2
// D_ENV_NET_SOCKET_BACKEND_BSD
//   constant: identifies the BSD sockets API.
#define D_ENV_NET_SOCKET_BACKEND_BSD     1

// 2.1.1.3
// D_ENV_NET_SOCKET_BACKEND_WINSOCK
//   constant: identifies the Windows Sockets 2 API (Winsock).
#define D_ENV_NET_SOCKET_BACKEND_WINSOCK 2

// 2.2    Backend selection
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_NET_HAS_BSD_SOCKETS
//   feature: 1 if the BSD sockets API is available.
#ifndef D_ENV_NET_HAS_BSD_SOCKETS
    #if D_ENV_C_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_BSD_SOCKETS 1
    #else
        #define D_ENV_NET_HAS_BSD_SOCKETS 0
    #endif
#endif  // D_ENV_NET_HAS_BSD_SOCKETS

// 2.2.2
// D_ENV_NET_HAS_WINSOCK
//   feature: 1 if the Windows Sockets (Winsock) API is available.
#ifndef D_ENV_NET_HAS_WINSOCK
    #if D_ENV_C_HAS_WINSOCK
        #define D_ENV_NET_HAS_WINSOCK 1
    #else
        #define D_ENV_NET_HAS_WINSOCK 0
    #endif
#endif  // D_ENV_NET_HAS_WINSOCK

// 2.2.3
// D_ENV_NET_SOCKET_BACKEND
//   feature: the selected socket backend identifier. BSD wins when both are
// somehow flagged (a POSIX layer on Windows, e.g. Cygwin), since it is the
// portable path.
#ifndef D_ENV_NET_SOCKET_BACKEND
    #if D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_SOCKET_BACKEND D_ENV_NET_SOCKET_BACKEND_BSD
    #elif D_ENV_NET_HAS_WINSOCK
        #define D_ENV_NET_SOCKET_BACKEND D_ENV_NET_SOCKET_BACKEND_WINSOCK
    #else
        #define D_ENV_NET_SOCKET_BACKEND D_ENV_NET_SOCKET_BACKEND_NONE
    #endif
#endif  // D_ENV_NET_SOCKET_BACKEND

// 2.2.4
// D_ENV_NET_SOCKET_BACKEND_NAME
//   value: a human-readable name for the selected socket backend.
#ifndef D_ENV_NET_SOCKET_BACKEND_NAME
    #if (D_ENV_NET_SOCKET_BACKEND == D_ENV_NET_SOCKET_BACKEND_BSD)
        #define D_ENV_NET_SOCKET_BACKEND_NAME "BSD sockets"
    #elif (D_ENV_NET_SOCKET_BACKEND == D_ENV_NET_SOCKET_BACKEND_WINSOCK)
        #define D_ENV_NET_SOCKET_BACKEND_NAME "Winsock"
    #else
        #define D_ENV_NET_SOCKET_BACKEND_NAME "none"
    #endif
#endif  // D_ENV_NET_SOCKET_BACKEND_NAME

// 2.2.5
// D_ENV_NET_HAS_SOCKETS
//   feature: 1 if any socket transport is available at all.
#ifndef D_ENV_NET_HAS_SOCKETS
    #if (D_ENV_NET_SOCKET_BACKEND != D_ENV_NET_SOCKET_BACKEND_NONE)
        #define D_ENV_NET_HAS_SOCKETS 1
    #else
        #define D_ENV_NET_HAS_SOCKETS 0
    #endif
#endif  // D_ENV_NET_HAS_SOCKETS


//==============================================================================
// 3.  PLATFORM NETWORKING HEADERS
//==============================================================================
// Availability of the individual headers a sockets backend includes. Each is
// probed with __has_include where possible; on a toolchain without the probe,
// it falls back to the socket-backend classification (BSD headers assumed
// present with BSD sockets, Winsock headers with Winsock).


// 3.1    BSD / POSIX networking headers
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_NET_HAS_SYS_SOCKET_H
//   feature: <sys/socket.h> (core BSD socket API).
#ifndef D_ENV_NET_HAS_SYS_SOCKET_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<sys/socket.h>)
            #define D_ENV_NET_HAS_SYS_SOCKET_H 1
        #else
            #define D_ENV_NET_HAS_SYS_SOCKET_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_SYS_SOCKET_H 1
    #else
        #define D_ENV_NET_HAS_SYS_SOCKET_H 0
    #endif
#endif  // D_ENV_NET_HAS_SYS_SOCKET_H

// 3.1.2
// D_ENV_NET_HAS_NETINET_IN_H
//   feature: <netinet/in.h> (IPv4/IPv6 address structures, htons/htonl).
#ifndef D_ENV_NET_HAS_NETINET_IN_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<netinet/in.h>)
            #define D_ENV_NET_HAS_NETINET_IN_H 1
        #else
            #define D_ENV_NET_HAS_NETINET_IN_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_NETINET_IN_H 1
    #else
        #define D_ENV_NET_HAS_NETINET_IN_H 0
    #endif
#endif  // D_ENV_NET_HAS_NETINET_IN_H

// 3.1.3
// D_ENV_NET_HAS_NETINET_TCP_H
//   feature: <netinet/tcp.h> (TCP-level options such as TCP_NODELAY).
#ifndef D_ENV_NET_HAS_NETINET_TCP_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<netinet/tcp.h>)
            #define D_ENV_NET_HAS_NETINET_TCP_H 1
        #else
            #define D_ENV_NET_HAS_NETINET_TCP_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_NETINET_TCP_H 1
    #else
        #define D_ENV_NET_HAS_NETINET_TCP_H 0
    #endif
#endif  // D_ENV_NET_HAS_NETINET_TCP_H

// 3.1.4
// D_ENV_NET_HAS_ARPA_INET_H
//   feature: <arpa/inet.h> (inet_ntop/inet_pton and byte-order helpers).
#ifndef D_ENV_NET_HAS_ARPA_INET_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<arpa/inet.h>)
            #define D_ENV_NET_HAS_ARPA_INET_H 1
        #else
            #define D_ENV_NET_HAS_ARPA_INET_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_ARPA_INET_H 1
    #else
        #define D_ENV_NET_HAS_ARPA_INET_H 0
    #endif
#endif  // D_ENV_NET_HAS_ARPA_INET_H

// 3.1.5
// D_ENV_NET_HAS_NETDB_H
//   feature: <netdb.h> (getaddrinfo/getnameinfo name resolution).
#ifndef D_ENV_NET_HAS_NETDB_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<netdb.h>)
            #define D_ENV_NET_HAS_NETDB_H 1
        #else
            #define D_ENV_NET_HAS_NETDB_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_NETDB_H 1
    #else
        #define D_ENV_NET_HAS_NETDB_H 0
    #endif
#endif  // D_ENV_NET_HAS_NETDB_H

// 3.1.6
// D_ENV_NET_HAS_UNISTD_H
//   feature: <unistd.h> (close/read/write on descriptors).
#ifndef D_ENV_NET_HAS_UNISTD_H
    #if D_ENV_C_HAS_UNISTD_H
        #define D_ENV_NET_HAS_UNISTD_H 1
    #else
        #define D_ENV_NET_HAS_UNISTD_H 0
    #endif
#endif  // D_ENV_NET_HAS_UNISTD_H

// 3.1.7
// D_ENV_NET_HAS_FCNTL_H
//   feature: <fcntl.h> (O_NONBLOCK / FD_CLOEXEC via fcntl).
#ifndef D_ENV_NET_HAS_FCNTL_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<fcntl.h>)
            #define D_ENV_NET_HAS_FCNTL_H 1
        #else
            #define D_ENV_NET_HAS_FCNTL_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_FCNTL_H 1
    #else
        #define D_ENV_NET_HAS_FCNTL_H 0
    #endif
#endif  // D_ENV_NET_HAS_FCNTL_H

// 3.1.8
// D_ENV_NET_HAS_SYS_UN_H
//   feature: <sys/un.h> (unix-domain socket address structure).
#ifndef D_ENV_NET_HAS_SYS_UN_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<sys/un.h>)
            #define D_ENV_NET_HAS_SYS_UN_H 1
        #else
            #define D_ENV_NET_HAS_SYS_UN_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_SYS_UN_H 1
    #else
        #define D_ENV_NET_HAS_SYS_UN_H 0
    #endif
#endif  // D_ENV_NET_HAS_SYS_UN_H

// 3.2    Windows networking headers
//------------------------------------------------------------------------------
// 3.2.1
// D_ENV_NET_HAS_WINSOCK2_H
//   feature: <winsock2.h> (Windows Sockets 2 core API).
#ifndef D_ENV_NET_HAS_WINSOCK2_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<winsock2.h>)
            #define D_ENV_NET_HAS_WINSOCK2_H 1
        #else
            #define D_ENV_NET_HAS_WINSOCK2_H 0
        #endif
    #elif D_ENV_NET_HAS_WINSOCK
        #define D_ENV_NET_HAS_WINSOCK2_H 1
    #else
        #define D_ENV_NET_HAS_WINSOCK2_H 0
    #endif
#endif  // D_ENV_NET_HAS_WINSOCK2_H

// 3.2.2
// D_ENV_NET_HAS_WS2TCPIP_H
//   feature: <ws2tcpip.h> (getaddrinfo / inet_ntop on Windows).
#ifndef D_ENV_NET_HAS_WS2TCPIP_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<ws2tcpip.h>)
            #define D_ENV_NET_HAS_WS2TCPIP_H 1
        #else
            #define D_ENV_NET_HAS_WS2TCPIP_H 0
        #endif
    #elif D_ENV_NET_HAS_WINSOCK
        #define D_ENV_NET_HAS_WS2TCPIP_H 1
    #else
        #define D_ENV_NET_HAS_WS2TCPIP_H 0
    #endif
#endif  // D_ENV_NET_HAS_WS2TCPIP_H


//==============================================================================
// 4.  READINESS BACKENDS
//==============================================================================
// Which readiness mechanism a future non-blocking / reactor backend can use.
// Probed by header presence; the summary identifier prefers the most scalable
// available (epoll on Linux, kqueue on the BSDs/macOS, then poll, then
// select). The blocking foundation does not require any of these -- they are
// detected for the layers built above it.


// 4.1    Readiness identifiers
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_NET_READINESS_*
//   constant: identifiers for D_ENV_NET_READINESS_BACKEND, in increasing order
// of scalability.

// 4.1.1.1
// D_ENV_NET_READINESS_NONE
//   constant: identifies no readiness mechanism.
#define D_ENV_NET_READINESS_NONE         0

// 4.1.1.2
// D_ENV_NET_READINESS_SELECT
//   constant: identifies select().
#define D_ENV_NET_READINESS_SELECT       1

// 4.1.1.3
// D_ENV_NET_READINESS_POLL
//   constant: identifies poll().
#define D_ENV_NET_READINESS_POLL         2

// 4.1.1.4
// D_ENV_NET_READINESS_EPOLL
//   constant: identifies Linux epoll.
#define D_ENV_NET_READINESS_EPOLL        3

// 4.1.1.5
// D_ENV_NET_READINESS_KQUEUE
//   constant: identifies BSD / macOS kqueue.
#define D_ENV_NET_READINESS_KQUEUE       4

// 4.2    Readiness headers and selection
//------------------------------------------------------------------------------
// 4.2.1
// D_ENV_NET_HAS_POLL_H
//   feature: <poll.h> (poll()).
#ifndef D_ENV_NET_HAS_POLL_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<poll.h>)
            #define D_ENV_NET_HAS_POLL_H 1
        #else
            #define D_ENV_NET_HAS_POLL_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_POLL_H 1
    #else
        #define D_ENV_NET_HAS_POLL_H 0
    #endif
#endif  // D_ENV_NET_HAS_POLL_H

// 4.2.2
// D_ENV_NET_HAS_SYS_SELECT_H
//   feature: <sys/select.h> (select() and fd_set).
#ifndef D_ENV_NET_HAS_SYS_SELECT_H
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<sys/select.h>)
            #define D_ENV_NET_HAS_SYS_SELECT_H 1
        #else
            #define D_ENV_NET_HAS_SYS_SELECT_H 0
        #endif
    #elif D_ENV_NET_HAS_BSD_SOCKETS
        #define D_ENV_NET_HAS_SYS_SELECT_H 1
    #else
        #define D_ENV_NET_HAS_SYS_SELECT_H 0
    #endif
#endif  // D_ENV_NET_HAS_SYS_SELECT_H

// 4.2.3
// D_ENV_NET_HAS_EPOLL
//   feature: <sys/epoll.h> (Linux epoll).
#ifndef D_ENV_NET_HAS_EPOLL
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<sys/epoll.h>)
            #define D_ENV_NET_HAS_EPOLL 1
        #else
            #define D_ENV_NET_HAS_EPOLL 0
        #endif
    #else
        #define D_ENV_NET_HAS_EPOLL 0
    #endif
#endif  // D_ENV_NET_HAS_EPOLL

// 4.2.4
// D_ENV_NET_HAS_KQUEUE
//   feature: <sys/event.h> (BSD/macOS kqueue).
#ifndef D_ENV_NET_HAS_KQUEUE
    #if D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<sys/event.h>)
            #define D_ENV_NET_HAS_KQUEUE 1
        #else
            #define D_ENV_NET_HAS_KQUEUE 0
        #endif
    #else
        #define D_ENV_NET_HAS_KQUEUE 0
    #endif
#endif  // D_ENV_NET_HAS_KQUEUE

// 4.2.5
// D_ENV_NET_READINESS_BACKEND
//   feature: the preferred available readiness mechanism (most scalable first).
#ifndef D_ENV_NET_READINESS_BACKEND
    #if D_ENV_NET_HAS_EPOLL
        #define D_ENV_NET_READINESS_BACKEND D_ENV_NET_READINESS_EPOLL
    #elif D_ENV_NET_HAS_KQUEUE
        #define D_ENV_NET_READINESS_BACKEND D_ENV_NET_READINESS_KQUEUE
    #elif D_ENV_NET_HAS_POLL_H
        #define D_ENV_NET_READINESS_BACKEND D_ENV_NET_READINESS_POLL
    #elif ( (D_ENV_NET_HAS_SYS_SELECT_H) ||                                    \
            (D_ENV_NET_HAS_WINSOCK) )
        #define D_ENV_NET_READINESS_BACKEND D_ENV_NET_READINESS_SELECT
    #else
        #define D_ENV_NET_READINESS_BACKEND D_ENV_NET_READINESS_NONE
    #endif
#endif  // D_ENV_NET_READINESS_BACKEND

// 4.2.6
// D_ENV_NET_READINESS_BACKEND_NAME
//   value: a human-readable name for the selected readiness backend.
#ifndef D_ENV_NET_READINESS_BACKEND_NAME
    #if (D_ENV_NET_READINESS_BACKEND == D_ENV_NET_READINESS_EPOLL)
        #define D_ENV_NET_READINESS_BACKEND_NAME "epoll"
    #elif (D_ENV_NET_READINESS_BACKEND == D_ENV_NET_READINESS_KQUEUE)
        #define D_ENV_NET_READINESS_BACKEND_NAME "kqueue"
    #elif (D_ENV_NET_READINESS_BACKEND == D_ENV_NET_READINESS_POLL)
        #define D_ENV_NET_READINESS_BACKEND_NAME "poll"
    #elif (D_ENV_NET_READINESS_BACKEND == D_ENV_NET_READINESS_SELECT)
        #define D_ENV_NET_READINESS_BACKEND_NAME "select"
    #else
        #define D_ENV_NET_READINESS_BACKEND_NAME "none"
    #endif
#endif  // D_ENV_NET_READINESS_BACKEND_NAME


//==============================================================================
// 5.  ADDRESSING AND NAME RESOLUTION
//==============================================================================


// 5.1    Address families and socket types
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_NET_HAS_IPV6
//   feature: 1 if IPv6 address structures are expected to be available -- the
// BSD path requires <netinet/in.h>; the Winsock path requires <ws2tcpip.h>.
#ifndef D_ENV_NET_HAS_IPV6
    #if ( (D_ENV_NET_HAS_BSD_SOCKETS) &&                                       \
          (D_ENV_NET_HAS_NETINET_IN_H) )
        #define D_ENV_NET_HAS_IPV6 1
    #elif ( (D_ENV_NET_HAS_WINSOCK) &&                                         \
            (D_ENV_NET_HAS_WS2TCPIP_H) )
        #define D_ENV_NET_HAS_IPV6 1
    #else
        #define D_ENV_NET_HAS_IPV6 0
    #endif
#endif  // D_ENV_NET_HAS_IPV6

// 5.1.2
// D_ENV_NET_HAS_UNIX_SOCKETS
//   feature: 1 if unix-domain sockets are available (gated on <sys/un.h>;
// Windows AF_UNIX via <afunix.h> is not asserted here).
#ifndef D_ENV_NET_HAS_UNIX_SOCKETS
    #if D_ENV_NET_HAS_SYS_UN_H
        #define D_ENV_NET_HAS_UNIX_SOCKETS 1
    #else
        #define D_ENV_NET_HAS_UNIX_SOCKETS 0
    #endif
#endif  // D_ENV_NET_HAS_UNIX_SOCKETS

// 5.2    Name resolution
//------------------------------------------------------------------------------
// 5.2.1
// D_ENV_NET_HAS_GETADDRINFO
//   feature: getaddrinfo, the protocol-independent resolver, is available.
#ifndef D_ENV_NET_HAS_GETADDRINFO
    #if D_ENV_C_HAS_GETADDRINFO
        #define D_ENV_NET_HAS_GETADDRINFO 1
    #else
        #define D_ENV_NET_HAS_GETADDRINFO 0
    #endif
#endif  // D_ENV_NET_HAS_GETADDRINFO


//==============================================================================
// 6.  BYTE ORDER
//==============================================================================
// Network byte order is big-endian. These derive the host order from the
// architecture layer, so higher layers can decide at compile time whether
// hton / ntoh-style swaps are actually needed.


// 6.1    Host and network byte order
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_NET_BYTE_ORDER
//   value: the host byte order, taken from the architecture layer. Network
// byte order is big-endian; a backend compares against this to decide whether
// htons/htonl are no-ops. (The net foundation's own framing packs big-endian
// by hand and so is independent of this.)
#ifndef D_ENV_NET_BYTE_ORDER
    #define D_ENV_NET_BYTE_ORDER D_ENV_ARCH_ENDIAN
#endif  // D_ENV_NET_BYTE_ORDER

// 6.1.2
// D_ENV_NET_IS_LITTLE_ENDIAN
//   feature: 1 if the host is little-endian.
#ifndef D_ENV_NET_IS_LITTLE_ENDIAN
    #if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_LITTLE)
        #define D_ENV_NET_IS_LITTLE_ENDIAN 1
    #else
        #define D_ENV_NET_IS_LITTLE_ENDIAN 0
    #endif
#endif  // D_ENV_NET_IS_LITTLE_ENDIAN

// 6.1.3
// D_ENV_NET_IS_BIG_ENDIAN
//   feature: 1 if the host is big-endian.
#ifndef D_ENV_NET_IS_BIG_ENDIAN
    #if (D_ENV_ARCH_ENDIAN == D_ENV_ARCH_ENDIAN_BIG)
        #define D_ENV_NET_IS_BIG_ENDIAN 1
    #else
        #define D_ENV_NET_IS_BIG_ENDIAN 0
    #endif
#endif  // D_ENV_NET_IS_BIG_ENDIAN

// 6.1.4
// D_ENV_NET_NEEDS_BYTE_SWAP
//   feature: 1 when host order differs from network (big-endian) order, so
// multi-byte integer fields must be swapped on the wire: little-endian hosts
// need the swap, big-endian hosts do not.
#ifndef D_ENV_NET_NEEDS_BYTE_SWAP
    #if D_ENV_NET_IS_LITTLE_ENDIAN
        #define D_ENV_NET_NEEDS_BYTE_SWAP 1
    #else
        #define D_ENV_NET_NEEDS_BYTE_SWAP 0
    #endif
#endif  // D_ENV_NET_NEEDS_BYTE_SWAP


//==============================================================================
// 7.  CAPABILITY SUMMARIES
//==============================================================================


// 7.1    Rolled-up capabilities
//------------------------------------------------------------------------------
// 7.1.1
// D_ENV_NET_CAN_SOCKET
//   feature: 1 if the platform can do socket I/O at all -- a socket backend
// plus the core address structures. The baseline gate for the net backends.
#ifndef D_ENV_NET_CAN_SOCKET
    #if D_ENV_NET_HAS_BSD_SOCKETS
        #if ( (D_ENV_NET_HAS_SYS_SOCKET_H) &&                                  \
              (D_ENV_NET_HAS_NETINET_IN_H) )
            #define D_ENV_NET_CAN_SOCKET 1
        #else
            #define D_ENV_NET_CAN_SOCKET 0
        #endif
    #elif D_ENV_NET_HAS_WINSOCK
        #if D_ENV_NET_HAS_WINSOCK2_H
            #define D_ENV_NET_CAN_SOCKET 1
        #else
            #define D_ENV_NET_CAN_SOCKET 0
        #endif
    #else
        #define D_ENV_NET_CAN_SOCKET 0
    #endif
#endif  // D_ENV_NET_CAN_SOCKET

// 7.1.2
// D_ENV_NET_CAN_TCP
//   feature: 1 if TCP stream sockets with name resolution are usable.
#ifndef D_ENV_NET_CAN_TCP
    #if D_ENV_NET_HAS_BSD_SOCKETS
        #if ( (D_ENV_NET_CAN_SOCKET) &&                                        \
              (D_ENV_NET_HAS_NETDB_H) )
            #define D_ENV_NET_CAN_TCP 1
        #else
            #define D_ENV_NET_CAN_TCP 0
        #endif
    #elif D_ENV_NET_HAS_WINSOCK
        #if ( (D_ENV_NET_CAN_SOCKET) &&                                        \
              (D_ENV_NET_HAS_WS2TCPIP_H) )
            #define D_ENV_NET_CAN_TCP 1
        #else
            #define D_ENV_NET_CAN_TCP 0
        #endif
    #else
        #define D_ENV_NET_CAN_TCP 0
    #endif
#endif  // D_ENV_NET_CAN_TCP

// 7.1.3
// D_ENV_NET_CAN_UNIX
//   feature: 1 if unix-domain stream sockets are usable.
#ifndef D_ENV_NET_CAN_UNIX
    #if ( (D_ENV_NET_CAN_SOCKET) &&                                            \
          (D_ENV_NET_HAS_UNIX_SOCKETS) )
        #define D_ENV_NET_CAN_UNIX 1
    #else
        #define D_ENV_NET_CAN_UNIX 0
    #endif
#endif  // D_ENV_NET_CAN_UNIX


#endif  // DJINTERP_ENV_NET_ENV_NET_H
