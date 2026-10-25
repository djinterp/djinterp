/*******************************************************************************
* djinterp [env]                                                       env_tcp.h
*
* TCP environment detection.
*   What the TCP transport can be built on, derived from env_net.h's socket
* detection: the socket API tcp.c compiles against, and whether that API
* offers unix-domain stream sockets, TCP_NODELAY, poll, and a count of the
* bytes waiting on a socket. It includes no system header; env_net.h has
* already probed for most of them, and the rest are probed here.
*   Were env_net.h's detection ever incomplete when this header is reached,
* every flag here would read as absent; it refuses to compile in that case
* rather than disable TCP silently.
*
*
* path:      /inc/djinterp/env/net/tcp/env_tcp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BACKENDS
    --------
    1.  Backend identifiers
         1.  D_ENV_TCP_BACKEND_NONE
         2.  D_ENV_TCP_BACKEND_BSD
         3.  D_ENV_TCP_BACKEND_WINSOCK
    2.  Selection
         1.  D_ENV_TCP_BACKEND
2.  CAPABILITIES
    ------------
    1.  Features
         1.  D_ENV_TCP_AVAILABLE
         2.  D_ENV_TCP_HAS_UNIX
         3.  D_ENV_TCP_HAS_NODELAY
         4.  D_ENV_TCP_HAS_POLL
    2.  Waiting bytes
         1.  D_ENV_TCP_HAS_SYS_IOCTL_H
         2.  D_ENV_TCP_HAS_SYS_FILIO_H
         3.  D_ENV_TCP_CAN_COUNT
*/

#ifndef DJINTERP_ENV_NET_TCP_ENV_TCP_H
#define DJINTERP_ENV_NET_TCP_ENV_TCP_H 1

// djinterp
#include "../env_net.h"  // D_ENV_NET_SOCKET_BACKEND, D_ENV_NET_CAN_TCP,
                         // D_ENV_NET_CAN_UNIX, D_ENV_NET_HAS_*_H

#ifndef D_ENV_NET_CAN_TCP
    #error "env_tcp.h: include the framework root before this header"
#endif  // D_ENV_NET_CAN_TCP


//==============================================================================
// 1.  BACKENDS
//==============================================================================
// The socket APIs tcp.c has an implementation for.


// 1.1    Backend identifiers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_TCP_BACKEND_NONE
//   constant: no socket API; tcp.c builds stubs.
#define D_ENV_TCP_BACKEND_NONE    0

// 1.1.2
// D_ENV_TCP_BACKEND_BSD
//   constant: BSD sockets, as on Linux, the BSDs, and macOS.
#define D_ENV_TCP_BACKEND_BSD     1

// 1.1.3
// D_ENV_TCP_BACKEND_WINSOCK
//   constant: Winsock 2, as on Windows Vista and later.
#define D_ENV_TCP_BACKEND_WINSOCK 2

// 1.2    Selection
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_TCP_BACKEND
//   detection: the socket API tcp.c compiles against: env_net.h's socket
// backend where it can open a TCP stream, and none otherwise. Define it to
// override, D_ENV_TCP_BACKEND_NONE building without sockets.
#ifndef D_ENV_TCP_BACKEND
    #if !D_ENV_NET_CAN_TCP
        #define D_ENV_TCP_BACKEND D_ENV_TCP_BACKEND_NONE
    #elif (D_ENV_NET_SOCKET_BACKEND == D_ENV_NET_SOCKET_BACKEND_BSD)
        #define D_ENV_TCP_BACKEND D_ENV_TCP_BACKEND_BSD
    #elif (D_ENV_NET_SOCKET_BACKEND == D_ENV_NET_SOCKET_BACKEND_WINSOCK)
        #define D_ENV_TCP_BACKEND D_ENV_TCP_BACKEND_WINSOCK
    #else
        #define D_ENV_TCP_BACKEND D_ENV_TCP_BACKEND_NONE
    #endif
#endif  // D_ENV_TCP_BACKEND


//==============================================================================
// 2.  CAPABILITIES
//==============================================================================
// What the selected API offers. Each flag is 0 without a backend.


// 2.1    Features
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_TCP_AVAILABLE
//   detection: 1 when a socket API was selected.
#ifndef D_ENV_TCP_AVAILABLE
    #if (D_ENV_TCP_BACKEND != D_ENV_TCP_BACKEND_NONE)
        #define D_ENV_TCP_AVAILABLE 1
    #else
        #define D_ENV_TCP_AVAILABLE 0
    #endif
#endif  // D_ENV_TCP_AVAILABLE

// 2.1.2
// D_ENV_TCP_HAS_UNIX
//   detection: 1 when unix-domain stream sockets can be opened: BSD sockets
// with <sys/un.h>. Winsock's AF_UNIX is not used.
#ifndef D_ENV_TCP_HAS_UNIX
    #if ( (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_BSD) &&                      \
          (D_ENV_NET_CAN_UNIX) )
        #define D_ENV_TCP_HAS_UNIX 1
    #else
        #define D_ENV_TCP_HAS_UNIX 0
    #endif
