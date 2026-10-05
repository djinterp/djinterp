/*******************************************************************************
* djinterp [config]                                                    cfg_net.h
*
* Build-time configuration for the net foundation.
*   Sizes the endpoint record and the scratch buffer of net/net.h, and fixes
* the frame ceiling its framing helpers default to. Every knob reaches the
* C++ layer too, since net/net.hpp is built on net/net.h.
*
*   targets:  net/net.h -> D_INTERNAL_NET_HOST_MAX, D_INTERNAL_NET_IO_CHUNK,
*             D_INTERNAL_NET_FRAME_MAX
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/net/cfg_net.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FOUNDATION
    ----------
    1.  Records and buffers
         1.  D_CFG_NET_HOST_MAX
         2.  D_CFG_NET_IO_CHUNK
    2.  Framing
         1.  D_CFG_NET_FRAME_MAX
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_NET_HOST_MAX
         2.  D_INTERNAL_NET_IO_CHUNK
         3.  D_INTERNAL_NET_FRAME_MAX
*/

#ifndef DJINTERP_CONFIG_NET_CFG_NET_H
#define DJINTERP_CONFIG_NET_CFG_NET_H 1

// djinterp
#include "../cfg_common.h"  // D_CFG_NORM


//==============================================================================
// 1.  FOUNDATION
//==============================================================================
// Knobs read by the net foundation, net/net.h. None depends on the platform:
// the foundation performs no I/O of its own, so the environment has nothing
// to say about them.


// 1.1    Records and buffers
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_NET_HOST_MAX
//   brief: the longest host a d_net_endpoint stores, in bytes, terminator
// excluded -- a name, a numeric literal, or a unix-domain path. Default 255,
// which holds any DNS name (253 at most), any IPv6 literal with a zone, and
// every platform's unix-domain path (108 at most). Each endpoint embeds this
// many bytes plus one. Must lie in [64, 1024].
#ifndef D_CFG_NET_HOST_MAX
#   define D_CFG_NET_HOST_MAX 255
#endif  // D_CFG_NET_HOST_MAX

// 1.1.2
// D_CFG_NET_IO_CHUNK
//   brief: the size in bytes of the stack buffer the stream algorithms move
// data through -- read-all, pump, framed reads into a sink, and a small
// frame's single write -- and so the most any one of their reads asks for.
// Default 4096. Raising it trades stack for fewer calls on bulk transfers.
// Must lie in [256, 65536].
#ifndef D_CFG_NET_IO_CHUNK
#   define D_CFG_NET_IO_CHUNK 4096
#endif  // D_CFG_NET_IO_CHUNK

// 1.2    Framing
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_NET_FRAME_MAX
//   brief: the default ceiling on one framed message's payload, in bytes. It
// becomes D_NET_FRAME_MAX, which the C++ framing helpers take by default and
// C callers pass explicitly. A declared length above the ceiling is refused
// before its payload is read, which is what stops a hostile or corrupt length
// prefix. Default 67108864 (64 MiB). Must lie in [1, 4294967295], the range a
// 32-bit length prefix can carry.
#ifndef D_CFG_NET_FRAME_MAX
#   define D_CFG_NET_FRAME_MAX 67108864
#endif  // D_CFG_NET_FRAME_MAX


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if ( (D_CFG_NORM(D_CFG_NET_HOST_MAX) < 64) ||                                 \
      (D_CFG_NORM(D_CFG_NET_HOST_MAX) > 1024) )
#   error "cfg_net: D_CFG_NET_HOST_MAX must lie in [64, 1024]"
#endif  // D_CFG_NET_HOST_MAX

#if ( (D_CFG_NORM(D_CFG_NET_IO_CHUNK) < 256) ||                                \
      (D_CFG_NORM(D_CFG_NET_IO_CHUNK) > 65536) )
#   error "cfg_net: D_CFG_NET_IO_CHUNK must lie in [256, 65536]"
#endif  // D_CFG_NET_IO_CHUNK

#if ( (D_CFG_NORM(D_CFG_NET_FRAME_MAX) < 1) ||                                 \
      (D_CFG_NORM(D_CFG_NET_FRAME_MAX) > 4294967295) )
#   error "cfg_net: D_CFG_NET_FRAME_MAX must lie in [1, 4294967295]"
#endif  // D_CFG_NET_FRAME_MAX


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================
// The effective values the module reads. net/net.h reads these, never the
// D_CFG_NET_* knobs directly, and republishes them as size_t constants.


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_NET_HOST_MAX
//   value: the resolved host capacity of a d_net_endpoint, terminator
// excluded.
#define D_INTERNAL_NET_HOST_MAX  D_CFG_NORM(D_CFG_NET_HOST_MAX)

// 3.1.2
// D_INTERNAL_NET_IO_CHUNK
//   value: the resolved size of the stream algorithms' stack buffer.
#define D_INTERNAL_NET_IO_CHUNK  D_CFG_NORM(D_CFG_NET_IO_CHUNK)

// 3.1.3
// D_INTERNAL_NET_FRAME_MAX
//   value: the resolved default frame ceiling, in bytes.
#define D_INTERNAL_NET_FRAME_MAX D_CFG_NORM(D_CFG_NET_FRAME_MAX)


#endif  // DJINTERP_CONFIG_NET_CFG_NET_H
