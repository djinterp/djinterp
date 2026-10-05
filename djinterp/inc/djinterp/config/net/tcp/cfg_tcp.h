/*******************************************************************************
* djinterp [config]                                                    cfg_tcp.h
*
* Configuration of the TCP transport.
*   Whether tcp.c builds its socket backend or stubs, and the listen backlog
* d_tcp_options_init gives listeners, resolved into D_INTERNAL_TCP_*.
*   targets:  net/tcp/tcp.h and tcp.c -> D_INTERNAL_TCP_ENABLED,
*             D_INTERNAL_TCP_BACKEND, D_INTERNAL_TCP_LISTEN_BACKLOG
*   requires: cfg_common.h; env_tcp.h, for the environment-detected default
*
*
* path:      /inc/djinterp/config/net/tcp/cfg_tcp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  The backend
         1.  D_CFG_TCP
    2.  Listeners
         1.  D_CFG_TCP_LISTEN_BACKLOG
2.  RESOLVED VALUES
    ---------------
    1.  Read by the module
         1.  D_INTERNAL_TCP_ENABLED
         2.  D_INTERNAL_TCP_BACKEND
         3.  D_INTERNAL_TCP_LISTEN_BACKLOG
*/

#ifndef DJINTERP_CONFIG_NET_TCP_CFG_TCP_H
#define DJINTERP_CONFIG_NET_TCP_CFG_TCP_H 1

// djinterp
#include "../../cfg_common.h"                // D_CFG_NORM, D_CFG_IS_ON
#include "../../../env/net/tcp/env_tcp.h"  // D_ENV_TCP_*


//==============================================================================
// 1.  KNOBS
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier wins.


// 1.1    The backend
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_TCP
//   knob: 1 builds the socket backend env_tcp.h selected; 0 builds stubs,
// whose operations report D_NET_ERROR_NETWORK_DOWN. Defaults to 1 where a
// backend with poll was found.
#ifndef D_CFG_TCP
    #if ( (D_ENV_TCP_AVAILABLE) &&                                             \
          (D_ENV_TCP_HAS_POLL) )
        #define D_CFG_TCP 1
    #else
        #define D_CFG_TCP 0
    #endif
#endif  // D_CFG_TCP

// 1.2    Listeners
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_TCP_LISTEN_BACKLOG
//   knob: the backlog d_tcp_options_init gives listeners; 1 to 65535.
#ifndef D_CFG_TCP_LISTEN_BACKLOG
    #define D_CFG_TCP_LISTEN_BACKLOG 128
#endif  // D_CFG_TCP_LISTEN_BACKLOG

#if ( !D_CFG_IS_ON(D_CFG_TCP) &&                                               \
      !D_CFG_IS_OFF(D_CFG_TCP) )
    #error "cfg_tcp: D_CFG_TCP must be 0 or 1"
#endif

#if ( (D_CFG_IS_ON(D_CFG_TCP)) &&                                              \
      ( (!D_ENV_TCP_AVAILABLE) ||                                              \
        (!D_ENV_TCP_HAS_POLL) ) )
    #error "cfg_tcp: D_CFG_TCP is 1, but env_tcp.h found no backend with poll"
#endif

#if ( (D_CFG_NORM(D_CFG_TCP_LISTEN_BACKLOG) < 1) ||                            \
      (D_CFG_NORM(D_CFG_TCP_LISTEN_BACKLOG) > 65535) )
    #error "cfg_tcp: D_CFG_TCP_LISTEN_BACKLOG must lie in [1, 65535]"
#endif


//==============================================================================
// 2.  RESOLVED VALUES
//==============================================================================
// What tcp.h and tcp.c read. Nothing outside this file resolves them.


// 2.1    Read by the module
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_TCP_ENABLED
//   resolved: 1 when the socket backend is built, 0 for stubs.
#define D_INTERNAL_TCP_ENABLED        D_CFG_NORM(D_CFG_TCP)

// 2.1.2
// D_INTERNAL_TCP_BACKEND
//   resolved: the env_tcp.h backend compiled, or D_ENV_TCP_BACKEND_NONE for
// stubs.
#if D_CFG_IS_ON(D_CFG_TCP)
    #define D_INTERNAL_TCP_BACKEND    D_ENV_TCP_BACKEND
#else
    #define D_INTERNAL_TCP_BACKEND    D_ENV_TCP_BACKEND_NONE
#endif

// 2.1.3
// D_INTERNAL_TCP_LISTEN_BACKLOG
//   resolved: the default listen backlog.
#define D_INTERNAL_TCP_LISTEN_BACKLOG D_CFG_NORM(D_CFG_TCP_LISTEN_BACKLOG)


#endif  // DJINTERP_CONFIG_NET_TCP_CFG_TCP_H
