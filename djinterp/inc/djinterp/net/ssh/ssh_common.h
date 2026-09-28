/*******************************************************************************
* djinterp [net]                                                    ssh_common.h
*
* SSH shared base: what every other SSH header builds on.
*   The status every fallible call returns, and which of its values end a
* session; the platform's socket type, which the caller connects and a
* session runs over; and the port SSH is served on by default.
*
*
* path:      /inc/djinterp/net/ssh/ssh_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Outcomes
         1.  d_ssh_status
    2.  Sockets
         1.  d_ssh_socket
         2.  D_SSH_SOCKET_INVALID
    3.  Constants
         1.  D_SSH_PORT
2.  NAMES
    -----
    1.  Status names
*/

#ifndef DJINTERP_NET_SSH_SSH_COMMON_H
#define DJINTERP_NET_SSH_SSH_COMMON_H 1

// std
#include <stdint.h>  // uintptr_t
// djinterp
#include "../../c/djinterp.h"              // framework root
#include "../../config/net/ssh/cfg_ssh.h"  // D_INTERNAL_SSH_*


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Outcomes
//------------------------------------------------------------------------------
// 1.1.1
// d_ssh_status
//   enum: the outcome of every fallible call. A value marked FATAL, returned
// by a session or channel call, leaves the session D_SSH_STATE_FAILED: its
// transport can no longer be trusted, and only d_ssh_disconnect and
// d_ssh_destroy remain useful. Every other value leaves the session as it
// was, so the call can be retried or another one tried.
enum d_ssh_status
{
    D_SSH_OK                 = 0,   // success
    D_SSH_ERR_ARGUMENT       = 1,   // invalid argument; nothing was sent
    D_SSH_ERR_STATE          = 2,   // not valid in the current state
    D_SSH_ERR_MEMORY         = 3,   // allocation failed (FATAL mid-session)
    D_SSH_ERR_IO             = 4,   // transport failed (FATAL)
    D_SSH_ERR_CLOSED         = 5,   // a channel stream has ended
    D_SSH_ERR_TIMEOUT        = 6,   // the deadline expired (FATAL)
    D_SSH_ERR_PROTOCOL       = 7,   // the peer broke the protocol (FATAL)
    D_SSH_ERR_FORMAT         = 8,   // local data is malformed
    D_SSH_ERR_NO_BACKEND     = 9,   // no SSH engine is available
    D_SSH_ERR_UNSUPPORTED    = 10,  // the engine or platform cannot do it
    D_SSH_ERR_HANDSHAKE      = 11,  // key exchange failed (FATAL)
    D_SSH_ERR_HOST_UNKNOWN   = 12,  // unrecorded host refused (FATAL)
    D_SSH_ERR_HOST_CHANGED   = 13,  // host key differs (FATAL)
    D_SSH_ERR_HOST_REVOKED   = 14,  // host key is revoked (FATAL)
    D_SSH_ERR_KNOWN_HOSTS    = 15,  // known_hosts file failed (FATAL)
    D_SSH_ERR_AUTH_DENIED    = 16,  // the server refused every attempt
    D_SSH_ERR_NO_AUTH_METHOD = 17,  // no credential fits an offered method
    D_SSH_ERR_KEY            = 18,  // a private key could not be loaded
    D_SSH_ERR_CHANNEL        = 19   // the server refused a channel request
};

// 1.2    Sockets
//------------------------------------------------------------------------------
// 1.2.1
// d_ssh_socket
//   typedef: the platform's socket: an int descriptor under BSD sockets, and
// under Winsock an unsigned handle as wide as a pointer (SOCKET), declared
// here without <winsock2.h>.
#if (D_INTERNAL_SSH_SOCKET_HANDLE == 1)
    typedef uintptr_t d_ssh_socket;
#else
    typedef int d_ssh_socket;
#endif

// 1.2.2
// D_SSH_SOCKET_INVALID
//   constant: the invalid socket value (-1, or INVALID_SOCKET).
#if (D_INTERNAL_SSH_SOCKET_HANDLE == 1)
    #define D_SSH_SOCKET_INVALID ((d_ssh_socket)~(uintptr_t)0)
#else
    #define D_SSH_SOCKET_INVALID ((d_ssh_socket)-1)
#endif

// 1.3    Constants
//------------------------------------------------------------------------------
// 1.3.1
// D_SSH_PORT
//   constant: the port SSH is served on unless configured otherwise.
#define D_SSH_PORT 22


//==============================================================================
// 2.  NAMES
//==============================================================================


// 2.1    Status names
//------------------------------------------------------------------------------
/**
 * @brief Names a status.
 *
 * @param[in] _status  the status.
 * @return a static, human-readable name, or "unknown" for a value outside
 *         the enum.
 */
D_NODISCARD const char*
d_ssh_status_string(enum d_ssh_status _status);


D_EXTERN_C_END


#endif  // DJINTERP_NET_SSH_SSH_COMMON_H
