/*******************************************************************************
* djinterp [config]                                                    cfg_ftp.h
*
* Configuration of the FTP client.
*   The sizes of the buffers a d_ftp_client holds: the reply text it keeps,
* the control bytes it reads ahead, the longest command line it sends, and
* the chunk a transfer moves per read or write. Each is resolved into
* D_INTERNAL_FTP_*, which ftp_client.h republishes as D_FTP_CLIENT_*.
*   targets:  net/ftp/ftp_client.h -> D_INTERNAL_FTP_REPLY_SIZE,
*             D_INTERNAL_FTP_INPUT_SIZE, D_INTERNAL_FTP_LINE_SIZE,
*             D_INTERNAL_FTP_CHUNK_SIZE
*   requires: cfg_common.h
*   It includes no env header, so the include-order cycle that keeps
* cfg_tcp.h out of dconfig.h does not apply: registered there, it resolves
* cleanly. Registration is the config owner's edit.
*
*
* path:      /inc/djinterp/config/net/ftp/cfg_ftp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Control connection
         1.  D_CFG_FTP_REPLY_SIZE
         2.  D_CFG_FTP_INPUT_SIZE
         3.  D_CFG_FTP_LINE_SIZE
    2.  Transfers
         1.  D_CFG_FTP_CHUNK_SIZE
2.  RESOLVED VALUES
    ---------------
    1.  Read by the module
         1.  D_INTERNAL_FTP_REPLY_SIZE
         2.  D_INTERNAL_FTP_INPUT_SIZE
         3.  D_INTERNAL_FTP_LINE_SIZE
         4.  D_INTERNAL_FTP_CHUNK_SIZE
*/

#ifndef DJINTERP_CONFIG_NET_FTP_CFG_FTP_H
#define DJINTERP_CONFIG_NET_FTP_CFG_FTP_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_NORM


//==============================================================================
// 1.  KNOBS
//==============================================================================
// Each is #ifndef-guarded, so a value defined earlier wins. The bounds keep
// every buffer large enough for the protocol and small enough that the
// transfer chunks, which live on the stack, stay modest.


// 1.1    Control connection
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FTP_REPLY_SIZE
//   knob: bytes kept of a reply's text, its terminator included; 512 to
// 65536. A longer reply -- FEAT, HELP, and STAT run long -- is truncated but
// still parsed whole.
#ifndef D_CFG_FTP_REPLY_SIZE
    #define D_CFG_FTP_REPLY_SIZE 4096
#endif  // D_CFG_FTP_REPLY_SIZE

// 1.1.2
// D_CFG_FTP_INPUT_SIZE
//   knob: bytes read from the control connection ahead of the reply parser;
// 256 to 65536.
#ifndef D_CFG_FTP_INPUT_SIZE
    #define D_CFG_FTP_INPUT_SIZE 2048
#endif  // D_CFG_FTP_INPUT_SIZE

// 1.1.3
// D_CFG_FTP_LINE_SIZE
//   knob: the longest command line the client sends, CR LF and a terminator
// included; 512 to 65536. It bounds the paths a command can name.
#ifndef D_CFG_FTP_LINE_SIZE
    #define D_CFG_FTP_LINE_SIZE 4096
#endif  // D_CFG_FTP_LINE_SIZE

// 1.2    Transfers
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FTP_CHUNK_SIZE
//   knob: bytes a transfer moves per read or write; 512 to 65536. A transfer
// holds two chunks on the stack.
#ifndef D_CFG_FTP_CHUNK_SIZE
    #define D_CFG_FTP_CHUNK_SIZE 8192
#endif  // D_CFG_FTP_CHUNK_SIZE

#if ( (D_CFG_NORM(D_CFG_FTP_REPLY_SIZE) < 512) ||                              \
      (D_CFG_NORM(D_CFG_FTP_REPLY_SIZE) > 65536) )
    #error "cfg_ftp: D_CFG_FTP_REPLY_SIZE must lie in [512, 65536]"
#endif

#if ( (D_CFG_NORM(D_CFG_FTP_INPUT_SIZE) < 256) ||                              \
      (D_CFG_NORM(D_CFG_FTP_INPUT_SIZE) > 65536) )
    #error "cfg_ftp: D_CFG_FTP_INPUT_SIZE must lie in [256, 65536]"
#endif

#if ( (D_CFG_NORM(D_CFG_FTP_LINE_SIZE) < 512) ||                               \
      (D_CFG_NORM(D_CFG_FTP_LINE_SIZE) > 65536) )
    #error "cfg_ftp: D_CFG_FTP_LINE_SIZE must lie in [512, 65536]"
#endif

#if ( (D_CFG_NORM(D_CFG_FTP_CHUNK_SIZE) < 512) ||                              \
      (D_CFG_NORM(D_CFG_FTP_CHUNK_SIZE) > 65536) )
    #error "cfg_ftp: D_CFG_FTP_CHUNK_SIZE must lie in [512, 65536]"
#endif


//==============================================================================
// 2.  RESOLVED VALUES
//==============================================================================
// What ftp_client.h reads. Nothing outside this file resolves them.


// 2.1    Read by the module
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_FTP_REPLY_SIZE
//   resolved: the reply text storage, in bytes.
#define D_INTERNAL_FTP_REPLY_SIZE D_CFG_NORM(D_CFG_FTP_REPLY_SIZE)

// 2.1.2
// D_INTERNAL_FTP_INPUT_SIZE
//   resolved: the control read-ahead, in bytes.
#define D_INTERNAL_FTP_INPUT_SIZE D_CFG_NORM(D_CFG_FTP_INPUT_SIZE)

// 2.1.3
// D_INTERNAL_FTP_LINE_SIZE
//   resolved: the command line buffer, in bytes.
#define D_INTERNAL_FTP_LINE_SIZE  D_CFG_NORM(D_CFG_FTP_LINE_SIZE)

// 2.1.4
// D_INTERNAL_FTP_CHUNK_SIZE
//   resolved: the transfer chunk, in bytes.
#define D_INTERNAL_FTP_CHUNK_SIZE D_CFG_NORM(D_CFG_FTP_CHUNK_SIZE)


#endif  // DJINTERP_CONFIG_NET_FTP_CFG_FTP_H