#endif  // D_ENV_TCP_HAS_UNIX

// 2.1.3
// D_ENV_TCP_HAS_NODELAY
//   detection: 1 when TCP_NODELAY can be set: through <netinet/tcp.h> on BSD
// sockets, and always on Winsock.
#ifndef D_ENV_TCP_HAS_NODELAY
    #if ( (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_WINSOCK) ||                  \
          ( (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_BSD) &&                    \
            (D_ENV_NET_HAS_NETINET_TCP_H) ) )
        #define D_ENV_TCP_HAS_NODELAY 1
    #else
        #define D_ENV_TCP_HAS_NODELAY 0
    #endif
#endif  // D_ENV_TCP_HAS_NODELAY

// 2.1.4
// D_ENV_TCP_HAS_POLL
//   detection: 1 when readiness can be awaited: poll through <poll.h> on BSD
// sockets, and WSAPoll on Winsock. The connect timeout and a listener's
// wakeable accept rely on it.
#ifndef D_ENV_TCP_HAS_POLL
    #if ( (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_WINSOCK) ||                  \
          ( (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_BSD) &&                    \
            (D_ENV_NET_HAS_POLL_H) ) )
        #define D_ENV_TCP_HAS_POLL 1
    #else
        #define D_ENV_TCP_HAS_POLL 0
    #endif
#endif  // D_ENV_TCP_HAS_POLL

// 2.2    Waiting bytes
//------------------------------------------------------------------------------
// FIONREAD counts the bytes a socket has received and not yet handed over,
// without taking them: what d_tcp_connection_available reports, and what a
// STARTTLS client reads at its upgrade point to find plaintext an attacker
// appended to the go-ahead.

// 2.2.1
// D_ENV_TCP_HAS_SYS_IOCTL_H
//   detection: 1 when <sys/ioctl.h> is includable under BSD sockets. It
// carries FIONREAD on Linux, the BSDs, and macOS.
#ifndef D_ENV_TCP_HAS_SYS_IOCTL_H
    #if (D_ENV_TCP_BACKEND != D_ENV_TCP_BACKEND_BSD)
        #define D_ENV_TCP_HAS_SYS_IOCTL_H 0
    #elif D_ENV_NET_CAN_PROBE_INCLUDE
        #if D_ENV_NET_HAS_INCLUDE(<sys/ioctl.h>)
            #define D_ENV_TCP_HAS_SYS_IOCTL_H 1
        #else
            #define D_ENV_TCP_HAS_SYS_IOCTL_H 0
        #endif
    #else
        #define D_ENV_TCP_HAS_SYS_IOCTL_H 1
    #endif
#endif  // D_ENV_TCP_HAS_SYS_IOCTL_H

// 2.2.2
// D_ENV_TCP_HAS_SYS_FILIO_H
//   detection: 1 when <sys/filio.h> is includable under BSD sockets. Solaris
// and illumos define FIONREAD there, and their <sys/ioctl.h> includes it only
// under BSD_COMP, so tcp.c includes it directly where it exists.
#ifndef D_ENV_TCP_HAS_SYS_FILIO_H
    #if ( (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_BSD) &&                      \
          (D_ENV_NET_CAN_PROBE_INCLUDE) )
        #if D_ENV_NET_HAS_INCLUDE(<sys/filio.h>)
            #define D_ENV_TCP_HAS_SYS_FILIO_H 1
        #else
            #define D_ENV_TCP_HAS_SYS_FILIO_H 0
        #endif
    #else
        #define D_ENV_TCP_HAS_SYS_FILIO_H 0
    #endif
#endif  // D_ENV_TCP_HAS_SYS_FILIO_H

// 2.2.3
// D_ENV_TCP_CAN_COUNT
//   detection: 1 when a socket's waiting bytes can be counted: ioctlsocket
// with FIONREAD under Winsock, or ioctl with FIONREAD and one of the headers
// above under BSD sockets. The request itself is confirmed with #ifdef
// FIONREAD at the point of use: it is a symbol rather than a header, and this
// layer asserts headers only.
#ifndef D_ENV_TCP_CAN_COUNT
    #if ( (D_ENV_TCP_BACKEND == D_ENV_TCP_BACKEND_WINSOCK) ||                  \
          (D_ENV_TCP_HAS_SYS_IOCTL_H)                      ||                  \
          (D_ENV_TCP_HAS_SYS_FILIO_H) )
        #define D_ENV_TCP_CAN_COUNT 1
    #else
        #define D_ENV_TCP_CAN_COUNT 0
    #endif
#endif  // D_ENV_TCP_CAN_COUNT


#endif  // DJINTERP_ENV_NET_TCP_ENV_TCP_H
