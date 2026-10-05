/*******************************************************************************
* djinterp [config]                                                   cfg_http.h
*
* Configuration of HTTP.
*   The limits HTTP/1.1's head parser enforces by default, resolved into
* D_INTERNAL_HTTP_*; a caller can lower or raise them per parse through
* d_http1_options.
*   targets:  net/http/http1.h and its sources ->
*             D_INTERNAL_HTTP_START_LINE_MAX, D_INTERNAL_HTTP_HEAD_MAX
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/net/http/cfg_http.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Head limits
         1.  D_CFG_HTTP_START_LINE_MAX
         2.  D_CFG_HTTP_HEAD_MAX
2.  RESOLVED VALUES
    ---------------
    1.  Read by the module
         1.  D_INTERNAL_HTTP_START_LINE_MAX
         2.  D_INTERNAL_HTTP_HEAD_MAX
*/

#ifndef DJINTERP_CONFIG_NET_HTTP_CFG_HTTP_H
#define DJINTERP_CONFIG_NET_HTTP_CFG_HTTP_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_NORM


//==============================================================================
// 1.  KNOBS
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier wins.


// 1.1    Head limits
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_HTTP_START_LINE_MAX
//   knob: the longest request or status line accepted, CRLF excluded; 64 to
// 1048576. RFC 9112, 3, asks servers to take at least 8000 octets.
#ifndef D_CFG_HTTP_START_LINE_MAX
    #define D_CFG_HTTP_START_LINE_MAX 8192
#endif  // D_CFG_HTTP_START_LINE_MAX

// 1.1.2
// D_CFG_HTTP_HEAD_MAX
//   knob: the longest head accepted, from the start line through the empty
// line that ends it; from D_CFG_HTTP_START_LINE_MAX to 16777216.
#ifndef D_CFG_HTTP_HEAD_MAX
    #define D_CFG_HTTP_HEAD_MAX 65536
#endif  // D_CFG_HTTP_HEAD_MAX

#if ( (D_CFG_NORM(D_CFG_HTTP_START_LINE_MAX) < 64) ||                         \
      (D_CFG_NORM(D_CFG_HTTP_START_LINE_MAX) > 1048576) )
    #error "cfg_http: D_CFG_HTTP_START_LINE_MAX must be from 64 to 1048576"
#endif

#if ( (D_CFG_NORM(D_CFG_HTTP_HEAD_MAX) <                                      \
       D_CFG_NORM(D_CFG_HTTP_START_LINE_MAX)) ||                              \
      (D_CFG_NORM(D_CFG_HTTP_HEAD_MAX) > 16777216) )
    #error "cfg_http: D_CFG_HTTP_HEAD_MAX must be from the line limit to 16 MiB"
#endif


//==============================================================================
// 2.  RESOLVED VALUES
//==============================================================================
// What the module reads; never set by users.


// 2.1    Read by the module
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_HTTP_START_LINE_MAX
//   constant: D_CFG_HTTP_START_LINE_MAX, as d_http1_options_init gives it.
#define D_INTERNAL_HTTP_START_LINE_MAX D_CFG_NORM(D_CFG_HTTP_START_LINE_MAX)

// 2.1.2
// D_INTERNAL_HTTP_HEAD_MAX
//   constant: D_CFG_HTTP_HEAD_MAX, as d_http1_options_init gives it.
#define D_INTERNAL_HTTP_HEAD_MAX D_CFG_NORM(D_CFG_HTTP_HEAD_MAX)


#endif  // DJINTERP_CONFIG_NET_HTTP_CFG_HTTP_H
